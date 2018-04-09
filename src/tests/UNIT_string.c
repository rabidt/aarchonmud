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
