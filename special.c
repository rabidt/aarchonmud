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
#include "magic.h"
#include "buffer_util.h"
#include "religion.h"
#include "special.h"

/* command procedures needed */
DECLARE_DO_FUN(do_yell      );
DECLARE_DO_FUN(do_open      );
DECLARE_DO_FUN(do_close     );
DECLARE_DO_FUN(do_say   );
DECLARE_DO_FUN(do_backstab);
DECLARE_DO_FUN(do_flee);
DECLARE_DO_FUN(do_murder);
DECLARE_DO_FUN(do_cast);
DECLARE_DO_FUN(do_religion_talk);


/* the function table */
const   struct  spec_type    spec_table[] =
{
    {   "spec_breath_any",      spec_breath_any,    FALSE   },
    {   "spec_breath_acid",     spec_breath_acid,   FALSE   },
    {   "spec_breath_fire",     spec_breath_fire,   FALSE   },
    {   "spec_breath_frost",    spec_breath_frost,  FALSE   },
    {   "spec_breath_gas",      spec_breath_gas,    FALSE   },
    {   "spec_breath_lightning",spec_breath_lightning,FALSE },  
    {   "spec_cast_adept",      spec_cast_adept,    FALSE   },
    {   "spec_cast_cleric",     spec_cast_cleric,   FALSE   },
    {   "spec_cast_judge",      spec_cast_judge,    FALSE   },
    {   "spec_cast_mage",       spec_cast_mage,     FALSE   },
    {   "spec_cast_draconic",   spec_cast_draconic, TRUE    },
    {   "spec_cast_undead",     spec_cast_undead,   FALSE   },
    {   "spec_executioner",     spec_executioner,   FALSE   },
    {   "spec_fido",            spec_fido,          FALSE   },
    {   "spec_guard",           spec_guard,         FALSE   },
    {   "spec_janitor",         spec_janitor,       FALSE   },
    {   "spec_mayor",           spec_mayor,         FALSE   },
    {   "spec_poison",          spec_poison,        FALSE   },
    {   "spec_thief",           spec_thief,         FALSE   },
    {   "spec_nasty",           spec_nasty,         FALSE   },
    {   "spec_troll_member",    spec_troll_member,  FALSE   },
    {   "spec_ogre_member",     spec_ogre_member,   FALSE   },
    {   "spec_patrolman",       spec_patrolman,     FALSE   },
    {   "spec_questmaster",     spec_questmaster,   FALSE   }, /* Vassago */ 
    {   "spec_bounty_hunter",   spec_bounty_hunter, FALSE   },
    {   "spec_remort",          spec_remort,        FALSE   },
    {   "spec_temple_guard",    spec_temple_guard,  TRUE    },
    {   NULL,                   NULL,               FALSE   }
};
/*
struct spell_type
{
    char    *spell;
    sh_int  min_level;
    sh_int  max_level;
};*/

#define NO_MAX 200
const struct spell_type spell_list_cleric[] =
{
    { "dispel magic", 40, NO_MAX },
    { "blindness", 20, NO_MAX },
    { "curse", 1, NO_MAX },
    { "plague", 1, 120 },
    { "poison", 1, 120 },
    { "slow", 1, NO_MAX },
    { "weaken", 1, NO_MAX },
    { "flamestrike", 40, NO_MAX },
    { "harm", 40, NO_MAX },
    { "heal", 40, 120 },
    { "cure mortal", 80, NO_MAX },
    { "heat metal", 100, NO_MAX },
    { "restoration", 120, NO_MAX },
    { NULL, 0, 0 }
};

const struct spell_type spell_list_mage[] = {
    { "dispel magic", 20, NO_MAX },
    { "magic missile", 1, 40 },
    { "chill touch", 10, 80 },
    { "burning hands", 10, 40 },
    { "colour spray", 1, 40 },
    { "fireball", 20, 120 },
    { "lightning bolt", 20, 120 },
    { "acid blast", 40, NO_MAX },
    { "energy drain", 40, NO_MAX },
    { "heat metal", 80, NO_MAX },
    { "stop", 80, NO_MAX },
    { NULL, 0, 0 }
};

