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
#include <stdint.h>
#include <assert.h>
#include "merc.h"
#include "lua_arclib.h"
#define MEM_C_
#include "mem.h"
#undef MEM_C_

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

static struct arc_obj_type *all_arc_obj_types[];

static void *aoh_get_wrapped(struct arc_obj *aoh)
{
    assert(aoh);
    assert(aoh->ao_type);
    uintptr_t offset = aoh->ao_type->wrapped_offset - aoh->ao_type->aoh_offset;
    uintptr_t v = (uintptr_t)aoh + offset;
    return (void *)v;
}

static void *aot_get_wrapped(struct arc_obj *aot)
{
    assert(aot);
    assert(aot->ao_type);
    uintptr_t offset = aot->ao_type->aot_offset - aot->ao_type->wrapped_offset;
    uintptr_t v = (uintptr_t)aot - offset;
    return (void *)v;
}

/* Returns true if any error */
bool arc_obj_diag(BUFFER *output)
{
    bool rtn = FALSE;

    struct arc_obj *aoh;
    struct arc_obj *aot;

    unsigned long long ao_counted = 0;

    for ( aoh = all_aoh.ao_next, aot = all_aot.ao_next;
          aoh != &all_aoh && aot != &all_aot;
          aoh = aoh->ao_next, aot = aot->ao_next )
    {
        if ( aoh->magic_id[0] != AO_MAGIC_INIT_0 ||
             aoh->magic_id[1] != AO_MAGIC_INIT_1 )
        {

            add_buf(output, "Found invalid obj.");
            rtn = TRUE;
            /* All bets are off. Should not continue to iterate or dereference. */
            break;
        }
        if (aoh_get_wrapped(aoh) != aot_get_wrapped(aot))
        {
            add_buf(output, "Discrepancy in all_aoh and all_aot lists.");
            rtn = TRUE;
            /* All bets are off. Should not continue to iterate or dereference. */
            break;
        }
        ++ao_counted;
    }

    
    unsigned long long ao_summed = 0;    
    for (unsigned i = 0; all_arc_obj_types[i]; ++i)
    {
        const struct arc_obj_type *t = all_arc_obj_types[i];
        ao_summed += t->ao_count;
    }

    if (ao_counted != ao_summed)
    {
        rtn = TRUE;
        addf_buf(output, "ao_counted(%llu) != ao_summed(%llu).", ao_counted, ao_summed);
    }

    if (FALSE == rtn)
    {
        add_buf(output, "No issues found.");
    }

    return rtn;
}


AO_STATIC struct arc_obj all_aoh =
{
    .ao_next = &all_aoh,
    .ao_prev = &all_aoh,
};

AO_STATIC struct arc_obj all_aot =
{
    .ao_next = &all_aot,
    .ao_prev = &all_aot,
};

static void add_ao_list(struct arc_obj *list, struct arc_obj *ao)
{
    assert(list);
    assert(ao);
    ao->ao_next = list->ao_next;
    ao->ao_prev = list;
    assert(list->ao_next);
    list->ao_next->ao_prev = ao;
    list->ao_next = ao;
}

static void rem_ao_list(struct arc_obj *ao)
{
    assert(ao);
    assert(ao->ao_next);
    ao->ao_next->ao_prev = ao->ao_prev;
    assert(ao->ao_prev);
    ao->ao_prev->ao_next = ao->ao_next;
    ao->ao_next = NULL;
    ao->ao_prev = NULL;
}

AO_STATIC void arc_obj_init(struct arc_obj_type *ao_type, struct arc_obj *aoh, struct arc_obj *aot)
{
    assert(ao_type);
    assert(aoh);
    assert(aot);

    add_ao_list(&all_aoh, aoh);
    add_ao_list(&all_aot, aot);
    aoh->ao_type = ao_type;
    aot->ao_type = ao_type;
    ++ao_type->ao_count;

    // mark as initialized
    aoh->magic_id[0] = AO_MAGIC_INIT_0;
    aoh->magic_id[1] = AO_MAGIC_INIT_1;

    aot->magic_id[0] = AO_MAGIC_INIT_0;
    aot->magic_id[1] = AO_MAGIC_INIT_1;
}

