#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_arclib.h"
#include "lua_main.h"
#include "olc.h"
#include "tables.h"

bool deal_damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type, bool show, bool lethal, bool avoidable );

/* Define game object types and global functions */

#define GETP(type, field, sec ) { \
    #field , \
    type ## _get_ ## field, \
    sec,  \
    & type ## _get_ ## field ## _help, \
    STS_ACTIVE}

#define SETP(type, field, sec) { \
    #field, \
    type ## _set_ ## field, \
    sec, \
    & type ## _set_ ## field ## _help,\
    STS_ACTIVE}

#define METH(type, field, sec) { \
    #field, \
    type ## _ ## field, \
    sec, \
    & type ## _ ## field ## _help,\
    STS_ACTIVE}

#define CHGET( field, sec ) GETP( CH, field, sec)
#define CHSET( field, sec ) SETP( CH, field, sec)
#define CHMETH( field, sec ) METH( CH, field, sec)

#define OBJGET( field, sec ) GETP( OBJ, field, sec)
#define OBJSET( field, sec ) SETP( OBJ, field, sec)
#define OBJMETH( field, sec ) METH( OBJ, field, sec)

#define AREAGET( field, sec ) GETP( AREA, field, sec)
#define AREASET( field, sec ) SETP( AREA, field, sec)
#define AREAMETH( field, sec ) METH( AREA, field, sec)

#define ROOMGET( field, sec ) GETP( ROOM, field, sec)
#define ROOMSET( field, sec ) SETP( ROOM, field sec)
#define ROOMMETH( field, sec ) METH( ROOM, field, sec)

#define OPGET( field, sec ) GETP( OBJPROTO, field, sec)
#define OPSET( field, sec ) SETP( OBJPROTO, field sec)
#define OPMETH( field, sec ) METH( OBJPROTO, field, sec)

#define MPGET( field, sec ) GETP( MOBPROTO, field, sec)
#define MPSET( field, sec ) SETP( MOBPROTO, field sec)
#define MPMETH( field, sec ) METH( MOBPROTO, field, sec)

#define EXGET( field, sec ) GETP( EXIT, field, sec)
#define EXSET( field, sec ) SETP( EXIT, field sec)
#define EXMETH( field, sec ) METH( EXIT, field, sec)

#define RSTGET( field, sec ) GETP( RESET, field, sec)
#define RSTSET( field, sec ) SETP( RESET, field sec)
#define RSTMETH( field, sec ) METH( RESET, field, sec)

typedef struct lua_help_topic
{
    char *summary;
    char *info;
} HELPTOPIC;

struct prop_type
{
    char *field;
    int  (*func)();
    int security;
    HELPTOPIC *help;
    int status; 
};
#define STS_ACTIVE     0
#define STS_DEPRECATED 1

#define ENDPTABLE {NULL, NULL, 0, NULL, 0}

OBJ_TYPE *CH_type=NULL;
OBJ_TYPE *OBJ_type=NULL;
OBJ_TYPE *AREA_type=NULL;
OBJ_TYPE *ROOM_type=NULL;
OBJ_TYPE *EXIT_type=NULL;
OBJ_TYPE *RESET_type=NULL;
OBJ_TYPE *OBJPROTO_type=NULL;
OBJ_TYPE *MOBPROTO_type=NULL;

/* for iterating */
OBJ_TYPE *type_list [] =
{
    &CH_type,
    &OBJ_type,
    &AREA_type,
    &ROOM_type,
    &EXIT_type,
    &RESET_type,
    &OBJPROTO_type,
    &MOBPROTO_type,
    NULL
};


static OBJ_TYPE *new_obj_type(
        lua_State *LS,
        const char *type_name,
        const LUA_PROP_TYPE *get_table,
        const LUA_PROP_TYPE *set_table,
        const LUA_PROP_TYPE *method_table);

/* base functionality for lua object types */
static void * check_func( OBJ_TYPE *self,
        lua_State *LS, int index )
{
    lua_getfield(LS, index, "TYPE");
    OBJ_TYPE *type=lua_touserdata( LS, -1 );
    lua_pop(LS, 1);
    if ( type != self )
    {
        luaL_error(LS, "Bad parameter %d. Expected %s. ",
                index, self->type_name );
    }

    lua_getfield(LS, index, "tableid");
    luaL_checktype( LS, -1, LUA_TLIGHTUSERDATA);
    void *game_object=lua_touserdata(LS, -1 );
    lua_pop(LS, 1);

    return game_object;
}

static int index_metamethod( lua_State *LS)
{
    OBJ_TYPE *obj=lua_touserdata( LS, lua_upvalueindex(1));
    const char *arg=luaL_checkstring( LS, 2 );

    LUA_PROP_TYPE *get=obj->get_table;
    
    if (!strcmp("TYPE", arg) )
    {
        lua_pushlightuserdata( LS, obj );
        return 1;
    }
    else if (!strcmp("valid", arg) )
    {
        /* if the metatable is still working
           then the game object is still valid */
        lua_pushboolean( LS, TRUE );
        return 1;
    }

    int i;
    for (i=0; get[i].field; i++ )
    {
        if (!strcmp(get[i].field, arg) )
        {
           void *gobj=obj->check(obj, LS, 1 );
           if (get[i].func)
           {
               int val;
               val=(get[i].func)(LS, gobj);
               return val;
           }
           else
           {
               bugf("No function entry for %s %s.", 
                       obj->type_name, arg );
               luaL_error(LS, "No function found.");
           }

        }
    }

    LUA_PROP_TYPE *method=obj->method_table;

    for (i=0; method[i].field; i++ )
    {
        if (!strcmp(method[i].field, arg) )
        {
            lua_pushcfunction(LS, method[i].func);
            return 1;
        }
    }

    return 0;
}

static int newindex_metamethod( lua_State *LS )
{
    OBJ_TYPE *obj=lua_touserdata( LS, lua_upvalueindex(1));
    const char *arg=check_string( LS, 2, MIL );

    LUA_PROP_TYPE *set=obj->set_table;

    int i;
    for (i=0 ; set[i].field ; i++ )
    {
        if ( !strcmp(set[i].field, arg) )
        {
            void *gobj=obj->check(obj, LS, 1 ); 
            if ( set[i].func )
            {
                lua_pushcfunction( LS, set[i].func );
                lua_pushvalue( LS, 1);
                lua_pushvalue( LS, 3);
                lua_call(LS, 2, 0);
                return 0;
            }
            else
            {
                bugf("No function entry for %s %s.",
                        obj->type_name, arg );
                luaL_error(LS, "No function found.");
            }
        }
    }

    luaL_error(LS, "Can't set field '%s' for type %s.",
            arg, obj->type_name );

    return 0;
}

static void register_type( OBJ_TYPE *tp,
        lua_State *LS)
{
    luaL_newmetatable(LS, tp->type_name);
    
    lua_pushlightuserdata( LS, ( void *)tp);
    lua_pushcclosure( LS, index_metamethod, 1 );

    lua_setfield( LS, -2, "__index");

    lua_pushlightuserdata( LS, ( void *)tp);
    lua_pushcclosure( LS, newindex_metamethod, 1 );

    lua_setfield( LS, -2, "__newindex");
    lua_pop(LS, 1);
}

static bool make_func( OBJ_TYPE *self,
        lua_State *LS, void *game_obj)
{
    /* we don't want stuff that was destroyed */
    if ( self == CH_type && ((CHAR_DATA *)game_obj)->must_extract )
        return FALSE;
    if ( self == OBJ_type && ((OBJ_DATA *)game_obj)->must_extract )
        return FALSE;

    /* see if it exists already */
    lua_getglobal( LS, UD_TABLE_NAME);
    if ( lua_isnil( LS, -1) )
    {
        bugf("udtbl is nil in make_ud_table.");
        return FALSE;
    }

    lua_pushlightuserdata(LS, game_obj);
    lua_gettable( LS, -2);
    lua_remove( LS, -2); /* don't need udtbl anymore */

    if ( !lua_isnil(LS, -1) )
    {
        /* already exists, now at top of stack */
        return TRUE;
    }
    lua_remove(LS, -1); // kill the nil

    lua_newtable( LS);

    luaL_getmetatable (LS, self->type_name);
    lua_setmetatable (LS, -2);  /* set metatable for object data */
    
    lua_pushstring( LS, "tableid");
    lua_pushlightuserdata( LS, game_obj);
    lua_rawset( LS, -3 );

    lua_getfield( LS, LUA_GLOBALSINDEX, REGISTER_UD_FUNCTION);
    lua_pushvalue( LS, -2);
    if (CallLuaWithTraceBack( LS, 1, 1) )
    {
        bugf ( "Error registering UD:\n %s",
                lua_tostring(LS, -1));
        return FALSE;
    }

    /* get rid of our original table, register sends back a new version */
    lua_remove( LS, -2 );

    return TRUE;
}

bool is_func( OBJ_TYPE *self,
        lua_State *LS, int arg )
{
    if ( !lua_istable(LS, arg ) )
        return FALSE;

    lua_getfield(LS, arg, "TYPE");
    OBJ_TYPE *type=lua_touserdata(LS, -1);
    lua_pop(LS, 1);
    return ( type == self );
}


static OBJ_TYPE *new_obj_type(
        lua_State *LS,
        const char *type_name,
        const LUA_PROP_TYPE *get_table,
        const LUA_PROP_TYPE *set_table,
        const LUA_PROP_TYPE *method_table)
{
    /*tbc check for table structure correctness */
    /*check_table(get_table)
      check-table(set_table)
      check_table(method_table)
      */

    OBJ_TYPE *tp=alloc_mem(sizeof(OBJ_TYPE));
    tp->type_name=type_name;
    tp->set_table=set_table;
    tp->get_table=get_table;
    tp->method_table=method_table;

    tp->check=check_func;
    tp->make=make_func;
    tp->is=is_func;

    register_type( tp, LS );
    return tp;
}

/* global section */
static int godlib_bless (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_bless( NULL, ch, "" ));
    return 1;
}
#define GODLIBHELP( funcname ) \
HELPTOPIC godlib_ ## funcname ## _help = \
{\
    .summary="God " #funcname " target CH.", \
    .info=\
    "Arguments: target[CH]\n\r\n\r"\
    "Return: success[boolean]\n\r\n\r"\
    "Example:\n\r"\
    "god." #funcname "(ch)\n\r\n\r"\
    "Note:\n\r"\
    "Return value is whether the " #funcname " was successful."\
}
GODLIBHELP( bless );

static int godlib_curse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_curse( NULL, ch, "" ));
    return 1;
}
GODLIBHELP( curse );

static int godlib_heal (lua_State *LS)
{

    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_heal( NULL, ch, "" ));
    return 1;
}
GODLIBHELP( heal );

static int godlib_speed (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_speed( NULL, ch, "" ));
    return 1; 
}

GODLIBHELP( speed );

static int godlib_slow (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_slow( NULL, ch, "" ));
    return 1; 
}

GODLIBHELP( slow );

static int godlib_cleanse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_cleanse( NULL, ch, "" ));
    return 1; 
}
GODLIBHELP( cleanse );

static int godlib_defy (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_defy( NULL, ch, "" ));
    return 1; 
}
GODLIBHELP( defy );

static int godlib_enlighten (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_enlighten( NULL, ch, "" ));
    return 1; 
}
GODLIBHELP( enlighten );

static int godlib_protect (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_protect( NULL, ch, "" ));
    return 1;
}
GODLIBHELP( protect );

static int godlib_fortune (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_fortune( NULL, ch, "" ));
    return 1;
}
GODLIBHELP( fortune );

static int godlib_haunt (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_haunt( NULL, ch, "" ));
    return 1;
}
GODLIBHELP( haunt );

static int godlib_plague (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_plague( NULL, ch, "" ));
    return 1;
}
GODLIBHELP( plague );

static int godlib_confuse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_confuse( NULL, ch, "" ));
    return 1;
}
GODLIBHELP( confuse );