const struct spell_type spell_list_draconic[] = {
    { "acid breath", 1, NO_MAX },
    { "lightning breath", 1, NO_MAX },
    { "fire breath", 1, NO_MAX },
    { "frost breath", 1, NO_MAX },
    { "gas breath", 1, NO_MAX },
    { NULL, 0, 0 }
};

const struct spell_type spell_list_undead[] = {
    { "curse", 1, NO_MAX },
    { "weaken", 1, NO_MAX },
    { "chill touch", 10, 80 },
    { "blindness", 20, NO_MAX },
    { "poison", 1, 120 },
    { "harm", 40, NO_MAX },
    { "energy drain", 20, NO_MAX },
    { "plague", 1, 120 },
    { "necrosis", 40, NO_MAX },
    { "tomb rot", 80, NO_MAX },
    { "soreness", 40, NO_MAX },
    { "haunt", 40, NO_MAX },
    { "mana burn", 80, NO_MAX },
    { "fear", 20, NO_MAX },
    { NULL, 0, 0 }
};

/*
 * Given a name, return the appropriate spec fun.
 */
SPEC_FUN *spec_lookup( const char *name )
{
   int i;
 
   for ( i = 0; spec_table[i].name != NULL; i++)
   {
		if (LOWER(name[0]) == LOWER(spec_table[i].name[0])
		&&  !str_prefix( name,spec_table[i].name))
			return spec_table[i].function;
   }
 
	return 0;
}

const char* spec_name_lookup( SPEC_FUN *function )
{
	int i;

	for (i = 0; spec_table[i].function != NULL; i++)
	{
	if (function == spec_table[i].function)
		return spec_table[i].name;
	}

	return NULL;
}

bool is_wait_based( SPEC_FUN *function )
{
    int i;

    for ( i = 0; spec_table[i].function != NULL; i++ )
    {
        if ( function == spec_table[i].function )
            return spec_table[i].wait_based;
    }
    
    return FALSE;
}

bool spec_troll_member( CHAR_DATA *ch)
{
	CHAR_DATA *vch, *victim = NULL;
	int count = 0;
	char *message;

	if (!IS_AWAKE(ch) || IS_AFFECTED(ch,AFF_CALM) || ch->in_room == NULL 
	||  IS_AFFECTED(ch,AFF_CHARM) || ch->fighting != NULL)
	return FALSE;

	/* find an ogre to beat up */
	for (vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room)
	{
	if (!IS_NPC(vch) || ch == vch)
		continue;

	if (vch->pIndexData->vnum == MOB_VNUM_PATROLMAN)
		return FALSE;

	if (vch->pIndexData->group == GROUP_VNUM_OGRES
	&&  ch->level > vch->level - 2 && !is_safe(ch,vch))
	{
		if (number_range(0,count) == 0)
		victim = vch;

		count++;
	}
	}

	if (victim == NULL)
	return FALSE;

	/* say something, then raise hell */
	switch (number_range(0,6))
	{
	default:  message = NULL;   break;
	case 0: message = "$n yells 'I've been looking for you, punk!'";
		break;
	case 1: message = "With a scream of rage, $n attacks $N.";
		break;
	case 2: message = 
		"$n says 'What's slimy Ogre trash like you doing around here?'";
		break;
	case 3: message = "$n cracks his knuckles and says 'Do ya feel lucky?'";
		break;
	case 4: message = "$n says 'There's no cops to save you this time!'";
		break;  
	case 5: message = "$n says 'Time to join your brother, spud.'";
		break;
	case 6: message = "$n says 'Let's rock.'";
		break;
	}

	if (message != NULL)
		act(message,ch,NULL,victim,TO_ALL);
	multi_hit( ch, victim, TYPE_UNDEFINED );
	return TRUE;
}

