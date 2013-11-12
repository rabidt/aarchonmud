/********************************-********************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *			 Lua Scripting Module     by Nick Gammon                  			    *
 ****************************************************************************/

/*

   Lua scripting written by Nick Gammon
Date: 8th July 2007

You are welcome to incorporate this code into your MUD codebases.

Post queries at: http://www.gammon.com.au/forum/

Description of functions in this file is at:

http://www.gammon.com.au/forum/?id=8015

 ****************************************
 Highly modified and adapted for Aarchon MUD by
 Clayton Richey, May 2013

 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <lualib.h>
#include <lauxlib.h>

#include "merc.h"
#include "mob_cmds.h"
#include "tables.h"
#include "lua_scripting.h"
#include "olc.h"
#include "lua_object_type.h"
#include "lua_common.h"

lua_State *g_mud_LS = NULL;  /* Lua state for entire MUD */
/* Mersenne Twister stuff - see mt19937ar.c */

OBJ_TYPE *RESET_type;
OBJ_TYPE *EXIT_type;
OBJ_TYPE *CH_type;
OBJ_TYPE *OBJ_type;
OBJ_TYPE *ROOM_type;
OBJ_TYPE *AREA_type;
OBJ_TYPE *OBJPROTO_type;
OBJ_TYPE *MOBPROTO_type;

#define MAKEOBJ( LS, obj ) OBJ_type->make( OBJ_type, LS, obj )
#define MAKECH( LS, ch ) CH_type->make( CH_type, LS, ch )
#define MAKEAREA( LS, area) AREA_type->make( AREA_type, LS, area)
#define MAKEROOM( LS, room) ROOM_type->make( ROOM_type, LS, room)

#define CHECKCH( LS, index ) CH_type->check( CH_type, LS, index )

#define LUA_LOOP_CHECK_MAX_CNT 10000 /* give 1000000 instructions */
#define LUA_LOOP_CHECK_INCREMENT 100
#define ERR_INF_LOOP      -1

#define CHECK_SECURITY( ls, sec ) if (s_ScriptSecurity<sec) luaL_error(ls, "Function requires security of %d", sec)

/* file scope variables */
bool        g_LuaScriptInProgress=FALSE;
static int         s_LoopCheckCounter;
static int         s_ScriptSecurity=0;

void init_genrand(unsigned long s);
void init_by_array(unsigned long init_key[], int key_length);
double genrand(void);

int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn);

#define MUD_LIBRARY "mud"
#define MT_LIBRARY "mt"
#define GOD_LIBRARY "god"
#define DEBUG_LIBRARY "debug"

#define MOB_ARG "mob"
#define NUM_MPROG_ARGS 8 
#define CH_ARG "ch"
#define OBJ1_ARG "obj1"
#define OBJ2_ARG "obj2"
#define TRIG_ARG "trigger"
#define TEXT1_ARG "text1"
#define TEXT2_ARG "text2"
#define VICTIM_ARG "victim"
#define TRIGTYPE_ARG "trigtype"

/* oprogs args */
#define OBJ_ARG "obj"
#define NUM_OPROG_ARGS 5 
/* OBJ2_ARG */ 
#define CH1_ARG "ch1"
#define CH2_ARG "ch2"
/* TRIG_ARG */
/* TRIGTYPE_ARG */
#define NUM_OPROG_RESULTS 1

/* aprog args */
#define AREA_ARG "area"
#define NUM_APROG_ARGS 3 
/* CH1_ARG */
/* TRIG_ARG */
/* TRIGTYPE_ARG */
#define NUM_APROG_RESULTS 1

/* rprog args */
#define ROOM_ARG "room"
#define NUM_RPROG_ARGS 6
/* CH1_ARG */
/* CH2_ARG */
/* OBJ1_ARG */
/* OBJ2_ARG */
/* TRIG_ARG */
/* TRIGTYPE_ARG */
#define NUM_RPROG_RESULTS 1

// number of items in an array
#define NUMITEMS(arg) (sizeof (arg) / sizeof (arg [0]))

int ScriptSecurity()
{
    return s_ScriptSecurity;
}


LUALIB_API int luaopen_bits(lua_State *LS);  /* Implemented in lua_bits.c */

static int optboolean (lua_State *LS, const int narg, const int def) 
{
    /* that argument not present, take default  */
    if (lua_gettop (LS) < narg)
        return def;

    /* nil will default to the default  */
    if (lua_isnil (LS, narg))
        return def;

    if (lua_isboolean (LS, narg))
        return lua_toboolean (LS, narg);

    return luaL_checknumber (LS, narg) != 0;
}

