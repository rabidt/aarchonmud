#include <lualib.h>
#include <lauxlib.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "merc.h"
#include "timer.h"
#include "lsqlite3.h"
#include "lua_main.h"
#include "lua_arclib.h"
#include "interp.h"
#include "mudconfig.h"
#include "perfmon.h"
#include "changelog.h"


lua_State *g_mud_LS = NULL;  /* Lua state for entire MUD */
bool       g_LuaScriptInProgress=FALSE;
int        g_ScriptSecurity=SEC_NOSCRIPT;
int        g_LoopCheckCounter;


/* keep these as LUAREFS for ease of use on the C side */
LUAREF REF_TRACEBACK;
LUAREF REF_TABLE_INSERT;
LUAREF REF_TABLE_MAXN;
LUAREF REF_TABLE_CONCAT;
LUAREF REF_STRING_FORMAT;
LUAREF REF_UNPACK;

void register_LUAREFS( lua_State *LS)
{
    /* initialize the variables */
    new_ref( &REF_TABLE_INSERT );
    new_ref( &REF_TABLE_MAXN);
    new_ref( &REF_TABLE_CONCAT );
    new_ref( &REF_STRING_FORMAT );
    new_ref( &REF_UNPACK );

    /* put stuff in the refs */
    lua_getglobal( LS, "table" );

    lua_getfield( LS, -1, "insert" );
    save_ref( LS, -1, &REF_TABLE_INSERT );
    lua_pop( LS, 1 ); /* insert */

    lua_getfield( LS, -1, "maxn" );
    save_ref( LS, -1, &REF_TABLE_MAXN );
    lua_pop( LS, 1 ); /* maxn */

    lua_getfield( LS, -1, "concat" );
    save_ref( LS, -1, &REF_TABLE_CONCAT );
    lua_pop( LS, 2 ); /* concat and table */

    lua_getglobal( LS, "string" );
    lua_getfield( LS, -1, "format" );
    save_ref( LS, -1, &REF_STRING_FORMAT );
    lua_pop( LS, 2 ); /* string and format */

    lua_getglobal( LS, "unpack" );
    save_ref( LS, -1, &REF_UNPACK );
    lua_pop( LS, 1 );
}

#define LUA_LOOP_CHECK_MAX_CNT 10000 /* give 1000000 instructions */
#define LUA_LOOP_CHECK_INCREMENT 100
#define ERR_INF_LOOP      -1

int GetLuaMemoryUsage( void )
{
    return lua_gc( g_mud_LS, LUA_GCCOUNT, 0);
}

int GetLuaGameObjectCount( void )
{
    lua_getglobal( g_mud_LS, "UdCnt");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 1) )
    {
        bugf ( "Error with UdCnt:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop(g_mud_LS, 1);
        return -1;
    }

    int rtn=luaL_checkinteger( g_mud_LS, -1 );
    lua_pop( g_mud_LS, 1 );
    return rtn;
}

int GetLuaEnvironmentCount( void )
{
    lua_getglobal( g_mud_LS, "EnvCnt");  
    if (CallLuaWithTraceBack( g_mud_LS, 0, 1) )
    {
        bugf ( "Error with EnvCnt:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop(g_mud_LS, 1);
        return -1;
    }

    int rtn=luaL_checkinteger( g_mud_LS, -1 );
    lua_pop( g_mud_LS, 1 );
    return rtn;
}

static void infinite_loop_check_hook( lua_State *LS, lua_Debug *ar)
{
    if (!g_LuaScriptInProgress)
        return;

    if ( g_LoopCheckCounter < LUA_LOOP_CHECK_MAX_CNT)
    {
        g_LoopCheckCounter++;
        return;
    }
    else
    {
        /* exit */
        luaL_error( LS, "Interrupted infinite loop." );
        return;
    }
}



void stackDump (lua_State *LS) {
    int i;
    int top = lua_gettop(LS);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(LS, i);
        switch (t) {

            case LUA_TSTRING:  /* strings */
                logpf("`%s'", lua_tostring(LS, i));
                break;

            case LUA_TBOOLEAN:  /* booleans */
                logpf(lua_toboolean(LS, i) ? "true" : "false");
                break;

            case LUA_TNUMBER:  /* numbers */
                logpf("%g", lua_tonumber(LS, i));
                break;

            default:  /* other values */
                logpf("%s", lua_typename(LS, t));
                break;

        }
    }
    logpf("\n");  /* end the listing */
}