bool spec_ogre_member( CHAR_DATA *ch)
{
	CHAR_DATA *vch, *victim = NULL;
	int count = 0;
	char *message;
 
	if (!IS_AWAKE(ch) || IS_AFFECTED(ch,AFF_CALM) || ch->in_room == NULL
	||  IS_AFFECTED(ch,AFF_CHARM) || ch->fighting != NULL)
		return FALSE;

	/* find an troll to beat up */
	for (vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room)
	{
		if (!IS_NPC(vch) || ch == vch)
			continue;
 
		if (vch->pIndexData->vnum == MOB_VNUM_PATROLMAN)
			return FALSE;
 
		if (vch->pIndexData->group == GROUP_VNUM_TROLLS
		&&  ch->level > vch->level - 2 && !is_safe(ch,vch))
		{
			if (number_range(0,count) == 0)
				victim = vch;
 
			count++;
		}
	}
 
	if (victim == NULL)
		return FALSE;
 
	/* say something, then raise hell */
	switch (number_range(0,6))
	{
	default: message = NULL;    break;
		case 0: message = "$n yells 'I've been looking for you, punk!'";
				break;
		case 1: message = "With a scream of rage, $n attacks $N.'";
				break;
		case 2: message =
				"$n says 'What's Troll filth like you doing around here?'";
				break;
		case 3: message = "$n cracks his knuckles and says 'Do ya feel lucky?'";
				break;
		case 4: message = "$n says 'There's no cops to save you this time!'";
				break;
		case 5: message = "$n says 'Time to join your brother, spud.'";
				break;
		case 6: message = "$n says 'Let's rock.'";
				break;
	}
 
	if (message != NULL)
		act(message,ch,NULL,victim,TO_ALL);
	multi_hit( ch, victim, TYPE_UNDEFINED );
	return TRUE;
}

bool spec_patrolman(CHAR_DATA *ch)
{
	CHAR_DATA *vch,*victim = NULL;
	OBJ_DATA *obj;
	char *message;
	int count = 0;

	if (!IS_AWAKE(ch) || IS_AFFECTED(ch,AFF_CALM) || ch->in_room == NULL
	||  IS_AFFECTED(ch,AFF_CHARM) || ch->fighting != NULL)
		return FALSE;

	/* look for a fight in the room */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
	if (vch == ch)
		continue;

	if (vch->fighting != NULL)  /* break it up! */
	{
		if (number_range(0,count) == 0)
			victim = (vch->level > vch->fighting->level) 
			? vch : vch->fighting;
		count++;
	}
	}

	if (victim == NULL || (IS_NPC(victim) && victim->spec_fun == ch->spec_fun))
	return FALSE;

	if (((obj = get_eq_char(ch,WEAR_NECK_1)) != NULL 
	&&   obj->pIndexData->vnum == OBJ_VNUM_WHISTLE)
	||  ((obj = get_eq_char(ch,WEAR_NECK_2)) != NULL
	&&   obj->pIndexData->vnum == OBJ_VNUM_WHISTLE))
	{
	act("You blow down hard on $p.",ch,obj,NULL,TO_CHAR);
	act("$n blows on $p, ***WHEEEEEEEEEEEET***",ch,obj,NULL,TO_ROOM);

		for ( vch = char_list; vch != NULL; vch = vch->next )
		{
			if ( vch->in_room == NULL )
				continue;

			if (vch->in_room != ch->in_room 
		&&  vch->in_room->area == ch->in_room->area)
				send_to_char( "You hear a shrill whistling sound.\n\r", vch );
		}
	}

	switch (number_range(0,6))
	{
	default:    message = NULL;     break;
	case 0: message = "$n yells 'All roit! All roit! break it up!'";
		break;
	case 1: message = 
		"$n says 'Society's to blame, but what's a bloke to do?'";
		break;
	case 2: message = 
		"$n mumbles 'bloody kids will be the death of us all.'";
		break;
	case 3: message = "$n shouts 'Stop that! Stop that!' and attacks.";
		break;
	case 4: message = "$n pulls out his billy and goes to work.";
		break;
	case 5: message = 
		"$n sighs in resignation and proceeds to break up the fight.";
		break;
	case 6: message = "$n says 'Settle down, you hooligans!'";
		break;
	}

	if (message != NULL)
	act(message,ch,NULL,NULL,TO_ALL);

	multi_hit(ch,victim,TYPE_UNDEFINED);

	return TRUE;
}
	

