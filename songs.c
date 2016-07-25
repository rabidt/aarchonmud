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

static void wail_at( CHAR_DATA *ch, CHAR_DATA *victim, int level, int dam )
{
    int song = ch->song;

    if ( is_safe(ch, victim) )
        return;
    
    if ( saves_physical(victim, ch, level, DAM_SOUND) )
    {
        // half damage and no extra effects
        full_dam(ch, victim, dam/2, gsn_wail, DAM_SOUND, TRUE);
        return;
    }

    full_dam(ch, victim, dam, gsn_wail, DAM_SOUND, TRUE);
    
    // bonus based on current song
    if ( song == SONG_LULLABY )
    {
        AFFECT_DATA af;

        if ( IS_UNDEAD(victim) || IS_SET(victim->imm_flags, IMM_SLEEP) || IS_IMMORTAL(victim) )
            return;
    
        if ( saves_spell(victim, ch, level, DAM_MENTAL)
                || number_bits(2)
                || (!IS_NPC(victim) && number_bits(1)) )
            return;

        send_to_char("You feel very sleepy ..... zzzzzz.\n\r", victim);
        act("$n goes to sleep.", victim, NULL, NULL, TO_ROOM);
        stop_fighting(victim, TRUE);
        set_pos(victim, POS_SLEEPING);
    
        af.where     = TO_AFFECTS;
        af.type      = gsn_sleep;
        af.level     = level;
        af.duration  = 1;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_SLEEP;
        affect_join( victim, &af );
    }
    else if ( song == SONG_REFLECTIVE_HYMN )
    {
        AFFECT_DATA af;

        if (IS_AFFECTED(victim, AFF_IRON_MAIDEN))
        {
            return;
        }

        if ( !saves_spell(victim, ch, level, DAM_MENTAL) )
        {
            send_to_char( "You're being tortured with 1000 needles!\n\r", victim );
            act( "$n is being tortured by 1000 needles!", victim, NULL, NULL, TO_ROOM );
            af.where     = TO_AFFECTS;
            af.type      = gsn_iron_maiden;
            af.level     = level;
            af.duration  = 0;
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = AFF_IRON_MAIDEN;
            affect_to_char( victim, &af );
        }
    }
    else if ( song == SONG_COMBAT_SYMPHONY )
    {
        int drain = dam / 2;
        victim->move = UMAX(0, victim->move - drain);
    }
    else if ( song == SONG_BATTLE_DIRGE )
    {
        if ( !saves_spell(victim, ch, level, DAM_MENTAL) )
        {
            send_to_char( "You stop in your tracks!\n\r", victim );
            act("$n stands perfectly still.",victim,NULL,NULL,TO_ROOM);
            victim->stop += 1;
            victim->wait += PULSE_VIOLENCE;
        }
    }
}

// basic damage, plus extra effect based on current song
DEF_DO_FUN(do_wail)
{
    CHAR_DATA *victim;
    int skill, song = ch->song;

    if ( (skill = get_skill(ch, gsn_wail)) == 0 )
    {
        send_to_char("You scream your lungs out without effect.\n\r", ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL)
        return;
    
    // wailing consumes both mana and moves - part magic, part big lungs
    // bonus cost based on current mana/move
    float mastery_factor = (100 + mastery_bonus(ch, gsn_wail, 20, 25)) / 100.0;
    int mana_cost = skill_table[gsn_wail].min_mana + ch->mana * mastery_factor / 200;
    int move_cost = skill_table[gsn_wail].min_mana + ch->move * mastery_factor / 200;
    
    if ( ch->mana < mana_cost || ch->move < move_cost )
    {
        send_to_char("You are too exhausted to wail effectively.\n\r", ch);
        return;
    }

    reduce_mana(ch,  mana_cost);
    ch->move -= move_cost;
    WAIT_STATE(ch, skill_table[gsn_wail].beats);
    
    int level = ch->level * (100 + skill) / 200;
    int dam = martial_damage(ch, victim, gsn_wail) * (100 + skill) / 200;
    // cost-based bonus damage to primary target
    int bonus = dice(2 * (mana_cost + move_cost), 4);
    // song-based bonus
    if ( song == SONG_DEVASTATING_ANTHEM )
        dam *= 1.5;
    else if ( song == SONG_ARCANE_ANTHEM )
        // higher level means harder to resist
        level = UMIN(level * 1.5, 200);

    wail_at(ch, victim, level, dam + bonus);
    // make this a room skill if affected by deadly dance
    if ( song == SONG_DEADLY_DANCE )
    {
        CHAR_DATA *vch, *vch_next;
        for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( is_opponent(ch, vch) && vch != victim )
                wail_at(ch, vch, level, dam * AREA_SPELL_FACTOR);
        }
    }
    check_improve(ch, gsn_wail, TRUE, 3);
}

