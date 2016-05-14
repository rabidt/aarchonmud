/***********************************************************************
*                                                                      * 
*   Bard class skills and spells, intended for use by Aeaea MUD.       *
*   All rights are reserved.                                           *
*                                                                      * 
*   Core ranger group by Brian Castle a.k.a. "Rimbol".                 *
*   Another batch added by James Stewart a.k.a "Siva".                 * 
***********************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "magic.h"
#include "tables.h"
#include "warfare.h"
#include "lookup.h"
#include "special.h"
#include "mudconfig.h"
#include "mob_stats.h"
#include "interp.h"
#include "songs.h"

DEF_DO_FUN(do_wail)
{
    CHAR_DATA *victim;
    int skill;
    int dam, chance;

    if ((skill = get_skill(ch,gsn_wail)) == 0)
    {
        send_to_char("You scream your lungs out without effect.\n\r", ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL)
        return;

    
    chance = (100 + get_skill(ch,gsn_wail)) / 2;

    if ( check_hit(ch, victim, gsn_wail, DAM_SOUND, chance) )
    {
        dam = martial_damage( ch, victim, gsn_wail );

        full_dam(ch, victim, dam, gsn_wail, DAM_SOUND, TRUE);
        check_improve(ch, gsn_wail, TRUE, 3);
    } else {
        damage( ch, victim, 0, gsn_wail, DAM_SOUND, TRUE);
        check_improve(ch, gsn_wail, FALSE, 3);
    }

    // make this a room skill if affected by deadly dance
    CHAR_DATA *vch;

    if (IS_AFFECTED(ch, AFF_DEADLY_DANCE))
    {
        for (vch = room->people; vch != NULL; vch = vch->next_in_room)
        {
            if ( is_opponent(ch,vch) && vch != victim)
            {
                if ( check_hit(ch, vch, gsn_wail, DAM_SOUND, chance) )
                {
                    dam = martial_damage( ch, vch, gsn_wail );
            
                    full_dam(ch, vch, dam, gsn_wail, DAM_SOUND, TRUE);
                    check_improve(ch, gsn_wail, TRUE, 3);
                } else {
                    damage( ch, vch, 0, gsn_wail, DAM_SOUND, TRUE);
                    check_improve(ch, gsn_wail, FALSE, 3);
                }   
            }
        }
    }
    return;
}

DEF_DO_FUN(do_fox)
{
    AFFECT_DATA af;
    CHAR_DATA *vch;
    CHAR_DATA *target = NULL;
    bool all = FALSE;
    
    int skill = get_skill(ch, gsn_foxs_cunning);
    int chance = (100 + skill) / 2;
    int cost = skill_table[gsn_foxs_cunning].min_mana * 200 / (100 + skill);
    int level = ch->level * (100 + skill) / 200;
    
    if ( !skill )
    {
        send_to_char("You don't know how.\n\r", ch);
        return;
    }
    
    if ( argument[0] != '\0' )
    {
        char arg[MIL];
        one_argument(argument, arg);
        if ( !strcmp(arg, "all") )
            all = TRUE;
        else if ( (target = get_char_room(ch, arg)) == NULL )
        {
            send_to_char("Give fox's cunning to whom?\n\r", ch);
            return;
        }
    }
    
    if ( ch->mana < cost )
    {
        send_to_char("You have run out of cunning.\n\r", ch);
        return;
    }
    
    WAIT_STATE( ch, skill_table[gsn_foxs_cunning].beats );

    if ( !per_chance(chance) )
    {
        ch->mana -= cost/2;
        send_to_char("Your song isn't very cunning.\n\r", ch);
        check_improve(ch, gsn_foxs_cunning, FALSE, 3);
        return;
    }
        
    ch->mana -= cost;
    send_to_char("You sing a cunning melody!\n\r", ch);
    act("$n sings a cunning song!", ch, NULL, NULL, TO_ROOM);
        
    af.where     = TO_AFFECTS;
    af.type      = gsn_foxs_cunning;
    af.level     = level;
    af.duration  = get_duration(gsn_foxs_cunning, level);
    af.bitvector = AFF_PASSIVE_SONG;

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( !all && !is_same_group(vch, ch) && !is_same_group(vch, target) )
            continue;

        send_to_char("You gain the fox's cunning.\n\r", vch);
        if ( vch != ch )
            act("Your song gives $N the fox's cunning.", ch, NULL, vch, TO_CHAR);
        
        remove_passive_bard_song(vch);

        af.modifier = 5 + level / 9;
        af.location = APPLY_WIS;
        affect_to_char(vch, &af);
        check_improve(ch, gsn_foxs_cunning, TRUE, 3);

    }
}

DEF_DO_FUN(do_bear)
{
    AFFECT_DATA af;
    CHAR_DATA *vch;
    CHAR_DATA *target = NULL;
    bool all = FALSE;
    
    int skill = get_skill(ch, gsn_bears_endurance);
    int chance = (100 + skill) / 2;
    int cost = skill_table[gsn_bears_endurance].min_mana * 200 / (100 + skill);
    int level = ch->level * (100 + skill) / 200;
    
    if ( !skill )
    {
        send_to_char("You don't know how.\n\r", ch);
        return;
    }
    
    if ( argument[0] != '\0' )
    {
        char arg[MIL];
        one_argument(argument, arg);
        if ( !strcmp(arg, "all") )
            all = TRUE;
        else if ( (target = get_char_room(ch, arg)) == NULL )
        {
            send_to_char("Give bear's endurance to whom?\n\r", ch);
            return;
        }
    }
    
    if ( ch->mana < cost )
    {
        send_to_char("You have run out of endurance.\n\r", ch);
        return;
    }
    
    WAIT_STATE( ch, skill_table[gsn_bears_endurance].beats );

    if ( !per_chance(chance) )
    {
        ch->mana -= cost/2;
        send_to_char("Your song isn't very enduring.\n\r", ch);
        check_improve(ch, gsn_bears_endurance, FALSE, 3);
        return;
    }
        
    ch->mana -= cost;
    send_to_char("You sing an enduring melody!\n\r", ch);
    act("$n sings an enduring song!", ch, NULL, NULL, TO_ROOM);
        
    af.where     = TO_AFFECTS;
    af.type      = gsn_bears_endurance;
    af.level     = level;
    af.duration  = get_duration(gsn_bears_endurance, level);
    af.bitvector = AFF_PASSIVE_SONG;

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( !all && !is_same_group(vch, ch) && !is_same_group(vch, target) )
            continue;

        send_to_char("You gain the bear's endurance.\n\r", vch);
        if ( vch != ch )
            act("Your song gives $N the bear's endurance.", ch, NULL, vch, TO_CHAR);
        
        remove_passive_bard_song(vch);

        af.modifier = 5 + level / 9;
        af.location = APPLY_CON;
        affect_to_char(vch, &af);
        check_improve(ch, gsn_bears_endurance, TRUE, 3);
    }
}

DEF_DO_FUN(do_cat)
{
    AFFECT_DATA af;
    CHAR_DATA *vch;
    CHAR_DATA *target = NULL;
    bool all = FALSE;
    
    int skill = get_skill(ch, gsn_cats_grace);
    int chance = (100 + skill) / 2;
    int cost = skill_table[gsn_cats_grace].min_mana * 200 / (100 + skill);
    int level = ch->level * (100 + skill) / 200;
    
    if ( !skill )
    {
        send_to_char("You don't know how.\n\r", ch);
        return;
    }
    
    if ( argument[0] != '\0' )
    {
        char arg[MIL];
        one_argument(argument, arg);
        if ( !strcmp(arg, "all") )
            all = TRUE;
        else if ( (target = get_char_room(ch, arg)) == NULL )
        {
            send_to_char("Give cat's grace to whom?\n\r", ch);
            return;
        }
    }
    
    if ( ch->mana < cost )
    {
        send_to_char("You have run out of grace.\n\r", ch);
        return;
    }
    
    WAIT_STATE( ch, skill_table[gsn_cats_grace].beats );

    if ( !per_chance(chance) )
    {
        ch->mana -= cost/2;
        send_to_char("Your song isn't very graceful.\n\r", ch);
        check_improve(ch, gsn_cats_grace, FALSE, 3);
        return;
    }
        
    ch->mana -= cost;
    send_to_char("You sing a graceful melody!\n\r", ch);
    act("$n sings a graceful song!", ch, NULL, NULL, TO_ROOM);
        
    af.where     = TO_AFFECTS;
    af.type      = gsn_cats_grace;
    af.level     = level;
    af.duration  = get_duration(gsn_cats_grace, level);
    af.bitvector = AFF_PASSIVE_SONG;

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( !all && !is_same_group(vch, ch) && !is_same_group(vch, target) )
            continue;

        send_to_char("You gain the cat's grace.\n\r", vch);
        if ( vch != ch )
            act("Your song gives $N the cat's grace.", ch, NULL, vch, TO_CHAR);
        
        remove_passive_bard_song(vch);

        af.modifier = 5 + level / 9;
        af.location = APPLY_AGI;
        affect_to_char(vch, &af);
        check_improve(ch, gsn_cats_grace, TRUE, 3);
    }
}

void apply_bard_song_affect(CHAR_DATA *ch, int song)
{
    AFFECT_DATA af;

    af.where     = TO_AFFECTS;
    af.level     = ch->level;
    af.duration  = -1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SONG;
    if (song == SONG_COMBAT_SYMPHONY)
    {
        af.type      = gsn_combat_symphony;
        affect_to_char(ch, &af);
        af.bitvector = AFF_REFRESH;
        affect_to_char(ch, &af);
    }
    else if (song == SONG_DEVESTATING_ANTHEM)
    {
        af.type      = gsn_devestating_anthem;
        affect_to_char(ch, &af);  
        af.bitvector = AFF_DEVESTATING_ANTHEM;
        affect_to_char(ch, &af);   
    }
    else if (song == SONG_REFLECTIVE_HYMN)
    {
        af.type      = gsn_reflective_hymn;
        affect_to_char(ch, &af);
        af.bitvector = AFF_REFLECTIVE_HYMN;
        affect_to_char(ch, &af);
    }
    else if (song == SONG_LULLABY)
    {
        af.type      = gsn_lullaby;
        affect_to_char(ch, &af);
        af.bitvector = AFF_LULLABY;
        affect_to_char(ch, &af);  
    }
    else if (song == SONG_DEADLY_DANCE)
    {
        af.type      = gsn_deadly_dance;
        affect_to_char(ch, &af);
        af.bitvector = AFF_DEADLY_DANCE;
        affect_to_char(ch, &af);     
    }
}

void apply_bard_song_affect_to_group(CHAR_DATA *ch)
{
    CHAR_DATA *gch;
    int song = ch->song;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group(gch, ch) )
        {
            apply_bard_song_affect(gch, song);
        }
    }
}

// completely ripped off do_stance. not ashamed
DEF_DO_FUN(do_sing)
{
    int i;

    if (argument[0] == '\0')
    {
        if (ch->song == 0)
        {
            send_to_char("What song do you wish to sing?\n\r", ch);
        } else {
            remove_bard_song_group(ch);
            send_to_char("You are no longer singing.\n\r", ch);
            ch->song = 0;
        }
        return;
    }

    // only search known songs
    for ( i = 0; songs[i].name != NULL; i++ )
    {
        if ( !str_prefix(argument, songs[i].name) && get_skill(ch, *(songs[i].gsn)) )
        {
            break;
        }
    }
    if ( songs[i].name == NULL )
    {
        if (ch->song && songs[ch->song].name)
        {
            char buffer[80];
            if (sprintf(buffer, "You don't know that song so you continue to sing %s.\n\r", songs[ch->song].name) > 0)
            {
                send_to_char(buffer, ch);
                return;
            }
        }

        send_to_char("You don't know that song.\n\r", ch);
        return;
    }

    if ( ch->song == i )
    {
        send_to_char("You're already singing that song.\n\r", ch);
        return;
    }

    ch->song = i;

    printf_to_char(ch, "You begin singing the %s.\n\r", songs[i].name);
    if ( ch->fighting != NULL )
    {
        char buf[MSL];
        sprintf( buf, "$n begins to sing the %s.", songs[i].name);
        act( buf, ch, NULL, NULL, TO_ROOM );
    }

    // make sure any songs already applied are taken away first
    remove_bard_song_group(ch);
    apply_bard_song_affect_to_group(ch);
    
}

int check_bard_room(CHAR_DATA *ch)
{
    CHAR_DATA *gch;
    int song = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group(gch, ch) )
        {
            if (gch->song != 0)
            {
                song = gch->song;
                break;
            }
        }
    }
    return song;
}

void check_bard_song(CHAR_DATA *ch)
{
    int song = ch->song;
    
    if (song != 0)
    {   
        // if they're fighting we'll deduct cost in fight.c
        if (ch->fighting == NULL)
        {
            deduct_song_cost(ch);
            check_improve(ch, *(songs[song].gsn), TRUE, 3);
        }
    }
    int group_song = check_bard_room(ch);
    if ( IS_AFFECTED(ch, AFF_SONG) )
    {
        if (group_song == 0)
        {
            remove_bard_song(ch);
        }
    } else {
        if (group_song != 0)
        {
            apply_bard_song_affect(ch, group_song);
        }
    }
}

void remove_bard_song(CHAR_DATA *ch)
{
    if ( IS_AFFECTED(ch, AFF_SONG) )
    {
        affect_strip_flag(ch, AFF_SONG);
    }
}

void remove_bard_song_group( CHAR_DATA *ch )
{   
    CHAR_DATA *gch;

    if (ch->song != 0)
    {
        for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
        {
            if ( is_same_group(gch, ch) && IS_AFFECTED(gch, AFF_SONG) )
            {
                affect_strip_flag(gch, AFF_SONG);
            }
        }
    }
}

int song_cost( CHAR_DATA *ch )
{   
    int song = ch->song;
    int sn = *(songs[song].gsn);
    int skill = get_skill(ch, sn);
    int cost = songs[song].cost * (140-skill)/40;

    return cost;
}

void deduct_song_cost( CHAR_DATA *ch )
{
    int cost;

    if (ch->song == 0) return;

    cost = song_cost(ch);
    if (cost > ch->mana)
    {
        send_to_char("You are too tired to keep singing that song.\n\r", ch);
        ch->song = 0;
    } else {
        ch->mana -= cost;
    }
}

void remove_passive_bard_song( CHAR_DATA *ch )
{
    if (IS_AFFECTED(ch, AFF_PASSIVE_SONG))
    {
        affect_strip_flag(ch, AFF_PASSIVE_SONG);
    }
}
