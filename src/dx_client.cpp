#include "dx_client.hpp"

extern "C" {
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
} // extern "C"

#include <cstdint>
#include <iostream>

#include "dx_msg.hpp"


extern "C" {
void    log_string( const char *str );
void    bug_string( const char *str );
} // extern "C"

static const char *cSockPath = ".DXPORT";


// static struct timeval sNullTime;
void
DxClient::Open()
{
    struct sockaddr_un server;
    int sock;
    size_t size;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return; // eDXPORT_CLOSED;
    }

    server.sun_family = AF_UNIX;
    strncpy(server.sun_path, cSockPath, sizeof(server.sun_path));

    size = SUN_LEN(&server);

    if (connect(sock, reinterpret_cast<struct sockaddr *>(&server), size) < 0)
    {
        return; // eDXPORT_CLOSED;
    }
    else
    {
        mSockFd = sock;
        return; // eDXPORT_SUCCESS;
    }
}

void
DxClient::Close()
{
    if (mSockFd == 0)
    {
        return; // eDXPORT_CLOSED;
    }

    int rtn = close(mSockFd);
    if (rtn != 0)
    {
        std::ostringstream os;
        os << "close returned " << rtn;
        std::string msg = os.str();
        bug_string(msg.c_str());
    }

    mSockFd = 0;

    mOutMsgQueue = std::queue<std::unique_ptr<const DxMsg>>();

    return; // eDXPORT_SUCCESS;
}


void
DxClient::MsgRecver::RecvMsgs()
{
    if (!mpParent->IsOpen())
        return;

    while ( true )
    {
        if (mStatus == RX_MSGS_IDLE)
        {
            // no message in progress, try to start reading header
            mRxCount = 0;

            mStatus = RX_MSGS_RECV_HEADER;
            continue;
        }
        else if (mStatus == RX_MSGS_RECV_HEADER)
        {
            // start or continue receive header
            ssize_t n = ::recv(
                mpParent->mSockFd, 
                mRxHeaderBuf + mRxCount, sizeof(mRxHeaderBuf) - mRxCount, MSG_DONTWAIT);

            // 0 for closed, -1 for error
            if ( n == 0 )
            {
                mpParent->Close();
                break;
            }
            else if ( n < 0 )
            {
                if ( errno == EAGAIN 
                #if EAGAIN != EWOULDBLOCK // avoid duplicate check to avoid compiler warning
                    || errno == EWOULDBLOCK 
                #endif
                    )
                {
                    // Call would block, try again next pulse
                    break;
                }
                else
                {
                    // Real error
                    mStatus = RX_MSGS_IDLE;
                    mpParent->Close();
                    break;
                }
            }
            else
            {
                // some bytes were received
                mRxCount += static_cast<size_t>(n);

                if ( mRxCount == sizeof(mRxHeaderBuf) )
                {
                    // Whole header received, parse the header then let's start receiving the message
                    if (mRxHeaderBuf[0] != DX_MSG_START)
                    {
                        // Invalid format
                        // TODO: spit an error message
                        mStatus = RX_MSGS_IDLE;
                        mpParent->Close();
                        break;
                    }

                    uint32_t *pMsgSize = static_cast<uint32_t *>(static_cast<void *>(&mRxHeaderBuf[1]));
                    mRxMsgSize = *pMsgSize;

                    if (mvRxBuf.size() < mRxMsgSize)
                    {
                        mvRxBuf.resize(mRxMsgSize);
                    }
                    mRxCount = 0;
                    mStatus = RX_MSGS_RECV_MSG;
                    continue;
                }
                else
                {
                    // Not all bytes received, try to finish next pulse
                    break;
                }
            }
        }
        else if (mStatus == RX_MSGS_RECV_MSG)
        {
            // start or continue receiving message
            ssize_t n = ::recv(
                mpParent->mSockFd, &mvRxBuf[0] + mRxCount, mRxMsgSize - mRxCount, MSG_DONTWAIT);

            // 0 for closed, -1 for error
            if ( n == 0 )
            {
                mpParent->Close();
                break;
            }
            else if ( n < 0 )
            {
                if ( errno == EAGAIN 
                #if EAGAIN != EWOULDBLOCK // avoid duplicate check to avoid compiler warning
                    || errno == EWOULDBLOCK 
                #endif
                    )
                {
                    // Call would block, try again next pulse
                    break;
                }
                else
                {
                    // Real error
                    mStatus = RX_MSGS_IDLE;
                    mpParent->Close();
                    break;
                }
            }
            else
            {
                // some bytes received
                mRxCount += static_cast<size_t>(n);

                if ( mRxCount == mRxMsgSize )
                {
                    // Whoe message received, handle then try to receive next if any
                    mRxCount = 0;
                    mStatus = RX_MSGS_IDLE;

                    std::unique_ptr<const DxMsg> msg;

                    try
                    {
                        msg = ParseDxMsg(&mvRxBuf[0], mRxMsgSize);
                    }
                    catch (DxParseException &ex)
                    {
                        // TODO: log bug message
                        std::ostringstream os;
                        os  << "RecvMsgs: Error parsing message. "
                            << ex.what();
                        std::string exMsg = os.str();
                        bug_string(exMsg.c_str());
                    }

                    if (msg)
                    {
                        mpParent->HandleRxMsg(*msg);
                    }

                    continue;
                }
                else
                {
                    // Not all bytes received, try to finish next pulse
                    break;
                }
            }
        }
    }
}

