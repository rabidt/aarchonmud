#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"

OBJ_TYPE *ROOM_type;

static const LUA_PROP_TYPE get_table [] =
{
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

OBJ_TYPE *ROOM_init(lua_State *LS)
{
    if (!ROOM_type)
        ROOM_type=new_obj_type(
            LS,
            "ROOM",
            get_table,
            set_table,
            method_table);

    return ROOM_type;
}