static int glob_sendtochar (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    char *msg=check_fstring( LS, 2, MSL);

    send_to_char(msg, ch);
    return 0;
}
HELPTOPIC glob_sendtochar_help =
{
    .summary="Send text to target CH.",
    .info="Arguments: target[CH], text[string]\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "sendtochar(ch, \"Hello there\")"

};

static int glob_clearloopcount (lua_State *LS)
{
    g_LoopCheckCounter=0;
    return 0;
}
HELPTOPIC glob_clearloopcount_help=
{
    .summary="Clear infinite loop protection counter",
    .info="Arguments: none\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "clearloopcount()\n\r\n\r"
          "Note:\n\r"
          "Infinite loop protection is provided by limiting any script to a \n\r"
          "maximum number of instructions, throwing an error if this maximum\n\r"
          "is exceeded. 'clearloopcount()' clears the counter, thereby \n\r"
          "bypassing this loop protection."
};

static int glob_log (lua_State *LS)
{
    char buf[MSL];
    sprintf(buf, "LUA::%s", check_fstring( LS, 1, MIL));

    log_string(buf);
    return 0;
}
HELPTOPIC glob_log_help={
    .summary="Print a string to the mud log.",
    .info="Arguments: string (accepts format arguments)\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "log(\"Something happened!\")\n\r"
          "log(\"%s just killed %s.\", ch.name, mob.name)\n\r\n\r"
          "Note:\n\r"
          "In the game log the argument will be prepended by 'LUA::'"
};

static int glob_hour (lua_State *LS)
{
    lua_pushnumber( LS, time_info.hour );
    return 1;
}
HELPTOPIC glob_hour_help={
    .summary="Returns the current game hour.",
    .info="Arguments: none\n\r\n\r"
          "Return: hour[number]\n\r\n\r"
          "Example:\n\r"
          "local hourvar=hour()\n\r\n\r"
          "Note:\n\r"
          "The hour is returned in 24hr format, so midnight is 0, noon is 12, 3PM is 15, etc."
};

static int glob_getroom (lua_State *LS)
{
    // do some if is number thing here eventually
    int num = (int)luaL_checknumber (LS, 1);

    ROOM_INDEX_DATA *room=get_room_index(num);

    if (!room)
        return 0;

    if ( !make_ROOM( LS, room) )
        return 0;
    else
        return 1;

}
HELPTOPIC glob_getroom_help={
    .summary="Returns the ROOM with given vnum.",
    .info="Arguments: vnum[number]\n\r\n\r"
          "Return: room[ROOM]\n\r\n\r"
          "Example:\n\r"
          "local mainroom=getroom(31404)\n\r\n\r"
          "Note:\n\r"
          "If room does not exist, returns nil."
};

static int glob_getobjproto (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    OBJ_INDEX_DATA *obj=get_obj_index(num);

    if (!obj)
        return 0;

    if ( !make_OBJPROTO( LS, obj) )
        return 0;
    else
        return 1;
}
HELPTOPIC glob_getobjproto_help={
    .summary="Returns the OBJPROTO with the given vnum",
    .info="Arguments: vnum[number]\n\r\n\r"
          "Return: target[OBJPROTO]\n\r\n\r"
          "Example:\n\r"
          "local op=getobjproto(31404)\n\r\n\r"
          "Note:\n\r"
          "If obj proto does not exist, returns nil."
};

static int glob_getobjworld (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    OBJ_DATA *obj;

    int index=1;
    lua_newtable(LS);
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( obj->pIndexData->vnum == num )
        {
            if (make_OBJ( LS, obj))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}
HELPTOPIC glob_getobjworld_help={
    .summary="Returns table of all object instances with given vnum.",
    .info="Arguments: vnum[number]\n\r\n\r"
          "Return: objects[table]\n\r\n\r"
          "Example:\n\r"
          "local objlist=getobjworld(31404)\n\r\n\r"
          "Note:\n\r"
          "If no instances exist, an empty table is returned.\n\r"
};

static int glob_getmobproto (lua_State *LS)
{
    int num = luaL_checknumber (LS, 1);

    MOB_INDEX_DATA *mob=get_mob_index(num);

    if (!mob)
        return 0;

    if ( !make_MOBPROTO( LS, mob) )
        return 0;
    else
        return 1;
}
HELPTOPIC glob_getmobproto_help={
    .summary="Returns the MOBROTO with the given vnum",
    .info="Arguments: vnum[number]\n\r\n\r"
          "Return: target[MOBPROTO]\n\r\n\r"
          "Example:\n\r"
          "local mp=getmobproto(31404)\n\r\n\r"
          "Note:\n\r"
          "If mob proto does not exist, returns nil."
};

static int glob_getmobworld (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);
    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if ( ch->pIndexData )
        {
            if ( ch->pIndexData->vnum == num )
            {
                if (make_CH( LS, ch))
                    lua_rawseti(LS, -2, index++);
            }
        }
    }
    return 1;
}
HELPTOPIC glob_getmobworld_help={
    .summary="Returns table of all mob instances with given vnum.",
    .info="Arguments: vnum[number]\n\r\n\r"
          "Return: mobs[table]\n\r\n\r"
          "Example:\n\r"
          "local moblist=getmobworld(31404)\n\r\n\r"
          "Note:\n\r"
          "If no instances exist, an empty table is returned.\n\r"
};

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
HELPTOPIC glob_pagetochar_help={
    .summary="Send string to target CH as paged output.",
    .info="Arguments: target[CH], text[string] <, raw[boolean]>\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "pagetochar( ch, reallylongstring)\n\r"
          "pagetochar( ch, reallylongstring, true)\n\r\n\r"
          "Note:\n\r"
          "Optional 3nd argument 'raw' is defaulted to false if not provided.\n\r"
          "If 'raw' is true then text is sent to the CH without processing color codes\n\r"
          "so \"{{rHello!{x\" would show as \"{{rHello!{x\" instead of \"{rHello!{x\" "
};

static int glob_getcharlist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if (make_CH(LS, ch))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}
HELPTOPIC glob_getcharlist_help={
    .summary="Return a table of all mobs and players in the game.",
    .info="Arguments: none\n\r\n\r"
          "Return: chars[table of CHs]\n\r\n\r"
          "Example:\n\r"
          "local charlist=getcharlist()\n\r\n\r"
};

static int glob_getmoblist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if ( IS_NPC(ch) )
        {
            if (make_CH(LS, ch))
                lua_rawseti(LS, -2, index++);
        }
    }

    return 1;
}
HELPTOPIC glob_getmoblist_help={
    .summary="Return a table of all mobs in the game.",
    .info="Arguments: none\n\r\n\r"
          "Return: chars[table of CHs]\n\r\n\r"
          "Example:\n\r"
          "local moblist=getmoblist()\n\r\n\r"
};

static int glob_getplayerlist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if ( !IS_NPC(ch) )
        {
            if (make_CH(LS, ch))
                lua_rawseti(LS, -2, index++);
        }
    }

    return 1;
}
HELPTOPIC glob_getplayerlist_help={
    .summary="Return a table of all mobs in the game.",
    .info="Arguments: none\n\r\n\r"
          "Return: chars[table of CHs]\n\r\n\r"
          "Example:\n\r"
          "local moblist=getmoblist()\n\r\n\r"
};

static int glob_getarealist (lua_State *LS)
{
    AREA_DATA *area;

    int index=1;
    lua_newtable(LS);

    for ( area=area_first ; area ; area=area->next )
    {
        if (make_AREA(LS, area))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}
HELPTOPIC glob_getarealist_help={
    .summary="Return a table of all areas in the game.",
    .info="Arguments: none\n\r\n\r"
          "Return: chars[table of AREAs]\n\r\n\r"
          "Example:\n\r"
          "local arealist=getarealist()\n\r\n\r"
};

/* Mersenne Twister pseudo-random number generator */

static int mtlib_srand (lua_State *LS)
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
} /* end of mtlib_srand */
HELPTOPIC mtlib_srand_help={};

static int mtlib_rand (lua_State *LS)
{
    lua_pushnumber (LS, (double)genrand ());
    return 1;
} /* end of mtlib_rand */
HELPTOPIC mtlib_rand_help={};

static int mudlib_luadir( lua_State *LS)
{
    lua_pushliteral( LS, LUA_DIR);
    return 1;
}
HELPTOPIC mudlib_luadir_help={};

static int mudlib_userdir( lua_State *LS)
{
    lua_pushliteral( LS, USER_DIR);
    return 1;
}
HELPTOPIC mudlib_userdir_help={};

/* return tprintstr of the given global (string arg)*/
static int dbglib_show ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    lua_getglobal( LS, luaL_checkstring( LS, 1 ) );
    lua_call( LS, 1, 1 );

    return 1;
}
HELPTOPIC dbglib_show_help={
    .summary="Returns tprintstr of the global table with given name",
    .info="Arguments: tablename[string]\n\r\n\r"
          "Return: result[string]\n\r\n\r"
          "Example:\n\r"
          "pagetochar(mob, dbg.show(\"script_globs\"))\n\r\n\r"
          "Note:\n\r"
          "'Global' in this case means global to the mud script space, not to script environment."

};

static int glob_randnum ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_randnum");
    lua_insert( LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}
HELPTOPIC glob_randnum_help={
    .summary="Return random integer in given range.",
    .info="Arguments: min[number], max[number]\n\r\n\r"
          "Return: result[number]\n\r\n\r"
          "Example:\n\r"
          "local num=randnum(1,3) -- random number, could be 1, 2, or 3 \n\r\n\r"
};

static int glob_rand ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_rand");
    lua_insert( LS, 1 );
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}
HELPTOPIC glob_rand_help={
    .summary="Random percentage check.",
    .info="Arguments: percent[string]\n\r\n\r"
          "Return: result[boolean]\n\r\n\r"
          "Example:\n\r"
          "if rand(35) then say(\"Passed!\") end\n\r\n\r"
          "Note:\n\r"
          "In example, rand has 35% chance to return true, otherwise returns false."
};

static int glob_tprintstr ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_tprintstr");
    lua_insert(LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}
HELPTOPIC glob_tprintstr_help={
    .summary="Returns contents of table formatted into a string.",
    .info="Arguments: target[table]\n\r\n\r"
          "Return: result[string]\n\r\n\r"
          "Example:\n\r"
          "echo(tprintstr({{\"hi\",\"bye\",3456}))\n\r\n\r"
};

static int glob_cancel ( lua_State *LS)
{
    return L_cancel(LS);
}
HELPTOPIC glob_cancel_help={};

static int glob_arguments ( lua_State *LS)
{   
    const char *argument=check_string( LS, 1, MIL );
    char buf[MIL];
    
    lua_newtable( LS );
    int index=1;

    while ( argument[0] != '\0' )
    {
        argument = one_argument( argument, buf );
        lua_pushstring( LS, buf );
        lua_rawseti( LS, -2, index++ );
    }

    return 1;
}
HELPTOPIC glob_arguments_help={};
        


#define SEC_NOSCRIPT -1
typedef struct glob_type
{
    char *lib;
    char *name;
    int (*func)();
    int security; /* if SEC_NOSCRIPT then not available in prog scripts */ 
    HELPTOPIC *help;
    int status;
} GLOB_TYPE;

#define ENDGTABLE { NULL, NULL, NULL, 0, NULL, 0 }
#define GFUN( fun, sec ) { NULL, #fun , glob_ ## fun , sec, & glob_ ## fun ## _help, STS_ACTIVE }
#define LFUN( lib, fun, sec) { #lib, #fun, lib ## lib_ ## fun , sec, & lib ## lib_ ## fun ## _help, STS_ACTIVE}
#define GODF( fun ) LFUN( god, fun, 9 )
#define DBGF( fun ) LFUN( dbg, fun, 9 )
GLOB_TYPE glob_table[] =
{
    GFUN(hour,          0),
    GFUN(getroom,       0),
    GFUN(randnum,       0),
    GFUN(rand,          0),
    GFUN(tprintstr,     0),
    GFUN(getobjproto,   0),
    GFUN(getobjworld,   0),
    GFUN(getmobproto,   0),
    GFUN(getmobworld,   0),
    GFUN(sendtochar,    0),
    GFUN(pagetochar,    0),
    GFUN(log,           0),
    GFUN(getcharlist,   9),
    GFUN(getmoblist,    9),
    GFUN(getplayerlist, 9),
    GFUN(getarealist,   9),
    GFUN(clearloopcount,9),

    GODF(confuse),
    GODF(curse),
    GODF(plague),
    GODF(bless),
    GODF(slow),
    GODF(speed),
    GODF(heal),
    GODF(enlighten),
    GODF(protect),
    GODF(fortune),
    GODF(haunt),
    GODF(cleanse),
    GODF(defy),
    
    DBGF(show),

    /* SEC_NOSCRIPT means aren't available for prog scripts */

    LFUN( mt, srand,        SEC_NOSCRIPT ),
    LFUN( mt, rand,         SEC_NOSCRIPT ),

    LFUN( mud, luadir,      SEC_NOSCRIPT ),
    LFUN( mud, userdir,     SEC_NOSCRIPT ),
    
    GFUN( arguments,        SEC_NOSCRIPT ),
    ENDGTABLE
};

