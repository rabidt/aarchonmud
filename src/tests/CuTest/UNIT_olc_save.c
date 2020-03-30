#include <stddef.h>
#include <time.h>
#include "CuTest.h"
#include "../../merc.h"
#include "../../olc_save.h"


void Test_fwrite_flag(CuTest *tc)
{
    struct fwrite_flag_buf fwbuf;

    {
        long input = 0xFFFFFFFF;
        const char *exp = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef";
        const char *res = fwrite_flag(input, &fwbuf);
        CuAssertStrEquals(tc, exp, res);
    }

    {
        long input = 0x00000000;
        const char *exp = "0";
        const char *res = fwrite_flag(input, &fwbuf);
        CuAssertStrEquals(tc, exp, res);
    }

    {
        long input = 0xAAAAAAAA;
        const char *exp = "BDFHJLNPRTVXZbdf";
        const char *res = fwrite_flag(input, &fwbuf);
        CuAssertStrEquals(tc, exp, res);
    }
}
