/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,    *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                     *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael      *
 *  Chastain, Michael Quan, and Mitchell Tse.                  *
 *                                     *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                           *
 *                                     *
 *  Much time and thought has gone into this software and you are      *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                          *
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

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

/* command procedures needed */
DECLARE_DO_FUN(do_groups    );
DECLARE_DO_FUN(do_help      );
DECLARE_DO_FUN(do_say       );

void do_skills(CHAR_DATA *ch, char *argument);
bool train_stat(int trained, CHAR_DATA *ch);
void show_groups( int skill, BUFFER *buffer );
void show_races( int skill, BUFFER *buffer );

bool can_gain_skill( CHAR_DATA *ch, int sn )
{
    return skill_table[sn].rating[ch->class] > 0
	&& skill_table[sn].skill_level[ch->class] < LEVEL_IMMORTAL;
}

// populate skill_costs recursively
static void set_group_skill_costs( int gn, int class, int *skill_costs )
{
    int i;
    for ( i = 0; i < MAX_IN_GROUP; i++ )
    {
        char *name = group_table[gn].spells[i];
        if ( name == NULL )
            break;
        int sn = skill_lookup_exact(name);
        if ( sn != -1 )
        {
            if ( skill_table[sn].skill_level[class] < LEVEL_IMMORTAL )
                skill_costs[sn] = skill_table[sn].rating[class];
        }
        else
        {
            int sub_gn = group_lookup(name);
            if ( sub_gn == -1 )
                bugf("add_group_skill_costs: Invalid skill or group name '%s' in group '%s'.", name, group_table[gn].name);
            else
                set_group_skill_costs(sub_gn, class, skill_costs);
        }
    }
}

// set skill cost to 0 for basic skills
static void filter_basic_skills( int class, int *skill_costs )
{
    int i, gn = group_lookup(class_table[class].base_group);
    for ( i = 0; i < MAX_IN_GROUP; i++ )
    {
        char *name = group_table[gn].spells[i];
        if ( name == NULL )
            break;
        int sn = skill_lookup_exact(name);
        if ( sn != -1 )
            skill_costs[sn] = 0;
    }
}

// get map of skill-number to cost for class (0 if not in group or not for class)
static int* get_group_skill_costs( int gn, int class )
{
    static int skill_costs[MAX_SKILL];
    int sn;
    for ( sn = 0; sn < MAX_SKILL; sn++ )
        skill_costs[sn] = 0;
    set_group_skill_costs(gn, class, skill_costs);
    filter_basic_skills(class, skill_costs);
    return skill_costs;
}

// set skill cost to 0 for skills already known
static void filter_known_skills( CHAR_DATA *ch, int *skill_costs )
{
    int sn;
    for ( sn = 0; sn < MAX_SKILL; sn++ )
        if ( ch->pcdata->learned[sn] )
            skill_costs[sn] = 0;
}

// calculate total cost from a set of individual skill costs (group rebate)
static int get_multi_skill_cost( int *skill_costs )
{
    int sn, sum = 0, skill_count = 0, costly_skill_count = 0;
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        sum += skill_costs[sn];
        if ( skill_costs[sn] > 0 )
            skill_count++;
        if ( skill_costs[sn] > 1 )
            costly_skill_count++;
    }
    // compute total cost so that any skill with cost > 0 increases total
    // also no rebate for groups with a single skill, and no more than 20%
    int max_rebate_by_count = costly_skill_count - 1;
    int max_rebate_by_cost = (sum - skill_count) / 4;
    return sum - UMAX(0,UMIN(max_rebate_by_count, max_rebate_by_cost));
}

int get_group_base_cost( int gn, int class )
{
    return get_multi_skill_cost(get_group_skill_costs(gn, class));
}

// group cost is reduced for a character if they already know skills in the group
int get_group_cost( CHAR_DATA *ch, int gn )
{
    int *skill_costs = get_group_skill_costs(gn, ch->class);
    filter_known_skills( ch, skill_costs );
    return get_multi_skill_cost( skill_costs );
}

// update cost for all groups to auto-calculated value
void update_group_costs()
{
    int gn, class;
    for ( gn = 0; gn < MAX_GROUP; gn++ )
        for ( class = 0; class < MAX_CLASS; class++ )
            if ( group_table[gn].rating[class] > 0 )
            {
                int group_cost = get_group_base_cost(gn, class);
                group_table[gn].rating[class] = (group_cost > 0 ? group_cost : -1);
            }
}

