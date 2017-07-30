#include "dx_msg.hpp"

#include <iostream>
#include <iomanip>
#include <cstddef>

using std::string;
using std::unique_ptr;
using std::move;

static const char *DUMP_INDENT = "  ";


static void SerializeStr( std::ostream &os, const std::string &val );
static void DumpStr( std::ostream &os, int level, const std::string &val );
static void SerializeInt32( std::ostream &os, int32_t val );
static void DumpInt32( std::ostream &os, int level, int32_t val );


std::string DxValTypeToString( DxValTypeEnum val )
{
    #define entry( V ) case DxValTypeEnum::V : return #V
    switch (val)
    {
        entry( DX_NULL );
        entry( DX_MAP );
        entry( DX_SEQ );
        entry( DX_STR );
        entry( DX_INT16 );
        entry( DX_UINT16 );
        entry( DX_INT32 );
        entry( DX_UINT32 );
        entry( DX_INT64 );
        entry( DX_UINT64 );
        entry( DX_FLT );
        entry( DX_DBL );
        entry( DX_BOOL );
        default:
            return "UNKNOWN";
    }
    #undef entry
}


/* DxVal section */
template<>
unique_ptr<DxVal>
DxVal::MakeVal<nullptr_t>( nullptr_t v )
{
    return unique_ptr<DxVal>( new DxNull() );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<string &&>( string &&v )
{
    return unique_ptr<DxVal>( new DxStr( move(v) ) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<char const *>( char const *v )
{
    return unique_ptr<DxVal>( new DxStr( v ) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<int16_t>( int16_t v )
{
    return unique_ptr<DxVal>( new DxInt16(v) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<uint16_t>( uint16_t v )
{
    return unique_ptr<DxVal>( new DxUint16(v) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<int32_t>( int32_t v )
{
    return unique_ptr<DxVal>( new DxInt32(v) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<uint32_t>( uint32_t v )
{
    return unique_ptr<DxVal>( new DxUint32(v) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<int64_t>( int64_t v )
{
    return unique_ptr<DxVal>( new DxInt64(v) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<uint64_t>( uint64_t v )
{
    return unique_ptr<DxVal>( new DxUint64(v) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<float>( float v )
{
    return unique_ptr<DxVal>( new DxFlt( v ) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<double>( double v )
{
    return unique_ptr<DxVal>( new DxDbl(v) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<bool>( bool v )
{
    return unique_ptr<DxVal>( new DxBool(v) );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<DxVal &&>( DxVal &&v)
{
    return unique_ptr<DxVal>( v.MoveToNew() );
}

template<>
unique_ptr<DxVal>
DxVal::MakeVal<unique_ptr<DxVal>>( unique_ptr<DxVal> v )
{
    return move(v);
}

/* end DxVal section */


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

/* DxBool section */
DxBool *
DxBool::MoveToNew()
{
    return new DxBool(mVal);
}

void
DxBool::Serialize(std::ostream &os) const
{
    os << ((mVal == true) ? DX_TRUE_BYTE : DX_FALSE_BYTE);
}

void
DxBool::Dump(std::ostream &os, int level) const
{
    os << (( mVal == true ) ? "true" : "false");
}

/* end DxBool section */


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
    os  << DX_STR_START
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
static unique_ptr<DxVal> ParseVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<DxMap> ParseMapVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<DxSeq> ParseSeqVal(char const *buf, size_t len, size_t &ind);
static unique_ptr<DxStr> ParseStrVal(char const *buf, size_t len, size_t &ind);


static unique_ptr<DxMap>
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
        unique_ptr<DxVal> k = ParseVal(buf, len, ind);
        unique_ptr<DxVal> v = ParseVal(buf, len, ind);

        switch (k->GetType())
        {
            case DxValTypeEnum::DX_STR:
            {
                unique_ptr<DxStr> kStr( static_cast<DxStr *>(k.release()) );
                rtn->AddKeyVal( kStr->MoveVal(), move(v) );
                break;
            }
            case DxValTypeEnum::DX_INT32:
            {
                unique_ptr<DxInt32> kInt32( static_cast<DxInt32 *>(k.release()) );
                rtn->AddKeyVal( kInt32->GetVal(), move(v) );
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

static unique_ptr<DxSeq>
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
        unique_ptr<DxVal> val = ParseVal(buf, len, ind);
        rtn->AddVal( move(val) );
    }

    ind += 1;  // pass up DX_VAL_END

    return move(rtn);
}

static unique_ptr<DxStr>
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

    unique_ptr<DxStr> rtn( new DxStr( std::string(buf + start, ind - start) ) );

    ind += 1;  // pass up DX_VAL_END

    return rtn;
}

static unique_ptr<DxVal>
ParseVal(char const *buf, size_t len, size_t &ind)
{
    
    if ( ind >= len )
    {
        throw DxParseException("ParseVal: unexpected end of buffer");
    }

    char const valType = buf[ind];

    switch (valType)
    {
        case DX_STR_START:
            return ParseStrVal(buf, len, ++ind);
        case DX_INT16_START:
            return DxInt16::Parse(buf, len, ++ind);
        case DX_UINT16_START:
            return DxUint16::Parse(buf, len, ++ind);
        case DX_INT32_START:
            return DxInt32::Parse(buf, len, ++ind);
        case DX_UINT32_START:
            return DxUint32::Parse(buf, len, ++ind);
        case DX_INT64_START:
            return DxInt64::Parse(buf, len, ++ind);
        case DX_UINT64_START:
            return DxUint64::Parse(buf, len, ++ind);
        case DX_FLT_START:
            return DxFlt::Parse(buf, len, ++ind);
        case DX_DBL_START:
            return DxDbl::Parse(buf, len, ++ind);
        case DX_TRUE_BYTE:
            ++ind;
            return unique_ptr<DxBool>(new DxBool(true));
        case DX_FALSE_BYTE:
            ++ind;
            return unique_ptr<DxBool>(new DxBool(false));
        case DX_MAP_START:
            return ParseMapVal(buf, len, ++ind);
        case DX_SEQ_START:
            return ParseSeqVal(buf, len, ++ind);
        case DX_NULL_BYTE:
            ++ind;
            return unique_ptr<DxVal>(new DxNull);
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

unique_ptr<DxMsg>
ParseDxMsg(char const *buf, size_t len)
{
    size_t ind = 0;

    unique_ptr<DxVal> msgType = ParseVal(buf, len, ind);

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
    
    unique_ptr<DxStr> msgTypeStr(
        static_cast<DxStr *>(msgType.release())
        );

    unique_ptr<DxVal> msgVal = ParseVal(buf, len, ind);

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
