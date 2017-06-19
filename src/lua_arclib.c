#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <lualib.h>
#include <lauxlib.h>
#include <sqlite3.h>
#include "merc.h"
#include "lua_arclib.h"
#include "lua_main.h"
#include "olc.h"
#include "tables.h"
#include "mudconfig.h"
#include "religion.h"
#include "mob_cmds.h"
#include "interp.h"


static void register_arclib_type( LUA_OBJ_TYPE *type, lua_State *LS );

//#define LUA_TEST
struct lua_prop_type;
struct lua_obj_type
{
    const char * const type_name;

    const char * const C_type_name;
    const size_t C_struct_size;

    const struct lua_prop_type * const get_table;
    const struct lua_prop_type * const set_table;
    const struct lua_prop_type * const method_table;

    int count;
};


/* Type declarations */
static LUA_OBJ_TYPE CH_type;
static LUA_OBJ_TYPE OBJ_type;
static LUA_OBJ_TYPE AREA_type;
static LUA_OBJ_TYPE ROOM_type;
static LUA_OBJ_TYPE EXIT_type;
static LUA_OBJ_TYPE RESET_type;
static LUA_OBJ_TYPE OBJPROTO_type;
static LUA_OBJ_TYPE MOBPROTO_type;
static LUA_OBJ_TYPE SHOP_type;
static LUA_OBJ_TYPE AFFECT_type;
static LUA_OBJ_TYPE PROG_type;
static LUA_OBJ_TYPE MTRIG_type;
static LUA_OBJ_TYPE OTRIG_type;
static LUA_OBJ_TYPE ATRIG_type;
static LUA_OBJ_TYPE RTRIG_type;
static LUA_OBJ_TYPE HELP_type;
static LUA_OBJ_TYPE DESCRIPTOR_type;
static LUA_OBJ_TYPE BOSSACHV_type;
static LUA_OBJ_TYPE BOSSREC_type;

LUA_OBJ_TYPE * const p_CH_type = &CH_type;
LUA_OBJ_TYPE * const p_OBJ_type = &OBJ_type;
LUA_OBJ_TYPE * const p_AREA_type = &AREA_type;
LUA_OBJ_TYPE * const p_ROOM_type = &ROOM_type;
LUA_OBJ_TYPE * const p_EXIT_type = &EXIT_type;
LUA_OBJ_TYPE * const p_RESET_type = &RESET_type;
LUA_OBJ_TYPE * const p_OBJPROTO_type = &OBJPROTO_type;
LUA_OBJ_TYPE * const p_MOBPROTO_type = &MOBPROTO_type;
LUA_OBJ_TYPE * const p_SHOP_type = &SHOP_type;
LUA_OBJ_TYPE * const p_AFFECT_type = &AFFECT_type;
LUA_OBJ_TYPE * const p_PROG_type = &PROG_type;
LUA_OBJ_TYPE * const p_MTRIG_type = &MTRIG_type;
LUA_OBJ_TYPE * const p_OTRIG_type = &OTRIG_type;
LUA_OBJ_TYPE * const p_ATRIG_type = &ATRIG_type;
LUA_OBJ_TYPE * const p_RTRIG_type = &RTRIG_type;
LUA_OBJ_TYPE * const p_HELP_type = &HELP_type;
LUA_OBJ_TYPE * const p_DESCRIPTOR_type = &DESCRIPTOR_type;
LUA_OBJ_TYPE * const p_BOSSACHV_type = &BOSSACHV_type;
LUA_OBJ_TYPE * const p_BOSSREC_type = &BOSSREC_type;


/* for iterating */
LUA_OBJ_TYPE * const type_list [] =
{
    &CH_type,
    &OBJ_type,
    &AREA_type,
    &ROOM_type,
    &EXIT_type,
    &RESET_type,
    &OBJPROTO_type,
    &MOBPROTO_type,
    &SHOP_type,
    &AFFECT_type,
    &PROG_type,
    &MTRIG_type,
    &OTRIG_type,
    &ATRIG_type,
    &RTRIG_type,
    &HELP_type,
    &DESCRIPTOR_type,
    &BOSSACHV_type,
    &BOSSREC_type,
    NULL
};

/* Define game object types and global functions */


typedef struct lua_prop_type
{
    const char *field;
    int  (*func)( lua_State * );
    int security;
    int status; 
} LUA_PROP_TYPE;

#define STS_ACTIVE     0
#define STS_DEPRECATED 1

/* global section */
typedef struct glob_type
{
    const char *lib;
    const char *name;
    int (*func)( lua_State * );
    int security; /* if SEC_NOSCRIPT then not available in prog scripts */
    int status;
} GLOB_TYPE;

static struct glob_type glob_table[];

static int utillib_func (lua_State *LS, const char *funcname)
{
    int narg=lua_gettop(LS);
    lua_getglobal( LS, "glob_util");
    lua_getfield( LS, -1, funcname);
    lua_remove( LS, -2 );
    lua_insert( LS, 1 );
    lua_call( LS, narg, LUA_MULTRET );

    return lua_gettop(LS);
}

static int utillib_trim (lua_State *LS )
{
    return utillib_func( LS, "trim");
}

static int utillib_convert_time (lua_State *LS )
{
    return utillib_func( LS, "convert_time");
}

static int utillib_capitalize( lua_State *LS )
{
    return utillib_func( LS, "capitalize");
}

static int utillib_pluralize( lua_State *LS )
{
    return utillib_func( LS, "pluralize");
}

static int utillib_format_list( lua_State *LS )
{
    return utillib_func( LS, "format_list");
}

static int utillib_strlen_color( lua_State *LS )
{
   lua_pushinteger( LS,
           strlen_color( luaL_checkstring( LS, 1) ) );
   return 1;
}

static int utillib_truncate_color_string( lua_State *LS )
{
    lua_pushstring( LS,
            truncate_color_string( 
                check_string( LS, 1, MSL),
                luaL_checkinteger( LS, 2 )
            ) 
    );
    return 1;
}

static int utillib_format_color_string( lua_State *LS )
{
    lua_pushstring( LS,
            format_color_string(
                check_string( LS, 1, MSL),
                luaL_checkinteger( LS, 2 )
            )
    );
    return 1;
}
            
static int godlib_helper_get_duration(lua_State* LS, int index)
{
    if (lua_isnone(LS, index))
    {
        return GOD_FUNC_DEFAULT_DURATION;
    }
    else
    {
        return luaL_checkinteger(LS, index);
    }
}

