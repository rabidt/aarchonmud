#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_object_type.h"
#include "lua_common.h"
#include "olc.h"
#include "tables.h"


#define GETP(type, field, sec ) { \
    #field , \
    type ## _get_ ## field, \
    sec,  \
    & type ## _get_ ## field ## _help }

#define SETP(type, field, sec) { \
    #field, \
    type ## _set_ ## field, \
    sec, \
    & type ## _set_ ## field ## _help }

#define METH(type, field, sec) { \
    #field, \
    type ## _ ## field, \
    sec, \
    & type ## _ ## field ## _help}

#define CHGET( field, sec ) GETP( CH, field, sec)
#define CHSET( field, sec ) SETP( CH, field, sec)
#define CHMETH( field, sec ) METH( CH, field, sec)

#define ENDPTABLE {NULL, NULL, 0, NULL}

#define check_CH( LS, index) ((CHAR_DATA *)CH_type->check( CH_type, LS, index ))
#define make_CH(LS, ch ) CH_type->make( CH_type, LS, ch )
#define is_CH(LS, ch ) CH_type->is( CH_type, LS, ch )

#define check_OBJ( LS, obj ) ((OBJ_DATA *)OBJ_type->check( OBJ_type, LS, obj ))
#define make_OBJ(LS, obj ) OBJ_type->make( OBJ_type, LS, obj )

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
};

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

static type_init(LS)
{
    if (!CH_type)
        CH_type=CH_init(LS);
    if (!OBJ_type)
        OBJ_type=OBJ_init(LS);
    if (!AREA_type)
        AREA_type=AREA_init(LS);
    if (!ROOM_type)
        ROOM_type=ROOM_init(LS);
    if (!EXIT_type)
        EXIT_type=EXIT_init(LS);
    if (!RESET_type)
        RESET_type=RESET_init(LS);
    if (!OBJPROTO_type)
        OBJPROTO_type=OBJPROTO_init(LS);
    if (!MOBPROTO_type)
        MOBPROTO_type=MOBPROTO_init(LS);
}
        
/* base functionality for lua object types */
static void * check_func( OBJ_TYPE *self,
        lua_State *LS, int index )
{
    lua_getfield(LS, index, "UDTYPE");
    int type=luaL_checkint( LS, -1 );
    lua_pop(LS, 1);
    if ( type != self->udtype )
    {
        luaL_error(LS, "Bad parameter %d. Expected %s. %d %d",
                index, self->type_name, type, self->udtype);
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
    
    if (!strcmp("UDTYPE", arg) )
    {
        lua_pushinteger( LS, obj->udtype );
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
    const char *arg=luaL_checkstring( LS, 2 );

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
}

static bool make_func( OBJ_TYPE *self,
        lua_State *LS, void *game_obj)
{
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

    /* get rid of our original table, register send back a new version */
    lua_remove( LS, -2 );

    return TRUE;
}

bool is_func( OBJ_TYPE *self,
        lua_State *LS, int arg )
{
    if ( !lua_istable(LS, arg ) )
        return FALSE;

    lua_getfield(LS, arg, "UDTYPE");
    sh_int type=luaL_checkint(LS, -1);
    lua_pop(LS, 1);
    return ( type == self->udtype );
}


static OBJ_TYPE *new_obj_type(
        lua_State *LS,
        const char *type_name,
        const LUA_PROP_TYPE *get_table,
        const LUA_PROP_TYPE *set_table,
        const LUA_PROP_TYPE *method_table)
{
    static int udtype=10;
    udtype=udtype+1;

    /*tbc check for table structure correctness */
    /*check_table(get_table)
      check-table(set_table)
      check_table(method_table)
      */

    OBJ_TYPE *tp=alloc_mem(sizeof(OBJ_TYPE));
    tp->udtype=udtype;
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

HELPTOPIC godlib_bless_help =
{
};

static int L_god_curse (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_curse( NULL, ch, "" ));
    return 1;
}

static int L_god_heal (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_heal( NULL, ch, "" ));
    return 1;
}

static int L_god_speed (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_speed( NULL, ch, "" ));
    return 1; 
}

