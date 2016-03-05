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
#include <math.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "tattoo.h"
#include "warfare.h"
#include "religion.h"
#include "mudconfig.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_recall    );
DECLARE_DO_FUN(do_flee      );
DECLARE_DO_FUN(do_cast      );

/*
 * Local functions.
 */
void    say_spell   args( ( CHAR_DATA *ch, int sn ) );
int get_obj_focus( CHAR_DATA *ch );
int get_dagger_focus( CHAR_DATA *ch );

/* imported functions */
bool check_spell_disabled args( (const struct skill_type *command) );
char* wear_location_info( int pos );

/*
 * Lookup a skill by name.
 */
int skill_lookup( const char *name )
{
    int sn;
    
    if ( (sn = skill_lookup_exact(name)) >= 0 )
        return sn;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( LOWER(name[0]) == LOWER(skill_table[sn].name[0])
                &&   !str_prefix( name, skill_table[sn].name ) )
            return sn;
    }

    return -1;
}

int skill_lookup_exact( const char *name )
{
    int sn;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( !strcmp(name, skill_table[sn].name) )
            return sn;
    }

    return -1;
}

int known_skill_lookup( CHAR_DATA *ch, const char *name )
{
    int sn;

    // exact match
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( !strcmp(name, skill_table[sn].name) && get_skill(ch, sn) > 0 )
            return sn;
    }
    
    // prefix match
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( !str_prefix(name, skill_table[sn].name) && get_skill(ch, sn) > 0 )
            return sn;
    }

    return -1;
}

int class_skill_lookup( int class, const char *name )
{
    int sn;

    // exact match
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( !strcmp(name, skill_table[sn].name) && is_class_skill(class, sn) > 0 )
            return sn;
    }
    
    // prefix match
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( !str_prefix(name, skill_table[sn].name) && is_class_skill(class, sn) > 0 )
            return sn;
    }

    return -1;
}

int affect_list_lookup( AFFECT_DATA *aff, const char *name )
{
    while ( aff )
    {
        if ( !str_prefix(name, skill_table[aff->type].name) )
            return aff->type;
        aff = aff->next;
    }
    return -1;
}

/*
 * Lookup a spell by name.
 */
int spell_lookup( const char *name )
{
    int sn;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;

        if ( !IS_SPELL(sn) )
            continue;

        if ( LOWER(name[0]) == LOWER(skill_table[sn].name[0])
                &&   !str_prefix( name, skill_table[sn].name ) )
            return sn;
    }

    return -1;
}

// replace sn with "similar" spell for confused character
int similar_spell( CHAR_DATA *ch, int spell )
{
    int sn, new_spell = spell;
    int found = 0;
    
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( !IS_SPELL(sn) || get_skill(ch, sn) == 0 )
            continue;
        // it's a known spell - but is it similar?
        if ( skill_table[sn].target != skill_table[spell].target
            || (ch->fighting && skill_table[sn].minimum_position > skill_table[spell].minimum_position) )
            continue;
        // chance to cast this particular spell instead
        found++;
        if ( number_range(1, found) == found )
            new_spell = sn;
    }
    return new_spell;
}

// returns a known spell if one matches and desired, otherwise a matching spell if one exists
// returning unknown spells (if no known match exists) is essential for wish casting
int find_spell( CHAR_DATA *ch, const char *name, bool known_preferred )
{
    /* finds a spell the character can cast if possible */
    int sn, found = -1;

    if (IS_NPC(ch))
        return skill_lookup(name);

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if (skill_table[sn].name == NULL)
            break;
        if ( LOWER(name[0]) == LOWER(skill_table[sn].name[0])
                && !str_prefix(name,skill_table[sn].name)
                && skill_table[sn].spell_fun != spell_null )
        {
            if ( known_preferred && get_skill(ch, sn) > 0 )
            {
                found = sn;
                break;
            }
            // ensure we still return a match if one exists, even if unknown
            // should preserve order of spells, so only overwrite if none found yet
            if ( found == -1 )
                found = sn;
        }
    }

    // confusion
    if ( found > 0 && IS_AFFECTED(ch, AFF_INSANE) && number_bits(1)==0 )
        found = similar_spell(ch, found);

    return found;
}


/*
 * Utter mystical words for an sn.
 */
void say_spell( CHAR_DATA *ch, int sn )
{
    char buf  [MAX_STRING_LENGTH];
    char buf2 [MAX_STRING_LENGTH];
    CHAR_DATA *rch;
    const char *pName;
    int iSyl;
    int length;

    struct syl_type
    {
        char *  old;
        char *  new;
    };

    static const struct syl_type syl_table[] =
    {
        { " ",      " "     },
        { "ar",     "abra"      },
        { "au",     "kada"      },
        { "bless",  "fido"      },
        { "blind",  "nose"      },
        { "bur",    "mosa"      },
        { "cu",     "judi"      },
        { "de",     "oculo"     },
        { "en",     "unso"      },
        { "light",  "dies"      },
        { "lo",     "hi"        },
        { "mor",    "zak"       },
        { "move",   "sido"      },
        { "ness",   "lacri"     },
        { "ning",   "illa"      },
        { "per",    "duda"      },
        { "ra",     "gru"       },
        { "fresh",  "ima"       },
        { "re",     "candus"    },
        { "son",    "sabru"     },
        { "tect",   "infra"     },
        { "tri",    "cula"      },
        { "ven",    "nofo"      },
        { "a", "a" }, { "b", "b" }, { "c", "q" }, { "d", "e" },
        { "e", "z" }, { "f", "y" }, { "g", "o" }, { "h", "p" },
        { "i", "u" }, { "j", "y" }, { "k", "t" }, { "l", "r" },
        { "m", "w" }, { "n", "i" }, { "o", "a" }, { "p", "s" },
        { "q", "d" }, { "r", "f" }, { "s", "g" }, { "t", "h" },
        { "u", "j" }, { "v", "z" }, { "w", "x" }, { "x", "n" },
        { "y", "l" }, { "z", "k" },
        { "", "" }
    };

    buf[0]  = '\0';
    for ( pName = skill_table[sn].name; *pName != '\0'; pName += length )
    {
        for ( iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++ )
        {
            if ( !str_prefix( syl_table[iSyl].old, pName ) )
            {
                strcat( buf, syl_table[iSyl].new );
                break;
            }
        }

        if ( length == 0 )
            length = 1;
    }

    sprintf( buf2, "$n utters the words, '%s'.", buf );
    sprintf( buf,  "$n utters the words, '%s'.", skill_table[sn].name );

    for ( rch = ch->in_room->people; rch; rch = rch->next_in_room )
    {
        if ( rch != ch )
            act((!IS_NPC(ch) && ch->class==rch->class) ? buf : buf2, ch, NULL, rch, TO_VICT );
    }

    return;
}

bool can_cast_transport( CHAR_DATA *ch )
{
    if ( IS_REMORT(ch) )
    {
        send_to_char( "Not in remort!\n\r", ch );
        return FALSE;
    }

    if ( IS_TAG(ch) )
    {
        send_to_char( "Not while playing tag!\n\r", ch );
        return FALSE;
    }

    if ( PLR_ACT(ch, PLR_WAR) )
    {
        send_to_char( "Not during warfare!\n\r", ch );
        return FALSE;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) )
    {
        send_to_char( "The gods refuse to let you go!\n\r", ch );
        return FALSE;
    }

    if ( ch->pcdata != NULL && ch->pcdata->pkill_timer > 0 )
    {
        send_to_char( "Adrenaline is pumping!\n\r", ch );
        return FALSE;
    }

    if ( IS_AFFECTED(ch, AFF_ROOTS) )
    {
        send_to_char( "Not with your roots sunk into the ground!\n\r", ch );
        return FALSE;
    }

    /*
    if ( carries_relic(ch) )
    {
        send_to_char( "Not with a relic!\n\r", ch );
        return FALSE;
    }
    */

    return TRUE;
}
/* returns wether a skill is offensive
 */
bool is_offensive( int sn )
{
    int target = skill_table[sn].target;

    return target == TAR_CHAR_OFFENSIVE
        || target == TAR_OBJ_CHAR_OFF
        || target == TAR_VIS_CHAR_OFF
        || target == TAR_IGNORE_OFF;
}

bool is_malediction( int sn )
{
    if ( !strcmp(skill_table[sn].name, "energy drain") )
        return TRUE;
    
    return is_offensive(sn) && skill_table[sn].duration != DUR_NONE;
}

int get_save(CHAR_DATA *ch, bool physical)
{
    int saves = ch->saving_throw;
    int save_factor = 100;
    int level = modified_level(ch);
    
    // level bonus
    if ( IS_NPC(ch) )
    {
        if ( IS_SET(ch->act, ACT_MAGE) && !physical )
            save_factor += 30;
        if ( IS_SET(ch->act, ACT_WARRIOR) && !physical )
            save_factor -= 30;
    }
    else
    {
        int physical_factor = class_table[ch->class].attack_factor * 3/5 + class_table[ch->class].defense_factor / 2;
        save_factor = 210 - physical_factor;
        // tweak so physically oriented classes get better physical and worse magic saves
        save_factor += (physical_factor - 110) * (physical ? 2 : -1) * 2/3;
    }
    saves -= (level + 10) * save_factor/100;
    
    // WIS or VIT bonus
    int stat = physical ? get_curr_stat(ch, STAT_CON) : get_curr_stat(ch, STAT_WIS);
    saves -= (level + 10) * stat / 500;

    return saves;
}

/*
 * Compute a saving throw.
 * Negative applys make saving throw better.
 */
bool saves_spell( CHAR_DATA *victim, CHAR_DATA *ch, int level, int dam_type )
{
    int hit_roll, save_roll;
    
    /* automatic saves/failures */
    switch(check_immune(victim,dam_type))
    {
        case IS_IMMUNE:     return TRUE;
        case IS_RESISTANT:  if ( per_chance(20) ) return TRUE;  break;
        case IS_VULNERABLE: if ( per_chance(10) ) return FALSE;  break;
    }
    
    if ( IS_AFFECTED(victim, AFF_CURSE) && per_chance(5) )
        return FALSE;

    if ( IS_AFFECTED(victim, AFF_PETRIFIED) && per_chance(50) )
        return TRUE;

    if ( (victim->stance == STANCE_INQUISITION || victim->stance == STANCE_UNICORN) && per_chance(25) )
        return TRUE;
        
    if ( IS_AFFECTED(victim, AFF_PHASE) && per_chance(50) )
        return TRUE;

    if ( IS_AFFECTED(victim, AFF_PROTECT_MAGIC) && per_chance(20) )
        return TRUE;

    /* now the resisted roll */
    save_roll = -get_save(victim, FALSE);
    hit_roll = get_spell_penetration(ch, level);

    if ( ch && ch->stance == STANCE_INQUISITION )
        hit_roll += hit_roll / 3;
    else if ( ch && ch->stance == STANCE_DECEPTION && dam_type == DAM_MENTAL )
        hit_roll += hit_roll / 3;

    if ( save_roll <= 0 )
        return FALSE;
    else
    {
        int hit_rolled = number_range(0, hit_roll);
        int save_rolled = number_range(0, save_roll);
        bool success = hit_rolled <= save_rolled;
        if ( cfg_show_rolls )
        {
            char buf[MSL];
            sprintf(buf, "Saving throw vs spell: %s rolls %d / %d, %s rolls %d / %d => %s\n\r",
                    ch ? ch_name(ch) : "attacker", hit_rolled, hit_roll,
                    ch_name(victim), save_rolled, save_roll,
                    success ? "success" : "failure");
            send_to_char(buf, victim);
            if ( ch && ch != victim )
                send_to_char(buf, ch);
        }
        return success;
    }
}

bool saves_physical( CHAR_DATA *victim, CHAR_DATA *ch, int level, int dam_type )
{
    int hit_roll, save_roll;

    /* automatic saves/failures */

    switch(check_immune(victim,dam_type))
    {
        case IS_IMMUNE:     return TRUE;
        case IS_RESISTANT:  if ( per_chance(20) ) return TRUE;  break;
        case IS_VULNERABLE: if ( per_chance(10) ) return FALSE;  break;
    }

    if ( IS_AFFECTED(victim, AFF_CURSE) && per_chance(5) )
        return FALSE;

    if ( IS_AFFECTED(victim, AFF_PETRIFIED) && per_chance(50) )
        return TRUE;

    if ( IS_AFFECTED(victim, AFF_BERSERK) && per_chance(10) )
        return TRUE;

    /* now the resisted roll */
    save_roll = -get_save(victim, TRUE);
    if ( ch )
    {
        int size_diff = ch->size - victim->size;
        hit_roll = (level + 10) * (500 + get_curr_stat(ch, STAT_STR) + 20 * size_diff) / 500;
    }
    else
        hit_roll = (level + 10) * 6/5;

    if ( save_roll <= 0 )
        return FALSE;
    else
    {
        int hit_rolled = number_range(0, hit_roll);
        int save_rolled = number_range(0, save_roll);
        bool success = hit_rolled <= save_rolled;
        if ( cfg_show_rolls )
        {
            char buf[MSL];
            sprintf(buf, "Saving throw vs physical: %s rolls %d / %d, %s rolls %d / %d => %s\n\r",
                    ch ? ch_name(ch) : "attacker", hit_rolled, hit_roll,
                    ch_name(victim), save_rolled, save_roll,
                    success ? "success" : "failure");
            send_to_char(buf, victim);
            if ( ch && ch != victim )
                send_to_char(buf, ch);
        }
        return success;
    }
}

/* RT save for dispels */

bool saves_dispel( int dis_level, int spell_level, int duration )
{
    /* very hard to dispel permanent effects */
    if ( duration == -1 && number_bits(1) )
        return TRUE;

    int off_roll = number_range(0, 20 + dis_level);
    int def_roll = number_range(0, 20 + spell_level);
    return off_roll <= def_roll;
}

static void dispel_sn( CHAR_DATA *victim, int sn )
{
    char buf[MAX_STRING_LENGTH];
    
    affect_strip(victim, sn);
    if ( skill_table[sn].msg_off )
    {
        send_to_char( skill_table[sn].msg_off, victim );
        send_to_char( "\n\r", victim );
        sprintf(buf, "The %s %s on $n vanishes.", skill_table[sn].name, IS_SPELL(sn) ? "spell" : "affect");
        act(buf,victim,NULL,NULL,TO_ROOM);
    }
}

/* co-routine for dispel magic and cancellation */

bool check_dispel( int dis_level, CHAR_DATA *victim, int sn )
{
    AFFECT_DATA *af;
    bool found = FALSE;

    /* some affects are hard to dispel */
    if ( (sn == gsn_prot_magic && number_bits(1))
            || (sn == gsn_reflection && number_bits(1))
            || (sn == gsn_deaths_door && number_bits(2)) )
        return FALSE;

    /* tomb rot makes negative effects harder to dispel */
    if ( IS_AFFECTED(victim, AFF_TOMB_ROT)
            && is_offensive(sn)
            && number_bits(2) )
        return FALSE;

    if (is_affected(victim, sn))
    {
        for ( af = victim->affected; af != NULL; af = af->next )
        {
            if ( af->type == sn )
            {
                // track if we have already found the affect, so we only attempt dispel once
                if ( !found && !saves_dispel(dis_level, af->level, af->duration) )
                {
                    dispel_sn(victim, sn);
                    return TRUE;
                }
                found = TRUE;
                af->level--;
            }
        }
    }
    return FALSE;
}

static bool check_cancel( int dis_level, CHAR_DATA *victim, int sn )
{
    if ( is_offensive(sn) )
        return check_dispel(dis_level, victim, sn);
    if ( !is_affected(victim, sn) )
        return FALSE;
    // for beneficial spells we automatically succeed
    dispel_sn(victim, sn);
    return TRUE;
}

/* returns wether an affect can be dispelled or canceled 
 * called by spell_dispel_magic and spell_cancelation 
 */
bool can_dispel(int sn)
{
    if (!IS_SPELL(sn))
        return FALSE;

    /* some spells have special cures */
    if ( is_curse(sn)
        || is_blindness(sn)
        || sn == gsn_poison
        || is_disease(sn) )
        return FALSE;

    /*
       if ( is_mental(sn) )
       return FALSE;
     */

    return TRUE;
}

bool check_dispel_magic(int level, CHAR_DATA *victim)
{
    int sn;
    bool found = FALSE;
    for (sn = 1; skill_table[sn].name != NULL; sn++)
        if ( can_dispel(sn) && check_dispel(level, victim, sn) )
            found = TRUE;
    return found;
}

/* check whether one player is allowed to spell up another */
bool can_spellup( CHAR_DATA *ch, CHAR_DATA *victim, int sn )
{
    CHAR_DATA *opp;

    /* aggro spells are not 'spellup' */
    if ( ch == victim || is_offensive(sn) )
        return TRUE;

    /* prevent illegal pkill assistance */
    if ( !in_pkill_battle(victim) )
        return TRUE;

    /* more illegal pkill assistance: person who recently fled cannot be assisted
       by someone who is not eligible to participate in pkill with them ...
       this will work now that all pkill follows the same level ranges. */
    if( victim->pcdata && victim->pcdata->pkill_timer > 0 && is_safe( ch, victim ) )
    {
        act( "You cannot help $N until $E cools down.\n\r", ch, NULL, victim, TO_CHAR );
        return FALSE;
    }

    /* if char could attack one of victim's pkill opponents, spellup is allowed */
    for ( opp = victim->in_room->people; opp != NULL; opp = opp->next_in_room )
    {
        if ( IS_NPC(opp)
                || !(opp->fighting == victim || victim->fighting == opp) )
            continue;

        if ( !is_safe_spell(ch, opp, FALSE) )
            return TRUE;
    }

    act( "You cannot cast this spell on $N right now.", ch, NULL, victim, TO_CHAR );
    return FALSE;
}

int mana_cost (CHAR_DATA *ch, int sn, int skill)
{
    int mana = skill_table[sn].min_mana;
    
    mana = mana * (200-skill) / 100;
    mana = mastery_adjust_cost(mana, get_mastery(ch, sn));

    return mana;
}

