#include <stddef.h>
#include <time.h>
#include <string.h>
#include "CuTest.h"
#include "../merc.h"


void Test_colour(CuTest *tc)
{
    
    {
        // Should write nothing if ch is NULL
        char buf[5] = "testx";
        int rc = colour('r', NULL, buf, sizeof(buf));
        CuAssertIntEquals(tc, 0, rc);
        CuAssertStrEquals(tc, "testx", buf);
    }

    {
        // Should write nothing if ch->pcdata is NULL
        char buf[5] = "testx";

        CHAR_DATA ch;
        ch.pcdata = NULL;
        int rc = colour('r', &ch, buf, sizeof(buf));
        CuAssertIntEquals(tc, 0, rc);
        CuAssertStrEquals(tc, "testx", buf);
    }

    {
        CHAR_DATA ch;
        PC_DATA pcdata;
        ch.pcdata = &pcdata;

        char buf[20];

        // Some regular non custom codes
        {
            int rc = colour('r', &ch, buf, sizeof(buf));
            CuAssertIntEquals(tc, 7, rc);
            CuAssertStrEquals(tc, "\x1b[0;31m", buf);
        }

        {
            int rc = colour('R', &ch, buf, sizeof(buf));
            CuAssertIntEquals(tc, 7, rc);
            CuAssertStrEquals(tc, "\x1b[1;31m", buf);
        }

        {
            int rc = colour('M', &ch, buf, sizeof(buf));
            CuAssertIntEquals(tc, 7, rc);
            CuAssertStrEquals(tc, "\x1b[1;35m", buf);
        }

        {
            int rc = colour('x', &ch, buf, sizeof(buf));
            CuAssertIntEquals(tc, 4, rc);
            CuAssertStrEquals(tc, "\x1b[0m", buf);
        }

        {
            int rc = colour('{', &ch, buf, sizeof(buf));
            CuAssertIntEquals(tc, 1, rc);
            CuAssertStrEquals(tc, "{", buf);
        }

        // Unsupported code should give CLEAR code
        {
            int rc = colour('@', &ch, buf, sizeof(buf));
            CuAssertIntEquals(tc, 4, rc);
            CuAssertStrEquals(tc, "\x1b[0m", buf);
        }

        // custom color checks
        {
            pcdata.gossip[0] = NORMAL;
            pcdata.gossip[1] = YELLOW;
            pcdata.gossip[2] = 0;

            int rc = colour('p', &ch, buf, sizeof(buf));
            CuAssertIntEquals(tc, 7, rc);
            CuAssertStrEquals(tc, "\x1b[0;33m", buf);
        }

        {
            pcdata.tells[0] = BRIGHT;
            pcdata.tells[1] = GREEN;
            pcdata.tells[2] = 1;

            int rc = colour('t', &ch, buf, sizeof(buf));
            CuAssertIntEquals(tc, 8, rc);
            CuAssertStrEquals(tc, "\x1b[1;32m\a", buf);
        }

        // test truncation
        {
            
            int rc = colour('r', &ch, buf, 3);
            CuAssertIntEquals(tc, 7, rc);
            CuAssertStrEquals(tc, "\x1b[", buf);
        }

        {
            pcdata.gossip[0] = NORMAL;
            pcdata.gossip[1] = YELLOW;
            pcdata.gossip[2] = 0;

            int rc = colour('p', &ch, buf, 4);
            CuAssertIntEquals(tc, 7, rc);
            CuAssertStrEquals(tc, "\x1b[0", buf);
        }
    }
}

void Test_colourconv(CuTest *tc)
{
    CHAR_DATA ch;
    DESCRIPTOR_DATA desc;
    PC_DATA pcdata;
    ch.desc = &desc;
    ch.pcdata = &pcdata;

    // color enabled
    SET_BIT( ch.act, PLR_COLOUR );
    {
        // standard case
        const char *input = "{RHello {gworld {{{xdonkey}";
        const char *exp = "\x1b[1;31mHello \x1b[0;32mworld {\x1b[0mdonkey}";
        char outbuf[128];

        colourconv(outbuf, sizeof(outbuf), input, &ch);
        CuAssertStrEquals(tc, exp, outbuf);
    }

    {
        // should trim a trailing single '{''
        const char *input = "Some sort of string{";
        const char *exp = "Some sort of string";
        char outbuf[128];

        colourconv(outbuf, sizeof(outbuf), input, &ch);
        CuAssertStrEquals(tc, exp, outbuf);
    }

    {
        // truncate
        const char *input = "{RHello {gworld {{{xdonkey}";
        const char *exp = "\x1b[1;31mHe";
        char outbuf[10];

        colourconv(outbuf, sizeof(outbuf), input, &ch);
        CuAssertStrEquals(tc, exp, outbuf);
    }

    {
        // truncate with no partial escapes
        const char *input = "{RHello {gworld {{{xdonkey}";
        const char *exp = "\x1b[1;31mHello ";
        char outbuf[16];

        colourconv(outbuf, sizeof(outbuf), input, &ch);
        CuAssertStrEquals(tc, exp, outbuf);
    }

    // color disabled
    REMOVE_BIT( ch.act, PLR_COLOUR );
    {
        // standard case
        const char *input = "{RHello {gworld {{{xdonkey}";
        const char *exp = "Hello world {donkey}";
        char outbuf[128];

        colourconv(outbuf, sizeof(outbuf), input, &ch);
        CuAssertStrEquals(tc, exp, outbuf);
    }

    {
        // truncate case
        const char *input = "{RHello {gworld {{{xdonkey}";
        const char *exp = "Hello w";
        char outbuf[8];

        colourconv(outbuf, sizeof(outbuf), input, &ch);
        CuAssertStrEquals(tc, exp, outbuf);
    }
}
