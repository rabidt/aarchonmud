#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"

OBJ_TYPE *AREA_type=NULL;

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

OBJ_TYPE *AREA_init(lua_State *LS)
{
    if (!AREA_type)
        AREA_type=new_obj_type(
            LS,
            "AREA",
            get_table,
            set_table,
            method_table);

    return AREA_type;
}
