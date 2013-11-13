#ifndef LUA_ARCLIB_H
#define LUA_ARCLIB_H

typedef struct prop_type LUA_PROP_TYPE;

/* base functionality for lua object types */
typedef struct obj_type
{
    int udtype; /* unique type ID generated at init*/
    char *type_name;
    bool (*make)();

    void *(*check)();

    bool (*is)();

    LUA_PROP_TYPE *get_table;
    LUA_PROP_TYPE *set_table;
    LUA_PROP_TYPE *method_table;

} OBJ_TYPE;

void type_init();

extern OBJ_TYPE *CH_type;
extern OBJ_TYPE *OBJ_type;
extern OBJ_TYPE *AREA_type;
extern OBJ_TYPE *ROOM_type;
extern OBJ_TYPE *EXIT_type;
extern OBJ_TYPE *RESET_type;
extern OBJ_TYPE *OBJPROTO_type;
extern OBJ_TYPE *MOBPROTO_type;

void register_globals( lua_State *LS );

#define check_CH( LS, index) ((CHAR_DATA *)CH_type->check( CH_type, LS, index ))
#define make_CH(LS, ch ) CH_type->make( CH_type, LS, ch )
#define is_CH(LS, ch ) CH_type->is( CH_type, LS, ch )

#define check_OBJ( LS, obj ) ((OBJ_DATA *)OBJ_type->check( OBJ_type, LS, obj ))
#define make_OBJ(LS, obj ) OBJ_type->make( OBJ_type, LS, obj )

#define check_AREA( LS, area ) ((AREA_DATA *)AREA_type->check( AREA_type, LS, area))
#define make_AREA( LS, area) ((AREA_DATA *)AREA_type->check( AREA_type, LS, area ))

#define make_MOBPROTO( LS, mp) ((MOB_INDEX_DATA *)MOBPROTO_type->check( MOBPROTO_type, LS, mp))

#define make_OBJPROTO( LS, op) ((OBJ_INDEX_DATA *)OBJPROTO_type->check( OBJPROTO_type, LS, op))

#define make_ROOM( LS, room) ((ROOM_INDEX_DATA *)ROOM_type->check( ROOM_type, LS, room))

#endif
