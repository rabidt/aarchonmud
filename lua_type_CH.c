#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"

OBJ_TYPE *CH_type;
static const struct luaL_reg CH_lib [];

static int L_ch_ispc (lua_State *LS)
{
    CHAR_DATA * ud_ch = CH_type->check(CH_type, LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && !IS_NPC( ud_ch ) );
    return 1;
}

static const LUA_PROP_TYPE get_table [] =
{
    {"name", PTYPE_STR,  offsetof(CHAR_DATA, name) , NULL},
    {"hp",   PTYPE_INT,  offsetof(CHAR_DATA, hit)  , NULL},
    {NULL, PTYPE_NONE, NO_OFF, NULL}
};

static const LUA_PROP_TYPE set_table [] =
{
    {"hp", PTYPE_INT,    offsetof(CHAR_DATA, hit), NULL},
    {NULL, PTYPE_NONE, NO_OFF, NULL}
};

static const LUA_PROP_TYPE method_table [] =
{
    {"ispc", PTYPE_FUN, NO_OFF, L_ch_ispc},
    {NULL, PTYPE_NONE, NO_OFF, NULL}
}; 

OBJ_TYPE *CH_init(lua_State *LS)
{
    if (!CH_type)
        CH_type=new_obj_type(
            LS,
            "CH",
            get_table,
            set_table,
            method_table);

    return CH_type;
}
