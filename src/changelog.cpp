#include <iostream>

#include <vector>
#include <cstdio>

#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

extern "C" {
#include <lauxlib.h>
#include "changelog.h"
#include "perfmon.h"
}


struct cl_entry
{
    cl_entry(lua_Integer date_, const char * author_, const char * desc_)
        : date( date_ )
        , author( author_ )
        , desc( desc_ )
    { }

    lua_Integer date;
    std::string author;
    std::string desc;

};


static int fill_vector(lua_State *LS)
{
    std::vector<cl_entry> *tbl = static_cast<std::vector<cl_entry> *>(lua_touserdata(LS, 1));

    lua_getglobal(LS, "changelog_table");

    for (int i = 1 ; ; ++i)
    {
        lua_rawgeti(LS, -1, i);
        if (lua_isnil(LS, -1))
        {
            lua_pop(LS, 1);
            break;
        }

        lua_getfield(LS, -1, "date");
        lua_getfield(LS, -2, "author");
        lua_getfield(LS, -3, "desc");

        tbl->emplace_back(
            luaL_checkinteger(LS, -3),
            luaL_checkstring(LS, -2),
            luaL_checkstring(LS, -1)
            );
        lua_pop(LS, 4);
    }

    return 0;
}

int L_save_changelog(lua_State *LS)
{
    PERF_PROF_ENTER( pr_, "L_save_changelog" );

    // First pull everything into a vector from lua, handle any lua error
    // and clean up memory cleanly
    std::vector<cl_entry> *tbl = new std::vector<cl_entry>();

    lua_pushcfunction(LS, fill_vector);
    lua_pushlightuserdata(LS, tbl);

    int error = lua_pcall(LS, 1, 0, 0);
    if (error != 0)
    {
        delete tbl;
        PERF_PROF_EXIT( pr_ );
        return lua_error(LS);
    }
   
    // Now save from the vector to json
    FILE *fp = ::fopen("changelog_table.json", "w");

    if (!fp)
    {
        PERF_PROF_EXIT( pr_ );
        return luaL_error(LS, "Couldn't open file changelog_table.json");
    }

    char fBuf[1024];

    rapidjson::FileWriteStream os(fp, fBuf, sizeof(fBuf));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);

    writer.StartArray();
    
    for (auto &entry : *tbl)
    {
        writer.StartObject();

        writer.Key("date");
        writer.Int(entry.date);
        writer.Key("author");
        writer.String(entry.author.c_str());
        writer.Key("desc");
        writer.String(entry.desc.c_str());

        writer.EndObject();
    }

    writer.EndArray();

    fclose(fp);
    delete tbl;

    PERF_PROF_EXIT( pr_ );
    return 0;
}

int L_load_changelog(lua_State *LS)
{
    FILE *fp = ::fopen("changelog_table.json", "r");

    if (!fp)
    {
        // Convert old lua file if it exists
        int r = luaL_loadfile(LS, "changelog_table.lua");
        if (r != 0)
        {
            // Doesn't exist, or some other error
            return 0;
        }

        lua_call(LS, 0, 1);

        lua_setglobal(LS, "changelog_table");
        L_save_changelog(LS);
        ::remove("changelog_table.lua");

        return 0;
    }

    char fBuf[1024];
    rapidjson::FileReadStream is(fp, fBuf, sizeof(fBuf));

    rapidjson::Document doc;
    doc.ParseStream(is);

    fclose(fp);

    lua_newtable(LS);
    
    for (rapidjson::SizeType i = 0; i < doc.Size(); ++i)
    {
        rapidjson::Value &val = doc[i];

        lua_newtable(LS);
        
        lua_pushinteger(LS, val["date"].GetInt());
        lua_setfield(LS, -2, "date");

        lua_pushstring(LS, val["author"].GetString());
        lua_setfield(LS, -2, "author");

        lua_pushstring(LS, val["desc"].GetString());
        lua_setfield(LS, -2, "desc");

        lua_rawseti(LS, -2, static_cast<int>(i) + 1);
    }

    lua_setglobal(LS, "changelog_table");    

    return 0;
}