/* used to get new skills */
void do_gain(CHAR_DATA *ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
    char *argPtr;
	char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *trainer;
	int gn = 0, sn = 0;
	bool introspect = FALSE;
	
	if (IS_NPC(ch))
		return;
	
	/* find a trainer */
	for ( trainer = ch->in_room->people; trainer != NULL; trainer = trainer->next_in_room )
		if (IS_NPC(trainer) && IS_SET(trainer->act,ACT_GAIN))
			break;
		
		if (trainer == NULL || !can_see(ch,trainer))
		{
			if ( (get_skill(ch,gsn_introspection)) > 1 )
			{
				if ((get_skill(ch,gsn_introspection)) > number_percent() )
				{
					act( "$n thinks over what $e has experienced recently.",ch,NULL,NULL,TO_ROOM );
					check_improve(ch,gsn_introspection,TRUE,8);
					introspect = TRUE;
				}
				else
				{
					send_to_char("You find nothing meaningful in your introspection.\n\r",ch);
					check_improve(ch,gsn_introspection,FALSE,8);
					return;
				}
			}
			else
			{
				send_to_char( "You can't do that here.\n\r", ch );
				return;
			}
		}
		
                argPtr = one_argument( argument, arg );
                argPtr = one_argument( argPtr, arg2 );

		if (arg[0] == '\0')
		{
			if ( introspect )
				send_to_char("See HELP GAIN.\n\r",ch);
			else
				do_say(trainer,"Pardon me?");
			return;
		}
		else if (!str_prefix(arg,"list"))
		{
			int col;
			col = 0;
			sprintf(buf, "%-16s    %-5s %-16s    %-5s %-16s    %-5s\n\r",
				"group","cost","group","cost","group","cost");
			send_to_char(buf,ch);
			for (gn = 0; gn < MAX_GROUP; gn++)
			{
				if (group_table[gn].name == NULL)
					break;
				if (!ch->pcdata->group_known[gn]
				    &&  group_table[gn].rating[ch->class] > 0)
				{
                    sprintf(buf,"%-17s    %-5d ", group_table[gn].name, get_group_cost(ch, gn));
					send_to_char(buf,ch);
					if (++col % 3 == 0)
						send_to_char("\n\r",ch);
				}
			}
			if (col % 3 != 0)
				send_to_char("\n\r",ch);
			send_to_char("\n\r",ch);
			col = 0;
			sprintf(buf, "%-16slvl/%-5s %-16slvl/%-5s %-16slvl/%-5s\n\r",
				"skill","cost","skill","cost","skill","cost");
			send_to_char(buf,ch);
			for (sn = 0; sn < MAX_SKILL; sn++)
			{
				if (skill_table[sn].name == NULL)
					break;
				if ( !ch->pcdata->learned[sn] && can_gain_skill(ch, sn) )
				{
				    sprintf(buf,"%-17s%2d/%-5d ",
					    skill_table[sn].name,
					    skill_table[sn].skill_level[ch->class],
					    skill_table[sn].rating[ch->class]);
				    send_to_char(buf,ch);
				    if (++col % 3 == 0)
					send_to_char("\n\r",ch);
				}
			}
            if (col % 3 != 0)
				send_to_char("\n\r",ch);
			sprintf( buf, "\n\rYou have %d training sessions left.\n\r",
				 ch->train );
			send_to_char( buf, ch );
			return;
		}
		else if (!str_cmp(arg,"convert"))
		{
                    int train_count = 0;
                    if ( strcmp(arg2, "") == 0 )
                        train_count = 1;
                    else if ( strcmp(arg2, "all") == 0 )
                        train_count = ch->practice/10;
                    else if ( is_number(arg2) )
                        train_count = atoi(arg2);
                    else
                    {
                        send_to_char("Syntax: convert <#|all>.\n\r",ch);
                        return;
                    }
                    if (train_count < 0)
                    {
                        send_to_char("Use revert to to change trains back into practices.\n\r", ch);
                        return;                        
                    }
                    
			if (ch->practice < 10 * train_count)
			{
				if ( introspect )
					send_to_char("You are not ready.\n\r",ch);
				else
					act("$N tells you 'You are not yet ready.'",
					ch,NULL,trainer,TO_CHAR );
				return;
			}
			if ( introspect )
				send_to_char("You apply your practice to training.\n\r",ch);
			else
				act("$N helps you apply your practice to training.",
				ch,NULL,trainer,TO_CHAR );                        
			ch->practice -= 10 * train_count;
			ch->train += train_count;
                        printf_to_char(ch,"You gain %d train(s).\n\r", train_count);
			return;
		}
		else if (!str_cmp(arg,"revert"))
		{
			if (ch->train < 1)
			{
				if ( introspect )
					send_to_char("You are not ready.\n\r",ch);
				else
					act("$N tells you 'You are not yet ready.'",
					ch,NULL,trainer,TO_CHAR);
				return;
			}
			if ( introspect )
				send_to_char("You apply your training to practices.\n\r",ch);   
			else
				act("$N helps you apply your training to practices.",
				ch,NULL,trainer,TO_CHAR);
			ch->train -=1 ;
			ch->practice += 8;
			return;
		}
		else if (!str_cmp(arg, "losehp"))
		{
            if (ch->pcdata->trained_hit < 2)
            {
                if ( introspect )
                    send_to_char("That wouldn't be prudent.\n\r",ch);
                else
                    act( "$N does not think that would be prudent.",
                    ch,NULL,trainer,TO_CHAR );
                return;
            }
            if ( introspect )
                send_to_char("You transfer some hit points to 1 train.\n\r",ch);
            else
                act( "$N channels power from the gods, and transfers some of your hit points into 1 train.",
                ch,NULL,trainer,TO_CHAR );
            ch->pcdata->trained_hit -= 2;
            ch->train += 1;
            update_perm_hp_mana_move(ch);
            return;

		}
		else if (!str_cmp(arg, "losemove"))
		{
            if (ch->pcdata->trained_move < 2)
            {
                if ( introspect )
                    send_to_char("That wouldn't be prudent.\n\r",ch);
                else
                    act( "$N does not think that would be prudent.",
                    ch,NULL,trainer,TO_CHAR );
                return;
            }
            if ( introspect )
                send_to_char("You transfer some move points to 1 train.\n\r",ch);
            else
                act( "$N channels power from the gods, and transfers some of your move points into 1 train.",
                ch,NULL,trainer,TO_CHAR );
            ch->pcdata->trained_move -= 2;
            ch->train += 1;
            update_perm_hp_mana_move(ch);
            return;
		}
		else if (!str_cmp(arg, "losemana"))
		{
            if (ch->pcdata->trained_mana < 2)
            {
                if ( introspect )
                    send_to_char("That wouldn't be prudent.\n\r",ch);
                else
                    act( "$N does not think that would be prudent.",
                    ch,NULL,trainer,TO_CHAR );
                return;
            }
            if ( introspect )
                send_to_char("You transfer some mana into 1 train.\n\r",ch);
            else
                act( "$N channels power from the gods, and transfers some of your mana into 1 train.",
                ch,NULL,trainer,TO_CHAR );
            ch->pcdata->trained_mana -= 2;
            ch->train += 1;
            update_perm_hp_mana_move(ch);
            return;
		}
		else if ( (gn = group_lookup(argument)) > 0)
		{
			if (ch->pcdata->group_known[gn])
			{
				if ( introspect )
					send_to_char("You already know that group!\n\r",ch);
				else
					act( "$N tells you 'You already know that group!'",
					ch,NULL,trainer,TO_CHAR );
				return;
			}
			if (group_table[gn].rating[ch->class] <= 0)
			{
				if ( introspect )
					send_to_char("That group is beyond your powers.\n\r",ch);
				else
					act( "$N tells you 'That group is beyond your powers.'",
					ch,NULL,trainer,TO_CHAR );
				return;
			}
            int group_cost = get_group_cost(ch, gn);
            if (ch->train < group_cost)
			{
				if ( introspect )
					send_to_char("You aren't ready for that group.\n\r",ch);
				else
					act ("$N tells you 'You are not yet ready for that group.'",
					ch,NULL,trainer,TO_CHAR );
				return;
			}
			gn_add(ch,gn);
			if ( introspect )
				act("You learn the art of $t",
				ch,group_table[gn].name,NULL,TO_CHAR);
			else
				act("$N trains you in the art of $t",
				ch,group_table[gn].name,trainer,TO_CHAR );
            ch->train -= group_cost;
			return;
		}
		else if ((sn = skill_lookup(argument)) > -1)
		{			
			if (ch->pcdata->learned[sn])
			{
				if ( introspect )
					send_to_char("You already know that skill.\n\r",ch);
				else
					act( "$N tells you 'You already know that skill!'",
					ch,NULL,trainer,TO_CHAR );
				return;
			}
			
			if ( !can_gain_skill(ch, sn) )
			{
				if ( introspect )
					send_to_char("That skill is beyond your powers.\n\r",ch);
				else
					act( "$N tells you 'That skill is beyond your powers.'",
					ch,NULL,trainer,TO_CHAR );
				return;
			}
			if (ch->train < skill_table[sn].rating[ch->class])
			{
				if ( introspect )
					send_to_char("You aren't ready for that skill.\n\r",ch);
				else
					act( "$N tells you 'You are not yet ready for that skill.'",
					ch,NULL,trainer,TO_CHAR );
				return;
			}
			ch->pcdata->learned[sn] = 1;
			if ( introspect )
				act("You learn the art of $t.",
				ch,skill_table[sn].name,NULL,TO_CHAR);
			else
				act("$N trains you in the art of $t.",
				ch,skill_table[sn].name,trainer,TO_CHAR);
			ch->train -= skill_table[sn].rating[ch->class];
			return;
		}		
		if ( introspect )
			send_to_char( "See HELP GAIN.\n\r", ch );
		else
			act("$N tells you 'I do not understand...'",ch,NULL,trainer,TO_CHAR);
		
}


void do_skill( CHAR_DATA *ch, char *argument )
{
    char buf[MSL];
    int sn, level, skill, mana;
    
    if (IS_NPC(ch))
	return;

    /* For lazy players who want to access the 'skills' command easier :) */
    if ( argument[0] == '\0' )
    {
	do_skills( ch, "" );
	return;
    }

    if ( !strcmp(argument, "all") )
    {
	do_skills( ch, "all" );
	return;
    }

    if ( (sn = skill_lookup(argument)) < 0 )
    {
	send_to_char( "That skill doesn't exist.\n\r", ch );
	return;
    }

    level = skill_table[sn].skill_level[ch->class];
    skill = get_skill(ch,sn);

  /* Now uses existing functions to display mana on spells - Astark 4-23-13 */

    if (!IS_SPELL(sn))
    {
        if( level > 100 )
        {
	    if( skill > 0 )
            {
                sprintf( buf, "Proficiency for raceskill %s:  %3d%% practiced  %3d%% effective\n\r",
                    skill_table[sn].name, ch->pcdata->learned[sn], skill );
            }
            else
                sprintf( buf, "You do not have the %s skill.\n\r", skill_table[sn].name );
        }
        else
	    sprintf( buf, "Proficiency for lvl %d skill %s:  %3d%% practiced  %3d%% effective\n\r",
                level, skill_table[sn].name, ch->pcdata->learned[sn], skill );
    }
    else
    {
        if( level > 100 )
        {
	    if( skill > 0 )
            {
                sprintf( buf, "Proficiency for raceskill %s:  %3d%% practiced  %3d%% effective %3d mana\n\r",
                    skill_table[sn].name, ch->pcdata->learned[sn], skill, mana_cost(ch, sn, skill));
            }
            else
                sprintf( buf, "You do not have the %s skill.\n\r", skill_table[sn].name );
        }
        else
            sprintf( buf, "Proficiency for lvl %d skill %s:  %3d%% practiced  %3d%% effective %3d mana\n\r",
                level, skill_table[sn].name, ch->pcdata->learned[sn], skill, mana_cost(ch, sn, skill));  
    }

    send_to_char( buf, ch );
}

/* RT spells and skills show the players spells (or skills) */

