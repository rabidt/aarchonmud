#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"

#define UD_TABLE_NAME "udtbl"
#define REGISTER_UD_FUNCTION "RegisterUd"
#define MAKE_META_FUNCTION "MakeMetatable"


/* base functionality for lua object types */
static void * check_func( OBJ_TYPE *self,
        lua_State *LS, int index )
{
    lua_getfield(LS, index, "UDTYPE");
    int type=luaL_checkint( LS, -1 );
    lua_pop(LS, 1);
    if ( type != self->udtype )
    {
        luaL_error(LS, "Bad parameter %d. Expected %s. %d %d",
                index, self->type_name, type, self->udtype);
    }

    lua_getfield(LS, index, "tableid");
    luaL_checktype( LS, -1, LUA_TLIGHTUSERDATA);
    void *game_object=lua_touserdata(LS, -1 );
    lua_pop(LS, 1);

    return game_object;
}

static int index_metamethod( lua_State *LS)
{
    OBJ_TYPE *obj=lua_touserdata( LS, lua_upvalueindex(1));
    const char *arg=luaL_checkstring( LS, 2 );

    LUA_PROP_TYPE *get=obj->get_table;
    
    if (!strcmp("UDTYPE", arg) )
    {
        lua_pushinteger( LS, obj->udtype );
        return 1;
    }

    int i;
    for (i=0; get[i].field; i++ )
    {
        if (!strcmp(get[i].field, arg) )
        {
           void *gobj=obj->check(obj, LS, 1 );
           if (get[i].offset != NO_OFF )
           {
               lua_pushinteger( LS,
                       * ( (int *)(gobj+get[i].offset) ) );
               return 1;
           }
           else if (get[i].func)
           {
               int val;
               val=(get[i].func)(LS, gobj);
               return val;
           }
           else
               luaL_error(LS, "BAAAAD");

        }
    }

    LUA_PROP_TYPE *method=obj->method_table;

    for (i=0; method[i].field; i++ )
    {
        if (!strcmp(method[i].field, arg) )
        {
            lua_pushcfunction(LS, method[i].func);
            return 1;
        }
    }

    return 0;
}

static int newindex_metamethod( lua_State *LS )
{
    return 0;
}

static void register_type( OBJ_TYPE *tp,
        lua_State *LS)
{
    luaL_newmetatable(LS, tp->type_name);
    
    lua_pushlightuserdata( LS, ( void *)tp);
    lua_pushcclosure( LS, index_metamethod, 1 );

    lua_setfield( LS, -2, "__index");

    lua_pushlightuserdata( LS, ( void *)tp);
    lua_pushcclosure( LS, newindex_metamethod, 1 );

    lua_setfield( LS, -2, "__newindex");
}

static bool make_func( OBJ_TYPE *self,
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

    luaL_getmetatable (LS, self->type_name);
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


OBJ_TYPE *new_obj_type(
        lua_State *LS,
        const char *type_name,
        const LUA_PROP_TYPE *get_table,
        const LUA_PROP_TYPE *set_table,
        const LUA_PROP_TYPE *method_table)
{
    static int udtype=10;
    udtype=udtype+1;

    /*tbc check for table structure correctness */
    /*check_table(get_table)
      check-table(set_table)
      check_table(method_table)
      */

    OBJ_TYPE *tp=alloc_mem(sizeof(OBJ_TYPE));
    tp->udtype=udtype;
    tp->type_name=type_name;
    tp->set_table=set_table;
    tp->get_table=get_table;
    tp->method_table=method_table;

    tp->check=check_func;
    tp->make=make_func;

    register_type( tp, LS );
    return tp;
}
     
