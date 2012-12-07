/*
 *	Song.c and song.h originally written by Mike Smullens for Aeaea (1997),
 *	a ROM 2.4b4a based mud.  Code modelled after the ROM magic.c.
 */


#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "song.h"


sh_int sn_first_song;
sh_int sn_last_song;

sh_int gsn_pied_piper;
sh_int gsn_shafts_theme;
sh_int gsn_cacophony;
sh_int gsn_lust_life;
sh_int gsn_white_noise;

int song_lookup 		args((CHAR_DATA *ch, const char *name ));
int mana_cost_song 	args((CHAR_DATA *ch, int sn));
void do_sing 		args(( CHAR_DATA *ch, char *argument ));
void song_effect 		args((CHAR_DATA *tar, int sn, int loc, int mod, int bit));
void hear_song 		args(( CHAR_DATA *singer, CHAR_DATA *victim, int sn));



int calc_song_sns()
{
	int sn;
	sn_first_song = -1;	

	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
		if ( skill_table[sn].name == NULL )
			break;
		if ( skill_table[sn].song_fun != NULL )
		{
			if (sn_first_song == -1)
				sn_first_song = sn;
			sn_last_song = sn;
		}
	}

	return 0;
}
	

int song_lookup(CHAR_DATA *ch, const char *name )
{
	int sn;

	for ( sn = sn_first_song; sn <= sn_last_song; sn++ )
	{
		if ( LOWER(name[0]) == LOWER(skill_table[sn].name[0])
			&& !str_prefix( name, skill_table[sn].name )
			&& get_skill(ch, sn) > 0 )
				return sn;
	}

	return -1;
}


bool saves_song( int level, CHAR_DATA *ch, CHAR_DATA *victim, int base_chance )
{
	int save;

	save = base_chance;
	save += ( victim->level - level) * 3 - victim->saving_throw;
	save += get_curr_stat(victim, STAT_WIS)/4 + get_curr_stat(victim, STAT_INT)/8;
	save -= get_curr_stat(ch, STAT_WIS)/4;
	save -= ch->hitroll/10;

	if (IS_AFFECTED(victim, AFF_FEEBLEMIND))
		save -=6;

	if (IS_AFFECTED(victim, AFF_CALM))
		save +=6;

	switch(check_immune(victim,DAM_SOUND))
	{
		case IS_IMMUNE:     save += 50;  break;
		case IS_RESISTANT:  save += 16;  break;
		case IS_VULNERABLE: save -= 16;  break;
	}

	switch(check_immune(victim,DAM_MENTAL))
	{
		case IS_IMMUNE:		save +=12; break;
		case IS_RESISTANT:	save +=8;  break;
		case IS_VULNERABLE:	save -=8;  break;
	}

	if (save > base_chance)
		save = ((save-base_chance)*(100-base_chance))/
			(100-2*base_chance+save)+base_chance;
	else
		save = ((save-base_chance)*base_chance)/(2*base_chance-save) + base_chance;

	return number_percent( ) < save;
}


int mana_cost_song (CHAR_DATA *ch, int sn)
{
	int min_mana, level;

	min_mana = skill_table[sn].min_mana;
	level = skill_table[sn].skill_level[ch->class];

	if (ch->level < level) return 1000;

	if (level > (LEVEL_HERO - 30))
		return ( ((20-LEVEL_HERO+level)*min_mana)/(20+ch->level-level) );
	else
		return ( (50*min_mana)/(20+UMIN(ch->level-level, 30)) );
}


int song_level ( CHAR_DATA *ch, int sn )
{
	int level;
	
	level = get_skill(ch,sn);

	if (!IS_NPC(ch) && (ch->pcdata->condition[COND_DRUNK]>0))
		level += ch->pcdata->condition[COND_DRUNK]/3 + 5;

	level = (level * ch->level)/100;

	return level;
}


void do_sing( CHAR_DATA *ch, char *argument )
{
	int sn, mana;
	char buf[MAX_STRING_LENGTH];

	if ( IS_NPC(ch) && ch->desc == NULL)
		return;

	if (ch->song_singing != song_null)
	{
		stop_singing(ch);
		if (argument[0] == '\0')
			return;
	}

	if ( argument[0] == '\0' )
	{
		send_to_char( "You raise your voice in song.\n\r", ch );
		act ("$n raises $s voice in song.",ch,NULL,NULL,TO_ROOM);
		return;
	}

	if ((sn = song_lookup(ch,argument)) < 1
		||  skill_table[sn].song_fun == NULL
		||  get_skill(ch, sn) < 1 )
	{
		send_to_char( "You don't know the words to that one.\n\r", ch );
		return;
	}

	if ( ch->position < skill_table[sn].minimum_position )
	{
		if (ch->position == POS_FIGHTING)
			send_to_char( "You are too worked up by the fight.\n\r", ch );
		else
			send_to_char ( "You are too relaxed.\n\r", ch );
		return;
	}

	mana=mana_cost_song(ch,sn); 

	if ( ch->mana < 2*mana )
	{
		send_to_char( "Your voice is too hoarse.  You'd best let it recover.\n\r", ch );
		return;
	}

	ch->mana -= mana;
	
	sprintf(buf, "You start to sing %s.\n\r", skill_table[sn].msg_off);
	send_to_char(buf,ch);
	sprintf(buf, "%s starts to sing %s.", ch->name, skill_table[sn].msg_off);
	act (buf,ch,NULL,NULL,TO_ROOM);
	  
	WAIT_STATE( ch, PULSE_VIOLENCE );

	ch->song_singing = sn;

	update_song(ch);
	  
	return;
}