/* returns wether a valid spell target was found */
bool get_spell_target( CHAR_DATA *ch, const char *arg, int sn, /* input */
        int *target, void **vo ) /* output */
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char buf[MSL];

    *vo = NULL;
    *target = skill_table[sn].target;

    /* small fix for target + extra info */
    one_argument( arg, buf );
    arg = buf;

    switch ( *target )
    {
        default:
            bug( "get_spell_target: bad target (%d)", *target );
            return FALSE;

        case TAR_IGNORE:
        case TAR_IGNORE_OFF:
        case TAR_IGNORE_OBJ:
            break;

        case TAR_VIS_CHAR_OFF:
            if (( arg[0] == '\0' )
                    && (ch->fighting != NULL)
                    && !can_see_combat(ch, ch->fighting) )
            {
                send_to_char( "You can't see your target.\n\r", ch );
                return FALSE;
            }
            *target = TAR_CHAR_OFFENSIVE;
            /* no break at the end...carry through to next case check */

        case TAR_CHAR_OFFENSIVE:
            if ( arg[0] == '\0' )
            {
                if ( ( victim = ch->fighting ) == NULL )
                {
                    send_to_char( "Cast the spell on whom?\n\r", ch );
                    return FALSE;
                }
            }
            else
            {
                if ( ( victim = get_victim_room( ch, arg ) ) == NULL )
                {
                    send_to_char( "You can't find your victim.\n\r", ch );
                    return FALSE;
                }
            }

            if ( !IS_NPC(ch) )
            {

                if (is_safe_spell(ch,victim, FALSE) && victim != ch)
                {
                    send_to_char("Not on that target.\n\r",ch);
                    return FALSE; 
                }
                check_killer(ch,victim);
            }

            if ( IS_AFFECTED(victim, AFF_CHARM) && victim->leader == ch )
            {
                send_to_char( "You can't do that to your own follower.\n\r", ch );
                return FALSE;
            }

            *vo = (void *) victim;
            *target = TARGET_CHAR;
            break;

        case TAR_CHAR_DEFENSIVE:
            if ( arg[0] == '\0' )
            {
                victim = ch;
            }
            else
            {
                if ( ( victim = get_char_room( ch, arg ) ) == NULL )
                {
                    send_to_char( "They aren't here.\n\r", ch );
                    return FALSE;
                }
            }

            *vo = (void *) victim;
            *target = TARGET_CHAR;
            break;

        case TAR_CHAR_SELF:
            if ( arg[0] != '\0' && strcmp(arg, "self") && !is_name( arg, ch->name ) )
            {
                send_to_char( "You cannot cast this spell on another.\n\r", ch );
                return FALSE;
            }

            *vo = (void *) ch;
            *target = TARGET_CHAR;
            break;

        case TAR_OBJ_INV:
            if ( arg[0] == '\0' )
            {
                send_to_char( "What should the spell be cast upon?\n\r", ch );
                return FALSE;
            }

            if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
            {
                send_to_char( "You are not carrying that.\n\r", ch );
                return FALSE;
            }

            /* Stops enchant spells from casting when invalid targets are selected 
               -- Astark Oct 2012 */
            if (sn == gsn_enchant_armor && obj->item_type != ITEM_ARMOR)
            {
                send_to_char( "That item isn't considered armor.\n\r", ch );
                return FALSE;
            }

            if (sn == gsn_enchant_weapon && obj->item_type != ITEM_WEAPON)
            {
                send_to_char( "That item isn't considered weapon.\n\r", ch );
                return FALSE;
            }

            if (sn == gsn_enchant_arrow && obj->item_type != ITEM_ARROWS)
            {
                send_to_char( "That item isn't considered arrows.\n\r", ch );
                return FALSE;
            }
            /* End Astark's code */

            *vo = (void *) obj;
            *target = TARGET_OBJ;
            break;

        case TAR_OBJ_CHAR_OFF:
            if (arg[0] == '\0')
            {
                if ((victim = ch->fighting) == NULL
                        || (victim != NULL && !can_see_combat(ch,victim)))
                {
                    send_to_char("Cast the spell on whom or what?\n\r",ch);
                    return FALSE;
                }

                *target = TARGET_CHAR;
            }
            else if ((victim = get_victim_room(ch,arg)) != NULL)
            {
                *target = TARGET_CHAR;
            }

            if (*target == TARGET_CHAR) /* check the sanity of the attack */
            {
                if (is_safe_spell(ch,victim,FALSE) && victim != ch)
                {
                    send_to_char("Not on that target.\n\r",ch);
                    return FALSE;
                }

                if ( IS_AFFECTED(victim, AFF_CHARM) && victim->leader == ch )
                {
                    send_to_char( "You can't do that to your own follower.\n\r", ch );
                    return FALSE;
                }

                if (!IS_NPC(ch))
                    check_killer(ch,victim);

                *vo = (void *) victim;
            }
            else if ((obj = get_obj_here(ch,arg)) != NULL)
            {
                *vo = (void *) obj;
                *target = TARGET_OBJ;
            }
            else
            {
                send_to_char("You don't see that here.\n\r",ch);
                return FALSE;
            }
            break; 

        case TAR_OBJ_CHAR_DEF:
            if (arg[0] == '\0')
            {
                *vo = (void *) ch;
                *target = TARGET_CHAR;                                                 
            }
            else if ((victim = get_char_room(ch,arg)) != NULL)
            {
                *vo = (void *) victim;
                *target = TARGET_CHAR;
            }
            else if ((obj = get_obj_carry(ch,arg,ch)) != NULL)
            {
                *vo = (void *) obj;
                *target = TARGET_OBJ;
            }
            else
            {
                send_to_char("You don't see that here.\n\r",ch);
                return FALSE;
            }
            break;
        case TAR_CHAR_NEUTRAL:
            if (arg[0] == '\0')
            {
                victim = ch; /* for later check */
                *vo = (void *) ch;
            }
            else if ((victim = get_char_room(ch,arg)) != NULL)
            {
                *vo = (void *) victim;
            }
            else
            {
                send_to_char("You don't see them here.\n\r",ch);
                return FALSE;
            }
            *target = TARGET_CHAR;

            /* check if spell is wanted */
            if ( ch == victim
                    || ch->fighting == victim
                    || victim->fighting == ch
                    || (!IS_NPC(victim) && !IS_SET(victim->act, PLR_NOCANCEL))
                    || (IS_AFFECTED(victim, AFF_CHARM) && victim->master == ch) )
                break;
            else
            {
                send_to_char("Your target wouldn't like that. Initiate combat first.\n\r",ch);
                return FALSE;
            }
    }

    if ( (*target == TARGET_CHAR) && (*vo != NULL)
            && !can_spellup(ch, (CHAR_DATA*)(*vo), sn) )
        return FALSE;    

    /* catch multiplayers spelling up alts */
    if ( *target == TARGET_CHAR )
        check_sn_multiplay( ch, *vo, sn );

    /* Stops weather spells from using mana and lagging the player when
       specific parameters aren't met -- Astark Oct 2012 */

    if ((sn == gsn_hailstorm ||
                sn == gsn_call_lightning ||
                sn == gsn_monsoon) && (weather_info.sky < SKY_RAINING))
    {
        send_to_char( "The weather is much too nice for that!\n\r", ch );
        return FALSE;
    }

    if ( sn == gsn_solar_flare && (weather_info.sky >= SKY_RAINING || !room_is_sunlit(ch->in_room)) )
    {
        send_to_char( "There isn't enough sunshine out for that!\n\r", ch );
        return FALSE;
    }

    if (sn == gsn_hailstorm || 
            sn == gsn_meteor_swarm || 
            sn == gsn_call_lightning || 
            sn == gsn_monsoon || 
            sn == gsn_solar_flare)
    { 
        if (!IS_OUTSIDE(ch))
        {
            send_to_char( "INDOORS? I think not.\n\r", ch );
            return FALSE;
        } 

        if ( ch->in_room->sector_type == SECT_UNDERGROUND )
        {
            send_to_char( "There is no weather down here...\n\r", ch );
            return FALSE;
        } 
    }

    /* Smote's Anachronsm can't use mana/lag player outside of combat
       -- Astark Oct 2012 */
    if (sn == gsn_smotes_anachronism && ch->position != POS_FIGHTING)
    {
        send_to_char( "You can't cast that out of combat.\n\r", ch );
        return FALSE;
    }
    /* End Astark's code */

    return TRUE;
}

int get_duration_by_type( int type, int level )
{
    if ( IS_SET(meta_magic, META_MAGIC_PERMANENT) )
        return -1;

    int duration;

    switch ( type )
    {
        case DUR_BRIEF:   duration = (level + 20) / 8; break;
        case DUR_SHORT:   duration = (level + 20) / 4; break;
        case DUR_NORMAL:  duration = (level + 20) / 2; break;
        case DUR_LONG:    duration = (level + 20); break;
        case DUR_EXTREME: duration = (level + 20) * 2; break;
        default:          duration = 0; break;
    }

    if ( IS_SET(meta_magic, META_MAGIC_EXTEND) )
        duration = duration * 3/2;
    
    return duration;
}

int get_duration( int sn, int level )
{
    return get_duration_by_type(skill_table[sn].duration, level);
}

/* check if a spell is reflected back on the caster */
void* check_reflection( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA*) vo;

    if ( !is_offensive(sn)
            || target != TARGET_CHAR
            || vo == NULL
            || !IS_AFFECTED(victim, AFF_REFLECTION)
            || number_bits(3) )
        return vo;

    act( "The aura around $N reflects your spell back on you!", ch, NULL, victim, TO_CHAR );
    act( "The aura around you reflects $n's spell back on $m!", ch, NULL, victim, TO_VICT );
    act( "The aura around $N reflects $n's spell back on $m!", ch, NULL, victim, TO_NOTVICT );

    return (void*)ch;
}

int concentration_power( CHAR_DATA *ch )
{
    return 10 + ch->level;
}

int disruption_power( CHAR_DATA *ch )
{
    int level = ch->level;

    // non-boss NPCs don't disrupt quite so much
    if ( IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_FAST) )
        level = level * 3/4;

    return 10 + level;
}

bool check_concentration( CHAR_DATA *ch )
{
    CHAR_DATA *att;
    int ch_roll, att_roll;
    
    if ( !ch->in_room )
        return FALSE;
    
    if ( check_skill(ch, gsn_combat_casting) )
        return TRUE;
    
    ch_roll = number_range(0, concentration_power(ch));
    for ( att = ch->in_room->people; att; att = att->next_in_room )
    {
        if ( att->fighting != ch || att->stop )
            continue;
        att_roll = number_range(0, disruption_power(att));
        if ( att_roll > ch_roll )
            return FALSE;
    }
    return TRUE;
}

// global variable for storing what meta-magic skills are used
tflag meta_magic = {};

int meta_magic_sn( int meta )
{
    switch ( meta )
    {
        case META_MAGIC_EXTEND: return gsn_extend_spell;
        case META_MAGIC_EMPOWER: return gsn_empower_spell;
        case META_MAGIC_QUICKEN: return gsn_quicken_spell;
        case META_MAGIC_CHAIN: return gsn_chain_spell;
        default: return 0;
    }
}

// meta-magic casting functions
void meta_magic_cast( CHAR_DATA *ch, const char *meta_arg, const char *argument )
{
    tflag meta_flag;
    int i;
    
    if ( meta_arg[0] == '\0' )
    {
        printf_to_char(ch, "What meta-magic flags do you want to apply?\n\r");
        printf_to_char(ch, "Syntax: mmcast [cepq] <spell name> [other args]\n\r");
        return;
    }
    
    // check for spell mastery - enables meta-magic
    char spell_arg[MIL];
    one_argument(argument, spell_arg);
    int spell_sn = find_spell(ch, spell_arg, true);
    int spell_mastery = spell_sn > 0 ? get_mastery(ch, spell_sn) : 0;
    
    // parse meta-magic flags requested
    flag_clear(meta_flag);
    for ( i = 0; i < strlen(meta_arg); i++ )
    {
        // valid flag?
        int meta = 0;
        switch( meta_arg[i] )
        {
            case 'e': meta = META_MAGIC_EXTEND; break;
            case 'p': meta = META_MAGIC_EMPOWER; break;
            case 'q': meta = META_MAGIC_QUICKEN; break;
            case 'c': meta = META_MAGIC_CHAIN; break;
            default:
                printf_to_char(ch, "Invalid meta-magic option: %c\n\r", meta_arg[i]);
                return;
        }
        // character has skill?
        int sn = meta_magic_sn(meta);
        if ( get_skill(ch, sn) || spell_mastery >= 2 )
            flag_set(meta_flag, meta);
        else
            printf_to_char(ch, "You need the '%s' skill for this!\n\r", skill_table[sn].name);
    }
        
    flag_copy(meta_magic, meta_flag);
    do_cast(ch, argument);
    flag_clear(meta_magic);
}

DEF_DO_FUN(do_mmcast)
{
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg1);
    meta_magic_cast(ch, arg1, argument);
}

DEF_DO_FUN(do_ecast)
{
    meta_magic_cast(ch, "e", argument);
}

DEF_DO_FUN(do_pcast)
{
    meta_magic_cast(ch, "p", argument);
}

DEF_DO_FUN(do_qcast)
{
    meta_magic_cast(ch, "q", argument);
}

DEF_DO_FUN(do_ccast)
{
    meta_magic_cast(ch, "c", argument);
}

DEF_DO_FUN(do_permcast)
{
    flag_set(meta_magic, META_MAGIC_PERMANENT);
    do_cast(ch, argument);
    flag_clear(meta_magic);
}

int meta_magic_adjust_cost( CHAR_DATA *ch, int cost, bool base )
{
    int flag;

    // each meta-magic effect doubles casting cost
    for ( flag = 1; flag < FLAG_MAX_BIT; flag++ )
        if ( IS_SET(meta_magic, flag) && (base || flag != META_MAGIC_CHAIN) )
        {
            int sn = meta_magic_sn(flag);
            if ( sn )
                cost = cost * (200 - mastery_bonus(ch, sn, 40, 60)) / 100;
        }

    return cost;
}

int meta_magic_perm_cost( CHAR_DATA *ch, int sn )
{
    if ( !IS_SET(meta_magic, META_MAGIC_PERMANENT) )
        return 0;
    
    int mana = skill_table[sn].min_mana + skill_table[sn].beats * 10 / PULSE_VIOLENCE;
    int duration;
    
    // increase based on normal duration
    for ( duration = skill_table[sn].duration; duration < DUR_EXTREME; duration++ )
        mana *= 2;
    if ( sn == gsn_stone_skin )
        mana *= 2;
    // decrease for combat spells
    if ( skill_table[sn].minimum_position == POS_FIGHTING )
        mana /= 2;
    // decrease for extend and spell mastery
    if ( get_skill(ch, gsn_extend_spell) > 0 )
    {
        mana = mana * (100 - mastery_bonus(ch, sn, 20, 30)) / 100;
        mana = mana * (100 - mastery_bonus(ch, gsn_extend_spell, 40, 60)) / 100;
    }
    
    return mana;
}

int apply_perm_cost( CHAR_DATA *ch, int sn )
{
    int cost = meta_magic_perm_cost(ch, sn);
    if ( !cost )
        return 0;
    
    // ensure that the character has a matching permanent affect
    AFFECT_DATA *paf = affect_find(ch->affected, sn);
    if ( !paf || paf->duration >= 0 )
        return 0;
    
    // ensure we haven't applied a permanent cost already
    int level = paf->level;
    while ( paf != NULL && paf->type == sn )
    {
        if ( paf->location == APPLY_MANA_CAP && paf->modifier < 0 )
            return 0;
        paf = paf->next;
    }
    
    // time for some fun
    AFFECT_DATA af;
    af.where        = TO_AFFECTS;
    af.type         = sn;
    af.level        = level;
    af.duration     = -1;
    af.bitvector    = 0;
    af.location     = APPLY_MANA_CAP;
    af.modifier     = -cost;
    affect_to_char(ch, &af);
    
    // might have reduced max mana below current mana, due to training
    ch->mana = UMIN(ch->mana, ch->max_mana);
    
    return cost;
}

int meta_magic_adjust_wait( CHAR_DATA *ch, int wait )
{
    if ( IS_SET(meta_magic, META_MAGIC_CHAIN) )
        wait *= 2;
    
    // can't reduce below half a round (e.g. dracs)
    int min_wait = PULSE_VIOLENCE / 2;
    if ( IS_SET(meta_magic, META_MAGIC_QUICKEN) && wait > min_wait )
    {
        float quicken_factor = 100.0 / (200 + get_skill_overflow(ch, gsn_quicken_spell));
        wait = UMAX(min_wait, wait * quicken_factor);
    }

    return wait;
}

bool meta_magic_concentration_check( CHAR_DATA *ch )
{
    int flag;

    // each meta-magic effect has chance of failure
    for ( flag = 1; flag < FLAG_MAX_BIT; flag++ )
        if ( IS_SET(meta_magic, flag) )
        {
            int sn = meta_magic_sn(flag);
            if ( sn )
            {
                if ( number_bits(1) || per_chance(get_skill(ch, sn)) )
                {
                    check_improve(ch, sn, TRUE, 3);
                }
                else
                {
                    check_improve(ch, sn, FALSE, 3);
                    return FALSE;
                }
            }
        }

    return TRUE;
}

// remove invalid meta-magic effects
void meta_magic_strip( CHAR_DATA *ch, int sn, int target_type, void *vo )
{
    // can only extend spells with duration
    if ( IS_SET(meta_magic, META_MAGIC_EXTEND) )
    {
        int duration = skill_table[sn].duration;
        if ( (duration == DUR_NONE || duration == DUR_SPECIAL) && sn != skill_lookup("renewal") )
        {
            send_to_char("Only spells with standard durations can be extended.\n\r", ch);
            flag_remove(meta_magic, META_MAGIC_EXTEND);
        }
    }
    
    // similar for infinite spells, but further restricted to personal
    if ( IS_SET(meta_magic, META_MAGIC_PERMANENT) )
    {
        int duration = skill_table[sn].duration;
        if ( duration == DUR_NONE || duration == DUR_SPECIAL )
        {
            send_to_char("Only spells with standard durations can be made permanent.\n\r", ch);
            flag_remove(meta_magic, META_MAGIC_PERMANENT);
        }
        else if ( target_type != TARGET_CHAR || ch != vo )
        {
            send_to_char("Only spells cast on yourself can be made permanent.\n\r", ch);
            flag_remove(meta_magic, META_MAGIC_PERMANENT);
        }
        // must have extend spell or spell grandmastered
        else if ( get_skill(ch, gsn_extend_spell) < 1 && get_mastery(ch, sn) < 2 )
        {
            ptc(ch, "You must first learn extend spell or achieve grand mastery in %s.\n\r", skill_table[sn].name);
            flag_remove(meta_magic, META_MAGIC_PERMANENT);
        }
    }
    
    // can only quicken spells with longish casting time
    if ( IS_SET(meta_magic, META_MAGIC_QUICKEN) )
    {
        int wait = skill_table[sn].beats;
        int min_wait = PULSE_VIOLENCE / 2;
        if ( wait <= min_wait )
        {
            send_to_char("This spell cannot be quickened any further.\n\r", ch);
            flag_remove(meta_magic, META_MAGIC_QUICKEN);
        }
    }

    // can only chain single-target non-personal spells
    if ( IS_SET(meta_magic, META_MAGIC_CHAIN) )
    {
        int target = skill_table[sn].target;

        if ( target == TAR_CHAR_SELF )
        {
            send_to_char("Personal spells cannot be chained.\n\r", ch);
            flag_remove(meta_magic, META_MAGIC_CHAIN);
        }

        if ( target == TAR_IGNORE
            || target == TAR_IGNORE_OFF
            || target == TAR_IGNORE_OBJ
            || sn == skill_lookup("betray")
            || sn == skill_lookup("chain lightning") )
        {
            send_to_char("Only single-target spells can be chained.\n\r", ch);
            flag_remove(meta_magic, META_MAGIC_CHAIN);
        }

        if ( target_type == TARGET_OBJ )
        {
            send_to_char("Spells targeting objects cannot be chained.\n\r", ch);
            flag_remove(meta_magic, META_MAGIC_CHAIN);
        }
    }
}

void post_spell_process( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim )
{
    // spell triggers
    if ( victim != NULL && IS_NPC(victim) && mp_spell_trigger(skill_table[sn].name, victim, ch) )
        return; // Return because it might have killed the victim or ch

    if ( is_offensive(sn) && victim != ch && victim->in_room == ch->in_room
         && victim->fighting == NULL && victim->position > POS_SLEEPING
         && !is_same_group(ch, victim) )
    {
        set_fighting(victim, ch);
    }
    
    // mystic infusion heals or harms
    if ( !was_obj_cast && per_chance(get_skill(ch, gsn_mystic_infusion)) )
    {
        int target = skill_table[sn].target;
        if ( (target == TAR_CHAR_OFFENSIVE || target == TAR_VIS_CHAR_OFF || target == TAR_OBJ_CHAR_OFF)
            && victim != ch && victim->in_room == ch->in_room
            && victim->position > POS_SLEEPING && !is_same_group(ch, victim) )
        {
            int dam = get_sn_damage(sn, ch->level, ch) * 0.25;
            if ( saves_spell(victim, ch, ch->level, DAM_HOLY) )
                dam /= 2;
            deal_damage(ch, victim, dam, gsn_mystic_infusion, DAM_HOLY, TRUE, TRUE);
        }
        else if ( target == TAR_CHAR_DEFENSIVE
                || target == TAR_OBJ_CHAR_DEF
                || target == TAR_CHAR_SELF )
        {
            int heal = get_sn_heal(sn, ch->level, ch, victim) * 0.25;
            if ( victim->hit < victim->max_hit )
            {
                if ( ch == victim )
                    ptc(ch, "Your mystic infusion restores some of your health.\n\r");
                else
                {
                    act("Your mystic infusion restores some of $N's health.", ch, NULL, victim, TO_CHAR);
                    act("$n's mystic infusion restores some of your health.", ch, NULL, victim, TO_VICT);
                }
                victim->hit += UMIN(victim->max_hit - victim->hit, heal);
            }
        }
    }
}

int mastery_adjust_cost( int cost, int mastery )
{
    if ( mastery > 0 )
        return cost - cost * (1 + mastery) / 10;
    return cost;
}

int mastery_adjust_level( int level, int mastery )
{
    if ( mastery > 0 )
        return level + (3 + mastery) * 2;
    return level;
}

void chain_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA *target, *next_target;
    bool offensive = is_offensive(sn);
    bool must_see = (skill_table[sn].target == TAR_VIS_CHAR_OFF);
    
    if ( !ch->in_room || victim->in_room != ch->in_room )
        return;
    
    for ( target = ch->in_room->people; target; target = next_target )
    {
        next_target = target->next_in_room;
        
        if ( target == victim
            || (offensive && is_safe_spell(ch, target, TRUE))
            || (must_see && !check_see(ch, target)) )
            continue;

        if ( !offensive && !is_same_group(victim, target) )
            continue;

        bool success = (*skill_table[sn].spell_fun) (sn, level, ch, (void*)target, TARGET_CHAR, FALSE);
        if ( !success )
            continue;
        
        post_spell_process(sn, level, ch, target);
    }
}

void reduce_mana( CHAR_DATA *ch, int amount )
{
    ch->mana -= amount;
#ifdef FSTAT 
    ch->mana_used += amount;
#endif
}

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
const char *target_name = NULL;
bool was_obj_cast = FALSE;
bool was_wish_cast = FALSE;

