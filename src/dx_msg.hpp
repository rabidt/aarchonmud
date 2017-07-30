#ifndef DX_MSG_HPP_
#define DX_MSG_HPP_


#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <exception>
#include <map>


const char DX_MSG_START = '\x1';
const char DX_VAL_END = '\x2';
const char DX_MAP_START = '\x3';
const char DX_SEQ_START = '\x4';
const char DX_STR_START = '\x5';
const char DX_INT16_START = '\x6';
const char DX_UINT16_START ='\x7';
const char DX_INT32_START = '\x8';
const char DX_UINT32_START = '\x9';
const char DX_INT64_START = '\x10';
const char DX_UINT64_START = '\x11';
const char DX_FLT_START = '\x12';
const char DX_DBL_START = '\x13';
const char DX_NULL_BYTE = '\x14';
const char DX_TRUE_BYTE = '\x15';
const char DX_FALSE_BYTE = '\x16';


enum class DxValTypeEnum
{
    UNKNOWN = 0,
    DX_NULL,
    DX_MAP,
    DX_SEQ,
    DX_STR,
    DX_INT16,
    DX_UINT16,
    DX_INT32,
    DX_UINT32,
    DX_INT64,
    DX_UINT64,
    DX_FLT,
    DX_DBL,
    DX_BOOL
};

std::string DxValTypeToString( DxValTypeEnum val );


class DxVal
{
public:
    virtual ~DxVal( ) { };

    virtual DxValTypeEnum GetType( ) const = 0;
    virtual void Serialize( std::ostream &os ) const = 0;
    virtual void Dump( std::ostream &os, int level ) const = 0;

    virtual DxVal *MoveToNew() = 0;

    template< typename T >
    static std::unique_ptr<DxVal> MakeVal( T v );
};


class DxNull : public DxVal
{
public:
    DxValTypeEnum GetType( ) const override { return DxValTypeEnum::DX_NULL; }

    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override;

    DxNull *MoveToNew() override;
};

template <
    typename Tscalar, 
    const size_t cExpectedSize_, 
    const char cStartByte_, 
    const DxValTypeEnum cType_>
class DxScalar : public DxVal
{
    static_assert( sizeof(Tscalar) == cExpectedSize_, "");

public:
    explicit DxScalar( Tscalar val )
        : mVal( val )
    { }

    static const size_t sTypeSize = sizeof(Tscalar);

    DxValTypeEnum GetType( ) const override { return cType_; }

    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override 
    { 
        os << "<" << DxValTypeToString( cType_ ) << ">"
           << mVal;
    }
    
    DxScalar *MoveToNew() override { return new DxScalar(mVal); }

    Tscalar GetVal( ) const { return mVal; }

    static std::unique_ptr<DxScalar> Parse(char const *buf, size_t len, size_t &ind);

private:
    Tscalar mVal;
};


typedef DxScalar< int16_t,   2, DX_INT16_START,  DxValTypeEnum::DX_INT16 >  DxInt16;
typedef DxScalar< uint16_t,  2, DX_UINT16_START, DxValTypeEnum::DX_UINT16 > DxUint16;
typedef DxScalar< int32_t,   4, DX_INT32_START,  DxValTypeEnum::DX_INT32 >  DxInt32;
typedef DxScalar< uint32_t,  4, DX_UINT32_START, DxValTypeEnum::DX_UINT32 > DxUint32;
typedef DxScalar< int64_t,   8, DX_INT64_START,  DxValTypeEnum::DX_INT64 >  DxInt64;
typedef DxScalar< uint64_t,  8, DX_UINT64_START, DxValTypeEnum::DX_UINT64 > DxUint64;

typedef DxScalar< float,     4, DX_FLT_START,    DxValTypeEnum::DX_FLT >    DxFlt;
typedef DxScalar< double,    8, DX_DBL_START,    DxValTypeEnum::DX_DBL >    DxDbl;


class DxStr;


class DxMap : public DxVal
{
public:
    DxMap()
        : mStrMap( ) 
        , mIntMap( )
    { }
    ~DxMap() = default;
    DxMap(DxMap &&) = default;
    DxMap(DxMap const &) = delete;
    DxMap& operator=(DxMap const &) = delete;
    
    DxValTypeEnum GetType( ) const override { return DxValTypeEnum::DX_MAP; }

    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override;

    DxMap *MoveToNew() override;

    template< typename T >
    void AddKeyVal( std::string &&key, T val )
    {
        mStrMap[ move(key) ] = DxVal::MakeVal<T>( std::move(val) );
    }

    template< typename T >
    void AddKeyVal( int32_t key, T val )
    {
        mIntMap[ key ] = DxVal::MakeVal<T>( std::move(val) );
    }

private:
    std::map< const std::string, std::unique_ptr< const DxVal > > mStrMap;
    std::map< int32_t, std::unique_ptr< const DxVal > > mIntMap;
};


