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

lua_State *LS_mud = NULL;  /* Lua state for entire MUD */
/* Mersenne Twister stuff - see mt19937ar.c */

void init_genrand(unsigned long s);
void init_by_array(unsigned long init_key[], int key_length);
double genrand(void);

static int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn);
static const struct luaL_reg CH_lib [];
static const struct luaL_reg OBJ_lib [];
static const struct luaL_reg OBJPROTO_lib[];
static const struct luaL_reg ROOM_lib [];
static const struct luaL_reg EXIT_lib [];

/* in lua_tables.c */

void add_lua_tables (lua_State *LS);

#define CHARACTER_STATE "character.state"
#define CH_META        "CH.meta"
#define UD_META        "UD.meta"
#define OBJ_META       "OBJ.meta"
#define OBJPROTO_META  "OBJPROTO.meta"
#define ROOM_META      "ROOM.meta"
#define EXIT_META      "EXIT.meta"
#define MUD_LIBRARY "mud"
#define MT_LIBRARY "mt"

#define CLEANUP_FUNCTION "cleanup"
#define REGISTER_UD_FUNCTION "RegisterUd"

#define NUM_MPROG_ARGS 8 
#define MOB_ARG "mob"
#define CH_ARG "ch"
#define OBJ1_ARG "obj1"
#define OBJ2_ARG "obj2"
#define TRIG_ARG "trigger"
#define TEXT1_ARG "text1"
#define TEXT2_ARG "text2"
#define VICTIM_ARG "victim"

#define UDTYPE_UNDEFINED 0
#define UDTYPE_CH        1
#define UDTYPE_OBJ       2
#define UDTYPE_ROOM      3
#define UDTYPE_EXIT      4
#define UDTYPE_OBJPROTO  5


// number of items in an array
#define NUMITEMS(arg) (sizeof (arg) / sizeof (arg [0]))

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