void cast_spell( CHAR_DATA *ch, int sn, int chance )
{
    CHAR_DATA *victim;
    void *vo;
    int mana;
    int level, wait;
    int target;
    bool concentrate = FALSE;
    bool overcharging = (IS_AFFECTED(ch, AFF_OVERCHARGE) && !ch->fighting);
    int orig_mana = ch->mana;
    int orig_wait = ch->wait;
    int orig_stop = ch->stop;

    /* check to see if spell is disabled */
    if (check_spell_disabled (&skill_table[sn]))
    {
        send_to_char ("This spell has been temporarily disabled.\n\r",ch);
        return;
    }

    /* check to see if spell can be cast in current position */
    if ( ch->position < skill_table[sn].minimum_position )
    {
        if ( ch->position < POS_FIGHTING || sn == gsn_overcharge )
        {
            send_to_char( "You can't concentrate enough.\n\r", ch );
            return;
        }
        else
            concentrate = TRUE;
    }
    
    /* check for explicit caster level cap */
    int level_cap = 200;
    if ( target_name[0] == '@' )
    {
        char cap_buf[MIL];
        target_name = one_argument(target_name + 1, cap_buf);
        if ( is_number(cap_buf) )
        {
            level_cap = atoi(cap_buf);
            level_cap = URANGE(1, level_cap, 200);
        }
        else
        {
            ptc(ch, "Caster level cap must be a number (found '%s').\n\r", cap_buf);
            return;
        }
    }

    /* Locate targets */
    if ( !get_spell_target( ch, target_name, sn, &target, &vo ) )
        return;
    
    /* wish casting restrictions */
    /*
    if ( was_wish_cast && target != TARGET_CHAR )
    {
        send_to_char ("You can only grant wishes to characters.\n\r", ch);
        return;
    }
    */
    
    // strip meta-magic options that are invalid for the spell & target
    meta_magic_strip(ch, sn, target, vo);

    // mana cost must be calculated after meta-magic effects have been worked out
    mana = mana_cost(ch, sn, chance);
    mana = meta_magic_adjust_cost(ch, mana, TRUE);
    if ( overcharging )
        mana *= 2;
    if ( was_wish_cast )
        mana = wish_cast_adjust_cost(ch, mana, sn, vo == ch);
    mana += meta_magic_perm_cost(ch, sn);

    if ( ch->mana < mana )
    {
        send_to_char( "You don't have enough mana.\n\r", ch );
        return;
    }

    // calculate level - needed prior to casting check
    level = ch->level;
    level = (100+chance)*level/200;
    // additive adjustments
    level += (20 + ch->level) * get_skill_overflow(ch, sn) / 1000;
    if ( is_malediction(sn) )
        level += (20 + ch->level) * get_skill(ch, gsn_arcane_defiling) / 500;
    // generic adjustments
    level = mastery_adjust_level(level, get_mastery(ch, sn));
    // multiplicative adjustments
    if ( IS_SET(meta_magic, META_MAGIC_EMPOWER) )
        level += UMAX(1, level/8);
    level = URANGE(1, level, level_cap);
    
    // check if spell could be cast successfully
    // that's done via a call to the spell function with check = TRUE
    // sending appropriate failure message is job of the spell function
    if ( !(*skill_table[sn].spell_fun)(sn, level, ch, vo, target, TRUE) )
        return;

    if ( str_cmp( skill_table[sn].name, "ventriloquate" ) )
    {
        say_spell( ch, sn );
    }

    wait = skill_table[sn].beats * (200-chance) / 100;
    wait = meta_magic_adjust_wait(ch, wait);
    // combat casting reduces all casting times
    if ( check_skill(ch, gsn_combat_casting) )
        wait = rand_div(wait * 4, 5);
    /* Check for overcharge (less lag) */
    if ( overcharging )
        wait /= 4;

    WAIT_STATE( ch, wait );

    /* no attacks while concentrating */
    if ( concentrate )
    {
        int rounds_missed = rand_div(wait+1, PULSE_VIOLENCE);
        ch->stop += rounds_missed;
    }

    if (is_affected(ch, gsn_choke_hold) && number_bits(3) == 0)
    {
        send_to_char( "You choke and your spell fumbles.\n\r", ch);
        reduce_mana(ch, mana/2);
        return;
    }
    else if (is_affected(ch, gsn_slash_throat) && number_bits(2) == 0)
    {
        send_to_char( "You can't speak and your spell fails.\n\r", ch);
        reduce_mana(ch, mana/2);
        return;
    }
    else if ( 2*number_percent() > (chance+100)
            || !meta_magic_concentration_check(ch)
            || (IS_AFFECTED(ch, AFF_INSANE) && IS_NPC(ch) && per_chance(25))
            || (IS_AFFECTED(ch, AFF_FEEBLEMIND) && per_chance(20))
            || (IS_AFFECTED(ch, AFF_CURSE) && per_chance(5))
            || (ch->fighting && per_chance(get_heavy_armor_penalty(ch)/2))
            || (concentrate && !check_concentration(ch)) )
    {
        ptc(ch, "You lost your concentration trying to cast %s.\n\r", skill_table[sn].name);
        check_improve(ch,sn,FALSE,3);
        reduce_mana(ch, mana/2);
        return;
    }

    else
    {
        reduce_mana(ch, mana);

        if ( target == TARGET_OBJ )
        {
            if (!op_act_trigger( (OBJ_DATA *) vo, ch, NULL, skill_table[sn].name, OTRIG_SPELL) ) 
                return;
        }

        vo = check_reflection( sn, level, ch, vo, target );

        victim = (CHAR_DATA*) vo;
        // remove invisibility etc.
        if ( is_offensive(sn) && target == TARGET_CHAR )
        {
            attack_affect_strip(ch, victim);
            if ( !ch->fighting && check_kill_trigger(ch, victim) )
                return;
            if ( !victim->fighting )
            {
                check_quick_draw(ch, victim);
                if ( IS_DEAD(ch) )
                    return;
            }
        }

        bool success = (*skill_table[sn].spell_fun) (sn, level, ch, vo, target, FALSE);
        if ( !success )
        {
            
            // refund resources used for syntax errors or other issues that prevent the spell from being cast
            // should not occur since we just already did a check for this earlier
            bugf("cast_spell: spell function for '%s' failed during actual casting", skill_table[sn].name);
            ch->mana = orig_mana;
            ch->wait = orig_wait;
            ch->stop = orig_stop;
            return;
        }
        check_improve(ch,sn,TRUE,3);
        
        apply_perm_cost(ch, sn);

        if ( target == TARGET_CHAR )
        {
            post_spell_process(sn, level, ch, (CHAR_DATA*)vo);
            if ( IS_SET(meta_magic, META_MAGIC_CHAIN) )
                chain_spell(sn, level*3/4, ch, (CHAR_DATA*)vo);
        }
    }
    
    /* mana burn */
    if ( IS_AFFECTED(ch, AFF_MANA_BURN) )
    {
        direct_damage( ch, ch, 2*mana, skill_lookup("mana burn") );
    }
    else if ( overcharging && number_bits(1) == 0 )
    {
        direct_damage( ch, ch, mana, skill_lookup("mana burn") );
    }
}

DEF_DO_FUN(do_cast)
{
    char arg1[MAX_INPUT_LENGTH];
    int sn, chance;

    target_name = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        send_to_char( "Cast which what where?\n\r", ch );
        return;
    }

    if ((sn = find_spell(ch, arg1, true)) < 1
            ||  (chance=get_skill(ch, sn))==0)
    {
        send_to_char( "You don't know any spells of that name.\n\r", ch );
        return;
    }
    
    cast_spell(ch, sn, chance);
}

bool can_wish_cast( int sn )
{
    if ( skill_table[sn].spell_fun == spell_null )
        return FALSE;

    // exceptions for specific spells
    if ( sn == gsn_mimic )
        return FALSE;
    
    switch ( skill_table[sn].target )
    {
        case TAR_CHAR_DEFENSIVE:
        case TAR_CHAR_NEUTRAL:
        case TAR_OBJ_CHAR_DEF:
        case TAR_OBJ_INV:
        case TAR_IGNORE_OBJ:
            return TRUE;
        default:
            return FALSE;
    }
}

int wish_level( int sn )
{
    // find minimum level required to cast
    int class, min_level = LEVEL_IMMORTAL;
    for ( class = 0; class < MAX_CLASS; class++ )
        min_level = UMIN(min_level, skill_table[sn].skill_level[class]);
    return min_level;
}

// skill for wish-casting sn
int wish_skill( CHAR_DATA *ch, int sn )
{
    int skill = get_skill(ch, gsn_wish);
    if ( skill > 0 )
        // bonus if spell is already known
        skill += (100 - skill) * get_skill(ch, sn) / 100;
    return skill;
}

// mana cost factor for wishcasting a spell
int wish_cast_adjust_cost( CHAR_DATA *ch, int mana, int sn, bool self )
{
    // extra cost when casting on self
    float factor = self ? 1.5 : 1.0;
    // mana cost reduction to cast spell already known, in percent
    float rebate = get_skill(ch, gsn_wish) * get_skill(ch, sn) / 30000.0;
    return UMAX(1, mana * factor * (1-rebate));
}

void show_wishes( CHAR_DATA *ch, bool all )
{
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char spell_columns[LEVEL_HERO + 1];
    int sn, level;

    /* initialize data */
    for ( level = 0; level <= LEVEL_HERO; level++ )
    {
        spell_columns[level] = 0;
        spell_list[level][0] = '\0';
    }
    
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            break;
        if ( !can_wish_cast(sn) )
            continue;
        if ( (level = wish_level(sn)) > LEVEL_HERO )
            continue;
        
        int skill = wish_skill(ch, sn);
        int mana = mana_cost(ch, sn, skill);
        mana = wish_cast_adjust_cost(ch, mana, sn, FALSE);
        sprintf(buf, "  %-20s %3dm %3d%%", skill_table[sn].name, mana, skill);

        if ( spell_list[level][0] == '\0' )
            sprintf(spell_list[level], "\n\rLevel %2d:%s", level, buf);
        else /* append */
        {
            if ( ++spell_columns[level] % 2 == 0 )
                strcat(spell_list[level], "\n\r         ");
            strcat(spell_list[level], buf);
        }
    }

    buffer = new_buf();
    int max_level = all ? LEVEL_HERO : ch->level;
    for ( level = 0; level <= max_level; level++ )
        if (spell_list[level][0] != '\0')
            add_buf(buffer, spell_list[level]);
    add_buf(buffer,"\n\r");
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
}

// Djinn wish casting
DEF_DO_FUN(do_wish)
{
    char arg1[MAX_INPUT_LENGTH];
    int sn, chance;

    target_name = one_argument( argument, arg1 );

    if ( (chance = get_skill(ch, gsn_wish)) == 0 )
    {
        send_to_char( "Yeah, you wish!\n\r", ch );
        return;
    }
    
    if ( arg1[0] == '\0' )
    {
        send_to_char( "What do you wish for? Type \t(wish list\t) to see all possible wishes.\n\r", ch );
        return;
    }
    
    if ( !strcmp(arg1, "list") )
    {
        show_wishes(ch, !strcmp(target_name, "all"));
        return;
    }

    if ( (sn = find_spell(ch, arg1, false)) < 1 )
    {
        send_to_char( "No spell of that name exists.\n\r", ch );
        return;
    }

    if ( !can_wish_cast(sn) )
    {
        send_to_char( "You cannot grant that wish.\n\r", ch );
        return;
    }
    
    if ( ch->level < wish_level(sn) )
    {
        send_to_char( "This wish is beyond your power.\n\r", ch );
        return;
    }
    
    // increase effective skill level if character posesses skill already
    chance = wish_skill(ch, sn);
    
    was_wish_cast = TRUE;
    cast_spell(ch, sn, chance);
    was_wish_cast = FALSE;
}

/*
 * Cast spells at targets using a magical object.
 */
bool obj_cast_spell( int sn, int level, CHAR_DATA *ch, OBJ_DATA *obj, const char *argument, bool check )
{
    char arg[MAX_INPUT_LENGTH];
    void *vo;
    int target;
    int levelmod;

    if ( sn <= 0 )
        return FALSE;

    if ( sn >= MAX_SKILL || skill_table[sn].spell_fun == 0 )
    {
        if (obj)
            bugf( "Obj_cast_spell: bad sn %d for object %d.", sn, obj->pIndexData->vnum );
        else
            bug( "Obj_cast_spell: bad sn %d.", sn );
        return FALSE;
    }

    /* check to see if spell is disabled */
    if (check_spell_disabled (&skill_table[sn]))
    {
        send_to_char ("This spell has been temporarily disabled.\n\r",ch);
        return FALSE;
    }

    if ( level > 1 )
    {
        if ( (levelmod = get_skill(ch, gsn_arcane_lore)) && !check )
            check_improve(ch, gsn_arcane_lore, TRUE, 3);
        level = level * (900 + levelmod) / 1000;
    }
    level += mastery_bonus(ch, gsn_arcane_lore, 3, 5);

    if ( IS_SET( ch->act, PLR_WAR ) )
    {
        if ( skill_table[sn].spell_fun == spell_heal 
                || skill_table[sn].spell_fun == spell_cure_critical
                || skill_table[sn].spell_fun == spell_cure_serious
                || skill_table[sn].spell_fun == spell_cure_light
                || skill_table[sn].spell_fun == spell_cure_mortal
                || skill_table[sn].spell_fun == spell_restoration
                || skill_table[sn].spell_fun == spell_mass_healing
                || skill_table[sn].spell_fun == spell_minor_group_heal
                || skill_table[sn].spell_fun == spell_group_heal
                || skill_table[sn].spell_fun == spell_major_group_heal
                || skill_table[sn].spell_fun == spell_heal_all
                || skill_table[sn].spell_fun == spell_mana_heal )
        {
            send_to_char( "Healing spell failed.  Damn.\n\r", ch );
            return FALSE;
        }
    }

    target_name = one_argument(argument, arg);
    
    /* get target */
    if ( !get_spell_target( ch, arg, sn, &target, &vo ) )
        return FALSE;

    // check if spell could be cast successfully
    // that's done via a call to the spell function with check = TRUE
    // sending appropriate failure message is job of the spell function
    if ( !(*skill_table[sn].spell_fun)(sn, level, ch, vo, target, TRUE) )
        return FALSE;
    
    if ( check )
        return TRUE;
    
    /* execute spell */
    vo = check_reflection( sn, level, ch, vo, target );

    // remove invisibility
    if ( is_offensive(sn) && target == TARGET_CHAR )
    {
        attack_affect_strip(ch, (CHAR_DATA*)vo);
        if ( !ch->fighting && check_kill_trigger(ch, (CHAR_DATA*)vo) )
            return FALSE;
    }        

    was_obj_cast = TRUE;
    bool success = (*skill_table[sn].spell_fun) ( sn, level, ch, vo, target, FALSE );
    was_obj_cast = FALSE;

    if ( success && target == TARGET_CHAR )
        post_spell_process(sn, level, ch, (CHAR_DATA*)vo);
    
    return success;
}


/*
 * Functions for balancing healing and damage spells
 */
int get_spell_damage( int mana, int lag, int level )
{
    int base_dam = (int)sqrt( 1000 * mana * (lag + 1) / (PULSE_VIOLENCE + 1) );
    base_dam = base_dam * (100 + 3 * level) / 100;
    return base_dam * 2/3;
}

int get_spell_heal( int mana, int lag, int level )
{
    int base_heal = (int)sqrt( 1000 * mana * (lag + 1) / (PULSE_VIOLENCE + 1) );
    return base_heal * (100 + 3 * level) / 400;
}

int get_obj_focus( CHAR_DATA *ch )
{
    OBJ_DATA *obj = get_eq_char(ch, WEAR_HOLD);
    bool has_shield = get_eq_char(ch, WEAR_SHIELD) != NULL;
    
    if ( !obj || obj->item_type == ITEM_ARROWS )
        return 0;
    
    // non-metal held object suffers a small penalty
    int base = IS_OBJ_STAT(obj, ITEM_NONMETAL) ? 99 : 100;
    
    if ( has_shield )
        return (base + get_skill(ch, gsn_wrist_shield)) / 3;
    else
        return base;
}

int get_dagger_focus( CHAR_DATA *ch )
{
    OBJ_DATA *obj = get_eq_char(ch, WEAR_SECONDARY);
    bool has_shield = get_eq_char(ch, WEAR_SHIELD) != NULL;
    int skill = get_skill(ch, gsn_dagger_focus) + mastery_bonus(ch, gsn_dagger_focus, 18, 30);
    
    if ( !obj || obj->item_type != ITEM_WEAPON )
        return 0;
    else if ( has_shield )
        return skill * (100 + get_skill(ch, gsn_wrist_shield)) / 300;
    else
        return skill;
}

int get_focus_bonus( CHAR_DATA *ch )
{
    int skill = get_skill(ch, gsn_focus) + mastery_bonus(ch, gsn_focus, 18, 30);
    int obj_focus = get_obj_focus(ch);
    int dagger_focus = get_dagger_focus(ch);
    // only one of obj_focus or dagger_focus can be positive, so sum = max
    int base = obj_focus + dagger_focus;
    int bonus = skill * (100 + obj_focus) / 100;
    return (base + bonus) / 5;
}

/* needes to be seperate for dracs */
int adjust_spell_damage( int dam, CHAR_DATA *ch )
{
    if ( was_obj_cast )
        return dam;

    dam += dam * get_focus_bonus(ch) / 100;
    check_improve(ch, gsn_focus, TRUE, 4);
    if ( get_dagger_focus(ch) )
        check_improve(ch, gsn_dagger_focus, TRUE, 4);

    if ( !IS_NPC(ch) && ch->level >= LEVEL_MIN_HERO )
    {
        dam += dam * (10 + ch->level - LEVEL_MIN_HERO) / 100;
    }
    
    if ( IS_SET(meta_magic, META_MAGIC_EMPOWER) )
        dam += dam / 4;

    return dam * number_range(90, 110) / 100;
}

int get_spell_bonus_damage( CHAR_DATA *ch, int cast_time, bool avg )
{
    int edge = get_skill(ch, gsn_warmage_edge);
    if ( ch->stance == STANCE_ARCANA )
        edge += 100;
    int bonus = ch->level * edge / 150;
    // damroll from affects applies here as well
    if ( avg )
        bonus += (ch->damroll / 4) * 2.5;
    else
        bonus += dice(ch->damroll / 4, 4);

    // adjust for casting time
    bonus = bonus * (cast_time + 1) / (PULSE_VIOLENCE + 1);

    return bonus * (100 + get_focus_bonus(ch)) / 100;
}

int get_spell_bonus_damage_sn( CHAR_DATA *ch, int sn )
{
    int cast_time = skill_table[sn].beats;
    if ( IS_SET(meta_magic, META_MAGIC_QUICKEN) )
        cast_time /= 2;
    return get_spell_bonus_damage(ch, cast_time, FALSE);
}

int get_sn_damage( int sn, int level, CHAR_DATA *ch )
{
    int dam, bonus;

    if ( sn < 1 || sn >= MAX_SKILL )
        return 0;

    dam = get_spell_damage( skill_table[sn].min_mana, skill_table[sn].beats, level );
    dam = adjust_spell_damage(dam, ch);
    bonus = get_spell_bonus_damage_sn(ch, sn);
    // bonus can at most double the spell damage
    dam += UMIN(bonus, dam);

    return dam;
}

int get_sn_heal( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim )
{
    int heal;

    if ( sn < 1 || sn >= MAX_SKILL )
        return 0;

    heal = get_spell_heal( skill_table[sn].min_mana, skill_table[sn].beats, level );

    if ( !was_obj_cast )
    {
        int skill = get_skill(ch, gsn_anatomy) + mastery_bonus(ch, gsn_anatomy, 15, 25);
        heal += heal * skill / 200;
        check_improve(ch, gsn_anatomy, TRUE, 4);

        if ( !IS_NPC(ch) && ch->level >= LEVEL_MIN_HERO )
        {
            heal += heal * (10 + ch->level - LEVEL_MIN_HERO) / 100;
        }
    }

    /* bonus for healing others */
    if ( ch != victim )
    {
        heal += heal / 3;
    }

    /* bonus/penalty for target's vitality */
    heal = heal * (200 + get_curr_stat(victim, STAT_VIT)) / 300;

    if ( IS_SET(meta_magic, META_MAGIC_EMPOWER) )
        heal += heal / 4;

    return heal;
}


/*
 * Spell functions.
 */
DEF_SPELL_FUN(spell_acid_blast)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    SPELL_CHECK_RETURN
    
    dam = get_sn_damage( sn, level, ch );
    if ( saves_spell(victim, ch, level, DAM_ACID ) )
        dam /= 2;

    full_dam( ch, victim, dam, sn,DAM_ACID,TRUE);
    return TRUE;
}

