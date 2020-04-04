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
#include "lua_arclib.h"

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

HELP_DATA		*	help_last;

void	free_extra_descr	args( ( EXTRA_DESCR_DATA *pExtra ) );
void	free_affect		args( ( AFFECT_DATA *af ) );
void	free_mprog              args ( ( PROG_LIST *mp ) );


RESET_DATA *new_reset_data( void )
{
    RESET_DATA *pReset;

    pReset          =   alloc_RESET_DATA();
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
    dealloc_RESET_DATA( pReset );
    top_reset--;
    return;
}



AREA_DATA *new_area( void )
{
    AREA_DATA *pArea;
    char buf[MAX_INPUT_LENGTH];
    int i;

    pArea   =   alloc_AREA_DATA();
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
    snprintf( buf, sizeof(buf), "area%d.are", pArea->vnum );
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
    dealloc_AREA_DATA( pArea );
    return;
}



EXIT_DATA *new_exit( void )
{
    EXIT_DATA *pExit;

    pExit           =   alloc_EXIT_DATA();
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
    dealloc_EXIT_DATA( pExit );
    return;
}


ROOM_INDEX_DATA *new_room_index( void )
{
    ROOM_INDEX_DATA *pRoom;
    int door;

    pRoom           =   alloc_ROOM_INDEX_DATA();
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
    dealloc_ROOM_INDEX_DATA( pRoom );
    return;
}

SHOP_DATA *new_shop( void )
{
    SHOP_DATA *pShop;
    int buy;

    pShop           =   alloc_SHOP_DATA();
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
    dealloc_SHOP_DATA( pShop );
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
    dealloc_BOSSACHV( pBoss );
} 


OBJ_INDEX_DATA *new_obj_index( void )
{
    OBJ_INDEX_DATA *pObj;
    int value;

    pObj           =   alloc_OBJ_INDEX_DATA();
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
    dealloc_OBJ_INDEX_DATA( pObj );
    return;
}

MOB_INDEX_DATA *new_mob_index( void )
{
    MOB_INDEX_DATA *pMob;

    pMob           =   alloc_MOB_INDEX_DATA();
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
    dealloc_MOB_INDEX_DATA( pMob );
    return;
}

PROG_CODE *new_mpcode(void)
{
     PROG_CODE *NewCode;

     NewCode = alloc_PROG_CODE();
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
    dealloc_PROG_CODE( pMcode );
    return;
}

PROG_CODE *new_opcode(void)
{
     PROG_CODE *NewCode;

     NewCode = alloc_PROG_CODE();
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
    dealloc_PROG_CODE( pOcode );
    return;
}

PROG_CODE *new_apcode(void)
{
     PROG_CODE *NewCode;

     NewCode = alloc_PROG_CODE();
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
    dealloc_PROG_CODE( pAcode );
    return;
}

PROG_CODE *new_rpcode(void)
{
    PROG_CODE *NewCode;

    NewCode = alloc_PROG_CODE();
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
    dealloc_PROG_CODE( pRcode );
    return;
}

