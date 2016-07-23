/***********************************************************************
*                                                                      * 
*   Ranger class skills and spells, intended for use by Aeaea MUD.     *
*   All rights are reserved.                                           *
*                                                                      * 
*   Core ranger group by Brian Castle a.k.a. "Rimbol".                 *
*   Another batch added by James Stewart a.k.a "Siva".                 * 
***********************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"

/* Forage skill.  Rimbol, 7/97 */
void do_forage(CHAR_DATA *ch)
{
	OBJ_DATA *trailmix;
	int skill;
	
	skill = get_skill(ch, gsn_forage);

	if (skill < 1)
	{
	   send_to_char("You'd probably poison yourself, or worse.\n\r",ch);
	   return;
	}

	switch(ch->in_room->sector_type)
	{
	   case(SECT_DESERT):
		  send_to_char("You examine the desert scrub.\n\r",ch);
		  skill -= 45;   /* Very Tough */
		  break;
	   case(SECT_CITY):
		  send_to_char("You examine the trampled vegetation near the road.\n\r",ch);
		  skill -= 30;   /* Tough */
		  break;
	   case(SECT_FIELD):
	   case(SECT_FOREST):
	   case(SECT_HILLS):
	   case(SECT_MOUNTAIN):
		  send_to_char("You examine the prolific vegetation.\n\r",ch);
		  break;         /* As is */
	   case(SECT_INSIDE):
	   case(SECT_WATER_SHALLOW):
	   case(SECT_WATER_DEEP):
	   case(SECT_UNDERWATER):
	   case(SECT_AIR):
		  send_to_char("You don't see any vegetation here.\n\r",ch);
		  skill = 0;  /* Impossible */
		  break;
	   default:
		  skill = 0;  /* Impossible */
		  break;
	}

	skill = UMAX(1, skill); /* Handle any negatives due to subtraction */

	if (number_percent() < skill) /* success! */
	{
	   trailmix = create_object_vnum(OBJ_VNUM_TRAILMIX);
	   trailmix->value[0] = ch->level / 2;  /* "Full" value */
	   trailmix->value[1] = ch->level;      /* "Hunger" value */
	   obj_to_room( trailmix, ch->in_room );
	   act( "$n produces $p from the surroundings.", ch, trailmix, NULL, TO_ROOM );
	   act( "You produce $p from the surrounding vegetation.", 
		  ch, trailmix, NULL, TO_CHAR );
	   check_improve(ch, gsn_forage, TRUE, 3);
	}
	else
	{
	   send_to_char("You can't find anything edible.\n\r", ch);
	   check_improve(ch, gsn_forage, FALSE, 3);
	}

	WAIT_STATE(ch,skill_table[gsn_forage].beats);
	return;
}

/* Torch skill. --Rimbol, 7/97 */
void do_torch(CHAR_DATA *ch)
{
	OBJ_DATA *torch;
	int skill;

	skill = get_skill(ch, gsn_torch);

	if (skill < 1)
	{
	   send_to_char("You have no idea how to do that.\n\r",ch);
	   return;
	}

	switch(ch->in_room->sector_type)
	{
	   case(SECT_INSIDE):
		  send_to_char("You look around the building for flammable materials.\n\r",ch);
		  skill -= 50;       /* Extremely tough */
		  break;
	   case(SECT_CITY):
		  send_to_char("You search for suitable branches near the road.\n\r",ch);
		  skill -= 30;       /* Very Tough */
		  break;
	   case(SECT_DESERT):
		  send_to_char("You examine the desert scrub for something flammable.\n\r",ch);
		  skill -= 20;       /* Tough */
		  break;
	   case(SECT_FOREST):    /* Forest has more branches, a little easier */
		  skill += 10;       /* Falls thru to next send_to_char() */
	   case(SECT_FIELD):
	   case(SECT_HILLS):
	   case(SECT_MOUNTAIN):
		  send_to_char("You attempt to cut a torch from a nearby branch.\n\r",ch);
		  break;         /* As is */
	   case(SECT_WATER_DEEP):
	   case(SECT_WATER_SHALLOW):
	   case(SECT_UNDERWATER):
	   case(SECT_AIR):
		  send_to_char("You don't see any vegetation here.\n\r",ch);
		  skill = 0;  /* Impossible */
		  break;
	   default:
		  skill = 0;  /* Impossible */
		  break;
	}

	skill = UMAX(1, skill);   /* Handle overflows due to add/subtract. */
	skill = UMIN(99, skill); 

	if (number_percent() < skill)
	{
	   torch = create_object_vnum(OBJ_VNUM_TORCH);
	   torch->value[2] = ch->level / 2;  /* Duration */
	   obj_to_room( torch, ch->in_room );
	   act( "$n produces $p from the surroundings.", ch, torch, NULL, TO_ROOM );
	   act( "You produce $p from your surroundings.", 
		  ch, torch, NULL, TO_CHAR );
	   check_improve(ch, gsn_torch, TRUE, 3);
	}
	else
	{
	   send_to_char("You can't find anything suitable for making a torch.\n\r", ch);
	   check_improve(ch, gsn_torch, FALSE, 3);
	}

	WAIT_STATE(ch,skill_table[gsn_torch].beats);
	return;
}