static int global_sec_check (lua_State *LS)
{
    int security=luaL_checkinteger( LS, lua_upvalueindex(1) );
    
    if ( g_ScriptSecurity < security )
        luaL_error( LS, "Current security %d. Function requires %d.",
                g_ScriptSecurity,
                security);

    int (*fun)()=lua_tocfunction( LS, lua_upvalueindex(2) );

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
    int index;

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
static int set_flag( lua_State *LS,
        const char *funcname, 
        const struct flag_type *flagtbl, 
        tflag flagvar )
{
    const char *argument = check_string( LS, 2, MIL);
    luaL_checktype( LS, 3, LUA_TBOOLEAN );
    bool set=lua_toboolean( LS, 3 );
    
    int flag=flag_lookup( argument, flagtbl );
    if ( flag == NO_FLAG )
        luaL_error(LS, "'%s' invalid flag for %s", argument, funcname);
        
    if ( set )
        SET_BIT( flagvar, flag );
    else
        REMOVE_BIT( flagvar, flag);
        
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

    lua_getfield( LS, -2, "security");
    int sec=luaL_checkinteger( LS, -1);
    lua_pop(LS, 1);

    lua_getfield( LS, -2, "func"); 

    /* kill the entry before call in case of error */
    lua_pushvalue( LS, 1 ); /* lightud as key */
    lua_pushnil( LS ); /* nil as value */
    lua_settable( LS, 2 ); /* pops key and value */ 

    if ( is_CH( LS, -2 ) )
    {
        lua_mob_program( NULL, RUNDELAY_VNUM, NULL,
                check_CH(LS, -2), NULL, 
                NULL, NULL, 0, 0,
                TRIG_CALL, sec );
    }
    else if ( is_OBJ( LS, -2 ) )
    {
        lua_obj_program( NULL, RUNDELAY_VNUM, NULL,
                check_OBJ(LS, -2), NULL,
                NULL, NULL,
                TRIG_CALL, sec );
    }
    else if ( is_AREA( LS, -2 ) )
    {
        lua_area_program( NULL, RUNDELAY_VNUM, NULL,
                check_AREA(LS, -2), NULL,
                TRIG_CALL, sec );
    }
    else if ( is_ROOM( LS, -2 ) )
    {
        lua_room_program( NULL, RUNDELAY_VNUM, NULL, 
                check_ROOM(LS, -2), NULL,
                NULL, NULL, NULL,
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
    const char *tag=NULL;
    int val=luaL_checkint( LS, 2 );
    luaL_checktype( LS, 3, LUA_TFUNCTION);
    if (!lua_isnone( LS, 4 ) )
    {
       tag=str_dup(check_string( LS, 4, MIL ));
    }

    lua_getglobal( LS, "delaytbl");
    TIMER_NODE *tmr=register_lua_timer( val, tag );
    lua_pushlightuserdata( LS, (void *)tmr);
    lua_newtable( LS );

/*
    lua_pushliteral( LS, "tableid");
    lua_getfield( LS, 1, "tableid");
    lua_settable( LS, -3 );
*/
    lua_pushliteral( LS, "udobj");
    lua_pushvalue( LS, 1 );
    lua_settable( LS, -3 );

    lua_pushliteral( LS, "func");
    lua_pushvalue( LS, 3 );
    lua_settable( LS, -3 );

    lua_pushliteral( LS, "security");
    lua_pushinteger( LS, g_ScriptSecurity ); 
    lua_settable( LS, -3 );

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
/* end common section */

/* CH section */
static int CH_randchar (lua_State *LS)
{
    CHAR_DATA *ch=get_random_char(check_CH(LS,1) );
    if ( ! ch )
        return 0;

    if ( !make_CH(LS,ch))
        return 0;
    else
        return 1;

}
HELPTOPIC CH_randchar_help = {
    .summary="Get a random PC in the room",
    .info="Arguments: none\n\r\n\r"
          "Return: result[CH]\n\r\n\r"
          "Example:\n\r"
          "mob:say(\"Hi %s\", mob:randchar().name)\n\r\n\r"
};

/* analog of run_olc_editor in olc.c */
static bool run_olc_editor_lua( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
        return FALSE;

    switch ( ch->desc->editor )
    {
        case ED_AREA:
            aedit( ch, argument );
            break;
        case ED_ROOM:
            redit( ch, argument );
            break;
        case ED_OBJECT:
            oedit( ch, argument );
            break;
        case ED_MOBILE:
            medit( ch, argument );
            break;
        case ED_MPCODE:
            mpedit( ch, argument );
            break;
        case ED_OPCODE:
            opedit( ch, argument );
            break;
        case ED_APCODE:
            apedit( ch, argument );
            break;
        case ED_RPCODE:
            rpedit( ch, argument );
            break;
        case ED_HELP:
            hedit( ch, argument );
            break;
        default:
            return FALSE;
    }
    return TRUE; 
}

static int CH_olc (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);
    if (IS_NPC(ud_ch) )
    {
        luaL_error( LS, "NPCs cannot use OLC!");
    }

    if (!run_olc_editor_lua( ud_ch, check_fstring( LS, 2, MIL)) )
        luaL_error(LS, "Not currently in olc edit mode.");

    return 0;
}
HELPTOPIC CH_olc_help = {
    .summary="Execute OLC command (PC only)",
    .info="Arguments: string (accepts format arguments)\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mdo(\"redit\")\n\r"
          "olc(\"name AWESOME ROOM\")\n\r"
          "mdo(\"done\")\n\r\n\r"
          "Note:\n\r"
          "Error is thrown if not in olc editor mode.\n\r"

};

static int CH_tprint ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);

    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    do_say( ud_ch, check_string(LS, -1, MIL));

    return 0;
}
HELPTOPIC CH_tprint_help = {
    .summary="Print tprintstr of given table using say.",
    .info="Arguments: target[table]\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "tprint({{1,\"apple\",true}\n\r\n\r"
          "Note:\n\r"
          "See 'help global tprintstr'"
};

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
HELPTOPIC CH_savetbl_help = {};

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
HELPTOPIC CH_loadtbl_help = {};

static int CH_loadscript (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular mprog with vnum 0*/
    lua_mob_program( NULL, LOADSCRIPT_VNUM, check_string(LS, -1, MAX_SCRIPT_LENGTH), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 );

    return 0;
}
HELPTOPIC CH_loadscript_help = {};

static int CH_loadfunction ( lua_State *LS )
{
    lua_mob_program( NULL, RUNDELAY_VNUM, NULL,
                check_CH(LS, -2), NULL,
                NULL, NULL, 0, 0,
                TRIG_CALL, 0 );
    return 0;
}
HELPTOPIC CH_loadfunction_help = {};

static int CH_loadstring (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_mob_program( NULL, LOADSCRIPT_VNUM, check_string(LS, 2, MAX_SCRIPT_LENGTH), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 );
    return 0;
} 
HELPTOPIC CH_loadstring_help = {};

static int CH_loadprog (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int num = (int)luaL_checknumber (LS, 2);
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

    lua_mob_program( NULL, num, pMcode->code, ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 ); 

    return 0;
}
HELPTOPIC CH_loadprog_help = {};

static int CH_emote (lua_State *LS)
{
    do_emote( check_CH(LS, 1), check_fstring( LS, 2, MIL) );
    return 0;
}
HELPTOPIC CH_emote_help = {};

static int CH_asound (lua_State *LS)
{
    do_mpasound( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    return 0; 
}
HELPTOPIC CH_asound_help = {
    .summary="Emote the given argument.",
    .info="Arguments: text[string] (accepts format arguments)\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mob:emote(\"has a pet hamster\")\n\r\n\r"
};

static int CH_gecho (lua_State *LS)
{
    do_mpgecho( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    return 0;
}
HELPTOPIC CH_gecho_help = {
    .summary="Globally echo the given text.",
    .info="Arguments: text[string] (accepts format arguments)\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mob:gecho(\"HI EVERYBODY\")\n\r\n\r"
};

static int CH_zecho (lua_State *LS)
{
    do_mpzecho( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    return 0;
}
HELPTOPIC CH_zecho_help = {
    .summary="Echo text to all in same area.",
    .info="Arguments: text[string] (accepts format arguments)\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mob:zecho(\"HI AREA\")\n\r\n\r"
};

static int CH_kill (lua_State *LS)
{
    if ( lua_isstring(LS, 2) )
        do_mpkill( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    else
        mpkill( check_CH(LS, 1),
                check_CH(LS, 2) );

    return 0;
}
HELPTOPIC CH_kill_help = {
    .summary="Attack target if possible.",
    .info="Arguments: target[string] (accepts format arguments)\n\r"
          "           target[CH]\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mob:kill(ch.name)\n\r"
          "mob:kill(ch)\n\r\n\r"
};

static int CH_assist (lua_State *LS)
{
    if ( lua_isstring(LS, 2) )
        do_mpassist( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    else
        mpassist( check_CH(LS, 1), 
                check_CH(LS, 2) );
    return 0;
}
HELPTOPIC CH_assist_help = {
    .summary="Assist target if possible.",
    .info="Arguments: target[string] (accepts format arguments)\n\r"
          "           target[CH]\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mob:assist(ch.name)\n\r"
          "mob:assist(ch)\n\r"
};

static int CH_junk (lua_State *LS)
{
    do_mpjunk( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_junk_help = {};

static int CH_echo (lua_State *LS)
{
    do_mpecho( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_echo_help = {
    .summary="Echos to all CHs in room except actor",
    .info="Arguments: text[string] (accepts format arguments)\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mob:echo(\"Ribbit\")\n\r\n\r"
};

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
HELPTOPIC CH_echoaround_help = {
    .summary="Echo given text to all in room except target.",
    .info="Arguments: argument[string] ( format: '[victim] [message]', accepts format arguments)\n\r"
          "           target[CH], text[string] (accepts format arguments)\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mob:echoaround(ch.name..\" hehehehehe\")\n\r"
          "mob:echoaround(ch, \"hehehehehe\"\n\r\n\r"
};

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
HELPTOPIC CH_echoat_help = {
    .summary="Echos text to target CH (not same as actor)",
    .info="Arguments: argument[string] ( format: '[victimname] [message]', accepts format arguments\n\r"
          "           target[CH], text[string] (accepts format arguments)\n\r\n\r"
          "Return: none\n\r\n\r"
          "Example:\n\r"
          "mob:echoat(ch, \"Wooooooooooooooop\")\n\r\n\r"
          "Note:\n\r"
          "If target CH is same as actor, no message will be displayed.\n\r"
          "See 'luahelp global sendtochar'."
};

static int CH_mload (lua_State *LS)
{
    CHAR_DATA *mob=mpmload( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    if ( mob && make_CH(LS,mob) )
        return 1;
    else
        return 0;
}
HELPTOPIC CH_mload_help = {
    .summary="Load mob in same room as actor.",
    .info="Arguments: vnum[number] ( accepts format arguments )\n\r\n\r"
          "Return: mob[CH]\n\r\n\r"
          "Example:\n\r"
          "mob:mload(31404)\n\r\n\r"
          "Note:\n\r"
          "Script will not error/stop if vnum does not exist.\n\r"
          "See 'luahelp room mload'."

};

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
HELPTOPIC CH_purge_help = {};

static int CH_goto (lua_State *LS)
{

    do_mpgoto( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_goto_help = {};

static int CH_at (lua_State *LS)
{

    do_mpat( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_at_help = {};

static int CH_transfer (lua_State *LS)
{

    do_mptransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_transfer_help = {};

static int CH_gtransfer (lua_State *LS)
{

    do_mpgtransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_gtransfer_help = {};

static int CH_otransfer (lua_State *LS)
{

    do_mpotransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_otransfer_help = {};

static int CH_force (lua_State *LS)
{

    do_mpforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_force_help = {};

static int CH_gforce (lua_State *LS)
{

    do_mpgforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_gforce_help = {};

static int CH_vforce (lua_State *LS)
{

    do_mpvforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_vforce_help = {};

static int CH_cast (lua_State *LS)
{

    do_mpcast( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_cast_help = {};

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
            deal_damage( ud_ch, victim, dam, TYPE_UNDEFINED, damtype, FALSE, kill, FALSE ));
    return 1;
}
HELPTOPIC CH_damage_help = {
    .summary="Damage CH.",
    .info="Arguments: victim[CH], damage[number] <, lethal[boolean], damtype[string]>\n\r\n\r"
          "Return: success[boolean]\n\r\n\r"
          "Example:\n\r"
          "mob:damage(ch, 3000)) -- lethal by default\n\r"
          "mob:damage(ch, 3000, false)) -- won't kill ch\n\r"
          "mob:damage(ch, 3000, false, \"fire\") -- fire damage\n\r"
          "ch:damage(ch, 3000) -- damage self, don't have to worry about safe check\n\r\n\r"
          "Note:\n\r"
          "Error if actor not in same room as victim\n\r"
          "Optional 'lethal' argument is true by default\n\r"
          "Optional 'damtype' argument is \"none\" by default\n\r"
          "For valid damtype arguments see 'tables damage_type'\n\r"
          "If damtype is used, actual damage may be higher or lower than\n\r"
          "argument depending on victim vuln/resist/immune.\n\r"
          "Return values represents whether the damage was successful;\n\r"
          "it could fail for a variety of reasons including safe checks\n\r"
          
};

static int CH_remove (lua_State *LS)
{

    do_mpremove( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_remove_help = {};

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
HELPTOPIC CH_remort_help = {};

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
HELPTOPIC CH_qset_help = {};

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
HELPTOPIC CH_qadvance_help = {};

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
HELPTOPIC CH_reward_help = {};

static int CH_peace (lua_State *LS)
{
    if ( lua_isnone( LS, 2) )
        do_mppeace( check_CH(LS, 1), "");
    else
        do_mppeace( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_peace_help = {};

static int CH_restore (lua_State *LS)
{
    do_mprestore( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_restore_help = {};

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
HELPTOPIC CH_hit_help = {};

static int CH_mdo (lua_State *LS)
{
    interpret( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}
HELPTOPIC CH_mdo_help = {};

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
HELPTOPIC CH_mobhere_help = {};

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
HELPTOPIC CH_objhere_help = {};

static int CH_mobexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS,(bool) (get_mp_char( ud_ch, argument) != NULL) );

    return 1;
}
HELPTOPIC CH_mobexists_help = {};

static int CH_objexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, (bool) (get_mp_obj( ud_ch, argument) != NULL) );

    return 1;
}
HELPTOPIC CH_objexists_help = {};

static int CH_ispc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && !IS_NPC( ud_ch ) );
    return 1;
}
HELPTOPIC CH_ispc_help = {
};

static int CH_canattack (lua_State *LS)
{
    lua_pushboolean( LS, !is_safe(check_CH (LS, 1), check_CH (LS, 2)) );
    return 1;
}
HELPTOPIC CH_canattack_help = {};

static int CH_isnpc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_NPC( ud_ch ) );
    return 1;
}
HELPTOPIC CH_isnpc_help = {};

static int CH_isgood (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_GOOD( ud_ch ) ) ;
    return 1;
}
HELPTOPIC CH_isgood_help = {};

static int CH_isevil (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_EVIL( ud_ch ) ) ;
    return 1;
}
HELPTOPIC CH_isevil_help = {};

static int CH_isneutral (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_NEUTRAL( ud_ch ) ) ;
    return 1;
}
HELPTOPIC CH_isneutral_help = {};

static int CH_isimmort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_IMMORTAL( ud_ch ) ) ;
    return 1;
}
HELPTOPIC CH_isimmort_help = {};

static int CH_ischarm (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_AFFECTED( ud_ch, AFF_CHARM ) ) ;
    return 1;
}
HELPTOPIC CH_ischarm_help = {};

static int CH_isfollow (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->master != NULL ) ;
    return 1;
}
HELPTOPIC CH_isfollow_help = {};

static int CH_isactive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->position > POS_SLEEPING ) ;
    return 1;
}
HELPTOPIC CH_isactive_help = {};

static int CH_isvisible (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH(LS, 1);
    CHAR_DATA * ud_vic = check_CH (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && ud_vic != NULL && can_see( ud_ch, ud_vic ) ) ;

    return 1;
}
HELPTOPIC CH_isvisible_help = {};

static int CH_affected (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string(LS, 2, MIL);

    lua_pushboolean( LS,  ud_ch != NULL
            &&  is_affected_parse(ud_ch, argument) );

    return 1;
}
HELPTOPIC CH_affected_help = {};

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
HELPTOPIC CH_act_help = {};

static int CH_setact (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (lua_isnone(LS, 3) )
    {
        /* only 1 arg so using old syntax */
        do_mpact( ud_ch, check_fstring( LS, 2, MIL));
        return 0;
    }

    /* new syntax */
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "act[NPC]", act_flags, ud_ch->act );
    }
    else
    {
        return check_flag( LS, "act[PC]", plr_flags, ud_ch->act );
    }
}
HELPTOPIC CH_setact_help = {};

static int CH_offensive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "offensive",off_flags, ud_ch->off_flags );
}
HELPTOPIC CH_offensive_help = {};

static int CH_immune (lua_State *LS)
{ 
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "immune", imm_flags, ud_ch->imm_flags );
}
HELPTOPIC CH_immune_help = {};

static int CH_carries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, FALSE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_carry( ud_ch, argument, ud_ch ) != NULL) );

    return 1;
}
HELPTOPIC CH_carries_help = {};

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
HELPTOPIC CH_wears_help = {};

static int CH_has (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), FALSE ) );

    return 1;
}
HELPTOPIC CH_has_help = {};

static int CH_uses (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), TRUE ) );

    return 1;
}
HELPTOPIC CH_uses_help = {};