static void unregister_UD( lua_State *LS,  void *ptr )
{
    if (!LS)
    {
        bugf("NULL LS passed to unregister_UD.");
        return;
    }

    lua_getfield( LS, LUA_GLOBALSINDEX, UNREGISTER_UD_FUNCTION);
    lua_pushlightuserdata( LS, ptr );
    if (CallLuaWithTraceBack( LS, 1, 0) )
    {
        bugf ( "Error unregistering UD:\n %s",
                lua_tostring(LS, -1));
    }

}

/* unregister_lua, to be called when destroying in game structures that may
   be registered in an active lua state*/
void unregister_lua( void *ptr )
{
    if (ptr == NULL)
    {
        bugf("NULL ptr in unregister_lua.");
        return;
    }

    unregister_UD( g_mud_LS, ptr );
}


static void infinite_loop_check_hook( lua_State *LS, lua_Debug *ar)
{
    if (!g_LuaScriptInProgress)
        return;

    if ( s_LoopCheckCounter < LUA_LOOP_CHECK_MAX_CNT)
    {
        s_LoopCheckCounter++;
        return;
    }
    else
    {
        /* exit */
        luaL_error( g_mud_LS, "Interrupted infinite loop." );
        return;
    }
}


static int L_rundelay( lua_State *LS)
{
    lua_getglobal( LS, "delaytbl"); /*2*/
    if (lua_isnil( LS, -1) )
    {
        luaL_error( LS, "run_delayed_function: couldn't find delaytbl");
    }

    lua_pushvalue( LS, 1 );
    lua_gettable( LS, 2 ); /* pops key */ /*3, delaytbl entry*/

    if (lua_isnil( LS, 3) )
    {
        luaL_error( LS, "Didn't find entry in delaytbl");
    }
    /* check if the game object is still valid */
    lua_getglobal( LS, UD_TABLE_NAME); /*4, udtbl*/
    lua_getfield( LS, -2, "tableid"); /* 5 */
    lua_gettable( LS, -2 ); /* pops key */ /*5, game object*/

    if (lua_isnil( LS, -1) )
    {
        luaL_error(LS, "Couldn't find delayed function's game boject.");
    }

    lua_pop( LS, 2 );

    lua_getfield( LS, -1, "func"); 

    /* kill the entry before call in case of error */
    lua_pushvalue( LS, 1 ); /* lightud as key */
    lua_pushnil( LS ); /* nil as value */
    lua_settable( LS, 2 ); /* pops key and value */ 

    lua_call( LS, 0, 0);

    return 0;
}

void run_delayed_function( TIMER_NODE *tmr )
{
    lua_pushcfunction( g_mud_LS, L_rundelay );
    lua_pushlightuserdata( g_mud_LS, (void *)tmr );

    if (CallLuaWithTraceBack( g_mud_LS, 1, 0) )
    {
        bugf ( "Error running delayed function:\n %s",
                lua_tostring(g_mud_LS, -1));
        return;
    }

}



static int L_pagetochar (lua_State *LS)
{
    page_to_char( check_fstring(LS, 2),
            check_CH(LS,1) );

    return 0;
}

static int L_clearloopcount (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    s_LoopCheckCounter=0;

    return 0;
}

static int L_log (lua_State *LS)
{
    char buf[MSL];
    sprintf(buf, "LUA::%s", check_fstring (LS, 1));

    log_string(buf);
    return 0;
}

static int L_hour (lua_State *LS)
{
    lua_pushnumber( LS, time_info.hour );
    return 1;
}

static int L_mud_luadir( lua_State *LS)
{
    lua_pushliteral( LS, LUA_DIR);
    return 1;
}

static int L_mud_userdir( lua_State *LS)
{
    lua_pushliteral( LS, USER_DIR);
    return 1;
}

/* return tprintstr of the given global (string arg)*/
static int L_debug_show ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    lua_getglobal( LS, luaL_checkstring( LS, 1 ) );
    lua_call( LS, 1, 1 );

    return 1;
}

static const struct luaL_reg debuglib [] =
{
    {"show", L_debug_show}
};

static const struct luaL_reg mudlib [] = 
{
    {"luadir", L_mud_luadir}, 
    {"userdir",L_mud_userdir},
    {NULL, NULL}
};  /* end of mudlib */

/* Mersenne Twister pseudo-random number generator */