OBJ_INDEX_DATA *check_OBJPROTO( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    if ( type != UDTYPE_OBJPROTO )
    {
        luaL_error(LS,"Bad parameter %d. Expected OBJPROTO.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    return(OBJ_INDEX_DATA *)luaL_checkudata(LS, -1, UD_META);
}

OBJ_DATA *check_OBJ( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    if ( type != UDTYPE_OBJ )
    {
        luaL_error(LS,"Bad parameter %d. Expected OBJ.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    return(OBJ_DATA *)luaL_checkudata(LS, -1, UD_META);
}

CHAR_DATA *check_CH( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    if ( type != UDTYPE_CH )
    {
        luaL_error(LS, "Bad parameter %d. Expected CH.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    return(CHAR_DATA *)luaL_checkudata(LS, -1, UD_META); 
}

ROOM_INDEX_DATA *check_ROOM( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    if ( type != UDTYPE_ROOM )
    {
        luaL_error(LS, "Bad parameter %d. Expected ROOM.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    return(ROOM_INDEX_DATA *)luaL_checkudata(LS, -1, UD_META);
}

EXIT_DATA *check_exit( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    if ( type != UDTYPE_EXIT )
    {
        luaL_error(LS, "Bad parameter %d. Expected EXIT.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    return(EXIT_DATA *)luaL_checkudata(LS, -1, UD_META);
}


static void make_ud_table ( lua_State *LS, void *ptr, int UDTYPE )
{
    if ( !ptr )
        luaL_error (LS, "make_ud_table called with NULL object. UDTYPE: %s", UDTYPE);
    char *meta;



    lua_newtable( LS);
    switch (UDTYPE)
    {
        case UDTYPE_CH:
            meta=CH_META; luaL_register(LS,NULL,CH_lib); break;
        case UDTYPE_OBJ:
            meta=OBJ_META; luaL_register(LS,NULL,OBJ_lib); break;
        case UDTYPE_ROOM:
            meta=ROOM_META; luaL_register(LS,NULL,ROOM_lib); break;
        case UDTYPE_EXIT:
            meta=EXIT_META; luaL_register(LS,NULL,EXIT_lib); break;
        case UDTYPE_OBJPROTO:
            meta=OBJPROTO_META; luaL_register(LS,NULL, OBJPROTO_lib); break;
        default:
            luaL_error (LS, "make_ud_table called with unknown UD_TYPE: %d", UDTYPE);
            break;
    }
            
    luaL_getmetatable (LS, meta);
    lua_setmetatable (LS, -2);  /* set metatable for object data */
    lua_pushstring( LS, "tableid");
    lua_pushlightuserdata( LS, ptr);
    luaL_getmetatable(LS, UD_META);
    lua_setmetatable(LS, -2);
    lua_rawset( LS, -3 );

    lua_getfield( LS, LUA_GLOBALSINDEX, REGISTER_UD_FUNCTION);
    lua_pushvalue( LS, -2);
    if (CallLuaWithTraceBack( LS, 1, 0) )
    {
        bugf ( "Error registering UD:\n %s",
                lua_tostring(LS, -1));
    }


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

static int L_getroom (lua_State *LS)
{
    // do some if is number thing here eventually
    int num = luaL_checknumber (LS, 1);

    ROOM_INDEX_DATA *room=get_room_index(num);

    if (!room)
        return 0;

    make_ud_table( LS, room, UDTYPE_ROOM);
    return 1;

}

static int L_getobjproto (lua_State *LS)
{
    int num = luaL_checknumber (LS, 1);

    OBJ_INDEX_DATA *obj=get_object_index(num);

    if (!obj)
        return 0;

    make_ud_table( LS, obj, UDTYPE_OBJPROTO);
    return 1;
}

static int L_randchar (lua_State *LS)
{
    CHAR_DATA *ch=get_random_char(L_getchar(LS) );
    if ( ! ch )
        return 0;

    make_ud_table( LS, ch, UDTYPE_CH);
    return 1;

}


static int L_loadprog (lua_State *LS)
{
    int num = luaL_checknumber (LS, 1);
    MPROG_CODE *pMcode;

    if ( (pMcode = get_mprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: vnum %d doesn't exist", num);
        return 0;
    }

    if ( !pMcode->is_lua)
    {
        luaL_error(LS, "loadprog: vnum %d is not lua code", num);
        return 0;
    }

    if (luaL_loadstring (LS, pMcode->code) ||
            CallLuaWithTraceBack (LS, 0, 0))
    {
        bugf ( "loadprog error loading vnum %d:\n %s",
                num,
                lua_tostring(LS, -1));
    } 
    return 0;
}

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
    return 0; 
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
    CHAR_DATA * ud_ch = L_getchar(LS);
    const char *argument = luaL_checkstring (LS, 1);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, (bool) get_mob_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) ); 
    else
        lua_pushboolean( LS,  (bool) (get_char_room( ud_ch, argument) != NULL) );

    return 1;
}

static int L_objhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = L_getchar(LS);
    const char *argument = luaL_checkstring (LS, 1);

    if ( is_r_number( argument ) )
        return( get_obj_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) );
    else
        lua_pushboolean( LS,(bool) (get_obj_here( ud_ch, argument) != NULL) );

    return 1;
}

static int L_mobexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = L_getchar(LS);
    const char *argument = luaL_checkstring (LS, 1);

    lua_pushboolean( LS,(bool) (get_mp_char( ud_ch, argument) != NULL) );

    return 1;
}

static int L_objexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = L_getchar(LS);
    const char *argument = luaL_checkstring (LS, 1);

    lua_pushboolean( LS, (bool) (get_mp_obj( ud_ch, argument) != NULL) );

    return 1;
}

static int L_hour (lua_State *LS)
{
    lua_pushnumber( LS, time_info.hour );
    return 1;
}

static int L_ispc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && !IS_NPC( ud_ch ) );
    return 1;
}

static int L_isnpc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_NPC( ud_ch ) );
    return 1;
}

static int L_isgood (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_GOOD( ud_ch ) ) ;
    return 1;
}

