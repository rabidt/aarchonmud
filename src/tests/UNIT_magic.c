#include <stddef.h>
#include <time.h>
#include "CuTest.h"
#include "../merc.h"


void Test_skill_lookup(CuTest *tc)
{
    int val;
    
    val = skill_lookup("asdff");
    CuAssertIntEquals(tc, -1, val);

    val = skill_lookup("reser");
    CuAssertIntEquals(tc, 0, val);
    
    val = skill_lookup("reserved");
    CuAssertIntEquals(tc, 0, val);
}

void Test_skill_lookup_exact(CuTest *tc)
{
    int val;
    
    val = skill_lookup_exact("asdff");
    CuAssertIntEquals(tc, -1, val);

    val = skill_lookup_exact("reser");
    CuAssertIntEquals(tc, -1, val);
    
    val = skill_lookup_exact("reserved");
    CuAssertIntEquals(tc, 0, val);
}

void Test_class_skill_lookup(CuTest *tc)
{
    int val;

    int warrior=class_lookup("warrior");

    val = class_skill_lookup(warrior, "asdff");
    CuAssertIntEquals(tc, -1, val);

    int sword= class_skill_lookup(warrior, "sword");
    CuAssertTrue(tc, -1 < sword);
    
    val = class_skill_lookup(warrior, "swo");
    CuAssertTrue(tc, -1 < val);

    CuAssertIntEquals(tc, sword, val);
}