/* Shelter skill.  --Rimbol, 7/97 */
void do_shelter( CHAR_DATA *ch ) 
{
	CHAR_DATA *gch;
	AFFECT_DATA af;
	bool ready;
	int gcount;
	int skill;

	ready = TRUE;
	gcount = 0;  /* People in group */

	skill = get_skill(ch, gsn_shelter);

	if (skill < 1)
	{
	   send_to_char("You don't know how to build a shelter.\n\r",ch);
	   return;
	}

	if ( IS_SET( ch->act, PLR_WAR ) ) 
	{
	   send_to_char("Sorry, we'd like the war to end during this century.\n\r",ch);
	   return;
	}

         if (IS_TAG(ch))
        {
            send_to_char("There is no place to hide in freeze tag.\n\r", ch );
            return;
	}

	if (ch->in_room == NULL) return;  /* A safeguard. */

	/* Have to scan thru group members in room to ensure they are sleeping. */
	for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
	   if ( gch != ch && is_same_group( gch, ch ) )
	   {
		  if (gch->position != POS_SLEEPING)
		  {
			 ready = FALSE;
			 break;
		  }
		  else
			 gcount++;
	   }

	if (!ready)
	{
	   act( "You are unable to shelter the group, since $N is still awake.\n\r",
		  ch, NULL, gch, TO_CHAR);
	   return;
	}

	switch(ch->in_room->sector_type)
	{
	   case(SECT_INSIDE):
	   case(SECT_CITY):
		  send_to_char("You find it difficult to build an effective shelter away from the wilderness.\n\r",ch);
		  skill /= 2;   /* Very Tough */
		  break;
	   case(SECT_DESERT):
		  send_to_char("You find it tough to build a shelter in the sand.\n\r",ch);
		  skill = skill*3/4;   /* Tough */
		  break;
	   case(SECT_FOREST):
	   case(SECT_FIELD):
	   case(SECT_HILLS):
	   case(SECT_MOUNTAIN):
		  break;         /* As is */
	   case(SECT_WATER_SHALLOW):
	   case(SECT_WATER_DEEP):
	   case(SECT_UNDERWATER):
	   case(SECT_AIR):
		  send_to_char("There is nothing here with which to build a shelter.\n\r",ch);
		  skill = 0;  /* Impossible */
		  break;
	   default:
		  skill = 0;  /* Impossible */
		  break;
	}
	
		
	if ( number_percent() > skill )
	{
	   send_to_char("You failed.\n\r",ch);
	   check_improve(ch, gsn_shelter, FALSE, 2);
	   WAIT_STATE(ch,skill_table[gsn_shelter].beats);
	   return;
	}

	af.where     = TO_AFFECTS;
	af.type      = gsn_shelter;
	af.level     = ch->level; 
	af.duration  = get_duration(gsn_shelter, ch->level);
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_SHELTER;

	if (gcount == 0) 
	{
	   send_to_char( "You build a shelter for yourself, masterfully disguising it as part of the area.\n\r", ch);
	   affect_to_char( ch, &af );
	}
	else
	{
	   send_to_char( "You build a shelter for your group, masterfully disguising it as part of the area.\n\r", ch);

	   for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
		  if (is_same_group( gch, ch ))
		  {
			  if (gch != ch)
				 act_new( "$N builds a shelter over your sleeping form!", 
					gch, NULL, ch, TO_CHAR, POS_SLEEPING);
			  affect_to_char( gch, &af );
		  }
	}

	send_to_char( "You go to sleep inside the shelter, totally concealed.\n\r", ch );
	set_pos( ch, POS_SLEEPING );
	check_improve(ch, gsn_shelter, TRUE, 2);
	WAIT_STATE(ch,skill_table[gsn_shelter].beats);

	return;
}