static int L_isevil (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_EVIL( ud_ch ) ) ;
    return 1;
}

static int L_isneutral (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_NEUTRAL( ud_ch ) ) ;
    return 1;
}

static int L_isimmort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_IMMORTAL( ud_ch ) ) ;
    return 1;
}

static int L_ischarm (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_AFFECTED( ud_ch, AFF_CHARM ) ) ;
    return 1;
}

static int L_isfollow (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->master != NULL ) ;
    return 1;
}

static int L_isactive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->position > POS_SLEEPING ) ;
    return 1;
}

static int L_isdelay (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->mprog_delay > 0 ) ;
    return 1;
}

static int L_isvisible (lua_State *LS)
{
    CHAR_DATA * ud_ch = L_getchar(LS);
    CHAR_DATA * ud_vic = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_vic != NULL && can_see( ud_ch, ud_vic ) ) ;

    return 1;
}

static int L_hastarget (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS,  ud_ch != NULL && ud_ch->mprog_target != NULL
            &&  ud_ch->in_room == ud_ch->mprog_target->in_room );
    return 1;
}

static int L_istarget (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->mprog_target == ud_ch );

    return 1;
}

static int L_affected (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL
            &&  is_affected_parse(ud_ch, argument) );

    return 1;
}

static int L_act (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL
            &&  IS_SET(ud_ch->act, flag_lookup(argument, act_flags)) );

    return 1;
}

static int L_off (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,
            IS_SET(ud_ch->off_flags, flag_lookup(argument, off_flags)) );

    return 1;
}

static int L_imm (lua_State *LS)
{ 
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL
            &&  IS_SET(ud_ch->imm_flags, flag_lookup(argument, imm_flags)) );

    return 1;
}

static int L_carries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, FALSE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_carry( ud_ch, argument, ud_ch ) != NULL) );

    return 1;
}

static int L_wears (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, TRUE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_wear( ud_ch, argument ) != NULL) );

    return 1;
}

static int L_has (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), FALSE ) );

    return 1;
}

static int L_uses (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), TRUE ) );

    return 1;
}

static int L_name (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && is_name( argument, ud_ch->name ) ); 

    return 1;
}

static int L_vnum (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushnumber( LS,  ( ud_ch != NULL && IS_NPC(ud_ch) ) ? ud_ch->pIndexData->vnum : 0 );

    return 1;
}

static int L_qstatus (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, quest_status( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_vuln (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL
            && IS_SET(ud_ch->vuln_flags, flag_lookup(argument, vuln_flags)) );

    return 1;
}

static int L_res (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL
            && IS_SET(ud_ch->res_flags, flag_lookup(argument, res_flags)) );

    return 1;
}

static int L_skilled (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && skill_lookup(argument) != -1
            && get_skill(ud_ch, skill_lookup(argument)) > 0 );

    return 1;
}

static int L_ccarries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && has_item_in_container( ud_ch, r_atoi(ud_ch, argument), "zzyzzxzzyxyx" ) );

    return 1;
}