DEF_SPELL_FUN(spell_armor)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    SPELL_CHECK_RETURN
    
    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
            send_to_char("You are already armored.\n\r",ch);
        else
            act("$N is already armored.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.modifier  = -(20 + level);
    af.location  = APPLY_AC;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel someone protecting you.\n\r", victim );
    if ( ch != victim )
        act("$N is protected by your magic.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}




DEF_SPELL_FUN(spell_bless)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    SPELL_CHECK_RETURN
    
    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;
        if (IS_OBJ_STAT(obj,ITEM_BLESS))
        {
            act("$p is already blessed.",ch,obj,NULL,TO_CHAR);
            return SR_AFFECTED;
        }

        if (IS_OBJ_STAT(obj,ITEM_EVIL))
        {
            AFFECT_DATA *paf;

            paf = affect_find(obj->affected,gsn_curse);
            if (!saves_dispel(level,paf != NULL ? paf->level : obj->level,0))
            {
                if (paf != NULL)
                    affect_remove_obj(obj,paf);
                act("$p glows a pale blue.",ch,obj,NULL,TO_ALL);
                REMOVE_BIT(obj->extra_flags,ITEM_EVIL);
                return TRUE;
            }
            else
            {
                act("The evil of $p is too powerful for you to overcome.",
                        ch,obj,NULL,TO_CHAR);
                return TRUE;
            }
        }

        af.where    = TO_OBJECT;
        af.type     = sn;
        af.level    = level;
        af.duration = get_duration_by_type(DUR_EXTREME, level);
        af.location = APPLY_SAVES;
        af.modifier = -1;
        af.bitvector    = ITEM_BLESS;
        affect_to_obj(obj,&af);

        act("$p glows with a holy aura.",ch,obj,NULL,TO_ALL);

        if (obj->wear_loc != WEAR_NONE)
            ch->saving_throw -= 1;
        return TRUE;
    }

    /* character target */
    victim = (CHAR_DATA *) vo;


    if ( is_affected( victim, sn ) || is_affected(victim, gsn_prayer) )
    {
        if (victim == ch)
            send_to_char("You are already blessed.\n\r",ch);
        else
            act("$N is already blessed.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_HITROLL;
    af.modifier  = (level + 20) / 8;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVES;
    af.modifier  = -(level + 20) / 8;
    affect_to_char( victim, &af );
    send_to_char( "You feel righteous.\n\r", victim );
    if ( ch != victim )
        act("You grant $N the favor of your god.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}



DEF_SPELL_FUN(spell_blindness)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    SPELL_CHECK_RETURN
    
    if ( IS_AFFECTED(victim, AFF_BLIND) )
    {
        send_to_char( "Your target is already blind!\n\r", ch );
        return SR_AFFECTED;
    }

    if ( saves_spell(victim, ch, level * 2/3, DAM_OTHER) )
    {
        if ( victim != ch )
            act( "$N blinks $S eyes, and the spell has no effect.", ch, NULL, victim, TO_CHAR );
        send_to_char( "Your eyes begin to water, causing you to blink several times.\n\r", victim );
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_HITROLL;
    af.modifier  = -4 - number_range(0,6);
    af.duration  = get_duration(sn, level);
    af.bitvector = AFF_BLIND;
    affect_to_char( victim, &af );
    send_to_char( "You are blinded!\n\r", victim );
    act("$n appears to be blinded.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}



DEF_SPELL_FUN(spell_burning_hands)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    SPELL_CHECK_RETURN
    
    if ( check_hit( ch, victim, sn, DAM_FIRE, 100 ) )
    {
        dam = get_sn_damage( sn, level, ch ) * 14/10;
        if ( saves_spell(victim, ch, level, DAM_FIRE) )
            dam /= 2;
        fire_effect( victim, level, dam, TARGET_CHAR );
    }
    else
        dam = 0;

    full_dam( ch, victim, dam, sn, DAM_FIRE,TRUE);
    return TRUE;
}

DEF_SPELL_FUN(spell_call_lightning)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    if ( !IS_OUTSIDE(ch) )
    {
        send_to_char( "You must be out of doors.\n\r", ch );
        return SR_UNABLE;
    }

    if ( weather_info.sky < SKY_RAINING )
    {
        send_to_char( "The weather is MUCH too nice for that!\n\r", ch );
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    dam = get_sn_damage( sn, level, ch ) * AREA_SPELL_FACTOR * 1.5;

    send_to_char( "Lightning leaps out of the sky to strike your foes!\n\r", ch );
    act( "$n calls lightning from the sky to strike $s foes!", ch, NULL, NULL, TO_ROOM );

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;
        if ( is_safe_spell(ch, vch, TRUE) )
            continue;

        if ( saves_spell(vch, ch, level, DAM_LIGHTNING) )
            full_dam( ch, vch, dam/2, sn,DAM_LIGHTNING,TRUE);
        else
            full_dam( ch, vch, dam, sn,DAM_LIGHTNING,TRUE);
    }

    return TRUE;
}

/**
 * Calm spell works like this: 
 * 1) all violent characters need to save or be calmed and stop fighting
 * 2) non-violent characters attacking non-violent characters stop fighting
 */
bool is_violent( CHAR_DATA *vch, CHAR_DATA *ch )
{
    return !IS_AFFECTED(vch, AFF_CALM) && !is_same_group(vch, ch);
}

DEF_SPELL_FUN(spell_calm)
{
    CHAR_DATA *vch;
    AFFECT_DATA af;
    bool conflict = FALSE;

    if( IS_SET(ch->in_room->room_flags, ROOM_SAFE) )
    {
        send_to_char( "All is already calm in a safe room.\n\r", ch );
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    /* try to calm all characters that need it */
    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        // no need
        if ( vch->fighting == NULL )
            continue;

        conflict = TRUE;
        
        if ( !is_violent(vch, ch) )
            continue;
        
        // failure
        if ( is_safe(ch, vch) || saves_spell(vch, ch, level, DAM_MENTAL) )
            continue;
        
        if ( IS_AFFECTED(vch, AFF_BERSERK) )
        {
            // remove berserk, but not calm yet - halfway there
            affect_strip_flag(vch, AFF_BERSERK);
            send_to_char("You feel less angry.\n\r", vch);
            act("$n seems a little calmer.", vch, NULL, NULL, TO_ROOM);
            continue;
        }
        
        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = level;
        af.duration = get_duration(sn, level);
        af.location = APPLY_HITROLL;
        af.modifier = -5;
        af.bitvector = AFF_CALM;
        affect_to_char(vch, &af);
        af.location = APPLY_DAMROLL;
        affect_to_char(vch, &af);
        
        stop_fighting(vch, FALSE);
        
        send_to_char("A wave of calm passes over you.\n\r", vch);
        act("A wave of calm passes over $n.", vch, NULL, NULL, TO_ROOM);
    }

    if ( !conflict )
    {
        send_to_char("Things seem pretty calm already.\n\r", ch);
        return TRUE;
    }
    
    conflict = FALSE;
    /* stop the fighting */
    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( vch->fighting == NULL )
            continue;

        if ( is_violent(vch, ch) || is_violent(vch->fighting, ch) )
        {
            conflict = TRUE;
            continue;
        }
        
        stop_fighting(vch, FALSE);
    }
    
    if ( conflict )
        send_to_char("Your environment continues to be hostile.\n\r", ch);
    else
        send_to_char("... and world peace.\n\r", ch);
    
    return TRUE;
}

DEF_SPELL_FUN(spell_cancellation)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;

    level += 2;
    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_IS_HEALER) )
        level+=18;
    else
    { 
        if ((!IS_NPC(ch) && IS_NPC(victim) && /* PC casting on NPC, unless NPC is charmed by the PC */
                    !(IS_AFFECTED(victim, AFF_CHARM) && victim->master == ch) ) ||
                (IS_NPC(ch) && !IS_NPC(victim)) ) /* NPC casting on PC */
        {
            send_to_char("You failed, try dispel magic.\n\r",ch);
            return SR_TARGET;
        }

        if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOCANCEL) && ch != victim)
        {
            send_to_char("That player does not wish to be cancelled.\n\r",ch);
            return SR_TARGET;
        }
    }

    /* unlike dispel magic, the victim gets NO save */

    /* we kill first arg (target), if there's more args then
       they're trying to cancel a certain spell*/
    const char *arg = one_argument( target_name, NULL);
    if ( arg[0] != '\0' )
    {
        int sn = affect_list_lookup(victim->affected, arg);

        if ( sn == -1 )
        {
            send_to_char("Cancel which spell?\n\r",ch);
            return SR_SYNTAX;
        }

        SPELL_CHECK_RETURN
        
        if ( can_dispel(sn) && check_cancel(level, victim, sn) )
            send_to_char( "Ok.\n\r", ch);
        else
            send_to_char( "Spell failed.\n\r", ch);
    } 
    else
    {
        SPELL_CHECK_RETURN
        
        /* begin running through the spells */
        for (sn = 1; skill_table[sn].name != NULL; sn++)
            if ( can_dispel(sn) && check_cancel(level, victim, sn) )
                found = TRUE;

        if (found)
            send_to_char("Ok.\n\r",ch);
        else
            send_to_char("Spell failed.\n\r",ch);
    }
    return TRUE;
}

CHAR_DATA* get_next_victim( CHAR_DATA *ch, CHAR_DATA *start_victim )
{
    CHAR_DATA *victim;

    for ( victim = start_victim; victim != NULL; victim = victim->next_in_room )
        if ( !is_safe_spell(ch, victim, TRUE) )
            return victim;
    return NULL;
}

void deal_chain_damage( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, int dam_type )
{
    CHAR_DATA *next_vict;
    int curr_dam, dam;
    int per = 100;

    dam = get_sn_damage( sn, level, ch ) * AREA_SPELL_FACTOR;
    while ( per > 0 )
    {
        int count = 0;
        CHAR_DATA *vch;

        curr_dam = dam * per/100;

        /* count the targets.. having only a few targets makes it difficult to chain */
        for( vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room )
            if( !is_safe_spell(ch,vch,TRUE) && vch != ch )
                count++;

        // -15% each arc means 385% damage total => good for 2 or 3 targets
        per -= 15;

        if ( saves_spell(victim, ch, level, dam_type) )
            curr_dam /= 2;

        next_vict = victim->next_in_room;
        full_dam(ch,victim,curr_dam,sn,dam_type,TRUE);
        if ( IS_DEAD(ch) )
            return;

        /* If the player is casting this spell versus only one target.. */
        if( count < 2 && per > 0)
        {
            send_to_char("The chain of magical energy arcs back to you!\n\r",ch);
            act("$n's spell arcs back to $mself!",ch,NULL,NULL,TO_ROOM);
            curr_dam = dam * per/100;
            per -= 15;
            if ( saves_spell(ch, ch, level, dam_type) )
                curr_dam /= 2;
            full_dam(ch,ch,curr_dam,sn,dam_type,TRUE);
        }

        /* find new victim */
        next_vict = get_next_victim( ch, next_vict );
        if ( next_vict == NULL )
            next_vict = get_next_victim( ch, ch->in_room->people );
        if ( next_vict == NULL ) // removed this to let the chain continue viciously!  || next_vict == victim )
            return;
        else
            victim = next_vict;
    }
}

DEF_SPELL_FUN(spell_chain_lightning)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    SPELL_CHECK_RETURN
        
    act("A lightning bolt leaps from $n's hand and arcs to $N.",
            ch,NULL,victim,TO_ROOM);
    act("A lightning bolt leaps from your hand and arcs to $N.",
            ch,NULL,victim,TO_CHAR);
    act("A lightning bolt leaps from $n's hand and hits you!",
            ch,NULL,victim,TO_VICT);  

    deal_chain_damage( sn, level, ch, victim, DAM_LIGHTNING );
    return TRUE;
}


DEF_SPELL_FUN(spell_change_sex)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_SET( ch->act, PLR_WAR ) && war.type == GENDER_WAR )
    {
        send_to_char( "It won't work... sides in the war are decided by a person's real sex.\n\r", ch );
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN

    if ( is_affected( victim, sn ))
    {
        if (victim == ch)
            send_to_char("You've already been changed.\n\r",ch);
        else
            act("$N has already had $s(?) sex changed.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    if ( ch != victim && saves_spell(victim, ch, level, DAM_OTHER) )
    {
        act("Hmmm... nope, $E's still a $E.",ch,NULL,victim,TO_CHAR );
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_SEX;
    do
    {
        af.modifier  = number_range( 0, 2 ) - victim->sex;
    }
    while ( af.modifier == 0 );
    af.bitvector = 0;
    /* affect_to_char( victim, &af ); moved */
    send_to_char( "You feel different.\n\r", victim );
    act("$n doesn't look like $mself anymore...",victim,NULL,NULL,TO_ROOM);
    affect_to_char( victim, &af ); /* moved */
    return TRUE;
}



DEF_SPELL_FUN(spell_charm_person)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int mlevel;
    bool sex_bonus;

    if ( is_safe(ch,victim) )
        return SR_UNABLE;

    if ( victim == ch )
    {
        send_to_char( "You like yourself even better!\n\r", ch );
        return SR_TARGET;
    }

    if (IS_SET(victim->in_room->room_flags,ROOM_LAW))
    {
        send_to_char("Charming is not allowed here.\n\r",ch);
        return SR_UNABLE;
    }

    if ( !IS_NPC(ch) && IS_SET( ch->act, PLR_WAR ) )
    {
        act( "Don't charm $M, KILL $M!", ch, NULL, victim, TO_CHAR );
        return SR_UNABLE;
    }

    mlevel = victim->level;
    if ( check_cha_follow(ch, mlevel) < mlevel )
        return SR_UNABLE;

    SPELL_CHECK_RETURN
    
    if ( IS_AFFECTED(victim, AFF_CHARM)
            || IS_AFFECTED(ch, AFF_CHARM)
            || IS_SET(victim->imm_flags, IMM_CHARM)
            || IS_SET(victim->imm_flags, IMM_CHARMPERSON)
            || IS_IMMORTAL(victim) )
    {
        act( "You can't charm $N.", ch, NULL, victim, TO_CHAR );
        return SR_IMMUNE;
    }
    
    sex_bonus =  
        (ch->sex == SEX_FEMALE && victim->sex == SEX_MALE)
        || (ch->sex == SEX_MALE && victim->sex == SEX_FEMALE);

    /* PCs are harder to charm */
    if ( saves_spell(victim, ch, level, DAM_CHARM)
            || number_range(1, 200) > get_curr_stat(ch, STAT_CHA)
            || (!sex_bonus && number_bits(1) == 0)
            || (!IS_NPC(victim) && number_bits(2)) )
    {
        send_to_char("The spell has failed to have an effect.\n\r", ch );
        return TRUE;
    }

    if ( is_same_group(victim->fighting, ch) )
        stop_fighting(victim, TRUE);
    if ( victim->master )
        stop_follower( victim );
    add_follower( victim, ch );
    victim->leader = ch;
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    if ( !IS_NPC(victim) )
        af.duration /= 2;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( victim, &af );
    act( "Isn't $n just so nice?", ch, NULL, victim, TO_VICT );
    if ( ch != victim )
        act("$N looks at you with adoring eyes.",ch,NULL,victim,TO_CHAR);

    return TRUE;
}



DEF_SPELL_FUN(spell_chill_touch)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    SPELL_CHECK_RETURN
    
    if ( check_hit(ch, victim, sn, DAM_COLD, 100) )
    {
        dam = get_sn_damage(sn, level, ch) * 14/10;
        if ( saves_spell(victim, ch, level, DAM_COLD) )
            dam /= 2;
        cold_effect( victim, level, dam, TARGET_CHAR );
    }
    else
        dam = 0;

    full_dam( ch, victim, dam, sn, DAM_COLD,TRUE );
    return TRUE;
}



DEF_SPELL_FUN(spell_colour_spray)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    SPELL_CHECK_RETURN

    dam = get_sn_damage(sn, level, ch) * 3/4;
    if ( saves_spell(victim, ch, level, DAM_LIGHT) )
        dam /= 2;
    else 
        spell_blindness(gsn_blindness, level/2, ch, (void*)victim, TARGET_CHAR, FALSE);

    full_dam( ch, victim, dam, sn, DAM_LIGHT,TRUE );
    return TRUE;
}



DEF_SPELL_FUN(spell_continual_light)
{
    OBJ_DATA *light;

    if (target_name[0] != '\0')  /* do a glow on some object */
    {
        light = get_obj_carry(ch,target_name,ch);

        if (light == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            return SR_TARGET;
        }
        
        SPELL_CHECK_RETURN

        if (IS_OBJ_STAT(light, ITEM_DARK))
        {
            REMOVE_BIT(light->extra_flags, ITEM_DARK);
            act("$p loses its dark aura.",ch,light,NULL,TO_ALL);
            return TRUE;
        }

        if (IS_OBJ_STAT(light,ITEM_GLOW))
        {
            act("$p is already glowing.",ch,light,NULL,TO_CHAR);
            return SR_AFFECTED;
        }

        SET_BIT(light->extra_flags,ITEM_GLOW);
        act("$p glows with a white light.",ch,light,NULL,TO_ALL);
        return TRUE;
    }

    SPELL_CHECK_RETURN
    
    light = create_object_vnum(OBJ_VNUM_LIGHT_BALL);
    obj_to_room( light, ch->in_room );
    act( "$n twiddles $s thumbs and $p appears.",   ch, light, NULL, TO_ROOM );
    act( "You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR );
    return TRUE;
}



DEF_SPELL_FUN(spell_control_weather)
{
    SPELL_CHECK_RETURN
    
    if ( !str_cmp( target_name, "better" ) )
    {
        weather_info.change += dice( level/3 , 4 );
        send_to_char( "The winds shift, and the weather seems slightly nicer.\n\r", ch );

        /* Control weather actually checks for weather updates making it useful - Astark Nov 2012 */

        if (!number_bits(2))
            weather_update ( );
    }
    else if ( !str_cmp( target_name, "worse" ) )
    {
        weather_info.change -= dice( level/3 , 4 );
        send_to_char( "The winds shift, and the weather seems slightly worse.\n\r", ch );
        if (!number_bits(2))
            weather_update ( );
    }
    else
        send_to_char ("Do you want it to get better or worse?\n\r", ch );

    send_to_char( "Ok.\n\r", ch );
    return TRUE;
}

/* Explosives ala Rimbol.  Original idea from Wurm. */
/* Added if check to make sure a player had room in there inventory for the item
 *  * before casting it. Assholes were using this to core the mud 
 *   */
DEF_SPELL_FUN(spell_create_bomb)
{
    OBJ_DATA *bomb;

    if ( target_name[0] != '\0' )
    {
        if ( !is_number(target_name) )
        {
            send_to_char("Syntax: c 'create bomb' [level]\n\r", ch);
            return SR_UNABLE;
        }
        int bomb_level = atoi(target_name);
        level = URANGE(1, bomb_level, level);
    }
    
    if ( ch->carry_number >= can_carry_n(ch) )
    {
        send_to_char("You have no room in your inventory for that!\r\n", ch);
        return SR_UNABLE;
    }
        
    SPELL_CHECK_RETURN
    
    bomb = create_object_vnum(OBJ_VNUM_BOMB);
    act( "$n has created $p.", ch, bomb, NULL, TO_ROOM );
    act( "You create $p.", ch, bomb, NULL, TO_CHAR );
    bomb->timer = -1;
    // #dice
    bomb->value[0] = 2 * (25 + level * 3/4);
    // type of dice
    bomb->value[1] = 20;
    bomb->level = level;
    obj_to_char(bomb,ch);
    
    return TRUE;
}


DEF_SPELL_FUN(spell_create_food)
{
    SPELL_CHECK_RETURN
    
    OBJ_DATA *mushroom = create_object_vnum(OBJ_VNUM_MUSHROOM);
    mushroom->value[0] = level / 2;
    mushroom->value[1] = level;
    obj_to_room( mushroom, ch->in_room );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_ROOM );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_CHAR );
    return TRUE;
}

DEF_SPELL_FUN(spell_create_rose)
{
    if ( ch->carry_number >= can_carry_n(ch) )
    {
        send_to_char("You have no room in your inventory for that!\n\r",ch);
        return SR_UNABLE;
    }
    
    SPELL_CHECK_RETURN
    
    OBJ_DATA *rose = create_object_vnum(OBJ_VNUM_ROSE);
    act("$n has created a beautiful red rose.",ch,rose,NULL,TO_ROOM);
    send_to_char("You create a beautiful red rose.\n\r",ch);
    obj_to_char(rose,ch);
        
    return TRUE;
}

DEF_SPELL_FUN(spell_create_spring)
{
    SPELL_CHECK_RETURN
    
    OBJ_DATA *spring = create_object_vnum(OBJ_VNUM_SPRING);
    spring->timer = get_duration(sn, level);
    obj_to_room( spring, ch->in_room );
    act( "$p flows from the ground.", ch, spring, NULL, TO_ROOM );
    act( "$p flows from the ground.", ch, spring, NULL, TO_CHAR );
    return TRUE;
}

DEF_SPELL_FUN(spell_create_water)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int water;

    if ( obj->item_type != ITEM_DRINK_CON )
    {
        send_to_char( "It is unable to hold water.\n\r", ch );
        return SR_TARGET;
    }

    if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 )
    {
        send_to_char( "It contains some other liquid.\n\r", ch );
        return SR_UNABLE;
    }

    if ( obj->value[0] <= obj->value[1] )
    {
        send_to_char( "It is already full.\n\r", ch );
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    water = UMIN(
            level * (weather_info.sky >= SKY_RAINING ? 4 : 2),
            obj->value[0] - obj->value[1]
            );

    if ( water > 0 )
    {
        obj->value[2] = LIQ_WATER;
        obj->value[1] += water;
        if ( !is_name( "water", obj->name ) )
        {
            char buf[MAX_STRING_LENGTH];

            sprintf( buf, "%s water", obj->name );
            free_string( obj->name );
            obj->name = str_dup( buf );
        }
        act( "$p is filled.", ch, obj, NULL, TO_CHAR );
    }

    return TRUE;
}

