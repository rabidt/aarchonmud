/***************************************************************************
 *  File: mem.c                                                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/



#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

/*
 * Globals
 */
extern          int                     top_reset;
extern          int                     top_area;
extern          int                     top_exit;
extern          int                     top_ed;
extern          int                     top_room;
extern		int			top_mprog_index;
extern      int         top_oprog_index;
extern      int         top_aprog_index;
extern      int         top_rprog_index;

EXTRA_DESCR_DATA	*	extra_descr_free;
SHOP_DATA		*	shop_free;
HELP_DATA		*	help_free;

HELP_DATA		*	help_last;

void	free_extra_descr	args( ( EXTRA_DESCR_DATA *pExtra ) );
void	free_affect		args( ( AFFECT_DATA *af ) );
void	free_mprog              args ( ( PROG_LIST *mp ) );


RESET_DATA *new_reset_data( void )
{
    RESET_DATA *pReset;

    pReset          =   lua_new_ud( &type_RESET_DATA );
    top_reset++;

    pReset->next        =   NULL;
    pReset->command     =   'X';
    pReset->arg1        =   0;
    pReset->arg2        =   0;
    pReset->arg3        =   0;
    pReset->arg4	=   0;

    return pReset;
}



void free_reset_data( RESET_DATA *pReset )
{
    pReset->next            = NULL;
    lua_free_ud( pReset );
    return;
}



AREA_DATA *new_area( void )
{
    AREA_DATA *pArea;
    char buf[MAX_INPUT_LENGTH];
    int i;

    pArea   =   lua_new_ud( &type_AREA_DATA );

    pArea->next             =   NULL;
    pArea->name             =   str_dup( "New area" );
/*    pArea->recall           =   ROOM_VNUM_TEMPLE;      ROM OLC */
    flag_clear( pArea->area_flags );
    /* SET_BIT( pArea->area_flags, AREA_ADDED ); */
    pArea->security         =   1;
    pArea->builders         =   str_dup( "None" );
    pArea->min_vnum         =   0;
    pArea->max_vnum         =   0;
    pArea->age              =   0;
    pArea->save		    = TRUE;
    pArea->nplayer          =   0;
    pArea->empty            =   TRUE;              /* ROM patch */
    sprintf( buf, "area%d.are", pArea->vnum );
    pArea->file_name        =   str_dup( buf );
    pArea->vnum             =   top_area-1;
    pArea->atrig_timer      =   NULL;

    for ( i = 0; i < MAX_AREA_CLONE; i++ )
	pArea->clones[i] = 0;

    return pArea;
}



void free_area( AREA_DATA *pArea )
{
    free_string( pArea->name );
    free_string( pArea->file_name );
    free_string( pArea->builders );
    free_string( pArea->credits );

    pArea->next         =   NULL;

    lua_free_ud( pArea );
    return;
}



EXIT_DATA *new_exit( void )
{
    EXIT_DATA *pExit;

    pExit           =   lua_new_ud( &type_EXIT_DATA );
    top_exit++;

    pExit->u1.to_room   =   NULL;                  /* ROM OLC */
    pExit->next         =   NULL;
/*  pExit->vnum         =   0;                        ROM OLC */
    flag_clear( pExit->exit_info );
    pExit->key          =   0;
    pExit->keyword      =   &str_empty[0];
    pExit->description  =   &str_empty[0];
    flag_clear( pExit->rs_flags );

    return pExit;
}



void free_exit( EXIT_DATA *pExit )
{
    free_string( pExit->keyword );
    free_string( pExit->description );

    pExit->next         =   NULL;
    lua_free_ud( pExit );
    return;
}


ROOM_INDEX_DATA *new_room_index( void )
{
    ROOM_INDEX_DATA *pRoom;
    int door;

    pRoom           =   lua_new_ud( &type_ROOM_INDEX_DATA );
    top_room++;

    pRoom->next             =   NULL;
    pRoom->people           =   NULL;
    pRoom->contents         =   NULL;
    pRoom->extra_descr      =   NULL;
    pRoom->area             =   NULL;

    for ( door=0; door < MAX_DIR; door++ )
        pRoom->exit[door]   =   NULL;

    pRoom->name             =   &str_empty[0];
    pRoom->description      =   &str_empty[0];
    pRoom->owner	    =	&str_empty[0];
    pRoom->vnum             =   0;
    flag_clear( pRoom->room_flags );
    pRoom->light            =   0;
    pRoom->sector_type      =   0;
    pRoom->clan		    =	0;
    pRoom->heal_rate	    =   100;
    pRoom->mana_rate	    =   100;

    return pRoom;
}