bool spec_nasty( CHAR_DATA *ch )
{
	CHAR_DATA *victim, *v_next;
	long gold;
 
	if (!IS_AWAKE(ch)) {
	   return FALSE;
	}
 
	if (ch->position != POS_FIGHTING) {
	   for ( victim = ch->in_room->people; victim != NULL; victim = v_next)
	   {
		  v_next = victim->next_in_room;
		  if ( !IS_NPC(victim)
		       && (victim->level > ch->level)
		       && (victim->level < ch->level + 10) )
		  {
		      do_backstab(ch,victim->name);
		      if (ch->position != POS_FIGHTING)
			  do_murder(ch,victim->name);
		      /* should steal some coins right away? :) */
		      return TRUE;
		  }
	   }
	   return FALSE;    /*  No one to attack */
	}
 
	/* okay, we must be fighting.... steal some coins and flee */
	if ( (victim = ch->fighting) == NULL)
	    return FALSE;   /* let's be paranoid.... */
 
	switch ( number_bits(2) )
	{
	case 0:  act( "$n rips apart your coin purse, spilling your gold!",
		      ch, NULL, victim, TO_VICT);
	act( "You slash apart $N's coin purse and gather his gold.",
	     ch, NULL, victim, TO_CHAR);
	act( "$N's coin purse is ripped apart!",
	     ch, NULL, victim, TO_NOTVICT);
	gold = victim->gold / 10;  /* steal 10% of his gold */
	victim->gold -= gold;
	ch->gold     += gold;
	return TRUE;
 
	case 1:  do_flee( ch, "");
	    return TRUE;
 
	default: return FALSE;
	}
}

/*
 * Core procedure for dragons.
 */
bool dragon( CHAR_DATA *ch, char *spell_name )
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int sn;

	if ( ch->position != POS_FIGHTING )
	return FALSE;

	for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
	{
	v_next = victim->next_in_room;
	if ( victim->fighting == ch && number_bits( 3 ) == 0 )
		break;
	}

	if ( victim == NULL )
	return FALSE;

	if ( ( sn = skill_lookup( spell_name ) ) < 0 )
	return FALSE;
	(*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim, TARGET_CHAR, FALSE );
	return TRUE;
}



/*
 * Special procedures for mobiles.
 */
bool spec_breath_any( CHAR_DATA *ch )
{
	if ( ch->position != POS_FIGHTING )
	return FALSE;

	switch ( number_bits( 3 ) )
	{
	case 0: return spec_breath_fire     ( ch );
	case 1:
	case 2: return spec_breath_lightning    ( ch );
	case 3: return spec_breath_gas      ( ch );
	case 4: return spec_breath_acid     ( ch );
	case 5:
	case 6:
	case 7: return spec_breath_frost        ( ch );
	}

	return FALSE;
}



bool spec_breath_acid( CHAR_DATA *ch )
{
	return dragon( ch, "acid breath" );
}



bool spec_breath_fire( CHAR_DATA *ch )
{
	return dragon( ch, "fire breath" );
}



bool spec_breath_frost( CHAR_DATA *ch )
{
	return dragon( ch, "frost breath" );
}



bool spec_breath_gas( CHAR_DATA *ch )
{
	int sn;

	if ( ch->position != POS_FIGHTING )
	return FALSE;

	if ( ( sn = skill_lookup( "gas breath" ) ) < 0 )
	return FALSE;
	(*skill_table[sn].spell_fun) ( sn, ch->level, ch, NULL, TARGET_CHAR, FALSE );
	return TRUE;
}



bool spec_breath_lightning( CHAR_DATA *ch )
{
	return dragon( ch, "lightning breath" );
}