/* First Aid skill.  --Rimbol, 7/97 */
DEF_DO_FUN(do_firstaid)
{
	CHAR_DATA *target;
	char arg1 [MAX_INPUT_LENGTH];
	int heal = 0;
	int skill;
	int max_heal;
	int mana_cost = skill_table[gsn_firstaid].min_mana;
        char buf[MAX_STRING_LENGTH];

        skill = get_skill(ch, gsn_firstaid);
        if (skill < 1)
        {
           send_to_char("You don't know how to give first aid.\n\r", ch);
        }

        argument = one_argument( argument, arg1 );

	if ( arg1[0] == '\0' )
	    target = ch;
	else
	{
	    if ( ( target = get_char_room( ch, arg1 ) ) == NULL )
	    {
		send_to_char( "They aren't here.\n\r", ch );
		return;
	    }
	}

        if (ch->mana < mana_cost)
        {
           send_to_char("You don't have enough mana to focus properly.\n\r", ch);
           return;
        }

        max_heal = get_sn_heal( gsn_firstaid, ch->level, ch, target );

	switch ( ch->in_room->sector_type )
	{
	case SECT_FOREST:
	    max_heal = max_heal * 2;
	    break;
	case SECT_FIELD:
	case SECT_HILLS:
	case SECT_MOUNTAIN:
	    max_heal = max_heal * 7/4;
	    break;
	default:
            max_heal = max_heal * 3/2;
            break;
	}
	
	if (number_percent() < skill)
	{
           ch->mana -= mana_cost;
	   heal = max_heal * (target->max_hit - target->hit) / UMAX(1, target->max_hit);
       gain_hit(target, heal);
	   update_pos( target );
           if (ch == target)
	   {
		  send_to_char("You draw healing energy from the earth and give yourself first aid.\n\r", ch);
		  act("$n draws healing energy from the earth and gives $mself first aid.",
			 ch, NULL, target, TO_ROOM);
	   }
	   else
	   {
		  act("You draw healing energy from the earth and give $N first aid.",
			 ch, NULL, target, TO_CHAR);
		  act("$n draws healing energy from the earth and gives you first aid.",
			 ch, NULL, target, TO_VICT);
		  act("$n draws healing energy from the earth and gives $N first aid.",
			 ch, NULL, target, TO_NOTVICT);
	   }
           if (IS_IMMORTAL(ch) || IS_AFFECTED(ch, AFF_BATTLE_METER)) {
               sprintf(buf, "Healed %dhp.\n\r", heal);
               send_to_char(buf, ch);
           }

           check_improve(ch, gsn_firstaid, TRUE, 3);
	   WAIT_STATE(ch, skill_table[gsn_firstaid].beats);
	}
	else
	{
	   ch->mana -= mana_cost / 2;
	   send_to_char("You lose concentration and fumble your bandages into the muck.\n\r", ch);
	   check_improve(ch, gsn_firstaid, FALSE, 3);
	   WAIT_STATE(ch, skill_table[gsn_firstaid].beats);
	}

	check_sn_multiplay( ch, target, gsn_firstaid );

	return;
}

/* Detoxify skill.  --Rimbol, 7/97 */
DEF_DO_FUN(do_detoxify)
{
   OBJ_DATA *obj;
   AFFECT_DATA af;
   AFFECT_DATA *paf;
   int percent,skill;
	char arg[MAX_INPUT_LENGTH];

	one_argument( argument, arg );
	if ( arg[0] == '\0' )
   {
	  send_to_char("Detoxify what item?\n\r",ch);
		 return;
   }

   obj = get_obj_list(ch, arg, ch->carrying);

   if (obj == NULL)
   {
	  send_to_char("You don't have that item.\n\r",ch);
		 return;
   }

   skill = get_skill(ch, gsn_detoxify);

   if (skill < 1)
   {
	  send_to_char("You don't know how to do that.\n\r",ch);
		 return;
   }

   percent = number_percent();

   /* Give a little boost for blessed items. */
   if (IS_OBJ_STAT(obj, ITEM_BLESS))
	  percent = UMAX(0, percent-10);

   /* Remove poison from food or drink. */
   if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
   {
	  if (obj->value[3] == 0)
	  {
		 send_to_char("That item is not poisoned.\n\r",ch);
		 return;
	  }

	  if (percent < skill)  /* success! */
	  {
		 act("$n removes the deadly poison from $p.",ch,obj,NULL,TO_ROOM);
		 act("You remove the deadly poison from $p.",ch,obj,NULL,TO_CHAR);
		 if (obj->value[3])
		 {
			obj->value[3] = 0;
			check_improve(ch,gsn_detoxify,TRUE,2);
		 }
		 WAIT_STATE(ch,skill_table[gsn_detoxify].beats);
		 return;
	  }
   }
   /* Remove venom from weapons */
   else if (obj->item_type == ITEM_WEAPON)
   {
	  if (!IS_WEAPON_STAT(obj,WEAPON_POISON))
	  {
		 act("$p is not envenomed.",ch,obj,NULL,TO_CHAR);
		 return;
	  }

	  if (percent < skill)
	  {
		 paf = affect_find(obj->affected, gsn_poison);
		 if (paf != NULL)
			if (!saves_dispel(ch->level, paf->level, 0))
			{
			   act("$n removes the deadly poison from $p.",ch,obj,NULL,TO_ROOM);
			   act("You remove the deadly poison from $p.",ch,obj,NULL,TO_CHAR);
			   affect_remove_obj(obj,paf);
			   check_improve(ch,gsn_detoxify,TRUE,2);
			   WAIT_STATE(ch,skill_table[gsn_detoxify].beats);
			}
		 return;
	  }
   } 
   else
   {
	  send_to_char("Only food, drink, and weapons can be detoxified.\n\r",ch);
	  return;
   }

   /* To make it this far, the detox must have failed. */

   act("You fail to remove the poison from $p.",ch,obj,NULL,TO_CHAR);
   act("$n fails to remove the poison from $p.",ch,obj,NULL,TO_ROOM);

   check_improve(ch,gsn_detoxify,FALSE,2);

 /* Give a 20% chance for failed detox to poison the char making the attempt. */
   if (number_percent() <= 20 && !IS_IMMORTAL(ch))
   {
	  af.where     = TO_AFFECTS;
	  af.type      = gsn_poison;
	  af.level     = ch->level * skill / 100;
	  af.duration  = 1;
	  af.location  = APPLY_STR;
	  af.modifier  = -2;
	  af.bitvector = AFF_POISON;
	  affect_join( ch, &af );
	  send_to_char( "You feel very sick.\n\r", ch );
	  act("$n looks very ill.",ch,NULL,NULL,TO_ROOM);
   }

   WAIT_STATE(ch,skill_table[gsn_detoxify].beats);
	  return;
}


