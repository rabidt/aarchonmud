#include <stddef.h>
#include <time.h>
#include "CuTest.h"
#include "../../merc.h"


void Test_smash_tilde_cpy(CuTest *tc)
{
    {
        // standard case
        const char *input = "It has ~some~ tildes";
        const char *exp = "It has -some- tildes";
        char buf[32];
        smash_tilde_cpy(buf, input, sizeof(buf));
        CuAssertStrEquals(tc, exp, buf);
    }
    
    {
        // truncation
        const char *input = "It has ~some~ tildes";
        const char *exp = "It has -some- t";
        char buf[16];
        smash_tilde_cpy(buf, input, sizeof(buf));
        CuAssertStrEquals(tc, exp, buf);
    }

    {
        // no truncation. exact size
        const char *input = "It has ~some~ tildes";
        const char *exp = "It has -some- tildes";
        char buf[21];
        smash_tilde_cpy(buf, input, sizeof(buf));
        CuAssertStrEquals(tc, exp, buf);
    }

    {
        // truncation. 1 character
        const char *input = "It has ~some~ tildes";
        const char *exp = "It has -some- tilde";
        char buf[20];
        smash_tilde_cpy(buf, input, sizeof(buf));
        CuAssertStrEquals(tc, exp, buf);
    }
}