void free_room_index( ROOM_INDEX_DATA *pRoom )
{
    int door;
    EXTRA_DESCR_DATA *pExtra;
    RESET_DATA *pReset;

    free_string( pRoom->name );
    free_string( pRoom->description );
    free_string( pRoom->owner );

    for ( door = 0; door < MAX_DIR; door++ )
    {
        if ( pRoom->exit[door] )
            free_exit( pRoom->exit[door] );
    }

    for ( pExtra = pRoom->extra_descr; pExtra; pExtra = pExtra->next )
    {
        free_extra_descr( pExtra );
    }

    for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
    {
        free_reset_data( pReset );
    }
    lua_free_ud( pRoom );

    pRoom->next     =   NULL;

    return;
}

extern AFFECT_DATA *affect_free;


SHOP_DATA *new_shop( void )
{
    SHOP_DATA *pShop;
    int buy;

    if ( !shop_free )
    {
        pShop           =   alloc_perm( sizeof(*pShop) );
        top_shop++;
    }
    else
    {
        pShop           =   shop_free;
        shop_free       =   shop_free->next;
    }

    pShop->next         =   NULL;
    pShop->keeper       =   0;

    for ( buy=0; buy<MAX_TRADE; buy++ )
        pShop->buy_type[buy]    =   0;

    pShop->profit_buy   =   100;
    pShop->profit_sell  =   100;
    pShop->open_hour    =   0;
    pShop->close_hour   =   23;

    return pShop;
}



void free_shop( SHOP_DATA *pShop )
{
    pShop->next = shop_free;
    shop_free   = pShop;
    return;
}

OBJ_INDEX_DATA *new_obj_index( void )
{
    OBJ_INDEX_DATA *pObj;
    int value;

    pObj           =   lua_new_ud( &type_OBJ_INDEX_DATA );
    top_obj_index++;

    pObj->next          =   NULL;
    pObj->extra_descr   =   NULL;
    pObj->affected      =   NULL;
    pObj->area          =   NULL;
    pObj->name          =   str_dup( "no name" );
    pObj->short_descr   =   str_dup( "(no short description)" );
    pObj->description   =   str_dup( "(no description)" );
    pObj->vnum          =   0;
    pObj->item_type     =   ITEM_TRASH;
    flag_clear( pObj->extra_flags );
    flag_clear( pObj->wear_flags );
    pObj->count         =   0;
    pObj->weight        =   0;
    pObj->cost          =   0;
    pObj->material      =   str_dup( "unknown" );      /* ROM */
    pObj->condition     =   100;                        /* ROM */
    for ( value = 0; value < 5; value++ )               /* 5 - ROM */
        pObj->value[value]  =   0;

    pObj->reset_num	= 0;
    pObj->clan          = 0;
    pObj->rank          = 0;
    pObj->combine_vnum  = 0;
    pObj->diff_rating   = 0;

    return pObj;
}

void free_obj_index( OBJ_INDEX_DATA *pObj )
{
    EXTRA_DESCR_DATA *pExtra;
    AFFECT_DATA *pAf;

    free_string( pObj->name );
    free_string( pObj->short_descr );
    free_string( pObj->description );

    for ( pAf = pObj->affected; pAf; pAf = pAf->next )
    {
        free_affect( pAf );
    }

    for ( pExtra = pObj->extra_descr; pExtra; pExtra = pExtra->next )
    {
        free_extra_descr( pExtra );
    }
    
    pObj->next              = NULL;

    lua_free_ud( pObj );
    return;
}



MOB_INDEX_DATA *new_mob_index( void )
{
    MOB_INDEX_DATA *pMob;

    pMob           = lua_new_ud( &type_MOB_INDEX_DATA ); 
    top_mob_index++;

    pMob->next          =   NULL;
    pMob->spec_fun      =   NULL;
    pMob->pShop         =   NULL;
    pMob->area          =   NULL;
    pMob->player_name   =   str_dup( "no name" );
    pMob->short_descr   =   str_dup( "(no short description)" );
    pMob->long_descr    =   str_dup( "(no long description)" );
    pMob->description   =   &str_empty[0];
    pMob->vnum          =   0;
    pMob->count         =   0;
    pMob->killed        =   0;
    pMob->sex           =   0;
    pMob->level         =   0;
    flag_clear( pMob->act ); SET_BIT( pMob->act, ACT_IS_NPC );
    flag_clear( pMob->affect_field );
    pMob->alignment     =   0;
    pMob->race          =   race_lookup( "human" ); /* - Hugin */
    flag_clear( pMob->form );
    flag_clear( pMob->parts );
    flag_clear( pMob->imm_flags );
    flag_clear( pMob->res_flags );
    flag_clear( pMob->vuln_flags );
    flag_clear( pMob->off_flags );
    pMob->size          =   SIZE_MEDIUM; /* ROM patch -- Hugin */
    pMob->hitpoint_percent      = 100;
    pMob->mana_percent          = 100;
    pMob->move_percent          = 100;
    pMob->hitroll_percent       = 100;
    pMob->damage_percent        = 100;
    pMob->ac_percent            = 100;
    pMob->saves_percent         = 100;
    pMob->start_pos             =   POS_STANDING; /*  -- Hugin */
    pMob->default_pos           =   POS_STANDING; /*  -- Hugin */
    pMob->wealth_percent        = 100;

    return pMob;
}