bool spec_cast_adept( CHAR_DATA *ch )
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;

	if ( !IS_AWAKE(ch) )
	return FALSE;

	for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
	{
	v_next = victim->next_in_room;
	if ( victim != ch && can_see( ch, victim ) && number_bits( 1 ) == 0 
		 && !IS_NPC(victim) && victim->level < 11)
		break;
	}

	if ( victim == NULL )
	return FALSE;

	switch ( number_bits( 4 ) )
	{
	case 0:
	act( "$n utters the word 'abrazak'.", ch, NULL, NULL, TO_ROOM );
	spell_armor( skill_lookup( "armor" ), ch->level, ch, victim, TARGET_CHAR, FALSE );
	return TRUE;

	case 1:
	act( "$n utters the word 'fido'.", ch, NULL, NULL, TO_ROOM );
	spell_bless( skill_lookup( "bless" ), ch->level, ch, victim, TARGET_CHAR, FALSE );
	return TRUE;

	case 2:
	act("$n utters the words 'judicandus noselacri'.",ch,NULL,NULL,TO_ROOM);
	spell_cure_blindness( skill_lookup( "cure blindness" ), ch->level, ch, victim, TARGET_CHAR, FALSE );
	return TRUE;

	case 3:
	act("$n utters the words 'judicandus dies'.", ch,NULL, NULL, TO_ROOM );
	spell_cure_light( skill_lookup( "cure light" ), ch->level, ch, victim, TARGET_CHAR, FALSE);
	return TRUE;

	case 4:
	act( "$n utters the words 'judicandus sausabru'.",ch,NULL,NULL,TO_ROOM);
	spell_cure_poison( skill_lookup( "cure poison" ), ch->level, ch, victim, TARGET_CHAR, FALSE );
	return TRUE;

	case 5:
	act("$n utters the word 'candusima'.", ch, NULL, NULL, TO_ROOM );
	spell_refresh( skill_lookup("refresh"), ch->level, ch, victim, TARGET_CHAR, FALSE );
	return TRUE;

	case 6:
	act("$n utters the words 'judicandus eugzagz'.",ch,NULL,NULL,TO_ROOM);
	spell_cure_disease(skill_lookup("cure disease"), ch->level, ch, victim, TARGET_CHAR, FALSE );
	}

	return FALSE;
}

const struct spell_type* get_spell_list( CHAR_DATA *ch )
{
    SPEC_FUN *spec_fun = ch->pIndexData->spec_fun;
    
    if ( spec_fun == spec_cast_cleric )
        return spell_list_cleric;
    if ( spec_fun == spec_cast_mage )
        return spell_list_mage;
    if ( spec_fun == spec_cast_draconic )
        return spell_list_draconic;
    if ( spec_fun == spec_cast_undead )
        return spell_list_undead;
    
    return NULL;
}

bool spec_cast_any( CHAR_DATA *ch )
{
    if ( ch->position != POS_FIGHTING )
        return FALSE;

    const struct spell_type* spell_list = get_spell_list( ch );
    if ( spell_list == NULL )
        return FALSE;

    // select a random spell to cast
    int spell_count = 0;
    int spell_selected = 0;
    int spell_index = 0;
    
    while ( spell_list[spell_index].spell )
    {
        if ( spell_list[spell_index].min_level <= ch->level && ch->level <= spell_list[spell_index].max_level )
        {
            // uniform distribution with one parse of the list :)
            if ( !number_range(0, spell_count++) )
                spell_selected = spell_index;
        }
        spell_index++;
    }
    
    char argument[255];
    sprintf( argument, "'%s'", spell_list[spell_selected].spell );
    do_cast( ch, argument );
    return TRUE;
}

bool spec_cast_cleric( CHAR_DATA *ch )
{
    return spec_cast_any( ch );
}

bool spec_cast_judge( CHAR_DATA *ch )
{
    return spec_cast_any( ch );
}

bool spec_cast_mage( CHAR_DATA *ch )
{
    return spec_cast_any( ch );
}

bool spec_cast_draconic( CHAR_DATA *ch )
{
    if ( ch->wait > 0 )
        return FALSE;
    return spec_cast_any( ch );
}

bool spec_cast_undead( CHAR_DATA *ch )
{
    return spec_cast_any( ch );
}

