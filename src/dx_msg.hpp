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
const char DX_STRING_START = '\x5';
const char DX_INT32_START = '\x6';
const char DX_DBL_START = '\x7';
const char DX_NULL_BYTE = '\x8';


enum class DxValTypeEnum
{
    UNKNOWN = 0,
    DX_NULL,
    DX_MAP,
    DX_SEQ,
    DX_STR,
    DX_INT32,
    DX_DBL
};


class DxVal
{
public:
    virtual ~DxVal( ) { };

    virtual DxValTypeEnum GetType( ) const = 0;
    virtual void Serialize( std::ostream &os ) const = 0;
    virtual void Dump( std::ostream &os, int level ) const = 0;

    virtual DxVal *MoveToNew() = 0;
};


class DxNull : public DxVal
{
public:
    DxValTypeEnum GetType( ) const override { return DxValTypeEnum::DX_NULL; }

    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override;

    DxNull *MoveToNew() override;
};


class DxInt32;
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

    void AddKeyVal( DxInt32 &&key, DxVal &&val );
    void AddKeyVal( std::unique_ptr<const DxInt32>, std::unique_ptr<const DxVal> );
    void AddKeyVal( DxStr &&key, DxVal &&val );
    void AddKeyVal( std::unique_ptr<const DxStr>, std::unique_ptr<const DxVal> );

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

    void AddVal( DxVal &&val );
    void AddVal( std::unique_ptr<const DxVal> val );

private:
    std::vector<std::unique_ptr<const DxVal>> mVal;
};


class DxInt32 : public DxVal
{
public:
    explicit DxInt32( int32_t val )
        : mVal( val ) { }

    DxValTypeEnum GetType() const override { return DxValTypeEnum::DX_INT32; }

    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override;

    DxInt32 *MoveToNew() override;

    int32_t GetVal( ) const { return mVal; }

private:
    int32_t mVal;
};


class DxDbl : public DxVal
{
public:
    explicit DxDbl( double val )
        : mVal( val ) { }

    DxValTypeEnum GetType() const override { return DxValTypeEnum::DX_DBL; }
    
    void Serialize( std::ostream &os ) const override;
    void Dump( std::ostream &os, int level ) const override;

    DxDbl *MoveToNew() override;

    double GetVal( ) const { return mVal; }

private:
    double mVal;
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
    std:: string && MoveVal( ) { return std::move(mVal); }

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


std::unique_ptr<const DxMsg> ParseDxMsg(char const *buf, size_t len);


#endif // DX_MSG_HPP_