static int L_qtimer (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, qset_timer( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_mpcnt (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, (ud_ch->mana * 100)/(UMAX(1,ud_ch->max_mana)));
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_remort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    if ( ud_ch != NULL && !IS_NPC(ud_ch) )
        lua_pushnumber( LS, ud_ch->pcdata->remorts );
    else
        lua_pushnumber( LS, -1);

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

static int L_exit_flag( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_exit(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    sh_int flag=flag_lookup( argument, exit_flags);
    if ( flag==NO_FLAG )
        return 0;

    lua_pushboolean( LS, IS_SET( ud_exit->exit_info, flag));
    return 1;
}

static int L_room_flag( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room = check_ROOM(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    sh_int flag=flag_lookup( argument, room_flags);
    if ( flag==NO_FLAG )
        return 0;

    lua_pushboolean( LS, IS_SET( ud_room->room_flags, flag));
    return 1;
}

static int L_objproto_wear( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    sh_int flag=flag_lookup( argument, wear_flags);
    if ( flag==NO_FLAG )
        return 0;

    lua_pushboolean( LS, IS_SET( ud_objp->wear_flags, flag));
    return 1;
}

static int L_objproto_extra( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    sh_int flag=flag_lookup( argument, extra_flags);
    if ( flag==NO_FLAG )
        return 0;

    lua_pushboolean( LS, IS_SET( ud_objp->extra_flags, flag));
    return 1;
}

static int L_obj_wear( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    sh_int flag=flag_lookup( argument, wear_flags);
    if ( flag==NO_FLAG )
        return 0;

    lua_pushboolean( LS, IS_SET( ud_obj->wear_flags, flag));
    return 1;
}

static int L_obj_extra( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    sh_int flag=flag_lookup( argument, extra_flags);
    if ( flag==NO_FLAG )
        return 0;

    lua_pushboolean( LS, IS_SET( ud_obj->extra_flags, flag));
    return 1;
}

static const struct luaL_reg mudlib [] = 
{
    {"luadir", L_mud_luadir}, 
    {"userdir",L_mud_userdir},
    {NULL, NULL}
};  /* end of mudlib */


static const struct luaL_reg CH_lib [] =
{
    {"affected", L_affected},
    {"offensive", L_off},
    {"immune", L_imm},
    {"resist", L_res},
    {"vuln", L_vuln},

    {NULL, NULL}
};

static const struct luaL_reg ROOM_lib [] =
{
    {"flag", L_room_flag},
    {NULL, NULL}
};

static const struct luaL_reg OBJ_lib [] =
{
    {"extra", L_obj_extra},
    {"wear", L_obj_wear},
    {NULL, NULL}
};

static const struct luaL_reg OBJPROTO_lib [] =
{
    {"extra", L_objproto_extra},
    {"wear", L_objproto_wear},
    {NULL, NULL}
};

static const struct luaL_reg EXIT_lib [] =
{
    {"flag", L_exit_flag},
    {NULL, NULL}
}; 

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

static int CH2string (lua_State *LS) 
{
    lua_pushliteral (LS, "mud_character");
    return 1;
}

static int OBJ2string (lua_State *LS)
{
    lua_pushliteral (LS, "mud_object");
    return 1;
}

static int OBJPROTO2string (lua_State *LS)
{
    lua_pushliteral (LS, "mud_object_prototype");
    return 1;
}

static int ROOM2string (lua_State *LS)
{
    lua_pushliteral (LS, "mud_room");
    return 1;
}

static int EXIT2string (lua_State *LS)
{
    lua_pushliteral (LS, "mud_exit");
    return 1;
}




#define FLDSTR(key,value) \
    if ( !strcmp( argument, key ) ) \
        {lua_pushstring( LS, value ); return 1;}
#define FLDNUM(key,value) \
    if ( !strcmp( argument, key ) ) \
        {lua_pushnumber( LS, value ); return 1;}
#define FLDBOOL(key,value) \
    if ( !strcmp( argument, key ) ) \
        {lua_pushboolean( LS, value ); return 1;}
    


static int check_OBJ_equal( lua_State *LS)
{
    lua_pushboolean( LS, check_OBJ(LS, 1) == check_OBJ(LS, 2) );
    return 1;
}

static int check_OBJPROTO_equal( lua_State *LS)
{
    lua_pushboolean( LS, check_OBJPROTO(LS, 1) == check_OBJPROTO(LS, 2) );
    return 1;
}

static int get_OBJPROTO_field ( lua_State *LS )
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_OBJPROTO); /* Need this for type checking */

    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);

    if ( !ud_objp )
        return 0;

    FLDSTR("name", ud_objp->name);
    FLDSTR("shortdescr", ud_objp->short_descr);
    FLDSTR("clan", clan_table[ud_objp->clan].name);
    FLDNUM("clanrank", ud_objp->rank);
    FLDNUM("level", ud_objp->level);
    FLDNUM("cost", ud_objp->cost);
    FLDSTR("material", ud_objp->material);
    FLDNUM("vnum", ud_objp->vnum);
    FLDSTR("type", type_flags[ud_objp->item_type].name);
    FLDNUM("weight", ud_objp->weight);
    FLDNUM("v0", ud_objp->value[0]);
    FLDNUM("v1", ud_objp->value[1]);
    FLDNUM("v2", ud_objp->value[2]);
    FLDNUM("v3", ud_objp->value[3]);
    FLDNUM("v4", ud_objp->value[4]);
    FLDNUM("v5", ud_objp->value[5]);
    
    return 0;
}

static int get_OBJ_field ( lua_State *LS )
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_OBJ); /* Need this for type checking */

    OBJ_DATA *ud_obj = check_OBJ(LS, 1);

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
    FLDNUM("weight", ud_obj->weight);
    
    if ( !strcmp(argument, "proto" ) )
    {
        if ( !ud_obj->pIndexData )
          return 0;
          
        make_ud_table(LS, ud_obj->pIndexData, UDTYPE_OBJPROTO);
        return 1;
    }
    
    if ( !strcmp(argument, "contents") )
    {
        int index=1;
        lua_newtable(LS);
        OBJ_DATA *obj;
        for (obj=ud_obj->contains ; obj ; obj=obj->next_content)
        {
            make_ud_table(LS, obj, UDTYPE_OBJ);
            lua_rawseti(LS, -2, index++);
        }
        return 1;
    }
 
    if (!strcmp(argument, "inroom") )
    {
        if (!ud_obj->in_room)
            return 0;

        make_ud_table(LS, ud_obj->in_room, UDTYPE_ROOM);
        return 1;
    }

    if (!strcmp(argument, "carriedby") )
    {
        if (!ud_obj->carried_by )
            return 0;

        make_ud_table(LS, ud_obj->carried_by, UDTYPE_CH);
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

static int check_EXIT_equal( lua_State *LS)
{
    lua_pushboolean( LS, check_exit(LS, 1) == check_exit(LS, 2) );
    return 1;
}

static int get_EXIT_field ( lua_State *LS )
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_EXIT); /* Need this for type checking */

    EXIT_DATA *ud_exit = check_exit(LS, 1);

    if ( !ud_exit )
        return 0;

    if (!strcmp(argument, "toroom"))
    {
        make_ud_table( LS, ud_exit->u1.to_room, UDTYPE_ROOM );
        return 1;
    }
    FLDSTR("keyword", ud_exit->keyword ? ud_exit->keyword : "");
    FLDSTR("description", ud_exit->description ? ud_exit->description : "");
    FLDNUM("key", ud_exit->key);

    return 0;

}
static int check_ROOM_equal( lua_State *LS)
{
    lua_pushboolean( LS, check_ROOM(LS, 1) == check_ROOM(LS, 2) );
    return 1;
}