static int godlib_bless (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_bless( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_curse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_curse( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_heal (lua_State *LS)
{

    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_heal( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_speed (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_speed( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1; 
}

static int godlib_slow (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_slow( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1; 
}

static int godlib_cleanse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_cleanse( NULL, ch, "", GOD_FUNC_DEFAULT_DURATION ));
    return 1; 
}

static int godlib_defy (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_defy( NULL, ch, "", GOD_FUNC_DEFAULT_DURATION ));
    return 1; 
}

static int godlib_enlighten (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_enlighten( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1; 
}

static int godlib_protect (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_protect( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_fortune (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_fortune( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_haunt (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_haunt( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_plague (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_plague( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_confuse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    

    lua_pushboolean( LS,
            god_confuse( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int glob_transfer (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *arg=check_string(LS,2,MIL);
    ROOM_INDEX_DATA *location=find_mp_location( ch, arg); 

    if (!location)
    {
        lua_pushboolean( LS, FALSE);
        return 1;
    }
    
    
    bool result=transfer_char( ch, location );
    lua_pushboolean( LS, result);
    return 1;
}

static int glob_gtransfer (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *arg=check_string(LS,2,MIL);
    ROOM_INDEX_DATA *location=find_location( ch, arg);

    if (!location)
    {
        lua_pushboolean( LS, FALSE);
        return 1;
    }

    bool all_success=TRUE;
    CHAR_DATA *victim, *next_char;
    
    if (!ch->in_room)
    {
        return luaL_error( LS, "'gtransfer' called but target '%s' has no in_room", ch->name);
    }

    for ( victim=ch->in_room->people; victim; victim=next_char )
    {
        next_char=victim->next_in_room;
        if ( is_same_group( ch, victim ) )
        {
            if (!transfer_char(victim, location))
                all_success=FALSE;
        }
    }

    lua_pushboolean( LS, all_success);
    return 1;
}

static int glob_gecho (lua_State *LS)
{
    DESCRIPTOR_DATA *d;
    const char *argument=check_fstring( LS, 1, MIL );

    for ( d=descriptor_list; d; d=d->next )
    {
        if ( IS_PLAYING(d->connected ) )
        {
            if ( IS_IMMORTAL(d->character) )
                send_to_char( "gecho> ", d->character );
            send_to_char( argument, d->character );
            send_to_char( "\n\r", d->character );
        }
    }

    return 0;
}

static int glob_sendtochar (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *msg=check_fstring( LS, 2, MSL);

    send_to_char(msg, ch);
    return 0;
}

static int glob_echoat (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *msg=check_fstring( LS, 2, MSL);

    send_to_char(msg, ch);
    send_to_char("\n\r",ch);
    return 0;
}

static int glob_echoaround (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *msg=check_fstring( LS, 2, MSL);

    CHAR_DATA *tochar, *next_char;

    for ( tochar=ch->in_room->people; tochar ; tochar=next_char )
    {
        next_char=tochar->next_in_room;
        if ( tochar == ch )
            continue;

        send_to_char( msg, tochar );
        send_to_char( "\n\r", tochar);
    }

    return 0;
}

static int glob_dammessage (lua_State *LS)
{
    const char *vs;
    const char *vp;
    char punct;
    char punctstr[2];
    get_damage_messages( 
            luaL_checkinteger( LS, 1 ),
            0, &vs, &vp, &punct );

    punctstr[0]=punct;
    punctstr[1]='\0';

    lua_pushstring( LS, vs );
    lua_pushstring( LS, vp );
    lua_pushstring( LS, punctstr);

    return 3;
}

static int glob_do_luaquery ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "do_luaquery");
    lua_insert(LS, 1);
    lua_call( LS, top, LUA_MULTRET );

    return lua_gettop(LS);
}

static void push_mudconfig_val( lua_State *LS, const CFG_DATA_ENTRY *en )
{
    switch( en->type )
    {
        case CFG_INT:
            {
                lua_pushinteger( LS, *((int *)(en->value)));
                break;
            }
        case CFG_FLOAT:
            {
                lua_pushnumber( LS, *((float *)(en->value)));
                break;
            }
        case CFG_STRING:
            {
                lua_pushstring( LS, *((char **)(en->value)));
                break;
            }
        case CFG_BOOL:
            {
                lua_pushboolean( LS, *((bool *)(en->value)));
                break;
            }
        default:
            {
                luaL_error( LS, "Bad type.");
            }
    }
}

static int glob_mudconfig (lua_State *LS)
{
    int i;
    const CFG_DATA_ENTRY *en;

    /* no arg, return the whole table */
    if (lua_isnone(LS,1))
    {
        lua_newtable(LS);
        for ( i=0 ; mudconfig_table[i].name ; i++ )
        {
            en=&mudconfig_table[i];
            push_mudconfig_val( LS, en );
            lua_setfield(LS, -2, en->name);
        }
        return 1;
    }

    const char *arg1=check_string(LS, 1, MIL);
    /* 1 argument, return the value */
    if (lua_isnone(LS, 2))
    {
        for ( i=0 ; mudconfig_table[i].name ; i++ )
        {
            en=&mudconfig_table[i];
            if (!strcmp(en->name, arg1))
            {
                push_mudconfig_val( LS, en );
                return 1;
            }
        }
        luaL_error(LS, "No such mudconfig value: %s", arg1);
    }

    /* 2 args, set the value */
    for ( i=0 ; mudconfig_table[i].name ; i++ )
    {
        en=&mudconfig_table[i];
        if (!strcmp(en->name, arg1))
        {
            switch( en->type )
            {
                case CFG_INT:
                    {
                        *((int *)(en->value))=luaL_checkinteger( LS, 2 );
                        break;
                    }
                case CFG_FLOAT:
                    {
                        *((float *)(en->value))=luaL_checknumber(LS, 2 );
                        break;
                    }
                case CFG_STRING:
                    {
                        const char *newval=check_string(LS, 2, MIL);
                        *((const char **)(en->value))=str_dup(newval);
                        break;
                    }
                case CFG_BOOL:
                    {
                        luaL_checktype( LS, 2, LUA_TBOOLEAN );
                        *((bool *)(en->value))=lua_toboolean(LS, 2 );
                        break;
                    }
                default:
                    {
                        luaL_error( LS, "Bad type.");
                    }
            }
            return 0;
        }
    }
    luaL_error(LS, "No such mudconfig value: %s", arg1);

    return 0;
}

static int glob_start_con_handler( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "start_con_handler");
    lua_insert( LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}
#ifdef LUA_TEST
static int glob_runglobal(lua_State *LS)
{
    luaL_checktype(LS, 1, LUA_TFUNCTION);
    lua_pushvalue(LS, LUA_GLOBALSINDEX);
    if (lua_setfenv(LS, 1) == 0)
    {
       return luaL_error(LS, "Couldn't setfenv.");
    }
    lua_call(LS, 0, 0);

    return 0;
}
#endif

static int glob_getglobals (lua_State *LS)
{
    int i;
    int index=1;
    lua_newtable( LS );

    for ( i=0 ; glob_table[i].name ; i++ )
    {
        if ( glob_table[i].status == STS_ACTIVE )
        {
            lua_newtable( LS );
            
            if (glob_table[i].lib)
            {
                lua_pushstring( LS, glob_table[i].lib );
                lua_setfield( LS, -2, "lib" );
            }

            lua_pushstring( LS, glob_table[i].name );
            lua_setfield( LS, -2, "name" );

            lua_pushinteger( LS, glob_table[i].security );
            lua_setfield( LS, -2, "security" );

            lua_rawseti( LS, -2, index++ );
        }
    }
    return 1;
}

static int glob_forceget (lua_State *LS)
{
    lua_getmetatable( LS, 1);
    lua_getfield( LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );
    lua_pop( LS, 2 );
    const char *arg=check_string( LS, 2, MIL);
    lua_remove( LS, 2 );

    
    const LUA_PROP_TYPE *get=type->get_table;
    int i;

    for (i=0 ; get[i].field ; i++ )
    {
        if ( !strcmp(get[i].field, arg) )
        {
            return (get[i].func)(LS);
        }
    }

    luaL_error( LS, "Can't get field '%s' for type %s.", arg, type->type_name);
    return 0;
}

static int glob_forceset (lua_State *LS)
{
    lua_getmetatable( LS, 1);
    lua_getfield( LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );
    lua_pop( LS, 2 );
    const char *arg=check_string( LS, 2, MIL);
    lua_remove( LS, 2 );

    
    const LUA_PROP_TYPE *set=type->set_table;
    int i;

    for (i=0 ; set[i].field ; i++ )
    {
        if ( !strcmp(set[i].field, arg) )
        {
            lua_pushcfunction( LS, set[i].func );
            lua_insert( LS, 1 );
            lua_call(LS, 2, 0);
            return 0;
        }
    }

    luaL_error( LS, "Can't set field '%s' for type %s.", arg, type->type_name);
    return 0;
}

static int glob_getluatype (lua_State *LS)
{
    if ( lua_isnone( LS, 1 ) )
    {
        /* Send a list of types */
        lua_newtable( LS );
        int i;
        int index=1;
        for ( i=0 ; type_list[i] ; i++ )
        {
            lua_pushstring( LS, type_list[i]->type_name );
            lua_rawseti( LS, -2, index++ );
        }
        return 1;
    }

    const char *arg1=luaL_checkstring( LS, 1 );
    int i;

    LUA_OBJ_TYPE *tp=NULL;
    for ( i=0 ; type_list[i] ; i++ )
    {
        if (!str_cmp( arg1, type_list[i]->type_name ) )
        {
            tp=type_list[i];
        }
    } 

    if (!tp)
        return 0;

    lua_newtable( LS ); /* main table */

    int index=1;
    lua_newtable( LS ); /* get table */
    for ( i=0 ; tp->get_table[i].field ; i++ )
    {
        if ( tp->get_table[i].status == STS_ACTIVE )
        {
            lua_newtable( LS ); /* get entry */
            lua_pushstring( LS, tp->get_table[i].field );
            lua_setfield( LS, -2, "field" );
            lua_pushinteger( LS, tp->get_table[i].security );
            lua_setfield( LS, -2, "security" );
            
            lua_rawseti( LS, -2, index++ );
        }
    }
    lua_setfield( LS, -2, "get" );


    index=1;
    lua_newtable( LS ); /* set table */
    for ( i=0 ; tp->set_table[i].field ; i++ )
    {
        if ( tp->set_table[i].status == STS_ACTIVE )
        {
            lua_newtable( LS ); /* get entry */
            lua_pushstring( LS, tp->set_table[i].field );
            lua_setfield( LS, -2, "field" );
            lua_pushinteger( LS, tp->set_table[i].security );
            lua_setfield( LS, -2, "security" );

            lua_rawseti( LS, -2, index++ );
        }
    }
    lua_setfield( LS, -2, "set" );

    index=1;
    lua_newtable( LS ); /* method table */
    for ( i=0 ; tp->method_table[i].field ; i++ )
    {
        if ( tp->method_table[i].status == STS_ACTIVE )
        {
            lua_newtable( LS ); /* get entry */
            lua_pushstring( LS, tp->method_table[i].field );
            lua_setfield( LS, -2, "field" );
            lua_pushinteger( LS, tp->method_table[i].security );
            lua_setfield( LS, -2, "security" );
            
            lua_rawseti( LS, -2, index++ );
        }
    }
    lua_setfield( LS, -2, "method" );

    return 1;
}

static int glob_clearloopcount (lua_State *LS)
{
    g_LoopCheckCounter=0;
    return 0;
}

static int glob_log (lua_State *LS)
{
    const char *msg = check_string(LS, 1, MIL);
    const char *chan=NULL;
    int wiznet_chan = -1;
    if (!lua_isnone(LS, 2))
    {
        chan = check_string(LS, 2, MIL);
        int i;
        for (i=0; wiznet_table[i].name; i++)
        {
            if (!strcmp(wiznet_table[i].name, chan))
            {
                wiznet_chan = wiznet_table[i].flag;
                break;
            }
        }

        if (wiznet_chan == -1)
        {
            return luaL_error(LS, "No such wiznet channel: %s", chan);
        }
    }

    char buf[MSL];
    sprintf(buf, "LUA::%s:%s", chan ? chan : "",  msg);
    log_string(buf);

    if (wiznet_chan != -1)
    {
        wiznet(buf, NULL, NULL, wiznet_chan, 0, 0);
    }

    return 0;
}

static int glob_hour (lua_State *LS)
{
    lua_pushnumber( LS, time_info.hour );
    return 1;
}

static int glob_gettime (lua_State *LS)
{
    char buf[MSL];
    struct timeval t;
    gettimeofday( &t, NULL);

    sprintf(buf, "%ld.%ld", (long)t.tv_sec, (long)t.tv_usec);
    lua_pushstring( LS, buf);
   
    lua_pushnumber( LS, lua_tonumber( LS, -1 ) );
    return 1;
}

static int glob_getroom (lua_State *LS)
{
    // do some if is number thing here eventually
    int num = (int)luaL_checknumber (LS, 1);

    ROOM_INDEX_DATA *room=get_room_index(num);

    if (!room)
        return 0;

    if ( !push_ROOM( LS, room) )
        return 0;
    else
        return 1;

}

static int glob_getobjproto (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    OBJ_INDEX_DATA *obj=get_obj_index(num);

    if (!obj)
        return 0;

    if ( !push_OBJPROTO( LS, obj) )
        return 0;
    else
        return 1;
}

static int glob_getobjworld (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    OBJ_DATA *obj;

    int index=1;
    lua_newtable(LS);
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( obj->pIndexData->vnum == num
                && !obj->must_extract )
        {
            if (push_OBJ( LS, obj))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int glob_getmobproto (lua_State *LS)
{
    int num = luaL_checknumber (LS, 1);

    MOB_INDEX_DATA *mob=get_mob_index(num);

    if (!mob)
        return 0;

    if ( !push_MOBPROTO( LS, mob) )
        return 0;
    else
        return 1;
}

static int glob_getmobworld (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);
    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if ( ch->pIndexData 
                && ch->pIndexData->vnum == num
                && !ch->must_extract  )
        {
            if (push_CH( LS, ch))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int glob_getpc (lua_State *LS)
{
    const char *name=check_string (LS, 1, MIL );
    
    CHAR_DATA *ch;
    for (ch=char_list; ch; ch=ch->next)
    {
        if (IS_NPC(ch))
            continue;

        if (!str_cmp(name, ch->name))
        {
            push_CH(LS, ch);
            return 1;
        }
    }

    return 0;
}

static int glob_pagetochar (lua_State *LS)
{
    if (!lua_isnone(LS, 3) )
    {
        page_to_char_new( 
                luaL_checkstring(LS, 2),
                check_CH(LS,1),
                lua_toboolean( LS, 3 ) );
        return 0;
    }
    else
    {
        page_to_char( 
                luaL_checkstring( LS, 2),
                check_CH(LS,1) );
    }

    return 0;
}

static int glob_getobjlist (lua_State *LS)
{
    OBJ_DATA *obj;

    int index=1;
    lua_newtable(LS);

    for ( obj=object_list ; obj ; obj=obj->next )
    {
        if (push_OBJ(LS, obj))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_getcharlist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if (push_CH(LS, ch))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_getmoblist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if ( IS_NPC(ch) )
        {
            if (push_CH(LS, ch))
                lua_rawseti(LS, -2, index++);
        }
    }

    return 1;
}

static int glob_getdescriptorlist (lua_State *LS)
{
    DESCRIPTOR_DATA *d;

    int index=1;
    lua_newtable(LS);

    for ( d=descriptor_list ; d ; d=d->next )
    {
        if (push_DESCRIPTOR(LS, d))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_getplayerlist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if ( !IS_NPC(ch) )
        {
            if (push_CH(LS, ch))
                lua_rawseti(LS, -2, index++);
        }
    }

    return 1;
}

static int glob_getarealist (lua_State *LS)
{
    AREA_DATA *area;

    int index=1;
    lua_newtable(LS);

    for ( area=area_first ; area ; area=area->next )
    {
        if (push_AREA(LS, area))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_getshoplist ( lua_State *LS)
{
    SHOP_DATA *shop;
    
    int index=1;
    lua_newtable(LS);

    for ( shop=shop_first ; shop ; shop=shop->next )
    {
        if (push_SHOP(LS, shop))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_gethelplist ( lua_State *LS )
{
    HELP_DATA *help;

    int index=1;
    lua_newtable(LS);

    for ( help=help_first ; help ; help=help->next )
    {
        if (push_HELP(LS, help))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}
/* Mersenne Twister pseudo-random number generator */

static int mtlib_srand (lua_State *LS)
{
    size_t i;

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
} /* end of mtlib_srand */

static int mtlib_rand (lua_State *LS)
{
    lua_pushnumber (LS, (double)genrand ());
    return 1;
} /* end of mtlib_rand */

static int mudlib_luadir( lua_State *LS)
{
    lua_pushliteral( LS, LUA_DIR);
    return 1;
}

static int mudlib_userdir( lua_State *LS)
{
    lua_pushliteral( LS, USER_DIR);
    return 1;
}

/* dblib section */
static int dblib_helper( lua_State *LS, const char *field )
{
    int narg=lua_gettop(LS);
    lua_getglobal(LS, "glob_db");
    lua_getfield(LS, -1, field);
    lua_remove(LS, -2);
    lua_insert(LS, 1);
    lua_call(LS, narg, LUA_MULTRET);
    
    return lua_gettop(LS);
}
static int dblib_errcode( lua_State *LS ) { return dblib_helper( LS, "errcode" ); }
static int dblib_errmsg( lua_State *LS ) { return dblib_helper( LS, "errmsg" ); }
static int dblib_exec( lua_State *LS ) { return dblib_helper( LS, "exec" ); }
static int dblib_nrows( lua_State *LS ) { return dblib_helper( LS, "nrows" ); }
static int dblib_prepare( lua_State *LS ) { return dblib_helper( LS, "prepare" ); }
static int dblib_rows( lua_State *LS ) { return dblib_helper( LS, "rows" ); }
static int dblib_urows( lua_State *LS ) { return dblib_helper( LS, "urows" ); }
/* end dblib section */

/* return tprintstr of the given global (string arg)*/
static int dbglib_show ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    lua_getglobal( LS, luaL_checkstring( LS, 1 ) );
    lua_call( LS, 1, 1 );

    return 1;
}

static int glob_awardptitle( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal(LS, "glob_awardptitle");

    lua_insert( LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}

static int glob_randnum ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_randnum");
    lua_insert( LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}

static int glob_rand ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_rand");
    lua_insert( LS, 1 );
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}

static int glob_tprintstr ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_tprintstr");
    lua_insert(LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}

static int glob_getrandomroom ( lua_State *LS)
{
    ROOM_INDEX_DATA *room;

    int i;
    for ( i=0; i<10000; i++ ) // limit to 10k loops just in case
    {
        room=get_room_index(number_range(0,65535));
        if ( ( room )
                &&   is_room_ingame( room )
                &&   !room_is_private(room)
                &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
                &&   !IS_SET(room->room_flags, ROOM_SOLITARY)
                &&   !IS_SET(room->room_flags, ROOM_SAFE)
                &&   !IS_SET(room->room_flags, ROOM_JAIL)
                &&   !IS_SET(room->room_flags, ROOM_NO_TELEPORT) )
            break;
    }

    if (!room)
        luaL_error(LS, "Couldn't get a random room.");

    if (push_ROOM(LS,room))
        return 1;
    else
        return 0;

}

/* currently unused - commented out to avoid warning
static int glob_cancel ( lua_State *LS)
{
    return L_cancel(LS);
}
*/

static int glob_arguments ( lua_State *LS)
{   
    const char *argument=check_string( LS, 1, MIL );
    char buf[MIL];
    bool keepcase=FALSE;

    if (!lua_isnone(LS,2))
    {
        keepcase=lua_toboolean(LS, 2);
    }
    
    lua_newtable( LS );
    int index=1;

    while ( argument[0] != '\0' )
    {
        if (keepcase)
            argument=one_argument_keep_case( argument, buf);
        else
            argument = one_argument( argument, buf );
        lua_pushstring( LS, buf );
        lua_rawseti( LS, -2, index++ );
    }

    return 1;
}
        

static GLOB_TYPE glob_table[] =
{
    { NULL,  "hour",              glob_hour,                         0, STS_ACTIVE },
    { NULL,  "gettime",           glob_gettime,                      0, STS_ACTIVE },
    { NULL,  "getroom",           glob_getroom,                      0, STS_ACTIVE },
    { NULL,  "randnum",           glob_randnum,                      0, STS_ACTIVE },
    { NULL,  "rand",              glob_rand,                         0, STS_ACTIVE },
    { NULL,  "tprintstr",         glob_tprintstr,                    0, STS_ACTIVE },
    { NULL,  "getobjproto",       glob_getobjproto,                  0, STS_ACTIVE },
    { NULL,  "getobjworld",       glob_getobjworld,                  0, STS_ACTIVE },
    { NULL,  "getmobproto",       glob_getmobproto,                  0, STS_ACTIVE },
    { NULL,  "getmobworld",       glob_getmobworld,                  0, STS_ACTIVE },
    { NULL,  "getpc",             glob_getpc,                        0, STS_ACTIVE },
    { NULL,  "getrandomroom",     glob_getrandomroom,                0, STS_ACTIVE },
    { NULL,  "transfer",          glob_transfer,                     0, STS_ACTIVE },
    { NULL,  "gtransfer",         glob_gtransfer,                    0, STS_ACTIVE },
    { NULL,  "awardptitle",       glob_awardptitle,                  5, STS_ACTIVE },
    { NULL,  "sendtochar",        glob_sendtochar,                   0, STS_ACTIVE },
    { NULL,  "echoat",            glob_echoat,                       0, STS_ACTIVE },
    { NULL,  "echoaround",        glob_echoaround,                   0, STS_ACTIVE },
    { NULL,  "gecho",             glob_gecho,                        0, STS_ACTIVE },
    { NULL,  "pagetochar",        glob_pagetochar,                   0, STS_ACTIVE },
    { NULL,  "arguments",         glob_arguments,                    0, STS_ACTIVE },
    { NULL,  "log",               glob_log,                          0, STS_ACTIVE },
    { NULL,  "getcharlist",       glob_getcharlist,                  9, STS_ACTIVE },
    { NULL,  "getobjlist",        glob_getobjlist,                   9, STS_ACTIVE },
    { NULL,  "getmoblist",        glob_getmoblist,                   9, STS_ACTIVE },
    { NULL,  "getplayerlist",     glob_getplayerlist,                9, STS_ACTIVE },
    { NULL,  "getarealist",       glob_getarealist,                  9, STS_ACTIVE },
    { NULL,  "getshoplist",       glob_getshoplist,                  9, STS_ACTIVE },
    { NULL,  "gethelplist",       glob_gethelplist,                  9, STS_ACTIVE },
    { NULL,  "getdescriptorlist", glob_getdescriptorlist,            9, STS_ACTIVE },
    { NULL,  "dammessage",        glob_dammessage,                   0, STS_ACTIVE },
    { NULL,  "clearloopcount",    glob_clearloopcount,               9, STS_ACTIVE },
    { NULL,  "mudconfig",         glob_mudconfig,                    9, STS_ACTIVE },
    { NULL,  "start_con_handler", glob_start_con_handler,            9, STS_ACTIVE },
    { NULL,  "forceget",          glob_forceget,          SEC_NOSCRIPT, STS_ACTIVE },
    { NULL,  "forceset",          glob_forceset,          SEC_NOSCRIPT, STS_ACTIVE },
    { NULL,  "getluatype",        glob_getluatype,        SEC_NOSCRIPT, STS_ACTIVE },
    { NULL,  "getglobals",        glob_getglobals,        SEC_NOSCRIPT, STS_ACTIVE },

#ifdef TESTER
    { NULL,  "do_luaquery",       glob_do_luaquery,                  9, STS_ACTIVE },
#else
    { NULL,  "do_luaquery",       glob_do_luaquery,       SEC_NOSCRIPT, STS_ACTIVE },
#endif
#ifdef LUA_TEST
    { NULL,  "runglobal", glob_runglobal, 9, STS_ACTIVE },
#endif

    { "god", "confuse",   godlib_confuse,   9, STS_ACTIVE },
    { "god", "curse",     godlib_curse,     9, STS_ACTIVE },
    { "god", "plague",    godlib_plague,    9, STS_ACTIVE },
    { "god", "bless",     godlib_bless,     9, STS_ACTIVE },
    { "god", "slow",      godlib_slow,      9, STS_ACTIVE },
    { "god", "speed",     godlib_speed,     9, STS_ACTIVE },
    { "god", "heal",      godlib_heal,      9, STS_ACTIVE },
    { "god", "enlighten", godlib_enlighten, 9, STS_ACTIVE },
    { "god", "protect",   godlib_protect,   9, STS_ACTIVE },
    { "god", "fortune",   godlib_fortune,   9, STS_ACTIVE },
    { "god", "haunt",     godlib_haunt,     9, STS_ACTIVE },
    { "god", "cleanse",   godlib_cleanse,   9, STS_ACTIVE },
    { "god", "defy",      godlib_defy,      9, STS_ACTIVE },

    { "util", "trim",                  utillib_trim,                  0, STS_ACTIVE },
    { "util", "convert_time",          utillib_convert_time,          0, STS_ACTIVE },
    { "util", "capitalize",            utillib_capitalize,            0, STS_ACTIVE },
    { "util", "pluralize",             utillib_pluralize,             0, STS_ACTIVE },
    { "util", "format_list",           utillib_format_list,           0, STS_ACTIVE },
    { "util", "strlen_color",          utillib_strlen_color,          0, STS_ACTIVE },
    { "util", "truncate_color_string", utillib_truncate_color_string, 0, STS_ACTIVE },
    { "util", "format_color_string",   utillib_format_color_string,   0, STS_ACTIVE },
    
    { "db", "errcode", dblib_errcode, 9, STS_ACTIVE },
    { "db", "errmsg",  dblib_errmsg,  9, STS_ACTIVE },
    { "db", "exec",    dblib_exec,    9, STS_ACTIVE },
    { "db", "nrows",   dblib_nrows,   9, STS_ACTIVE },
    { "db", "prepare", dblib_prepare, 9, STS_ACTIVE },
    { "db", "rows",    dblib_rows,    9, STS_ACTIVE },
    { "db", "urows",   dblib_urows,   9, STS_ACTIVE },
    
    { "debug", "show", dbglib_show,   9, STS_ACTIVE },

    /* SEC_NOSCRIPT means aren't available for prog scripts */

    { "mt", "srand", mtlib_srand, SEC_NOSCRIPT, STS_ACTIVE },
    { "mt", "rand",  mtlib_rand,  SEC_NOSCRIPT, STS_ACTIVE },

    { "mud", "luadir",  mudlib_luadir,  SEC_NOSCRIPT, STS_ACTIVE },
    { "mud", "userdir", mudlib_userdir, SEC_NOSCRIPT, STS_ACTIVE },
    
    { NULL, NULL, NULL, 0, 0 }
};

static int global_sec_check (lua_State *LS)
{
    int security=luaL_checkinteger( LS, lua_upvalueindex(1) );
    
    if ( g_ScriptSecurity < security )
        luaL_error( LS, "Current security %d. Function requires %d.",
                g_ScriptSecurity,
                security);

    int (*fun)( lua_State * )=lua_tocfunction( LS, lua_upvalueindex(2) );

    return fun(LS);
}

#define SCRIPT_GLOBS_TABLE "script_globs"
/* Register funcs globally then if needed,
   security closure into script_globs
   to be inserted into main_lib in startup.lua */
/* would be nice to do this in lua but we run it before startup */
void register_globals( lua_State *LS )
{
    //if (1) return;
    int top=lua_gettop(LS); 
    int i;

    /* create script_globs */
    lua_newtable(LS);
    lua_setglobal(LS, SCRIPT_GLOBS_TABLE );

    for ( i=0 ; glob_table[i].name ; i++ )
    {
        /* is it a lib thing? */
        if ( glob_table[i].lib )
        {
            lua_getglobal( LS, glob_table[i].lib );
            if ( lua_isnil( LS, -1 ) )
            {
                lua_pop(LS, 1); /* kill the nil */
                lua_newtable( LS );
                lua_pushvalue( LS, -1 ); /* make a copy cause we poppin it */
                lua_setglobal( LS, glob_table[i].lib );
                
            }
            lua_pushcfunction( LS, glob_table[i].func );
            lua_setfield( LS, -2, glob_table[i].name );
            lua_pop(LS, 1); // kill lib table
        }
        else
        {
            lua_getglobal( LS, glob_table[i].name );
            if (!lua_isnil( LS, -1 ) )
            {
                luaL_error( LS, "Global already exists: %s",
                        glob_table[i].name);
            }
            else
                lua_pop(LS, 1); /* kill the nil */
            
            lua_pushcfunction( LS, glob_table[i].func );
            lua_setglobal( LS, glob_table[i].name );

        }


        if (glob_table[i].security == SEC_NOSCRIPT)
            continue; /* don't add to script_globs */ 

        
        /* get script_globs */
        lua_getglobal( LS, SCRIPT_GLOBS_TABLE );
        if ( glob_table[i].lib )
        {
            lua_getfield( LS, -1, glob_table[i].lib );
            if ( lua_isnil( LS, -1 ) )
            {
                lua_pop(LS, 1); // kill the nil
                lua_newtable(LS);
                lua_pushvalue( LS, -1); //make a copy because
                lua_setfield( LS, -3, glob_table[i].lib );
            }
        }


        /* create the security closure */
        lua_pushinteger( LS, glob_table[i].security );
        lua_pushcfunction( LS, glob_table[i].func );
        lua_pushcclosure( LS, global_sec_check, 2 );
        
        /* set as field to script_globs script_globs.lib */
        lua_setfield( LS, -2, glob_table[i].name );

        lua_settop(LS, top); // clearn junk after each loop
    }

}



/* end global section */


/* common section */
LUA_EXTRA_VAL *new_luaval( int type, const char *name, const char *val, bool persist )
{
    LUA_EXTRA_VAL *new=alloc_mem(sizeof(LUA_EXTRA_VAL));
    new->type=type;
    new->name=name;
    new->val=val;
    new->persist=persist;

    return new;
}

void free_luaval( LUA_EXTRA_VAL *luaval)
{
    free_string(luaval->name);
    free_string(luaval->val);

    free_mem(luaval, sizeof(LUA_EXTRA_VAL) );
}

static void push_luaval( lua_State *LS, LUA_EXTRA_VAL *luaval )
{
    switch(luaval->type)
    {
        case LUA_TSTRING:
            lua_pushstring(LS, luaval->val);
            return;

        case LUA_TNUMBER:
            lua_getglobal( LS, "tonumber");
            lua_pushstring(LS, luaval->val);
            lua_call( LS, 1, 1 );
            return;

        case LUA_TBOOLEAN:
            lua_pushboolean( LS, !strcmp( luaval->val, "true" ) );
            return;

        default:
            luaL_error(LS, "Invalid type '%s'",
                    lua_typename( LS, luaval->type) );
    }
}

static int get_luaval( lua_State *LS, LUA_EXTRA_VAL **luavals )
{
    if (lua_isnone( LS, 1 ) )
    {
        /* no argument, send a table of all vals */
        lua_newtable( LS );
        LUA_EXTRA_VAL *luaval;

        for (luaval=*luavals; luaval; luaval=luaval->next)
        {
            lua_pushstring( LS, luaval->name);
            push_luaval( LS, luaval );
            lua_rawset( LS, -3 );
        }
        return 1;
    }

    const char *name=check_string(LS, 1, MIL );

    LUA_EXTRA_VAL *luaval;

    for ( luaval=*luavals; luaval; luaval=luaval->next )
    {
        if (!strcmp( name, luaval->name) )
        {
            push_luaval( LS, luaval );
            return 1;
        }
    }

    lua_pushnil( LS );
    return 1;
}

static int set_luaval( lua_State *LS, LUA_EXTRA_VAL **luavals )
{
    const char *name=check_string(LS, 1, MIL );
    int type=lua_type(LS, 2 );
    const char *val;
    bool persist=FALSE;
    if (!lua_isnone(LS,3))
    {
        persist=lua_toboolean(LS, 3);
        lua_remove(LS,3);
    }

    switch(type)
    {
        case LUA_TNONE:
        case LUA_TNIL:
            /* just break 
               clear value lower down if it's already set,
               otherwise do nothing 
             */
            val = NULL;
            break;

        case LUA_TSTRING:
        case LUA_TNUMBER:
            val=check_string(LS, 2, MIL );
            break;

        case LUA_TBOOLEAN:
            lua_getglobal( LS, "tostring");
            lua_insert( LS, -2 );
            lua_call( LS, 1, 1 );
            val=check_string( LS, 2, MIL );
            break;

        default:
            return luaL_error( LS, "Cannot set value type '%s'.",
                    lua_typename( LS, type ) );
    }

    LUA_EXTRA_VAL *luaval;
    LUA_EXTRA_VAL *prev=NULL;
    LUA_EXTRA_VAL *luaval_next=NULL;

    for ( luaval=*luavals; luaval; luaval=luaval_next )
    {
        luaval_next=luaval->next;

        if (!strcmp( name, luaval->name) )
        {
            /* sending nil as 2nd arg actually comes through
               as "no value" (LUA_TNONE) for whatever reason*/
            if ( type == LUA_TNONE || type == LUA_TNIL)
            {
                if (prev)
                {
                    prev->next=luaval->next;
                }
                else
                {
                    /* top of the list */
                    *luavals=luaval->next;
                }
                free_luaval(luaval);
                return 0;
            }
            
            free_string( luaval->val );
            luaval->val = str_dup(smash_tilde_cc(val));
            luaval->type = type;
            luaval->persist= persist;
            return 0;
        }

        prev=luaval;
    }
    
    if ( type != LUA_TNONE && type != LUA_TNIL )
    {        
        luaval=new_luaval( 
                type, 
                str_dup( name ), 
                str_dup( smash_tilde_cc(val) ),
                persist );
        luaval->next = *luavals;
        *luavals     = luaval;
    }
    return 0;
}

static int L_rvnum( lua_State *LS, AREA_DATA *area )
{
    if (!area)
        luaL_error(LS, "NULL area in L_rvnum.");

    int nr=luaL_checkinteger(LS,1);
    int vnum=area->min_vnum + nr;

    if ( vnum < area->min_vnum || vnum > area->max_vnum )
        luaL_error(LS, "Rvnum %d (%d) out of area vnum bounds.", vnum, nr );

    lua_pushinteger(LS, vnum);
    return 1;
}

static int set_flag( lua_State *LS,
        const char *funcname, 
        const struct flag_type *flagtbl, 
        tflag flagvar )
{
    const char *argument = check_string( LS, 2, MIL);
    bool set = TRUE;
    if (!lua_isnone( LS, 3 ) )
    {
        set = lua_toboolean( LS, 3 );
    }
    
    int flag=flag_lookup( argument, flagtbl );
    if ( flag == NO_FLAG )
        luaL_error(LS, "'%s' invalid flag for %s", argument, funcname);
        
    if ( set )
        SET_BIT( flagvar, flag );
    else
        REMOVE_BIT( flagvar, flag);
        
    return 0;
}

static int set_iflag( lua_State *LS,
        const char *funcname,
        const struct flag_type *flagtbl,
        int *intvar)
{
    const char *argument = check_string( LS, 2, MIL);
    bool set = TRUE;
    if (!lua_isnone( LS, 3 ) )
    {
        set=lua_toboolean(LS,3);
    }

    int flag=flag_lookup( argument, flagtbl);
    if ( flag == NO_FLAG )
        return luaL_error(LS, "'%s' invalid flag for %s", argument, funcname);

    if ( set )
        I_SET_BIT( *intvar, flag);
    else
        I_REMOVE_BIT( *intvar, flag);

    return 0;
}
        

static int check_tflag_iflag( lua_State *LS, 
        const char *funcname, 
        const struct flag_type *flagtbl, 
        tflag flagvar,
        int intvar )
{
    if (lua_isnone( LS, 2)) /* called with no string arg */
    {
        /* return array of currently set flags */
        int index=1;
        lua_newtable( LS );
        int i;
        for ( i=0 ; flagtbl[i].name ; i++)
        {

            if ( (flagvar && IS_SET(flagvar, flagtbl[i].bit) )
                    || I_IS_SET( intvar, flagtbl[i].bit) )
            {
                lua_pushstring( LS, flagtbl[i].name);
                lua_rawseti(LS, -2, index++);
            }
        }
        return 1;
    }
    
    const char *argument = check_fstring( LS, 2, MIL);
    int flag=NO_FLAG;
       
    if ((flag=flag_lookup(argument, flagtbl)) == NO_FLAG)
        luaL_error(LS, "'%s' invalid flag for %s", argument, funcname);
    
    if (flagvar)
        lua_pushboolean( LS, IS_SET( flagvar, flag ) );
    else
        lua_pushboolean( LS, I_IS_SET( intvar, flag ) );
    return 1;
}

static int check_flag( lua_State *LS,
        const char *funcname,
        const struct flag_type *flagtbl,
        tflag flagvar)
{
    return check_tflag_iflag( LS,
            funcname,
            flagtbl,
            flagvar,
            0);
}

static int check_iflag( lua_State *LS,
        const char *funcname,
        const struct flag_type *flagtbl,
        int iflagvar)
{
    return check_tflag_iflag( LS,
            funcname,
            flagtbl,
            NULL,
            iflagvar);
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

    lua_getfield( LS, -1, "udobj");
    lua_getfield( LS, -1, "valid");
    if ( !lua_toboolean(LS, -1) )
    {
        /* game object was invalidated/destroyed */
        /* kill the entry and get out of here */
        lua_pushvalue( LS, 1 ); /* lightud as key */
        lua_pushnil( LS ); /* nil as value */
        lua_settable( LS, 2 ); /* pops key and value */

        return 0;
    }
    else
        lua_pop(LS, 1); // pop valid

    /* kill the entry before call in case of error */
    lua_pushvalue( LS, 1 ); /* lightud as key */
    lua_pushnil( LS ); /* nil as value */
    lua_settable( LS, 2 ); /* pops key and value */ 

    lua_getfield( LS, -2, "security");
    int sec=luaL_checkinteger( LS, -1);
    lua_pop(LS, 1);

    lua_getfield( LS, -2, "func");
    lua_getfield( LS, -3, "args");


    /* Check if any args are userdata that are not valid. Don't run if so */
    if ( !lua_isnil( LS, -1 ) )
    {
        push_ref( LS, REF_TABLE_MAXN );
        lua_pushvalue( LS, -2 ); // args table
        lua_call( LS, 1, 1); // call table.maxn
        int args_cnt = luaL_checkinteger( LS, -1);
        lua_pop( LS, 1 );
    
        int i;
        for ( i = 1; i <= args_cnt; ++i )
        {
            lua_rawgeti( LS, -1, i );
            if ( lua_isuserdata( LS, -1 ) )
            {
                // verify it's an arclib object
                lua_getfield( LS, -1, "isarclibobject" );
                bool isarclib = lua_toboolean( LS, -1 );
                lua_pop( LS, 1 );

                if ( !isarclib )
                {
                    continue;
                }

                // finally verify it's valid
                lua_getfield( LS, -1, "valid" );
                bool isvalid = lua_toboolean( LS, -1 );
                lua_pop( LS, 1 );

                if ( !isvalid )
                {
                    /* invalidated reference in arguments, do not run the function */
                    return 0;
                }
            }
            
            lua_pop( LS, 1 );
        } // for each arg
    } // if args is nil

    if ( is_CH( LS, -3 ) )
    {
        lua_mob_program( LS, NULL, RUNDELAY_VNUM, NULL,
                check_CH(LS, -3), NULL, 
                NULL, 0, NULL, 0,
                TRIG_CALL, sec );
    }
    else if ( is_OBJ( LS, -3 ) )
    {
        lua_obj_program( LS, NULL, RUNDELAY_VNUM, NULL,
                check_OBJ(LS, -3), NULL,
                NULL, NULL,
                TRIG_CALL, sec );
    }
    else if ( is_AREA( LS, -3 ) )
    {
        lua_area_program( LS, NULL, RUNDELAY_VNUM, NULL,
                check_AREA(LS, -3), NULL,
                TRIG_CALL, sec );
    }
    else if ( is_ROOM( LS, -3 ) )
    {
        lua_room_program( LS, NULL, RUNDELAY_VNUM, NULL, 
                check_ROOM(LS, -3), NULL,
                NULL, NULL, NULL, NULL,
                TRIG_CALL, sec );
    }
    else
        luaL_error(LS, "Bad udobj type." );

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

int L_delay (lua_State *LS)
{
    /* delaytbl has timer pointers as keys
       value is table with 'tableid' and 'func' keys */
    /* delaytbl[tmr]={ tableid=tableid, func=func } */
    int narg = lua_gettop( LS );
    const char *tag=NULL;
    int val=luaL_checkint( LS, 2 );
    luaL_checktype( LS, 3, LUA_TFUNCTION);

    if ( narg >= 4 && !lua_isnil( LS, 4 ) )
    {   
        tag = check_string( LS, 4, MIL );
    }

    lua_getglobal( LS, "delaytbl");
    TIMER_NODE *tmr=register_lua_timer( val, tag );
    lua_pushlightuserdata( LS, (void *)tmr);
    lua_newtable( LS );

    if ( narg >= 5 )
    {
        /* args 5 and on are args to be passed to the delayed function */
        int arg_ind;
        int tbl_ind;

        lua_newtable( LS );
        for ( arg_ind = 5, tbl_ind = 1; arg_ind <= narg; ++arg_ind, ++tbl_ind )
        {
            lua_pushvalue( LS, arg_ind );
            lua_rawseti( LS, -2, tbl_ind );
        }

        lua_setfield( LS, -2, "args");
    }

    lua_pushvalue( LS, 1 );
    lua_setfield( LS, -2, "udobj" );

    lua_pushvalue( LS, 3 );
    lua_setfield( LS, -2, "func" );

    lua_pushinteger( LS, g_ScriptSecurity ); 
    lua_setfield( LS, -2, "security" );

    lua_settable( LS, -3 );

    return 0;
}

int L_cancel (lua_State *LS)
{
    /* http://pgl.yoyo.org/luai/i/next specifies it is safe
       to modify or clear fields during iteration */
    /* for k,v in pairs(delaytbl) do
            if v.udobj==arg1 then
                unregister_lua_timer(k)
                delaytbl[k]=nil
            end
       end
       */

    /* 1, game object */
    const char *tag=NULL;
    if (!lua_isnone(LS, 2))
    {
        tag=check_string( LS, 2, MIL );
        lua_remove( LS, 2 );
    }

    lua_getglobal( LS, "delaytbl"); /* 2, delaytbl */

    lua_pushnil( LS );
    while ( lua_next(LS, 2) != 0 ) /* pops nil */
    {
        /* key at 3, val at 4 */
        lua_getfield( LS, 4, "udobj");
        if (lua_equal( LS, 5, 1 )==1)
        {
            luaL_checktype( LS, 3, LUA_TLIGHTUSERDATA);
            TIMER_NODE *tmr=(TIMER_NODE *)lua_touserdata( LS, 3);
            if (unregister_lua_timer( tmr, tag ) ) /* return false if tag no match*/
            {
                /* set table entry to nil */
                lua_pushvalue( LS, 3 ); /* push key */
                lua_pushnil( LS );
                lua_settable( LS, 2 );
            }
        }
        lua_pop(LS, 2); /* pop udobj and value */
    }

    return 0;
}

/* macro the heck out of this stuff so we don't have to rewrite for OBJ_DATA and OBJ_INDEX_DATA */
#define OBJVGT( funcname, funcbody ) \
static int OBJ_get_ ## funcname (lua_State *LS)\
{\
    OBJ_DATA *ud_obj=check_OBJ(LS,1);\
    \
    funcbody \
}\
\
static int OBJPROTO_get_ ## funcname (lua_State *LS)\
{\
    OBJ_INDEX_DATA *ud_obj=check_OBJPROTO(LS,1);\
    \
    funcbody \
}

#define OBJVGETINT( funcname, otype, vind ) \
OBJVGT( funcname, \
    if (ud_obj->item_type != otype )\
        luaL_error(LS, #funcname " for %s only.", \
                item_name( otype ) );\
    \
    lua_pushinteger( LS, ud_obj->value[ vind ] );\
    return 1;\
)

#define OBJVGETSTR( funcname, otype, vval )\
OBJVGT( funcname, \
    if (ud_obj->item_type != otype )\
        luaL_error(LS, #funcname " for %s only.", \
                item_name( otype ) );\
    \
    lua_pushstring( LS, vval );\
    return 1;\
)

OBJVGT( weartype,
    lua_pushstring( LS, flag_bit_name( wear_types, ud_obj->wear_type) );
    return 1;
)

OBJVGETINT( light, ITEM_LIGHT, 2 )

OBJVGETINT( arrowcount, ITEM_ARROWS, 0 )

OBJVGETINT( arrowdamage, ITEM_ARROWS, 1 )

OBJVGETSTR( arrowdamtype, ITEM_ARROWS, 
        flag_bit_name(damage_type, ud_obj->value[2]) )

OBJVGT( spelllevel,  
    switch(ud_obj->item_type)
    {
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            lua_pushinteger( LS,
                    ud_obj->value[0]);
            return 1;
        default:
            luaL_error(LS, "Spelllevel for wands, staves, scrolls, potions, and pills only.");
    }
    return 0;
)

OBJVGT( chargestotal,
    switch(ud_obj->item_type)
    {
        case ITEM_WAND:
        case ITEM_STAFF:
            lua_pushinteger( LS,
                    ud_obj->value[1]);
            return 1;
        default:
            luaL_error(LS, "Chargestotal for wands and staves only.");
    }

    return 1;
)

OBJVGT( chargesleft,
    switch(ud_obj->item_type)
    {
        case ITEM_WAND:
        case ITEM_STAFF:
            lua_pushinteger(LS,
                    ud_obj->value[2]);
            return 1;
        case ITEM_PORTAL:
            lua_pushinteger(LS,
                    ud_obj->value[0]);
            return 1;
        default:
            luaL_error(LS, "Chargesleft for wands, staves, and portals only.");
    }

    return 0;
)

OBJVGT( spellname, 
    switch(ud_obj->item_type)
    {
        case ITEM_WAND:
        case ITEM_STAFF:
            lua_pushstring( LS,
                    ud_obj->value[3] != -1 ? skill_table[ud_obj->value[3]].name
                        : "reserved" );
            return 1; 
        default:
            luaL_error(LS, "Spellname for wands and staves only.");
    }

    return 1;
)

OBJVGETINT( toroom, ITEM_PORTAL, 3 )

OBJVGETINT( maxpeople, ITEM_FURNITURE, 0 )

OBJVGT( maxweight, 
    switch(ud_obj->item_type)
    {
        case ITEM_FURNITURE:
            lua_pushinteger( LS,
                    ud_obj->value[1] );
            return 1;
        case ITEM_CONTAINER:
            lua_pushinteger( LS,
                    ud_obj->value[0] );
            return 1;
        default:
            luaL_error(LS, "Maxweight for furniture and containers only.");
    }

    return 0;
)

OBJVGETINT( healbonus, ITEM_FURNITURE, 3 )

OBJVGETINT( manabonus, ITEM_FURNITURE, 4 )

OBJVGT( spells, 
    switch(ud_obj->item_type)
    {
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            lua_newtable(LS);
            int index=1;
            int i;

            for ( i=1 ; i<5 ; i++ )
            {
                if ( ud_obj->value[i] < 1 )
                    continue;

                lua_pushstring( LS,
                        skill_table[ud_obj->value[i]].name );
                lua_rawseti( LS, -2, index++ );
            } 
            return 1;
        default:
            luaL_error( LS, "Spells for pill, potion, and scroll only.");
    }
    
    return 0;
)

OBJVGETINT( ac, ITEM_ARMOR, 0 )

OBJVGETSTR( weapontype, ITEM_WEAPON,
        flag_bit_name(weapon_class, ud_obj->value[0]) )

OBJVGETINT( numdice, ITEM_WEAPON, 1 )

OBJVGETINT( dicetype, ITEM_WEAPON, 2 )

OBJVGETSTR( attacktype, ITEM_WEAPON, attack_table[ud_obj->value[3]].name )

OBJVGETSTR( damtype, ITEM_WEAPON, 
        flag_bit_name(damage_type, attack_table[ud_obj->value[3]].damage) )

OBJVGETSTR( damnoun, ITEM_WEAPON, attack_table[ud_obj->value[3]].noun )

static int OBJ_get_damavg( lua_State *LS )
{
    OBJ_DATA *ud_obj=check_OBJ( LS, 1);
    if (ud_obj->item_type != ITEM_WEAPON )
        luaL_error(LS, "damavg for %s only.", 
                item_name( ITEM_WEAPON ) );
    
    lua_pushinteger( LS, average_weapon_dam( ud_obj ) );
    return 1;
}

static int OBJPROTO_get_damavg( lua_State *LS )
{
    OBJ_INDEX_DATA *ud_obj=check_OBJPROTO( LS, 1);
    if (ud_obj->item_type != ITEM_WEAPON )
        luaL_error(LS, "damavg for %s only.",
                item_name( ITEM_WEAPON ) );
    
    lua_pushinteger( LS, average_weapon_index_dam( ud_obj ) );
    return 1;
}

OBJVGETINT( key, ITEM_CONTAINER, 2 )

OBJVGETINT( capacity, ITEM_CONTAINER, 3 )

OBJVGETINT( weightmult, ITEM_CONTAINER, 4 )

OBJVGT( liquidtotal, 
    switch(ud_obj->item_type)
    {
        case ITEM_FOUNTAIN:
        case ITEM_DRINK_CON:
            lua_pushinteger( LS, ud_obj->value[0] );
            return 1;
        default:
            luaL_error(LS, "liquidtotal for drink and fountain only");
    }

    return 0;
)

OBJVGT( liquidleft,
    switch(ud_obj->item_type)
    {
        case ITEM_FOUNTAIN:
        case ITEM_DRINK_CON:
            lua_pushinteger( LS, ud_obj->value[1] );
            return 1;
        default:
            luaL_error(LS, "liquidleft for drink and fountain only");
    }

    return 0;
)

OBJVGT( liquid,
    switch(ud_obj->item_type)
    {
        case ITEM_FOUNTAIN:
        case ITEM_DRINK_CON:
            lua_pushstring( LS,
                    liq_table[ud_obj->value[2]].liq_name);
            return 1;
        default:
            luaL_error(LS, "liquid for drink and fountain only");
    }

    return 0;
)

OBJVGT( liquidcolor,
    switch(ud_obj->item_type)
    {
        case ITEM_FOUNTAIN:
        case ITEM_DRINK_CON:
            lua_pushstring( LS,
                    liq_table[ud_obj->value[2]].liq_color);
            return 1;
        default:
            luaL_error(LS, "liquidcolor for drink and fountain only");
    }

    return 0;
)

OBJVGT( poisoned, 
    switch(ud_obj->item_type)
    {
        case ITEM_DRINK_CON:
        case ITEM_FOOD:
            lua_pushboolean( LS, ud_obj->value[3] );
            return 1;
        default:
            luaL_error(LS, "poisoned for drink and food only");
    }

    return 0;
)

OBJVGETINT( foodhours, ITEM_FOOD, 0 )

OBJVGETINT( fullhours, ITEM_FOOD, 1 )

OBJVGETINT( silver, ITEM_MONEY, 0 )

OBJVGETINT( gold, ITEM_MONEY, 1 )

#define OBJVM( funcname, body ) \
static int OBJ_ ## funcname ( lua_State *LS )\
{\
    OBJ_DATA *ud_obj=check_OBJ(LS,1);\
    body \
}\
static int OBJPROTO_ ## funcname ( lua_State *LS )\
{\
    OBJ_INDEX_DATA *ud_obj=check_OBJPROTO(LS,1);\
    body \
}

#define OBJVIF( funcname, otype, vind, flagtbl ) \
OBJVM( funcname, \
    if (ud_obj->item_type != otype)\
        luaL_error( LS, #funcname " for %s only", item_name( otype ) );\
    \
    return check_iflag( LS, #funcname, flagtbl, ud_obj->value[ vind ] );\
)

OBJVM( apply,
    const char *type=check_string(LS,2,MIL);
    AFFECT_DATA *pAf;
    for (pAf=ud_obj->affected ; pAf ; pAf=pAf->next)
    {
        if ( !strcmp(
                flag_bit_name(apply_flags, pAf->location),
                type ) )
        {
            lua_pushinteger( LS, pAf->modifier );
            return 1;
        }
    }
    return 0;
)

OBJVIF ( exitflag, ITEM_PORTAL, 1, exit_flags )

OBJVIF ( portalflag, ITEM_PORTAL, 2, portal_flags )

OBJVIF ( furnitureflag, ITEM_FURNITURE, 2, furniture_flags )

OBJVIF ( weaponflag, ITEM_WEAPON, 4, weapon_type2 )

OBJVIF ( containerflag, ITEM_CONTAINER, 1, container_flags )

/* end common section */

/* CH section */
static int CH_rvnum ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_remove(LS,1);

    if (IS_NPC(ud_ch))
        return L_rvnum( LS, ud_ch->pIndexData->area );
    else if (!ud_ch->in_room)
        return luaL_error(LS, "%s not in a room.", ud_ch->name );
    else
        return L_rvnum( LS, ud_ch->in_room->area );
}

static int CH_setval ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_remove(LS, 1);
    return set_luaval( LS, &(ud_ch->luavals) );
}

static int CH_getval ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_remove(LS,1);
    return get_luaval( LS, &(ud_ch->luavals) );
}

static int CH_randchar (lua_State *LS)
{
    CHAR_DATA *ch=get_random_char(check_CH(LS,1) );
    if ( ! ch )
        return 0;

    if ( !push_CH(LS,ch))
        return 0;
    else
        return 1;

}

static int CH_olc (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);
    if (IS_NPC(ud_ch) )
    {
        luaL_error( LS, "NPCs cannot use OLC!");
    }

    if (!run_olc_argument( ud_ch, ud_ch->desc->editor, 
                (char *)check_fstring( LS, 2, MIL)) )
        luaL_error(LS, "Not currently in olc edit mode.");

    return 0;
}

static int CH_tprint ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);

    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    lua_insert( LS, 2);
    /* original arg 2 is pushed to index 3 */
    if (lua_isnone(LS,3))
    {
        lua_pushnil(LS);
    }

    lua_call( LS, 1, 1 );

    do_say( ud_ch, check_string(LS, -1, MIL));

    return 0;
}

static int CH_savetbl (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
    {
        luaL_error( LS, "PCs cannot call savetbl.");
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

static int CH_loadtbl (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
    {
        luaL_error( LS, "PCs cannot call loadtbl.");
        return 0;
    }

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_ch->pIndexData->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int CH_loadscript (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular mprog with vnum 0*/
    lua_mob_program( LS, NULL, LOADSCRIPT_VNUM, check_string(LS, -1, MAX_SCRIPT_LENGTH), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 );

    return 0;
}

static int CH_loadfunction ( lua_State *LS )
{
    lua_mob_program( LS, NULL, RUNDELAY_VNUM, NULL,
                check_CH(LS, -2), NULL,
                NULL, 0, NULL, 0,
                TRIG_CALL, 0 );
    return 0;
}

static int CH_loadstring (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_mob_program( LS, NULL, LOADSCRIPT_VNUM, check_string(LS, 2, MAX_SCRIPT_LENGTH), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 );
    return 0;
} 

static int CH_loadprog (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int num = (int)luaL_checknumber (LS, 2);
    PROG_CODE *pMcode;

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

    lua_mob_program( LS, NULL, num, pMcode->code, ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 ); 

    return 0;
}

static int CH_emote (lua_State *LS)
{
    do_emote( check_CH(LS, 1), check_fstring( LS, 2, MIL) );
    return 0;
}

static int CH_asound (lua_State *LS)
{
    do_mpasound( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    return 0; 
}

static int CH_zecho (lua_State *LS)
{
    do_mpzecho( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    return 0;
}

static int CH_kill (lua_State *LS)
{
    if ( lua_isstring(LS, 2) )
        do_mpkill( check_CH(LS, 1), check_string( LS, 2, MIL));
    else
        mpkill( check_CH(LS, 1),
                check_CH(LS, 2) );

    return 0;
}

static int CH_assist (lua_State *LS)
{
    if ( lua_isstring(LS, 2) )
        do_mpassist( check_CH(LS, 1), check_string( LS, 2, MIL));
    else
        mpassist( check_CH(LS, 1), 
                check_CH(LS, 2) );
    return 0;
}

static int CH_junk (lua_State *LS)
{
    do_mpjunk( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_echo (lua_State *LS)
{
    do_mpecho( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_echoaround (lua_State *LS)
{
    if ( !is_CH(LS, 2) )
    {
        /* standard 'mob echoaround' syntax */
        do_mpechoaround( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }

    mpechoaround( check_CH(LS, 1), check_CH(LS, 2), check_fstring( LS, 3, MIL) );

    return 0;
}

static int CH_echoat (lua_State *LS)
{
    if ( lua_isnone(LS, 3) )
    {
        /* standard 'mob echoat' syntax */
        do_mpechoat( check_CH(LS, 1), check_string(LS, 2, MIL));
        return 0;
    }

    mpechoat( check_CH(LS, 1), check_CH(LS, 2), check_fstring( LS, 3, MIL) );
    return 0;
}

static int CH_mload (lua_State *LS)
{
    CHAR_DATA *mob=mpmload( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    if ( mob && push_CH(LS,mob) )
        return 1;
    else
        return 0;
}

static int CH_purge (lua_State *LS)
{
    // Send empty string for no argument
    if ( lua_isnone( LS, 2) )
    {
        do_mppurge( check_CH(LS, 1), "");
    }
    else
    {
        do_mppurge( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    }

    return 0;
}

static int CH_goto (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    const char *location = check_string(LS,2,MIL);
    bool hidden=FALSE;
    if ( !lua_isnone(LS,3) )
    {
        hidden=lua_toboolean(LS,3);
    }

    do_mpgoto( ud_ch, location);

    if (!hidden)
    {
        do_look( ud_ch, "auto");
    }

    return 0;
}

static int CH_at (lua_State *LS)
{

    do_mpat( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_transfer (lua_State *LS)
{

    do_mptransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_gtransfer (lua_State *LS)
{

    do_mpgtransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_otransfer (lua_State *LS)
{

    do_mpotransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_force (lua_State *LS)
{

    do_mpforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_gforce (lua_State *LS)
{

    do_mpgforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_vforce (lua_State *LS)
{

    do_mpvforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_cast (lua_State *LS)
{

    do_mpcast( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_damage (lua_State *LS)
{
    if ( lua_type(LS, 2 ) == LUA_TSTRING )
    {
        do_mpdamage( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }
    
    CHAR_DATA *ud_ch=check_CH(LS, 1);
    CHAR_DATA *victim=check_CH(LS, 2);
    if (ud_ch->in_room != victim->in_room)
        luaL_error(LS, 
                "Actor and victim must be in same room." );
    int dam=luaL_checkinteger(LS, 3);
    
    bool kill;
    if ( !lua_isnone( LS, 4 ) )
    {
        kill=lua_toboolean( LS, 4 );
    }
    else
    {
        kill=TRUE;
    }

    int damtype;
    if ( !lua_isnone( LS, 5 ) )
    {
        const char *dam_arg=check_string( LS, 5, MIL );
        damtype=flag_lookup( dam_arg, damage_type );
        if ( damtype == NO_FLAG )
            luaL_error(LS, "No such damage type '%s'",
                    dam_arg );
    }
    else
    {
        damtype=DAM_NONE;
    }

    lua_pushboolean( LS,
            deal_damage(ud_ch, victim, dam, TYPE_UNDEFINED, damtype, FALSE, kill) );
    return 1;
}

static int CH_remove (lua_State *LS)
{

    do_mpremove( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_remort (lua_State *LS)
{
    if ( !is_CH(LS, 2) )
    {
        /* standard 'mob remort' syntax */
        do_mpremort( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }

    mpremort( check_CH(LS, 1), check_CH(LS, 2));

    return 0;
}

static int CH_qset (lua_State *LS)
{
    if ( !is_CH( LS, 2 ) )
    {
        /* standard 'mob qset' syntax */
        do_mpqset( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }


    mpqset( check_CH(LS, 1), check_CH(LS, 2),
            check_string(LS, 3, MIL), check_string(LS, 4, MIL),
            lua_isnone( LS, 5 ) ? 0 : (int)luaL_checknumber( LS, 5),
            lua_isnone( LS, 6 ) ? 0 : (int)luaL_checknumber( LS, 6) );

    return 0;
}

static int CH_qadvance (lua_State *LS)
{
    if ( !is_CH( LS, 2) )
    {
        /* standard 'mob qset' syntax */
        do_mpqadvance( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }

    mpqadvance( check_CH(LS, 1), check_CH(LS, 2),
            check_string(LS, 3, MIL),
            lua_isnone( LS, 4 ) ? "" : check_string(LS, 4, MIL) ); 


    return 0;
}

static int CH_reward (lua_State *LS)
{
    if ( !is_CH( LS, 2 ) )
    {
        /* standard 'mob reward' syntax */
        do_mpreward( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }

    mpreward( check_CH(LS, 1), check_CH(LS, 2),
            check_string(LS, 3, MIL),
            (int)luaL_checknumber(LS, 4) );
    return 0;
}

static int CH_peace (lua_State *LS)
{
    if ( lua_isnone( LS, 2) )
        do_mppeace( check_CH(LS, 1), "");
    else
        do_mppeace( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_restore (lua_State *LS)
{
    do_mprestore( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_hit (lua_State *LS)
{
    if (lua_isnone( LS, 2 ) )
    {
        do_mphit( check_CH(LS,1), "" );
    }
    else
    {
        do_mphit( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    }

    return 0;

}

static int CH_behead (lua_State *LS)
{
    CHAR_DATA *ch = check_CH(LS, 1);
    CHAR_DATA *victim = check_CH(LS, 2);
    
    if ( !ch || !victim )
        return 1;
    
    behead(ch, victim);
    return 0;
}

static int CH_mdo (lua_State *LS)
{
    interpret( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_tell (lua_State *LS)
{
    if (lua_isstring(LS, 2))
    {
        char buf[MIL];
        sprintf( buf,
                "'%s' %s",
                check_string(LS, 2, 25),
                check_fstring(LS, 3, MIL-30) );
        do_tell( check_CH(LS, 1), buf );
        return 0;
    }

    tell_char( check_CH( LS, 1),
            check_CH( LS, 2),
            check_fstring( LS, 3, MIL) );
    return 0;
}

static int CH_mobhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, (bool) get_mob_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) ); 
    else
        lua_pushboolean( LS,  (bool) (get_char_room( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_objhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS,(bool) get_obj_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) );
    else
        lua_pushboolean( LS,(bool) (get_obj_here( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_mobexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS,(bool) (get_mp_char( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_objexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, (bool) (get_mp_obj( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_get_ispc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && !IS_NPC( ud_ch ) );
    return 1;
}

static int CH_canattack (lua_State *LS)
{
    lua_pushboolean( LS, !is_safe(check_CH (LS, 1), check_CH (LS, 2)) );
    return 1;
}

static int CH_get_isnpc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_NPC( ud_ch ) );
    return 1;
}

static int CH_get_isgood (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_GOOD( ud_ch ) ) ;
    return 1;
}

static int CH_get_isevil (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_EVIL( ud_ch ) ) ;
    return 1;
}

static int CH_get_isneutral (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_NEUTRAL( ud_ch ) ) ;
    return 1;
}

static int CH_get_isimmort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_IMMORTAL( ud_ch ) ) ;
    return 1;
}

static int CH_get_ischarm (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_AFFECTED( ud_ch, AFF_CHARM ) ) ;
    return 1;
}

static int CH_get_isfollow (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->master != NULL ) ;
    return 1;
}

static int CH_get_isactive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->position > POS_SLEEPING ) ;
    return 1;
}

static int CH_cansee (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH(LS, 1);
    CHAR_DATA * ud_vic = check_CH (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && ud_vic != NULL && can_see( ud_ch, ud_vic ) ) ;

    return 1;
}

static int CH_affected (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string(LS, 2, MIL);

    lua_pushboolean( LS,  ud_ch != NULL
            &&  is_affected_parse(ud_ch, argument) );

    return 1;
}

static int CH_act (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (IS_NPC(ud_ch))
    {
        return check_flag( LS, "act[NPC]", act_flags, ud_ch->act );
    }
    else
    {
        return check_flag( LS, "act[PC]", plr_flags, ud_ch->act );
    }
}

static int CH_setact (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "act[NPC]", act_flags, ud_ch->act );
    }
    else
        return luaL_error( LS, "'setact' for NPC only." );

}

static int CH_offensive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "offensive",off_flags, ud_ch->off_flags );
}

static int CH_setoffensive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "offensive", off_flags, ud_ch->off_flags );
    }
    else
        return luaL_error( LS, "'setoffensive' for NPC only.");
}

static int CH_immune (lua_State *LS)
{ 
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "immune", imm_flags, ud_ch->imm_flags );
}

static int CH_setimmune (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "immune", imm_flags, ud_ch->imm_flags );
    }
    else
        return luaL_error( LS, "'setimmune' for NPC only.");

}

static int CH_carries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string( LS, 2, MIL);
    int count=0;

    if ( is_number( argument ) )
    {
        int vnum=atoi( argument );
        OBJ_DATA *obj;

        for ( obj=ud_ch->carrying ; obj ; obj=obj->next_content )
        {
            if ( obj->pIndexData->vnum == vnum )
            {
                count++;
            }
        }

        if (count<1)
        {
            lua_pushboolean(LS, FALSE);
            return 1;
        }
        else
        {
            lua_pushinteger(LS, count);
            return 1;
        } 
    }
    else
    {
        OBJ_DATA *obj;
        bool exact=FALSE;
        if (!lua_isnone(LS,3))
        {
            exact=lua_toboolean(LS,3);
        }

        for ( obj=ud_ch->carrying ; obj ; obj=obj->next_content )
        {
            if (    obj->wear_loc == WEAR_NONE 
                 && is_either_name( argument, obj->name, exact))
            {
                count++;
            }
        }

        if (count<1)
        {
            lua_pushboolean(LS, FALSE);
            return 1;
        }
        else
        {
            lua_pushinteger(LS, count);
            return 1;
        }
    }
}

static int CH_wears (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, TRUE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_wear( ud_ch, argument ) != NULL) );

    return 1;
}

static int CH_has (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), FALSE ) );

    return 1;
}

static int CH_uses (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), TRUE ) );

    return 1;
}

static int CH_say (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    do_say( ud_ch, check_fstring( LS, 2, MIL) );
    return 0;
}

static int CH_describe (lua_State *LS)
{
    bool cleanUp = false;
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    OBJ_DATA * ud_obj;

    if (lua_isnumber(LS, 2))
    {
        int num = (int)luaL_checknumber (LS, 2);
        OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

        if (!pObjIndex)
        {
            luaL_error(LS, "No object with vnum: %d", num);
        }

        ud_obj = create_object(pObjIndex);
        cleanUp = true;
    }
    else
    {
        ud_obj = check_OBJ(LS, 2);
    }

    describe_item(ud_ch, ud_obj);

    if (cleanUp)
    {
        extract_obj(ud_obj);
    }

    return 0;
}

static int CH_addaffect (lua_State *LS)
{
    int arg_index=1;
    CHAR_DATA * ud_ch = check_CH (LS, arg_index++);
    AFFECT_DATA af;
    const char *temp = NULL;
    const struct flag_type *flag_table;

    /* where */
    temp=check_string(LS,arg_index++,MIL);
    af.where=flag_lookup( temp, apply_types);

    if (af.where==NO_FLAG)
        luaL_error(LS, "No such 'apply_type' flag: %s", temp); 
    else if (af.where != TO_AFFECTS &&
             af.where != TO_IMMUNE &&
             af.where != TO_RESIST &&
             af.where != TO_VULN /* &&
             af.where != TO_SPECIAL*/ /* not supported yet */
            )
        luaL_error(LS, "%s not supported for CH affects.", temp);

    /* type */
    temp=check_string(LS,arg_index++,MIL);
    af.type=skill_lookup( temp );

    if (af.type == -1)
        luaL_error(LS, "Invalid skill: %s", temp);

    /* level */
    af.level=luaL_checkinteger(LS,arg_index++);
    if (af.level<1)
        luaL_error(LS, "Level must be > 0.");

    /* duration */
    af.duration=luaL_checkinteger(LS,arg_index++);
    if (af.duration<-1)
        luaL_error(LS, "Duration must be -1 (indefinite) or greater.");

    /* location */
    switch (af.where)
    {
        case TO_IMMUNE:
        case TO_RESIST:
        case TO_VULN:
            af.location=APPLY_NONE;
            break;
        case TO_AFFECTS:
        {
            temp=check_string(LS,arg_index++,MIL);
            af.location=flag_lookup( temp, apply_flags );
            if (af.location == NO_FLAG)
                luaL_error(LS, "Invalid location: %s", temp);
            break;
        }
        default:
            luaL_error(LS, "Invalid where.");
      
    }

    /* modifier */
    switch (af.where)
    {
        case TO_IMMUNE:
        case TO_RESIST:
        case TO_VULN:
            af.modifier=0;
            break;
        case TO_AFFECTS:
            af.modifier=luaL_checkinteger(LS,arg_index++);
            break;
        default:
            luaL_error(LS, "Invalid where.");
    }

    /* bitvector */
    temp=check_string(LS,arg_index++,MIL);
    if (!strcmp(temp, "none"))
    {
        af.bitvector=0;
    }
    else
    {
        switch (af.where)
        {
            case TO_AFFECTS:
                flag_table=affect_flags;
                break;
            case TO_IMMUNE:
                flag_table=imm_flags;
                break;
            case TO_RESIST:
                flag_table=res_flags;
                break;
            case TO_VULN:
                flag_table=vuln_flags;
                break;
            default:
                return luaL_error(LS, "'where' not supported");
        }
        af.bitvector=flag_lookup( temp, flag_table);
        if (af.bitvector==NO_FLAG)
            luaL_error(LS, "Invalid bitvector: %s", temp);
        else if ( !flag_table[index_lookup(af.bitvector, flag_table)].settable )
            luaL_error(LS, "Flag '%s' is not settable.", temp);
    }

    /* tag (custom_affect only) */
    if (af.type==gsn_custom_affect)
    {
        af.tag=str_dup(check_string(LS,arg_index++,MIL));
    }
    else
        af.tag=NULL;
    
    affect_to_char_tagsafe( ud_ch, &af );

    return 0;
}

static int CH_removeaffect (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    
    if (is_AFFECT(LS,2))
    {
        /* remove a specific affect */
        affect_remove( ud_ch, check_AFFECT(LS,2));
        return 0;
    }

    /* remove by sn */
    const char *skill = check_string(LS, 2, MIL);
    int sn=skill_lookup( skill );
    if (sn==-1)
       luaL_error(LS, "Invalid skill: %s", skill);
    else if (sn==gsn_custom_affect)
    {
        custom_affect_strip( ud_ch, check_string(LS,3,MIL) );
    }
    else
    {
        affect_strip( ud_ch, sn );
    }

    return 0;
} 

static int CH_oload (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj = create_object(pObjIndex);
    check_enchant_obj( obj );

    obj_to_char(obj,ud_ch);

    if ( !push_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}

static int CH_destroy (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    if (!ud_ch)
    {
        luaL_error(LS, "Null pointer in destroy");
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

static int CH_vuln (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "vuln", vuln_flags, ud_ch->vuln_flags );
}

static int CH_setvuln (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "vuln", vuln_flags, ud_ch->vuln_flags );
    }
    else
        return luaL_error( LS, "'setvuln' for NPC only." );

}

static int CH_qstatus (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, quest_status( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int CH_resist (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "resist", res_flags, ud_ch->res_flags );
}

static int CH_setresist (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "resist", res_flags, ud_ch->res_flags );
    }
    else
        return luaL_error( LS, "'setresist' for NPC only.");
}

static int CH_skilled (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string( LS, 2, MIL);
    bool prac=FALSE;
    if (!lua_isnone(LS, 3))
    {
        prac=lua_toboolean(LS, 3);
    }

    int sn=skill_lookup(argument);
    if (sn==-1)
        luaL_error(LS, "No such skill '%s'", argument);

    int skill=get_skill(ud_ch, sn);
    
    if (skill<1)
    {
        lua_pushboolean( LS, FALSE);
        return 1;
    }

    if (prac)
    {
        lua_pushinteger( LS, get_skill_prac( ud_ch, sn) );
        return 1;
    }

    lua_pushinteger(LS, skill);
    return 1;
}

static int CH_ccarries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

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

static int CH_qtimer (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, qset_timer( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int CH_delay (lua_State *LS)
{
    return L_delay( LS );
}

static int CH_cancel (lua_State *LS)
{
    return L_cancel( LS );
}

static int CH_get_ac (lua_State *LS)
{
    lua_pushinteger( LS,
            GET_AC( check_CH( LS, 1 ) ) );
    return 1;
}

static int CH_get_acbase (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH( LS, 1 ))->armor );
    return 1;
}

static int CH_set_acbase (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set acbase on PCs.");

    int val=luaL_checkinteger( LS, 2 );

    if ( val < -10000 || val > 10000 )
    {
        return luaL_error( LS, "Value must be between -10000 and 10000." );
    }
    ud_ch->armor=val;

    return 0;
}

static int CH_set_acpcnt (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set acpcnt on PCs.");

    /* analogous to mob_base_ac */
    ud_ch->armor = ( ud_ch->level * -6 ) * luaL_checkinteger( LS, 2 ) / 100;
    return 0;
}

static int CH_set_waitcount (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    int val=luaL_checkinteger( LS, 2);

    if ( val < 0 || val > 120 )
    {
        return luaL_error( LS, "Valid stopcount range is 0 to 120");
    }
    
    ud_ch->wait=val;

    return 0;
}

static int CH_set_stopcount (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    int val=luaL_checkinteger( LS, 2);

    if ( val < 0 || val > 10 )
    {
        return luaL_error( LS, "Valid stopcount range is 0 to 10");
    }
    
    ud_ch->stop=val;

    return 0;
}

static int CH_get_hitroll (lua_State *LS)
{
    lua_pushinteger( LS,
            GET_HITROLL( check_CH( LS, 1 ) ) );
    return 1;
}

static int CH_get_hitrollbase (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH( LS, 1 ))->hitroll );
    return 1;
}

static int CH_set_hitrollbase (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set hitrollbase on PCs.");

    int val=luaL_checkinteger( LS, 2 );

    if ( val < 0 || val > 1000 )
    {
        return luaL_error( LS, "Value must be between 0 and 1000." );
    } 
    ud_ch->hitroll=val;

    return 0;
}

static int CH_set_hrpcnt (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set hrpcnt on PCs.");

    /* analogous to mob_base_hitroll */
    ud_ch->hitroll= ud_ch->level * luaL_checkinteger( LS, 2 ) / 100 ; 
    return 0;
}

static int CH_get_damroll (lua_State *LS)
{
    lua_pushinteger( LS,
            GET_DAMROLL( check_CH( LS, 1 ) ) );
    return 1;
}

static int CH_get_damrollbase (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH( LS, 1 ))->damroll );
    return 1;
}

static int CH_set_damrollbase (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set damrollbase on PCs.");

    int val=luaL_checkinteger( LS, 2 );

    if ( val < 0 || val > 1000 )
    {
        return luaL_error( LS, "Value must be between 0 and 1000." );
    }
    ud_ch->damroll=val;

    return 0;
}

static int CH_get_dicenumber (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't get dicenumber on PCs.");
    lua_pushinteger( LS,
            ud_ch->damage[DICE_NUMBER]);
    return 1;
}

static int CH_set_dicenumber (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set dicenumber on PCs.");

    int val=luaL_checkinteger( LS, 2);

    if ( val < 1 )
    {
        return luaL_error( LS, "Value must be 1 or greater.");
    }
    ud_ch->damage[DICE_NUMBER]=val;

    return 0;
}

static int CH_get_dicetype (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't get dicetype on PCs.");
    lua_pushinteger( LS,
            ud_ch->damage[DICE_TYPE]);
    return 1;
}

static int CH_set_dicetype (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set dicetype on PCs.");

    int val=luaL_checkinteger( LS, 2);

    if ( val < 1 )
    {
        return luaL_error( LS, "Value must be 1 or greater.");
    }
    ud_ch->damage[DICE_TYPE]=val;

    return 0;
}


static int CH_set_drpcnt (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set drpcnt on PCs.");

    /* analogous to mob_base_damroll */
    ud_ch->damroll= ud_ch->level * luaL_checkinteger( LS, 2 ) / 100 ;
    return 0;
}

static int CH_get_attacktype( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    lua_pushstring( LS, attack_table[ud_ch->dam_type].name );
    return 1;
}

static int CH_set_attacktype (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set attacktype on PCs.");

    const char *arg=check_string(LS,2, MIL);

    int i;
    for ( i=0 ; attack_table[i].name ; i++ )
    {
        if (!strcmp( attack_table[i].name, arg ) )
        {
            ud_ch->dam_type=i;
            return 0;
        }
    }

    luaL_error(LS, "No such attacktype: %s", arg );
    return 1;
}

static int CH_get_savesphys (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);

    lua_pushinteger( LS, get_save(ud_ch, TRUE) );
    return 1;
}

static int CH_get_savesmagic (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);

    lua_pushinteger( LS, get_save(ud_ch, FALSE));
    return 1;
}

static int CH_get_damtype (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    lua_pushstring( LS,
            flag_bit_name(damage_type, attack_table[ud_ch->dam_type].damage) );
    return 1;
}

static int CH_get_damnoun (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    lua_pushstring( LS, attack_table[ud_ch->dam_type].noun );
    return 1;
}

static int CH_get_hp (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH (LS, 1))->hit );
    return 1;
}

static int CH_set_hp (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->hit=num;
    return 0;
}

static int CH_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_CH(LS,1))->name );
    return 1;
}

static int CH_set_name (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set name on PCs.");
    const char *new=check_string(LS, 2, MIL);
    free_string( ud_ch->name );
    ud_ch->name=str_dup(new);
    return 0;
}

static int CH_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->level );
    return 1;
}

static int CH_set_level (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Cannot set level on PC.");

    int num = (int)luaL_checknumber (LS, 2);
    if ( num < 1 || num > 200 )
        luaL_error( LS, "Invalid level: %d, range is 1 to 200.", num);

    float hppcnt= (float)ud_ch->hit/ud_ch->max_hit;
    float mppcnt= (float)ud_ch->mana/ud_ch->max_mana;
    float mvpcnt= (float)ud_ch->move/ud_ch->max_move;

    set_mob_level( ud_ch, num );

    ud_ch->hit  = UMAX(1,hppcnt*ud_ch->max_hit);
    ud_ch->mana = UMAX(0,mppcnt*ud_ch->max_mana);
    ud_ch->move = UMAX(0,mvpcnt*ud_ch->max_move);
    return 0;
}

static int CH_get_maxhp (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_hit );
    return 1;
}

static int CH_set_maxhp (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxhp on PCs.");
        
    ud_ch->max_hit = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_mana (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->mana );
    return 1;
}

static int CH_set_mana (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->mana=num;
    return 0;
}

static int CH_get_maxmana (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_mana );
    return 1;
}

static int CH_set_maxmana (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxmana on PCs.");
        
    ud_ch->max_mana = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_move (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->move );
    return 1;
}

static int CH_set_move (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->move=num;
    return 0;
}

static int CH_get_maxmove (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_move );
    return 1;
}

static int CH_set_maxmove (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxmove on PCs.");
        
    ud_ch->max_move = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_gold (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->gold );
    return 1;
}

static int CH_set_gold (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set gold on PCs.");
        
    ud_ch->gold = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_silver (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->silver );
    return 1;
}

static int CH_set_silver (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set silver on PCs.");
        
    ud_ch->silver = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_money (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_pushinteger( LS,
            ud_ch->silver + ud_ch->gold*100 );
    return 1;
}

static int CH_get_sex (lua_State *LS)
{
    lua_pushstring( LS,
            sex_table[(check_CH(LS,1))->sex].name );
    return 1;
}

static int CH_set_sex (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set sex on PCs.");
    const char *arg=check_string( LS, 2, MIL);
    
    int i;
    for ( i=0 ; sex_table[i].name ; i++ )
    {
        if (!strcmp(sex_table[i].name, arg) )
        {
            ud_ch->sex=i;
            return 0;
        }
    }
    
    luaL_error(LS, "No such sex: %s", arg );
    return 0;
}

static int CH_get_size (lua_State *LS)
{
    lua_pushstring( LS,
            size_table[(check_CH(LS,1))->size].name );
    return 1;
}

static int CH_set_size (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set gold on PCs.");
        
    const char *arg=check_string( LS, 2, MIL);
    int i;
    for ( i=0 ; size_table[i].name ; i++ )
    {
        if (!strcmp( size_table[i].name, arg ) )
        {
            ud_ch->size=i;
            return 0;
        }
    }
    
    luaL_error( LS, "No such size: %s", arg );
    return 0;
}

static int CH_get_position (lua_State *LS)
{
    lua_pushstring( LS,
            position_table[(check_CH(LS,1))->position].short_name );
    return 1;
}

static int CH_get_align (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->alignment );
    return 1;
}

static int CH_set_align (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int num=luaL_checkinteger( LS, 2);
    if (num < -1000 || num > 1000)
        luaL_error(LS, "Invalid align: %d, range is -1000 to 1000.", num);
    ud_ch->alignment = num;
    return 0;
}

static int CH_get_stat_helper( lua_State *LS, int statnum )
{
    CHAR_DATA *ch = check_CH( LS, 1 );
    int statval = get_curr_stat(ch, statnum);

    lua_pushinteger( LS, statval );

    return 1;
}
static int CH_get_str( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_STR ); }
static int CH_get_con( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_CON ); }
static int CH_get_vit( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_VIT ); }
static int CH_get_agi( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_AGI ); }
static int CH_get_dex( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_DEX ); }
static int CH_get_int( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_INT ); }
static int CH_get_wis( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_WIS ); }
static int CH_get_dis( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_DIS ); }
static int CH_get_cha( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_CHA ); }
static int CH_get_luc( lua_State *LS ) { return CH_get_stat_helper( LS, STAT_LUC ); }


static int CH_set_stat_helper( lua_State *LS, int statnum )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set stats on PCs.");
    
    int num = luaL_checkinteger( LS, 2);
    if (num < 1 || num > 200 )
        luaL_error(LS, "Invalid stat value: %d, range is 1 to 200.", num );
    
    ud_ch->perm_stat[ statnum ] = num;
    return 0;
}
static int CH_set_str( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_STR ); }
static int CH_set_con( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_CON ); }
static int CH_set_vit( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_VIT ); }
static int CH_set_agi( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_AGI ); }
static int CH_set_dex( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_DEX ); }
static int CH_set_int( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_INT ); }
static int CH_set_wis( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_WIS ); }
static int CH_set_dis( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_DIS ); }
static int CH_set_cha( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_CHA ); }
static int CH_set_luc( lua_State *LS ) { return CH_set_stat_helper( LS, STAT_LUC ); }