bool spec_executioner( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *crime;

    if ( !IS_AWAKE(ch) || ch->fighting != NULL )
	return FALSE;

    if ( !IS_NPC(ch))
        return FALSE;

    /*
    if ( IS_SET(ch->in_room->room_flags, ROOM_SAFE) )
	return FALSE;
    */

	crime = "";
	for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
	{
	    v_next = victim->next_in_room;

	    if ( !is_mimic(victim) )
	    {
		if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER) 
		     && !is_safe(ch, victim)
		     && check_see(ch,victim))
		    { crime = "KILLER"; break; }
		
		if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF) 
		     && !is_safe(ch, victim)
		     && check_see(ch,victim))
		    { crime = "THIEF"; break; }
	    }
	}

	if ( victim == NULL )
	    return FALSE;

	sprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!",
	victim->name, crime );
	REMOVE_BIT(ch->penalty, PENALTY_NOSHOUT);
	do_yell( ch, buf );
	multi_hit( ch, victim, TYPE_UNDEFINED );
	return TRUE;
}

			

bool spec_fido( CHAR_DATA *ch )
{
	OBJ_DATA *corpse;
	OBJ_DATA *c_next;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if ( !IS_AWAKE(ch) )
	return FALSE;

	for ( corpse = ch->in_room->contents; corpse != NULL; corpse = c_next )
	{
	c_next = corpse->next_content;
	if ( corpse->item_type != ITEM_CORPSE_NPC )
		continue;

	act( "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );
	for ( obj = corpse->contains; obj; obj = obj_next )
	{
		obj_next = obj->next_content;
		obj_from_obj( obj );
		obj_to_room( obj, ch->in_room );
	}
	extract_obj( corpse );
	return TRUE;
	}

	return FALSE;
}



bool spec_guard( CHAR_DATA *ch )
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	CHAR_DATA *ech;
	char *crime;
	int max_evil;

	/*
	if ( IS_SET(ch->in_room->room_flags, ROOM_SAFE) )
	    return FALSE;
	*/

	if ( !IS_AWAKE(ch) || ch->fighting != NULL )
	    return FALSE;

	max_evil = 300;
	ech      = NULL;
	crime    = "";

	for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
	{
	    v_next = victim->next_in_room;

	    if ( !is_mimic(victim) )
	    {
		if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER) 
		     && !is_safe(ch, victim)
		     && check_see(ch,victim))
		    { crime = "KILLER"; break; }
		
		if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF) 
		     && !is_safe(ch, victim)
		     && check_see(ch,victim))
		    { crime = "THIEF"; break; }
	    }		

        if ( victim->fighting != NULL
            && !is_allied(ch, victim)
            && IS_SET(victim->fighting->form, FORM_SENTIENT)
            && !IS_UNDEAD(victim->fighting)
            && victim->alignment < max_evil
            && check_see(ch,victim) )
		{
		    max_evil = victim->alignment;
		    ech      = victim;
		}
	}
	
	if ( victim != NULL )
	    {
		sprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!!  BANZAI!!",
			 victim->name, crime );
		REMOVE_BIT(ch->penalty, PENALTY_NOSHOUT);
		do_yell( ch, buf );
		multi_hit( ch, victim, TYPE_UNDEFINED );
		return TRUE;
	    }
	
	if ( ech != NULL )
	    {
		act( "$n screams 'PROTECT THE INNOCENT!!  BANZAI!!",
		     ch, NULL, NULL, TO_ROOM );
		multi_hit( ch, ech, TYPE_UNDEFINED );
		return TRUE;
	    }
	
	return FALSE;
}



bool spec_janitor( CHAR_DATA *ch )
{
	OBJ_DATA *trash;
	OBJ_DATA *trash_next;

	if ( !IS_AWAKE(ch) )
	return FALSE;

   if (IS_SET(ch->in_room->room_flags, ROOM_DONATION))
      return FALSE;

	for ( trash = ch->in_room->contents; trash != NULL; trash = trash_next )
	{
	trash_next = trash->next_content;
    if ( trash->wear_type == ITEM_NO_CARRY || !can_loot(ch,trash,FALSE)
	      || trash->item_type == ITEM_CORPSE_PC || trash->item_type == ITEM_CORPSE_NPC
	      || IS_BETWEEN(10375,trash->pIndexData->vnum,10379) )   /* quest objs */
		continue;
	if ( trash->item_type == ITEM_DRINK_CON
	||   trash->item_type == ITEM_TRASH
	||   trash->cost < 10 )
	{
		act( "$n picks up some trash.", ch, NULL, NULL, TO_ROOM );
		obj_from_room( trash );
		obj_to_char( trash, ch );
		return TRUE;
	}
	}

	return FALSE;
}