static int get_ROOM_field ( lua_State *LS )
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_ROOM); /* Need this for type checking */

    ROOM_INDEX_DATA *ud_room = check_ROOM(LS, 1);

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

    if ( !strcmp(argument, "people") )
    {   
        int index=1;
        lua_newtable(LS);
        CHAR_DATA *people;
        for (people=ud_room->people ; people ; people=people->next_in_room)
        {
            make_ud_table(LS, people, UDTYPE_CH);
            lua_rawseti(LS, -2, index++);
        }
        return 1;
    }

    /* array of valid exit names*/
    if ( !strcmp(argument, "exits") )
    {
        lua_newtable(LS);
        sh_int i;
        sh_int index=1;
        for ( i=0; i<MAX_DIR ; i++)
        {
            if (ud_room->exit[i])
            {
                lua_pushstring(LS,dir_name[i]);
                lua_rawseti(LS, -2, index++);
            }
        }
        return 1;
    }
            

    /* specific EXITs*/
    sh_int i;
    for (i=0; i<MAX_DIR; i++)
    {
        if (!strcmp(dir_name[i], argument) )
        {
            if (!ud_room->exit[i])
                return 0;

            lua_newtable(LS);
            make_ud_table(LS, ud_room->exit[i], UDTYPE_EXIT);
            return 1;
        }
    }

    return 0;
}