void do_spells(CHAR_DATA *ch, char *argument)
{
	BUFFER *buffer;
	char arg[MAX_INPUT_LENGTH];
	char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
	char spell_columns[LEVEL_HERO + 1];
	int sn, level, skill, min_lev = 1, max_lev = LEVEL_HERO, mana, prac;
	bool fAll = FALSE, found = FALSE;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
	  return;

	if (argument[0] != '\0')
	{
	fAll = TRUE;

	if (str_prefix(argument,"all"))
	{
		argument = one_argument(argument,arg);
		if (!is_number(arg))
		{
		send_to_char("Arguments must be numerical or all.\n\r",ch);
		return;
		}
		max_lev = atoi(arg);

		if (max_lev < 1 || max_lev > LEVEL_HERO)
		{
		sprintf(buf,"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		send_to_char(buf,ch);
		return;
		}

		if (argument[0] != '\0')
		{
		argument = one_argument(argument,arg);
		if (!is_number(arg))
		{
			send_to_char("Arguments must be numerical or all.\n\r",ch);
			return;
		}
		min_lev = max_lev;
		max_lev = atoi(arg);

		if (max_lev < 1 || max_lev > LEVEL_HERO)
		{
			sprintf(buf,
			"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
			send_to_char(buf,ch);
			return;
		}

		if (min_lev > max_lev)
		{
			send_to_char("That would be silly.\n\r",ch);
			return;
		}
		}
	}
	}


	/* initialize data */
	for (level = 0; level < LEVEL_HERO + 1; level++)
	{
		spell_columns[level] = 0;
		spell_list[level][0] = '\0';
	}

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
	    if (skill_table[sn].name == NULL )
		break;

	    if ((level = skill_table[sn].skill_level[ch->class]) < LEVEL_HERO + 1
		&&  (fAll || level <= ch->level)
		&&  level >= min_lev && level <= max_lev
                &&  skill_table[sn].spell_fun != spell_null
		&&  ch->pcdata->learned[sn] > 0)
	    {
		found = TRUE;
		level = skill_table[sn].skill_level[ch->class];

		prac = ch->pcdata->learned[sn];
		skill = get_skill(ch, sn);
		mana = mana_cost(ch, sn, skill);
		sprintf(buf,"%-16s %3dm %3d%%(%3d%%) ",
			skill_table[sn].name, mana, prac, skill);

		if (spell_list[level][0] == '\0')
			sprintf(spell_list[level],"\n\rLevel %2d: %s",level,buf);
		else /* append */
		{
			if ( ++spell_columns[level] % 2 == 0)
			strcat(spell_list[level],"\n\r          ");
			strcat(spell_list[level],buf);
		}
	}
	}

	/* return results */

	if (!found)
	{
		send_to_char("No spells found.\n\r",ch);
		return;
	}

    // show injury penalty
    int penalty = get_injury_penalty(ch);
    if ( penalty > 0 )
        printf_to_char(ch, "{rNote: Your spells are reduced by up to %d%% due to injury.{x\n\r", penalty);

	buffer = new_buf();
	for (level = 0; level < LEVEL_HERO + 1; level++)
		if (spell_list[level][0] != '\0')
		add_buf(buffer,spell_list[level]);
	add_buf(buffer,"\n\r");
	page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
}

void do_skills(CHAR_DATA *ch, char *argument)
{
	BUFFER *buffer;
	char arg[MAX_INPUT_LENGTH];
	char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
	char skill_columns[LEVEL_HERO + 1];
	int sn, level, prac, min_lev = 1, max_lev = LEVEL_HERO;
	bool fAll = FALSE, found = FALSE;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
	  return;

	if (argument[0] != '\0')
	{
	fAll = TRUE;

    argument = one_argument(argument,arg);
    if (!strcmp(arg, "class" ) )
    {
        show_class_skills( ch, argument);
        return;
    }

	if (str_prefix(arg,"all"))
	{
		if (!is_number(arg))
		{
		send_to_char("Arguments must be numerical or all.\n\r",ch);
		return;
		}
		max_lev = atoi(arg);

		if (max_lev < 1 || max_lev > LEVEL_HERO)
		{
		sprintf(buf,"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		send_to_char(buf,ch);
		return;
		}

		if (argument[0] != '\0')
		{
		argument = one_argument(argument,arg);
		if (!is_number(arg))
		{
			send_to_char("Arguments must be numerical or all.\n\r",ch);
			return;
		}
		min_lev = max_lev;
		max_lev = atoi(arg);

		if (max_lev < 1 || max_lev > LEVEL_HERO)
		{
			sprintf(buf,
			"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
			send_to_char(buf,ch);
			return;
		}

		if (min_lev > max_lev)
		{
			send_to_char("That would be silly.\n\r",ch);
			return;
		}
		}
	}
	}


	/* initialize data */
	for (level = 0; level < LEVEL_HERO + 1; level++)
	{
		skill_columns[level] = 0;
		skill_list[level][0] = '\0';
	}

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
	    if (skill_table[sn].name == NULL )
		break;

	    if ((level = skill_table[sn].skill_level[ch->class]) < LEVEL_HERO + 1
		&&  (fAll || level <= ch->level)
		&&  level >= min_lev && level <= max_lev
		&&  skill_table[sn].spell_fun == spell_null
		&&  ch->pcdata->learned[sn] > 0)
	    {
		found = TRUE;
		level = skill_table[sn].skill_level[ch->class];
		prac = ch->pcdata->learned[sn];
		sprintf(buf,"%-21s %3d%%(%3d%%) ",
			skill_table[sn].name, prac, get_skill(ch, sn));
		
		if (skill_list[level][0] == '\0')
		    sprintf(skill_list[level],"\n\rLevel %2d: %s",level,buf);
		else /* append */
		{
		    if ( ++skill_columns[level] % 2 == 0)
			strcat(skill_list[level],"\n\r          ");
		    strcat(skill_list[level],buf);
		}
	    }
	}

	/* return results */

	if (!found)
	{
		send_to_char("No skills found.\n\r",ch);
		return;
	}

	// show injury penalty
    int penalty = get_injury_penalty(ch);
    if ( penalty > 0 )
        printf_to_char(ch, "{rNote: Your skills are reduced by up to %d%% due to injury.{x\n\r", penalty);

    /* let's show exotic */
    printf_to_char( ch, "\n\r          %-21s     (%3d%%)", "exotic", get_weapon_skill(ch, -1));

	buffer = new_buf();
	for (level = 0; level < LEVEL_HERO + 1; level++)
		if (skill_list[level][0] != '\0')
		add_buf(buffer,skill_list[level]);
	add_buf(buffer,"\n\r");
	page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
}

void show_class_skills( CHAR_DATA *ch, char *argument )
{
    int class = class_lookup( argument );
    if ( class == -1 )
    {
        send_to_char("Invalid class.\n\r", ch);
        return;
    }

    BUFFER *buffer=new_buf();
    int i=0;

    char buf[MSL];
    sprintf( buf, "\n\r%-20s %5s    %3s    %3s{x\n\r", "Skill", "Lvl", "Rtg", "Max");
    add_buf( buffer, buf );
    for ( i=0; skill_table[i].name != NULL ; i++ )
    {
        if ( skill_table[i].skill_level[class] <= HERO )
        {
           sprintf( buf, "%-20s {g%5d    %3d    %3d{x\n\r",
                         capitalize( skill_table[i].name ),
                         skill_table[i].skill_level[class],
                         skill_table[i].rating[class],
                         skill_table[i].cap[class] );
           add_buf( buffer, buf );
        }
    }
    page_to_char( buf_string(buffer), ch );
    free_buf(buffer);
}

/* shows skills, groups and costs (only if not bought) */
void list_group_costs(CHAR_DATA *ch)
{
    char buf[MSL];
    int gn,sn,col;
    
    if (IS_NPC(ch))
        return;
    
    col = 0;
    
    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s\n\r","group","cp","group","cp","group","cp");
    send_to_char(buf,ch);
    
    for (gn = 0; gn < MAX_GROUP; gn++)
    {
        if (group_table[gn].name == NULL)
            break;
        
        if (!ch->gen_data->group_chosen[gn]
            &&  !ch->pcdata->group_known[gn]
            &&  group_table[gn].rating[ch->class] > 0)
        {
            sprintf(buf,"%-18s %-5d ",group_table[gn].name,
                group_table[gn].rating[ch->class]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);
    
    col = 0;
    
    sprintf(buf,"%-16s%-6s    %-16s%-6s    %-16s%-6s\n\r","skill","lvl/cp","skill","lvl/cp","skill","lvl/cp");
    send_to_char(buf,ch);
    
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL)
            break;
        
        if (!ch->gen_data->skill_chosen[sn]
            &&  ch->pcdata->learned[sn] == 0
            &&  can_gain_skill(ch, sn) )
        {
            sprintf(buf,"%-16s %2d/%-5d ",
                skill_table[sn].name,
                skill_table[sn].skill_level[ch->class],
                skill_table[sn].rating[ch->class]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);

    sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
    send_to_char(buf,ch);
    if ( ch->pcdata->points > 50 )
    {
        send_to_char( "NOTE: You may spend up to 60 creation points, but it is recommended to safe some.\n\r", ch );
        send_to_char( "      Unspent points convert to trains which can be used to train stats early on.\n\r", ch );
    }
    return;
}


void list_group_chosen(CHAR_DATA *ch)
{
	char buf[100];
	int gn,sn,col;

	if (IS_NPC(ch))
		return;

	col = 0;

	sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s","group","cp","group","cp","group","cp\n\r");
	send_to_char(buf,ch);

	for (gn = 0; gn < MAX_GROUP; gn++)
	{
		if (group_table[gn].name == NULL)
			break;

		if (ch->gen_data->group_chosen[gn]
	&&  group_table[gn].rating[ch->class] > 0)
		{
			sprintf(buf,"%-18s %-5d ",group_table[gn].name,
									group_table[gn].rating[ch->class]);
			send_to_char(buf,ch);
			if (++col % 3 == 0)
				send_to_char("\n\r",ch);
		}
	}
	if ( col % 3 != 0 )
		send_to_char( "\n\r", ch );
	send_to_char("\n\r",ch);

	col = 0;

	sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s","skill","cp","skill","cp","skill","cp\n\r");
	send_to_char(buf,ch);

	for (sn = 0; sn < MAX_SKILL; sn++)
	{
		if (skill_table[sn].name == NULL)
			break;

		if ( ch->gen_data->skill_chosen[sn]
		     && can_gain_skill(ch, sn) )
		{
			sprintf(buf,"%-18s %-5d ",skill_table[sn].name,
									skill_table[sn].rating[ch->class]);
			send_to_char(buf,ch);
			if (++col % 3 == 0)
				send_to_char("\n\r",ch);
		}
	}
	if ( col % 3 != 0 )
		send_to_char( "\n\r", ch );
	send_to_char("\n\r",ch);

	sprintf(buf,"Creation points: %d\n\r",ch->gen_data->points_chosen);
	send_to_char(buf,ch);
	return;
}

/* gives a character the experience needed for his level if he has too little */
void set_level_exp( CHAR_DATA *ch )
{
    int exp_needed;

    if ( ch == NULL || IS_NPC(ch) )
	return;

    exp_needed = exp_per_level(ch) * ch->level;
    
    if ( ch->exp < exp_needed )
	ch->exp = exp_needed;
}

int exp_per_level(CHAR_DATA *ch)
{
    int rem_add,race_factor;

    if (IS_NPC(ch))
        return 1000;

    if ( !strcmp(pc_race_table[ch->race].name, "gimp") )
        rem_add = 5;
    else if ( !strcmp(pc_race_table[ch->race].name, "doppelganger") )
        rem_add = 20;
    else
        rem_add = 15;

    race_factor = pc_race_table[ch->race].class_mult[ch->class] +
        rem_add * (ch->pcdata->remorts - pc_race_table[ch->race].remorts);

    return 10 * (race_factor);
}

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups(CHAR_DATA *ch,char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char buf[100];
   int gn,sn,i;

   if (argument[0] == '\0')
	  return FALSE;

   argument = one_argument(argument,arg);

   if (!str_prefix(arg,"help"))
   {
	  if (argument[0] == '\0')
	  {
		 do_help(ch,"group help");
		 return TRUE;
	  }
	
	  do_help(ch,argument);
	  return TRUE;
   }

   if (!str_prefix(arg,"add"))
   {
	  if (argument[0] == '\0')
	  {
		 send_to_char("You must provide a skill name.\n\r",ch);
		 return TRUE;
	  }
	
	  gn = group_lookup(argument);
	  if (gn != -1)
	  {
		 if (ch->gen_data->group_chosen[gn]
			||  ch->pcdata->group_known[gn])
		 {
			send_to_char("You already know that group!\n\r",ch);
			return TRUE;
		 }
		
		 if (group_table[gn].rating[ch->class] < 1)
		 {
			send_to_char("That group is not available.\n\r",ch);
			return TRUE;
		 }
		
		 /* Added 1/10/98 by Rimbol. Code suggested by dennis@realms.reichel.net */
		 for ( i=0; group_table[gn].spells[i] != NULL ; i++)
		 {
			if ( group_lookup( group_table[gn].spells[i] ) == -1 )
			   continue;
			if ( ch->pcdata->group_known[group_lookup( group_table[gn].spells[i])] )
			{
			   send_to_char("That group contains groups you already know.\n\r",ch);
			   send_to_char("Please \"drop\" them if you wish to gain this one.\n\r",ch);
			   return TRUE;
			}
		 }
		
		 for ( i=0; group_table[gn].spells[i] != NULL ; i++)
		 {
			if ( skill_lookup( group_table[gn].spells[i] ) == -1 )
			   continue;
			if ( ch->gen_data->skill_chosen[skill_lookup( group_table[gn].spells[i])]  )
			{
			   send_to_char("That group contains skills/spells you already know.\n\r",ch);
			   send_to_char("Please \"drop\" them if you wish to gain this one.\n\r",ch);
			   return TRUE;
			}
		 }
		 if ( ( ch->pcdata->points + group_table[gn].rating[ch->class] )> MAX_CP)
		 {
		     printf_to_char(ch,"You can't exceed %d creation points, but you will be able to gain new skills later.\n\r", MAX_CP);
		     return TRUE;
		 }
		
		 sprintf(buf,"%s group added\n\r",group_table[gn].name);
		 send_to_char(buf,ch);
		 ch->gen_data->group_chosen[gn] = TRUE;
		 ch->gen_data->points_chosen += group_table[gn].rating[ch->class];
		 gn_add(ch,gn);
		 ch->pcdata->points += group_table[gn].rating[ch->class];
		 return TRUE;
	  }
	
	  sn = skill_lookup(argument);
	  if (sn != -1)
	  {
		 if (ch->gen_data->skill_chosen[sn]
			||  ch->pcdata->learned[sn] > 0)
		 {
			send_to_char("You already know that skill!\n\r",ch);
			return TRUE;
		 }
		
		 if ( !can_gain_skill(ch, sn) )
		 {
			send_to_char("That skill is not available.\n\r",ch);
			return TRUE;
		 }
		 if ( ( ch->pcdata->points + skill_table[sn].rating[ch->class]) > MAX_CP)
		 {
			printf_to_char(ch,"You can't exceed %d creation points, but you will be able to gain new skills later.\n\r",MAX_CP);
			return TRUE;
		 }
		 sprintf(buf, "%s skill added\n\r",skill_table[sn].name);
		 send_to_char(buf,ch);
		 ch->gen_data->skill_chosen[sn] = TRUE;
		 ch->gen_data->points_chosen += skill_table[sn].rating[ch->class];
		 ch->pcdata->learned[sn] = 1;
		 ch->pcdata->points += skill_table[sn].rating[ch->class];
		 return TRUE;
	  }
	
	  send_to_char("No skills or groups by that name...\n\r",ch);
	  return TRUE;
   }

   if (!strcmp(arg,"drop"))
   {
	  if (argument[0] == '\0')
	  {
		 send_to_char("You must provide a skill to drop.\n\r",ch);
		 return TRUE;
	  }
	
	  gn = group_lookup(argument);
	  if (gn != -1 && ch->gen_data->group_chosen[gn])
	  {
		 send_to_char("Group dropped.\n\r",ch);
		 ch->gen_data->group_chosen[gn] = FALSE;
		 ch->gen_data->points_chosen -= group_table[gn].rating[ch->class];
		 gn_remove(ch,gn);
		 for (i = 0; i < MAX_GROUP; i++)
		 {
			if (ch->gen_data->group_chosen[gn])
			   gn_add(ch,gn);
		 }
		 ch->pcdata->points -= group_table[gn].rating[ch->class];
		 return TRUE;
	  }
	
	  sn = skill_lookup(argument);
	  if (sn != -1 && ch->gen_data->skill_chosen[sn])
	  {
		 send_to_char("Skill dropped.\n\r",ch);
		 ch->gen_data->skill_chosen[sn] = FALSE;
		 ch->gen_data->points_chosen -= skill_table[sn].rating[ch->class];
		 ch->pcdata->learned[sn] = 0;
		 ch->pcdata->points -= skill_table[sn].rating[ch->class];
		 return TRUE;
	  }
	
	  send_to_char("You haven't bought any such skill or group.\n\r",ch);
	  return TRUE;
   }

   if (!str_prefix(arg,"premise"))
   {
	  do_help(ch,"premise");
	  return TRUE;
   }

   if (!str_prefix(arg,"list"))
   {
	  list_group_costs(ch);
	  return TRUE;
   }

   if (!str_prefix(arg,"learned"))
   {
	  list_group_chosen(ch);
	  return TRUE;
   }

   if (!str_prefix(arg,"info"))
   {
	  do_groups(ch,argument);
	  return TRUE;
   }

   return FALSE;
}
		
	


		

/* shows all groups, or the sub-members of a group */
void do_groups(CHAR_DATA *ch, char *argument)
{
   char buf[100];
   int gn,sn,col,i;

   if (IS_NPC(ch))
	  return;

   col = 0;

   if (argument[0] == '\0')
   {   /* show all groups */
	
	  for (gn = 0; gn < MAX_GROUP; gn++)
	  {
		 if (group_table[gn].name == NULL)
			break;
		 if (ch->pcdata->group_known[gn])
		 {
			sprintf(buf,"%-20s ",group_table[gn].name);
			send_to_char(buf,ch);
			if (++col % 3 == 0)
			   send_to_char("\n\r",ch);
		 }
	  }
	  if ( col % 3 != 0 )
		 send_to_char( "\n\r", ch );
	  sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
	  send_to_char(buf,ch);
	  return;
   }

   if (!str_cmp(argument,"all"))    /* show all groups */
   {
	  for (gn = 0; gn < MAX_GROUP; gn++)
	  {
		 if (group_table[gn].name == NULL)
			break;
		 sprintf(buf,"%-20s ",group_table[gn].name);
		 send_to_char(buf,ch);
		 if (++col % 3 == 0)
			send_to_char("\n\r",ch);
	  }
	  if ( col % 3 != 0 )
		 send_to_char( "\n\r", ch );
	  return;
   }


   /* show the sub-members of a group */
   gn = group_lookup(argument);
   if (gn == -1)
   {
	  send_to_char("No group of that name exist.\n\r",ch);
	  send_to_char(
		 "Type 'groups all' or 'info all' for a full listing.\n\r",ch);
	  return;
   }

   sprintf( buf, "Group: %s\n\rCost: %d\n\r", group_table[gn].name, 
	    group_table[gn].rating[ch->class] );
   send_to_char( buf, ch );
   for (sn = 0; sn < MAX_IN_GROUP; sn++)
   {
	  if (group_table[gn].spells[sn] == NULL)
	      break;
	  i = skill_lookup_exact(group_table[gn].spells[sn]);
	  if (i < 0)
	  {
	      /* is it a group? */
	      i = group_lookup(group_table[gn].spells[sn]);
	      if ( i < 0 )
		  sprintf(buf,"[             ?              ] %s\n\r",
			   group_table[gn].spells[sn] );
	      else
		  sprintf(buf,"[           cost %2d          ] %-20s\n\r",
			  group_table[i].rating[ch->class],
			  group_table[gn].spells[sn]);
	  }
	  else
	  {
		  sprintf(buf,"[level %3d, cost %2d, max %3d%%] %-20s\n\r",
			  skill_table[i].skill_level[ch->class],
			  skill_table[i].rating[ch->class],
			  skill_table[i].cap[ch->class],
			  group_table[gn].spells[sn]);
	  }
	  send_to_char(buf,ch);
   }
}

/* checks for skill improvement */
void check_improve( CHAR_DATA *ch, int sn, bool success, int multiplier )
{
	int chance;
	char buf[100];

	if (IS_NPC(ch))
	return;

	if (ch->level < skill_table[sn].skill_level[ch->class]
	||  skill_table[sn].rating[ch->class] == 0
	||  ch->pcdata->learned[sn] == 0
	||  ch->pcdata->learned[sn] == 100)
	return;  /* skill is not known */

	if (multiplier > 0)
	{
	/* check to see if the character has a chance to learn */
	chance = 5 * ch_int_learn(ch);
	chance /= skill_table[sn].rating[ch->class];
	chance += ch->level;
	if ( IS_AFFECTED(ch, AFF_LEARN) )
	    chance *= 3;
	chance /= multiplier;
	chance = UMAX(chance, 1);

	if (number_range(1,3000) > chance)
	return;
	}

	/* now that the character has a CHANCE to learn, see if they really have */

	if (success)
	{
	chance = URANGE(2,100 - ch->pcdata->learned[sn], 98);
	if (number_percent() < chance)
	{
		sprintf(buf,"You have become better at %s!\n\r",
			skill_table[sn].name);
		send_to_char(buf,ch);
		ch->pcdata->learned[sn]++;
		gain_exp(ch,2 * skill_table[sn].rating[ch->class]);
	}
	}

	else
	{
	chance = URANGE(5,ch->pcdata->learned[sn]/2,30);
	if (number_percent() < chance)
	{
		sprintf(buf,
		"You learn from your mistakes, and your %s skill improves.\n\r",
		skill_table[sn].name);
		send_to_char(buf,ch);
		ch->pcdata->learned[sn] += number_range(1,3);
		ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn],100);
		gain_exp(ch,2 * skill_table[sn].rating[ch->class]);
	}
	}
}

/* returns a group index number given the name */
int group_lookup( const char *name )
{
	int gn;

	for ( gn = 0; gn < MAX_GROUP; gn++ )
	{
		if ( group_table[gn].name == NULL )
			break;
		if ( LOWER(name[0]) == LOWER(group_table[gn].name[0])
		&&   !str_prefix( name, group_table[gn].name ) )
			return gn;
	}

	return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add( CHAR_DATA *ch, int gn)
{
	int i;
	
	ch->pcdata->group_known[gn] = TRUE;
	for ( i = 0; i < MAX_IN_GROUP; i++)
	{
		if (group_table[gn].spells[i] == NULL)
			break;
		group_add(ch,group_table[gn].spells[i],FALSE);
	}
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove( CHAR_DATA *ch, int gn)
{
	int i;

	ch->pcdata->group_known[gn] = FALSE;

	for ( i = 0; i < MAX_IN_GROUP; i ++)
	{
	if (group_table[gn].spells[i] == NULL)
		break;
	group_remove(ch,group_table[gn].spells[i]);
	}
}
	
/* use for processing a skill or group for addition  */
void group_add( CHAR_DATA *ch, const char *name, bool deduct)
{
	int sn,gn;

	if (IS_NPC(ch)) /* NPCs do not have skills */
	return;

	sn = skill_lookup(name);

	if (sn != -1)
	{
	if (ch->pcdata->learned[sn] == 0) /* i.e. not known */
	{
		ch->pcdata->learned[sn] = 1;
		if (deduct)
		ch->pcdata->points += skill_table[sn].rating[ch->class];
	}
	return;
	}
	
	/* now check groups */

	gn = group_lookup(name);

	if (gn != -1)
	{
	if (ch->pcdata->group_known[gn] == FALSE)
	{
		ch->pcdata->group_known[gn] = TRUE;
		if (deduct)
		ch->pcdata->points += group_table[gn].rating[ch->class];
	}
	gn_add(ch,gn); /* make sure all skills in the group are known */
	}
}

/* used for processing a skill or group for deletion -- no points back! */

void group_remove(CHAR_DATA *ch, const char *name)
{
	int sn, gn;
	
	 sn = skill_lookup(name);

	if (sn != -1)
	{
	ch->pcdata->learned[sn] = 0;
	return;
	}

	/* now check groups */

	gn = group_lookup(name);

	if (gn != -1 && ch->pcdata->group_known[gn] == TRUE)
	{
	ch->pcdata->group_known[gn] = FALSE;
	gn_remove(ch,gn);  /* be sure to call gn_add on all remaining groups */
	}
}

int get_injury_penalty( CHAR_DATA *ch )
{
    int penalty = 1000 * (ch->max_hit - ch->hit) / UMAX(1, ch->max_hit) - 5 * get_curr_stat(ch,STAT_DIS);
    // check if further reduction is needed at all (for efficiency)
    if ( penalty > 0 )
    {
        if (ch->stance == STANCE_KAMIKAZE || IS_AFFECTED(ch, AFF_BERSERK))
            penalty /= 2;
        else
            penalty -= penalty * get_skill(ch, gsn_ashura) / 200;
    }
    
    return URANGE(0, penalty / 20, 50);
}

int mob_has_skill(CHAR_DATA *ch, int sn)
{
    bool charmed;

    if ( IS_AFFECTED(ch, AFF_CHARM) || IS_SET(ch->act, ACT_PET) )
	charmed = TRUE;
    else
	charmed = FALSE;

    /* if a mob uses an active skill (not ordered), he knows it */
    if ( !charmed
	 && (sn >= 0 && sn < MAX_SKILL)
	 && skill_table[sn].beats > 0 )
	return TRUE;

    /* skills they always have */
    if ( (sn==gsn_shield_block)
	 || (sn==gsn_wrist_shield)
	 || (sn==gsn_dual_wield)
	 || (sn==gsn_two_handed)
	 || (sn==gsn_rescue)
	 || (sn==gsn_flee)
	 || (sn==gsn_sneak)
	 || (sn==gsn_hide)
	 || (sn==-1) )
	return TRUE;

    if ((sn==gsn_backstab) || (sn==gsn_circle))
	return IS_SET(ch->act, ACT_THIEF);
    if ((sn==gsn_lore) || (sn==gsn_arcane_lore))
	return IS_SET(ch->act, ACT_MAGE);
    if ( sn == gsn_enhanced_damage )
	return IS_SET(ch->act, ACT_WARRIOR);
    if ( sn == gsn_focus )
	return IS_SET(ch->act, ACT_MAGE);
    if ( sn == gsn_anatomy )
	return (IS_SET(ch->act, ACT_CLERIC) || IS_SET(ch->act, ACT_THIEF))
	    && !IS_SET(ch->act, ACT_IS_HEALER);
    if ((sn==gsn_duck) || (sn==gsn_burst))
	return IS_SET(ch->act, ACT_GUN);
    

    /* skills by offensive flags */
    if (sn==gsn_dodge)
	return IS_SET(ch->off_flags, OFF_DODGE);
    if (sn==gsn_parry)
	return IS_SET(ch->off_flags, OFF_PARRY);
    if (sn==gsn_second_attack || sn==gsn_third_attack)
	return IS_SET(ch->off_flags, OFF_FAST);

    if ( sn == gsn_brawl || sn == gsn_melee )
	return IS_SET(ch->off_flags, OFF_AREA_ATTACK);
    
    if (sn==gsn_trip)
	return IS_SET(ch->off_flags, OFF_TRIP);
    if (sn==gsn_bash)
	return IS_SET(ch->off_flags, OFF_BASH);
    if (sn==gsn_berserk)
	return IS_SET(ch->off_flags, OFF_BERSERK);
    if (sn==gsn_disarm)
	return (IS_SET(ch->off_flags, OFF_DISARM)
		|| IS_SET(ch->act, ACT_THIEF) || IS_SET(ch->act, ACT_WARRIOR));
    if (sn==gsn_bodyguard)
	return IS_SET(ch->off_flags, OFF_RESCUE);

    // mobs that cast spells via spec_fun normally can do so even while charmed
    if ( skill_table[sn].spell_fun != spell_null )
    {
        char** spell_list = get_spell_list( ch );
        if ( spell_list != NULL )
        {
            int spell;
            for (spell = 0; spell_list[spell] != NULL; spell++)
                if ( sn == skill_lookup(spell_list[spell]) )
                    return TRUE;
        }        
    }
    
    return FALSE;
}

int mob_get_skill(CHAR_DATA *ch, int sn)
{
    int skill = 50 + ch->level / 4;

    if (sn < -1 || sn > MAX_SKILL)
    {
        bug("Bad sn %d in mob_get_skill.",sn);
        return 0;
    }
    
    if ( !mob_has_skill(ch, sn) )
        return 0;

    skill = URANGE(0,skill,100);

    return skill;
}

int get_race_skill( CHAR_DATA *ch, int sn )
{
    int i;
    struct pc_race_type *race;

    /* safety net */
    if ( IS_NPC(ch) )
	return 0;

    /* doppelganger don't get skills of morph_race */
    if ( ch->race == race_doppelganger )
	race = &pc_race_table[race_doppelganger];
    else
	race = get_morph_pc_race_type( ch );

    /* check for racial skill */
    for (i=0; i < race->num_skills; i++)
	if ( (race->skill_gsns[i] == sn) && (ch->level >= race->skill_level[i]) )
	    return race->skill_percent[i];

    return 0;
}

int pc_skill_prac(CHAR_DATA *ch, int sn)
{
	int skill,race_skill,i;

	if (sn == -1) /* shorthand for level based skills */
	{
	    skill = ch->level;
	}

	else if (sn < -1 || sn > MAX_SKILL)
	{
		bug("Bad sn %d in skill_prac.",sn);
		skill = 0;
	}

	if (ch->level < skill_table[sn].skill_level[ch->class])
		skill = 0;
	else
	{
		skill = ch->pcdata->learned[sn];
//		skill = (skill*skill_table[sn].cap[ch->class])/10;
	}
	return URANGE(0,skill,100);
}

int pc_get_skill(CHAR_DATA *ch, int sn)
{
	int skill,race_skill,i;

	if (sn == -1) /* shorthand for level based skills */
	{
	    skill = ch->level;
	}

	else if (sn < -1 || sn > MAX_SKILL)
	{
		bug("Bad sn %d in pc_get_skill.",sn);
		skill = 0;
	}

	if (ch->level < skill_table[sn].skill_level[ch->class])
		skill = 0;
	else
	{
		skill = ch->pcdata->learned[sn];
		skill = (skill*skill_table[sn].cap[ch->class])/10;
	}

	/* adjust for race skill */
	race_skill = get_race_skill( ch, sn );
	if ( race_skill > 0 )
	    skill = skill * (100 - race_skill) / 100 + race_skill * 10;

	if (skill)
	{
		i = 3 * get_curr_stat(ch, skill_table[sn].stat_prime);
		i+= 2 * get_curr_stat(ch, skill_table[sn].stat_second);
		i+= get_curr_stat(ch, skill_table[sn].stat_third);
		skill += i/6-100;

		if (skill>1000)
			skill = skill/2+450;
		else if (skill>750)
			skill = 4*skill/5 + 150;

		if (skill<10)
			skill = 10;
	}

	if (ch->pcdata->condition[COND_DRUNK]>10 && !IS_AFFECTED(ch, AFF_BERSERK))
		skill = 9 * skill / 10;
	if (ch->pcdata->condition[COND_SMOKE]<-1 )
		skill = 9 * skill / 10;

        skill = URANGE(0,skill/10,100);

        return skill;
}


/* for returning skill information */
int get_skill(CHAR_DATA *ch, int sn)
{
	int skill;

	if (IS_NPC(ch))
	    skill = mob_get_skill(ch, sn);
	else
	    skill = pc_get_skill(ch, sn);

    if (ch->daze > 0)
    {
        if (skill_table[sn].spell_fun != spell_null)
            skill /= 2;
        else
            skill = skill * 2/3;
    }
    
    /* encumberance */
    skill = skill * (1000 - get_encumberance(ch)) / 1000;

    /* injury */
    if (sn != gsn_ashura) // needed to avoid infinite recursion
        skill = skill * (100 - get_injury_penalty(ch)) / 100;
    
    /* poison & disease */
    if ( skill > 1 && IS_AFFECTED(ch, AFF_POISON) )
        skill -= 1;    
    if ( skill > 1 && IS_AFFECTED(ch, AFF_PLAGUE) )
        skill -= 1;

    return skill;
}

/* This is used for returning the practiced % of a skill for a player */
int get_skill_prac(CHAR_DATA *ch, int sn)
{
	int skill;

	if (IS_NPC(ch))
	    skill = mob_get_skill(ch, sn);
	else
	    skill = pc_skill_prac(ch, sn);

	return skill;
}

bool check_skill( CHAR_DATA *ch, int sn )
{
    int skill = get_skill( ch, sn );
    return number_percent() <= skill;
}

int get_weapon_skill(CHAR_DATA *ch, int sn)
{
	 int skill, i, max, total;

	 /* -1 is exotic */
	if (IS_NPC(ch))
	{
        if ( IS_SET(ch->act,ACT_NOWEAPON) && sn != gsn_hand_to_hand )
            return 0;
        
	    if (sn == -1)
		skill = ch->level;
	    else
		skill = 40 + ch->level/2;
	    
	    skill = URANGE(0, skill, 100);
	    /* injury */
	    skill = skill * (100 - get_injury_penalty(ch)) / 100;
	}
	
	else
	if (sn == -1)
	{
		max=0;
		total=0;
		for (i=gsn_axe; i<=gsn_whip; i++)
		{
			skill = get_weapon_skill(ch, i);
			total+=skill;
			if (skill>max) max=skill;
		}
		skill=(total/(gsn_whip-gsn_axe)+2*max)/3;
	}
	else
		skill = get_skill(ch, sn);

	return URANGE(0,skill,100);
}



void do_practice( CHAR_DATA *ch, char *argument )
{
	BUFFER *buffer;
   char buf[MAX_STRING_LENGTH];
   int sn;
   int skill;

   if ( IS_NPC(ch) )
	  return;

   if ( argument[0] == '\0' )
   {
	  int col;
	
	  buffer = new_buf();
	  col    = 0;
	  for ( sn = 0; sn < MAX_SKILL; sn++ )
	  {
		 if ( skill_table[sn].name == NULL )
			break;
		 if ( (skill=ch->pcdata->learned[sn])< 1
			|| ch->level < skill_table[sn].skill_level[ch->class] )
			continue;
		
		 sprintf( buf, "%-18s %3d%%  ",
			skill_table[sn].name, skill );
		 add_buf(buffer, buf);
		 if ( ++col % 3 == 0 )
			add_buf(buffer, "\n\r");
	  }
	
	  if ( col % 3 != 0 )
		 add_buf(buffer, "\n\r");
	
	  sprintf(buf, "You have %d practice sessions left.\n\r",
		 ch->practice);
	  add_buf(buffer, buf);

	 page_to_char(buf_string(buffer),ch);
	 free_buf(buffer);
   }
   else
   {
	CHAR_DATA *mob;
	  int adept;
	
	  if ( !IS_AWAKE(ch) )
	  {
		 send_to_char( "In your dreams, or what?\n\r", ch );
		 return;
	  }
	
	  for ( mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room )
	  {
		 if ( IS_NPC(mob) && IS_SET(mob->act, ACT_PRACTICE) )
			break;
	  }

   if ( mob == NULL )
   {
	if (get_skill(ch, gsn_introspection) > 1)
	{
	      act( "$n thinks over what $e has experienced recently.", ch, NULL, NULL, TO_ROOM);
	   if ((get_skill(ch,gsn_introspection)) > number_percent ())
	      check_improve(ch,gsn_introspection,TRUE,8);
	   else
	   {
	      send_to_char("You've learned nothing from your recent experiences.\n\r",ch);
	      check_improve(ch,gsn_introspection,FALSE,8);
	      return;
	   }
	}
        else
	{
	  send_to_char( "You can't do that here.\n\r", ch );
	  return;
	}
   }
	
	  if ( ch->practice <= 0 )
	  {
		 send_to_char( "You have no practice sessions left.\n\r", ch );
		 return;
	  }

	if (!str_cmp("field",argument))
	{
            /* no more burning practices when you don't have field */
            if (ch->pcdata->field < 100)
            {
                send_to_char("You don't have enough field experience to practice.\n\r",ch);
                return;
            }
            else
            {
		ch->practice--;
		sn = number_range(ch->pcdata->field/2, ch->pcdata->field);
		ch->pcdata->field-=sn;
		gain_exp(ch, sn);
		return;
            }
	}
	
	  if ( ( sn = find_spell( ch,argument ) ) < 0
		 || ( !IS_NPC(ch)
		 &&   (ch->level < skill_table[sn].skill_level[ch->class]
		 ||    ch->pcdata->learned[sn] < 1 /* skill is not known */
		 ||    skill_table[sn].rating[ch->class] == 0)))
	  {
		 send_to_char( "You can't practice that.\n\r", ch );
		 return;
	  }
	
	  adept = IS_NPC(ch) ? 100 : class_table[ch->class].skill_adept;
	
	  if ( ch->pcdata->learned[sn] >= adept )
	  {
		 sprintf( buf, "You are already learned at %s.\n\r",
			skill_table[sn].name );
		 send_to_char( buf, ch );
	  }
	  else
	  {
		  WAIT_STATE(ch, 8);
		ch->practice--;
		 ch->pcdata->learned[sn] +=
			ch_int_learn(ch) / skill_table[sn].rating[ch->class];
		 if ( ch->pcdata->learned[sn] < adept )
		 {
			act( "You practice $T.",
			   ch, NULL, skill_table[sn].name, TO_CHAR );
			act( "$n practices $T.",
			   ch, NULL, skill_table[sn].name, TO_ROOM );
		 }
		 else
		 {
			ch->pcdata->learned[sn] = adept;
			act( "You are now learned at $T.",
			   ch, NULL, skill_table[sn].name, TO_CHAR );
			act( "$n is now learned at $T.",
			   ch, NULL, skill_table[sn].name, TO_ROOM );
		 }
	  }
   }
   return;
}


void do_raceskills( CHAR_DATA *ch, char *argument )
{
	int race, i;
	char buf[MAX_STRING_LENGTH];

	if (argument[0]=='\0')
		race=ch->race;
	else
		race=race_lookup(argument);

	if (race==0)
	{
		send_to_char("Which race do you want to know the skills for?\n\r", ch);
		return;
	}

	if (!race_table[race].pc_race)
	{
		send_to_char("That is not a valid player race.\n\r", ch );
		return;
	}

	if (pc_race_table[race].num_skills == 0)
	{
		send_to_char("No racial skills.\n\r", ch);
		return;
	}

	send_to_char("Skill                 Level  Percent\n\r",ch);

	for (i=0; i<pc_race_table[race].num_skills; i++)
	{
		sprintf(buf, "%20s  %5d  %5d%%\n\r",
			pc_race_table[race].skills[i],
			pc_race_table[race].skill_level[i],
			pc_race_table[race].skill_percent[i]);
		send_to_char(buf, ch);
	}

	return;
}

/********************************************************
 * Show skills code by Keridan of Exile: The Crusades   *
 * This code is released under no license and is freely *
 * distributable. I require no credit, but would        *
 * appreciate a quick note to keridan@exile.mudsrus.com *
 * if you choose to use it in your mud.                 *
 ********************************************************/

/* Color, group support, buffers and other tweaks by Rimbol, 10/99. */
void show_skill(char *argument, BUFFER *buffer);
void show_skill_all(BUFFER *buffer);
void do_showskill(CHAR_DATA *ch,char *argument)
{
    char arg1[MIL];
    int skill, group = -1;
    BUFFER *buffer;
    bool show_all = FALSE;
    
    /*argument = one_argument(argument,arg1);*/
    strcpy(arg1, argument);

    if (argument[0] == '\0')
    { 
        printf_to_char(ch,"Syntax: showskill <spell/skill name>\n\r");
        printf_to_char(ch,"        showskill all\n\r");
        return; 
    }

    if ( str_cmp(argument, "all") == 0 )
        show_all = TRUE;
    else
    {
        if ( (skill = skill_lookup(arg1)) == -1 && (group = group_lookup(arg1)) == -1 )
        { 
            printf_to_char(ch,"Skill not found.\n\r");
            return; 
        }
    }
    
    buffer = new_buf();
    
    if ( show_all )
    {
        show_skill_all(buffer);
    }
    else if (group > 0)  /* Argument was a valid group name. */
    {
        int sn;
        
        printf_to_char(ch, "{cGroup:  {Y%s{x\n\r\n\r", capitalize(group_table[group].name));
        
        for (sn = 0; sn < MAX_IN_GROUP; sn++)
        {
            if (group_table[group].spells[sn] == NULL)
                break;
            
            show_skill(group_table[group].spells[sn], buffer);
            add_buf( buffer, "\n\r" );
        }
    }
    else           /* Argument was a valid skill/spell/stance name */
    {
        show_skill(arg1, buffer);
        show_groups(skill, buffer);
        show_races(skill, buffer);
    }
    
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    
    return;
}

bool is_in_group( int skill, int group )
{
    int sn;
    for (sn = 0; sn < MAX_IN_GROUP; sn++)
    {
	if (group_table[group].spells[sn] == NULL)
	    break;
	if ( !str_cmp(group_table[group].spells[sn], skill_table[skill].name) )
	    return TRUE;
    }
    return FALSE;
}

void show_groups( int skill, BUFFER *buffer )
{
    char buf[MSL];
    int gn, col = 0;

    add_buf( buffer, "\n\rIt is contained in the following groups:\n\r" );
    for (gn = 0; gn < MAX_GROUP; gn++)
    {
	if ( group_table[gn].name == NULL )
	    break;
	if ( !is_in_group(skill, gn) )
	    continue;

	sprintf( buf, "%-20s ", group_table[gn].name );
	add_buf( buffer, buf );
	if (++col % 3 == 0)
	    add_buf( buffer, "\n\r" );
    }
    if ( col % 3 != 0 )
	add_buf( buffer, "\n\r" );
}

bool has_race_skill( int skill, int rn )
{
    int i;

    for (i=0; i < pc_race_table[rn].num_skills; i++)
	if ( (pc_race_table[rn].skill_gsns[i] == skill) )
	    return TRUE;

    return FALSE;
}

void show_races( int skill, BUFFER *buffer )
{
    char buf[MSL];
    int rn, col = 0;

    add_buf( buffer, "\n\rIt is possessed by the following races:\n\r" );
    for (rn = 0; rn < MAX_PC_RACE; rn++)
    {
	if ( pc_race_table[rn].name == NULL )
	    break;
	if ( !has_race_skill(skill, rn) )
	    continue;

	sprintf( buf, "%-20s ", pc_race_table[rn].name );
	add_buf( buffer, buf );
	if (++col % 3 == 0)
	    add_buf( buffer, "\n\r" );
    }
    if ( col % 3 != 0 )
	add_buf( buffer, "\n\r" );
}

void show_skill(char *argument, BUFFER *buffer)
{
    int skill, cls = 0;
    int stance;
    bool is_spell;

    if (buffer == NULL)
        return;

    if ((skill = skill_lookup(argument)) == -1)        
    { 
        add_buff(buffer,"Skill not found.\n\r");
        return; 
    }
    
    is_spell = IS_SPELL(skill);
    add_buff(buffer, "{cSettings for %s:  {Y%s{x\n\r", (is_spell ? "spell" : "skill"), capitalize(skill_table[skill].name));

    /* check if skill is a stance */
    for (stance = 0; stances[stance].gsn != NULL; stance++)
        if (stances[stance].gsn == skill_table[skill].pgsn)
            break;

    if ( is_spell )
    {
        add_buff( buffer, "Base Mana: %d  Lag: %d  Duration: %s\n\r",
            skill_table[skill].min_mana, skill_table[skill].beats,
            spell_duration_names[skill_table[skill].duration] );
        add_buff( buffer, "Target: %s  Combat: %s\n\r",
            spell_target_names[skill_table[skill].target],
            skill_table[skill].minimum_position <= POS_FIGHTING ? "yes" : "no" );
    }
    else if (stances[stance].cost != 0)
        add_buff(buffer, "Base Move: %d\n\r", stances[stance].cost);
    else
    {
        bool found = FALSE;
        if (skill_table[skill].min_mana != 0)
        {
            add_buff(buffer, "Base Mana: %d", skill_table[skill].min_mana);
            found = TRUE;
        }
        if (skill_table[skill].beats != 0)
        {
            add_buff(buffer, "%sLag: %d", (found ? "  " : ""), skill_table[skill].beats);
            found = TRUE;
        }
        if (skill_table[skill].duration != DUR_NONE)
        {
            add_buff(buffer, "%sDuration: %s", (found ? "  " : ""), spell_duration_names[skill_table[skill].duration]);
            found = TRUE;
        }
        if (found)
            add_buff(buffer, "\n\r");
    }
    
    add_buff(buffer, "Prime Stat: %s   Second Stat: %s   Third Stat: %s\n\r",
        (skill_table[skill].stat_prime>=STAT_NONE) ? "none" :
        stat_table[skill_table[skill].stat_prime].name,
        (skill_table[skill].stat_second>=STAT_NONE) ? "none" :
        stat_table[skill_table[skill].stat_second].name,
        (skill_table[skill].stat_third>=STAT_NONE) ? "none" :
        stat_table[skill_table[skill].stat_third].name);
    
    add_buff(buffer, "\n\r{wClass          Level Points  Max{x\n\r");
    
    add_buff(buffer, "{w------------   ----- ------ -----{x\n\r");
    
    for ( cls = 0; cls < MAX_CLASS; cls++ )
    {
        if (skill_table[skill].skill_level[cls] >= HERO)
        {
            sprintf(log_buf, "{r   --     --     --{x\n\r");
        }
        else
        {
                sprintf( log_buf, "{g%5d    %3d    %3d{x\n\r",
                    skill_table[skill].skill_level[cls],
                    skill_table[skill].rating[cls],
                    skill_table[skill].cap[cls] );
        }
        
        add_buff(buffer, "{w%-12s{x %-5s", capitalize(class_table[cls].name), log_buf);
    }
}

void show_skill_all(BUFFER *buffer)
{
    int skill;

    if (buffer == NULL)
        return;

    add_buff(buffer, "Skill/Spell         Cost    Lag     Duration    Combat  Target\n\r");
    add_buff(buffer, "==============================================================\n\r");
    
    for ( skill = 1; skill_table[skill].name != NULL; skill++ )
    {
        int cost = skill_table[skill].min_mana;
        // check if skill is a stance - if so, display stance cost
        if ( cost == 0 )
        {
            int stance;
            for (stance = 0; stances[stance].gsn != NULL; stance++)
                if (stances[stance].gsn == skill_table[skill].pgsn)
                {
                    cost = stances[stance].cost;
                    break;
                }
        }
        add_buff( buffer, "%-20.20s %3d %6d     %-11.11s %-7.7s %s\n\r",
            skill_table[skill].name,
            cost,
            skill_table[skill].beats,
            spell_duration_names[skill_table[skill].duration],
            skill_table[skill].minimum_position <= POS_FIGHTING ? "yes" : "no",
            spell_target_names[skill_table[skill].target]
        );
    }            
}

extern int dice_lookup(char *);
void do_setskill(CHAR_DATA *ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char arg4[MAX_INPUT_LENGTH];
	int sn, field, val1, val2, class;

	argument=one_argument(argument, arg1);
	argument=one_argument(argument, arg2);
	argument=one_argument(argument, arg3);

	if ((arg1[0]==0) || (arg2[0]==0) || (arg3[0]==0))
	{
		send_to_char("Syntax: setskill <skill> level/points/max <class|all> <value>\n\r", ch);
		send_to_char("Syntax: setskill <skill> prime/second/third <stat>\n\r", ch);
		send_to_char("Syntax: setskill <skill> lag/mana <value>\n\r", ch);
		return;
	}

	if ((sn=skill_lookup(arg1)) < 0)
	{
		send_to_char("No such skill or spell.\n\r", ch);
		return;
	}

	if (!strcmp(arg2, "level"))
		field=1;
	else if (!strcmp(arg2, "points"))
		field=2;
	else if (!strcmp(arg2, "max"))
		field=3;
	else if (!strcmp(arg2, "prime"))
		field=4;
	else if (!strcmp(arg2, "second"))
		field=5;
	else if (!strcmp(arg2, "third"))
		field=6;
	else if (!strcmp(arg2, "lag"))
		field=7;
	else if (!strcmp(arg2, "mana"))
		field=8;
	else
	{
		send_to_char("Syntax: setskill <skill> level/points/max <class|all> <value>\n\r", ch);
		send_to_char("Syntax: setskill <skill> prime/second/third <stat>\n\r", ch);
		send_to_char("Syntax: setskill <skill> lag/mana <value>\n\r", ch);
		return;
	}

	if (field<4)
	{
        if ( !strcmp(arg3, "all") )
            val1 = -1;
        else
            if ((val1=class_lookup(arg3)) < 0)
            {
                send_to_char("Invalid class.\n\r", ch);
                return;
            }

		one_argument(argument, arg4);
		if ((arg4[0]==0) || !is_number(arg4) ||
				((val2=atoi(arg4))<0) || (val2>110))
		{
			send_to_char("Invalid value.\n\r", ch);
			return;
		}

        for ( class = 0; class < MAX_CLASS; class++ )
        {
            if ( val1 != -1 && class != val1 )
                continue;
            switch ( field )
            {
                case 1: skill_table[sn].skill_level[class] = val2; break;
                case 2: skill_table[sn].rating[class] = val2; break;
                case 3: skill_table[sn].cap[class] = val2; break;
            }
        }
	}
	else if (field<7)
	{
		if ((val1=dice_lookup(arg3)) >= STAT_NONE)
		{
			send_to_char("Invalid statistic.\n\r", ch);
			return;
		}

		if (field==4)
			skill_table[sn].stat_prime = val1;
		else if (field==5)
			skill_table[sn].stat_second = val1;
		else if (field==6)
			skill_table[sn].stat_third = val1;
	}
	else
	{
		if (!is_number(arg3) || ((val1=atoi(arg3))<0) || (val1>1000))
		{
			send_to_char("Invalid value.\n\r", ch);
			return;
		}

		if (field==7)
			skill_table[sn].beats = val1;
		else if (field==8)
			skill_table[sn].min_mana = val1;
	}

    update_group_costs();
	send_to_char("OK.\n\r", ch);
}


void do_slookup( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int sn;
    
    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Lookup which skill or spell?\n\r", ch );
        return;
    }
    
    if ( !str_cmp( arg, "all" ) )
    {
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name == NULL )
                break;
            sprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
                sn, skill_table[sn].slot, skill_table[sn].name );
            send_to_char( buf, ch );
        }
    }
    else
    {
        if ( ( sn = skill_lookup( arg ) ) < 0 )
        {
            send_to_char( "No such skill or spell.\n\r", ch );
            return;
        }
        
        sprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
            sn, skill_table[sn].slot, skill_table[sn].name );
        send_to_char( buf, ch );
    }
    
    return;
}

