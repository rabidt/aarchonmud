/**************************************************************
                         playback.cpp

Written by Vodur <dr.vodur@gmail.com>
for Aarchon MUD
(aarchonmud.com:7000).
**************************************************************/
#include <string>
#include <list>
#include <cstring>

#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

extern "C" {
#include "merc.h"
DECLARE_DO_FUN(do_playback);
}

typedef struct pers_comm_history PERS_HISTORY;

struct pers_comm_history
{
    pers_comm_history()
        : entries()
    { }
    std::list< std::string > entries;
};
    
struct COMM_ENTRY
{
    COMM_ENTRY(
        const char *timestamp_,
        const sh_int &channel_,
        const char *text_,
        const char *name_)
        : timestamp(timestamp_)
        , channel(channel_)
        , text(text_)
        , name(name_)
    { }
    std::string timestamp;
    sh_int channel;
    std::string text;
    std::string name;
};

typedef std::list< COMM_ENTRY > COMM_HISTORY;

/* local functions */
static void playback_pers( CHAR_DATA *ch, const PERS_HISTORY *history, unsigned entries );
static void playback_to_char( CHAR_DATA *ch, const COMM_HISTORY *history, unsigned entries );
static void playback_clear( COMM_HISTORY *history );

#define MAX_COMM_HISTORY 300
/* Default number of results, needs to  be <=MAX_COMM_HISTORY */
#define DEFAULT_RESULTS 30

#define MAX_PERS_HISTORY 100 
#define DEFAULT_PERS 30

/* declare the actual structures we will use*/
static COMM_HISTORY public_history;
static COMM_HISTORY immtalk_history;
static COMM_HISTORY savant_history;

PERS_HISTORY *pers_history_new( void )
{
    return new PERS_HISTORY;
}

void pers_history_free(PERS_HISTORY *history)
{
    delete history;
}

void log_pers( PERS_HISTORY *history, const char *text )
{
    char time[MSL];
    char buf[MSL*2];
    
    strlcpy(time, ctime( &current_time ), sizeof(time) );
    time[strlen(time)-1] = '\0';
    snprintf( buf, sizeof(buf), "%s::%s", time, text );
    history->entries.emplace_back(buf);

    while (history->entries.size() > MAX_PERS_HISTORY)
    {
        history->entries.pop_front();
    }
}

void log_chan( CHAR_DATA * ch, const char *text , sh_int channel )
{
    char time[MSL];

    strlcpy(time, ctime( &current_time ), sizeof(time) );
    time[strlen(time)-1] = '\0';

    const char *name;

    if ( ch == NULL )
    {
        name = "";
    }
    else if ( IS_NPC(ch) )
    {
        name = ch->short_descr;
    }
    else
    {
        name = ch->name;
    }

    /* Assign the correct history based on which channel.
    All public channels using public_history, immtalk uses
    immtalk history, etc. */
    COMM_HISTORY * history;
    
    if (channel == cn_immtalk)
        history = &immtalk_history;
    else if (channel == cn_savantalk)
        history = &savant_history;
    else
        history = &public_history;
    
    history->emplace_back(time, channel, text, name);

    while (history->size() > MAX_COMM_HISTORY)
    {
        history->pop_front();
    }
}

DEF_DO_FUN(do_playback)
{
    
    if ( IS_NPC(ch) )
        return;
    
    char arg[MSL];
    sh_int arg_number;
    bool immortal = IS_IMMORTAL(ch);
    bool savant= ( ch->level >= SAVANT );

    argument = one_argument(argument,arg);
    
    if ( arg[0] == '\0')
    {
        send_to_char("playback [public|tell|gtell|clan|info", ch);
        if ( immortal )
            send_to_char("|imm", ch);
        if ( savant )
            send_to_char("|savant", ch);
        send_to_char("] <#>\n\r", ch);
        return;
    }

    COMM_HISTORY *history = NULL;
    PERS_HISTORY *phistory = NULL;
    
    if ( arg[0] == 'p' )
    {
        history = &public_history;
    }
    else if ( arg[0] == 't' )
    {
        phistory = ch->pcdata->tell_history;
        ch->pcdata->new_tells = FALSE;
    }
    else if ( arg[0] == 'g' )
    {
        phistory = ch->pcdata->gtell_history;
    }
    else if ( arg[0] == 'c' )
    {
        phistory = ch->pcdata->clan_history;
    }
    else if ( immortal && arg[0] == 'i' )
    {
        history = &immtalk_history;
    }
    else if ( savant && arg[0] == 's' )
    {
        history = &savant_history;
    }
    else
    {
        send_to_char("Invalid syntax.\n\r", ch);
        return;
    }
    
    /* if we're here, it's a COMM_HISTORY, not PERS_HISTORY, check for 2nd arg */
    argument = one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        if (phistory)
        {
            playback_pers( ch, phistory, ch->lines > 0 ? static_cast<unsigned>(ch->lines) : DEFAULT_PERS );
            return;
        }
        else
        {
            playback_to_char( ch, history, ch->lines > 0 ? static_cast<unsigned>(ch->lines) : DEFAULT_RESULTS );
            return;
        }
    }
    if (is_number(arg))
    {
        arg_number = atoi(arg);
        if (arg_number > MAX_COMM_HISTORY || arg_number < 1)
        {
            printf_to_char(ch, "Argument should be a number from 1 to %d.\n\r",
                    phistory ? MAX_PERS_HISTORY : MAX_COMM_HISTORY);
            return;
        }
        else
        {
            if (phistory)
            {
                playback_pers( ch, phistory, static_cast<unsigned>(arg_number) );
                return;
            }
            else
            {
                playback_to_char( ch, history, static_cast<unsigned>(arg_number) );
                return;
            }
        }
    }
    else if (!strcmp(arg, "clear") && immortal && history )
    {
        playback_clear( history );
        return;
    }
    else
    {
        send_to_char("Invalid syntax.\n\r",ch);
        return;
    }
}

