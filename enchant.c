/*
 * Enchanting of items
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"


void enchant_obj( OBJ_DATA *obj, int ops );
void enchant_obj_stat( OBJ_DATA *obj, int ops );
void enchant_obj_roll( OBJ_DATA *obj, int ops );
void enchant_obj_max( OBJ_DATA *obj, int ops );
void add_enchant_affect( OBJ_DATA *obj, AFFECT_DATA *aff );

/* get ops added by an enchantment
 * 0 indicates failure, -1, -2, .. indicate extreme failure
 */
int get_enchant_ops( OBJ_DATA *obj, int level )
{
    AFFECT_DATA *aff;
    int ops_left, fail, vnum;

    /* no enchanting of quest eq etc. */
    if ( obj == NULL
	 || IS_OBJ_STAT(obj, ITEM_STICKY)
	 || obj->pIndexData->vnum == OBJ_VNUM_BLACKSMITH)
	return 0;

    /* no enchanting of objects which add special effects */
    for ( aff = obj->pIndexData->affected; aff != NULL; aff = aff->next )
	if ( aff->bitvector != 0 )
	    return 0;

    /* ops below spec */
    ops_left = get_obj_spec( obj ) - get_obj_ops( obj );
    if ( IS_OBJ_STAT(obj, ITEM_BLESS) )
	ops_left += 1;
    /* check for artificially created items => harder to enchant */
    if ( obj->pIndexData->vnum == OBJ_VNUM_SIVA_WEAPON )
	ops_left -= 5;

    /* check for failure */
    fail = 50 + UMAX(0, obj->level - level);
    fail -= 5 * ops_left;
    fail = URANGE(1, fail, 99);
    if ( chance(fail) )
    {
	fail = 0;
	while ( number_bits(2) == 0 )
	    fail--;
	return fail;
    }

    /* ok, successful enchant */
    ops_left = UMAX( 0, ops_left );
    return ops_left/2 + 1;
}

/* enchants 'random' flagged objects */
void check_enchant_obj( OBJ_DATA *obj )
{
    if ( obj == NULL || !IS_OBJ_STAT(obj, ITEM_RANDOM) )
	return;

    REMOVE_BIT( obj->extra_flags, ITEM_RANDOM );
    enchant_obj( obj, get_obj_spec(obj) - get_obj_ops(obj) );
}

void enchant_obj( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA aff;
    int add;

    if ( obj == NULL || ops <= 0 )
	return;

    /* stats */
    add = number_range( 0, ops );
    add = number_range( 0, add );
    enchant_obj_stat( obj, add );
    ops -= add;
    /* rolls */
    add = number_range( 0, ops );
    enchant_obj_roll( obj, add );
    ops -= add;
    /* remaining to max stats */
    enchant_obj_max( obj, ops );
}

/* add str, con, .. */
void enchant_obj_stat( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    int add, total, max;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops * 4;
    
    while ( total > 0 )
    {
	/* modifier */
	if ( total < 4 )
	    add = total;
	else
	{
	    max = UMIN( 10, total );
	    add = number_range( 2, max );
	}
	af.modifier = add;

	/* location */
	switch ( number_range(0, 9) )
	{
	case 0: af.location = APPLY_STR; break;
	case 1: af.location = APPLY_CON; break;
	case 2: af.location = APPLY_VIT; break;
	case 3: af.location = APPLY_AGI; break;
	case 4: af.location = APPLY_DEX; break;
	case 5: af.location = APPLY_INT; break;
	case 6: af.location = APPLY_WIS; break;
	case 7: af.location = APPLY_DIS; break;
	case 8: af.location = APPLY_CHA; break;
	case 9: af.location = APPLY_LUC; break;
	}

	add_enchant_affect( obj, &af );
	
	total -= add;
    }
}

/* add hit, dam, ac, saves */
void enchant_obj_roll( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    int add, total, max, choice;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops;
    
    while ( total > 0 )
    {
	/* modifier */
	max = UMIN( 5, total );
	add = number_range( 1, max );
	af.modifier = add;
	
	/* location */
	switch ( obj->item_type )
	{
	default:
	    choice = number_range(0,3); break;
	case ITEM_WEAPON:
	    choice = number_range(0,1); break;
	case ITEM_ARMOR:
	    if ( number_bits(1) )
		choice = number_range(2,3);
	    else
		choice = number_range(0,3);
	}
	/* translate location & adjust modifier */
	switch( choice )
	{
	default: return;
	case 0: af.location = APPLY_HITROLL; break;
	case 1: af.location = APPLY_DAMROLL; break;
	case 2:
	    af.location = APPLY_SAVES;
	    af.modifier *= -1;
	    break;
	case 3:
	    af.location = APPLY_AC; 
	    af.modifier *= -5;
	}
		
	add_enchant_affect( obj, &af );
	
	total -= add;
    }
}

/* add hp, mana, move */
void enchant_obj_max( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    int add, total, max;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops * 10;
    
    while ( total > 0 )
    {
	/* modifier */
	if ( total < 20 )
	    add = total;
	else
	{
	    max = UMIN( 50, total );
	    add = number_range( 10, max );
	}
	af.modifier = add;

	/* location */
	switch ( number_range(0, 2) )
	{
	case 0: af.location = APPLY_HIT; break;
	case 1: af.location = APPLY_MANA; break;
	case 2: af.location = APPLY_MOVE; break;
	}

	add_enchant_affect( obj, &af );
	
	total -= add;
    }
}

void add_enchant_affect( OBJ_DATA *obj, AFFECT_DATA *aff )
{
    AFFECT_DATA *obj_aff;

    if ( obj == NULL || aff == NULL )
	return;

    /* search for matching affect already on object */
    for ( obj_aff = obj->affected; obj_aff != NULL; obj_aff = obj_aff->next )
	if ( obj_aff->location == aff->location
	     && obj_aff->duration == aff->duration )
	    break;

    /* found matching affect on object? */
    if ( obj_aff != NULL )
	obj_aff->modifier += aff->modifier;
    else
	affect_to_obj( obj, aff );
}
