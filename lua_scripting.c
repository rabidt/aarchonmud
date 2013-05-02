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
#define OBJECT_META "object.meta"
#define MUD_LIBRARY "mud"
#define MT_LIBRARY "mt"

// number of items in an array
#define NUMITEMS(arg) (sizeof (arg) / sizeof (arg [0]))

/* void RegisterLuaCommands (lua_State *LS); */ /* Implemented in lua_commands.c */
LUALIB_API int luaopen_bits(lua_State *LS);  /* Implemented in lua_bits.c */
static OBJ_DATA *obj1=NULL;
static OBJ_DATA *obj2=NULL;


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

static int check_vnum (lua_State *LS)
{

    int vnum = luaL_checknumber (LS, 1);
    //if ( vnum < 1 || vnum > MAX_VNUM )
    //luaL_error (LS, "Vnum %d is out of range 1 to %d", vnum, MAX_VNUM);
    /* TBC */
    return vnum;
}  /* end of check_vnum */

/* given a character pointer makes a userdata for it */
static void make_char_ud (lua_State *LS, CHAR_DATA * ch)  
{
    if (!ch)
        luaL_error (LS, "make_char_ud called with NULL character");

    lua_pushlightuserdata(LS, (void *)ch);    /* push value */
    luaL_getmetatable (LS, CHARACTER_META);
    lua_setmetatable (LS, -2);  /* set metatable for userdata */

    /* returns with userdata on the stack */
}
/*
static void make_obj_ud (lua_State *LS, OBJ_DATA * obj)
{
    if (!obj)
        luaL_error (LS, "make_obj_ud called with NULL object");

    lua_pushlightuserdata( LS, (void *)obj);
    luaL_getmetatable (LS, OBJECT_META);
    lua_setmetatable (LS, -2);
}
#define check_object(LS, arg) \
    (OBJ_DATA *) luaL_checkudata (LS, arg, OBJECT_META)
    */
#define check_character(LS, arg)  \
    (CHAR_DATA *) luaL_checkudata (LS, arg, CHARACTER_META)
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

static ROOM_INDEX_DATA * L_find_room (lua_State *LS, const int nArg)
{

    /* first, they might have actually specified the room vnum */
    if (lua_isnumber (LS, nArg))
    {
        int vnum = luaL_checknumber (LS, nArg);
        /*if ( vnum < 1 || vnum > MAX_VNUM )
          luaL_error (LS, "Room vnum %d is out of range 1 to %d", vnum, MAX_VNUM);
         */
        /* TBC */
        return get_room_index (vnum);
    }  

    /* if not, deduce from character's current room */ 

    return L_getchar (LS)->in_room;

}  /* end of L_find_room */

/*

   find a character by: userdata / mob vnum / character name

   Possibilities are:

   (char_userdata, boolean)   --> search room/world for userdata, based on room
   of current character

   (char_userdata, room_vnum) --> search room for userdata, in room vnum

   (mob_vnum, boolean)        --> search room/world for mob vnum, based on room
   of current character

   (mob_vnum, room_vnum)      --> search room for mob vnum, in room vnum

   (name)                     --> search room for named player, then world


 */


static CHAR_DATA * L_find_character (lua_State *LS)
{
    CHAR_DATA * ch = NULL;
    ROOM_INDEX_DATA * room = NULL;
    const char * name;
    CHAR_DATA *wch = NULL;
    int iArg1Type = lua_type (LS, 1);
    int iArg2Type = lua_type (LS, 2);

    /* characters/mobs can be specified by character userdata */
    if (iArg1Type == LUA_TLIGHTUSERDATA)
    {
        CHAR_DATA * ud_ch = check_character (LS, 1);

        /* 2nd argument is number - must be room number to look in */

        if (iArg2Type == LUA_TNUMBER)
            room = L_find_room (LS, 2); 
        else
        {  /* otherwise, look in character's room */
            ch = L_getchar (LS);
            room = ch->in_room; 

            /* before reconnected event, we aren't in a room, but our character is still valid */
            /* also, self is a special, quick case */    
            if (ud_ch == ch)
                return ch;
        }

        if (!room)
            luaL_error (LS, "No current room");

        /* for safety, make sure pointer is still valid */
        for ( wch = room->people; wch; wch = wch->next_in_room )
            if (wch == ud_ch)
                return ud_ch; 

        /* if 2nd argument is true, try the world */
        if (iArg2Type == LUA_TBOOLEAN && lua_toboolean (LS, 2))
        {
            for( wch = char_list; wch; wch = wch->next ) 
                if (wch == ud_ch)
                    return ud_ch; 
        }  /* end 2nd argument is true */

        return NULL;  /* pointer no longer valid */
    }

    /* given a number, assume a mob vnum */
    if (iArg1Type == LUA_TNUMBER)
    {
        int vnum = luaL_checknumber (LS, 1);

        /* 2nd argument is number - must be room number to look in */

        if (iArg2Type == LUA_TNUMBER)
            room = L_find_room (LS, 2); 
        else
        {  /* otherwise, look in character's room */
            ch = L_getchar (LS);
            room = ch->in_room; 
        }

        if (!room)
            luaL_error (LS, "No current room");

        /*
         * check the room for an exact match 
         */
        for( wch = room->people; wch; wch = wch->next_in_room )
        {
            if ( IS_NPC( wch ) && wch->pIndexData->vnum == vnum )
                return wch;
        }  /* end of for loop */

        /* if 2nd argument is true, try the world */

        if (iArg2Type == LUA_TBOOLEAN && lua_toboolean (LS, 2))
        {
            for( wch = char_list; wch; wch = wch->next ) 
            {
                if ( IS_NPC( wch ) && wch->pIndexData->vnum == vnum )
                    return wch;
            }  /* end of for loop */
        }  /* end 2nd argument is true */

        return NULL;  /* can't find it */   
    }  /* end of numeric mob wanted */

    /* not number or userdata, must want someone by name */

    ch = L_getchar (LS);
    room = ch->in_room; 

    name = luaL_optstring (LS, 1, "self");

    if (strcasecmp (name, "self") == 0)
        return ch;

    /*
     * check the room for an exact match 
     */
    for( wch = room->people; wch; wch = wch->next_in_room )
        if(!IS_NPC( wch ) && 
                ( strcasecmp ( name, wch->name ) == 0 ))
            break;  /* found it! */

    /*
     * check the world for an exact match 
     */
    if (!wch)
    {
        for( wch = char_list; wch; wch = wch->next )
            if(!IS_NPC( wch ) &&
                    ( strcasecmp( name, wch->name ) == 0 ) )
                break;        
    }  /* end of checking entire world */   

    return wch;  /* use target character, not self */

}  /* end of L_find_character */


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