const char *check_string( lua_State *LS, int index, size_t size)
{
    size_t rtn_size;
    const char *rtn=luaL_checklstring( LS, index, &rtn_size );
    /* Check >= because we assume 'size' argument refers to
       size of a char buffer rather than desired strlen.
       If called with MSL then the result (including terminating '\0'
       will fit in MSL sized buffer. */
    if (rtn_size >= size )
        luaL_error( LS, "String size %d exceeds maximum %d.",
                (int)rtn_size, (int)size-1 );

    return rtn;
}

/* Run string.format using args beginning at index 
   Assumes top is the last argument*/
const char *check_fstring( lua_State *LS, int index, size_t size)
{
    int narg=lua_gettop(LS)-(index-1);

    if ( (narg>1))
    {
        push_ref( LS, REF_STRING_FORMAT );
        lua_insert( LS, index );
        lua_call( LS, narg, 1);
    }

    return check_string( LS, index, size);
}

static void GetTracebackFunction (lua_State *LS)
{
    push_ref( LS, REF_TRACEBACK );
}  /* end of GetTracebackFunction */

int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn)
{
    int error;

    int base = lua_gettop (LS) - iArguments;  /* function index */
    GetTracebackFunction (LS);
    if (lua_isnil (LS, -1))
    {
        lua_pop (LS, 1);   /* pop non-existent function  */
        bugf("pop non-existent function");
        error = lua_pcall (LS, iArguments, iReturn, 0);
    }
    else
    {
        lua_insert (LS, base);  /* put it under chunk and args */
        error = lua_pcall (LS, iArguments, iReturn, base);
        lua_remove (LS, base);  /* remove traceback function */
    }

    return error;
}  /* end of CallLuaWithTraceBack  */

