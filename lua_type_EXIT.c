#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"
#include "tables.h"

OBJ_TYPE *EXIT_type;

static int flag (lua_State *LS)
{
    EXIT_DATA *ed=EXIT_type->check(EXIT_type, LS, 1 );
    const char *argument = check_fstring (LS, 2);

    sh_int flag=flag_lookup( argument, exit_flags);
    if ( flag==NO_FLAG )
        luaL_error(LS, "Invalid exit flag: '%s'", argument);

    lua_pushboolean( LS, IS_SET( ed->exit_info, flag));
    return 1;
}

static const LUA_PROP_TYPE get_table [] =
{
    {"key", PTYPE_INT, offsetof(EXIT_DATA, key), NULL},
    {NULL, PTYPE_NONE, NO_OFF, NULL}
};

static const LUA_PROP_TYPE set_table [] =
{
    {NULL, PTYPE_NONE, NO_OFF, NULL}
};

static const LUA_PROP_TYPE method_table [] =
{
    {"flag", PTYPE_NONE, NO_OFF, flag },
    {NULL, PTYPE_NONE, NO_OFF, NULL}
}; 

OBJ_TYPE *EXIT_init(lua_State *LS)
{
    if (!EXIT_type)
        EXIT_type=new_obj_type(
            LS,
            "EXIT",
            get_table,
            set_table,
            method_table);

    return EXIT_type;
}
