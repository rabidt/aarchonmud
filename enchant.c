/*
 * Enchanting of items
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

void add_enchant_affect( OBJ_DATA *obj, AFFECT_DATA *aff );
bool is_affect_cap_hard( int location );

struct enchantment_type
{
    int apply;
    int physical_chance;
    int mental_chance;
    int delta;
};

const struct enchantment_type enchantment_table[] =
{
    { APPLY_STR, 40, 0, 4 },
    { APPLY_CON, 40, 0, 4 },
    { APPLY_VIT, 40, 0, 4 },
    { APPLY_AGI, 40, 0, 4 },
    { APPLY_DEX, 40, 0, 4 },
    { APPLY_INT, 0, 40, 4 },
    { APPLY_WIS, 0, 40, 4 },
    { APPLY_DIS, 0, 40, 4 },
    { APPLY_CHA, 0, 40, 4 },
    { APPLY_LUC, 0, 40, 4 },
    { APPLY_HIT, 100, 100, 10 },
    { APPLY_MANA, 0, 100, 10 },
    { APPLY_MOVE, 100, 0, 10 },
    { APPLY_HITROLL, 100, 0, 1 },
    { APPLY_DAMROLL, 100, 0, 1 },
    { APPLY_AC, 50, 50, -10 },
    { APPLY_SAVES, 40, 60, -1 },
    {}
};

// returns random apply value based on rand_type
int get_random_apply( int rand_type )
{
    int i, total_chance = 0;
    int apply = APPLY_HIT; // just some default, should always get overwritten

    if ( per_chance(75) )
        return APPLY_STR;
    
    for ( i = 0; enchantment_table[i].apply; i++ )
    {
        // get relative chance to select this apply
        int chance;
        switch ( rand_type )
        {
            case ITEM_RANDOM_PHYSICAL:
                chance = enchantment_table[i].physical_chance;
                break;
            case ITEM_RANDOM_CASTER:
                chance = enchantment_table[i].mental_chance;
                break;
            default:
                chance = enchantment_table[i].physical_chance + enchantment_table[i].mental_chance;
        }
        // check
        if ( chance > 0 )
        {
            total_chance += chance;
            if ( number_range(1, total_chance) <= chance )
                apply = enchantment_table[i].apply;
        }
    }
    return apply;
}

// returns 1 OP worth of given apply
int get_delta( int apply )
{
    int i;
    for ( i = 0; enchantment_table[i].apply; i++ )
        if ( enchantment_table[i].apply == apply )
            return enchantment_table[i].delta;
    return 0;
}

/* get ops added by an enchantment
 * 0 indicates failure, -1, -2, .. indicate extreme failure
 */
int get_enchant_ops( OBJ_DATA *obj, int level )
{
    AFFECT_DATA *aff;
    int ops_left, fail, vnum;

    /* no enchanting of quest eq etc. */
    if ( obj == NULL || IS_OBJ_STAT(obj, ITEM_QUESTEQ) )
        return 0;

    /* ops below spec */
    ops_left = get_obj_spec( obj ) - get_obj_ops( obj );
    /* check for artificially created items => harder to enchant */
    if ( obj->pIndexData->vnum == OBJ_VNUM_SIVA_WEAPON )
        ops_left -= 5;

    /* check for failure */
    int level_diff = obj->level > level;
    fail = 50 + (level_diff > 0 ? level_diff : level_diff / 4);
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
    if ( obj == NULL || !IS_OBJ_STAT(obj, ITEM_RANDOM) 
      && !IS_OBJ_STAT(obj, ITEM_RANDOM_PHYSICAL) 
      && !IS_OBJ_STAT(obj, ITEM_RANDOM_CASTER))
        return;

    int ops_left = get_obj_spec(obj) - get_obj_ops(obj);
    
    if (IS_OBJ_STAT(obj, ITEM_RANDOM))
    {
        REMOVE_BIT( obj->extra_flags, ITEM_RANDOM );
        // chance to be physically or mentally oriented
        switch ( number_range(1,4) )
        {
            case 1:
                enchant_obj( obj, ops_left, ITEM_RANDOM_PHYSICAL, AFFDUR_INFINITE );
                break;
            case 2:
                enchant_obj( obj, ops_left, ITEM_RANDOM_CASTER, AFFDUR_INFINITE );
                break;
            default:
                enchant_obj( obj, ops_left, ITEM_RANDOM, AFFDUR_INFINITE );
                break;
        }
    }
    else if (IS_OBJ_STAT(obj, ITEM_RANDOM_PHYSICAL))
    {
        REMOVE_BIT( obj->extra_flags, ITEM_RANDOM_PHYSICAL );
        enchant_obj( obj, ops_left, ITEM_RANDOM_PHYSICAL, AFFDUR_INFINITE );
    }
    else if (IS_OBJ_STAT(obj, ITEM_RANDOM_CASTER))
    {
        REMOVE_BIT( obj->extra_flags, ITEM_RANDOM_CASTER );
        enchant_obj( obj, ops_left, ITEM_RANDOM_CASTER, AFFDUR_INFINITE );
    }
    else
        bugf("check_enchant_obj failed on obj : %d", obj->pIndexData->vnum);
}