static int L_god_slow (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_slow( NULL, ch, "" ));
    return 1; 
}

static int L_god_cleanse (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_cleanse( NULL, ch, "" ));
    return 1; 
}

static int L_god_defy (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_defy( NULL, ch, "" ));
    return 1; 
}

static int L_god_enlighten (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_enlighten( NULL, ch, "" ));
    return 1; 
}

static int L_god_protect (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_protect( NULL, ch, "" ));
    return 1;
}

static int L_god_fortune (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_fortune( NULL, ch, "" ));
    return 1;
}

static int L_god_haunt (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_haunt( NULL, ch, "" ));
    return 1;
}

static int L_god_plague (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_plague( NULL, ch, "" ));
    return 1;
}

static int L_god_confuse (lua_State *LS)
{
    CHECK_SECURITY(LS, MAX_LUA_SECURITY);

    CHAR_DATA *ch=CHECKCH(LS,1);

    lua_pushboolean( LS,
            god_confuse( NULL, ch, "" ));
    return 1;
}

static int glob_sendtochar (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    char *msg=check_fstring(LS, 2);

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

typedef struct glob_type
{
    char *lib;
    char *name;
    int (*func)();
    int security;
    HELPTOPIC *help;
} GLOB_TYPE;

#define ENDGTABLE { NULL, NULL, NULL, 0, NULL }
#define GFUN( fun, sec ) { NULL, #fun , glob_ ## fun , sec, & glob_ ## fun ## _help }
#define LFUN( lib, fun, sec) { #lib, #fun, lib ## lib_ ## fun , sec, & lib ## lib_ ## fun ## _help}
GLOB_TYPE glob_table[] =
{
    GFUN(sendtochar,    0), 
    LFUN(god,   bless,  9),
    ENDGTABLE
};

static int global_sec_check (lua_State *LS)
{
    int security=luaL_checkinteger( LS, lua_upvalueindex(1) );
    bugf( "%d %d", ScriptSecurity(), security );
    
    if ( ScriptSecurity() < security )
        luaL_error( LS, "Current security %d. Function requires %d.",
                ScriptSecurity(),
                security);

    int (*fun)()=lua_tocfunction( LS, lua_upvalueindex(2) );

    return fun(LS);
}

void register_globals( lua_State *LS )
{
    int i;
    int index;

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

        }

        lua_pushinteger( LS, glob_table[i].security );
        lua_pushcfunction( LS, glob_table[i].func );

        lua_pushcclosure( LS, global_sec_check, 2 );

        if ( glob_table[i].lib )
        {
            lua_setfield( LS, -2, glob_table[i].name );
        }
        else
        {
            lua_setglobal( LS, glob_table[i].name );
        }
    }
}



/* end global section */

/* common section */
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
       tag=str_dup(luaL_checkstring( LS, 4 ));
    }

    lua_getglobal( LS, "delaytbl");
    TIMER_NODE *tmr=register_lua_timer( val, tag );
    lua_pushlightuserdata( LS, (void *)tmr);
    lua_newtable( LS );

    lua_pushliteral( LS, "tableid");
    lua_getfield( LS, 1, "tableid");
    lua_settable( LS, -3 );


    lua_pushliteral( LS, "func");
    lua_pushvalue( LS, 3 );
    lua_settable( LS, -3 );

    lua_settable( LS, -3 );

    return 0;
}

