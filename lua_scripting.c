/****************************************************************************
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

 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <lualib.h>
#include <lauxlib.h>

#include "merc.h"
#include "mob_cmds.h"
#include "tables.h"

lua_State *LS_mud = NULL;  /* Lua state for entire MUD */
/* Mersenne Twister stuff - see mt19937ar.c */

void init_genrand(unsigned long s);
void init_by_array(unsigned long init_key[], int key_length);
double genrand(void);

/* in lua_tables.c */

void add_lua_tables (lua_State *LS);

#define CHARACTER_STATE "character.state"
#define CHARACTER_META "character.metadata"
#define UD_META "ud.meta"
#define OBJECT_META "object.meta"
#define ROOM_META "room.meta"
#define MUD_LIBRARY "mud"
#define MT_LIBRARY "mt"


#define UDTYPE_UNDEFINED 0
#define UDTYPE_CHARACTER 1
#define UDTYPE_OBJECT    2
#define UDTYPE_ROOM      3

// number of items in an array
#define NUMITEMS(arg) (sizeof (arg) / sizeof (arg [0]))

/* void RegisterLuaCommands (lua_State *LS); */ /* Implemented in lua_commands.c */
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


OBJ_DATA *check_object( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    if ( type != UDTYPE_OBJECT )
    {
        luaL_error(LS,"Bad parameter %d. Expected OBJECT.", arg );
        return NULL;
    }
    
        lua_getfield(LS, arg, "tableid");
        return(OBJ_DATA *)luaL_checkudata(LS, -1, UD_META);
}

CHAR_DATA *check_character( lua_State *LS, int arg)
{
      lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    if ( type != UDTYPE_CHARACTER )
    {
        luaL_error(LS, "Bad parameter %d. Expected CHARACTER.", arg );
        return NULL;
    }

        lua_getfield(LS, arg, "tableid");
        return(OBJ_DATA *)luaL_checkudata(LS, -1, UD_META); 
}

CHAR_DATA *check_room( lua_State *LS, int arg)
{
      lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    if ( type != UDTYPE_ROOM )
    {
        luaL_error(LS, "Bad parameter %d. Expected ROOM.", arg );
        return NULL;
    }

        lua_getfield(LS, arg, "tableid");
        return(OBJ_DATA *)luaL_checkudata(LS, -1, UD_META);
}


static void make_ud_table ( lua_State *LS, void *ptr, char *meta)
{
    if ( !ptr )
        luaL_error (LS, "make_ud_table called with NULL object");

    lua_newtable( LS);
    luaL_getmetatable (LS, meta);
    lua_setmetatable (LS, -2);  /* set metatable for object data */
    lua_pushlightuserdata( LS, ptr);
    luaL_getmetatable(LS, UD_META);
    lua_setmetatable(LS, -2);
    lua_setfield( LS, -2, "tableid" );

    lua_getfield( LS, LUA_GLOBALSINDEX, "RegisterUd");
    lua_pushvalue( LS, -2);
    lua_call( LS, 1, 0 );

}
/* Given a Lua state, return the character it belongs to */


CHAR_DATA * L_getchar (lua_State *LS)
{
    /* retrieve our character */

    CHAR_DATA * ch;

    /* retrieve the character */
    lua_pushstring(LS, CHARACTER_STATE);  /* push address */
    lua_gettable(LS, LUA_ENVIRONINDEX);  /* retrieve value */

    ch = (CHAR_DATA *) lua_touserdata(LS, -1);  /* convert to data */

    if (!ch)  
        luaL_error (LS, "No current character");

    lua_pop(LS, 1);  /* pop result */

    return ch;
} /* end of L_getchar */

/* For debugging, show traceback information */

static void GetTracebackFunction (lua_State *LS)
{
    lua_pushliteral (LS, LUA_DBLIBNAME);     /* "debug"   */
    lua_rawget      (LS, LUA_GLOBALSINDEX);    /* get debug library   */

    if (!lua_istable (LS, -1))
    {
        lua_pop (LS, 2);   /* pop result and debug table  */
        lua_pushnil (LS);
        return;
    }

    /* get debug.traceback  */
    lua_pushstring(LS, "traceback");  
    lua_rawget    (LS, -2);               /* get traceback function  */

    if (!lua_isfunction (LS, -1))
    {
        lua_pop (LS, 2);   /* pop result and debug table  */
        lua_pushnil (LS);
        return;
    }

    lua_remove (LS, -2);   /* remove debug table, leave traceback function  */
}  /* end of GetTracebackFunction */

