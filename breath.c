/*

  All the breath spells are very similar, and copy'n paste isn't the best
  solution for that. So they now all use a commen function that deals out
  damage and effects to all targets. The effects are given as a function
  to be called.
  By Henning Koehler <koehlerh@in.tum.de>

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

#define SPELL_CHECK_RETURN if (check) return TRUE;

typedef void EFFECT_FUN ( void *vo, int level, int dam, int target );

/* special effect functions */
void slow_effect( void *vo, int level, int dam, int target );
void ooze_effect( void *vo, int level, int dam, int target );
void plague_effect( void *vo, int level, int dam, int target );
void weak_effect( void *vo, int level, int dam, int target );
void curse_effect( void *vo, int level, int dam, int target );

/* the prototype for all breath spells
 * NULL victim is allowed for multi_target spells
 */
void proto_spell_breath( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, 
			 int dam_type, EFFECT_FUN *effect_fun, bool multi_target )
{
  CHAR_DATA *vch, *vch_next;
  int cost, dam;

  /* extra cost */
  if ( IS_NPC(ch) )
      cost = ch->level;
  else
  {
      if ( ch->stance == STANCE_ARCANA && !IS_SET(meta_magic, META_MAGIC_EMPOWER) )
	  cost = ch->mana / 100;
      else
	  cost = 0;
      ch->mana -= meta_magic_adjust_cost(ch, mastery_adjust_cost(cost, get_mastery(ch, sn)), FALSE);
  }

  /* calc damage */
  dam = get_spell_damage( base_mana_cost(ch, sn) + cost, skill_table[sn].beats, level );
  dam = adjust_spell_damage( dam, ch );
  dam += get_spell_bonus_damage_sn(ch, sn);

  if (multi_target)
  {
    dam *= AREA_SPELL_FACTOR;

    /* effect to room */
    (*effect_fun)(ch->in_room, level, dam, TARGET_ROOM);

    /* damage and effect to people in room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        
        if ( is_safe_spell(ch, vch, vch != victim) )
            continue;
        
        if (saves_spell(vch, ch, level, dam_type))
        {
            (*effect_fun)(vch, level/2, dam/2, TARGET_CHAR);
            full_dam(ch, vch, dam/2, sn, dam_type, TRUE);
        }
        else
        {
            (*effect_fun)(vch, level, dam, TARGET_CHAR);
            full_dam(ch, vch, dam, sn, dam_type, TRUE);
        }
    }
  }
  else /* single target */
  {
    if (victim == NULL)
    {
      bug("proto_spell_breath: NULL victim", 0);
      return;
    }
    if (saves_spell(victim, ch, level, dam_type))
    {
      (*effect_fun)(victim, level/2, dam/2, TARGET_CHAR);
      full_dam(ch, victim, dam/2, sn, dam_type, TRUE);
    }
    else
    {
      (*effect_fun)(victim, level, dam, TARGET_CHAR);
      full_dam(ch, victim, dam, sn, dam_type, TRUE);
    }
  }
}

/* draconian spells */

DEF_SPELL_FUN(spell_acid_breath)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n spits acid at $N.",ch,NULL,victim,TO_NOTVICT);
  act("$n spits a stream of corrosive acid at you.",ch,NULL,victim,TO_VICT);
  act("You spit acid at $N.",ch,NULL,victim,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_ACID, &acid_effect, FALSE);
  return TRUE;
}

DEF_SPELL_FUN(spell_fire_breath)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n breathes forth a cone of fire.",ch,NULL,NULL,TO_ROOM);
  act("You breathe forth a cone of fire.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_FIRE, &fire_effect, TRUE);
  return TRUE;
}

DEF_SPELL_FUN(spell_frost_breath)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n breathes out a freezing cone of frost!",ch,NULL,NULL,TO_ROOM);
  act("You breathe out a cone of frost.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_COLD, &cold_effect, TRUE);
  return TRUE;
}

