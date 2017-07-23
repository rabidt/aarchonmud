#include "dx_msg.hpp"

#include <iostream>
#include <iomanip>


using std::unique_ptr;
using std::move;

static const char *DUMP_INDENT = "  ";


static void SerializeStr( std::ostream &os, const std::string &val );
static void DumpStr( std::ostream &os, int level, const std::string &val );
static void SerializeInt32( std::ostream &os, int32_t val );
static void DumpInt32( std::ostream &os, int level, int32_t val );


static std::string DxValTypeToString( DxValTypeEnum val )
{
    switch (val)
    {
        case DxValTypeEnum::DX_NULL:
            return "DX_NULL";
        case DxValTypeEnum::DX_MAP:
            return "DX_MAP";
        case DxValTypeEnum::DX_SEQ:
            return "DX_SEQ";
        case DxValTypeEnum::DX_STR:
            return "DX_STR";
        case DxValTypeEnum::DX_INT32:
            return "DX_INT32";
        case DxValTypeEnum::DX_DBL:
            return "DX_DBL";
        default:
            return "UNKNOWN";
    }
}


/* DxNull section */
DxNull *
DxNull::MoveToNew()
{
    return new DxNull;
}

void
DxNull::Serialize(std::ostream &os) const
{
    os << DX_NULL_BYTE;
}

void
DxNull::Dump(std::ostream &os, int level) const
{
    os << "NULL";
}
/* end DxNull section */

/* DxMap section */
DxMap *
DxMap::MoveToNew()
{
    return new DxMap(std::move(*this));
}

void
DxMap::AddKeyVal( DxInt32 &&key, DxVal &&val )
{
    mIntMap[ key.GetVal() ] = unique_ptr<const DxVal>( val.MoveToNew() );
}

void
DxMap::AddKeyVal( unique_ptr<const DxInt32> key, unique_ptr<const DxVal> val )
{
    mIntMap[ key->GetVal() ] = move(val);
}

void
DxMap::AddKeyVal( DxStr &&key, DxVal &&val )
{
    mStrMap[ key.MoveVal() ] = unique_ptr<const DxVal>( val.MoveToNew() );
}

void
DxMap::AddKeyVal( unique_ptr<const DxStr> key, unique_ptr<const DxVal> val )
{
    mStrMap[ key->GetVal() ] = move(val);
}

void
DxMap::Serialize(std::ostream &os) const
{
    os << DX_MAP_START;
    for ( auto itr = mIntMap.begin() ; itr != mIntMap.end() ; ++itr )
    {      
        SerializeInt32( os, itr->first );
        itr->second->Serialize(os);
    }
    for ( auto itr = mStrMap.begin() ; itr != mStrMap.end() ; ++itr )
    {
        SerializeStr( os, itr->first );
        itr->second->Serialize(os);
    }
    os << DX_VAL_END;
}

void
DxMap::Dump(std::ostream &os, int level ) const
{
    os << "{" << std::endl;

    for ( auto itr = mIntMap.begin() ; itr != mIntMap.end() ; ++itr )
    {
        for ( int i = 0; i <= level; ++i )
        {
            os << DUMP_INDENT;
        }
        DumpInt32( os, level + 1, itr->first );
        os << " : ";
        itr->second->Dump( os, level + 1 );
        os << "," << std::endl;
    }

    for ( auto itr = mStrMap.begin() ; itr != mStrMap.end() ; ++itr )
    {
        for ( int i = 0; i <= level; ++i )
        {
            os << DUMP_INDENT;
        }
        DumpStr( os, level + 1, itr->first );
        os << " : ";
        itr->second->Dump( os, level + 1 );
        os << "," << std::endl;
    }
    
    for (int i = 0; i < level; ++i)
    {
        os << DUMP_INDENT;
    }

    os << "}";
}
/* end DxMap section */

/* DxSeq section */
void
DxSeq::AddVal( DxVal &&val )
{
    mVal.push_back( std::unique_ptr<DxVal>( val.MoveToNew() ) );
}

void
DxSeq::AddVal( std::unique_ptr<const DxVal> val )
{
    mVal.push_back( move(val) );
}

DxSeq *
DxSeq::MoveToNew()
{
    return new DxSeq(std::move(*this));
}

void
DxSeq::Serialize(std::ostream &os) const
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
DxSeq::Dump( std::ostream &os, int level ) const
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
/* end DxSeq section */

/* DxStr section */
DxStr *
DxStr::MoveToNew()
{
    return new DxStr(std::move(*this));
}

