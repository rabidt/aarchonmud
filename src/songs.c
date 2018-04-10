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

bool has_instrument( CHAR_DATA *ch )
{
    OBJ_DATA *instrument = get_eq_char(ch, WEAR_HOLD);
    return instrument && IS_OBJ_STAT(instrument, ITEM_INSTRUMENT);
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
            af.duration  = 1;
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
    int mana_cost = skill_table[gsn_wail].min_mana;
    int move_cost = skill_table[gsn_wail].min_mana;
    // bonus cost based on current mana/move
    if ( IS_AFFECTED(ch, AFF_BARDIC_KNOWLEDGE) )
    {
        float mastery_factor = (100 + mastery_bonus(ch, gsn_wail, 20, 25)) / 100.0;
        mana_cost += ch->mana * mastery_factor / 200;
        move_cost += ch->move * mastery_factor / 200;
    }
    
    if ( ch->mana < mana_cost || ch->move < move_cost )
    {
        send_to_char("You are too exhausted to wail effectively.\n\r", ch);
        return;
    }

    reduce_mana(ch,  mana_cost);
    ch->move -= move_cost;
    if ( song == SONG_FURIOUS_BALLAD )
        WAIT_STATE(ch, skill_table[gsn_wail].beats * 2/3);
    else
        WAIT_STATE(ch, skill_table[gsn_wail].beats);
    
    int level = ch->level * (100 + skill) / 200;
    int dam = martial_damage(ch, victim, gsn_wail) * (100 + skill) / 200;
    
    // cost-based bonus damage to primary target
    int bonus_dice = 2 * (mana_cost + move_cost);
    // riff affect is consumed for bonus
    if ( is_affected(ch, gsn_riff) )
    {
        AFFECT_DATA *aff = affect_find(ch->affected, gsn_riff);
        send_to_char("You finish your riffs with a mighty wail.\n\r", ch);
        bonus_dice += 5 * aff->modifier;
        affect_strip(ch, gsn_riff);
    }
    
    // lonesome melody can act as either devastating anthem or deadly dance
    if ( song == SONG_LONESOME_MELODY )
    {
        if ( opponent_count(ch) > 1 )
            song = SONG_DEADLY_DANCE;
        else
            song = SONG_DEVASTATING_ANTHEM;
    }
    // song-based bonus
    if ( song == SONG_DEVASTATING_ANTHEM )
        dam *= 1.5;
    else if ( song == SONG_ARCANE_ANTHEM )
        // higher level means harder to resist
        level = UMIN(level * 1.5, 200);
    // bonus dice are not multiplied
    dam += dice(bonus_dice, 4);
    
    // make this a room skill if affected by deadly dance
    if ( song == SONG_DEADLY_DANCE )
    {
        CHAR_DATA *vch, *vch_next;
        for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( vch == victim || is_opponent(ch, vch) )
                wail_at(ch, vch, level, dam * AREA_SPELL_FACTOR);
        }
    }
    else
        wail_at(ch, victim, level, dam);
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
        af.location  = APPLY_VIT;
        af.modifier  = get_skill_overflow(ch, gsn_combat_symphony) / 5;
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
        af.bitvector = AFF_DEVASTATING_ANTHEM;
        af.location  = APPLY_DAMROLL;
        af.modifier  = (20 + level) / 5;
        affect_to_char(ch, &af);
        af.bitvector = AFF_DEADLY_DANCE;
        af.location  = APPLY_HITROLL;
        affect_to_char(ch, &af);
        af.bitvector = AFF_BATTLE_DIRGE;
        af.modifier  = (20 + level) * -2;
        af.location  = APPLY_AC;
        affect_to_char(ch, &af);
    }
    else if (song_num == SONG_FURIOUS_BALLAD)
    {
        af.type      = gsn_furious_ballad;
        af.bitvector = AFF_FURY;
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
            if (snprintf( buffer, sizeof(buffer), "You don't know that song so you continue to sing %s.\n\r", songs[ch->song].name) > 0)
            {
                send_to_char(buffer, ch);
                return;
            }
        }

        send_to_char("You don't know that song.\n\r", ch);
        return;
    }

    if ( songs[i].instrumental && !has_instrument(ch) )
    {
        ptc(ch, "You require an instrument for this song.\n\r");
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
        snprintf( buf, sizeof(buf), "$n begins to sing the %s.", songs[i].name);
        act( buf, ch, NULL, NULL, TO_ROOM );
    }

    // start the song effect, which includes song cost
    check_bard_song(ch, TRUE);
    check_bard_song_group(ch);
}

