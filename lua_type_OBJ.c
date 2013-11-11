#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"

OBJ_TYPE *OBJ_type;

static const LUA_PROP_TYPE get_table [] =
{
    {"name", PTYPE_STR,  offsetof(OBJ_DATA, name) , NULL},
    {NULL, PTYPE_NONE, NO_OFF, NULL}
};

static const LUA_PROP_TYPE set_table [] =
{
    {NULL, PTYPE_NONE, NO_OFF, NULL}
};

static const LUA_PROP_TYPE method_table [] =
{
    {NULL, PTYPE_NONE, NO_OFF, NULL}
}; 

OBJ_TYPE *OBJ_init(lua_State *LS)
{
    if (!OBJ_type)
        OBJ_type=new_obj_type(
            LS,
            "OBJ",
            get_table,
            set_table,
            method_table);

    return OBJ_type;
}