static int L_mt_srand (lua_State *LS)
{
    int i;

    /* allow for table of seeds */

    if (lua_istable (LS, 1))
    {
        size_t length = lua_objlen (LS, 1);  /* size of table */
        if (length == 0)
            luaL_error (LS, "mt.srand table must not be empty");

        unsigned long * v = (unsigned long *) malloc (sizeof (unsigned long) * length);
        if (!v)
            luaL_error (LS, "Cannot allocate memory for seeds table");

        for (i = 1; i <= length; i++)
        {
            lua_rawgeti (LS, 1, i);  /* get number */
            if (!lua_isnumber (LS, -1))
            {
                free (v);  /* get rid of table now */
                luaL_error (LS, "mt.srand table must consist of numbers");
            }
            v [i - 1] = luaL_checknumber (LS, -1);  
            lua_pop (LS, 1);   /* remove value   */
        }    
        init_by_array (&v [0], length);
        free (v);  /* get rid of table now */
    }
    else
        init_genrand (luaL_checknumber (LS, 1));

    return 0;
} /* end of L_mt_srand */

static int L_mt_rand (lua_State *LS)
{
    lua_pushnumber (LS, genrand ());
    return 1;
} /* end of L_mt_rand */

static const struct luaL_reg mtlib[] = {

    {"srand", L_mt_srand},  /* seed */
    {"rand", L_mt_rand},    /* generate */
    {NULL, NULL}
};

static void RegisterGlobalFunctions(lua_State *LS)
{
    /* These are registed in the main script
       space then the appropriate ones are 
       exposed to scripts in main_lib */
    /* checks */
    lua_register(LS,"hour",        L_hour);

    /* other */
    lua_register(LS,"log",         L_log );
    lua_register(LS,"clearloopcount", L_clearloopcount);

    /* not meant for main_lib */
    lua_register(LS,"cancel", L_cancel); 

    register_globals( LS );
}


static int RegisterLuaRoutines (lua_State *LS)
{
    time_t timer;
    time (&timer);

    /* register all mud.xxx routines */
    luaL_register (LS, MUD_LIBRARY, mudlib);

    /* register all god.xxx routines */
    //luaL_register (LS, GOD_LIBRARY, godlib);

    /* register all debug.xxx routines */
    luaL_register (LS, DEBUG_LIBRARY, debuglib);

    luaopen_bits (LS);     /* bit manipulation */
    luaL_register (LS, MT_LIBRARY, mtlib);  /* Mersenne Twister */

    /* Marsenne Twister generator  */
    init_genrand (timer);

    RegisterGlobalFunctions(LS);

    /* meta tables to identify object types */
/*    luaL_newmetatable(LS, OBJ_META);
    luaL_register (LS, NULL, OBJ_metatable);
    luaL_newmetatable(LS, ROOM_META);
    luaL_register (LS, NULL, ROOM_metatable);
    luaL_newmetatable(LS, OBJPROTO_META);
    luaL_register (LS, NULL, OBJPROTO_metatable);
    luaL_newmetatable(LS, AREA_META);
    luaL_register (LS, NULL, AREA_metatable);
    luaL_newmetatable(LS, MOBPROTO_META);
    luaL_register (LS, NULL, MOBPROTO_metatable);
*/
    /* our metatable for lightuserdata */
    //luaL_newmetatable(LS, UD_META);

    RESET_type=RESET_init(LS);
    EXIT_type=EXIT_init(LS);
    CH_type=CH_init(LS);
    OBJ_type=OBJ_init(LS);
    ROOM_type=ROOM_init(LS);
    AREA_type=AREA_init(LS);
    OBJPROTO_type=OBJPROTO_init(LS);
    MOBPROTO_type=MOBPROTO_init(LS);
    return 0;

}  /* end of RegisterLuaRoutines */

int GetLuaMemoryUsage()
{
    return lua_gc( g_mud_LS, LUA_GCCOUNT, 0);
}

int GetLuaGameObjectCount()
{
    lua_getglobal( g_mud_LS, "UdCnt");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 1) )
    {
        bugf ( "Error with UdCnt:\n %s",
                lua_tostring(g_mud_LS, -1));
        return -1;
    }

    return (int)luaL_checknumber( g_mud_LS, -1 ); 

}

int GetLuaEnvironmentCount()
{
    lua_getglobal( g_mud_LS, "EnvCnt");  
    if (CallLuaWithTraceBack( g_mud_LS, 0, 1) )
    {
        bugf ( "Error with EnvCnt:\n %s",
                lua_tostring(g_mud_LS, -1));
        return -1;
    }

    return (int)luaL_checknumber( g_mud_LS, -1 );
}

