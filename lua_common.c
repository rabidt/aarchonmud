#include <lualib.h>
#include <lauxlib.h>
#include "timer.h"
#include "lua_common.h"

void stackDump (lua_State *LS) {
    int i;
    int top = lua_gettop(LS);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(LS, i);
        switch (t) {

            case LUA_TSTRING:  /* strings */
                bugf("`%s'", lua_tostring(LS, i));
                break;

            case LUA_TBOOLEAN:  /* booleans */
                bugf(lua_toboolean(LS, i) ? "true" : "false");
                break;

            case LUA_TNUMBER:  /* numbers */
                bugf("%g", lua_tonumber(LS, i));
                break;

            default:  /* other values */
                bugf("%s", lua_typename(LS, t));
                break;

        }
    }
    bugf("\n");  /* end the listing */
}

int L_delay (lua_State *LS)
{
    /* delaytbl has timer pointers as keys
       value is table with 'tableid' and 'func' keys */
    /* delaytbl[tmr]={ tableid=tableid, func=func } */
    const char *tag=NULL;
    int val=luaL_checkint( LS, 2 );
    luaL_checktype( LS, 3, LUA_TFUNCTION);
    if (!lua_isnone( LS, 4 ) )
    {
       tag=str_dup(luaL_checkstring( LS, 4 ));
    }

    lua_getglobal( LS, "delaytbl");
    TIMER_NODE *tmr=register_lua_timer( val, tag );
    lua_pushlightuserdata( LS, (void *)tmr); 
    lua_newtable( LS );
 
    lua_pushliteral( LS, "tableid");
    lua_getfield( LS, 1, "tableid");
    lua_settable( LS, -3 );

    
    lua_pushliteral( LS, "func");
    lua_pushvalue( LS, 3 );
    lua_settable( LS, -3 );

    lua_settable( LS, -3 );

    return 0;
}

int L_cancel (lua_State *LS)
{
    /* http://pgl.yoyo.org/luai/i/next specifies it is safe
       to modify or clear fields during iteration */
    /* for k,v in pairs(delaytbl) do
            if v.tableid==arg1.tableid then
                unregister_lua_timer(k)
                delaytbl[k]=nil
            end
       end
       */

    /* 1, game object */
    const char *tag=NULL;
    if (!lua_isnone(LS, 2))
    {
        tag=luaL_checkstring( LS, 2 );
        lua_remove( LS, 2 );
    }

    lua_getfield( LS, 1, "tableid"); /* 2, arg1.tableid (game object pointer) */ 
    lua_getglobal( LS, "delaytbl"); /* 3, delaytbl */

    lua_pushnil( LS );
    while ( lua_next(LS, 3) != 0 ) /* pops nil */
    {
        /* key at 4, val at 5 */
        lua_getfield( LS, 5, "tableid");
        if (lua_equal( LS, 6, 2 )==1)
        {
            luaL_checktype( LS, 4, LUA_TLIGHTUSERDATA);
            TIMER_NODE *tmr=(TIMER_NODE *)lua_touserdata( LS, 4);
            if (unregister_lua_timer( tmr, tag ) ) /* return false if tag no match*/
            {
                /* set table entry to nil */
                lua_pushvalue( LS, 4 ); /* push key */
                lua_pushnil( LS );
                lua_settable( LS, 3 );
            }
        }
        lua_pop(LS, 2); /* pop tableid and value */
    }

    return 0;
}

/* Run string.format using args beginning at index 
   Assumes top is the last argument*/
const char *check_fstring( lua_State *LS, int index)
{
    int narg=lua_gettop(LS)-(index-1);

    if ( !(narg==1))
    {
        lua_getglobal( LS, "string");
        lua_getfield( LS, -1, "format");
        /* kill string table */
        lua_remove( LS, -2);
        lua_insert( LS, index );
        lua_call( LS, narg, 1);
    }
    return luaL_checkstring( LS, index);
}

static void GetTracebackFunction (lua_State *LS)
{
    lua_pushliteral (LS, LUA_DBLIBNAME);     /* "debug"   */
    lua_rawget      (LS, LUA_GLOBALSINDEX);    /* get debug library   */

    if (!lua_istable (LS, -1))
    {
        lua_pop (LS, 2);   /* pop result and debug table  */
        lua_pushnil (LS);
        return;
    }

    /* get debug.traceback  */
    lua_pushstring(LS, "traceback");
    lua_rawget    (LS, -2);               /* get traceback function  */

    if (!lua_isfunction (LS, -1))
    {
        lua_pop (LS, 2);   /* pop result and debug table  */
        lua_pushnil (LS);
        return;
    }

    lua_remove (LS, -2);   /* remove debug table, leave traceback function  */
}  /* end of GetTracebackFunction */

int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn)
{
    int error;

    int base = lua_gettop (LS) - iArguments;  /* function index */
    GetTracebackFunction (LS);
    if (lua_isnil (LS, -1))
    {
        lua_pop (LS, 1);   /* pop non-existent function  */
        bugf("pop non-existent function");
        error = lua_pcall (LS, iArguments, iReturn, 0);
    }
    else
    {
        lua_insert (LS, base);  /* put it under chunk and args */
        error = lua_pcall (LS, iArguments, iReturn, base);
        lua_remove (LS, base);  /* remove traceback function */
    }

    return error;
}  /* end of CallLuaWithTraceBack  */
