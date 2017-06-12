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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "merc.h"
#include "mob_cmds.h"
#include "tables.h"
#include "lua_scripting.h"
#include "olc.h"
#include "lua_arclib.h"
#include "lua_main.h"


struct lua_script_type
{
    const char *name;
    int narg;
    int nrtn;
    const char *arg_list[];
}; 


#define MPARGS { "ch", "trigger", "obj1", "obj2", "text1", "text2", "victim", "trigtype" }
static struct lua_script_type const mprog_type =
{
    .name = "MPROG",
    .narg = sizeof((const char *[])MPARGS) / sizeof(const char *),
    .nrtn = 0,
    .arg_list = MPARGS
    
};
#undef MPARGS

#define OPARGS { "obj2", "ch1", "ch2", "trigger", "trigtype" }
static struct lua_script_type const oprog_type =
{
    .name = "OPROG",
    .narg = sizeof((const char *[])OPARGS) / sizeof(const char *),
    .nrtn = 1,
    .arg_list = OPARGS
};
#undef OPARGS

#define RPARGS { "ch1", "ch2", "obj1", "obj2", "text1", "trigger", "trigtype" }
static struct lua_script_type const rprog_type =
{
    .name = "RPROG",
    .narg = sizeof((const char *[])RPARGS) / sizeof(const char *),
    .nrtn = 1,
    .arg_list = RPARGS
};
#undef RPARGS

#define APARGS { "ch1", "trigger", "trigtype" }
static struct lua_script_type const aprog_type =
{
    .name = "APROG",
    .narg = sizeof((const char *[])APARGS) / sizeof(const char *),
    .nrtn = 1,
    .arg_list = APARGS
};
#undef APARGS


static bool lua_load_prog( lua_State *LS, int vnum, const char *code, struct lua_script_type const *script_type)
{
    char buf[MAX_SCRIPT_LENGTH + MSL]; /* Allow big strings from loadscript */

    if ( strlen(code) >= MAX_SCRIPT_LENGTH )
    {
        bugf("%s script %d exceeds %d characters.",
                script_type->name, vnum, MAX_SCRIPT_LENGTH);
        return FALSE;
    }

    strcpy( buf, "return function (" );
    
    int i;
    for (i=0; i < script_type->narg ; i++)
    {
        if (i>0)
        {
            strcat( buf, ",");
        }
        strcat( buf, script_type->arg_list[i] );
    }
    strcat( buf, ") ");
    strcat( buf, code );
    strcat( buf, "\nend");

    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 1))
    {
        if ( vnum == LOADSCRIPT_VNUM )
        {
            luaL_error( LS, "Error loading script:\n%s", lua_tostring( LS, -1));
        }
        bugf ( "LUA %s error loading vnum %d:\n %s",
                script_type->name,
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

bool lua_load_mprog( lua_State *LS, int vnum, const char *code )
{
    return lua_load_prog( LS, vnum, code, &mprog_type);
}

void check_mprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_mprog( LS, vnum, code ))
        lua_pop(LS, 1);
}

bool lua_load_aprog( lua_State *LS, int vnum, const char *code)
{
    return lua_load_prog( LS, vnum, code, &aprog_type );
}

void check_aprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_aprog( LS, vnum, code ))
        lua_pop(LS, 1);
}

bool lua_load_rprog( lua_State *LS, int vnum, const char *code)
{
    return lua_load_prog( LS, vnum, code, &rprog_type );
}

void check_rprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_rprog( LS, vnum, code ))
        lua_pop(LS, 1);
}

bool lua_load_oprog( lua_State *LS, int vnum, const char *code )
{
    return lua_load_prog( LS, vnum, code, &oprog_type);   
}

void check_oprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_oprog( LS, vnum, code ))
        lua_pop(LS, 1);
}

/* lua_mob_program
   lua equivalent of program_flow
 */
void lua_mob_program( lua_State *LS, const char *text, int pvnum, const char *source, 
        CHAR_DATA *mob, CHAR_DATA *ch, 
        const void *arg1, sh_int arg1type, 
        const void *arg2, sh_int arg2type,
        int trig_type,
        int security ) 

