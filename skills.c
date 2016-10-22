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

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "special.h"

/* command procedures needed */
DECLARE_DO_FUN(do_groups    );
DECLARE_DO_FUN(do_help      );
DECLARE_DO_FUN(do_say       );

DECLARE_DO_FUN(do_skills);
bool train_stat(int trained, CHAR_DATA *ch);
void show_groups( int skill, BUFFER *buffer );
void show_races( int skill, BUFFER *buffer );
void show_skill_subclasses( int skill, BUFFER *buffer );
void show_class_skills( CHAR_DATA *ch, const char *argument );
void show_skill_points( BUFFER *buffer );
void show_mastery_groups( int skill, BUFFER *buffer );
int get_injury_penalty( CHAR_DATA *ch );
int get_sickness_penalty( CHAR_DATA *ch );
static int hprac_cost( CHAR_DATA *ch, int sn );
int mob_get_skill( CHAR_DATA *ch, int sn );

bool is_class_skill( int class, int sn )
{
    return skill_table[sn].rating[class] > 0 && skill_table[sn].skill_level[class] < LEVEL_IMMORTAL;
}

bool can_gain_skill( CHAR_DATA *ch, int sn )
{
    return is_class_skill(ch->class, sn);
}

// populate skill_costs recursively
static void set_group_skill_costs( int gn, int class, int *skill_costs )
{
    int i;
    for ( i = 0; i < MAX_IN_GROUP; i++ )
    {
        const char *name = group_table[gn].spells[i];
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
        const char *name = group_table[gn].spells[i];
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

// update cost for all skills based on min_rating
void update_skill_costs()
{
    int sn, class;
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        int min_rating = skill_table[sn].min_rating;
        for ( class = 0; class < MAX_CLASS; class++ )
        {
            int cap = skill_table[sn].cap[class];
            skill_table[sn].rating[class] = min_rating + (cap < 80 ? 2 : cap < 90 ? 1 : 0);
        }
    }
}

// check for issues with skills - checked during startup
void verify_skills()
{
    int sn, class, race, subclass, skill;
    // check level gained
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        for ( class = 0; class < MAX_CLASS; class++ )
        {
            int level = skill_table[sn].skill_level[class];
            if ( level < 1 || level > 102 )
                logpf("Skill %s gained at level %d (%s)", skill_table[sn].name, level, class_table[class].name);
        }
    }
    // make sure all race/subclass skills exist
    for ( race = 0; pc_race_table[race].name != NULL; race++ )
        for ( skill = 0; skill < pc_race_table[race].num_skills; skill++ )
        {
            const char *skill_name = pc_race_table[race].skills[skill];
            if ( skill_lookup_exact(skill_name) == -1 )
                logpf("Unknown skill %s for race %s.", skill_name, pc_race_table[race].name);
        }
    for ( subclass = 1; subclass_table[subclass].name != NULL; subclass++ )
        for ( skill = 0; skill < MAX_SUBCLASS_SKILL; skill++ )
        {
            const char *skill_name = subclass_table[subclass].skills[skill];
            if ( skill_name && skill_lookup_exact(skill_name) == -1 )
                logpf("Unknown skill %s for subclass %s.", skill_name, subclass_table[subclass].name);
        }
}

CHAR_DATA* find_trainer( CHAR_DATA *ch, int act_flag, bool *introspect )
{
    CHAR_DATA *trainer;

    for ( trainer = ch->in_room->people; trainer != NULL; trainer = trainer->next_in_room )
        if ( IS_NPC(trainer) && IS_SET(trainer->act, act_flag) && can_see(ch, trainer) )
        {
            *introspect = FALSE;
            return trainer;
        }
    
    // no trainer, try introspection
    int skill = get_skill(ch, gsn_introspection);
    if ( skill > 1 && *introspect )
    {
        if ( (*introspect = per_chance(skill)) )
        {
            act("$n thinks over what $e has experienced recently.", ch, NULL, NULL, TO_ROOM);
            check_improve(ch, gsn_introspection, TRUE, 3);
        }
        else
        {
            send_to_char("You find nothing meaningful in your introspection.\n\r", ch);
            check_improve(ch, gsn_introspection, FALSE, 3);
        }
    }
    else
    {
        *introspect = FALSE;
        send_to_char( "You need a trainer for this. Visit your guild.\n\r", ch );
    }

    return NULL;
}

void gain_skill(CHAR_DATA *ch, int sn, CHAR_DATA *trainer)
{
    if ( ch->pcdata->learned[sn] )
    {
        if ( trainer )
            act("$N tells you 'You already know that skill!'", ch, NULL, trainer, TO_CHAR);
        else
            send_to_char("You already know that skill.\n\r", ch);
        return;
    }

    if ( !can_gain_skill(ch, sn) )
    {
        if ( trainer )
            act("$N tells you 'That skill is beyond your powers.'", ch, NULL, trainer, TO_CHAR);
        else
            send_to_char("That skill is beyond your powers.\n\r", ch);
        return;
    }

    int cost = skill_table[sn].rating[ch->class];
    if ( ch->train < cost )
    {
        if ( trainer )
            act("$N tells you 'You are not yet ready for that skill.'", ch, NULL, trainer, TO_CHAR);
        else
            send_to_char("You aren't ready for that skill.\n\r", ch);
        return;
    }

    if ( trainer )
        act("$N trains you in the art of $t.", ch, skill_table[sn].name, trainer, TO_CHAR);
    else
        act("You learn the art of $t.", ch, skill_table[sn].name, NULL, TO_CHAR);

    ch->pcdata->learned[sn] = 1;
    ch->train -= cost;
}


