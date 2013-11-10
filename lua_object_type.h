#ifndef LUA_OBJECT_TYPE_H
#define LUA_OBJECT_TYPE_H

#define NO_OFF -1
typedef const char * LUA_HELP_FUN ( const char *argument );
typedef void * CHECK_FUN ( lua_State *LS, int index );
typedef struct prop_type
{
    char *field;
    size_t offset;
    int *func;
} LUA_PROP_TYPE;

/* base functionality for lua object types */
typedef struct obj_type
{
    int udtype; /* unique type ID */
    char *typename;
    char *metatable_name;
    struct luaL_reg *metatable;
    void *register_type;
    bool *make;

    int *get;
    int *set;

    int *eq_func;
    int *tostring_func;
    int *index_func;
    int *newindex_func;

    LUA_PROP_TYPE *get_table;
    LUA_PROP_TYPE *set_table;
    LUA_PROP_TYPE *method_table;

    LUA_HELP_FUN *help;
} OBJ_TYPE;


#endif