int L_cancel (lua_State *LS)
{
    /* http://pgl.yoyo.org/luai/i/next specifies it is safe
       to modify or clear fields during iteration */
    /* for k,v in pairs(delaytbl) do
            if v.tableid==arg1.tableid then
                unregister_lua_timer(k)
                delaytbl[k]=nil
            end
       end
       */

    /* 1, game object */
    const char *tag=NULL;
    if (!lua_isnone(LS, 2))
    {
        tag=luaL_checkstring( LS, 2 );
        lua_remove( LS, 2 );
    }

    lua_getfield( LS, 1, "tableid"); /* 2, arg1.tableid (game object pointer) */
    lua_getglobal( LS, "delaytbl"); /* 3, delaytbl */

    lua_pushnil( LS );
    while ( lua_next(LS, 3) != 0 ) /* pops nil */
    {
        /* key at 4, val at 5 */
        lua_getfield( LS, 5, "tableid");
        if (lua_equal( LS, 6, 2 )==1)
        {
            luaL_checktype( LS, 4, LUA_TLIGHTUSERDATA);
            TIMER_NODE *tmr=(TIMER_NODE *)lua_touserdata( LS, 4);
            if (unregister_lua_timer( tmr, tag ) ) /* return false if tag no match*/
            {
                /* set table entry to nil */
                lua_pushvalue( LS, 4 ); /* push key */
                lua_pushnil( LS );
                lua_settable( LS, 3 );
            }
        }
        lua_pop(LS, 2); /* pop tableid and value */
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

    if (!run_olc_editor_lua( ud_ch, check_fstring( LS, 2)) )
        luaL_error(LS, "Not currently in olc edit mode.");

    return 0;
}

static int CH_tprint ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);

    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    do_say( ud_ch, luaL_checkstring (LS, -1));

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
    lua_mob_program( NULL, LOADSCRIPT_VNUM, luaL_checkstring(LS, -1), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 );

    return 0;
}

static int CH_loadstring (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_mob_program( NULL, LOADSCRIPT_VNUM, luaL_checkstring(LS, 2), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 );
    return 0;
} 

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

static int CH_emote (lua_State *LS)
{
    do_emote( check_CH(LS, 1), check_fstring (LS, 2) );
    return 0;
}

static int CH_asound (lua_State *LS)
{
    do_mpasound( check_CH(LS, 1), check_fstring (LS, 2));
    return 0; 
}

static int CH_gecho (lua_State *LS)
{
    do_mpgecho( check_CH(LS, 1), check_fstring(LS, 2));
    return 0;
}

