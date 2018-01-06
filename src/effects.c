/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
 
/***************************************************************************
*   ROM 2.4 is copyright 1993-1996 Russ Taylor             *
*   ROM has been brought to you by the ROM consortium          *
*       Russ Taylor (rtaylor@efn.org)                  *
*       Gabrielle Taylor                           *
*       Brian Moore (zump@rom.org)                     *
*   By using this code, you have agreed to follow the terms of the     *
*   ROM license, in the file Rom24/doc/rom.license             *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"

void acid_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM) /* nail objects on the floor */
	 {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		acid_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
	}

	if (target == TARGET_CHAR)  /* do the effect on a victim */
	{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;
	
	/* let's toast some gear */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		acid_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
	}

	if (target == TARGET_OBJ) /* toast an object */
	{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	OBJ_DATA *t_obj,*n_obj;
	int chance;
	char *msg;

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
	||  IS_OBJ_STAT(obj,ITEM_NOPURGE)
	||  number_range(0,4) == 0)
		return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	 if (chance > 50)
		chance = (chance - 50) / 2 + 50;

	if (IS_OBJ_STAT(obj,ITEM_BLESS))
		chance -= 5;

	chance -= obj->level * 2;

	switch (obj->item_type)
	{
		default:
		return;
		case ITEM_CONTAINER:
		case ITEM_CORPSE_PC:
		case ITEM_CORPSE_NPC:
		msg = "$p fumes and dissolves.";
		break;
		case ITEM_ARMOR:
		msg = "$p is pitted and etched.";
		break;
		case ITEM_CLOTHING:
		msg = "$p is corroded into scrap.";
		break;
		case ITEM_STAFF:
		case ITEM_WAND:
		chance -= 10;
		msg = "$p corrodes and breaks.";
		break;
		case ITEM_SCROLL:
		chance += 10;
		msg = "$p is burned into waste.";
		break; 
	}

	chance = URANGE(5,chance,95);

	if (number_percent() > chance)
		return;

	if (obj->carried_by != NULL)
		act_gag(msg, obj->carried_by, obj, NULL, TO_ALL, GAG_EFFECT);
	else if (obj->in_room != NULL && obj->in_room->people != NULL)
		act_gag(msg, obj->in_room->people, obj, NULL, TO_ALL, GAG_EFFECT);

	if (obj->item_type == ITEM_ARMOR)  /* etch it */
	{
        AFFECT_DATA af  = {};
        af.where        = TO_OBJECT;
        af.type         = gsn_acid_blast;
        af.level        = level;
        af.duration     = get_duration_by_type(DUR_LONG, level);
        af.location     = APPLY_AC;
        af.modifier     = dice(2,4);
        
        add_enchant_affect(obj, &af);
 
			if (obj->carried_by != NULL && obj->wear_loc != WEAR_NONE)
			    obj->carried_by->armor += 1;
			return;
	}

	/* get rid of the object */
	if (obj->contains)  /* dump contents */
	{
		for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
		{
		n_obj = t_obj->next_content;
		obj_from_obj(t_obj);
		if (obj->in_room != NULL)
			obj_to_room(t_obj,obj->in_room);
		else if (obj->carried_by != NULL)
			obj_to_char(t_obj,obj->carried_by);
		else
		{
			extract_obj(t_obj);
			continue;
		}

		acid_effect(t_obj,level/2,dam/2,TARGET_OBJ);
		}
	}

	extract_obj(obj);
	return;
	}
}

void cold_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM) /* nail objects on the floor */
	{
		ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
		OBJ_DATA *obj, *obj_next;
 
		for (obj = room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			cold_effect(obj,level,dam,TARGET_OBJ);
		}
		return;
	}

	if (target == TARGET_CHAR) /* whack a character */
	{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;
	
	/* chill touch effect */
	if (!saves_spell(victim, NULL, level/4 + dam / 20, DAM_COLD))
	{
		AFFECT_DATA af;

		act_gag("$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
		act_gag("A chill sinks deep into your bones.", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
		af.where     = TO_AFFECTS;
		af.type      = skill_lookup("chill touch");
		af.level     = level;
		af.duration  = 6;
		af.location  = APPLY_STR;
		af.modifier  = -10;
		af.bitvector = 0;
		affect_join( victim, &af );
	}

	/* hunger! (warmth sucked out */
	if (!IS_NPC(victim))
    {
        int hunger = rand_div(dam, 20 + victim->level);
        gain_condition(victim, COND_HUNGER, -hunger);
        gain_condition(victim, COND_FULL, -hunger);
    }

	/* let's toast some gear */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		cold_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
   }

   if (target == TARGET_OBJ) /* toast an object */
   {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;
	char *msg;

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
	||  IS_OBJ_STAT(obj,ITEM_NOPURGE)
	||  number_range(0,4) == 0)
		return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	if (chance > 50)
		chance = (chance - 50) / 2 + 50;

	if (IS_OBJ_STAT(obj,ITEM_BLESS))
		chance -= 5;

	chance -= obj->level * 2;

	switch(obj->item_type)
	{
		default:
		return;
		case ITEM_POTION:
		msg = "$p freezes and shatters!";
		chance += 25;
		break;
		case ITEM_DRINK_CON:
		msg = "$p freezes and shatters!";
		chance += 5;
		break;
	}

	chance = URANGE(5,chance,95);

	if (number_percent() > chance)
		return;

	if (obj->carried_by != NULL)
		act_gag(msg, obj->carried_by, obj, NULL, TO_ALL, GAG_EFFECT);
	else if (obj->in_room != NULL && obj->in_room->people != NULL)
		act_gag(msg, obj->in_room->people, obj, NULL, TO_ALL, GAG_EFFECT);

	extract_obj(obj);
	return;
	}
}