DEF_SPELL_FUN(spell_cure_blindness)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA *aff;

    if ( !(IS_AFFECTED(victim, AFF_BLIND) ))
    {
        if (victim == ch)
            send_to_char("You aren't blind.\n\r",ch);
        else
            act("$N doesn't appear to be blinded.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    for ( aff = victim->affected; aff != NULL; aff = aff->next )
    {
        if ( aff->where == TO_AFFECTS && aff->bitvector == AFF_BLIND )
        {
            if ( !check_dispel(level, victim, aff->type) )
                send_to_char( "Spell failed.\n\r", ch );
            else
                if ( ch != victim )
                    act( "$N is no longer blinded.", ch, NULL, victim, TO_CHAR );
            return TRUE;
        }
    }

    /* blindness not added by an affect... */
    if ( ch == victim )
        send_to_char( "Your blindness cannot be cured.\n\r", ch );
    else
        act( "$N's blindness cannot be cured.", ch, NULL, victim, TO_CHAR );

    return SR_IMMUNE;
}

bool is_mental( int sn )
{
    return sn == gsn_confusion
        || sn == gsn_mass_confusion
        || sn == gsn_laughing_fit
        || sn == gsn_charm_person
        || sn == gsn_sleep
        || sn == gsn_fear
        || sn == gsn_mindflay
        || sn == gsn_feeblemind;
}

/* Added for use in show_affects -Astark */
bool is_blindness( int sn )
{
    return sn == gsn_blindness
        || sn == gsn_fire_breath
        || sn == gsn_dirt
        || sn == gsn_spit
        || sn == gsn_gouge;
}

bool is_curse( int sn )
{
    return sn == gsn_curse
        || sn == gsn_tomb_rot
        || sn == gsn_cursed_wound
        || sn == gsn_eldritch_curse;
}

bool is_disease( int sn )
{
    return sn == gsn_plague
        || sn == gsn_necrosis
        || sn == gsn_decompose;
}

DEF_SPELL_FUN(spell_cure_mental)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
    int i;
    for (i = 1; skill_table[i].name != NULL; i++)
        if ( is_mental(i) && check_dispel(level, victim, i) )
            found = TRUE;

    if (found)
        send_to_char("Ok.\n\r",ch);
    else
        send_to_char("Spell failed.\n\r",ch);
    
    return TRUE;
}

/* RT added to cure plague */
DEF_SPELL_FUN(spell_cure_disease)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE, success = FALSE;
    int asn; // affect skill number
    
    for ( asn = 1; skill_table[asn].name != NULL; asn++ )
        if ( is_disease(asn) && is_affected(victim, asn) )
        {
            found = TRUE;
            if ( check_dispel(level, victim, asn) )
                success = TRUE;
        }
    
    if ( !found )
    {
        if ( victim == ch )
            send_to_char("You aren't ill.\n\r", ch);
        else
            act("$N doesn't appear to be diseased.", ch, NULL, victim, TO_CHAR);
        return SR_AFFECTED;
    }

    if ( success )
        act("$n looks relieved as $s sores vanish.", victim, NULL, NULL, TO_ROOM);
    else
        send_to_char("Spell failed.\n\r",ch);
    
    return TRUE;
}


DEF_SPELL_FUN(spell_cure_poison)
{
    SPELL_CHECK_RETURN

    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( !is_affected( victim, gsn_poison ) )
    {
        if (victim == ch)
            send_to_char("You aren't poisoned.\n\r",ch);
        else
            act("$N doesn't appear to be poisoned.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    if (check_dispel(level,victim,gsn_poison))
    {
        send_to_char("A warm feeling runs through your body.\n\r",victim);
        act("$n looks much better.",victim,NULL,NULL,TO_ROOM);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
    
    return TRUE;
}


DEF_SPELL_FUN(spell_curse)
{
    SPELL_CHECK_RETURN
        
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;
        if (IS_OBJ_STAT(obj,ITEM_EVIL))
        {
            act("$p is already filled with evil.",ch,obj,NULL,TO_CHAR);
            return SR_AFFECTED;
        }

        if (IS_OBJ_STAT(obj,ITEM_BLESS))
        {
            AFFECT_DATA *paf;

            paf = affect_find(obj->affected,skill_lookup("bless"));
            if (!saves_dispel(level,paf != NULL ? paf->level : obj->level,0))
            {
                if (paf != NULL)
                    affect_remove_obj(obj,paf);
                act("$p glows with a red aura.",ch,obj,NULL,TO_ALL);
                REMOVE_BIT(obj->extra_flags,ITEM_BLESS);
                return TRUE;
            }
            else
            {
                act("The holy aura of $p is too powerful for you to overcome.",
                        ch,obj,NULL,TO_CHAR);
                return TRUE;
            }
        }

        af.where        = TO_OBJECT;
        af.type         = sn;
        af.level        = level;
        af.duration     = get_duration_by_type(DUR_EXTREME, level);
        af.location     = APPLY_SAVES;
        af.modifier     = +1;
        af.bitvector    = ITEM_EVIL;
        affect_to_obj(obj,&af);

        act("$p glows with a malevolent aura.",ch,obj,NULL,TO_ALL);

        if (obj->wear_loc != WEAR_NONE)
            ch->saving_throw += 1;
        return TRUE;
    }

    /* character curses */
    victim = (CHAR_DATA *) vo;

    if (IS_AFFECTED(victim,AFF_CURSE))
    {
        send_to_char("They are already cursed.\n\r",ch);
        return SR_AFFECTED;
    }

    if ( saves_spell(victim, ch, level, DAM_NEGATIVE) )
    {
        act("$N feels a shiver, but resists your curse.",ch,NULL,victim,TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_HITROLL;
    af.modifier  = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVES;
    af.modifier  = level / 8;
    affect_to_char( victim, &af );

    send_to_char( "You feel unclean.\n\r", victim );
    if ( ch != victim )
        act("$N looks very uncomfortable.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}

/* RT replacement demonfire spell */
DEF_SPELL_FUN(spell_demonfire)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    /*drop_align( ch );*/

    if ( !IS_NPC(ch) && !IS_EVIL(ch) )
    {
        victim = ch;
        send_to_char("The demons turn upon you!\n\r",ch);
    }

    if (victim != ch)
    {
        act("$n calls forth the demons of Hell upon $N!",
                ch,NULL,victim,TO_ROOM);
        act("$n has assailed you with the demons of Hell!",
                ch,NULL,victim,TO_VICT);
        send_to_char("You conjure forth the demons of hell!\n\r",ch);
    }

    dam = get_sn_damage( sn, level, ch );

    if ( saves_spell(victim, ch, level, DAM_NEGATIVE) )
        dam /= 2;

    if ( IS_GOOD(victim) && !IS_AFFECTED(victim, AFF_CURSE) )
        spell_curse(gsn_curse, level/2, ch, (void*)victim, TARGET_CHAR, FALSE);
    full_dam( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
    return TRUE;
}

DEF_SPELL_FUN(spell_angel_smite)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( !IS_NPC(ch) && !IS_GOOD(ch) )
    {
        victim = ch;
        send_to_char("The angels punish you!\n\r",ch);
    }

    if (victim != ch)
    {
        act("$n calls forth angels to punish $N!",
                ch,NULL,victim,TO_ROOM);
        act("$n has called forth angels to punish you!",
                ch,NULL,victim,TO_VICT);
        send_to_char("You call the angels of Heaven!\n\r",ch);
    }

    dam = get_sn_damage( sn, level, ch );
    if ( saves_spell(victim, ch, level, DAM_HOLY) )
        dam /= 2;

    if ( IS_EVIL(victim) && !IS_AFFECTED(victim, AFF_CURSE) )
        spell_curse(gsn_curse, level/2, ch, (void*)victim, TARGET_CHAR, FALSE);
    full_dam( ch, victim, dam, sn, DAM_HOLY ,TRUE);
    return TRUE;
}


DEF_SPELL_FUN(spell_detect_evil)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_EVIL) )
    {
        if (victim == ch)
            send_to_char("You can already sense evil.\n\r",ch);
        else
            act("$N can already detect evil.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}


DEF_SPELL_FUN(spell_detect_good)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_GOOD) )
    {
        if (victim == ch)
            send_to_char("You can already sense good.\n\r",ch);
        else
            act("$N can already detect good.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_GOOD;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}



DEF_SPELL_FUN(spell_detect_hidden)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_HIDDEN) )
    {
        if (victim == ch)
            send_to_char("You are already as alert as you can be. \n\r",ch);
        else
            act("$N can already sense hidden lifeforms.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char( victim, &af );
    send_to_char( "Your awareness improves.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}



DEF_SPELL_FUN(spell_detect_invis)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_INVIS) )
    {
        if (victim == ch)
            send_to_char("You can already see invisible.\n\r",ch);
        else
            act("$N can already see invisible things.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}



DEF_SPELL_FUN(spell_detect_magic)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_MAGIC) )
    {
        if (victim == ch)
            send_to_char("You can already sense magical auras.\n\r",ch);
        else
            act("$N can already detect magic.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}



DEF_SPELL_FUN(spell_detect_poison)
{
    SPELL_CHECK_RETURN
    
    OBJ_DATA *obj = (OBJ_DATA *) vo;

    if ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD )
    {
        if ( obj->value[3] != 0 )
            send_to_char( "You smell poisonous fumes.\n\r", ch );
        else
            send_to_char( "It looks delicious.\n\r", ch );
    }
    else
    {
        send_to_char( "It doesn't look poisoned.\n\r", ch );
    }

    return TRUE;
}

DEF_SPELL_FUN(spell_dispel_evil)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    /* Neutral victims are not affected. */
    if ( IS_NEUTRAL(victim) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return SR_IMMUNE;
    }

    /* Good victims are not affected either, and are protected by their god if they have one. */
    if ( IS_GOOD(victim) )
    {
        sprintf(log_buf, "%s protects $N.", get_god_name(victim));
        act( log_buf, ch, NULL, victim, TO_CHAR );
        return SR_IMMUNE;
    }

    dam = get_sn_damage( sn, level, ch );
    dam += dam * (ch->alignment - victim->alignment) / 4000;
    if ( saves_spell(victim, ch, level, DAM_HOLY) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_HOLY, TRUE);
    return TRUE;
}


DEF_SPELL_FUN(spell_dispel_good)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    /* Neutral victims are not affected. */
    if ( IS_NEUTRAL(victim) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return SR_IMMUNE;
    }

    /* Evil victims are not affected either, and are protected by their god if they have one. */
    if ( IS_EVIL(victim) )
    {
        sprintf(log_buf, "%s protects $N.", get_god_name(victim));
        act( log_buf, ch, NULL, victim, TO_CHAR );
        return SR_IMMUNE;
    }

    dam = get_sn_damage( sn, level, ch );
    dam += dam * (victim->alignment - ch->alignment) / 4000;
    if ( saves_spell(victim, ch, level, DAM_NEGATIVE) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
    return TRUE;
}


/* modified for enhanced use */

DEF_SPELL_FUN(spell_dispel_magic)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (IS_SET(victim->imm_flags, IMM_MAGIC))
    {
        send_to_char( "Your victim is immune to magic.\n\r", ch);
        return SR_IMMUNE;
    }

    if (saves_spell(victim, ch, level, DAM_OTHER))
    {     
        send_to_char( "You feel a brief tingling sensation.\n\r",victim);
        send_to_char( "You failed.\n\r", ch);
        return TRUE;
    }

    if ( check_dispel_magic(level, victim) )
        send_to_char("Ok.\n\r",ch);
    else
        send_to_char("Spell failed.\n\r",ch);
    return TRUE;
}

DEF_SPELL_FUN(spell_earthquake)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    send_to_char( "The earth trembles beneath your feet!\n\r", ch );
    act( "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );

    dam = get_sn_damage( sn, level, ch ) * AREA_SPELL_FACTOR;

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ( vch->in_room == ch->in_room )
        {
            if ( is_safe_spell(ch, vch, TRUE) || IS_AFFECTED(vch, AFF_FLYING) )
                continue;
            if ( saves_spell(vch, ch, level, DAM_BASH) )
                full_dam(ch, vch, dam/2, sn, DAM_BASH, TRUE);
            else
            {
                full_dam(ch, vch, dam, sn, DAM_BASH, TRUE);
                if ( IS_DEAD(vch) )
                    continue;
                send_to_char("You are knocked prone.\n\r", vch);
                act("$n is knocked prone.", vch, NULL, NULL, TO_ROOM);
                set_pos(vch, POS_RESTING);
                check_lose_stance(vch);
            }
            continue;
        }

        if ( vch->in_room->area == ch->in_room->area )
            send_to_char( "The earth trembles and shivers.\n\r", vch );
    }

    return TRUE;
}

DEF_SPELL_FUN(spell_enchant_arrow)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    const char *arg;
    int nr, mana;
    int type;

    if (obj == NULL || obj->item_type != ITEM_ARROWS)
    {
        send_to_char( "That isn't a pack of arrows.\n\r", ch);
        return SR_TARGET;
    }

    if ( (nr = obj->value[0]) <= 0 )
    {
        bugf( "spell_enchant_arrow: %d arrows", nr );
        extract_obj( obj );
        send_to_char( "There are no arrows to enchant.\n\r", ch );
        return SR_TARGET;
    }

    /* get enchantment type */
    arg = one_argument( target_name, buf );
    arg = one_argument( arg, buf );
    if ( !strcmp(buf,"fire") )
        type = DAM_FIRE;
    else if ( !strcmp(buf,"cold") )
        type = DAM_COLD;
    else if ( !strcmp(buf,"lightning") )
        type = DAM_LIGHTNING;
    else if ( !strcmp(buf,"poison") )
        type = DAM_POISON;
    else if ( !strcmp(buf,"acid") )
        type = DAM_ACID;
    else
    {
        send_to_char( "What type of enchantment do you want?\n\r", ch );
        send_to_char( "You can choose fire, cold, lightning, poison or acid.\n\r", ch );
        return SR_SYNTAX;
    }

    /* no re-enchanting */
    if ( obj->value[1] > 0 )
    {
        send_to_char( "The arrows are already enchanted.\n\r", ch );
        return SR_TARGET;
    }

    SPELL_CHECK_RETURN
    
    /* extra mana cost based on nr of arrows */
    nr = meta_magic_adjust_cost(ch, nr, false);
    mana = UMIN(nr, ch->mana);
    ch->mana -= mana;
    if ( number_range(1, nr) > mana )
    {
        send_to_char( "Your power is too low. The spell failed.\n\r", ch );
        return TRUE;
    }

    // prevent creation of unusable arrows due to mastery
    obj->level = UMIN(level, ch->level);
    obj->value[1] = 10 + level;
    obj->value[2] = type;

    /* give new name according to type */
    free_string( obj->name );
    free_string( obj->short_descr );
    free_string( obj->description  );
    switch ( type )
    {
        default:
            obj->name = str_dup( "bugged arrows" );
            obj->short_descr = str_dup( "!Bugged Arrows!" );
            obj->description = str_dup( "A pack of {DBUGGED{x Arrows{x are here." );
            break;
        case DAM_FIRE:
            obj->name = str_dup( "flaming arrows" );
            obj->short_descr = str_dup( "{RFlaming Arrows{x" );
            obj->description = str_dup( "A pack of {RFlaming Arrows{x are here." );
            break;
        case DAM_COLD:
            obj->name = str_dup( "freezing arrows" );
            obj->short_descr = str_dup( "{BFreezing Arrows{x" );
            obj->description = str_dup( "A pack of {BFreezing Arrows{x are here." );
            break;
        case DAM_LIGHTNING:
            obj->name = str_dup( "shocking arrows" );
            obj->short_descr = str_dup( "{YShocking Arrows{x" );
            obj->description = str_dup( "A pack of {YShocking Arrows{x are here." );
            break;
        case DAM_POISON:
            obj->name = str_dup( "poison arrows" );
            obj->short_descr = str_dup( "{GPoison Arrows{x" );
            obj->description = str_dup( "A pack of {GPoison Arrows{x are here." );
            break;
        case DAM_ACID:
            obj->name = str_dup( "acid arrows" );
            obj->short_descr = str_dup( "{DAcid Arrows{x" );
            obj->description = str_dup( "A pack of {DAcid Arrows{x are here." );
            break;
    }

    send_to_char( "Your arrows are now enchanted.\n\r", ch );
    return TRUE;
}

DEF_SPELL_FUN(spell_enchant_armor)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    const char *arg;
    
    if (obj->item_type != ITEM_ARMOR)
    {
        send_to_char("That's not an armor.\n\r",ch);
        return SR_TARGET;
    }
    
    arg = one_argument(target_name, buf);
    one_argument(arg, buf);
    
    return spell_enchant_obj(ch, obj, level, buf, check, sn);
}

DEF_SPELL_FUN(spell_enchant_weapon)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    const char *arg;
    
    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("That's not a weapon.\n\r",ch);
        return SR_TARGET;
    }
    
    arg = one_argument(target_name, buf);
    one_argument(arg, buf);
    
    return spell_enchant_obj(ch, obj, level, buf, check, sn);
}

/*
 * Drain mana and move and add percentage to caster.
 */
DEF_SPELL_FUN(spell_energy_drain)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int drain, drain_mana, drain_move;

    if ( saves_spell(victim, ch, level, DAM_NEGATIVE) )
    {
        act( "$N shivers slightly as the spell passes harmlessly over $S body.",
                ch, NULL, victim, TO_CHAR );
        send_to_char("You feel a momentary chill.\n\r",victim);     
        return TRUE;
    }

    drop_align( ch );

    drain = get_sn_damage( sn, level, ch ) / 2;
    /* if one stat is fully drained, drain missing points on other */
    drain_mana = UMIN( drain, victim->mana );
    drain_move = UMIN( 2*drain - drain_mana, victim->move );
    drain_mana = UMIN( 2*drain - drain_move, victim->mana );

    /* drain from victim and add to caster */
    victim->mana -= drain_mana;
    victim->move -= drain_move;
    ch->mana += drain_mana / 5;
    ch->move += drain_move / 5;

    send_to_char("You feel your life slipping away!\n\r",victim);
    send_to_char("Wow....what a rush!\n\r",ch);

    dam_message( ch, victim, drain_mana + drain_move, sn, FALSE );

    return TRUE;
}



DEF_SPELL_FUN(spell_fireball)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    act("A ball of fire explodes from $n's hands!",ch,NULL,NULL,TO_ROOM);
    act("A ball of fire explodes from your hands.",ch,NULL,NULL,TO_CHAR);

    dam = get_sn_damage( sn, level, ch ) * AREA_SPELL_FACTOR;

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if ( is_safe_spell(ch,vch,TRUE) )
            continue;

        if (saves_spell(vch, ch, level, DAM_FIRE))
            full_dam(ch,vch,dam/2,sn,DAM_FIRE,TRUE);
        else
            full_dam(ch,vch,dam,sn,DAM_FIRE,TRUE);
    }
    return TRUE;
}


DEF_SPELL_FUN(spell_fireproof)
{
    SPELL_CHECK_RETURN
    
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;

    if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
    {
        act("$p is already protected from burning.",ch,obj,NULL,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = ITEM_BURN_PROOF;

    affect_to_obj(obj,&af);

    act("You protect $p from fire.",ch,obj,NULL,TO_CHAR);
    act("$p is surrounded by a protective aura.",ch,obj,NULL,TO_ROOM);
    return TRUE;
}

DEF_SPELL_FUN(spell_flamestrike)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = get_sn_damage( sn, level, ch );
    if ( saves_spell(victim, ch, level, DAM_FIRE) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_FIRE ,TRUE);
    return TRUE;
}

DEF_SPELL_FUN(spell_faerie_fire)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) )
    {
        act( "$N is already affected by faerie fire.", ch, NULL, victim, TO_CHAR );
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_AC;
    af.modifier  = 2 * level;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char( victim, &af );
    send_to_char( "You are surrounded by a pink outline.\n\r", victim );
    act( "$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM );
    return TRUE;
}



DEF_SPELL_FUN(spell_faerie_fog)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *ich;
    OBJ_DATA *obj;

    act( "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
    send_to_char( "You conjure a cloud of purple smoke.\n\r", ch );

    for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
    {
        if (ich->invis_level > 0)
            continue;

        if ( ich == ch || saves_spell(ich, ch, level, DAM_OTHER) )
            continue;

        affect_strip ( ich, gsn_hide            );
        affect_strip ( ich, gsn_invis           );
        affect_strip ( ich, gsn_mass_invis      );
        //        affect_strip ( ich, gsn_sneak           );
        affect_strip ( ich, gsn_shelter         );
        affect_strip ( ich, gsn_astral          );
        affect_strip ( ich, gsn_mimic           );
        REMOVE_BIT   ( ich->affect_field, AFF_HIDE    );
        REMOVE_BIT   ( ich->affect_field, AFF_INVISIBLE  );
        REMOVE_BIT   ( ich->affect_field, AFF_SNEAK   );
        REMOVE_BIT   ( ich->affect_field, AFF_SHELTER );
        REMOVE_BIT   ( ich->affect_field, AFF_ASTRAL  );
        act( "$n is revealed!", ich, NULL, NULL, TO_ROOM );
        send_to_char( "You are revealed!\n\r", ich );
    }

    /* reveal objects in room.. */
    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
        if ( !IS_OBJ_STAT(obj, ITEM_INVIS)
                || number_range(0, level) <= number_range(0, obj->level) )
            continue;

        affect_strip_obj( obj, gsn_invis );
        REMOVE_BIT(obj->extra_flags, ITEM_INVIS);
        act("$p is revealed!",ch,obj,NULL,TO_ALL);
    }
    /* ..and in inventory */
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( !IS_OBJ_STAT(obj, ITEM_INVIS)
                || number_range(0, level) <= number_range(0, obj->level) )
            continue;

        affect_strip_obj( obj, gsn_invis );
        REMOVE_BIT(obj->extra_flags, ITEM_INVIS);
        act("$p is revealed!",ch,obj,NULL,TO_CHAR);
    }

    return TRUE;
}