void open_lua ()
{
    lua_State *LS = luaL_newstate ();   /* opens Lua */
    g_mud_LS = LS;

    if (g_mud_LS == NULL)
    {
        bugf("Cannot open Lua state");
        return;  /* catastrophic failure */
    }

    luaL_openlibs (LS);    /* open all standard libraries */

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

static void call_lua_function (CHAR_DATA * ch, 
        lua_State *LS, 
        const char * fname, 
        const int nArgs)

{

    if (CallLuaWithTraceBack (LS, nArgs, 0))
    {
        bugf ("Error executing Lua function %s:\n %s\n** Last player command: %s", 
                fname, 
                lua_tostring(LS, -1),
                last_command);

        if (ch && !IS_IMMORTAL(ch))
        {
            //set_char_color( AT_YELLOW, ch );
            ptc (ch, "** A server scripting error occurred, please notify an administrator.\n"); 
        }  /* end of not immortal */


        lua_pop(LS, 1);  /* pop error */

    }  /* end of error */

}  /* end of call_lua_function */


bool lua_load_mprog( lua_State *LS, int vnum, const char *code)
{
    char buf[MAX_SCRIPT_LENGTH + MSL]; /* Allow big strings from loadscript */

    if ( strlen(code) >= MAX_SCRIPT_LENGTH )
    {
        bugf("MPROG script %d exceeds %d characters.",
                vnum, MAX_SCRIPT_LENGTH);
        return FALSE;
    }

    sprintf(buf, "function M_%d (%s,%s,%s,%s,%s,%s,%s,%s)"
            "%s\n"
            "end",
            vnum,
            /*MOB_ARG,*/ CH_ARG, TRIG_ARG, OBJ1_ARG,
            OBJ2_ARG, TEXT1_ARG, TEXT2_ARG, VICTIM_ARG, TRIGTYPE_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 0))
    {
        bugf ( "LUA mprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));
        /* bad code, let's kill it */
        sprintf(buf, "M_%d", vnum);
        lua_pushnil( LS );
        lua_setglobal( LS, buf);

        return FALSE;
    }
    else return TRUE;

}

bool lua_load_aprog( lua_State *LS, int vnum, const char *code)
{
    char buf[MAX_SCRIPT_LENGTH + MSL]; /* Allow big strings from loadscript */

    if ( strlen(code) >= MAX_SCRIPT_LENGTH )
    {
        bugf("APROG script %d exceeds %d characters.",
                vnum, MAX_SCRIPT_LENGTH);
        return FALSE;
    }

    sprintf(buf, "function A_%d (%s,%s,%s)"
            "%s\n"
            "end",
            vnum,
            CH1_ARG, TRIG_ARG, TRIGTYPE_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 0))
    {
        bugf ( "LUA aprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));
        /* bad code, let's kill it */
        sprintf(buf, "A_%d", vnum);
        lua_pushnil( LS );
        lua_setglobal( LS, buf);

        return FALSE;
    }
    else return TRUE;
}

bool lua_load_rprog( lua_State *LS, int vnum, const char *code)
{
    char buf[MAX_SCRIPT_LENGTH + MSL]; /* Allow big strings from loadscript */

    if ( strlen(code) >= MAX_SCRIPT_LENGTH )
    {
        bugf("RPROG script %d exceeds %d characters.",
                vnum, MAX_SCRIPT_LENGTH);
        return FALSE;
    }

    sprintf(buf, "function R_%d (%s,%s,%s,%s,%s,%s)"
            "%s\n"
            "end",
            vnum,
            CH1_ARG, CH2_ARG, OBJ1_ARG, OBJ2_ARG,
            TRIG_ARG, TRIGTYPE_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 0))
    {
        bugf ( "LUA Rprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));
        /* bad code, let's kill it */
        sprintf(buf, "R_%d", vnum);
        lua_pushnil( LS );
        lua_setglobal( LS, buf);

        return FALSE;
    }
    else return TRUE;
}

bool lua_load_oprog( lua_State *LS, int vnum, const char *code)
{
    char buf[MAX_SCRIPT_LENGTH + MSL]; /* Allow big strings from loadscript */

    if ( strlen(code) >= MAX_SCRIPT_LENGTH )
    {
        bugf("OPROG script %d exceeds %d characters.",
                vnum, MAX_SCRIPT_LENGTH);
        return FALSE;
    }

    sprintf(buf, "function O_%d (%s,%s,%s,%s,%s)"
            "%s\n"
            "end",
            vnum,
            OBJ2_ARG, CH1_ARG, CH2_ARG, TRIG_ARG, TRIGTYPE_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 0))
    {
        bugf ( "LUA oprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));
        /* bad code, let's kill it */
        sprintf(buf, "O_%d", vnum);
        lua_pushnil( LS );
        lua_setglobal( LS, buf);

        return FALSE;
    }
    else return TRUE;
}

#define LOADSCRIPT_VNUM 0
/* lua_mob_program
   lua equivalent of program_flow
 */
void lua_mob_program( const char *text, int pvnum, const char *source, 
        CHAR_DATA *mob, CHAR_DATA *ch, 
        const void *arg1, sh_int arg1type, 
        const void *arg2, sh_int arg2type,
        int trig_type,
        int security ) 
{
    lua_getglobal( g_mud_LS, "mob_program_setup");

    //if (!make_ud_table( g_mud_LS, mob, UDTYPE_CH))
    if ( CH_type->make(CH_type, g_mud_LS, mob ) )
    {
        /* Most likely failed because the mob was destroyed */
        return;
    }
    if (lua_isnil(g_mud_LS, -1) )
    {
        bugf("make_ud_table pushed nil to lua_mob_program");
        return;
    }

    /* load up the script as a function so args will be local */
    char buf[MSL*2];
    sprintf(buf, "M_%d", pvnum);

    if ( pvnum==LOADSCRIPT_VNUM ) /* run with loadscript */
    {
        /* always reload */
        if ( !lua_load_mprog( g_mud_LS, pvnum, source) )
        {
            /* if we're here then loadscript was called from within
               a script so we can do a lua error */
            luaL_error( g_mud_LS, "Couldn't load script with loadscript.");
        }
        /* loaded without errors, now get it on the stack */
        lua_getglobal( g_mud_LS, buf);
    }
    else
    {
        /* check if already loaded, load if not */
        lua_getglobal( g_mud_LS, buf);

        if ( lua_isnil( g_mud_LS, -1) )
        {
            lua_remove( g_mud_LS, -1); /* Remove the nil */
            /* not loaded yet*/
            if ( !lua_load_mprog( g_mud_LS, pvnum, source) )
            {
                /* don't bother running it if there were errors */
                return;
            }

            /* loaded without errors, now get it on the stack */
            lua_getglobal( g_mud_LS, buf);
        }
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error for mob_program_setup:\n %s",
                lua_tostring(g_mud_LS, -1));
    } 

    /* CH_ARG */
    if ( !(ch && CH_type->make(CH_type, g_mud_LS,(void *) ch)))
        lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (text)
        lua_pushstring ( g_mud_LS, text);
    else lua_pushnil(g_mud_LS);

    /* OBJ1_ARG */
    if ( !((arg1type== ACT_ARG_OBJ && arg1) 
                && MAKEOBJ(g_mud_LS, arg1)))
        lua_pushnil(g_mud_LS);

    /* OBJ2_ARG */
    if ( !((arg2type== ACT_ARG_OBJ && arg2)
                && MAKEOBJ( g_mud_LS, arg2)))
        lua_pushnil(g_mud_LS);

    /* TEXT1_ARG */
    if (arg1type== ACT_ARG_TEXT && arg1)
        lua_pushstring ( g_mud_LS, (char *)arg1);
    else lua_pushnil(g_mud_LS);

    /* TEXT2_ARG */
    if (arg2type== ACT_ARG_TEXT && arg2)
        lua_pushstring ( g_mud_LS, (char *)arg2);
    else lua_pushnil(g_mud_LS);

    /* VICTIM_ARG */
    if ( !((arg2type== ACT_ARG_CHARACTER && arg2)
                && CH_type->make( CH_type, g_mud_LS, arg2)) )
        lua_pushnil(g_mud_LS);

    /* TRIGTYPE_ARG */
    lua_pushstring ( g_mud_LS, flag_stat_string( mprog_flags, trig_type) );


    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        s_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        s_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (g_mud_LS, NUM_MPROG_ARGS, 0) ;
    if (error > 0 )
    {
        bugf ( "LUA mprog error for %s(%d), mprog %d:\n %s",
                mob->name,
                mob->pIndexData ? mob->pIndexData->vnum : 0,
                pvnum,
                lua_tostring(g_mud_LS, -1));
    }

    if ( !nest )
    {
        g_LuaScriptInProgress=FALSE;
        lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
        s_ScriptSecurity=0; /*just in case*/
    }
}


