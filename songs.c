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
    return;
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
            song = gch->song;
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
        }
    }
    if ( IS_AFFECTED(ch, AFF_SONG) )
    {
        int group_song = check_bard_room(ch);
        if (group_song == 0)
        {
            remove_bard_song(ch);
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