static int CH_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_CH(LS,1))->clan].name);
    return 1;
}

static int CH_get_class (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        luaL_error(LS, "Can't check class on NPC.");
    }

    lua_pushstring( LS,
            class_table[ud_ch->clss].name);
    return 1;
}

static int CH_get_race (lua_State *LS)
{
    lua_pushstring( LS,
            race_table[(check_CH(LS,1))->race].name);
    return 1;
}

static int CH_set_race (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
#ifndef TESTER
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set race on PCs.");
#endif    
    const char * arg=check_string(LS, 2, MIL);
    int race=race_lookup(arg);
    if (race==0)
        luaL_error(LS, "No such race: %s", arg );

#ifdef TESTER
    if ( !IS_NPC(ud_ch) )
    {
        if ( !race_table[race].pc_race )
            luaL_error(LS, "Not a valid player race: %s", arg);
        ud_ch->race=race;
        take_default_stats(ud_ch);
        reset_char( ud_ch );
        morph_update( ud_ch );
        return 0;
    }
#endif
    set_mob_race( ud_ch, race );
    return 0;
}

static int CH_get_fighting (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!ud_ch->fighting)
        return 0;
    else if (!push_CH(LS, ud_ch->fighting) )
        return 0;
    else
        return 1;
}

static int CH_get_waitcount (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_pushinteger(LS, ud_ch->wait);
    return 1;
}