static void apply_bard_song_affect(CHAR_DATA *ch, int song_num, int level)
{
    AFFECT_DATA af;

    af.where     = TO_AFFECTS;
    af.level     = level;
    af.duration  = -1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SONG;
    
    if (song_num == SONG_COMBAT_SYMPHONY)
    {
        af.type      = gsn_combat_symphony;
        affect_to_char(ch, &af);
        af.bitvector = AFF_REFRESH;
        affect_to_char(ch, &af);
    }
    else if (song_num == SONG_DEVASTATING_ANTHEM)
    {
        af.type      = gsn_devastating_anthem;
        affect_to_char(ch, &af);  
        af.bitvector = AFF_DEVASTATING_ANTHEM;
        affect_to_char(ch, &af);   
    }
    else if (song_num == SONG_REFLECTIVE_HYMN)
    {
        af.type      = gsn_reflective_hymn;
        affect_to_char(ch, &af);
        af.bitvector = AFF_REFLECTIVE_HYMN;
        affect_to_char(ch, &af);
    }
    else if (song_num == SONG_LULLABY)
    {
        af.type      = gsn_lullaby;
        affect_to_char(ch, &af);
        af.bitvector = AFF_LULLABY;
        affect_to_char(ch, &af);  
    }
    else if (song_num == SONG_DEADLY_DANCE)
    {
        af.type      = gsn_deadly_dance;
        affect_to_char(ch, &af);
        af.bitvector = AFF_DEADLY_DANCE;
        affect_to_char(ch, &af);     
    }
    else if (song_num == SONG_ARCANE_ANTHEM)
    {
        af.type      = gsn_arcane_anthem;
        affect_to_char(ch, &af);
        af.bitvector = AFF_ARCANE_ANTHEM;
        affect_to_char(ch, &af);     
    }
    else if (song_num == SONG_BATTLE_DIRGE)
    {
        af.type      = gsn_battle_dirge;
        affect_to_char(ch, &af);
        af.bitvector = AFF_BATTLE_DIRGE;
        affect_to_char(ch, &af);
    }
}

// completely ripped off do_stance. not ashamed
DEF_DO_FUN(do_sing)
{
    int i;

    if ( argument[0] == '\0' )
    {
        send_to_char("What song do you wish to sing?\n\r", ch);
        return;
    }
    if ( !strcmp(argument, "stop") )
    {
        if ( ch->song == SONG_DEFAULT )
        {
            send_to_char("You already stopped singing.\n\r", ch);
            return;
        }
        //remove_bard_song_group(ch);
        send_to_char("You are no longer singing.\n\r", ch);
        ch->song = SONG_DEFAULT;
        check_bard_song_group(ch);
        return;
    }

    // only search known songs
    for ( i = 1; songs[i].name != NULL; i++ )
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
    // starting a song/switching takes time
    // in particular this prevents spamming for reflective hymn refresh
    WAIT_STATE(ch, PULSE_VIOLENCE);

    printf_to_char(ch, "You begin singing the %s.\n\r", songs[i].name);
    if ( ch->fighting != NULL )
    {
        char buf[MSL];
        sprintf( buf, "$n begins to sing the %s.", songs[i].name);
        act( buf, ch, NULL, NULL, TO_ROOM );
    }

    // make sure any songs already applied are taken away first
    check_bard_song_group(ch);
    //remove_bard_song_group(ch);
    //apply_bard_song_affect_to_group(ch);
    
}