void save_skill(FILE *f, int sn)
{
	int i;

	fprintf(f, "\n%s\n", skill_table[sn].name);
	fprintf(f, "%d %d\n", skill_table[sn].beats, skill_table[sn].min_mana);

	for (i=0; i<MAX_CLASS; i++)
		fprintf(f, "%3d ", skill_table[sn].skill_level[i]);
	fprintf(f, "\n");

	for (i=0; i<MAX_CLASS; i++)
		fprintf(f, "%3d ", skill_table[sn].rating[i]);
	fprintf(f, "\n");

	for (i=0; i<MAX_CLASS; i++)
		fprintf(f, "%3d ", skill_table[sn].cap[i]);
	fprintf(f, "\n");

	fprintf(f, "%d %d %d\n", skill_table[sn].stat_prime,
			skill_table[sn].stat_second, skill_table[sn].stat_third);
}

void save_skills()
{
	FILE *f;
	int i, n;

	f = fopen(SKILL_FILE, "w");
	if (f==NULL)
	{
		bug("Could not open " SKILL_FILE " for writing.",0);
		return;
	}

	for (n=0; skill_table[n].name; n++);

	fprintf(f, "%d %d\n", n, MAX_CLASS);

	for (i=0; i<n; i++)
		save_skill(f, i);

	fclose(f);
}