void update_song( CHAR_DATA *ch )
{
	int mana, chance, sn, skill;
	char buf[MAX_STRING_LENGTH];

	if ((sn = ch->song_singing)==song_null) return;

	mana=mana_cost_song(ch,sn); 

	if ( ch->mana < mana )
	{
		send_to_char( "Your voice falters.\n\r", ch );
		stop_singing(ch);
		return;
	}

	ch->mana -= mana;
	
	skill = get_skill(ch, sn);

	if (skill>60)
		chance = 50 + skill/2;
	else
		chance = (4 * skill) /2;

	if ( number_percent() > chance )
	{
		send_to_char( "You don't remember the rest of the song.\n\r", ch );
		sprintf(buf, "%s forgets the rest of the words to %s.", 
			ch->name, skill_table[sn].msg_off);
		act (buf,ch,NULL,NULL,TO_ROOM);
		if (ch->in_room->singer == ch) stop_singing(ch);
		check_improve(ch,sn,FALSE,2);
		return;
	}

	check_improve(ch,sn,TRUE,5);
	
	ch->song_delay = skill_table[sn].beats;

	song_to_room(ch);

	return;	
}


void stop_singing( CHAR_DATA *ch )
{
	char buf[MAX_STRING_LENGTH];
	int sn;

	if ((sn = ch->song_singing) == song_null) return;

	sprintf(buf, "You stop singing %s.\n\r", skill_table[sn].msg_off);
	send_to_char(buf,ch);
	sprintf(buf, "%s stops singing %s.", ch->name,
		skill_table[sn].msg_off);
	act (buf,ch,NULL,NULL,TO_ROOM);
	  
	song_from_room(ch);
	ch->song_singing = song_null;

	return;
}


void song_from_room( CHAR_DATA *ch )
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if (ch->song_singing == song_null) return;

	for (vch=ch->in_room->people; vch !=NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;
		song_from_char(vch);
	}

	ch->in_room->singer = NULL;	

	return;
}


void song_to_room( CHAR_DATA *ch )
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	CHAR_DATA *singer;
	char buf[MAX_STRING_LENGTH];
	if (ch->song_singing == song_null) return;

	if ((singer = ch->in_room->singer) != ch)
	{
		if (singer == NULL)
			ch->in_room->singer = ch;
		else
		{
			send_to_char("It's too noisy in here to keep singing.\n\r", ch);
			sprintf(buf, "%s's music drowned out by %s.", ch->name,
				singer->name);
			act (buf,ch,NULL,NULL,TO_ROOM);
			ch->song_singing = song_null;
			return;
		}
	}
	else
		send_to_char("", ch);

	for (vch=ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;
		song_to_char(vch);
	}

	return;
}


void song_effect(CHAR_DATA *tar, int sn, int loc, int mod, int bit)
{
	AFFECT_DATA af;

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = 120;
	af.duration  = -1;
	af.location  = loc;
	af.modifier  = mod;
	af.bitvector = bit;
	affect_to_char( tar, &af );

	return;
}


void song_to_char( CHAR_DATA *ch )
{
	int sn;
	CHAR_DATA *singer;

	if ((singer = ch->in_room->singer)==NULL) return;
	sn = singer->song_singing;

	if (IS_SELF(ch , singer))
		if (skill_table[sn].target & TAR_SELF)
			hear_song(singer, ch, sn);
		else return;
	else if (IS_GROUP(ch , singer))
		if (skill_table[sn].target & TAR_GROUP)
			hear_song(singer, ch, sn);
		else return;
	else if (IS_ENEMY(ch , singer))
	{
		if (skill_table[sn].target & TAR_ENEMY)
		{
			if ((skill_table[sn].target & TAR_OFFENSIVE) 
					&& (is_safe_spell(singer, ch, TRUE)))
				return;
			else
			{
				if ((skill_table[sn].target & TAR_OFFENSIVE)
						&& (ch->fighting == NULL))
				    set_fighting( ch, singer );
				hear_song(singer, ch, sn);
			}
		}
		else return;
	}
	else
	{
		if (skill_table[sn].target & TAR_NEUTRAL)
		{
			if ((skill_table[sn].target & TAR_OFFENSIVE) 
					&& (is_safe_spell(singer, ch, TRUE)))
				return;
			else
			{
				if ((skill_table[sn].target & TAR_OFFENSIVE)
						&& (ch->fighting == NULL))
				    set_fighting( ch, singer );
				hear_song(singer, ch, sn);
			}
		}
		else return;
	}
	return;			
}