void get_bard_level_and_song(CHAR_DATA *ch, int *level, int *song)
{
    *level = 0;
    *song = 0;

    CHAR_DATA *gch;

    // first pick the leader's song
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group(gch, ch) && gch->leader != NULL)
        {
            if (gch->leader->song !=0)
            {
                *level = gch->leader->level;
                *song = gch->leader->song;
            }
        }
    }

    // we found our leader's song, return out
    if (*level != 0 && *song != 0) return;

    // then, if leader not singing, pick a "random" song
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group(gch, ch) )
        {
            if (gch->song != 0)
            {
                *level = gch->level;
                *song = gch->song;
            }
        }
    }
}

void check_bard_song(CHAR_DATA *ch)
{
    // make sure the bard in the room
    if (!ch->in_room) return;

    // first check to assign bard song to himself
    if (ch->song != 0)
    {
        remove_bard_song(ch);
        apply_bard_song_affect(ch, ch->song, ch->level);

        // remove cost if not fighting
        if (ch->fighting == NULL)
        {
            deduct_song_cost(ch);
            check_improve(ch, *songs[ch->song].gsn, TRUE, 3);
        }
        return;
    }

    int song, level;
    get_bard_level_and_song(ch, &level, &song);

    if (song == 0)
    {
        remove_bard_song(ch);
    } else {
        remove_bard_song(ch);
        apply_bard_song_affect(ch, song, level);
    }
}

void check_bard_song_group(CHAR_DATA *ch)
{
    CHAR_DATA *gch;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group(gch, ch) )
        {
            check_bard_song(gch);
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

int song_cost( CHAR_DATA *ch, int song )
{   
//    int song = ch->song;
    int sn = *(songs[song].gsn);
    int skill = get_skill(ch, sn);
    int cost = songs[song].cost * (140-skill)/40;

    cost -= cost * mastery_bonus(ch, sn, 10, 20) / 100;

    return cost;
}


void deduct_song_cost( CHAR_DATA *ch )
{
    int cost, skill;
    OBJ_DATA *instrument;

    if (ch->song == 0) return;

    instrument = get_eq_char(ch, WEAR_HOLD);
    skill = get_skill(ch, gsn_instrument);
    cost = song_cost(ch, ch->song);

    if (instrument != NULL && IS_OBJ_STAT(instrument, ITEM_INSTRUMENT) && skill > 0)
    {   
        cost -= (cost*3*skill)/1000;
        check_improve(ch, gsn_instrument, TRUE, 3);
    }

    // more expensive if not fighting/standing
    if (ch->position == POS_RESTING || ch->position == POS_SITTING)
    {
        cost *= 1.5;
    }

    // and also more expensive if not fighting since it deducts every tick instead of round
    if (ch->fighting == NULL)
    {
        cost *= 7.5;
    }

    if (cost > ch->mana)
    {
        send_to_char("You are too tired to keep singing that song.\n\r", ch);
        ch->song = 0;
        remove_bard_song_group(ch);
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

int get_lunge_skill( CHAR_DATA *ch )
{
    OBJ_DATA *wield;
    int chance = 0;

    wield = get_eq_char(ch, WEAR_WIELD);
    if (wield->weight <= 50 && wield->value[0] == WEAPON_SWORD)
    {
        chance = get_skill(ch, gsn_lunge) * 2/3;
        chance += mastery_bonus(ch, gsn_lunge, 15, 25);
    }

    check_improve(ch, gsn_lunge, TRUE, 5);
    return chance;
}
