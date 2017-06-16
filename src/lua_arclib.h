#ifndef LUA_ARCLIB_H
#define LUA_ARCLIB_H


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

const char * arclib_type_name( LUA_OBJ_TYPE *type );
bool arclib_push( LUA_OBJ_TYPE *type, lua_State *LS, void *ud );
void * arclib_check( LUA_OBJ_TYPE *type, lua_State *LS, int index );
bool arclib_valid( void * );

void init_script_db( void );
void close_script_db( void );
void register_globals( lua_State *LS );
LUA_EXTRA_VAL *new_luaval( int type, const char *name, const char *val, bool persist );
void free_luaval( LUA_EXTRA_VAL *luaval );
void cleanup_uds( void );

#define DECLARETYPEFUNCS( LTYPE, CTYPE ) \
    CTYPE * check_ ## LTYPE ( lua_State *LS, int index ); \
    bool    is_ ## LTYPE ( lua_State *LS, int index ); \
    bool    push_ ## LTYPE ( lua_State *LS, CTYPE *ud );\
    CTYPE * alloc_ ## LTYPE (void) ;\
    void    free_ ## LTYPE ( CTYPE * ud );\
    bool    valid_ ## LTYPE ( CTYPE *ud );\
    int     count_ ## LTYPE ( void )

DECLARETYPEFUNCS(CH, CHAR_DATA);
DECLARETYPEFUNCS(OBJ, OBJ_DATA);
DECLARETYPEFUNCS(AREA, AREA_DATA);
DECLARETYPEFUNCS(ROOM, ROOM_INDEX_DATA);
DECLARETYPEFUNCS(EXIT, EXIT_DATA);
DECLARETYPEFUNCS(RESET, RESET_DATA);
DECLARETYPEFUNCS(MOBPROTO, MOB_INDEX_DATA);
DECLARETYPEFUNCS(OBJPROTO, OBJ_INDEX_DATA);
DECLARETYPEFUNCS(PROG, PROG_CODE);
DECLARETYPEFUNCS(MTRIG, PROG_LIST);
DECLARETYPEFUNCS(OTRIG, PROG_LIST);
DECLARETYPEFUNCS(ATRIG, PROG_LIST);
DECLARETYPEFUNCS(RTRIG, PROG_LIST);
DECLARETYPEFUNCS(SHOP, SHOP_DATA);
DECLARETYPEFUNCS(AFFECT, AFFECT_DATA);
DECLARETYPEFUNCS(HELP, HELP_DATA);
DECLARETYPEFUNCS(DESCRIPTOR, DESCRIPTOR_DATA);
DECLARETYPEFUNCS(BOSSACHV, BOSSACHV);
DECLARETYPEFUNCS(BOSSREC, BOSSREC);
#undef DECLARETYPEFUNCS


#endif