static int CH_get_stopcount (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_pushinteger(LS, ud_ch->stop);
    return 1;
}

static int CH_get_heshe (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
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

static int CH_get_himher (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
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

static int CH_get_hisher (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
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

static int CH_get_inventory (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_ch->carrying ; obj ; obj=obj->next_content)
    {
        if (push_OBJ(LS, obj))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int CH_get_room (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    
    if (!ud_ch->in_room)
        return 0;
    else if ( push_ROOM(LS, check_CH(LS,1)->in_room) )
        return 1;
    else
        return 0;
}

static int CH_get_groupsize (lua_State *LS)
{
    lua_pushinteger( LS,
            count_people_room( check_CH(LS, 1), 4 ) );
    return 1;
}

static int CH_get_clanrank( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get clanrank on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->clan_rank);
    return 1;
}

static int CH_get_remorts( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get remorts on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->remorts);
    return 1;
}

static int CH_get_explored( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get explored on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->explored->set);
    return 1;
}

static int CH_get_beheads( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get beheads on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->behead_cnt);
    return 1;
}

static int CH_get_pkills( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get pkills on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->pkill_count);
    return 1;
}

static int CH_get_pkdeaths( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get pkdeaths on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->pkill_deaths);
    return 1;
}

static int CH_get_questpoints( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get questpoints on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->questpoints);
    return 1;
}

static int CH_set_questpoints( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't set questpoints on NPCs.");

    ud_ch->pcdata->questpoints=luaL_checkinteger(LS, 2);
    return 0;
}

static int CH_get_achpoints( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get achpoints on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->achpoints);
    return 1;
}

static int CH_get_bank( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get bank on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->bank);
    return 1;
}

static int CH_get_mobkills( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get mobkills on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->mob_kills);
    return 1;
}

static int CH_get_mobdeaths( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get mobdeaths on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->mob_deaths);
    return 1;
}

static int CH_get_bossachvs( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get bossachvs on NPCs.");

    BOSSREC *rec;
    int index=1;
    lua_newtable(LS);

    for ( rec=ud_ch->pcdata->boss_achievements ; rec; rec=rec->next)
    {
        if (push_BOSSREC(LS, rec))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int CH_get_vnum( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get vnum on PCs.");

    lua_pushinteger( LS,
            ud_ch->pIndexData->vnum);
    return 1;
}

static int CH_get_proto( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get proto on PCs.");

    if (!push_MOBPROTO( LS, ud_ch->pIndexData ) )
        return 0;
    else
        return 1;
}

static int CH_get_ingame( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get ingame on PCs.");

    lua_pushboolean( LS, is_mob_ingame( ud_ch->pIndexData ) );
    return 1;
}

static int CH_get_shortdescr( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get shortdescr on PCs.");

    lua_pushstring( LS,
            ud_ch->short_descr);
    return 1;
}

static int CH_set_shortdescr (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set shortdescr on PCs.");
    const char *new=check_string(LS, 2, MIL);
    free_string( ud_ch->short_descr );
    ud_ch->short_descr=str_dup(new);
    return 0;
}

static int CH_get_longdescr( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get longdescr on PCs.");

    lua_pushstring( LS,
            ud_ch->long_descr);
    return 1;
}

static int CH_set_longdescr (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set longdescr on PCs.");
    const char *new=check_string(LS, 2, MIL);
    free_string( ud_ch->long_descr );
    ud_ch->long_descr=str_dup(new);
    return 0;
}

static int CH_get_description( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    lua_pushstring( LS,
            ud_ch->description);
    return 1;
}

static int CH_set_description (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set description on PCs.");
    const char *new=check_string(LS, 2, MSL);

    // Need to make sure \n\r at the end but don't add if already there.
    int len=strlen(new);
    if ( len>1 &&
            !( new[len-2]=='\n' && new[len-1]=='\r') )
    {
        if ( len > (MSL-3) )
            luaL_error( LS, "Description must be %d characters or less.", MSL-3);

        char buf[MSL];
        sprintf(buf, "%s\n\r",new);
        new=buf;
    }
    free_string( ud_ch->description );
    ud_ch->description=str_dup(new);
    return 0;
}

static int CH_get_ptitles( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'ptitles' for NPC.");
    }

    push_ref( LS, ud_ch->pcdata->ptitles );
    return 1;
}

static int CH_set_ptitles( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't set 'ptitles' for NPC.");
    }

    /* probably should check type and format of table in the future */

    save_ref( LS, 2, &(ud_ch->pcdata->ptitles));
    return 0;
}

static int CH_get_ptitle( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'ptitle' for NPC.");
    }

    lua_pushstring(LS, ud_ch->pcdata->pre_title);
    return 1;
}

static int CH_set_ptitle( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't set 'ptitle' for NPC.");
    }

    const char *new=check_string( LS, 2, MIL);
    free_string(ud_ch->pcdata->pre_title);
    ud_ch->pcdata->pre_title=str_dup(new);
    return 0;
}

static int CH_get_stance (lua_State *LS)
{
    lua_pushstring( LS,
            stances[ (check_CH( LS, 1) )->stance ].name );
    return 1;
}

static int CH_get_pet (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if ( ud_ch->pet && push_CH(LS, ud_ch->pet) )
        return 1;
    else
        return 0;
}

static int CH_get_master (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if ( IS_AFFECTED(ud_ch, AFF_CHARM) 
            && ud_ch->master 
            && push_CH(LS, ud_ch->master) )
        return 1;
    else
        return 0;
}

static int CH_get_leader (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if ( ud_ch->leader && push_CH(LS, ud_ch->leader) )
        return 1;
    else
        return 0;
}

static int CH_set_pet (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    CHAR_DATA *pet=check_CH(LS,2);

    if (IS_NPC(ud_ch))
        luaL_error(LS,
                "Can only add pets to PCs.");
    else if (!IS_NPC(pet))
        luaL_error(LS,
                "Can only add NPCs as pets.");
    else if (ud_ch->pet)
        luaL_error(LS,
                "%s already has a pet.", ud_ch->name);
    else if (IS_AFFECTED(pet, AFF_CHARM))
        luaL_error(LS,
                "%s is already charmed.", pet->name);

    SET_BIT(pet->act, ACT_PET);
    SET_BIT(pet->affect_field, AFF_CHARM);
    flag_clear( pet->penalty );
    SET_BIT( pet->penalty, PENALTY_NOTELL );
    SET_BIT( pet->penalty, PENALTY_NOSHOUT );
    SET_BIT( pet->penalty, PENALTY_NOCHANNEL );
    add_follower( pet, ud_ch );
    pet->leader = ud_ch;
    ud_ch->pet = pet;

    return 0;
}

static int CH_get_id ( lua_State *LS )
{
    lua_pushinteger( LS,
            check_CH(LS,1)->id );
    return 1;
}

static int CH_get_scroll ( lua_State *LS )
{
    lua_pushinteger( LS,
            check_CH(LS,1)->lines );
    return 1;
}

static int CH_get_affects ( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    AFFECT_DATA *af;

    int index=1;
    lua_newtable( LS );

    for ( af=ud_ch->affected ; af ; af=af->next )
    {
        if (push_AFFECT(LS,af))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int CH_get_descriptor( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (ud_ch->desc)
    {
        if ( push_DESCRIPTOR(LS, ud_ch->desc) )
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

static int CH_get_godname( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'godname' for NPC.");
    }

    if (!ud_ch->pcdata->god_name)
        return 0;
    
    lua_pushstring(LS, ud_ch->pcdata->god_name);
    return 1;
}

static int CH_set_godname( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't set 'godname' for NPC.");
    }

    const char *value=check_string(LS, 2, MSL);
    free_string(ud_ch->pcdata->god_name);
    ud_ch->pcdata->god_name = str_dup(value);
    return 0;
}    

static int CH_get_ascents(lua_State *LS)
{
    CHAR_DATA *ud_ch = check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'ascents' for NPC.");
    }
    lua_pushinteger(LS, ud_ch->pcdata->ascents);
    return 1;
}

#ifdef TESTER
static int CH_set_subclass(lua_State *LS)
{
    CHAR_DATA *ud_ch = check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't set 'subclass' for NPC.");
    }

    if (lua_isnil(LS, 2)) 
    {
        ud_ch->pcdata->subclass = 0;
        return 0;
    }
    else
    {
        const char *arg = check_string(LS, 2, MIL);

        int sc = subclass_lookup(arg);
        if (sc == 0)
        {
            return luaL_error(LS, "No such subclass '%s'", arg);
        }

        if (!ch_can_take_subclass(ud_ch, sc))
        {
            return luaL_error(LS, "%s does not qualify for subclass %s", ud_ch->name, arg);
        }
        
        ud_ch->pcdata->subclass = sc;

        return 0;
    }
}
#endif

static int CH_get_subclass(lua_State *LS)
{
    CHAR_DATA *ud_ch = check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'subclass' for NPC.");
    }

    if (!ud_ch->pcdata->subclass)
    {
        return 0;
    }
    else
    {
        lua_pushstring(LS, subclass_table[ud_ch->pcdata->subclass].name);
        return 1;
    }
}

static int CH_get_faith( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'faith' for NPC.");
    }

    lua_pushinteger(LS, ud_ch->pcdata->faith);
    return 1;
}

static int CH_get_religionrank( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'religionrank' for NPC.");
    }

    lua_pushstring(LS, get_ch_rank_name(ud_ch));
    return 1;
}

static int CH_set_religionrank( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't set 'religionrank' for NPC.");
    }

    if (ud_ch->pcdata->god_name[0] == '\0')
    {
        return luaL_error(LS, "Can't set 'religionrank' with no 'godname' setting.");
    }

    const char *val=check_string(LS,2,MSL);
    int newrank=get_religion_rank_number( val );

    if (newrank == -1)
    {
        return luaL_error(LS, "No such religion rank '%s'", val);
    }

    ud_ch->pcdata->religion_rank=newrank;

    return 0;
}