{
    int narg;
    int nrtn;
    int orig_top = lua_gettop( LS );

    lua_getglobal( LS, "mob_program_setup");

    if ( !push_CH( LS, mob ) )
    {
        /* Most likely failed because the gobj was destroyed */
        return;
    }
    if (lua_isnil(LS, -1) )
    {
        bugf("push_ud_table pushed nil to lua_mob_program");
        return;
    }

    if ( pvnum == RUNDELAY_VNUM )
    {
        /* args should have been at top of stack, func at -1 */
        lua_pushvalue( LS, orig_top - 1);
    }
    else if ( !lua_load_mprog( LS, pvnum, source) )
    {
        return;
    }

    int error=CallLuaWithTraceBack (LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error for mob_program_setup:\n %s",
                lua_tostring(LS, -1));
    } 

    if ( pvnum == RUNDELAY_VNUM )
    {
        /* args should be at orig_top index */
        if ( lua_isnil( LS, orig_top ) )
        {
            narg = 0;
        }
        else
        {
            int ref_top = lua_gettop( LS );
            push_ref( LS, REF_UNPACK);
            lua_pushvalue( LS, orig_top );
            lua_pushinteger( LS, 1 );
            push_ref( LS, REF_TABLE_MAXN );
            lua_pushvalue( LS, orig_top );
            lua_call( LS, 1, 1); // call table.maxn
            lua_call( LS, 3, LUA_MULTRET ); // call unpack

            narg = lua_gettop( LS ) - ref_top;
        }
        nrtn = 0;
    }
    else
    {
        /* CH_ARG */
        if ( !(ch && push_CH( LS, ch)))
            lua_pushnil(LS);

        /* TRIG_ARG */
        if (text)
            lua_pushstring ( LS, text);
        else lua_pushnil(LS);

        /* OBJ1_ARG */
        if ( !((arg1type== ACT_ARG_OBJ && arg1) 
                    && push_OBJ(LS, (OBJ_DATA*)arg1)))
            lua_pushnil(LS);

        /* OBJ2_ARG */
        if ( !((arg2type== ACT_ARG_OBJ && arg2)
                    && push_OBJ( LS, (OBJ_DATA*)arg2)))
            lua_pushnil(LS);

        /* TEXT1_ARG */
        if (arg1type== ACT_ARG_TEXT && arg1)
            lua_pushstring ( LS, (char *)arg1);
        else lua_pushnil(LS);

        /* TEXT2_ARG */
        if (arg2type== ACT_ARG_TEXT && arg2)
            lua_pushstring ( LS, (char *)arg2);
        else lua_pushnil(LS);

        /* VICTIM_ARG */
        if ( !((arg2type== ACT_ARG_CHARACTER && arg2)
                    && push_CH( LS, (CHAR_DATA*)arg2)) )
            lua_pushnil(LS);

        /* TRIGTYPE_ARG */
        lua_pushstring ( LS, flag_bit_name(mprog_flags, trig_type) );

        narg = mprog_type.narg;
        nrtn = mprog_type.nrtn;
    }

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (LS, narg, nrtn) ;
    if (error > 0 )
    {
        bugf ( "LUA mprog error for %s(%d), mprog %d:\n %s",
                mob->name,
                mob->pIndexData ? mob->pIndexData->vnum : 0,
                pvnum,
                lua_tostring(LS, -1));
    }

    if ( !nest )
    {
        g_LuaScriptInProgress=FALSE;
        lua_settop (LS, 0);    /* get rid of stuff lying around */
        g_ScriptSecurity=SEC_NOSCRIPT; /*just in case*/
    }
}


