#include <iostream>
#include <string>
#include <sstream>

#include "dx_msg.hpp"

extern "C" {
#include "CuTest.h"
}


using std::unique_ptr;


extern "C" {
extern void bug_string(const char *msg);
void bug_string(const char *msg)
{
    std::cout << "BUG::" << msg << std::endl;
}
}


static unique_ptr<DxMsg> GetSimpleStringMsg( void )
{
    return unique_ptr<DxMsg>(
        new DxMsg(
            DxStr("some type"),
            DxStr("some val")
        )
    );
}

static unique_ptr<DxMsg> GetSimpleInt32Msg( void )
{
    return unique_ptr<DxMsg>(
        new DxMsg(
            DxStr("some type"),
            DxInt32(123456789)
        )
    );
}

static unique_ptr<DxMsg> GetComplexMsg1( void )
{
    DxMap m;

    m.AddKeyVal(
        "Taco",
        "Tuesday");

    m.AddKeyVal(
        "Purple",
        "Nurple");

    m.AddKeyVal(
        0xFFFF,
        0xAAAA);

    m.AddKeyVal(
        12345,
        10.987654321);

    m.AddKeyVal(
        "HerpDDDDDerp",
        true);

    m.AddKeyVal(
        4433221,
        0xFFFFFFFFFFFFFFFF);

    m.AddKeyVal(
        "int16",
        int16_t(1234));

    m.AddKeyVal(
        "uint16",
        uint16_t(4321));

    m.AddKeyVal(
        "int32",
        int32_t(1234));

    m.AddKeyVal(
        "uint32",
        uint32_t(4321));

    m.AddKeyVal(
        "int64",
        int64_t(1234));

    m.AddKeyVal(
        "uint64",
        uint64_t(4321));

    m.AddKeyVal(
        "float",
        1.234f);

    m.AddKeyVal(
        "double",
        1.234);


    DxSeq seq;

    seq.AddVal(1234);
    seq.AddVal("Herrrrro");
    seq.AddVal(nullptr);

    m.AddKeyVal<DxVal &&>(
        "ALIST",
        std::move(seq));

    unique_ptr<DxMsg> msg1( 
        new DxMsg(
            DxStr("llama"),
            std::move(m)));

    return msg1;
}

static unique_ptr<DxMsg> (* msgGetFuncs[])( void ) = 
{
    &GetSimpleStringMsg,
    &GetSimpleInt32Msg,
    &GetComplexMsg1
};

extern void TestSerializeDeserialize(CuTest *tc);
void TestSerializeDeserialize(CuTest *tc)
{
    const size_t funcCount = sizeof(msgGetFuncs) / sizeof(msgGetFuncs[0]);
    
    for (size_t i = 0; i < funcCount; ++i)
    {
        unique_ptr<const DxMsg> msg1 = msgGetFuncs[i]();

        std::ostringstream os;

        os.str("");
        msg1->Serialize( os );
        std::string msgSerStr1 = os.str();

        os.str("");
        msg1->Dump( os );
        std::string msgDumpStr1 = os.str();


        unique_ptr<const DxMsg> msg2 = ParseDxMsg(msgSerStr1.c_str(), msgSerStr1.size());

        os.str("");
        msg2->Serialize( os );
        std::string msgSerStr2 = os.str();

        os.str("");
        msg2->Dump( os );
        std::string msgDumpStr2 = os.str();

        std::cout <<  msgDumpStr1;

        CuAssertTrue( tc, msgSerStr1   ==  msgSerStr2  );
        CuAssertTrue( tc, msgDumpStr1  ==  msgDumpStr2 );    
    }        
}

extern void TestAbc(CuTest *tc);
void TestAbc(CuTest *tc)
{
    // std::cout << "TACOOOOO";
    
}