void free_mob_index( MOB_INDEX_DATA *pMob )
{
    free_string( pMob->player_name );
    free_string( pMob->short_descr );
    free_string( pMob->long_descr );
    free_string( pMob->description );
    free_mprog( pMob->mprogs );

    free_shop( pMob->pShop );

    pMob->next              = NULL;
    lua_free_ud( pMob );
    return;
}

PROG_CODE              *       mpcode_free;

PROG_CODE *new_mpcode(void)
{
     PROG_CODE *NewCode;

     if (!mpcode_free)
     {
         NewCode = alloc_perm(sizeof(*NewCode) );
         top_mprog_index++;
     }
     else
     {
         NewCode     = mpcode_free;
         mpcode_free = mpcode_free->next;
     }
     NewCode->security = 0;
     NewCode->is_lua  = TRUE;
     NewCode->vnum    = 0;
     NewCode->code    = str_dup("");
     NewCode->next    = NULL;

     return NewCode;
}

void free_mpcode(PROG_CODE *pMcode)
{
    free_string(pMcode->code);
    pMcode->next = mpcode_free;
    mpcode_free  = pMcode;
    return;
}

PROG_CODE              *       opcode_free;

PROG_CODE *new_opcode(void)
{
     PROG_CODE *NewCode;

     if (!opcode_free)
     {
         NewCode = alloc_perm(sizeof(*NewCode) );
         top_oprog_index++;
     }
     else
     {
         NewCode     = opcode_free;
         opcode_free = opcode_free->next;
     }

     NewCode->vnum    = 0;
     NewCode->is_lua  = TRUE;
     NewCode->code    = str_dup("");
     NewCode->next    = NULL;
     NewCode->security= 0;

     return NewCode;
}

void free_opcode(PROG_CODE *pOcode)
{
    free_string(pOcode->code);
    pOcode->next = opcode_free;
    opcode_free  = pOcode;
    return;
}

PROG_CODE              *       apcode_free;

PROG_CODE *new_apcode(void)
{
     PROG_CODE *NewCode;

     if (!apcode_free)
     {
         NewCode = alloc_perm(sizeof(*NewCode) );
         top_aprog_index++;
     }
     else
     {
         NewCode     = apcode_free;
         apcode_free = apcode_free->next;
     }

     NewCode->vnum    = 0;
     NewCode->is_lua  = TRUE;
     NewCode->code    = str_dup("");
     NewCode->next    = NULL;
     NewCode->security = 0;

     return NewCode;
}

void free_apcode(PROG_CODE *pAcode)
{
    free_string(pAcode->code);
    pAcode->next = apcode_free;
    apcode_free  = pAcode;
    return;
}

PROG_CODE * rpcode_free;

PROG_CODE *new_rpcode(void)
{
    PROG_CODE *NewCode;

    if (!rpcode_free)
    {
        NewCode = alloc_perm(sizeof(*NewCode) );
        top_rprog_index++;
    }
    else
    {
        NewCode = rpcode_free;
        rpcode_free = rpcode_free->next;
    }

    NewCode->vnum = 0;
    NewCode->is_lua  = TRUE;
    NewCode->code = str_dup("");
    NewCode->next = NULL;
    NewCode->security = 0;

    return NewCode;
}

void free_rpcode(PROG_CODE *pRcode)
{
    free_string(pRcode->code);
    pRcode->next = rpcode_free;
    rpcode_free=pRcode;
    return;
}


/* Help Editor - kermit 1/98 */
/*
HELP_DATA *new_help(void)
{
     HELP_DATA *NewHelp;

     NewHelp = alloc_perm(sizeof(*NewHelp) );

     NewHelp->level   = 0;
     NewHelp->keyword = str_dup("");
     NewHelp->text    = str_dup("");
     NewHelp->next    = NULL;

     return NewHelp;
}
*/