bool lua_obj_program( const char *trigger, int pvnum, const char *source, 
        OBJ_DATA *obj, OBJ_DATA *obj2,CHAR_DATA *ch1, CHAR_DATA *ch2,
        int trig_type,
        int security ) 
{
    bool result=FALSE;

    lua_getglobal( g_mud_LS, "obj_program_setup");

    if (!MAKEOBJ( g_mud_LS, obj))
    {
        /* Most likely failed because the obj was destroyed */
        return;
    }

    if (lua_isnil(g_mud_LS, -1) )
    {
        bugf("make_ud_table pushed nil to lua_obj_program");
        return FALSE;
    }

    /* load up the script as a function so args will be local */
    char buf[MSL*2];
    sprintf(buf, "O_%d", pvnum);

    if ( pvnum==LOADSCRIPT_VNUM ) /* run with loadscript */
    {
        /* always reload */
        if ( !lua_load_oprog( g_mud_LS, pvnum, source) )
        {
            /* if we're here then loadscript was called from within
               a script so we can do a lua error */
            luaL_error( g_mud_LS, "Couldn't load script with loadscript.");
        }
        /* loaded without errors, now get it on the stack */
        lua_getglobal( g_mud_LS, buf);
    }
    else
    {   
        lua_getglobal( g_mud_LS, buf);
        if ( lua_isnil( g_mud_LS, -1) )
        {
            lua_remove( g_mud_LS, -1); /* Remove the nil */
            /* not loaded yet*/
            if ( !lua_load_oprog( g_mud_LS, pvnum, source) )
            {
                /* don't bother running it if there were errors */
                return FALSE;
            }

            /* loaded without errors, now get it on the stack */
            lua_getglobal( g_mud_LS, buf);
        }
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running obj_program_setup: %s",
                lua_tostring(g_mud_LS, -1));
    }

    /* OBJ2_ARG */
    if ( !(obj2 && MAKEOBJ(g_mud_LS,(void *) obj2)))
        lua_pushnil(g_mud_LS);

    /* CH1_ARG */
    if ( !(ch1 && MAKECH(g_mud_LS,(void *) ch1)))
        lua_pushnil(g_mud_LS);

    /* CH2_ARG */
    if ( !(ch2 && MAKECH(g_mud_LS,(void *) ch2)))
        lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (trigger)
        lua_pushstring(g_mud_LS,trigger);
    else lua_pushnil(g_mud_LS);

    /* TRIGTYPE_ARG */
    lua_pushstring ( g_mud_LS, flag_stat_string( oprog_flags, trig_type) );

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        s_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        s_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (g_mud_LS, NUM_OPROG_ARGS, NUM_OPROG_RESULTS) ;
    if (error > 0 )
    {
        bugf ( "LUA oprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(g_mud_LS, -1));
    }
    else
    {
        result=lua_toboolean (g_mud_LS, -1);
    }

    if ( !nest )
    {
        g_LuaScriptInProgress=FALSE;
        lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
        s_ScriptSecurity=0; /* just in case */
    }
    return result;
}

