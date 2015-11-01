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

void LStbl_create( LStbl *tbl );
void LStbl_release( LStbl *tbl );
void LStbl_save( LStbl *tbl, const char *filename );
void LStbl_kv_str( LStbl *tbl, const char *key, const char *val );
void LStbl_kv_int( LStbl *tbl, const char *key, const int val );
void LStbl_kv_bool( LStbl *tbl, const char *key, const bool val );
void LStbl_kv_tbl( LStbl *tbl, const char *key, LStbl *subtbl );
void LStbl_kv_arr( LStbl *tbl, const char *key, LSarr *arr );
void LStbl_kv_flags( LStbl *tbl, const char *key, const struct flag_type *flag_table, const tflag f);

void LSarr_create( LSarr *arr );
void LSarr_release( LSarr *arr );
void LSarr_add_str( LSarr *arr, const char *val );
void LSarr_add_int( LSarr *arr, const int val );
void LSarr_add_tbl( LSarr *arr, LStbl *tbl );
/* to complete as needed
void LSarr_add_bool( LSarr *arr, const bool val );
void LSarr_add_array( LSarr *arr, LSarr *subarr );
*/
void LLtbl_load( LLtbl *tbl, const char *filename );
void LLtbl_release( LLtbl *tbl );
const char *LLtbl_get_kv_str( LLtbl *tbl, const char *key);
int LLtbl_get_kv_int( LLtbl *tbl, const char *key);
void LLtbl_get_kv_tbl( LLtbl *tbl, const char *key, LLtbl *subtbl);
const char *LLtbl_get_iv_str( LLtbl *tbl, int index);
int LLtbl_get_iv_int( LLtbl *tbl, int index);
void LLtbl_get_iv_tbl( LLtbl *tbl, int index, LLtbl *subtbl);

bool LLtbl_i_exists( LLtbl *tbl, int index);
#endif