static void playback_clear( COMM_HISTORY *history)
{
    history->clear();
}

static void playback_to_char( CHAR_DATA *ch, const COMM_HISTORY *history, unsigned entries )
{
    if (history == NULL)
    {
        bugf("NULL history passed to playback_to_char.");
        return;
    }

    if (entries < 1)
    {
        return;
    }

    char buf[2*MSL];
    BUFFER *output = new_buf();

    auto position = history->size();

    for (const auto &entry : *history)
    {
        if (position-- > static_cast<size_t>(entries))
        {
            continue;
        }

        if (IS_CHAN_OFF(ch, entry.channel))
        {
            continue;
        }

        snprintf( buf, sizeof(buf),"%s::{%c%s%s", 
                    entry.timestamp.c_str(),
                    public_channel_table[entry.channel].prime_color,
                    entry.name.c_str(),
                    entry.text.c_str());

        add_buf(output, buf);
        add_buf(output, "{x\n\r");
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);
}

static void playback_pers( CHAR_DATA *ch, const PERS_HISTORY *history, unsigned entries)
{
    if (history == NULL)
    {   
        bugf("NULL history passed to playback_pers.");
        return;
    }

    auto position = history->entries.size();

    for (const auto &entry : history->entries)
    {
        if (position-- > entries)
        {
            continue;
        }

        send_to_char(entry.c_str(), ch);
    }
}

static void save_comm_history(const COMM_HISTORY &history, const char * const filename)
{
    FILE *fp = ::fopen(filename, "w");

    if (!fp)
    {
        bugf("%s: couldn't open %s", __func__, filename);
        return;
    }

    char fBuf[1024];

    rapidjson::FileWriteStream os(fp, fBuf, sizeof(fBuf));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);

    writer.StartArray();

    for (const auto &entry : history)
    {
        writer.StartObject();

        writer.Key("timestamp");
        writer.String(entry.timestamp.c_str());
        writer.Key("channel");
        writer.Int(entry.channel);
        writer.Key("text");
        writer.String(entry.text.c_str());
        writer.Key("name");
        writer.String(entry.name.c_str());

        writer.EndObject();
    }

    writer.EndArray();

    fclose(fp);
}

void save_comm_histories( void )
{
    PERF_PROF_ENTER( pr_, "save_comm_histories" );
    save_comm_history(public_history, "playback_public.json");
    save_comm_history(savant_history, "playback_savant.json");
    save_comm_history(immtalk_history, "playback_immtalk.json");
    PERF_PROF_EXIT( pr_ );
}

static void load_comm_history(COMM_HISTORY &history, const char * const filename)
{
    FILE *fp = ::fopen(filename, "r");

    if (!fp)
    {
        bugf("%s: couldn't open %s", __func__, filename);
        return;
    }

    char fBuf[1024];
    rapidjson::FileReadStream is(fp, fBuf, sizeof(fBuf));

    rapidjson::Document doc;
    doc.ParseStream(is);

    fclose(fp);

    for (rapidjson::SizeType i = 0; i < doc.Size(); ++i)
    {
        rapidjson::Value &val = doc[i];

        history.emplace_back(
            val["timestamp"].GetString(),
            val["channel"].GetInt(),
            val["text"].GetString(),
            val["name"].GetString());
    }
}

void load_comm_histories( void )
{
    load_comm_history(public_history, "playback_public.json");
    load_comm_history(savant_history, "playback_savant.json");
    load_comm_history(immtalk_history, "playback_immtalk.json");
}