void
DxClient::MsgSender::SendMsgs()
{
    if (!mpParent->IsOpen())
        return;

    while ( true )
    {
        if (mpParent->mOutMsgQueue.empty())
        {
            break;
        }

        if (mStatus == TX_MSGS_IDLE)
        {
            static_assert( sizeof(uint32_t) == 4 , "" );

            // Rig up the header then move on to sending
            const DxMsg &msg = *(mpParent->mOutMsgQueue.front());
            std::ostringstream os;
            msg.Serialize( os );

            mCurrMsgStr = os.str();
            mTxCount = 0;
            uint32_t msgSize = mCurrMsgStr.size();

            mTxHeaderBuf[0] = DX_MSG_START;
            ::memcpy(&mTxHeaderBuf[1], &msgSize, sizeof(uint32_t));

            mStatus = TX_MSGS_SEND_HEADER;
            continue;
        }
        else if (mStatus == TX_MSGS_SEND_HEADER)
        {
            // start or continue sending header
            ssize_t n = ::send(
                mpParent->mSockFd, mTxHeaderBuf + mTxCount, sizeof(mTxHeaderBuf) - mTxCount, 
                MSG_NOSIGNAL | MSG_DONTWAIT);

            // -1 is error
            if ( n < 0 )
            {
                if ( errno == EAGAIN 
                #if EAGAIN != EWOULDBLOCK // avoid duplicate check to avoid compiler warning
                    || errno == EWOULDBLOCK 
                #endif
                    )
                {
                    // Call would block, try again next pulse
                    break;
                }
                else
                {
                    // Real error
                    mStatus = TX_MSGS_IDLE;
                    mpParent->Close();
                    break;
                }
            }
            else
            {
                // some bytes were sent
                mTxCount += static_cast<size_t>(n);

                if ( mTxCount == sizeof(mTxHeaderBuf) )
                {
                    // Whole header sent, let's start sending the message
                    mTxCount = 0;
                    mStatus = TX_MSGS_SEND_MSG;
                    continue;
                }
                else
                {
                    // Not all bytes sent, try to finish next pulse
                    break;
                }
            }
        }
        else if (mStatus == TX_MSGS_SEND_MSG)
        {
            // start or continue sending message            
            char const * data = mCurrMsgStr.c_str();
            size_t msgSize = mCurrMsgStr.size();

            ssize_t n = ::send(
                mpParent->mSockFd, data + mTxCount, msgSize - mTxCount, MSG_NOSIGNAL | MSG_DONTWAIT);

            // -1 is error
            if ( n < 0 )
            {
                if ( errno == EAGAIN 
                #if EAGAIN != EWOULDBLOCK // avoid duplicate check to avoid compiler warning
                    || errno == EWOULDBLOCK 
                #endif
                    )
                {
                    // Call would block, try again next pulse
                    break;
                }
                else
                {
                    // Real error
                    mStatus = TX_MSGS_IDLE;
                    mpParent->Close();
                    break;
                }
            }
            else
            {
                // some bytes were sent
                mTxCount += static_cast<size_t>(n);

                if ( mTxCount == msgSize )
                {
                    // Whole message sent, try to send the next one if any
                    mTxCount = 0;
                    mStatus = TX_MSGS_IDLE;
                    mCurrMsgStr.clear();
                    mpParent->mOutMsgQueue.pop();
                    continue;
                }
                else
                {
                    // Not all bytes sent, try to finish next pulse
                    break;
                }
            }
        }
    }   
}

void 
DxClient::Pulse()
{
    mRecver.RecvMsgs();
    mSender.SendMsgs();
}

bool
DxClient::IsOpen()
{
    return (mSockFd != 0);
}

void
DxClient::PushMsg(std::unique_ptr<const DxMsg> msg)
{
    mOutMsgQueue.push(std::move(msg));
}

void
DxClient::SetMsgHandler(const std::string &msgType, MsgHndlrIf *hndlr)
{
    mMsgHndlrMap[msgType] = hndlr;
}

void
DxClient::HandleRxMsg(const DxMsg &msg)
{
    const std::string &msgType = msg.GetMsgType().GetVal();

    auto itr = mMsgHndlrMap.find( msgType );

    if ( itr == mMsgHndlrMap.end() )
    {
        // Not found
        std::string bugMsg = "No message handler for message type: " + msgType;
        bug_string(bugMsg.c_str());
    }
    else
    {
        itr->second->HandleMsg( msg );
    }

}