/* Tame animal/beast skill.  --Rimbol, 7/97 */
DEF_DO_FUN(do_tame)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	char arg1 [MAX_INPUT_LENGTH];
	int skill;

	argument = one_argument( argument, arg1 );

	skill = get_skill(ch, gsn_tame);

	if ( skill < 1 ) 
	{
	   send_to_char( "You don't know how to tame creatures.\n\r",ch);
	   return;
	}

	if ( arg1[0] == '\0' )
	{
	   send_to_char( "Tame what animal?\n\r", ch );
	   return;
	}

	if ( ( victim = get_victim_room( ch, arg1 ) ) == NULL )
	{
	   send_to_char( "They aren't here.\n\r", ch );
	   return;
	}

	if ( victim == ch )
	{
	   send_to_char( "You make a valiant effort to tame yourself.\n\r", ch );
	   return;
	}

	if ( IS_SET(victim->form, FORM_SENTIENT) || !IS_NPC(victim) )
	{
	   act("$N snickers at you.", ch, NULL, victim, TO_CHAR);
	   return;
	}

	if (is_safe(ch,victim)) return;

    if ( check_cha_follow(ch, victim->level) < victim->level )
        return;
    
	if ( IS_AFFECTED(victim, AFF_CHARM)
	     || IS_AFFECTED(ch, AFF_CHARM)
	     || ch->level < victim->level
	     || IS_SET(victim->imm_flags, IMM_CHARM) )
	{
	   act("$N growls viciously at you.", ch, NULL, victim, TO_CHAR);
	   return;
	}

	/* Modify based on size difference, charisma of ch, and int of victim.*/
	skill += 3 * (ch->size - victim->size);
	skill += (get_curr_stat(ch, STAT_CHA) - get_curr_stat(victim, STAT_INT)) / 4;
	skill += (ch->level - victim->level) - 25;

	/* Halve the chances for charm-resistant mobs */
	if (IS_SET(victim->res_flags, RES_CHARM))
	    skill /= 2;

	if ( number_percent() < skill || saves_spell(victim, ch, ch->level, DAM_CHARM) )
	{

	   if ( victim->master )
		  stop_follower( victim );

	   add_follower( victim, ch );

	   victim->leader = ch;
	   af.where       = TO_AFFECTS;
	   af.type        = gsn_tame;
	   af.level       = ch->level;
	   af.duration    = get_duration(gsn_tame, ch->level);
	   af.location    = 0;
	   af.modifier    = 0;
	   af.bitvector   = AFF_CHARM;
	   affect_to_char( victim, &af );

	   act("$N is ready to do your bidding.",ch, NULL, victim, TO_CHAR);
	   check_improve(ch, gsn_tame, TRUE, 3);
	}
	else
	{
	   act("$N growls at you.", ch, NULL, victim, TO_CHAR);
	   check_improve(ch, gsn_tame, FALSE, 3);
	}
	
	WAIT_STATE(ch, skill_table[gsn_tame].beats);
	return;
}

/*Some New Skills by Siva*/

