extern "C" {
#include "../../lua_main.h"
}

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <catch.hpp>

void handle_arclua_assert(const char *cond, const char *func, const char *file, unsigned line)
{
   FAIL("arclua assert failed [" << cond << "] " << file << "::" << func << "@" << line);
}

TEST_CASE("open_lua")
{
    open_lua();
}

/* Returns the count of objects in registry that are equal to value at given index */
static int get_ref_count(lua_State *LS, int index)
{
    if (index < 0)
    {
        index = lua_gettop(LS) + index + 1;
    }

    int cnt = 0;
    lua_pushnil(LS);
    while (lua_next(LS, LUA_REGISTRYINDEX) != 0) /* pops nil */
    {
        if (1 == lua_equal(LS, -1, index))
        {
            ++cnt;
        }
        lua_pop(LS, 1); /* pop val */
    }
    return cnt;
}

TEST_CASE("LUAREF")
{
    lua_State *LS = luaL_newstate();
    luaL_openlibs(LS);

    SECTION("save_luaref")
    {
        LUAREF *ref = new_luaref();

        lua_pushinteger(LS, 4321);
        REQUIRE(1 == lua_gettop(LS));

        save_luaref(LS, -1, ref);
        REQUIRE(1 == lua_gettop(LS));
        REQUIRE(1 == get_ref_count(LS, -1));

        lua_pushinteger(LS, 4322);
        save_luaref(LS, -1, ref);
        REQUIRE(0 == get_ref_count(LS, -2));
        REQUIRE(1 == get_ref_count(LS, -1));

        lua_pushinteger(LS, 4323);
        save_luaref(LS, -1, ref);
        REQUIRE(0 == get_ref_count(LS, -3));
        REQUIRE(0 == get_ref_count(LS, -2));
        REQUIRE(1 == get_ref_count(LS, -1));

        free_luaref(LS, ref);
    }

    SECTION("is_set_luaref")
    {
        LUAREF *ref = new_luaref();

        REQUIRE(false == is_set_luaref(ref));
        REQUIRE(0 == lua_gettop(LS));

        lua_pushinteger(LS, 4321);
        REQUIRE(1 == lua_gettop(LS));

        save_luaref(LS, -1, ref);
        REQUIRE(1 == lua_gettop(LS));
        REQUIRE(true == is_set_luaref(ref));

        release_luaref(LS, ref);
        REQUIRE(false == is_set_luaref(ref));
        REQUIRE(1 == lua_gettop(LS));

        free_luaref(LS, ref);
    }

    SECTION("release_luaref")
    {
        LUAREF *ref = new_luaref();

        REQUIRE(false == is_set_luaref(ref));
        // Release when not set is just a no-op
        release_luaref(LS, ref);
        REQUIRE(false == is_set_luaref(ref));

        lua_pushinteger(LS, 4321);
        REQUIRE(1 == lua_gettop(LS));

        save_luaref(LS, -1, ref);
        REQUIRE(1 == lua_gettop(LS));
        REQUIRE(true == is_set_luaref(ref));

        release_luaref(LS, ref);
        REQUIRE(false == is_set_luaref(ref));

        free_luaref(LS, ref);
    }

    SECTION("push_luaref")
    {
        LUAREF *ref = new_luaref();

        /* pushing unset ref just pushes nil */
        REQUIRE(false == is_set_luaref(ref));
        REQUIRE(0 == lua_gettop(LS));
        push_luaref(LS, ref);
        REQUIRE(1 == lua_gettop(LS));
        REQUIRE(1 == lua_isnil(LS, -1));
        lua_pop(LS, 1);

        REQUIRE(0 == lua_gettop(LS));
        push_luaref(LS, ref);
        push_luaref(LS, ref);
        push_luaref(LS, ref);
        REQUIRE(3 == lua_gettop(LS));
        REQUIRE(1 == lua_isnil(LS, -1));
        REQUIRE(1 == lua_isnil(LS, -2));
        REQUIRE(1 == lua_isnil(LS, -3));
        lua_pop(LS, 3);

        /* regular value */
        lua_pushinteger(LS, 1342);
        save_luaref(LS, -1, ref);
        lua_pop(LS, 1);

        REQUIRE(0 == lua_gettop(LS));
        push_luaref(LS, ref);
        push_luaref(LS, ref);
        push_luaref(LS, ref);
        REQUIRE(3 == lua_gettop(LS));
        lua_pushinteger(LS, 1342);
        REQUIRE(1 == lua_equal(LS, -1, -2));
        REQUIRE(1 == lua_equal(LS, -1, -3));
        REQUIRE(1 == lua_equal(LS, -1, -4));

        lua_pop(LS, 4);

        free_luaref(LS, ref);
    }
}