static int CH_zecho (lua_State *LS)
{

    do_mpzecho( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_kill (lua_State *LS)
{
    if ( lua_isstring(LS, 2) )
        do_mpkill( check_CH(LS, 1), check_fstring(LS, 2));
    else
        mpkill( check_CH(LS, 1),
                check_CH(LS, 2) );

    return 0;
}

static int CH_assist (lua_State *LS)
{
    if ( lua_isstring(LS, 2) )
        do_mpassist( check_CH(LS, 1), check_fstring(LS, 2));
    else
        mpassist( check_CH(LS, 1), 
                check_CH(LS, 2) );
    return 0;
}

static int CH_junk (lua_State *LS)
{

    do_mpjunk( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_echo (lua_State *LS)
{

    do_mpecho( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_echoaround (lua_State *LS)
{
    if ( !is_CH(LS, 2) )
    {
        /* standard 'mob echoaround' syntax */
        do_mpechoaround( check_CH(LS, 1), check_fstring(LS, 2));
        return 0;
    }

    mpechoaround( check_CH(LS, 1), check_CH(LS, 2), check_fstring(LS, 3) );

    return 0;
}

static int CH_echoat (lua_State *LS)
{
    if ( lua_isnone(LS, 3) )
    {
        /* standard 'mob echoat' syntax */
        do_mpechoat( check_CH(LS, 1), luaL_checkstring(LS, 2));
        return 0;
    }

    mpechoat( check_CH(LS, 1), check_CH(LS, 2), check_fstring(LS, 3) );
    return 0;
}

static int CH_mload (lua_State *LS)
{

    CHAR_DATA *mob=mpmload( check_CH(LS, 1), check_fstring(LS, 2));
    if ( mob && make_CH(LS,mob) )
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
        do_mppurge( check_CH(LS, 1), check_fstring(LS, 2));
    }

    return 0;
}

static int CH_goto (lua_State *LS)
{

    do_mpgoto( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_at (lua_State *LS)
{

    do_mpat( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_transfer (lua_State *LS)
{

    do_mptransfer( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_gtransfer (lua_State *LS)
{

    do_mpgtransfer( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_otransfer (lua_State *LS)
{

    do_mpotransfer( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_force (lua_State *LS)
{

    do_mpforce( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}


static int CH_gforce (lua_State *LS)
{

    do_mpgforce( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_vforce (lua_State *LS)
{

    do_mpvforce( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_cast (lua_State *LS)
{

    do_mpcast( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_damage (lua_State *LS)
{
    do_mpdamage( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_remove (lua_State *LS)
{

    do_mpremove( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_remort (lua_State *LS)
{
    if ( !is_CH(LS, 2) )
    {
        /* standard 'mob remort' syntax */
        do_mpremort( check_CH(LS, 1), check_fstring(LS, 2));
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
        do_mpqset( check_CH(LS, 1), check_fstring(LS, 2));
        return 0;
    }


    mpqset( check_CH(LS, 1), check_CH(LS, 2),
            luaL_checkstring(LS, 3), luaL_checkstring(LS, 4),
            lua_isnone( LS, 5 ) ? 0 : (int)luaL_checknumber( LS, 5),
            lua_isnone( LS, 6 ) ? 0 : (int)luaL_checknumber( LS, 6) );

    return 0;
}

static int CH_qadvance (lua_State *LS)
{
    if ( !is_CH( LS, 2) )
    {
        /* standard 'mob qset' syntax */
        do_mpqadvance( check_CH(LS, 1), check_fstring(LS, 2));
        return 0;
    }

    mpqadvance( check_CH(LS, 1), check_CH(LS, 2),
            luaL_checkstring(LS, 3),
            lua_isnone( LS, 4 ) ? "" : luaL_checkstring(LS, 4) ); 


    return 0;
}

static int CH_reward (lua_State *LS)
{
    if ( !is_CH( LS, 2 ) )
    {
        /* standard 'mob reward' syntax */
        do_mpreward( check_CH(LS, 1), check_fstring(LS, 2));
        return 0;
    }

    mpreward( check_CH(LS, 1), check_CH(LS, 2),
            luaL_checkstring(LS, 3),
            (int)luaL_checknumber(LS, 4) );
    return 0;
}

static int CH_peace (lua_State *LS)
{
    if ( lua_isnone( LS, 2) )
        do_mppeace( check_CH(LS, 1), "");
    else
        do_mppeace( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_restore (lua_State *LS)
{
    do_mprestore( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_setact (lua_State *LS)
{
    do_mpact( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;
}

static int CH_hit (lua_State *LS)
{
    do_mphit( check_CH(LS, 1), check_fstring(LS, 2));

    return 0;

}

static int CH_mdo (lua_State *LS)
{
    interpret( check_CH(LS, 1), check_fstring (LS, 2));

    return 0;
}

static int CH_mobhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, (bool) get_mob_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) ); 
    else
        lua_pushboolean( LS,  (bool) (get_char_room( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_objhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS,(bool) get_obj_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) );
    else
        lua_pushboolean( LS,(bool) (get_obj_here( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_mobexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring (LS, 2);

    lua_pushboolean( LS,(bool) (get_mp_char( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_objexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring (LS, 2);

    lua_pushboolean( LS, (bool) (get_mp_obj( ud_ch, argument) != NULL) );

    return 1;
}


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

static int CH_isnpc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_NPC( ud_ch ) );
    return 1;
}

static int CH_isgood (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_GOOD( ud_ch ) ) ;
    return 1;
}

static int CH_isevil (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_EVIL( ud_ch ) ) ;
    return 1;
}

static int CH_isneutral (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_NEUTRAL( ud_ch ) ) ;
    return 1;
}

static int CH_isimmort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_IMMORTAL( ud_ch ) ) ;
    return 1;
}

static int CH_ischarm (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_AFFECTED( ud_ch, AFF_CHARM ) ) ;
    return 1;
}

static int CH_isfollow (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->master != NULL ) ;
    return 1;
}

static int CH_isactive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->position > POS_SLEEPING ) ;
    return 1;
}

static int CH_isvisible (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH(LS, 1);
    CHAR_DATA * ud_vic = check_CH (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && ud_vic != NULL && can_see( ud_ch, ud_vic ) ) ;

    return 1;
}

static int CH_affected (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = luaL_checkstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL
            &&  is_affected_parse(ud_ch, argument) );

    return 1;
}

static int CH_act (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);
    int flag=NO_FLAG;

    if (IS_NPC(ud_ch))
    {
        if ((flag=flag_lookup(argument, act_flags)) == NO_FLAG) 
            luaL_error(LS, "act: flag '%s' not found in act_flags (mob)", argument);
    }
    else
    {
        if ((flag=flag_lookup(argument, plr_flags)) == NO_FLAG)
            luaL_error(LS, "act: flag '%s' not found in plr_flags (player)", argument);
    }

    lua_pushboolean( LS, ud_ch != NULL
            &&  IS_SET(ud_ch->act, flag) );

    return 1;
}

static int CH_offensive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);
    int flag=flag_lookup(argument, off_flags);

    if ( flag == NO_FLAG )
        luaL_error(LS, "offesive: flag '%s' not found in off_flags", argument);

    lua_pushboolean( LS,
            IS_SET(ud_ch->off_flags, flag) );

    return 1;
}

static int CH_immune (lua_State *LS)
{ 
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);
    int flag=flag_lookup(argument, imm_flags);

    if ( flag == NO_FLAG ) 
        luaL_error(LS, "immune: flag '%s' not found in imm_flags", argument);

    lua_pushboolean( LS, ud_ch != NULL
            &&  IS_SET(ud_ch->imm_flags, flag) );

    return 1;
}

static int CH_carries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, FALSE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_carry( ud_ch, argument, ud_ch ) != NULL) );

    return 1;
}

static int CH_wears (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, TRUE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_wear( ud_ch, argument ) != NULL) );

    return 1;
}

static int CH_has (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), FALSE ) );

    return 1;
}

static int CH_uses (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), TRUE ) );

    return 1;
}

static int CH_say (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    do_say( ud_ch, check_fstring(LS, 2) );
    return 0;
}


static int CH_setlevel (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Cannot set level on PC.");

    int num = (int)luaL_checknumber (LS, 2);
    set_mob_level( ud_ch, num );
    return 0;
}

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
    const char *argument = check_fstring (LS, 2);
    int flag=flag_lookup(argument, vuln_flags);

    if ( flag == NO_FLAG )
        luaL_error(LS, "L_vuln: flag '%s' not found in vuln_flags", argument);

    lua_pushboolean( LS, ud_ch != NULL
            && IS_SET(ud_ch->vuln_flags, flag ) );

    return 1;
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
    const char *argument = check_fstring (LS, 2);
    int flag=flag_lookup(argument, res_flags);

    if ( flag == NO_FLAG )
        luaL_error(LS, "resist: flag '%s' not found in res_flags", argument);

    lua_pushboolean( LS, ud_ch != NULL
            && IS_SET(ud_ch->res_flags, flag) );

    return 1;
}

static int CH_skilled (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);

    lua_pushboolean( LS,  ud_ch != NULL && skill_lookup(argument) != -1
            && get_skill(ud_ch, skill_lookup(argument)) > 0 );

    return 1;
}

static int CH_ccarries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring (LS, 2);

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

static int CH_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_CH(LS,1))->name );
    return 1;
}

HELPTOPIC CH_get_name_help = {
};

static const LUA_PROP_TYPE CH_get_table [] =
{
    CHGET(name, 0),
    CHGET(hp, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE CH_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE CH_method_table [] =
{
    CHMETH(ispc, 0),
/*    CHMETH(isnpc, 0),
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
    CHMETH(setlevel, 0),
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
    CHMETH(savetbl, 0),
    CHMETH(loadtbl, 0),
    CHMETH(tprint, 0),
    CHMETH(olc, 0),
    CHMETH(delay, 0),
    CHMETH(cancel, 0), */
    ENDPTABLE
}; 
OBJ_TYPE *CH_init(lua_State *LS)
{
    if (!CH_type)
        CH_type=new_obj_type(
            LS,
            "CH",
            CH_get_table,
            CH_set_table,
            CH_method_table);

    type_init(LS); /* cascade init since methods depend on other types */
    return CH_type;
}


/* end CH section */

/* OBJ section */
static int OBJ_echo( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    char *argument= check_fstring (LS, 2);

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

static const LUA_PROP_TYPE OBJ_get_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE OBJ_set_table [] =
{
    ENDPTABLE
};


static const LUA_PROP_TYPE OBJ_method_table [] =
{
    ENDPTABLE
}; 

OBJ_TYPE *OBJ_init(lua_State *LS)
{
    if (!OBJ_type)
        OBJ_type=new_obj_type(
            LS,
            "OBJ",
            OBJ_get_table,
            OBJ_set_table,
            OBJ_method_table);
    
    type_init(LS); /* cascade init since methods depend on other types */
    return OBJ_type;
}
/* end OBJ section */

/* AREA section */
static const LUA_PROP_TYPE AREA_get_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE AREA_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE AREA_method_table [] =
{
    ENDPTABLE
}; 

OBJ_TYPE *AREA_init(lua_State *LS)
{
    if (!AREA_type)
        AREA_type=new_obj_type(
            LS,
            "AREA",
            AREA_get_table,
            AREA_set_table,
            AREA_method_table);

    type_init(LS); /* cascade init since methods depend on other types */
    return AREA_type;
}

/* end AREA section */

/* ROOM section */
static const LUA_PROP_TYPE ROOM_get_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE ROOM_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE ROOM_method_table [] =
{
    ENDPTABLE
}; 

OBJ_TYPE *ROOM_init(lua_State *LS)
{
    if (!ROOM_type)
        ROOM_type=new_obj_type(
            LS,
            "ROOM",
            ROOM_get_table,
            ROOM_set_table,
            ROOM_method_table);

    type_init(LS); /* cascade init since methods depend on other types */
    return ROOM_type;
}
/* end ROOM section */

/* EXIT section */
static int EXIT_flag (lua_State *LS)
{
    EXIT_DATA *ed=EXIT_type->check(EXIT_type, LS, 1 );
    const char *argument = check_fstring (LS, 2);

    sh_int flag=flag_lookup( argument, exit_flags);
    if ( flag==NO_FLAG )
        luaL_error(LS, "Invalid exit flag: '%s'", argument);

    lua_pushboolean( LS, IS_SET( ed->exit_info, flag));
    return 1;
}

static const LUA_PROP_TYPE EXIT_get_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE EXIT_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE EXIT_method_table [] =
{
    ENDPTABLE
}; 

OBJ_TYPE *EXIT_init(lua_State *LS)
{
    if (!EXIT_type)
        EXIT_type=new_obj_type(
            LS,
            "EXIT",
            EXIT_get_table,
            EXIT_set_table,
            EXIT_method_table);

    type_init(LS); /* cascade init since methods depend on other types */
    return EXIT_type;
}
/* end EXIT section */

/* RESET section */
static int RESET_get_command(lua_State *LS, RESET_DATA *rd )
{
    static char buf[2];
    sprintf(buf, "%c", rd->command);
    lua_pushstring(LS, buf);
    return 1;
}

static const LUA_PROP_TYPE RESET_get_table [] =
{
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

OBJ_TYPE *RESET_init(lua_State *LS)
{
    if (!RESET_type)
        RESET_type=new_obj_type(
            LS,
            "RESET",
            RESET_get_table,
            RESET_set_table,
            RESET_method_table);

    type_init(LS); /* cascade init since methods depend on other types */
    return RESET_type;
}

/* end RESET section */

/* OBJPROTO section */
static const LUA_PROP_TYPE OBJPROTO_get_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE OBJPROTO_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE OBJPROTO_method_table [] =
{
    ENDPTABLE
}; 

OBJ_TYPE *OBJPROTO_init(lua_State *LS)
{
    if (!OBJPROTO_type)
        OBJPROTO_type=new_obj_type(
            LS,
            "OBJPROTO",
            OBJPROTO_get_table,
            OBJPROTO_set_table,
            OBJPROTO_method_table);

    type_init(LS); /* cascade init since methods depend on other types */
    return OBJPROTO_type;
}

/* end OBJPROTO section */

/* MOBPROTO section */
static const LUA_PROP_TYPE MOBPROTO_get_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE MOBPROTO_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE MOBPROTO_method_table [] =
{
    ENDPTABLE
}; 

OBJ_TYPE *MOBPROTO_init(lua_State *LS)
{
    if (!MOBPROTO_type)
        MOBPROTO_type=new_obj_type(
            LS,
            "MOBPROTO",
            MOBPROTO_get_table,
            MOBPROTO_set_table,
            MOBPROTO_method_table);
    
    type_init(LS); /* cascade init since methods depend on other types */
    return MOBPROTO_type;
}

/* end MOBPROTO section */


/* help section */


static void print_help_usage( CHAR_DATA *ch )
{
    OBJ_TYPE *ot;
    int i;

    ptc( ch, "SECTIONS: \n\r\n\r" );

    ptc( ch, "global\n\r\n\r" );

    for ( i=0 ; type_list[i] ; i++ )
    {
        ot=*(OBJ_TYPE **)type_list[i];
        ptc( ch, "%s\n\r", ot->type_name);
    }
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

static void help_three_arg( CHAR_DATA *ch, const char *arg1, const char *arg2, const char *arg3)
{
}


static void help_two_arg( CHAR_DATA *ch, const char *arg1, const char *arg2 )
{
    OBJ_TYPE *ot;
    int i;

    if ( !strcmp("global", arg1) )
    {
        for ( i=0 ; glob_table[i].name ; i++ )
        {
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

            for ( j=0 ; ot->get_table[j].field ; j++ )
            {
                if (strcmp( ot->get_table[j].field, arg2 ) )
                    continue;

                found=TRUE;

                print_topic( ch, ot->get_table[j].help );

            }

            for ( j=0 ; ot->set_table[j].field ; j++ )
            {
                if (strcmp( ot->set_table[j].field, arg2 ) )
                    continue;

                found=TRUE;

                print_topic( ch, ot->set_table[j].help );

            }

            for ( j=0 ; ot->method_table[j].field ; j++ )
            {
                if (strcmp( ot->method_table[j].field, arg2 ) )
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

    if ( !strcmp("global", arg1) )
    {
        ptc( ch, "\n\rGLOBAL functions\n\r");

        for ( i=0 ; glob_table[i].name ; i++ )
        {
            if (glob_table[i].lib)
            {
                char buf[MSL];
                sprintf(buf, "%s.%s", glob_table[i].lib, glob_table[i].name);
                ptc( ch, "%-20s -", buf);
                if (glob_table[i].help && glob_table[i].help->summary)
                   ptc( ch, glob_table[i].help->summary );
                ptc( ch, "\n\r");
            }
            else
            {
                ptc( ch, "%-20s -", glob_table[i].name);
                if (glob_table[i].help && glob_table[i].help->summary)
                    ptc( ch, glob_table[i].help->summary );
                ptc( ch, "\n\r");
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

            ptc( ch, "\n\rGET fields\n\r");
            for ( j=0 ; ot->get_table[j].field ; j++ )
            {
                ptc( ch, "%-20s - ", ot->get_table[j].field );
                if (ot->get_table[j].help && ot->get_table[j].help->summary)
                    ptc( ch, ot->get_table[j].help->summary );
                ptc( ch, "\n\r");

            }

            ptc( ch, "\n\rSET fields\n\r");
            for ( j=0 ; ot->set_table[j].field ; j++ )
            {
                ptc( ch, "%-20s - ", ot->set_table[j].field );
                if (ot->set_table[j].help && ot->set_table[j].help->summary)
                    ptc( ch, ot->set_table[j].help->summary );
                ptc( ch, "\n\r");

            }

            ptc( ch, "\n\rMETHODS\n\r");
            for ( j=0 ; ot->method_table[j].field ; j++ )
            {
                ptc( ch, "%-20s - ", ot->method_table[j].field );
                if (ot->method_table[j].help && ot->method_table[j].help->summary)
                    ptc( ch, ot->method_table[j].help->summary );
                ptc( ch, "\n\r");

            }
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
    else if (nargs==3)
    {
        help_three_arg(ch, arg1, arg2,arg3);
        return;
    }

}


/* end help section */
