#ifndef LUA_SAVE_H
#define LUA_SAVE_H

typedef struct lua_save_file LSF;

LSF *LSF_open(const char *filename);
void LSF_close(LSF *lsfp);

void LSF_add_str( LSF *lsfp, const char *val );
void LSF_add_int( LSF *lsfp, const int val );
void LSF_add_tbl( LSF *lsfp );

void LSF_kv_str( LSF *lsfp, const char *key, const char *val );
void LSF_kv_int( LSF *lsfp, const char *key, const int val );
void LSF_kv_bool( LSF *lsfp, const char *key, const bool val );
void LSF_kv_tbl( LSF *lsfp, const char *key );
void LSF_kv_flags( LSF *lsfp, const char *key, const struct flag_type *flag_table, const tflag f);
void LSF_iv_int( LSF *lsfp, int index, int value);
void LSF_end_tbl( LSF *lsfp );
#if 0
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
#endif
