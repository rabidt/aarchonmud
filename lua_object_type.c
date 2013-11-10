#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"

#define UD_TABLE_NAME "udtbl"
#define REGISTER_UD_FUNCTION "RegisterUd"


/* base functionality for lua object types */
static void * check_func( OBJ_TYPE *self,
        lua_State *LS, int index )
{
    lua_getfield(LS, index, "UDTYPE");
    int type=luaL_checkint( LS, -1 );
    lua_pop(LS, 1);
    if ( type != self->udtype )
    {
        luaL_error(LS, "Bad parameter %d. Expected %s.",
                index, self->typename);
    }

    lua_getfield(LS, index, "tableid");
    luaL_checktype( LS, -1, LUA_TLIGHTUSERDATA);
    void *game_object=lua_touserdata(LS, -1 );
    lua_pop(LS, 1);
}

static void register_type( OBJ_TYPE *self,
        lua_State *LS)
{
    luaL_newmetatable(LS, self->metatable_name);
    luaL_register( LS, NULL, self->metatable);
}

static bool make_type( OBJ_TYPE *self,
        lua_State *LS, void *game_obj)
{
    /* see if it exists already */
    lua_getglobal( LS, UD_TABLE_NAME);
    if ( lua_isnil( LS, -1) )
    {
        bugf("udtbl is nil in make_ud_table.");
        return FALSE;
    }

    lua_pushlightuserdata(LS, game_obj);
    lua_gettable( LS, -2);
    lua_remove( LS, -2); /* don't need udtbl anymore */

    if ( !lua_isnil(LS, -1) )
    {
        /* already exists, now at top of stack */
        return TRUE;
    }
    lua_remove(LS, -1); // kill the nil

    lua_newtable( LS);

    luaL_getmetatable (LS, self->metatable_name);
    lua_setmetatable (LS, -2);  /* set metatable for object data */
    lua_pushstring( LS, "tableid");

    lua_pushlightuserdata( LS, game_obj);
    lua_rawset( LS, -3 );

    lua_getfield( LS, LUA_GLOBALSINDEX, REGISTER_UD_FUNCTION);
    lua_pushvalue( LS, -2);
    if (CallLuaWithTraceBack( LS, 1, 1) )
    {
        bugf ( "Error registering UD:\n %s",
                lua_tostring(LS, -1));
        return FALSE;
    }

    /* get rid of our original table, register send back a new version */
    lua_remove( LS, -2 );

    return TRUE;
}

static struct luaL_reg *new_metatable( OBJ_TYPE *self )
{
    struct luaL_reg mt [] =
    {
        {"__eq", self->eq_func },
        { NULL, NULL}
    };
}