static int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn)
{

    int error;
    int base = lua_gettop (LS) - iArguments;  /* function index */
    GetTracebackFunction (LS);
    if (lua_isnil (LS, -1))
    {
        lua_pop (LS, 1);   /* pop non-existent function  */
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


/* let scripters find our directories and file names */

#define INFO_STR_ITEM(arg) \
    lua_pushstring (LS, arg);  \
lua_setfield (LS, -2, #arg)

#define INFO_NUM_ITEM(arg) \
    lua_pushnumber (LS, arg);  \
lua_setfield (LS, -2, #arg)

static int L_system_info (lua_State *LS)
{
    lua_newtable(LS);  /* table for the info */

    /* directories */

    INFO_STR_ITEM (LUA_DIR);
    INFO_STR_ITEM (LUA_STARTUP);
    INFO_STR_ITEM (LUA_MUD_STARTUP);
    return 1;  /* the table itself */
}  /* end of L_system_info */

static int L_say (lua_State *LS)
{
    do_say( L_getchar(LS), luaL_checkstring (LS, 1) );
    return 0;
}

static int L_emote (lua_State *LS)
{
    do_emote( L_getchar(LS), luaL_checkstring (LS, 1) );
    return 0;
}

static int L_cmd_asound (lua_State *LS)
{
    do_mpasound( L_getchar(LS), luaL_checkstring (LS, 1));
    return 1; 
}

static int L_cmd_gecho (lua_State *LS)
{
    do_mpgecho( L_getchar(LS), luaL_checkstring (LS, 1));
    return 0;
}

static int L_cmd_zecho (lua_State *LS)
{
   
    do_mpzecho( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_kill (lua_State *LS)
{
   
    do_mpkill( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_assist (lua_State *LS)
{
    do_mpkill( L_getchar(LS) , luaL_checkstring (LS, 1));
    return 0;
}

static int L_cmd_junk (lua_State *LS)
{
  
    do_mpjunk( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_echo (lua_State *LS)
{
   
    do_mpecho( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_echoaround (lua_State *LS)
{
  
    do_mpechoaround( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_echoat (lua_State *LS)
{
   
    do_mpechoat( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_mload (lua_State *LS)
{
  
    do_mpmload( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_oload (lua_State *LS)
{
  
    do_mpoload( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_purge (lua_State *LS)
{
   
    do_mppurge( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_goto (lua_State *LS)
{
  
    do_mpgoto( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_at (lua_State *LS)
{
   
    do_mpat( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_transfer (lua_State *LS)
{
   
    do_mptransfer( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_gtransfer (lua_State *LS)
{
  
    do_mpgtransfer( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_otransfer (lua_State *LS)
{
  
    do_mpotransfer( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_force (lua_State *LS)
{
  
    do_mpotransfer( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}


static int L_cmd_gforce (lua_State *LS)
{
   
    do_mpgforce( L_getchar(LS), luaL_checkstring (LS, 1));

   return 0;
}

static int L_cmd_vforce (lua_State *LS)
{
  
    do_mpvforce( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_cast (lua_State *LS)
{
  
    do_mpcast( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_damage (lua_State *LS)
{
   
    do_mpdamage( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_remember (lua_State *LS)
{
   
    do_mpremember( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_forget (lua_State *LS)
{
   
    do_mpforget( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_delay (lua_State *LS)
{
   
    do_mpdelay( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_cancel (lua_State *LS)
{
  
    do_mpcancel( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_call (lua_State *LS)
{
   
    do_mpcall( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_flee (lua_State *LS)
{
   
    do_mpflee( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_remove (lua_State *LS)
{
   
    do_mpremove( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_remort (lua_State *LS)
{
   
    do_mpremort( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_qset (lua_State *LS)
{
   
    do_mpqset( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_qadvance (lua_State *LS)
{
   
    do_mpqadvance( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_reward (lua_State *LS)
{
   
    do_mpreward( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_peace (lua_State *LS)
{
    
    do_mppeace( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_restore (lua_State *LS)
{
    do_mprestore( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_act (lua_State *LS)
{
    do_mpact( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_cmd_hit (lua_State *LS)
{
    do_mphit( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;

}

static int L_mdo (lua_State *LS)
{
    interpret( L_getchar(LS), luaL_checkstring (LS, 1));

    return 0;
}

static int L_mobhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, (bool) get_mob_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) ); 
    else
        lua_pushboolean( LS,  (bool) (get_char_room( ud_ch, argument) != NULL) );

    return 1;
}

static int L_objhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        return( get_obj_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) );
    else
        lua_pushboolean( LS,(bool) (get_obj_here( ud_ch, argument) != NULL) );

    return 1;
}

static int L_mobexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,(bool) (get_mp_char( ud_ch, argument) != NULL) );

    return 1;
}

static int L_objexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, (bool) (get_mp_obj( ud_ch, argument) != NULL) );

    return 1;
}

static int L_people (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushnumber( LS, count_people_room( ud_ch, 0 ) );

    return 1;
}

static int L_hour (lua_State *LS)
{
    lua_pushnumber( LS, time_info.hour );
    return 1;
}

static int L_ispc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && !IS_NPC( ud_ch ) );
    return 1;
}

static int L_isnpc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_NPC( ud_ch ) );
    return 1;
}

static int L_isgood (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_GOOD( ud_ch ) ) ;
    return 1;
}

static int L_isevil (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_EVIL( ud_ch ) ) ;
    return 1;
}

static int L_isneutral (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_NEUTRAL( ud_ch ) ) ;
    return 1;
}

static int L_isimmort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_IMMORTAL( ud_ch ) ) ;
    return 1;
}

static int L_ischarm (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_AFFECTED( ud_ch, AFF_CHARM ) ) ;
    return 1;
}

static int L_isfollow (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->master != NULL ) ;
    return 1;
}

static int L_isactive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->position > POS_SLEEPING ) ;
    return 1;
}

static int L_isdelay (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->mprog_delay > 0 ) ;
    return 1;
}

static int L_isvisible (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    CHAR_DATA * ud_vic = check_character (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && ud_vic != NULL && can_see( ud_ch, ud_vic ) ) ;

    return 1;
}

static int L_hastarget (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS,  ud_ch != NULL && ud_ch->mprog_target != NULL
            &&  ud_ch->in_room == ud_ch->mprog_target->in_room );
    return 1;
}

static int L_istarget (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->mprog_target == ud_ch );

    return 1;
}

static int L_affected (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL
            &&  is_affected_parse(ud_ch, argument) );

    return 1;
}

static int L_act (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL
            &&  IS_SET(ud_ch->act, flag_lookup(argument, act_flags)) );

    return 1;
}

static int L_off (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,
            IS_SET(ud_ch->off_flags, flag_lookup(argument, off_flags)) );

    return 1;
}

static int L_imm (lua_State *LS)
{ 
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL
            &&  IS_SET(ud_ch->imm_flags, flag_lookup(argument, imm_flags)) );

    return 1;
}

static int L_carries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, FALSE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_carry( ud_ch, argument, ud_ch ) != NULL) );

    return 1;
}

static int L_wears (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, TRUE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_wear( ud_ch, argument ) != NULL) );

    return 1;
}

static int L_has (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), FALSE ) );

    return 1;
}

static int L_uses (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), TRUE ) );

    return 1;
}

static int L_name (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && is_name( argument, ud_ch->name ) ); 

    return 1;
}

static int L_pos (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && ud_ch->position == position_lookup( argument ) ); 

    return 1;
}

static int L_clan (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && ud_ch->clan == clan_lookup( argument ) );

    return 1;
}


static int L_vnum (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushnumber( LS,  ( ud_ch != NULL && IS_NPC(ud_ch) ) ? ud_ch->pIndexData->vnum : 0 );

    return 1;
}

static int L_grpsize (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, count_people_room( ud_ch, 4 ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}


static int L_qstatus (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    int num = luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, quest_status( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_vuln (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL
            && IS_SET(ud_ch->vuln_flags, flag_lookup(argument, vuln_flags)) );

    return 1;
}

static int L_res (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL
            && IS_SET(ud_ch->res_flags, flag_lookup(argument, res_flags)) );

    return 1;
}

static int L_religion (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && get_religion(ud_ch) != NULL
            && get_religion(ud_ch) == get_religion_by_name(argument) );

    return 1;
}

static int L_skilled (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && skill_lookup(argument) != -1
            && get_skill(ud_ch, skill_lookup(argument)) > 0 );

    return 1;
}

static int L_ccarries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && has_item_in_container( ud_ch, r_atoi(ud_ch, argument), "zzyzzxzzyxyx" ) );

    return 1;
}

static int L_qtimer (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    int num = luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, qset_timer( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_mpcnt (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, (ud_ch->mana * 100)/(UMAX(1,ud_ch->max_mana)));
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_remort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL && !IS_NPC(ud_ch) )
        lua_pushnumber( LS, ud_ch->pcdata->remorts );
    else
        lua_pushnumber( LS, -1);

    return 1;
}

static const struct luaL_reg mudlib [] = 
{
    {"system_info", L_system_info}, 
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

static int char2string (lua_State *LS) 
{
    lua_pushliteral (LS, "mud_character");
    return 1;
}

static int obj2string (lua_State *LS)
{
    lua_pushliteral (LS, "mud_object");
    return 1;
}

static int room2string (lua_State *LS)
{
    lua_pushliteral (LS, "mud_room");
    return 1;
}




#define FLDSTR(key,value) \
    if ( !strcmp( argument, key ) ) \
{lua_pushstring( LS, value ); return 1;}
#define FLDNUM(key,value) \
    if ( !strcmp( argument, key ) ) \
{lua_pushnumber( LS, value ); return 1;}



static int get_object_field ( lua_State *LS )
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_OBJECT); /* Need this for type checking */

    OBJ_DATA *ud_obj = check_object(LS, 1);

    if ( !ud_obj )
        return 0;


    FLDSTR("name", ud_obj->name);
    FLDSTR("shortdescr", ud_obj->short_descr);
    FLDSTR("clan", clan_table[ud_obj->clan].name);
    FLDNUM("clanrank", ud_obj->rank);
    FLDNUM("level", ud_obj->level);
    FLDSTR("owner", ud_obj->owner);
    FLDNUM("cost", ud_obj->cost);
    FLDSTR("material", ud_obj->material);
    FLDNUM("vnum", ud_obj->pIndexData->vnum);
    FLDSTR("type", type_flags[ud_obj->item_type].name);
    FLDNUM("inroom", ud_obj->in_room ?
                     ud_obj->in_room->vnum:
                     0);
    if (!strcmp(argument, "carriedby") )
    {
        if (!ud_obj->carried_by )
            return 0;

        make_ud_table(LS, ud_obj->carried_by, CHARACTER_META);
        return 1;
    }
       
    FLDNUM("v0", ud_obj->value[0]);
    FLDNUM("v1", ud_obj->value[1]);
    FLDNUM("v2", ud_obj->value[2]);
    FLDNUM("v3", ud_obj->value[3]);
    FLDNUM("v4", ud_obj->value[4]);
    FLDNUM("v5", ud_obj->value[5]);
    return 0;
}

static int get_room_field ( lua_State *LS )
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_ROOM); /* Need this for type checking */

    ROOM_INDEX_DATA *ud_room = check_room(LS, 1);

    if ( !ud_room )
        return 0;


    FLDSTR("name", ud_room->name);
    FLDNUM("vnum", ud_room->vnum);
    FLDSTR("clan", clan_table[ud_room->clan].name);
    FLDNUM("clanrank", ud_room->clan_rank);
    FLDNUM("healrate", ud_room->heal_rate);
    FLDNUM("manarate", ud_room->mana_rate);
    FLDSTR("owner", ud_room->owner ? ud_room->owner : "");
    FLDSTR("description", ud_room->description);
    FLDSTR("area", ud_room->area->name );

    return 0;
}

static int get_character_field ( lua_State *LS)
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_CHARACTER); /* Need this for type checking */

    CHAR_DATA *ud_ch = check_character(LS, 1);

    if ( !ud_ch)
        return 0;

    FLDSTR("name", ud_ch->name);
    FLDNUM("level", ud_ch->level);
    FLDNUM("hp", ud_ch->hit);
    FLDNUM("maxhp", ud_ch->max_hit);
    FLDNUM("mana", ud_ch->mana);
    FLDNUM("maxmana", ud_ch->max_mana);
    FLDNUM("move", ud_ch->move);
    FLDNUM("maxmove", ud_ch->max_move);
    FLDNUM("gold", ud_ch->gold);
    FLDNUM("silver", ud_ch->silver);
    FLDNUM("money", (ud_ch->silver + ud_ch->gold * 100 ) );
    FLDSTR("sex", ( sex_table[ud_ch->sex].name ) );
    FLDSTR("size", ( size_table[ud_ch->size].name ) );
    FLDSTR("position", ( position_table[ud_ch->position].short_name ) );
    FLDNUM("align", ud_ch->alignment);
    FLDNUM("str", get_curr_stat( ud_ch, STAT_STR ) );
    FLDNUM("con", get_curr_stat( ud_ch, STAT_CON ) );
    FLDNUM("vit", get_curr_stat( ud_ch, STAT_VIT ) );
    FLDNUM("agi", get_curr_stat( ud_ch, STAT_AGI ) );
    FLDNUM("dex", get_curr_stat( ud_ch, STAT_DEX ) );
    FLDNUM("int", get_curr_stat( ud_ch, STAT_INT ) );
    FLDNUM("wis", get_curr_stat( ud_ch, STAT_WIS ) );
    FLDNUM("dis", get_curr_stat( ud_ch, STAT_DIS ) );
    FLDNUM("cha", get_curr_stat( ud_ch, STAT_CHA ) );
    FLDNUM("luc", get_curr_stat( ud_ch, STAT_LUC ) );
    FLDSTR("clan", clan_table[ud_ch->clan].name );
    FLDSTR("class", IS_NPC(ud_ch) ? "mobile" : class_table[ud_ch->class].name );
    FLDSTR("race", race_table[ud_ch->race].name );
    if ( !strcmp(argument, "room" ) )
    {
        make_ud_table(LS, ud_ch->in_room, ROOM_META);
        return 1;
    }
    FLDNUM("groupsize", count_people_room( ud_ch, 4 ) );
    if ( !IS_NPC(ud_ch) )
    {
        FLDNUM("clanrank", ud_ch->pcdata->clan_rank );
        FLDNUM("remorts", ud_ch->pcdata->remorts);
        FLDNUM("explored", ud_ch->pcdata->explored->set);
        FLDNUM("beheads", ud_ch->pcdata->behead_cnt);
        FLDNUM("pkills", ud_ch->pcdata->pkill_count);
        FLDNUM("pkdeaths", ud_ch->pcdata->pkill_deaths);
        FLDNUM("questpoints", ud_ch->pcdata->questpoints );
        FLDNUM("bank", ud_ch->pcdata->bank );
        FLDNUM("mobkills", ud_ch->pcdata->mob_kills);
        FLDNUM("mobdeaths", ud_ch->pcdata->mob_deaths);
    }


    return 0;

}

static const struct luaL_reg object_meta [] =
{
    {"__tostring", obj2string},
    {"__index", get_object_field},
    {NULL, NULL}
};

static const struct luaL_reg room_meta [] =
{
    {"__tostring", room2string},
    {"__index", get_room_field},
    {NULL, NULL}
};

static const struct luaL_reg character_meta [] = 
{
    {"__tostring", char2string},
    {"__index", get_character_field},
    {NULL, NULL}
};

static const struct luaL_reg cmdlib_m [] =
{
    /* the originals */
    {"asound",      L_cmd_asound},
    {"gecho",       L_cmd_gecho},
    {"zecho",       L_cmd_zecho},
    {"kill",        L_cmd_kill},
    {"assist",      L_cmd_assist},
    {"junk",        L_cmd_junk},
    {"echo",        L_cmd_echo},
    {"echoaround",  L_cmd_echoaround},
    {"echoat",      L_cmd_echoat},
    {"mload",       L_cmd_mload},
    {"oload",       L_cmd_oload},
    {"purge",       L_cmd_purge},
    {"goto",        L_cmd_goto},
    {"at",          L_cmd_at},
    {"transfer",    L_cmd_transfer},
    {"gtransfer",   L_cmd_gtransfer},
    {"otransfer",   L_cmd_otransfer},
    {"force",       L_cmd_force},
    {"gforce",      L_cmd_gforce},
    {"vforce",      L_cmd_vforce},
    {"cast",        L_cmd_cast},
    {"damage",      L_cmd_damage},
    {"remember",    L_cmd_remember},
    {"forget",      L_cmd_forget},
    {"delay",       L_cmd_delay},
    {"cancel",      L_cmd_cancel},
    {"call",        L_cmd_call},
    {"flee",        L_cmd_flee},
    {"remove",      L_cmd_remove},
    {"remort",      L_cmd_remort},
    {"qset",        L_cmd_qset},
    {"qadvance",    L_cmd_qadvance},
    {"reward",      L_cmd_reward},
    {"peace",       L_cmd_peace},
    {"restore",     L_cmd_restore},
    {"act",         L_cmd_act},
    {"hit",         L_cmd_hit},
    {NULL, NULL}
};

static const struct luaL_reg checklib_m [] =
{
    /* the originals */
    {"mobhere",     L_mobhere},
    {"objhere",     L_objhere},
    {"mobexists",   L_mobexists},
    {"objexists",   L_objexists },
    {"people",      L_people },
    {"hour",        L_hour },
    {"ispc",        L_ispc },
    {"isnpc",       L_isnpc },
    {"isgood",      L_isgood },
    {"isevil",      L_isevil },
    {"isneutral",   L_isneutral },
    {"isimmort",    L_isimmort },
    {"ischarm",     L_ischarm },
    {"isfollow",     L_isfollow },
    {"isactive",       L_isactive },
    {"isdelay",       L_isdelay },
    {"isvisible",       L_isvisible },
    {"hastarget",       L_hastarget },
    {"istarget",          L_istarget },
    {"affected",          L_affected },
    {"act",          L_act },
    {"off",          L_off },
    {"imm",          L_imm },
    {"carries",          L_carries },
    {"wears",          L_wears },
    {"has",          L_has },
    {"uses",          L_uses },
    {"vnum",          L_vnum },
    {"grpsize",          L_grpsize },
    {"qstatus",          L_qstatus },
    {"vuln",          L_vuln },
    {"res",          L_res },
    {"religion",          L_religion },
    {"skilled",          L_skilled },
    {"ccarries",          L_ccarries },
    {"qtimer",          L_qtimer },
    {NULL, NULL}
};

static int RegisterLuaRoutines (lua_State *LS)
{
    time_t timer;
    time (&timer);

    lua_newtable (LS);  /* environment */
    lua_replace (LS, LUA_ENVIRONINDEX);

    /* this makes environment variable "character.state" by the pointer to our character */
    lua_settable(LS, LUA_ENVIRONINDEX);

    /* register all mud.xxx routines */
    luaL_register (LS, MUD_LIBRARY, mudlib);

    /* using interpret now
       RegisterLuaCommands (LS);
     */

    luaopen_bits (LS);     /* bit manipulation */
    luaL_register (LS, MT_LIBRARY, mtlib);  /* Mersenne Twister */

    /* Marsenne Twister generator  */
    init_genrand (timer);

    /* colours, and other constants */
    add_lua_tables (LS);

    luaL_register (LS, "cmd", cmdlib_m);
    luaL_register (LS, "check", checklib_m);

    lua_pushcfunction ( LS, L_say );
    lua_setglobal( LS, "say" );

    lua_pushcfunction ( LS, L_emote );
    lua_setglobal( LS, "emote");

    lua_pushcfunction ( LS, L_mdo );
    lua_setglobal( LS, "mdo" );

    /* meta table to identify character types */
    luaL_newmetatable(LS, CHARACTER_META);
    luaL_register (LS, NULL, character_meta);  /* give us a __tostring function */
    luaL_newmetatable(LS, OBJECT_META);
    luaL_register (LS, NULL, object_meta);

    luaL_newmetatable(LS, ROOM_META);
    luaL_register (LS, NULL, room_meta);

    luaL_newmetatable(LS, UD_META);

    return 0;

}  /* end of RegisterLuaRoutines */

void open_lua  ( CHAR_DATA * ch)
{
    lua_State *LS = luaL_newstate ();   /* opens Lua */
    ch->LS = LS;

    if (ch->LS == NULL)
    {
        bugf("Cannot open Lua state");
        return;  /* catastrophic failure */
    }

    luaL_openlibs (LS);    /* open all standard libraries */

    /* call as Lua function because we need the environment  */
    lua_pushcfunction(LS, RegisterLuaRoutines);
    lua_pushstring(LS, CHARACTER_STATE);  /* push address */
    lua_pushlightuserdata(LS, (void *)ch);    /* push value */
    lua_call(LS, 2, 0);

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

void close_lua ( CHAR_DATA * ch)
{

    if (ch->LS == NULL)
        return;  /* never opened */

    lua_close (ch->LS);
    ch->LS = NULL;

}/* end of close_lua */

static lua_State * find_lua_function (CHAR_DATA * ch, const char * fname)
{
    lua_State *LS = ch->LS;

    if (!L || !fname)
        return NULL;  /* can't do it */

    /* find requested function */

    lua_getglobal (LS, fname);  
    if (!lua_isfunction (LS, -1))
    {
        lua_pop (LS, 1);
        bug ("Warning: Lua script function '%s' does not exist", fname);
        return NULL;  /* not there */
    }

    return L;  
}

extern char lastplayercmd[MAX_INPUT_LENGTH * 2];

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
                "Unknown (TBC)");

        if (ch && !IS_IMMORTAL(ch))
        {
            //set_char_color( AT_YELLOW, ch );
            ptc (ch, "** A server scripting error occurred, please notify an administrator.\n"); 
        }  /* end of not immortal */


        lua_pop(LS, 1);  /* pop error */

    }  /* end of error */

}  /* end of call_lua_function */


void call_lua (CHAR_DATA * ch, const char * fname, const char * argument)
{

    int nArgs = 0;

    if (!ch || !fname)  /* note, argument is OPTIONAL */
        return;

    lua_State *LS = find_lua_function (ch, fname);

    if (!L)
        return;

    /* if they want to send an argument, push it now */  
    if (argument)
    {
        lua_pushstring(LS, argument);  /* push argument, if any */
        nArgs++;
    }

    call_lua_function (ch, L, fname, nArgs);

}  /* end of call_lua */

void call_lua_num (CHAR_DATA * ch, const char * fname, const int argument)
{

    if (!ch || !fname)
        return;

    lua_State *LS = find_lua_function (ch, fname);

    if (!L)
        return;

    lua_pushnumber (LS, argument);  

    call_lua_function (ch, L, fname, 1);

}  /* end of call_lua_num */

/* call lua function with a victim name and numeric argument */
void call_lua_char_num (CHAR_DATA * ch, 
        const char * fname, 
        const char * victim, 
        const int argument)
{
    lua_State *LS = find_lua_function (ch, fname);

    if (!L || !victim)
        return;

    lua_pushstring (LS, victim);  /* push victim */
    lua_pushnumber (LS, argument);  

    call_lua_function (ch, L, fname, 2);

}  /* end of call_lua_char_num */

/* call lua function with a mob vnum and numeric argument */
void call_lua_mob_num (CHAR_DATA * ch, 
        const char * fname, 
        const int victim, 
        const int argument)
{
    lua_State *LS = find_lua_function (ch, fname);

    if (!L)
        return;

    lua_pushnumber (LS, victim);  /* push victim */
    lua_pushnumber (LS, argument);  

    call_lua_function (ch, L, fname, 2);

}  /* end of call_lua_mob_num */


static int RegisterGlobalLuaRoutines (lua_State *LS)
{
    time_t timer;
    time (&timer);

    /* register all mud.xxx routines */
    luaL_register (LS, MUD_LIBRARY, mudlib);

    luaopen_bits (LS);     /* bit manipulation */
    luaL_register (LS, MT_LIBRARY, mtlib);  /* Mersenne Twister */

    /* Marsenne Twister generator  */
    init_genrand (timer);

    /* colours, and other constants */
    add_lua_tables (LS);

    /* meta table to identify character types */
    luaL_newmetatable(LS, CHARACTER_META);
    luaL_register (LS, NULL, character_meta);  /* give us a __tostring function */

    return 0;

}  /* end of RegisterLuaRoutines */

void open_mud_lua (void)    /* set up Lua state - entire MUD */
{

    LS_mud = luaL_newstate ();   /* opens Lua */

    if (LS_mud == NULL)
    {
        bugf ("Cannot open Lua state");
        exit (1);  /* catastrophic failure */
    }

    luaL_openlibs (LS_mud);    /* open all standard libraries */

    /* call as Lua function because we need the environment  */
    lua_pushcfunction(LS_mud, RegisterGlobalLuaRoutines);
    lua_call(LS_mud, 0, 0);


    /* run initialiation script */
    if (luaL_loadfile (LS_mud, LUA_MUD_STARTUP) ||
            CallLuaWithTraceBack (LS_mud, 0, 0))
    {
        bugf ( "Error loading Lua startup file %s:\n %s", 
                LUA_MUD_STARTUP,
                lua_tostring(LS_mud, -1));
        exit (1);  /* catastrophic failure */
    }

    lua_settop (LS_mud, 0);    /* get rid of stuff lying around */    

}  /* end of open_mud_lua */

void close_mud_lua (void)   /* close Lua state - entire MUD */  
{
    if (LS_mud == NULL)
        return;  /* never opened */

    lua_close (LS_mud);
    LS_mud = NULL;   
}  /* end of close_mud_lua */

static int find_mud_lua_function (const char * fname)
{

    if (!fname)
        return FALSE;

    /* find requested function */

    lua_getglobal (LS_mud, fname);  
    if (!lua_isfunction (LS_mud, -1))
    {
        lua_pop (LS_mud, 1);
        bug ("Warning: Lua mud-wide script function '%s' does not exist", fname);
        return FALSE;  /* not there */
    }

    return TRUE;  
}  /* end of find_mud_lua_function */

static int call_mud_lua_function (const char * fname, 
        const int nArgs)

{
    int iResult = 0;

    if (CallLuaWithTraceBack (LS_mud, nArgs, 1))
    {
        bugf ("Error executing mud-wide Lua function %s:\n %s\n** Last player command: %s", 
                fname, 
                lua_tostring(LS_mud, -1),
                "Unknown, TBC.");

        lua_pop(LS_mud, 1);  /* pop error */

    }  /* end of error */

    if (lua_isnumber (LS_mud, -1))
        iResult = luaL_checknumber (LS_mud, -1);
    else if (lua_isboolean (LS_mud, -1))
        iResult = lua_toboolean (LS_mud, -1);

    return iResult;
}  /* end of call_lua_function */

int call_mud_lua (const char * fname, const char * argument)
{

    int nArgs = 0;

    if (!argument || !fname)
        return FALSE;

    lua_settop (LS_mud, 0);    /* get rid of stuff lying around */

    if (!find_mud_lua_function (fname))
        return 0;

    /* if they want to send an argument, push it now */  
    if (argument)
    {
        lua_pushstring(LS_mud, argument);  /* push argument, if any */
        nArgs++;
    }

    return call_mud_lua_function (fname, nArgs);

}  /* end of call_mud_lua */

/* call lua function with a string and numeric argument */
void call_mud_lua_char_num (const char * fname, 
        const char * str, 
        const int num)
{
    if (!str || !fname)
        return;

    if (!find_mud_lua_function (fname))
        return;

    lua_pushstring (LS_mud, str);  /* push string */
    lua_pushnumber (LS_mud, num);  /* and number */

    call_mud_lua_function (fname, 2);

}  /* end of call_mud_lua_char_num */


/* lua_program
   lua equivalent of program_flow
 */
void lua_program( char *text, int pvnum, char *source, 
        CHAR_DATA *mob, CHAR_DATA *ch, 
        const void *arg1, sh_int arg1type, 
        const void *arg2, sh_int arg2type ) 
{
    /* Open lua if it isn't */
    if ( mob->LS == NULL )
        open_lua(mob);

    /* some init stuff before we run the prog */
    make_ud_table (mob->LS, (void *)mob, CHARACTER_META);
    lua_setglobal(mob->LS, "mob");
    
    if (ch)
    {
        make_ud_table (mob->LS,(void *) ch, CHARACTER_META);
        lua_setglobal(mob->LS, "ch");
    }
    if (text)
    {
        lua_pushstring ( mob->LS, text );
        lua_setglobal(mob->LS, "text");
    }
    if (arg1type== ACT_ARG_OBJ)
    {
        make_ud_table( mob->LS, arg1, OBJECT_META);
        lua_setglobal( mob->LS, "obj");
    }
    if (arg2type== ACT_ARG_OBJ)
    {
        make_ud_table( mob->LS, arg2, OBJECT_META);
        lua_setglobal( mob->LS, "obj2");
    }
   



    if (luaL_loadstring (mob->LS, source) ||
            CallLuaWithTraceBack (mob->LS, 0, 0))
    {
        bugf ( "LUA mprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(mob->LS, -1));
    }

    /* cleanup routines */
    if (luaL_loadstring (mob->LS, "cleanup()") ||
                CallLuaWithTraceBack (mob->LS, 0, 0))
    {
        bugf ( "Cleanup error for vnum %d:\n %s",
                pvnum,
                lua_tostring(mob->LS, -1));
    }

    lua_settop (mob->LS, 0);    /* get rid of stuff lying around */
}
