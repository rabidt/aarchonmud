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
#define INDENT_SPACES 2

extern LUAREF STRING_FORMAT;

struct lua_save_file
{
    FILE *fp;
    int indent_level;
};

/* LSF section */
LSF *LSF_open(const char *filename)
{
    FILE *fp=fopen(filename, "w");
    if (!fp)
    {
        bugf("Couldn't open lua file: %s", filename);
        return NULL;
    }
    LSF *lsfp=alloc_mem(sizeof(LSF));


    fputs("return {\n", fp);    
    lsfp->fp=fp;
    lsfp->indent_level=1;
    return lsfp;
}

void LSF_close(LSF *lsfp)
{
    if (!lsfp->fp)
    {
        bugf("LSF_close: no file pointer");
        return;
    }
    
    if (lsfp->indent_level != 1)
    {
        bugf("LSF_close: indent level not back at baseline");
    }
    else
    {
        fputs("}\n", lsfp->fp);
    }
    
    fclose(lsfp->fp);
    free_mem(lsfp, sizeof(LSF));

    return;
}

static void write_indent(LSF *lsfp)
{
    fprintf(lsfp->fp, "%*s", lsfp->indent_level * INDENT_SPACES, "");
}

static void write_key_str(LSF *lsfp, const char *key)
{
    push_ref(LS, STRING_FORMAT);
    lua_pushstring(LS, "[%q] = ");
    lua_pushstring(LS, key);
    lua_call(LS, 2, 1);
    fputs( luaL_checkstring(LS, -1), lsfp->fp);
    lua_pop(LS,1);
}

static void write_val_str(LSF *lsfp, const char *val)
{
    push_ref(LS, STRING_FORMAT);
    lua_pushstring(LS, "%q,\n");
    lua_pushstring(LS, val);
    lua_call(LS, 2, 1);
    fputs( luaL_checkstring(LS, -1), lsfp->fp);
    lua_pop(LS,1);
}

static void write_key_int(LSF *lsfp, const int key)
{
    fprintf( lsfp->fp, "[%d] = ", key);
}

static void write_val_int(LSF *lsfp, const int val)
{
    fprintf( lsfp->fp, "%d,\n", val);
}

static void write_val_bool(LSF *lsfp, const bool val)
{
    fprintf( lsfp->fp, "%s,\n", val ? "true" : "false");
}

static void write_val_tbl(LSF *lsfp)
{
    //fputs("\n", lsfp->fp);
    //write_indent(lsfp);
    fputs("{\n", lsfp->fp);
    lsfp->indent_level+=1;
}

void LSF_add_str( LSF *lsfp, const char *val)
{
    write_indent(lsfp);
    write_val_str(lsfp, val);
}

void LSF_add_int( LSF *lsfp, const int val)
{
    write_indent(lsfp);
    write_val_int(lsfp, val);
}

void LSF_add_tbl( LSF *lsfp)
{
    write_indent(lsfp);
    write_val_tbl(lsfp);
}

void LSF_kv_str( LSF *lsfp, const char *key, const char *val)
{
    write_indent(lsfp);
    write_key_str(lsfp, key);
    write_val_str(lsfp, val);
}

void LSF_kv_bool( LSF *lsfp, const char *key, const bool val)
{
    write_indent(lsfp);
    write_key_str(lsfp, key);
    write_val_bool(lsfp, val);
}

void LSF_kv_int( LSF *lsfp, const char *key, const int val)
{
    write_indent(lsfp);
    write_key_str(lsfp, key);
    write_val_int(lsfp, val);
}

void LSF_kv_tbl( LSF *lsfp, const char *key )
{
    write_indent(lsfp);
    write_key_str(lsfp, key);
    write_val_tbl(lsfp);
}

void LSF_end_tbl( LSF *lsfp)
{
    lsfp->indent_level-=1;
    write_indent(lsfp);
    fputs("},\n", lsfp->fp);
}

void LSF_kv_flags( LSF *lsfp, const char *key, const struct flag_type *flag_table, const tflag f)
{
    LSF_kv_tbl( lsfp, key);

    int i=0;
    for (i=0; flag_table[i].name; i++)
    {
        if (IS_SET(f, flag_table[i].bit))
        {
            LSF_add_str(lsfp, flag_table[i].name);
        }
    }

    LSF_end_tbl(lsfp);
}

void LSF_iv_int( LSF *lsfp, int index, int value )
{
    write_indent(lsfp);
    write_key_int(lsfp, index);
    write_val_int(lsfp, value);
}
/* end LSF section */

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

void LLtbl_get_kv_flags( LLtbl *tbl, const char *key, const struct flag_type *flag_table, tflag f) 
{
    LLtbl flag_array;
    LLtbl_get_kv_tbl(tbl, key, &flag_array);

    int ind;
    for (ind=1 ; LLtbl_i_exists(&flag_array, ind) ; ind++)
    {
        const char *flagstr=LLtbl_get_iv_str(&flag_array, ind);
        int flagval=flag_lookup( flagstr, flag_table);
        if (flagval == NO_FLAG)
        {
            bugf("Unrecognized flag: %s", flagstr);
        }
        else
        {
            SET_BIT( f, flagval);
        }
        free_string(flagstr);
    }

    LLtbl_release(&flag_array);
}

bool LLtbl_k_exists( LLtbl *tbl, const char *key)
{
    bool rtn;
    lua_getfield( LS, tbl->ref, key );
    if ( lua_isnil( LS, -1 ) )
        rtn = FALSE;
    else
        rtn = TRUE;
    lua_pop(LS, 1);
    return rtn;
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
