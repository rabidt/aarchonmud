#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"

OBJ_TYPE *RESET_type;

static int get_command(lua_State *LS, RESET_DATA *rd )
{
    static char buf[2];
    sprintf(buf, "%c", rd->command);
    lua_pushstring(LS, buf);
    return 1;
}

static const LUA_PROP_TYPE get_table [] =
{
    {"command", PTYPE_STR, NO_OFF, get_command},
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

OBJ_TYPE *RESET_init(lua_State *LS)
{
    RESET_type=new_obj_type(
            LS,
            "RESET",
            get_table,
            set_table,
            method_table);

    return RESET_type;
}
