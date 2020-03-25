#include <stddef.h>
#include <time.h>
#include <string.h>
#include "CuTest.h"
#include "../merc.h"


void Test_strlcat(CuTest *tc)
{
    char buf[8];
    const size_t bufsz = sizeof(buf);

    size_t res;

    // Simple case, no truncation
    strcpy(buf, "abc");
    CuAssertStrEquals(tc, "abc", buf);
    res = strlcat(buf, "def", bufsz);
    CuAssertStrEquals(tc, "abcdef", buf);
    CuAssertIntEquals(tc, 6, res);
    CuAssertIntEquals(tc, 6, strlen(buf));

    // Truncate 1 char
    strcpy(buf, "abcd");
    CuAssertStrEquals(tc, "abcd", buf);
    res = strlcat(buf, "efgh", bufsz);
    CuAssertStrEquals(tc, "abcdefg", buf);
    CuAssertIntEquals(tc, 8, res);
    CuAssertIntEquals(tc, 7, strlen(buf));

    // Truncate multiple chars
    strcpy(buf, "abcd");
    CuAssertStrEquals(tc, "abcd", buf);
    res = strlcat(buf, "efghijklmnop", bufsz);
    CuAssertStrEquals(tc, "abcdefg", buf);
    CuAssertIntEquals(tc, 16, res);
    CuAssertIntEquals(tc, 7, strlen(buf));

    // empty src
    strcpy(buf, "abcd");
    CuAssertStrEquals(tc, "abcd", buf);
    res = strlcat(buf, "", bufsz);
    CuAssertStrEquals(tc, "abcd", buf);
    CuAssertIntEquals(tc, 4, res);
    CuAssertIntEquals(tc, 4, strlen(buf));
}

void Test_strlcpy(CuTest *tc)
{
    char buf[8];
    const size_t bufsz = sizeof(buf);

    size_t res;

    // Simple case, copy less than buffer size
    res = strlcpy(buf, "abcd", bufsz);
    CuAssertStrEquals(tc, "abcd", buf);
    CuAssertIntEquals(tc, 4, res);
    CuAssertIntEquals(tc, 4, strlen(buf));

    // src is 1 char too big (src_len == bufsz)
    res = strlcpy(buf, "abcdefgh", bufsz);
    CuAssertStrEquals(tc, "abcdefg", buf);
    CuAssertIntEquals(tc, 8, res);
    CuAssertIntEquals(tc, 7, strlen(buf));

    // src is many chars too big (src_len > bufsz)
    res = strlcpy(buf, "abcdefghijklmnop", bufsz);
    CuAssertStrEquals(tc, "abcdefg", buf);
    CuAssertIntEquals(tc, 16, res);
    CuAssertIntEquals(tc, 7, strlen(buf));

    // copy empty string
    res = strlcpy(buf, "", bufsz);
    CuAssertStrEquals(tc, "", buf);
    CuAssertIntEquals(tc, 0, res);
    CuAssertIntEquals(tc, 0, strlen(buf));
}

void Test_split_string(CuTest *tc)
{
    {
        /* no split */
        char pf[8] = "abc";
        char sf[8] = "def";

        const char * const input = "hotdogs are not sandwiches, but hamburgers are";

        bool res = split_string(input, '/', pf, sizeof(pf), sf, sizeof(sf));
        CuAssertIntEquals(tc, FALSE, res);
        CuAssertStrEquals(tc, "abc", pf);
        CuAssertStrEquals(tc, "def", sf);
    }

    {
        /* split on ' ' and no truncate */
        char pf[16] = "";
        char sf[64] = "";

        const char * const input = "hotdogs are not sandwiches, but hamburgers are";

        bool res = split_string(input, ' ', pf, sizeof(pf), sf, sizeof(sf));
        CuAssertIntEquals(tc, TRUE, res);
        CuAssertStrEquals(tc, "hotdogs", pf);
        CuAssertStrEquals(tc, "are not sandwiches, but hamburgers are", sf);   
    }

    {
        /* split on ',' and no truncate */
        char pf[32] = "";
        char sf[32] = "";

        const char * const input = "hotdogs are not sandwiches, but hamburgers are";

        bool res = split_string(input, ',', pf, sizeof(pf), sf, sizeof(sf));
        CuAssertIntEquals(tc, TRUE, res);
        CuAssertStrEquals(tc, "hotdogs are not sandwiches", pf);
        CuAssertStrEquals(tc, " but hamburgers are", sf);
    }

    {
        /* no truncate. exact size */
        char pf[27] = "";
        char sf[20] = "";

        const char * const input = "hotdogs are not sandwiches, but hamburgers are";

        bool res = split_string(input, ',', pf, sizeof(pf), sf, sizeof(sf));
        CuAssertIntEquals(tc, TRUE, res);
        CuAssertStrEquals(tc, "hotdogs are not sandwiches", pf);
        CuAssertStrEquals(tc, " but hamburgers are", sf);
    }

    {
        /* truncate 1 character */
        char pf[26] = "";
        char sf[19] = "";

        const char * const input = "hotdogs are not sandwiches, but hamburgers are";

        bool res = split_string(input, ',', pf, sizeof(pf), sf, sizeof(sf));
        CuAssertIntEquals(tc, TRUE, res);
        CuAssertStrEquals(tc, "hotdogs are not sandwiche", pf);
        CuAssertStrEquals(tc, " but hamburgers ar", sf);
    }
    
    {
        /* first char split */
        char pf[4] = "abc";
        char sf[32] = "";

        const char * const input = ",first char split";

        bool res = split_string(input, ',', pf, sizeof(pf), sf, sizeof(sf));
        CuAssertIntEquals(tc, TRUE, res);
        CuAssertStrEquals(tc, "", pf);
        CuAssertStrEquals(tc, "first char split", sf);
    }

    {
        /* last char split */
        char pf[32] = "";
        char sf[4] = "abc";

        const char * const input = "last char split,";

        bool res = split_string(input, ',', pf, sizeof(pf), sf, sizeof(sf));
        CuAssertIntEquals(tc, TRUE, res);
        CuAssertStrEquals(tc, "last char split", pf);
        CuAssertStrEquals(tc, "", sf);
    }
}
