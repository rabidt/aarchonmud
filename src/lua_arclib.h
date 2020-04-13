#ifndef LUA_ARCLIB_H
#define LUA_ARCLIB_H

struct lua_arclib_obj
{
    unsigned char magic_id_[2];
    int ref_;
};

typedef struct lua_extra_val
{
    struct lua_extra_val *next;
    const char *name;

    int type;

    const char *val;
    bool persist;

} LUA_EXTRA_VAL;

void arclib_type_init( lua_State *LS );

typedef struct lua_obj_type LUA_OBJ_TYPE;

extern LUA_OBJ_TYPE * const p_CH_type;
extern LUA_OBJ_TYPE * const p_OBJ_type;
extern LUA_OBJ_TYPE * const p_AREA_type;
extern LUA_OBJ_TYPE * const p_ROOM_type;
extern LUA_OBJ_TYPE * const p_EXIT_type;
extern LUA_OBJ_TYPE * const p_RESET_type;
extern LUA_OBJ_TYPE * const p_OBJPROTO_type;
extern LUA_OBJ_TYPE * const p_MOBPROTO_type;
extern LUA_OBJ_TYPE * const p_SHOP_type;
extern LUA_OBJ_TYPE * const p_PROG_type;
extern LUA_OBJ_TYPE * const p_MTRIG_type;
extern LUA_OBJ_TYPE * const p_OTRIG_type;
extern LUA_OBJ_TYPE * const p_ATRIG_type;
extern LUA_OBJ_TYPE * const p_RTRIG_type;
extern LUA_OBJ_TYPE * const p_AFFECT_type;
extern LUA_OBJ_TYPE * const p_HELP_type;
extern LUA_OBJ_TYPE * const p_DESCRIPTOR_type;
extern LUA_OBJ_TYPE * const p_BOSSACHV_type;
extern LUA_OBJ_TYPE * const p_BOSSREC_type;

const char * arclib_type_name( LUA_OBJ_TYPE *ltype );
bool arclib_push( LUA_OBJ_TYPE *ltype, lua_State *LS, void *ud );
void * arclib_check( LUA_OBJ_TYPE *ltype, lua_State *LS, int index );
bool arclib_valid( LUA_OBJ_TYPE *ltype, void * );

bool lua_arclib_diag(BUFFER *output);

int GetLuaGameObjectCount( void );

void init_script_db( void );
void close_script_db( void );
void register_globals( lua_State *LS );
LUA_EXTRA_VAL *new_luaval( int type, const char *name, const char *val, bool persist );
void free_luaval( LUA_EXTRA_VAL *luaval );

void run_delayed_function( TIMER_NODE *tmr );

#define DECLARETYPEFUNCS( LTYPE, CTYPE ) \
    CTYPE * check_ ## LTYPE ( lua_State *LS, int index ); \
    bool    is_ ## LTYPE ( lua_State *LS, int index ); \
    bool    push_ ## LTYPE ( lua_State *LS, CTYPE *ud ); \
    void    lua_init_ ## LTYPE ( CTYPE *p ); \
    void    lua_deinit_ ## LTYPE ( CTYPE *p ); \
    bool    valid_ ## LTYPE ( CTYPE *ud );\
    int     count_ ## LTYPE ( void )

DECLARETYPEFUNCS(CH, struct char_data);
DECLARETYPEFUNCS(OBJ, struct obj_data);
DECLARETYPEFUNCS(AREA, struct area_data);
DECLARETYPEFUNCS(ROOM, struct room_index_data);
DECLARETYPEFUNCS(EXIT, struct exit_data);
DECLARETYPEFUNCS(RESET, struct reset_data);
DECLARETYPEFUNCS(MOBPROTO, struct mob_index_data);
DECLARETYPEFUNCS(OBJPROTO, struct obj_index_data);
DECLARETYPEFUNCS(PROG, struct prog_code);
DECLARETYPEFUNCS(MTRIG, struct prog_list);
DECLARETYPEFUNCS(OTRIG, struct prog_list);
DECLARETYPEFUNCS(ATRIG, struct prog_list);
DECLARETYPEFUNCS(RTRIG, struct prog_list);
DECLARETYPEFUNCS(SHOP, struct shop_data);
DECLARETYPEFUNCS(AFFECT, struct affect_data);
DECLARETYPEFUNCS(HELP, struct help_data);
DECLARETYPEFUNCS(DESCRIPTOR, struct descriptor_data);
DECLARETYPEFUNCS(BOSSACHV, struct boss_achieve_entry);
DECLARETYPEFUNCS(BOSSREC, struct boss_achieve_record);
#undef DECLARETYPEFUNCS


#endif
