#include "dx_msg.hpp"

#include <iostream>
#include <iomanip>


using std::unique_ptr;
using std::move;

static const char *DUMP_INDENT = "  ";


static unique_ptr<const DxVal> ParseVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxValMap> ParseMapVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxValSeq> ParseSeqVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxValDbl> ParseDblVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxValInt32> ParseInt32Val(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxValString> ParseStringVal(char const *buf, size_t len, size_t &ind);


// static void DxByteToString(std::string &outString, char const b)
// {
//     switch (b)
//     {
//         case DX_MSG_START:
//             outString = "DX_MSG_START";
//             break;
//         case DX_MAP_START:
//             outString = "DX_MAP_START";
//             break;
//         case DX_SEQ_START:
//             outString = "DX_SEQ_START";
//             break;
//         case DX_STRING_START:
//             outString = "DX_STRING_START";
//             break;
//         case DX_INT32_START:
//             outString = "DX_INT32_START";
//             break;
//         case DX_VAL_END:
//             outString = "DX_VAL_END";
//             break;
//         default:
//         {
//             char buf[4];
//             unsigned int uintVal = static_cast<unsigned int>(static_cast<unsigned char>(b));
//             ::snprintf(buf, sizeof(buf), "%u", uintVal);
//             outString = &buf[0];
//             break;
//         }
//     }
// }

static void DxValTypeToString(std::string &outString, DxValTypeEnum val)
{
    switch (val)
    {
        case DX_MAP:
            outString = "DX_MAP";
            break;
        case DX_SEQ:
            outString = "DX_SEQ";
            break;
        case DX_INT32:
            outString = "DX_INT32";
            break;
        case DX_STRING:
            outString = "DX_STRING";
            break;
        default:
            outString = "UNKNOWN";
            break;
    }
}

/* DxValNull section */
void
DxValNull::Serialize(std::ostream &os) const
{
    os << DX_NULL_BYTE;
}

void
DxValNull::Dump(std::ostream &os, int level) const
{
    os << "NULL";
}
/* end DxValNull section */

/* DxValMap section */

void
DxValMap::AddKeyVal(unique_ptr<const DxVal> k, unique_ptr<const DxVal> v)
{
    mVal.push_back({
        move(k), 
        move(v)
    });
}

void
DxValMap::AddKeyVal( const DxVal *k, const DxVal *v )
{
    mVal.push_back({
        unique_ptr<const DxVal>(k),
        unique_ptr<const DxVal>(v)
    });
}

void
DxValMap::Serialize(std::ostream &os) const
{
    os << DX_MAP_START;
    for ( std::vector<kv>::const_iterator itr = mVal.begin() ; itr != mVal.end() ; ++itr )
    {
        itr->key->Serialize(os);
        itr->val->Serialize(os);
    }
    os << DX_VAL_END;
}

void
DxValMap::Dump(std::ostream &os, int level ) const
{
    os << "{" << std::endl;

    for ( std::vector<kv>::const_iterator itr = mVal.begin() ; itr != mVal.end() ; ++itr )
    {
        for (int i = 0; i <= level; ++i)
        {
            os << DUMP_INDENT;
        }
        itr->key->Dump(os, level + 1);
        os << " : ";
        itr->val->Dump(os, level + 1);
        os << "," << std::endl;
    }
    
    for (int i = 0; i < level; ++i)
    {
        os << DUMP_INDENT;
    }

    os << "}";
}
/* end DxValMap section */

/* DxValSeq section */
void
DxValSeq::AddVal( unique_ptr<const DxVal> val )
{
    mVal.push_back( move(val) );
}

void
DxValSeq::AddVal( const DxVal *val )
{
    mVal.push_back( unique_ptr<const DxVal>( val ) );
}

void
DxValSeq::Serialize(std::ostream &os) const
{
    os << DX_SEQ_START;
    for ( std::vector<unique_ptr<const DxVal>>::const_iterator itr = mVal.begin() ; 
          itr != mVal.end() ; ++itr )
    {
        (*itr)->Serialize(os);
    }
    os << DX_VAL_END;
}

void
DxValSeq::Dump( std::ostream &os, int level ) const
{
    os << "[" << std::endl;
 
    for ( std::vector<unique_ptr<const DxVal>>::const_iterator itr = mVal.begin() ; 
          itr != mVal.end() ; ++itr )
    {
        for (int i = 0; i <= level; ++i)
        {
            os << DUMP_INDENT;
        }

        (*itr)->Dump(os, level + 1);
        os << "," << std::endl;
    }

    for (int i = 0; i < level; ++i)
    {
        os << DUMP_INDENT;
    }
    os << "]";
}
/* end DxValSeq section */

/* DxValString section */
void
DxValString::Serialize(std::ostream &os) const
{
    os  << DX_STRING_START
        << mVal
        << DX_VAL_END;
}

void
DxValString::Dump(std::ostream &os, int level) const
{
    os  << "\"" 
        << mVal 
        << "\"";
}
/* end DxValString section */

/* DxValInt32 section */

void
DxValInt32::Serialize(std::ostream &os) const
{
    char const * bytes = static_cast<char const *>(static_cast<void const *>(&mVal));

    os  << DX_INT32_START
        << bytes[0]
        << bytes[1]
        << bytes[2]
        << bytes[3];
}

void
DxValInt32::Dump(std::ostream &os, int level) const
{
    os << mVal;
}

/* end DxValInt32 section */

/* DxValDbl section */
void
DxValDbl::Serialize(std::ostream &os) const
{
    static_assert(sizeof(mVal) == 8, "");

    char const * bytes = static_cast<char const *>(static_cast<void const *>(&mVal));

    os  << DX_DBL_START
        << bytes[0]
        << bytes[1]
        << bytes[2]
        << bytes[3]
        << bytes[4]
        << bytes[5]
        << bytes[6]
        << bytes[7];
}

void
DxValDbl::Dump(std::ostream &os, int level) const
{
    os << mVal;
}

/* end DxValDbl section */


/* DxMsg section */
void
DxMsg::Serialize(std::ostream &os) const
{
    mMsgType->Serialize( os );
    mMsgVal->Serialize( os );
}

void
DxMsg::Dump(std::ostream &os) const
{
    os  << "MsgType: ";
    mMsgType->Dump( os, 0 );
    os  << std::endl
        << "MsgVal: " << std::endl;
    mMsgVal->Dump( os, 0 );
    os  << std::endl;
}

/* end DxMsg section */

static unique_ptr<const DxValMap>
ParseMapVal(char const *buf, size_t len, size_t &ind)
{
    unique_ptr<DxValMap> rtn( new DxValMap );

    while (true)
    {
        if (ind >= len)
        {
            throw DxParseException("ParseMapVal: unexpected end of buffer");
        }
        if (buf[ind] == DX_VAL_END)
        {
            // end of map
            break;
        }
        unique_ptr<const DxVal> k = ParseVal(buf, len, ind);
        unique_ptr<const DxVal> v = ParseVal(buf, len, ind);

        rtn->AddKeyVal(
            move(k), 
            move(v));
    }

    ind += 1;  // pass up DX_VAL_END

    return move(rtn);
}

static unique_ptr<const DxValSeq>
ParseSeqVal(char const *buf, size_t len, size_t &ind)
{
    unique_ptr<DxValSeq> rtn( new DxValSeq );

    while (true)
    {
        if (ind >= len)
        {
            throw DxParseException("ParseSeqVal: unexpected end of buffer");
        }
        if (buf[ind] == DX_VAL_END)
        {
            // end of seq
            break; 
        }
        unique_ptr<const DxVal> val = ParseVal(buf, len, ind);
        rtn->AddVal( move(val) );
    }

    ind += 1;  // pass up DX_VAL_END

    return move(rtn);
}

static unique_ptr<const DxValString>
ParseStringVal(char const *buf, size_t len, size_t &ind)
{
    size_t start = ind;

    while (true)
    {
        if (ind >= len)
        {
            throw DxParseException("ParseStringVal: unexpected end of buffer");
        }
        if (buf[ind] == DX_VAL_END)
        {
            // end of string
            break;
        }
        ++ind;
    }

    unique_ptr<const DxValString> rtn( new DxValString( buf, start, ind - start ) );

    ind += 1;  // pass up DX_VAL_END

    return rtn;
}

static unique_ptr<const DxValInt32>
ParseInt32Val(char const *buf, size_t len, size_t &ind)
{
    static_assert(sizeof(int32_t) == 4, "");

    if ( (len - ind) < 4 )
    {
        std::ostringstream os;
        os << "ParseInt32Val: only " << (len - ind) << " bytes available, 4 needed";
        std::string msg = os.str();
        throw DxParseException(msg);
    }

    int32_t val = *(static_cast<const int32_t *>(static_cast<const void *>(&buf[ind])));

    ind += 4;

    return unique_ptr<DxValInt32>( new DxValInt32(val) );
}

static unique_ptr<const DxValDbl>
ParseDblVal(char const *buf, size_t len, size_t &ind)
{
    static_assert(sizeof(double) == 8, "");

    if ( (len - ind) < 8 )
    {
        std::ostringstream os;
        os << "ParseDblVal: only " << (len - ind) << " bytes available, 8 needed";
        std::string msg = os.str();
        throw DxParseException(msg);
    }

    double val = *(static_cast<const double *>(static_cast<const void *>(&buf[ind])));

    ind += 8;

    return unique_ptr<const DxValDbl>( new DxValDbl(val) );
}

static unique_ptr<const DxVal>
ParseVal(char const *buf, size_t len, size_t &ind)
{
    
    if ( ind >= len )
    {
        throw DxParseException("ParseVal: unexpected end of buffer");
    }

    char const valType = buf[ind];

    switch (valType)
    {
        case DX_STRING_START:
        {
            unique_ptr<const DxValString> rtn = ParseStringVal(buf, len, ++ind);
            return move(rtn);
        }
        case DX_INT32_START:
        {
            unique_ptr<const DxValInt32> rtn = ParseInt32Val(buf, len, ++ind);
            return move(rtn);
        }
        case DX_DBL_START:
            return ParseDblVal(buf, len, ++ind);
        case DX_MAP_START:
            return ParseMapVal(buf, len, ++ind);
        case DX_SEQ_START:
            return ParseSeqVal(buf, len, ++ind);
        case DX_NULL_BYTE:
            ++ind;
            return unique_ptr<const DxVal>(new DxValNull);
        default:
        {
            unsigned int uintVal = static_cast<unsigned int>(static_cast<unsigned char>(valType));
            std::ostringstream os;
            os << "ParseVal: Invalid valType " << uintVal;
            std::string msg = os.str();
            
            throw DxParseException(msg);            
        }
    }
}

unique_ptr<const DxMsg>
ParseDxMsg(char const *buf, size_t len)
{
    size_t ind = 0;

    unique_ptr<const DxVal> msgType = ParseVal(buf, len, ind);

    if (msgType->GetType() != DX_STRING)
    {
        std::string actual;
        DxValTypeToString(actual, msgType->GetType());
        std::string expected;
        DxValTypeToString(expected, DX_STRING);

        std::ostringstream os;
        os  << "Message type value was " << actual
            << " insead of " << expected;

        std::string msg = os.str();

        throw DxParseException(msg);
    }
    // unique_ptr<DxValString> msgTypeStr = std::dynamic_pointer_cast<DxValString>(msgType);
    unique_ptr<const DxValString> msgTypeStr(
        reinterpret_cast<const DxValString *>(msgType.release())
        );

    unique_ptr<const DxVal> msgVal = ParseVal(buf, len, ind);

    if (ind < len)
    {
        std::ostringstream os;
        os  << (len - ind) << " extra bytes not parsed. "
            << "Message type: " << msgTypeStr->GetVal();

        std::string msg = os.str();

        throw DxParseException(msg);
    }

    return unique_ptr<DxMsg>( 
        new DxMsg(
            move(msgTypeStr), 
            move(msgVal)));
    //return std::make_shared<DxMsg>(msgTypeStr, msgVal);
}
