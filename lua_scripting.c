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

lua_State *g_mud_LS = NULL;  /* Lua state for entire MUD */
/* Mersenne Twister stuff - see mt19937ar.c */

#define LUA_LOOP_CHECK_MAX_CNT 10000 /* give 1000000 instructions */
#define LUA_LOOP_CHECK_INCREMENT 100
#define ERR_INF_LOOP      -1


/* file scope variables */
static bool        s_LuaScriptInProgress=FALSE;
static int         s_LoopCheckCounter;

void init_genrand(unsigned long s);
void init_by_array(unsigned long init_key[], int key_length);
double genrand(void);

static int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn);
static const struct luaL_reg CH_lib [];
static const struct luaL_reg OBJ_lib [];
static const struct luaL_reg OBJPROTO_lib[];
static const struct luaL_reg ROOM_lib [];
static const struct luaL_reg EXIT_lib [];
static const struct luaL_reg AREA_lib [];

#define CHARACTER_STATE "character.state"
#define CH_META        "CH.meta"
#define UD_META        "UD.meta"
#define OBJ_META       "OBJ.meta"
#define OBJPROTO_META  "OBJPROTO.meta"
#define ROOM_META      "ROOM.meta"
#define EXIT_META      "EXIT.meta"
#define AREA_META	   "AREA.meta"
#define MUD_LIBRARY "mud"
#define MT_LIBRARY "mt"
#define UD_TABLE_NAME "udtbl"

/* Names of some functions declared on the lua side */
#define REGISTER_UD_FUNCTION "RegisterUd"
#define UNREGISTER_UD_FUNCTION "UnregisterUd"
#define GETSCRIPT_FUNCTION "GetScript"
#define SAVETABLE_FUNCTION "SaveTable"
#define LOADTABLE_FUNCTION "LoadTable"
#define TPRINTSTR_FUNCTION "tprintstr"

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

#define UDTYPE_UNDEFINED 0
#define UDTYPE_CH        1
#define UDTYPE_OBJ       2
#define UDTYPE_ROOM      3
#define UDTYPE_EXIT      4
#define UDTYPE_OBJPROTO  5
#define UDTYPE_AREA      6


// number of items in an array
#define NUMITEMS(arg) (sizeof (arg) / sizeof (arg [0]))

LUALIB_API int luaopen_bits(lua_State *LS);  /* Implemented in lua_bits.c */

static void stackDump (lua_State *LS) {
      int i;
      int top = lua_gettop(LS);
      for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(LS, i);
        switch (t) {
    
          case LUA_TSTRING:  /* strings */
            bugf("`%s'", lua_tostring(LS, i));
            break;
    
          case LUA_TBOOLEAN:  /* booleans */
            bugf(lua_toboolean(LS, i) ? "true" : "false");
            break;
    
          case LUA_TNUMBER:  /* numbers */
            bugf("%g", lua_tonumber(LS, i));
            break;
    
          default:  /* other values */
            bugf("%s", lua_typename(LS, t));
            break;
    
        }
        bugf("  ");  /* put a separator */
      }
      bugf("\n");  /* end the listing */
    }

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

