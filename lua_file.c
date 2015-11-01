#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_main.h"
#include "lua_file.h"


/* LStbl section */
void LStbl_create( lua_State *LS, LStbl *tbl )
{
    lua_newtable( LS );
    tbl->ref=lua_gettop( LS );
}

void LStbl_release( lua_State *LS, LStbl *tbl )
{
    lua_remove( LS, tbl->ref);
    tbl->ref = LUA_NOREF;
} 

void LStbl_save( lua_State *LS, LStbl *tbl, const char *filename )
{
    lua_getglobal( LS, "LSAVE_table" );
    lua_pushvalue( LS, tbl->ref );
    lua_pushstring( LS, filename );
    lua_call( LS, 2, 0 );
}

void LStbl_kv_string( lua_State *LS, LStbl *tbl, const char *key, const char *val)
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushstring( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}

void LStbl_kv_bool( lua_State *LS, LStbl *tbl, const char *key, const bool val)
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushboolean( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}

void LStbl_kv_int( lua_State *LS, LStbl *tbl, const char *key, const int val)
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushinteger( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}

void LStbl_kv_array( lua_State *LS, LStbl *tbl, const char *key, LSarr *arr)
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushvalue( LS, arr->ref );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}
/* end LStbl section */

/* LSarr section */
void LSarr_create( lua_State *LS, LSarr *arr )
{
    lua_newtable( LS );
    arr->ref=lua_gettop( LS );
}

void LSarr_release( lua_State *LS, LSarr *arr )
{
    lua_remove( LS, arr->ref );
    arr->ref=LUA_NOREF;
}

void LSarr_add_string( lua_State *LS, LSarr *arr, const char *val )
{
    lua_newtable( LS );
    lua_pushstring( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, arr->ref, lua_objlen( LS, arr->ref) + 1 );
}

void LSarr_add_int( lua_State *LS, LSarr *arr, const int val )
{
    lua_newtable( LS );
    lua_pushinteger( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, arr->ref, lua_objlen( LS, arr->ref) + 1 );
}

/* end LSarr section */

/* LLtbl section */
void LLtbl_load( lua_State *LS, LLtbl *tbl, const char *filename )
{
    lua_getglobal( LS, "LLOAD_table" );
    lua_pushstring( LS, filename );
    lua_call(LS, 1, 1);
    tbl->ref = lua_gettop(LS);
}

void LLtbl_release( lua_State *LS, LLtbl *tbl )
{
    lua_remove( LS, tbl->ref );
    tbl->ref=LUA_NOREF;
}

int LLtbl_get_kv_int( lua_State *LS, LLtbl *tbl, const char *key )
{
    int rtn;
    lua_getfield( LS, tbl->ref, key );
    rtn=luaL_checkinteger( LS, -1 );
    lua_pop( LS, 1 );
    return rtn;  
}

const char *LLtbl_get_kv_str( lua_State *LS, LLtbl *tbl, const char *key) 
{
    const char *rtn;
    lua_getfield( LS, tbl->ref, key );
    rtn=str_dup(luaL_checkstring( LS, -1 ));
    lua_pop( LS, 1 );
    return rtn;    
}

void LLtbl_get_kv_table( lua_State *LS, LLtbl *tbl, const char *key, LLtbl *subtbl)
{
    lua_getfield( LS, tbl->ref, key );
    luaL_checktype( LS, -1, LUA_TTABLE );
    subtbl->ref=lua_gettop( LS );
}

const char *LLtbl_get_iv_str( lua_State *LS, LLtbl *tbl, int index)
{
    const char *rtn;
    lua_rawgeti( LS, tbl->ref, index );
    rtn=str_dup(luaL_checkstring( LS, -1 ));
    lua_pop( LS, 1);
    return rtn;
}
    
bool LLtbl_i_exists( lua_State *LS, LLtbl *tbl, int index )
{
    bool rtn;
    lua_rawgeti( LS, tbl->ref, index );
    if ( lua_isnil( LS, -1 ) )
        rtn = FALSE;
    else
        rtn = TRUE;
    lua_pop(LS, 1);
    return rtn;
}


/* end LLtbl section */