DEF_SPELL_FUN(spell_floating_disc)
{
    OBJ_DATA *disc, *floating;

    floating = get_eq_char(ch,WEAR_FLOAT);
    if (floating != NULL && IS_OBJ_STAT(floating,ITEM_NOREMOVE))
    {
        act("You can't remove $p.",ch,floating,NULL,TO_CHAR);
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    disc = create_object_vnum(OBJ_VNUM_DISC);
    disc->value[0]  = ch->level * 10; /* 10 pounds per level capacity */
    disc->value[3]  = ch->level * 5; /* 5 pounds per level max per item */
    disc->timer     = get_duration(sn, level); 

    act("$n has created a floating black disc.",ch,NULL,NULL,TO_ROOM);
    send_to_char("You create a floating disc.\n\r",ch);
    obj_to_char(disc,ch);
    wear_obj(ch,disc,TRUE);
    return TRUE;
}


DEF_SPELL_FUN(spell_fly)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_FLYING) )
    {
        if (victim == ch)
            send_to_char("You are already airborne.\n\r",ch);
        else
            act("$N doesn't need your help to fly.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char( victim, &af );
    send_to_char( "Your feet rise off the ground.\n\r", victim );
    act( "$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM );
    return TRUE;
}

/* RT clerical berserking spell */

DEF_SPELL_FUN(spell_frenzy)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim, skill_lookup("calm")) )
    {
        if (victim == ch)
            send_to_char("Why don't you just relax for a while?\n\r", ch);
        else
            act("$N doesn't look like $e wants to fight anymore.", ch, NULL, victim, TO_CHAR);
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    if (is_affected(victim,sn) || IS_AFFECTED(victim,AFF_BERSERK))
    {
        if (victim == ch)
            send_to_char("You are already in a frenzy.\n\r",ch);
        else
            act("$N is already in a frenzy.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.modifier  = level / 6;
    af.bitvector = AFF_BERSERK;

    af.location  = APPLY_HITROLL;
    affect_to_char(victim,&af);

    af.location  = APPLY_DAMROLL;
    affect_to_char(victim,&af);

    af.modifier  = 10 * (level / 12);
    af.location  = APPLY_AC;
    affect_to_char(victim,&af);

    if (IS_GOOD(ch) || IS_NEUTRAL(ch))
        send_to_char("You are filled with holy wrath!\n\r",victim);
    else
        send_to_char("You are filled with unholy wrath!\n\r",victim);

    act("$n gets a wild look in $s eyes!",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

// returns original room unless a misgate occurs, in which case a random room is returned
ROOM_INDEX_DATA* room_with_misgate( CHAR_DATA *ch, ROOM_INDEX_DATA *to_room, int misgate_chance )
{
    if ( per_chance(misgate_chance) || (IS_AFFECTED(ch, AFF_CURSE) && per_chance(20)) )
    {
        OBJ_DATA *stone = get_eq_char(ch,WEAR_HOLD);
        // warpstone prevents misgate but is destroyed in the process
        if ( stone != NULL && stone->item_type == ITEM_WARP_STONE )
        {
            printf_to_char(ch, "{R%s {Rflares brightly and vanishes!{x\n\r", stone->short_descr);
            extract_obj(stone);
        }
        else
        {
            ROOM_INDEX_DATA *rand_room = get_random_room(ch);
            if ( rand_room != NULL )
                to_room = rand_room;
        }
    }

    return to_room;
}

/* RT ROM-style gate */
DEF_SPELL_FUN(spell_gate)
{
    CHAR_DATA *victim;
    bool gate_pet;
    ROOM_INDEX_DATA *to_room;
    AREA_DATA *from_area;

    if ( !can_cast_transport(ch) )
        return SR_UNABLE;

    ignore_invisible = TRUE;
    if ( ( victim = get_char_world( ch, target_name ) ) == NULL 
            || (!IS_NPC(victim) && victim->level > LEVEL_HERO) ) /*not trust*/
    {
        send_to_char( "You failed completely.\n\r", ch );
        return SR_TARGET;
    }
    ignore_invisible = FALSE;

    if ( !can_move_room(ch, victim->in_room, FALSE)
            ||   IS_TAG(ch) || IS_TAG(victim)
            ||   victim->in_room == NULL
            ||   !can_see_room(ch,victim->in_room) 
            ||   !is_room_ingame(victim->in_room)
            ||   IS_SET(victim->in_room->room_flags, ROOM_JAIL)
            ||   IS_SET(victim->in_room->room_flags, ROOM_NO_TELEPORT) )
    {
        send_to_char( "Your powers can't get you there.\n\r", ch );
        return SR_UNABLE;
    }

    if ( victim->level > level + 5 )
    {
        send_to_char( "Your powers aren't strong enough.\n\r", ch );
        return SR_UNABLE;
    }

    if ( !is_same_group(ch, victim) &&
        ((IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
         || (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOSUMMON))) )
    {
        send_to_char( "Your target refuses your company.\n\r", ch );
        return SR_UNABLE;
    }

    if ( ch->in_room == victim->in_room )
    {
        send_to_char( "You're already there!\n\r", ch );
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    check_sn_multiplay( ch, victim, sn );

    if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
        gate_pet = TRUE;
    else
        gate_pet = FALSE;

    /* check for exit triggers */
    if ( !IS_NPC(ch) )
    {
        if ( !rp_exit_trigger(ch) )
            return TRUE;
        if ( !ap_rexit_trigger(ch) )
            return TRUE;
        if ( !ap_exit_trigger(ch, victim->in_room->area) )
            return TRUE;
    }

    from_area=ch->in_room ? ch->in_room->area : NULL;
    
    act("$n steps through a gate and vanishes.",ch,NULL,NULL,TO_ROOM);
    send_to_char("You step through a gate and vanish.\n\r",ch);
    char_from_room(ch);

    to_room = room_with_misgate(ch, victim->in_room, 5);
    char_to_room(ch, to_room);

    act("$n has arrived through a gate.",ch,NULL,NULL,TO_ROOM);
    do_look(ch,"auto");

    if (gate_pet)
    {
        act("$n steps through a gate and vanishes.",ch->pet,NULL,NULL,TO_ROOM);
        send_to_char("You step through a gate and vanish.\n\r",ch->pet);
        char_from_room(ch->pet);
        char_to_room(ch->pet, to_room);
        act("$n has arrived through a gate.",ch->pet,NULL,NULL,TO_ROOM);
        do_look(ch->pet,"auto");
    }

    if ( !IS_NPC(ch) )
    {
        ap_enter_trigger(ch, from_area);
        ap_renter_trigger(ch);
        rp_enter_trigger(ch);
    }
    return TRUE;
}



DEF_SPELL_FUN(spell_giant_strength)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
            send_to_char("You are already as strong as you can get!\n\r",ch);
        else
            act("$N can't get any stronger.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    if (IS_AFFECTED(victim,AFF_WEAKEN))
    {
        if (!check_dispel(level,victim,skill_lookup("weaken")))
        {
            if (victim != ch)
		        act( "Spell failed to grant $N with giant strength.\n\r", ch, NULL, victim, TO_CHAR );
            send_to_char("You feel momentarily stronger.\n\r",victim);
            return TRUE;
        }
        act("$n's muscles return to normal.",victim,NULL,NULL,TO_ROOM);
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_STR;
    af.modifier  = (20 + level) / 4;
    af.bitvector = AFF_GIANT_STRENGTH;
    affect_to_char( victim, &af );
    send_to_char( "Your muscles surge with heightened power!\n\r", victim );
    act("$n's muscles surge with heightened power.",victim,NULL,NULL,TO_ROOM);
    if (ch != victim)
        send_to_char( "Ok.\n\r", ch);
    return TRUE;
}

/* used for harm as well as cause X spells */
DEF_SPELL_FUN(spell_cause_harm)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int harm = get_sn_damage(sn, level, ch);
    // scale with victim's health
    harm *= (1 + 3.0 * victim->hit / victim->max_hit) / 4;
    if ( IS_AFFECTED(victim, AFF_SANCTUARY) || saves_spell(victim, ch, level, DAM_HARM) )
        harm /= 2;
    direct_damage(ch, victim, harm, sn);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}

/* RT haste spell */
DEF_SPELL_FUN(spell_haste)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_HASTE) )
    {
        if (victim == ch)
            send_to_char("You can't move any faster!\n\r",ch);
        else
            act("$N is already moving as fast as $E can.",
                    ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    if (IS_AFFECTED(victim,AFF_SLOW))
    {
        if (!check_dispel(level,victim,skill_lookup("slow")))
        {
            if (victim != ch)
                send_to_char("Your haste spell failed.\n\r",ch);
            send_to_char("You feel momentarily faster.\n\r",victim);
            return TRUE;
        }
        act("$n is moving less slowly.",victim,NULL,NULL,TO_ROOM);
        //return;
    }

    if (IS_AFFECTED(victim,AFF_ENTANGLE))
    {
        if (!check_dispel(level,victim,skill_lookup("entangle")))
        {
            if (victim != ch)
                send_to_char("Your haste spell failed.\n\r",ch);
            send_to_char("You feel momentarily faster.\n\r",victim);
            return TRUE;
        }
        act("With haste, $n escapes $s entaglement!",victim,NULL,NULL,TO_ROOM);
        //return;                       
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_AGI;
    af.modifier  = 1 + level/5;
    af.bitvector = AFF_HASTE;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself moving more quickly.\n\r", victim );
    act("$n is moving more quickly.",victim,NULL,NULL,TO_ROOM);
    //printf_to_char(ch, "gsn: %d sn: %d", gsn_haste, sn);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}

DEF_SPELL_FUN(spell_heal)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal = get_sn_heal( sn, level, ch, victim );

    gain_hit(victim, heal);
    update_pos( victim );
    
    if ( victim->max_hit <= victim->hit )
    {
        send_to_char( "You feel excellent!\n\r", victim );
        if ( ch != victim )
            act( "$E is fully healed.", ch, NULL, victim, TO_CHAR );
    }
    else
    {
        send_to_char( "A warm feeling fills your body.\n\r", victim );
        if ( ch != victim )
            act( "You heal $N.", ch, NULL, victim, TO_CHAR );
    }
    return TRUE;
}

DEF_SPELL_FUN(spell_heat_metal)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj, *obj_next;
    int dam = 0;

    SPELL_CHECK_RETURN
    
    if ( !IS_SET(victim->imm_flags, IMM_FIRE) && !saves_spell(victim, ch, level, DAM_FIRE) )
    {
        // constructs take extra damage
        if ( IS_SET(victim->form, FORM_CONSTRUCT) )
            dam += 20;
        
        // burn damage for each object worn
        for ( obj = victim->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            if ( obj->wear_loc == WEAR_NONE || IS_OBJ_STAT(obj, ITEM_NONMETAL) )
                continue;
            
            if ( IS_OBJ_STAT(obj, ITEM_BURN_PROOF) && per_chance(50) )
                continue;
            
            // damage depends on wear location
            int obj_dam = 0;
            int itemwear = wear_to_itemwear(obj->wear_loc);
            switch ( itemwear )
            {
                case ITEM_WIELD:
                    if ( IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) )
                        obj_dam = 5;
                    else
                        obj_dam = 3;
                    break;
                case ITEM_WEAR_SHIELD:
                    obj_dam = 5;
                    break;
                case ITEM_HOLD:
                    obj_dam = 2;
                    break;
                default:
                    obj_dam = itemwear_ac_factor(itemwear);
                    if ( IS_OBJ_STAT(obj, ITEM_HEAVY_ARMOR) )
                        obj_dam *= 2;
                    break;
            }
            
            if ( obj_dam == 0 )
                continue;
            
            // discipline check - failure means item is unequipped
            if ( !IS_SET(obj->extra_flags, ITEM_NOREMOVE) && !per_chance(get_curr_stat(victim, STAT_DIS) / 2) )
            {
                if ( obj->item_type == ITEM_WEAPON )
                {
                    act("$n screams and drops $p!", victim, obj, NULL, TO_ROOM);
                    act("You drop $p before it burns you.", victim, obj, NULL, TO_CHAR);
                    unequip_char(victim, obj);
                    SET_BIT(obj->extra_flags, ITEM_DISARMED);
                }
                else
                {
                    act("$n screams and removes $p!", victim, obj, NULL, TO_ROOM);
                    act("You remove $p before it burns you.", victim, obj, NULL, TO_CHAR);
                    unequip_char(victim, obj);
                }
                // dexterity check - failure means some burn damage still applies
                if ( !per_chance(get_curr_stat(victim, STAT_DEX) / 2) )
                    dam += rand_div(obj_dam, 2);
            }
            else
                dam += obj_dam;
        }
    }
    
    if ( !dam )
    {
        send_to_char("Your spell failed to have an effect.\n\r", ch);
        send_to_char("You feel momentarily warmer.\n\r", victim);
    }
    else
    {
        dam = get_sn_damage(sn, level, ch) * dam / 50;
        full_dam(ch, victim, dam, sn, DAM_FIRE, TRUE);
    }
    return TRUE;
}

/* RT really nasty high-level attack spell */
DEF_SPELL_FUN(spell_holy_word)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam, dam_type, curr_dam;
    int bless_num, curse_num, frenzy_num;
    bool same_align;

    if ( IS_NPC(ch)
            || was_obj_cast
            || ch->pcdata->trained_hit < 2
            || ch->pcdata->trained_mana < 2
            || ch->pcdata->trained_move < 2
            || IS_AFFECTED(ch, AFF_CHARM)
            || IS_AFFECTED(ch, AFF_INSANE)
            || (ch->hit > ch->max_hit / 2
                && ch->mana > ch->max_mana / 2
                && ch->move > ch->max_move / 2) )
    {
        send_to_char( "The gods don't answer your call!\n\r", ch );
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    act("$n utters a word of divine power!",ch,NULL,NULL,TO_ROOM);
    send_to_char("You utter a word of divine power.\n\r",ch);

    bless_num = skill_lookup("bless");
    curse_num = skill_lookup("curse");
    frenzy_num = skill_lookup("frenzy");

    dam = get_sn_damage(sn, level, ch) * AREA_SPELL_FACTOR;

    if ( IS_GOOD(ch) )
        dam_type = DAM_HOLY;
    else if ( IS_EVIL(ch) )
        dam_type = DAM_NEGATIVE;
    else
        dam_type = DAM_HARM;

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;

        same_align = (IS_GOOD(ch) && IS_GOOD(vch))
            || (IS_EVIL(ch) && IS_EVIL(vch))
            || (IS_NEUTRAL(ch) && IS_NEUTRAL(vch));

        if ( is_same_group(ch, vch) )
        {
            send_to_char("You feel full more powerful.\n\r",vch);
            /* full heal */
            restore_char( vch );
            /* little spellup */
            if ( !IS_AFFECTED(vch, AFF_SANCTUARY) )
                spell_sanctuary(gsn_sanctuary, level, ch, (void*)vch, TARGET_CHAR, FALSE);
            if ( !is_affected(vch, gsn_prayer) && !is_affected(vch, gsn_bless) )
                spell_bless(bless_num, level, ch, (void*)vch, TARGET_CHAR, FALSE);
            if ( same_align && !IS_AFFECTED(vch, AFF_BERSERK) )
                spell_frenzy(frenzy_num, level, ch, (void*)vch, TARGET_CHAR, FALSE);
        }

        else if ( vch->fighting != NULL
                && is_same_group(ch, vch->fighting)
                && !is_safe_spell(ch,vch,TRUE)
                && !same_align )
        {
            spell_curse(curse_num, level, ch, (void*)vch, TARGET_CHAR, FALSE);
            send_to_char("You are struck down!\n\r",vch);

            curr_dam = dam;
            if ( IS_UNDEAD(vch) )
                curr_dam = curr_dam * 3/2;
            if ( saves_spell(vch, ch, level, dam_type) )
                curr_dam /= 2;
            full_dam(ch,vch,curr_dam,sn,dam_type,TRUE);
        }
    }  

    /* sacrifice permanent stats */
    send_to_char( "You feel drained.\n\r", ch );
    ch->pcdata->trained_hit -= 2;
    ch->pcdata->trained_mana -= 2;
    ch->pcdata->trained_move -= 2;
    update_perm_hp_mana_move( ch );
    return TRUE;
}

DEF_SPELL_FUN(spell_identify)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
    int ac = 0;

    if ( (ch->level+10) < obj->level)
    {
        sprintf( buf, "You must be at least level %d to identify %s{x.\n\r" , obj->level - 10, obj->short_descr);
        send_to_char(buf, ch);
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    sprintf( buf,
            "Object %s is type %s, extra flags %s.\n\rWeight is %d, level is %d.\n\r",
            obj->short_descr,
            item_name( obj->item_type ),        
            extra_bits_name( obj->extra_flags ),
            obj->weight / 10,
            obj->level
           );
    send_to_char( buf, ch );

    if ( IS_OBJ_STAT(obj, ITEM_TRANSLUCENT_EX) )
    {
        int lore_level = get_lore_level(ch, obj->level);
        int tattoo_percent = (int)(tattoo_bonus_factor(get_obj_tattoo_level(obj->level, lore_level)) * 100);
        printf_to_char(ch,  "It is translucent so tattoos will shine through (%d%% bonus).\n\r", tattoo_percent );
    }

    if (obj->owner != NULL)
        printf_to_char(ch, "Owned by: %s\n\r", capitalize(obj->owner));

    switch ( obj->item_type )
    {
        case ITEM_SCROLL: 
        case ITEM_POTION:
        case ITEM_PILL:
            sprintf( buf, "Level %d spells of:", obj->value[0] );
            send_to_char( buf, ch );

            if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
            {
                send_to_char( " '", ch );
                send_to_char( skill_table[obj->value[1]].name, ch );
                send_to_char( "'", ch );
            }

            if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
            {
                send_to_char( " '", ch );
                send_to_char( skill_table[obj->value[2]].name, ch );
                send_to_char( "'", ch );
            }

            if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
            {
                send_to_char( " '", ch );
                send_to_char( skill_table[obj->value[3]].name, ch );
                send_to_char( "'", ch );
            }

            if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
            {
                send_to_char(" '",ch);
                send_to_char(skill_table[obj->value[4]].name,ch);
                send_to_char("'",ch);
            }

            send_to_char( ".\n\r", ch );
            break;

        case ITEM_WAND: 
        case ITEM_STAFF: 
            sprintf( buf, "Has %d/%d charges of level %d",
                    obj->value[2], obj->value[1], obj->value[0] );
            send_to_char( buf, ch );

            if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
            {
                send_to_char( " '", ch );
                send_to_char( skill_table[obj->value[3]].name, ch );
                send_to_char( "'", ch );
            }

            send_to_char( ".\n\r", ch );
            break;

        case ITEM_DRINK_CON:
            sprintf(buf,"It holds %s-colored %s.\n\r",
                    liq_table[obj->value[2]].liq_color,
                    liq_table[obj->value[2]].liq_name);
            send_to_char(buf,ch);
            break;

        case ITEM_CONTAINER:
            sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
                    obj->value[0], obj->value[3], cont_bits_name(obj->value[1]));
            send_to_char(buf,ch);
            if (obj->value[4] != 100)
            {
                sprintf(buf,"Weight multiplier: %d%%\n\r",
                        obj->value[4]);
                send_to_char(buf,ch);
            }
            break;

        case ITEM_WEAPON:
            send_to_char("Weapon type is ",ch);
            switch (obj->value[0])
            {
                case(WEAPON_EXOTIC) : send_to_char("exotic.\n\r",ch);   break;
                case(WEAPON_SWORD)  : send_to_char("sword.\n\r",ch);    break;  
                case(WEAPON_DAGGER) : send_to_char("dagger.\n\r",ch);   break;
                case(WEAPON_SPEAR)  : send_to_char("spear/staff.\n\r",ch);  break;
                case(WEAPON_MACE)   : send_to_char("mace/club.\n\r",ch);    break;
                case(WEAPON_AXE)    : send_to_char("axe.\n\r",ch);      break;
                case(WEAPON_FLAIL)  : send_to_char("flail.\n\r",ch);    break;
                case(WEAPON_WHIP)   : send_to_char("whip.\n\r",ch);     break;
                case(WEAPON_POLEARM): send_to_char("polearm.\n\r",ch);  break;
                case(WEAPON_GUN): send_to_char("gun.\n\r",ch);  break; 
                case(WEAPON_BOW): send_to_char("bow.\n\r",ch);  break; 
                default     : send_to_char("unknown.\n\r",ch);  break;
            }
            sprintf(buf,"It does %s damage of %dd%d (average %d).\n\r",
                    attack_table[obj->value[3]].noun,
                    obj->value[1],obj->value[2],
                    (1 + obj->value[2]) * obj->value[1] / 2);
            send_to_char( buf, ch );
            if (obj->value[4])  /* weapon flags */
            {
                sprintf(buf,"Weapons flags: %s\n\r",weapon_bits_name(obj->value[4]));
                send_to_char(buf,ch);
            }
            break;

        case ITEM_ARMOR:
            {
                const char *wear = wear_location_info(obj->wear_type);
                if ( wear )
                {
                    printf_to_char(ch, "%s\n\r", wear);
                    ac = predict_obj_ac(obj, obj->wear_type);
                }
                if ( ac )
                    printf_to_char(ch, "Armor class is %d.\n\r", ac );
                break;
            }

        case ITEM_LIGHT:
            if ( obj->value[2] >= 0 )
                sprintf( buf, "It has %d hours of light remaining.\n\r", obj->value[2] );
            else
                sprintf( buf, "It is an infinite light source.\n\r" );
            send_to_char( buf, ch );
            break;

        case ITEM_ARROWS:
            sprintf( buf, "It contains %d arrows.\n\r", obj->value[0] );
            send_to_char( buf, ch );
            if ( obj->value[1] > 0 )
            {
                sprintf( buf, "Each arrow deals %d extra %s damage.\n\r",
                        obj->value[1], flag_bit_name(damage_type, obj->value[2]) );
                send_to_char( buf, ch );
            }
            break;
    }

    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        if ( paf->detect_level >= 0 && paf->detect_level <= level )
            show_affect(ch, paf, FALSE);

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
        if ( paf->detect_level >= 0 && paf->detect_level <= level )
            show_affect(ch, paf, FALSE);
    
    return TRUE;
}

DEF_SPELL_FUN(spell_infravision)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_INFRARED) )
    {
        if (victim == ch)
            send_to_char("You can already see in the dark.\n\r",ch);
        else
            act("$N can already see in the dark.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INFRARED;
    affect_to_char( victim, &af );

    send_to_char( "Your eyes glow red.\n\r", victim );
    act( "$n's eyes glow red.", victim, NULL, NULL, TO_ROOM );
    return TRUE;
}



DEF_SPELL_FUN(spell_invis)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /*
    if (IS_REMORT(ch))
    {
        send_to_char("There is noplace to hide in remort.\n\r",ch);
        return;
    }
    */

    /* object invisibility */
    if (target == TARGET_OBJ)
    {
        SPELL_CHECK_RETURN
        
        obj = (OBJ_DATA *) vo;  

        if (IS_OBJ_STAT(obj,ITEM_INVIS))
        {
            act("$p is already invisible.",ch,obj,NULL,TO_CHAR);
            return SR_AFFECTED;
        }

        af.where    = TO_OBJECT;
        af.type     = sn;
        af.level    = level;
        af.duration = get_duration(sn, level);
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector    = ITEM_INVIS;
        affect_to_obj(obj,&af);

        act("$p fades out of sight.",ch,obj,NULL,TO_ALL);
        return TRUE;
    }

    /* character invisibility */
    victim = (CHAR_DATA *) vo;

    /*if (IS_REMORT(ch))
      {
      send_to_char("There is no place to hide in remort.\n\r",ch);
      return;
      }
     */

    if (IS_NOHIDE(ch))
    {
        send_to_char("There is no place to hide.\n\r",ch);
        return SR_UNABLE;
    }

    if (IS_TAG(ch))
    {
        send_to_char("There is no place to hide in freeze tag.\n\r", ch );
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    if ( IS_AFFECTED(victim, AFF_INVISIBLE) )
    {
        if ( victim == ch )
            send_to_char( "You are already invisible.\n\r", ch );
        else
            act( "$N is already invisible.", ch, NULL, victim, TO_CHAR );
        return SR_AFFECTED;
    }    

    if ( IS_AFFECTED(victim, AFF_ASTRAL) )
    {
        send_to_char("All is visible on the Astral plane.\n\r",ch);
        return SR_AFFECTED;
    }

    act( "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char( victim, &af );
    send_to_char( "You fade out of existence.\n\r", victim );
    return TRUE;
}

DEF_SPELL_FUN(spell_improved_invis)
{
    AFFECT_DATA af;

    if ( IS_NOHIDE(ch) || IS_TAG(ch) )
    {
        send_to_char("There is no place to hide.\n\r",ch);
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    if ( is_affected(ch, sn) )
    {
        send_to_char("You are already invisible.\n\r", ch);
        return SR_AFFECTED;
    }    

    if ( IS_AFFECTED(ch, AFF_ASTRAL) )
    {
        send_to_char("All is visible on the Astral plane.\n\r", ch);
        return SR_AFFECTED;
    }

    affect_strip_flag(ch, AFF_INVISIBLE);
    act( "$n fades out of existence.", ch, NULL, NULL, TO_ROOM );

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_AC;
    af.modifier  = -5 * (10 + level);
    af.bitvector = AFF_INVISIBLE;
    affect_to_char(ch, &af);
    send_to_char("You fade out of existence.\n\r", ch);
    return TRUE;
}

DEF_SPELL_FUN(spell_know_alignment)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    char *msg;
    int ap;

    ap = victim->alignment;

    if ( ap >  700 ) msg = "$N has a pure and good aura.";
    else if ( ap >  350 ) msg = "$N is of excellent moral character.";
    else if ( ap >  100 ) msg = "$N is often kind and thoughtful.";
    else if ( ap > -100 ) msg = "$N doesn't have a firm moral commitment.";
    else if ( ap > -350 ) msg = "$N lies to $S friends.";
    else if ( ap > -700 ) msg = "$N is a black-hearted murderer.";
    else msg = "$N is the embodiment of pure evil!.";

    act( msg, ch, NULL, victim, TO_CHAR );
    return TRUE;
}

DEF_SPELL_FUN(spell_lightning_bolt)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = get_sn_damage( sn, level, ch );
    if ( saves_spell(victim, ch, level, DAM_LIGHTNING) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_LIGHTNING ,TRUE);
    return TRUE;
}

DEF_SPELL_FUN(spell_locate_object)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;

    /*	if ( IS_SET( ch->act, PLR_IMMQUEST ) )
        {
        send_to_char( "You can't locate objects during an imm quest!\n\r", ch );
        return;
        }
     */
    buffer = new_buf();

    if ( target_name[0] == '\0' )
    {
        send_to_char( "Which object do you want to locate?\n\r", ch );
        return SR_SYNTAX;
    }

    SPELL_CHECK_RETURN
    
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( target_name, obj->name ) 
                ||   IS_OBJ_STAT(obj,ITEM_NOLOCATE) || number_percent() > 2 * level
                ||   ch->level < obj->level)
            continue;

        found = TRUE;
        number++;

        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;

        if ( in_obj->carried_by != NULL && can_see(ch, in_obj->carried_by))
        {
            sprintf( buf, "one is carried by %s\n\r",
                    PERS(in_obj->carried_by, ch) );
        }
        else
        {
            if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
                sprintf( buf, "one is in %s [Room %d]\n\r",
                        in_obj->in_room->name, in_obj->in_room->vnum);
            else 
                sprintf( buf, "one is in %s\n\r",
                        in_obj->in_room == NULL
                        ? "somewhere" : in_obj->in_room->name );
        }

        buf[0] = UPPER(buf[0]);
        add_buf(buffer,buf);

        if (number >= max_found)
            break;
    }

    if ( !found )
        send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
    else
        page_to_char(buf_string(buffer),ch);

    free_buf(buffer);

    return TRUE;
}