static const LUA_PROP_TYPE CH_get_table [] =
{
    { "name", CH_get_name, 0, STS_ACTIVE },
    { "level", CH_get_level, 0, STS_ACTIVE },
    { "hp", CH_get_hp, 0, STS_ACTIVE },
    { "maxhp", CH_get_maxhp, 0, STS_ACTIVE },
    { "mana", CH_get_mana, 0, STS_ACTIVE },
    { "maxmana", CH_get_maxmana, 0, STS_ACTIVE },
    { "move", CH_get_move, 0, STS_ACTIVE },
    { "maxmove", CH_get_maxmove, 0, STS_ACTIVE },
    { "gold", CH_get_gold, 0, STS_ACTIVE },
    { "silver", CH_get_silver, 0, STS_ACTIVE },
    { "money", CH_get_money, 0, STS_ACTIVE },
    { "sex", CH_get_sex, 0, STS_ACTIVE },
    { "size", CH_get_size, 0, STS_ACTIVE },
    { "position", CH_get_position, 0, STS_ACTIVE },
    { "align", CH_get_align, 0, STS_ACTIVE },
    { "str", CH_get_str, 0, STS_ACTIVE },
    { "con", CH_get_con, 0, STS_ACTIVE },
    { "vit", CH_get_vit, 0, STS_ACTIVE },
    { "agi", CH_get_agi, 0, STS_ACTIVE },
    { "dex", CH_get_dex, 0, STS_ACTIVE },
    { "int", CH_get_int, 0, STS_ACTIVE },
    { "wis", CH_get_wis, 0, STS_ACTIVE },
    { "dis", CH_get_dis, 0, STS_ACTIVE },
    { "cha", CH_get_cha, 0, STS_ACTIVE },
    { "ac", CH_get_ac, 0, STS_ACTIVE },
    { "acbase", CH_get_acbase, 0, STS_ACTIVE },
    { "hitroll", CH_get_hitroll, 0, STS_ACTIVE },
    { "hitrollbase", CH_get_hitrollbase, 0, STS_ACTIVE },
    { "damroll", CH_get_damroll, 0, STS_ACTIVE },
    { "damrollbase", CH_get_damrollbase, 0, STS_ACTIVE },
    { "attacktype", CH_get_attacktype, 0, STS_ACTIVE },
    { "damnoun", CH_get_damnoun, 0, STS_ACTIVE },
    { "damtype", CH_get_damtype, 0, STS_ACTIVE },
    { "savesphys", CH_get_savesphys, 0, STS_ACTIVE },
    { "savesmagic", CH_get_savesmagic, 0, STS_ACTIVE },
    { "luc", CH_get_luc, 0, STS_ACTIVE },
    { "clan", CH_get_clan, 0, STS_ACTIVE },
    { "class", CH_get_class, 0, STS_ACTIVE },
    { "race", CH_get_race, 0, STS_ACTIVE },
    { "ispc", CH_get_ispc, 0, STS_ACTIVE },
    { "isnpc", CH_get_isnpc, 0, STS_ACTIVE },
    { "isgood", CH_get_isgood, 0, STS_ACTIVE },
    { "isevil", CH_get_isevil, 0, STS_ACTIVE },
    { "isneutral", CH_get_isneutral, 0, STS_ACTIVE },
    { "isimmort", CH_get_isimmort, 0, STS_ACTIVE },
    { "ischarm", CH_get_ischarm, 0, STS_ACTIVE },
    { "isfollow", CH_get_isfollow, 0, STS_ACTIVE },
    { "isactive", CH_get_isactive, 0, STS_ACTIVE },
    { "fighting", CH_get_fighting, 0, STS_ACTIVE },
    { "stopcount", CH_get_stopcount, 0, STS_ACTIVE },
    { "waitcount", CH_get_waitcount, 0, STS_ACTIVE },
    { "heshe", CH_get_heshe, 0, STS_ACTIVE },
    { "himher", CH_get_himher, 0, STS_ACTIVE },
    { "hisher", CH_get_hisher, 0, STS_ACTIVE },
    { "inventory", CH_get_inventory, 0, STS_ACTIVE },
    { "room", CH_get_room, 0, STS_ACTIVE },
    { "groupsize", CH_get_groupsize, 0, STS_ACTIVE },
    { "stance", CH_get_stance, 0, STS_ACTIVE },
    { "description", CH_get_description, 0, STS_ACTIVE },
    { "pet", CH_get_pet, 0, STS_ACTIVE },
    { "master", CH_get_master, 0, STS_ACTIVE },
    { "leader", CH_get_leader, 0, STS_ACTIVE },
    { "affects", CH_get_affects, 0, STS_ACTIVE },
    { "scroll", CH_get_scroll, 0, STS_ACTIVE },
    { "id", CH_get_id, 0, STS_ACTIVE },
    /* PC only */
    { "godname", CH_get_godname, 0, STS_ACTIVE },
    { "faith", CH_get_faith, 0, STS_ACTIVE },
    { "religionrank", CH_get_religionrank, 0, STS_ACTIVE },
    { "ascents", CH_get_ascents, 0, STS_ACTIVE },
    { "subclass", CH_get_subclass, 0, STS_ACTIVE },
    { "clanrank", CH_get_clanrank, 0, STS_ACTIVE },
    { "remorts", CH_get_remorts, 0, STS_ACTIVE },
    { "explored", CH_get_explored, 0, STS_ACTIVE },
    { "beheads", CH_get_beheads, 0, STS_ACTIVE },
    { "pkills", CH_get_pkills, 0, STS_ACTIVE },
    { "pkdeaths", CH_get_pkdeaths, 0, STS_ACTIVE },
    { "questpoints", CH_get_questpoints, 0, STS_ACTIVE },
    { "achpoints", CH_get_achpoints, 0, STS_ACTIVE },
    { "bank", CH_get_bank, 0, STS_ACTIVE },
    { "mobkills", CH_get_mobkills, 0, STS_ACTIVE },
    { "mobdeaths", CH_get_mobdeaths, 0, STS_ACTIVE },
    { "descriptor", CH_get_descriptor, 0, STS_ACTIVE },
    { "bossachvs", CH_get_bossachvs, 0, STS_ACTIVE },
    { "ptitle", CH_get_ptitle, 0, STS_ACTIVE },
    { "ptitles", CH_get_ptitles, 0, STS_ACTIVE },
    /* NPC only */
    { "vnum", CH_get_vnum, 0, STS_ACTIVE },
    { "proto", CH_get_proto, 0, STS_ACTIVE },
    { "ingame", CH_get_ingame, 0, STS_ACTIVE },
    { "shortdescr", CH_get_shortdescr, 0, STS_ACTIVE },
    { "longdescr", CH_get_longdescr, 0, STS_ACTIVE },    
    { "dicenumber", CH_get_dicenumber, 0, STS_ACTIVE },
    { "dicetype", CH_get_dicetype, 0, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE CH_set_table [] =
{
    { "name", CH_set_name, 5, STS_ACTIVE },
    { "level", CH_set_level, 5, STS_ACTIVE },
    { "hp", CH_set_hp, 5, STS_ACTIVE },
    { "maxhp", CH_set_maxhp, 5, STS_ACTIVE },
    { "mana", CH_set_mana, 5, STS_ACTIVE },
    { "maxmana", CH_set_maxmana, 5, STS_ACTIVE },
    { "move", CH_set_move, 5, STS_ACTIVE },
    { "maxmove", CH_set_maxmove, 5, STS_ACTIVE },
    { "gold", CH_set_gold, 5, STS_ACTIVE },
    { "silver", CH_set_silver, 5, STS_ACTIVE },
    { "sex", CH_set_sex, 5, STS_ACTIVE },
    { "size", CH_set_size, 5, STS_ACTIVE },
    { "align", CH_set_align, 5, STS_ACTIVE },
    { "str", CH_set_str, 5, STS_ACTIVE },
    { "con", CH_set_con, 5, STS_ACTIVE },
    { "vit", CH_set_vit, 5, STS_ACTIVE },
    { "agi", CH_set_agi, 5, STS_ACTIVE },
    { "dex", CH_set_dex, 5, STS_ACTIVE },
    { "int", CH_set_int, 5, STS_ACTIVE },
    { "wis", CH_set_wis, 5, STS_ACTIVE },
    { "dis", CH_set_dis, 5, STS_ACTIVE },
    { "cha", CH_set_cha, 5, STS_ACTIVE },
    { "luc", CH_set_luc, 5, STS_ACTIVE },
    { "stopcount", CH_set_stopcount, 5, STS_ACTIVE },
    { "waitcount", CH_set_waitcount, 5, STS_ACTIVE },
    { "acpcnt", CH_set_acpcnt, 5, STS_ACTIVE },
    { "acbase", CH_set_acbase, 5, STS_ACTIVE },
    { "hrpcnt", CH_set_hrpcnt, 5, STS_ACTIVE },
    { "hitrollbase", CH_set_hitrollbase, 5, STS_ACTIVE },
    { "drpcnt", CH_set_drpcnt, 5, STS_ACTIVE },
    { "damrollbase", CH_set_damrollbase, 5, STS_ACTIVE },
    { "attacktype", CH_set_attacktype, 5, STS_ACTIVE },
    { "race", CH_set_race, 5, STS_ACTIVE },
    { "shortdescr", CH_set_shortdescr, 5, STS_ACTIVE },
    { "longdescr", CH_set_longdescr, 5, STS_ACTIVE },
    { "description", CH_set_description, 5, STS_ACTIVE },
    { "pet", CH_set_pet, 5, STS_ACTIVE },
    /* PC only */
    { "godname", CH_set_godname, 9, STS_ACTIVE },
    { "religionrank", CH_set_religionrank, 9, STS_ACTIVE },
#ifdef TESTER
    { "subclass", CH_set_subclass, 9, STS_ACTIVE },
#endif
    { "questpoints", CH_set_questpoints, SEC_NOSCRIPT, STS_ACTIVE },
    { "ptitles", CH_set_ptitles, SEC_NOSCRIPT, STS_ACTIVE },
    { "ptitle", CH_set_ptitle, SEC_NOSCRIPT, STS_ACTIVE },
    /* NPC only */
    { "dicenumber", CH_set_dicenumber, 5, STS_ACTIVE },
    { "dicetype", CH_set_dicetype, 5, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE CH_method_table [] =
{
    { "mobhere", CH_mobhere, 0, STS_ACTIVE },
    { "objhere", CH_objhere, 0, STS_ACTIVE },
    { "mobexists", CH_mobexists, 0, STS_ACTIVE },
    { "objexists", CH_objexists, 0, STS_ACTIVE },
    { "affected", CH_affected, 0, STS_ACTIVE },
    { "act", CH_act, 0, STS_ACTIVE },
    { "offensive", CH_offensive, 0, STS_ACTIVE },
    { "immune", CH_immune, 0, STS_ACTIVE },
    { "carries", CH_carries, 0, STS_ACTIVE },
    { "wears", CH_wears, 0, STS_ACTIVE },
    { "has", CH_has, 0, STS_ACTIVE },
    { "uses", CH_uses, 0, STS_ACTIVE },
    { "qstatus", CH_qstatus, 0, STS_ACTIVE },
    { "resist", CH_resist, 0, STS_ACTIVE },
    { "vuln", CH_vuln, 0, STS_ACTIVE },
    { "skilled", CH_skilled, 0, STS_ACTIVE },
    { "ccarries", CH_ccarries, 0, STS_ACTIVE },
    { "qtimer", CH_qtimer, 0, STS_ACTIVE },
    { "cansee", CH_cansee, 0, STS_ACTIVE },
    { "canattack", CH_canattack, 0, STS_ACTIVE },
    { "destroy", CH_destroy, 1, STS_ACTIVE },
    { "oload", CH_oload, 1, STS_ACTIVE },
    { "say", CH_say, 1, STS_ACTIVE },
    { "emote", CH_emote, 1, STS_ACTIVE },
    { "mdo", CH_mdo, 1, STS_ACTIVE },
    { "tell", CH_tell, 1, STS_ACTIVE },
    { "asound", CH_asound, 1, STS_ACTIVE },
    { "zecho", CH_zecho, 1, STS_ACTIVE },
    { "kill", CH_kill, 1, STS_ACTIVE },
    { "assist", CH_assist, 1, STS_ACTIVE },
    { "junk", CH_junk, 1, STS_ACTIVE },
    { "echo", CH_echo, 1, STS_ACTIVE },
    
    /* deprecated in favor of global funcs */
    { "echoaround", CH_echoaround, 1, STS_DEPRECATED},
    { "echoat", CH_echoat, 1, STS_DEPRECATED},

    { "mload", CH_mload, 1, STS_ACTIVE },
    { "purge", CH_purge, 1, STS_ACTIVE },
    { "goto", CH_goto, 1, STS_ACTIVE },
    { "at", CH_at, 1, STS_ACTIVE },
    
    /* deprecated in favor of global funcs */
    { "transfer", CH_transfer, 1, STS_DEPRECATED},
    { "gtransfer", CH_gtransfer, 1, STS_DEPRECATED},
    
    { "otransfer", CH_otransfer, 1, STS_ACTIVE },
    { "force", CH_force, 1, STS_ACTIVE },
    { "gforce", CH_gforce, 1, STS_ACTIVE },
    { "vforce", CH_vforce, 1, STS_ACTIVE },
    { "cast", CH_cast, 1, STS_ACTIVE },
    { "damage", CH_damage, 1, STS_ACTIVE },
    { "remove", CH_remove, 1, STS_ACTIVE },
    { "remort", CH_remort, 1, STS_ACTIVE },
    { "qset", CH_qset, 1, STS_ACTIVE },
    { "qadvance", CH_qadvance, 1, STS_ACTIVE },
    { "reward", CH_reward, 1, STS_ACTIVE },
    { "peace", CH_peace, 1, STS_ACTIVE },
    { "restore", CH_restore, 1, STS_ACTIVE },
    { "setact", CH_setact, 1, STS_ACTIVE },
    { "setoffensive", CH_setoffensive, 1, STS_ACTIVE },
    { "setvuln", CH_setvuln, 1, STS_ACTIVE },
    { "setimmune", CH_setimmune, 1, STS_ACTIVE },
    { "setresist", CH_setresist, 1, STS_ACTIVE },
    { "hit", CH_hit, 1, STS_ACTIVE },
    { "behead", CH_behead, 1, STS_ACTIVE },
    { "randchar", CH_randchar, 0, STS_ACTIVE },
    { "loadprog", CH_loadprog, 1, STS_ACTIVE },
    { "loadscript", CH_loadscript, 1, STS_ACTIVE },
    { "loadstring", CH_loadstring, 1, STS_ACTIVE },
    { "loadfunction", CH_loadfunction, 1, STS_ACTIVE },
    { "savetbl", CH_savetbl, 1, STS_ACTIVE },
    { "loadtbl", CH_loadtbl, 1, STS_ACTIVE },
    { "tprint", CH_tprint, 1, STS_ACTIVE },
    { "olc", CH_olc, 1, STS_ACTIVE },
    { "delay", CH_delay, 1, STS_ACTIVE },
    { "cancel", CH_cancel, 1, STS_ACTIVE }, 
    { "setval", CH_setval, 1, STS_ACTIVE },
    { "getval", CH_getval, 1, STS_ACTIVE },
    { "rvnum", CH_rvnum, 0, STS_ACTIVE },
    { "describe", CH_describe, 1, STS_ACTIVE },
    { "addaffect", CH_addaffect, 9, STS_ACTIVE },
    { "removeaffect", CH_removeaffect, 9, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
}; 

/* end CH section */

/* OBJ section */
static int OBJ_set_weapontype( lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    if (ud_obj->item_type != ITEM_WEAPON )
        return luaL_error(LS, "weapontype for weapon only.");
 
    const char *arg1=check_string(LS,2,MIL);

    int new_type=flag_lookup(arg1, weapon_class);
    if ( new_type == NO_FLAG )
    {
        return luaL_error(LS, "No such weapontype '%s'", arg1);
    }

    ud_obj->value[0]=new_type;

    return 0;
}

static int OBJ_setexitflag( lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    if (ud_obj->item_type != ITEM_PORTAL)
        return luaL_error(LS, "setexitflag for portal only");

    return set_iflag( LS, "exit_flags", exit_flags, &ud_obj->value[1] );
}
    

static int OBJ_rvnum ( lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_remove(LS,1);

    return L_rvnum( LS, ud_obj->pIndexData->area );
}

static int OBJ_loadfunction (lua_State *LS)
{
    lua_obj_program( LS, NULL, RUNDELAY_VNUM, NULL,
                check_OBJ(LS, -2), NULL,
                NULL, NULL,
                TRIG_CALL, 0 );
    return 0;
}

static int OBJ_setval (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_remove( LS, 1 );
    return set_luaval( LS, &(ud_obj->luavals) );
}

static int OBJ_getval (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_remove( LS, 1);

    return get_luaval( LS, &(ud_obj->luavals) );
}

static int OBJ_delay (lua_State *LS)
{
    return L_delay(LS);
}

static int OBJ_cancel (lua_State *LS)
{
    return L_cancel(LS);
}

static int OBJ_savetbl (lua_State *LS)
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

static int OBJ_loadtbl (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_obj->pIndexData->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int OBJ_loadscript (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular oprog with vnum 0*/

    lua_pushboolean( LS,
            lua_obj_program( LS, NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH), ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );

    return 1;

}

static int OBJ_loadstring (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_pushboolean( LS,
            lua_obj_program( LS, NULL, LOADSCRIPT_VNUM, check_string( LS, 2, MAX_SCRIPT_LENGTH), ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );
    return 1;
}

static int OBJ_loadprog (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    PROG_CODE *pOcode;

    if ( (pOcode = get_oprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: oprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_obj_program( LS, NULL, num, pOcode->code, ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );

    return 1;
}

static int OBJ_destroy( lua_State *LS)
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

static int OBJ_clone( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    bool copy_luavals=FALSE;

    if (!lua_isnone(LS,2))
    {
        copy_luavals=lua_toboolean(LS,2);
    }

    OBJ_DATA *clone = create_object(ud_obj->pIndexData);
    clone_object( ud_obj, clone );

    if (copy_luavals)
    {
        LUA_EXTRA_VAL *luaval;
        LUA_EXTRA_VAL *cloneval;
        for ( luaval=ud_obj->luavals; luaval ; luaval=luaval->next )
        {
            cloneval=new_luaval(
                    luaval->type,
                    str_dup( luaval->name ),
                    str_dup( luaval->val ),
                    luaval->persist);

            cloneval->next=clone->luavals;
            clone->luavals=cloneval;
        }
    }

    if (ud_obj->carried_by)
        obj_to_char( clone, ud_obj->carried_by );
    else if (ud_obj->in_room)
        obj_to_room( clone, ud_obj->in_room );
    else if (ud_obj->in_obj)
        obj_to_obj( clone, ud_obj->in_obj );
    else
        luaL_error( LS, "Cloned object has no location.");

    if (push_OBJ( LS, clone))
        return 1;
    else
        return 0;
}

static int OBJ_oload (lua_State *LS)
{
    OBJ_DATA * ud_obj = check_OBJ (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if ( ud_obj->item_type != ITEM_CONTAINER )
    {
        luaL_error(LS, "Tried to load object in non-container." );
    }

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj = create_object(pObjIndex);
    check_enchant_obj( obj );
    obj_to_obj(obj,ud_obj);

    if ( !push_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}

static int OBJ_extra( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    return check_flag( LS, "extra", extra_flags, ud_obj->extra_flags );
}

static int OBJ_echo( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    const char *argument = check_fstring(LS, 2, MIL);

    if (ud_obj->carried_by)
    {
        send_to_char(argument, ud_obj->carried_by);
        send_to_char( "\n\r", ud_obj->carried_by);
    }
    else if (ud_obj->in_room)
    {
        CHAR_DATA *ch;
        for ( ch=ud_obj->in_room->people ; ch ; ch=ch->next_in_room )
        {
            send_to_char( argument, ch );
            send_to_char( "\n\r", ch );
        }
    }
    else
    {
        // Nothing, must be in a container
    }

    return 0;
}

static int OBJ_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    lua_insert( LS, 2);
    /* original arg 2 is pushed to index 3 */
    if (lua_isnone(LS,3))
    {
        lua_pushnil(LS);
    }
    
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, OBJ_echo );
    /* now line up arguments for echo */
    lua_pushvalue( LS, 1); /* obj */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;

}

static int OBJ_setweaponflag( lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    
    if (ud_obj->item_type != ITEM_WEAPON)
        return luaL_error(LS, "setweaponflag for weapon only");

    return set_iflag( LS, "weapon_type2", weapon_type2, &ud_obj->value[4]);
}

static int OBJ_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->name);
    return 1;
}

static int OBJ_set_name (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->name);
    ud_obj->name=str_dup(arg);
    return 0;
}

static int OBJ_get_shortdescr (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->short_descr);
    return 1;
}

static int OBJ_set_shortdescr (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->short_descr);
    ud_obj->short_descr=str_dup(arg);
    return 0;
}

static int OBJ_get_description (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->description);
    return 1;
}

static int OBJ_set_description (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->description);
    ud_obj->description=str_dup(arg);
    return 0;
}


static int OBJ_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_OBJ(LS,1))->clan].name);
    return 1;
}

static int OBJ_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->rank);
    return 1;
}

static int OBJ_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->level);
    return 1;
}

static int OBJ_set_level (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int arg=luaL_checkinteger(LS,2);
    
    ud_obj->level=arg;
    return 0;
}

static int OBJ_get_owner (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->owner)
        return 0;
    lua_pushstring( LS,
            ud_obj->owner);
    return 1;
}

static int OBJ_set_owner (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->owner);
    ud_obj->owner=str_dup(arg);
    return 0;
}

static int OBJ_get_cost (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->cost);
    return 1;
}

static int OBJ_get_material (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->material);
    return 1;
}

static int OBJ_set_material (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->material);
    ud_obj->material=str_dup(arg);
    return 0;
}

static int OBJ_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->pIndexData->vnum);
    return 1;
}

static int OBJ_get_otype (lua_State *LS)
{
    lua_pushstring( LS,
            item_name((check_OBJ(LS,1))->item_type));
    return 1;
}

static int OBJ_get_weight (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->weight);
    return 1;
}

static int OBJ_set_weight (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int arg=luaL_checkinteger(LS,2);
    
    ud_obj->weight=arg;
    return 0;
}

static int OBJ_get_wearlocation (lua_State *LS)
{
    lua_pushstring( LS,
            flag_bit_name(wear_loc_flags,(check_OBJ(LS,1))->wear_loc) );
    return 1;
}

static int OBJ_get_proto (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!push_OBJPROTO( LS, ud_obj->pIndexData) )
        return 0;
    else
        return 1;
}

static int OBJ_get_ingame (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_pushboolean( LS, is_obj_ingame( ud_obj->pIndexData ) );
    return 1;
}

static int OBJ_get_contents (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_obj->contains ; obj ; obj=obj->next_content)
    {
        if ( push_OBJ(LS, obj) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int OBJ_get_room (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->in_room)
        return 0;
    if ( push_ROOM(LS, ud_obj->in_room) )
        return 1;
    else
        return 0;
}

static int OBJ_set_room (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    ROOM_INDEX_DATA *rid=check_ROOM(LS,2);

    if (ud_obj->in_room)
        obj_from_room(ud_obj);
    else if (ud_obj->carried_by)
        obj_from_char(ud_obj);
    else if (ud_obj->in_obj)
        obj_from_obj(ud_obj);
    else
        luaL_error(LS, "No location for %s (%d)", 
                ud_obj->name, 
                ud_obj->pIndexData->vnum);

    obj_to_room(ud_obj, rid);
    return 0;
}

static int OBJ_get_inobj (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->in_obj)
        return 0;

    if ( !push_OBJ(LS, ud_obj->in_obj) )
        return 0;
    else
        return 1;
}

static int OBJ_get_carriedby (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->carried_by )
        return 0;
    else if (!push_CH( LS, ud_obj->carried_by) )
        return 0;
    else
        return 1;
}

static int OBJ_set_carriedby (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    CHAR_DATA *ch=check_CH(LS,2);

    if (ud_obj->carried_by)
        obj_from_char(ud_obj);
    else if (ud_obj->in_room)
        obj_from_room(ud_obj);
    else if (ud_obj->in_obj)
        obj_from_obj(ud_obj);
    else
        luaL_error(LS, "No location for %s (%d)",
                ud_obj->name,
                ud_obj->pIndexData->vnum);
    
    obj_to_char( ud_obj, ch );

    return 0;
}

static int OBJ_get_v0 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[0]);
    return 1;
}

static int OBJ_get_v1 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[1]);
    return 1;
}


static int OBJ_get_v2 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[2]);
    return 1;
}

static int OBJ_get_v3 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[3]);
    return 1;
}

static int OBJ_get_v4 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[4]);
    return 1;
}

static int OBJ_get_timer (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->timer);
    return 1;
}

static int OBJ_get_affects ( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    AFFECT_DATA *af;

    int index=1;
    lua_newtable( LS );

    for ( af = ud_obj->affected ; af ; af = af->next )
    {
        if (push_AFFECT( LS, af) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int OBJ_set_attacktype ( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    const char *attack_arg = check_string(LS, 2, MIL);
    int attack; 

    if (ud_obj->item_type != ITEM_WEAPON )
        return luaL_error(LS, "attacktype for weapon only.");

    attack=attack_exact_lookup(attack_arg);
    if ( attack == -1 )
        return luaL_error(LS, "No such attack type '%s'",
                attack_arg );

    ud_obj->value[3]=attack;

    return 0;
}

static const LUA_PROP_TYPE OBJ_get_table [] =
{
    { "name", OBJ_get_name, 0, STS_ACTIVE },
    { "shortdescr", OBJ_get_shortdescr, 0, STS_ACTIVE },
    { "description", OBJ_get_description, 0, STS_ACTIVE },
    { "clan", OBJ_get_clan, 0, STS_ACTIVE },
    { "clanrank", OBJ_get_clanrank, 0, STS_ACTIVE },
    { "level", OBJ_get_level, 0, STS_ACTIVE },
    { "owner", OBJ_get_owner, 0, STS_ACTIVE },
    { "cost", OBJ_get_cost, 0, STS_ACTIVE },
    { "material", OBJ_get_material, 0, STS_ACTIVE },
    { "vnum", OBJ_get_vnum, 0, STS_ACTIVE },
    { "otype", OBJ_get_otype, 0, STS_ACTIVE },
    { "weight", OBJ_get_weight, 0, STS_ACTIVE },
    { "room", OBJ_get_room, 0, STS_ACTIVE },
    { "inobj", OBJ_get_inobj, 0, STS_ACTIVE },
    { "carriedby", OBJ_get_carriedby, 0, STS_ACTIVE },
    { "v0", OBJ_get_v0, 0, STS_ACTIVE },
    { "v1", OBJ_get_v1, 0, STS_ACTIVE },
    { "v2", OBJ_get_v2, 0, STS_ACTIVE },
    { "v3", OBJ_get_v3, 0, STS_ACTIVE },
    { "v4", OBJ_get_v4, 0, STS_ACTIVE },
    { "weartype", OBJ_get_weartype, 0, STS_ACTIVE },
    { "wearlocation", OBJ_get_wearlocation, 0, STS_ACTIVE },
    { "contents", OBJ_get_contents, 0, STS_ACTIVE },
    { "proto", OBJ_get_proto, 0, STS_ACTIVE },
    { "ingame", OBJ_get_ingame, 0, STS_ACTIVE },
    { "timer", OBJ_get_timer, 0, STS_ACTIVE },
    { "affects", OBJ_get_affects, 0, STS_ACTIVE },
    
    /*light*/
    { "light", OBJ_get_light, 0, STS_ACTIVE },

    /*arrows*/
    { "arrowcount", OBJ_get_arrowcount, 0, STS_ACTIVE },
    { "arrowdamage", OBJ_get_arrowdamage, 0, STS_ACTIVE },
    { "arrowdamtype", OBJ_get_arrowdamtype, 0, STS_ACTIVE },
    
    /* wand, staff */
    { "spelllevel", OBJ_get_spelllevel, 0, STS_ACTIVE },
    { "chargestotal", OBJ_get_chargestotal, 0, STS_ACTIVE },
    { "chargesleft", OBJ_get_chargesleft, 0, STS_ACTIVE },
    { "spellname", OBJ_get_spellname, 0, STS_ACTIVE },
    
    /* portal */
    // chargesleft
    { "toroom", OBJ_get_toroom, 0, STS_ACTIVE },

    /* furniture */
    { "maxpeople", OBJ_get_maxpeople, 0, STS_ACTIVE },
    { "maxweight", OBJ_get_maxweight, 0, STS_ACTIVE },
    { "healbonus", OBJ_get_healbonus, 0, STS_ACTIVE },
    { "manabonus", OBJ_get_manabonus, 0, STS_ACTIVE },

    /* scroll, potion, pill */
    //spelllevel
    { "spells", OBJ_get_spells, 0, STS_ACTIVE },

    /* armor */
    { "ac", OBJ_get_ac, 0, STS_ACTIVE },

    /* weapon */
    { "weapontype", OBJ_get_weapontype, 0, STS_ACTIVE },
    { "numdice", OBJ_get_numdice, 0, STS_ACTIVE },
    { "dicetype", OBJ_get_dicetype, 0, STS_ACTIVE },
    { "attacktype", OBJ_get_attacktype, 0, STS_ACTIVE },
    { "damtype", OBJ_get_damtype, 0, STS_ACTIVE },
    { "damnoun", OBJ_get_damnoun, 0, STS_ACTIVE },
    { "damavg", OBJ_get_damavg, 0, STS_ACTIVE },

    /* container */
    //maxweight
    { "key", OBJ_get_key, 0, STS_ACTIVE },
    { "capacity", OBJ_get_capacity, 0, STS_ACTIVE },
    { "weightmult", OBJ_get_weightmult, 0, STS_ACTIVE },

    /* drink container */
    { "liquidtotal", OBJ_get_liquidtotal, 0, STS_ACTIVE },
    { "liquidleft", OBJ_get_liquidleft, 0, STS_ACTIVE },
    { "liquid", OBJ_get_liquid, 0, STS_ACTIVE },
    { "liquidcolor", OBJ_get_liquidcolor, 0, STS_ACTIVE },
    { "poisoned", OBJ_get_poisoned, 0, STS_ACTIVE },

    /*fountain*/
    //liquid
    //liquidleft
    //liquidtotal

    /* food */
    { "foodhours", OBJ_get_foodhours, 0, STS_ACTIVE },
    { "fullhours", OBJ_get_fullhours, 0, STS_ACTIVE },
    // poisoned
    
    /* money */
    { "silver", OBJ_get_silver, 0, STS_ACTIVE },
    { "gold", OBJ_get_gold, 0, STS_ACTIVE },
    
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE OBJ_set_table [] =
{
    { "name", OBJ_set_name, 5, STS_ACTIVE },
    { "shortdescr", OBJ_set_shortdescr, 5, STS_ACTIVE },
    { "description", OBJ_set_description, 5, STS_ACTIVE },
    { "level", OBJ_set_level, 5, STS_ACTIVE },
    { "owner", OBJ_set_owner, 5, STS_ACTIVE },
    { "material", OBJ_set_material, 5, STS_ACTIVE },
    { "weight", OBJ_set_weight, 5, STS_ACTIVE },
    { "room", OBJ_set_room, 5, STS_ACTIVE },
    { "carriedby", OBJ_set_carriedby, 5, STS_ACTIVE },
    { "attacktype", OBJ_set_attacktype, 5, STS_ACTIVE },
    { "weapontype", OBJ_set_weapontype, 9, STS_ACTIVE },
       
    { NULL, NULL, 0, 0 }
};


static const LUA_PROP_TYPE OBJ_method_table [] =
{
    { "extra", OBJ_extra, 0, STS_ACTIVE },
    { "apply", OBJ_apply, 0, STS_ACTIVE },
    { "destroy", OBJ_destroy, 1, STS_ACTIVE },
    { "clone", OBJ_clone, 1, STS_ACTIVE },
    { "echo", OBJ_echo, 1, STS_ACTIVE },
    { "loadprog", OBJ_loadprog, 1, STS_ACTIVE },
    { "loadscript", OBJ_loadscript, 1, STS_ACTIVE },
    { "loadstring", OBJ_loadstring, 1, STS_ACTIVE },
    { "loadfunction", OBJ_loadfunction, 1, STS_ACTIVE },
    { "oload", OBJ_oload, 1, STS_ACTIVE },
    { "savetbl", OBJ_savetbl, 1, STS_ACTIVE },
    { "loadtbl", OBJ_loadtbl, 1, STS_ACTIVE },
    { "tprint", OBJ_tprint, 1, STS_ACTIVE },
    { "delay", OBJ_delay, 1, STS_ACTIVE },
    { "cancel", OBJ_cancel, 1, STS_ACTIVE },
    { "setval", OBJ_setval, 1, STS_ACTIVE },
    { "getval", OBJ_getval, 1, STS_ACTIVE },
    { "rvnum", OBJ_rvnum, 1, STS_ACTIVE },
    
    /* portal only */
    { "exitflag", OBJ_exitflag, 0, STS_ACTIVE },
    { "setexitflag", OBJ_setexitflag, 1, STS_ACTIVE },
    { "portalflag", OBJ_portalflag, 0, STS_ACTIVE },

    /* furniture only */
    { "furnitureflag", OBJ_furnitureflag, 0, STS_ACTIVE },
    
    /* weapon only */
    { "weaponflag", OBJ_weaponflag, 0, STS_ACTIVE },
    { "setweaponflag", OBJ_setweaponflag, 5, STS_ACTIVE },
    
    /* container only */
    { "containerflag", OBJ_containerflag, 0, STS_ACTIVE },
    
    { NULL, NULL, 0, 0 }
}; 

/* end OBJ section */

/* AREA section */
static int AREA_rvnum ( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS, 1);
    lua_remove(LS,1);

    return L_rvnum( LS, ud_area );
}

static int AREA_loadfunction( lua_State *LS)
{
    lua_area_program( LS, NULL, RUNDELAY_VNUM, NULL,
                check_AREA(LS, -2), NULL,
                TRIG_CALL, 0 );
    return 0;
}

static int AREA_delay (lua_State *LS)
{
    return L_delay(LS);
}

static int AREA_cancel (lua_State *LS)
{
    return L_cancel(LS);
}

static int AREA_savetbl (lua_State *LS)
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

static int AREA_loadtbl (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int AREA_loadscript (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular aprog with vnum 0*/
    lua_pushboolean( LS,
            lua_area_program( LS, NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH), ud_area, NULL, ATRIG_CALL, 0) );

    return 1;
}

static int AREA_loadstring (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);
    lua_pushboolean( LS,
            lua_area_program( LS, NULL, LOADSCRIPT_VNUM, check_string( LS, 2, MAX_SCRIPT_LENGTH), ud_area, NULL, ATRIG_CALL, 0) );
    return 1;
}

static int AREA_loadprog (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    PROG_CODE *pAcode;

    if ( (pAcode = get_aprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: aprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_area_program( LS, NULL, num, pAcode->code, ud_area, NULL, ATRIG_CALL, 0) );

    return 1;
}

static int AREA_flag( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS, 1);
    return check_flag( LS, "area", area_flags, ud_area->area_flags );
}

static int AREA_reset( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS,1);
    reset_area(ud_area);
    return 0;
}

static int AREA_purge( lua_State *LS)
{
    purge_area( check_AREA(LS,1) );
    return 0;
}

static int AREA_echo( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS, 1);
    const char *argument = check_fstring( LS, 2, MSL);
    DESCRIPTOR_DATA *d;

    for ( d = descriptor_list; d; d = d->next )
    {
        if ( IS_PLAYING(d->connected) )
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

static int AREA_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    lua_insert( LS, 2);
    /* original arg 2 is pushed to index 3 */
    if (lua_isnone(LS,3))
    {
        lua_pushnil(LS);
    }
    
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, AREA_echo );
    /* now line up arguments for echo */
    lua_pushvalue( LS, 1); /* area */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;

}

static int AREA_get_name( lua_State *LS)
{
    lua_pushstring( LS, (check_AREA(LS, 1))->name);
    return 1;
}