bool lua_area_program( const char *trigger, int pvnum, const char *source, 
        AREA_DATA *area, CHAR_DATA *ch1,
        int trig_type,
        int security ) 
{
    bool result=FALSE;

    lua_getglobal( g_mud_LS, "area_program_setup");

    if (!MAKEAREA( g_mud_LS, area))
    {
        bugf("Make_ud_table failed in lua_area_program. %s : %d",
                area->name,
                pvnum);
        return;
    }

    if (lua_isnil(g_mud_LS, -1) )
    {
        bugf("make_ud_table pushed nil to lua_area_program");
        return FALSE;
    }

    /* load up the script as a function so args will be local */
    char buf[MSL*2];
    sprintf(buf, "A_%d", pvnum);

    if ( pvnum==LOADSCRIPT_VNUM ) /* run with loadscript */
    {
        /* always reload */
        if ( !lua_load_aprog( g_mud_LS, pvnum, source) )
        {
            /* if we're here then loadscript was called from within
               a script so we can do a lua error */
            luaL_error( g_mud_LS, "Couldn't load script with loadscript.");
        }
        /* loaded without errors, now get it on the stack */
        lua_getglobal( g_mud_LS, buf);
    }
    else
    {
        lua_getglobal( g_mud_LS, buf);
        if ( lua_isnil( g_mud_LS, -1) )
        {
            lua_remove( g_mud_LS, -1); /* Remove the nil */
            /* not loaded yet*/
            if ( !lua_load_aprog( g_mud_LS, pvnum, source) )
            {
                /* don't bother running it if there were errors */
                return FALSE;
            }

            /* loaded without errors, now get it on the stack */
            lua_getglobal( g_mud_LS, buf);
        }
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running area_program_setup: %s",
                lua_tostring(g_mud_LS, -1));
    }

    /* CH1_ARG */
    if ( !(ch1 && MAKECH(g_mud_LS,(void *) ch1)))
        lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (trigger)
        lua_pushstring(g_mud_LS,trigger);
    else lua_pushnil(g_mud_LS);

    /* TRIGTYPE_ARG */
    lua_pushstring ( g_mud_LS, flag_stat_string( aprog_flags, trig_type) );

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        s_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        s_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (g_mud_LS, NUM_APROG_ARGS, NUM_APROG_RESULTS) ;
    if (error > 0 )
    {
        bugf ( "LUA aprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(g_mud_LS, -1));
    }
    else
    {
        result=lua_toboolean (g_mud_LS, -1);
    }
    if (!nest)
    {
        g_LuaScriptInProgress=FALSE;
        s_ScriptSecurity=0; /* just in case */
        lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
    }
    return result;
}