DEF_SPELL_FUN(spell_magic_missile)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int missle;
    int dam;
    int max = get_sn_damage(sn, level, ch) / 10;

    if (get_skill(ch, gsn_magic_missile) == 100 )
        max += 2;

    max = UMAX(1, max);
    for (missle=0; missle<max; missle++)
    {
        dam = dice(4,4);
        if ( saves_spell(victim, ch, level, DAM_ENERGY) )
            dam /= 2;
        full_dam( ch, victim, dam, sn, DAM_ENERGY ,TRUE);
        if ( stop_attack(ch, victim) )
            return TRUE;
    }
    return TRUE;
}

DEF_SPELL_FUN(spell_mass_healing)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *gch;
    int heal_num, refresh_num;

    heal_num = skill_lookup("heal");
    refresh_num = skill_lookup("refresh"); 

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( !can_spellup(ch, gch, sn) )
            continue;
        if ( gch->fighting && is_same_group(ch, gch->fighting) )
            continue;
        if ( IS_NPC(gch) && !IS_AFFECTED(gch, AFF_CHARM) && gch->fighting == NULL)
            continue;

        spell_heal(heal_num, level, ch, (void *) gch, TARGET_CHAR, FALSE);
        spell_refresh(refresh_num, level, ch, (void *) gch, TARGET_CHAR, FALSE);
        check_sn_multiplay(ch, gch, sn);
    }
    return TRUE;
}


