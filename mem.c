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

HELP_DATA		*	help_last;

void	free_extra_descr	args( ( EXTRA_DESCR_DATA *pExtra ) );
void	free_affect		args( ( AFFECT_DATA *af ) );
void	free_mprog              args ( ( PROG_LIST *mp ) );


RESET_DATA *new_reset_data( void )
{
    RESET_DATA *pReset;

    pReset          =   alloc_RESET();
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
    free_RESET( pReset );
    top_reset--;
    return;
}



AREA_DATA *new_area( void )
{
    AREA_DATA *pArea;
    char buf[MAX_INPUT_LENGTH];
    int i;

    pArea   =   alloc_AREA();
    top_area++;

    pArea->next             =   NULL;
    pArea->name             =   str_dup( "New area" );
/*    pArea->recall           =   ROOM_VNUM_TEMPLE;      ROM OLC */
    flag_clear( pArea->area_flags );
    /* SET_BIT( pArea->area_flags, AREA_ADDED ); */
    pArea->security         =   1;
    pArea->builders         =   str_dup( "None" );
    pArea->credits          =   str_dup( "" );
    pArea->comments         =   str_dup( "" );
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
    free_string( pArea->comments );

    pArea->next         =   NULL;
    free_AREA( pArea );
    return;
}



EXIT_DATA *new_exit( void )
{
    EXIT_DATA *pExit;

    pExit           =   alloc_EXIT();
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
    top_exit--;

    pExit->next         =   NULL;
    free_EXIT( pExit );
    return;
}


ROOM_INDEX_DATA *new_room_index( void )
{
    ROOM_INDEX_DATA *pRoom;
    int door;

    pRoom           =   alloc_ROOM();
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
    pRoom->comments            =   &str_empty[0];
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
    free_string( pRoom->comments );

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

    top_room--;

    pRoom->next     =   NULL;
    free_ROOM( pRoom );
    return;
}

SHOP_DATA *new_shop( void )
{
    SHOP_DATA *pShop;
    int buy;

    pShop           =   alloc_SHOP();
    top_shop++;

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
    pShop->next = NULL;
    free_SHOP( pShop );
    return;
}

BOSSACHV *new_boss_achieve( void )
{
    BOSSACHV *pBoss;

    pBoss = alloc_BOSSACHV();

    pBoss->quest_reward = 0;
    pBoss->exp_reward = 0;
    pBoss->gold_reward = 0;
    pBoss->ach_reward = 0;

    return pBoss;
}

void free_boss_achieve( BOSSACHV *pBoss )
{
    free_BOSSACHV( pBoss );
} 


OBJ_INDEX_DATA *new_obj_index( void )
{
    OBJ_INDEX_DATA *pObj;
    int value;

    pObj           =   alloc_OBJPROTO();
    top_obj_index++;

    pObj->next          =   NULL;
    pObj->extra_descr   =   NULL;
    pObj->affected      =   NULL;
    pObj->area          =   NULL;
    pObj->name          =   str_dup( "no name" );
    pObj->short_descr   =   str_dup( "(no short description)" );
    pObj->description   =   str_dup( "(no description)" );
    pObj->comments         =   str_dup( "" );
    pObj->vnum          =   0;
    pObj->item_type     =   ITEM_TRASH;
    flag_clear( pObj->extra_flags );
    pObj->wear_type     =   ITEM_NO_CARRY;
    pObj->count         =   0;
    pObj->weight        =   0;
    pObj->cost          =   0;
    pObj->material      =   str_dup( "unknown" );      /* ROM */
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
    free_string( pObj->comments );

    for ( pAf = pObj->affected; pAf; pAf = pAf->next )
    {
        free_affect( pAf );
    }

    for ( pExtra = pObj->extra_descr; pExtra; pExtra = pExtra->next )
    {
        free_extra_descr( pExtra );
    }
    
    top_obj_index--;

    pObj->next              = NULL;
    free_OBJPROTO( pObj );
    return;
}

MOB_INDEX_DATA *new_mob_index( void )
{
    MOB_INDEX_DATA *pMob;

    pMob           =   alloc_MOBPROTO();
    top_mob_index++;

    pMob->next          =   NULL;
    pMob->spec_fun      =   NULL;
    pMob->pShop         =   NULL;
    pMob->boss_achieve  =   NULL;
    pMob->area          =   NULL;
    pMob->player_name   =   str_dup( "no name" );
    pMob->short_descr   =   str_dup( "(no short description)" );
    pMob->long_descr    =   str_dup( "(no long description)" );
    pMob->description   =   &str_empty[0];
    pMob->comments         =   str_dup( "" );
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
    free_string( pMob->comments );
    free_mprog( pMob->mprogs );

    if ( pMob->pShop )
        free_shop( pMob->pShop );

    top_mob_index--;

    pMob->next              = NULL;
    free_MOBPROTO( pMob );
    return;
}

PROG_CODE *new_mpcode(void)
{
     PROG_CODE *NewCode;

     NewCode = alloc_PROG();
     top_mprog_index++;

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
    pMcode->next = NULL;
    free_PROG( pMcode );
    return;
}

PROG_CODE *new_opcode(void)
{
     PROG_CODE *NewCode;

     NewCode = alloc_PROG();
     top_oprog_index++;

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
    pOcode->next = NULL;
    free_PROG( pOcode );
    return;
}

PROG_CODE *new_apcode(void)
{
     PROG_CODE *NewCode;

     NewCode = alloc_PROG();
     top_aprog_index++;

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
    pAcode->next = NULL;
    free_PROG( pAcode );
    return;
}

PROG_CODE *new_rpcode(void)
{
    PROG_CODE *NewCode;

    NewCode = alloc_PROG();
    top_rprog_index++;

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
    pRcode->next = NULL;
    free_PROG( pRcode );
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