/* Adapted from Rimbol's Forage code by Siva */    
void do_camp_fire(CHAR_DATA *ch)
{
	OBJ_DATA *obj, *fire;
	int skill, chance;
	
	skill = get_skill(ch, gsn_camp_fire);

	if (skill < 1)
	{
	   send_to_char("Smokey says only you can prevent forest fires.\n\r",ch);
	   return;
	}

	for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
	{
		switch( obj->pIndexData->vnum )    
		{
		case(OBJ_VNUM_FIRE):
		  send_to_char("There's already a nice fire here.\n\r",ch);
		  return;
		  break;
		case(OBJ_VNUM_BIG_FIRE):
		  send_to_char("What's wrong with the big fire that's already here?\n\r",ch);
		  return;
		  break;
		case(OBJ_VNUM_HUGE_FIRE):
		  send_to_char("Are you trying to burn down the mud?  Use the HUGE fire that's already here!\n\r",ch);
		  return;
		  break;
		default:
		  break;
		}
	
	}

	switch(ch->in_room->sector_type)
	{
	   case(SECT_WATER_SHALLOW):
	   case(SECT_WATER_DEEP):
	   case(SECT_UNDERWATER):
		  send_to_char("Can't really build a fire out of this water.\n\r",ch);
		  return;
		  break;
	   case(SECT_DESERT):
		  send_to_char("The desert yields few materials for a fire.\n\r",ch);
		  skill = skill/4;   
		  return;
		  break;
	   case(SECT_INSIDE):  
		  send_to_char("Didn't your momma tell you not to start fires indoors?\n\r",ch);
		  return;
		  break;
	   case(SECT_CITY):
		  send_to_char("There's no camping in the city limits.\n\r",ch);
		  return;
		  break;
	   case(SECT_FIELD):
	   case(SECT_HILLS):     
	   case(SECT_MOUNTAIN):    
		  send_to_char("You gather up some scattered twigs.\n\r",ch);
		  skill = skill/2;
		  break;         
	   case(SECT_FOREST):
		  send_to_char("You begin to prepare a camp site.\n\r",ch);
		  break;         /* As is */
	   default:
		  skill = 0;  /* Impossible */
		  break;
	}

	skill = UMAX(1, skill); /* Handle any negatives due to subtraction */

	chance = (number_percent ());
	
	if (chance < skill)
	{
	if ((chance + 50 ) < skill)
	{
		fire = create_object_vnum(OBJ_VNUM_HUGE_FIRE);
		fire->timer = number_range(7,12);
	}
	else if ((chance + 25) < skill)
	{
		fire = create_object_vnum(OBJ_VNUM_BIG_FIRE);
		fire->timer = number_range(10,15);
	}
	else
	{
		fire = create_object_vnum(OBJ_VNUM_FIRE); 
		fire->timer = number_range(13,18);
	}
	
	   obj_to_room( fire, ch->in_room );
	   act( "$n builds $p.", ch, fire, NULL, TO_ROOM );
	   act( "You build $p to keep you warm through the night.", 
		  ch, fire, NULL, TO_CHAR );
	   check_improve(ch, gsn_camp_fire, TRUE, 3);
	}
	else
	{
	   send_to_char("Your efforts end in failure.\n\r", ch);
	   check_improve(ch, gsn_camp_fire, FALSE, 3);
	}

	WAIT_STATE(ch,skill_table[gsn_camp_fire].beats);
	return;
}

void do_fishing(CHAR_DATA *ch)
{
	OBJ_DATA *fish;
	CHAR_DATA *mob;    
	int skill, chance, level;
	
	skill = get_skill(ch, gsn_fishing);

	if (skill < 1)
	{
	   send_to_char("You couldn't get a fish in Long John Silver's.\n\r",ch);
	   return;
	}

	if ((ch->in_room->sector_type) != (SECT_WATER_SHALLOW) && 
	    (ch->in_room->sector_type) != (SECT_WATER_DEEP) &&
	    (ch->in_room->sector_type) != (SECT_UNDERWATER))
	  {
	    send_to_char("Most fishing is done near {bwater{x.\n\r",ch);
	    return;
	  }
	
	/* Slight random chance of finding Mephfishton! */
	if ( number_percent() <= 2 )
	{
	  mob = create_mobile(get_mob_index(MOB_VNUM_MEPHFISHTON));
	  level = 30 + number_range(0, ch->level);
	  if ( !number_bits(2) )
	  { /* chance for really big nasty fish */ 
	    level += number_range(30, ch->level);
	    level = URANGE(ch->level, level, 200);
	    mob->size = UMIN(mob->size+2, SIZE_GIANT);
	    SET_BIT(mob->off_flags, OFF_HUNT);
	    SET_BIT(mob->off_flags, OFF_FAST);
	    REMOVE_BIT(mob->act, ACT_WIMPY);
	    act( "{DA large shadow shoots towards the surface!{x", ch, NULL, NULL, TO_ROOM );
	    act( "{DA large shadow shoots towards you!{x", ch, NULL, NULL, TO_CHAR );
	  }
	  set_mob_level( mob, level );
	  act( "{gUh oh! $n has caught $N, horrible fish thief of Aarchon!{x", ch, NULL, mob, TO_ROOM );
	  act( "{gUh oh, you've caught $N, horrible fish thief of Aarchon!{x", ch, NULL, mob, TO_CHAR );
	  char_to_room( mob, ch->in_room );
	  check_improve(ch, gsn_fishing, TRUE, 1);
	  return;
	}

	/* Chance is 2/3 random, 1/3 skill-dependent, with an extra dash of luck */
	chance = number_percent()*2/3 + skill/3 + get_curr_stat(ch,STAT_LUC)/15;
	/* typical:  range( 0 to 66 ) + range( 25 to 33 ) + range( 5 to 12 ) */

	if ( chance < 50 && !number_bits(2))
	{
	    fish = create_object_vnum(OBJ_VNUM_BOOT);
	    fish->level = ch->level;
	    fish->value[0] = number_range(0, get_curr_stat(ch,STAT_LUC)/6);
	    fish->value[1] = number_range(0, get_curr_stat(ch,STAT_LUC)/6);
	    fish->value[2] = number_range(0, get_curr_stat(ch,STAT_LUC)/6);
	    fish->value[3] = number_range(0, get_curr_stat(ch,STAT_LUC)/7);
	    obj_to_room( fish, ch->in_room );
	    act( "$n catches $p.", ch, fish, NULL, TO_ROOM );
	    send_to_char( "You catch .... an old, soggy boot!  And it's exactly your size!\n\r", ch );
	    check_improve(ch, gsn_fishing, FALSE, 3);
	    WAIT_STATE(ch,skill_table[gsn_fishing].beats);
	    return;
	}
	else if (chance < 50)
	{
	    send_to_char("Ohhhh, something's tugging on your line!!  ...but it got away.\n\r", ch);
	    check_improve(ch, gsn_fishing, FALSE, 3);
	    WAIT_STATE(ch,skill_table[gsn_fishing].beats);
	    return;
	}
	else if (chance < 90)
	    fish = create_object_vnum(OBJ_VNUM_FISH);
	else if (chance < 101)
	    fish = create_object_vnum(OBJ_VNUM_BIG_FISH);
	else
	    fish = create_object_vnum(OBJ_VNUM_HUGE_FISH);

	obj_to_room( fish, ch->in_room );
	act( "$n catches $p.", ch, fish, NULL, TO_ROOM );
	act( "You catch $p ... now that's good eatin!",  ch, fish, NULL, TO_CHAR );
	check_improve(ch, gsn_fishing, TRUE, 3);
	WAIT_STATE(ch,skill_table[gsn_fishing].beats);
	return;
}