bool lua_room_program( const char *trigger, int pvnum, const char *source, 
        ROOM_INDEX_DATA *room, 
        CHAR_DATA *ch1, CHAR_DATA *ch2, OBJ_DATA *obj1, OBJ_DATA *obj2,
        int trig_type,
        int security ) 
{
    bool result=FALSE;

    lua_getglobal( g_mud_LS, "room_program_setup");

    if (!MAKEROOM( g_mud_LS, room))
    {
        bugf("Make_ud_table failed in lua_room_program. %d : %d",
                room->vnum,
                pvnum);
        return;
    }

    if (lua_isnil(g_mud_LS, -1) )
    {
        bugf("make_ud_table pushed nil to lua_room_program");
        return FALSE;
    }

    /* load up the script as a function so args will be local */
    char buf[MSL*2];
    sprintf(buf, "R_%d", pvnum);

    if ( pvnum==LOADSCRIPT_VNUM ) /* run with loadscript */
    {
        /* always reload */
        if ( !lua_load_rprog( g_mud_LS, pvnum, source) )
        {
            /* if we're here then loadscript was called from within
               a script so we can do a lua error */
            luaL_error( g_mud_LS, "Couldn't load script with loadscript.");
        }
        /* loaded without errors, now get it on the stack */
        lua_getglobal( g_mud_LS, buf);
    }
    else
    {
        lua_getglobal( g_mud_LS, buf);
        if ( lua_isnil( g_mud_LS, -1) )
        {
            lua_remove( g_mud_LS, -1); /* Remove the nil */
            /* not loaded yet*/
            if ( !lua_load_rprog( g_mud_LS, pvnum, source) )
            {
                /* don't bother running it if there were errors */
                return FALSE;
            }

            /* loaded without errors, now get it on the stack */
            lua_getglobal( g_mud_LS, buf);
        }
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running room_program_setup: %s",
                lua_tostring(g_mud_LS, -1));
    }

    /* CH1_ARG */
    if ( !(ch1 && MAKECH(g_mud_LS,(void *) ch1)))
        lua_pushnil(g_mud_LS);
        
    /* CH2_ARG */
    if ( !(ch2 && MAKECH(g_mud_LS,(void *) ch2)))
        lua_pushnil(g_mud_LS);

    /* OBJ1_ARG */
    if ( !(obj1 && MAKEOBJ(g_mud_LS,(void *) obj1)))
        lua_pushnil(g_mud_LS);

    /* OBJ2_ARG */
    if ( !(obj2 && MAKEOBJ(g_mud_LS,(void *) obj2)))
        lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (trigger)
        lua_pushstring(g_mud_LS,trigger);
    else lua_pushnil(g_mud_LS);

    /* TRIGTYPE_ARG */
    lua_pushstring ( g_mud_LS, flag_stat_string( rprog_flags, trig_type) );

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        s_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        s_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (g_mud_LS, NUM_RPROG_ARGS, NUM_RPROG_RESULTS) ;
    if (error > 0 )
    {
        bugf ( "LUA rprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(g_mud_LS, -1));
    }
    else
    {
        result=lua_toboolean (g_mud_LS, -1);
    }
    if (!nest)
    {
        g_LuaScriptInProgress=FALSE;
        s_ScriptSecurity=0; /* just in case */
        lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
    }
    return result;
}

