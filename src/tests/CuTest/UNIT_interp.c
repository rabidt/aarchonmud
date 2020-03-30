#include <stddef.h>
#include <time.h>
#include "CuTest.h"
#include "../../merc.h"


/* base function for number_argument and mult_argument */
void Test_split_argument(CuTest *tc)
{
    {
        // no .
        const char * const input = "1324hamster";
        const char * const exp = input;
        char buf[16];

        int n = split_argument(input, buf, sizeof(buf), '.');
        CuAssertIntEquals(tc, 1, n);
        CuAssertStrEquals(tc, exp, buf);
    }

    {
        // no . and truncate
        const char * const input = "1324hamster";
        const char * const exp = "1324ham";
        char buf[8];

        int n = split_argument(input, buf, sizeof(buf), '.');
        CuAssertIntEquals(tc, 1, n);
        CuAssertStrEquals(tc, exp, buf);
    }

    {
        // with .
        const char * const input = "1324.hamster";
        const char * const exp = "hamster";
        char buf[16];

        int n = split_argument(input, buf, sizeof(buf), '.');
        CuAssertIntEquals(tc, 1324, n);
        CuAssertStrEquals(tc, exp, buf);   
    }

    {
        // with . and truncate
        const char * const input = "1324.hamster";
        const char * const exp = "ham";
        char buf[4];

        int n = split_argument(input, buf, sizeof(buf), '.');
        CuAssertIntEquals(tc, 1324, n);
        CuAssertStrEquals(tc, exp, buf);   
    }

    {
        // with . but not number
        const char * const input = "abc.hamster";
        const char * const exp = input;
        char buf[16];

        int n = split_argument(input, buf, sizeof(buf), '.');
        CuAssertIntEquals(tc, 1, n);
        CuAssertStrEquals(tc, exp, buf);   
    }

}