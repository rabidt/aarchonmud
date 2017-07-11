#ifndef DX_CLIENT_HPP_
#define DX_CLIENT_HPP_


#include <string>
#include <queue>
#include <vector>
#include <map>

#include "dx_msg.hpp"


class DxClient
{
public:
    class MsgHndlrIf;
    explicit DxClient( )
        : mSockFd( 0 )
        , mOutMsgQueue( )
        , mMsgHndlrMap( )
        , mSender( this )
        , mRecver( this )
    {

    }
    DxClient(const DxClient &) = delete;
    DxClient & operator=(const DxClient &) = delete;

    void Open();
    void Close();
    bool IsOpen();

    void Pulse();

    void PushMsg( std::unique_ptr<const DxMsg> msg );

    class MsgHndlrIf
    {
    public:
        virtual void HandleMsg(const DxMsg &msg) = 0;
    };
    void SetMsgHandler(const std::string &msgType, MsgHndlrIf *hndlr);

private:
    void HandleRxMsg(const DxMsg &msg);

    int mSockFd;
    std::queue<std::unique_ptr<const DxMsg>> mOutMsgQueue;
    
    std::map< std::string, MsgHndlrIf *> mMsgHndlrMap;


    class MsgSender
    {
    public:
        MsgSender(DxClient *parent)
            : mpParent( parent )
            , mStatus( TX_MSGS_IDLE )
            , mTxCount( 0 )
            , mCurrMsgStr( )
        { }
        MsgSender(const MsgSender &) = delete;
        MsgSender& operator=(const MsgSender &) = delete;

        void SendMsgs();

    private:
        enum eSTATUS
        {
            TX_MSGS_UNKNOWN = 0,
            TX_MSGS_IDLE,
            TX_MSGS_SEND_HEADER,
            TX_MSGS_SEND_MSG
        };

        DxClient *mpParent;

        eSTATUS mStatus;
        size_t mTxCount;
        std::string mCurrMsgStr;
        unsigned char mTxHeaderBuf[5];
    };

    class MsgRecver
    {
    public:
        MsgRecver(DxClient *parent)
            : mpParent( parent )
            , mStatus( RX_MSGS_IDLE )
            , mRxCount( 0 )
            , mRxMsgSize( 0 )
            , mvRxBuf( 1024 ) // Arbitrary starting size
        { }
        MsgRecver(const MsgRecver &) = delete;
        MsgRecver& operator=(const MsgRecver &) = delete;

        void RecvMsgs();

    private:
        enum eSTATUS
        {
            RX_MSGS_UNKNOWN = 0,
            RX_MSGS_IDLE,
            RX_MSGS_RECV_HEADER,
            RX_MSGS_RECV_MSG    
        };

        DxClient *mpParent;

        eSTATUS mStatus;
        size_t mRxCount;
        size_t mRxMsgSize;
        unsigned char mRxHeaderBuf[5];
        std::vector<char> mvRxBuf;        
    };

    MsgSender mSender;
    MsgRecver mRecver;
};


#endif // DX_CLIENT_HPP_