/* Adapted from Rimbol's Forage code by Siva */    
void do_build_raft(CHAR_DATA *ch)
{
	OBJ_DATA *raft;
	int skill;
	
	skill = get_skill(ch, gsn_raft);

	if (skill < 1)
	{
	   send_to_char("That route ends in your drowning.\n\r",ch);
	   return;
	}

	switch(ch->in_room->sector_type)
	{
	   case(SECT_WATER_DEEP):
	   case(SECT_WATER_SHALLOW):
	   case(SECT_UNDERWATER):
		  send_to_char("It's a little late for that.\n\r",ch);
		  return;
		  break;
	   case(SECT_DESERT):
		  send_to_char("Get out of the desert.  Wet sand sinks.\n\r",ch);
		  return;
		  break;
	   case(SECT_INSIDE):  
	   case(SECT_MOUNTAIN):        
	   case(SECT_CITY):
		  send_to_char("There's nothing to build a raft from.\n\r",ch);
		  return;
		  break;
	   case(SECT_FIELD):
	   case(SECT_HILLS):     
		  send_to_char("You gather up some choice materials.\n\r",ch);
		  skill = skill/2;
		  break;         
	   case(SECT_FOREST):
		  send_to_char("You gather up some choice materials.\n\r",ch);
		  break;         /* As is */
	   default:
		  skill = 0;  /* Impossible */
		  break;
	}

	skill = UMAX(1, skill); /* Handle any negatives due to subtraction */

	if (number_percent() < skill) 
	{
	   raft = create_object_vnum(OBJ_VNUM_RAFT);
	   obj_to_room( raft, ch->in_room );
	   act( "$n builds $p from some nearby branches.", ch, raft, NULL, TO_ROOM );
	   act( "You build $p from some nearby branches.", 
		  ch, raft, NULL, TO_CHAR );
	   check_improve(ch, gsn_raft, TRUE, 2);
	}
	else
	{
	   send_to_char("You can't build a useable raft.\n\r", ch);
	   check_improve(ch, gsn_raft, FALSE, 2);
	}

	WAIT_STATE(ch,skill_table[gsn_raft].beats);
	return;
}

DEF_DO_FUN(do_taxidermy)
{
        OBJ_DATA *obj;
	int skill;

	/* find out what */
	if (argument == '\0')
	{
	send_to_char("What do you want to preserve?\n\r",ch);
	return;
	}

	obj = get_obj_list(ch,argument,ch->carrying);

	if (obj == NULL)
	{
	send_to_char("You don't have that item.\n\r",ch);
	return;
	}

	if ((skill = get_skill(ch,gsn_taxidermy)) < 1)
	{
	send_to_char("You'd better leave that to a trained taxidermist.\n\r",ch);
	return;
	}

	switch( obj->pIndexData->vnum )    
	{
	   case(OBJ_VNUM_GUTS):
	   case(OBJ_VNUM_BRAINS):
	   case(OBJ_VNUM_TORN_HEART):       
	       send_to_char("Try formaldahide.  You sure can't stuff that.\n\r",ch);
		  return;
		  break;
		case(OBJ_VNUM_CORPSE_NPC):
		case(OBJ_VNUM_CORPSE_PC):
		case(OBJ_VNUM_SEVERED_HEAD):
		case(OBJ_VNUM_SLICED_ARM):
		case(OBJ_VNUM_SLICED_LEG):
		   break;
	   default:
		  send_to_char("You can only preserve a corpse, or a part thereof.\n\r",ch);
		  return;
		  break;
	}
	
    if ( obj->timer == -1 )
    {
        act("$p is already preserved.", ch, obj, NULL, TO_CHAR);
        return;
    }

	if (number_percent() < skill)  /* success! */
	{
            char buf[MSL];
	    act("$n preserves $p for all time.",ch,obj,NULL,TO_ROOM);
	    act("You preserve $p for all time.",ch,obj,NULL,TO_CHAR);
	    obj->timer = -1;
        
        // remove ownership
        if ( obj->owner )
        {
            free_string(obj->owner);
            obj->owner = NULL;
        }
        
            if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
            {
                // add preserved keyword
                sprintf(buf, "%s preserved", obj->name);
                free_string(obj->name);
                obj->name = str_dup(buf);
            }
            else
            {
                // replace the fresh keyword on corpses
                obj->name = string_replace(obj->name, "fresh", "preserved");
            }
	    
            check_improve(ch,gsn_taxidermy,TRUE,2);
	    WAIT_STATE(ch,skill_table[gsn_taxidermy].beats);
	    return;
	}

	act("You fail to preserve $p. In fact, you botch it pretty badly.",ch,obj,NULL,TO_CHAR);
        act("$n fails to preserve $p. In fact, $e botches it pretty badly.",ch,obj,NULL,TO_ROOM);
	extract_obj( obj );
	check_improve(ch,gsn_taxidermy,FALSE,2);
	WAIT_STATE(ch,skill_table[gsn_taxidermy].beats);
	return;
}