class DxSeq : public DxVal
{
public:
    DxSeq()
        : mVal ( ) { }
    ~DxSeq() = default;
    DxSeq(DxSeq &&) = default;
    DxSeq(DxSeq const &) = delete;
    DxSeq& operator=(DxSeq const &) = delete;

    DxValTypeEnum GetType( ) const override { return DxValTypeEnum::DX_SEQ; }

    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override;

    DxSeq *MoveToNew() override;

    template< typename T >
    void AddVal( T val )
    {
        mVal.push_back( DxVal::MakeVal<T>( std::move(val) ) );
    }

private:
    std::vector<std::unique_ptr<const DxVal>> mVal;
};


class DxBool : public DxVal
{
public:
    explicit DxBool( bool val )
        : mVal( val ) { }

    DxValTypeEnum GetType() const override { return DxValTypeEnum::DX_BOOL; }

    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override;

    DxBool *MoveToNew() override;

    bool GetVal( ) const { return mVal; }

private:
    bool mVal;
};


class DxStr : public DxVal
{
public:
    explicit DxStr( std::string &&val )
        : mVal(std::move(val))
    { }
    explicit DxStr( const char *val )
        : mVal( val )
    { }
    DxStr( DxStr && ) = default;
    DxStr( DxStr const & ) = delete;
    DxStr& operator=( DxStr const & ) = delete;

    DxValTypeEnum GetType() const override { return DxValTypeEnum::DX_STR; }

    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override;

    DxStr *MoveToNew() override;

    std::string const &GetVal( ) const { return mVal; }
    std::string && MoveVal( ) { return std::move(mVal); }

private:
    std::string mVal;
};


class DxMsg
{
public:
    DxMsg( DxStr &&msgType, DxVal &&msgVal )
        : mMsgType( std::unique_ptr< DxStr >( msgType.MoveToNew() ) )
        , mMsgVal( std::unique_ptr< DxVal >( msgVal.MoveToNew() ) )
    { }
    DxMsg( std::unique_ptr<const DxStr> msgType, std::unique_ptr<const DxVal> msgVal)
        : mMsgType( std::move( msgType ) )
        , mMsgVal( std::move( msgVal ) )
    { }
    DxMsg( DxMsg && ) = default;

    void Serialize( std::ostream &os ) const;
    void Dump( std::ostream &os ) const;

    const DxStr &GetMsgType() const { return *mMsgType; }
    const DxVal &GetMsgVal() const { return *mMsgVal; }

private:
    std::unique_ptr<const DxStr> mMsgType;
    std::unique_ptr<const DxVal> mMsgVal;
};


class DxParseException : public std::exception
{
public:
    DxParseException(std::string &msg)
        : mMsg( msg )
    { }
    DxParseException(std::string &&msg)
        : mMsg( msg )
    { }
    DxParseException(char const *msg)
        : mMsg( msg )
    { }

    virtual const char *what() const throw()
    {
        return mMsg.c_str();
    }

private:
    std::string mMsg;
};


std::unique_ptr<DxMsg> ParseDxMsg(char const *buf, size_t len);


template <
    typename Tscalar, 
    const size_t cExpectedSize_, 
    const char cStartByte_, 
    const DxValTypeEnum cType_>
void
DxScalar< Tscalar, cExpectedSize_, cStartByte_, cType_>
::Serialize( std::ostream &os ) const
{
    char const * bytes = static_cast<const char *>(static_cast<const void *>(&mVal));

    os  << cStartByte_;
    for ( size_t i = 0; i < sizeof(Tscalar); ++i )
    {
        os << bytes[i];
    }
}

template <
    typename Tscalar, 
    const size_t cExpectedSize_, 
    const char cStartByte_, 
    const DxValTypeEnum cType_>
std::unique_ptr< DxScalar< Tscalar, cExpectedSize_, cStartByte_, cType_> > 
DxScalar< Tscalar, cExpectedSize_, cStartByte_, cType_>
::Parse(char const *buf, size_t len, size_t &ind)
{
    if ( (len - ind) < sizeof(Tscalar) )
    {
        std::ostringstream os;
        os << "Error parsing " << DxValTypeToString( cType_ ) << ". "
           << "Expecting " << (sizeof(Tscalar)) << " bytes but only "
           << (len - ind) << " bytes available";

        std::string msg = os.str();
        throw DxParseException(msg);
    }

    Tscalar val = *(static_cast<const Tscalar *>(static_cast<const void *>(&buf[ind])));

    ind += sizeof(Tscalar);

    return std::unique_ptr< 
        DxScalar< 
            Tscalar, 
            cExpectedSize_, 
            cStartByte_, 
            cType_ 
        > 
    >( new DxScalar(val) );
}

#endif // DX_MSG_HPP_