static int CH_say (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    do_say( ud_ch, check_fstring( LS, 2, MIL) );
    return 0;
}
HELPTOPIC CH_say_help = {};

static int CH_oload (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj=create_object( pObjIndex, 0);
    check_enchant_obj( obj );

    obj_to_char(obj,ud_ch);

    if ( !make_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}
HELPTOPIC CH_oload_help = {};

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
HELPTOPIC CH_destroy_help = {};

static int CH_vuln (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "vuln", vuln_flags, ud_ch->vuln_flags );
}
HELPTOPIC CH_vuln_help = {};

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
HELPTOPIC CH_qstatus_help = {};

static int CH_resist (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "resist", res_flags, ud_ch->res_flags );
}
HELPTOPIC CH_resist_help = {};

static int CH_skilled (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS,  ud_ch != NULL && skill_lookup(argument) != -1
            && get_skill(ud_ch, skill_lookup(argument)) > 0 );

    return 1;
}
HELPTOPIC CH_skilled_help = {};

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
HELPTOPIC CH_ccarries_help = {};

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
HELPTOPIC CH_qtimer_help = {};

static int CH_delay (lua_State *LS)
{
    return L_delay( LS );
}
HELPTOPIC CH_delay_help = {};

static int CH_cancel (lua_State *LS)
{
    return L_cancel( LS );
}
HELPTOPIC CH_cancel_help = {};

static int CH_get_hp (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH (LS, 1))->hit );
    return 1;
}
HELPTOPIC CH_get_hp_help = {
};

static int CH_set_hp (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->hit=num;
    return 0;
}
HELPTOPIC CH_set_hp_help = {};

static int CH_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_CH(LS,1))->name );
    return 1;
}
HELPTOPIC CH_get_name_help = {
};

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
HELPTOPIC CH_set_name_help = {
    .summary="NPC only."
};

static int CH_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->level );
    return 1;
}
HELPTOPIC CH_get_level_help = {};

static int CH_set_level (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Cannot set level on PC.");

    int num = (int)luaL_checknumber (LS, 2);
    if ( num < 1 || num > 200 )
        luaL_error( LS, "Invalid level: %d, range is 1 to 200.", num);
    set_mob_level( ud_ch, num );
    return 0;
}
HELPTOPIC CH_set_level_help = 
{
    .summary="NPC only. Range 1-200. Restores mob to full health."
};

static int CH_setlevel (lua_State *LS)
{
    /*
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Cannot set level on PC.");

    int num = (int)luaL_checknumber (LS, 2);
    if ( num < 1 || num > 200 )
        luaL_error( LS, "Invalid level: %d, range is 1 to 200.", num);
    set_mob_level( ud_ch, num );
    return 0;*/
    return CH_set_level (LS);
}
HELPTOPIC CH_setlevel_help = {};

static int CH_get_maxhp (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_hit );
    return 1;
}
HELPTOPIC CH_get_maxhp_help={};

static int CH_set_maxhp (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxhp on PCs.");
        
    ud_ch->max_hit = luaL_checkinteger( LS, 2);
    return 0;
}
HELPTOPIC CH_set_maxhp_help={
    .summary="NPC only."};

static int CH_get_mana (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->mana );
    return 1;
}
HELPTOPIC CH_get_mana_help={};

static int CH_set_mana (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->mana=num;
    return 0;
}
HELPTOPIC CH_set_mana_help = {};

static int CH_get_maxmana (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_mana );
    return 1;
}
HELPTOPIC CH_get_maxmana_help={};

static int CH_set_maxmana (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxmana on PCs.");
        
    ud_ch->max_mana = luaL_checkinteger( LS, 2);
    return 0;
}
HELPTOPIC CH_set_maxmana_help={
    .summary="NPC only."};

static int CH_get_move (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->move );
    return 1;
}
HELPTOPIC CH_get_move_help={};

static int CH_set_move (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->move=num;
    return 0;
}
HELPTOPIC CH_set_move_help = {};

static int CH_get_maxmove (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_move );
    return 1;
}
HELPTOPIC CH_get_maxmove_help={};

static int CH_set_maxmove (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxmove on PCs.");
        
    ud_ch->max_move = luaL_checkinteger( LS, 2);
    return 0;
}
HELPTOPIC CH_set_maxmove_help={
    .summary="NPC only."};

static int CH_get_gold (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->gold );
    return 1;
}
HELPTOPIC CH_get_gold_help={};

static int CH_set_gold (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set gold on PCs.");
        
    ud_ch->gold = luaL_checkinteger( LS, 2);
    return 0;
}
HELPTOPIC CH_set_gold_help={
    .summary="NPC only."};

static int CH_get_silver (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->silver );
    return 1;
}
HELPTOPIC CH_get_silver_help={};

static int CH_set_silver (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set silver on PCs.");
        
    ud_ch->silver = luaL_checkinteger( LS, 2);
    return 0;
}
HELPTOPIC CH_set_silver_help={
    .summary="NPC only."};

static int CH_get_money (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_pushinteger( LS,
            ud_ch->silver + ud_ch->gold*100 );
    return 1;
}
HELPTOPIC CH_get_money_help={};

static int CH_get_sex (lua_State *LS)
{
    lua_pushstring( LS,
            sex_table[(check_CH(LS,1))->sex].name );
    return 1;
}
HELPTOPIC CH_get_sex_help={};

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
HELPTOPIC CH_set_sex_help={
    .summary="NPC only."};

static int CH_get_size (lua_State *LS)
{
    lua_pushstring( LS,
            size_table[(check_CH(LS,1))->size].name );
    return 1;
}
HELPTOPIC CH_get_size_help={};

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
HELPTOPIC CH_set_size_help={
    .summary="NPC only."};

static int CH_get_position (lua_State *LS)
{
    lua_pushstring( LS,
            position_table[(check_CH(LS,1))->position].short_name );
    return 1;
}
HELPTOPIC CH_get_position_help={};

static int CH_get_align (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->alignment );
    return 1;
}
HELPTOPIC CH_get_align_help={};

static int CH_set_align (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int num=luaL_checkinteger( LS, 2);
    if (num < -1000 || num > 1000)
        luaL_error(LS, "Invalid align: %d, range is -1000 to 1000.", num);
    ud_ch->alignment = num;
    return 0;
}
HELPTOPIC CH_set_align_help={};

#define CHGETSTAT( statname, statnum ) \
static int CH_get_ ## statname ( lua_State *LS ) \
{\
    lua_pushinteger( LS, \
            get_curr_stat((check_CH(LS,1)), statnum ));\
    return 1;\
}\
HELPTOPIC CH_get_ ## statname ## _help= {\
    .summary="Including spell/armor bonuses if any."\
}

CHGETSTAT( str, STAT_STR );
CHGETSTAT( con, STAT_CON );
CHGETSTAT( vit, STAT_VIT );
CHGETSTAT( agi, STAT_AGI );
CHGETSTAT( dex, STAT_DEX );
CHGETSTAT( int, STAT_INT );
CHGETSTAT( wis, STAT_WIS );
CHGETSTAT( dis, STAT_DIS );
CHGETSTAT( cha, STAT_CHA );
CHGETSTAT( luc, STAT_LUC );

#define CHSETSTAT( statname, statnum ) \
static int CH_set_ ## statname ( lua_State *LS ) \
{\
    CHAR_DATA *ud_ch=check_CH(LS,1);\
    if (!IS_NPC(ud_ch))\
        luaL_error(LS, "Can't set stats on PCs.");\
    \
    int num = luaL_checkinteger( LS, 2);\
    if (num < 1 || num > 200 )\
        luaL_error(LS, "Invalid stat value: %d, range is 1 to 200.", num );\
    \
    ud_ch->perm_stat[ statnum ] = num;\
    return 0;\
}\
HELPTOPIC CH_set_ ## statname ## _help= {\
    .summary="NPC only. Range 1-200."\
}

CHSETSTAT( str, STAT_STR );     
CHSETSTAT( con, STAT_CON );
CHSETSTAT( vit, STAT_VIT );
CHSETSTAT( agi, STAT_AGI );
CHSETSTAT( dex, STAT_DEX );
CHSETSTAT( int, STAT_INT );
CHSETSTAT( wis, STAT_WIS );
CHSETSTAT( dis, STAT_DIS );
CHSETSTAT( cha, STAT_CHA );
CHSETSTAT( luc, STAT_LUC );

static int CH_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_CH(LS,1))->clan].name);
    return 1;
}
HELPTOPIC CH_get_clan_help={};

static int CH_get_class (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        luaL_error(LS, "Can't check class on NPC.");
    }

    lua_pushstring( LS,
            class_table[ud_ch->class].name);
    return 1;
}
HELPTOPIC CH_get_class_help={};

static int CH_get_race (lua_State *LS)
{
    lua_pushstring( LS,
            race_table[(check_CH(LS,1))->race].name);
    return 1;
}
HELPTOPIC CH_get_race_help={};

static int CH_set_race (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set race on PCs.");
    
    const char * arg=check_string(LS, 2, MIL);
    int race=race_lookup(arg);
    if (race==0)
        luaL_error(LS, "No such race: %s", arg );

    ud_ch->race=race;
    return 0;
}
HELPTOPIC CH_set_race_help={
    .summary="NPC only."};

static int CH_get_fighting (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!ud_ch->fighting)
        return 0;
    else if (!make_CH(LS, ud_ch->fighting) )
        return 0;
    else
        return 1;
}
HELPTOPIC CH_get_fighting_help={};

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
HELPTOPIC CH_get_heshe_help={};

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
HELPTOPIC CH_get_himher_help={};

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
HELPTOPIC CH_get_hisher_help={};

static int CH_get_inventory (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_ch->carrying ; obj ; obj=obj->next_content)
    {
        if (make_OBJ(LS, obj))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC CH_get_inventory_help={};

static int CH_get_room (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!make_ROOM(LS, check_CH(LS,1)->in_room) )
        return 0;
    else
        return 1;
}
HELPTOPIC CH_get_room_help={};