DEF_DO_FUN(do_treat_weapon)
{
	char arg[MAX_STRING_LENGTH];           
	OBJ_DATA *obj, *herb;
	int percent,skill;
	int flag_bit;
	
	argument = one_argument(argument,arg);
	
	if (arg[0] == '\0' || argument[0] == '\0')
	{
	send_to_char("Treat what with what?\n\r",ch);
	return;
	}
	
	obj = get_obj_list(ch,arg,ch->carrying);
	herb = get_obj_list(ch,argument,ch->carrying); 
	
	if ((obj == NULL) || (herb == NULL))
	{
	send_to_char("You don't have that item.\n\r",ch);
	return;
	}

	/*
	if ((herb = get_obj_carry(ch,argument, ch)) == NULL)
	{
	send_to_char("You don't have that.\n\r",ch);
	return;
	}
	*/
	
	if ((skill = get_skill(ch,gsn_treat_weapon)) < 1)
	{
	send_to_char("Better get a skilled ranger to do this for you.\n\r",ch);
	return;
	}

	if (obj->item_type != ITEM_WEAPON) 
	{
	send_to_char("You can only treat weapons.\n\r",ch);
	return;
	}
	
	if (herb->item_type == ITEM_BLACK_HERB)
	    flag_bit = WEAPON_FROST;
	else  if (herb->item_type == ITEM_RED_HERB)     
	    flag_bit = WEAPON_FLAMING;
	else  if (herb->item_type == ITEM_MOTTLED_HERB)     
	    flag_bit = WEAPON_MOVESUCK;
	else  if (herb->item_type == ITEM_SILVER_HERB)     
	    flag_bit = WEAPON_DUMB;
	else
	{
	    act( "$p isn't a herb.", ch, herb, NULL, TO_CHAR );
	    return;
	}
	
	if (obj->value[3] < 0 
	||  attack_table[obj->value[3]].damage == DAM_BASH)
	{
		send_to_char("You can't treat this particular weapon.\n\r",ch);
		return;
	}

	if (obj->item_type == ITEM_WEAPON)
	{

	    /*
	    if (IS_WEAPON_STAT(obj,WEAPON_FLAMING)
		||  IS_WEAPON_STAT(obj,WEAPON_FROST)
		||  IS_WEAPON_STAT(obj,WEAPON_VAMPIRIC)
		||  IS_WEAPON_STAT(obj,WEAPON_SHARP)
		||  IS_WEAPON_STAT(obj,WEAPON_VORPAL)
		||  IS_WEAPON_STAT(obj,WEAPON_SHOCKING)
		||  IS_WEAPON_STAT(obj,WEAPON_MANASUCK)
		||  IS_WEAPON_STAT(obj,WEAPON_MOVESUCK)
		||  IS_WEAPON_STAT(obj,WEAPON_DUMB)
		||  IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
	    {
		act("$p is bound in a more powerful enchantment, and cannot be treated.",
		    ch,obj,NULL,TO_CHAR);
		return;
	    }
	    */
	    
	    if ( IS_WEAPON_STAT(obj, flag_bit) )
	    {
		char buf[MSL];
		sprintf( buf, "$p already has the %s effect.",
			 weapon_bit_name(flag_bit) );
		act( buf, ch, obj, NULL, TO_CHAR );
		return;
	    }

	}

	percent = number_percent();
	extract_obj(herb);   
	if ( percent < skill - flag_add_malus(obj) )
	{
	    /*
        AFFECT_DATA af;
	    af.where     = TO_WEAPON;
	    af.type      = gsn_poison;
	    af.level     = ch->level * percent / 100;
	    af.duration  = ch->level/2 * percent / 100;
	    af.location  = 0;
	    af.modifier  = 0;
	    af.bitvector = flag_bit;
	    affect_to_obj(obj,&af);
	    */
	    SET_WEAPON_STAT( obj, flag_bit );
	    
	    act("$n treats $p with $s herbs.",ch,obj,NULL,TO_ROOM);
	    act("You treat $p with your herbs.",ch,obj,NULL,TO_CHAR);
	    check_improve(ch,gsn_treat_weapon,TRUE,2);
	    WAIT_STATE(ch,skill_table[gsn_treat_weapon].beats);
	    return;
	}
	else
	{
	   act("Your treatment has somehow gone awry.",ch,obj,NULL,TO_CHAR);
	   /*
	   extract_obj( obj );
	   act("$n ruins $p with $s shoddy treatment.",ch,obj,NULL,TO_ROOM);
	   act("You ruin $p with your shoddy treatment.",ch,obj,NULL,TO_CHAR);
	   */
	   check_improve(ch,gsn_treat_weapon,FALSE,2);
	   WAIT_STATE(ch,skill_table[gsn_treat_weapon].beats);
	   return;
	}
}