DEF_SPELL_FUN(spell_gas_breath)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n breathes out a cloud of poisonous gas!",ch,NULL,NULL,TO_ROOM);
  act("You breathe out a cloud of poisonous gas.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_POISON, &poison_effect, TRUE);
  return TRUE;
}

DEF_SPELL_FUN(spell_lightning_breath)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n breathes a bolt of lightning at $N.",ch,NULL,victim,TO_NOTVICT);
  act("$n breathes a bolt of lightning at you!",ch,NULL,victim,TO_VICT);
  act("You breathe a bolt of lightning at $N.",ch,NULL,victim,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_LIGHTNING, &shock_effect, FALSE);
  return TRUE;
}

/* Necromancer spells */

DEF_SPELL_FUN(spell_cone_of_exhaustion)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n breathes out a cone of sickly white.",ch,NULL,NULL,TO_ROOM);
  act("You breathe out a cone of sickly white.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_COLD, &slow_effect, TRUE);
  return TRUE;
}    
    
DEF_SPELL_FUN(spell_forboding_ooze)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n breathes a ball of ooze at $N.",ch,NULL,victim,TO_NOTVICT);
  act("$n breathes a ball of ooze at you!",ch,NULL,victim,TO_VICT);
  act("You breathe a ball of ooze at $N.",ch,NULL,victim,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_ACID, &ooze_effect, FALSE);
  return TRUE;
}

DEF_SPELL_FUN(spell_tomb_stench)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n breathes death at $N.",ch,NULL,victim,TO_NOTVICT);
  act("$n breathes death at you!",ch,NULL,victim,TO_VICT);
  act("You breathe death at $N.",ch,NULL,victim,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_DISEASE, &plague_effect, FALSE);
  return TRUE;
}

DEF_SPELL_FUN(spell_zombie_breath)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n breathes out a putrid smelling cloud.",ch,NULL,NULL,TO_ROOM);
  act("You breathe out a putrid smelling cloud.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_HARM, &weak_effect, TRUE);
  return TRUE;
}

DEF_SPELL_FUN(spell_zone_of_damnation)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  SPELL_CHECK_RETURN
  act("$n damns the area.",ch,NULL,NULL,TO_ROOM);
  act("You damn the area.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_NEGATIVE, &curse_effect, TRUE);
  return TRUE;
}

/* new effects for necromancer spells */

