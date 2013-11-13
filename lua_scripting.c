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
#include "lua_arclib.h"
#include "lua_main.h"



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


bool lua_load_mprog( lua_State *LS, int vnum, const char *code )
{
    char buf[MAX_SCRIPT_LENGTH + MSL]; /* Allow big strings from loadscript */

    if ( strlen(code) >= MAX_SCRIPT_LENGTH )
    {
        bugf("MPROG script %d exceeds %d characters.",
                vnum, MAX_SCRIPT_LENGTH);
        return FALSE;
    }

    sprintf(buf, "return function (%s,%s,%s,%s,%s,%s,%s,%s)\n"
            "%s\n"
            "end",
            CH_ARG, TRIG_ARG, OBJ1_ARG,
            OBJ2_ARG, TEXT1_ARG, TEXT2_ARG, VICTIM_ARG, TRIGTYPE_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 1))
    {
        if ( vnum == LOADSCRIPT_VNUM )
            luaL_error( LS, "Error loading script:\n%s", lua_tostring( LS, -1));
        bugf ( "LUA mprog error loading vnum%d:\n %s",
                vnum,
                lua_tostring( LS, -1));
        lua_settop( LS, 0 );

        return FALSE;
    }
    else
    {
       return TRUE;
    }

}

void check_mprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_mprog( LS, vnum, code ))
        lua_pop(LS, 1);
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

    sprintf(buf, "return function (%s,%s,%s)\n"
            "%s\n"
            "end",
            CH1_ARG, TRIG_ARG, TRIGTYPE_ARG,
            code);

    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 0))
    {
        if ( vnum == LOADSCRIPT_VNUM )
            luaL_error( LS, "Error loading script:\n%s", lua_tostring( LS, -1));
        bugf ( "LUA aprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));
        return FALSE;
    }
    else return TRUE;
}

void check_aprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_aprog( LS, vnum, code ))
        lua_pop(LS, 1);
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

    sprintf(buf, "return function (%s,%s,%s,%s,%s,%s)\n"
            "%s\n"
            "end",
            CH1_ARG, CH2_ARG, OBJ1_ARG, OBJ2_ARG,
            TRIG_ARG, TRIGTYPE_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 0))
    {
        if ( vnum == LOADSCRIPT_VNUM )
            luaL_error( LS, "Error loading script:\n%s", lua_tostring( LS, -1));
        bugf ( "LUA Rprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));
        return FALSE;
    }
    else return TRUE;
}

void check_rprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_rprog( LS, vnum, code ))
        lua_pop(LS, 1);
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

    sprintf(buf, "return function (%s,%s,%s,%s,%s)"
            "%s\n"
            "end",
            OBJ2_ARG, CH1_ARG, CH2_ARG, TRIG_ARG, TRIGTYPE_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 0))
    {
        if ( vnum == LOADSCRIPT_VNUM )
            luaL_error( LS, "Error loading script:\n%s", lua_tostring( LS, -1));
        bugf ( "LUA oprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));

        return FALSE;
    }
    else return TRUE;
}

void check_oprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_oprog( LS, vnum, code ))
        lua_pop(LS, 1);
}

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

    if ( !make_CH( g_mud_LS, mob ) )
    {
        /* Most likely failed because the mob was destroyed */
        return;
    }
    if (lua_isnil(g_mud_LS, -1) )
    {
        bugf("make_ud_table pushed nil to lua_mob_program");
        return;
    }

    if ( !lua_load_mprog( g_mud_LS, pvnum, source) )
    {
        return;
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error for mob_program_setup:\n %s",
                lua_tostring(g_mud_LS, -1));
    } 

    /* CH_ARG */
    if ( !(ch && make_CH( g_mud_LS,(void *) ch)))
        lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (text)
        lua_pushstring ( g_mud_LS, text);
    else lua_pushnil(g_mud_LS);

    /* OBJ1_ARG */
    if ( !((arg1type== ACT_ARG_OBJ && arg1) 
                && make_OBJ(g_mud_LS, arg1)))
        lua_pushnil(g_mud_LS);

    /* OBJ2_ARG */
    if ( !((arg2type== ACT_ARG_OBJ && arg2)
                && make_OBJ( g_mud_LS, arg2)))
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
                && make_CH( g_mud_LS, arg2)) )
        lua_pushnil(g_mud_LS);

    /* TRIGTYPE_ARG */
    lua_pushstring ( g_mud_LS, flag_stat_string( mprog_flags, trig_type) );


    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
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
        g_ScriptSecurity=0; /*just in case*/
    }
}


bool lua_obj_program( const char *trigger, int pvnum, const char *source, 
        OBJ_DATA *obj, OBJ_DATA *obj2,CHAR_DATA *ch1, CHAR_DATA *ch2,
        int trig_type,
        int security ) 
{
    bool result=FALSE;

    lua_getglobal( g_mud_LS, "obj_program_setup");

    if (!make_OBJ( g_mud_LS, obj))
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

    if ( !lua_load_oprog( g_mud_LS, pvnum, source) )
    {
        return;
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running obj_program_setup: %s",
                lua_tostring(g_mud_LS, -1));
    }

    /* OBJ2_ARG */
    if ( !(obj2 && make_OBJ(g_mud_LS,(void *) obj2)))
        lua_pushnil(g_mud_LS);

    /* CH1_ARG */
    if ( !(ch1 && make_CH(g_mud_LS,(void *) ch1)))
        lua_pushnil(g_mud_LS);

    /* CH2_ARG */
    if ( !(ch2 && make_CH(g_mud_LS,(void *) ch2)))
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
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
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
        g_ScriptSecurity=0; /* just in case */
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

    if (!make_AREA( g_mud_LS, area))
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

    if ( !lua_load_aprog( g_mud_LS, pvnum, source) )
    {
        return;
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running area_program_setup: %s",
                lua_tostring(g_mud_LS, -1));
    }

    /* CH1_ARG */
    if ( !(ch1 && make_CH(g_mud_LS,(void *) ch1)))
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
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
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
        g_ScriptSecurity=0; /* just in case */
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

    if (!make_ROOM( g_mud_LS, room))
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

    if ( !lua_load_rprog( g_mud_LS, pvnum, source) )
    {
        return;
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running room_program_setup: %s",
                lua_tostring(g_mud_LS, -1));
    }

    /* CH1_ARG */
    if ( !(ch1 && make_CH(g_mud_LS,(void *) ch1)))
        lua_pushnil(g_mud_LS);
        
    /* CH2_ARG */
    if ( !(ch2 && make_CH(g_mud_LS,(void *) ch2)))
        lua_pushnil(g_mud_LS);

    /* OBJ1_ARG */
    if ( !(obj1 && make_OBJ(g_mud_LS,(void *) obj1)))
        lua_pushnil(g_mud_LS);

    /* OBJ2_ARG */
    if ( !(obj2 && make_OBJ(g_mud_LS,(void *) obj2)))
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
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
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
        g_ScriptSecurity=0; /* just in case */
        lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
    }
    return result;
}