/* wrap structs */
struct CHAR_DATA_wrap
{
    CHAR_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct OBJ_DATA_wrap
{
    OBJ_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct AREA_DATA_wrap
{
    AREA_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct ROOM_INDEX_DATA_wrap
{
    ROOM_INDEX_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct EXIT_DATA_wrap
{
    EXIT_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct RESET_DATA_wrap
{
    RESET_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct MOB_INDEX_DATA_wrap
{
    MOB_INDEX_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct OBJ_INDEX_DATA_wrap
{
    OBJ_INDEX_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct PROG_CODE_wrap
{
    PROG_CODE wrapped;
    struct lua_arclib_obj lao;
};
struct PROG_LIST_wrap
{
    PROG_LIST wrapped;
    struct lua_arclib_obj lao;
};
struct SHOP_DATA_wrap
{
    SHOP_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct AFFECT_DATA_wrap
{
    AFFECT_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct HELP_DATA_wrap
{
    HELP_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct DESCRIPTOR_DATA_wrap
{
    DESCRIPTOR_DATA wrapped;
    struct lua_arclib_obj lao;
};
struct BOSSACHV_wrap
{
    BOSSACHV wrapped;
    struct lua_arclib_obj lao;
};
struct BOSSREC_wrap
{
    BOSSREC wrapped;
    struct lua_arclib_obj lao;
};

/* lao offset definitions */
const size_t CHAR_DATA_lao_offset = offsetof(struct CHAR_DATA_wrap, lao);
const size_t OBJ_DATA_lao_offset = offsetof(struct OBJ_DATA_wrap, lao);
const size_t AREA_DATA_lao_offset = offsetof(struct AREA_DATA_wrap, lao);
const size_t ROOM_INDEX_DATA_lao_offset = offsetof(struct ROOM_INDEX_DATA_wrap, lao);
const size_t EXIT_DATA_lao_offset = offsetof(struct EXIT_DATA_wrap, lao);
const size_t RESET_DATA_lao_offset = offsetof(struct RESET_DATA_wrap, lao);
const size_t MOB_INDEX_DATA_lao_offset = offsetof(struct MOB_INDEX_DATA_wrap, lao);
const size_t OBJ_INDEX_DATA_lao_offset = offsetof(struct OBJ_INDEX_DATA_wrap, lao);
const size_t PROG_CODE_lao_offset = offsetof(struct PROG_CODE_wrap, lao);
const size_t PROG_LIST_lao_offset = offsetof(struct PROG_LIST_wrap, lao);
const size_t SHOP_DATA_lao_offset = offsetof(struct SHOP_DATA_wrap, lao);
const size_t AFFECT_DATA_lao_offset = offsetof(struct AFFECT_DATA_wrap, lao);
const size_t HELP_DATA_lao_offset = offsetof(struct HELP_DATA_wrap, lao);
const size_t DESCRIPTOR_DATA_lao_offset = offsetof(struct DESCRIPTOR_DATA_wrap, lao);
const size_t BOSSACHV_lao_offset = offsetof(struct BOSSACHV_wrap, lao);
const size_t BOSSREC_lao_offset = offsetof(struct BOSSREC_wrap, lao);


/* alloc and dealloc definitions */
CHAR_DATA *alloc_CHAR_DATA(void)
{
    struct CHAR_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_CH(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_CHAR_DATA(CHAR_DATA *p)
{
    struct CHAR_DATA_wrap *wr = (struct CHAR_DATA_wrap *)p;
    lua_deinit_CH(&wr->wrapped);
    free(wr);
}

OBJ_DATA *alloc_OBJ_DATA(void)
{
    struct OBJ_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_OBJ(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_OBJ_DATA(OBJ_DATA *p)
{
    struct OBJ_DATA_wrap *wr = (struct OBJ_DATA_wrap *)p;
    lua_deinit_OBJ(&wr->wrapped);
    free(wr);
}

AREA_DATA *alloc_AREA_DATA(void)
{
    struct AREA_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_AREA(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_AREA_DATA(AREA_DATA *p)
{
    struct AREA_DATA_wrap *wr = (struct AREA_DATA_wrap *)p;
    lua_deinit_AREA(&wr->wrapped);
    free(wr);
}

ROOM_INDEX_DATA *alloc_ROOM_INDEX_DATA(void)
{
    struct ROOM_INDEX_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_ROOM(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_ROOM_INDEX_DATA(ROOM_INDEX_DATA *p)
{
    struct ROOM_INDEX_DATA_wrap *wr = (struct ROOM_INDEX_DATA_wrap *)p;
    lua_deinit_ROOM(&wr->wrapped);
    free(wr);
}

EXIT_DATA *alloc_EXIT_DATA(void)
{
    struct EXIT_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_EXIT(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_EXIT_DATA(EXIT_DATA *p)
{
    struct EXIT_DATA_wrap *wr = (struct EXIT_DATA_wrap *)p;
    lua_deinit_EXIT(&wr->wrapped);
    free(wr);
}

RESET_DATA *alloc_RESET_DATA(void)
{
    struct RESET_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_RESET(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_RESET_DATA(RESET_DATA *p)
{
    struct RESET_DATA_wrap *wr = (struct RESET_DATA_wrap *)p;
    lua_deinit_RESET(&wr->wrapped);
    free(wr);
}

MOB_INDEX_DATA *alloc_MOB_INDEX_DATA(void)
{
    struct MOB_INDEX_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_MOBPROTO(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_MOB_INDEX_DATA(MOB_INDEX_DATA *p)
{
    struct MOB_INDEX_DATA_wrap *wr = (struct MOB_INDEX_DATA_wrap *)p;
    lua_deinit_MOBPROTO(&wr->wrapped);
    free(wr);
}

OBJ_INDEX_DATA *alloc_OBJ_INDEX_DATA(void)
{
    struct OBJ_INDEX_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_OBJPROTO(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_OBJ_INDEX_DATA(OBJ_INDEX_DATA *p)
{
    struct OBJ_INDEX_DATA_wrap *wr = (struct OBJ_INDEX_DATA_wrap *)p;
    lua_deinit_OBJPROTO(&wr->wrapped);
    free(wr);
}

PROG_CODE *alloc_PROG_CODE(void)
{
    struct PROG_CODE_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_PROG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_PROG_CODE(PROG_CODE *p)
{
    struct PROG_CODE_wrap *wr = (struct PROG_CODE_wrap *)p;
    lua_deinit_PROG(&wr->wrapped);
    free(wr);
}

PROG_LIST *alloc_MPROG_LIST(void)
{
    struct PROG_LIST_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_MTRIG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_MPROG_LIST(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = (struct PROG_LIST_wrap *)p;
    lua_deinit_MTRIG(&wr->wrapped);
    free(wr);
}

PROG_LIST *alloc_OPROG_LIST(void)
{
    struct PROG_LIST_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_OTRIG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_OPROG_LIST(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = (struct PROG_LIST_wrap *)p;
    lua_deinit_OTRIG(&wr->wrapped);
    free(wr);
}

PROG_LIST *alloc_APROG_LIST(void)
{
    struct PROG_LIST_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_ATRIG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_APROG_LIST(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = (struct PROG_LIST_wrap *)p;
    lua_deinit_ATRIG(&wr->wrapped);
    free(wr);
}

PROG_LIST *alloc_RPROG_LIST(void)
{
    struct PROG_LIST_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_RTRIG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_RPROG_LIST(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = (struct PROG_LIST_wrap *)p;
    lua_deinit_RTRIG(&wr->wrapped);
    free(wr);
}

SHOP_DATA *alloc_SHOP_DATA(void)
{
    struct SHOP_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_SHOP(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_SHOP_DATA(SHOP_DATA *p)
{
    struct SHOP_DATA_wrap *wr = (struct SHOP_DATA_wrap *)p;
    lua_deinit_SHOP(&wr->wrapped);
    free(wr);
}

AFFECT_DATA *alloc_AFFECT_DATA(void)
{
    struct AFFECT_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_AFFECT(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_AFFECT_DATA(AFFECT_DATA *p)
{
    struct AFFECT_DATA_wrap *wr = (struct AFFECT_DATA_wrap *)p;
    lua_deinit_AFFECT(&wr->wrapped);
    free(wr);
}

HELP_DATA *alloc_HELP_DATA(void)
{
    struct HELP_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_HELP(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_HELP_DATA(HELP_DATA *p)
{
    struct HELP_DATA_wrap *wr = (struct HELP_DATA_wrap *)p;
    lua_deinit_HELP(&wr->wrapped);
    free(wr);
}

DESCRIPTOR_DATA *alloc_DESCRIPTOR_DATA(void)
{
    struct DESCRIPTOR_DATA_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_DESCRIPTOR(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_DESCRIPTOR_DATA(DESCRIPTOR_DATA *p)
{
    struct DESCRIPTOR_DATA_wrap *wr = (struct DESCRIPTOR_DATA_wrap *)p;
    lua_deinit_DESCRIPTOR(&wr->wrapped);
    free(wr);
}

BOSSACHV *alloc_BOSSACHV(void)
{
    struct BOSSACHV_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_BOSSACHV(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_BOSSACHV(BOSSACHV *p)
{
    struct BOSSACHV_wrap *wr = (struct BOSSACHV_wrap *)p;
    lua_deinit_BOSSACHV(&wr->wrapped);
    free(wr);
}

BOSSREC *alloc_BOSSREC(void)
{
    struct BOSSREC_wrap *wr = calloc(1, sizeof(*wr));
    lua_init_BOSSREC(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_BOSSREC(BOSSREC *p)
{
    struct BOSSREC_wrap *wr = (struct BOSSREC_wrap *)p;
    lua_deinit_BOSSREC(&wr->wrapped);
    free(wr);
}