/*static*/ int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn)
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

    INFO_STR_ITEM (PLAYER_DIR);
    INFO_STR_ITEM (CLAN_DIR);
    INFO_STR_ITEM (LUA_DIR);
    INFO_STR_ITEM (AREA_LIST);
    INFO_STR_ITEM (RESERVED_LIST);
    INFO_STR_ITEM (CLAN_LIST);
    INFO_STR_ITEM (SHUTDOWN_FILE);
    INFO_STR_ITEM (SKILL_FILE);
    INFO_STR_ITEM (LUA_STARTUP);
    INFO_STR_ITEM (LUA_MUD_STARTUP);

    /* levels */
    INFO_NUM_ITEM (MAX_LEVEL);
    INFO_NUM_ITEM (MAX_CLAN);
    INFO_NUM_ITEM (LEVEL_HERO);
    INFO_NUM_ITEM (LEVEL_IMMORTAL);

    /* other numbers */

    INFO_NUM_ITEM (PULSE_PER_SECOND);
    INFO_NUM_ITEM (PULSE_VIOLENCE	);
    INFO_NUM_ITEM (PULSE_MOBILE		);
    INFO_NUM_ITEM (PULSE_TICK		  );
    INFO_NUM_ITEM (PULSE_AREA			);

    return 1;  /* the table itself */
}  /* end of L_system_info */