/* used to get new skills */
DEF_DO_FUN(do_gain)
{
    char buf[MAX_STRING_LENGTH];
    const char *argPtr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *trainer;
    int gn = 0, sn = 0;
    int seq;
    bool introspect = TRUE;
    
    if (IS_NPC(ch))
        return;

    trainer = find_trainer(ch, ACT_GAIN, &introspect);
    if ( !trainer && !introspect )
        return;
    
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
#ifdef TESTER
        else if ( !strcmp(arg, "all") )
        {
            int target = is_number(arg2) ? URANGE(1, atoi(arg2), 100) : 75;
            logpf("%s gained and practiced all skills at %d%%.", ch->name, target);
            for ( gn = 0; gn < MAX_GROUP; gn++ )
                if ( group_table[gn].rating[ch->class] > 0 && ch->pcdata->group_known[gn] == 0 )
                {
                    ptc(ch, "You learn %s.\n\r", group_table[gn].name);
                    gn_add(ch, gn);
                }
            for ( sn = 0; sn < MAX_SKILL; sn++ )
                if ( can_gain_skill(ch, sn) && ch->pcdata->learned[sn] < target )
                {
                    ptc(ch, "You practice %s.\n\r", skill_table[sn].name);
                    ch->pcdata->learned[sn] = target;
                }
            ptc(ch, "Done.\n\r");
            return;
        }
#endif
        else if (!str_prefix(arg,"list"))
        {
            int col;
            col = 0;
            sprintf(buf,"{c%-10s {w%13s   {c%-10s {w%13s   {c%-10s {w%13s{x\n\r",
                "group","trains","group","trains","group","trains");
            send_to_char(buf,ch);
            sprintf(buf,"{w------------------------   ------------------------   ------------------------{x\n\r");
            send_to_char(buf,ch);

            for ( seq=0
                    ; (gn=name_sorted_group_table(seq)) != -1
                    ; seq++ )
            {
                if (group_table[gn].name == NULL)
                    break;
                if (!ch->pcdata->group_known[gn] &&  group_table[gn].rating[ch->class] > 0)
                {
                    sprintf(buf,"%-18s %5d   ",group_table[gn].name, get_group_cost(ch, gn));
                    send_to_char(buf,ch);
                    if (++col % 3 == 0)
                        send_to_char("\n\r",ch);
                }
            }
            if (col % 3 != 0)
                send_to_char("\n\r",ch);
            send_to_char("\n\r",ch);
            col = 0;
            
            sprintf(buf,"{c%-10s {w%13s   {c%-10s {w%13s   {c%-10s {w%13s{x\n\r",
                "skill","lvl/tr","skill","lvl/tr","skill","lvl/tr");
            send_to_char(buf,ch);
            sprintf(buf,"{w------------------------   ------------------------   ------------------------{x\n\r");
            send_to_char(buf,ch);
            
            for ( seq=0
                    ; (sn=name_sorted_skill_table(seq)) != -1
                    ; seq++ )
            {
                if (skill_table[sn].name == NULL)
                    break;
                if ( !ch->pcdata->learned[sn] 
                    && can_gain_skill(ch, sn)
                    &&  skill_table[sn].spell_fun == spell_null)
                {
                    sprintf(buf,"%-17s %3d/%2d   ",
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

            col = 0;

            sprintf(buf,"{c%-10s {w%13s   {c%-10s {w%13s   {c%-10s {w%13s{x\n\r",
                "spell","lvl/tr","spell","lvl/tr","spell","lvl/tr");
            send_to_char(buf,ch);
            sprintf(buf,"{w------------------------   ------------------------   ------------------------{x\n\r");
            send_to_char(buf,ch);
    
            for ( seq=0
                ; (sn=name_sorted_skill_table(seq)) != -1
                ; seq++ )
            {
                if (skill_table[sn].name == NULL)
                    break;
        
                if ( !ch->pcdata->learned[sn]
                    &&  skill_table[sn].spell_fun != spell_null
                    &&  can_gain_skill(ch, sn) )
                {
                    sprintf(buf,"%-17s %3d/%2d   ",
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
        else if ( (sn = skill_lookup_exact(argument)) > 0 )
        {
            gain_skill(ch, sn, trainer);
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
        else if ( (sn = class_skill_lookup(ch->class, argument)) > 0 )
        {
            gain_skill(ch, sn, trainer);
            return;
        }
        if ( introspect )
            send_to_char( "See HELP GAIN.\n\r", ch );
        else
            act("$N tells you 'I do not understand...'",ch,NULL,trainer,TO_CHAR);
        
}
int get_mastery( CHAR_DATA *ch, int sn )
{
    if ( !ch || !ch->pcdata || sn < 0 || sn >= MAX_SKILL )
        return 0;
    return ch->pcdata->mastered[sn];
}

int mastery_bonus( CHAR_DATA *ch, int sn, int m_bonus, int gm_bonus )
{
    switch ( get_mastery(ch, sn) )
    {
        case 1: return m_bonus;
        case 2: return gm_bonus;
        default: return 0;
    }
}

static int get_group_mastery( CHAR_DATA *ch, int gn )
{
    int i, sn, mastery = 0;
    for ( i = 0; mastery_group_table[gn].skills[i]; i++ )
    {
        sn = skill_lookup_exact(mastery_group_table[gn].skills[i]);
        if ( sn < 0 )
            bugf("Unknown skill %s in mastery group %s.", mastery_group_table[gn].skills[i], mastery_group_table[gn].name);
        else
            mastery = UMAX(mastery, ch->pcdata->mastered[sn]);
    }
    return mastery;
}

static bool is_in_mastery_group( int sn, int gn )
{
    int i;
    for ( i = 0; mastery_group_table[gn].skills[i]; i++ )
        if ( !strcmp(mastery_group_table[gn].skills[i], skill_table[sn].name) )
            return TRUE;
    return FALSE;
}

#define MAX_MASTERY_GROUP 100
// get array of all mastery groups the given skill belongs to, 0-terminated
static int* get_mastery_groups( int sn )
{
    static int groups[MAX_MASTERY_GROUP];
    int gn, count = 0;
    for ( gn = 1; mastery_group_table[gn].name; gn++ )
        if ( is_in_mastery_group(sn, gn) )
            groups[count++] = gn;
    // terminate array
    groups[count] = 0;
    return groups;
}
#undef MAX_MASTERY_GROUP

// additional cost from mastery groups to advance the given skill
static int get_mastery_group_cost( CHAR_DATA *ch, int sn )
{
    int i, gn, cost = 0;
    int mastery = get_mastery(ch, sn);
    for ( gn = 1; mastery_group_table[gn].name; gn++ )
        for ( i = 0; mastery_group_table[gn].skills[i]; i++ )
            if ( !strcmp(mastery_group_table[gn].skills[i], skill_table[sn].name) && get_group_mastery(ch, gn) <= mastery )
                cost += mastery_group_table[gn].rating;
    return cost;
}

static int mastery_points( CHAR_DATA *ch )
{
    int sn, gn, points = 0;
    // points for skills
    for ( sn = 1; sn < MAX_SKILL; sn++ )
        points += ch->pcdata->mastered[sn] * skill_table[sn].mastery_rating;
    // extra points for groups
    for ( gn = 1; mastery_group_table[gn].name; gn++ )
        points += get_group_mastery(ch, gn) * mastery_group_table[gn].rating;
    return points;
}

static int max_mastery_points( CHAR_DATA *ch )
{
    if ( ch->pcdata->ascents > 0 )
        return 40 + 10 * UMIN(5, ch->pcdata->ascents) + ch->pcdata->remorts;
    else
        return 30 + 2 * ch->pcdata->remorts;
}

static const char* mastery_title( int level )
{
    switch( level )
    {
        case 2: return "grandmaster";
        case 1: return "master";
        default: return "(BUG)";
    }
}

static int max_mastery_class( int class, int sn )
{
    // skill must have a mastery_rating
    if ( skill_table[sn].mastery_rating < 1 )
        return -1;
    
    int class_max = skill_table[sn].cap[class];

    if ( class_max < 80 || skill_table[sn].skill_level[class] > LEVEL_HERO )
        return 0;
    else if ( class_max < 90 )
        return 1;
    else
        return 2;
}

static int max_mastery_level( CHAR_DATA *ch, int sn )
{
    int level = max_mastery_class(ch->class, sn);
    int practice = ch->pcdata->learned[sn];
    
    if ( practice < 80 )
        return UMIN(level, 0);
    else if ( practice < 90 )
        return UMIN(level, 1);
    else
        return UMIN(level, 2);
}

static void show_master_syntax( CHAR_DATA *ch )
{
    send_to_char("Syntax: master <skill>\n\r", ch);
    send_to_char("        master forget <skill>\n\r", ch);
    send_to_char("        master retrain <skill>\n\r", ch);
    send_to_char("        master list\n\r", ch);
}

static void show_master_list( CHAR_DATA *ch )
{
    BUFFER *buf = new_buf();
    int sn, gn, lvl;

    add_buf(buf, "{gYou have mastered the following skills:{x\n\r");
    add_buf(buf, "{WSkill/Spell            School               Rank            Cost{x\n\r");
    for ( sn = 1; sn < MAX_SKILL; sn++ )
        if ( (lvl = ch->pcdata->mastered[sn]) > 0 )
        {
            int *groups = get_mastery_groups(sn);
            add_buff(buf, "  %-20s %-20s %-15s %2d\n\r",
                skill_table[sn].name,
                *groups ? mastery_group_table[*groups].name : "",
                mastery_title(lvl),
                skill_table[sn].mastery_rating * lvl
            );
            // may belong to more than one school
            while ( *groups && *(++groups) )
                add_buff(buf, "  %-20s %-20s\n\r", "", mastery_group_table[*groups].name);
        }

    add_buf(buf, "\n\r{gYou have mastered the following schools:{x\n\r");
    for ( gn = 1; mastery_group_table[gn].name; gn++ )
        if ( (lvl = get_group_mastery(ch, gn)) > 0 )
            add_buff(buf, "  %-20s %-20s %-15s %2d\n\r",
                "",
                mastery_group_table[gn].name,
                mastery_title(lvl),
                mastery_group_table[gn].rating * lvl
            );

    add_buff(buf, "\n\r{gTrains spent on skill/school mastery:{x %24d / %d\n\r",
        mastery_points(ch), max_mastery_points(ch));

    add_buf(buf, "\n\r{gYou may advance in the following skills:{x\n\r");
    for ( sn = 1; sn < MAX_SKILL; sn++ )
        if ( (lvl = ch->pcdata->mastered[sn]) < max_mastery_level(ch, sn) )
        {
            int *groups = get_mastery_groups(sn);
            add_buff(buf, "  %-20s %-20s %-15s %2d\n\r",
                skill_table[sn].name,
                *groups ? mastery_group_table[*groups].name : "",
                mastery_title(lvl+1),
                skill_table[sn].mastery_rating + get_mastery_group_cost(ch, sn)
            );
            // may belong to more than one school
            while ( *groups && *(++groups) )
                add_buff(buf, "  %-20s %-20s\n\r", "", mastery_group_table[*groups].name);
        }

    page_to_char(buf_string(buf), ch);
    free_buf(buf);
}

DEF_DO_FUN(do_master)
{
    char arg[MAX_INPUT_LENGTH];
    const char *arg2;
    int sn;
    
    if (IS_NPC(ch))
        return;

    arg2 = one_argument( argument, arg );

    if ( !strcmp(arg, "list") || !strcmp(arg, "") )
    {
        show_master_list(ch);
        return;
    }
    else if ( ch->position < POS_STANDING )
    {
        send_position_message(ch);
        return;
    }
    else if ( !strcmp(arg, "forget") )
    {
        // forget all option for debugging - hence imm only
        if ( !strcmp(arg2, "all") && IS_IMMORTAL(ch) )
        {
            printf_to_char(ch, "You forget all your masteries.\n\r");
            for ( sn = 1; sn < MAX_SKILL; sn++ )
                ch->pcdata->mastered[sn] = 0;
            return;
        }
        
        if ( (sn = skill_lookup(arg2)) <= 0 )
        {
            printf_to_char(ch, "Invalid skill '%s'.\n\r", arg2);
            return;
        }
        if ( !ch->pcdata->mastered[sn] )
        {
            printf_to_char(ch, "Seems you forgot about it already.\n\r", arg2);
            return;
        }
        // ok, reduce mastery level
        printf_to_char(ch, "You forget some of the finer details about %s.\n\r", skill_table[sn].name);
        ch->pcdata->mastered[sn]--;
        ch->pcdata->smc_retrained++;
        check_achievement(ch);
        return;
    }
    else if ( !strcmp(arg, "retrain") ) // like forget, but reclaims trains for gold
    {
        bool introspect = FALSE; // no introspection allowed - whom would we pay?
        CHAR_DATA *trainer = find_trainer(ch, ACT_PRACTICE, &introspect);

        if ( !trainer )
            return;

        if ( (sn = skill_lookup(arg2)) <= 0 )
        {
            act("$N {ttells you {T'Hmm, I'm not familiar with that mastery.'{x", ch, NULL, trainer, TO_CHAR);
            return;
        }
        if ( !ch->pcdata->mastered[sn] )
        {
            act("$N {ttells you {T'You've gotta give me something to work with here.'{x", ch, NULL, trainer, TO_CHAR);
            return;
        }
        // work out reimbursement & cost
        int points_before = mastery_points(ch);
        ch->pcdata->mastered[sn]--;
        int points_after = mastery_points(ch);
        int points_diff = (points_before - points_after);
        int reclaim = rand_div(points_diff * 9, 10);
        int cost = points_diff * 5000;
        if ( (ch->silver/100 + ch->gold) < cost)
        {
            char buf[MSL];
            sprintf(buf, "I don't work for free, you know. Come back when you have %d gold.", cost);
            do_say(trainer, buf);
            ch->pcdata->mastered[sn]++;
            return;
        }
        // ok, we're good
        act("$N helps you retrain your mastery of $t.", ch, skill_table[sn].name, trainer, TO_CHAR);
        act("$N helps $n retrain $s mastery of $t.", ch, skill_table[sn].name, trainer, TO_ROOM);
        printf_to_char(ch, "You reclaim %d train%s for %d gold.\n\r", reclaim, reclaim == 1 ? "" : "s", cost);
        deduct_cost(ch, cost*100);
        ch->train += reclaim;
        ch->pcdata->smc_retrained++;
        check_achievement(ch);
        return;
    }
    else if ( (sn = known_skill_lookup(ch, argument)) > 0 )
    {
        int max_mastery = max_mastery_level(ch, sn);
        int current_mastery = ch->pcdata->mastered[sn];

        bool introspect = TRUE;
        CHAR_DATA *trainer = find_trainer(ch, ACT_PRACTICE, &introspect);

        if ( !trainer && !introspect )
            return;
        
        if ( max_mastery == -1 )
        {
            printf_to_char(ch, "The %s skill cannot be mastered.\n\r", skill_table[sn].name);
            return;
        }
        
        if ( current_mastery >= max_mastery_class(ch->class, sn) )
        {
            if ( trainer )
                act("$N tells you 'There is nothing more I can teach you.'", ch, NULL, trainer, TO_CHAR);
            else
                printf_to_char(ch, "You have reached your limit of expertise in %s.\n\r",  skill_table[sn].name);
            return;
        }

        if ( max_mastery <= current_mastery )
        {
            if ( trainer )
                act("$N tells you 'Come back after you practice some more.'", ch, NULL, trainer, TO_CHAR);
            else
                printf_to_char(ch, "You don't feel skilled enough yet.\n\r", skill_table[sn].name);
            return;            
        }

        int cost = skill_table[sn].mastery_rating + get_mastery_group_cost(ch, sn);
        if ( mastery_points(ch) + cost > max_mastery_points(ch) )
        {
            if ( trainer )
                act("$N tells you 'You cannot master that many skills.'", ch, NULL,trainer, TO_CHAR);
            else
                printf_to_char(ch, "You feel like your head would explode.\n\r", skill_table[sn].name);
            return;
        }
        if ( ch->train < cost )
        {
            if ( trainer )
                act("$N tells you 'You are not yet ready to master that skill.'", ch, NULL,trainer, TO_CHAR);
            else
                printf_to_char(ch, "You don't feel ready yet.\n\r", skill_table[sn].name);
            return;
        }

        // all good, master it
        ch->pcdata->mastered[sn]++;
        ch->train -= cost;
        if ( ch->pcdata->mastered[sn] == 2 )
            ch->pcdata->smc_grandmastered++;
        else
            ch->pcdata->smc_mastered++;
        
        if ( trainer )
        {
            act("$N helps you master the art of $t.", ch, skill_table[sn].name, trainer, TO_CHAR);
            act("$N helps $n master the art of $t.", ch, skill_table[sn].name, trainer, TO_ROOM);
        }
        else
            printf_to_char(ch, "You master the art of %s.\n\r", skill_table[sn].name);
        if ( ch->pcdata->mastered[sn] == 2 )
            printf_to_char(ch, "You are now a grandmaster in the art of %s.\n\r", skill_table[sn].name);

        check_achievement(ch);
        return;
    }
    
    // couldn't parse argument
    show_master_syntax(ch);
}

DEF_DO_FUN(do_skill)
{
    char buf[MSL];
    int sn, level, skill, stance;
    
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

    if ( (sn = class_skill_lookup(ch->class, argument)) < 0
        && (sn = skill_lookup(argument)) < 0 )
    {
        send_to_char( "That skill doesn't exist.\n\r", ch );
        return;
    }

    level = skill_table[sn].skill_level[ch->class];
    skill = get_skill(ch,sn);

    /* check if skill is a stance */
    for (stance = 0; stances[stance].gsn != NULL; stance++)
        if (stances[stance].gsn == skill_table[sn].pgsn)
            break;
 
   /* Now uses existing functions to display mana on spells - Astark 4-23-13 */

    if (stances[stance].cost != 0)
    {
        if ( level > 100 )
        {
            if ( skill > 0 )
            {
                sprintf(buf, "Proficiency for racestance %s:  %3d%% practiced  %3d%% effective  %3d move\n\r", 
                    skill_table[sn].name, ch->pcdata->learned[sn], skill, stance_cost(ch, stance));
            }
            else
            {
                sprintf(buf, "You do not have the %s skill.\n\r", skill_table[sn].name);
            }
        }
        else
        {
            sprintf(buf, "Proficiency for level %d stance %s:  %3d%% practiced  %3d%% effective  %3d move\n\r", 
                level, skill_table[sn].name, ch->pcdata->learned[sn], skill, stance_cost(ch, stance));
        }
    }        
    else if (!IS_SPELL(sn))
    {
        if( level > 100 )
        {
            if( skill > 0 )
            {
                sprintf( buf, "Proficiency for raceskill %s:  %3d%% practiced  %3d%% effective\n\r", 
                    skill_table[sn].name, ch->pcdata->learned[sn], skill );
            }
            else
            {
                sprintf( buf, "You do not have the %s skill.\n\r", skill_table[sn].name );
            }
        }
        else
        {
            sprintf( buf, "Proficiency for level %d skill %s:  %3d%% practiced  %3d%% effective\n\r", 
                level, skill_table[sn].name, ch->pcdata->learned[sn], skill );
        }
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
            {
                sprintf( buf, "You do not have the %s skill.\n\r", skill_table[sn].name );
            }
        }
        else
        {
            sprintf( buf, "Proficiency for level %d skill %s:  %3d%% practiced  %3d%% effective %3d mana\n\r",
                level, skill_table[sn].name, ch->pcdata->learned[sn], skill, mana_cost(ch, sn, skill));  
        }
    }

    send_to_char( buf, ch );
}

/* RT spells and skills show the players spells (or skills) */

DEF_DO_FUN(do_spells)
{
	BUFFER *buffer;
	char arg[MAX_INPUT_LENGTH];
	char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
	char spell_columns[LEVEL_HERO + 1];
	int i, sn, level, skill, min_lev = 1, max_lev = LEVEL_HERO, mana, prac;
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

    // race skills
    struct pc_race_type *pc_race = MULTI_MORPH(ch) ? &pc_race_table[ch->race] : get_morph_pc_race_type(ch);
    for ( i = 0; i < pc_race->num_skills; i++ )
    {
        sn = skill_lookup_exact(pc_race->skills[i]);
        level = pc_race->skill_level[i];
        prac = pc_race->skill_percent[i];
        
        if ( (fAll || level <= ch->level)
            && level >= min_lev && level <= max_lev
            && skill_table[sn].spell_fun != spell_null )
        {
            found = TRUE;
            skill = get_skill(ch, sn);
            mana = mana_cost(ch, sn, skill);
            sprintf(buf, "{g%-16s %3dm %3d%%(%3d%%){x ",
                skill_table[sn].name, mana, prac, skill);

            if ( spell_list[level][0] == '\0' )
                sprintf(spell_list[level], "\n\rLevel %2d: %s", level, buf);
            else /* append */
            {
                if ( ++spell_columns[level] % 2 == 0 )
                    strcat(spell_list[level], "\n\r          ");
                strcat(spell_list[level], buf);
            }
        }
    }

    // subclass skills
    if ( ch->pcdata->subclass > 0 )
    {
        const struct subclass_type *subclass = &subclass_table[ch->pcdata->subclass];
        for ( i = 0; i < MAX_SUBCLASS_SKILL; i++ )
        {
            if ( subclass->skills[i] == NULL )
                break;
            
            if ( (sn = skill_lookup_exact(subclass->skills[i])) < 1 )
                continue;
            level = subclass->skill_level[i];
            prac = subclass->skill_percent[i];
            
            if ( (fAll || level <= ch->level)
                && level >= min_lev && level <= max_lev
                && skill_table[sn].spell_fun != spell_null )
            {
                found = TRUE;
                skill = get_skill(ch, sn);
                mana = mana_cost(ch, sn, skill);
                sprintf(buf, "{B%-16s %3dm %3d%%(%3d%%){x ",
                    skill_table[sn].name, mana, prac, skill);

                if ( spell_list[level][0] == '\0' )
                    sprintf(spell_list[level], "\n\rLevel %2d: %s", level, buf);
                else /* append */
                {
                    if ( ++spell_columns[level] % 2 == 0 )
                        strcat(spell_list[level], "\n\r          ");
                    strcat(spell_list[level], buf);
                }
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
    int penalty = get_injury_penalty(ch) + get_sickness_penalty(ch);
    if ( penalty > 0 )
        printf_to_char(ch, "{rNote: Your spells are reduced by up to %d%% due to injury and/or sickness.{x\n\r", penalty);

	buffer = new_buf();
	for (level = 0; level < LEVEL_HERO + 1; level++)
		if (spell_list[level][0] != '\0')
		add_buf(buffer,spell_list[level]);
	add_buf(buffer,"\n\r");
	page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
}

void show_skills_npc( CHAR_DATA *ch, bool active, CHAR_DATA *viewer )
{
    BUFFER *buffer = new_buf();
    int sn, counter = 0;
    
    for ( sn = 1; sn < MAX_SKILL; sn++ )
    {
        if ( !active && (skill_table[sn].beats > 0 || get_stance_index(sn) >= 0) )
            continue;
            
        int base_skill = mob_get_skill(ch, sn);
        int skill = get_skill(ch, sn);
        if ( base_skill == 0 )
            continue;
        
        if ( counter++ > 0 )
        {
            if ( counter % 2 == 1 )
                add_buf(buffer, "\n\r");
            else
                add_buf(buffer, "    ");
        }
        add_buff(buffer, "%-20s %3d%%(%3d%%)", skill_table[sn].name, base_skill, skill);
    }
    add_buf(buffer, "\n\r");
    page_to_char(buf_string(buffer), viewer);
    free_buf(buffer);
}

void show_advanced_skills( CHAR_DATA *ch )
{
    int sn, counter = 0;
    
    ptc(ch, "Advanced skills (and spells):");
    for ( sn = 1; sn < MAX_SKILL; sn++ )
    {
        int skill = get_skill_overflow(ch, sn);
        if ( skill > 0 )
        {
            const char *sep = (++counter % 2 == 1 ? "\n\r  " : "    ");
            ptc(ch, "%s%-20s %3d%%", sep, skill_table[sn].name, skill);
        }
    }
    ptc(ch, "\n\r");
    if ( counter == 0 )
        ptc(ch, "No advanced skills found.\n\r");
}

DEF_DO_FUN(do_skills)
{
	BUFFER *buffer;
	char arg[MAX_INPUT_LENGTH];
	char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
	char skill_columns[LEVEL_HERO + 1];
	int i, sn, level, prac, min_lev = 1, max_lev = LEVEL_HERO;
	bool fAll = FALSE, found = FALSE;
	char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
    {
        one_argument(argument, arg);
        show_skills_npc(ch, !strcmp(arg, "all"), ch);
        return;
    }

	if (argument[0] != '\0')
	{
	fAll = TRUE;

    argument = one_argument(argument,arg);
    if (!strcmp(arg, "class" ) )
    {
        show_class_skills( ch, argument);
        return;
    }

    if ( !strcmp(arg, "advanced") )
    {
        show_advanced_skills(ch);
        return;
    }

	if (str_prefix(arg,"all"))
	{
		if (!is_number(arg))
		{
		send_to_char("Arguments must be numerical, advanced or all.\n\r",ch);
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
			send_to_char("Argument must be numerical.\n\r",ch);
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

    // race skills
    struct pc_race_type *pc_race = MULTI_MORPH(ch) ? &pc_race_table[ch->race] : get_morph_pc_race_type(ch);
    for ( i = 0; i < pc_race->num_skills; i++ )
    {
        sn = skill_lookup_exact(pc_race->skills[i]);
        level = pc_race->skill_level[i];
        prac = pc_race->skill_percent[i];
        
        if ( (fAll || level <= ch->level)
            && level >= min_lev && level <= max_lev
            && skill_table[sn].spell_fun == spell_null )
        {
            found = TRUE;
            sprintf(buf, "{g%-21s %3d%%(%3d%%){x ", skill_table[sn].name, prac, get_skill(ch, sn));
            if ( skill_list[level][0] == '\0' )
                sprintf(skill_list[level], "\n\rLevel %2d: %s", level, buf);
            else /* append */
            {
                if ( ++skill_columns[level] % 2 == 0 )
                    strcat(skill_list[level], "\n\r          ");
                strcat(skill_list[level], buf);
            }
        }
    }

    // subclass skills
    if ( ch->pcdata->subclass > 0 )
    {
        const struct subclass_type *subclass = &subclass_table[ch->pcdata->subclass];
        for ( i = 0; i < MAX_SUBCLASS_SKILL; i++ )
        {
            if ( subclass->skills[i] == NULL )
                break;
            
            if ( (sn = skill_lookup_exact(subclass->skills[i])) < 1 )
                continue;
            level = subclass->skill_level[i];
            prac = subclass->skill_percent[i];
            
            if ( (fAll || level <= ch->level)
                && level >= min_lev && level <= max_lev
                && skill_table[sn].spell_fun == spell_null )
            {
                found = TRUE;
                sprintf(buf, "{B%-21s %3d%%(%3d%%){x ", skill_table[sn].name, prac, get_skill(ch, sn));
                if ( skill_list[level][0] == '\0' )
                    sprintf(skill_list[level], "\n\rLevel %2d: %s", level, buf);
                else /* append */
                {
                    if ( ++skill_columns[level] % 2 == 0 )
                        strcat(skill_list[level], "\n\r          ");
                    strcat(skill_list[level], buf);
                }
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
    int penalty = get_injury_penalty(ch) + get_sickness_penalty(ch);
    if ( penalty > 0 )
        printf_to_char(ch, "{rNote: Your skills are reduced by up to %d%% due to injury and/or sickness.{x\n\r", penalty);

    /* let's show exotic */
    printf_to_char( ch, "\n\r          %-21s     (%3d%%)", "exotic", get_weapon_skill(ch, -2));

	buffer = new_buf();
	for (level = 0; level < LEVEL_HERO + 1; level++)
		if (skill_list[level][0] != '\0')
		add_buf(buffer,skill_list[level]);
	add_buf(buffer,"\n\r");
	page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
}

void show_class_skills( CHAR_DATA *ch, const char *argument )
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
    int gn,sn,col,seq;
    
    if (IS_NPC(ch))
        return;
    
    col = 0;
    
    sprintf(buf,"{c%-20s {w%3s   {c%-20s {w%3s   {c%-20s {w%3s{x\n\r","group","cp","group","cp","group","cp");
    send_to_char(buf,ch);
    sprintf(buf,"{w------------------------   ------------------------   ------------------------{x\n\r");
    send_to_char(buf,ch);
    
    for ( seq=0
            ; (gn=name_sorted_group_table(seq)) != -1
            ; seq++ )
    {
        if (group_table[gn].name == NULL)
            break;
        
        if (!ch->gen_data->group_chosen[gn]
            &&  !ch->pcdata->group_known[gn]
            &&  group_table[gn].rating[ch->class] > 0)
        {
            sprintf(buf,"%-18s %5d   ",group_table[gn].name,
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

    sprintf(buf,"{c%-17s {w%6s   {c%-17s {w%6s   {c%-17s {w%6s{x\n\r","skill","lvl/cp","skill","lvl/cp","skill","lvl/cp");
    send_to_char(buf,ch);
    sprintf(buf,"{w------------------------   ------------------------   ------------------------{x\n\r");
    send_to_char(buf,ch);
    
    for ( seq=0
            ; (sn=name_sorted_skill_table(seq)) != -1
            ; seq++ )
    {
        if (skill_table[sn].name == NULL)
            break;
        
        if (!ch->gen_data->skill_chosen[sn]
            &&  ch->pcdata->learned[sn] == 0
            &&  skill_table[sn].spell_fun == spell_null
            &&  can_gain_skill(ch, sn) )
        {
            sprintf(buf,"%-17s %3d/%2d   ",
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

    col = 0;

    sprintf(buf,"{c%-17s {w%6s   {c%-17s {w%6s   {c%-17s {w%6s{x\n\r","spell","lvl/cp","spell","lvl/cp","spell","lvl/cp");
    send_to_char(buf,ch);
    sprintf(buf,"{w------------------------   ------------------------   ------------------------{x\n\r");
    send_to_char(buf,ch);
    
    for ( seq=0
            ; (sn=name_sorted_skill_table(seq)) != -1
            ; seq++ )
    {
        if (skill_table[sn].name == NULL)
            break;
        
        if (!ch->gen_data->skill_chosen[sn]
            &&  ch->pcdata->learned[sn] == 0
            &&  skill_table[sn].spell_fun != spell_null
            &&  can_gain_skill(ch, sn) )
        {
            sprintf(buf,"%-17s %3d/%2d   ",
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
    if ( ch->pcdata->points > OPT_CP )
    {
        printf_to_char(ch, "NOTE: You may spend up to %d creation points, but it is recommended to save some.\n\r", MAX_CP);
        printf_to_char(ch, "      Unspent points convert to trains which can be used to train stats early on.\n\r");
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
    int rem_add,race_factor,asc_add;

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

    asc_add = ch->pcdata->ascents ? 100 * (ch->pcdata->ascents + 5) : 0;
    
    return 10 * (race_factor) + asc_add;
}

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups( CHAR_DATA *ch, const char *argument )
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
                 do_help(ch,"header group help");
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
DEF_DO_FUN(do_groups)
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

/* checks for skill improvement - max chance without AFF_LEARN is 2 ^ -chance_exp */
/* default value for chance_exp should be 3 */
void check_improve( CHAR_DATA *ch, int sn, bool success, int chance_exp )
{
    int chance;
    char buf[100];

    if (IS_NPC(ch))
        return;

    // skills that haven't been used in a while are more likely to go up
    // this curbs bot-training and benefits rarely used skills / normal usage
    if ( ch->pcdata->ready2learn[sn] )
    {
        //ptc(ch, "You are ready to learn %s.\n\r", skill_table[sn].name);
        chance_exp = URANGE(0, chance_exp - 2, 3);
        ch->pcdata->ready2learn[sn] = FALSE;
    }
    else
        chance_exp++;
        
    if ( IS_AFFECTED(ch, AFF_LEARN) )
         chance_exp--;
    
    // safety net
    chance_exp = URANGE(0, chance_exp, 10);

    // base chance, 2 ^ -chance_exp
    if ( chance_exp && number_bits(chance_exp) )
        return;
    
    if (ch->level < skill_table[sn].skill_level[ch->class]
        ||  skill_table[sn].rating[ch->class] == 0
        ||  ch->pcdata->learned[sn] == 0
        ||  ch->pcdata->learned[sn] == 100)
        return;  /* skill is not known */

    // int-based chance to fail
    chance = 100 * ch_int_learn(ch) / int_app_learn(MAX_CURRSTAT);
    if ( !per_chance(chance) )
        return;

    // having practiced above 75% decreases chance to learn, same progression as hprac
    int fail_factor = hprac_cost(ch, sn);
    if ( number_range(1, fail_factor) != 1 )
        return;
    
    if ( success )
        ptc(ch, "You have become better at %s!\n\r", skill_table[sn].name);
    else
        ptc(ch, "You learn from your mistakes, and your %s skill improves.\n\r", skill_table[sn].name);

    ch->pcdata->learned[sn] += 1;
    ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn],100);
    gain_exp(ch, fail_factor * skill_table[sn].rating[ch->class]);

    if (ch->pcdata->learned[sn] == 100)
    {
        sprintf(buf,"{RAfter extensive training, you reach maximum proficiency in %s!{x\n\r", skill_table[sn].name);
        send_to_char(buf,ch);
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

	sn = skill_lookup_exact(name);

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

int get_sickness_penalty( CHAR_DATA *ch )
{
    int penalty = 0;
    if ( IS_AFFECTED(ch, AFF_POISON) )
        penalty += 1;
    if ( IS_AFFECTED(ch, AFF_PLAGUE) )
        penalty += 1;
    return penalty;
}

int mob_has_skill(CHAR_DATA *ch, int sn)
{
    bool charmed;

    if ( ch->pIndexData->skills[sn] )
        return TRUE;
    
    if ( IS_AFFECTED(ch, AFF_CHARM) || IS_SET(ch->act, ACT_PET) )
	charmed = TRUE;
    else
	charmed = FALSE;

    // some skills are both active and passive
    if ( sn == gsn_petrify )
        return IS_SET(ch->off_flags, OFF_PETRIFY);
    
    /* if a mob uses an active skill (not ordered), he knows it */
    if ( !charmed
	 && (sn >= 0 && sn < MAX_SKILL)
	 && skill_table[sn].beats > 0 )
	return TRUE;
    
    /* non-charmed mobs also know all stances */
    int stance = get_stance_index(sn);
    if ( stance >= 0 && (!charmed || ch->pIndexData->stance == stance) )
        return TRUE;

    /* skills they always have */
    if ( (sn==gsn_hand_to_hand)
     || (sn==gsn_dodge)
	 || (sn==gsn_rescue)
	 || (sn==gsn_flee)
	 || (sn==gsn_sneak)
	 || (sn==gsn_hide)
	 || (sn==-1) )
	return TRUE;
    
    /* skills most mobs have */
    if ( (sn==gsn_shield_block)
     || (sn==gsn_wrist_shield)
     || (sn==gsn_dual_wield)
     || (sn==gsn_ambidextrous)
     || (sn==gsn_two_handed) )
        return !IS_SET(ch->act, ACT_NOWEAPON);

    if ( sn==gsn_backstab || sn==gsn_circle || sn==gsn_flanking )
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
    
    // all mobs know meta-magic skills
    if ( sn==gsn_extend_spell || sn==gsn_empower_spell || sn==gsn_quicken_spell || sn==gsn_chain_spell )
        return TRUE;
    
    /* skills by offensive flags */
    if (sn==gsn_parry)
	return IS_SET(ch->off_flags, OFF_PARRY);
    if (sn==gsn_second_attack || sn==gsn_third_attack)
	return IS_SET(ch->off_flags, OFF_FAST);

    if ( sn == gsn_brawl || sn == gsn_melee )
	return IS_SET(ch->off_flags, OFF_AREA_ATTACK);
    if ( sn == gsn_cursed_wound )
        return IS_SET(ch->off_flags, OFF_WOUND);
    
    if (sn==gsn_trip)
	return IS_SET(ch->off_flags, OFF_TRIP);
    if (sn==gsn_bash)
	return IS_SET(ch->off_flags, OFF_BASH);
    if (sn==gsn_berserk)
	return IS_SET(ch->off_flags, OFF_BERSERK);
    if (sn==gsn_disarm)
	return (IS_SET(ch->off_flags, OFF_DISARM)
		|| IS_SET(ch->act, ACT_THIEF) || IS_SET(ch->act, ACT_WARRIOR));
    // if (sn==gsn_bodyguard)
    //    return IS_SET(ch->off_flags, OFF_RESCUE);
    if (sn==gsn_entrapment)
        return IS_SET(ch->off_flags, OFF_ENTRAP);

    // mobs that cast spells via spec_fun normally can do so even while charmed
    if ( skill_table[sn].spell_fun != spell_null )
    {
        const struct spell_type *spell_list = get_spell_list( ch );
        if ( spell_list != NULL )
        {
            int spell;
            for (spell = 0; spell_list[spell].spell != NULL; spell++)
                if ( sn == skill_lookup(spell_list[spell].spell) )
                    return TRUE;
        }        
    }
    
    return FALSE;
}

int mob_get_skill(CHAR_DATA *ch, int sn)
{
    int skill = 50 + ch->level / 4;

    if (sn < -1 || sn >= MAX_SKILL)
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
    if ( MULTI_MORPH(ch) )
        race = &pc_race_table[ch->race];
    else
        race = get_morph_pc_race_type( ch );

    /* check for racial skill */
    for (i=0; i < race->num_skills; i++)
	if ( (race->skill_gsns[i] == sn) && (ch->level >= race->skill_level[i]) )
	    return race->skill_percent[i];

    return 0;
}

int get_subclass_skill( CHAR_DATA *ch, int sn )
{
    if ( IS_NPC(ch) || !ch->pcdata->subclass )
        return 0;
    if ( sn < 0 || sn >= MAX_SKILL )
        return 0;
    
    const struct subclass_type *sc = &subclass_table[ch->pcdata->subclass];

    int i;
    for ( i = 0; i < MAX_SUBCLASS_SKILL; i++ )
    {
        if ( sc->skills[i] == NULL )
            return 0;
        if ( !strcmp(sc->skills[i], skill_table[sn].name) )
            return ch->level >= sc->skill_level[i] ? sc->skill_percent[i] : 0;
    }

    return 0;
}

int pc_skill_prac(CHAR_DATA *ch, int sn)
{
	int skill;

	if (sn == -1) /* shorthand for level based skills */
	{
	    skill = ch->level;
	}

	else if (sn < -1 || sn >= MAX_SKILL)
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

// returns base skill for pc, as well as overflow when getting skill from multiple sources
// passing NULL as overflow is valid (no overflow returned)
static int pc_get_skill( CHAR_DATA *ch, int sn, int *overflow )
{
    int skill, total;

    if ( sn == -1 ) /* shorthand for level based skills */
    {
        skill = ch->level;
    }
    else if (sn < -1 || sn >= MAX_SKILL)
    {
        bug("Bad sn %d in pc_get_skill.", sn);
        return 0;
    }
    else if ( ch->level < skill_table[sn].skill_level[ch->class] || !ch->pcdata->learned[sn] )
        skill = 0;
    else
    {
        skill = ch->pcdata->learned[sn] * skill_table[sn].cap[ch->class] / 100;
        skill = UMAX(1, skill);
    }
    
    total = skill;

    /* adjust for race skill */
    int race_skill = get_race_skill( ch, sn );
    if ( race_skill > 0 )
    {
        skill = skill * (100 - race_skill) / 100 + race_skill;
        total += race_skill;
    }

    /* adjust for subclass skill */
    int subclass_skill = get_subclass_skill(ch, sn);
    if ( subclass_skill > 0 )
    {
        skill = skill * (100 - subclass_skill) / 100 + subclass_skill;
        total += subclass_skill;
    }
    
    // return overflow if required
    if ( overflow != NULL )
        *overflow = total - skill;
    
    return skill;
}

int get_skill_overflow( CHAR_DATA *ch, int sn )
{
    if ( IS_NPC(ch) )
        return 0;

    int overflow = 0;
    pc_get_skill(ch, sn, &overflow);
    return overflow;
}

/* for returning skill information */
int get_skill(CHAR_DATA *ch, int sn)
{
	int skill;

	if (IS_NPC(ch))
	    skill = mob_get_skill(ch, sn);
	else
	    skill = pc_get_skill(ch, sn, NULL);

    if ( skill == 0 )
        return 0;

    // generic modifier to all skills
    if ( ch->mod_skills )
    {
        int mod = URANGE(-100, ch->mod_skills, 100);
        if ( mod > 0 )
            skill += (100 - skill) * mod / 100;
        else
            skill = skill * (100 + mod) / 100;
    }
    
    // adjustment for stats below max
    int statSum = 3 * get_curr_stat(ch, skill_table[sn].stat_prime);
    statSum += 2 * get_curr_stat(ch, skill_table[sn].stat_second);
    statSum += get_curr_stat(ch, skill_table[sn].stat_third);

    float factor = (statSum/6.0 + 800) / 1000.0;
    
    if ( ch->pcdata && ch->pcdata->condition[COND_DRUNK] > 10 && !IS_AFFECTED(ch, AFF_BERSERK) )
        factor *= 0.9;
    
    if (ch->daze > 0)
    {
        if (skill_table[sn].spell_fun != spell_null)
            factor *= 0.5;
        else
            factor *= 0.666;
    }
    
    /* injury */
    // needed to avoid infinite recursion for ashura, and true grit only works a 1 hp left
    if ( sn != gsn_ashura && sn != gsn_true_grit )
        factor *=  (100 - get_injury_penalty(ch)) / 100.0;
    
    // apply all factors in one go to reduce rounding errors
    skill *= factor;
    
    /* poison & disease */
    skill -= get_sickness_penalty(ch);

    return URANGE(1, skill, 100);
}

int get_skill_total( CHAR_DATA *ch, int sn, float overflow_weight )
{
    return get_skill(ch, sn) + get_skill_overflow(ch, sn) * overflow_weight;
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

	if (IS_NPC(ch))
	{
        if ( IS_SET(ch->act,ACT_NOWEAPON) && sn != gsn_hand_to_hand )
            return 0;
        
        skill = 50 + ch->level/4;
	    skill = URANGE(0, skill, 100);
	    /* injury */
	    skill = skill * (100 - get_injury_penalty(ch)) / 100;
	}
    // -2 is exotic
    else if ( sn == -2 )
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

static int hprac_cost( CHAR_DATA *ch, int sn )
{
    int adapt = class_table[ch->class].skill_adept;
    int learned = ch->pcdata->learned[sn];
    return 1 + UMAX(0, learned - adapt);
}

// hard practice - above 75%
DEF_DO_FUN(do_hpractice)
{
    bool introspect = TRUE;
    CHAR_DATA *trainer = find_trainer(ch, ACT_PRACTICE, &introspect);

    if ( (!trainer && !introspect) || IS_NPC(ch) )
        return;
    
    if ( !argument[0] )
    {
        send_to_char( "Syntax: hpractice <skill>\n\r", ch );
        send_to_char( "Hard practice will increase your skill beyond normal limits, at increasing cost.\n\r", ch );
        return;
    }

    int sn = known_skill_lookup(ch, argument);
    if ( sn < 0 || IS_NPC(ch) || ch->level < skill_table[sn].skill_level[ch->class] )
    {
        send_to_char( "You can't practice that.\n\r", ch );
        return;
    }

    int adapt = class_table[ch->class].skill_adept;
    int learned = ch->pcdata->learned[sn];
    if ( learned < adapt )
    {
        send_to_char( "Try practicing normally.\n\r", ch );
        return;
    }
    if ( learned >= 100 )
    {
        send_to_char( "You are as good as it gets.\n\r", ch );
        return;
    }
    
    int cost = hprac_cost(ch, sn);
    if ( ch->practice < cost )
    {
        printf_to_char(ch, "It costs %d practice sessions to improve %s.\n\r", cost, skill_table[sn].name);
        return;
    }
    
    ch->practice -= cost;
    ch->pcdata->learned[sn]++;
    printf_to_char(ch, "Your hard practice improves %s from %d%% to %d%%, at the cost of %d session%s.\n\r",
        skill_table[sn].name, learned, ch->pcdata->learned[sn], cost, cost > 1 ? "s" : "");
}

DEF_DO_FUN(do_practice)
{
	BUFFER *buffer;
   char buf[MAX_STRING_LENGTH];
   int sn;
   int seq;
   int skill;
   int prac_curr, prac_gain;

   if ( IS_NPC(ch) )
	  return;

   if ( argument[0] == '\0' )
   {
	  int col;
	
	  buffer = new_buf();
	  col    = 0;
      for ( seq=0
              ; (sn=name_sorted_skill_table(seq)) != -1
              ; seq++ )
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
	      check_improve(ch,gsn_introspection,TRUE,3);
	   else
	   {
	      send_to_char("You've learned nothing from your recent experiences.\n\r",ch);
	      check_improve(ch,gsn_introspection,FALSE,3);
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
            if ( IS_SET(ch->act, PLR_NOEXP) || IS_HERO(ch) )
            {
                send_to_char("You cannot gain experience right now.\n\r", ch);
                return;
            }
            /* no more burning practices when you don't have field */
            if (ch->pcdata->field < 100)
            {
                send_to_char("You don't have enough field experience to practice.\n\r",ch);
                return;
            }
            else
            {
                int convert = number_range(ch->pcdata->field/3, ch->pcdata->field*2/3);
                ptc(ch, "Your practice converts %d field experience into real experience.\n\r", convert);
                ch->practice--;
                ch->pcdata->field -= convert;
                ch->exp += convert;
                update_pc_level(ch);
                return;
            }
	}
	
	  if ( (sn = known_skill_lookup(ch, argument)) < 0
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
          printf_to_char(ch, "You are already learned at %s. Try \t(hpractice\t) instead.\n\r", skill_table[sn].name);
	  }
	  else
	  {
		  WAIT_STATE(ch, 8);
		ch->practice--;
              prac_curr = ch->pcdata->learned[sn];
              prac_gain = ch_int_learn(ch) / skill_table[sn].rating[ch->class];
		 ch->pcdata->learned[sn] += prac_gain;
                 /*
		 if ( ch->pcdata->learned[sn] < adept )
		 {
			act( "You practice $T.",
			   ch, NULL, skill_table[sn].name, TO_CHAR );
			act( "$n practices $T.",
			   ch, NULL, skill_table[sn].name, TO_ROOM );
		 } 
                 */
                 /* Practices output now shows amount gained */
                 if (ch->pcdata->learned[sn] < adept)
                 {
                     sprintf(buf, "You practice %s from %d%% to %d%%.\n\r",
                         skill_table[sn].name, prac_curr, prac_curr + prac_gain);
                     send_to_char(buf, ch);
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

// reimburse character for skill gained and then lost due to class losing the skill
void skill_reimburse( CHAR_DATA *ch )
{
    int sn;
    
    if ( !ch || !ch->pcdata || IS_IMMORTAL(ch) )
        return;
    
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        // still accessible
        if ( skill_table[sn].skill_level[ch->class] <= LEVEL_HERO )
            continue;
        
        int learned = ch->pcdata->learned[sn];
        // racial skills or groups set learned to 1, don't reimburse for these
        // characters who gained the skill but didn't practice it lose out, that's life
        if ( learned <= 1 )
            continue;
        
        int train_cost = skill_table[sn].min_rating;
        // assume an int rating of 100 for the purpose of determining practice cost
        int prac_cost = 1 + UMIN(learned, 75) * train_cost / int_app_learn(100);
        // reimburse for practice above 75 as if hard practiced
        if ( learned > 75 )
            prac_cost += (learned - 75) * (learned - 74) / 2;
        
        printf_to_char(ch, "You have been reimbursed %d trains and %d practices for the loss of %s (%d%%).\n\r",
            train_cost, prac_cost, skill_table[sn].name, learned);
        logpf("%s has been reimbursed %d trains and %d practices for the loss of %s (%d%%).",
            ch->name, train_cost, prac_cost, skill_table[sn].name, learned);
        
        ch->pcdata->learned[sn] = 0;
        ch->train += train_cost;
        ch->practice += prac_cost;
    }
}

DEF_DO_FUN(do_raceskills)
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
void show_skill(const char *argument, BUFFER *buffer, CHAR_DATA *ch);
void show_skill_all(BUFFER *buffer);
void show_skill_low(BUFFER *buffer, int threshold, int active_threshold);
DEF_DO_FUN(do_showskill)
{
    char arg[MIL] = "";
    int skill, group = -1;
    BUFFER *buffer;
    bool show_all = FALSE, show_points = FALSE, show_low = FALSE;
    int threshold = 50, active = 75;

    one_argument(argument, arg);

    if (argument[0] == '\0')
    { 
        printf_to_char(ch,"Syntax: showskill <spell/skill name>\n\r");
        printf_to_char(ch,"        showskill all|points\n\r");
        printf_to_char(ch,"        showskill low [threshold] [active threshold]\n\r");
        return; 
    }

    if ( str_cmp(argument, "all") == 0 )
        show_all = TRUE;
    else if ( str_cmp(argument, "points") == 0 )
        show_points = TRUE;
    else if ( str_cmp(arg, "low") == 0 )
    {
        show_low = TRUE;
        argument = one_argument(argument, arg); // low
        argument = one_argument(argument, arg); // threshold
        if ( is_number(arg) )
        {
            threshold = active = atoi(arg);
            argument = one_argument(argument, arg); // active threshold
            if ( is_number(arg) )
                active = atoi(arg);
        }
    }
    else
    {
        if ( (skill = skill_lookup(argument)) == -1 && (group = group_lookup(argument)) == -1 )
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
    else if ( show_points )
    {
        show_skill_points(buffer);
    }
    else if ( show_low )
    {
        show_skill_low(buffer, threshold, active);
    }
    else if (group >= 0)  /* Argument was a valid group name. */
    {
        int sn;
        
        printf_to_char(ch, "{cGroup:  {Y%s{x\n\r\n\r", capitalize(group_table[group].name));
        
        for (sn = 0; sn < MAX_IN_GROUP; sn++)
        {
            if (group_table[group].spells[sn] == NULL)
                break;
            
            show_skill(group_table[group].spells[sn], buffer, ch);
            add_buf( buffer, "\n\r" );
        }
    }
    else           /* Argument was a valid skill/spell/stance name */
    {
        show_skill(argument, buffer, ch);
        show_groups(skill, buffer);
        show_mastery_groups(skill, buffer);
        show_races(skill, buffer);
        show_skill_subclasses(skill, buffer);
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

void show_mastery_groups( int skill, BUFFER *buffer )
{
    char buf[MSL];
    int gn, col = 0;

    if ( skill_table[skill].mastery_rating < 1 )
        return;

    add_buf( buffer, "\n\rIt belongs to the following schools:\n\r" );
    for ( gn = 1; mastery_group_table[gn].name; gn++ )
    {
        if ( !is_in_mastery_group(skill, gn) )
            continue;
        sprintf( buf, "%s (%d)  ", mastery_group_table[gn].name, mastery_group_table[gn].rating );
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

bool has_subclass_skill( int skill, int subclass )
{
    int i;
    const struct subclass_type *sc = &subclass_table[subclass];
    
    for ( i = 0; i < MAX_SUBCLASS_SKILL; i++ )
    {
        if ( sc->skills[i] == NULL )
            break;
        if ( !str_cmp(sc->skills[i], skill_table[skill].name) )
            return TRUE;
    }
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

void show_skill_subclasses( int skill, BUFFER *buffer )
{
    char buf[MSL];
    int subclass, col = 0;

    add_buf( buffer, "\n\rIt is possessed by the following subclasses:\n\r" );
   
    for ( subclass = 1; subclass_table[subclass].name != NULL; subclass++ )
    {
        if ( !has_subclass_skill(skill, subclass) )
            continue;
        sprintf( buf, "%-20s ", subclass_table[subclass].name );
        add_buf( buffer, buf );
        if ( ++col % 3 == 0 )
            add_buf( buffer, "\n\r" );
    }
    if ( col % 3 != 0 )
    add_buf( buffer, "\n\r" );
}

void show_skill(const char *argument, BUFFER *buffer, CHAR_DATA *ch)
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

    stance = get_stance_index(skill);

    if ( is_spell )
    {
        if ( skill_table[skill].mana_boost > 0 )
        {
            add_buff( buffer, "Base Mana: %d + %.1f%%  Lag: %d  Duration: %s\n\r",
                skill_table[skill].min_mana, 0.1 * skill_table[skill].mana_boost, skill_table[skill].beats,
                spell_duration_names[skill_table[skill].duration]);
        }
        else
        {
            add_buff( buffer, "Base Mana: %d  Lag: %d  Duration: %s\n\r",
                skill_table[skill].min_mana, skill_table[skill].beats,
                spell_duration_names[skill_table[skill].duration]);
        }
        add_buff( buffer, "Target: %s  Combat: %s\n\r",
            spell_target_names[skill_table[skill].target],
            skill_table[skill].minimum_position <= POS_FIGHTING ? "yes" : "no" );
    }
    else if ( stance >= 0 && stances[stance].cost != 0 )
        add_buff(buffer, "Base Move: %d\n\r", 
            stances[stance].cost);
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

    //if (IS_IMMORTAL(ch))
    //    add_buff(buffer, "Skill Number: %d\n\r", skill_lookup(argument));
    
    add_buff(buffer, "\n\r{wClass          Level Points  Max  Mastery{x\n\r");
    add_buff(buffer,     "{w------------   ----- ------ ----- -------{x\n\r");
    
    for ( cls = 0; cls < MAX_CLASS; cls++ )
    {
        if (skill_table[skill].skill_level[cls] > LEVEL_HERO)
        {
            sprintf(log_buf, "{r   --     --     --   -- --{x\n\r");
        }
        else
        {
            char mbuf[MSL];
            int max_mastery = max_mastery_class(cls, skill);
            if ( skill_table[skill].mastery_rating < 1 || max_mastery < 1 )
                sprintf(mbuf, "{r-- --");
            else
                sprintf(mbuf, "%2d %2s", skill_table[skill].mastery_rating, max_mastery == 2 ? "GM" : "MA");
            
            sprintf(log_buf, "{g%5d    %3d    %3d   %s{x\n\r",
                skill_table[skill].skill_level[cls],
                skill_table[skill].rating[cls],
                skill_table[skill].cap[cls],
                mbuf
            );
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

void show_skill_low(BUFFER *buffer, int threshold, int active_threshold)
{
    int skill, class;

    if (buffer == NULL)
        return;

    add_buff(buffer, "Skill/Spell          Type         Min     Max\n\r");
    add_buff(buffer, "=============================================\n\r");
    
    for ( skill = 1; skill_table[skill].name != NULL; skill++ )
    {
        bool active = skill_table[skill].beats > 0;
        // find min/max percentage for all classes
        int min_per=100, max_per=0;
        for ( class = 0; class < MAX_CLASS; class++ )
            if ( is_class_skill(class, skill) )
            {
                min_per = UMIN(min_per, skill_table[skill].cap[class]);
                max_per = UMAX(max_per, skill_table[skill].cap[class]);
            }
        // skip non-class skills
        if ( min_per > max_per )
            continue;
        // skip skill with high percentages
        if ( max_per == 100 && min_per >= threshold && !(active && min_per < active_threshold) )
            continue;
        add_buff( buffer, "%-20.20s %-8.8s    %3d%%    %3d%%\n\r",
            skill_table[skill].name,
            active ? "active" : "passive",
            min_per, max_per
        );
    }            
}

void show_skill_points(BUFFER *buffer)
{
    int skill, class;

    if (buffer == NULL)
        return;

    add_buff(buffer, "Skill/Spell         ");
    for ( class = 0; class < MAX_CLASS; class++ )
        add_buff(buffer, "%c%c ", class_table[class].who_name[0], class_table[class].who_name[1]);
    add_buff(buffer, "\n\r===================================================================\n\r");
    
    for ( skill = 1; skill_table[skill].name != NULL; skill++ )
    {
        add_buff( buffer, "%-20.20s", skill_table[skill].name );
        for ( class = 0; class < MAX_CLASS; class++ )
            if ( is_class_skill(class, skill) )
                add_buff(buffer, "%2d ", skill_table[skill].rating[class]);
            else
                add_buff(buffer, " - ");
        add_buff(buffer, "\n\r");
    }            
}

static void setskill_syntax( CHAR_DATA *ch )
{
    send_to_char("Syntax: setskill <skill> level/max <class|all> <value>\n\r", ch);
    send_to_char("Syntax: setskill <skill> prime/second/third <stat>\n\r", ch);
    send_to_char("Syntax: setskill <skill> lag/mana/points <value>\n\r", ch);   
}

extern int dice_lookup(char *);
DEF_DO_FUN(do_setskill)
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
        setskill_syntax(ch);
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
        setskill_syntax(ch);
		return;
	}

    if ( field == 1 || field == 3 )
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
    else if ( field == 4 || field == 5 || field == 6 )
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
        else if ( field == 2 )
            skill_table[sn].min_rating = val1;
	}

    update_skill_costs();
    update_group_costs();
	send_to_char("OK.\n\r", ch);
}


DEF_DO_FUN(do_slookup)
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
            sprintf( buf, "Sn: %3d  Skill/spell: '%s'\n\r",
                sn, skill_table[sn].name );
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
        
        sprintf( buf, "Sn: %3d  Skill/spell: '%s'\n\r",
            sn, skill_table[sn].name );
        send_to_char( buf, ch );
    }
    
    return;
}

void save_skill(FILE *f, int sn)
{
	int i;

	fprintf(f, "\n%s\n", skill_table[sn].name);
	fprintf(f, "%d %d %d\n", skill_table[sn].beats, skill_table[sn].min_mana, skill_table[sn].min_rating);

	for (i=0; i<MAX_CLASS; i++)
		fprintf(f, "%3d ", skill_table[sn].skill_level[i]);
	fprintf(f, "\n");

    /*
	for (i=0; i<MAX_CLASS; i++)
		fprintf(f, "%3d ", skill_table[sn].rating[i]);
	fprintf(f, "\n");
    */

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

    fprintf(f, "#VER 1\n");
	fprintf(f, "%d %d\n", n, MAX_CLASS);

	for (i=0; i<n; i++)
		save_skill(f, i);

	fclose(f);
}


void count_stat(FILE *f, int sn)
{
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



void load_skill(FILE *f, int cnum, int version)
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

    skill_table[sn].beats = fread_number(f);
    skill_table[sn].min_mana = fread_number(f);
    
    if ( version > 0 )
        skill_table[sn].min_rating = fread_number(f);

	for (i=0; i<cnum; i++)
        skill_table[sn].skill_level[i] = fread_number(f);

    if ( version == 0 )
        for (i=0; i<cnum; i++)
            skill_table[sn].rating[i] = fread_number(f);

	for (i=0; i<cnum; i++)
        skill_table[sn].cap[i] = fread_number(f);

    skill_table[sn].stat_prime = fread_number(f);
    skill_table[sn].stat_second = fread_number(f);
    skill_table[sn].stat_third = fread_number(f);
}

void load_skills()
{
	FILE *f;
	int i, n, cnum;
    int version = 0;

	f = fopen(SKILL_FILE, "r");
	if (f==NULL)
	{
		bug("Could not open " SKILL_FILE " for reading.",0);
		exit(1);
	}

    // find version
    if ( !strcmp(fread_word(f), "#VER") )
        version = fread_number(f);
    else
        rewind(f);
    
    n = fread_number(f);
    cnum = fread_number(f);

	for (i=0; i<n; i++)
        load_skill(f, cnum, version);

	fclose(f);
}