void count_stat(FILE *f, int sn)
{
	int i;

	fprintf(f, "\n%d %s, ",sn, skill_table[sn].name);
	fprintf(f, "%d, %d, %d\n", skill_table[sn].stat_prime,
			skill_table[sn].stat_second, skill_table[sn].stat_third);
}

void count_stats()
{
	FILE *f;
	int i, n;

	f = fopen(STAT_FILE, "w");
	if (f==NULL)
	{
		bug("Could not open STAT_FILE for writing.",0);
		return;
	}

	for (n=0; skill_table[n].name; n++);

	fprintf(f, "%d %d\n", n, MAX_CLASS);

	for (i=0; i<n; i++)
		count_stat(f, i);

	fclose(f);
}



void load_skill(FILE *f, int cnum)
{
	char buf[81];
	char *s;
	int i, sn;

	do
		s=fgets(buf, 80, f);
	while (s && (*s<=' '));
	if (s==NULL) return;

	for (i=0; s[i]>=' '; i++);
	s[i]='\0';

	if ((sn=skill_lookup(s))<0)
	{
		bugf("Bad skill in skill.txt: %s.", s);
		sn=0;
	}

	fscanf(f, "%hd %hd", &(skill_table[sn].beats), &(skill_table[sn].min_mana));

	for (i=0; i<cnum; i++)
		fscanf(f, "%hd", &(skill_table[sn].skill_level[i]));

	for (i=0; i<cnum; i++)
		fscanf(f, "%hd", &(skill_table[sn].rating[i]));

	for (i=0; i<cnum; i++)
		fscanf(f, "%hd", &(skill_table[sn].cap[i]));

	fscanf(f, "%hd %hd %hd", &(skill_table[sn].stat_prime),
			&(skill_table[sn].stat_second), &(skill_table[sn].stat_third));
}

void load_skills()
{
	FILE *f;
	int i, n, cnum;

	f = fopen(SKILL_FILE, "r");
	if (f==NULL)
	{
		bug("Could not open " SKILL_FILE " for reading.",0);
		exit(1);
	}

	fscanf(f, "%d %d", &n, &cnum);

	for (i=0; i<n; i++)
		load_skill(f, cnum);

	fclose(f);
}