#define CH_STR_ITEM(arg) \
    if (ch->arg)  \
{   \
    lua_pushstring (LS, ch->arg);  \
    lua_setfield (LS, -2, #arg); \
}

#define CH_NUM_ITEM(arg) \
    lua_pushnumber (LS, ch->arg);  \
lua_setfield (LS, -2, #arg)

#define PC_STR_ITEM(arg) \
    if (pc->arg)  \
{   \
    lua_pushstring (LS, pc->arg);  \
    lua_setfield (LS, -2, #arg); \
}

#define PC_NUM_ITEM(arg) \
    lua_pushnumber (LS, pc->arg);  \
lua_setfield (LS, -2, #arg)
#define OBJ_STR_ITEM(arg) \
    if (obj->arg)  \
{   \
    lua_pushstring (LS, obj->arg);  \
    lua_setfield (LS, -2, #arg); \
}

#define OBJ_NUM_ITEM(arg) \
    lua_pushnumber (LS, obj->arg);  \
lua_setfield (LS, -2, #arg)


static void build_inventory (lua_State *LS, OBJ_DATA * obj );

static void add_object_item (lua_State *LS, OBJ_DATA * obj, const int item)
{
    int i;
    lua_newtable(LS);  /* table for the info */

    OBJ_STR_ITEM (name);
    OBJ_STR_ITEM (short_descr);
    OBJ_STR_ITEM (description);
    /*  OBJ_STR_ITEM (action_desc);

        OBJ_NUM_ITEM (item_type);
        OBJ_NUM_ITEM (magic_flags);  
        OBJ_NUM_ITEM (wear_flags);
        OBJ_NUM_ITEM (wear_loc);
        OBJ_NUM_ITEM (weight);
        OBJ_NUM_ITEM (cost);
        OBJ_NUM_ITEM (level);
        OBJ_NUM_ITEM (timer);
        OBJ_NUM_ITEM (count);    
        OBJ_NUM_ITEM (serial);   
        OBJ_NUM_ITEM (room_vnum);

        if (obj->pIndexData)
        {
        lua_pushnumber (LS, obj->pIndexData->vnum); 
        lua_setfield (LS, -2, "vnum");
        }
     */  
    lua_newtable(LS);  /* table for the values */

    /* do 6 values */
    for (i = 0; i < 6; i++)
    {
        lua_pushnumber (LS, obj->value [i]); 
        lua_rawseti (LS, -2, i + 1);       
    }
    lua_setfield (LS, -2, "value");    

    if( obj->contains )   /* node has a child? */
    {
        lua_newtable(LS);  /* table for the inventory */
        build_inventory(LS,  obj->contains );
        lua_setfield (LS, -2, "contents");
    }
    lua_rawseti (LS, -2, item);         


} /* end of add_object_item */

/* do one inventory level */

static void build_inventory (lua_State *LS, OBJ_DATA * obj )
{
    int item = 1;

    for ( ; obj; obj = obj->next_content)
    {

        /* if carrying it, add to table */

        if( obj->wear_loc == WEAR_NONE)
            add_object_item (LS, obj, item++);

    }  /* end of for loop */

}  /* end of build_inventory */

static int L_inventory (lua_State *LS)
{
    CHAR_DATA * ch = L_find_character (LS);

    if (!ch)
        return 0;

    lua_newtable(LS);  /* table for the inventory */

    /* recursively build inventory */

    build_inventory (LS, ch->carrying);

    return 1;  /* the table itself */
}  /* end of L_inventory */


static int L_equipped (lua_State *LS)
{
    int item = 1;
    CHAR_DATA * ch = L_find_character (LS);
    OBJ_DATA * obj;

    if (!ch)
        return 0;

    lua_newtable(LS);  /* table for the inventory */

    for (obj = ch->carrying ; obj; obj = obj->next_content)
    {
        if (ch == obj->carried_by && 
                obj->wear_loc > WEAR_NONE)
            add_object_item (LS, obj, item++);
    }  /* end of for loop */

    return 1;  /* the table itself */
}  /* end of L_equipped */

static bool check_inventory (CHAR_DATA * ch, OBJ_DATA * obj, const int vnum )
{
    /* check all siblings */   
    for ( ; obj; obj = obj->next_content)
    {
        if( obj->wear_loc == WEAR_NONE)
        {
            if (obj->pIndexData && obj->pIndexData->vnum == vnum)
                return TRUE;

        }  /* end of carrying it */

        if( obj->contains )   /* node has a child? */
        {
            if (check_inventory(ch, obj->contains, vnum ))
                return TRUE;
        }

    }  /* end of for loop */

    return FALSE;
}  /* end of check_inventory */

#define MOB_STR_ITEM(arg) \
    if (mob->arg)  \
{   \
    lua_pushstring (LS, mob->arg);  \
    lua_setfield (LS, -2, #arg); \
}

#define MOB_NUM_ITEM(arg) \
    lua_pushnumber (LS, mob->arg);  \
lua_setfield (LS, -2, #arg)

static int L_mob_info (lua_State *LS)
{
    MOB_INDEX_DATA *mob = get_mob_index( check_vnum (LS) );
    if (!mob)
        return 0;

    lua_newtable(LS);  /* table for the info */

    /* strings */

    MOB_STR_ITEM (player_name);
    MOB_STR_ITEM (short_descr);
    MOB_STR_ITEM (long_descr);
    MOB_STR_ITEM (description);
    /*  MOB_STR_ITEM (spec_funname);

        MOB_NUM_ITEM (vnum);
        MOB_NUM_ITEM (count);
        MOB_NUM_ITEM (killed);
        MOB_NUM_ITEM (sex);
        MOB_NUM_ITEM (level);
        MOB_NUM_ITEM (alignment);
        MOB_NUM_ITEM (mobthac0);
        MOB_NUM_ITEM (ac);
        MOB_NUM_ITEM (hitnodice);
        MOB_NUM_ITEM (hitsizedice);
        MOB_NUM_ITEM (hitplus);
        MOB_NUM_ITEM (damnodice);
        MOB_NUM_ITEM (damsizedice);
        MOB_NUM_ITEM (damplus);
        MOB_NUM_ITEM (numattacks);
        MOB_NUM_ITEM (gold);
        MOB_NUM_ITEM (exp);
        MOB_NUM_ITEM (xflags);
        MOB_NUM_ITEM (immune);
        MOB_NUM_ITEM (resistant);
        MOB_NUM_ITEM (susceptible);
        MOB_NUM_ITEM (speaks);
        MOB_NUM_ITEM (speaking);
        MOB_NUM_ITEM (position);
        MOB_NUM_ITEM (defposition);
        MOB_NUM_ITEM (height);
        MOB_NUM_ITEM (weight);
        MOB_NUM_ITEM (race);
        MOB_NUM_ITEM (Class);
        MOB_NUM_ITEM (hitroll);
        MOB_NUM_ITEM (damroll);
        MOB_NUM_ITEM (perm_str);
        MOB_NUM_ITEM (perm_int);
        MOB_NUM_ITEM (perm_wis);
        MOB_NUM_ITEM (perm_dex);
        MOB_NUM_ITEM (perm_con);
        MOB_NUM_ITEM (perm_cha);
        MOB_NUM_ITEM (perm_lck);
        MOB_NUM_ITEM (saving_poison_death);
        MOB_NUM_ITEM (saving_wand);
        MOB_NUM_ITEM (saving_para_petri);
        MOB_NUM_ITEM (saving_breath);
        MOB_NUM_ITEM (saving_spell_staff);
     */
    return 1;  /* the table itself */ 
}  /* end of L_mob_info */


static int L_object_info (lua_State *LS)
{
    OBJ_INDEX_DATA *obj = get_obj_index ( check_vnum (LS) );
    int i;

    if (!obj)
        return 0;

    lua_newtable(LS);  /* table for the info */

    /* strings */

    OBJ_STR_ITEM (name);
    OBJ_STR_ITEM (short_descr);
    OBJ_STR_ITEM (description);
    /* OBJ_STR_ITEM (action_desc);

       OBJ_NUM_ITEM (vnum);
       OBJ_NUM_ITEM (level);
       OBJ_NUM_ITEM (item_type);
       OBJ_NUM_ITEM (magic_flags);  
       OBJ_NUM_ITEM (wear_flags);
       OBJ_NUM_ITEM (count);
       OBJ_NUM_ITEM (weight);
       OBJ_NUM_ITEM (cost);
       OBJ_NUM_ITEM (serial);
       OBJ_NUM_ITEM (layers);
     */
    lua_newtable(LS);  /* table for the values */

    /* do 6 values */
    for (i = 0; i < 6; i++)
    {
        lua_pushnumber (LS, obj->value [i]); 
        lua_rawseti (LS, -2, i + 1);       
    }
    lua_setfield (LS, -2, "value");    


    return 1;  /* the table itself */ 
}  /* end of L_object_info */

static int L_object_name (lua_State *LS)
{
    OBJ_INDEX_DATA *obj = get_obj_index ( check_vnum (LS) );

    if (!obj)
        return 0;  

    lua_pushstring (LS, obj->short_descr);
    lua_pushstring (LS, obj->description);

    return 2;
} /* end of L_object_name */

static int L_mob_name (lua_State *LS)
{
    MOB_INDEX_DATA *mob = get_mob_index ( check_vnum (LS) );

    if (!mob)
        return 0;  

    lua_pushstring (LS, mob->short_descr);
    lua_pushstring (LS, mob->description);

    return 2;
} /* end of L_mob_name */

#define AREA_STR_ITEM(arg) \
    if (area->arg)  \
{   \
    lua_pushstring (LS, area->arg);  \
    lua_setfield (LS, -2, #arg); \
}

#define AREA_NUM_ITEM(arg) \
    lua_pushnumber (LS, area->arg);  \
lua_setfield (LS, -2, #arg)


static int L_area_info (lua_State *LS)
{
    const char * name = luaL_checkstring (LS, 1); /* area file name */

    AREA_DATA * area;

    for ( area = area_first; area; area = area->next )
        if (strcasecmp (area->file_name, name) == 0)
            break;

    if (area == NULL) 
        return 0;  /* oops - area does not exist */

    lua_newtable(LS);  /* table for the info */

    /* strings */

    AREA_STR_ITEM (name);
    AREA_STR_ITEM (file_name);
    AREA_STR_ITEM (credits);  

    /* numbers */

    //AREA_NUM_ITEM (area_flags);
    AREA_NUM_ITEM (age);
    AREA_NUM_ITEM (nplayer);
    AREA_NUM_ITEM (reset_time);
    AREA_NUM_ITEM (min_vnum);
    AREA_NUM_ITEM (max_vnum);
    AREA_NUM_ITEM (minlevel);
    AREA_NUM_ITEM (maxlevel);
    return 1;  /* the table itself */
}  /* end of L_area_info */

static int L_area_list (lua_State *LS)
{
    AREA_DATA * area;
    int count = 1;

    lua_newtable(LS);  /* table for the info */

    for ( area = area_first; area; area = area->next )
    {
        lua_pushstring (LS, area->file_name);  
        lua_rawseti (LS, -2, count++);  
    }

    return 1;  /* the table itself */
}  /* end of L_area_list */

#define ROOM_STR_ITEM(arg) \
    if (room->arg)  \
{   \
    lua_pushstring (LS, room->arg);  \
    lua_setfield (LS, -2, #arg); \
}

#define ROOM_NUM_ITEM(arg) \
    lua_pushnumber (LS, room->arg);  \
lua_setfield (LS, -2, #arg)

#define EXIT_STR_ITEM(arg) \
    if (pexit->arg)  \
{   \
    lua_pushstring (LS, pexit->arg);  \
    lua_setfield (LS, -2, #arg); \
}

#define  EXIT_NUM_ITEM(arg) \
    lua_pushnumber (LS, pexit->arg);  \
lua_setfield (LS, -2, #arg)

#define CONTENTS_STR_ITEM(arg) \
    if (obj->arg)  \
{   \
    lua_pushstring (LS, obj->arg);  \
    lua_setfield (LS, -2, #arg); \
}

#define CONTENTS_NUM_ITEM(arg) \
    lua_pushnumber (LS, obj->arg);  \
lua_setfield (LS, -2, #arg)

static int L_room_info (lua_State *LS)
{
    ROOM_INDEX_DATA * room = L_find_room (LS, 1);  /* which room */
    EXIT_DATA * pexit;
    OBJ_DATA * obj;
    int item = 1;

    if (room == NULL) 
        return 0;  /* oops - not in room or specified room does not exist */

    lua_newtable(LS);  /* table for the info */

    /* strings */

    ROOM_STR_ITEM (name);
    ROOM_STR_ITEM (description);

    /* numbers */

    ROOM_NUM_ITEM (vnum);
    //ROOM_NUM_ITEM (room_flags);
    ROOM_NUM_ITEM (light);   
    ROOM_NUM_ITEM (sector_type);

    lua_newtable(LS);  /* table for all exits */

    //for( pexit = room->first_exit; pexit; pexit = pexit->next )
    int i;
    for ( i=0 ; i < 10; i++ )
    {
        lua_newtable(LS);  /* table for the info */

        EXIT_STR_ITEM (keyword);    
        EXIT_STR_ITEM (description);

        //EXIT_NUM_ITEM (vnum);       
        //EXIT_NUM_ITEM (rvnum);      
        //EXIT_NUM_ITEM (exit_info);  
        EXIT_NUM_ITEM (key);        

        // key is exit name
        lua_setfield (LS, -2, dir_name [i]) ;    

    }
    lua_setfield (LS, -2, "exits");

    /* now do the contents */

    lua_newtable(LS);  /* table for all objects */

    for( obj = room->contents; obj; obj = obj->next_content )
        add_object_item (LS, obj, item++);

    lua_setfield (LS, -2, "contents"); 

    return 1;  /* the table itself */
}  /* end of L_room_info */

static int L_room_name (lua_State *LS)
{
    ROOM_INDEX_DATA * room = L_find_room (LS, 1);  /* which room */

    if (room == NULL) 
        return 0;  /* oops - not in room or specified room does not exist */

    lua_pushstring (LS, room->name);
    lua_pushstring (LS, room->description);
    return 2;
} /* end of L_room_name */

static int L_room_exits (lua_State *LS)
{
    ROOM_INDEX_DATA * room = L_find_room (LS, 1);  /* which room */
    EXIT_DATA * pexit;
    int iExceptFlag = luaL_optnumber (LS, 2, 0);  /* eg. except locked */

    if (room == NULL) 
        return 0;  /*  specified room does not exist */

    /* now do the exits */

    lua_newtable(LS);  /* table for all exits */

    int i;
    //for( pexit = room->first_exit; pexit; pexit = pexit->next )
    for( i=0 ; i < MAX_DIR ; i++ )
    {
        pexit=room->exit[i];
        if (  IS_SET(pexit->exit_info, EX_LOCKED) )
            continue;  /* ignore, eg. locked exit */

        // key is exit name, value is exit direction
        lua_pushnumber (LS, pexit->u1.to_room->vnum);
        lua_setfield (LS, -2, dir_name [i]) ;    
    }  /* end for loop */

    return 1;  /* the table itself */

}  /* end of L_room_exits */


/* act as if character had typed the command */
static int L_interpret (lua_State *LS)
{
    char command [MAX_INPUT_LENGTH];  
    strncpy (command, luaL_checkstring (LS, 1), sizeof (command));  
    command [sizeof (command) - 1] = 0;  

    interpret (L_getchar (LS), command);
    return 0;
}  /* end of L_interpret */

/* force any character, or a mob in this room, to do something */
static int L_force (lua_State *LS)
{
    CHAR_DATA * ch = L_find_character (LS);
    int iCommand = 2;

    if (!ch)
        luaL_error (LS, "Cannot find character/mob to force.");

    /* if character was (vnum, boolean) then command is 3rd argument */
    if ((lua_type (LS, 1) == LUA_TNUMBER || lua_type (LS, 1) == LUA_TLIGHTUSERDATA)
            && (lua_type (LS, 2) == LUA_TBOOLEAN || lua_type (LS, 2) == LUA_TNUMBER))
        iCommand++;

    char command [MAX_INPUT_LENGTH];  
    strncpy (command, luaL_checkstring (LS, iCommand), sizeof (command));  
    command [sizeof (command) - 1] = 0;  

    interpret (ch, command);
    return 0;
}  /* end of L_force */

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


/* transfer character or mob to somewhere (default is here) */
static int L_transfer (lua_State *LS)
{
    CHAR_DATA * ch = L_getchar (LS);
    CHAR_DATA * victim = L_find_character (LS);
    int iDestination = 2;
    ROOM_INDEX_DATA *location;
    int vnum;

    /* if character was (vnum, boolean) then room is 3rd argument */
    if ((lua_type (LS, 1) == LUA_TNUMBER || lua_type (LS, 1) == LUA_TLIGHTUSERDATA)
            && (lua_type (LS, 2) == LUA_TBOOLEAN || lua_type (LS, 2) == LUA_TNUMBER))
        iDestination++;

    vnum = luaL_optnumber (LS, iDestination, ch->in_room->vnum);

    //if ( vnum < 1 || vnum > MAX_VNUM )
    //luaL_error (LS, "Room vnum %d is out of range 1 to %d", vnum, MAX_VNUM);

    if (!victim)
        luaL_error (LS, "Cannot find character/mob to transfer.");

    location = get_room_index(vnum );

    if (!location)
        luaL_error (LS, "Cannot find destination room: %d", vnum);

    if (victim->fighting)
        stop_fighting( victim, TRUE );

    char_from_room( victim );
    char_to_room( victim, location );

    return 0;
} /* end of L_transfer */

/* stop the nominated character fighting */
static int L_stop_fighting (lua_State *LS)
{
    CHAR_DATA * ch = L_find_character (LS);

    if (!ch)
        luaL_error (LS, "Cannot find character/mob to stop fighting.");

    if (ch->fighting)
        stop_fighting( ch, TRUE );

    return 0;
} /* end of L_stop_fighting */

/* is the nominated character fighting? */
static int L_fighting (lua_State *LS)
{
    CHAR_DATA * ch = L_find_character (LS);

    if (!ch)
        luaL_error (LS, "Cannot find character/mob.");

    lua_pushboolean (LS, ch->fighting != NULL);

    return 1;
} /* end of L_fighting */


static int L_gain_exp (lua_State *LS)
{
    CHAR_DATA * ch = L_getchar (LS); /* get character pointer */
    gain_exp (ch, luaL_checknumber (LS, 1));
    lua_pushnumber (LS, ch->exp);
    return 1;  /* return new amount */
}  /* end of L_gain_exp */

static int L_gain_gold (lua_State *LS)
{
    CHAR_DATA * ch = L_getchar (LS); /* get character pointer */
    ch->gold += luaL_checknumber (LS, 1);
    lua_pushnumber (LS, ch->gold);
    return 1;
}  /* end of L_gain_gold */

static int L_char_name (lua_State *LS)
{

    CHAR_DATA * ch = L_find_character (LS); /* get character pointer */

    if (!ch)
        return 0;

    lua_pushstring (LS, ch->name); 
    return 1;
}  /* end of L_char_name */

static int L_npc (lua_State *LS)
{

    CHAR_DATA * ch = L_find_character (LS); /* get character pointer */

    if (!ch)
        return 0;

    lua_pushboolean (LS, IS_NPC (ch)); 
    return 1;
}  /* end of L_npc */

static int L_immortal (lua_State *LS)
{

    CHAR_DATA * ch = L_find_character (LS); /* get character pointer */

    if (!ch)
        return 0;

    lua_pushboolean (LS, IS_IMMORTAL (ch)); 
    return 1;
}  /* end of L_immortal */

static int L_char_exists (lua_State *LS)
{

    CHAR_DATA * ch = L_find_character (LS); /* get character pointer */

    lua_pushboolean (LS, ch != NULL);

    return 1;
}  /* end of L_char_exists */

static int L_cmd_asound (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpasound( ud_ch, argument);

    return 1; 
}

static int L_cmd_gecho (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpgecho( ud_ch, argument);

    return 1;
}

static int L_cmd_zecho (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpzecho( ud_ch, argument);

    return 1;
}

static int L_cmd_kill (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    CHAR_DATA * ud_vic = check_character (LS, 2);

    if ( !ud_ch || !ud_vic )
    {
        bugf("NULL pointer in L_kill.");
        return;
    }
    if ( ud_ch->in_room != ud_vic->in_room )
    {
        bugf("Trying to assist char not in room.");
        return;
    }

    if ( IS_AFFECTED( ud_ch, AFF_CHARM ) && ud_ch->master == ud_vic )
    {
        bug( "MpKill - Charmed mob attacking master from vnum %d.",
                IS_NPC(ud_ch) ? ud_ch->pIndexData->vnum : 0 );
        return;
    }

    multi_hit( ud_ch, ud_vic, TYPE_UNDEFINED );

    return 1;
}

static int L_cmd_assist (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    CHAR_DATA * ud_vic = check_character (LS, 2);

    if ( !ud_ch || !ud_vic )
    {
        bugf("NULL pointer in L_assist.");
        return;
    }
    if ( ud_ch->in_room != ud_vic->in_room )
    {
        bugf("Trying to assist char not in room.");
        return;
    }

    if ( ud_vic == ud_ch || ud_ch->fighting != NULL || ud_vic->fighting == NULL )
        return;

    multi_hit( ud_ch, ud_vic->fighting, TYPE_UNDEFINED );

    return 1;
}

static int L_cmd_junk (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpjunk( ud_ch, argument);

    return 1;
}

static int L_cmd_echo (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpecho( ud_ch, argument);

    return 1;
}

static int L_cmd_echoaround (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpechoaround( ud_ch, argument);

    return 1;
}

static int L_cmd_echoat (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpechoat( ud_ch, argument);

    return 1;
}

static int L_cmd_mload (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpmload( ud_ch, argument);

    return 1;
}

static int L_cmd_oload (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpoload( ud_ch, argument);

    return 1;
}

static int L_cmd_purge (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mppurge( ud_ch, argument);

    return 1;
}

static int L_cmd_goto (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpgoto( ud_ch, argument);

    return 1;
}

static int L_cmd_at (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpat( ud_ch, argument);

    return 1;
}

static int L_cmd_transfer (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mptransfer( ud_ch, argument);

    return 1;
}

static int L_cmd_gtransfer (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpgtransfer( ud_ch, argument);

    return 1;
}

static int L_cmd_otransfer (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpotransfer( ud_ch, argument);

    return 1;
}

static int L_cmd_force (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpotransfer( ud_ch, argument);

    return 1;
}


static int L_cmd_gforce (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpgforce( ud_ch, argument);

    return 1;
}

static int L_cmd_vforce (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpvforce( ud_ch, argument);

    return 1;
}

static int L_cmd_cast (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpcast( ud_ch, argument);

    return 1;
}

static int L_cmd_damage (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpdamage( ud_ch, argument);

    return 1;
}

static int L_cmd_remember (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpremember( ud_ch, argument);

    return 1;
}

static int L_cmd_forget (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpforget( ud_ch, argument);

    return 1;
}

static int L_cmd_delay (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpdelay( ud_ch, argument);

    return 1;
}

static int L_cmd_cancel (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpcancel( ud_ch, argument);

    return 1;
}

static int L_cmd_call (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpcall( ud_ch, argument);

    return 1;
}

static int L_cmd_flee (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpflee( ud_ch, argument);

    return 1;
}

static int L_cmd_remove (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpremove( ud_ch, argument);

    return 1;
}

static int L_cmd_remort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpremort( ud_ch, argument);

    return 1;
}

static int L_cmd_qset (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpqset( ud_ch, argument);

    return 1;
}

static int L_cmd_qadvance (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpqadvance( ud_ch, argument);

    return 1;
}

static int L_cmd_reward (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpreward( ud_ch, argument);

    return 1;
}

static int L_cmd_peace (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mppeace( ud_ch, argument);

    return 1;
}

static int L_cmd_restore (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mprestore( ud_ch, argument);

    return 1;
}

static int L_cmd_act (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);
    do_mpact( ud_ch, argument);

    return 1;
}

static int L_cmd_hit (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    CHAR_DATA * ud_vic = check_character (LS, 2);

    if ( !ud_ch || !ud_vic )
    {
        bugf("NULL pointer in L_hit.");
        return;
    }

    one_hit( ud_ch, ud_vic, TYPE_UNDEFINED, FALSE );
    //const char *argument = luaL_checkstring (LS, 2);
    //do_mphit( ud_ch, argument);

    return 1;

}

static int L_cmd_exec (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    interpret( ud_ch, argument);

    return 1;
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

static int L_race (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && ud_ch->race == race_lookup( argument ) );

    return 1;
}

static int L_class (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && ud_ch->class == class_lookup( argument ) );

    return 1;
}

static int L_objtype (lua_State *LS)
{/* TBC
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && ud_ch->class == class_lookup( argument ) );

    return 1;
  */}

static int L_vnum (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushnumber( LS,  ( ud_ch != NULL && IS_NPC(ud_ch) ) ? ud_ch->pIndexData->vnum : 0 );

    return 1;
}

static int L_hpcnt (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    lua_pushnumber( LS,  ( ud_ch != NULL ) ? ((ud_ch->hit * 100)/(UMAX(1,ud_ch->max_hit))) : 0 );

    return 1;
}

static int L_room (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL && ud_ch->in_room != NULL )
        lua_pushnumber( LS, ud_ch->in_room->vnum);
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_sex (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, ud_ch->sex);
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_level (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, ud_ch->level);
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_align (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, ud_ch->alignment);
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_money (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, (ud_ch->gold * 100) + ud_ch->silver);
    else
        lua_pushnumber( LS, 0);

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

static int L_clanrank (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && !IS_NPC(ud_ch) && ud_ch->pcdata->clan_rank == clan_rank_lookup(ud_ch->clan, argument ) );

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

static int L_statstr (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_STR) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statcon (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_CON) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statvit (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_VIT) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statagi (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_AGI) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statdex (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_DEX) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statint (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_INT) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statwis (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_WIS) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statdis (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_DIS) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statcha (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_CHA) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int L_statluc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, get_curr_stat(ud_ch, STAT_LUC) );
    else
        lua_pushnumber( LS, 0);

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
    //{"character_info", L_character_info},
    {"area_info", L_area_info},
    {"area_list", L_area_list},
    {"mob_info", L_mob_info},
    {"room_info", L_room_info},
    {"room_name", L_room_name},
    {"room_exits", L_room_exits},
    {"object_info", L_object_info},  /* object prototype */
    {"inventory", L_inventory},      /* invoked objects */
    {"equipped", L_equipped},        /* invoked objects */
    {"object_name", L_object_name},   /* short, long object name */
    {"mob_name", L_mob_name},   /* short, long mob name */
    //{"level", L_level},                /* what is my level? */
    //{"race", L_race},                  /* what is my race? */
    //{"class", L_class},                /* what is my class? */
    //{"room", L_room},                  /* what room am I in? */
    //{"char_name", L_char_name},        /* what is my name? */
    //{"char_exists", L_char_exists},    /* does character exist (now)? */

    /* do stuff to the character or others */

    //{"interpret", L_interpret},        /* interpret command, as if we typed it */
    //{"force", L_force},                /* force another character or mob to do something */
    //{"transfer", L_transfer},          /* transfer arg1 to arg2 location */

    /* alter character state */

    {"gain_exp", L_gain_exp},         /* give character xp */
    {"gain_gold", L_gain_gold},       /* give character gold */

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

static OBJ_DATA *find_pointer( char *id)
{
    /* TBC laterzzzz */
    if ( !strcmp(id, "obj1") )
        return obj1;
    if ( !strcmp(id, "obj2") )
        return obj2;
    

    return NULL;
}

static int get_object_field ( lua_State *LS )
{
    lua_getfield(LS, 1, "tableid") ;
    OBJ_DATA * ud_obj = find_pointer(luaL_checkstring (LS, 3 ));
    const char *argument = luaL_checkstring (LS, 2 );

    if ( !ud_obj )
        return 0;

    if ( !strcmp( argument, "name" ) )
    {
        lua_pushstring( LS, ud_obj->name );
        return 1;
    }

    return 0;
}

#define CHSTR(key,value) \
    if ( !strcmp( argument, key ) ) \
{lua_pushstring( LS, value ); return 1;} 
#define CHNUM(key,value) \
    if ( !strcmp( argument, key ) ) \
{lua_pushnumber( LS, value ); return 1;}
static int get_character_field ( lua_State *LS)
{
    CHAR_DATA * ud_ch = check_character (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    if ( !ud_ch)
        return 0;

    CHSTR("name", ud_ch->name);
    CHNUM("level", ud_ch->level);
    CHNUM("hp", ud_ch->hit);
    CHNUM("maxhp", ud_ch->max_hit);
    CHNUM("mana", ud_ch->mana);
    CHNUM("maxmana", ud_ch->max_mana);
    CHNUM("move", ud_ch->move);
    CHNUM("maxmove", ud_ch->max_move);
    CHNUM("gold", ud_ch->gold);
    CHNUM("silver", ud_ch->silver);
    CHNUM("money", (ud_ch->silver + ud_ch->gold * 100 ) );
    CHSTR("sex", ( sex_table[ud_ch->sex].name ) );
    CHSTR("size", ( size_table[ud_ch->size].name ) );
    CHSTR("position", ( position_table[ud_ch->position].short_name ) );
    CHNUM("align", ud_ch->alignment);
    CHNUM("str", get_curr_stat( ud_ch, STAT_STR ) );
    CHNUM("con", get_curr_stat( ud_ch, STAT_CON ) );
    CHNUM("vit", get_curr_stat( ud_ch, STAT_VIT ) );
    CHNUM("agi", get_curr_stat( ud_ch, STAT_AGI ) );
    CHNUM("dex", get_curr_stat( ud_ch, STAT_DEX ) );
    CHNUM("int", get_curr_stat( ud_ch, STAT_INT ) );
    CHNUM("wis", get_curr_stat( ud_ch, STAT_WIS ) );
    CHNUM("dis", get_curr_stat( ud_ch, STAT_DIS ) );
    CHNUM("cha", get_curr_stat( ud_ch, STAT_CHA ) );
    CHNUM("luc", get_curr_stat( ud_ch, STAT_LUC ) );
    CHSTR("clan", clan_table[ud_ch->clan].name );
    CHSTR("class", IS_NPC(ud_ch) ? "mobile" : class_table[ud_ch->class].name );
    CHNUM("room", ud_ch->in_room->vnum );
    CHNUM("groupsize", count_people_room( ud_ch, 4 ) );
    if ( !IS_NPC(ud_ch) )
    {
        CHNUM("clanrank", ud_ch->pcdata->clan_rank );
        CHNUM("remorts", ud_ch->pcdata->remorts);
        CHNUM("explored", ud_ch->pcdata->explored->set);
        CHNUM("beheads", ud_ch->pcdata->behead_cnt);
        CHNUM("pkills", ud_ch->pcdata->pkill_count);
        CHNUM("pkdeaths", ud_ch->pcdata->pkill_deaths);
        CHNUM("questpoints", ud_ch->pcdata->questpoints );
        CHNUM("bank", ud_ch->pcdata->bank );
        CHNUM("mobkills", ud_ch->pcdata->mob_kills);
        CHNUM("mobdeaths", ud_ch->pcdata->mob_deaths);
    }


    return 0;

}

static const struct luaL_reg object_meta [] =
{
    //{"__tostring", obj2string},
    {"__index", get_object_field},
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
    /* special cmd for executing normal commands */
    {"exec",        L_cmd_exec},
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
    //{"exists",          L_exists }, //never actually worked in original
    {"affected",          L_affected },
    {"act",          L_act },
    {"off",          L_off },
    {"imm",          L_imm },
    {"carries",          L_carries },
    {"wears",          L_wears },
    {"has",          L_has },
    {"uses",          L_uses },
    {"name",          L_name },
    {"pos",          L_pos },
    {"clan",          L_clan },
    {"race",          L_race },
    {"class",          L_class },
    {"objtype",          L_objtype },
    {"vnum",          L_vnum },
    {"hpcnt",          L_hpcnt },
    {"room",          L_room },
    {"sex",          L_sex },
    {"level",          L_level },
    {"align",          L_align },
    {"money",          L_money },
    /*{"objval0",          L_objval0 }, TBC */
    {"grpsize",          L_grpsize },
    {"clanrank",          L_clanrank },
    {"qstatus",          L_qstatus },
    {"vuln",          L_vuln },
    {"res",          L_res },
    {"statstr",          L_statstr},
    {"statcon",          L_statcon},
    {"statvit",          L_statvit},
    {"statagi",          L_statagi},
    {"statdex",          L_statdex},
    {"statint",          L_statint},
    {"statwis",          L_statwis},
    {"statdis",          L_statdis},
    {"statcha",          L_statcha},
    {"statluc",          L_statluc},
    {"religion",          L_religion },
    {"skilled",          L_skilled },
    {"ccarries",          L_ccarries },
    {"qtimer",          L_qtimer },
    {"mpcnt",          L_mpcnt },
    {"remort",          L_remort },
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

    /* meta table to identify character types */
    luaL_newmetatable(LS, CHARACTER_META);
    luaL_register (LS, NULL, character_meta);  /* give us a __tostring function */

    luaL_newmetatable(LS, OBJECT_META);
    luaL_register (LS, NULL, object_meta);

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

    lua_getglobal (LS, MUD_LIBRARY);
    make_char_ud (LS, ch);
    lua_setfield (LS, -2, "self");
    lua_pop (LS, 1);  /* pop mud table */    


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
    make_char_ud (mob->LS, mob);
    lua_setglobal(mob->LS, "mob");
    if (ch)
    {
        make_char_ud (mob->LS, ch);
        lua_setglobal(mob->LS, "ch");
    }
    else
    {
        luaL_loadstring( mob->LS, "ch=nil");
        CallLuaWithTraceBack (mob->LS, 0, 0);
    }

    if (text)
    {
        lua_pushstring ( mob->LS, text );
        lua_setglobal(mob->LS, "text");
    }
    else
    {
        luaL_loadstring( mob->LS, "text=nil");
        CallLuaWithTraceBack (mob->LS, 0, 0);
    }

    if (arg1type== ACT_ARG_OBJ)
    {
        obj1= (OBJ_DATA *) arg1;
        lua_newtable( mob->LS);
        luaL_getmetatable (mob->LS, OBJECT_META);           
        lua_setmetatable (mob->LS, -2);  /* set metatable for object data */
        lua_pushstring( mob->LS, "obj1"); // give it a name field
        lua_setfield( mob->LS, -2, "tableid" );
        lua_setglobal( mob->LS, "obj1");
    }/*
    else
    {
        luaL_loadstring( mob->LS, "obj1=nil");
        CallLuaWithTraceBack (mob->LS, 0, 0);
    }
    if (arg2type== ACT_ARG_OBJ)
    {
        make_obj_ud (mob->LS, (OBJ_DATA *)arg2);
        lua_setglobal( mob->LS, "obj2");
    }
    else
    {
        luaL_loadstring( mob->LS, "obj2=nil");
        CallLuaWithTraceBack (mob->LS, 0, 0);
    }

    /*
       if (arg1)
       {
       make_obj_ud (mob->LS, ch);
       lua_setglobal(mob->LS, "obj1");
       }
       if (arg2)
       {
       make_obj_ud (mob->LS, ch);
       lua_setglobal(mob->LS, "obj2");
       }
     */
    if (luaL_loadstring (mob->LS, source) ||
            CallLuaWithTraceBack (mob->LS, 0, 0))
    {
        bugf ( "LUA mprog error for vnum %d:\n %s",
                pvnum,
                lua_tostring(mob->LS, -1));
    }

    lua_settop (mob->LS, 0);    /* get rid of stuff lying around */
}