static int CH_get_groupsize (lua_State *LS)
{
    lua_pushinteger( LS,
            count_people_room( check_CH(LS, 1), 4 ) );
    return 1;
}
HELPTOPIC CH_get_groupsize_help={};

static int CH_get_clanrank( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get clanrank on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->clan_rank);
    return 1;
}
HELPTOPIC CH_get_clanrank_help={};

static int CH_get_remorts( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get remorts on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->remorts);
    return 1;
}
HELPTOPIC CH_get_remorts_help={};

static int CH_get_explored( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get explored on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->explored->set);
    return 1;
}
HELPTOPIC CH_get_explored_help={};

static int CH_get_beheads( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get beheads on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->behead_cnt);
    return 1;
}
HELPTOPIC CH_get_beheads_help={};

static int CH_get_pkills( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get pkills on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->pkill_count);
    return 1;
}
HELPTOPIC CH_get_pkills_help={};

static int CH_get_pkdeaths( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get pkdeaths on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->pkill_deaths);
    return 1;
}
HELPTOPIC CH_get_pkdeaths_help={};

static int CH_get_questpoints( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get questpoints on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->questpoints);
    return 1;
}
HELPTOPIC CH_get_questpoints_help={};

static int CH_get_bank( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get bank on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->bank);
    return 1;
}
HELPTOPIC CH_get_bank_help={};

static int CH_get_mobkills( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get mobkills on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->mob_kills);
    return 1;
}
HELPTOPIC CH_get_mobkills_help={};

static int CH_get_mobdeaths( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get mobdeaths on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->mob_deaths);
    return 1;
}
HELPTOPIC CH_get_mobdeaths_help={};

static int CH_get_vnum( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get vnum on PCs.");

    lua_pushinteger( LS,
            ud_ch->pIndexData->vnum);
    return 1;
}
HELPTOPIC CH_get_vnum_help={};

static int CH_get_proto( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get proto on PCs.");

    if (!make_MOBPROTO( LS, ud_ch->pIndexData ) )
        return 0;
    else
        return 1;
}
HELPTOPIC CH_get_proto_help={};

static int CH_get_shortdescr( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get shortdescr on PCs.");

    lua_pushstring( LS,
            ud_ch->short_descr);
    return 1;
}
HELPTOPIC CH_get_shortdescr_help={};

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
HELPTOPIC CH_set_shortdescr_help = {
    .summary="NPC only."
};

static int CH_get_longdescr( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get longdescr on PCs.");

    lua_pushstring( LS,
            ud_ch->long_descr);
    return 1;
}
HELPTOPIC CH_get_longdescr_help={};

static int CH_set_longdescr (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set longdescr on PCs.");
    const char *new=check_string(LS, 2, MIL);
    char buf[MSL];
    sprintf(buf, "%s\n\r", new);
    free_string( ud_ch->long_descr );
    ud_ch->long_descr=str_dup(buf);
    return 0;
}
HELPTOPIC CH_set_longdescr_help = {
    .summary="NPC only."
};

static const LUA_PROP_TYPE CH_get_table [] =
{
    CHGET(name, 0),
    CHGET(level, 0),
    CHGET(hp, 0),
    CHGET(maxhp, 0),
    CHGET(mana, 0),
    CHGET(maxmana, 0),
    CHGET(move, 0),
    CHGET(maxmove, 0),
    CHGET(gold, 0),
    CHGET(silver, 0),
    CHGET(money, 0),
    CHGET(sex, 0),
    CHGET(size, 0),
    CHGET(position, 0),
    CHGET(align, 0),
    CHGET(str, 0),
    CHGET(con, 0),
    CHGET(vit, 0),
    CHGET(agi, 0),
    CHGET(dex, 0),
    CHGET(int, 0),
    CHGET(wis, 0),
    CHGET(dis, 0),
    CHGET(cha, 0),
    CHGET(luc, 0),
    CHGET(clan, 0),
    CHGET(class, 0),
    CHGET(race, 0),
    CHGET(fighting, 0),
    CHGET(heshe, 0),
    CHGET(himher, 0),
    CHGET(hisher, 0),
    CHGET(inventory, 0),
    CHGET(room, 0),
    CHGET(groupsize, 0),
    /* PC only */
    CHGET(clanrank, 0),
    CHGET(remorts, 0),
    CHGET(explored, 0),
    CHGET(beheads, 0),
    CHGET(pkills, 0),
    CHGET(pkdeaths, 0),
    CHGET(questpoints, 0),
    CHGET(bank, 0),
    CHGET(mobkills, 0),
    CHGET(mobdeaths, 0),
    /* NPC only */
    CHGET(vnum, 0),
    CHGET(proto,0),
    CHGET(shortdescr, 0),
    CHGET(longdescr, 0),    
    ENDPTABLE
};

static const LUA_PROP_TYPE CH_set_table [] =
{
    CHSET(name, 9),
    CHSET(level, 9),
    CHSET(hp, 9),
    CHSET(maxhp, 9),
    CHSET(mana, 9),
    CHSET(maxmana, 9),
    CHSET(move, 9),
    CHSET(maxmove, 9),
    CHSET(gold, 9),
    CHSET(silver, 9),
    CHSET(sex, 9),
    CHSET(size, 9),
    CHSET(align, 9),
    CHSET(str, 9),
    CHSET(con, 9),
    CHSET(vit, 9),
    CHSET(agi, 9),
    CHSET(dex, 9),
    CHSET(int, 9),
    CHSET(wis, 9),
    CHSET(dis, 9),
    CHSET(cha, 9),
    CHSET(luc, 9),
    CHSET(race, 9),
    CHSET(shortdescr, 9),
    CHSET(longdescr, 9),
    ENDPTABLE
};

static const LUA_PROP_TYPE CH_method_table [] =
{
    CHMETH(ispc, 0),
    CHMETH(isnpc, 0),
    CHMETH(isgood, 0),
    CHMETH(isevil, 0),
    CHMETH(isneutral, 0),
    CHMETH(isimmort, 0),
    CHMETH(ischarm, 0),
    CHMETH(isfollow, 0),
    CHMETH(isactive, 0),
    CHMETH(isvisible, 0),
    CHMETH(mobhere, 0),
    CHMETH(objhere, 0),
    CHMETH(mobexists, 0),
    CHMETH(objexists, 0),
    CHMETH(affected, 0),
    CHMETH(act, 0),
    CHMETH(offensive, 0),
    CHMETH(immune, 0),
    CHMETH(carries, 0),
    CHMETH(wears, 0),
    CHMETH(has, 0),
    CHMETH(uses, 0),
    CHMETH(qstatus, 0),
    CHMETH(resist, 0),
    CHMETH(vuln, 0),
    CHMETH(skilled, 0),
    CHMETH(ccarries, 0),
    CHMETH(qtimer, 0),
    CHMETH(canattack, 0),
    CHMETH(destroy, 0),
    CHMETH(oload, 0),
    /* deprecated */
    //CHMETH(setlevel, 0),
    { "setlevel", CH_setlevel, 0, &CH_setlevel_help, STS_DEPRECATED},
    CHMETH(say, 0),
    CHMETH(emote, 0),
    CHMETH(mdo, 0),
    CHMETH(asound, 0),
    CHMETH(gecho, 0),
    CHMETH(zecho, 0),
    CHMETH(kill, 0),
    CHMETH(assist, 0),
    CHMETH(junk, 0),
    CHMETH(echo, 0),
    CHMETH(echoaround, 0),
    CHMETH(echoat, 0),
    CHMETH(mload, 0),
    CHMETH(purge, 0),
    CHMETH(goto, 0),
    CHMETH(at, 0),
    CHMETH(transfer, 0),
    CHMETH(gtransfer, 0),
    CHMETH(otransfer, 0),
    CHMETH(force, 0),
    CHMETH(gforce, 0),
    CHMETH(vforce, 0),
    CHMETH(cast, 0),
    CHMETH(damage, 0),
    CHMETH(remove, 0),
    CHMETH(remort, 0),
    CHMETH(qset, 0),
    CHMETH(qadvance, 0),
    CHMETH(reward, 0),
    CHMETH(peace, 0),
    CHMETH(restore, 0),
    CHMETH(setact, 0),
    CHMETH(hit, 0),
    CHMETH(randchar, 0),
    CHMETH(loadprog, 0),
    CHMETH(loadscript, 0),
    CHMETH(loadstring, 0),
    CHMETH(loadfunction, 0),
    CHMETH(savetbl, 0),
    CHMETH(loadtbl, 0),
    CHMETH(tprint, 0),
    CHMETH(olc, 0),
    CHMETH(delay, 0),
    CHMETH(cancel, 0), 
    ENDPTABLE
}; 

/* end CH section */

/* OBJ section */
static int OBJ_exitflag( lua_State *LS )
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (ud_obj->item_type != ITEM_PORTAL)
        luaL_error( LS, "%s(%d) is not a portal.",
                ud_obj->name, ud_obj->pIndexData->vnum);
    return check_iflag( LS, "exit", exit_flags, ud_obj->value[1] );
}
HELPTOPIC OBJ_exitflag_help={};

static int OBJ_portalflag( lua_State *LS )
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (ud_obj->item_type != ITEM_PORTAL)
        luaL_error( LS, "%s(%d) is not a portal.",
                ud_obj->name, ud_obj->pIndexData->vnum);
    return check_iflag( LS, "portal", portal_flags, ud_obj->value[2] );
}
HELPTOPIC OBJ_portalflag_help={};

static int OBJ_loadfunction (lua_State *LS)
{
    lua_obj_program( NULL, RUNDELAY_VNUM, NULL,
                check_OBJ(LS, -2), NULL,
                NULL, NULL,
                TRIG_CALL, 0 );
    return 0;
}
HELPTOPIC OBJ_loadfunction_help = {};

static int OBJ_delay (lua_State *LS)
{
    return L_delay(LS);
}
HELPTOPIC OBJ_delay_help={};

static int OBJ_cancel (lua_State *LS)
{
    return L_cancel(LS);
}
HELPTOPIC OBJ_cancel_help={};

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
HELPTOPIC OBJ_savetbl_help={};

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
HELPTOPIC OBJ_loadtbl_help={};

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
            lua_obj_program( NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH), ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );

    return 1;

}
HELPTOPIC OBJ_loadscript_help={};

static int OBJ_loadstring (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_pushboolean( LS,
            lua_obj_program( NULL, LOADSCRIPT_VNUM, check_string( LS, 2, MAX_SCRIPT_LENGTH), ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );
    return 1;
}
HELPTOPIC OBJ_loadstring_help={};

static int OBJ_loadprog (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OPROG_CODE *pOcode;

    if ( (pOcode = get_oprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: oprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_obj_program( NULL, num, pOcode->code, ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );

    return 1;
}
HELPTOPIC OBJ_loadprog_help={};

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
HELPTOPIC OBJ_destroy_help={};

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

    OBJ_DATA *obj=create_object( pObjIndex, 0);
    check_enchant_obj( obj );
    obj_to_obj(obj,ud_obj);

    if ( !make_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}
HELPTOPIC OBJ_oload_help={};

static int OBJ_extra( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    return check_flag( LS, "extra", extra_flags, ud_obj->extra_flags );
}
HELPTOPIC OBJ_extra_help={};

static int OBJ_wear( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    return check_flag( LS, "wear", wear_flags, ud_obj->wear_flags );
}
HELPTOPIC OBJ_wear_help={};

static int OBJ_echo( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    char *argument= check_fstring( LS, 2, MIL);

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
HELPTOPIC OBJ_echo_help={};

static int OBJ_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, OBJ_echo );
    /* now line up arguments for echo */
    lua_pushvalue( LS, 1); /* obj */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;

}
HELPTOPIC OBJ_tprint_help={};

static int OBJ_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->name);
    return 1;
}
HELPTOPIC OBJ_get_name_help={};

static int OBJ_set_name (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->name);
    ud_obj->name=str_dup(arg);
    return 0;
}
HELPTOPIC OBJ_set_name_help={};

static int OBJ_get_shortdescr (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->short_descr);
    return 1;
}
HELPTOPIC OBJ_get_shortdescr_help={};

static int OBJ_set_shortdescr (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->short_descr);
    ud_obj->short_descr=str_dup(arg);
    return 0;
}
HELPTOPIC OBJ_set_shortdescr_help={};

static int OBJ_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_OBJ(LS,1))->clan].name);
    return 1;
}
HELPTOPIC OBJ_get_clan_help={};

static int OBJ_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->rank);
    return 1;
}
HELPTOPIC OBJ_get_clanrank_help={};

static int OBJ_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->level);
    return 1;
}
HELPTOPIC OBJ_get_level_help={};

static int OBJ_set_level (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int arg=luaL_checkinteger(LS,2);
    
    ud_obj->level=arg;
    return 0;
}
HELPTOPIC OBJ_set_level_help={};

static int OBJ_get_owner (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->owner)
        return 0;
    lua_pushstring( LS,
            ud_obj->owner);
    return 1;
}
HELPTOPIC OBJ_get_owner_help={};

static int OBJ_set_owner (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->owner);
    ud_obj->owner=str_dup(arg);
    return 0;
}
HELPTOPIC OBJ_set_owner_help={};