static int AREA_get_filename( lua_State *LS)
{
    lua_pushstring( LS, (check_AREA(LS, 1))->file_name);
    return 1;
}

static int AREA_get_nplayer( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->nplayer);
    return 1;
}

static int AREA_get_minlevel( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->minlevel);
    return 1;
}

static int AREA_get_maxlevel( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->maxlevel);
    return 1;
}

static int AREA_get_security( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->security);
    return 1;
}

static int AREA_get_ingame( lua_State *LS)
{
    lua_pushboolean( LS, is_area_ingame(check_AREA(LS, 1)));
    return 1;
}

static int AREA_get_rooms( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    lua_newtable(LS);
    ROOM_INDEX_DATA *room;
    int vnum;
    for (vnum=ud_area->min_vnum ; vnum<=ud_area->max_vnum ; vnum++)
    {
        if ((room=get_room_index(vnum))==NULL)
            continue;
        if (push_ROOM(LS, room))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_people( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=char_list ; people ; people=people->next)
    {
        if ( !people || !people->in_room
                || (people->in_room->area != ud_area) )
            continue;
        if (push_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_players( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=char_list ; people ; people=people->next)
    {
        if ( IS_NPC(people)
                || !people || !people->in_room
                || (people->in_room->area != ud_area) )
            continue;
        if (push_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_mobs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=char_list ; people ; people=people->next)
    {
        if ( !IS_NPC(people)
                || !people || !people->in_room
                || (people->in_room->area != ud_area) )
            continue;
        if (push_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_mobprotos( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    MOB_INDEX_DATA *mid;
    for ( vnum=ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((mid=get_mob_index(vnum)) != NULL )
        {
            if (push_MOBPROTO(LS, mid))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_objprotos( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    OBJ_INDEX_DATA *oid;
    for ( vnum=ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((oid=get_obj_index(vnum)) != NULL )
        {
            if (push_OBJPROTO(LS, oid))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_mprogs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    PROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_mprog_index(vnum)) != NULL )
        {
            if (push_PROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_oprogs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    PROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_oprog_index(vnum)) != NULL )
        {
            if (push_PROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_aprogs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    PROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_aprog_index(vnum)) != NULL )
        {
            if (push_PROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_rprogs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    PROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_rprog_index(vnum)) != NULL )
        {
            if (push_PROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_atrigs ( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA( LS, 1);
    PROG_LIST *atrig;

    int index=1;
    lua_newtable( LS );

    for ( atrig = ud_area->aprogs ; atrig ; atrig = atrig->next )
    {
        if (push_ATRIG( LS, atrig) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_vnum ( lua_State *LS)
{
    lua_pushinteger( LS,
            (check_AREA(LS,1))->vnum);
    return 1;
}

static int AREA_get_minvnum ( lua_State *LS)
{
    lua_pushinteger( LS,
            (check_AREA(LS,1))->min_vnum);
    return 1;
}

static int AREA_get_maxvnum ( lua_State *LS)
{
    lua_pushinteger( LS,
            (check_AREA(LS,1))->max_vnum);
    return 1;
}

static int AREA_get_credits ( lua_State *LS)
{
    lua_pushstring( LS,
            (check_AREA(LS,1))->credits);
    return 1;
}

static int AREA_get_builders ( lua_State *LS)
{
    lua_pushstring( LS,
            (check_AREA(LS,1))->builders);
    return 1;
}

static const LUA_PROP_TYPE AREA_get_table [] =
{
    { "name", AREA_get_name, 0, STS_ACTIVE },
    { "filename", AREA_get_filename, 0, STS_ACTIVE },
    { "nplayer", AREA_get_nplayer, 0, STS_ACTIVE },
    { "minlevel", AREA_get_minlevel, 0, STS_ACTIVE },
    { "maxlevel", AREA_get_maxlevel, 0, STS_ACTIVE },
    { "security", AREA_get_security, 0, STS_ACTIVE },
    { "vnum", AREA_get_vnum, 0, STS_ACTIVE },
    { "minvnum", AREA_get_minvnum, 0, STS_ACTIVE },
    { "maxvnum", AREA_get_maxvnum, 0, STS_ACTIVE },
    { "credits", AREA_get_credits, 0, STS_ACTIVE },
    { "builders", AREA_get_builders, 0, STS_ACTIVE },
    { "ingame", AREA_get_ingame, 0, STS_ACTIVE },
    { "rooms", AREA_get_rooms, 0, STS_ACTIVE },
    { "people", AREA_get_people, 0, STS_ACTIVE },
    { "players", AREA_get_players, 0, STS_ACTIVE },
    { "mobs", AREA_get_mobs, 0, STS_ACTIVE },
    { "mobprotos", AREA_get_mobprotos, 0, STS_ACTIVE },
    { "objprotos", AREA_get_objprotos, 0, STS_ACTIVE },
    { "mprogs", AREA_get_mprogs, 0, STS_ACTIVE },
    { "oprogs", AREA_get_oprogs, 0, STS_ACTIVE },
    { "aprogs", AREA_get_aprogs, 0, STS_ACTIVE },
    { "rprogs", AREA_get_rprogs, 0, STS_ACTIVE },
    { "atrigs", AREA_get_atrigs, 0, STS_ACTIVE },
    
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE AREA_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE AREA_method_table [] =
{
    { "flag", AREA_flag, 0, STS_ACTIVE },
    { "echo", AREA_echo, 1, STS_ACTIVE },
    { "reset", AREA_reset, 5, STS_ACTIVE },
    { "purge", AREA_purge, 5, STS_ACTIVE },
    { "loadprog", AREA_loadprog, 1, STS_ACTIVE },
    { "loadscript", AREA_loadscript, 1, STS_ACTIVE },
    { "loadstring", AREA_loadstring, 1, STS_ACTIVE },
    { "loadfunction", AREA_loadfunction, 1, STS_ACTIVE },
    { "savetbl", AREA_savetbl, 1, STS_ACTIVE },
    { "loadtbl", AREA_loadtbl, 1, STS_ACTIVE },
    { "tprint", AREA_tprint, 1, STS_ACTIVE },
    { "delay", AREA_delay, 1, STS_ACTIVE },
    { "cancel", AREA_cancel, 1, STS_ACTIVE },
    { "rvnum", AREA_rvnum, 1, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
}; 

/* end AREA section */

/* ROOM section */
static int ROOM_rvnum ( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_remove(LS,1);

    return L_rvnum( LS, ud_room->area );
}

static int ROOM_loadfunction ( lua_State *LS)
{
    lua_room_program( LS, NULL, RUNDELAY_VNUM, NULL,
                check_ROOM(LS, -2), NULL,
                NULL, NULL, NULL, NULL,
                TRIG_CALL, 0 );
    return 0;
}

static int ROOM_mload (lua_State *LS)
{
    ROOM_INDEX_DATA * ud_room = check_ROOM (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    MOB_INDEX_DATA *pObjIndex = get_mob_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No mob with vnum: %d", num);

    CHAR_DATA *mob=create_mobile( pObjIndex);
    arm_npc( mob );
    char_to_room(mob,ud_room);

    if ( !push_CH(LS, mob))
        return 0;
    else
        return 1;

}

static int ROOM_oload (lua_State *LS)
{
    ROOM_INDEX_DATA * ud_room = check_ROOM (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj = create_object(pObjIndex);
    check_enchant_obj( obj );
    obj_to_room(obj,ud_room);

    if ( !push_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}

static int ROOM_flag( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room = check_ROOM(LS, 1);
    return check_flag( LS, "room", room_flags, ud_room->room_flags );
}

static int ROOM_reset( lua_State *LS)
{
    reset_room( check_ROOM(LS,1) );
    return 0;
}

static int ROOM_purge( lua_State *LS)
{
    purge_room( check_ROOM(LS,1) );
    return 0;
}

static int ROOM_echo( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room = check_ROOM(LS, 1);
    const char *argument = check_fstring( LS, 2, MSL);

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

static int ROOM_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    lua_insert( LS, 2);
    /* original arg 2 is pushed to index 3 */
    if (lua_isnone(LS,3))
    {
        lua_pushnil(LS);
    }
    
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, ROOM_echo );
    /* now line up argumenets for echo */
    lua_pushvalue( LS, 1); /* obj */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;
}

static int ROOM_savetbl (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, SAVETABLE_FUNCTION);

    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_pushstring( LS, ud_room->area->file_name );
    lua_call( LS, 3, 0);

    return 0;
}

static int ROOM_loadtbl (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_room->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int ROOM_loadscript (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    lua_pushboolean( LS,
            lua_room_program( LS, NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH),
                ud_room, NULL, NULL, NULL, NULL, NULL, RTRIG_CALL, 0) );
    return 1;
}

static int ROOM_loadstring (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_pushboolean( LS,
            lua_room_program( LS, NULL, LOADSCRIPT_VNUM, check_string(LS, 2, MAX_SCRIPT_LENGTH),
                ud_room, NULL, NULL, NULL, NULL, NULL, RTRIG_CALL, 0) );
    return 1;
}

static int ROOM_loadprog (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int num = (int)luaL_checknumber (LS, 2);
    PROG_CODE *pRcode;

    if ( (pRcode = get_rprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: rprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_room_program( LS, NULL, num, pRcode->code,
                ud_room, NULL, NULL, NULL, NULL, NULL,
                RTRIG_CALL, 0) );
    return 1;
}

static int ROOM_delay (lua_State *LS)
{
    return L_delay(LS);
}

static int ROOM_cancel (lua_State *LS)
{
    return L_cancel(LS);
}

static int ROOM_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->name);
    return 1;
}

static int ROOM_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->vnum);
    return 1;
}

static int ROOM_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[check_ROOM(LS,1)->clan].name);
    return 1;
}

static int ROOM_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->clan_rank);
    return 1;
}

static int ROOM_get_healrate (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->heal_rate);
    return 1;
}

static int ROOM_get_manarate (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->mana_rate);
    return 1;
}

static int ROOM_get_owner (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->owner);
    return 1;
}

static int ROOM_get_description (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->description);
    return 1;
}

static int ROOM_get_sector (lua_State *LS)
{
    lua_pushstring( LS,
            flag_bit_name(sector_flags, (check_ROOM(LS,1))->sector_type) );
    return 1;
}

static int ROOM_get_contents (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_room->contents ; obj ; obj=obj->next_content)
    {
        if (push_OBJ(LS, obj))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_area (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    if ( !push_AREA(LS, ud_room->area))
        return 0;
    else
        return 1;
}

static int ROOM_get_people (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=ud_room->people ; people ; people=people->next_in_room)
    {
        if (push_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_players (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *plr;
    for ( plr=ud_room->people ; plr ; plr=plr->next_in_room)
    {
        if (!IS_NPC(plr) && push_CH(LS, plr))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_mobs (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *mob;
    for ( mob=ud_room->people ; mob ; mob=mob->next_in_room)
    {
        if ( IS_NPC(mob) && push_CH(LS, mob))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_exits (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_newtable(LS);
    int i;
    int index=1;
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

static int ROOM_get_dir_helper( lua_State *LS, int dirnumber )
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    if (!ud_room->exit[dirnumber])
        return 0;
    if (!push_EXIT(LS, ud_room->exit[dirnumber]))
        return 0;
    else
        return 1;
}
static int ROOM_get_north( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_NORTH ); }
static int ROOM_get_south( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_SOUTH ); }
static int ROOM_get_east( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_EAST ); }
static int ROOM_get_west( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_WEST ); }
static int ROOM_get_northeast( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_NORTHEAST ); }
static int ROOM_get_northwest( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_NORTHWEST ); }
static int ROOM_get_southeast( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_SOUTHEAST ); }
static int ROOM_get_southwest( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_SOUTHWEST ); }
static int ROOM_get_up( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_UP ); }
static int ROOM_get_down( lua_State *LS ) { return ROOM_get_dir_helper( LS, DIR_DOWN ); }


static int ROOM_get_resets (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_newtable(LS);
    int index=1;
    RESET_DATA *reset;
    for ( reset=ud_room->reset_first; reset; reset=reset->next)
    {
        if (push_RESET(LS, reset) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_ingame( lua_State *LS )
{
    lua_pushboolean( LS,
            is_room_ingame( check_ROOM(LS,1) ) );
    return 1;
}

static int ROOM_get_rtrigs ( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM( LS, 1);
    PROG_LIST *rtrig;

    int index=1;
    lua_newtable( LS );

    for ( rtrig = ud_room->rprogs ; rtrig ; rtrig = rtrig->next )
    {
        if (push_RTRIG( LS, rtrig) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static const LUA_PROP_TYPE ROOM_get_table [] =
{
    { "name", ROOM_get_name, 0, STS_ACTIVE },
    { "vnum", ROOM_get_vnum, 0, STS_ACTIVE },
    { "clan", ROOM_get_clan, 0, STS_ACTIVE },
    { "clanrank", ROOM_get_clanrank, 0, STS_ACTIVE },
    { "healrate", ROOM_get_healrate, 0, STS_ACTIVE },
    { "manarate", ROOM_get_manarate, 0, STS_ACTIVE },
    { "owner", ROOM_get_owner, 0, STS_ACTIVE },
    { "description", ROOM_get_description, 0, STS_ACTIVE },
    { "ingame", ROOM_get_ingame, 0, STS_ACTIVE },
    { "sector", ROOM_get_sector, 0, STS_ACTIVE },
    { "contents", ROOM_get_contents, 0, STS_ACTIVE },
    { "area", ROOM_get_area, 0, STS_ACTIVE },
    { "people", ROOM_get_people, 0, STS_ACTIVE },
    { "players", ROOM_get_players, 0, STS_ACTIVE },
    { "mobs", ROOM_get_mobs, 0, STS_ACTIVE },
    { "exits", ROOM_get_exits, 0, STS_ACTIVE },
    { "north", ROOM_get_north, 0, STS_ACTIVE },
    { "south", ROOM_get_south, 0, STS_ACTIVE },
    { "east", ROOM_get_east, 0, STS_ACTIVE },
    { "west", ROOM_get_west, 0, STS_ACTIVE },
    { "northwest", ROOM_get_northwest, 0, STS_ACTIVE },
    { "northeast", ROOM_get_northeast, 0, STS_ACTIVE },
    { "southwest", ROOM_get_southwest, 0, STS_ACTIVE },
    { "southeast", ROOM_get_southeast, 0, STS_ACTIVE },
    { "up", ROOM_get_up, 0, STS_ACTIVE },
    { "down", ROOM_get_down, 0, STS_ACTIVE },
    { "resets", ROOM_get_resets, 0, STS_ACTIVE },
    { "rtrigs", ROOM_get_rtrigs, 0, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE ROOM_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE ROOM_method_table [] =
{
    { "flag", ROOM_flag, 0, STS_ACTIVE },
    { "reset", ROOM_reset, 5, STS_ACTIVE },
    { "purge", ROOM_purge, 5, STS_ACTIVE },
    { "oload", ROOM_oload, 1, STS_ACTIVE },
    { "mload", ROOM_mload, 1, STS_ACTIVE },
    { "echo", ROOM_echo, 1, STS_ACTIVE },
    { "loadprog", ROOM_loadprog, 1, STS_ACTIVE },
    { "loadscript", ROOM_loadscript, 1, STS_ACTIVE },
    { "loadstring", ROOM_loadstring, 1, STS_ACTIVE },
    { "loadfunction", ROOM_loadfunction, 1, STS_ACTIVE },
    { "tprint", ROOM_tprint, 1, STS_ACTIVE },
    { "delay", ROOM_delay, 1, STS_ACTIVE },
    { "cancel", ROOM_cancel, 1, STS_ACTIVE },
    { "savetbl", ROOM_savetbl, 1, STS_ACTIVE },
    { "loadtbl", ROOM_loadtbl, 1, STS_ACTIVE },
    { "rvnum", ROOM_rvnum, 1, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
}; 

/* end ROOM section */

/* EXIT section */
static int EXIT_flag (lua_State *LS)
{
    EXIT_DATA *ed=check_EXIT( LS, 1 );
    return check_flag( LS, "exit", exit_flags, ed->exit_info );
}

static int EXIT_setflag( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);
    return set_flag( LS, "exit", exit_flags, ud_exit->exit_info); 
}

static int EXIT_lock( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);

    if (!IS_SET(ud_exit->exit_info, EX_ISDOOR))
    {
        luaL_error(LS, "Exit is not a door, cannot lock.");
    }

    /* force closed if necessary */
    SET_BIT(ud_exit->exit_info, EX_CLOSED);
    SET_BIT(ud_exit->exit_info, EX_LOCKED);
    return 0;
}

static int EXIT_unlock( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);

    if (!IS_SET(ud_exit->exit_info, EX_ISDOOR))
    {
        luaL_error(LS, "Exit is not a door, cannot unlock.");
    }

    REMOVE_BIT(ud_exit->exit_info, EX_LOCKED);
    return 0;
}

static int EXIT_close( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);

    if (!IS_SET(ud_exit->exit_info, EX_ISDOOR))
    {
        luaL_error(LS, "Exit is not a door, cannot close.");
    }

    SET_BIT(ud_exit->exit_info, EX_CLOSED);
    return 0;
}

static int EXIT_open( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);

    if (!IS_SET(ud_exit->exit_info, EX_ISDOOR))
    {
        luaL_error(LS, "Exit is not a door, cannot open.");
    }

    /* force unlock if necessary */
    REMOVE_BIT(ud_exit->exit_info, EX_LOCKED);
    REMOVE_BIT(ud_exit->exit_info, EX_CLOSED);

    return 0;
}

static int EXIT_get_toroom (lua_State *LS)
{
    EXIT_DATA *ud_exit=check_EXIT(LS,1);
    if ( !push_ROOM( LS, ud_exit->u1.to_room ))
        return 0;
    else
        return 1;
}

static int EXIT_get_keyword (lua_State *LS)
{
    lua_pushstring(LS,
            (check_EXIT(LS,1))->keyword);
    return 1;
}

static int EXIT_get_description (lua_State *LS)
{
    lua_pushstring(LS,
            (check_EXIT(LS,1))->description);
    return 1;
}

static int EXIT_get_key (lua_State *LS)
{
    lua_pushinteger(LS,
            (check_EXIT(LS,1))->key);
    return 1;
}

static const LUA_PROP_TYPE EXIT_get_table [] =
{
    { "toroom", EXIT_get_toroom, 0, STS_ACTIVE },
    { "keyword", EXIT_get_keyword, 0, STS_ACTIVE },
    { "description", EXIT_get_description, 0, STS_ACTIVE },
    { "key", EXIT_get_key, 0, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE EXIT_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE EXIT_method_table [] =
{
    { "flag", EXIT_flag, 0, STS_ACTIVE },
    { "setflag", EXIT_setflag, 1, STS_ACTIVE },
    { "open", EXIT_open, 1, STS_ACTIVE },
    { "close", EXIT_close, 1, STS_ACTIVE },
    { "unlock", EXIT_unlock, 1, STS_ACTIVE },
    { "lock", EXIT_lock, 1, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
}; 

/* end EXIT section */

/* RESET section */
static int RESET_get_command(lua_State *LS)
{
    RESET_DATA *rd = check_RESET(LS, 1);
    static char buf[2];
    sprintf(buf, "%c", rd->command);
    lua_pushstring(LS, buf);
    return 1;
}

static int RESET_get_arg1( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_RESET(LS,1))->arg1 );
    return 1;
}
static int RESET_get_arg2( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_RESET(LS,1))->arg2 );
    return 1;
}
static int RESET_get_arg3( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_RESET(LS,1))->arg3 );
    return 1;
}
static int RESET_get_arg4( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_RESET(LS,1))->arg4 );
    return 1;
}

static const LUA_PROP_TYPE RESET_get_table [] =
{
    { "command", RESET_get_command, 0, STS_ACTIVE },
    { "arg1", RESET_get_arg1, 0, STS_ACTIVE },
    { "arg2", RESET_get_arg2, 0, STS_ACTIVE },
    { "arg3", RESET_get_arg3, 0, STS_ACTIVE },
    { "arg4", RESET_get_arg4, 0, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE RESET_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE RESET_method_table [] =
{
    { NULL, NULL, 0, 0 }
}; 

/* end RESET section */

/* OBJPROTO section */
static int OBJPROTO_adjustdamage( lua_State *LS)
{

    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    if ( ud_objp->item_type != ITEM_WEAPON )
        luaL_error( LS, "adjustdamage for weapon only");

    lua_pushboolean( LS, adjust_weapon_dam( ud_objp ) );
    return 1;
}

static int OBJPROTO_extra( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    return check_flag( LS, "extra", extra_flags, ud_objp->extra_flags );
}

static int OBJPROTO_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->name);
    return 1;
}

static int OBJPROTO_get_shortdescr (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->short_descr);
    return 1;
}

static int OBJPROTO_get_description (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->description);
    return 1;
}

static int OBJPROTO_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_OBJPROTO(LS,1))->clan].name);
    return 1;
}

static int OBJPROTO_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->rank);
    return 1;
}

static int OBJPROTO_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->level);
    return 1;
}

static int OBJPROTO_get_cost (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->cost);
    return 1;
}

static int OBJPROTO_get_material (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->material);
    return 1;
}

static int OBJPROTO_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->vnum);
    return 1;
}

static int OBJPROTO_get_otype (lua_State *LS)
{
    lua_pushstring( LS,
            item_name((check_OBJPROTO(LS,1))->item_type));
    return 1;
}

static int OBJPROTO_get_weight (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->weight);
    return 1;
}

#define OPGETV( num ) static int OBJPROTO_get_v ## num (lua_State *LS)\
{\
    lua_pushinteger( LS,\
            (check_OBJPROTO(LS,1))->value[num]);\
    return 1;\
}

OPGETV(0);
OPGETV(1);
OPGETV(2);
OPGETV(3);
OPGETV(4);

static int OBJPROTO_get_ingame ( lua_State *LS )
{
    lua_pushboolean( LS,
            is_obj_ingame( check_OBJPROTO(LS,1) ) );
    return 1;
}

static int OBJPROTO_get_area ( lua_State *LS )
{
    if (push_AREA(LS, (check_OBJPROTO(LS,1))->area) )
        return 1;
    return 0;
}

