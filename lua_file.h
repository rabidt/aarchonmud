#ifndef LUA_SAVE_H
#define LUA_SAVE_H

typedef struct 
{
    int ref;
} LStbl;

typedef struct
{
    int ref;
} LSarr;

typedef struct
{
    int ref;
} LLtbl;

void LStbl_create( lua_State *LS, LStbl *tbl );
void LStbl_release( lua_State *LS, LStbl *tbl );
void LStbl_save( lua_State *LS, LStbl *tbl, const char *filename );
void LStbl_kv_str( lua_State *LS, LStbl *tbl, const char *key, const char *val );
void LStbl_kv_int( lua_State *LS, LStbl *tbl, const char *key, const int val );
void LStbl_kv_bool( lua_State *LS, LStbl *tbl, const char *key, const bool val );
void LStbl_kv_tbl( lua_State *LS, LStbl *tbl, const char *key, LStbl *subtbl );
void LStbl_kv_arr( lua_State *LS, LStbl *tbl, const char *key, LSarr *arr );

void LSarr_create( lua_State *LS, LSarr *arr );
void LSarr_release( lua_State *LS, LSarr *arr );
void LSarr_add_str( lua_State *LS, LSarr *arr, const char *val );
void LSarr_add_int( lua_State *LS, LSarr *arr, const int val );
void LSarr_add_tbl( lua_State *LS, LSarr *arr, LStbl *tbl );
/* to complete as needed
void LSarr_add_bool( lua_State *LS, LSarr *arr, const bool val );
void LSarr_add_array( lua_State *LS, LSarr *arr, LSarr *subarr );
*/
void LLtbl_load( lua_State *LS, LLtbl *tbl, const char *filename );
void LLtbl_release( lua_State *LS, LLtbl *tbl );
const char *LLtbl_get_kv_str( lua_State *LS, LLtbl *tbl, const char *key);
int LLtbl_get_kv_int( lua_State *LS, LLtbl *tbl, const char *key);
void LLtbl_get_kv_tbl( lua_State *LS, LLtbl *tbl, const char *key, LLtbl *subtbl);
const char *LLtbl_get_iv_str( lua_State *LS, LLtbl *tbl, int index);
int LLtbl_get_iv_int( lua_State *LS, LLtbl *tbl, int index);
void LLtbl_get_iv_tbl( lua_State *LS, LLtbl *tbl, int index, LLtbl *subtbl);

bool LLtbl_i_exists( lua_State *LS, LLtbl *tbl, int index);
#endif