DEF_DO_FUN(do_dowsing)
{
    OBJ_DATA *spring, *stick;
    int skill;

    stick = get_eq_char(ch,WEAR_HOLD);

    if ( (skill = get_skill(ch, gsn_dowsing)) == 0 )
    {
	send_to_char( "You have no clue about dowsing.\n\r", ch );
	return;
    }

    if ((stick == NULL) || (stick->item_type != ITEM_DOWSING_STICK))
    {
        send_to_char("You'll need a dowsing stick for that.\n\r",ch);
        return;
    }

    switch(ch->in_room->sector_type)
    {
    case(SECT_DESERT):
	skill -= 50;
	break;
    case(SECT_WATER_SHALLOW):
    case(SECT_WATER_DEEP):
    case(SECT_UNDERWATER):
	skill = 100;
	break;
    case(SECT_CITY):
    case(SECT_INSIDE):
    case(SECT_AIR):
	skill = 0;
	break;
    default:
	break;
    }
    
    WAIT_STATE( ch, skill_table[gsn_dowsing].beats );
    if ( !chance(skill) )
    {
	send_to_char( "You can't find any water here.\n\r", ch );
	check_improve( ch, gsn_dowsing, FALSE, 2 );
	return;
    }
    check_improve( ch, gsn_dowsing, TRUE, 2 );

    spring = create_object_vnum(OBJ_VNUM_SPRING);
    spring->timer = ch->level + 10;
    obj_to_room( spring, ch->in_room );
    act( "You sense water and draw forth a spring.", ch, spring, NULL, TO_CHAR );
    act( "$n has divined the location of a spring!", ch, spring, NULL, TO_ROOM );
}

DEF_DO_FUN(do_rustle_grub)
{
    OBJ_DATA *mushroom;
    int skill;

    if ( (skill = get_skill(ch, gsn_rustle_grub)) == 0 )
    {
	send_to_char( "You have no clue how to do that.\n\r", ch );
	return;
    }

    switch(ch->in_room->sector_type)
    {
    case(SECT_DESERT):
    case(SECT_WATER_SHALLOW):
    case(SECT_WATER_DEEP):
    case(SECT_UNDERWATER):
    case(SECT_CITY):
    case(SECT_INSIDE):
	skill -= 30;
	break;
    case(SECT_AIR):
	skill = 0;
	break;
    default:
	break;
    }

    WAIT_STATE( ch, skill_table[gsn_rustle_grub].beats );
    if ( !chance(skill) )
    {
	send_to_char( "You can't find any grub here.\n\r", ch );
	check_improve( ch, gsn_rustle_grub, FALSE, 3 );
	return;
    }
    check_improve( ch, gsn_rustle_grub, TRUE, 3 );

    mushroom = create_object_vnum(OBJ_VNUM_GRUB);
    mushroom->value[0] = ch->level / 3;
    mushroom->value[1] = ch->level / 2;
    obj_to_room( mushroom, ch->in_room );
    act( "You rustle up some grub.", ch, mushroom, NULL, TO_CHAR );
    act( "$n rustles up some grub.", ch, mushroom, NULL, TO_ROOM );
}

DEF_DO_FUN(do_fledge)
{
    OBJ_DATA *arrows;
    int skill;

    if ( (skill = get_skill(ch, gsn_fledging)) == 0 )
    {
	send_to_char( "You have no clue how to do that.\n\r", ch );
	return;
    }
    
    if ( ch->in_room->sector_type != SECT_FOREST )
    {
	send_to_char( "You can't find enough wood here.\n\r", ch );
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_fledging].beats );

    if (number_percent() > skill )
    {
        send_to_char( "You fumble the arrows to the ground, breaking them.\n\r", ch);
        return;
    }

    arrows = create_object_vnum(OBJ_VNUM_ARROWS);

    if ( arrows == NULL )
	return;
	
    arrows->value[0] = MAX_ARROWS;
    arrows->value[1] = 0; /* just to be safe */
    obj_to_room( arrows, ch->in_room );
    act( "You carve and fledge some arrows.", ch, arrows, NULL, TO_CHAR );
    act( "$n carves and fledges some arrows.", ch, arrows, NULL, TO_ROOM );
    check_improve( ch, gsn_fledging, TRUE, 3 );
}