static OBJ_INDEX_DATA *check_OBJPROTO( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    lua_pop(LS, 1);

    if ( type != UDTYPE_OBJPROTO )
    {
        luaL_error(LS,"Bad parameter %d. Expected OBJPROTO.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    OBJ_INDEX_DATA *oid=luaL_checkudata(LS, -1, UD_META);
    lua_pop(LS, 1);
    return oid;
}

static OBJ_DATA *check_OBJ( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    lua_pop(LS, 1);
    if ( type != UDTYPE_OBJ )
    {
        luaL_error(LS,"Bad parameter %d. Expected OBJ.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    OBJ_DATA *obj=(OBJ_DATA *)luaL_checkudata(LS, -1, UD_META);
    lua_pop(LS, 1);
    return obj;
}

static CHAR_DATA *check_CH( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    lua_pop(LS, 1);
    if ( type != UDTYPE_CH )
    {
        luaL_error(LS, "Bad parameter %d. Expected CH.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    CHAR_DATA *ch=luaL_checkudata(LS, -1, UD_META);
    lua_pop(LS, 1);
    return ch; 
}

static ROOM_INDEX_DATA *check_ROOM( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    lua_pop(LS, 1);
    if ( type != UDTYPE_ROOM )
    {
        luaL_error(LS, "Bad parameter %d. Expected ROOM.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    ROOM_INDEX_DATA *room=(ROOM_INDEX_DATA *)luaL_checkudata(LS, -1, UD_META);
    lua_pop(LS, 1);
    return room;
}

static EXIT_DATA *check_exit( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    lua_pop(LS, 1);
    if ( type != UDTYPE_EXIT )
    {
        luaL_error(LS, "Bad parameter %d. Expected EXIT.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    EXIT_DATA *exit=(EXIT_DATA *)luaL_checkudata(LS, -1, UD_META);
    lua_pop(LS, 1);
    return exit;
}

static AREA_DATA *check_AREA( lua_State *LS, int arg)
{
    lua_getfield(LS, arg, "UDTYPE");
    sh_int type= luaL_checknumber(LS, -1);
    lua_pop(LS, 1);
    if ( type != UDTYPE_AREA )
    {
        luaL_error(LS, "Bad parameter %d. Expected AREA.", arg );
        return NULL;
    }

    lua_getfield(LS, arg, "tableid");
    AREA_DATA *exit=(EXIT_DATA *)luaL_checkudata(LS, -1, UD_META);
    lua_pop(LS, 1);
    return exit;
}

static void make_ud_table ( lua_State *LS, void *ptr, int UDTYPE, bool reg )
{
    if ( !ptr )
        luaL_error (LS, "make_ud_table called with NULL object. UDTYPE: %d", UDTYPE);
    
    /* see if it exists already */
    lua_getglobal(g_mud_LS, UD_TABLE_NAME);
    if ( lua_isnil(g_mud_LS, -1) )
    {
        bugf("udtbl is nil in make_ud_table.");
        return;
    }
    
    lua_pushlightuserdata(g_mud_LS, ptr);
    lua_gettable( g_mud_LS, -2);
    lua_remove( g_mud_LS, -2); /* don't need udtbl anymore */
    
    if ( ! lua_isnil(g_mud_LS, -1) )
    {
        /* already exists, now at top of stack */
        //log_string("already exists in make_ud_table");
        //bugf("%d",UDTYPE);  
        return;
    }
    lua_remove(g_mud_LS, -1); // kill the nil 
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
		case UDTYPE_AREA:
			meta=AREA_META; luaL_register(LS,NULL,AREA_lib); break;
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

    if ( reg )
    {
        lua_getfield( LS, LUA_GLOBALSINDEX, REGISTER_UD_FUNCTION);
        lua_pushvalue( LS, -2);
        if (CallLuaWithTraceBack( LS, 1, 0) )
        {
            bugf ( "Error registering UD:\n %s",
                    lua_tostring(LS, -1));
        }
    }
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

static void infinite_loop_check_hook( lua_State *LS, lua_Debug *ar)
{
    if (!s_LuaScriptInProgress)
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

static int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn)
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

static int L_sendtochar (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    char *msg=luaL_checkstring(LS, 2);

    send_to_char(msg, ch);
    return 0;
}

static int L_pagetochar (lua_State *LS)
{
    page_to_char( luaL_checkstring(LS, 2),
                  check_CH(LS,1) );

    return 0;
}

static int L_getcharlist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        make_ud_table(LS, ch, UDTYPE_CH, TRUE);
        lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int L_getmoblist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if ( IS_NPC(ch) )
        {
            make_ud_table(LS, ch, UDTYPE_CH, TRUE);
            lua_rawseti(LS, -2, index++);
        }
    }

    return 1;
}    

static int L_getplayerlist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if ( !IS_NPC(ch) )
        {
            make_ud_table(LS, ch, UDTYPE_CH, TRUE);
            lua_rawseti(LS, -2, index++);
        }
    }

    return 1;
}

static int L_getmobworld (lua_State *LS)
{
    int num = luaL_checknumber (LS, 1);

    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);
    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if ( ch->pIndexData )
        {
            if ( ch->pIndexData->vnum == num )
            {
                make_ud_table( LS, ch, UDTYPE_CH, TRUE);
                lua_rawseti(LS, -2, index++);
            }
        }
    }
    return 1;
}

static int L_getobjworld (lua_State *LS)
{
    int num = luaL_checknumber (LS, 1);

    OBJ_DATA *obj;

    int index=1;
    lua_newtable(LS);
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( obj->pIndexData->vnum == num )
        {
            make_ud_table( LS, obj, UDTYPE_OBJ, TRUE);
            lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}


static int L_getroom (lua_State *LS)
{
    // do some if is number thing here eventually
    int num = luaL_checknumber (LS, 1);

    ROOM_INDEX_DATA *room=get_room_index(num);

    if (!room)
        return 0;

    make_ud_table( LS, room, UDTYPE_ROOM, TRUE);
    return 1;

}

static int L_getobjproto (lua_State *LS)
{
    int num = luaL_checknumber (LS, 1);

    OBJ_INDEX_DATA *obj=get_obj_index(num);

    if (!obj)
        return 0;

    make_ud_table( LS, obj, UDTYPE_OBJPROTO, TRUE);
    return 1;
}

static int L_log (lua_State *LS)
{
    char buf[MSL];
    sprintf(buf, "LUA::%s", luaL_checkstring (LS, 1));

    log_string(buf);
    return 0;
}

static int L_ch_randchar (lua_State *LS)
{
    CHAR_DATA *ch=get_random_char(check_CH(LS,1) );
    if ( ! ch )
        return 0;

    make_ud_table( LS, ch, UDTYPE_CH, TRUE);
    return 1;

}

static int L_ch_tprint ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);

    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    
    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    do_say( ud_ch, luaL_checkstring (LS, -1));
    
    return 0;
}

static int L_ch_savetbl (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
    {
        luaL_error( g_mud_LS, "PCs cannot call savetbl.");
        return 0;
    }

    lua_getfield( LS, LUA_GLOBALSINDEX, SAVETABLE_FUNCTION);

    /* Push original args into SaveTable */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_pushstring( LS, ud_ch->pIndexData->area->file_name );
    lua_call( LS, 3, 0);

    return 0;
}

static int L_obj_savetbl (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, SAVETABLE_FUNCTION);

    /* Push original args into SaveTable */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_pushstring( LS, ud_obj->pIndexData->area->file_name );
    lua_call( LS, 3, 0);

    return 0;
}

static int L_area_savetbl (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, SAVETABLE_FUNCTION);

    /* Push original args into SaveTable */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_pushstring( LS, ud_area->file_name );
    lua_call( LS, 3, 0);

    return 0;
}

static int L_ch_loadtbl (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
    {
        luaL_error( g_mud_LS, "PCs cannot call loadtbl.");
        return 0;
    }

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_ch->pIndexData->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int L_obj_loadtbl (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_obj->pIndexData->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int L_area_loadtbl (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

#define LOADSCRIPT_VNUM 0
static int L_ch_loadscript (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);
    
    /* now run the result as a regular mprog with vnum 0*/
    lua_mob_program( NULL, LOADSCRIPT_VNUM, luaL_checkstring(LS, -1), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL ); 

    return 0;
}
    

static int L_ch_loadprog (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int num = luaL_checknumber (LS, 2);
    MPROG_CODE *pMcode;

    if ( (pMcode = get_mprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: mprog vnum %d doesn't exist", num);
        return 0;
    }

    if ( !pMcode->is_lua)
    {
        luaL_error(LS, "loadprog: mprog vnum %d is not lua code", num);
        return 0;
    }

    lua_mob_program( NULL, num, pMcode->code, ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL ); 

    return 0;
}

static int L_obj_loadscript (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular oprog with vnum 0*/

    lua_pushboolean( LS, 
        lua_obj_program( NULL, LOADSCRIPT_VNUM, luaL_checkstring( LS, -1), ud_obj, NULL, NULL, NULL, OTRIG_CALL) );

    return 1;

}

static int L_obj_loadprog (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int num = luaL_checknumber (LS, 2);
    OPROG_CODE *pOcode;

    if ( (pOcode = get_oprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: oprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS, 
        lua_obj_program( NULL, num, pOcode->code, ud_obj, NULL, NULL, NULL, OTRIG_CALL) );

    return 1;
}

static int L_area_loadscript (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular aprog with vnum 0*/
    lua_pushboolean( LS,
        lua_area_program( NULL, LOADSCRIPT_VNUM, luaL_checkstring( LS, -1), ud_area, NULL, ATRIG_CALL) );

    return 1;

}

static int L_area_loadprog (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int num = luaL_checknumber (LS, 2);
    APROG_CODE *pAcode;

    if ( (pAcode = get_aprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: aprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_area_program( NULL, num, pAcode->code, ud_area, NULL, ATRIG_CALL) );

    return 1;
}

static int L_ch_emote (lua_State *LS)
{
    do_emote( check_CH(LS, 1), luaL_checkstring (LS, 2) );
    return 0;
}

static int L_ch_asound (lua_State *LS)
{
    do_mpasound( check_CH(LS, 1), luaL_checkstring (LS, 2));
    return 0; 
}

static int L_ch_gecho (lua_State *LS)
{
    do_mpgecho( check_CH(LS, 1), luaL_checkstring(LS, 2));
    return 0;
}

static int L_ch_zecho (lua_State *LS)
{

    do_mpzecho( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_kill (lua_State *LS)
{

    do_mpkill( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_assist (lua_State *LS)
{
    do_mpassist( check_CH(LS, 1), luaL_checkstring(LS, 2));
    return 0;
}

static int L_ch_junk (lua_State *LS)
{

    do_mpjunk( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_echo (lua_State *LS)
{

    do_mpecho( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_echoaround (lua_State *LS)
{

    do_mpechoaround( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_echoat (lua_State *LS)
{

    do_mpechoat( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_mload (lua_State *LS)
{

    do_mpmload( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_purge (lua_State *LS)
{

    // Send empty string for no argument
    if ( lua_isnone( g_mud_LS, 2) )
    {
        do_mppurge( check_CH(LS, 1), "");
    }
    else
    {
        do_mppurge( check_CH(LS, 1), luaL_checkstring(LS, 2));
    }

    return 0;
}

static int L_ch_goto (lua_State *LS)
{

    do_mpgoto( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_at (lua_State *LS)
{

    do_mpat( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_transfer (lua_State *LS)
{

    do_mptransfer( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_gtransfer (lua_State *LS)
{

    do_mpgtransfer( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_otransfer (lua_State *LS)
{

    do_mpotransfer( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_force (lua_State *LS)
{

    do_mpforce( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}


static int L_ch_gforce (lua_State *LS)
{

    do_mpgforce( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_vforce (lua_State *LS)
{

    do_mpvforce( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_cast (lua_State *LS)
{

    do_mpcast( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_damage (lua_State *LS)
{
    do_mpdamage( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_delay (lua_State *LS)
{

    do_mpdelay( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_cancel (lua_State *LS)
{

    do_mpcancel( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_call (lua_State *LS)
{

    do_mpcall( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_remove (lua_State *LS)
{

    do_mpremove( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_remort (lua_State *LS)
{

    do_mpremort( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_qset (lua_State *LS)
{

    do_mpqset( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_qadvance (lua_State *LS)
{

    do_mpqadvance( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_reward (lua_State *LS)
{

    do_mpreward( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_peace (lua_State *LS)
{
    if ( lua_isnone( g_mud_LS, 2) )
        do_mppeace( check_CH(LS, 1), "");
    else
        do_mppeace( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_restore (lua_State *LS)
{
    do_mprestore( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_setact (lua_State *LS)
{
    do_mpact( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;
}

static int L_ch_hit (lua_State *LS)
{
    do_mphit( check_CH(LS, 1), luaL_checkstring(LS, 2));

    return 0;

}

static int L_ch_mdo (lua_State *LS)
{
    interpret( check_CH(LS, 1), luaL_checkstring (LS, 2));

    return 0;
}

static int L_ch_mobhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, (bool) get_mob_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) ); 
    else
        lua_pushboolean( LS,  (bool) (get_char_room( ud_ch, argument) != NULL) );

    return 1;
}

static int L_ch_objhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS,(bool) get_obj_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) );
    else
        lua_pushboolean( LS,(bool) (get_obj_here( ud_ch, argument) != NULL) );

    return 1;
}

static int L_ch_mobexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,(bool) (get_mp_char( ud_ch, argument) != NULL) );

    return 1;
}

static int L_ch_objexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, (bool) (get_mp_obj( ud_ch, argument) != NULL) );

    return 1;
}

static int L_hour (lua_State *LS)
{
    lua_pushnumber( LS, time_info.hour );
    return 1;
}

static int L_ch_ispc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && !IS_NPC( ud_ch ) );
    return 1;
}

static int L_ch_canattack (lua_State *LS)
{
    lua_pushboolean( LS, !is_safe(check_CH (LS, 1), check_CH (LS, 2)) );
    return 1;
}

static int L_ch_isnpc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_NPC( ud_ch ) );
    return 1;
}

static int L_ch_isgood (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_GOOD( ud_ch ) ) ;
    return 1;
}

static int L_ch_isevil (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_EVIL( ud_ch ) ) ;
    return 1;
}

static int L_ch_isneutral (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_NEUTRAL( ud_ch ) ) ;
    return 1;
}

static int L_ch_isimmort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_IMMORTAL( ud_ch ) ) ;
    return 1;
}

static int L_ch_ischarm (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_AFFECTED( ud_ch, AFF_CHARM ) ) ;
    return 1;
}

static int L_ch_isfollow (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->master != NULL ) ;
    return 1;
}

static int L_ch_isactive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->position > POS_SLEEPING ) ;
    return 1;
}

static int L_ch_isvisible (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH(LS, 1);
    CHAR_DATA * ud_vic = check_CH (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && ud_vic != NULL && can_see( ud_ch, ud_vic ) ) ;

    return 1;
}

static int L_ch_affected (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    
    lua_pushboolean( LS,  ud_ch != NULL
            &&  is_affected_parse(ud_ch, argument) );

    return 1;
}

static int L_ch_act (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    int flag=NO_FLAG;

    if (IS_NPC(ud_ch))
    {
        if ((flag=flag_lookup(argument, act_flags)) == NO_FLAG) 
            luaL_error(LS, "L_ch_act: flag '%s' not found in act_flags (mob)", argument);
    }
    else
    {
        if ((flag=flag_lookup(argument, plr_flags)) == NO_FLAG)
            luaL_error(LS, "L_ch_act: flag '%s' not found in plr_flags (player)", argument);
    }
    
    lua_pushboolean( LS, ud_ch != NULL
            &&  IS_SET(ud_ch->act, flag) );

    return 1;
}

static int L_ch_offensive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    int flag=flag_lookup(argument, off_flags);

    if ( flag == NO_FLAG )
        luaL_error(LS, "L_ch_offesive: flag '%s' not found in off_flags", argument);

    lua_pushboolean( LS,
            IS_SET(ud_ch->off_flags, flag) );

    return 1;
}

static int L_ch_immune (lua_State *LS)
{ 
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    int flag=flag_lookup(argument, imm_flags);

    if ( flag == NO_FLAG ) 
        luaL_error(LS, "L_ch_immune: flag '%s' not found in imm_flags", argument);

    lua_pushboolean( LS, ud_ch != NULL
            &&  IS_SET(ud_ch->imm_flags, flag) );

    return 1;
}

static int L_ch_carries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, FALSE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_carry( ud_ch, argument, ud_ch ) != NULL) );

    return 1;
}

static int L_ch_wears (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, TRUE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_wear( ud_ch, argument ) != NULL) );

    return 1;
}

static int L_ch_has (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), FALSE ) );

    return 1;
}

static int L_ch_uses (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), TRUE ) );

    return 1;
}


static int L_ch_qstatus (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, quest_status( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_ch_say (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    do_say( ud_ch, luaL_checkstring(LS, 2) );
    return 0;
}


static int L_ch_setlevel (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Cannot set level on PC.");

    int num = luaL_checknumber (LS, 2);
    set_mob_level( ud_ch, num );
    return 0;
}

static int L_ch_oload (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj=create_object( pObjIndex, 0);
    check_enchant_obj( obj );

    obj_to_char(obj,ud_ch);

    make_ud_table(LS, obj, UDTYPE_OBJ, TRUE);
    return 1;

}

static int L_ch_destroy (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    if (!ud_ch)
    {
        luaL_error(LS, "Null pointer in L_ch_destroy");
        return 0;
    }

    if (!IS_NPC(ud_ch))
    {
        luaL_error(LS, "Trying to destroy player");
        return 0;
    }
    
    extract_char(ud_ch,TRUE);
    return 0;
}

static int L_ch_vuln (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    int flag=flag_lookup(argument, vuln_flags);

    if ( flag == NO_FLAG )
        luaL_error(LS, "L_vuln: flag '%s' not found in vuln_flags", argument);

    lua_pushboolean( LS, ud_ch != NULL
            && IS_SET(ud_ch->vuln_flags, flag ) );

    return 1;
}

static int L_ch_resist (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    int flag=flag_lookup(argument, res_flags);

    if ( flag == NO_FLAG )
        luaL_error(LS, "L_res: flag '%s' not found in res_flags", argument);

    lua_pushboolean( LS, ud_ch != NULL
            && IS_SET(ud_ch->res_flags, flag) );

    return 1;
}

static int L_ch_skilled (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && skill_lookup(argument) != -1
            && get_skill(ud_ch, skill_lookup(argument)) > 0 );

    return 1;
}

static int L_ch_ccarries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( is_r_number( argument ) )
    {
        lua_pushboolean( LS, ud_ch != NULL && has_item_in_container( ud_ch, r_atoi(ud_ch, argument), "zzyzzxzzyxyx" ) );
    }
    else
    {
         lua_pushboolean( LS, ud_ch != NULL && has_item_in_container( ud_ch, -1, argument ) );
    }

    return 1;
}

static int L_ch_qtimer (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, qset_timer( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

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

static int L_room_mload (lua_State *LS)
{
    ROOM_INDEX_DATA * ud_room = check_ROOM (LS, 1);
    int num = luaL_checknumber (LS, 2);
    MOB_INDEX_DATA *pObjIndex = get_mob_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No mob with vnum: %d", num);

    CHAR_DATA *mob=create_mobile( pObjIndex);
    arm_npc( mob );
    char_to_room(mob,ud_room);

    make_ud_table(LS, mob, UDTYPE_CH, TRUE);
    return 1;

}

static int L_room_oload (lua_State *LS)
{
    ROOM_INDEX_DATA * ud_room = check_ROOM (LS, 1);
    int num = luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj=create_object( pObjIndex, 0);
    check_enchant_obj( obj );
    obj_to_room(obj,ud_room);

    make_ud_table(LS, obj, UDTYPE_OBJ, TRUE);
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

static int L_room_echo( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room = check_ROOM(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    CHAR_DATA *vic;
    for ( vic=ud_room->people ; vic ; vic=vic->next_in_room )
    {
        if (!IS_NPC(vic) )
        {
            send_to_char(argument, vic);
            send_to_char("\n\r", vic);
        }
    }

    return 0;
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

static int L_obj_destroy( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);

    if (!ud_obj)
    {
        luaL_error(LS, "Null pointer in L_obj_destroy.");
        return 0;
    }
    extract_obj(ud_obj);
    return 0;
}

static int L_obj_echo( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    char *argument= luaL_checkstring (LS, 2);

    if (ud_obj->carried_by)
    {
        send_to_char(argument, ud_obj->carried_by);
        send_to_char( "\n\r", ud_obj->carried_by);
    }
    else if (ud_obj->in_room)
    {
        DESCRIPTOR_DATA *d;
        for ( d = descriptor_list; d; d = d->next )
        {
            if ( (d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected))
                &&   d->character->in_room == ud_obj->in_room )
            {
                send_to_char( argument, d->character );
                send_to_char( "\n\r",   d->character );
            }
        } 
    }
    else
    {
        // Nothing, must be in a container
    }
    
    return 0;
}

static int L_obj_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, L_obj_echo );
    /* now line up argumenets for echo */
    lua_pushvalue( LS, 1); /* obj */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;

}

static int L_obj_oload (lua_State *LS)
{
    OBJ_DATA * ud_obj = check_OBJ (LS, 1);
    int num = luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if ( ud_obj->item_type != ITEM_CONTAINER )
    {
        luaL_error(LS, "Tried to load object in non-container." );
    }

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj=create_object( pObjIndex, 0);
    check_enchant_obj( obj );
    obj_to_obj(obj,ud_obj);

    make_ud_table(LS, obj, UDTYPE_OBJ, TRUE);
    return 1;

}

static int L_obj_wear( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    sh_int flag=flag_lookup( argument, wear_flags);
    if ( flag==NO_FLAG )
        luaL_error( LS, "Invalid wear flag '%s'", argument );

    lua_pushboolean( LS, IS_SET( ud_obj->wear_flags, flag));
    return 1;
}

static int L_obj_extra( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    sh_int flag=flag_lookup( argument, extra_flags);
    if ( flag==NO_FLAG )
        luaL_error( LS, "Invalid extra flag '%s'", argument );

    lua_pushboolean( LS, IS_SET( ud_obj->extra_flags, flag));
    return 1;
}

static int L_area_echo( lua_State *LS)
{
	AREA_DATA *ud_area = check_AREA(LS, 1);
	const char *argument = luaL_checkstring(LS, 2);
	DESCRIPTOR_DATA *d;

    for ( d = descriptor_list; d; d = d->next )
    {
		if ( d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected) )
		{
			if ( !d->character->in_room )
				continue;
			if ( d->character->in_room->area != ud_area )
				continue;
				
			if ( IS_IMMORTAL(d->character) )
				send_to_char( "Area echo> ", d->character );
			send_to_char( argument, d->character );
			send_to_char( "\n\r", d->character );
		}
    }
	
    return 0;
}

static int L_area_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, L_area_echo );
    /* now line up argumenets for echo */
    lua_pushvalue( LS, 1); /* area */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;

}

static const struct luaL_reg mudlib [] = 
{
    {"luadir", L_mud_luadir}, 
    {"userdir",L_mud_userdir},
    {NULL, NULL}
};  /* end of mudlib */


static const struct luaL_reg CH_lib [] =
{
    {"ispc", L_ch_ispc},
    {"isnpc", L_ch_isnpc},
    {"isgood", L_ch_isgood},
    {"isevil", L_ch_isevil},
    {"isneutral", L_ch_isneutral},
    {"isimmort", L_ch_isimmort},
    {"ischarm", L_ch_ischarm},
    {"isfollow", L_ch_isfollow},
    {"isactive", L_ch_isactive},
    {"isvisible", L_ch_isvisible},
    {"mobhere", L_ch_mobhere},
    {"objhere", L_ch_objhere},
    {"mobexists", L_ch_mobexists},
    {"objexists", L_ch_objexists},
    {"affected", L_ch_affected},
    {"act", L_ch_act},
    {"offensive", L_ch_offensive},
    {"immune", L_ch_immune},
    {"carries", L_ch_carries},
    {"wears", L_ch_wears},
    {"has", L_ch_has},
    {"uses", L_ch_uses},
    {"qstatus", L_ch_qstatus},
    {"resist", L_ch_resist},
    {"vuln", L_ch_vuln},
    {"skilled", L_ch_skilled},
    {"ccarries", L_ch_ccarries},
    {"qtimer", L_ch_qtimer},
    {"canattack", L_ch_canattack},
    {"destroy",L_ch_destroy},
    {"oload", L_ch_oload},
    {"setlevel", L_ch_setlevel},
    {"say", L_ch_say},
    {"emote", L_ch_emote},
    {"mdo", L_ch_mdo},
    {"asound", L_ch_asound},
    {"gecho", L_ch_gecho},
    {"zecho", L_ch_zecho},
    {"kill", L_ch_kill},
    {"assist", L_ch_assist},
    {"junk", L_ch_junk},
    {"echo", L_ch_echo},
    {"echoaround", L_ch_echoaround},
    {"echoat", L_ch_echoat},
    {"mload", L_ch_mload},
    {"purge", L_ch_purge},
    {"goto", L_ch_goto},
    {"at", L_ch_at},
    {"transfer", L_ch_transfer},
    {"gtransfer", L_ch_gtransfer},
    {"otransfer", L_ch_otransfer},
    {"force", L_ch_force},
    {"gforce", L_ch_gforce},
    {"vforce", L_ch_vforce},
    {"cast", L_ch_cast},
    {"damage", L_ch_damage},
    {"remove", L_ch_remove},
    {"remort", L_ch_remort},
    {"qset", L_ch_qset},
    {"qadvance", L_ch_qadvance},
    {"reward", L_ch_reward},
    {"peace", L_ch_peace},
    {"restore", L_ch_restore},
    {"setact", L_ch_setact},
    {"hit", L_ch_hit},
    {"randchar", L_ch_randchar},
    {"loadprog", L_ch_loadprog},
    {"loadscript", L_ch_loadscript},
    {"savetbl", L_ch_savetbl},
    {"loadtbl", L_ch_loadtbl},
    {"tprint", L_ch_tprint},
    {NULL, NULL}
};

static const struct luaL_reg ROOM_lib [] =
{
    {"flag", L_room_flag},
    {"oload", L_room_oload},
    {"mload", L_room_mload},
    {"echo", L_room_echo},
    {NULL, NULL}
};

static const struct luaL_reg OBJ_lib [] =
{
    {"extra", L_obj_extra},
    {"wear", L_obj_wear},
    {"destroy", L_obj_destroy},
    {"echo", L_obj_echo},
    {"loadprog", L_obj_loadprog},
    {"loadscript", L_obj_loadscript},
    {"oload", L_obj_oload},
    {"savetbl", L_obj_savetbl},
    {"loadtbl", L_obj_loadtbl},
    {"tprint", L_obj_tprint},
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

static const struct luaL_reg AREA_lib [] =
{
    {"echo", L_area_echo},
    {"loadprog", L_area_loadprog},
    {"loadscript", L_area_loadscript},
    {"savetbl", L_area_savetbl},
    {"loadtbl", L_area_loadtbl},
    {"tprint", L_area_tprint},
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

static int AREA2string (lua_State *LS)
{
    lua_pushliteral (LS, "mud_area");
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
    FLDSTR("otype", item_name(ud_objp->item_type));
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
    FLDSTR("otype", item_name(ud_obj->item_type));
    FLDNUM("weight", ud_obj->weight);

    if ( !strcmp(argument, "proto" ) )
    {
        if ( !ud_obj->pIndexData )
            return 0;

        make_ud_table(LS, ud_obj->pIndexData, UDTYPE_OBJPROTO, TRUE);
        return 1;
    }

    if ( !strcmp(argument, "contents") )
    {
        int index=1;
        lua_newtable(LS);
        OBJ_DATA *obj;
        for (obj=ud_obj->contains ; obj ; obj=obj->next_content)
        {
            make_ud_table(LS, obj, UDTYPE_OBJ, TRUE);
            lua_rawseti(LS, -2, index++);
        }
        return 1;
    }

    if (!strcmp(argument, "room") )
    {
        if (!ud_obj->in_room)
            return 0;

        make_ud_table(LS, ud_obj->in_room, UDTYPE_ROOM, TRUE);
        return 1;
    }

    if (!strcmp(argument, "inobj") )
    {
        if (!ud_obj->in_obj)
            return 0;

        make_ud_table(LS, ud_obj->in_obj, UDTYPE_OBJ, TRUE);
        return 1;
    }

    if (!strcmp(argument, "carriedby") )
    {
        if (!ud_obj->carried_by )
            return 0;

        make_ud_table(LS, ud_obj->carried_by, UDTYPE_CH, TRUE);
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

static int check_AREA_equal( lua_State *LS)
{
    lua_pushboolean( LS, check_AREA(LS, 1) == check_AREA(LS, 2) );
    return 1;
}

static int get_AREA_field ( lua_State *LS )
{
    const char *argument = luaL_checkstring (LS, 2 );

    FLDNUM("UDTYPE",UDTYPE_AREA); /* Need this for type checking */

    AREA_DATA *ud_area = check_AREA(LS, 1);

    if ( !ud_area )
        return 0;

    FLDSTR("name", ud_area->name);
	FLDSTR("filename", ud_area->file_name);
	FLDNUM("nplayer", ud_area->nplayer);
	FLDNUM("minlevel", ud_area->minlevel);
	FLDNUM("maxlevel", ud_area->maxlevel);
	
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
        make_ud_table( LS, ud_exit->u1.to_room, UDTYPE_ROOM , TRUE);
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
    if ( !strcmp(argument, "area") )
    {
        make_ud_table(LS, ud_room->area, UDTYPE_AREA, TRUE);
        return 1;
    }

    if ( !strcmp(argument, "people") )
    {   
        int index=1;
        lua_newtable(LS);
        CHAR_DATA *people;
        for (people=ud_room->people ; people ; people=people->next_in_room)
        {
            make_ud_table(LS, people, UDTYPE_CH, TRUE);
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
            make_ud_table(LS, ud_room->exit[i], UDTYPE_EXIT, TRUE);
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

    if ( !strcmp(argument, "heshe") )
    {
        if ( ud_ch->sex==SEX_MALE )
        {
            lua_pushstring( LS, "he");
            return 1;
        }
        else if ( ud_ch->sex==SEX_FEMALE )
        {
            lua_pushstring( LS, "she");
            return 1;
        }
        else
        {
            lua_pushstring( LS, "it");
            return 1;
        }
    }

    if ( !strcmp(argument, "himher") )
    {
        if ( ud_ch->sex==SEX_MALE )
        {
            lua_pushstring( LS, "him");
            return 1;
        }
        else if ( ud_ch->sex==SEX_FEMALE )
        {
            lua_pushstring( LS, "her");
            return 1;
        }
        else
        {
            lua_pushstring( LS, "it");
            return 1;
        }
    }

    if ( !strcmp(argument, "hisher") )
    {
        if ( ud_ch->sex==SEX_MALE )
        {
            lua_pushstring( LS, "his");
            return 1;
        }
        else if ( ud_ch->sex==SEX_FEMALE )
        {
            lua_pushstring( LS, "her");
            return 1;
        }
        else
        {
            lua_pushstring( LS, "its");
            return 1;
        }
    }

    if ( !strcmp(argument, "mprogtarget") )
    {
        if (!ud_ch->mprog_target)
            return 0;

        make_ud_table(LS, ud_ch->mprog_target, UDTYPE_CH, TRUE);
        return 1;
    }

    if ( !strcmp(argument, "inventory") )
    {
        int index=1;
        lua_newtable(LS);
        OBJ_DATA *obj;
        for (obj=ud_ch->carrying ; obj ; obj=obj->next_content)
        {
            make_ud_table(LS, obj, UDTYPE_OBJ, TRUE);
            lua_rawseti(LS, -2, index++);
        }
        return 1;
    } 

    if ( !strcmp(argument, "room" ) )
    {
        make_ud_table(LS, ud_ch->in_room, UDTYPE_ROOM, TRUE);
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

static const struct luaL_reg AREA_metatable [] =
{
    {"__tostring", AREA2string},
    {"__index", get_AREA_field},
    {"__newindex", newindex_error},
    {"__eq", check_AREA_equal},
    {NULL, NULL}
};

void RegisterGlobalFunctions(lua_State *LS)
{
    /* checks */
    lua_register(LS,"hour",        L_hour);

    /* other */
    lua_register(LS,"getroom",     L_getroom);
    lua_register(LS,"getobjproto", L_getobjproto);
    lua_register(LS,"getobjworld", L_getobjworld );
    lua_register(LS,"getmobworld", L_getmobworld );
    lua_register(LS,"log",         L_log );
    lua_register(LS,"sendtochar",  L_sendtochar  );
    lua_register(LS,"pagetochar",  L_pagetochar  );
    lua_register(LS,"getcharlist", L_getcharlist);
    lua_register(LS,"getmoblist",  L_getmoblist );
    lua_register(LS,"getplayerlist", L_getplayerlist);
}

static int RegisterLuaRoutines (lua_State *LS)
{
    time_t timer;
    time (&timer);

    /* register all mud.xxx routines */
    luaL_register (LS, MUD_LIBRARY, mudlib);

    luaopen_bits (LS);     /* bit manipulation */
    luaL_register (LS, MT_LIBRARY, mtlib);  /* Mersenne Twister */

    /* Marsenne Twister generator  */
    init_genrand (timer);

    RegisterGlobalFunctions(LS);

    /* meta tables to identify object types */
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
    luaL_newmetatable(LS, AREA_META);
    luaL_register (LS, NULL, AREA_metatable);

    /* our metatable for lightuserdata */
    luaL_newmetatable(LS, UD_META);

    return 0;

}  /* end of RegisterLuaRoutines */

void lua_reset ()
{
    lua_close(g_mud_LS);
    open_lua();
}

void open_lua  ()
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


bool lua_load_mprog( lua_State *LS, int vnum, char *code)
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

bool lua_load_aprog( lua_State *LS, int vnum, char *code)
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

bool lua_load_oprog( lua_State *LS, int vnum, char *code)
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
/* lua_mob_program
   lua equivalent of program_flow
 */
void lua_mob_program( char *text, int pvnum, char *source, 
        CHAR_DATA *mob, CHAR_DATA *ch, 
        const void *arg1, sh_int arg1type, 
        const void *arg2, sh_int arg2type,
        int trig_type ) 
{
    lua_getglobal( g_mud_LS, "mob_program_setup");
    
    make_ud_table( g_mud_LS, mob, UDTYPE_CH, TRUE);
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
    if (ch)
        make_ud_table (g_mud_LS,(void *) ch, UDTYPE_CH, TRUE);
    else lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (text)
        lua_pushstring ( g_mud_LS, text);
    else lua_pushnil(g_mud_LS);

    /* OBJ1_ARG */
    if (arg1type== ACT_ARG_OBJ && arg1)
        make_ud_table( g_mud_LS, arg1, UDTYPE_OBJ, TRUE);
    else lua_pushnil(g_mud_LS);

    /* OBJ2_ARG */
    if (arg2type== ACT_ARG_OBJ && arg2)
        make_ud_table( g_mud_LS, arg2, UDTYPE_OBJ, TRUE);
    else lua_pushnil(g_mud_LS);

    /* TEXT1_ARG */
    if (arg1type== ACT_ARG_TEXT && arg1)
        lua_pushstring ( g_mud_LS, (char *)arg1);
    else lua_pushnil(g_mud_LS);

    /* TEXT2_ARG */
    if (arg2type== ACT_ARG_TEXT && arg2)
        lua_pushstring ( g_mud_LS, (char *)arg2);
    else lua_pushnil(g_mud_LS);

    /* VICTIM_ARG */
    if (arg2type== ACT_ARG_CHARACTER)
        make_ud_table( g_mud_LS, arg2, UDTYPE_CH, TRUE);
    else lua_pushnil(g_mud_LS);

    /* TRIGTYPE_ARG */
    lua_pushstring ( g_mud_LS, flag_stat_string( mprog_flags, trig_type) );


    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=s_LuaScriptInProgress;
    if ( !nest )
    {
        s_LoopCheckCounter=0;
        s_LuaScriptInProgress=TRUE;
    }

    error=CallLuaWithTraceBack (g_mud_LS, NUM_MPROG_ARGS, LUA_MULTRET) ;
    if (error > 0 )
    {
        bugf ( "LUA mprog error for %s(%d), mprog %d:\n %s",
                mob->name,
                mob->pIndexData ? mob->pIndexData->vnum : 0,
                pvnum,
                lua_tostring(g_mud_LS, -1));
    }

    if ( !nest )
        s_LuaScriptInProgress=FALSE;
    lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
}


bool lua_obj_program( char *trigger, int pvnum, char *source, 
        OBJ_DATA *obj, OBJ_DATA *obj2,CHAR_DATA *ch1, CHAR_DATA *ch2,
        int trig_type ) 
{
    bool result=FALSE;

    lua_getglobal( g_mud_LS, "obj_program_setup");
    
    make_ud_table( g_mud_LS, obj, UDTYPE_OBJ, TRUE);
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
    if (obj2)
        make_ud_table (g_mud_LS,(void *) obj2, UDTYPE_OBJ, TRUE);
    else lua_pushnil(g_mud_LS);
    
    /* CH1_ARG */
    if (ch1)
        make_ud_table (g_mud_LS,(void *) ch1, UDTYPE_CH, TRUE);
    else lua_pushnil(g_mud_LS);

    /* CH2_ARG */
    if (ch2)
        make_ud_table (g_mud_LS,(void *) ch2, UDTYPE_CH, TRUE);
    else lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (trigger)
        lua_pushstring(g_mud_LS,trigger);
    else lua_pushnil(g_mud_LS);

    /* TRIGTYPE_ARG */
    lua_pushstring ( g_mud_LS, flag_stat_string( oprog_flags, trig_type) );

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=s_LuaScriptInProgress;
    if ( !nest )
    {
        s_LoopCheckCounter=0;
        s_LuaScriptInProgress=TRUE;
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
        s_LuaScriptInProgress=FALSE;
    lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
    return result;
}

bool lua_area_program( char *trigger, int pvnum, char *source, 
        AREA_DATA *area, CHAR_DATA *ch1,
        int trig_type ) 
{
    bool result=FALSE;

    lua_getglobal( g_mud_LS, "area_program_setup");
    
    make_ud_table( g_mud_LS, area, UDTYPE_AREA, TRUE);
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
    if (ch1)
        make_ud_table (g_mud_LS,(void *) ch1, UDTYPE_CH, TRUE);
    else lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (trigger)
        lua_pushstring(g_mud_LS,trigger);
    else lua_pushnil(g_mud_LS);

    /* TRIGTYPE_ARG */
    lua_pushstring ( g_mud_LS, flag_stat_string( aprog_flags, trig_type) );

    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=s_LuaScriptInProgress;
    if ( !nest )
    {
        s_LoopCheckCounter=0;
        s_LuaScriptInProgress=TRUE;
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
        s_LuaScriptInProgress=FALSE;
    lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
    return result;
}


void do_lboard( CHAR_DATA *ch, char *argument)
{
    lua_getglobal(g_mud_LS, "do_lboard");
    make_ud_table(g_mud_LS, ch, UDTYPE_CH, TRUE);
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
    make_ud_table(g_mud_LS, ch, UDTYPE_CH, TRUE);
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
