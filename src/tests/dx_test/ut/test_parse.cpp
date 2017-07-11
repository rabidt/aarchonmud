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
            unique_ptr<DxValString>( new DxValString("some type") ),
            unique_ptr<DxVal>( new DxValString("some val") )
        )
    );
}

static unique_ptr<DxMsg> GetSimpleInt32Msg( void )
{
    return unique_ptr<DxMsg>(
        new DxMsg(
            unique_ptr<DxValString>( new DxValString("some type") ),
            unique_ptr<DxVal>( new DxValInt32(123456789) )
        )
    );
}

static unique_ptr<DxMsg> GetComplexMsg1( void )
{
    unique_ptr<DxValMap> m( new DxValMap );

    m->AddKeyVal(
        unique_ptr<DxVal>(new DxValString("Taco")),
        unique_ptr<DxVal>(new DxValString("Tuesday")));

    m->AddKeyVal(
        unique_ptr<DxVal>(new DxValString("Purple")),
        unique_ptr<DxVal>(new DxValString("Nurple")));

    m->AddKeyVal(
        unique_ptr<DxVal>(new DxValInt32(0xFFFF)),
        unique_ptr<DxVal>(new DxValInt32(0xAAAA)));

    m->AddKeyVal(
        unique_ptr<DxVal>(new DxValDbl(1.2345)),
        unique_ptr<DxVal>(new DxValDbl(10.987654321)));


    unique_ptr<DxValSeq> seq( new DxValSeq() );

    seq->AddVal(unique_ptr<DxVal>(new DxValInt32(1234)));
    seq->AddVal(unique_ptr<DxVal>(new DxValString("Herrrrro")));
    seq->AddVal(unique_ptr<DxVal>(new DxValNull));

    m->AddKeyVal(
        unique_ptr<DxVal>(new DxValString("ALIST")),
        std::move(seq));

    unique_ptr<DxMsg> msg1( 
        new DxMsg(
            unique_ptr<DxValString>( new DxValString("llama") ),
            //unique_ptr<DxVal>( m.release() )
            std::move(m)
        )
    );

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

        CuAssertTrue( tc, msgSerStr1   ==  msgSerStr2  );
        CuAssertTrue( tc, msgDumpStr1  ==  msgDumpStr2 );    
    }        
}

extern void TestAbc(CuTest *tc);
void TestAbc(CuTest *tc)
{
    // std::cout << "TACOOOOO";
    
}