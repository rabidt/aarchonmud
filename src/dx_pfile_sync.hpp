#ifndef DX_FILE_SYNC_HPP_
#define DX_FILE_SYNC_HPP_


#include "dx_client.hpp"


class DxPfileSync
{
public:
    DxPfileSync( DxClient *dc )
        : mpDc( dc )
        , mReqPfileSeqHndlr( this )
        , mReqPfileHndlr( this )
    { 
        mpDc->SetMsgHandler("req_pfile_seq", &mReqPfileSeqHndlr);
        mpDc->SetMsgHandler("req_pfile",     &mReqPfileHndlr);
    }

private:
    DxClient *mpDc;

    class ReqPfileSeqHndlr : public DxClient::MsgHndlrIf
    {
    public:
        ReqPfileSeqHndlr( DxPfileSync *parent )
            : mpParent( parent )
        { }

        void HandleMsg( const DxMsg &msg );

    private:
        DxPfileSync *mpParent;
    };

    class ReqPfileHndlr : public DxClient::MsgHndlrIf
    {
    public:
        ReqPfileHndlr( DxPfileSync *parent )
            : mpParent( parent )
        { }

        void HandleMsg( const DxMsg &msg );

    private:
        DxPfileSync *mpParent;
    };

    ReqPfileSeqHndlr mReqPfileSeqHndlr;
    ReqPfileHndlr mReqPfileHndlr;
};


#endif // DX_FILE_SYNC_HPP_
