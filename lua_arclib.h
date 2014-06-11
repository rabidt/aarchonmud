#ifndef LUA_ARCLIB_H
#define LUA_ARCLIB_H

struct lua_prop_type;
typedef struct lua_obj_type
{
    const char *type_name;

    struct lua_prop_type * const get_table;
    struct lua_prop_type * const set_table;
    struct lua_prop_type * const method_table;

    size_t size;
} LUA_OBJ_TYPE;

typedef struct lua_extra_val
{
    struct lua_extra_val *next;
    char *name;

    int type;

    char *val;
    bool persist;

} LUA_EXTRA_VAL;

void type_init();

extern LUA_OBJ_TYPE CH_type;
extern LUA_OBJ_TYPE OBJ_type;
extern LUA_OBJ_TYPE AREA_type;
extern LUA_OBJ_TYPE ROOM_type;
extern LUA_OBJ_TYPE EXIT_type;
extern LUA_OBJ_TYPE RESET_type;
extern LUA_OBJ_TYPE OBJPROTO_type;
extern LUA_OBJ_TYPE MOBPROTO_type;
extern LUA_OBJ_TYPE SHOP_type;
extern LUA_OBJ_TYPE PROG_type;
extern LUA_OBJ_TYPE MTRIG_type;
extern LUA_OBJ_TYPE OTRIG_type;
extern LUA_OBJ_TYPE ATRIG_type;
extern LUA_OBJ_TYPE RTRIG_type;
extern LUA_OBJ_TYPE AFFECT_type;

void register_globals( lua_State *LS );
bool lua_make_type( LUA_OBJ_TYPE *tp,
                lua_State *LS, void *game_obj);
bool lua_is_type( LUA_OBJ_TYPE *tp,
                lua_State *LS, int arg );
void * lua_check_type( LUA_OBJ_TYPE *tp,
                lua_State *LS, int index );

#define declf( ltype, ctype ) \
ctype * check_ ## ltype ( lua_State *LS, int index ); \
bool    is_ ## ltype ( lua_State *LS, int index ); \
bool    push_ ## ltype ( lua_State *LS, int index );\
ctype * new_ ## ltype (void) ;\
void    free_ ## ltype ( ctype * ud );\
int     count_ ## ltype (void) ;


declf(CH, CHAR_DATA)
declf(OBJ, OBJ_DATA)
declf(AREA, AREA_DATA)
declf(ROOM, ROOM_INDEX_DATA)
declf(EXIT, EXIT_DATA)
declf(RESET, RESET_DATA)
declf(MOBPROTO, MOB_INDEX_DATA)
declf(OBJPROTO, OBJ_INDEX_DATA)
declf(PROG, PROG_CODE)
declf(MTRIG, PROG_LIST)
declf(OTRIG, PROG_LIST)
declf(ATRIG, PROG_LIST)
declf(RTRIG, PROG_LIST)
declf(SHOP, SHOP_DATA)
declf(AFFECT, AFFECT_DATA)
#undef declf


#endif