static int OBJPROTO_get_otrigs ( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_oid=check_OBJPROTO( LS, 1);
    PROG_LIST *otrig;

    int index=1;
    lua_newtable( LS );

    for ( otrig = ud_oid->oprogs ; otrig ; otrig = otrig->next )
    {
        if (push_OTRIG( LS, otrig) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int OBJPROTO_get_affects ( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_oid=check_OBJPROTO( LS, 1);
    AFFECT_DATA *af;
    
    int index=1;
    lua_newtable( LS );

    for ( af = ud_oid->affected ; af ; af = af->next )
    {
        if (push_AFFECT( LS, af) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int OBJPROTO_get_rating ( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_oid=check_OBJPROTO( LS, 1);

    lua_pushinteger( LS,
            ud_oid->diff_rating);

    return 1;
}

static const LUA_PROP_TYPE OBJPROTO_get_table [] =
{
    { "name", OBJPROTO_get_name, 0, STS_ACTIVE },
    { "shortdescr", OBJPROTO_get_shortdescr, 0, STS_ACTIVE },
    { "description", OBJPROTO_get_description, 0, STS_ACTIVE },
    { "clan", OBJPROTO_get_clan, 0, STS_ACTIVE },
    { "clanrank", OBJPROTO_get_clanrank, 0, STS_ACTIVE },
    { "level", OBJPROTO_get_level, 0, STS_ACTIVE },
    { "cost", OBJPROTO_get_cost, 0, STS_ACTIVE },
    { "material", OBJPROTO_get_material, 0, STS_ACTIVE },
    { "vnum", OBJPROTO_get_vnum, 0, STS_ACTIVE },
    { "otype", OBJPROTO_get_otype, 0, STS_ACTIVE },
    { "weartype", OBJPROTO_get_weartype, 0, STS_ACTIVE },
    { "weight", OBJPROTO_get_weight, 0, STS_ACTIVE },
    { "rating", OBJPROTO_get_rating, 0, STS_ACTIVE },
    { "v0", OBJPROTO_get_v0, 0, STS_ACTIVE },
    { "v1", OBJPROTO_get_v1, 0, STS_ACTIVE },
    { "v2", OBJPROTO_get_v2, 0, STS_ACTIVE },
    { "v3", OBJPROTO_get_v3, 0, STS_ACTIVE },
    { "v4", OBJPROTO_get_v4, 0, STS_ACTIVE },
    { "area", OBJPROTO_get_area, 0, STS_ACTIVE },
    { "ingame", OBJPROTO_get_ingame, 0, STS_ACTIVE },
    { "otrigs", OBJPROTO_get_otrigs, 0, STS_ACTIVE },
    { "affects", OBJPROTO_get_affects, 0, STS_ACTIVE },

    /*light*/
    { "light", OBJPROTO_get_light, 0, STS_ACTIVE },

    /*arrows*/
    { "arrowcount", OBJPROTO_get_arrowcount, 0, STS_ACTIVE },
    { "arrowdamage", OBJPROTO_get_arrowdamage, 0, STS_ACTIVE },
    { "arrowdamtype", OBJPROTO_get_arrowdamtype, 0, STS_ACTIVE },
    
    /* wand, staff */
    { "spelllevel", OBJPROTO_get_spelllevel, 0, STS_ACTIVE },
    { "chargestotal", OBJPROTO_get_chargestotal, 0, STS_ACTIVE },
    { "chargesleft", OBJPROTO_get_chargesleft, 0, STS_ACTIVE },
    { "spellname", OBJPROTO_get_spellname, 0, STS_ACTIVE },
    
    /* portal */
    // chargesleft
    { "toroom", OBJPROTO_get_toroom, 0, STS_ACTIVE },

    /* furniture */
    { "maxpeople", OBJPROTO_get_maxpeople, 0, STS_ACTIVE },
    { "maxweight", OBJPROTO_get_maxweight, 0, STS_ACTIVE },
    { "healbonus", OBJPROTO_get_healbonus, 0, STS_ACTIVE },
    { "manabonus", OBJPROTO_get_manabonus, 0, STS_ACTIVE },

    /* scroll, potion, pill */
    //spelllevel
    { "spells", OBJPROTO_get_spells, 0, STS_ACTIVE },

    /* armor */
    { "ac", OBJPROTO_get_ac, 0, STS_ACTIVE },

    /* weapon */
    { "weapontype", OBJPROTO_get_weapontype, 0, STS_ACTIVE },
    { "numdice", OBJPROTO_get_numdice, 0, STS_ACTIVE },
    { "dicetype", OBJPROTO_get_dicetype, 0, STS_ACTIVE },
    { "attacktype", OBJPROTO_get_attacktype, 0, STS_ACTIVE },
    { "damtype", OBJPROTO_get_damtype, 0, STS_ACTIVE },
    { "damnoun", OBJPROTO_get_damnoun, 0, STS_ACTIVE },
    { "damavg", OBJPROTO_get_damavg, 0, STS_ACTIVE },

    /* container */
    //maxweight
    { "key", OBJPROTO_get_key, 0, STS_ACTIVE },
    { "capacity", OBJPROTO_get_capacity, 0, STS_ACTIVE },
    { "weightmult", OBJPROTO_get_weightmult, 0, STS_ACTIVE },

    /* drink container */
    { "liquidtotal", OBJPROTO_get_liquidtotal, 0, STS_ACTIVE },
    { "liquidleft", OBJPROTO_get_liquidleft, 0, STS_ACTIVE },
    { "liquid", OBJPROTO_get_liquid, 0, STS_ACTIVE },
    { "liquidcolor", OBJPROTO_get_liquidcolor, 0, STS_ACTIVE },
    { "poisoned", OBJPROTO_get_poisoned, 0, STS_ACTIVE },

    /*fountain*/
    //liquid
    //liquidleft
    //liquidtotal

    /* food */
    { "foodhours", OBJPROTO_get_foodhours, 0, STS_ACTIVE },
    { "fullhours", OBJPROTO_get_fullhours, 0, STS_ACTIVE },
    // poisoned
    
    /* money */
    { "silver", OBJPROTO_get_silver, 0, STS_ACTIVE },
    { "gold", OBJPROTO_get_gold, 0, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE OBJPROTO_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE OBJPROTO_method_table [] =
{
    { "extra", OBJPROTO_extra, 0, STS_ACTIVE },
    { "apply", OBJPROTO_apply, 0, STS_ACTIVE },
   
    /* portal only */
    { "exitflag", OBJPROTO_exitflag, 0, STS_ACTIVE },
    { "portalflag", OBJPROTO_portalflag, 0, STS_ACTIVE },
    
    /* furniture only */
    { "furnitureflag", OBJPROTO_furnitureflag, 0, STS_ACTIVE },
    
    /* weapon only */
    { "weaponflag", OBJPROTO_weaponflag, 0, STS_ACTIVE },
    { "adjustdamage", OBJPROTO_adjustdamage, 9, STS_ACTIVE },
    
    /* container only */
    { "containerflag", OBJPROTO_containerflag, 0, STS_ACTIVE },
    
    { NULL, NULL, 0, 0 }
}; 

/* end OBJPROTO section */

/* MOBPROTO section */
static int MOBPROTO_affected (lua_State *LS)
{
    MOB_INDEX_DATA *ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "affected", affect_flags, ud_mobp->affect_field );
}

static int MOBPROTO_act (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "act", act_flags, ud_mobp->act );
}

static int MOBPROTO_offensive (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "offensive", off_flags, ud_mobp->off_flags );
}

static int MOBPROTO_immune (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "immune", imm_flags, ud_mobp->imm_flags );
}

static int MOBPROTO_vuln (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "vuln", vuln_flags, ud_mobp->vuln_flags );
}

static int MOBPROTO_resist (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "resist", res_flags, ud_mobp->res_flags );
}

#define MPGETSTR( field, val, hsumm, hinfo ) static int MOBPROTO_get_ ## field (lua_State *LS)\
{\
    MOB_INDEX_DATA *ud_mobp=check_MOBPROTO(LS,1);\
    lua_pushstring(LS, val );\
    return 1;\
}

#define MPGETINT( field, val, hsumm, hinfo ) static int MOBPROTO_get_ ## field (lua_State *LS)\
{\
    MOB_INDEX_DATA *ud_mobp=check_MOBPROTO(LS,1);\
    lua_pushinteger(LS, val );\
    return 1;\
}

#define MPGETBOOL( field, val, hsumm, hinfo ) static int MOBPROTO_get_ ## field (lua_State *LS)\
{\
    MOB_INDEX_DATA *ud_mobp=check_MOBPROTO(LS,1);\
    lua_pushboolean(LS, val );\
    return 1;\
}

MPGETINT( vnum, ud_mobp->vnum ,"" ,"" );
MPGETSTR( name, ud_mobp->player_name , "" ,"");
MPGETSTR( shortdescr, ud_mobp->short_descr,"" ,"");
MPGETSTR( longdescr, ud_mobp->long_descr,"" ,"");
MPGETSTR( description, ud_mobp->description, "", "");
MPGETINT( alignment, ud_mobp->alignment,"" ,"");
MPGETINT( level, ud_mobp->level,"" ,"");
MPGETINT( hppcnt, ud_mobp->hitpoint_percent,"" ,"");
MPGETINT( mnpcnt, ud_mobp->mana_percent,"" ,"");
MPGETINT( mvpcnt, ud_mobp->move_percent,"" ,"");
MPGETINT( hrpcnt, ud_mobp->hitroll_percent,"" ,"");
MPGETINT( drpcnt, ud_mobp->damage_percent,"" ,"");
MPGETINT( acpcnt, ud_mobp->ac_percent,"" ,"");
MPGETINT( savepcnt, ud_mobp->saves_percent,"" ,"");
MPGETSTR( damtype, attack_table[ud_mobp->dam_type].name,"" ,"");
MPGETSTR( startpos, flag_bit_name(position_flags, ud_mobp->start_pos),"" ,"");
MPGETSTR( defaultpos, flag_bit_name(position_flags, ud_mobp->default_pos),"" ,"");
MPGETSTR( sex,
    ud_mobp->sex == SEX_NEUTRAL ? "neutral" :
    ud_mobp->sex == SEX_MALE    ? "male" :
    ud_mobp->sex == SEX_FEMALE  ? "female" :
    ud_mobp->sex == SEX_BOTH    ? "random" :
    NULL,"" ,"");
MPGETSTR( race, race_table[ud_mobp->race].name,"" ,"");
MPGETINT( wealthpcnt, ud_mobp->wealth_percent,"" ,"");
MPGETSTR( size, flag_bit_name(size_flags, ud_mobp->size),"" ,"");
MPGETSTR( stance, stances[ud_mobp->stance].name,
    "Mob's default stance." ,
    "See 'stances' table.");
MPGETBOOL( ingame, is_mob_ingame( ud_mobp ),"" ,"");
MPGETINT( count, ud_mobp->count, "", "");

static int MOBPROTO_get_area (lua_State *LS)
{
    if (push_AREA( LS, (check_MOBPROTO( LS, 1))->area))
        return 1;
    return 0;
}

static int MOBPROTO_get_mtrigs ( lua_State *LS)
{
    MOB_INDEX_DATA *ud_mid=check_MOBPROTO( LS, 1);
    PROG_LIST *mtrig;
    
    int index=1;
    lua_newtable( LS );

    for ( mtrig = ud_mid->mprogs ; mtrig ; mtrig = mtrig->next )
    {
        if (push_MTRIG( LS, mtrig) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int MOBPROTO_get_shop ( lua_State *LS)
{
    MOB_INDEX_DATA *ud_mid=check_MOBPROTO( LS, 1);
    if ( ud_mid->pShop )
    {
        if ( push_SHOP(LS, ud_mid->pShop) )
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

static int MOBPROTO_get_bossachv ( lua_State *LS)
{
    MOB_INDEX_DATA *ud_mid=check_MOBPROTO( LS, 1);
    if ( ud_mid->boss_achieve )
    {
        if ( push_BOSSACHV(LS, ud_mid->boss_achieve) )
            return 1;
        else
            return 0;
    }
    else
        return 0;
}
    
static const LUA_PROP_TYPE MOBPROTO_get_table [] =
{
    { "vnum", MOBPROTO_get_vnum, 0, STS_ACTIVE },
    { "name", MOBPROTO_get_name, 0, STS_ACTIVE },
    { "shortdescr", MOBPROTO_get_shortdescr, 0, STS_ACTIVE },
    { "longdescr", MOBPROTO_get_longdescr, 0, STS_ACTIVE },
    { "description", MOBPROTO_get_description, 0, STS_ACTIVE },
    { "alignment", MOBPROTO_get_alignment, 0, STS_ACTIVE },
    { "level", MOBPROTO_get_level, 0, STS_ACTIVE },
    { "hppcnt", MOBPROTO_get_hppcnt, 0, STS_ACTIVE },
    { "mnpcnt", MOBPROTO_get_mnpcnt, 0, STS_ACTIVE },
    { "mvpcnt", MOBPROTO_get_mvpcnt, 0, STS_ACTIVE },
    { "hrpcnt", MOBPROTO_get_hrpcnt, 0, STS_ACTIVE },
    { "drpcnt", MOBPROTO_get_drpcnt, 0, STS_ACTIVE },
    { "acpcnt", MOBPROTO_get_acpcnt, 0, STS_ACTIVE },
    { "savepcnt", MOBPROTO_get_savepcnt, 0, STS_ACTIVE },
    { "damtype", MOBPROTO_get_damtype, 0, STS_ACTIVE },
    { "startpos", MOBPROTO_get_startpos, 0, STS_ACTIVE },
    { "defaultpos", MOBPROTO_get_defaultpos, 0, STS_ACTIVE },
    { "sex", MOBPROTO_get_sex, 0, STS_ACTIVE },
    { "race", MOBPROTO_get_race, 0, STS_ACTIVE },
    { "wealthpcnt", MOBPROTO_get_wealthpcnt, 0, STS_ACTIVE },
    { "size", MOBPROTO_get_size, 0, STS_ACTIVE },
    { "stance", MOBPROTO_get_stance, 0, STS_ACTIVE },
    { "area", MOBPROTO_get_area, 0, STS_ACTIVE },
    { "ingame", MOBPROTO_get_ingame, 0, STS_ACTIVE },
    { "mtrigs", MOBPROTO_get_mtrigs, 0, STS_ACTIVE },
    { "shop", MOBPROTO_get_shop, 0, STS_ACTIVE },
    { "bossachv", MOBPROTO_get_bossachv, 0, STS_ACTIVE },
    { "count", MOBPROTO_get_count, 0, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE MOBPROTO_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE MOBPROTO_method_table [] =
{
    { "act", MOBPROTO_act, 0, STS_ACTIVE },
    { "vuln", MOBPROTO_vuln, 0, STS_ACTIVE },
    { "immune", MOBPROTO_immune, 0, STS_ACTIVE },
    { "offensive", MOBPROTO_offensive, 0, STS_ACTIVE },
    { "resist", MOBPROTO_resist, 0, STS_ACTIVE },
    { "affected", MOBPROTO_affected, 0, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
}; 

/* end MOBPROTO section */

/* SHOP section */
static int SHOP_get_keeper( lua_State *LS )
{
    SHOP_DATA *ud_shop=check_SHOP( LS, 1);
    lua_pushinteger( LS,
            ud_shop->keeper );
    return 1;
}

static int SHOP_get_profitbuy( lua_State *LS )
{
    SHOP_DATA *ud_shop=check_SHOP( LS, 1);
    lua_pushinteger( LS,
            ud_shop->profit_buy );
    return 1;
}

static int SHOP_get_profitsell( lua_State *LS )
{
    SHOP_DATA *ud_shop=check_SHOP( LS, 1);
    lua_pushinteger( LS,
            ud_shop->profit_sell );
    return 1;
}

static int SHOP_get_openhour( lua_State *LS )
{
    SHOP_DATA *ud_shop=check_SHOP( LS, 1);
    lua_pushinteger( LS,
            ud_shop->open_hour );
    return 1;
}

static int SHOP_get_closehour( lua_State *LS )
{
    SHOP_DATA *ud_shop=check_SHOP( LS, 1);
    lua_pushinteger( LS,
            ud_shop->close_hour );
    return 1;
}

static int SHOP_buytype ( lua_State *LS )
{
    SHOP_DATA *ud_shop=check_SHOP(LS, 1);
    if (lua_isnone( LS, 2) ) // no arg
    {
        lua_newtable(LS);
        int i;
        int index=1;
        for (i=0; i<MAX_TRADE; i++)
        {
            if (ud_shop->buy_type[i] != 0)
            {
                lua_pushstring( LS,
                        flag_bit_name(type_flags, ud_shop->buy_type[i]));
                lua_rawseti( LS, -2, index++);
            }
        }
        return 1;
    } 

    // arg was given
    const char *arg=check_string(LS, 2, MIL);
    int flag=flag_lookup(arg, type_flags);
    if ( flag==NO_FLAG )
        luaL_error(LS, "No such type flag '%s'", arg);

    int i;
    for (i=0; i<MAX_TRADE ; i++)
    {
        if (ud_shop->buy_type[i] == flag)
        {
            lua_pushboolean( LS, TRUE);
            return 1;
        }
    }

    lua_pushboolean( LS, FALSE );
    return 1;
}
       

static const LUA_PROP_TYPE SHOP_get_table [] =
{
    { "keeper", SHOP_get_keeper, 0, STS_ACTIVE },
    { "profitbuy", SHOP_get_profitbuy, 0, STS_ACTIVE },
    { "profitsell", SHOP_get_profitsell, 0, STS_ACTIVE },
    { "openhour", SHOP_get_openhour, 0, STS_ACTIVE },
    { "closehour", SHOP_get_closehour, 0, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE SHOP_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE SHOP_method_table [] =
{
    { "buytype", SHOP_buytype, 0, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};
/* end SHOP section */

/* AFFECT section */
static int AFFECT_get_where ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushstring( LS,
            flag_bit_name(apply_types, ud_af->where) );
    return 1;
}

static int AFFECT_get_type ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    if (ud_af->type < 1)
    {
        lua_pushliteral( LS, "none");
        return 1;
    }
    else
    {
        lua_pushstring( LS,
                skill_table[ud_af->type].name );
        return 1;
    }
}

static int AFFECT_get_location ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushstring( LS,
            flag_bit_name(apply_flags, ud_af->location) );
    return 1;
}

static int AFFECT_get_level ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushinteger( LS,
            ud_af->level);
    return 1;
}

static int AFFECT_get_duration ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushinteger( LS,
            ud_af->duration);
    return 1;
}

static int AFFECT_get_modifier ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushinteger( LS,
            ud_af->modifier);
    return 1;
}

static int AFFECT_get_detectlevel ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushinteger( LS,
            ud_af->detect_level);
    return 1;
}

static int AFFECT_get_bitvector ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    switch (ud_af->where)
    {
        case TO_AFFECTS:
            lua_pushstring(LS,
                    flag_bit_name( affect_flags, ud_af->bitvector ) ); 
            break;
        case TO_OBJECT:
            lua_pushstring(LS, 
                    flag_bit_name( extra_flags, ud_af->bitvector ) ); 
            break;
        case TO_WEAPON:
            /* tbc, make it return table of flags */
            lua_pushstring(LS,
                    i_flag_bits_name( weapon_type2, ud_af->bitvector ) );
            break;
        case TO_IMMUNE:
            lua_pushstring(LS,
                    flag_bit_name( imm_flags, ud_af->bitvector ) );
            break;
        case TO_RESIST:
            lua_pushstring(LS,
                    flag_bit_name( res_flags, ud_af->bitvector ) );
            break;
        case TO_VULN:
            lua_pushstring(LS,
                    flag_bit_name( vuln_flags, ud_af->bitvector ) );
            break;
        case TO_SPECIAL:
            lua_pushinteger( LS, ud_af->bitvector );
            break;
        default:
            luaL_error( LS, "Invalid where." );
    }
    return 1;
}

static int AFFECT_get_tag ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    if (ud_af->type != gsn_custom_affect)
        luaL_error(LS, "Can only get tag for custom_affect type.");

    lua_pushstring( LS, ud_af->tag);
    return 1;
}


static const LUA_PROP_TYPE AFFECT_get_table [] =
{
    { "where", AFFECT_get_where, 0, STS_ACTIVE },
    { "type", AFFECT_get_type, 0, STS_ACTIVE },
    { "level", AFFECT_get_level, 0, STS_ACTIVE },
    { "duration", AFFECT_get_duration, 0, STS_ACTIVE },
    { "location", AFFECT_get_location, 0, STS_ACTIVE },
    { "modifier", AFFECT_get_modifier, 0, STS_ACTIVE },
    { "bitvector", AFFECT_get_bitvector, 0, STS_ACTIVE },
    { "detectlevel", AFFECT_get_detectlevel, 0, STS_ACTIVE },
    { "tag", AFFECT_get_tag, 0, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE AFFECT_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE AFFECT_method_table [] =
{
    { NULL, NULL, 0, 0 }
};


/* end AFFECT section */

/* PROG section */
static int PROG_get_islua ( lua_State *LS )
{
    lua_pushboolean( LS,
            (check_PROG( LS, 1) )->is_lua);
    return 1;
}

static int PROG_get_vnum ( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_PROG( LS, 1) )->vnum);
    return 1;
}

static int PROG_get_code ( lua_State *LS )
{
    lua_pushstring( LS,
            (check_PROG( LS, 1) )->code);
    return 1;
}

static int PROG_get_security ( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_PROG( LS, 1) )->security);
    return 1;
}

static const LUA_PROP_TYPE PROG_get_table [] =
{
    { "islua", PROG_get_islua, 0, STS_ACTIVE },
    { "vnum", PROG_get_vnum, 0, STS_ACTIVE },
    { "code", PROG_get_code, 0, STS_ACTIVE },
    { "security", PROG_get_security, 0, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE PROG_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE PROG_method_table [] =
{
    { NULL, NULL, 0, 0 }
};
/* end PROG section */

/* TRIG section */
static int TRIG_get_trigtype ( lua_State *LS )
{
    const struct flag_type *tbl;

    lua_getmetatable(LS,1);
    lua_getfield(LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );

    if (type==&MTRIG_type)
    {
        tbl=mprog_flags;
    }
    else if (type==&OTRIG_type)
    {
        tbl=oprog_flags;
    }
    else if (type==&ATRIG_type)
    {
        tbl=aprog_flags;
    }
    else if (type==&RTRIG_type)
    {
        tbl=rprog_flags;
    }
    else
    {
        return luaL_error( LS, "Invalid type: %s.", type->type_name );
    }

    lua_pushstring( LS,
            flag_bit_name(
                tbl,
                ((PROG_LIST *) (arclib_check( type, LS, 1 )) )->trig_type ) );
    return 1;
}

static int TRIG_get_trigphrase ( lua_State *LS )
{
    lua_getmetatable(LS,1);
    lua_getfield(LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );

    if ( type != &MTRIG_type
            && type != &OTRIG_type
            && type != &ATRIG_type
            && type != &RTRIG_type )
        luaL_error( LS,
                "Invalid type: %s.",
                type->type_name);

    lua_pushstring( LS,
            ((PROG_LIST *) arclib_check( type, LS, 1 ) )->trig_phrase);
    return 1;
}

static int TRIG_get_prog ( lua_State *LS )
{
    lua_getmetatable(LS,1);
    lua_getfield(LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );

    if ( type != &MTRIG_type
            && type != &OTRIG_type
            && type != &ATRIG_type
            && type != &RTRIG_type )
        luaL_error( LS,
                "Invalid type: %s.",
                type->type_name);

    if ( push_PROG( LS,
            ((PROG_LIST *)arclib_check( type, LS, 1 ) )->script ) )
        return 1;
    return 0;
}    


static const LUA_PROP_TYPE TRIG_get_table [] =
{
    { "trigtype", TRIG_get_trigtype, 0, STS_ACTIVE },
    { "trigphrase", TRIG_get_trigphrase, 0, STS_ACTIVE },
    { "prog", TRIG_get_prog, 0, STS_ACTIVE },
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE TRIG_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE TRIG_method_table [] =
{
    { NULL, NULL, 0, 0 }
};

/* end TRIG section */

/* HELP section */
static int HELP_get_level( lua_State *LS )
{
    lua_pushinteger( LS, check_HELP( LS, 1 )->level );
    return 1;
}

static int HELP_get_keywords( lua_State *LS )
{
    lua_pushstring( LS, check_HELP( LS, 1 )->keyword );
    return 1;
}

static int HELP_get_text( lua_State *LS )
{
    lua_pushstring( LS, check_HELP( LS, 1 )->text );
    return 1;
}

static int HELP_get_delete( lua_State *LS )
{
    lua_pushboolean( LS, check_HELP( LS, 1 )->to_delete );
    return 1;
}

static const LUA_PROP_TYPE HELP_get_table [] =
{
    { "level", HELP_get_level, 0, STS_ACTIVE },
    { "keywords", HELP_get_keywords, 0, STS_ACTIVE },
    { "text", HELP_get_text, 0, STS_ACTIVE },
    { "delete", HELP_get_delete, 0, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE HELP_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE HELP_method_table [] =
{
    { NULL, NULL, 0, 0 }
};

/* end HELP section */

/* DESCRIPTOR section */
static int DESCRIPTOR_get_character( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1 );

    if (!ud_d->character)
        return 0;

    if ( push_CH( LS,
                ud_d->original ? ud_d->original : ud_d->character ) )
        return 1;
    else
        return 0;
}

static int DESCRIPTOR_get_constate( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);

    int state=con_state(ud_d);

    const char *name = name_lookup( state, con_states );

    if (!name)
    {
        bugf( "Unrecognized con state: %d", state );
        lua_pushstring( LS, "ERROR" );
    }
    else
    {
        lua_pushstring( LS, name );
    }
    return 1;
}

static int DESCRIPTOR_set_constate( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);
    const char *name=check_string(LS, 2, MIL);

    int state=flag_lookup( name, con_states );

    if ( state == -1 )
        return luaL_error( LS, "No such constate: %s", name );

    if (!is_settable(state, con_states))
        return luaL_error( LS, "constate cannot be set to %s", name );

    set_con_state( ud_d, state );
    return 0;
}

static int DESCRIPTOR_get_inbuf( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);
    lua_pushstring( LS, ud_d->inbuf);
    return 1;
}

static int DESCRIPTOR_set_inbuf( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);
    const char *arg=check_string( LS, 2, MAX_PROTOCOL_BUFFER );

    strcpy( ud_d->inbuf, arg );
    return 0;
}

static int DESCRIPTOR_get_conhandler( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);

    if (!is_set_ref(ud_d->conhandler))
        return 0;

    push_ref( LS, ud_d->conhandler); 
    return 1;
}

static int DESCRIPTOR_set_conhandler( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);

    if (is_set_ref(ud_d->conhandler))
        release_ref( LS, &ud_d->conhandler);

    save_ref( LS, 2, &ud_d->conhandler);
    return 0;
}