bool spec_mayor( CHAR_DATA *ch )
{
	static const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

	static const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

	static const char *path;
	static int pos;
	static bool move;

	if ( !move )
	{
	if ( time_info.hour ==  6 )
	{
		path = open_path;
		move = TRUE;
		pos  = 0;
	}

	if ( time_info.hour == 20 )
	{
		path = close_path;
		move = TRUE;
		pos  = 0;
	}
	}

	if ( ch->fighting != NULL )
	return spec_cast_mage( ch );
	if ( !move || ch->position < POS_SLEEPING )
	return FALSE;

	switch ( path[pos] )
	{
	case '0':
	case '1':
	case '2':
	case '3':
	move_char( ch, path[pos] - '0', FALSE );
	break;

	case 'W':
	set_pos( ch, POS_STANDING );
	act( "$n awakens and groans loudly.", ch, NULL, NULL, TO_ROOM );
	break;

	case 'S':
	set_pos( ch, POS_SLEEPING );
	act( "$n lies down and falls asleep.", ch, NULL, NULL, TO_ROOM );
	break;

	case 'a':
	act( "$n says 'Hello Honey!'", ch, NULL, NULL, TO_ROOM );
	break;

	case 'b':
	act( "$n says 'What a view!  I must do something about that dump!'",
		ch, NULL, NULL, TO_ROOM );
	break;

	case 'c':
	act( "$n says 'Vandals!  Youngsters have no respect for anything!'",
		ch, NULL, NULL, TO_ROOM );
	break;

	case 'd':
	act( "$n says 'Good day, citizens!'", ch, NULL, NULL, TO_ROOM );
	break;

	case 'e':
	act( "$n says 'I hereby declare the city of Midgaard open!'",
		ch, NULL, NULL, TO_ROOM );
	break;

	case 'E':
	act( "$n says 'I hereby declare the city of Midgaard closed!'",
		ch, NULL, NULL, TO_ROOM );
	break;

	case 'O':
/*  do_unlock( ch, "gate" ); */
	do_open( ch, "gate" );
	break;

	case 'C':
	do_close( ch, "gate" );
/*  do_lock( ch, "gate" ); */
	break;

	case '.' :
	move = FALSE;
	break;
	}

	pos++;
	return FALSE;
}


bool spec_poison( CHAR_DATA *ch )
{
    CHAR_DATA *victim;

    if ( ch->position != POS_FIGHTING
	 || ( victim = ch->fighting ) == NULL
	 || number_bits(1) == 0
	 || !check_hit(ch, victim, gsn_poison, DAM_PIERCE, 100)
	 || check_avoid_hit(ch, victim, FALSE) )
	return FALSE;

    act( "You bite $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n bites $N!",  ch, NULL, victim, TO_NOTVICT );
    act( "$n bites you!", ch, NULL, victim, TO_VICT    );
    spell_poison( gsn_poison, ch->level, ch, victim, TARGET_CHAR, FALSE );
    return TRUE;
}

bool spec_questmaster (CHAR_DATA *ch)
{
	if (ch->fighting != NULL) return spec_cast_mage( ch );
	return FALSE;
}