DEF_DO_FUN(do_riff)
{
    CHAR_DATA *victim, *gch;
    int skill = get_skill(ch, gsn_riff);
    int instrument_skill = get_instrument_skill(ch);

    if ( !has_instrument(ch) )
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

    if ( is_safe(ch, victim) )
        return;

    int move_cost = skill_table[gsn_riff].min_mana;
    if ( ch->move < move_cost )
    {
        send_to_char("You are too exhausted to riff effectively.\n\r", ch);
        return;
    }
    
    WAIT_STATE( ch, skill_table[gsn_riff].beats );
    check_improve(ch, gsn_riff, TRUE, 3);
    check_improve(ch, gsn_instrument, TRUE, 4);
    
    int level = ch->level * (100 + skill) / 200;
    int dam = martial_damage(ch, victim, gsn_riff) * (100 + skill) / 200;
    dam += dam * instrument_skill / 200;
    int affect_cap = (20 + level) / 2 + mastery_bonus(ch, gsn_riff, 12, 20);

    // move cost determines bonus, so increasing it can be a good thing
    if ( IS_AFFECTED(ch, AFF_BARDIC_KNOWLEDGE) )
    {
        AFFECT_DATA *aff = affect_find(ch->affected, gsn_riff);
        int diff = affect_cap - (aff ? aff->modifier : 0);
        move_cost = URANGE(move_cost, diff/8, ch->move);
    }
    
    // apply riff affect to bard and possibly group members
    ch->move -= move_cost;

    AFFECT_DATA af;
    af.where        = TO_AFFECTS;
    af.type         = gsn_riff;
    af.level        = level;
    af.duration     = 1 + mastery_bonus(ch, gsn_riff, 1, 1);
    af.location     = APPLY_HITROLL;
    af.modifier     = move_cost;
    af.bitvector    = 0;

    if ( !ch->song )
    {
        affect_join_capped(ch, &af, affect_cap);
    }
    else if ( songs[ch->song].solo )
    {
        af.modifier *= 2;
        affect_join_capped(ch, &af, affect_cap);
    }
    else
    {
        for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
            if ( is_same_group(ch, gch) )
                affect_join_capped(gch, &af, affect_cap);
    }

    // finally deal some damage
    if ( saves_physical(victim, ch, level, DAM_MENTAL) )
        full_dam(ch, victim, dam/2, gsn_riff, DAM_MENTAL, TRUE);
    else
        full_dam(ch, victim, dam, gsn_riff, DAM_MENTAL, TRUE);
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

CHAR_DATA* get_singer( CHAR_DATA *ch, int song )
{
    CHAR_DATA *gch;
    
    if ( !ch || !ch->in_room )
        return NULL;

    if ( ch->song == song )
        return ch;
    
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( gch->song == song && is_same_group(gch, ch) )
            return gch;
    }
    
    return NULL;
}

