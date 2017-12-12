#include <map>
#include <memory>
#include <vector>
#include <cstring>
#include <sstream>
#include <fstream>

//
#include <iostream>
//

extern "C" {
#include "merc.h"
}


static const size_t MAX_LETTER = 26;

struct DiskIndexMeta
{
    long offHelps;
    long offFirstNode;
};

struct DiskIndexNode
{
    DiskIndexNode() 
        : next{ }
        , datasOff( )
    { }

    long next[MAX_LETTER];
    long datasOff;
};

struct DiskIndexData
{
    long keywordStart;
    long lineStart;
    long wordStart;
};

struct HelpIndexData
{
    HelpIndexData(const char *keyword_, const char *line_) 
        : keyword( keyword_ )
        , lineResult( line_ )
    { }
    std::string keyword;
    std::string lineResult;
};

struct HelpIndexNode    
{
    HelpIndexNode() 
        : next(MAX_LETTER)
        , datas()
    { }

    void Write(std::ofstream &f, long &nextPos);

    std::vector< std::unique_ptr< HelpIndexNode > > next;
    std::vector<DiskIndexData> datas;
};

void HelpIndexNode::Write(std::ofstream &f, long &nextPos)
{
    long myPos = nextPos;
    nextPos += static_cast<long>(sizeof(DiskIndexNode));
    nextPos += static_cast<long>(sizeof(size_t)); // datas count
    nextPos += static_cast<long>(datas.size() * sizeof(DiskIndexData)); // datas

    /* write all children and save their offsets */
    DiskIndexNode childPos;
    for (size_t i = 0; i < next.size(); ++i)
    {
        auto &node = next[i];

        if (node.get() != nullptr)
        {
            childPos.next[i] = nextPos;
            node->Write(f, nextPos);
        }
        else
        {
            childPos.next[i] = 0;
        }
    }

    /* Now actually write everything */
    f.seekp(myPos);

    f.write(reinterpret_cast<char *>(&childPos), sizeof(childPos));

    size_t datasCnt = datas.size();
    f.write(
        reinterpret_cast<char *>(&datasCnt),
        sizeof(datasCnt));

    f.write(
        reinterpret_cast<char *>(&(datas[0])),
        static_cast<long>(datas.size() * sizeof(datas[0])));
}

class HelpIndex
{
public:
    HelpIndex()
    { }
    HelpIndex(const HelpIndex &) = delete;
    HelpIndex operator=(const HelpIndex &) = delete;

    void BuildIndex();

    /* word is the word to search for.
       result is the actual result.
       maxResult limits size of result.

       return value is the total number of results found (may be larger than result.size())
    */
    size_t SearchWord(const char * word, std::vector<HelpIndexData> &result, size_t maxResult);

private:

};

