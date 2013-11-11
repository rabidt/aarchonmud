#include <time.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "tables.h"
#include "lua_object_type.h"
#include "lua_common.h"
#include "lua_type_OBJ.h"
#include "olc.h"

OBJ_TYPE *CH_type;
OBJ_TYPE *OBJ_type;
#define GETSCRIPT_FUNCTION "GetScript"
#define SAVETABLE_FUNCTION "SaveTable"
#define LOADTABLE_FUNCTION "LoadTable"
#define TPRINTSTR_FUNCTION "tprintstr"
#define LOADSCRIPT_VNUM 0

#define check_CH( LS, index) CH_type->check( CH_type, LS, index )
#define make_CH(LS, ch ) CH_type->make( CH_type, LS, ch )
#define make_OBJ(LS, obj ) OBJ_type->make( OBJ_type, LS, obj )
#define is_CH(LS, ch ) CH_type->is( CH_type, LS, ch )
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
#ifndef BUILDER
//    CHECK_SECURITY(LS, MAX_LUA_SECURITY);
#endif
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

static const LUA_PROP_TYPE get_table [] =
{
    {"name", PTYPE_STR,  offsetof(CHAR_DATA, name) , NULL},
    {"hp",   PTYPE_INT,  offsetof(CHAR_DATA, hit)  , NULL},
    {NULL, PTYPE_NONE, NO_OFF, NULL}
};

static const LUA_PROP_TYPE set_table [] =
{
    {"hp", PTYPE_INT,    offsetof(CHAR_DATA, hit), NULL},
    {NULL, PTYPE_NONE, NO_OFF, NULL}
};

#define METHOD( meth ) { #meth , PTYPE_FUN, NO_OFF, CH_ ## meth }
static const LUA_PROP_TYPE method_table [] =
{
    METHOD(ispc),
    METHOD(isnpc),
    METHOD(isgood),
    METHOD(isevil),
    METHOD(isneutral),
    METHOD(isimmort),
    METHOD(ischarm),
    METHOD(isfollow),
    METHOD(isactive),
    METHOD(isvisible),
    METHOD(mobhere),
    METHOD(objhere),
    METHOD(mobexists),
    METHOD(objexists),
    METHOD(affected),
    METHOD(act),
    METHOD(offensive),
    METHOD(immune),
    METHOD(carries),
    METHOD(wears),
    METHOD(has),
    METHOD(uses),
    METHOD(qstatus),
    METHOD(resist),
    METHOD(vuln),
    METHOD(skilled),
    METHOD(ccarries),
    METHOD(qtimer),
    METHOD(canattack),
    METHOD(destroy),
    METHOD(oload),
    METHOD(setlevel),
    METHOD(say),
    METHOD(emote),
    METHOD(mdo),
    METHOD(asound),
    METHOD(gecho),
    METHOD(zecho),
    METHOD(kill),
    METHOD(assist),
    METHOD(junk),
    METHOD(echo),
    METHOD(echoaround),
    METHOD(echoat),
    METHOD(mload),
    METHOD(purge),
    METHOD(goto),
    METHOD(at),
    METHOD(transfer),
    METHOD(gtransfer),
    METHOD(otransfer),
    METHOD(force),
    METHOD(gforce),
    METHOD(vforce),
    METHOD(cast),
    METHOD(damage),
    METHOD(remove),
    METHOD(remort),
    METHOD(qset),
    METHOD(qadvance),
    METHOD(reward),
    METHOD(peace),
    METHOD(restore),
    METHOD(setact),
    METHOD(hit),
    METHOD(randchar),
    METHOD(loadprog),
    METHOD(loadscript),
    METHOD(loadstring),
    METHOD(savetbl),
    METHOD(loadtbl),
    METHOD(tprint),
    METHOD(olc),
    METHOD(delay),
    METHOD(cancel),
    {NULL, PTYPE_NONE, NO_OFF, NULL}
}; 

OBJ_TYPE *CH_init(lua_State *LS)
{
    if (!CH_type)
        CH_type=new_obj_type(
            LS,
            "CH",
            get_table,
            set_table,
            method_table);

    OBJ_type=OBJ_init(LS);
    return CH_type;
}