bool lua_obj_program( lua_State *LS, const char *trigger, int pvnum, const char *source, 
        OBJ_DATA *obj, OBJ_DATA *obj2,CHAR_DATA *ch1, CHAR_DATA *ch2,
        int trig_type,
        int security ) 
{
    int narg;
    int nrtn;
    int orig_top = lua_gettop( LS );
    bool result=FALSE;

    lua_getglobal( LS, "obj_program_setup");

    if (!push_OBJ( LS, obj))
    {
        /* Most likely failed because the obj was destroyed */
        return FALSE;
    }

    if (lua_isnil(LS, -1) )
    {
        bugf("push_ud_table pushed nil to lua_obj_program");
        return FALSE;
    }

    if ( pvnum == RUNDELAY_VNUM )
    {
        /* args should have been at top of stack, func at -1 */
        lua_pushvalue( LS, orig_top - 1);
    }
    else if ( !lua_load_oprog( LS, pvnum, source) )
    {
        return FALSE;
    }

    int error=CallLuaWithTraceBack (LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running obj_program_setup: %s",
                lua_tostring(LS, -1));
    }

    if ( pvnum == RUNDELAY_VNUM )
    {
        /* args should be at orig_top index */
        if ( lua_isnil( LS, orig_top ) )
        {
            narg = 0;
        }
        else
        {
            int ref_top = lua_gettop( LS );
            push_ref( LS, REF_UNPACK);
            lua_pushvalue( LS, orig_top );
            lua_pushinteger( LS, 1 );
            push_ref( LS, REF_TABLE_MAXN );
            lua_pushvalue( LS, orig_top );
            lua_call( LS, 1, 1); // call table.maxn
            lua_call( LS, 3, LUA_MULTRET ); // call unpack

            narg = lua_gettop( LS ) - ref_top;
        }
        nrtn = 0;
    }
    else
    {
        /* OBJ2_ARG */
        if ( !(obj2 && push_OBJ(LS,(void *) obj2)))
            lua_pushnil(LS);

        /* CH1_ARG */
        if ( !(ch1 && push_CH(LS,(void *) ch1)))
            lua_pushnil(LS);

        /* CH2_ARG */
        if ( !(ch2 && push_CH(LS,(void *) ch2)))
            lua_pushnil(LS);

        /* TRIG_ARG */
        if (trigger)
            lua_pushstring(LS,trigger);
        else lua_pushnil(LS);

        /* TRIGTYPE_ARG */
        lua_pushstring ( LS, flag_bit_name(oprog_flags, trig_type) );

        narg = oprog_type.narg;
        nrtn = oprog_type.nrtn;        
    }

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (LS, narg, nrtn) ;
    if (error > 0 )
    {
        bugf ( "LUA oprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(LS, -1));
    }
    else
    {
        result=lua_toboolean (LS, -1);
    }

    if ( !nest )
    {
        g_LuaScriptInProgress=FALSE;
        lua_settop (LS, 0);    /* get rid of stuff lying around */
        g_ScriptSecurity=SEC_NOSCRIPT; /* just in case */
    }
    return result;
}

bool lua_area_program( lua_State *LS, const char *trigger, int pvnum, const char *source, 
        AREA_DATA *area, CHAR_DATA *ch1,
        int trig_type,
        int security ) 
{
    int narg;
    int nrtn;
    int orig_top = lua_gettop( LS );
    bool result=FALSE;

    lua_getglobal( LS, "area_program_setup");

    if (!push_AREA( LS, area))
    {
        bugf("Make_ud_table failed in lua_area_program. %s : %d",
                area->name,
                pvnum);
        return FALSE;
    }

    if (lua_isnil(LS, -1) )
    {
        bugf("push_ud_table pushed nil to lua_area_program");
        return FALSE;
    }

    if ( pvnum == RUNDELAY_VNUM )
    {
        /* args should have been at top of stack, func at -1 */
        lua_pushvalue( LS, orig_top - 1);
    }
    else if ( !lua_load_aprog( LS, pvnum, source) )
    {
        return FALSE;
    }

    int error=CallLuaWithTraceBack (LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running area_program_setup: %s",
                lua_tostring(LS, -1));
    }

    if ( pvnum == RUNDELAY_VNUM )
    {
        /* args should be at orig_top index */
        if ( lua_isnil( LS, orig_top ) )
        {
            narg = 0;
        }
        else
        {
            int ref_top = lua_gettop( LS );
            push_ref( LS, REF_UNPACK);
            lua_pushvalue( LS, orig_top );
            lua_pushinteger( LS, 1 );
            push_ref( LS, REF_TABLE_MAXN );
            lua_pushvalue( LS, orig_top );
            lua_call( LS, 1, 1); // call table.maxn
            lua_call( LS, 3, LUA_MULTRET ); // call unpack

            narg = lua_gettop( LS ) - ref_top;
        }
        nrtn = 0;
    }
    else
        {
        /* CH1_ARG */
        if ( !(ch1 && push_CH(LS,(void *) ch1)))
            lua_pushnil(LS);

        /* TRIG_ARG */
        if (trigger)
            lua_pushstring(LS,trigger);
        else lua_pushnil(LS);

        /* TRIGTYPE_ARG */
        lua_pushstring ( LS, flag_bit_name(aprog_flags, trig_type) );

        narg = aprog_type.narg;
        nrtn = aprog_type.nrtn;
    }

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (LS, narg, nrtn) ;
    if (error > 0 )
    {
        bugf ( "LUA aprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(LS, -1));
    }
    else
    {
        result=lua_toboolean (LS, -1);
    }
    if (!nest)
    {
        g_LuaScriptInProgress=FALSE;
        g_ScriptSecurity=SEC_NOSCRIPT; /* just in case */
        lua_settop (LS, 0);    /* get rid of stuff lying around */
    }
    return result;
}

