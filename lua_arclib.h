#ifndef LUA_ARCLIB_H
#define LUA_ARCLIB_H

typedef struct prop_type LUA_PROP_TYPE;

/* base functionality for lua object types */
typedef struct obj_type
{
    char *type_name;
    bool (*make)();

    void *(*check)();

    bool (*is)();

    LUA_PROP_TYPE *get_table;
    LUA_PROP_TYPE *set_table;
    LUA_PROP_TYPE *method_table;

} OBJ_TYPE;

typedef struct lua_extra_val
{
    struct lua_extra_val *next;
    char *name;

    int type;

    char *val;
    bool persist;

} LUA_EXTRA_VAL;

void type_init();

extern OBJ_TYPE *CH_type;
extern OBJ_TYPE *OBJ_type;
extern OBJ_TYPE *AREA_type;
extern OBJ_TYPE *ROOM_type;
extern OBJ_TYPE *EXIT_type;
extern OBJ_TYPE *RESET_type;
extern OBJ_TYPE *OBJPROTO_type;
extern OBJ_TYPE *MOBPROTO_type;
extern OBJ_TYPE *SHOP_type;
extern OBJ_TYPE *PROG_type;
extern OBJ_TYPE *MTRIG_type;
extern OBJ_TYPE *OTRIG_type;
extern OBJ_TYPE *ATRIG_type;
extern OBJ_TYPE *RTRIG_type;

void register_globals( lua_State *LS );

#define check_CH( LS, index) ((CHAR_DATA *)CH_type->check( CH_type, LS, index ))
#define check_OBJ( LS, index) ((OBJ_DATA *)OBJ_type->check( OBJ_type, LS, index ))
#define check_AREA( LS, index) ((AREA_DATA *)AREA_type->check( AREA_type, LS, index))
#define check_ROOM( LS, index) ((ROOM_INDEX_DATA *)ROOM_type->check( ROOM_type, LS, index))
#define check_EXIT( LS, index) ((EXIT_DATA *)EXIT_type->check( EXIT_type, LS, index))
#define check_RESET( LS, index) ((RESET_DATA *)RESET_type->check( RESET_type, LS, index))
#define check_MOBPROTO( LS, index) ((MOB_INDEX_DATA *)MOBPROTO_type->check( MOBPROTO_type, LS, index))
#define check_OBJPROTO( LS, index) ((OBJ_INDEX_DATA *)OBJPROTO_type->check( OBJPROTO_type, LS, index))
#define check_PROG( LS, index) ((PROG_CODE *)PROG_type->check( PROG_type, LS, index))
#define check_MTRIG( LS, index) ((PROG_LIST *)MTRIG_type->check( MTRIG_type, LS, index))
#define check_OTRIG( LS, index) ((PROG_LIST *)OTRIG_type->check( OTRIG_type, LS, index))
#define check_ATRIG( LS, index) ((PROG_LIST *)ATRIG_type->check( ATRIG_type, LS, index))
#define check_RTRIG( LS, index) ((PROG_LIST *)RTRIG_type->check( RTRIG_type, LS, index))
#define check_SHOP( LS, index) ((SHOP_DATA *)SHOP_type->check( SHOP_type, LS, index))


#define make_CH(LS, ch ) CH_type->make( CH_type, LS, ch )
#define make_OBJ(LS, obj) OBJ_type->make( OBJ_type, LS, obj )
#define make_AREA(LS, area) AREA_type->make( AREA_type, LS, area )
#define make_ROOM(LS, room) ROOM_type->make( ROOM_type, LS, room )
#define make_EXIT(LS, exit) EXIT_type->make( EXIT_type, LS, exit )
#define make_RESET(LS, reset) RESET_type->make( RESET_type, LS, reset )
#define make_MOBPROTO(LS, mp) MOBPROTO_type->make( MOBPROTO_type, LS, mp)
#define make_OBJPROTO(LS, op) OBJPROTO_type->make( OBJPROTO_type, LS, op)
#define make_PROG(LS, prog) PROG_type->make( PROG_type, LS, prog)
#define make_MTRIG(LS, trig) MTRIG_type->make( MTRIG_type, LS, trig)
#define make_OTRIG(LS, trig) OTRIG_type->make( OTRIG_type, LS, trig)
#define make_ATRIG(LS, trig) ATRIG_type->make( ATRIG_type, LS, trig)
#define make_RTRIG(LS, trig) RTRIG_type->make( RTRIG_type, LS, trig)
#define make_SHOP(LS, shop) SHOP_type->make( SHOP_type, LS, shop)

#define is_CH(LS, ch ) CH_type->is( CH_type, LS, ch )
#define is_OBJ(LS, obj ) OBJ_type->is( OBJ_type, LS, obj )
#define is_AREA(LS, area ) AREA_type->is( AREA_type, LS, area )
#define is_ROOM(LS, room ) ROOM_type->is( ROOM_type, LS, room )
#define is_MTRIG(LS, trig ) MTRIG_type->is( MTRIG_type, LS, trig )
#define is_OTRIG(LS, trig ) OTRIG_type->is( OTRIG_type, LS, trig )

#endif
