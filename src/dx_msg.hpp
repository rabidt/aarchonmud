#ifndef DX_MSG_HPP_
#define DX_MSG_HPP_


#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <exception>
#include <map>


const char DX_MSG_START = '\x1';
const char DX_MAP_START = '\x2';
const char DX_SEQ_START = '\x3';
const char DX_STRING_START = '\x4';
const char DX_INT32_START = '\x5';
const char DX_DBL_START = '\x6';
const char DX_NULL_BYTE = '\x7';
const char DX_VAL_END = '\x8';


enum DxValTypeEnum
{
    DX_VAL_TYPE_UNKNOWN = 0,
    DX_NULL,
    DX_MAP,
    DX_SEQ,
    DX_STRING,
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
};


class DxValNull : public DxVal
{
public:
    DxValTypeEnum GetType( ) const { return DX_NULL; }
    void Serialize( std::ostream &os ) const;
    void Dump( std::ostream &os, int level ) const;
};


class DxValMap : public DxVal
{
public:
    DxValMap()
        : mVal( ) { }
    
    DxValTypeEnum GetType( ) const { return DX_MAP; }
    void AddKeyVal( std::unique_ptr<const DxVal> k, std::unique_ptr<const DxVal> v );
    void AddKeyVal( const DxVal *k, const DxVal *v);
    void Serialize( std::ostream &os ) const;
    void Dump( std::ostream &os, int level ) const;

private:
    struct kv
    {
        std::unique_ptr<const DxVal> key;
        std::unique_ptr<const DxVal> val;
    };

    std::vector<kv> mVal;
};


class DxValSeq : public DxVal
{
public:
    DxValSeq()
        : mVal ( ) { }

    DxValTypeEnum GetType( ) const { return DX_SEQ; }
    void AddVal( std::unique_ptr<const DxVal> val );
    void AddVal( const DxVal *val );
    void Serialize( std::ostream &os ) const;
    void Dump( std::ostream &os, int level ) const;

private:
    std::vector<std::unique_ptr<const DxVal>> mVal;
};


class DxValInt32 : public DxVal
{
public:
    explicit DxValInt32( int32_t val = 0 )
        : mVal( val ) { }

    DxValTypeEnum GetType() const { return DX_INT32; }
    void SetVal(int32_t val) { mVal = val; }
    void Serialize( std::ostream &os ) const;
    void Dump( std::ostream &os, int level ) const;
    int32_t GetVal( ) const { return mVal; }

private:
    int32_t mVal;
};

class DxValDbl : public DxVal
{
public:
    explicit DxValDbl( double val = 0)
        : mVal( val ) { }

    DxValTypeEnum GetType() const { return DX_DBL; }
    void SetVal(double val) { mVal = val; }
    void Serialize( std::ostream &os ) const;
    void Dump( std::ostream &os, int level ) const;
    double GetVal( ) const { return mVal; }

private:
    double mVal;
};

class DxValString : public DxVal
{
public:
    DxValString()
        : mVal( ) { }
    explicit DxValString( std::string const &val )
        : mVal( val ) { }
    explicit DxValString( char const *val )
        : mVal( val ) { }
    DxValString( char const *buf, size_t ind, size_t len )
        : mVal( buf + ind, len ) { }

    DxValTypeEnum GetType() const { return DX_STRING; }
    void SetVal( std::string const &val ) { mVal = val; }
    void Serialize( std::ostream &os ) const;
    void Dump( std::ostream &os, int level ) const;

    std::string const &GetVal( ) const { return mVal; }

private:
    std::string mVal;
};


class DxMsg
{
public:
    DxMsg( std::unique_ptr<const DxValString> msgType, std::unique_ptr<const DxVal> msgVal)
        : mMsgType( std::move(msgType) )
        , mMsgVal( std::move(msgVal) )
    { }
    DxMsg( const DxValString *msgType, const DxVal *msgVal)
        : mMsgType( msgType )
        , mMsgVal( msgVal )
    { }

    void Serialize( std::ostream &os ) const;
    void Dump( std::ostream &os ) const;

    const DxValString &GetMsgType() const { return *mMsgType; }
    const DxVal &GetMsgVal() const { return *mMsgVal; }

private:
    std::unique_ptr<const DxValString> mMsgType;
    std::unique_ptr<const DxVal> mMsgVal;
};

std::unique_ptr<const DxMsg> ParseDxMsg(char const *buf, size_t len);


class DxParseException : public std::exception
{
public:
    DxParseException(std::string &msg)
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

#endif // DX_MSG_HPP_