void fire_effect(void *vo, int level, int dam, int target)
{
	if (target == TARGET_ROOM)  /* nail objects on the floor */
	{
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		fire_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
	}
 
	if (target == TARGET_CHAR)   /* do the effect on a victim */
	{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	/* chance of blindness */
	if (!IS_AFFECTED(victim,AFF_BLIND)
	&&  !saves_spell(victim, NULL, level / 4 + dam / 20, DAM_FIRE)
	&&  !number_bits(2))
	{
	    AFFECT_DATA af;
	    act_gag("$n is blinded by smoke!", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
	    act_gag("Your eyes tear up from smoke...you can't see a thing!",
		victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
	    
	    af.where        = TO_AFFECTS;
	    af.type         = gsn_fire_breath;
	    af.level        = level;
	    af.duration     = 0;
	    af.location     = APPLY_HITROLL;
	    af.modifier     = -4;
	    af.bitvector    = AFF_BLIND;
	    
	    affect_to_char(victim,&af);
	}

	/* getting thirsty */
	if (!IS_NPC(victim))
    {
        int thirst = rand_div(dam, 20 + victim->level);
        gain_condition(victim, COND_THIRST, -thirst);
        gain_condition(victim, COND_FULL, -thirst);
    }

	/* let's toast some gear! */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;

		fire_effect(obj,level,dam,TARGET_OBJ);
		}
	return;
	}

	if (target == TARGET_OBJ)  /* toast an object */
	{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	OBJ_DATA *t_obj,*n_obj;
	int chance;
	char *msg;

		if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
		||  IS_OBJ_STAT(obj,ITEM_NOPURGE)
	||  number_range(0,4) == 0)
			return;
 
		chance = level / 4 + dam / 10;
 
		if (chance > 25)
			chance = (chance - 25) / 2 + 25;
		if (chance > 50)
			chance = (chance - 50) / 2 + 50;

		if (IS_OBJ_STAT(obj,ITEM_BLESS))
			chance -= 5;
		chance -= obj->level * 2;

		switch ( obj->item_type )
		{
		default:             
		return;
		case ITEM_EXPLOSIVE:
			chance += 30;
			msg = "$p also explodes from the heat!";
			if (obj != vo)
			   explode(obj);
			break;
		case ITEM_CONTAINER:
			msg = "$p ignites and burns!";
			break;
		case ITEM_POTION:
			chance += 25;
			msg = "$p bubbles and boils!";
			break;
		case ITEM_SCROLL:
			chance += 50;
			msg = "$p crackles and burns!";
			break;
		case ITEM_STAFF:
			chance += 10;
			msg = "$p smokes and chars!";
			break;
		case ITEM_WAND:
			msg = "$p sparks and sputters!";
			break;
		case ITEM_FOOD:
			msg = "$p blackens and crisps!";
			break;
		case ITEM_PILL:
			msg = "$p melts and drips!";
			break;
		}

		chance = URANGE(5,chance,95);

		if (number_percent() > chance)
			return;
 
	if (obj->carried_by != NULL)
		act_gag(msg, obj->carried_by, obj, NULL, TO_ALL, GAG_EFFECT);
    else if (obj->in_room != NULL && obj->in_room->people != NULL)
        act_gag(msg, obj->in_room->people, obj, NULL, TO_ALL, GAG_EFFECT);

    if (obj->contains)
    {
        /* dump the contents */

        for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
        {
            n_obj = t_obj->next_content;
            obj_from_obj(t_obj);
            if (obj->in_room != NULL)
                obj_to_room(t_obj,obj->in_room);
            else if (obj->carried_by != NULL)
                obj_to_char(t_obj,obj->carried_by);
            else
            {
                extract_obj(t_obj);
                continue;
            }
            fire_effect(t_obj,level/2,dam/2,TARGET_OBJ);
        }
    }

    extract_obj( obj );
    return;
	}
}