// to ensure we don't exceed stat hardcap
int get_obj_stat_bonus( OBJ_DATA *obj, int stat )
{
    AFFECT_DATA *aff;
    int bonus = 0;
    // base stats
    for ( aff = obj->pIndexData->affected; aff != NULL; aff = aff->next )
        if ( aff->location == APPLY_STATS || aff->location == stat )
            bonus += aff->modifier;
    // enchanted stats
    for ( aff = obj->affected; aff != NULL; aff = aff->next )
        if ( aff->location == APPLY_STATS || aff->location == stat )
            bonus += aff->modifier;
    return bonus;
}

void enchant_obj( OBJ_DATA *obj, int ops, int rand_type, int duration )
{
    AFFECT_DATA af;

    if ( obj == NULL )
        return;

    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = duration;
    af.bitvector    = 0;
    af.detect_level = 0;
    
    while ( ops > 0 )
    {
        int apply = get_random_apply(rand_type);
        int delta = get_delta(apply);
        // ensure we don't exceed hardcaps
        if ( is_affect_cap_hard(apply) && (get_obj_stat_bonus(obj, apply) + delta) > get_affect_cap(apply, obj->level) )
            continue;
        // find existing affect, chance to abort if none found => reduced spread
        AFFECT_DATA *old_affect = affect_find_location(obj->affected, apply, duration);
        if ( !old_affect && number_bits(2) )
            continue;
        // add the apply
        af.location = apply;
        af.modifier = delta;
        add_enchant_affect(obj, &af);
        ops--;
    }
}

void add_enchant_affect( OBJ_DATA *obj, AFFECT_DATA *aff )
{
    AFFECT_DATA *obj_aff;

    if ( obj == NULL || aff == NULL )
        return;

    /* search for matching affect already on object */
    obj_aff = affect_find_location(obj->affected, aff->location, aff->duration);

    /* found matching affect on object? */
    if ( obj_aff != NULL )
        obj_aff->modifier += aff->modifier;
    else
        affect_to_obj( obj, aff );
}

void disenchant_obj( OBJ_DATA *obj )
{
    AFFECT_DATA *paf = obj->affected, *paf_prev = NULL;
    while ( paf )
    {
        if ( paf->duration == AFFDUR_DISENCHANTABLE )
        {
            if ( paf_prev )
            {
                paf_prev->next = paf->next;
                free_affect(paf);
                paf = paf_prev->next;
            }
            else // first affect
            {
                obj->affected = paf->next;
                free_affect(paf);
                paf = obj->affected;
            }
        }
        else
        {
            paf_prev = paf;
            paf = paf_prev->next;
        }
    }
    if ( !SET_BIT(obj->pIndexData->extra_flags, ITEM_MAGIC) )
        REMOVE_BIT(obj->extra_flags, ITEM_MAGIC);
}

// cost in gold of enchantment (for enchant weappn/armor spell)
int get_enchant_cost( OBJ_DATA *obj )
{
    int current_ops = get_obj_ops(obj);
    if ( CAN_WEAR(obj, ITEM_TRANSLUCENT) )
        current_ops += get_translucency_spec_penalty(obj->level);
    return 100 + current_ops * current_ops;
}