void hear_song( CHAR_DATA *singer, CHAR_DATA *victim, int sn)
{
	int old;
	old = victim->song_hearing;
	victim->song_hearing = sn;
	if (!((*skill_table[sn].song_fun) (sn, song_level(singer,sn), singer,
			victim, (old==sn) ? SONG_UPDATE : SONG_APPLY)))
		victim->song_hearing = song_null;

	return;
}


void song_from_char( CHAR_DATA *ch )
{
	int sn;
	CHAR_DATA *singer;

	if ((singer = ch->in_room->singer) == NULL) return;

	sn = singer->song_singing;

	(*skill_table[sn].song_fun)	(sn, song_level(singer,sn),
		singer, ch, SONG_REMOVE);

	ch->song_hearing = song_null;

	return;
}


/*
 *	Song Functions
 */


SONG(song_pied_piper)
{
	switch (task)
	{
		case SONG_APPLY:

			if (is_safe_spell(singer, target, TRUE)
					||   IS_AFFECTED(target, AFF_CHARM)
					||   IS_AFFECTED(singer, AFF_CHARM)
					||   level < target->level
					||   IS_SET(target->imm_flags,IMM_CHARM)
					||   LEVEL_HERO <= target->level 
					||   SAVES_SONG(60)
					||   (IS_SET(target->in_room->room_flags,ROOM_LAW)))
				return FALSE;
		
			if ( target->master )
				stop_follower( target );
			add_follower( target, singer );
			target->leader = singer;
			SONG_EFFECT_ADD(APPLY_INT, -5, AFF_CHARM);
			act( "Isn't $n just so nice?", singer, NULL, target, TO_VICT );
			act("$N looks at you with adoring eyes.",singer,NULL,target,TO_CHAR);
			
		break;
		
		case SONG_UPDATE:

			if (!SAVES_SONG(6))
				break;

		case SONG_REMOVE:

			SONG_EFFECT_REMOVE;
			stop_follower(target);

		break;
	}
	
	return TRUE;
}	

/*
 *  a powerful song, but keep in mind that bards can only sing one thing,
 *  and must keep paying for it, while spellcasters can accumulate fairly
 *  long-lasting spell effects.
 */
SONG(song_shafts_theme)
{
	switch (task)
	{
		case SONG_APPLY:
		
			if (IS_SELF(singer, target) || IS_GROUP(singer, target))
			{
				SONG_EFFECT_ADD(APPLY_STR, level/12, AFF_BERSERK);
				SONG_EFFECT_ADD(APPLY_DEX, level/12, AFF_HASTE);
				SONG_EFFECT_ADD(APPLY_HITROLL, level/12, 0);
				SONG_EFFECT_ADD(APPLY_DAMROLL, level/12, AFF_REGENERATION);
			}
			else if (IS_ENEMY(singer, target))
			{
				if (SAVES_SONG(50)) return FALSE;
				SONG_EFFECT_ADD(APPLY_STR, -level/12, AFF_WEAKEN);
				SONG_EFFECT_ADD(APPLY_DEX, -level/12, AFF_SLOW);
				SONG_EFFECT_ADD(APPLY_HITROLL, -level/12, AFF_CURSE);
				SONG_EFFECT_ADD(APPLY_DAMROLL, -level/12, AFF_FAERIE_FIRE);
			} 
			else 
				return FALSE;

		break;
		
		case SONG_UPDATE:

		break;

		case SONG_REMOVE:

			SONG_EFFECT_REMOVE;

		break;
	}
	
	return TRUE;
}	


SONG(song_cacophony)
{
	int dam;

	if (task == SONG_APPLY) return TRUE;
	if (task == SONG_REMOVE) return FALSE;

	dam = ((level+10) * (level+10))/20;
	dam = number_range(dam/2, dam);
	if (SAVES_SONG(50))
		dam /= 2;

	damage(singer, target, dam, sn, DAM_SOUND, TRUE);

	return TRUE;
}	


SONG(song_lust_life)
{
	return TRUE;
}	


SONG(song_white_noise)
{
	if (task == SONG_REMOVE) return FALSE;
	if (task == SONG_UPDATE)
	{
		if (SAVES_SONG(10))
		{
			act ("$n gets $n voice back.",target,NULL,NULL,TO_ROOM);
			act ("You regain your voice.",target,NULL,NULL,TO_CHAR);
			return FALSE;
		}
		return TRUE;
	}

	if (SAVES_SONG(60)) return FALSE;

	act ("$n's voice is muffled by the white noise.",target,NULL,NULL,TO_ROOM);
	act ("Your voice is muffled by the white noise.",target,NULL,NULL,TO_CHAR);

	return TRUE;
}	