static int OBJ_get_cost (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->cost);
    return 1;
}
HELPTOPIC OBJ_get_cost_help={};

static int OBJ_get_material (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->material);
    return 1;
}
HELPTOPIC OBJ_get_material_help={};

static int OBJ_set_material (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->material);
    ud_obj->material=str_dup(arg);
    return 0;
}
HELPTOPIC OBJ_set_material_help={};

static int OBJ_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->pIndexData->vnum);
    return 1;
}
HELPTOPIC OBJ_get_vnum_help={};

static int OBJ_get_otype (lua_State *LS)
{
    lua_pushstring( LS,
            item_name((check_OBJ(LS,1))->item_type));
    return 1;
}
HELPTOPIC OBJ_get_otype_help={};

static int OBJ_get_weight (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->weight);
    return 1;
}
HELPTOPIC OBJ_get_weight_help={};

static int OBJ_set_weight (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int arg=luaL_checkinteger(LS,2);
    
    ud_obj->weight=arg;
    return 0;
}
HELPTOPIC OBJ_set_weight_help={};

static int OBJ_get_wearlocation (lua_State *LS)
{
    lua_pushstring( LS,
            flag_stat_string(wear_loc_flags,(check_OBJ(LS,1))->wear_loc) );
    return 1;
}
HELPTOPIC OBJ_get_wearlocation_help={};

static int OBJ_get_proto (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!make_OBJPROTO( LS, ud_obj->pIndexData) )
        return 0;
    else
        return 1;
}
HELPTOPIC OBJ_get_proto_help={};

static int OBJ_get_contents (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_obj->contains ; obj ; obj=obj->next_content)
    {
        if ( make_OBJ(LS, obj) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC OBJ_get_contents_help={};

static int OBJ_get_room (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->in_room)
        return 0;
    if ( !make_ROOM(LS, ud_obj->in_room) )
        return 0;
    else
        return 1;
}
HELPTOPIC OBJ_get_room_help={};

static int OBJ_get_inobj (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->in_obj)
        return 0;

    if ( !make_OBJ(LS, ud_obj->in_obj) )
        return 0;
    else
        return 1;
}
HELPTOPIC OBJ_get_inobj_help={};

static int OBJ_get_carriedby (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->carried_by )
        return 0;
    else if (!make_CH( LS, ud_obj->carried_by) )
        return 0;
    else
        return 1;
}
HELPTOPIC OBJ_get_carriedby_help={};

static int OBJ_get_v0 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[0]);
    return 1;
}
HELPTOPIC OBJ_get_v0_help={};

static int OBJ_get_v1 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[1]);
    return 1;
}
HELPTOPIC OBJ_get_v1_help={};


static int OBJ_get_v2 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[2]);
    return 1;
}
HELPTOPIC OBJ_get_v2_help={};

static int OBJ_get_v3 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[3]);
    return 1;
}
HELPTOPIC OBJ_get_v3_help={};

static int OBJ_get_v4 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[4]);
    return 1;
}
HELPTOPIC OBJ_get_v4_help={};

static int OBJ_get_liquid (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    
    if (ud_obj->item_type != ITEM_FOUNTAIN)
        luaL_error(LS, "Liquid for fountain only.");
        
    lua_pushstring(LS, liq_table[ud_obj->value[2]].liq_name);
    
    return 1;
}
HELPTOPIC OBJ_get_liquid_help={};

static int OBJ_get_total (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    
    if (ud_obj->item_type != ITEM_FOUNTAIN)
        luaL_error(LS, "Total for fountain only.");
        
    lua_pushstring(LS, ud_obj->value[0]);
    
    return 1;
}
HELPTOPIC OBJ_get_total_help={};

static int OBJ_get_left (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    
    if (ud_obj->item_type != ITEM_FOUNTAIN)
        luaL_error(LS, "Left for fountain only.");
        
    lua_pushstring(LS, ud_obj->value[1]);
    
    return 1;
}
HELPTOPIC OBJ_get_left_help={};

static const LUA_PROP_TYPE OBJ_get_table [] =
{
    OBJGET(name, 0),
    OBJGET(shortdescr, 0),
    OBJGET(clan, 0),
    OBJGET(clanrank, 0),
    OBJGET(level, 0),
    OBJGET(owner, 0),
    OBJGET(cost, 0),
    OBJGET(material, 0),
    OBJGET(vnum, 0),
    OBJGET(otype, 0),
    OBJGET(weight, 0),
    OBJGET(room, 0),
    OBJGET(inobj, 0),
    OBJGET(carriedby, 0),
    OBJGET(v0, 0),
    OBJGET(v1, 0),
    OBJGET(v2, 0),
    OBJGET(v3, 0),
    OBJGET(v4, 0),
    OBJGET(wearlocation, 0),
    OBJGET(contents, 0),
    OBJGET(proto, 0),
    
    /*fountain*/
    OBJGET(liquid, 0),
    OBJGET(left, 0),
    OBJGET(total, 0),
    
    ENDPTABLE
};

static const LUA_PROP_TYPE OBJ_set_table [] =
{
    OBJSET(name, 9 ),
    OBJSET(shortdescr, 9),
    OBJSET(level, 9),
    OBJSET(owner, 9),
    OBJSET(material, 9),
    OBJSET(weight, 9),
       
    ENDPTABLE
};


static const LUA_PROP_TYPE OBJ_method_table [] =
{
    OBJMETH(extra, 0),
    OBJMETH(wear, 0),
    OBJMETH(destroy, 0),
    OBJMETH(echo, 0),
    OBJMETH(loadprog, 0),
    OBJMETH(loadscript, 0),
    OBJMETH(loadstring, 0),
    OBJMETH(loadfunction, 0),
    OBJMETH(oload, 0),
    OBJMETH(savetbl, 0),
    OBJMETH(loadtbl, 0),
    OBJMETH(tprint, 0),
    OBJMETH(delay, 0),
    OBJMETH(cancel, 0),

    /* portal only */
    OBJMETH(exitflag, 0),
    OBJMETH(portalflag, 0),
    ENDPTABLE
}; 

/* end OBJ section */

/* AREA section */
static int AREA_loadfunction( lua_State *LS)
{
    lua_area_program( NULL, RUNDELAY_VNUM, NULL,
                check_AREA(LS, -2), NULL,
                TRIG_CALL, 0 );
    return 0;
}
HELPTOPIC AREA_loadfunction_help = {};

static int AREA_delay (lua_State *LS)
{
    return L_delay(LS);
}
HELPTOPIC AREA_delay_help={};

static int AREA_cancel (lua_State *LS)
{
    return L_cancel(LS);
}
HELPTOPIC AREA_cancel_help={};

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
HELPTOPIC AREA_savetbl_help={};

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
HELPTOPIC AREA_loadtbl_help={};

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
            lua_area_program( NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH), ud_area, NULL, ATRIG_CALL, 0) );

    return 1;
}
HELPTOPIC AREA_loadscript_help={};

static int AREA_loadstring (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);
    lua_pushboolean( LS,
            lua_area_program( NULL, LOADSCRIPT_VNUM, check_string( LS, 2, MAX_SCRIPT_LENGTH), ud_area, NULL, ATRIG_CALL, 0) );
    return 1;
}
HELPTOPIC AREA_loadstring_help={};

static int AREA_loadprog (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    APROG_CODE *pAcode;

    if ( (pAcode = get_aprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: aprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_area_program( NULL, num, pAcode->code, ud_area, NULL, ATRIG_CALL, 0) );

    return 1;
}
HELPTOPIC AREA_loadprog_help={};

static int AREA_flag( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS, 1);
    return check_flag( LS, "area", area_flags, ud_area->area_flags );
}
HELPTOPIC AREA_flag_help={};

static int AREA_echo( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS, 1);
    const char *argument = check_fstring( LS, 2, MSL);
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
HELPTOPIC AREA_echo_help={};

static int AREA_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, AREA_echo );
    /* now line up arguments for echo */
    lua_pushvalue( LS, 1); /* area */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;

}
HELPTOPIC AREA_tprint_help={};

static int AREA_get_name( lua_State *LS)
{
    lua_pushstring( LS, (check_AREA(LS, 1))->name);
    return 1;
}
HELPTOPIC AREA_get_name_help={};

static int AREA_get_filename( lua_State *LS)
{
    lua_pushstring( LS, (check_AREA(LS, 1))->file_name);
    return 1;
}
HELPTOPIC AREA_get_filename_help={};

static int AREA_get_nplayer( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->nplayer);
    return 1;
}
HELPTOPIC AREA_get_nplayer_help={};

static int AREA_get_minlevel( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->minlevel);
    return 1;
}
HELPTOPIC AREA_get_minlevel_help={};

static int AREA_get_maxlevel( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->maxlevel);
    return 1;
}
HELPTOPIC AREA_get_maxlevel_help={};

static int AREA_get_security( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->security);
    return 1;
}
HELPTOPIC AREA_get_security_help={};

static int AREA_get_ingame( lua_State *LS)
{
    lua_pushboolean( LS, is_area_ingame(check_AREA(LS, 1)));
    return 1;
}
HELPTOPIC AREA_get_ingame_help={};

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
        if (make_AREA(LS, room))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC AREA_get_rooms_help={};

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
        if (make_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC AREA_get_people_help={};

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
        if (make_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC AREA_get_players_help={};

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
        if (make_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC AREA_get_mobs_help={};

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
            if (make_MOBPROTO(LS, mid))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}
HELPTOPIC AREA_get_mobprotos_help={};

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
            if (make_OBJPROTO(LS, oid))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}
HELPTOPIC AREA_get_objprotos_help = {
    .summary="A table of all OBJPROTOS in the area."
};

static const LUA_PROP_TYPE AREA_get_table [] =
{
    AREAGET(name, 0),
    AREAGET(filename, 0),
    AREAGET(nplayer, 0),
    AREAGET(minlevel, 0),
    AREAGET(maxlevel, 0),
    AREAGET(security, 0),
    AREAGET(ingame, 0),
    AREAGET(rooms, 0),
    AREAGET(people, 0),
    AREAGET(players, 0),
    AREAGET(mobs, 0),
    AREAGET(mobprotos, 0),
    AREAGET(objprotos, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE AREA_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE AREA_method_table [] =
{
    AREAMETH(flag, 0),
    AREAMETH(echo, 0),
    AREAMETH(loadprog, 0),
    AREAMETH(loadscript, 0),
    AREAMETH(loadstring, 0),
    AREAMETH(loadfunction, 0),
    AREAMETH(savetbl, 0),
    AREAMETH(loadtbl, 0),
    AREAMETH(tprint, 0),
    AREAMETH(delay, 0),
    AREAMETH(cancel, 0),
    ENDPTABLE
}; 

/* end AREA section */

/* ROOM section */
static int ROOM_loadfunction ( lua_State *LS)
{
    lua_room_program( NULL, RUNDELAY_VNUM, NULL,
                check_ROOM(LS, -2), NULL,
                NULL, NULL, NULL,
                TRIG_CALL, 0 );
    return 0;
}
HELPTOPIC ROOM_loadfunction_help = {};

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

    if ( !make_CH(LS, mob))
        return 0;
    else
        return 1;

}
HELPTOPIC ROOM_mload_help={};

static int ROOM_oload (lua_State *LS)
{
    ROOM_INDEX_DATA * ud_room = check_ROOM (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj=create_object( pObjIndex, 0);
    check_enchant_obj( obj );
    obj_to_room(obj,ud_room);

    if ( !make_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}
HELPTOPIC ROOM_oload_help={};

static int ROOM_flag( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room = check_ROOM(LS, 1);
    return check_flag( LS, "room", room_flags, ud_room->room_flags );
}
HELPTOPIC ROOM_flag_help={};

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
HELPTOPIC ROOM_echo_help={};

static int ROOM_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, ROOM_echo );
    /* now line up argumenets for echo */
    lua_pushvalue( LS, 1); /* obj */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;
}
HELPTOPIC ROOM_tprint_help={};

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
HELPTOPIC ROOM_savetbl_help={};

static int ROOM_loadtbl (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_room->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}
HELPTOPIC ROOM_loadtbl_help={};

static int ROOM_loadscript (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    lua_pushboolean( LS,
            lua_room_program( NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH),
                ud_room, NULL, NULL, NULL, NULL, RTRIG_CALL, 0) );
    return 1;
}
HELPTOPIC ROOM_loadscript_help={};

static int ROOM_loadstring (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_pushboolean( LS,
            lua_room_program( NULL, LOADSCRIPT_VNUM, check_string(LS, 2, MAX_SCRIPT_LENGTH),
                ud_room, NULL, NULL, NULL, NULL, RTRIG_CALL, 0) );
    return 1;
}
HELPTOPIC ROOM_loadstring_help={};

static int ROOM_loadprog (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int num = (int)luaL_checknumber (LS, 2);
    RPROG_CODE *pRcode;

    if ( (pRcode = get_rprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: rprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_room_program( NULL, num, pRcode->code,
                ud_room, NULL, NULL, NULL, NULL,
                RTRIG_CALL, 0) );
    return 1;
}
HELPTOPIC ROOM_loadprog_help={};

static int ROOM_delay (lua_State *LS)
{
    return L_delay(LS);
}
HELPTOPIC ROOM_delay_help={};

static int ROOM_cancel (lua_State *LS)
{
    return L_cancel(LS);
}
HELPTOPIC ROOM_cancel_help={};

static int ROOM_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->name);
    return 1;
}
HELPTOPIC ROOM_get_name_help={};