void do_lboard( CHAR_DATA *ch, char *argument)
{
    lua_getglobal(g_mud_LS, "do_lboard");
    MAKECH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf ( "Error with do_lboard:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void do_lhistory( CHAR_DATA *ch, char *argument)
{
    lua_getglobal(g_mud_LS, "do_lhistory");
    MAKECH(g_mud_LS, ch);
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

void save_lboards()
{
    lua_getglobal( g_mud_LS, "save_lboards");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with save_lboard:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }  
}

void load_lboards()
{
    lua_getglobal( g_mud_LS, "load_lboards");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with load_lboards:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void check_lboard_reset()
{
    lua_getglobal( g_mud_LS, "check_lboard_reset");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with check_lboard_resets:\n %s",
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
        d->lua.wait=FALSE;
        d->lua.incmpl=FALSE;

        lua_unregister_desc(d);

        ptc(d->character, "Exited lua interpreter.\n\r");
        return TRUE;
    }

    if (!strcmp( d->incomm, "WAIT") )
    {
        if (d->lua.incmpl)
        {
            ptc(d->character, "Can't enter WAIT mode with incomplete statement.\n\r"
                              "Finish statement or exit and re-enter interpreter.\n\r");
            return TRUE;
        }

        /* set wait mode for multiline chunks*/
        if (d->lua.wait)
            ptc(d->character, "Already in WAIT mode.\n\r");
        else
            d->lua.wait=TRUE;
        return TRUE;
    }

    if (!strcmp( d->incomm, "GO") )
    {
        /* turn WAIT mode off and go for it */
        if (!d->lua.wait)
        {
            ptc( d->character, "Can only use GO to end WAIT mode.\n\r");
            return TRUE; 
        }
        d->lua.wait=FALSE;
        lua_getglobal( g_mud_LS, "go_lua_interpret");
    }
    else if (d->lua.wait) /* WAIT mode enabled for multiline chunk */
    {
        lua_getglobal( g_mud_LS, "wait_lua_interpret");
    }
    else
    {
        lua_getglobal( g_mud_LS, "run_lua_interpret"); //what we'll call if no errors
    }

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
        d->lua.wait=FALSE;

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
        d->lua.wait=FALSE;

        ptc(d->character, "Exited lua interpreter.\n\r");
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }

    /* if we're here then we just need to push the string and call the func */
    lua_pushstring( g_mud_LS, d->incomm);

    s_ScriptSecurity= d->character->pcdata->security;
    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        ptc(d->character,  "LUA error for lua_interpret:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        d->lua.incmpl=FALSE; //force it whehter it was or wasn't
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


    s_ScriptSecurity=0;

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
    }
}

void do_luai( CHAR_DATA *ch, char *argument)
{
    if IS_NPC(ch)
        return;

    char arg1[MSL];
    char *name;

    argument=one_argument(argument, arg1);

    void *victim=NULL;
    OBJ_TYPE *type;

    if ( arg1[0]== '\0' )
    {
        victim=(void *)ch;
        type=CH_type;
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
        type= CH_type;
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
        type=OBJ_type;
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
        type=AREA_type;
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
        type=ROOM_type;
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
    if ( !type->make(type, g_mud_LS, victim) )
    {
        bugf("do_luai: couldn't make udtable for %s, argument %s",
                type->type_name, argument);
        lua_settop(g_mud_LS, 0);
        return;
    }

    if ( type == CH_type )
    {
        lua_pushliteral( g_mud_LS, "mob"); 
    }
    else if ( type == OBJ_type )
    {
        lua_pushliteral( g_mud_LS, "obj"); 
    }
    else if ( type == AREA_type )
    {
        lua_pushliteral( g_mud_LS, "area"); 
    }
    else if ( type == ROOM_type )
    {
        lua_pushliteral( g_mud_LS, "room"); 
    }
    else
    {
            bugf("do_luai: invalid type %s", type->type_name);
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
                luaL_checkstring( g_mud_LS, -1));
        lua_settop(g_mud_LS, 0);
        return;
    }

    /* finally, if everything worked out, we can set this stuff */
    ch->desc->lua.interpret=TRUE;
    ch->desc->lua.wait=FALSE;
    ch->desc->lua.incmpl=FALSE;

    ptc(ch, "Entered lua interpreter mode for for %s %s\n\r", 
            type->type_name,
            name);
    ptc(ch, "Use @ on a blank line to exit.\n\r");
    ptc(ch, "Use WAIT to enable WAIT mode for multiline chunks.\n\r");
    ptc(ch, "Use GO on a blank line to end WAIT mode and process the buffer.\n\r");
    lua_settop(g_mud_LS, 0);
    return;
}