static const LUA_PROP_TYPE DESCRIPTOR_get_table [] =
{
    { "character", DESCRIPTOR_get_character, 0, STS_ACTIVE },
    { "constate", DESCRIPTOR_get_constate, SEC_NOSCRIPT, STS_ACTIVE },
    { "inbuf", DESCRIPTOR_get_inbuf, SEC_NOSCRIPT, STS_ACTIVE },
    { "conhandler", DESCRIPTOR_get_conhandler, SEC_NOSCRIPT, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE DESCRIPTOR_set_table [] =
{
    { "constate", DESCRIPTOR_set_constate, SEC_NOSCRIPT, STS_ACTIVE },
    { "inbuf", DESCRIPTOR_set_inbuf, SEC_NOSCRIPT, STS_ACTIVE },
    { "conhandler", DESCRIPTOR_set_conhandler, SEC_NOSCRIPT, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE DESCRIPTOR_method_table [] =
{
    { NULL, NULL, 0, 0 }
};
/* end DESCRIPTOR section */

/* BOSSACHV section */
static int BOSSACHV_get_qp( lua_State *LS )
{
    lua_pushinteger( LS,
            check_BOSSACHV( LS, 1 )->quest_reward);
    return 1;
}

static int BOSSACHV_get_exp( lua_State *LS )
{
    lua_pushinteger( LS,
            check_BOSSACHV( LS, 1 )->exp_reward);
    return 1;
}
static int BOSSACHV_get_gold( lua_State *LS )
{
    lua_pushinteger( LS,
            check_BOSSACHV( LS, 1 )->gold_reward);
    return 1;
}
static int BOSSACHV_get_achp( lua_State *LS )
{
    lua_pushinteger( LS,
            check_BOSSACHV( LS, 1 )->ach_reward);
    return 1;
}

static const LUA_PROP_TYPE BOSSACHV_get_table [] =
{
    { "qp", BOSSACHV_get_qp, SEC_NOSCRIPT, STS_ACTIVE },
    { "exp", BOSSACHV_get_exp, SEC_NOSCRIPT, STS_ACTIVE },
    { "gold", BOSSACHV_get_gold, SEC_NOSCRIPT, STS_ACTIVE },
    { "achp", BOSSACHV_get_achp, SEC_NOSCRIPT, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE BOSSACHV_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE BOSSACHV_method_table [] =
{
    { NULL, NULL, 0, 0 }
};
/* end BOSSACHV section */

/* BOSSREC section */

static int BOSSREC_get_vnum( lua_State *LS )
{
    lua_pushinteger( LS, check_BOSSREC(LS,1)->vnum);
    return 1;
} 

static int BOSSREC_get_timestamp( lua_State *LS )
{
    lua_pushinteger( LS, check_BOSSREC(LS,1)->timestamp);
    return 1;
}

static const LUA_PROP_TYPE BOSSREC_get_table [] =
{
    { "vnum", BOSSREC_get_vnum, 0, STS_ACTIVE },
    { "timestamp", BOSSREC_get_timestamp, 0, STS_ACTIVE },

    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE BOSSREC_set_table [] =
{
    { NULL, NULL, 0, 0 }
};

static const LUA_PROP_TYPE BOSSREC_method_table [] =
{
    { NULL, NULL, 0, 0 }
};
/* end BOSSREC section */

void arclib_type_init( lua_State *LS)
{
    int i;

    for ( i=0 ; type_list[i] ; i++ )
    {
        register_arclib_type(type_list[i], LS);
    }
}

void cleanup_uds( void )
{
    lua_newtable( g_mud_LS );
    lua_setglobal( g_mud_LS, "cleanup" );
} 


#define REF_FREED -1

struct arclib_metadata
{
    int ref;
};

bool arclib_valid( void *ud )
{
    bool rtn;
    lua_getfield( g_mud_LS, LUA_GLOBALSINDEX, "validuds" );
    lua_pushlightuserdata( g_mud_LS, ud );
    lua_gettable( g_mud_LS, -2 );
    if (lua_isnil( g_mud_LS, -1 ))
        rtn=FALSE;
    else
        rtn=TRUE;
    lua_pop( g_mud_LS, 2 ); /* pop result and validuds */
    return rtn;
}

void * arclib_check( LUA_OBJ_TYPE *type, lua_State *LS, int index )
{
    return luaL_checkudata( LS, index, type->type_name );
}

bool arclib_is( LUA_OBJ_TYPE *type, lua_State *LS, int index )
{
    lua_getmetatable( LS, index );
    luaL_getmetatable( LS, type->type_name );
    bool result=lua_equal( LS, -1, -2 );
    lua_pop( LS, 2 );
    return result;
}

bool arclib_push( LUA_OBJ_TYPE *type, lua_State *LS, void *ud )
{
    if (!ud)
    {
        bugf( "NULL ud passed to arclib_push, type: %s", type->type_name );
        return FALSE;
    }
    if ( !arclib_valid( ud ) )
    {
        bugf( "Invalid %s in arclib_push", type->C_type_name );
        return FALSE;
    }
    struct arclib_metadata *meta = ud + type->C_struct_size;
    int ref=meta->ref;
    if (ref==REF_FREED)
        return FALSE;
    
    lua_rawgeti( LS, LUA_REGISTRYINDEX, ref );
    
    return TRUE;
}

void * arclib_alloc( LUA_OBJ_TYPE *type )
{
    void *ud_mem=lua_newuserdata( g_mud_LS, type->C_struct_size + sizeof(struct arclib_metadata) );
    struct arclib_metadata *meta = ud_mem + type->C_struct_size;
    luaL_getmetatable( g_mud_LS, type->type_name );
    lua_setmetatable( g_mud_LS, -2 );
    meta->ref=luaL_ref( g_mud_LS, LUA_REGISTRYINDEX );
    type->count++;
    memset( ud_mem, 0, type->C_struct_size );
    /* register in validuds table for valid checks later on */
    lua_getfield( g_mud_LS, LUA_GLOBALSINDEX, "validuds" );
    lua_pushlightuserdata( g_mud_LS, ud_mem );
    lua_pushboolean( g_mud_LS, TRUE );
    lua_settable( g_mud_LS, -3 );
    lua_pop( g_mud_LS, 1 );
    return ud_mem;
}

void arclib_free( LUA_OBJ_TYPE *type, void *ud )
{
    if ( !arclib_valid( ud ) )
    {
        bugf( "Invalid %s in arclib_free", type->C_type_name );
        return;
    }
    struct arclib_metadata *meta = ud + type->C_struct_size;
    int ref=meta->ref;
    if ( ref == REF_FREED )
    {
        bugf( "Tried to free already freed %s", type->type_name );
        return;
    }
    /* destroy env */
    lua_getglobal( g_mud_LS, "envtbl" );
    arclib_push( type, g_mud_LS, ud );
    lua_pushnil( g_mud_LS );
    lua_settable( g_mud_LS, -3 );
    lua_pop( g_mud_LS, 1 ); /* pop envtbl */
    
    /* move to cleanup table */
    lua_getglobal( g_mud_LS, "cleanup" );
    arclib_push( type, g_mud_LS, ud );
    luaL_ref( g_mud_LS, -2 );
    lua_pop( g_mud_LS, 1 ); /* pop cleanup */
    
    meta->ref=REF_FREED;
    luaL_unref( g_mud_LS, LUA_REGISTRYINDEX, ref );
    type->count--;
    /* unregister from validuds table */
    lua_getfield( g_mud_LS, LUA_GLOBALSINDEX, "validuds" );
    lua_pushlightuserdata( g_mud_LS, ud );
    lua_pushnil( g_mud_LS );
    lua_settable( g_mud_LS, -3 );
    lua_pop( g_mud_LS, 1 );
}

int arclib_count_type( LUA_OBJ_TYPE *type )
{
    int count=0;
    luaL_getmetatable( g_mud_LS, type->type_name );
    lua_pushnil( g_mud_LS );
    
    while (lua_next( g_mud_LS, LUA_REGISTRYINDEX ))
    {
        if ( lua_isuserdata( g_mud_LS, -1 ) )
        {
            lua_getmetatable( g_mud_LS, -1 );
            if (lua_equal( g_mud_LS, -1, -4 ))
            {
                count++;
            }
            lua_pop( g_mud_LS, 1 );
        }
        lua_pop( g_mud_LS, 1 );
    }
    
    lua_pop( g_mud_LS, 1 );
    
    return count;
}

static int arclib_newindex( LUA_OBJ_TYPE *type, lua_State *LS )
{
    void * gobj = arclib_check( type, LS, 1 );
    const char *arg=check_string( LS, 2, MIL );
    
    if (! arclib_valid( gobj ) )
    {
        luaL_error( LS, "Tried to index invalid %s.", type->type_name );
    }
    
    lua_remove(LS, 2);
    
    const LUA_PROP_TYPE *set = type->set_table ;
    
    int i;
    for (i=0 ; set[i].field ; i++ )
    {
        if ( !strcmp(set[i].field, arg) )
        {
            if ( set[i].security > g_ScriptSecurity )
                luaL_error( LS, "Current security %d. Setting field requires %d.",
                        g_ScriptSecurity,
                        set[i].security);
            
            if ( set[i].func )
            {
                lua_pushcfunction( LS, set[i].func );
                lua_insert( LS, 1 );
                lua_call(LS, 2, 0);
                return 0;
            }
            else
            {
                bugf("No function entry for %s %s.",
                        type->type_name , arg );
                luaL_error(LS, "No function found.");
            }
        }
    }
    
    luaL_error(LS, "Can't set field '%s' for type %s.",
            arg, type->type_name );
    
    return 0;
}

static int arclib_index( LUA_OBJ_TYPE *type, lua_State *LS )
{
    void * gobj = arclib_check( type, LS, 1 );
    const char *arg = luaL_checkstring( LS, 2 );
    const LUA_PROP_TYPE *get = type->get_table;
    
    if (!strcmp("isarclibobject", arg))
    {
        lua_pushboolean( LS, TRUE );
        return 1;
    }
    bool valid = arclib_valid( gobj );
    if (!strcmp("valid", arg))
    {
        lua_pushboolean( LS, valid );
        return 1;
    }
    
    if (!valid)
    {
        luaL_error( LS, "Tried to index invalid %s.", type->type_name );
    }
    
    int i;
    for (i=0; get[i].field; i++ )
    {
        if (!strcmp(get[i].field, arg) )
        {
            if ( get[i].security > g_ScriptSecurity )
                luaL_error( LS, "Current security %d. Getting field requires %d.",
                        g_ScriptSecurity,
                        get[i].security);
            
            if (get[i].func)
            {
                int val;
                val=(get[i].func)(LS);
                return val;
            }
            else
            {
                bugf("No function entry for %s %s.",
                        type->type_name, arg );
                luaL_error(LS, "No function found.");
            }
        }
    }
    
    const LUA_PROP_TYPE *method = type->method_table ;
    
    for (i=0; method[i].field; i++ )
    {
        if (!strcmp(method[i].field, arg) )
        {
            if ( method[i].security > g_ScriptSecurity )
                luaL_error( LS, "Current security %d. Method requires %d.",
                        g_ScriptSecurity,
                        method[i].security);
            
            lua_pushcfunction(LS, method[i].func);
            return 1;
        }
    }
    
    return 0;
}

static int L_arclib_index( lua_State *LS )
{
    LUA_OBJ_TYPE *type = lua_touserdata( LS, lua_upvalueindex( 1 ) );
    return arclib_index( type, LS );
}

static int L_arclib_newindex( lua_State *LS )
{
    LUA_OBJ_TYPE *type = lua_touserdata( LS, lua_upvalueindex( 1 ) );
    return arclib_newindex( type, LS );
}

static int L_arclib_tostring( lua_State *LS )
{
    LUA_OBJ_TYPE *type = lua_touserdata( LS, lua_upvalueindex( 1 ) );
    lua_pushstring( LS, type->type_name );

    return 1;
}

static void register_arclib_type( LUA_OBJ_TYPE *type, lua_State *LS )
{
    luaL_newmetatable( LS, type->type_name );
    
    lua_pushlightuserdata( LS, type );
    lua_pushcclosure( LS, L_arclib_index, 1 );
    lua_setfield( LS, -2, "__index");
    
    lua_pushlightuserdata( LS, type );
    lua_pushcclosure( LS, L_arclib_newindex, 1 );
    lua_setfield( LS, -2, "__newindex");
    
    lua_pushlightuserdata( LS, type );
    lua_pushcclosure( LS, L_arclib_tostring, 1 );
    lua_setfield( LS, -2, "__tostring");
    
    lua_pushlightuserdata( LS, type );
    lua_setfield( LS, -2, "TYPE" );
    
    lua_pop( LS, 1 );
}

const char *arclib_type_name( LUA_OBJ_TYPE *type )
{
    return type->type_name;
}

/* Type definitions */

static LUA_OBJ_TYPE CH_type =
{
    .type_name     = "CH",
    .C_type_name   = "CHAR_DATA",
    .C_struct_size = sizeof(CHAR_DATA),
    .get_table     = CH_get_table,
    .set_table     = CH_set_table,
    .method_table  = CH_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE OBJ_type =
{
    .type_name     = "OBJ",
    .C_type_name   = "OBJ_DATA",
    .C_struct_size = sizeof(OBJ_DATA),
    .get_table     = OBJ_get_table,
    .set_table     = OBJ_set_table,
    .method_table  = OBJ_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE AREA_type =
{
    .type_name     = "AREA",
    .C_type_name   = "AREA_DATA",
    .C_struct_size = sizeof(AREA_DATA),
    .get_table     = AREA_get_table,
    .set_table     = AREA_set_table,
    .method_table  = AREA_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE ROOM_type =
{
    .type_name     = "ROOM",
    .C_type_name   = "ROOM_INDEX_DATA",
    .C_struct_size = sizeof(ROOM_INDEX_DATA),
    .get_table     = ROOM_get_table,
    .set_table     = ROOM_set_table,
    .method_table  = ROOM_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE EXIT_type =
{
    .type_name     = "EXIT",
    .C_type_name   = "EXIT_DATA",
    .C_struct_size = sizeof(EXIT_DATA),
    .get_table     = EXIT_get_table,
    .set_table     = EXIT_set_table,
    .method_table  = EXIT_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE RESET_type =
{
    .type_name     = "RESET",
    .C_type_name   = "RESET_DATA",
    .C_struct_size = sizeof(RESET_DATA),
    .get_table     = RESET_get_table,
    .set_table     = RESET_set_table,
    .method_table  = RESET_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE MOBPROTO_type =
{
    .type_name     = "MOBPROTO",
    .C_type_name   = "MOB_INDEX_DATA",
    .C_struct_size = sizeof(MOB_INDEX_DATA),
    .get_table     = MOBPROTO_get_table,
    .set_table     = MOBPROTO_set_table,
    .method_table  = MOBPROTO_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE OBJPROTO_type =
{
    .type_name     = "OBJPROTO",
    .C_type_name   = "OBJ_INDEX_DATA",
    .C_struct_size = sizeof(OBJ_INDEX_DATA),
    .get_table     = OBJPROTO_get_table,
    .set_table     = OBJPROTO_set_table,
    .method_table  = OBJPROTO_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE PROG_type =
{
    .type_name     = "PROG",
    .C_type_name   = "PROG_CODE",
    .C_struct_size = sizeof(PROG_CODE),
    .get_table     = PROG_get_table,
    .set_table     = PROG_set_table,
    .method_table  = PROG_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE MTRIG_type =
{
    .type_name     = "MTRIG",
    .C_type_name   = "PROG_LIST",
    .C_struct_size = sizeof(PROG_LIST),
    .get_table     = TRIG_get_table,
    .set_table     = TRIG_set_table,
    .method_table  = TRIG_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE OTRIG_type =
{
    .type_name     = "OTRIG",
    .C_type_name   = "PROG_LIST",
    .C_struct_size = sizeof(PROG_LIST),
    .get_table     = TRIG_get_table,
    .set_table     = TRIG_set_table,
    .method_table  = TRIG_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE ATRIG_type =
{
    .type_name     = "ATRIG",
    .C_type_name   = "PROG_LIST",
    .C_struct_size = sizeof(PROG_LIST),
    .get_table     = TRIG_get_table,
    .set_table     = TRIG_set_table,
    .method_table  = TRIG_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE RTRIG_type =
{
    .type_name     = "RTRIG",
    .C_type_name   = "PROG_LIST",
    .C_struct_size = sizeof(PROG_LIST),
    .get_table     = TRIG_get_table,
    .set_table     = TRIG_set_table,
    .method_table  = TRIG_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE SHOP_type =
{
    .type_name     = "SHOP",
    .C_type_name   = "SHOP_DATA",
    .C_struct_size = sizeof(SHOP_DATA),
    .get_table     = SHOP_get_table,
    .set_table     = SHOP_set_table,
    .method_table  = SHOP_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE AFFECT_type =
{
    .type_name     = "AFFECT",
    .C_type_name   = "AFFECT_DATA",
    .C_struct_size = sizeof(AFFECT_DATA),
    .get_table     = AFFECT_get_table,
    .set_table     = AFFECT_set_table,
    .method_table  = AFFECT_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE HELP_type =
{
    .type_name     = "HELP",
    .C_type_name   = "HELP_DATA",
    .C_struct_size = sizeof(HELP_DATA),
    .get_table     = HELP_get_table,
    .set_table     = HELP_set_table,
    .method_table  = HELP_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE DESCRIPTOR_type =
{
    .type_name     = "DESCRIPTOR",
    .C_type_name   = "DESCRIPTOR_DATA",
    .C_struct_size = sizeof(DESCRIPTOR_DATA),
    .get_table     = DESCRIPTOR_get_table,
    .set_table     = DESCRIPTOR_set_table,
    .method_table  = DESCRIPTOR_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE BOSSACHV_type =
{
    .type_name     = "BOSSACHV",
    .C_type_name   = "BOSSACHV",
    .C_struct_size = sizeof(BOSSACHV),
    .get_table     = BOSSACHV_get_table,
    .set_table     = BOSSACHV_set_table,
    .method_table  = BOSSACHV_method_table,
    .count         = 0
};

static LUA_OBJ_TYPE BOSSREC_type =
{
    .type_name     = "BOSSREC",
    .C_type_name   = "BOSSREC",
    .C_struct_size = sizeof(BOSSREC),
    .get_table     = BOSSREC_get_table,
    .set_table     = BOSSREC_set_table,
    .method_table  = BOSSREC_method_table,
    .count         = 0
};

/* typesafe wrappers */

CHAR_DATA *check_CH( lua_State *LS, int index ) { return arclib_check( p_CH_type, LS, index ); }
bool is_CH( lua_State *LS, int index) { return arclib_is( p_CH_type, LS, index ); }
bool push_CH( lua_State *LS, CHAR_DATA *ud ) { return arclib_push( p_CH_type, LS, ud ); }
CHAR_DATA *alloc_CH( void ) { return arclib_alloc( p_CH_type ); }
void free_CH( CHAR_DATA *ud ) { arclib_free( p_CH_type, ud ); }
bool valid_CH( CHAR_DATA *ud ) { return arclib_valid( ud ); }
int count_CH( void ) { return arclib_count_type( p_CH_type); }

OBJ_DATA *check_OBJ( lua_State *LS, int index ) { return arclib_check( p_OBJ_type, LS, index ); }
bool is_OBJ( lua_State *LS, int index) { return arclib_is( p_OBJ_type, LS, index ); }
bool push_OBJ( lua_State *LS, OBJ_DATA *ud ) { return arclib_push( p_OBJ_type, LS, ud ); }
OBJ_DATA *alloc_OBJ( void ) { return arclib_alloc( p_OBJ_type ); }
void free_OBJ( OBJ_DATA *ud ) { arclib_free( p_OBJ_type, ud ); }
bool valid_OBJ( OBJ_DATA *ud ) { return arclib_valid( ud ); }
int count_OBJ( void ) { return arclib_count_type( p_OBJ_type); }

AREA_DATA *check_AREA( lua_State *LS, int index ) { return arclib_check( p_AREA_type, LS, index ); }
bool is_AREA( lua_State *LS, int index) { return arclib_is( p_AREA_type, LS, index ); }
bool push_AREA( lua_State *LS, AREA_DATA *ud ) { return arclib_push( p_AREA_type, LS, ud ); }
AREA_DATA *alloc_AREA( void ) { return arclib_alloc( p_AREA_type ); }
void free_AREA( AREA_DATA *ud ) { arclib_free( p_AREA_type, ud ); }
bool valid_AREA( AREA_DATA *ud ) { return arclib_valid( ud ); }
int count_AREA( void ) { return arclib_count_type( p_AREA_type); }

ROOM_INDEX_DATA *check_ROOM( lua_State *LS, int index ) { return arclib_check( p_ROOM_type, LS, index ); }
bool is_ROOM( lua_State *LS, int index) { return arclib_is( p_ROOM_type, LS, index ); }
bool push_ROOM( lua_State *LS, ROOM_INDEX_DATA *ud ) { return arclib_push( p_ROOM_type, LS, ud ); }
ROOM_INDEX_DATA *alloc_ROOM( void ) { return arclib_alloc( p_ROOM_type ); }
void free_ROOM( ROOM_INDEX_DATA *ud ) { arclib_free( p_ROOM_type, ud ); }
bool valid_ROOM( ROOM_INDEX_DATA *ud ) { return arclib_valid( ud ); }
int count_ROOM( void ) { return arclib_count_type( p_ROOM_type); }

EXIT_DATA *check_EXIT( lua_State *LS, int index ) { return arclib_check( p_EXIT_type, LS, index ); }
bool is_EXIT( lua_State *LS, int index) { return arclib_is( p_EXIT_type, LS, index ); }
bool push_EXIT( lua_State *LS, EXIT_DATA *ud ) { return arclib_push( p_EXIT_type, LS, ud ); }
EXIT_DATA *alloc_EXIT( void ) { return arclib_alloc( p_EXIT_type ); }
void free_EXIT( EXIT_DATA *ud ) { arclib_free( p_EXIT_type, ud ); }
bool valid_EXIT( EXIT_DATA *ud ) { return arclib_valid( ud ); }
int count_EXIT( void ) { return arclib_count_type( p_EXIT_type); }

RESET_DATA *check_RESET( lua_State *LS, int index ) { return arclib_check( p_RESET_type, LS, index ); }
bool is_RESET( lua_State *LS, int index) { return arclib_is( p_RESET_type, LS, index ); }
bool push_RESET( lua_State *LS, RESET_DATA *ud ) { return arclib_push( p_RESET_type, LS, ud ); }
RESET_DATA *alloc_RESET( void ) { return arclib_alloc( p_RESET_type ); }
void free_RESET( RESET_DATA *ud ) { arclib_free( p_RESET_type, ud ); }
bool valid_RESET( RESET_DATA *ud ) { return arclib_valid( ud ); }
int count_RESET( void ) { return arclib_count_type( p_RESET_type); }

MOB_INDEX_DATA *check_MOBPROTO( lua_State *LS, int index ) { return arclib_check( p_MOBPROTO_type, LS, index ); }
bool is_MOBPROTO( lua_State *LS, int index) { return arclib_is( p_MOBPROTO_type, LS, index ); }
bool push_MOBPROTO( lua_State *LS, MOB_INDEX_DATA *ud ) { return arclib_push( p_MOBPROTO_type, LS, ud ); }
MOB_INDEX_DATA *alloc_MOBPROTO( void ) { return arclib_alloc( p_MOBPROTO_type ); }
void free_MOBPROTO( MOB_INDEX_DATA *ud ) { arclib_free( p_MOBPROTO_type, ud ); }
bool valid_MOBPROTO( MOB_INDEX_DATA *ud ) { return arclib_valid( ud ); }
int count_MOBPROTO( void ) { return arclib_count_type( p_MOBPROTO_type); }

OBJ_INDEX_DATA *check_OBJPROTO( lua_State *LS, int index ) { return arclib_check( p_OBJPROTO_type, LS, index ); }
bool is_OBJPROTO( lua_State *LS, int index) { return arclib_is( p_OBJPROTO_type, LS, index ); }
bool push_OBJPROTO( lua_State *LS, OBJ_INDEX_DATA *ud ) { return arclib_push( p_OBJPROTO_type, LS, ud ); }
OBJ_INDEX_DATA *alloc_OBJPROTO( void ) { return arclib_alloc( p_OBJPROTO_type ); }
void free_OBJPROTO( OBJ_INDEX_DATA *ud ) { arclib_free( p_OBJPROTO_type, ud ); }
bool valid_OBJPROTO( OBJ_INDEX_DATA *ud ) { return arclib_valid( ud ); }
int count_OBJPROTO( void ) { return arclib_count_type( p_OBJPROTO_type); }

PROG_CODE *check_PROG( lua_State *LS, int index ) { return arclib_check( p_PROG_type, LS, index ); }
bool is_PROG( lua_State *LS, int index) { return arclib_is( p_PROG_type, LS, index ); }
bool push_PROG( lua_State *LS, PROG_CODE *ud ) { return arclib_push( p_PROG_type, LS, ud ); }
PROG_CODE *alloc_PROG( void ) { return arclib_alloc( p_PROG_type ); }
void free_PROG( PROG_CODE *ud ) { arclib_free( p_PROG_type, ud ); }
bool valid_PROG( PROG_CODE *ud ) { return arclib_valid( ud ); }
int count_PROG( void ) { return arclib_count_type( p_PROG_type); }

PROG_LIST *check_MTRIG( lua_State *LS, int index ) { return arclib_check( p_MTRIG_type, LS, index ); }
bool is_MTRIG( lua_State *LS, int index) { return arclib_is( p_MTRIG_type, LS, index ); }
bool push_MTRIG( lua_State *LS, PROG_LIST *ud ) { return arclib_push( p_MTRIG_type, LS, ud ); }
PROG_LIST *alloc_MTRIG( void ) { return arclib_alloc( p_MTRIG_type ); }
void free_MTRIG( PROG_LIST *ud ) { arclib_free( p_MTRIG_type, ud ); }
bool valid_MTRIG( PROG_LIST *ud ) { return arclib_valid( ud ); }
int count_MTRIG( void ) { return arclib_count_type( p_MTRIG_type); }

PROG_LIST *check_OTRIG( lua_State *LS, int index ) { return arclib_check( p_OTRIG_type, LS, index ); }
bool is_OTRIG( lua_State *LS, int index) { return arclib_is( p_OTRIG_type, LS, index ); }
bool push_OTRIG( lua_State *LS, PROG_LIST *ud ) { return arclib_push( p_OTRIG_type, LS, ud ); }
PROG_LIST *alloc_OTRIG( void ) { return arclib_alloc( p_OTRIG_type ); }
void free_OTRIG( PROG_LIST *ud ) { arclib_free( p_OTRIG_type, ud ); }
bool valid_OTRIG( PROG_LIST *ud ) { return arclib_valid( ud ); }
int count_OTRIG( void ) { return arclib_count_type( p_OTRIG_type); }

PROG_LIST *check_ATRIG( lua_State *LS, int index ) { return arclib_check( p_ATRIG_type, LS, index ); }
bool is_ATRIG( lua_State *LS, int index) { return arclib_is( p_ATRIG_type, LS, index ); }
bool push_ATRIG( lua_State *LS, PROG_LIST *ud ) { return arclib_push( p_ATRIG_type, LS, ud ); }
PROG_LIST *alloc_ATRIG( void ) { return arclib_alloc( p_ATRIG_type ); }
void free_ATRIG( PROG_LIST *ud ) { arclib_free( p_ATRIG_type, ud ); }
bool valid_ATRIG( PROG_LIST *ud ) { return arclib_valid( ud ); }
int count_ATRIG( void ) { return arclib_count_type( p_ATRIG_type); }

PROG_LIST *check_RTRIG( lua_State *LS, int index ) { return arclib_check( p_RTRIG_type, LS, index ); }
bool is_RTRIG( lua_State *LS, int index) { return arclib_is( p_RTRIG_type, LS, index ); }
bool push_RTRIG( lua_State *LS, PROG_LIST *ud ) { return arclib_push( p_RTRIG_type, LS, ud ); }
PROG_LIST *alloc_RTRIG( void ) { return arclib_alloc( p_RTRIG_type ); }
void free_RTRIG( PROG_LIST *ud ) { arclib_free( p_RTRIG_type, ud ); }
bool valid_RTRIG( PROG_LIST *ud ) { return arclib_valid( ud ); }
int count_RTRIG( void ) { return arclib_count_type( p_RTRIG_type); }

SHOP_DATA *check_SHOP( lua_State *LS, int index ) { return arclib_check( p_SHOP_type, LS, index ); }
bool is_SHOP( lua_State *LS, int index) { return arclib_is( p_SHOP_type, LS, index ); }
bool push_SHOP( lua_State *LS, SHOP_DATA *ud ) { return arclib_push( p_SHOP_type, LS, ud ); }
SHOP_DATA *alloc_SHOP( void ) { return arclib_alloc( p_SHOP_type ); }
void free_SHOP( SHOP_DATA *ud ) { arclib_free( p_SHOP_type, ud ); }
bool valid_SHOP( SHOP_DATA *ud ) { return arclib_valid( ud ); }
int count_SHOP( void ) { return arclib_count_type( p_SHOP_type); }

AFFECT_DATA *check_AFFECT( lua_State *LS, int index ) { return arclib_check( p_AFFECT_type, LS, index ); }
bool is_AFFECT( lua_State *LS, int index) { return arclib_is( p_AFFECT_type, LS, index ); }
bool push_AFFECT( lua_State *LS, AFFECT_DATA *ud ) { return arclib_push( p_AFFECT_type, LS, ud ); }
AFFECT_DATA *alloc_AFFECT( void ) { return arclib_alloc( p_AFFECT_type ); }
void free_AFFECT( AFFECT_DATA *ud ) { arclib_free( p_AFFECT_type, ud ); }
bool valid_AFFECT( AFFECT_DATA *ud ) { return arclib_valid( ud ); }
int count_AFFECT( void ) { return arclib_count_type( p_AFFECT_type); }

HELP_DATA *check_HELP( lua_State *LS, int index ) { return arclib_check( p_HELP_type, LS, index ); }
bool is_HELP( lua_State *LS, int index) { return arclib_is( p_HELP_type, LS, index ); }
bool push_HELP( lua_State *LS, HELP_DATA *ud ) { return arclib_push( p_HELP_type, LS, ud ); }
HELP_DATA *alloc_HELP( void ) { return arclib_alloc( p_HELP_type ); }
void free_HELP( HELP_DATA *ud ) { arclib_free( p_HELP_type, ud ); }
bool valid_HELP( HELP_DATA *ud ) { return arclib_valid( ud ); }
int count_HELP( void ) { return arclib_count_type( p_HELP_type); }

DESCRIPTOR_DATA *check_DESCRIPTOR( lua_State *LS, int index ) { return arclib_check( p_DESCRIPTOR_type, LS, index ); }
bool is_DESCRIPTOR( lua_State *LS, int index) { return arclib_is( p_DESCRIPTOR_type, LS, index ); }
bool push_DESCRIPTOR( lua_State *LS, DESCRIPTOR_DATA *ud ) { return arclib_push( p_DESCRIPTOR_type, LS, ud ); }
DESCRIPTOR_DATA *alloc_DESCRIPTOR( void ) { return arclib_alloc( p_DESCRIPTOR_type ); }
void free_DESCRIPTOR( DESCRIPTOR_DATA *ud ) { arclib_free( p_DESCRIPTOR_type, ud ); }
bool valid_DESCRIPTOR( DESCRIPTOR_DATA *ud ) { return arclib_valid( ud ); }
int count_DESCRIPTOR( void ) { return arclib_count_type( p_DESCRIPTOR_type); }

BOSSACHV *check_BOSSACHV( lua_State *LS, int index ) { return arclib_check( p_BOSSACHV_type, LS, index ); }
bool is_BOSSACHV( lua_State *LS, int index) { return arclib_is( p_BOSSACHV_type, LS, index ); }
bool push_BOSSACHV( lua_State *LS, BOSSACHV *ud ) { return arclib_push( p_BOSSACHV_type, LS, ud ); }
BOSSACHV *alloc_BOSSACHV( void ) { return arclib_alloc( p_BOSSACHV_type ); }
void free_BOSSACHV( BOSSACHV *ud ) { arclib_free( p_BOSSACHV_type, ud ); }
bool valid_BOSSACHV( BOSSACHV *ud ) { return arclib_valid( ud ); }
int count_BOSSACHV( void ) { return arclib_count_type( p_BOSSACHV_type); }

BOSSREC *check_BOSSREC( lua_State *LS, int index ) { return arclib_check( p_BOSSREC_type, LS, index ); }
bool is_BOSSREC( lua_State *LS, int index) { return arclib_is( p_BOSSREC_type, LS, index ); }
bool push_BOSSREC( lua_State *LS, BOSSREC *ud ) { return arclib_push( p_BOSSREC_type, LS, ud ); }
BOSSREC *alloc_BOSSREC( void ) { return arclib_alloc( p_BOSSREC_type ); }
void free_BOSSREC( BOSSREC *ud ) { arclib_free( p_BOSSREC_type, ud ); }
bool valid_BOSSREC( BOSSREC *ud ) { return arclib_valid( ud ); }
int count_BOSSREC( void ) { return arclib_count_type( p_BOSSREC_type); }