static int ROOM_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->vnum);
    return 1;
}
HELPTOPIC ROOM_get_vnum_help={};

static int ROOM_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[check_ROOM(LS,1)->clan].name);
    return 1;
}
HELPTOPIC ROOM_get_clan_help={};

static int ROOM_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->clan_rank);
    return 1;
}
HELPTOPIC ROOM_get_clanrank_help={};

static int ROOM_get_healrate (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->heal_rate);
    return 1;
}
HELPTOPIC ROOM_get_healrate_help={};

static int ROOM_get_manarate (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->mana_rate);
    return 1;
}
HELPTOPIC ROOM_get_manarate_help={};

static int ROOM_get_owner (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->owner);
    return 1;
}
HELPTOPIC ROOM_get_owner_help={};

static int ROOM_get_description (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->description);
    return 1;
}
HELPTOPIC ROOM_get_description_help={};

static int ROOM_get_sector (lua_State *LS)
{
    lua_pushstring( LS,
            flag_bit_name(sector_flags, (check_ROOM(LS,1))->sector_type) );
    return 1;
}
HELPTOPIC ROOM_get_sector_help={};

static int ROOM_get_contents (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_room->contents ; obj ; obj=obj->next_content)
    {
        if (make_OBJ(LS, obj))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC ROOM_get_contents_help={};

static int ROOM_get_area (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    if ( !make_AREA(LS, ud_room->area))
        return 0;
    else
        return 1;
}
HELPTOPIC ROOM_get_area_help={};

static int ROOM_get_people (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=ud_room->people ; people ; people=people->next_in_room)
    {
        if (make_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC ROOM_get_people_help={};

static int ROOM_get_players (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *plr;
    for ( plr=ud_room->people ; plr ; plr=plr->next_in_room)
    {
        if (!IS_NPC(plr) && make_CH(LS, plr))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC ROOM_get_players_help={};

static int ROOM_get_mobs (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *mob;
    for ( mob=ud_room->people ; mob ; mob=mob->next_in_room)
    {
        if ( IS_NPC(mob) && make_CH(LS, mob))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC ROOM_get_mobs_help={};

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
HELPTOPIC ROOM_get_exits_help={};

#define ROOM_dir(dirname, dirnumber) static int ROOM_get_ ## dirname (lua_State *LS)\
{\
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);\
    if (!ud_room->exit[dirnumber])\
        return 0;\
    if (!make_EXIT(LS, ud_room->exit[dirnumber]))\
        return 0;\
    else\
        return 1;\
}\
HELPTOPIC ROOM_get_ ## dirname ## _help={ };

ROOM_dir(north, DIR_NORTH)
ROOM_dir(south, DIR_SOUTH)
ROOM_dir(east, DIR_EAST)
ROOM_dir(west, DIR_WEST)
ROOM_dir(northeast, DIR_NORTHEAST)
ROOM_dir(northwest, DIR_NORTHWEST)
ROOM_dir(southeast, DIR_SOUTHEAST)
ROOM_dir(southwest, DIR_SOUTHWEST)
ROOM_dir(up, DIR_UP)
ROOM_dir(down, DIR_DOWN)

static int ROOM_get_resets (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_newtable(LS);
    int index=1;
    RESET_DATA *reset;
    for ( reset=ud_room->reset_first; reset; reset=reset->next)
    {
        if (make_RESET(LS, reset) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}
HELPTOPIC ROOM_get_resets_help={};


static const LUA_PROP_TYPE ROOM_get_table [] =
{
    ROOMGET(name, 0),
    ROOMGET(vnum, 0),
    ROOMGET(clan, 0),
    ROOMGET(clanrank, 0),
    ROOMGET(healrate, 0),
    ROOMGET(manarate, 0),
    ROOMGET(owner, 0),
    ROOMGET(description, 0),
    ROOMGET(sector, 0),
    ROOMGET(contents, 0),
    ROOMGET(area, 0),
    ROOMGET(people, 0),
    ROOMGET(players, 0),
    ROOMGET(mobs, 0),
    ROOMGET(exits, 0),
    ROOMGET(north, 0),
    ROOMGET(south, 0),
    ROOMGET(east, 0),
    ROOMGET(west, 0),
    ROOMGET(northwest, 0),
    ROOMGET(northeast, 0),
    ROOMGET(southwest, 0),
    ROOMGET(southeast, 0),
    ROOMGET(up, 0),
    ROOMGET(down, 0),
    ROOMGET(resets, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE ROOM_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE ROOM_method_table [] =
{
    ROOMMETH(flag, 0),
    ROOMMETH(oload, 0),
    ROOMMETH(mload, 0),
    ROOMMETH(echo, 0),
    ROOMMETH(loadprog, 0),
    ROOMMETH(loadscript, 0),
    ROOMMETH(loadstring, 0),
    ROOMMETH(loadfunction, 0),
    ROOMMETH(tprint, 0),
    ROOMMETH(delay, 0),
    ROOMMETH(cancel, 0),
    ROOMMETH(savetbl, 0),
    ROOMMETH(loadtbl, 0),
    ENDPTABLE
}; 

/* end ROOM section */

/* EXIT section */
static int EXIT_flag (lua_State *LS)
{
    EXIT_DATA *ed=EXIT_type->check(EXIT_type, LS, 1 );
    return check_flag( LS, "exit", exit_flags, ed->exit_info );
}
HELPTOPIC EXIT_flag_help={};

static int EXIT_setflag( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);
    return set_flag( LS, "exit", exit_flags, ud_exit->exit_info); 
}
HELPTOPIC EXIT_setflag_help={};

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
HELPTOPIC EXIT_lock_help={};

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
HELPTOPIC EXIT_unlock_help={};

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
HELPTOPIC EXIT_close_help={};

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
HELPTOPIC EXIT_open_help={};

static int EXIT_get_toroom (lua_State *LS)
{
    EXIT_DATA *ud_exit=check_EXIT(LS,1);
    if ( !make_ROOM( LS, ud_exit->u1.to_room ))
        return 0;
    else
        return 1;
}
HELPTOPIC EXIT_get_toroom_help={};

static int EXIT_get_keyword (lua_State *LS)
{
    lua_pushstring(LS,
            (check_EXIT(LS,1))->keyword);
    return 1;
}
HELPTOPIC EXIT_get_keyword_help={};

static int EXIT_get_description (lua_State *LS)
{
    lua_pushstring(LS,
            (check_EXIT(LS,1))->description);
    return 1;
}
HELPTOPIC EXIT_get_description_help={};

static int EXIT_get_key (lua_State *LS)
{
    lua_pushinteger(LS,
            (check_EXIT(LS,1))->key);
    return 1;
}
HELPTOPIC EXIT_get_key_help={};

static const LUA_PROP_TYPE EXIT_get_table [] =
{
    EXGET(toroom, 0),
    EXGET(keyword,0),
    EXGET(description, 0),
    EXGET(key, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE EXIT_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE EXIT_method_table [] =
{
    EXMETH(flag, 0),
    EXMETH(setflag, 0),
    EXMETH(open, 0),
    EXMETH(close, 0),
    EXMETH(unlock, 0),
    EXMETH(lock, 0),
    ENDPTABLE
}; 

/* end EXIT section */

/* RESET section */
static int RESET_get_command(lua_State *LS, RESET_DATA *rd )
{
    static char buf[2];
    sprintf(buf, "%c", rd->command);
    lua_pushstring(LS, buf);
    return 1;
}
HELPTOPIC RESET_get_command_help={};

#define RESETGETARG( num ) static int RESET_get_arg ## num ( lua_State *LS)\
{\
    lua_pushinteger( LS,\
            (check_RESET(LS,1))->arg ## num);\
}\
HELPTOPIC RESET_get_arg ## num ## _help={}

RESETGETARG(1);
RESETGETARG(2);
RESETGETARG(3);
RESETGETARG(4);

static const LUA_PROP_TYPE RESET_get_table [] =
{
    RSTGET( command, 0),
    RSTGET( arg1, 0),
    RSTGET( arg2, 0),
    RSTGET( arg3, 0),
    RSTGET( arg4, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE RESET_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE RESET_method_table [] =
{
    ENDPTABLE
}; 

/* end RESET section */

/* OBJPROTO section */
static int OBJPROTO_exitflag( lua_State *LS )
{
    OBJ_INDEX_DATA *ud_op=check_OBJPROTO(LS,1);
    if (ud_op->item_type != ITEM_PORTAL)
        luaL_error( LS, "%s(%d) is not a portal.",
                ud_op->name, ud_op->vnum);
    return check_iflag( LS, "exit", exit_flags, ud_op->value[1] );
}
HELPTOPIC OBJPROTO_exitflag_help={};

static int OBJPROTO_portalflag( lua_State *LS )
{
    OBJ_INDEX_DATA *ud_op=check_OBJPROTO(LS,1);
    if (ud_op->item_type != ITEM_PORTAL)
        luaL_error( LS, "%s(%d) is not a portal.",
                ud_op->name, ud_op->vnum);
    return check_iflag( LS, "portal", portal_flags, ud_op->value[2] );
}
HELPTOPIC OBJPROTO_portalflag_help={};

static int OBJPROTO_wear( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    return check_flag( LS, "wear", wear_flags, ud_objp->wear_flags );
}
HELPTOPIC OBJPROTO_wear_help={};

static int OBJPROTO_extra( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    return check_flag( LS, "extra", extra_flags, ud_objp->extra_flags );
}
HELPTOPIC OBJPROTO_extra_help={};

static int OBJPROTO_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->name);
    return 1;
}
HELPTOPIC OBJPROTO_get_name_help={};

static int OBJPROTO_get_shortdescr (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->short_descr);
    return 1;
}
HELPTOPIC OBJPROTO_get_shortdescr_help={};

static int OBJPROTO_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_OBJPROTO(LS,1))->clan].name);
    return 1;
}
HELPTOPIC OBJPROTO_get_clan_help={};

static int OBJPROTO_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->rank);
    return 1;
}
HELPTOPIC OBJPROTO_get_clanrank_help={};

static int OBJPROTO_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->level);
    return 1;
}
HELPTOPIC OBJPROTO_get_level_help={};

static int OBJPROTO_get_cost (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->cost);
    return 1;
}
HELPTOPIC OBJPROTO_get_cost_help={};

static int OBJPROTO_get_material (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->material);
    return 1;
}
HELPTOPIC OBJPROTO_get_material_help={};

static int OBJPROTO_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->vnum);
    return 1;
}
HELPTOPIC OBJPROTO_get_vnum_help={};

static int OBJPROTO_get_otype (lua_State *LS)
{
    lua_pushstring( LS,
            item_name((check_OBJPROTO(LS,1))->item_type));
    return 1;
}
HELPTOPIC OBJPROTO_get_otype_help={};

static int OBJPROTO_get_weight (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->weight);
    return 1;
}
HELPTOPIC OBJPROTO_get_weight_help={};

#define OPGETV( num ) static int OBJPROTO_get_v ## num (lua_State *LS)\
{\
    lua_pushinteger( LS,\
            (check_OBJPROTO(LS,1))->value[num]);\
    return 1;\
}\
HELPTOPIC OBJPROTO_get_v ## num ## _help = {}

OPGETV(0);
OPGETV(1);
OPGETV(2);
OPGETV(3);
OPGETV(4);


static const LUA_PROP_TYPE OBJPROTO_get_table [] =
{
    OPGET( name, 0),
    OPGET( shortdescr, 0),
    OPGET( clan, 0),
    OPGET( clanrank, 0),
    OPGET( level, 0),
    OPGET( cost, 0),
    OPGET( material, 0),
    OPGET( vnum, 0),
    OPGET( otype, 0),
    OPGET( weight, 0),
    OPGET( v0, 0),
    OPGET( v1, 0),
    OPGET( v2, 0),
    OPGET( v3, 0),
    OPGET( v4, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE OBJPROTO_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE OBJPROTO_method_table [] =
{
    OPMETH( extra, 0),
    OPMETH( wear, 0),
   
    /* portal only */
    OPMETH( exitflag, 0),
    OPMETH( portalflag, 0),
    ENDPTABLE
}; 

/* end OBJPROTO section */

/* MOBPROTO section */
static int MOBPROTO_affected (lua_State *LS)
{
    MOB_INDEX_DATA *ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "affected", affect_flags, ud_mobp->affect_field );
}
HELPTOPIC MOBPROTO_affected_help={};

static int MOBPROTO_act (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "act", act_flags, ud_mobp->act );
}
HELPTOPIC MOBPROTO_act_help={};

static int MOBPROTO_offensive (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "offensive", off_flags, ud_mobp->off_flags );
}
HELPTOPIC MOBPROTO_offensive_help={};

static int MOBPROTO_immune (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "immune", imm_flags, ud_mobp->imm_flags );
}
HELPTOPIC MOBPROTO_immune_help={};