DEF_SPELL_FUN(spell_mass_invis)
{
    AFFECT_DATA af;
    CHAR_DATA *gch;

    if (IS_NOHIDE(ch))
    {
        send_to_char("There is no place to hide.\n\r",ch);
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( !is_same_group( gch, ch )
                || IS_AFFECTED(gch, AFF_INVISIBLE) 
                || IS_AFFECTED(gch, AFF_ASTRAL) )
            continue;

        act( "$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM );
        send_to_char( "You slowly fade out of existence.\n\r", gch );

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level/2;
        af.duration  = get_duration(sn, level);
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_INVISIBLE;
        affect_to_char( gch, &af );

        check_sn_multiplay( ch, gch, sn );
    }
    send_to_char( "Ok.\n\r", ch );

    return TRUE;
}



DEF_SPELL_FUN(spell_null)
{
    send_to_char( "That's not a spell!\n\r", ch );
    return FALSE;
}



DEF_SPELL_FUN(spell_pass_door)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_PASS_DOOR) )
    {
        if (victim == ch)
            send_to_char("You are already out of phase.\n\r",ch);
        else
            act("$N is already shifted out of phase.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char( victim, &af );
    act( "$n turns translucent.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You turn translucent.\n\r", victim );
    return TRUE;
}

/* RT plague spell, very nasty */
DEF_SPELL_FUN(spell_plague)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (saves_spell(victim, ch, level, DAM_DISEASE))
    {
        if (ch == victim)
            send_to_char("You feel momentarily ill, but it passes.\n\r",ch);
        else
            act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_STR;
    af.modifier  = -5; 
    af.bitvector = AFF_PLAGUE;
    affect_join(victim,&af);

    send_to_char
        ("You scream in agony as plague sores erupt from your skin.\n\r",victim);
    act("$n screams in agony as plague sores erupt from $s skin.",
            victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

DEF_SPELL_FUN(spell_confusion)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_INSANE) )
    {
        act("$N is already confused.", ch, NULL, victim, TO_CHAR);
        return SR_AFFECTED;
    }
    
    if ( saves_spell(victim, ch, level*2/3, DAM_MENTAL) )
        // || saves_spell(level,victim,DAM_CHARM))
    {
        if (ch == victim)
            send_to_char("{xYou feel momentarily {Ms{yi{Gl{Cl{Ry{x, but it passes.\n\r",ch);
        else
            act("$N seems to keep $S sanity.",ch,NULL,victim,TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_INT;
    af.modifier  = -15; 
    af.bitvector = AFF_INSANE;
    affect_join(victim,&af);

    send_to_char
        ("{MY{bo{Cu{Gr {%{yw{Ro{mr{Bl{Cd{x {gi{Ys {%{ra{Ml{Bi{cv{Ge{x {yw{Ri{Mt{bh {%{wcolors{x{C?{x\n\r",victim);
    act("$n giggles like $e lost $s mind.",
            victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

DEF_SPELL_FUN(spell_poison)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;

        if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
        {
            SPELL_CHECK_RETURN
    
            if (IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
            {
                act("Your spell fails to corrupt $p.",ch,obj,NULL,TO_CHAR);
                return SR_IMMUNE;
            }
            obj->value[3] = 1;
            act("$p is infused with poisonous vapors.",ch,obj,NULL,TO_ALL);
            return TRUE;
        }

        if (obj->item_type == ITEM_WEAPON)
        {
            SPELL_CHECK_RETURN
            
            if (IS_WEAPON_STAT(obj,WEAPON_POISON))
            {
                act("$p is already envenomed.",ch,obj,NULL,TO_CHAR);
                return SR_AFFECTED;
            }

            af.where     = TO_WEAPON;
            af.type  = sn;
            af.level     = level / 2;
            af.duration  = get_duration(sn, level);
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj,&af);

            act("$p is coated with deadly venom.",ch,obj,NULL,TO_ALL);
            return TRUE;
        }

        act("You can't poison $p.",ch,obj,NULL,TO_CHAR);
        return SR_TARGET;
    }

    SPELL_CHECK_RETURN
    
    victim = (CHAR_DATA *) vo;

    if ( saves_spell(victim, ch, level, DAM_POISON) )
    {
        act("$n turns slightly green, but it passes.",victim,NULL,NULL,TO_ROOM);
        send_to_char("You feel momentarily ill, but it passes.\n\r",victim);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_STR;
    af.modifier  = -10;
    af.bitvector = AFF_POISON;
    affect_join( victim, &af );
    send_to_char( "You feel very sick.\n\r", victim );
    act("$n looks very ill.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}



DEF_SPELL_FUN(spell_protection_evil)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (IS_EVIL(victim))
    {
        if (victim == ch)
            send_to_char("You're not pure enough.\n\r",ch);
        else
            act("$N isn't pure enough.",ch,NULL,victim,TO_CHAR);
        return SR_IMMUNE;
    }    

    if ( IS_AFFECTED(victim, AFF_PROTECT_EVIL) 
            || IS_AFFECTED(victim, AFF_PROTECT_GOOD))
    {
        if (victim == ch)
            send_to_char("You are already protected.\n\r",ch);
        else
            act("$N is already protected.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_SAVES;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "You feel holy and pure.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from evil.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}

DEF_SPELL_FUN(spell_protection_good)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (IS_GOOD(victim))
    {
        if (victim == ch)
            send_to_char("You're not that evil.\n\r",ch);
        else
            act("$N isn't that evil.",ch,NULL,victim,TO_CHAR);
        return SR_IMMUNE;
    }    

    if ( IS_AFFECTED(victim, AFF_PROTECT_GOOD) 
            || IS_AFFECTED(victim, AFF_PROTECT_EVIL))
    {
        if (victim == ch)
            send_to_char("You are already protected.\n\r",ch);
        else
            act("$N is already protected.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_SAVES;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char( victim, &af );
    send_to_char( "You feel aligned with darkness.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from good.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}


DEF_SPELL_FUN(spell_ray_of_truth)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if (IS_EVIL(ch) )
    {
        victim = ch;
        send_to_char("The energy explodes inside you!\n\r",ch);
    }

    if (victim != ch)
    {
        act("$n raises $s hand, and a blinding ray of light shoots forth!",
                ch,NULL,NULL,TO_ROOM);
        send_to_char(
                "You raise your hand and a blinding ray of light shoots forth!\n\r",
                ch);
    }

    if ( IS_GOOD(victim) )
    {
        act("$n seems unharmed by the light.",victim,NULL,victim,TO_ROOM);
        send_to_char("The light seems powerless to affect you.\n\r",victim);
        return SR_IMMUNE;
    }

    spell_blindness(gsn_blindness, level/2, ch, (void *)victim, TARGET_CHAR, FALSE);

    dam = get_sn_damage( sn, level, ch );
    dam = dam * ( 1000 - victim->alignment ) / 2000;

    if ( saves_spell(victim, ch, level, DAM_HOLY) )
        dam /= 2;

    full_dam( ch, victim, dam, sn, DAM_HOLY, TRUE);
    return TRUE;
}


DEF_SPELL_FUN(spell_recharge)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int charges, cost;

    if (obj->item_type != ITEM_WAND && obj->item_type != ITEM_STAFF)
    {
        send_to_char("That item does not carry charges.\n\r",ch);
        return SR_TARGET;
    }

    if (obj->value[0] >= level)
    {
        send_to_char("Your skills are not great enough for that.\n\r",ch);
        return SR_UNABLE;
    }

    if (obj->value[1] <= 1)
    {
        send_to_char("That item cannot be recharged anymore.\n\r", ch);
        return SR_UNABLE;
    }

    charges = (obj->value[1] - 1) - obj->value[2];

    if ( charges < 1 )
    {
        send_to_char("That item cannot be recharged further.\n\r", ch);
        return SR_UNABLE;
    }
    
    cost = spell_obj_cost(obj->value[0], spell_base_cost(obj->value[3])) * charges / (obj->item_type == ITEM_WAND ? 8 : 16);
    if ( !has_money(ch, cost) )
    {
        ptc(ch, "It costs %.2f gold to recharge %s.\n\r", cost/100.0, obj->short_descr);
        return SR_UNABLE;
    }
    
    SPELL_CHECK_RETURN

    deduct_cost(ch, cost);
    ptc(ch, "You use up materials worth %.2f gold to restore %d charge%s to %s.\n\r",
        cost/100.0, charges, charges == 1 ? "" : "s", obj->short_descr);
    act("$p glows softly.", ch, obj, NULL, TO_ROOM);
    obj->value[2] += charges;
    // max number of charges is reduced by 1 with each recharge
    obj->value[1] -= 1;
    return TRUE;
}

DEF_SPELL_FUN(spell_refresh)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal = get_sn_heal( sn, level, ch, victim );
    gain_move(victim, heal);
    
    if ( victim->max_move <= victim->move )
    {
        send_to_char( "You feel fully refreshed!\n\r", victim );
        if ( ch != victim )
            act( "$E is fully refreshed.", ch, NULL, victim, TO_CHAR );
    }
    else
    {
        send_to_char( "You feel less tired!\n\r", victim );
        if ( ch != victim )
            act( "You refresh $N.", ch, NULL, victim, TO_CHAR );
    }

    return TRUE;
}

DEF_SPELL_FUN(spell_remove_curse)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char buf[MSL];
    int curse;

    /* do object cases first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;

        if (IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
        {
            if (IS_OBJ_STAT(obj,ITEM_NOUNCURSE))
            {
                act("The curse on $p cannot be removed.",ch,obj,NULL,TO_CHAR);
                return SR_IMMUNE;
            }
            
            if (!saves_dispel(level + 2,obj->level,0))
            {
                REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
                act("$p glows blue.",ch,obj,NULL,TO_ALL);
                return TRUE;
            }

            sprintf(buf,"Spell failed to uncurse %s.\n\r",obj->short_descr);
            send_to_char(buf,ch);
            return TRUE;
        }
        else
        {
            act("There doesn't seem to be a curse on $p.",ch,obj,NULL,TO_CHAR);
            return SR_AFFECTED;
        }
    }

    /* characters */
    victim = (CHAR_DATA *) vo;
    
    bool is_cursed = FALSE;
    bool success = FALSE;
    
    for ( curse = 0; curse < MAX_SKILL; curse++ )
        if ( is_curse(curse) && is_affected(victim, curse) )
        {
            if ( check_dispel(level, victim, curse) )
            {
                success = TRUE;
                break;
            }
            is_cursed = TRUE;
        }
    if ( success )
    {
        send_to_char("You feel better.\n\r",victim);
        act("$n looks more relaxed.",victim,NULL,NULL,TO_ROOM);
        return TRUE;
    }
    if ( is_cursed )
    {
        act("You failed to remove the curse on $N.",ch,NULL,victim,TO_CHAR);
        return TRUE;
    }
    for (obj = victim->carrying; obj != NULL; obj = obj->next_content)
    {
        if (IS_OBJ_STAT(obj,ITEM_NOUNCURSE))
        {
            act("The curse on $p cannot be removed.",ch,obj,NULL,TO_CHAR);
            continue;
        }

        if (IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
        {   /* attempt to remove curse */
            if (!saves_dispel(level,obj->level,0))
            {
                REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
                act("Your $p glows blue.",victim,obj,NULL,TO_CHAR);
                act("$n's $p glows blue.",victim,obj,NULL,TO_ROOM);
                return TRUE;
            }

            sprintf(buf,"Spell failed to uncurse %s.\n\r",obj->short_descr);
            send_to_char(buf,ch);
            return TRUE;
        }
    }

    send_to_char( "There is nothing to uncurse.\n\r", ch );
    return SR_AFFECTED;
} 

DEF_SPELL_FUN(spell_sanctuary)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
    {
        if (victim == ch)
            send_to_char("You are already in sanctuary.\n\r",ch);
        else
            act("$N is already in sanctuary.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a white aura.\n\r", victim );
    return TRUE;
}



DEF_SPELL_FUN(spell_shield)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
            send_to_char("You are already shielded from harm.\n\r",ch);
        else
            act("$N is already protected by a shield.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_AC;
    af.modifier  = -20;
    af.bitvector = AFF_SHIELD;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a force shield.\n\r", victim );
    return TRUE;
}



DEF_SPELL_FUN(spell_shocking_grasp)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( check_hit( ch, victim, sn, DAM_LIGHTNING, 100 ) )
    {
        dam = get_sn_damage( sn, level, ch ) * 14/10;
        if ( saves_spell(victim, ch, level, DAM_LIGHTNING) )
            dam /= 2;
        shock_effect( victim, level, dam, TARGET_CHAR );
    }
    else
        dam = 0;

    full_dam( ch, victim, dam, sn, DAM_LIGHTNING,TRUE);
    return TRUE;
}


DEF_SPELL_FUN(spell_sleep)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    /*
       if ( IS_SET(ch->act, PLR_WAR) )
       {
       send_to_char( "Play nice!\n\r", ch );
       return;
       }
     */

    if ( IS_AFFECTED(victim, AFF_SLEEP) )
    {
        send_to_char("Your victim is already sleeping.\n\r", ch );
        return SR_AFFECTED;
    }

    if ( IS_UNDEAD(victim) )
    {
        send_to_char("The undead never sleep!\n\r", ch );
        return SR_IMMUNE;
    }

    if ( IS_SET(victim->imm_flags, IMM_SLEEP) )
    {
        act( "$N finds you quite boring, but can't be put to sleep.", ch, NULL, victim, TO_CHAR );
        return SR_IMMUNE;
    }

    if ( saves_spell(victim, ch, level, DAM_MENTAL)
            || number_bits(1)
            || (!IS_NPC(victim) && number_bits(1))
            || IS_IMMORTAL(victim) )
    {
        send_to_char("Your spell failed to have an effect.\n\r", ch );
        return TRUE;
    }

    if ( IS_AWAKE(victim) )
    {
        send_to_char( "You feel very sleepy ..... zzzzzz.\n\r", victim );
        act( "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
        stop_fighting( victim, TRUE );
        set_pos( victim, POS_SLEEPING );
    }

    if ( victim->pcdata != NULL )
        victim->pcdata->pkill_timer = 
            UMAX(victim->pcdata->pkill_timer, 10 * PULSE_VIOLENCE);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SLEEP;
    affect_join( victim, &af );

    return TRUE;
}

DEF_SPELL_FUN(spell_slow)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_SLOW))
    {
        if (victim == ch)
            send_to_char("You can't move any slower!\n\r",ch);
        else
            act("$N can't get any slower than that.",
                    ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    if (saves_spell(victim, ch, level, DAM_OTHER) 
            ||  IS_SET(victim->imm_flags,IMM_MAGIC))
    {
        if (victim != ch)
            act( "Spell failed to slow $N down.", ch, NULL, victim,TO_CHAR);
        send_to_char("You feel momentarily lethargic.\n\r",victim);
        return TRUE;
    }

    if (IS_AFFECTED(victim,AFF_HASTE))
    {
        if (!check_dispel(level,victim,skill_lookup("haste")))
        {
            if (victim != ch)
		        act( "Spell failed to reduce $N's speed.", ch, NULL, victim, TO_CHAR );
            send_to_char("You feel momentarily slower.\n\r",victim);
            return TRUE;
        }

        act("$n is moving less quickly.",victim,NULL,NULL,TO_ROOM);
        // might have haste as well as perma-haste
        if ( IS_AFFECTED(victim, AFF_HASTE) )
            return TRUE;
    }


    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_AGI;
    af.modifier  = -1 - level/5;
    af.bitvector = AFF_SLOW;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself slowing d o w n...\n\r", victim );
    act("$n starts to move in slow motion.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}


DEF_SPELL_FUN(spell_stone_skin)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(ch, AFF_STONE_SKIN) )
    {
        if (victim == ch)
            send_to_char("Your skin is already as hard as a rock.\n\r",ch); 
        else
            act("$N is already as hard as can be.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_AC;
    af.modifier  = -40;
    af.bitvector = AFF_STONE_SKIN;
    affect_to_char( victim, &af );
    act( "$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "Your skin turns to stone.\n\r", victim );
    return TRUE;
}


/* returns wether the room contains aggro mobs */
bool is_aggro_room( ROOM_INDEX_DATA *room, CHAR_DATA *victim )
{
    CHAR_DATA *ch;
    int vlevel = 1;

    if ( victim != NULL )
        vlevel = victim->level;

    if ( room == NULL )
        return FALSE;

    for ( ch = room->people; ch != NULL; ch = ch->next_in_room )
        if ( NPC_ACT(ch, ACT_AGGRESSIVE) && ch->level >= vlevel )
            return TRUE;

    return FALSE;
}


DEF_SPELL_FUN(spell_summon)
{
    CHAR_DATA *victim;
    AREA_DATA *from_area;

    ignore_invisible = TRUE;
    if ( (victim = get_char_world(ch, target_name)) == NULL)
    {
        send_to_char( "You failed completely.\n\r", ch );
        return SR_TARGET;
    }
    ignore_invisible = FALSE;
    if ( victim == ch )
    {
        send_to_char( "Summon yourself?\n\r", ch );
        return SR_TARGET;
    }
    if ( IS_TAG(ch)
            || IS_REMORT(ch)
            || IS_SET(ch->in_room->room_flags, ROOM_NO_TELEPORT)
            || IS_SET(ch->in_room->room_flags, ROOM_ARENA))
    {
        send_to_char( "You can't summon anyone here!\n\r", ch );
        return SR_UNABLE;
    }
    if ( !can_move_room(victim, ch->in_room, FALSE) )
    {
        send_to_char( "You can't summon that character here!\n\r", ch );
        return SR_UNABLE;
    }
    if ( !is_same_group(ch, victim) &&
        (IS_NPC(victim) || IS_SET(victim->act, PLR_NOSUMMON) || IS_SET(victim->comm, COMM_AFK)) )
    {
        send_to_char( "Your target does not wish to be summoned.\n\r", ch );
        return SR_TARGET;
    }
    
    SPELL_CHECK_RETURN
    
    if ( IS_IMMORTAL(victim) || victim->level > level + 5 )
    {
        send_to_char( "Your target is too powerful for you to summon.\n\r", ch );
        return TRUE;
    }
    if ( IS_TAG(victim)
            || IS_REMORT(victim)
            || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
            || victim->fighting != NULL )
    {
        send_to_char( "Your target can't be summoned from its location!\n\r", ch );
        return TRUE;
    }

    check_sn_multiplay( ch, victim, sn );
    /* check for possibly illegal summoning */
    if ( is_aggro_room(ch->in_room, victim) && is_always_safe(ch, victim) )
    {
        char buf[MSL];
        sprintf( buf, "Indirect Pkill: %s summoning %s to aggro room %d",
                ch->name, victim->name, ch->in_room->vnum );
        log_string( buf );
        cheat_log( buf );
        wiznet(buf, ch, NULL, WIZ_CHEAT, 0, LEVEL_IMMORTAL);
    }

    if ( !IS_NPC(victim) )
    {
        if ( ( !rp_exit_trigger(victim) )  ||
             ( !ap_rexit_trigger(victim) ) ||
             ( !ap_exit_trigger(victim, ch->in_room->area) ) )
        {
            /* We need to send message to summoner here. Progs should send
               message to the victim */
            send_to_char( "Your target can't be summoned from its location!\n\r",   ch );
            return TRUE;
        }
    }

    from_area=victim->in_room ? victim->in_room->area : NULL;

    act( "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, ch->in_room );
    act( "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
    act( "$n has summoned you!", ch, NULL, victim,   TO_VICT );
    do_look( victim, "auto" );


    if ( !IS_NPC(victim) )
    {
        ap_enter_trigger(victim, from_area);
        ap_renter_trigger(victim);
        rp_enter_trigger(victim);
    }

    return TRUE;
}

DEF_SPELL_FUN(spell_teleport)
{
    ROOM_INDEX_DATA *pRoomIndex;
    OBJ_DATA *stone;
    AREA_DATA *from_area;

    if ( !can_cast_transport(ch) )
        return SR_UNABLE;

    if ( !strcmp(target_name, "locate") )
    {
        SPELL_CHECK_RETURN
        show_portal_names(ch);
        return TRUE;
    }

    if ( target_name[0] == '\0' )
    {
        SPELL_CHECK_RETURN
        pRoomIndex = get_random_room(ch);
        
        if ( pRoomIndex == NULL 
            || !is_room_ingame(pRoomIndex)
            || !can_see_room(ch, pRoomIndex) 
            /* Teleport wasn't working because the IS_SET check was missing - Astark 1-7-13 */
            || !can_move_room(ch, pRoomIndex, FALSE)
            || IS_SET(pRoomIndex->room_flags, ROOM_NO_TELEPORT)
            || IS_SET(pRoomIndex->room_flags, ROOM_JAIL))
        {
            send_to_char( "The room begins to fade from around you, but then it slowly returns.\n\r", ch );
            return TRUE;
        }
    }
    else
    {
        if ( (pRoomIndex = get_portal_room(target_name)) == NULL
            || !can_see_room(ch, pRoomIndex)
            || !is_room_ingame(pRoomIndex) )
        {
            send_to_char( "Teleport destination unknown.\n\r", ch );
            return SR_TARGET;
        }
        SPELL_CHECK_RETURN
        if ( !can_move_room(ch, pRoomIndex, FALSE) )
        {
            send_to_char( "Teleport failed.\n\r", ch );
            return TRUE;
        }
    }    
    
    stone = get_eq_char(ch,WEAR_HOLD);

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR);

        if( chance(15) )
        {
            act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR);
            extract_obj(stone);
        }

        if( IS_SET(pRoomIndex->room_flags, ROOM_NO_RECALL) )
        {
            send_to_char( "The room begins to fade from around you, but then it slowly returns.\n\r", ch );
            return TRUE;
        }
    }

    if ( !IS_NPC(ch) )
    {
        if ( !rp_exit_trigger(ch) )
            return TRUE;
        if ( !ap_rexit_trigger(ch) )
            return TRUE;
        if ( !ap_exit_trigger( ch, pRoomIndex->area) )
            return TRUE;
    }

    from_area=ch->in_room ? ch->in_room->area : NULL;

    act("$n vanishes!", ch, NULL, NULL, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, pRoomIndex);
    act("$n abruptly appears from out of nowhere.", ch, NULL, NULL, TO_ROOM);
    do_look(ch, "auto");

    CHAR_DATA *pet = ch->pet;
    if ( pet != NULL && can_cast_transport(pet) )
    {
        ROOM_INDEX_DATA *pet_location = room_with_misgate(pet, pRoomIndex, 0);
        act("$n disappears.", pet, NULL, NULL, TO_ROOM);
        char_from_room(pet);
        char_to_room(pet, pet_location);
        act("$n appears in the room.", pet, NULL, NULL, TO_ROOM);
    }

    if ( !IS_NPC(ch) )
    {
        ap_enter_trigger(ch, from_area);
        ap_renter_trigger(ch);
        rp_enter_trigger(ch);
    }
    
    return TRUE;
}



DEF_SPELL_FUN(spell_ventriloquate)
{
    char speaker[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;

    target_name = one_argument( target_name, speaker );

    if ( ch == vo )
    {
        send_to_char( "If you want to say something, just do so.\n\r", ch );
        return SR_TARGET;
    }

    SPELL_CHECK_RETURN
    
    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( vch != vo && IS_AWAKE(vch))
        {
            if ( vch == ch )
                act( "{sYou make $n say {S'$t'{x", vo, target_name, vch, TO_VICT );
            else if ( saves_spell(vch, ch, level, DAM_OTHER) )
                act( "{s$n says {S'$t'{x", vo, target_name, vch, TO_VICT );
            else
                act( "{sSomeone makes $n say {S'$t'{x", vo, target_name, vch, TO_VICT );
        }
    }

    return TRUE;
}



DEF_SPELL_FUN(spell_weaken)
{
    SPELL_CHECK_RETURN
    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_WEAKEN))
    {
        if (victim == ch)
            send_to_char("You can't get any weaker.\n\r",ch);
        else
            act("Weak as $E is, your spell had no effect.",ch,NULL,victim,TO_CHAR);
        return SR_AFFECTED;
    }

    if ( saves_spell(victim, ch, level, DAM_OTHER) )
    {
        send_to_char("Your weakening failed to have an effect.\n\r", ch );
        send_to_char("Your strength fails you for a moment.\n\r", victim );
        return TRUE;
    }

    if (IS_AFFECTED(victim,AFF_GIANT_STRENGTH))
    {
        if (!check_dispel(level,victim,skill_lookup("giant strength")))
        {
            if (victim != ch)
		        act( "Spell failed to make $N weaker.\n\r", ch, NULL, victim, TO_CHAR );
            send_to_char("You feel momentarily weaker.\n\r",victim);
            return TRUE;
        }

        act("$n is looking much weaker.",victim,NULL,NULL,TO_ROOM);
        //return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = get_duration(sn, level);
    af.location  = APPLY_STR;
    af.modifier  = -1 * (20 + level) / 2;
    af.bitvector = AFF_WEAKEN;
    affect_to_char( victim, &af );
    send_to_char( "You feel your strength slip away.\n\r", victim );
    act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}



/* RT recall spell is back */
DEF_SPELL_FUN(spell_word_of_recall)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *pet = victim->pet;
    ROOM_INDEX_DATA *location;
    AREA_DATA *from_area;
    int chance;

    if (IS_NPC(victim))
        return SR_UNABLE;

    if ( !can_cast_transport(ch) )
        return SR_UNABLE;

    if ((location = get_room_index( ROOM_VNUM_TEMPLE)) == NULL)
    {
        send_to_char("You are completely lost.\n\r",victim);
        return SR_UNABLE;
    } 

    if (NOT_AUTHED(victim))
    {
        send_to_char("You cannot recall until your character is authorized by the Immortals.\n\r",victim);
        return SR_UNABLE;
    }

    if (IS_TAG(victim))
    {
        send_to_char("You cannot recall while playing Freeze Tag.\n\r",victim);
        return SR_UNABLE;
    }

    if (!IS_NPC(victim) && in_pkill_battle(victim))    
    {
        send_to_char("You cannot recall during a pkill battle!\n\r",victim);
        return SR_UNABLE;
    }

    if (ch->pcdata != NULL && ch->pcdata->pkill_timer > 0)
    {
        send_to_char("Adrenaline is pumping!\n\r", ch);
        return SR_UNABLE;
    }

    SPELL_CHECK_RETURN
    
    if (IS_AFFECTED(victim, AFF_ENTANGLE))
    {
        chance = (number_percent ());
        if (  (chance) >  (get_curr_stat(victim, STAT_LUC)/10) )
        {
            send_to_char( "The plants entangling you hold you in place!\n\r", victim );
            return TRUE;
        }
    }

    /* Added exp loss during combat 2/22/99 -Rim */
    if ( victim->fighting != NULL )
    {
        int lose, skill;

        skill = get_skill(victim, gsn_word_of_recall);
        skill += get_curr_stat(victim, STAT_LUC) / 4 - 15;

        if ( number_percent() > 80 * skill / 100 )
        {
            WAIT_STATE( victim, 4 );
            send_to_char( "Spell failed.\n\r", victim );
            return TRUE;
        }

        stop_fighting( victim, TRUE );

        if (!IS_HERO(ch))
        {
            lose = (victim->desc != NULL) ? 25 : 50;
            gain_exp( victim, 0 - lose );

            printf_to_char(victim, "You recall from combat!  You lose %d exp.\n\r", lose );
        }
        else
            send_to_char("You recall from combat!\n\r",ch);
    }

    // misgate chance when cursed but not normally
    ROOM_INDEX_DATA *victim_location = room_with_misgate(victim, location, 0);

    if ( !IS_NPC(victim) )
    {
        if ( !rp_exit_trigger(victim) )
            return TRUE;
        if ( !ap_rexit_trigger(victim) )
            return TRUE;
        if ( !ap_exit_trigger( victim, victim_location->area) )
            return TRUE; 
    }
    
    from_area=victim->in_room ? victim->in_room->area : NULL;

    act("$n disappears.",victim,NULL,NULL,TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, victim_location);
    act("$n appears in the room.",victim,NULL,NULL,TO_ROOM);
    do_look(victim,"auto");

    if ( pet != NULL && can_cast_transport(pet) )
    {
        ROOM_INDEX_DATA *pet_location = room_with_misgate(pet, location, 0);
        act("$n disappears.", pet, NULL, NULL, TO_ROOM);
        char_from_room(pet);
        char_to_room(pet, pet_location);
        act("$n appears in the room.", pet, NULL, NULL, TO_ROOM);
    }

    if ( !IS_NPC(victim) )
    {
        ap_enter_trigger(victim, from_area);
        ap_renter_trigger(victim);
        rp_enter_trigger(victim);
    } 
    
    return TRUE;
}

/* Draconian & Necromancer spells are in breath.c --Bobble */

int cha_max_follow( CHAR_DATA *ch )
{
    int cha = get_curr_stat(ch, STAT_CHA) + mastery_bonus(ch, gsn_puppetry, 30, 50);
    return ch->level * (200 + cha) / 100;
}

int cha_cur_follow( CHAR_DATA *ch )
{
    CHAR_DATA *check;
    int charmed = 0;

    for ( check = char_list ; check != NULL; check = check->next )
        if ( IS_AFFECTED(check, AFF_CHARM) && check->master == ch && ch->pet != check )
            charmed += check->level;

    return charmed;
}

/* Check number of charmees against cha - returns number of hitdice left over
 * Also send error message when this number is below required amount
 */
int check_cha_follow( CHAR_DATA *ch, int required )
{
    int max = cha_max_follow(ch);
    int charmed = cha_cur_follow(ch);

    if (required > 0 && charmed + required > max)
        send_to_char("You are not charismatic enough to attract more followers.\n\r",ch);

    return UMAX(0, max - charmed);
}

DEF_DO_FUN(do_scribe)
{
    int skill, sn, spell_skill, cost, level;
    char arg1[MSL], arg2[MSL], buf[MSL], buf2[MSL], buf3[MSL];
    OBJ_DATA *scroll;

    if ( (skill = get_skill(ch, gsn_scribe)) == 0 )
    {
        send_to_char( "You don't know how to scribe scrolls.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "What spell do you want to scribe?\n\r", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg2[0] == '\0' )
        level = ch->level;
    else if ( is_number(arg2) )
    {
        level = atoi( arg2 );
        level = URANGE( 0, level, ch->level );
    }
    else
    {
        send_to_char( "Syntax: scribe <spell name> [level]\n\r", ch );
        return;
    }

    if ( (sn = spell_lookup(arg1)) < 0 )
    {
        send_to_char( "That spell doesn't exist.\n\r", ch );
        return;
    }

    if ( (spell_skill = get_skill(ch, sn)) == 0 )
    {
        send_to_char( "You can only scribe spells you know.\n\r", ch );
        return;
    }

    if ( skill_table[sn].target == TAR_CHAR_SELF
            || skill_table[sn].pgsn == &gsn_mimic )
    {
        send_to_char( "You can't scribe that spell.\n\r", ch );
        return;
    }

    cost = skill_table[sn].min_mana;
    if ( skill_table[sn].minimum_position == POS_STANDING )
        cost *= 2;
    cost = cost * skill_table[sn].beats / skill_table[gsn_scrolls].beats;

    if ( cost > ch->mana )
    {
        sprintf( buf, "You need %d mana to scribe that spell.\n\r", cost );
        send_to_char( buf, ch );
        return;
    }

    ch->mana -= cost;
    WAIT_STATE( ch, skill_table[gsn_scribe].beats );

    if ( chance(skill) && chance(spell_skill) )
    {
        if ( (scroll = create_object_vnum(OBJ_VNUM_SCROLL)) == NULL )
        {
            send_to_char( "BUG: no scroll object!\n\r", ch );
            return;
        }

        /* name scroll */
        sprintf( buf, "scroll %s", skill_table[sn].name );
        sprintf( buf2, "scroll of %s", skill_table[sn].name );
        sprintf( buf3, "A scroll of %s lies here.", skill_table[sn].name );
        rename_obj( scroll, buf, buf2, buf3 );
        scroll->cost = cost;

        /* assign spell */
        scroll->level = level;
        scroll->value[0] = UMAX( level - 10, 1 );
        scroll->value[1] = sn;

        /* put it into game */
        if ( ch->carry_number < can_carry_n(ch) )
            obj_to_char( scroll, ch );
        else
            obj_to_room( scroll, ch->in_room );

        act( "You scribe $p.", ch, scroll, NULL, TO_CHAR );
        act( "$n scribes $p.", ch, scroll, NULL, TO_ROOM );

        check_improve( ch, gsn_scribe, TRUE, 2 );
    }
    else
    {
        send_to_char( "You mix up two symbols and fail.\n\r", ch );
        check_improve( ch, gsn_scribe, FALSE, 3 );
    }
}