bool lua_room_program( lua_State *LS, const char *trigger, int pvnum, const char *source, 
        ROOM_INDEX_DATA *room, 
        CHAR_DATA *ch1, CHAR_DATA *ch2, OBJ_DATA *obj1, OBJ_DATA *obj2,
        const char *text1,
        int trig_type,
        int security ) 
{
    int narg;
    int nrtn;
    int orig_top = lua_gettop( LS );
    bool result=FALSE;

    lua_getglobal( LS, "room_program_setup");

    if (!push_ROOM( LS, room))
    {
        bugf("Make_ud_table failed in lua_room_program. %d : %d",
                room->vnum,
                pvnum);
        return FALSE;
    }

    if (lua_isnil(LS, -1) )
    {
        bugf("push_ud_table pushed nil to lua_room_program");
        return FALSE;
    }
    
    if ( pvnum == RUNDELAY_VNUM )
    {
        /* args should have been at top of stack, func at -1 */
        lua_pushvalue( LS, orig_top - 1);
    }
    else if ( !lua_load_rprog( LS, pvnum, source) )
    {
        return FALSE;
    }

    int error=CallLuaWithTraceBack (LS, 2, 1) ;
    if (error > 0 )
    {
        bugf ( "LUA error running room_program_setup: %s",
                lua_tostring(LS, -1));
    }

    if ( pvnum == RUNDELAY_VNUM )
    {
        /* args should be at orig_top index */
        if ( lua_isnil( LS, orig_top ) )
        {
            narg = 0;
        }
        else
        {
            int ref_top = lua_gettop( LS );
            push_ref( LS, REF_UNPACK);
            lua_pushvalue( LS, orig_top );
            lua_pushinteger( LS, 1 );
            push_ref( LS, REF_TABLE_MAXN );
            lua_pushvalue( LS, orig_top );
            lua_call( LS, 1, 1); // call table.maxn
            lua_call( LS, 3, LUA_MULTRET ); // call unpack

            narg = lua_gettop( LS ) - ref_top;
        }
        nrtn = 0;
    }
    else
    {
        /* CH1_ARG */
        if ( !(ch1 && push_CH(LS,(void *) ch1)))
            lua_pushnil(LS);
            
        /* CH2_ARG */
        if ( !(ch2 && push_CH(LS,(void *) ch2)))
            lua_pushnil(LS);

        /* OBJ1_ARG */
        if ( !(obj1 && push_OBJ(LS,(void *) obj1)))
            lua_pushnil(LS);

        /* OBJ2_ARG */
        if ( !(obj2 && push_OBJ(LS,(void *) obj2)))
            lua_pushnil(LS);

        /* TEXT1_ARG */
        if ( text1 )
            lua_pushstring(LS,text1);
        else
            lua_pushnil(LS);

        /* TRIG_ARG */
        if (trigger)
            lua_pushstring(LS,trigger);
        else lua_pushnil(LS);

        /* TRIGTYPE_ARG */
        lua_pushstring ( LS, flag_bit_name(rprog_flags, trig_type) );

        narg = rprog_type.narg;
        nrtn = rprog_type.nrtn;
    }

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (LS, narg, nrtn) ;
    if (error > 0 )
    {
        bugf ( "LUA rprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(LS, -1));
    }
    else
    {
        result=lua_toboolean (LS, -1);
    }
    if (!nest)
    {
        g_LuaScriptInProgress=FALSE;
        g_ScriptSecurity=SEC_NOSCRIPT; /* just in case */
        lua_settop (LS, 0);    /* get rid of stuff lying around */
    }
    return result;
}