void poison_effect(void *vo,int level, int dam, int target)
{
	if (target == TARGET_ROOM)  /* nail objects on the floor */
	{
		ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
		OBJ_DATA *obj, *obj_next;
 
		for (obj = room->contents; obj != NULL; obj = obj_next)
		{
			obj_next = obj->next_content;
			poison_effect(obj,level,dam,TARGET_OBJ);
		}
		return;
	}
 
	if (target == TARGET_CHAR)   /* do the effect on a victim */
	{
	    CHAR_DATA *victim = (CHAR_DATA *) vo;
	    OBJ_DATA *obj, *obj_next;
	    
	    /* chance of poisoning */
	    if (!number_bits(1) && !saves_spell(victim, NULL, level / 4 + dam / 20, DAM_POISON))
	    {
		AFFECT_DATA af;
		
		act_gag("You feel poison coursing through your veins.", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
		act_gag("$n looks very ill.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
		
		af.where     = TO_AFFECTS;
		af.type      = gsn_poison;
		af.level     = level;
		af.duration  = 10;
		af.location  = APPLY_STR;
		af.modifier  = -1;
		af.bitvector = AFF_POISON;
		affect_join( victim, &af );
	    }

	/* equipment */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		poison_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
	}

	if (target == TARGET_OBJ)  /* do some poisoning */
	{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;
	

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
	||  IS_OBJ_STAT(obj,ITEM_BLESS)
	||  number_range(0,4) == 0)
		return;

	chance = level / 4 + dam / 10;
	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	if (chance > 50)
		chance = (chance - 50) / 2 + 50;

	chance -= obj->level * 2;

	switch (obj->item_type)
	{
		default:
		return;
		case ITEM_FOOD:
		break;
		case ITEM_DRINK_CON:
		if (obj->value[0] == obj->value[1])
			return;
		break;
	}

	chance = URANGE(5,chance,95);

	if (number_percent() > chance)
		return;

	obj->value[3] = 1;
	return;
	}
}


void shock_effect(void *vo,int level, int dam, int target)
{
	if (target == TARGET_ROOM)
	{
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		shock_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
	}

	if (target == TARGET_CHAR)
	{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	/* daze and confused? */
	if (!saves_spell(victim, NULL, level/4 + dam/20, DAM_LIGHTNING))
	{
	    act_gag("Your muscles stop responding.", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
	    act_gag("$n's muscles stop responding.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
	    DAZE_STATE(victim, PULSE_VIOLENCE);
	}

	/* toast some gear */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
		obj_next = obj->next_content;
		shock_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
	}

	if (target == TARGET_OBJ)
	{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;
	char *msg;

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
	||  IS_OBJ_STAT(obj,ITEM_NOPURGE)
	||  number_range(0,4) == 0)
		return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	if (chance > 50)
		chance = (chance - 50) /2 + 50;

	if (IS_OBJ_STAT(obj,ITEM_BLESS))
		chance -= 5;

	chance -= obj->level * 2;

	switch(obj->item_type)
	{
		default:
		return;
	   case ITEM_WAND:
	   case ITEM_STAFF:
		chance += 10;
		msg = "$p overloads and explodes!";
		break;
	   case ITEM_JEWELRY:
		chance -= 10;
		msg = "$p is fused into a worthless lump.";
	}
	
	chance = URANGE(5,chance,95);

	if (number_percent() > chance)
		return;

	if (obj->carried_by != NULL)
		act_gag(msg, obj->carried_by, obj, NULL, TO_ALL, GAG_EFFECT);
	else if (obj->in_room != NULL && obj->in_room->people != NULL)
		act_gag(msg, obj->in_room->people, obj, NULL, TO_ALL, GAG_EFFECT);

	extract_obj(obj);
	return;
	}
}

void dumb_effect(void *vo, int level, int dam, int target)
{
    if (target == TARGET_CHAR) /* make a character dumb */
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	/* feeblemind effect */
	if (!saves_spell(victim, NULL, level/4 + dam / 20, DAM_SOUND))
	{
	    AFFECT_DATA af;
	    
	    act_gag("$n is having trouble thinking.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
	    act_gag("UuuhNnNNhhh. You're losing what's left of your feeble mind!", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
	    af.where     = TO_AFFECTS;
	    af.type      = skill_lookup("feeblemind");
	    af.level     = level;
	    af.duration  = 6;
	    af.location  = APPLY_INT;
	    af.modifier  = -10;
	    af.bitvector = 0;
	    affect_join( victim, &af );
	}
	return;
   }

}


void paralysis_effect(void *vo,int level, int dam, int target)
{
    if (target == TARGET_CHAR)   /* do the effect on a victim */
    {
        CHAR_DATA *victim = (CHAR_DATA *) vo;

        /* chance of poisoning */
        if (!number_bits(1) && !saves_spell(victim, NULL, level / 4 + dam / 20, DAM_POISON))
        {
            AFFECT_DATA af;

            act_gag("A paralysis poison makes your limbs feel heavy and weak.", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
            act_gag("$n is consumed by a paralysis poison.", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);

            af.where     = TO_AFFECTS;
            af.type      = gsn_paralysis_poison;
            af.level     = level;
            af.duration  = level/35;
            af.location  = APPLY_AGI;
            af.modifier  = -1 * (level/12);
            af.bitvector = AFF_PARALYSIS;
            affect_join( victim, &af );
        }

        return;
    }
}