void check_bard_song( CHAR_DATA *ch, bool deduct_cost )
{
    // make sure the bard in the room
    if (!ch->in_room) return;

    // first check to assign bard song to himself
    if (ch->song != 0)
    {
        remove_bard_song(ch);
       
        // remove cost
        if ( deduct_cost )
        {
            if ( songs[ch->song].instrumental && !has_instrument(ch) )
            {
                ptc(ch, "You cannot maintain your song without an instrument.\n\r");
                ch->song = 0;
                check_bard_song_group(ch);
                return;
            }
            if ( ch->position > POS_SLEEPING && deduct_song_cost(ch) )
            {
                check_improve(ch, *songs[ch->song].gsn, TRUE, 3);
                if ( check_skill(ch, gsn_song_healing) )
                    song_heal(ch);
            }
            else
            {
                send_to_char("You are too tired to keep singing that song.\n\r", ch);
                ch->song = 0;
                check_bard_song_group(ch);
                return;
            }
        }

        apply_bard_song_affect(ch, ch->song, ch->level);
        return;
    }

    int song, level;
    get_bard_level_and_song(ch, &level, &song);

    remove_bard_song(ch);
    if ( song )
        apply_bard_song_affect(ch, song, level);
}

void check_bard_song_group(CHAR_DATA *ch)
{
    CHAR_DATA *gch;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group(gch, ch) )
        {
            check_bard_song(gch, FALSE);
        }
    }
}

void remove_bard_song(CHAR_DATA *ch)
{
    affect_strip_song(ch);
}

int song_cost( CHAR_DATA *ch, int song )
{   
    if ( !song )
        return 0;
    
    int sn = *(songs[song].gsn);
    int skill = get_skill(ch, sn);
    int instrument_skill = get_instrument_skill(ch);
    int cost = skill_table[sn].min_mana * (140-skill)/40;

    cost = mastery_adjust_cost(cost, get_mastery(ch, sn));

    if ( has_instrument(ch) && instrument_skill > 0 )
    {   
        cost -= cost * (instrument_skill + mastery_bonus(ch, gsn_instrument, 30, 50)) / 300;
        check_improve(ch, gsn_instrument, TRUE, 3);
    }

    return cost;
}


bool deduct_song_cost( CHAR_DATA *ch )
{
    int cost = song_cost(ch, ch->song);
    if ( ch->mana < cost )
        return FALSE;
    reduce_mana(ch, cost);
    return TRUE;
}

void remove_passive_bard_song( CHAR_DATA *ch )
{
    if (IS_AFFECTED(ch, AFF_PASSIVE_SONG))
    {
        affect_strip_flag(ch, AFF_PASSIVE_SONG);
    }
}

int get_lunge_chance( CHAR_DATA *ch )
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

int get_instrument_skill( CHAR_DATA *ch )
{
    if ( !has_instrument(ch) )
        return 0;
    int skill = get_skill(ch, gsn_instrument);
    // wrist shield reduces effective instrument skill
    OBJ_DATA *shield = get_eq_char(ch, WEAR_SHIELD);
    if ( shield )
        skill *= (100 + get_skill(ch, gsn_wrist_shield)) / 300.0;
    return skill;
}

static void song_heal_ch( CHAR_DATA *ch, CHAR_DATA *victim, int heal )
{
    if ( victim->hit >= victim->max_hit )
        return;
    
    if ( victim == ch )
        act("Your song heals you.", ch, NULL, NULL, TO_CHAR);
    else
    {
        act("$n's song heals you.", ch, NULL, victim, TO_VICT);
        act("Your song heals $N.", ch, NULL, victim, TO_CHAR);
        heal += heal / 3;
    }
    
    heal *= (200 + get_curr_stat(victim, STAT_VIT)) / 300.0;
    gain_hit(victim, heal);
}

// heal self and possibly group, depending on song
void song_heal( CHAR_DATA *ch )
{
    if ( ch->song == SONG_DEFAULT || !ch->in_room )
        return;
    int sn = *songs[ch->song].gsn;
    int heal = get_spell_heal( skill_table[sn].min_mana, PULSE_VIOLENCE, ch->level ) / 4;
    // lullaby does nothing BUT heal
    if ( ch->song == SONG_LULLABY )
        heal *= 5;
    if ( songs[ch->song].solo )
        song_heal_ch(ch, ch, heal);
    else
    {
        heal *= AREA_SPELL_FACTOR;
        CHAR_DATA *gch;
        for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
            if ( is_same_group(ch, gch) )
                song_heal_ch(ch, gch, heal);
    }
}
