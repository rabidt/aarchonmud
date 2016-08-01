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

bool is_song( int sn )
{
    int song;
    
    if ( sn == 0 )
        return FALSE;
    
    for ( song = 1; songs[song].name; song++ )
        if ( *songs[song].gsn == sn )
            return TRUE;
    
    return FALSE;
}

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
    
    if (song_num == SONG_COMBAT_SYMPHONY)
    {
        af.type      = gsn_combat_symphony;
        af.bitvector = AFF_REFRESH;
        affect_to_char(ch, &af);
    }
    else if (song_num == SONG_DEVASTATING_ANTHEM)
    {
        af.type      = gsn_devastating_anthem;
        af.bitvector = AFF_DEVASTATING_ANTHEM;
        affect_to_char(ch, &af);   
    }
    else if (song_num == SONG_REFLECTIVE_HYMN)
    {
        af.type      = gsn_reflective_hymn;
        af.bitvector = AFF_REFLECTIVE_HYMN;
        affect_to_char(ch, &af);
    }
    else if (song_num == SONG_LULLABY)
    {
        af.type      = gsn_lullaby;
        af.bitvector = AFF_LULLABY;
        affect_to_char(ch, &af);  
    }
    else if (song_num == SONG_DEADLY_DANCE)
    {
        af.type      = gsn_deadly_dance;
        af.bitvector = AFF_DEADLY_DANCE;
        affect_to_char(ch, &af);     
    }
    else if (song_num == SONG_ARCANE_ANTHEM)
    {
        af.type      = gsn_arcane_anthem;
        af.bitvector = AFF_ARCANE_ANTHEM;
        affect_to_char(ch, &af);     
    }
    else if (song_num == SONG_BATTLE_DIRGE)
    {
        af.type      = gsn_battle_dirge;
        af.bitvector = AFF_BATTLE_DIRGE;
        affect_to_char(ch, &af);
    }
    else if (song_num == SONG_LONESOME_MELODY)
    {
        af.type      = gsn_lonesome_melody;
        af.bitvector = AFF_LONESOME_MELODY;
        af.location  = APPLY_DAMROLL;
        af.modifier  = 10 + ch->level;
        affect_to_char(ch, &af);
        af.location  = APPLY_HITROLL;
        affect_to_char(ch, &af);
        af.modifier  = -2*ch->level;
        af.location  = APPLY_AC;
        affect_to_char(ch, &af);
        return;
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
}

DEF_DO_FUN(do_riff)
{
    CHAR_DATA *victim, *gch;
    OBJ_DATA *instrument = get_eq_char(ch, WEAR_HOLD);
    int skill = get_skill(ch, gsn_riff), instrument_skill = get_skill(ch, gsn_instrument);

    if (!instrument || !IS_OBJ_STAT(instrument, ITEM_INSTRUMENT))
    {
        send_to_char("You need an instrument to truly riff.\n\r", ch);
        return;
    }

    if ( skill == 0 )
    {
        send_to_char("You don't know how to riff.\n\r",ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
    {
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_riff].beats );

    if ( is_safe(ch, victim) )
        return;

    // give level a 10% bonus for having to look at two skills
    int level = ch->level * (110 + instrument_skill + skill) / 300;
    int dam = martial_damage(ch, victim, gsn_riff);
    // bonus based on number of group members
    int bonus = 3;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if (is_same_group(gch, ch) && gch != ch && bonus > 1)
            bonus--;
    }
    
    dam *= bonus;

    int move = skill_table[gsn_riff].min_mana;
    int move_cost = IS_AFFECTED(ch, AFF_BARDIC_KNOWLEDGE) ? ch->move / 100 : move * (200-skill) / 100;
    move_cost += move_cost * mastery_bonus(ch, gsn_riff, 15, 25) / 100;
    ch->move -= move_cost;

    int move_bonus = IS_AFFECTED(ch, AFF_BARDIC_KNOWLEDGE) ? ch->move / 100 : 20;
    dam += move_bonus * 5;

    if ( saves_physical(victim, ch, level, DAM_MENTAL) )
    {
        // no damage
        full_dam(ch, victim, 0, gsn_riff, DAM_MENTAL, TRUE);
        check_improve(ch, gsn_riff, FALSE, 3);
        return;
    }

    full_dam(ch, victim, dam, gsn_riff, DAM_MENTAL, TRUE);
    check_improve(ch, gsn_riff, TRUE, 3);        
}

void get_bard_level_and_song(CHAR_DATA *ch, int *level, int *song)
{
    *level = 0;
    *song = 0;

    CHAR_DATA *gch;

    // first pick the leader's song
    if ( ch->leader && ch->in_room == ch->leader->in_room )
    {
        int lsong = ch->leader->song;
        if ( lsong != 0 && !songs[lsong].solo )
        {
            *level = ch->leader->level;
            *song = lsong;
            return;
        }
    }

    // then, if leader not singing, pick a "random" song
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( !is_same_group(gch, ch) )
            continue;
        
        int gsong = gch->song;
        if ( gsong != 0 && !songs[gsong].solo )
        {
            *level = gch->level;
            *song = gsong;
            return;
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
       
        // remove cost
        int cost = song_cost(ch, ch->song);
        if (cost < ch->mana)
        {
            deduct_song_cost(ch);
            check_improve(ch, *songs[ch->song].gsn, TRUE, 3);
        } else {
            send_to_char("You are too tired to keep singing that song.\n\r", ch);
            ch->song = 0;
            check_bard_song_group(ch);
        }

        apply_bard_song_affect(ch, ch->song, ch->level);
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
    affect_strip_song(ch);
}

int song_cost( CHAR_DATA *ch, int song )
{   
    int sn = *(songs[song].gsn);
    int skill = get_skill(ch, sn), instrument_skill = get_skill(ch, gsn_instrument);
    int cost = songs[song].cost * (140-skill)/40;
    OBJ_DATA *instrument = get_eq_char(ch, WEAR_HOLD);


    cost -= cost * mastery_bonus(ch, sn, 10, 20) / 100;

    if (instrument && IS_OBJ_STAT(instrument, ITEM_INSTRUMENT) && instrument_skill > 0)
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

    return cost;
}


void deduct_song_cost( CHAR_DATA *ch )
{
    int cost;

    if (ch->song == 0) return;

    cost = song_cost(ch, ch->song);

    ch->mana -= cost;
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
    OBJ_DATA *wield, *held, *shield;
    int chance = 0;

    wield = get_eq_char(ch, WEAR_WIELD);
    held = get_eq_char(ch, WEAR_HOLD);
    shield = get_eq_char(ch, WEAR_SHIELD);

    // make sure they're wearing the stuff for lunge
    if ( !(wield && held) || shield ) return 0;

    chance = get_skill(ch, gsn_lunge) * 2/3;
    chance += mastery_bonus(ch, gsn_lunge, 15, 25);

    check_improve(ch, gsn_lunge, TRUE, 5);
    return chance;
}