void slow_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( number_bits(1) || saves_spell(victim, NULL, level / 4 + dam / 20, DAM_OTHER) )
    return;
  
  if (IS_AFFECTED(victim,AFF_HASTE))
  {
      if (check_dispel(level,victim,skill_lookup("haste")))
	  act_gag("$n is moving less quickly.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
      else
	  return;
  }
  
  if (!IS_AFFECTED(victim,AFF_SLOW))
  {
    af.where     = TO_AFFECTS;
    af.type      = skill_lookup("slow");
    af.level     = level;
    af.duration  = level/8;
    af.location  = APPLY_AGI;
    af.modifier  = -1 - level/5;
    af.bitvector = AFF_SLOW;
    affect_to_char( victim, &af );
    act_gag("You feel yourself slowing d o w n...", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
    act_gag("$n starts to move in slow motion.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
  }
}

void ooze_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  int sn = skill_lookup("forboding ooze");

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( saves_spell(victim, NULL, level / 4 + dam / 20, DAM_OTHER) )
    return;

  af.where     = TO_AFFECTS;
  af.type      = sn;
  af.level     = level;
  af.duration  = level/8;
  af.location  = APPLY_DEX;
  af.modifier  = -11;
  af.bitvector = 0;
  affect_join( victim, &af );
  act_gag("You feel slow and clumsy.", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
  act_gag("$n begins to move around clumsily.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
}

void plague_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( number_bits(1) || saves_spell(victim, NULL, level / 4 + dam / 20, DAM_DISEASE) )
    return;

  if (!IS_AFFECTED(victim, AFF_PLAGUE))
  {
    af.where     = TO_AFFECTS;
    af.type      = skill_lookup("plague");
    af.level     = level;
    af.duration  = level/4;
    af.location  = APPLY_CON;
    af.modifier  = -1 - level/5;
    af.bitvector = AFF_PLAGUE;
    affect_to_char(victim, &af);
    act_gag("You scream in agony as plague sores erupt from your skin.", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
    act_gag("$n screams in agony as plague sores erupt from $s skin.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
  }
}

void weak_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( number_bits(1) || saves_spell(victim, NULL, level / 4 + dam / 20, DAM_OTHER) )
    return;
  
  if (!IS_AFFECTED(victim,AFF_WEAKEN))
  {
    af.where     = TO_AFFECTS;
    af.type      = skill_lookup("weaken");
    af.level     = level;
    af.duration  = level/4;
    af.location  = APPLY_STR;
    af.modifier  = -1 - level/2;
    af.bitvector = AFF_WEAKEN;
    affect_to_char( victim, &af );
    act_gag("You feel your strength slip away.", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
    act_gag("$n looks tired and weak.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
  }
}

void curse_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( number_bits(1) || saves_spell(victim, NULL, level / 4 + dam / 20, DAM_NEGATIVE) )
    return;
  
  if (!IS_AFFECTED(victim,AFF_CURSE))
  {
    af.where     = TO_AFFECTS;
    af.type      = skill_lookup("curse");
    af.level     = level;
    af.duration  = level/8;
    af.location  = APPLY_VIT;
    af.modifier  = -1 - level/5;
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );
    act_gag("You feel unclean.", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
    act_gag("$n looks very uncomfortable.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
  }
}


/******** draconic breath for PC races - similar but different **********/

static int deduct_draconic_breath_cost( CHAR_DATA *ch )
{
    int mana_reserved = ch->max_mana * ch->calm / 100;
    int move_reserved = ch->max_move * ch->calm / 100;
    int mana_cost = (ch->mana > mana_reserved ? 1 + (ch->mana - mana_reserved) / 100 : 0);
    int move_cost = (ch->move > move_reserved ? 1 + (ch->move - move_reserved) / 100 : 0);
    
    ch->mana -= mana_cost;
    ch->move -= move_cost;
    
    return mana_cost + move_cost;
}

static void proto_draconic_breath( int sn, int cost, CHAR_DATA *ch, CHAR_DATA *victim, int dam_type, EFFECT_FUN *effect_fun, bool multi_target )
{
    CHAR_DATA *vch, *vch_next;
    
    int level = ch->level;
    int dam = get_spell_damage(cost, PULSE_VIOLENCE, level);
    // half normal focus bonus applies - it's not really a spell, but it does draw on mana
    dam += dam * get_focus_bonus(ch) / 200;
    if ( ch->level >= LEVEL_MIN_HERO )
        dam += dam * (10 + ch->level - LEVEL_MIN_HERO) / 100;
    
    if ( multi_target )
    {
        /* effect to room */
        (*effect_fun)(ch->in_room, level, dam, TARGET_ROOM);

        /* damage and effect to people in room */
        for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next_in_room;
        
            if ( is_safe_spell(ch, vch, vch != victim) || !is_opponent(ch, vch) )
                continue;
            
            int vch_dam = dam * (vch == victim ? (1 + AREA_SPELL_FACTOR) / 2 : AREA_SPELL_FACTOR);
            if ( saves_spell(vch, ch, level, dam_type) )
                vch_dam /= 2;
            (*effect_fun)(vch, level, vch_dam, TARGET_CHAR);
            full_dam(ch, vch, vch_dam, sn, dam_type, TRUE);
        }
    }
    else /* single target */
    {
        if ( saves_spell(victim, ch, level, dam_type) )
            dam /= 2;
        (*effect_fun)(victim, level, dam, TARGET_CHAR);
        full_dam(ch, victim, dam, sn, dam_type, TRUE);
    }
}

void check_draconic_breath( CHAR_DATA *ch )
{
    // requires draconic breath skill - low skill means longer wait
    int skill = get_skill(ch, gsn_draconic_breath);
    if ( IS_NPC(ch) || !skill || !per_chance(skill) )
        return;
    
    // can only breathe once every 1d4 rounds
    // we track this via a draconic breath affect
    AFFECT_DATA *paf = affect_find(ch->affected, gsn_draconic_breath);
    if ( paf != NULL )
    {
        if ( paf->modifier <= 0 )
            affect_remove(ch, paf);
        else
        {
            // APPLY_NONE, so it's safe to update affect directly
            paf->modifier--;
            return;
        }
    }
    
    // ensure we have a victim
    CHAR_DATA *victim = ch->fighting;
    if ( !victim )
        return;

    // draconic breath costs mana/moves
    // but like tempest, it is more efficient
    int cost = 2 * deduct_draconic_breath_cost(ch);
    if ( cost <= 0 )
        return;
    
    // 1d4 rounds till next breath
    int wait = number_range(0,3);
    if ( wait > 0 )
    {
        AFFECT_DATA af = {};
        af.where        = TO_AFFECTS;
        af.type         = gsn_draconic_breath;
        af.level        = ch->level;
        af.duration     = 1;
        af.location     = APPLY_NONE;
        af.modifier     = wait;
        af.bitvector    = 0;
        affect_to_char(ch, &af);
    }
        
    // bloodline determines type of breath
    switch ( ch->pcdata ? ch->pcdata->morph_race : 0 )
    {
        default:
        case MORPH_DRAGON_RED:
            act("$n breathes forth a cone of fire.", ch, NULL, NULL, TO_ROOM);
            act("You breathe forth a cone of fire.", ch, NULL, NULL, TO_CHAR);
            proto_draconic_breath(gsn_fire_breath, cost, ch, victim, DAM_FIRE, &fire_effect, TRUE);
            break;
        case MORPH_DRAGON_GREEN:
            act("$n breathes out a cloud of poisonous gas!", ch, NULL, NULL, TO_ROOM);
            act("You breathe out a cloud of poisonous gas.", ch, NULL, NULL, TO_CHAR);
            proto_draconic_breath(gsn_gas_breath, cost, ch, victim, DAM_POISON, &poison_effect, TRUE);
            break;
        case MORPH_DRAGON_BLUE:
            act("$n breathes a blast of lightning at $N.", ch, NULL, victim, TO_NOTVICT);
            act("$n breathes a blast of lightning at you!", ch, NULL, victim, TO_VICT);
            act("You breathe a blast of lightning at $N.", ch, NULL, victim, TO_CHAR);
            proto_draconic_breath(gsn_lightning_breath, cost, ch, victim, DAM_LIGHTNING, &shock_effect, FALSE);
            break;
        case MORPH_DRAGON_BLACK:
            act("$n spits acid at $N.", ch, NULL, victim, TO_NOTVICT);
            act("$n spits a stream of corrosive acid at you.", ch, NULL, victim, TO_VICT);
            act("You spit acid at $N.", ch, NULL, victim, TO_CHAR);
            proto_draconic_breath(gsn_acid_breath, cost, ch, victim, DAM_ACID, &acid_effect, FALSE);
            break;
        case MORPH_DRAGON_WHITE:
            act("$n breathes out a freezing cone of frost!", ch, NULL, NULL, TO_ROOM);
            act("You breathe out a cone of frost.", ch, NULL, NULL, TO_CHAR);
            proto_draconic_breath(gsn_frost_breath, cost, ch, victim, DAM_COLD, &cold_effect, TRUE);
            break;
    }
}
