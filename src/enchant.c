/*
 * Enchanting of items
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

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

// chance for adding an additional enchantment (for enchant weapon/armor spell)
int get_enchant_chance( OBJ_DATA *obj, int level )
{
    int ops_left = get_obj_spec(obj) - get_obj_ops(obj);
    int level_diff = level - obj->level;
    int chance = 50 + (level_diff > 0 ? level_diff/4 : level_diff);
    chance += 5 * ops_left;
    return URANGE(1, chance, 99);
}

/* get ops added by an enchantment
 * 0 indicates failure, -1, -2, .. indicate extreme failure
 */
int get_enchant_ops( OBJ_DATA *obj, int level )
{
    int ops_left, fail;

    /* check for failure */
    fail = get_enchant_chance(obj, level);
    if ( !per_chance(fail) )
    {
        fail = 0;
        while ( number_bits(2) == 0 )
            fail--;
        return fail;
    }

    /* ok, successful enchant */
    ops_left = get_obj_spec(obj) - get_obj_ops(obj);
    ops_left = UMAX( 0, ops_left );
    return ops_left/2 + 1;
}

/* enchants 'random' flagged objects */
void check_enchant_obj( OBJ_DATA *obj )
{
    if ( obj == NULL )
        return;
    if ( !IS_OBJ_STAT(obj, ITEM_RANDOM) 
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

// auto-update under-enchanted random equipment (e.g. after spec changes)
void check_reenchant_obj( OBJ_DATA *obj )
{
    if ( obj == NULL )
        return;
    
    if ( !IS_OBJ_STAT(obj->pIndexData, ITEM_RANDOM) 
      && !IS_OBJ_STAT(obj->pIndexData, ITEM_RANDOM_PHYSICAL) 
      && !IS_OBJ_STAT(obj->pIndexData, ITEM_RANDOM_CASTER))
        return;
    
    int ops_left = get_obj_spec(obj) - get_obj_ops(obj);
    if ( ops_left <= 1 )
        return;
    
    // copy random flag from protoype to obj
    if ( IS_OBJ_STAT(obj->pIndexData, ITEM_RANDOM) )
        SET_BIT(obj->extra_flags, ITEM_RANDOM);
    else if ( IS_OBJ_STAT(obj->pIndexData, ITEM_RANDOM_PHYSICAL) )
        SET_BIT(obj->extra_flags, ITEM_RANDOM_PHYSICAL);
    else if ( IS_OBJ_STAT(obj->pIndexData, ITEM_RANDOM_CASTER) )
        SET_BIT(obj->extra_flags, ITEM_RANDOM_CASTER);
    
    check_enchant_obj(obj);
    
    if ( obj->carried_by )
    {
        // ensure stats on char are updated if obj is worn
        if ( obj->wear_loc != -1 )
            reset_char(obj->carried_by);
        act("$p glows brightly as new enchantments manifest.", obj->carried_by, obj, NULL, TO_CHAR);
        logpf("%s (#%d) carried by %s has been reenchanted (+%d OPs)",
            remove_color(obj->short_descr),
            obj->pIndexData->vnum,
            obj->carried_by->name,
            ops_left);
    }
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
    enchant_obj_sn(obj, ops, rand_type, duration, 0);
}

void enchant_obj_sn( OBJ_DATA *obj, int ops, int rand_type, int duration, int sn )
{
    AFFECT_DATA af;

    if ( obj == NULL )
        return;

    af.where        = TO_OBJECT;
    af.type         = sn;
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
        AFFECT_DATA *old_affect = affect_find_location(obj->affected, af.type, apply, duration);
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
    obj_aff = affect_find_location(obj->affected, aff->type, aff->location, aff->duration);

    /* found matching affect on object? */
    if ( obj_aff != NULL )
    {
        obj_aff->modifier += aff->modifier;
        // max duration in case of non-negative durations
        obj_aff->duration = UMAX(obj_aff->duration, aff->duration);
    }
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
    if ( !IS_SET(obj->pIndexData->extra_flags, ITEM_MAGIC) )
        REMOVE_BIT(obj->extra_flags, ITEM_MAGIC);
}

// cost in gold of enchantment (for enchant weapon/armor spell)
int get_enchant_cost( OBJ_DATA *obj )
{
    int current_ops = get_obj_ops(obj);
    if ( IS_OBJ_STAT(obj, ITEM_TRANSLUCENT_EX) )
        current_ops += get_translucency_spec_penalty(obj->level);
    return 100 + current_ops * current_ops;
}

bool spell_enchant_obj( CHAR_DATA *ch, OBJ_DATA *obj, int level, char *arg, bool check, int sn )
{
    int cost, result, rand_type;

    if ( obj->wear_loc != -1 )
    {
        send_to_char("The item must be carried to be enchanted.\n\r", ch);
        return FALSE;
    }

    // get enchantment type, physical or mental
    if ( !strcmp(arg, "physical") )
        rand_type = ITEM_RANDOM_PHYSICAL;
    else if ( !strcmp(arg, "mental") )
        rand_type = ITEM_RANDOM_CASTER;
    else if ( !strcmp(arg, "") )
        rand_type = ITEM_RANDOM;
    else if ( !strcmp(arg, "analyze") )
    {
        if ( check )
            return TRUE;
        int ops = get_obj_ops_by_duration(obj, AFFDUR_DISENCHANTABLE);
        int chance = get_enchant_chance(obj, level);
        cost = get_enchant_cost(obj);
        ptc(ch, "%s currently has %d enchantments on it.\n\r", obj->short_descr, ops);
        ptc(ch, "The next enchantment costs %d gold, and has a %d%% chance of success.\n\r", cost, chance);
        return TRUE;
    }
    else if ( !strcmp(arg, "disenchant") )
    {
        if ( check )
            return TRUE;
        act("$p glows brightly, then fades.", ch, obj, NULL,TO_CHAR);
        act("$p glows brightly, then fades.", ch, obj, NULL,TO_ROOM);
        disenchant_obj(obj);
        return TRUE;
    }
    else
    {
        ptc(ch, "'%s' is not a valid enchantment option. Select physical, mental, analyze or disenchant.\n\r", arg);
        return FALSE;
    }
    
    cost = get_enchant_cost(obj);
    if ( (ch->gold + ch->silver/100) < cost )
    {
        ptc(ch, "Enchanting %s requires material components worth %d gold.\n\r", obj->short_descr, cost);
        return FALSE;
    }

    if ( check )
        return TRUE;
    
    result = get_enchant_ops(obj, level);
    
    if ( result == 0 )  /* failed, no bad result, no components used up */
    {
        send_to_char("Nothing seemed to happen.\n\r",ch);
        return TRUE;
    }
    
    ptc(ch, "You invest material components worth %d gold.\n\r", cost);
    deduct_cost(ch, cost*100);

    if ( result == -1 )  /* failed, components are consumed */
    {
        send_to_char("Your spell components are used up with no effect.\n\r",ch);
        return TRUE;
    }
    
    if ( result <= -2 ) /* item disenchanted */
    {
        act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);
        disenchant_obj(obj);
        return TRUE;
    }

    if ( result == 1 )  /* success! */
    {
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR);
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM);
    }
    else  /* exceptional enchant */
    {
        act("$p glows a brilliant gold!",ch,obj,NULL,TO_CHAR);
        act("$p glows a brilliant gold!",ch,obj,NULL,TO_ROOM);
    }

    /* now add the enchantments */ 
    SET_BIT(obj->extra_flags, ITEM_MAGIC);
    enchant_obj_sn(obj, result, rand_type, AFFDUR_DISENCHANTABLE, sn);
    return TRUE;
}