DEF_DO_FUN(do_lboard)
{
    lua_getglobal(g_mud_LS, "do_lboard");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf ( "Error with do_lboard:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_lhistory)
{
    lua_getglobal(g_mud_LS, "do_lhistory");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf ( "Error with do_lhistory:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void update_lboard( int lboard_type, CHAR_DATA *ch, int current, int increment )
{
    if (IS_NPC(ch) || IS_IMMORTAL(ch) )
        return;

    lua_getglobal(g_mud_LS, "update_lboard");
    lua_pushnumber( g_mud_LS, lboard_type);
    lua_pushstring( g_mud_LS, ch->name);
    lua_pushnumber( g_mud_LS, current);
    lua_pushnumber( g_mud_LS, increment);

    if (CallLuaWithTraceBack( g_mud_LS, 4, 0) )
    {
        bugf ( "Error with update_lboard:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void save_lboards( void )
{
    PERF_PROF_ENTER( pr_, "save_lboards" );
    lua_getglobal( g_mud_LS, "save_lboards");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with save_lboard:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
    PERF_PROF_EXIT( pr_ );
}

void load_lboards( void )
{
    lua_getglobal( g_mud_LS, "load_lboards");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with load_lboards:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void check_lboard_reset( void )
{
    PERF_PROF_ENTER( pr_, "check_lboard_reset" );

    lua_getglobal( g_mud_LS, "check_lboard_reset");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with check_lboard_resets:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }

    PERF_PROF_EXIT( pr_ );
}

void load_mudconfig( void )
{
    lua_getglobal( g_mud_LS, "load_mudconfig");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with load_mudconfig:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }    
}

bool run_lua_interpret( DESCRIPTOR_DATA *d)
{
    if (!d->lua.interpret) /* not in interpreter */
        return FALSE; 

    if (!strcmp( d->incomm, "@") )
    {
        /* kick out of interpret */
        d->lua.interpret=FALSE;
        d->lua.incmpl=FALSE;

        lua_unregister_desc(d);

        ptc(d->character, "Exited lua interpreter.\n\r");
        return TRUE;
    }

    lua_getglobal( g_mud_LS, "run_lua_interpret"); //what we'll call if no errors

    /* Check this all in C so we can exit interpreter for missing env */
    lua_pushlightuserdata( g_mud_LS, d); /* we'll check this against the table */
    lua_getglobal( g_mud_LS, INTERP_TABLE_NAME);
    if (lua_isnil( g_mud_LS, -1) )
    {
        bugf("Couldn't find " INTERP_TABLE_NAME);
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }
    bool interpalive=FALSE;
    lua_pushnil( g_mud_LS);
    while (lua_next(g_mud_LS, -2) != 0)
    {
        lua_getfield( g_mud_LS, -1, "desc");
        if (lua_equal( g_mud_LS, -1,-5))
        {
           interpalive=TRUE;
        }
        lua_pop(g_mud_LS, 2);

        if (interpalive)
            break;
    }
    if (!interpalive)
    {
        ptc( d->character, "Interpreter session was closed, was object destroyed?\n\r"
                "Exiting interpreter.\n\r");
        d->lua.interpret=FALSE;

        ptc(d->character, "Exited lua interpreter.\n\r");
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }
    /* object pointer should be sitting at -1, interptbl at -2, desc lightud at -3 */
    lua_remove( g_mud_LS, -3);
    lua_remove( g_mud_LS, -2);


    lua_getglobal( g_mud_LS, ENV_TABLE_NAME);
    if (lua_isnil( g_mud_LS, -1) )
    {
        bugf("Couldn't find " ENV_TABLE_NAME);
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }
    lua_pushvalue( g_mud_LS, -2);
    lua_remove( g_mud_LS, -3);
    lua_gettable( g_mud_LS, -2);
    lua_remove( g_mud_LS, -2); /* don't need envtbl anymore*/
    if ( lua_isnil( g_mud_LS, -1) )
    {
        bugf("Game object not found for interpreter session for %s.", d->character->name);
        ptc( d->character, "Couldn't find game object, was it destroyed?\n\r"
                "Exiting interpreter.\n\r");
        d->lua.interpret=FALSE;

        ptc(d->character, "Exited lua interpreter.\n\r");
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }

    /* if we're here then we just need to push the string and call the func */
    lua_pushstring( g_mud_LS, d->incomm);

    g_ScriptSecurity= d->character->pcdata->security ;
    g_LoopCheckCounter=0;
    g_LuaScriptInProgress = TRUE;

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        ptc(d->character,  "LUA error for lua_interpret:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        d->lua.incmpl=FALSE; //force it whether it was or wasn't
    } 
    else
    {
        bool incmpl=(bool)luaL_checknumber( g_mud_LS, -1 );
        if (incmpl)
        {
            d->lua.incmpl=TRUE;
        } 
        else
        {
            d->lua.incmpl=FALSE;
        
        }
    }

    g_ScriptSecurity = SEC_NOSCRIPT;
    g_LuaScriptInProgress=FALSE;

    lua_settop( g_mud_LS, 0);
    return TRUE;

}

void lua_unregister_desc (DESCRIPTOR_DATA *d)
{
    lua_getglobal( g_mud_LS, "UnregisterDesc");
    lua_pushlightuserdata( g_mud_LS, d);
    int error=CallLuaWithTraceBack (g_mud_LS, 1, 0) ;
    if (error > 0 )
    {
        ptc(d->character,  "LUA error for UnregisterDesc:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop(g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_luai)
{
    if IS_NPC(ch)
        return;
    
    if ( ch->desc->lua.interpret )
    {
        ptc(ch, "Lua interpreter already active!\n\r");
        return;
    }

    char arg1[MSL];
    const char *name;

    argument=one_argument(argument, arg1);

    void *victim=NULL;
    LUA_OBJ_TYPE *type;

    if ( arg1[0]== '\0' )
    {
        victim=(void *)ch;
        type=p_CH_type;
        name=ch->name;
    }
    else if (!strcmp( arg1, "mob") )
    {
        CHAR_DATA *mob;
        mob=get_char_room( ch, argument );
        if (!mob)
        {
            ptc(ch, "Could not find %s in the room.\n\r", argument);
            return;
        }
        else if (!IS_NPC(mob))
        {
            ptc(ch, "Not on PCs.\n\r");
            return;
        }

        victim = (void *)mob;
        type= p_CH_type;
        name=mob->name;
    }
    else if (!strcmp( arg1, "obj") )
    {
        OBJ_DATA *obj=NULL;
        obj=get_obj_here( ch, argument);

        if (!obj)
        {
            ptc(ch, "Could not find %s in room or inventory.\n\r", argument);
            return;
        }

        victim= (void *)obj;
        type=p_OBJ_type;
        name=obj->name;
    }
    else if (!strcmp( arg1, "area") )
    {
        if (!ch->in_room)
        {
            bugf("do_luai: %s in_room is NULL.", ch->name);
            return;
        }

        victim= (void *)(ch->in_room->area);
        type=p_AREA_type;
        name=ch->in_room->area->name;
    }
    else if (!strcmp( arg1, "room") )
    {
        if (!ch->in_room)
        {
            bugf("do_luai: %s in_room is NULL.", ch->name);
            return;
        }
        
        victim= (void *)(ch->in_room);
        type=p_ROOM_type;
        name=ch->in_room->name;
    }
    else
    {
        ptc(ch, "luai [no argument] -- open interpreter in your own env\n\r"
                "luai mob <target>  -- open interpreter in env of target mob (in same room)\n\r"
                "luai obj <target>  -- open interpreter in env of target obj (inventory or same room)\n\r"
                "luai area          -- open interpreter in env of current area\n\r"
                "luai room          -- open interpreter in env of current room\n\r"); 
        return;
    }

    if (!ch->desc)
    {
        bugf("do_luai: %s has null desc", ch->name);
        return;
    }

    /* do the stuff */
    lua_getglobal( g_mud_LS, "interp_setup");
    if ( !arclib_push( type, g_mud_LS, victim) )
    {
        bugf("do_luai: couldn't make udtable argument %s",
                argument);
        lua_settop(g_mud_LS, 0);
        return;
    }

    if ( type == p_CH_type )
    {
        lua_pushliteral( g_mud_LS, "mob"); 
    }
    else if ( type == p_OBJ_type )
    {
        lua_pushliteral( g_mud_LS, "obj"); 
    }
    else if ( type == p_AREA_type )
    {
        lua_pushliteral( g_mud_LS, "area"); 
    }
    else if ( type == p_ROOM_type )
    {
        lua_pushliteral( g_mud_LS, "room"); 
    }
    else
    {
            bugf("do_luai: invalid type: %s", arclib_type_name(type));
            lua_settop(g_mud_LS, 0);
            return;
    }

    lua_pushlightuserdata(g_mud_LS, ch->desc);
    lua_pushstring( g_mud_LS, ch->name );

    int error=CallLuaWithTraceBack (g_mud_LS, 4, 2) ;
    if (error > 0 )
    {
        bugf ( "LUA error for interp_setup:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_settop(g_mud_LS, 0);
        return;
    }

    /* 2 values, true or false (false if somebody already interpreting on that object)
       and name of person interping if false */
    bool success=(bool)luaL_checknumber( g_mud_LS, -2);
    if (!success)
    {
        ptc(ch, "Can't open lua interpreter, %s already has it open for that object.\n\r",
                check_string( g_mud_LS, -1, MIL));
        lua_settop(g_mud_LS, 0);
        return;
    }

    /* finally, if everything worked out, we can set this stuff */
    ch->desc->lua.interpret=TRUE;
    ch->desc->lua.incmpl=FALSE;

    ptc(ch, "Entered lua interpreter mode for %s %s\n\r", 
            arclib_type_name(type),
            name);
    ptc(ch, "Use @ on a blank line to exit.\n\r");
    ptc(ch, "Use do and end to create multiline chunks.\n\r");
    ptc(ch, "Use '%s' to access target's self.\n\r",
            type == p_CH_type ? "mob" :
            type == p_OBJ_type ? "obj" :
            type == p_ROOM_type ? "room" :
            type == p_AREA_type ? "area" :
            "ERROR" );

    lua_settop(g_mud_LS, 0);
    return;
}

static int RegisterLuaRoutines (lua_State *LS)
{
    time_t timer;
    time (&timer);

    init_genrand ((unsigned long)timer);
    arclib_type_init( LS );

    register_globals ( LS );
    register_LUAREFS( LS );

    lua_pushcfunction(LS, L_save_changelog);
    lua_setglobal(LS, "save_changelog");

    return 0;

}  /* end of RegisterLuaRoutines */

void open_lua ( void )
{
    lua_State *LS = luaL_newstate ();   /* opens Lua */
    g_mud_LS = LS;

    if (g_mud_LS == NULL)
    {
        bugf("Cannot open Lua state");
        return;  /* catastrophic failure */
    }

    luaL_openlibs (LS);    /* open all standard libraries */

    /* special little tweak to debug.traceback here before anything else */
    new_ref( &REF_TRACEBACK );
    int rtn = luaL_dostring(g_mud_LS, 
        "local traceback = debug.traceback \n"
        "debug.traceback = function(message, level) \n"
          "level = level or 1 \n"
          "local rtn = traceback(message, level + 1) \n"
          "if not rtn then return rtn end \n"
          "rtn = rtn:gsub(\"\\t\", \"    \") \n"
          "rtn = rtn:gsub(\"\\n\", \"\\n\\r\") \n"
          "return rtn \n"
        "end \n"
        "return debug.traceback \n"
    );
    if (rtn != 0)
    {
        bugf("Error setting traceback function");
        exit(1);
    }
    save_ref(LS, -1, &REF_TRACEBACK);
    lua_pop(LS, 1);

    lua_pushcfunction(LS, luaopen_lsqlite3);
    lua_call(LS, 0, 0);

    /* call as Lua function because we need the environment  */
    lua_pushcfunction(LS, RegisterLuaRoutines);
    lua_call(LS, 0, 0);

    lua_sethook(LS, infinite_loop_check_hook, LUA_MASKCOUNT, LUA_LOOP_CHECK_INCREMENT);
    /* run initialiation script */
    if (luaL_loadfile (LS, LUA_STARTUP) ||
            CallLuaWithTraceBack (LS, 0, 0))
    {
        bugf ( "Error loading Lua startup file %s:\n %s", 
                LUA_STARTUP,
                lua_tostring(LS, -1));
    }

    lua_settop (LS, 0);    /* get rid of stuff lying around */

}  /* end of open_lua */

DEF_DO_FUN(do_scriptdump)
{
    lua_getglobal(g_mud_LS, "do_scriptdump");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_scriptdump:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }

}
static int L_wizhelp( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);
    
    int index=1; /* the current table index */
    lua_newtable( LS );/* the table of commands */
    
    int cmd;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
        if ( cmd_table[cmd].level >= LEVEL_HERO
            &&  is_granted( ud_ch, cmd_table[cmd].do_fun )
            &&  cmd_table[cmd].show )
        {
            lua_newtable( LS ); /* this command's table */
            
            lua_pushinteger( LS, cmd_table[cmd].level );
            lua_setfield( LS, -2, "level" );

            lua_pushstring( LS, cmd_table[cmd].name );
            lua_setfield( LS, -2, "name" );
            
            lua_rawseti( LS, -2, index++ ); /* save the command to the table */
        }
    }

    lua_getglobal( LS, "wizhelp" );
    lua_insert( LS, 1 ); /* shove it to the top */

    lua_call( LS, lua_gettop(LS)-1, 0 );

    return 0;
}
     
DEF_DO_FUN(do_luaquery)
{
    lua_getglobal( g_mud_LS, "do_luaquery");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_luaquery:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}    

DEF_DO_FUN(do_wizhelp)
{
    lua_pushcfunction(g_mud_LS, L_wizhelp);
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_wizhelp:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_qset)
{
    lua_getglobal( g_mud_LS, "do_qset");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_qset:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

const char *save_ptitles( CHAR_DATA *ch )
{
    if (IS_NPC(ch))
        return NULL;

    lua_getglobal(g_mud_LS, "save_ptitles" );
    push_CH(g_mud_LS, ch);
    if (CallLuaWithTraceBack( g_mud_LS, 1, 1 ) )
    {
        ptc (ch, "Error with save_ptitles:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
        return NULL;
    }

    const char *rtn;
    if (lua_isnil(g_mud_LS, -1) || lua_isnone(g_mud_LS, -1) )
    {
        rtn=NULL;
    }
    else if (!lua_isstring(g_mud_LS, -1))
    {
        bugf("String wasn't returned in save_ptitles.");
        rtn=NULL;
    }
    else
    {
        rtn=str_dup(luaL_checkstring( g_mud_LS, -1 ));
    }
   
    lua_pop( g_mud_LS, 1 );

    return rtn;
}

void load_ptitles( CHAR_DATA *ch, const char *text )
{
    lua_getglobal(g_mud_LS, "load_ptitles" );
    push_CH(g_mud_LS, ch);
    lua_pushstring( g_mud_LS, text );

    if (CallLuaWithTraceBack( g_mud_LS, 2, 0 ) )
    {
        ptc (ch, "Error with load_ptitles:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}
const char *save_luaconfig( CHAR_DATA *ch )
{
    if (!IS_IMMORTAL(ch))
        return NULL;

    lua_getglobal(g_mud_LS, "save_luaconfig" );
    push_CH(g_mud_LS, ch);
    if (CallLuaWithTraceBack( g_mud_LS, 1, 1 ) )
    {
        ptc (ch, "Error with save_luaconfig:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
        return NULL;
    }

    const char *rtn;
    if (lua_isnil(g_mud_LS, -1) || lua_isnone(g_mud_LS, -1) )
    {
        rtn=NULL;
    }
    else if (!lua_isstring(g_mud_LS, -1))
    {
        bugf("String wasn't returned in save_luaconfig.");
        rtn=NULL;
    }
    else
    {
        rtn=luaL_checkstring( g_mud_LS, -1 );
    }

    lua_pop( g_mud_LS, 1 );

    return rtn;
}

void load_luaconfig( CHAR_DATA *ch, const char *text )
{
    lua_getglobal(g_mud_LS, "load_luaconfig" );
    push_CH(g_mud_LS, ch);
    lua_pushstring( g_mud_LS, text );

    if (CallLuaWithTraceBack( g_mud_LS, 2, 0 ) )
    {
        ptc (ch, "Error with load_luaconfig:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_luaconfig)
{
    lua_getglobal(g_mud_LS, "do_luaconfig");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_luaconfig:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

static int L_dump_prog( lua_State *LS)
{
    // 1 is ch
    // 2 is prog 
    // 3 is numberlines
    bool numberlines=lua_toboolean(LS, 3);
    lua_pop(LS,1);


    lua_getglobal( LS, "colorize");
    lua_insert( LS, -2 );
    lua_pushvalue(LS,1); //push a copy of ch
    lua_call( LS, 2, 1 );

    // 1 is ch
    // 2 is colorized text
    if (numberlines)
    {
        lua_getglobal( LS, "linenumber");
        lua_insert( LS, -2);
        lua_call(LS, 1, 1);
    }
    lua_pushstring(LS, "\n\r");
    lua_concat( LS, 2);
    page_to_char_new( 
            luaL_checkstring(LS, 2),
            check_CH(LS, 1),
            TRUE);

    return 0;
}

void dump_prog( CHAR_DATA *ch, const char *prog, bool numberlines)
{
    lua_pushcfunction( g_mud_LS, L_dump_prog);
    push_CH(g_mud_LS, ch);
    lua_pushstring( g_mud_LS, prog);
    lua_pushboolean( g_mud_LS, numberlines);

    if (CallLuaWithTraceBack( g_mud_LS, 3, 0) )
    {
        ptc (ch, "Error with dump_prog:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_luareset)
{
    lua_getglobal(g_mud_LS, "do_luareset");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_luareset:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_alist)
{
    lua_getglobal(g_mud_LS, "do_alist");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_alist:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_mudconfig)
{
    lua_getglobal(g_mud_LS, "do_mudconfig");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_mudconfig:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_findreset)
{
    lua_getglobal(g_mud_LS, "do_findreset");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_findreset:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_diagnostic)
{
    lua_getglobal(g_mud_LS, "do_diagnostic");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_diagnostic:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void do_achievements_boss( CHAR_DATA *ch, CHAR_DATA *vic )
{
    lua_getglobal(g_mud_LS, "do_achievements_boss");
    push_CH(g_mud_LS, ch);
    push_CH(g_mud_LS, vic);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf( "Error with do_achievements_boss:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}
void do_achievements_boss_reward( CHAR_DATA *ch )
{
    lua_getglobal(g_mud_LS, "do_achievements_boss_reward");
    push_CH(g_mud_LS, ch);
    if (CallLuaWithTraceBack( g_mud_LS, 1, 0) )
    {
        bugf( "Error with do_achievements_boss_reward:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void update_bossachv_table( void )
{
    lua_getglobal(g_mud_LS, "update_bossachv_table" );
    
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf( "Error with update_bossachv_table:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void check_lua_stack( void )
{
    int top=lua_gettop( g_mud_LS );
    if ( top > 0 )
    {
        bugf("%d items left on Lua stack. Clearing.", top );
        lua_settop( g_mud_LS, 0);
    }
}

DEF_DO_FUN(do_path)
{
    lua_getglobal(g_mud_LS, "do_path");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_path:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_luahelp)
{
    lua_getglobal(g_mud_LS, "do_luahelp");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_luahelp:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_ptitle)
{
    lua_getglobal(g_mud_LS, "do_ptitle");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf( "Error with do_ptitle:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void quest_buy_ptitle( CHAR_DATA *ch, const char *argument)
{
    lua_getglobal(g_mud_LS, "quest_buy_ptitle");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf( "Error with quest_buy_ptitle:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void fix_ptitles( CHAR_DATA *ch)
{
    lua_getglobal(g_mud_LS, "fix_ptitles");
    push_CH(g_mud_LS, ch);
    
    if (CallLuaWithTraceBack( g_mud_LS, 1, 0) )
    {
        bugf( "Error with fix_ptitles:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_changelog)
{
    lua_getglobal(g_mud_LS, "do_changelog");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf("Error with do_changelog:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void load_changelog( void )
{
    lua_pushcfunction( g_mud_LS, L_load_changelog );
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with load_changelog:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}


/* LUAREF section */

/* called as constructor */
void new_ref( LUAREF *ref )
{
    *ref=LUA_NOREF;
}

/* called as destructor
   only call this when the LUAREF's lifetime is ending,
   otherwise use release_ref to release values */
void free_ref( LUAREF *ref )
{
    if (*ref!=LUA_NOREF)
    {
        luaL_unref( g_mud_LS, LUA_REGISTRYINDEX, *ref );
        *ref=LUA_NOREF;
    }

}

void save_ref( lua_State *LS, int index, LUAREF *ref )
{
    if ( *ref!=LUA_NOREF )
    {
        bugf( "Tried to save over existing ref.");
        return;
    }
    lua_pushvalue(LS, index);
    *ref = luaL_ref( LS, LUA_REGISTRYINDEX );
}

void release_ref( lua_State *LS,  LUAREF *ref )
{
    if ( *ref==LUA_NOREF )
    {
        bugf( "Tried to release bad ref.");
        return;
    }
    luaL_unref( LS, LUA_REGISTRYINDEX, *ref ); 
    *ref=LUA_NOREF;
}

void push_ref( lua_State *LS, LUAREF ref )
{
    lua_rawgeti( LS, LUA_REGISTRYINDEX, ref );
}

bool is_set_ref( LUAREF ref )
{
    return !(ref==LUA_NOREF);
}

/* end LUAREF section */

void lua_con_handler( DESCRIPTOR_DATA *d, const char *argument )
{
    lua_getglobal( g_mud_LS, "lua_con_handler" );
    push_DESCRIPTOR( g_mud_LS, d);
    if (argument)
    {
        lua_pushstring( g_mud_LS, argument);
    }
    else
    {
        lua_pushnil( g_mud_LS );
    }
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf("Error with lua_con_handler:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }

}

static int L_DO_FUN_caller( lua_State *LS )
{
    DO_FUN *fun=lua_touserdata( LS, 1 );
    CHAR_DATA *ch=check_CH( LS, 2 );
    const char *arg=check_string( LS, 3, MIL );
    fun(ch, arg);
    return 0;
}

/*  confirm_yes_no()

    Have the player confirm an action.
    
    If provided, yes/no callbacks are called upon selection of Y or n by
    the player.  These callbacks must have DO_FUN signature.

    If yes_argument/no_argument are provided, they are used as the 'argument'
    parameter to the respective callback, otherwise an empty string is used.
    
    Example using original do_fun as callback:

    DEF_DO_FUN( test1 )
    {
        if (strcmp(argument, "confirm"))
        {
            send_to_char( "Are you sure you want to do it?\n\r", ch);
            confirm_yes_no( ch->desc, do_test1, "confirm", NULL, NULL);
            return;
        }

        send_to_char( ch, "You did it!\n\r");
    }


    Example using separate callback function:

    DEF_DO_FUN( test1_confirm )
    {
        ptc( ch, "You did it!\n\rHere's your argument: %s\n\r", argument);
        return;
    }

    DEF_DO_FUN( test1 )
    {
        send_to_char( "Are you sure you want to do it?\n\r", ch);
        // forward original argument onto the callback
        confirm_yes_no( ch->desc, do_test1_confirm, argument, NULL, NULL);
        return;
    }

*/
void confirm_yes_no( DESCRIPTOR_DATA *d,
        DO_FUN yes_callback, 
        const char *yes_argument,
        DO_FUN no_callback,
        const char *no_argument)
{
    lua_getglobal( g_mud_LS, "confirm_yes_no");

    lua_pushcfunction( g_mud_LS, L_DO_FUN_caller);

    push_DESCRIPTOR( g_mud_LS, d );

    if (yes_callback)
    {
        lua_pushlightuserdata( g_mud_LS, yes_callback);
    }
    else
    {
        lua_pushnil( g_mud_LS);
    }

    if (yes_argument)
    {
        lua_pushstring( g_mud_LS, yes_argument);
    }
    else
    {
        lua_pushnil( g_mud_LS);
    }

    if (no_callback)
    {
        lua_pushlightuserdata( g_mud_LS, no_callback);
    }
    else
    {
        lua_pushnil( g_mud_LS);
    }

    if (no_argument)
    {
        lua_pushstring( g_mud_LS, no_argument);
    }
    else
    {
        lua_pushnil( g_mud_LS);
    }

    if (CallLuaWithTraceBack( g_mud_LS, 6, 0) )
    {
        bugf("Error with confirm_yes_no:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
    return;
}

/* push nil if pipe closed, string if data, empty string if no data */
static int L_pgrep_read(lua_State *LS)
{
    char buf[MSL];
    FILE *fp = lua_touserdata(LS, 1);
    int fd = fileno(fp);

    ssize_t r = read(fd, buf, MSL-1);
    if (r == -1 && errno == EAGAIN)
    {
        lua_pushstring(LS, "");
        return 1;
    }
    else if (r > 0)
    {
        buf[r] = '\0'; /* not sure if necessary? */
        lua_pushstring(LS, buf);
        return 1;
    }
    else 
    {
        pclose(fp);
        lua_pushnil(LS);
        return 1;
    }
}

/* for manual canceling before reading complete */ 
static int L_pgrep_cancel(lua_State *LS)
{
    FILE *fp = lua_touserdata(LS, 1);
    pclose(fp);

    return 0;
}

DEF_DO_FUN(do_pgrep)
{
    char buf[MSL];

    if (IS_NPC(ch))
        return;

    if ( argument[0] == '\0' )
    {
        send_to_char(" pgrep <text> -- searches for the text in the player folder\n\r", ch );
        return;
    }
    
    sprintf(buf, "grep \"%s\" ../player/* ../box/*", argument);


    /* http://stackoverflow.com/questions/1735781/non-blocking-pipe-using-popen */

    FILE *fp = popen(buf, "r");
    int fd = fileno(fp);
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    lua_getglobal(g_mud_LS, "start_pgrep");
    push_DESCRIPTOR(g_mud_LS, ch->desc);
    lua_pushcfunction(g_mud_LS, L_pgrep_read);
    lua_pushcfunction(g_mud_LS, L_pgrep_cancel);
    lua_pushlightuserdata(g_mud_LS, fp);

    if (CallLuaWithTraceBack( g_mud_LS, 4, 0) )
    {
        bugf ( "Error with do_pgrep:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop(g_mud_LS, 1);
        return;
    } 
}