static int MOBPROTO_vuln (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "vuln", vuln_flags, ud_mobp->vuln_flags );
}
HELPTOPIC MOBPROTO_vuln_help={};

static int MOBPROTO_resist (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "resist", res_flags, ud_mobp->res_flags );
}
HELPTOPIC MOBPROTO_resist_help={};

#define MPGETSTR( field, val, helpval ) static int MOBPROTO_get_ ## field (lua_State *LS)\
{\
    MOB_INDEX_DATA *ud_mobp=check_MOBPROTO(LS,1);\
    lua_pushstring(LS, val );\
    return 1;\
}\
HELPTOPIC MOBPROTO_get_ ## field ## _help = { helpval }

#define MPGETINT( field, val, helpval ) static int MOBPROTO_get_ ## field (lua_State *LS)\
{\
    MOB_INDEX_DATA *ud_mobp=check_MOBPROTO(LS,1);\
    lua_pushinteger(LS, val );\
    return 1;\
}\
HELPTOPIC MOBPROTO_get_ ## field ## _help = { helpval }

MPGETINT( vnum, ud_mobp->vnum, );
MPGETSTR( name, ud_mobp->player_name, );
MPGETSTR( shortdescr, ud_mobp->short_descr, );
MPGETSTR( longdescr, ud_mobp->long_descr, );
MPGETSTR( description, ud_mobp->description, );
MPGETINT( alignment, ud_mobp->alignment, );
MPGETINT( level, ud_mobp->level, );
MPGETINT( hppcnt, ud_mobp->hitpoint_percent, );
MPGETINT( mnpcnt, ud_mobp->mana_percent, );
MPGETINT( mvpcnt, ud_mobp->move_percent, );
MPGETINT( hrpcnt, ud_mobp->hitroll_percent, );
MPGETINT( drpcnt, ud_mobp->damage_percent, );
MPGETINT( acpcnt, ud_mobp->ac_percent, );
MPGETINT( savepcnt, ud_mobp->saves_percent, );
MPGETSTR( damtype, attack_table[ud_mobp->dam_type].name, );
MPGETSTR( startpos, flag_stat_string( position_flags, ud_mobp->start_pos ), );
MPGETSTR( defaultpos, flag_stat_string( position_flags, ud_mobp->default_pos ), );
MPGETSTR( sex,
    ud_mobp->sex == SEX_NEUTRAL ? "neutral" :
    ud_mobp->sex == SEX_MALE    ? "male" :
    ud_mobp->sex == SEX_FEMALE  ? "female" :
    ud_mobp->sex == SEX_BOTH    ? "random" :
    NULL, );
MPGETSTR( race, race_table[ud_mobp->race].name, );
MPGETINT( wealthpcnt, ud_mobp->wealth_percent, );
MPGETSTR( size, flag_stat_string( size_flags, ud_mobp->size ), );
MPGETSTR( stance, stances[ud_mobp->stance].name, );
    
static const LUA_PROP_TYPE MOBPROTO_get_table [] =
{
    MPGET( vnum, 0),
    MPGET( name, 0),
    MPGET( shortdescr, 0),
    MPGET( longdescr, 0),
    MPGET( description, 0),
    MPGET( alignment, 0),
    MPGET( level, 0),
    MPGET( hppcnt, 0),
    MPGET( mnpcnt, 0),
    MPGET( mvpcnt, 0),
    MPGET( hrpcnt, 0),
    MPGET( drpcnt, 0),
    MPGET( acpcnt, 0),
    MPGET( savepcnt, 0),
    MPGET( damtype, 0),
    MPGET( startpos, 0),
    MPGET( defaultpos, 0),
    MPGET( sex, 0),
    MPGET( race, 0),
    MPGET( wealthpcnt, 0),
    MPGET( size, 0),
    MPGET( stance, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE MOBPROTO_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE MOBPROTO_method_table [] =
{
    MPMETH( act, 0),
    MPMETH( vuln, 0),
    MPMETH( immune, 0),
    MPMETH( offensive, 0),
    MPMETH( resist, 0),
    MPMETH( affected, 0),
    ENDPTABLE
}; 

/* end MOBPROTO section */


/* help section */

/* add ptable output to existing buffer */
static void print_ptable( BUFFER *buffer, const struct prop_type *ptable )
{
    char buf[MSL];
    
    int j;
    #define CDEF 'w'
    #define CALT 'D'
    bool col=FALSE;
    add_buf( buffer, "\n\rSec Name\n\r");
    for ( j=0 ; ptable[j].field ; j++ )
    {
        if ( ptable[j].status == STS_DEPRECATED )
            continue;
            
        sprintf( buf, "{%c[%d] %-16s - ", 
                col ? CALT : CDEF,
                ptable[j].security, ptable[j].field );
        col=!col;
        if (ptable[j].help && ptable[j].help->summary)
            strcat( buf, ptable[j].help->summary );
        strcat( buf, "\n\r{x");
        add_buf( buffer, buf );
    }
    
    return;
}
    
static void print_help_usage( CHAR_DATA *ch )
{
    OBJ_TYPE *ot;
    int i;

    ptc( ch, "\n\rSECTIONS: \n\r\n\r" );

    ptc( ch, "global\n\r\n\r" );

    for ( i=0 ; type_list[i] ; i++ )
    {
        ot=*(OBJ_TYPE **)type_list[i];
        ptc( ch, "%s\n\r", ot->type_name);
    }
    
    ptc( ch,
    "\n\rSyntax: \n\r"
    "    luahelp <section>                - List all entries in section.\n\r"
    "    luahelp <section> <get|set|meth> - List only get/set/method entries respectively.\n\r"
    "    luahelp <section> <topic>        - Print full topic.\n\r"
    "\n\r"
    "Examples: \n\r"
    "    luahelp ch\n\r"
    "    luahelp global\n\r"
    "    luahelp obj meth\n\r"
    "    luahelp obj name\n\r"
    "    luahelp global sendtochar\n\r");
}

static void print_topic( CHAR_DATA *ch, HELPTOPIC *topic )
{
    if (!topic)
    {
        ptc(ch, "No info for this topic." );
        return;
    }
    bool printed=FALSE;

    if (topic->summary)
    {
        ptc(ch, "Summary:\n\r%s\n\r\n\r", topic->summary );
        printed=TRUE;
    }

    if (topic->info)
    {
        ptc(ch, "%s\n\r", topic->info );
        printed=TRUE;
    }

    if (!printed)
        ptc( ch, "Empty help.\n\r");
}

/*static void help_three_arg( CHAR_DATA *ch, const char *arg1, const char *arg2, const char *arg3)
{
}
*/

static void help_two_arg( CHAR_DATA *ch, const char *arg1, const char *arg2 )
{
    OBJ_TYPE *ot;
    int i;

    if ( !str_prefix("glob", arg1) )
    {
        for ( i=0 ; glob_table[i].name ; i++ )
        {
            if ( glob_table[i].status == STS_DEPRECATED || glob_table[i].security == SEC_NOSCRIPT)
                continue;
                    
            if (glob_table[i].lib)
            {
                char buf[MSL];
                sprintf(buf, "%s.%s", glob_table[i].lib, glob_table[i].name);

                if (!strcmp( buf, arg2 ) )
                {
                    print_topic( ch, glob_table[i].help );
                    return;
                }
            }
            else
            {
                if (!strcmp( glob_table[i].name, arg2 ) )
                {
                    print_topic( ch, glob_table[i].help );
                    return;
                }
            }
        }

        ptc(ch, "No global function '%s'\n\r", arg2 );
        return;
    }
    

    for ( i=0 ; type_list[i] ; i++ )
    {
        ot=*(OBJ_TYPE **)type_list[i];
        
        if (!str_cmp( ot->type_name, arg1 ) )
        {
            /* always go through all 3 tables since
               we might have duplicate fields in set and get*/
            int j;
            bool found=FALSE;

            
            if (!str_cmp( arg2, "get") )
            {
                BUFFER *buffer=new_buf();
                print_ptable( buffer, ot->get_table );
                page_to_char( buf_string(buffer), ch);
                return;
            }
            else if (!str_cmp( arg2, "set") )
            {
                BUFFER *buffer=new_buf();
                print_ptable( buffer, ot->set_table );
                page_to_char( buf_string(buffer), ch);
                return;
            }
            else if (!str_prefix( "meth", arg2) )
            {
                BUFFER *buffer=new_buf();
                print_ptable( buffer, ot->method_table );
                page_to_char( buf_string(buffer), ch);
                return;
            }
            
            for ( j=0 ; ot->get_table[j].field ; j++ )
            {
                if (strcmp( ot->get_table[j].field, arg2 ) || ot->get_table[j].status == STS_DEPRECATED )
                    continue;

                found=TRUE;

                print_topic( ch, ot->get_table[j].help );

            }

            for ( j=0 ; ot->set_table[j].field ; j++ )
            {
                if (strcmp( ot->set_table[j].field, arg2 ) || ot->set_table[j].status == STS_DEPRECATED )
                    continue;

                found=TRUE;

                print_topic( ch, ot->set_table[j].help );

            }

            for ( j=0 ; ot->method_table[j].field ; j++ )
            {
                if (strcmp( ot->method_table[j].field, arg2 ) || ot->method_table[j].status == STS_DEPRECATED)
                    continue;

                found=TRUE;

                print_topic( ch, ot->method_table[j].help );
            }
            if (!found)
            {
                ptc( ch, "Didn't find property or method %s for %s.\n\r",
                        arg2, ot->type_name);
            }
            return;
        }
    }

    ptc( ch, "No such help section '%s'", arg1 );
    return;

       
}

static void help_one_arg( CHAR_DATA *ch, const char *arg1 )
{
    OBJ_TYPE *ot;
    int i;

    if ( !str_prefix("glob", arg1) )
    {
        ptc( ch, "\n\rGLOBAL functions\n\r");

        ptc( ch, "\n\rSec Name\n\r");
        bool col=FALSE;
        for ( i=0 ; glob_table[i].name ; i++ )
        {
            if (glob_table[i].status == STS_DEPRECATED || glob_table[i].security == SEC_NOSCRIPT)
                continue;
            
            
            if (glob_table[i].lib)
            {
                char buf[MSL];
                sprintf(buf, "{%c[%d] %s.%s", 
                        col ? CALT : CDEF,
                        glob_table[i].security, glob_table[i].lib, glob_table[i].name);
                col=!col;
                ptc( ch, "%-22s - ", buf);
                if (glob_table[i].help && glob_table[i].help->summary)
                   ptc( ch, glob_table[i].help->summary );
                ptc( ch, "\n\r{x");
            }
            else
            {
                ptc( ch, "{%c[%d] %-16s - ", 
                    col ? CALT : CDEF,
                    glob_table[i].security, glob_table[i].name);
                col=!col;
                if (glob_table[i].help && glob_table[i].help->summary)
                    ptc( ch, glob_table[i].help->summary );
                ptc( ch, "\n\r{x");
            }

        }
        return;
    } 

    for ( i=0 ; type_list[i] ; i++ )
    {
        ot=*(OBJ_TYPE **)type_list[i];
        if (!str_cmp( ot->type_name, arg1 ) )
        {
            int j;
            BUFFER *buffer=new_buf();
            add_buf(buffer, "\n\rGET fields\n\r");
            print_ptable( buffer, ot->get_table );
            add_buf(buffer, "\n\rSET fields\n\r");
            print_ptable( buffer, ot->set_table );
            add_buf(buffer, "\n\rMETHODS\n\r");
            print_ptable( buffer, ot->method_table );
            
            page_to_char(buf_string(buffer), ch);

            return;
        }
    }

    ptc( ch, "No help for %s.\n\r", arg1);
    return;
}

void do_luahelp( CHAR_DATA *ch, const char *argument )
{
    if (argument[0]=='\0')
    {
        print_help_usage( ch );
        return;
    }

    static char arg1[MIL];
    static char arg2[MIL];
    static char arg3[MIL];
    int nargs=0;
    /* grab the args */
    nargs+=1;
    argument=one_argument( argument, arg1 );

    if (!(argument[0]=='\0'))
    {
        nargs+=1;
        argument=one_argument( argument, arg2 );

        if (!(argument[0]=='\0'))
        {
            nargs+=1;
            argument=one_argument( argument, arg3 );
        }
    }

    if (nargs==1)
    {
       help_one_arg(ch, arg1 );
       return;
    } 
    else if (nargs==2)
    {
        help_two_arg(ch, arg1, arg2);
        return;
    }
    else
        print_help_usage( ch );
    /*if (nargs==3)
    {
        help_three_arg(ch, arg1, arg2,arg3);
        return;
    }*/

}

/* end help section */
#define TYPEINIT( typename ) if (! typename ## _type ) \
    typename ## _type=new_obj_type(\
        LS,\
        #typename,\
        typename ## _get_table,\
        typename ## _set_table,\
        typename ## _method_table)

void type_init( lua_State *LS)
{
    TYPEINIT(CH);
    TYPEINIT(OBJ);
    TYPEINIT(AREA);
    TYPEINIT(ROOM);
    TYPEINIT(EXIT);
    TYPEINIT(RESET);
    TYPEINIT(OBJPROTO);
    TYPEINIT(MOBPROTO);
}