void
DxStr::Serialize(std::ostream &os) const
{
    SerializeStr(os, mVal);   
}

void
DxStr::Dump(std::ostream &os, int level) const
{
    DumpStr( os, level, mVal );
}
/* end DxStr section */

/* DxInt32 section */
DxInt32 *
DxInt32::MoveToNew()
{
    return new DxInt32(mVal);
}

void
DxInt32::Serialize(std::ostream &os) const
{
    SerializeInt32( os, mVal );
}

void
DxInt32::Dump(std::ostream &os, int level) const
{
    DumpInt32(os, level, mVal);
}

/* end DxInt32 section */

/* DxDbl section */
DxDbl *
DxDbl::MoveToNew()
{
    return new DxDbl(std::move(*this));
}

void
DxDbl::Serialize(std::ostream &os) const
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
DxDbl::Dump(std::ostream &os, int level) const
{
    os << mVal;
}

/* end DxDbl section */


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


static void SerializeStr( std::ostream &os, const std::string &val )
{
    os  << DX_STRING_START
        << val
        << DX_VAL_END;
}


static void DumpStr( std::ostream &os, int level, const std::string &val )
{
    os  << "\"" 
        << val 
        << "\"";
}

static void SerializeInt32( std::ostream &os, int32_t val )
{
    char const * bytes = static_cast<char const *>(static_cast<void const *>(&val));

    os  << DX_INT32_START
        << bytes[0]
        << bytes[1]
        << bytes[2]
        << bytes[3];
}

static void DumpInt32( std::ostream &os, int level, int32_t val )
{
    os << val;
}

/* Parse section */
static unique_ptr<const DxVal> ParseVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxMap> ParseMapVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxSeq> ParseSeqVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxDbl> ParseDblVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxInt32> ParseInt32Val(char const *buf, size_t len, size_t &ind);
static unique_ptr<const DxStr> ParseStrVal(char const *buf, size_t len, size_t &ind);

static unique_ptr<const DxMap>
ParseMapVal(char const *buf, size_t len, size_t &ind)
{
    unique_ptr<DxMap> rtn( new DxMap );

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

        switch (k->GetType())
        {
            case DxValTypeEnum::DX_STR:
            {
                unique_ptr<const DxStr> kStr( static_cast<const DxStr *>(k.release()) );
                rtn->AddKeyVal( move(kStr), move(v) );
                break;
            }
            case DxValTypeEnum::DX_INT32:
            {
                unique_ptr<const DxInt32> kInt32( static_cast<const DxInt32 *>(k.release()) );
                rtn->AddKeyVal( move(kInt32), move(v) );
                break;
            }
            default:
                throw DxParseException(
                    "ParseMapVal: key must be DX_STR or DX_INT32, got " + 
                     DxValTypeToString(k->GetType())
                 );
        }
    }

    ind += 1;  // pass up DX_VAL_END

    return move(rtn);
}

static unique_ptr<const DxSeq>
ParseSeqVal(char const *buf, size_t len, size_t &ind)
{
    unique_ptr<DxSeq> rtn( new DxSeq );

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

static unique_ptr<const DxStr>
ParseStrVal(char const *buf, size_t len, size_t &ind)
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

    unique_ptr<const DxStr> rtn( new DxStr( std::string(buf + start, ind - start) ) );

    ind += 1;  // pass up DX_VAL_END

    return rtn;
}

static unique_ptr<const DxInt32>
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

    return unique_ptr<DxInt32>( new DxInt32(val) );
}

static unique_ptr<const DxDbl>
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

    return unique_ptr<const DxDbl>( new DxDbl(val) );
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
            unique_ptr<const DxStr> rtn = ParseStrVal(buf, len, ++ind);
            return move(rtn);
        }
        case DX_INT32_START:
        {
            unique_ptr<const DxInt32> rtn = ParseInt32Val(buf, len, ++ind);
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
            return unique_ptr<const DxVal>(new DxNull);
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

    if (msgType->GetType() != DxValTypeEnum::DX_STR)
    {
        std::string actual = DxValTypeToString(msgType->GetType());
        std::string expected = DxValTypeToString(DxValTypeEnum::DX_STR);

        std::ostringstream os;
        os  << "Message type value was " << actual
            << " insead of " << expected;

        std::string msg = os.str();

        throw DxParseException(msg);
    }
    
    unique_ptr<const DxStr> msgTypeStr(
        static_cast<const DxStr *>(msgType.release())
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
}

// end Parse section
