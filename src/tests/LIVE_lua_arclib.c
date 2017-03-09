#include <stddef.h>
#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "../merc.h"
#include "../lua_main.h"
#include "CuTest.h"


static int pcall(lua_State *LS, int nargs, int nreturn)
{
    int rtn = CallLuaWithTraceBack(LS, nargs, nreturn);
    if (rtn != 0)
    {
        printf(lua_tostring(g_mud_LS, -1));
        lua_pop(g_mud_LS, 1);
    }
    return rtn;
}

void Test_CH_set_hp(CuTest *tc)
{
    CHAR_DATA *ch = new_char();


    CuAssertIntEquals(tc, 0, luaL_loadstring(g_mud_LS,
            "local ch = ...\r\n"
            "ch.hp = 12345"));
    push_CH(g_mud_LS, ch);

    CuAssertIntEquals(tc, 0, pcall(g_mud_LS, 1, 0));
    CuAssertIntEquals(tc, 12345, ch->hit);
}

void Test_CH_get_hp(CuTest *tc)
{
    CHAR_DATA *ch = new_char();
    ch->hit = 4321;


    CuAssertIntEquals(tc, 0, luaL_loadstring(g_mud_LS,
            "local ch = ...\r\n"
            "return ch.hp"));
    push_CH(g_mud_LS, ch);

    CuAssertIntEquals(tc, 0, pcall(g_mud_LS, 1, 1));
    int val = luaL_checkinteger(g_mud_LS, -1);
    CuAssertIntEquals(tc, ch->hit, val);
}
