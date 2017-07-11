#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <chrono>

#include "dx_client.hpp"


using std::unique_ptr;
using std::move;


static void bgThreadFunc( void );
static std::thread sBgThread;

static std::queue<std::string> sCmdQueue;
static std::mutex sCmdQueueMutex;


static bool sExit = false;

extern "C" {
void    log_string( const char *str );
void    bug_string( const char *str );
} // extern "C"
void    log_string( const char *str )
{
    std::cout << str << std::endl;
}
void    bug_string( const char *str )
{
    std::cout << str << std::endl;
}

static DxClient &GetDxClient()
{
    static DxClient c;
    return c;
}

static void sendMsg1();


int  main()
{
    GetDxClient().Open();

    sBgThread = std::thread(bgThreadFunc);

    while (true)
    {
        if (sExit)
            break;

        GetDxClient().Pulse();

        std::string cmd;
        bool isCmd = false;
        {
            std::lock_guard<std::mutex> lk(sCmdQueueMutex);

            isCmd = !sCmdQueue.empty();

            if (isCmd)
            {
                cmd = sCmdQueue.front();
                sCmdQueue.pop();
            }
        }

        if (!isCmd)
        {
            std::this_thread::sleep_for(
                std::chrono::seconds(1));
            continue;
        }

        std::cout << "Got command: " << cmd << std::endl;

        if (cmd == "exit")
        {
            // sExit = true;
            return 0;
        }
        else if (cmd == "open")
        {
            GetDxClient().Open();
        }
        else if (cmd == "close")
        {
            GetDxClient().Close();
        }
        else if (cmd == "isopen")
        {
            std::cout << "IsOpen: " << GetDxClient().IsOpen() << std::endl;
        }
        else if (cmd == "sendmsg1")
        {
            sendMsg1();
        }
        else
        {
            std::cout << "Invalid" << std::endl;
        }
    }

    //sBgThread.join();
    return 0;
}


static void bgThreadFunc( void )
{
    std::string inStr;


    while (true)
    {
        std::cout << "Enter command: ";
        if (!std::getline(std::cin, inStr))
            return;

        std::lock_guard<std::mutex> lk(sCmdQueueMutex);

        sCmdQueue.push(inStr);
    }  
}

static void sendMsg1()
{

    unique_ptr<DxValMap> m( new DxValMap );

    m->AddKeyVal(
        unique_ptr<DxValString>( new DxValString("Taco") ),
        unique_ptr<DxValString>( new DxValString("Tuesday") ) );

    m->AddKeyVal(
        unique_ptr<DxValString>( new DxValString("Purple") ),
        unique_ptr<DxValString>( new DxValString("Nurple") ) );

    m->AddKeyVal(
        unique_ptr<DxValInt32>( new DxValInt32(0xFFFF) ),
        unique_ptr<DxValInt32>( new DxValInt32(0xAAAA) ) );

    m->AddKeyVal(
        unique_ptr<DxValDbl>( new DxValDbl(1.2345) ),
        unique_ptr<DxValDbl>( new DxValDbl(10.987654321) ) );

    unique_ptr<DxValSeq> seq( new DxValSeq );

    seq->AddVal(unique_ptr<DxValInt32>( new DxValInt32(1234) ) );
    seq->AddVal(unique_ptr<DxValString>( new DxValString("Herrrrro") ) );

    m->AddKeyVal(
        unique_ptr<DxValString>( new DxValString("ALIST") ),
        move(seq)
    );

    unique_ptr<DxMsg> msg (
        new DxMsg(
            unique_ptr<DxValString>( new DxValString("llama") ),
            move(m)
        )
    );

    std::cout << "sendMsg1" << std::endl;
    msg->Dump(std::cout);
    //std::cout << std::endl;

    GetDxClient().PushMsg(move(msg));
}