bool spec_bounty_hunter (CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	int bounty;
	char buf[MAX_STRING_LENGTH];

	if (number_range(0,1)==1) return spec_thief(ch);

      if ( IS_SET(ch->in_room->room_flags, ROOM_SAFE) )
         return FALSE;

	if ( !IS_AWAKE(ch) || ch->fighting != NULL )
	return FALSE;

	bounty = 0;
	for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
	{
	v_next = victim->next_in_room;

	if ( !IS_NPC(victim) 
	     && check_see(ch,victim) 
	     && !is_mimic(victim) )
	    bounty = victim->pcdata->bounty;

	if ( bounty > 4*ch->gold
	     && bounty >= victim->level * 10 ) 
	    break;
	}

	if ( victim == NULL )
	return FALSE;

	sprintf( buf, "%s, I'm here to collect on your bounty!", victim->name );
	do_say( ch, buf );
	do_backstab(ch, victim->name);
	multi_hit( ch, victim, TYPE_UNDEFINED );
	return TRUE;
}

bool spec_thief( CHAR_DATA *ch )
{
	CHAR_DATA *victim;
	CHAR_DATA *v_next;
	long gold,silver;

	if ( ch->position != POS_STANDING )
	return FALSE;

	for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
	{
	v_next = victim->next_in_room;

	if ( IS_NPC(victim)
	     || victim->level >= LEVEL_IMMORTAL
	     || number_bits( 5 ) != 0
	     || is_safe_spell(ch, victim, TRUE)
	     || !check_see(ch,victim))
	    continue;

	if ( IS_AWAKE(victim) && number_range( 0, ch->level ) == 0 )
	{
		act( "You discover $n's hands in your wallet!",
		ch, NULL, victim, TO_VICT );
		act( "$N discovers $n's hands in $S wallet!",
		ch, NULL, victim, TO_NOTVICT );
		return TRUE;
	}
	else
	{
		gold = victim->gold * UMIN(number_range(1,20),ch->level / 2) / 100;
		gold = UMIN(gold, ch->level * ch->level * 10 );
		ch->gold     += gold;
		victim->gold -= gold;
		silver = victim->silver * UMIN(number_range(1,20),ch->level/2)/100;
		silver = UMIN(silver,ch->level*ch->level * 25);
		ch->silver  += silver;
		victim->silver -= silver;
		return TRUE;
	}
	}

	return FALSE;
}

bool spec_remort (CHAR_DATA *ch)
{
	return spec_cast_cleric( ch );
}

bool spec_temple_guard ( CHAR_DATA *ch )
{
    /*
    RELIGION_DATA *rel;
    CHAR_DATA *victim, *v_next, *target;
    char buf[MSL];

    if ( !IS_AWAKE(ch) )
	return FALSE;

    if ( (rel = get_religion_of_guard(ch)) == NULL )
	return FALSE;

    target = NULL;
    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;

	if ( IS_NPC(victim)
	     || IS_IMMORTAL(victim)
	     || !check_see(ch,victim) )
	    continue;
	
	if ( get_religion(victim) == rel 
	     && (is_religion_member(victim) || !carries_relic(victim)) )
	    continue;

	// we found a target - check if it's our favourite one
	if ( carries_relic(victim) )
	{
	    target = victim;
	    break;
	}

	// only continue searching if we're not already fighting
	if ( ch->fighting != NULL )
	    continue;

	// attack highest level intruder
	if ( target == NULL || victim->level > target->level )
	    target = victim;
    }
    
    // do we have a new target?
    if ( target == NULL || target == ch->fighting )
	return FALSE;

    // is new target just a harmless non-believer?
    if ( get_religion(target) == NULL && !carries_relic(target) )
    {
	sprintf( buf, "This is the temple of %s. Get out %s!",
		 rel->god, target->name );
	do_say( ch, buf );
	send_to_char( "You are transfered by divine powers.\n\r", target );
	if ( target->fighting != NULL )
	    stop_fighting( target, TRUE );
	char_from_room( target );
	char_to_room( target, get_room_index(ROOM_VNUM_RECALL) );
	return TRUE;
    }

    // attack!
    do_religion_talk( ch, "We are under attack!" );
    sprintf( buf, "Intruders have entered the temple! In the name of %s!",
	     rel->god );
    do_yell( ch, buf );
    set_fighting( ch, target );
    multi_hit( ch, target, TYPE_UNDEFINED );
    return TRUE;
    */
    return FALSE;
}