AO_STATIC void arc_obj_deinit(struct arc_obj_type *ao_type, struct arc_obj *aoh, struct arc_obj *aot)
{
    assert(ao_type);
    assert(aoh);
    assert(aot);
    assert(aoh->magic_id[0] == AO_MAGIC_INIT_0);
    assert(aoh->magic_id[1] == AO_MAGIC_INIT_1);
    assert(aot->magic_id[0] == AO_MAGIC_INIT_0);
    assert(aot->magic_id[1] == AO_MAGIC_INIT_1);

    rem_ao_list(aoh);
    rem_ao_list(aot);
    assert(ao_type == aoh->ao_type);
    assert(ao_type == aot->ao_type);
    --ao_type->ao_count;
    aoh->ao_type = NULL;
    aot->ao_type = NULL;

    // mark as uninitialized
    aoh->magic_id[0] = AO_MAGIC_DEINIT_0;
    aoh->magic_id[1] = AO_MAGIC_DEINIT_1;
    aot->magic_id[0] = AO_MAGIC_DEINIT_0;
    aot->magic_id[1] = AO_MAGIC_DEINIT_1;
}

static bool is_valid_arc_obj(struct arc_obj_type *ao_type, void *p)
{
    assert(ao_type);
    assert(p);

    uintptr_t aoh_p_off = ao_type->wrapped_offset - ao_type->aoh_offset;
    uintptr_t p_aot_off = ao_type->aot_offset - ao_type->wrapped_offset;


    const struct arc_obj *aoh = (struct arc_obj *)((uintptr_t)p - aoh_p_off);
    const struct arc_obj *aot = (struct arc_obj *)((uintptr_t)p + p_aot_off);

    return  aoh->magic_id[0] == AO_MAGIC_INIT_0 &&
            aoh->magic_id[1] == AO_MAGIC_INIT_1 &&
            aot->magic_id[0] == AO_MAGIC_INIT_0 &&
            aot->magic_id[1] == AO_MAGIC_INIT_1;
}