void HelpIndex::BuildIndex()
{
    DiskIndexMeta meta;
    HelpIndexNode first;
    
    long metaOff = 0;
    meta.offHelps = metaOff + static_cast<long>(sizeof(meta));

    std::ofstream f;
    f.open("HelpIndex.bin", std::ios::binary);
    
    f.seekp(meta.offHelps);
    for ( HELP_DATA *pHelp = help_first; pHelp; pHelp = pHelp->next )
    {
        long keywordOff = f.tellp();
        f << pHelp->keyword << "\n";

        HelpIndexNode *curr = &first;
        const char *pC;
        long lineStart = f.tellp();
        bool newLine = true;

        for (pC = pHelp->text; *pC != '\0'; ++pC)
        {
            long currOff = f.tellp();
            if (newLine)
            {
                lineStart = currOff;
                newLine = false;
            }

            char c = *pC;

            f << c;

            if (c == '\n' || c == '\r')
            {
                curr = &first;
                newLine = true;
                continue;
            }
            else if (c == ' ')
            {
                curr = &first;
                continue;
            }
            else if ( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') )
            {
                c = LOWER(c);
                size_t iL = static_cast<size_t>(c - 97);

                if (!curr->next[ iL ])
                {
                    curr->next[ iL ] = std::unique_ptr<HelpIndexNode>( new HelpIndexNode );
                    curr = curr->next[ iL ].get();
                }
                else
                {
                    curr = curr->next[ iL ].get();
                }
                
                curr->datas.push_back({keywordOff, lineStart, currOff});
            }
            else
            {
                // ignore non alpha
                continue;
            }
        }

        f << '\n'; // Force newline between helps
    }
    
    meta.offFirstNode = f.tellp();
    long nextOff = meta.offFirstNode;
    first.Write(f, nextOff);

    f.seekp(0);
    f.write(reinterpret_cast<char *>(&meta), sizeof(meta));
}


static size_t checkNode(std::ifstream &f, long maxOff, const char *chars, std::vector<HelpIndexData> &result, size_t maxResult)
{
    // node is current node (root node for first call)
    // chars is the remaining search string, full string for first call
    char c = *chars;

    if (c == '\0')
    {
        // match found
        f.seekg( sizeof(DiskIndexNode), std::ios_base::cur );

        size_t datasCnt;
        f.read(reinterpret_cast<char *>(&datasCnt), sizeof(size_t));

        size_t resultCnt = UMIN(datasCnt, maxResult);
        std::vector<DiskIndexData> datas(resultCnt);

        f.read(
            reinterpret_cast<char *>(&(datas[0])),
            static_cast<long>(resultCnt * sizeof(datas[0])));

        result.clear();

        for (auto d : datas)
        {
            char kwBuf[MSL];
            char lineBuf[MSL];
            
            f.seekg(d.keywordStart);
            f.getline(kwBuf, sizeof(kwBuf));

            f.seekg(d.lineStart);
            f.getline(lineBuf, sizeof(lineBuf));

            result.emplace_back(kwBuf, lineBuf);
        }
        return datasCnt; 
    }

    if (c < 'A' || c > 'z' || (c > 'Z' && c < 'a'))
    {
        // skip non alpha chars in search string
        return checkNode(f, maxOff, chars + 1, result, maxResult);
    }

    c = LOWER(c);
    size_t iL = static_cast<size_t>(c - 97);

    f.seekg( static_cast<long>(iL * sizeof(long)), std::ios_base::cur);

    long nextOff;

    f.read(reinterpret_cast<char *>(&nextOff), sizeof(long));

    if (nextOff == 0)
    {
        // no match
        return 0;
    }
    else
    {
        f.seekg(nextOff);
        return checkNode(f, maxOff, chars + 1, result, maxResult);
    }
}

size_t HelpIndex::SearchWord(const char *word, std::vector<HelpIndexData> &result, size_t maxResult)
{
    std::ifstream indexFile;
    indexFile.open("HelpIndex.bin", std::ios::binary | std::ios::ate);
    long maxOff = indexFile.tellg();

    DiskIndexMeta meta;
    indexFile.seekg(0);
    indexFile.read(reinterpret_cast<char *>(&meta), sizeof(meta));

    indexFile.seekg(meta.offFirstNode);
    
    return checkNode(indexFile, maxOff, word, result, maxResult);
}

static HelpIndex sIndex;


void build_help_index(void )
{
    sIndex.BuildIndex();
}


void help_search(CHAR_DATA *ch, const char *argument)
{
    if (argument[0] == '\0')
    {
        ptc(ch, "Must provide a word to search for.\n\r");
        return;
    }

    char word[MIL];
    argument = one_argument(argument, word);

    if (argument[0] != '\0')
    {
        ptc(ch, "You can only search for a single word.\n\r");
        return;
    }

    std::vector<HelpIndexData> result;
    size_t resultCnt = sIndex.SearchWord(word, result, 100);

    if (resultCnt == 0)
    {
        send_to_char("No results found.\n\r", ch);
    }
    else
    {
        ptc(ch, "%d results found. ", resultCnt);
        if (resultCnt > result.size())
        {
            ptc(ch, "Showing first %d.", result.size());
        }
        send_to_char("\n\r", ch);

        for (auto data : result)
        {
            ptc(ch, "%-20s - %s{x\n\r", data.keyword.c_str(), data.lineResult.c_str());
        }    
    }
}