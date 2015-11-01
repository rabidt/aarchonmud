#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_main.h"
#include "lua_file.h"

/* just some laziness here */
#define LS g_mud_LS

/* LStbl section */
void LStbl_create( LStbl *tbl )
{
    lua_newtable( LS );
    tbl->ref=lua_gettop( LS );
}

void LStbl_release( LStbl *tbl )
{
    lua_remove( LS, tbl->ref);
    tbl->ref = LUA_NOREF;
} 

void LStbl_save( LStbl *tbl, const char *filename )
{
    lua_getglobal( LS, "LSAVE_table" );
    lua_pushvalue( LS, tbl->ref );
    lua_pushstring( LS, filename );
    lua_call( LS, 2, 0 );
}

void LStbl_kv_str( LStbl *tbl, const char *key, const char *val)
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushstring( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}

void LStbl_kv_bool( LStbl *tbl, const char *key, const bool val)
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushboolean( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}

void LStbl_kv_int( LStbl *tbl, const char *key, const int val)
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushinteger( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}

void LStbl_kv_arr( LStbl *tbl, const char *key, LSarr *arr)
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushvalue( LS, arr->ref );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}

void LStbl_kv_tbl( LStbl *tbl, const char *key, LStbl *subtbl )
{
    lua_newtable( LS );
    lua_pushstring( LS, key );
    lua_setfield( LS, -2, "LKEY" );
    lua_pushvalue( LS, subtbl->ref );
    lua_rawseti( LS, tbl->ref, lua_objlen( LS, tbl->ref) + 1 );
}

void LStbl_kv_flags( LStbl *tbl, const char *key, const struct flag_type *flag_table, const tflag f)
{
    LSarr flags;
    LSarr_create( &flags );
    LStbl_kv_arr( tbl, key, &flags );

    int i=0;
    for (i=0; flag_table[i].name; i++)
    {
        if (IS_SET(f, flag_table[i].bit))
        {
            LSarr_add_str( &flags, flag_table[i].name);
        }
    }
    LSarr_release( &flags);
}
/* end LStbl section */

/* LSarr section */
void LSarr_create( LSarr *arr )
{
    lua_newtable( LS );
    arr->ref=lua_gettop( LS );
}

void LSarr_release( LSarr *arr )
{
    lua_remove( LS, arr->ref );
    arr->ref=LUA_NOREF;
}

void LSarr_add_str( LSarr *arr, const char *val )
{
    lua_newtable( LS );
    lua_pushstring( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, arr->ref, lua_objlen( LS, arr->ref) + 1 );
}

void LSarr_add_int( LSarr *arr, const int val )
{
    lua_newtable( LS );
    lua_pushinteger( LS, val );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, arr->ref, lua_objlen( LS, arr->ref) + 1 );
}

void LSarr_add_tbl( LSarr *arr, LStbl *tbl )
{
    lua_newtable( LS );
    lua_pushvalue( LS, tbl->ref );
    lua_setfield( LS, -2, "LVAL" );
    lua_rawseti( LS, arr->ref, lua_objlen( LS, arr->ref) + 1 );
}

/* end LSarr section */

/* LLtbl section */
void LLtbl_load( LLtbl *tbl, const char *filename )
{
    lua_getglobal( LS, "LLOAD_table" );
    lua_pushstring( LS, filename );
    lua_call(LS, 1, 1);
    tbl->ref = lua_gettop(LS);
}

void LLtbl_release( LLtbl *tbl )
{
    lua_remove( LS, tbl->ref );
    tbl->ref=LUA_NOREF;
}

int LLtbl_get_kv_int( LLtbl *tbl, const char *key )
{
    int rtn;
    lua_getfield( LS, tbl->ref, key );
    rtn=luaL_checkinteger( LS, -1 );
    lua_pop( LS, 1 );
    return rtn;  
}

const char *LLtbl_get_kv_str( LLtbl *tbl, const char *key) 
{
    const char *rtn;
    lua_getfield( LS, tbl->ref, key );
    rtn=str_dup(luaL_checkstring( LS, -1 ));
    lua_pop( LS, 1 );
    return rtn;    
}

void LLtbl_get_kv_tbl( LLtbl *tbl, const char *key, LLtbl *subtbl)
{
    lua_getfield( LS, tbl->ref, key );
    luaL_checktype( LS, -1, LUA_TTABLE );
    subtbl->ref=lua_gettop( LS );
}

const char *LLtbl_get_iv_str( LLtbl *tbl, int index)
{
    const char *rtn;
    lua_rawgeti( LS, tbl->ref, index );
    rtn=str_dup(luaL_checkstring( LS, -1 ));
    lua_pop( LS, 1);
    return rtn;
}

int LLtbl_get_iv_int( LLtbl *tbl, int index)
{
    int rtn;
    lua_rawgeti( LS, tbl->ref, index);
    rtn=luaL_checkinteger( LS, -1);
    lua_pop(LS,1);
    return rtn;
}

void LLtbl_get_iv_tbl( LLtbl *tbl, int index, LLtbl *subtbl)
{
    lua_rawgeti( LS, tbl->ref, index);
    luaL_checktype( LS, -1, LUA_TTABLE );
    subtbl->ref=lua_gettop( LS );
}
    
bool LLtbl_i_exists( LLtbl *tbl, int index )
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