/* wrap structs */
struct CHAR_DATA_wrap
{
    struct arc_obj aoh;
    CHAR_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct OBJ_DATA_wrap
{
    struct arc_obj aoh;
    OBJ_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct AREA_DATA_wrap
{
    struct arc_obj aoh;
    AREA_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct ROOM_INDEX_DATA_wrap
{
    struct arc_obj aoh;
    ROOM_INDEX_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct EXIT_DATA_wrap
{
    struct arc_obj aoh;
    EXIT_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct RESET_DATA_wrap
{
    struct arc_obj aoh;
    RESET_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct MOB_INDEX_DATA_wrap
{
    struct arc_obj aoh;
    MOB_INDEX_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct OBJ_INDEX_DATA_wrap
{
    struct arc_obj aoh;
    OBJ_INDEX_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct PROG_CODE_wrap
{
    struct arc_obj aoh;
    PROG_CODE wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct PROG_LIST_wrap
{
    struct arc_obj aoh;
    PROG_LIST wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct SHOP_DATA_wrap
{
    struct arc_obj aoh;
    SHOP_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct AFFECT_DATA_wrap
{
    struct arc_obj aoh;
    AFFECT_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct HELP_DATA_wrap
{
    struct arc_obj aoh;
    HELP_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct DESCRIPTOR_DATA_wrap
{
    struct arc_obj aoh;
    DESCRIPTOR_DATA wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct BOSSACHV_wrap
{
    struct arc_obj aoh;
    BOSSACHV wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};
struct BOSSREC_wrap
{
    struct arc_obj aoh;
    BOSSREC wrapped;
    struct arc_obj aot;
    struct lua_arclib_obj lao;
};

/* arc_obj_type definitions */
static struct arc_obj_type CHAR_DATA_type = 
{
    .ao_count = 0,
    .name = "CHAR_DATA",
    .aoh_offset = offsetof(struct CHAR_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct CHAR_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct CHAR_DATA_wrap, aot),
};
static struct arc_obj_type OBJ_DATA_type = 
{
    .ao_count = 0,
    .name = "OBJ_DATA",
    .aoh_offset = offsetof(struct OBJ_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct OBJ_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct OBJ_DATA_wrap, aot),
};
static struct arc_obj_type AREA_DATA_type = 
{
    .ao_count = 0,
    .name = "AREA_DATA",
    .aoh_offset = offsetof(struct AREA_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct AREA_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct AREA_DATA_wrap, aot),
};
static struct arc_obj_type ROOM_INDEX_DATA_type = 
{
    .ao_count = 0,
    .name = "ROOM_INDEX_DATA",
    .aoh_offset = offsetof(struct ROOM_INDEX_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct ROOM_INDEX_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct ROOM_INDEX_DATA_wrap, aot),
};
static struct arc_obj_type EXIT_DATA_type = 
{
    .ao_count = 0,
    .name = "EXIT_DATA",
    .aoh_offset = offsetof(struct EXIT_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct EXIT_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct EXIT_DATA_wrap, aot),
};
static struct arc_obj_type RESET_DATA_type = 
{
    .ao_count = 0,
    .name = "RESET_DATA",
    .aoh_offset = offsetof(struct RESET_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct RESET_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct RESET_DATA_wrap, aot),
};
static struct arc_obj_type MOB_INDEX_DATA_type = 
{
    .ao_count = 0,
    .name = "MOB_INDEX_DATA",
    .aoh_offset = offsetof(struct MOB_INDEX_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct MOB_INDEX_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct MOB_INDEX_DATA_wrap, aot),
};
static struct arc_obj_type OBJ_INDEX_DATA_type = 
{
    .ao_count = 0,
    .name = "OBJ_INDEX_DATA",
    .aoh_offset = offsetof(struct OBJ_INDEX_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct OBJ_INDEX_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct OBJ_INDEX_DATA_wrap, aot),
};
static struct arc_obj_type PROG_CODE_type = 
{
    .ao_count = 0,
    .name = "PROG_CODE",
    .aoh_offset = offsetof(struct PROG_CODE_wrap, aoh),
    .wrapped_offset = offsetof(struct PROG_CODE_wrap, wrapped),
    .aot_offset = offsetof(struct PROG_CODE_wrap, aot),
};
static struct arc_obj_type PROG_LIST_type = 
{
    .ao_count = 0,
    .name = "PROG_LIST",
    .aoh_offset = offsetof(struct PROG_LIST_wrap, aoh),
    .wrapped_offset = offsetof(struct PROG_LIST_wrap, wrapped),
    .aot_offset = offsetof(struct PROG_LIST_wrap, aot),
};
static struct arc_obj_type SHOP_DATA_type = 
{
    .ao_count = 0,
    .name = "SHOP_DATA",
    .aoh_offset = offsetof(struct SHOP_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct SHOP_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct SHOP_DATA_wrap, aot),
};
static struct arc_obj_type AFFECT_DATA_type = 
{
    .ao_count = 0,
    .name = "AFFECT_DATA",
    .aoh_offset = offsetof(struct AFFECT_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct AFFECT_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct AFFECT_DATA_wrap, aot),
};
static struct arc_obj_type HELP_DATA_type = 
{
    .ao_count = 0,
    .name = "HELP_DATA",
    .aoh_offset = offsetof(struct HELP_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct HELP_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct HELP_DATA_wrap, aot),
};
static struct arc_obj_type DESCRIPTOR_DATA_type = 
{
    .ao_count = 0,
    .name = "DESCRIPTOR_DATA",
    .aoh_offset = offsetof(struct DESCRIPTOR_DATA_wrap, aoh),
    .wrapped_offset = offsetof(struct DESCRIPTOR_DATA_wrap, wrapped),
    .aot_offset = offsetof(struct DESCRIPTOR_DATA_wrap, aot),
};
static struct arc_obj_type BOSSACHV_type = 
{
    .ao_count = 0,
    .name = "BOSSACHV",
    .aoh_offset = offsetof(struct BOSSACHV_wrap, aoh),
    .wrapped_offset = offsetof(struct BOSSACHV_wrap, wrapped),
    .aot_offset = offsetof(struct BOSSACHV_wrap, aot),
};
static struct arc_obj_type BOSSREC_type = 
{
    .ao_count = 0,
    .name = "BOSSREC",
    .aoh_offset = offsetof(struct BOSSREC_wrap, aoh),
    .wrapped_offset = offsetof(struct BOSSREC_wrap, wrapped),
    .aot_offset = offsetof(struct BOSSREC_wrap, aot),
};

static struct arc_obj_type *all_arc_obj_types[] =
{
    &CHAR_DATA_type,
    &OBJ_DATA_type,
    &AREA_DATA_type,
    &ROOM_INDEX_DATA_type,
    &EXIT_DATA_type,
    &RESET_DATA_type,
    &MOB_INDEX_DATA_type,
    &OBJ_INDEX_DATA_type,
    &PROG_CODE_type,
    &PROG_LIST_type,
    &SHOP_DATA_type,
    &AFFECT_DATA_type,
    &HELP_DATA_type,
    &DESCRIPTOR_DATA_type,
    &BOSSACHV_type,
    &BOSSREC_type,
    NULL
};

/* lao offset definitions */
const size_t CHAR_DATA_lao_offset = offsetof(struct CHAR_DATA_wrap, lao) - offsetof(struct CHAR_DATA_wrap, wrapped);
const size_t OBJ_DATA_lao_offset = offsetof(struct OBJ_DATA_wrap, lao) - offsetof(struct OBJ_DATA_wrap, wrapped);
const size_t AREA_DATA_lao_offset = offsetof(struct AREA_DATA_wrap, lao) - offsetof(struct AREA_DATA_wrap, wrapped);
const size_t ROOM_INDEX_DATA_lao_offset = offsetof(struct ROOM_INDEX_DATA_wrap, lao) - offsetof(struct ROOM_INDEX_DATA_wrap, wrapped);
const size_t EXIT_DATA_lao_offset = offsetof(struct EXIT_DATA_wrap, lao) - offsetof(struct EXIT_DATA_wrap, wrapped);
const size_t RESET_DATA_lao_offset = offsetof(struct RESET_DATA_wrap, lao) - offsetof(struct RESET_DATA_wrap, wrapped);
const size_t MOB_INDEX_DATA_lao_offset = offsetof(struct MOB_INDEX_DATA_wrap, lao) - offsetof(struct MOB_INDEX_DATA_wrap, wrapped);
const size_t OBJ_INDEX_DATA_lao_offset = offsetof(struct OBJ_INDEX_DATA_wrap, lao) - offsetof(struct OBJ_INDEX_DATA_wrap, wrapped);
const size_t PROG_CODE_lao_offset = offsetof(struct PROG_CODE_wrap, lao) - offsetof(struct PROG_CODE_wrap, wrapped);
const size_t PROG_LIST_lao_offset = offsetof(struct PROG_LIST_wrap, lao) - offsetof(struct PROG_LIST_wrap, wrapped);
const size_t SHOP_DATA_lao_offset = offsetof(struct SHOP_DATA_wrap, lao) - offsetof(struct SHOP_DATA_wrap, wrapped);
const size_t AFFECT_DATA_lao_offset = offsetof(struct AFFECT_DATA_wrap, lao) - offsetof(struct AFFECT_DATA_wrap, wrapped);
const size_t HELP_DATA_lao_offset = offsetof(struct HELP_DATA_wrap, lao) - offsetof(struct HELP_DATA_wrap, wrapped);
const size_t DESCRIPTOR_DATA_lao_offset = offsetof(struct DESCRIPTOR_DATA_wrap, lao) - offsetof(struct DESCRIPTOR_DATA_wrap, wrapped);
const size_t BOSSACHV_lao_offset = offsetof(struct BOSSACHV_wrap, lao) - offsetof(struct BOSSACHV_wrap, wrapped);
const size_t BOSSREC_lao_offset = offsetof(struct BOSSREC_wrap, lao) - offsetof(struct BOSSREC_wrap, wrapped);

static struct CHAR_DATA_wrap *CHAR_DATA_get_wrap(CHAR_DATA *p)
{
    struct CHAR_DATA_wrap *wr = (struct CHAR_DATA_wrap *)((uintptr_t)p - offsetof(struct CHAR_DATA_wrap, wrapped));
    return wr;
}

static struct OBJ_DATA_wrap *OBJ_DATA_get_wrap(OBJ_DATA *p)
{
    struct OBJ_DATA_wrap *wr = (struct OBJ_DATA_wrap *)((uintptr_t)p - offsetof(struct OBJ_DATA_wrap, wrapped));
    return wr;
}

static struct AREA_DATA_wrap *AREA_DATA_get_wrap(AREA_DATA *p)
{
    struct AREA_DATA_wrap *wr = (struct AREA_DATA_wrap *)((uintptr_t)p - offsetof(struct AREA_DATA_wrap, wrapped));
    return wr;
}

static struct ROOM_INDEX_DATA_wrap *ROOM_INDEX_DATA_get_wrap(ROOM_INDEX_DATA *p)
{
    struct ROOM_INDEX_DATA_wrap *wr = (struct ROOM_INDEX_DATA_wrap *)((uintptr_t)p - offsetof(struct ROOM_INDEX_DATA_wrap, wrapped));
    return wr;
}

static struct EXIT_DATA_wrap *EXIT_DATA_get_wrap(EXIT_DATA *p)
{
    struct EXIT_DATA_wrap *wr = (struct EXIT_DATA_wrap *)((uintptr_t)p - offsetof(struct EXIT_DATA_wrap, wrapped));
    return wr;
}

static struct RESET_DATA_wrap *RESET_DATA_get_wrap(RESET_DATA *p)
{
    struct RESET_DATA_wrap *wr = (struct RESET_DATA_wrap *)((uintptr_t)p - offsetof(struct RESET_DATA_wrap, wrapped));
    return wr;
}

static struct MOB_INDEX_DATA_wrap *MOB_INDEX_DATA_get_wrap(MOB_INDEX_DATA *p)
{
    struct MOB_INDEX_DATA_wrap *wr = (struct MOB_INDEX_DATA_wrap *)((uintptr_t)p - offsetof(struct MOB_INDEX_DATA_wrap, wrapped));
    return wr;
}

static struct OBJ_INDEX_DATA_wrap *OBJ_INDEX_DATA_get_wrap(OBJ_INDEX_DATA *p)
{
    struct OBJ_INDEX_DATA_wrap *wr = (struct OBJ_INDEX_DATA_wrap *)((uintptr_t)p - offsetof(struct OBJ_INDEX_DATA_wrap, wrapped));
    return wr;
}

static struct PROG_CODE_wrap *PROG_CODE_get_wrap(PROG_CODE *p)
{
    struct PROG_CODE_wrap *wr = (struct PROG_CODE_wrap *)((uintptr_t)p - offsetof(struct PROG_CODE_wrap, wrapped));
    return wr;
}

static struct PROG_LIST_wrap *PROG_LIST_get_wrap(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = (struct PROG_LIST_wrap *)((uintptr_t)p - offsetof(struct PROG_LIST_wrap, wrapped));
    return wr;
}

static struct SHOP_DATA_wrap *SHOP_DATA_get_wrap(SHOP_DATA *p)
{
    struct SHOP_DATA_wrap *wr = (struct SHOP_DATA_wrap *)((uintptr_t)p - offsetof(struct SHOP_DATA_wrap, wrapped));
    return wr;
}

static struct AFFECT_DATA_wrap *AFFECT_DATA_get_wrap(AFFECT_DATA *p)
{
    struct AFFECT_DATA_wrap *wr = (struct AFFECT_DATA_wrap *)((uintptr_t)p - offsetof(struct AFFECT_DATA_wrap, wrapped));
    return wr;
}

static struct HELP_DATA_wrap *HELP_DATA_get_wrap(HELP_DATA *p)
{
    struct HELP_DATA_wrap *wr = (struct HELP_DATA_wrap *)((uintptr_t)p - offsetof(struct HELP_DATA_wrap, wrapped));
    return wr;
}

static struct DESCRIPTOR_DATA_wrap *DESCRIPTOR_DATA_get_wrap(DESCRIPTOR_DATA *p)
{
    struct DESCRIPTOR_DATA_wrap *wr = (struct DESCRIPTOR_DATA_wrap *)((uintptr_t)p - offsetof(struct DESCRIPTOR_DATA_wrap, wrapped));
    return wr;
}

static struct BOSSACHV_wrap *BOSSACHV_get_wrap(BOSSACHV *p)
{
    struct BOSSACHV_wrap *wr = (struct BOSSACHV_wrap *)((uintptr_t)p - offsetof(struct BOSSACHV_wrap, wrapped));
    return wr;
}

static struct BOSSREC_wrap *BOSSREC_get_wrap(BOSSREC *p)
{
    struct BOSSREC_wrap *wr = (struct BOSSREC_wrap *)((uintptr_t)p - offsetof(struct BOSSREC_wrap, wrapped));
    return wr;
}


/* alloc and dealloc definitions */
CHAR_DATA *alloc_CHAR_DATA(void)
{
    struct CHAR_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&CHAR_DATA_type, &wr->aoh, &wr->aot);
    lua_init_CH(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_CHAR_DATA(CHAR_DATA *p)
{
    struct CHAR_DATA_wrap *wr = CHAR_DATA_get_wrap(p);
    lua_deinit_CH(&wr->wrapped);
    arc_obj_deinit(&CHAR_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

OBJ_DATA *alloc_OBJ_DATA(void)
{
    struct OBJ_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&OBJ_DATA_type, &wr->aoh, &wr->aot);
    lua_init_OBJ(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_OBJ_DATA(OBJ_DATA *p)
{
    struct OBJ_DATA_wrap *wr = OBJ_DATA_get_wrap(p);
    lua_deinit_OBJ(&wr->wrapped);
    arc_obj_deinit(&OBJ_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

AREA_DATA *alloc_AREA_DATA(void)
{
    struct AREA_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&AREA_DATA_type, &wr->aoh, &wr->aot);
    lua_init_AREA(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_AREA_DATA(AREA_DATA *p)
{
    struct AREA_DATA_wrap *wr = AREA_DATA_get_wrap(p);
    lua_deinit_AREA(&wr->wrapped);
    arc_obj_deinit(&AREA_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

ROOM_INDEX_DATA *alloc_ROOM_INDEX_DATA(void)
{
    struct ROOM_INDEX_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&ROOM_INDEX_DATA_type, &wr->aoh, &wr->aot);
    lua_init_ROOM(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_ROOM_INDEX_DATA(ROOM_INDEX_DATA *p)
{
    struct ROOM_INDEX_DATA_wrap *wr = ROOM_INDEX_DATA_get_wrap(p);
    lua_deinit_ROOM(&wr->wrapped);
    arc_obj_deinit(&ROOM_INDEX_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

EXIT_DATA *alloc_EXIT_DATA(void)
{
    struct EXIT_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&EXIT_DATA_type, &wr->aoh, &wr->aot);
    lua_init_EXIT(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_EXIT_DATA(EXIT_DATA *p)
{
    struct EXIT_DATA_wrap *wr = EXIT_DATA_get_wrap(p);
    lua_deinit_EXIT(&wr->wrapped);
    arc_obj_deinit(&EXIT_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

RESET_DATA *alloc_RESET_DATA(void)
{
    struct RESET_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&RESET_DATA_type, &wr->aoh, &wr->aot);
    lua_init_RESET(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_RESET_DATA(RESET_DATA *p)
{
    struct RESET_DATA_wrap *wr = RESET_DATA_get_wrap(p);
    lua_deinit_RESET(&wr->wrapped);
    arc_obj_deinit(&RESET_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

MOB_INDEX_DATA *alloc_MOB_INDEX_DATA(void)
{
    struct MOB_INDEX_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&MOB_INDEX_DATA_type, &wr->aoh, &wr->aot);
    lua_init_MOBPROTO(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_MOB_INDEX_DATA(MOB_INDEX_DATA *p)
{
    struct MOB_INDEX_DATA_wrap *wr = MOB_INDEX_DATA_get_wrap(p);
    lua_deinit_MOBPROTO(&wr->wrapped);
    arc_obj_deinit(&MOB_INDEX_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

OBJ_INDEX_DATA *alloc_OBJ_INDEX_DATA(void)
{
    struct OBJ_INDEX_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&OBJ_INDEX_DATA_type, &wr->aoh, &wr->aot);
    lua_init_OBJPROTO(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_OBJ_INDEX_DATA(OBJ_INDEX_DATA *p)
{
    struct OBJ_INDEX_DATA_wrap *wr = OBJ_INDEX_DATA_get_wrap(p);
    lua_deinit_OBJPROTO(&wr->wrapped);
    arc_obj_deinit(&OBJ_INDEX_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

PROG_CODE *alloc_PROG_CODE(void)
{
    struct PROG_CODE_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&PROG_CODE_type, &wr->aoh, &wr->aot);
    lua_init_PROG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_PROG_CODE(PROG_CODE *p)
{
    struct PROG_CODE_wrap *wr = PROG_CODE_get_wrap(p);
    lua_deinit_PROG(&wr->wrapped);
    arc_obj_deinit(&PROG_CODE_type, &wr->aoh, &wr->aot);
    free(wr);
}

PROG_LIST *alloc_MPROG_LIST(void)
{
    struct PROG_LIST_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&PROG_LIST_type, &wr->aoh, &wr->aot);
    lua_init_MTRIG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_MPROG_LIST(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = PROG_LIST_get_wrap(p);
    lua_deinit_MTRIG(&wr->wrapped);
    arc_obj_deinit(&PROG_LIST_type, &wr->aoh, &wr->aot);
    free(wr);
}

PROG_LIST *alloc_OPROG_LIST(void)
{
    struct PROG_LIST_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&PROG_LIST_type, &wr->aoh, &wr->aot);
    lua_init_OTRIG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_OPROG_LIST(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = PROG_LIST_get_wrap(p);
    lua_deinit_OTRIG(&wr->wrapped);
    arc_obj_deinit(&PROG_LIST_type, &wr->aoh, &wr->aot);
    free(wr);
}

PROG_LIST *alloc_APROG_LIST(void)
{
    struct PROG_LIST_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&PROG_LIST_type, &wr->aoh, &wr->aot);
    lua_init_ATRIG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_APROG_LIST(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = PROG_LIST_get_wrap(p);
    lua_deinit_ATRIG(&wr->wrapped);
    arc_obj_deinit(&PROG_LIST_type, &wr->aoh, &wr->aot);
    free(wr);
}

PROG_LIST *alloc_RPROG_LIST(void)
{
    struct PROG_LIST_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&PROG_LIST_type, &wr->aoh, &wr->aot);
    lua_init_RTRIG(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_RPROG_LIST(PROG_LIST *p)
{
    struct PROG_LIST_wrap *wr = PROG_LIST_get_wrap(p);
    lua_deinit_RTRIG(&wr->wrapped);
    arc_obj_deinit(&PROG_LIST_type, &wr->aoh, &wr->aot);
    free(wr);
}

SHOP_DATA *alloc_SHOP_DATA(void)
{
    struct SHOP_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&SHOP_DATA_type, &wr->aoh, &wr->aot);
    lua_init_SHOP(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_SHOP_DATA(SHOP_DATA *p)
{
    struct SHOP_DATA_wrap *wr = SHOP_DATA_get_wrap(p);
    lua_deinit_SHOP(&wr->wrapped);
    arc_obj_deinit(&SHOP_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

AFFECT_DATA *alloc_AFFECT_DATA(void)
{
    struct AFFECT_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&AFFECT_DATA_type, &wr->aoh, &wr->aot);
    lua_init_AFFECT(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_AFFECT_DATA(AFFECT_DATA *p)
{
    struct AFFECT_DATA_wrap *wr = AFFECT_DATA_get_wrap(p);
    lua_deinit_AFFECT(&wr->wrapped);
    arc_obj_deinit(&AFFECT_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

HELP_DATA *alloc_HELP_DATA(void)
{
    struct HELP_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&HELP_DATA_type, &wr->aoh, &wr->aot);
    lua_init_HELP(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_HELP_DATA(HELP_DATA *p)
{
    struct HELP_DATA_wrap *wr = HELP_DATA_get_wrap(p);
    lua_deinit_HELP(&wr->wrapped);
    arc_obj_deinit(&HELP_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

DESCRIPTOR_DATA *alloc_DESCRIPTOR_DATA(void)
{
    struct DESCRIPTOR_DATA_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&DESCRIPTOR_DATA_type, &wr->aoh, &wr->aot);
    lua_init_DESCRIPTOR(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_DESCRIPTOR_DATA(DESCRIPTOR_DATA *p)
{
    struct DESCRIPTOR_DATA_wrap *wr = DESCRIPTOR_DATA_get_wrap(p);
    lua_deinit_DESCRIPTOR(&wr->wrapped);
    arc_obj_deinit(&DESCRIPTOR_DATA_type, &wr->aoh, &wr->aot);
    free(wr);
}

BOSSACHV *alloc_BOSSACHV(void)
{
    struct BOSSACHV_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&BOSSACHV_type, &wr->aoh, &wr->aot);
    lua_init_BOSSACHV(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_BOSSACHV(BOSSACHV *p)
{
    struct BOSSACHV_wrap *wr = BOSSACHV_get_wrap(p);
    lua_deinit_BOSSACHV(&wr->wrapped);
    arc_obj_deinit(&BOSSACHV_type, &wr->aoh, &wr->aot);
    free(wr);
}

BOSSREC *alloc_BOSSREC(void)
{
    struct BOSSREC_wrap *wr = calloc(1, sizeof(*wr));
    arc_obj_init(&BOSSREC_type, &wr->aoh, &wr->aot);
    lua_init_BOSSREC(&wr->wrapped);
    return &wr->wrapped;
}

void dealloc_BOSSREC(BOSSREC *p)
{
    struct BOSSREC_wrap *wr = BOSSREC_get_wrap(p);
    lua_deinit_BOSSREC(&wr->wrapped);
    arc_obj_deinit(&BOSSREC_type, &wr->aoh, &wr->aot);
    free(wr);
}

/* is_valid struct definitions */
bool is_valid_CHAR_DATA(CHAR_DATA *p) { return is_valid_arc_obj(&CHAR_DATA_type, p); }
bool is_valid_OBJ_DATA(OBJ_DATA *p) { return is_valid_arc_obj(&OBJ_DATA_type, p); }
bool is_valid_AREA_DATA(AREA_DATA *p) { return is_valid_arc_obj(&AREA_DATA_type, p); }
bool is_valid_ROOM_INDEX_DATA(ROOM_INDEX_DATA *p) { return is_valid_arc_obj(&ROOM_INDEX_DATA_type, p); }
bool is_valid_EXIT_DATA(EXIT_DATA *p) { return is_valid_arc_obj(&EXIT_DATA_type, p); }
bool is_valid_RESET_DATA(RESET_DATA *p) { return is_valid_arc_obj(&RESET_DATA_type, p); }
bool is_valid_MOB_INDEX_DATA(MOB_INDEX_DATA *p) { return is_valid_arc_obj(&MOB_INDEX_DATA_type, p); }
bool is_valid_OBJ_INDEX_DATA(OBJ_INDEX_DATA *p) { return is_valid_arc_obj(&OBJ_INDEX_DATA_type, p); }
bool is_valid_PROG_CODE(PROG_CODE *p) { return is_valid_arc_obj(&PROG_CODE_type, p); }
bool is_valid_PROG_LIST(PROG_LIST *p) { return is_valid_arc_obj(&PROG_LIST_type, p); }
bool is_valid_SHOP_DATA(SHOP_DATA *p) { return is_valid_arc_obj(&SHOP_DATA_type, p); }
bool is_valid_AFFECT_DATA(AFFECT_DATA *p) { return is_valid_arc_obj(&AFFECT_DATA_type, p); }
bool is_valid_HELP_DATA(HELP_DATA *p) { return is_valid_arc_obj(&HELP_DATA_type, p); }
bool is_valid_DESCRIPTOR_DATA(DESCRIPTOR_DATA *p) { return is_valid_arc_obj(&DESCRIPTOR_DATA_type, p); }
bool is_valid_BOSSACHV(BOSSACHV *p) { return is_valid_arc_obj(&BOSSACHV_type, p); }
bool is_valid_BOSSREC(BOSSREC *p) { return is_valid_arc_obj(&BOSSREC_type, p); }