static int check_CH_equal ( lua_State *LS)
{
    lua_pushboolean( LS, check_CH(LS,1) == check_CH(LS,2) );
    return 1;
}

static int get_CH_field ( lua_State *LS)
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_CH); /* Need this for type checking */

    CHAR_DATA *ud_ch = check_CH(LS, 1);

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
    FLDSTR("shortdescr", ud_ch->short_descr ? ud_ch->short_descr : "");
    FLDSTR("longdescr", ud_ch->long_descr ? ud_ch->long_descr : "");
    if ( !strcmp(argument, "inventory") )
    {
        int index=1;
        lua_newtable(LS);
        OBJ_DATA *obj;
        for (obj=ud_ch->carrying ; obj ; obj=obj->next_content)
        {
            make_ud_table(LS, obj, UDTYPE_OBJ);
            lua_rawseti(LS, -2, index++);
        }
        return 1;
    } 

    if ( !strcmp(argument, "room" ) )
    {
        make_ud_table(LS, ud_ch->in_room, UDTYPE_ROOM);
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
    else
        /* MOB specific stuff */
    {
        FLDNUM("vnum", ud_ch->pIndexData->vnum);
    }



    return 0;

}

static int newindex_error ( lua_State *LS)
{
    lua_getfield(LS, 1, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    luaL_error( LS,"Cannot set values on game objects. UDTYPE: %d", type);
    return 0;

}

static const struct luaL_reg OBJ_metatable [] =
{
    {"__tostring", OBJ2string},
    {"__index", get_OBJ_field},
    {"__newindex", newindex_error},
    {"__eq", check_OBJ_equal},
    {NULL, NULL}
};

static const struct luaL_reg OBJPROTO_metatable [] =
{
    {"__tostring", OBJPROTO2string},
    {"__index", get_OBJPROTO_field},
    {"__newindex", newindex_error},
    {"__eq", check_OBJPROTO_equal},
    {NULL, NULL}
};

static const struct luaL_reg ROOM_metatable [] =
{
    {"__tostring", ROOM2string},
    {"__index", get_ROOM_field},
    {"__newindex", newindex_error},
    {"__eq", check_ROOM_equal},
    {NULL, NULL}
};

static const struct luaL_reg CH_metatable [] = 
{
    {"__tostring", CH2string},
    {"__index", get_CH_field},
    {"__newindex", newindex_error},
    {"__eq", check_CH_equal},
    {NULL, NULL}
};

static const struct luaL_reg EXIT_metatable [] =
{
    {"__tostring", EXIT2string},
    {"__index", get_EXIT_field},
    {"__newindex", newindex_error},
    {"__eq", check_EXIT_equal},
    {NULL, NULL}
};

void RegisterGlobalFunctions(lua_State *LS)
{
    lua_register(LS,"say",         L_say);
    lua_register(LS,"emote",       L_emote);
    lua_register(LS,"mdo",         L_mdo);

    /* mob commands */
    lua_register(LS,"asound",      L_cmd_asound);
    lua_register(LS,"gecho",       L_cmd_gecho);
    lua_register(LS,"zecho",       L_cmd_zecho);
    lua_register(LS,"kill",        L_cmd_kill);
    lua_register(LS,"assist",      L_cmd_assist);
    lua_register(LS,"junk",        L_cmd_junk);
    lua_register(LS,"echo",        L_cmd_echo);
    lua_register(LS,"echoaround",  L_cmd_echoaround);
    lua_register(LS,"echoat",      L_cmd_echoat);
    lua_register(LS,"mload",       L_cmd_mload);
    lua_register(LS,"oload",       L_cmd_oload);
    lua_register(LS,"purge",       L_cmd_purge);
    lua_register(LS,"goto",        L_cmd_goto);
    lua_register(LS,"at",          L_cmd_at);
    lua_register(LS,"transfer",    L_cmd_transfer);
    lua_register(LS,"gtransfer",   L_cmd_gtransfer);
    lua_register(LS,"otransfer",   L_cmd_otransfer);
    lua_register(LS,"force",       L_cmd_force);
    lua_register(LS,"gforce",      L_cmd_gforce);
    lua_register(LS,"vforce",      L_cmd_vforce);
    lua_register(LS,"cast",        L_cmd_cast);
    lua_register(LS,"damage",      L_cmd_damage);
    lua_register(LS,"remember",    L_cmd_remember);
    lua_register(LS,"forget",      L_cmd_forget);
    lua_register(LS,"delay",       L_cmd_delay);
    lua_register(LS,"cancel",      L_cmd_cancel);
    lua_register(LS,"call",        L_cmd_call);
    lua_register(LS,"flee",        L_cmd_flee);
    lua_register(LS,"remove",      L_cmd_remove);
    lua_register(LS,"remort",      L_cmd_remort);
    lua_register(LS,"qset",        L_cmd_qset);
    lua_register(LS,"qadvance",    L_cmd_qadvance);
    lua_register(LS,"reward",      L_cmd_reward);
    lua_register(LS,"peace",       L_cmd_peace);
    lua_register(LS,"restore",     L_cmd_restore);
    lua_register(LS,"act",         L_cmd_act);
    lua_register(LS,"hit",         L_cmd_hit);

    /* checks */
    lua_register(LS,"mobhere",     L_mobhere);
    lua_register(LS,"objhere",     L_objhere);
    lua_register(LS,"mobexists",   L_mobexists);
    lua_register(LS,"objexists",   L_objexists);
    lua_register(LS,"hour",        L_hour);
    lua_register(LS,"ispc",        L_ispc);
    lua_register(LS,"isnpc",       L_isnpc);
    lua_register(LS,"isgood",      L_isgood);
    lua_register(LS,"isevil",      L_isevil);
    lua_register(LS,"isneutral",   L_isneutral);
    lua_register(LS,"isimmort",    L_isimmort);
    lua_register(LS,"ischarm",     L_ischarm);
    lua_register(LS,"isfollow",    L_isfollow);
    lua_register(LS,"isactive",    L_isactive);
    lua_register(LS,"isdelay",     L_isdelay);
    lua_register(LS,"isvisible",   L_isvisible);
    lua_register(LS,"hastarget",   L_hastarget);
    lua_register(LS,"istarget",    L_istarget);
    lua_register(LS,"affected",    L_affected);
    lua_register(LS,"act",         L_act);
    lua_register(LS,"off",         L_off);
    lua_register(LS,"imm",         L_imm);
    lua_register(LS,"carries",     L_carries);
    lua_register(LS,"wears",       L_wears);
    lua_register(LS,"has",         L_has);
    lua_register(LS,"uses",        L_uses);
    lua_register(LS,"name",        L_name);
    lua_register(LS,"qstatus",     L_qstatus);
    lua_register(LS,"vuln",        L_vuln);
    lua_register(LS,"res",         L_res);
    lua_register(LS,"skilled",     L_skilled);
    lua_register(LS,"ccarries",    L_ccarries);
    lua_register(LS,"qtimer",      L_qtimer);

    /* other */
    lua_register(LS,"getroom",     L_getroom);
    lua_register(LS,"randchar",    L_randchar);

    lua_register(LS,"loadprog",    L_loadprog);

}

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

    luaopen_bits (LS);     /* bit manipulation */
    luaL_register (LS, MT_LIBRARY, mtlib);  /* Mersenne Twister */

    /* Marsenne Twister generator  */
    init_genrand (timer);

    RegisterGlobalFunctions(LS);

    /* meta table to identify object types */
    luaL_newmetatable(LS, CH_META);
    luaL_register (LS, NULL, CH_metatable); 
    luaL_newmetatable(LS, OBJ_META);
    luaL_register (LS, NULL, OBJ_metatable);
    luaL_newmetatable(LS, ROOM_META);
    luaL_register (LS, NULL, ROOM_metatable);
    luaL_newmetatable(LS, EXIT_META);
    luaL_register (LS, NULL, EXIT_metatable);
    luaL_newmetatable(LS, OBJPROTO_META);
    luaL_register (LS, NULL, OBJPROTO_metatable);

    /* our metatable for lightuserdata */
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

extern      char last_command [MSL];
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


bool lua_load_mprog( lua_State *LS, int vnum, char *code)
{
    char buf[MSL];

    sprintf(buf, "function P_%d (%s,%s,%s,%s,%s,%s,%s,%s)"
            "%s\n"
            "end",
            vnum,
            MOB_ARG, CH_ARG, TRIG_ARG, OBJ1_ARG,
            OBJ2_ARG, TEXT1_ARG, TEXT2_ARG, VICTIM_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 0))
    {
        bugf ( "LUA mprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));
        /* bad code, let's kill it */
        sprintf(buf, "P_%d", vnum);
        lua_pushnil( LS );
        lua_setglobal( LS, buf);

        return FALSE;
    }
    else return TRUE;

}

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

    /* load up the script as a function so args will be local */
    char buf[MSL*2];
    sprintf(buf, "P_%d", pvnum);
    lua_getglobal( mob->LS, buf);

    if ( lua_isnil( mob->LS, -1) )
    {
        /* not loaded yet*/
        if ( !lua_load_mprog( mob->LS, pvnum, source) )
        {
            /* don't bother running it if there were errors */
            return;
        }

        /* loaded without errors, now get it on the stack */
        lua_getglobal( mob->LS, buf);
    }

    /* MOB_ARG */
    make_ud_table (mob->LS, (void *)mob, UDTYPE_CH);

    /* CH_ARG */
    if (ch)
        make_ud_table (mob->LS,(void *) ch, UDTYPE_CH);
    else lua_pushnil(mob->LS);

    /* TRIG_ARG */
    if (text)
        lua_pushstring ( mob->LS, text);
    else lua_pushnil(mob->LS);

    /* OBJ1_ARG */
    if (arg1type== ACT_ARG_OBJ && arg1)
        make_ud_table( mob->LS, arg1, UDTYPE_OBJ);
    else lua_pushnil(mob->LS);

    /* OBJ2_ARG */
    if (arg2type== ACT_ARG_OBJ && arg2)
        make_ud_table( mob->LS, arg2, UDTYPE_OBJ);
    else lua_pushnil(mob->LS);

    /* TEXT1_ARG */
    if (arg1type== ACT_ARG_TEXT && arg1)
        lua_pushstring ( mob->LS, (char *)arg1);
    else lua_pushnil(mob->LS);

    /* TEXT2_ARG */
    if (arg2type== ACT_ARG_TEXT && arg2)
        lua_pushstring ( mob->LS, (char *)arg2);
    else lua_pushnil(mob->LS);

    /* VICTIM_ARG */
    if (arg2type== ACT_ARG_CHARACTER)
        make_ud_table( mob->LS, arg2, UDTYPE_CH);
    else lua_pushnil(mob->LS);


    if ( CallLuaWithTraceBack (mob->LS, NUM_MPROG_ARGS, 0))
    {
        bugf ( "LUA mprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(mob->LS, -1));
    }


    /* cleanup routines */
    lua_getfield( mob->LS, LUA_GLOBALSINDEX, CLEANUP_FUNCTION);
    if ( CallLuaWithTraceBack (mob->LS, 0, 0))
    {
        bugf ( "Cleanup error for vnum %d:\n %s",
                pvnum,
                lua_tostring(mob->LS, -1));
    }

    lua_settop (mob->LS, 0);    /* get rid of stuff lying around */
}


