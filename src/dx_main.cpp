#include "dx_main.hpp"

#include "dx_client.hpp"
#include "dx_pfile_sync.hpp"


/* handle echo back messages for testing purposes */
class EchoBackHandler : public DxClient::MsgHndlrIf
{
public:
    void HandleMsg(const DxMsg &msg)
    {
        std::ostringstream os;
        msg.Serialize( os );
        std::string msgStr = os.str();

        std::unique_ptr<const DxMsg> outMsg = ParseDxMsg(msgStr.c_str(), msgStr.size());

        GetDxClient().PushMsg( std::move(outMsg) );
    }
};

static EchoBackHandler sEchoBackHandler;


DxClient &GetDxClient()
{
    static DxClient p;
    return p;
}

static DxPfileSync &GetPfileSync()
{
    static DxPfileSync p( &GetDxClient() );
    return p;
}

void DX_init(void)
{
    GetDxClient(); // force construction
    GetPfileSync(); // force construction

    GetDxClient().SetMsgHandler("echo_back", &sEchoBackHandler);

    GetDxClient().Open();
}

void DX_open(void)
{
    GetDxClient().Open();
}

void DX_close(void)
{
    GetDxClient().Close();
}

bool DX_is_open(void)
{
    return GetDxClient().IsOpen();
}

void DX_pulse(void)
{
    GetDxClient().Pulse();
}
