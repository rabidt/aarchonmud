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
      ch->mana -= meta_magic_adjust_cost(cost, FALSE);
  }

  /* calc damage */
  dam = get_spell_damage( skill_table[sn].min_mana + cost,
			  skill_table[sn].beats, level );
  dam = adjust_spell_damage( dam, ch );

  if (multi_target)
  {
      /* more damage if no main target */
      if ( victim != NULL )
	  dam = dam * 3/4;

    /* effect to room */
    (*effect_fun)(ch->in_room, level, dam, TARGET_ROOM);

    /* damage and effect to people in room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
      vch_next = vch->next_in_room;
        
      if (is_safe_spell(ch,vch, vch != victim)
	  || (IS_NPC(vch) && IS_NPC(ch) 
	      && ch->fighting != vch && vch->fighting != ch))
	continue;
        
      if (vch == victim) /* full damage */
      {
	if (saves_spell(level, vch, dam_type))
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
      else /* half damage */
      {
	if (saves_spell(level, vch, dam_type))
	{
	  (*effect_fun)(vch, level/2, dam/4, TARGET_CHAR);
	  full_dam(ch, vch, dam/4, sn, dam_type, TRUE);
	}
	else
	{
	  (*effect_fun)(vch, level, dam/2, TARGET_CHAR);
	  full_dam(ch, vch, dam/2, sn, dam_type, TRUE);
	}
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
    if (saves_spell(level, victim, dam_type))
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

void spell_acid_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n spits acid at $N.",ch,NULL,victim,TO_NOTVICT);
  act("$n spits a stream of corrosive acid at you.",ch,NULL,victim,TO_VICT);
  act("You spit acid at $N.",ch,NULL,victim,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_ACID, &acid_effect, FALSE);
}

void spell_fire_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n breathes forth a cone of fire.",ch,NULL,NULL,TO_ROOM);
  act("You breathe forth a cone of fire.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_FIRE, &fire_effect, TRUE);
}

void spell_frost_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n breathes out a freezing cone of frost!",ch,NULL,NULL,TO_ROOM);
  act("You breathe out a cone of frost.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_COLD, &cold_effect, TRUE);
}

void spell_gas_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n breathes out a cloud of poisonous gas!",ch,NULL,NULL,TO_ROOM);
  act("You breathe out a cloud of poisonous gas.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_POISON, &poison_effect, TRUE);
}

void spell_lightning_breath(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n breathes a bolt of lightning at $N.",ch,NULL,victim,TO_NOTVICT);
  act("$n breathes a bolt of lightning at you!",ch,NULL,victim,TO_VICT);
  act("You breathe a bolt of lightning at $N.",ch,NULL,victim,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_LIGHTNING, &shock_effect, FALSE);
}

/* Necromancer spells */

void spell_cone_of_exhaustion( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n breathes out a cone of sickly white.",ch,NULL,victim,TO_NOTVICT);
  act("$n breathes out a cone of sickly white at you!",ch,NULL,victim,TO_VICT);
  act("You breathe out a cone of sickly white.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_COLD, &slow_effect, TRUE);
}    
    
void spell_forboding_ooze(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n breathes a ball of ooze at $N.",ch,NULL,victim,TO_NOTVICT);
  act("$n breathes a ball of ooze at you!",ch,NULL,victim,TO_VICT);
  act("You breathe a ball of ooze at $N.",ch,NULL,victim,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_ACID, &ooze_effect, FALSE);
}

void spell_tomb_stench(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n breathes death at $N.",ch,NULL,victim,TO_NOTVICT);
  act("$n breathes death at you!",ch,NULL,victim,TO_VICT);
  act("You breathe death at $N.",ch,NULL,victim,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_HARM, &plague_effect, FALSE);
}

void spell_zombie_breath( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n breathes out a putrid smelling cloud.",ch,NULL,victim,TO_NOTVICT);
  act("$n breathes a putrid smelling cloud on you!",ch,NULL,victim,TO_VICT);
  act("You breathe out a putrid smelling cloud.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_NEGATIVE, &weak_effect, TRUE);
}

void spell_zone_of_damnation( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  act("$n damns the area.",ch,NULL,victim,TO_NOTVICT);
  /*act("$n damns the area around you!",ch,NULL,victim,TO_VICT);*/
  act("You damn the area.",ch,NULL,NULL,TO_CHAR);
  proto_spell_breath(sn, level, ch, victim, DAM_HOLY, &curse_effect, TRUE);
}

/* new effects for necromancer spells */

void slow_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( number_bits(1) || saves_spell(level / 4 + dam / 20, victim, DAM_OTHER) )
    return;
  
  if (IS_AFFECTED(victim,AFF_HASTE))
  {
      if (check_dispel(level,victim,skill_lookup("haste")))
	  act("$n is moving less quickly.",victim,NULL,NULL,TO_ROOM);
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
    send_to_char( "You feel yourself slowing d o w n...\n\r", victim );
    act("$n starts to move in slow motion.",victim,NULL,NULL,TO_ROOM);
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
  if ( saves_spell(level / 4 + dam / 20, victim, DAM_OTHER) )
    return;

  af.where     = TO_AFFECTS;
  af.type      = sn;
  af.level     = level;
  af.duration  = level/8;
  af.location  = APPLY_DEX;
  af.modifier  = -11;
  af.bitvector = 0;
  affect_join( victim, &af );
  send_to_char( "You feel slow and clumsy.\n\r", victim );
  act("$n begins to move around clumsily.",victim,NULL,NULL,TO_ROOM);
}

void plague_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  int chance;

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( number_bits(1) || saves_spell(level / 4 + dam / 20, victim, DAM_DISEASE) )
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
    send_to_char("You scream in agony as plague sores erupt from your skin.\n\r",victim);
    act("$n screams in agony as plague sores erupt from $s skin.",victim,NULL,NULL,TO_ROOM);
  }
}

void weak_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  int chance;

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( number_bits(1) || saves_spell(level / 4 + dam / 20, victim, DAM_OTHER) )
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
    send_to_char( "You feel your strength slip away.\n\r", victim );
    act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);
  }
}

void curse_effect( void *vo, int level, int dam, int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

  if (target != TARGET_CHAR)
    return;

  /* saving throw */
  if ( number_bits(1) || saves_spell(level / 4 + dam / 20, victim, DAM_NEGATIVE) )
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
    send_to_char( "You feel unclean.\n\r", victim );
    act("$n looks very uncomfortable.",victim,NULL,NULL,TO_ROOM);
  }
}



