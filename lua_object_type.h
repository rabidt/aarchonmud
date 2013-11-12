#ifndef LUA_OBJECT_TYPE_H
#define LUA_OBJECT_TYPE_H

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

OBJ_TYPE *CH_init( lua_State *LS);
OBJ_TYPE *OBJ_init( lua_State *LS);
OBJ_TYPE *AREA_init( lua_State *LS);
OBJ_TYPE *ROOM_init( lua_State *LS);
OBJ_TYPE *EXIT_init( lua_State *LS);
OBJ_TYPE *RESET_init( lua_State *LS);
OBJ_TYPE *OBJPROTO_init( lua_State *LS);
OBJ_TYPE *MOBPROTO_init( lua_State *LS);
#endif
