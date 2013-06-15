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

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

#include "warfare.h"
#include "religion.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_recall    );

/*
 * Local functions.
 */
void    say_spell   args( ( CHAR_DATA *ch, int sn ) );

bool is_offensive( int sn );
bool can_cast_transport( CHAR_DATA *ch );
void spell_cure_mental( int sn, int level, CHAR_DATA *ch,void *vo, int target );

/* imported functions */
bool    remove_obj  args( ( CHAR_DATA *ch, int iWear, bool fReplace ) );
void    wear_obj    args( ( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace ) );
void    do_flee     args( ( CHAR_DATA *ch, char *argument ) );
bool check_spell_disabled args( (const struct skill_type *command) );
void    dam_message     args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
            int dt, bool immune ) );
bool  in_pkill_battle args( ( CHAR_DATA *ch ) );
RELIGION_DATA *get_religion args( ( CHAR_DATA *ch ) );

/*
 * Lookup a skill by name.
 */
int skill_lookup( const char *name )
{
    int sn;

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

int find_spell( CHAR_DATA *ch, const char *name )
{
    /* finds a spell the character can cast if possible */
    int sn, found = -1;

    if (IS_NPC(ch))
        return skill_lookup(name);

    if (IS_AFFECTED(ch, AFF_INSANE)&&number_bits(1)==0)
    {
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if (skill_table[sn].name == NULL)
                break;

            if (number_bits(5)==0 && get_skill(ch,sn)>0)
            {
                if ( found == -1)
                    found = sn;
                if (get_skill(ch,sn)>0)
                    return sn;
            }
        }
    }
    else
    {
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if (skill_table[sn].name == NULL)
                break;
            if (LOWER(name[0]) == LOWER(skill_table[sn].name[0])
                    &&  !str_prefix(name,skill_table[sn].name))
            {
                if ( found == -1)
                    found = sn;
                if (get_skill(ch,sn)>0)
                    return sn;
            }
        }
    }

    return found;
}



/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup( int slot )
{
    extern bool fBootDb;
    int sn;

    if ( slot <= 0 )
        return -1;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( slot == skill_table[sn].slot )
            return sn;
    }

    if ( fBootDb )
    {
        bug( "Slot_lookup: bad slot %d.", slot );
        abort( );
    }

    return -1;
}



/*
 * Utter mystical words for an sn.
 */
void say_spell( CHAR_DATA *ch, int sn )
{
    char buf  [MAX_STRING_LENGTH];
    char buf2 [MAX_STRING_LENGTH];
    CHAR_DATA *rch;
    char *pName;
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
            act((!IS_NPC(rch) && ch->class==rch->class) ? buf : buf2,
                    ch, NULL, rch, TO_VICT );
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

    if ( carries_relic(ch) )
    {
        send_to_char( "Not with a relic!\n\r", ch );
        return FALSE;
    }

    return TRUE;
}
/* returns wether a skill is offensive
 */
bool is_offensive( int sn )
{
    int target = skill_table[sn].target;

    return target == TAR_CHAR_OFFENSIVE
        || target == TAR_OBJ_CHAR_OFF
        || target == TAR_VIS_CHAR_OFF;
}

int get_save(CHAR_DATA *ch)
{
    int saves = ch->saving_throw;
    int save_factor = 100;
    
    // level bonus
    if ( IS_NPC(ch) )
    {
        if ( IS_SET(ch->act, ACT_MAGE) )
            save_factor += 30;
        if ( IS_SET(ch->act, ACT_WARRIOR) )
            save_factor -= 30;
    }
    else
    {
        save_factor = 250
            - class_table[ch->class].attack_factor
            - class_table[ch->class].defense_factor/2;
    }
    saves -= (ch->level + 10) * save_factor/100;
    
    // WIS bonus
    saves -= (ch->level + 10) * get_curr_stat( ch, STAT_WIS ) / 500;

    return saves;
}

/* hard to make saving throw */
bool saves_spell_hard( int level, CHAR_DATA *victim, int dam_type )
{
    return saves_spell( level, victim, dam_type )
        && saves_spell( level, victim, dam_type );
}

/*
 * Compute a saving throw.
 * Negative applys make saving throw better.
 */
bool saves_spell( int level, CHAR_DATA *victim, int dam_type )
{
    int hit_roll, save_roll, save_factor;

    /* automatic saves/failures */

    switch(check_immune(victim,dam_type))
    {
        case IS_IMMUNE:     return TRUE;
        case IS_RESISTANT:  if ( chance(20) ) return TRUE;  break;
        case IS_VULNERABLE: if ( chance(10) ) return FALSE;  break;
    }

    if ( victim->fighting != NULL
            && victim->fighting->stance == STANCE_INQUISITION
            && chance(20) )
        return FALSE;

    if ( (victim->stance == STANCE_UNICORN)
            && chance(25) )
        return TRUE;

    if ( IS_AFFECTED(victim, AFF_PHASE)
            && chance(50) )
        return;

    if ( IS_AFFECTED(victim, AFF_PROTECT_MAGIC)
            && chance(20) )
        return TRUE;

    if ( IS_AFFECTED(victim, AFF_BERSERK )
            && chance(10) )
        return TRUE;

    /* now the resisted roll */
    save_roll = -get_save(victim);
    hit_roll = (level + 10) * 6/5;

    if ( save_roll <= 0 )
        return FALSE;
    else
        return number_range(0, hit_roll) <= number_range(0, save_roll);
}

/* RT save for dispels */

bool saves_dispel( int dis_level, int spell_level, int duration )
{
    int save;

    /* very hard to dispel permanent effects */
    if ( duration == -1 && number_bits(1) )
        return TRUE;

    save = 50 + (spell_level - dis_level) / 2;
    save = URANGE( 5, save, 95 );
    return number_percent( ) < save;
}

/* co-routine for dispel magic and cancellation */

bool check_dispel( int dis_level, CHAR_DATA *victim, int sn )
{
    AFFECT_DATA *af;
    char buf[MAX_STRING_LENGTH];

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
                if (!saves_dispel(dis_level,af->level,af->duration))
                {
                    affect_strip(victim,sn);
                    if ( skill_table[sn].msg_off )
                    {
                        send_to_char( skill_table[sn].msg_off, victim );
                        send_to_char( "\n\r", victim );
                        sprintf(buf, "The %s %s on $n vanishes.",
                                skill_table[sn].name,
                                IS_SPELL(sn) ? "spell" : "affect" );
                        act(buf,victim,NULL,NULL,TO_ROOM);
                    }
                    return TRUE;
                }
                else
                    af->level--;
            }
        }
    }
    return FALSE;
}

/* returns wether an affect can be dispelled or canceled 
 * called by spell_dispel_magic and spell_cancelation 
 */
bool can_dispel(int sn)
{
    if (!IS_SPELL(sn))
        return FALSE;

    /* some spells have special cures */
    if ( sn == gsn_curse || sn == gsn_tomb_rot
            || sn == gsn_poison
            || sn == gsn_plague || sn == gsn_necrosis
            || sn == gsn_blindness || sn == gsn_fire_breath )
        return FALSE;

    /*
       if ( is_mental(sn) )
       return FALSE;
     */

    return TRUE;
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
    int mana, min_level, max_level;

    mana = skill_table[sn].min_mana;
    mana = (200-skill)*mana/100;

    return mana;
}

/* returns wether a valid spell target was found */
bool get_spell_target( CHAR_DATA *ch, char *arg, int sn, /* input */
        int *target, CHAR_DATA **vo ) /* output */
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
            break;

        case TAR_VIS_CHAR_OFF:
            if (( arg[0] == '\0' )
                    && (ch->fighting != NULL)
                    && !can_see(ch, ch->fighting) )
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
                if ( ( victim = get_char_room( ch, arg ) ) == NULL )
                {
                    send_to_char( "They aren't here.\n\r", ch );
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

            if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
            {
                send_to_char( "You can't do that on your own follower.\n\r", ch );
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
            if ( arg[0] != '\0' && !is_name( arg, ch->name ) )
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
                        || (victim != NULL && !can_see(ch,victim)))
                {
                    send_to_char("Cast the spell on whom or what?\n\r",ch);
                    return FALSE;
                }

                *target = TARGET_CHAR;
            }
            else if ((victim = get_char_room(ch,arg)) != NULL)
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

                if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
                {
                    send_to_char( "You can't do that on your own follower.\n\r", ch );
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

    if (sn == gsn_solar_flare && weather_info.sky >= SKY_RAINING )
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

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char *target_name = NULL;
bool was_obj_cast = FALSE;

void do_cast( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    void *vo;
    int mana;
    int sn, level, chance;
    int target;


    /* Switched NPC's can cast spells, but others can't.  */
    /*
       if ( IS_NPC(ch) && ch->desc == NULL)
       return;
     */    

    target_name = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        send_to_char( "Cast which what where?\n\r", ch );
        return;
    }

    /*
       if ((sn == gsn_hailstorm || gsn_meteor_swarm || gsn_call_lightning ||
       gsn_control_weather || gsn_monsoon ) && (weather_info.sky < SKY_RAINING 
       && !IS_OUTSIDE(ch)) )
       {
       send_to_char( "You can't cast that right now.\n\r", ch );
       return;
       }

       if (sn == gsn_smotes_anachronism && ch->position != POS_FIGHTING)
       {
       send_to_char( "You can't cast that out of combat.\n\r", ch );
       return;
       }    */  

    if ((sn = find_spell(ch,arg1)) < 1
            ||  skill_table[sn].spell_fun == spell_null
            ||  (chance=get_skill(ch, sn))==0)
    {
        send_to_char( "You don't know any spells of that name.\n\r", ch );
        return;
    }

    /* check to see if spell is disabled */
    if (check_spell_disabled (&skill_table[sn]))
    {
        send_to_char ("This spell has been temporarily disabled.\n\r",ch);
        return;
    }

    /* check to see if spell can be cast in current position */
    if ( ch->position < skill_table[sn].minimum_position )
    {
        send_to_char( "You can't concentrate enough.\n\r", ch );
        return;
    }

    mana = mana_cost(ch, sn, chance);
    if (IS_AFFECTED(ch, AFF_OVERCHARGE))
        mana=mana*2;

    /* Locate targets */
    if ( !get_spell_target( ch, target_name, sn, &target, &vo ) )
        return;

    if ( /*!IS_NPC(ch) &&*/ ch->mana < mana )
    {
        send_to_char( "You don't have enough mana.\n\r", ch );
        return;
    }

    if ( str_cmp( skill_table[sn].name, "ventriloquate" ) )
    {
        say_spell( ch, sn );
    }

    /* Check for overcharge (less lag) */
    if (IS_AFFECTED(ch, AFF_OVERCHARGE))
    {
        WAIT_STATE( ch, (200-chance)*skill_table[sn].beats/400 );
    }
    else
    {
        WAIT_STATE( ch, (200-chance)*skill_table[sn].beats/100 );
    }


    /* mana burn */
    if ( IS_AFFECTED(ch, AFF_MANA_BURN) )
    {
        direct_damage( ch, ch, 2*mana, skill_lookup("mana burn") );
        if ( IS_DEAD(ch) )
            return;
    }
    else if ( IS_AFFECTED(ch, AFF_OVERCHARGE) && number_bits(1) == 0 )
    {
        direct_damage( ch, ch, mana, skill_lookup("mana burn") );
        if ( IS_DEAD(ch) )
            return;
    }

    if (is_affected(ch, gsn_choke_hold) && number_bits(3) == 0)
    {
        send_to_char( "You choke and your spell fumbles.\n\r", ch);
        ch->mana -= mana / 2;
#ifdef FSTAT 
        ch->mana_used += mana / 2;
#endif
    }
    else if (is_affected(ch, gsn_slash_throat) && number_bits(2) == 0)
    {
        send_to_char( "You can't speak and your spell fails.\n\r", ch);
        ch->mana -= mana / 2;
#ifdef FSTAT
        ch->mana_used += mana / 2;
#endif
    }
    else if ( 2*number_percent() > (chance+100)
            || IS_AFFECTED(ch, AFF_FEEBLEMIND) && per_chance(10)
            || IS_AFFECTED(ch, AFF_CURSE) && per_chance(5) )
    {
        send_to_char( "You lost your concentration.\n\r", ch );
        check_improve(ch,sn,FALSE,2);
        ch->mana -= mana / 2;
#ifdef FSTAT
        ch->mana_used += mana / 2;
#endif
    }

    else
    {
        ch->mana -= mana;
#ifdef FSTAT
        ch->mana_used += mana;
#endif

        if ( target == TARGET_OBJ )
        {
            if (!op_act_trigger( (OBJ_DATA *) vo, ch, NULL, skill_table[sn].name, OTRIG_SPELL) ) 
                return;
        }

        level = ch->level;
        if (!IS_NPC(ch))
            level -= (100-class_table[ch->class].mana_gain)*level/500;
        level = (100+chance)*level/200;
        level = URANGE(1, level, 120);

        vo = check_reflection( sn, level, ch, vo, target );

        /*
           victim = (CHAR_DATA*) vo;
           if ( is_offensive(sn)
           && target == TARGET_CHAR
           && victim->fighting == NULL
           && check_kill_trigger(ch, victim) )
           return;
         */

        (*skill_table[sn].spell_fun) (sn, level, ch, vo, target);
        check_improve(ch,sn,TRUE,3);

        /* check for spell mprog triggers */
        if ( target == TARGET_CHAR )
        {
            CHAR_DATA *vic = (CHAR_DATA *) vo;

            if ( vic != NULL && IS_NPC(vic) )
            {
                if (mp_spell_trigger( skill_table[sn].name, vic, ch ) )
                    return; //Return because it might have killed the vic or ch
            }
        }



    }

    victim = (CHAR_DATA*) vo;
    if ((skill_table[sn].target == TAR_CHAR_OFFENSIVE
                || (skill_table[sn].target == TAR_VIS_CHAR_OFF)
                ||   (skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))
            &&   victim != ch
            &&   victim->master != ch)
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        for ( vch = ch->in_room->people; vch; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( victim == vch 
                    && victim->fighting == NULL
                    && !is_safe_spell(victim, ch, FALSE) )
            {
                multi_hit( victim, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }

    return;
}



/*
 * Cast spells at targets using a magical object.
 */
bool obj_cast_spell( int sn, int level, CHAR_DATA *ch, OBJ_DATA *obj, char *arg )
{
    CHAR_DATA *victim;
    void *vo;
    int target;
    int levelmod;
    bool cast_self = FALSE;

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
        if ( levelmod = get_skill(ch, gsn_arcane_lore) )
            check_improve(ch, gsn_arcane_lore, TRUE, 3);
        levelmod += get_curr_stat(ch, STAT_WIS)/8;
        levelmod = UMIN( levelmod, 100 );
        level = level * (900 + levelmod) / 1000;
    }

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

    /* check for self-only spells */
    if ( !str_cmp(arg, "self") )
    {
        cast_self = TRUE;
        arg = ch->name;
    }

    /* get target */
    if ( !get_spell_target( ch, arg, sn, &target, &vo ) )
        return FALSE;

    if ( cast_self && vo != ch )
    {
        if ( target == TARGET_CHAR )
            vo = (void*) ch;
        else
            return FALSE;
    }

    /* execute spell */
    target_name = arg;
    vo = check_reflection( sn, level, ch, vo, target );

    /*
       victim = (CHAR_DATA*) vo;
       if ( is_offensive(sn)
       && target == TARGET_CHAR
       && victim->fighting == NULL
       && check_kill_trigger(ch, victim) )
       return;
     */

    was_obj_cast = TRUE;
    (*skill_table[sn].spell_fun) ( sn, level, ch, vo, target);
    was_obj_cast = FALSE;

    victim = (CHAR_DATA*) vo;
    if ( (skill_table[sn].target == TAR_CHAR_OFFENSIVE
                ||  skill_table[sn].target == TAR_VIS_CHAR_OFF
                ||   (skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))
            &&   victim != ch
            &&   victim->master != ch )
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        for ( vch = ch->in_room->people; vch; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( victim == vch && victim->fighting == NULL )
            {
                check_killer(victim,ch);
                multi_hit( victim, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }

    return TRUE;
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

/* needes to be seperate for dracs */
int adjust_spell_damage( int dam, CHAR_DATA *ch )
{
    OBJ_DATA *obj = get_eq_char(ch, WEAR_HOLD);
    //OBJ_DATA *weapon = get_eq_char(ch, WEAR_WIELD);
    bool has_focus = (obj != NULL && obj->item_type != ITEM_ARROWS);

    if ( was_obj_cast )
        return dam;

    if ( chance(get_skill(ch, gsn_focus))
            && (has_focus || number_bits(1)) )
    {
        dam += dam / 2;
        check_improve(ch, gsn_focus, TRUE, 1);
    }
    else
        check_improve(ch, gsn_focus, FALSE, 1);

    if ( !IS_NPC(ch) && ch->level >= LEVEL_MIN_HERO )
    {
        dam += dam * (10 + ch->level - LEVEL_MIN_HERO) / 100;
    }

    /* focus obj makes for more steady damage */
    if ( has_focus )
        return dam * number_range(100, 120) / 100;
    else
        return dam * number_range(80, 120) / 100;

}

int get_sn_damage( int sn, int level, CHAR_DATA *ch )
{
    int dam;

    if ( sn < 1 || sn >= MAX_SKILL )
        return 0;

    dam = get_spell_damage( skill_table[sn].min_mana, skill_table[sn].beats, level );

    return adjust_spell_damage(dam, ch);
}

int get_sn_heal( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim )
{
    int heal;

    if ( sn < 1 || sn >= MAX_SKILL )
        return 0;

    heal = get_spell_heal( skill_table[sn].min_mana, skill_table[sn].beats, level );

    if ( !was_obj_cast )
    {
        heal += heal * get_skill(ch, gsn_anatomy) / 200;
        check_improve(ch, gsn_anatomy, TRUE, 1);

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

    return heal;
}


/*
 * Spell functions.
 */
void spell_acid_blast( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = get_sn_damage( sn, level, ch );
    if ( saves_spell( level, victim, DAM_ACID ) )
        dam /= 2;

    full_dam( ch, victim, dam, sn,DAM_ACID,TRUE);
    return;
}

void spell_armor( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
            send_to_char("You are already armored.\n\r",ch);
        else
            act("$N is already armored.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    /* af.modifier  = -20; */
    /* Make spell better as your level increases -- Maedhros 09/11/2012 */
    af.modifier  = -20 - (level/4);
    af.location  = APPLY_AC;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel someone protecting you.\n\r", victim );
    if ( ch != victim )
        act("$N is protected by your magic.",ch,NULL,victim,TO_CHAR);
    return;
}




void spell_bless( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;
    int bonus;


    if (get_skill(ch, gsn_bless) >= 95 )
        bonus = (ch->level/15);
    else
        bonus = 0;

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;
        if (IS_OBJ_STAT(obj,ITEM_BLESS))
        {
            act("$p is already blessed.",ch,obj,NULL,TO_CHAR);
            return;
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
                return;
            }
            else
            {
                act("The evil of $p is too powerful for you to overcome.",
                        ch,obj,NULL,TO_CHAR);
                return;
            }
        }

        af.where    = TO_OBJECT;
        af.type     = sn;
        af.level    = level;
        af.duration = 6 + level;
        af.location = APPLY_SAVES;
        af.modifier = -1;
        af.bitvector    = ITEM_BLESS;
        affect_to_obj(obj,&af);

        act("$p glows with a holy aura.",ch,obj,NULL,TO_ALL);

        if (obj->wear_loc != WEAR_NONE)
            ch->saving_throw -= 1;
        return;
    }

    /* character target */
    victim = (CHAR_DATA *) vo;


    if ( is_affected( victim, sn ) || is_affected(victim, gsn_prayer) || is_affected(victim, gsn_blessed_darkness) )
    {
        if (victim == ch)
            send_to_char("You are already blessed.\n\r",ch);
        else
            act("$N is already blessed.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 6+level;
    af.location  = APPLY_HITROLL;
    af.modifier  = (level / 8) + bonus;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = 0 - level / 8;
    affect_to_char( victim, &af );
    send_to_char( "You feel righteous.\n\r", victim );
    if ( ch != victim )
        act("You grant $N the favor of your god.",ch,NULL,victim,TO_CHAR);
    return;
}



void spell_blindness( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_BLIND))
    {
        send_to_char( "Your target is already blind!\n\r", ch );
        return;
    }

    if ( saves_spell(level,victim,DAM_OTHER)
            || (number_percent() < 33 ) )
    {
        if ( victim != ch )
            act( "$N blinks $S eyes, and the spell has no effect.", ch, NULL, victim, TO_CHAR );
        send_to_char( "Your eyes begin to water, causing you to blink several times.\n\r", victim );
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_HITROLL;

    /* Increased power of spell (Now reduces Hitroll by up to 10
       points instead of only 5) - Astark Oct 2012
       af.modifier  = -4; */

    af.modifier  = -4 - number_range(0,6); 

    /* Max duration cut in half (33 to 16) - Astark Oct 2012
       af.duration  = number_range(level/10, level/3); */

    af.duration  = number_range(level/10, level/6);
    af.bitvector = AFF_BLIND;
    affect_to_char( victim, &af );
    send_to_char( "You are blinded!\n\r", victim );
    act("$n appears to be blinded.",victim,NULL,NULL,TO_ROOM);
    return;
}



void spell_burning_hands(int sn,int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( check_hit( ch, victim, sn, DAM_FIRE, 100 ) )
    {
        dam = get_sn_damage( sn, level, ch ) * 15/10;
        if ( saves_spell( level, victim, DAM_LIGHT) )
            dam /= 2;

        /* Level 90+ chars have a chance to blind their opponent
           with burning hands - Astark */
        if (level >= 90)
        {

            if (!IS_AFFECTED(victim,AFF_BLIND) &&  !saves_spell(level / 4 + dam / 20, victim,DAM_FIRE) &&  !number_bits(2))
            {
                AFFECT_DATA af;
                act("$n is blinded by smoke!",victim,NULL,NULL,TO_ROOM);
                act("Your eyes tear up from smoke...you can't see a thing!",
                        victim,NULL,NULL,TO_CHAR);

                af.where        = TO_AFFECTS;
                af.type         = gsn_burning_hands;
                af.level        = level;
                af.duration     = 0;
                af.location     = APPLY_HITROLL;
                af.modifier     = -2;
                af.bitvector    = AFF_BLIND;
            }
        }

    }
    else
        dam = 0;

    full_dam( ch, victim, dam, sn, DAM_FIRE,TRUE);
    return;
}

void spell_call_lightning( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    if ( !IS_OUTSIDE(ch) )
    {
        send_to_char( "You must be out of doors.\n\r", ch );
        return;
    }

    if ( weather_info.sky < SKY_RAINING )
    {
        send_to_char( "The weather is MUCH too nice for that!\n\r", ch );
        return;
    }

    dam = get_sn_damage( sn, level, ch ) * 3/4;

    send_to_char( "Lightning leaps out of the sky to strike your foes!\n\r", ch );
    act( "$n calls lightning from the sky to strike $s foes!", ch, NULL, NULL, TO_ROOM );

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;
        if ( is_safe_spell(ch, vch, TRUE) )
            continue;

        if ( saves_spell(level, vch, DAM_LIGHTNING) )
            full_dam( ch, vch, dam/2, sn,DAM_LIGHTNING,TRUE);
        else
            full_dam( ch, vch, dam, sn,DAM_LIGHTNING,TRUE);
    }

    return;
}

/* RT calm spell stops all fighting in the room */

void spell_calm( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *vch;
    int mlevel = 0;
    int count = 0;
    int high_level = 0;    
    int chance;
    AFFECT_DATA af;
    int bonus;
    char buf[MSL];
    char buf2[MSL];
    char buf3[MSL];

    if( IS_SET(ch->in_room->room_flags, ROOM_SAFE) )
    {
        send_to_char( "All is already calm in a safe room.\n\r", ch );
        return;
    }

    /* get sum of all char levels in the room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->position == POS_FIGHTING)
        {
            count++;
            if (IS_NPC(vch)) /* mobs count half their level */
                mlevel += vch->level/2;
            else
                mlevel += vch->level;
            high_level = UMAX(high_level,vch->level);
        }
    }

    /* compute chance of stopping combat */
    chance = 2 * level /*- high_level + 2 * count*/;

    if (IS_IMMORTAL(ch)) /* always works */
    {
        sprintf( buf, "%d = chance\n\r", chance );
        send_to_char(buf, ch);
    }

    if (IS_IMMORTAL(ch)) /* always works */
    {
        sprintf( buf2, "%d = mlevel\n\r", mlevel );
        send_to_char(buf2, ch);
    }


    if (IS_IMMORTAL(ch)) /* always works */
        mlevel = 0;

    /* restart counter */
    count = 0;
    if (number_range(0, chance) >= number_range(0, mlevel))  /* hard to stop large fights */
    {
        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if ( IS_NPC(vch) &&
                    (IS_SET(vch->imm_flags,IMM_MAGIC) || IS_SET(vch->act,ACT_UNDEAD)) )
                continue;

            if ( IS_AFFECTED(vch,AFF_CALM) || IS_AFFECTED(vch,AFF_BERSERK)
                    ||  is_affected(vch,skill_lookup("frenzy")) || is_safe(vch,ch) )
                continue;

            if (IS_IMMORTAL(ch)) /* always works */
                send_to_char("You're in the for loop\n\r",ch);

            if (IS_NPC(vch))
            {
                stop_hunting(ch);
                SET_BIT(ch->off_flags, OFF_DISTRACT);
            }

            count++;

            if (IS_IMMORTAL(ch)) /* always works */
            {
                sprintf( buf3, "%d = count\n\r", count );
                send_to_char(buf3, ch);
            }

            send_to_char("A wave of calm passes over you.\n\r",vch);
            act( "A wave of calm passes over $n.", vch, NULL, NULL, TO_ROOM );

            if (vch->fighting || vch->position == POS_FIGHTING)
                stop_fighting(vch,FALSE);

            af.where = TO_AFFECTS;
            af.type = sn;
            af.level = level;
            af.duration = level/4;
            af.location = APPLY_HITROLL;
            if (!IS_NPC(vch))
                af.modifier = -5;
            else
                af.modifier = -2;
            af.bitvector = AFF_CALM;
            affect_to_char(vch,&af);

            af.location = APPLY_DAMROLL;
            affect_to_char(vch,&af);
        }
    }
    if( count == 0 )
        send_to_char("Your environment continues to be hostile.\n\r",ch);
}

void spell_cancellation( int sn, int level, CHAR_DATA *ch, void *vo,int target )
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
            return;
        }

        if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOCANCEL) && ch != victim)
        {
            send_to_char("That player does not wish to be cancelled.\n\r",ch);
            return;
        }
    }

    /* some logic to see if we have an arg*/
    if ( target_name[0] != '\0' )
    {
        char junk[MIL];
        target_name=one_argument( target_name, junk); /* get rid of first arg */
    }

    /* unlike dispel magic, the victim gets NO save */

    /* we killed first arg (target), if there's more args then
       they're trying to cancel a certain spell*/
    if ( target_name[0] != '\0' )
    {
        int sn=skill_lookup( target_name );

        if ( sn == -1 )
        {
            send_to_char("Cancel which spell?\n\r",ch);
            return;
        }

        if (can_dispel(sn) && check_dispel(level,victim,sn))
            send_to_char( "Ok.\n\r", ch);
        else
            send_to_char( "Spell failed.\n\r", ch);
    } 
    else
    {
        /* begin running through the spells */

        for (sn = 1; skill_table[sn].name != NULL; sn++)
            if (can_dispel(sn) && check_dispel(level,victim,sn))
                found = TRUE;

        if (found)
            send_to_char("Ok.\n\r",ch);
        else
            send_to_char("Spell failed.\n\r",ch);
    }
}

void spell_cause_light( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int harm = get_sn_damage( sn, level, ch) / 2;

    harm = UMAX(UMIN(harm, victim->hit - victim->max_hit/16),0);
    victim->hit-=harm;
    remember_attack(victim, ch, harm);
    dam_message(ch,victim,harm,sn, FALSE);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );

    return;
}

void spell_cause_critical(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int harm = get_sn_damage( sn, level, ch) / 2;

    harm = UMAX(UMIN(harm, victim->hit - victim->max_hit/4),0);
    victim->hit-=harm;
    remember_attack(victim, ch, harm);
    dam_message(ch,victim,harm,sn, FALSE);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );

    return;
}

void spell_cause_serious(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int harm = get_sn_damage( sn, level, ch) / 2;

    harm = UMAX(UMIN(harm, victim->hit - victim->max_hit/8),0);
    victim->hit-=harm;
    remember_attack(victim, ch, harm);
    dam_message(ch,victim,harm,sn, FALSE);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );

    return;
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
    bool found = TRUE;
    int per = 100;

    dam = get_sn_damage( sn, level, ch ) / 3;
    while ( per > 0 )
    {
        int count = 0;
        CHAR_DATA *vch;

        curr_dam = dam * per/100;

        /* count the targets.. having only a few targets makes it difficult to chain */
        for( vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room )
            if( !is_safe_spell(ch,vch,TRUE) && vch != ch )
                count++;

        /* The fewer targets, the more the damage is reduced each chain */
        if( count < 2 )
            per -= 25;
        else if( count < 4 )
            per -= 15;
        else if( count < 6 )
            per -= 12;
        else if( count < 10 )
            per -= 10;
        else
            per -= 9;

        /* Modify this loss with respect to chain skill and focus */
        /* (Still even a 25% of each modification failing even at 100% skill) */
        if( number_range(25,125) < get_skill(ch,sn) )
            per += number_range(3,5);
        if( number_range(25,125) < get_skill(ch,gsn_focus) )
            per += number_range(1,3);

        if ( saves_spell(level, victim, dam_type) )
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
            curr_dam = dam * (per+10)/100;
            if ( saves_spell(level, victim, dam_type) )
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

void spell_chain_lightning(int sn,int level,CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    act("A lightning bolt leaps from $n's hand and arcs to $N.",
            ch,NULL,victim,TO_ROOM);
    act("A lightning bolt leaps from your hand and arcs to $N.",
            ch,NULL,victim,TO_CHAR);
    act("A lightning bolt leaps from $n's hand and hits you!",
            ch,NULL,victim,TO_VICT);  

    deal_chain_damage( sn, level, ch, victim, DAM_LIGHTNING );
}


void spell_change_sex( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_SET( ch->act, PLR_WAR ) && war.type == GENDER_WAR )
    {
        send_to_char( "It won't work... sides in the war are decided by a person's real sex.\n\r", ch );
        return;
    }

    if ( is_affected( victim, sn ))
    {
        if (victim == ch)
            send_to_char("You've already been changed.\n\r",ch);
        else
            act("$N has already had $s(?) sex changed.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if ( ch != victim && saves_spell(level, victim, DAM_OTHER) )
    {
        act("Hmmm... nope, $E's still a $E.",ch,NULL,victim,TO_CHAR );
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level;
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
    return;
}



void spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;


    CHAR_DATA *check;
    int charmed,max;
    bool sex_bonus;

    if ( is_safe(ch,victim) )
        return;

    if ( victim == ch )
    {
        send_to_char( "You like yourself even better!\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(victim, AFF_CHARM)
            || IS_AFFECTED(ch, AFF_CHARM)
            || IS_SET(victim->imm_flags, IMM_CHARM)
            || IS_IMMORTAL(victim) )
    {
        act( "You can't charm $N.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if (IS_SET(victim->in_room->room_flags,ROOM_LAW))
    {
        send_to_char("Charming is not allowed here.\n\r",ch);
        return;
    }

    if ( !IS_NPC(ch) && IS_SET( ch->act, PLR_WAR ) )
    {
        act( "Don't charm $M, KILL $M!", ch, NULL, victim, TO_CHAR );
        return;
    }

    /* Check number of charmees against cha */
    charmed=0;
    max = get_curr_stat(ch,STAT_CHA) / 10;

    for ( check=char_list ; check != NULL; check = check->next )
    {
        if (IS_NPC(check) && IS_AFFECTED(check,AFF_CHARM) && 
                check->master == ch)
        {
            charmed++;
            if (charmed >= max)
            {
                send_to_char("You are not charismatic enough to attract more followers.\n\r",ch);
                return;
            }
        }
    }

    sex_bonus =  
        (ch->sex == SEX_FEMALE && victim->sex == SEX_MALE)
        || (ch->sex == SEX_MALE && victim->sex == SEX_FEMALE);

    /* PCs are harder to charm */
    if ( saves_spell(level, victim, DAM_CHARM)
            || number_range(1, 200) > get_curr_stat(ch, STAT_CHA)
            || (!sex_bonus && number_bits(1) == 0)
            || (!IS_NPC(victim) && number_bits(2)) )
    {
        send_to_char("The spell has no effect.\n\r", ch );
        return;
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
    af.duration  = number_fuzzy( level / 4 );
    if ( !IS_NPC(victim) )
        af.duration /= 2;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( victim, &af );
    act( "Isn't $n just so nice?", ch, NULL, victim, TO_VICT );
    if ( ch != victim )
        act("$N looks at you with adoring eyes.",ch,NULL,victim,TO_CHAR);

    /* Added a check so that charmed NPCs aren't aggressive. Was getting errors in
       the shell saying "Bad MPKill ... - Astark 1-4-13 
       if (IS_NPC(victim))
       REMOVE_BIT(ch->act, ACT_AGGRESSIVE); */


    return;
}



void spell_chill_touch( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( check_hit(ch, victim, sn, DAM_COLD, 100) )
    {
        dam = get_sn_damage(sn, level, ch) * 14/10;
        if ( saves_spell( level, victim, DAM_COLD) )
            dam /= 2;
        cold_effect( victim, level, dam, TARGET_CHAR );
    }
    else
        dam = 0;

    full_dam( ch, victim, dam, sn, DAM_COLD,TRUE );
    return;
}



void spell_colour_spray( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = get_sn_damage(sn, level, ch) * 3/4;
    if ( saves_spell( level, victim, DAM_LIGHT) )
        dam /= 2;
    else 
        spell_blindness( gsn_blindness, level/2, ch,(void *) victim,TARGET_CHAR );

    full_dam( ch, victim, dam, sn, DAM_LIGHT,TRUE );
    return;
}



void spell_continual_light(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *light;

    if (target_name[0] != '\0')  /* do a glow on some object */
    {
        light = get_obj_carry(ch,target_name,ch);

        if (light == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            return;
        }

        if (IS_OBJ_STAT(light, ITEM_DARK))
        {
            REMOVE_BIT(light->extra_flags, ITEM_DARK);
            act("$p loses its dark aura.",ch,light,NULL,TO_ALL);
            return;
        }

        if (IS_OBJ_STAT(light,ITEM_GLOW))
        {
            act("$p is already glowing.",ch,light,NULL,TO_CHAR);
            return;
        }

        SET_BIT(light->extra_flags,ITEM_GLOW);
        act("$p glows with a white light.",ch,light,NULL,TO_ALL);
        return;
    }

    light = create_object( get_obj_index( OBJ_VNUM_LIGHT_BALL ), 0 );
    obj_to_room( light, ch->in_room );
    act( "$n twiddles $s thumbs and $p appears.",   ch, light, NULL, TO_ROOM );
    act( "You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR );
    return;
}



void spell_control_weather(int sn,int level,CHAR_DATA *ch,void *vo,int target) 
{
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
    return;
}

/* Explosives ala Rimbol.  Original idea from Wurm. */
/* Added if check to make sure a player had room in there inventory for the item
 *  * before casting it. Assholes were using this to core the mud 
 *   */
void spell_create_bomb( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *bomb;
    if ( ch->carry_number < can_carry_n(ch) )
    {
        bomb = create_object( get_obj_index( OBJ_VNUM_BOMB ), 0 );
        act( "$n has created $p.", ch, bomb, NULL, TO_ROOM );
        act( "You create $p.", ch, bomb, NULL, TO_CHAR );
        bomb->timer = -1;
        bomb->value[0] = 
            bomb->value[0] = UMAX(1, level/10) + 3;
        bomb->level = level;
        obj_to_char(bomb,ch);
    }
    else
        send_to_char("You have no room in your inventory for that!\r\n", ch);
    return;
}


void spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *mushroom;

    mushroom = create_object( get_obj_index( OBJ_VNUM_MUSHROOM ), 0 );
    mushroom->value[0] = level / 2;
    mushroom->value[1] = level;
    obj_to_room( mushroom, ch->in_room );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_ROOM );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_CHAR );
    return;
}

void spell_create_rose( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    OBJ_DATA *rose;
    rose = create_object(get_obj_index(OBJ_VNUM_ROSE), 0);
    if ( ch->carry_number < can_carry_n(ch) )
    {
        act("$n has created a beautiful red rose.",ch,rose,NULL,TO_ROOM);
        send_to_char("You create a beautiful red rose.\n\r",ch);
        obj_to_char(rose,ch);
    }
    else
        send_to_char("You have no room in your inventory for that!\n\r",ch);
    return;
}

void spell_create_spring(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *spring;

    spring = create_object( get_obj_index( OBJ_VNUM_SPRING ), 0 );
    spring->timer = level;
    obj_to_room( spring, ch->in_room );
    act( "$p flows from the ground.", ch, spring, NULL, TO_ROOM );
    act( "$p flows from the ground.", ch, spring, NULL, TO_CHAR );
    return;
}

void spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int water;

    if ( obj->item_type != ITEM_DRINK_CON )
    {
        send_to_char( "It is unable to hold water.\n\r", ch );
        return;
    }

    if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 )
    {
        send_to_char( "It contains some other liquid.\n\r", ch );
        return;
    }

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

    return;
}

void spell_cure_blindness(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA *aff;

    if ( !(IS_AFFECTED(victim, AFF_BLIND) ))
    {
        if (victim == ch)
            send_to_char("You aren't blind.\n\r",ch);
        else
            act("$N doesn't appear to be blinded.",ch,NULL,victim,TO_CHAR);
        return;
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
            return;
        }
    }

    /* blindness not added by an affect... */
    if ( ch == victim )
        send_to_char( "Your blindness cannot be cured.\n\r", ch );
    else
        act( "$N's blindness cannot be cured.", ch, NULL, victim, TO_CHAR );

    /*
       if (is_affected(victim, gsn_blindness))
       if (check_dispel(level,victim,gsn_blindness))
       {
       send_to_char( "You are no longer blinded!\n\r", victim );
       act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
       }
       else
       {
       send_to_char("Spell failed.\n\r",ch);
       return;
       }

       if (is_affected(victim, gsn_gouge))
       if (check_dispel(level, victim, gsn_gouge))
       {
       send_to_char( "Your eyes have been restored to their sockets!\n\r", victim );
       act("$n's empty eye sockets have been healed.",victim,NULL,NULL,TO_ROOM);
       affect_strip(ch, gsn_gouge);
       }
       else
       {
       send_to_char("Spell failed.\n\r",ch);
       return;
       }

       if (is_affected(victim, gsn_fire_breath))
       if (check_dispel(level, victim, gsn_fire_breath))
       {
       send_to_char( "The smoke is removed from your eyes!\n\r", victim );
       act("$n is no longer blinded by smoke.",victim,NULL,NULL,TO_ROOM);
       affect_strip(ch, gsn_fire_breath);
       }
       else
       {
       send_to_char("Spell failed.\n\r",ch);
       return;
       }
     */
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

void spell_cure_mental( int sn, int level, CHAR_DATA *ch,void *vo, int target )
{
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
}

/* RT added to cure plague */
void spell_cure_disease( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( !is_affected( victim, gsn_plague ) && !is_affected(victim, gsn_necrosis))
    {
        if (victim == ch)
            send_to_char("You aren't ill.\n\r",ch);
        else
            act("$N doesn't appear to be diseased.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if ( check_dispel(level,victim, gsn_plague) || 
            check_dispel(level, victim, gsn_necrosis) )
    {
        /*act("Your sores vanish.",victim,NULL,NULL,TO_CHAR);*/
        act("$n looks relieved as $s sores vanish.",victim,NULL,NULL,TO_ROOM);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
}


void spell_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( !is_affected( victim, gsn_poison ) )
    {
        if (victim == ch)
            send_to_char("You aren't poisoned.\n\r",ch);
        else
            act("$N doesn't appear to be poisoned.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (check_dispel(level,victim,gsn_poison))
    {
        send_to_char("A warm feeling runs through your body.\n\r",victim);
        act("$n looks much better.",victim,NULL,NULL,TO_ROOM);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
}


void spell_curse( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
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
            return;
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
                return;
            }
            else
            {
                act("The holy aura of $p is too powerful for you to overcome.",
                        ch,obj,NULL,TO_CHAR);
                return;
            }
        }

        af.where        = TO_OBJECT;
        af.type         = sn;
        af.level        = level;
        /* Max duration drastically reduced (4 to 20) - Astark Oct 2012
           af.duration     = 2 * level; */
        af.duration     = 4 + number_range(level/8, level/6);
        af.location     = APPLY_SAVES;
        af.modifier     = +1;
        af.bitvector    = ITEM_EVIL;
        affect_to_obj(obj,&af);

        act("$p glows with a malevolent aura.",ch,obj,NULL,TO_ALL);

        if (obj->wear_loc != WEAR_NONE)
            ch->saving_throw += 1;
        return;
    }

    /* character curses */
    victim = (CHAR_DATA *) vo;

    if ( is_safe( ch, victim ) )
        return;

    if (IS_AFFECTED(victim,AFF_CURSE))
    {
        send_to_char("They are already cursed.\n\r",ch);
        return;
    }

    if ( saves_spell(level, victim, DAM_NEGATIVE) )
    {
        act("$N feels a shiver, but resists your curse.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    /* Max duration drastically reduced (4 to 20) - Astark Oct 2012
       af.duration     = 2 * level; */
    af.duration     = 4 + number_range(level/8, level/6);
    af.location  = APPLY_HITROLL;
    af.modifier  = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = level / 8;
    affect_to_char( victim, &af );

    send_to_char( "You feel unclean.\n\r", victim );
    if ( ch != victim )
        act("$N looks very uncomfortable.",ch,NULL,victim,TO_CHAR);
    return;
}

/* RT replacement demonfire spell */
void spell_demonfire(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
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

    if ( saves_spell(level, victim, DAM_NEGATIVE) )
        dam /= 2;

    if ( IS_GOOD(victim) && !IS_AFFECTED(victim, AFF_CURSE) )
        spell_curse(gsn_curse, level/2, ch, (void *) victim,TARGET_CHAR);
    full_dam( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
}

void spell_angel_smite(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam, align;

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
    if ( saves_spell( level, victim, DAM_HOLY) )
        dam /= 2;

    if ( IS_EVIL(victim) && !IS_AFFECTED(victim, AFF_CURSE) )
        spell_curse(gsn_curse, level/2, ch, (void *) victim,TARGET_CHAR);
    full_dam( ch, victim, dam, sn, DAM_HOLY ,TRUE);
}


void spell_detect_evil( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_EVIL) )
    {
        if (victim == ch)
            send_to_char("You can already sense evil.\n\r",ch);
        else
            act("$N can already detect evil.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}


void spell_detect_good( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_GOOD) )
    {
        if (victim == ch)
            send_to_char("You can already sense good.\n\r",ch);
        else
            act("$N can already detect good.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_GOOD;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_hidden(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_HIDDEN) )
    {
        if (victim == ch)
            send_to_char("You are already as alert as you can be. \n\r",ch);
        else
            act("$N can already sense hidden lifeforms.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char( victim, &af );
    send_to_char( "Your awareness improves.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_invis( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_INVIS) )
    {
        if (victim == ch)
            send_to_char("You can already see invisible.\n\r",ch);
        else
            act("$N can already see invisible things.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_magic( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_MAGIC) )
    {
        if (victim == ch)
            send_to_char("You can already sense magical auras.\n\r",ch);
        else
            act("$N can already detect magic.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
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

    return;
}

void spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    char *god_name;

    /* Neutral victims are not affected. */
    if ( IS_NEUTRAL(victim) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    /* Good victims are not affected either, and are protected by their god if they have one. */
    if ( IS_GOOD(victim) )
    {
        if( (god_name = get_god_name(victim)) == NULL )
            god_name = "Rimbol";

        sprintf(log_buf, "%s protects $N.", god_name);
        act( log_buf, ch, NULL, victim, TO_CHAR );
        return;
    }

    dam = get_sn_damage( sn, level, ch );
    dam += dam * (ch->alignment - victim->alignment) / 4000;
    if ( saves_spell( level, victim,DAM_HOLY) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_HOLY, TRUE);
    return;
}


void spell_dispel_good( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    char *god_name;

    /* Neutral victims are not affected. */
    if ( IS_NEUTRAL(victim) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return;
    }

    /* Evil victims are not affected either, and are protected by their god if they have one. */
    if ( IS_EVIL(victim) )
    {
        if( (god_name = get_god_name(victim)) == NULL )
            god_name = "Rimbol";

        sprintf(log_buf, "%s protects $N.", god_name);
        act( log_buf, ch, NULL, victim, TO_CHAR );
        return;
    }

    dam = get_sn_damage( sn, level, ch );
    dam += dam * (victim->alignment - ch->alignment) / 4000;
    if ( saves_spell( level, victim, DAM_NEGATIVE) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
    return;
}


/* modified for enhanced use */

void spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;


    if (saves_spell(level, victim,DAM_OTHER))
    {     
        send_to_char( "You feel a brief tingling sensation.\n\r",victim);
        send_to_char( "You failed.\n\r", ch);
        return;
    }

    /* begin running through the spells */ 

    for (sn = 1; skill_table[sn].name != NULL; sn++)
        if (can_dispel(sn) && check_dispel(level,victim,sn))
            found = TRUE;

    if (found)
        send_to_char("Ok.\n\r",ch);
    else
        send_to_char("Spell failed.\n\r",ch);
    return;
}

void spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    send_to_char( "The earth trembles beneath your feet!\n\r", ch );
    act( "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );

    dam = get_sn_damage( sn, level, ch ) / 3;

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next    = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ( vch->in_room == ch->in_room )
        {
            if ( !is_safe_spell(ch,vch,TRUE))
                if (IS_AFFECTED(vch,AFF_FLYING))
                    continue;
                else
                    full_dam( ch,vch, dam, sn, DAM_BASH,TRUE);
            continue;
        }

        if ( vch->in_room->area == ch->in_room->area )
            send_to_char( "The earth trembles and shivers.\n\r", vch );
    }

    return;
}

void spell_enchant_arrow( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    char *arg;
    int nr, mana;
    int type;

    if (obj == NULL || obj->item_type != ITEM_ARROWS)
    {
        send_to_char( "That isn't a pack of arrows.\n\r", ch);
        return;
    }

    if ( (nr = obj->value[0]) <= 0 )
    {
        bugf( "spell_enchant_arrow: %d arrows", nr );
        extract_obj( obj );
        send_to_char( "There are no arrows to enchant.\n\r", ch );
        return;
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
        return;
    }

    /* no re-enchanting */
    if ( obj->value[1] > 0 )
    {
        send_to_char( "The arrows are already enchanted.\n\r", ch );
        return;
    }

    /* extra mana cost based on nr of arrows */
    mana = UMIN(nr, ch->mana);
    ch->mana -= mana;
    if ( number_range(1, nr) > mana )
    {
        send_to_char( "Your power is too low. The spell failed.\n\r", ch );
        return;
    }

    /* avoid trouble */
    if ( level > ch->level )
        level = ch->level;

    obj->level = level;
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
}

void spell_enchant_armor( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int result;
    //int mana;
    //int skill;

    if (obj->item_type != ITEM_ARMOR)
    {
        send_to_char("That isn't an armor.\n\r",ch);
        //mana = (200-skill)*mana/1000;
        //ch->mana += mana;
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The item must be carried to be enchanted.\n\r",ch);
        return;
    }

    result = get_enchant_ops( obj, level );

    /* the moment of truth */
    if ( result < -1 )  /* item destroyed */
    {
        act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_CHAR);
        act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
        return;
    }

    if ( result == -1 ) /* item disenchanted */
    {
        AFFECT_DATA *paf, *paf_next;

        act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);

        /* remove all affects */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next; 
            free_affect(paf);
        }
        obj->affected = NULL;

        /* clear magic flags */
        REMOVE_BIT(obj->extra_flags, ITEM_MAGIC);
        return;
    }

    if ( result == 0 )  /* failed, no bad result */
    {
        send_to_char("Nothing seemed to happen.\n\r",ch);
        return;
    }

    if ( result == 1 )  /* success! */
    {
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR);
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM);
    }
    else  /* exceptional enchant */
    {
        act("$p glows a brilliant gold!",ch,obj,NULL,TO_CHAR);
        act("$p glows a brilliant gold!",ch,obj,NULL,TO_ROOM);
    }

    /* now add the enchantments */ 
    SET_BIT( obj->extra_flags, ITEM_MAGIC );
    enchant_obj( obj, result );
}

void spell_enchant_weapon( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int result;

    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("That isn't a weapon.\n\r",ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The item must be carried to be enchanted.\n\r",ch);
        return;
    }

    result = get_enchant_ops( obj, level );

    /* the moment of truth */
    if ( result < -1 )  /* item destroyed */
    {
        act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_CHAR);
        act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
        return;
    }

    if ( result == -1 ) /* item disenchanted */
    {
        AFFECT_DATA *paf, *paf_next;

        act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);

        /* remove all affects */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next; 
            free_affect(paf);
        }
        obj->affected = NULL;

        /* clear magic flags */
        REMOVE_BIT(obj->extra_flags, ITEM_MAGIC);
        return;
    }

    if ( result == 0 )  /* failed, no bad result */
    {
        send_to_char("Nothing seemed to happen.\n\r",ch);
        return;
    }

    if ( result == 1 )  /* success! */
    {
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR);
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM);
    }
    else  /* exceptional enchant */
    {
        act("$p glows a brilliant gold!",ch,obj,NULL,TO_CHAR);
        act("$p glows a brilliant gold!",ch,obj,NULL,TO_ROOM);
    }

    /* now add the enchantments */ 
    SET_BIT( obj->extra_flags, ITEM_MAGIC );
    enchant_obj( obj, result );
}

/*
 * Drain mana and move and add percentage to caster.
 */
void spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int drain, drain_mana, drain_move;

    if ( saves_spell( level, victim, DAM_NEGATIVE) )
    {
        act( "$N shivers slightly as the spell passes harmlessly over $S body.",
                ch, NULL, victim, TO_CHAR );
        send_to_char("You feel a momentary chill.\n\r",victim);     
        return;
    }

    drop_align( ch );

    drain = dice( level, 9 );
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

    return;
}



void spell_fireball( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    act("A ball of fire explodes from $n's hands!",ch,NULL,NULL,TO_ROOM);
    act("A ball of fire explodes from your hands.",ch,NULL,NULL,TO_CHAR);

    dam = get_sn_damage( sn, level, ch ) / 2;

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if ( is_safe_spell(ch,vch,TRUE) )
            continue;

        if (saves_spell(level,vch,DAM_FIRE))
            full_dam(ch,vch,dam/2,sn,DAM_FIRE,TRUE);
        else
            full_dam(ch,vch,dam,sn,DAM_FIRE,TRUE);
    }
}


void spell_fireproof(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;

    if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
    {
        act("$p is already protected from burning.",ch,obj,NULL,TO_CHAR);
        return;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = ITEM_BURN_PROOF;

    affect_to_obj(obj,&af);

    act("You protect $p from fire.",ch,obj,NULL,TO_CHAR);
    act("$p is surrounded by a protective aura.",ch,obj,NULL,TO_ROOM);
}

void spell_flamestrike( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = get_sn_damage( sn, level, ch );
    if ( saves_spell( level, victim,DAM_FIRE) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_FIRE ,TRUE);
    return;
}

void spell_faerie_fire( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) )
    {
        act( "$N is already affected by faerie fire.", ch, NULL, victim, TO_CHAR );
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    af.modifier  = 2 * level;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char( victim, &af );
    send_to_char( "You are surrounded by a pink outline.\n\r", victim );
    act( "$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM );
    return;
}



void spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *ich;
    OBJ_DATA *obj;

    act( "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
    send_to_char( "You conjure a cloud of purple smoke.\n\r", ch );

    for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
    {
        if (ich->invis_level > 0)
            continue;

        if ( ich == ch || saves_spell( level, ich, DAM_OTHER) )
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

    return;
}

void spell_floating_disc( int sn, int level,CHAR_DATA *ch,void *vo,int target )
{
    OBJ_DATA *disc, *floating;

    floating = get_eq_char(ch,WEAR_FLOAT);
    if (floating != NULL && IS_OBJ_STAT(floating,ITEM_NOREMOVE))
    {
        act("You can't remove $p.",ch,floating,NULL,TO_CHAR);
        return;
    }

    disc = create_object(get_obj_index(OBJ_VNUM_DISC), 0);
    disc->value[0]  = ch->level * 10; /* 10 pounds per level capacity */
    disc->value[3]  = ch->level * 5; /* 5 pounds per level max per item */
    disc->timer     = ch->level * 2 - number_range(0,level / 2); 

    act("$n has created a floating black disc.",ch,NULL,NULL,TO_ROOM);
    send_to_char("You create a floating disc.\n\r",ch);
    obj_to_char(disc,ch);
    wear_obj(ch,disc,TRUE);
    return;
}


void spell_fly( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_FLYING) )
    {
        if (victim == ch)
            send_to_char("You are already airborne.\n\r",ch);
        else
            act("$N doesn't need your help to fly.",ch,NULL,victim,TO_CHAR);
        return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level + 3;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char( victim, &af );
    send_to_char( "Your feet rise off the ground.\n\r", victim );
    act( "$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM );
    return;
}

/* RT clerical berserking spell */

void spell_frenzy(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim,sn) || IS_AFFECTED(victim,AFF_BERSERK))
    {
        if (victim == ch)
            send_to_char("You are already in a frenzy.\n\r",ch);
        else
            act("$N is already in a frenzy.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (is_affected(victim,skill_lookup("calm")))
    {
        if (victim == ch)
            send_to_char("Why don't you just relax for a while?\n\r",ch);
        else
            act("$N doesn't look like $e wants to fight anymore.",
                    ch,NULL,victim,TO_CHAR);
        return;
    }

    if ((IS_GOOD(ch) && !IS_GOOD(victim)) ||
            (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
            (IS_EVIL(ch) && !IS_EVIL(victim))
       )
    {
        act("Your god doesn't seem to like $N.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 3;
    af.modifier  = level / 6;
    af.bitvector = AFF_BERSERK;

    af.location  = APPLY_HITROLL;
    affect_to_char(victim,&af);

    af.location  = APPLY_DAMROLL;
    affect_to_char(victim,&af);

    af.modifier  = 10 * (level / 12);
    af.location  = APPLY_AC;
    affect_to_char(victim,&af);

    send_to_char("You are filled with holy wrath!\n\r",victim);
    act("$n gets a wild look in $s eyes!",victim,NULL,NULL,TO_ROOM);
}

/* RT ROM-style gate */
void spell_gate( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    bool gate_pet;
    ROOM_INDEX_DATA *to_room;
    OBJ_DATA *stone;
    bool has_warpstone=FALSE;

    if ( !can_cast_transport(ch) )
        return;

    ignore_invisible = TRUE;
    if ( ( victim = get_char_world( ch, target_name ) ) == NULL 
            || IS_TAG(ch) || IS_TAG(victim)
            || victim->in_room == NULL
            || !can_see_room(ch,victim->in_room) 
            || victim->in_room->area->security < 5
            || (!IS_NPC(victim) && victim->level >= LEVEL_HERO) ) /*not trust*/
    {
        send_to_char( "You failed completely.\n\r", ch );
        return;
    }
    ignore_invisible = FALSE;

    if ( IS_SET(victim->in_room->room_flags, ROOM_SAFE)
            ||   is_guild_room(victim->in_room)
            ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
            ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
            ||   IS_SET(victim->in_room->room_flags, ROOM_JAIL)
            ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL) )
    {
        send_to_char( "Your powers can't get you there.\n\r", ch );
        return;
    }

    if ( victim->level > level + 5 )
    {
        send_to_char( "Your powers aren't strong enough.\n\r", ch );
        return;
    }

    if ( (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
            ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON)) )
    {
        send_to_char( "Your target refuses your company.\n\r", ch );
        return;
    }

    /*
       if (IS_NPC(victim) && saves_spell( level, victim,DAM_OTHER) )
       {
       send_to_char( "You failed.\n\r", ch );
       return;
       }
     */

    if ( ch->in_room == victim->in_room )
    {
        send_to_char( "You're already there!\n\r", ch );
        return;
    }

    check_sn_multiplay( ch, victim, sn );

    if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
        gate_pet = TRUE;
    else
        gate_pet = FALSE;


    stone = get_eq_char(ch,WEAR_HOLD);

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR);
        has_warpstone = TRUE;
    }

    act("$n steps through a gate and vanishes.",ch,NULL,NULL,TO_ROOM);
    send_to_char("You step through a gate and vanish.\n\r",ch);
    char_from_room(ch);

    if ( !has_warpstone && chance(5) )
    {
        to_room = get_random_room(ch);
        if ( to_room == NULL )
            to_room = victim->in_room;
    }
    else
    {
        to_room = victim->in_room;
        if( has_warpstone && chance(15) )
        {
            printf_to_char(ch,"%s flares brightly and vanishes!\n\r", stone->short_descr);
            extract_obj(stone);
        }
    }

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
}



void spell_giant_strength(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
            send_to_char("You are already as strong as you can get!\n\r",ch);
        else
            act("$N can't get any stronger.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim,AFF_WEAKEN))
    {
        if (!check_dispel(level,victim,skill_lookup("weaken")))
        {
            if (victim != ch)
		        act( "Spell failed to grant $N with giant strength.\n\r", ch, NULL, victim, TO_CHAR );
            send_to_char("You feel momentarily stronger.\n\r",victim);
            return;
        }
        act("$n's muscles return to normal.",victim,NULL,NULL,TO_ROOM);
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_STR;
    af.modifier  = 1+level/5;
    af.bitvector = AFF_GIANT_STRENGTH;
    affect_to_char( victim, &af );
    send_to_char( "Your muscles surge with heightened power!\n\r", victim );
    act("$n's muscles surge with heightened power.",victim,NULL,NULL,TO_ROOM);
    if (ch != victim)
        send_to_char( "Ok.\n\r", ch);
    return;
}



void spell_harm( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int harm = get_sn_damage(sn, level, ch) / 2;

    harm = UMAX(UMIN(harm, victim->hit - victim->max_hit/2),0);
    victim->hit-=harm;
    remember_attack(victim, ch, harm);
    dam_message(ch,victim,harm,sn, FALSE);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );

    return;
}

/* RT haste spell */
void spell_haste( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_HASTE) )
    {
        if (victim == ch)
            send_to_char("You can't move any faster!\n\r",ch);
        else
            act("$N is already moving as fast as $E can.",
                    ch,NULL,victim,TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim,AFF_SLOW))
    {
        if (!check_dispel(level,victim,skill_lookup("slow")))
        {
            if (victim != ch)
                send_to_char("Your haste spell failed.\n\r",ch);
            send_to_char("You feel momentarily faster.\n\r",victim);
            return;
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
            return;
        }
        act("With haste, $n escapes $s entaglement!",victim,NULL,NULL,TO_ROOM);
        //return;                       
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    if (victim == ch)
        af.duration  = level/2;
    else
        af.duration  = level/4;
    af.location  = APPLY_AGI;
    af.modifier  = 1 + level/5;
    af.bitvector = AFF_HASTE;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself moving more quickly.\n\r", victim );
    act("$n is moving more quickly.",victim,NULL,NULL,TO_ROOM);
    //printf_to_char(ch, "gsn: %d sn: %d", gsn_haste, sn);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}

void spell_heal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal = get_sn_heal( sn, level, ch, victim );

    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    /*send_to_char( "A warm feeling fills your body.\n\r", victim );
      if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
      +       printf_to_char( ch, "You heal %s.", PERS(victim) );
      +       if (victim->hit >= victim->max_hit)
      +          printf_to_char(ch, "%s is at full health.",PERS(victim));
      +*/
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
    return;
}

void spell_heat_metal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj_lose, *obj_next;
    int dam = 0;
    bool fail = TRUE;

    if ( IS_SET( ch->act, PLR_WAR ) )
    {
        send_to_char( "Play nice!\n\r", ch );
        return;
    }

    if (!saves_spell(level, victim, DAM_FIRE) 
            &&  !IS_SET(victim->imm_flags,IMM_FIRE))
    {
        for ( obj_lose = victim->carrying;
                obj_lose != NULL; 
                obj_lose = obj_next)
        {
            obj_next = obj_lose->next_content;
            if ( number_range(1,2 * level) > obj_lose->level 
                    &&   !saves_spell(level,victim,DAM_FIRE)
                    &&   !IS_OBJ_STAT(obj_lose,ITEM_NONMETAL)
                    &&   !IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF)
                    &&   !IS_OBJ_STAT(obj_lose,ITEM_STICKY))
            {
                switch ( obj_lose->item_type )
                {
                    case ITEM_ARMOR:
                        if (obj_lose->wear_loc != -1) /* remove the item */
                        {
                            if (can_drop_obj(victim,obj_lose)
                                    &&  (obj_lose->weight / 10) < 
                                    number_range(1,get_curr_stat(victim,STAT_DEX)/2)
                                    &&  remove_obj( victim, obj_lose->wear_loc, TRUE ))
                            {
                                act("$n yelps and throws $p to the ground!",
                                        victim,obj_lose,NULL,TO_ROOM);
                                act("You remove and drop $p before it burns you.",
                                        victim,obj_lose,NULL,TO_CHAR);
                                dam += (number_range(1,obj_lose->level) / 3);
                                obj_from_char(obj_lose);
                                obj_to_room(obj_lose, victim->in_room);
                                fail = FALSE;
                            }
                            else /* stuck on the body! ouch! */
                            {
                                act("Your skin is seared by $p!",
                                        victim,obj_lose,NULL,TO_CHAR);
                                dam += (number_range(1,obj_lose->level));
                                fail = FALSE;
                            }

                        }
                        else /* drop it if we can */
                        {
                            if (can_drop_obj(victim,obj_lose))
                            {
                                act("$n yelps and throws $p to the ground!",
                                        victim,obj_lose,NULL,TO_ROOM);
                                act("You and drop $p before it burns you.",
                                        victim,obj_lose,NULL,TO_CHAR);
                                dam += (number_range(1,obj_lose->level) / 6);
                                obj_from_char(obj_lose);
                                obj_to_room(obj_lose, victim->in_room);
                                fail = FALSE;
                            }
                            else /* cannot drop */
                            {
                                act("Your skin is seared by $p!",
                                        victim,obj_lose,NULL,TO_CHAR);
                                dam += (number_range(1,obj_lose->level) / 2);
                                fail = FALSE;
                            }
                        }
                        break;
                    case ITEM_WEAPON:
                        if (obj_lose->wear_loc != -1) /* try to drop it */
                        {
                            if (IS_WEAPON_STAT(obj_lose,WEAPON_FLAMING))
                                continue;

                            if (can_drop_obj(victim,obj_lose) 
                                    &&  remove_obj(victim,obj_lose->wear_loc,TRUE))
                            {
                                act("$n is burned by $p, and throws it to the ground.",
                                        victim,obj_lose,NULL,TO_ROOM);
                                send_to_char(
                                        "You throw your red-hot weapon to the ground!\n\r",
                                        victim);
                                dam += 1;
                                obj_from_char(obj_lose);
                                obj_to_room(obj_lose,victim->in_room);
                                fail = FALSE;
                            }
                            else /* YOWCH! */
                            {
                                send_to_char("Your weapon sears your flesh!\n\r",
                                        victim);
                                dam += number_range(1,obj_lose->level);
                                fail = FALSE;
                            }
                        }
                        else /* drop it if we can */
                        {
                            if (can_drop_obj(victim,obj_lose))
                            {
                                act("$n throws a burning hot $p to the ground!",
                                        victim,obj_lose,NULL,TO_ROOM);
                                act("You and drop $p before it burns you.",
                                        victim,obj_lose,NULL,TO_CHAR);
                                dam += (number_range(1,obj_lose->level) / 6);
                                obj_from_char(obj_lose);
                                obj_to_room(obj_lose, victim->in_room);
                                fail = FALSE;
                            }
                            else /* cannot drop */
                            {
                                act("Your skin is seared by $p!",
                                        victim,obj_lose,NULL,TO_CHAR);
                                dam += (number_range(1,obj_lose->level) / 2);
                                fail = FALSE;
                            }
                        }
                        break;
                }
            }
        }
    } 
    if (fail)
    {
        send_to_char("Your spell had no effect.\n\r", ch);
        send_to_char("You feel momentarily warmer.\n\r",victim);
    }
    else /* damage! */
    {
        if (saves_spell(level,victim,DAM_FIRE))
            dam = 2 * dam / 3;
        full_dam(ch,victim,dam,sn,DAM_FIRE,TRUE);
    }
}

/* RT really nasty high-level attack spell */
void spell_holy_word(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam, dam_type, curr_dam;
    int bless_num, curse_num, frenzy_num;
    bool same_align;

    act("$n utters a word of divine power!",ch,NULL,NULL,TO_ROOM);
    send_to_char("You utter a word of divine power.\n\r",ch);

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
        return;
    }

    bless_num = skill_lookup("bless");
    curse_num = skill_lookup("curse");
    frenzy_num = skill_lookup("frenzy");

    dam = get_sn_damage(sn, level, ch) / 3;

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
                spell_sanctuary(gsn_sanctuary,level,ch,(void *) vch,TARGET_CHAR);
            if ( !is_affected(vch, gsn_prayer) && !is_affected(vch, gsn_bless) )
                spell_bless(bless_num,level,ch,(void *) vch,TARGET_CHAR);
            if ( same_align && !IS_AFFECTED(vch, AFF_BERSERK) )
                spell_frenzy(frenzy_num,level,ch,(void *) vch,TARGET_CHAR);
        }

        else if ( vch->fighting != NULL
                && is_same_group(ch, vch->fighting)
                && !is_safe_spell(ch,vch,TRUE)
                && !same_align )
        {
            spell_curse(curse_num,level,ch,(void *) vch,TARGET_CHAR);
            send_to_char("You are struck down!\n\r",vch);

            curr_dam = dam;
            if ( IS_UNDEAD(vch) )
                curr_dam = curr_dam * 3/2;
            if ( saves_spell( level, vch, dam_type ) )
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
}

void spell_identify( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;

    if ( (ch->level+10) < obj->level)
    {
        sprintf( buf, "%s is too powerful for you to identify.\n\r" , obj->name);
        send_to_char(buf, ch);
        return;
    }

    sprintf( buf,
            "Object '%s' is type %s, extra flags %s.\n\rWeight is %d, level is %d.\n\r",
            obj->name,
            item_name( obj->item_type ),        
            extra_bits_name( obj->extra_flags ),
            obj->weight / 10,
            obj->level
           );
    send_to_char( buf, ch );

    if ( CAN_WEAR(obj, ITEM_TRANSLUCENT) )
    {
        send_to_char( "It is translucent so tattoos will shine through.\n\r", ch );
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
            sprintf( buf, "Has %d charges of level %d",
                    obj->value[2], obj->value[0] );
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
            if (obj->pIndexData->new_format)
                sprintf(buf,"It does %s damage of %dd%d (average %d).\n\r",
                        attack_table[obj->value[3]].noun,
                        obj->value[1],obj->value[2],
                        (1 + obj->value[2]) * obj->value[1] / 2);
            else
                sprintf( buf, "Damage is %d to %d (average %d).\n\r",
                        obj->value[1], obj->value[2],
                        ( obj->value[1] + obj->value[2] ) / 2 );
            send_to_char( buf, ch );
            if (obj->value[4])  /* weapon flags */
            {
                sprintf(buf,"Weapons flags: %s\n\r",weapon_bits_name(obj->value[4]));
                send_to_char(buf,ch);
            }
            break;

        case ITEM_ARMOR:
            sprintf( buf, 
                    "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r", 
                    obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
            send_to_char( buf, ch );
            break;

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
}

void spell_infravision( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_INFRARED) )
    {
        if (victim == ch)
            send_to_char("You can already see in the dark.\n\r",ch);
        else
            act("$N can already see in the dark.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INFRARED;
    affect_to_char( victim, &af );

    send_to_char( "Your eyes glow red.\n\r", victim );
    act( "$n's eyes glow red.", victim, NULL, NULL, TO_ROOM );
    return;
}



void spell_invis( int sn, int level, CHAR_DATA *ch, void *vo,int target )
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
        obj = (OBJ_DATA *) vo;  

        if (IS_OBJ_STAT(obj,ITEM_INVIS))
        {
            act("$p is already invisible.",ch,obj,NULL,TO_CHAR);
            return;
        }

        af.where    = TO_OBJECT;
        af.type     = sn;
        af.level    = level;
        af.duration = level + 12;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector    = ITEM_INVIS;
        affect_to_obj(obj,&af);

        act("$p fades out of sight.",ch,obj,NULL,TO_ALL);
        return;
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
        return;
    }

    if (IS_TAG(ch))
    {
        send_to_char("There is no place to hide in freeze tag.\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(victim, AFF_INVISIBLE) )
    {
        if ( victim == ch )
            send_to_char( "You are already invisible.\n\r", ch );
        else
            act( "$N is already invisible.", ch, NULL, victim, TO_CHAR );
        return;
    }    

    if ( IS_AFFECTED(victim, AFF_ASTRAL) )
    {
        send_to_char("All is visible on the Astral plane.\n\r",ch);
        return;
    }

    act( "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level + 12;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char( victim, &af );
    send_to_char( "You fade out of existence.\n\r", victim );
    return;
}



void spell_know_alignment(int sn,int level,CHAR_DATA *ch,void *vo,int target )
{
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
    return;
}

void spell_lightning_bolt(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = get_sn_damage( sn, level, ch );
    if ( saves_spell( level, victim,DAM_LIGHTNING) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_LIGHTNING ,TRUE);
    return;
}

void spell_locate_object( int sn, int level, CHAR_DATA *ch, void *vo,int target)
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
        return;
    }

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

    return;
}



void spell_magic_missile( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
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
        if ( saves_spell(level, victim, DAM_ENERGY) )
            dam /= 2;
        full_dam( ch, victim, dam, sn, DAM_ENERGY ,TRUE);
        CHECK_RETURN(ch, victim);
    }
    return;
}

void spell_mass_healing(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *gch;
    int heal_num, refresh_num;

    heal_num = skill_lookup("heal");
    refresh_num = skill_lookup("refresh"); 

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ((IS_NPC(ch) && IS_NPC(gch)) ||
                (!IS_NPC(ch) && !IS_NPC(gch)))
        {
            spell_heal(heal_num,level,ch,(void *) gch,TARGET_CHAR);
            spell_refresh(refresh_num,level,ch,(void *) gch,TARGET_CHAR);  
            check_sn_multiplay( ch, gch, sn );
        }
    }
}


void spell_mass_invis( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
    CHAR_DATA *gch;

    if (IS_NOHIDE(ch))
    {
        send_to_char("There is no place to hide.\n\r",ch);
        return;
    }

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
        af.duration  = 24;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_INVISIBLE;
        affect_to_char( gch, &af );

        check_sn_multiplay( ch, gch, sn );
    }
    send_to_char( "Ok.\n\r", ch );

    return;
}



void spell_null( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    send_to_char( "That's not a spell!\n\r", ch );
    return;
}



void spell_pass_door( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_PASS_DOOR) )
    {
        if (victim == ch)
            send_to_char("You are already out of phase.\n\r",ch);
        else
            act("$N is already shifted out of phase.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy( level / 4 );
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char( victim, &af );
    act( "$n turns translucent.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You turn translucent.\n\r", victim );
    return;
}

/* RT plague spell, very nasty */
void spell_plague( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (saves_spell(level,victim,DAM_DISEASE))
    {
        if (ch == victim)
            send_to_char("You feel momentarily ill, but it passes.\n\r",ch);
        else
            act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level * 3/4;

    /* Max duration drastically reduced (97 to 37) - Astark Oct 2012
       af.duration     = level; */
    af.duration     = 4 + number_range(level/8, level/3);

    af.location  = APPLY_STR;
    af.modifier  = -5; 
    af.bitvector = AFF_PLAGUE;
    affect_join(victim,&af);

    send_to_char
        ("You scream in agony as plague sores erupt from your skin.\n\r",victim);
    act("$n screams in agony as plague sores erupt from $s skin.",
            victim,NULL,NULL,TO_ROOM);
}

void spell_confusion( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( saves_spell(level*2/3,victim,DAM_MENTAL) )
        // || saves_spell(level,victim,DAM_CHARM))
    {
        if (ch == victim)
            send_to_char("{xYou feel momentarily {Ms{yi{Gl{Cl{Ry{x, but it passes.\n\r",ch);
        else
            act("$N seems to keep $S sanity.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_range(1,4);//level/4;
    af.location  = APPLY_INT;
    af.modifier  = -15; 
    af.bitvector = AFF_INSANE;
    affect_join(victim,&af);

    send_to_char
        ("{MY{bo{Cu{Gr {%{yw{Ro{mr{Bl{Cd{x {gi{Ys {%{ra{Ml{Bi{cv{Ge{x {yw{Ri{Mt{bh {%{wcolors{x{C?{x\n\r",victim);
    act("$n giggles like $e lost $s mind.",
            victim,NULL,NULL,TO_ROOM);
}

void spell_poison( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;

        if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
        {
            if (IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
            {
                act("Your spell fails to corrupt $p.",ch,obj,NULL,TO_CHAR);
                return;
            }
            obj->value[3] = 1;
            act("$p is infused with poisonous vapors.",ch,obj,NULL,TO_ALL);
            return;
        }

        if (obj->item_type == ITEM_WEAPON)
        {
            /*
               if (IS_WEAPON_STAT(obj,WEAPON_FLAMING)
               ||  IS_WEAPON_STAT(obj,WEAPON_FROST)
               ||  IS_WEAPON_STAT(obj,WEAPON_VAMPIRIC)
               ||  IS_WEAPON_STAT(obj,WEAPON_SHARP)
               ||  IS_WEAPON_STAT(obj,WEAPON_VORPAL)
               ||  IS_WEAPON_STAT(obj,WEAPON_SHOCKING)
               ||  IS_WEAPON_STAT(obj,WEAPON_MANASUCK)
               ||  IS_WEAPON_STAT(obj,WEAPON_MOVESUCK)
               ||  IS_WEAPON_STAT(obj,WEAPON_DUMB)
               ||  IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
               {
               act("You can't seem to envenom $p.",ch,obj,NULL,TO_CHAR);
               return;
               }
             */

            if (IS_WEAPON_STAT(obj,WEAPON_POISON))
            {
                act("$p is already envenomed.",ch,obj,NULL,TO_CHAR);
                return;
            }

            af.where     = TO_WEAPON;
            af.type  = sn;
            af.level     = level / 2;
            af.duration  = level/8;
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj,&af);

            act("$p is coated with deadly venom.",ch,obj,NULL,TO_ALL);
            return;
        }

        act("You can't poison $p.",ch,obj,NULL,TO_CHAR);
        return;
    }

    victim = (CHAR_DATA *) vo;

    if ( is_safe( ch, victim ) )
        return;

    if ( saves_spell( level, victim, DAM_POISON) )
    {
        act("$n turns slightly green, but it passes.",victim,NULL,NULL,TO_ROOM);
        send_to_char("You feel momentarily ill, but it passes.\n\r",victim);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;

    /* Max duration drastically reduced (97 to 37) - Astark Oct 2012
       af.duration     = level; */
    af.duration     = 4 + number_range(level/8, level/3);

    af.location  = APPLY_STR;
    af.modifier  = -10;
    af.bitvector = AFF_POISON;
    affect_join( victim, &af );
    send_to_char( "You feel very sick.\n\r", victim );
    act("$n looks very ill.",victim,NULL,NULL,TO_ROOM);
    return;
}



void spell_protection_evil(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (IS_EVIL(victim))
    {
        if (victim == ch)
            send_to_char("You're not pure enough.\n\r",ch);
        else
            act("$N isn't pure enough.",ch,NULL,victim,TO_CHAR);
        return;
    }    

    if ( IS_AFFECTED(victim, AFF_PROTECT_EVIL) 
            || IS_AFFECTED(victim, AFF_PROTECT_GOOD))
    {
        if (victim == ch)
            send_to_char("You are already protected.\n\r",ch);
        else
            act("$N is already protected.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "You feel holy and pure.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from evil.",ch,NULL,victim,TO_CHAR);
    return;
}

void spell_protection_good(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (IS_GOOD(victim))
    {
        if (victim == ch)
            send_to_char("You're not that evil.\n\r",ch);
        else
            act("$N isn't that evil.",ch,NULL,victim,TO_CHAR);
        return;
    }    

    if ( IS_AFFECTED(victim, AFF_PROTECT_GOOD) 
            || IS_AFFECTED(victim, AFF_PROTECT_EVIL))
    {
        if (victim == ch)
            send_to_char("You are already protected.\n\r",ch);
        else
            act("$N is already protected.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char( victim, &af );
    send_to_char( "You feel aligned with darkness.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from good.",ch,NULL,victim,TO_CHAR);
    return;
}


void spell_ray_of_truth (int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
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
        return;
    }

    spell_blindness(gsn_blindness, level/2, ch, (void *) victim,TARGET_CHAR);

    dam = get_sn_damage( sn, level, ch );
    dam = dam * ( 1000 - victim->alignment ) / 2000;

    if ( saves_spell( level, victim,DAM_HOLY) )
        dam /= 2;

    full_dam( ch, victim, dam, sn, DAM_HOLY, TRUE);
    CHECK_RETURN( ch, victim );
}


void spell_recharge( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int chance, percent;

    if (obj->item_type != ITEM_WAND && obj->item_type != ITEM_STAFF)
    {
        send_to_char("That item does not carry charges.\n\r",ch);
        return;
    }

    if (obj->value[3] >= 3 * level / 2)
    {
        send_to_char("Your skills are not great enough for that.\n\r",ch);
        return;
    }

    if (obj->value[1] == 0)
    {
        send_to_char("That item has already been recharged once.\n\r",ch);
        return;
    }

    chance = 40 + 2 * level;

    chance -= obj->value[3]; /* harder to do high-level spells */
    chance -= (obj->value[1] - obj->value[2]) *
        (obj->value[1] - obj->value[2]);

    chance = UMAX(level/2,chance);

    percent = number_percent();

    if (percent < chance / 2)
    {
        act("$p glows softly.",ch,obj,NULL,TO_CHAR);
        act("$p glows softly.",ch,obj,NULL,TO_ROOM);
        obj->value[2] = UMAX(obj->value[1],obj->value[2]);
        obj->value[1] = 0;
        return;
    }

    else if (percent <= chance)
    {
        int chargeback,chargemax;

        act("$p glows softly.",ch,obj,NULL,TO_CHAR);
        act("$p glows softly.",ch,obj,NULL,TO_CHAR);

        chargemax = obj->value[1] - obj->value[2];

        if (chargemax > 0)
            chargeback = UMAX(1,chargemax * percent / 100);
        else
            chargeback = 0;

        obj->value[2] += chargeback;
        obj->value[1] = 0;
        return;
    }   

    else if (percent <= UMIN(95, 3 * chance / 2))
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
        if (obj->value[1] > 1)
            obj->value[1]--;
        return;
    }

    else /* whoops! */
    {
        act("$p glows brightly and explodes!",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly and explodes!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
    }
}

void spell_refresh( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal = get_sn_heal( sn, level, ch, victim );
    victim->move = UMIN( victim->move + heal, victim->max_move );
    /*if (victim->max_move == victim->move)
      send_to_char("You feel fully refreshed!\n\r",victim);
      else
      send_to_char( "You feel less tired.\n\r", victim );
      if ( ch != victim )
      act( "$N sighs in relief.", ch, NULL,victim,TO_CHAR );*/
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

    return;
}

void spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char buf[MSL]; 

    /* do object cases first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;

        if (IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
        {
            if (IS_OBJ_STAT(obj,ITEM_NOUNCURSE))
            {
                act("The curse on $p cannot be removed.",ch,obj,NULL,TO_CHAR);
                obj = obj->next;
            }
            
            if (!saves_dispel(level + 2,obj->level,0))
            {
                REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
                act("$p glows blue.",ch,obj,NULL,TO_ALL);
                return;
            }

            act("The curse on $p is beyond your power.",ch,obj,NULL,TO_CHAR);
            sprintf(buf,"Spell failed to uncurse %s.\n\r",obj->short_descr);
            send_to_char(buf,ch);
        }
        else
        {
            act("There doesn't seem to be a curse on $p.",ch,obj,NULL,TO_CHAR);
            return;
        }
    }

    /* characters */
    victim = (CHAR_DATA *) vo;
   
    if ( check_dispel(level,victim,gsn_curse) || check_dispel(level,victim,gsn_tomb_rot) )
    {
        send_to_char("You feel better.\n\r",victim);
        act("$n looks more relaxed.",victim,NULL,NULL,TO_ROOM);
        return;
    }

    for (obj = victim->carrying; obj != NULL; obj = obj->next_content)
    {
        if (IS_OBJ_STAT(obj,ITEM_NOUNCURSE))
        {
            act("The curse on $p cannot be removed.",ch,obj,NULL,TO_CHAR);
            obj = obj->next_content;
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
                return;
            }

            sprintf(buf,"Spell failed to uncurse %s.\n\r",obj->short_descr);
            send_to_char(buf,ch);
            return;
        }
    }

    send_to_char( "There is nothing to uncurse.\n\r", ch );
    return;
} 

void spell_sanctuary( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
    {
        if (victim == ch)
            send_to_char("You are already in sanctuary.\n\r",ch);
        else
            act("$N is already in sanctuary.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 6;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a white aura.\n\r", victim );
    return;
}



void spell_shield( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
            send_to_char("You are already shielded from harm.\n\r",ch);
        else
            act("$N is already protected by a shield.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 8 + level;
    af.location  = APPLY_AC;
    /* Now scales with level instead of static number - Astark Oct 2012
       It's slightly better than armor due to the level you get it at
       af.modifier  = -20; */
    af.modifier  = -25 - (level/4);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a force shield.\n\r", victim );
    return;
}



void spell_shocking_grasp(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( check_hit( ch, victim, sn, DAM_LIGHTNING, 100 ) )
    {
        /* Now works just like chill touch, but does slightly
           less damage - Astark Oct 2012 */
        dam = get_sn_damage( sn, level, ch ) * 12/10;
        if ( saves_spell( level, victim, DAM_LIGHTNING) )
            dam /= 2;
        shock_effect( victim, level, dam, TARGET_CHAR );
    }
    else
        dam = 0;

    full_dam( ch, victim, dam, sn, DAM_LIGHTNING,TRUE);
    return;
}


void spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
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
        return;
    }

    if ( IS_UNDEAD(victim) )
    {
        send_to_char("The undead never sleep!\n\r", ch );
        return;
    }

    if ( saves_spell(level, victim, DAM_MENTAL)
            || save_body_affect(victim, level) 
            || (!IS_NPC(victim) && number_bits(1))
            || IS_IMMORTAL(victim) )
    {
        send_to_char("Your spell has no effect.\n\r", ch );
        return;
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

    return;
}

void spell_slow( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_SLOW))
    {
        if (victim == ch)
            send_to_char("You can't move any slower!\n\r",ch);
        else
            act("$N can't get any slower than that.",
                    ch,NULL,victim,TO_CHAR);
        return;
    }

    if (saves_spell(level,victim,DAM_OTHER) 
            ||  IS_SET(victim->imm_flags,IMM_MAGIC))
    {
        if (victim != ch)
            act( "Spell failed to slow $N down.", ch, NULL, victim,TO_CHAR);
        send_to_char("You feel momentarily lethargic.\n\r",victim);
        return;
    }

    if (IS_AFFECTED(victim,AFF_HASTE))
    {
        if (!check_dispel(level,victim,skill_lookup("haste")))
        {
            if (victim != ch)
		        act( "Spell failed to reduce $N's speed.", ch, NULL, victim, TO_CHAR );
            send_to_char("You feel momentarily slower.\n\r",victim);
            return;
        }

        act("$n is moving less quickly.",victim,NULL,NULL,TO_ROOM);
        //return;
    }


    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    /* Max duration reduced from 49 to 25 - Astark Oct 2012
       af.duration  = level/2; */
    af.duration  = level/4;
    af.location  = APPLY_AGI;
    af.modifier  = -1 - level/5;
    af.bitvector = AFF_SLOW;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself slowing d o w n...\n\r", victim );
    act("$n starts to move in slow motion.",victim,NULL,NULL,TO_ROOM);
    return;
}


void spell_stone_skin( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( ch, sn ) )
    {
        if (victim == ch)
            send_to_char("Your skin is already as hard as a rock.\n\r",ch); 
        else
            act("$N is already as hard as can be.",ch,NULL,victim,TO_CHAR);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    /* AC now scales with level, max of 54 instead of
       baseline 40 - Astark Oct 2012
       af.modifier  = -40; */
    af.modifier  = -35 - (level/4);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "Your skin turns to stone.\n\r", victim );
    return;
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


void spell_summon( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;

    ignore_invisible = TRUE;
    if ( (victim = get_char_world(ch, target_name)) == NULL)
    {
        send_to_char( "You failed completely.\n\r", ch );
        return;
    }
    ignore_invisible = FALSE;
    if ( victim == ch )
    {
        send_to_char( "Summon yourself?\n\r", ch );
        return;
    }
    if ( !IS_NPC(ch) && IS_TAG(ch) 
            || IS_REMORT(ch)
            || IS_SET(ch->in_room->room_flags, ROOM_PRIVATE)
            || IS_SET(ch->in_room->room_flags, ROOM_SOLITARY)
            || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
            || IS_SET(ch->in_room->room_flags, ROOM_ARENA)
            || is_guild_room(ch->in_room->vnum)
            || room_is_private(ch->in_room) )
    {
        send_to_char( "You can't summon anyone here!\n\r", ch );
        return;
    }
    if ( !can_see_room(victim, ch->in_room) )
    {
        send_to_char( "You can't summon that player here!\n\r", ch );
        return;
    }
    if ( IS_NPC(victim) || IS_SET(victim->act, PLR_NOSUMMON)
            || IS_SET(victim->comm, COMM_AFK) )
    {
        send_to_char( "Your target does not wish to be summoned.\n\r", ch );
        return;
    }
    if ( IS_IMMORTAL(victim) || victim->level > level + 5 )
    {
        send_to_char( "Your target is too powerful for you to summon.\n\r", ch );
        return;
    }
    if ( IS_TAG(victim)
            || IS_REMORT(victim)
            || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
            || victim->fighting != NULL )
    {
        send_to_char( "Your target can't be summoned from its location!\n\r", ch );
        return;
    }

    /*
       if ( ( victim = get_char_world( ch, target_name ) ) == NULL
       ||   victim == ch
       ||   victim->in_room == NULL
       ||   IS_TAG(ch) || IS_TAG(victim)
       ||   IS_NPC(victim)
       ||   IS_REMORT(ch)
       ||   IS_REMORT(victim)
       ||   IS_SET(ch->in_room->room_flags, ROOM_SAFE)
       ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
       ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
       ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
       ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
       ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_AGGRESSIVE))
       ||   victim->level >= level + 3
       ||   (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL)
       ||   victim->fighting != NULL
       ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
       ||   (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
       ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON))
       ||   (IS_NPC(victim) && saves_spell(level, victim,DAM_OTHER))
       ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) )

       {
       send_to_char( "You failed.\n\r", ch );
       return;
       }
     */

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

    act( "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, ch->in_room );
    act( "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
    act( "$n has summoned you!", ch, NULL, victim,   TO_VICT );
    do_look( victim, "auto" );
    return;
}

void spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    ROOM_INDEX_DATA *pRoomIndex;
    OBJ_DATA *stone;

    if ( !can_cast_transport(ch) )
        return;

    if ( victim->in_room == NULL
            || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
            || IS_TAG(ch)
            || ( victim != ch && IS_SET(victim->imm_flags,IMM_SUMMON))
            || ( !IS_NPC(ch) && victim->fighting != NULL )
            || ( victim != ch
                && ( saves_spell( level - 5, victim,DAM_OTHER))))
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }

    stone = get_eq_char(ch,WEAR_HOLD);

    pRoomIndex = get_random_room(victim);

    if ( pRoomIndex == NULL 
            || pRoomIndex->area->security < 5
            || pRoomIndex->area->security > 8 /* Added this - Astark 1-7-13 */ 
            || !can_see_room(ch,pRoomIndex) 
            /* Teleport wasn't working because the IS_SET check was missing - Astark 1-7-13 */
            || IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
            || IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)
            || IS_SET(pRoomIndex->room_flags, ROOM_JAIL)
            || is_guild_room(pRoomIndex->vnum)
            || room_is_private(pRoomIndex) )
    {
        send_to_char( "The room begins to fade from around you, but then it slowly returns.\n\r", ch );
        return;
    }


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
            return;
        }
    }

    if (victim != ch)
        send_to_char("You have been teleported!\n\r",victim);

    act( "$n vanishes!", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    act( "$n abruptly appears from out of nowhere.", victim, NULL, NULL, TO_ROOM );
    do_look( victim, "auto" );
    return;
}



void spell_ventriloquate( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    char speaker[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;

    target_name = one_argument( target_name, speaker );

    if ( ch == vo )
    {
        send_to_char( "If you want to say something, just do so.\n\r", ch );
        return;
    }

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( vch != vo && IS_AWAKE(vch))
        {
            if ( vch == ch )
                act( "{sYou make $n say {S'$t'{x", vo, target_name, vch, TO_VICT );
            else if ( saves_spell(level,vch,DAM_OTHER) )
                act( "{s$n says {S'$t'{x", vo, target_name, vch, TO_VICT );
            else
                act( "{sSomeone makes $n say {S'$t'{x", vo, target_name, vch, TO_VICT );
        }
    }

    return;
}



void spell_weaken( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_WEAKEN))
    {
        if (victim == ch)
            send_to_char("You can't get any weaker.\n\r",ch);
        else
            act("Weak as $E is, your spell had no effect.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if ( saves_spell( level, victim,DAM_OTHER) )
    {
        send_to_char("Your weakening failed to have an effect.\n\r", ch );
        send_to_char("Your strength fails you for a moment.\n\r", victim );
        return;
    }

    if (IS_AFFECTED(victim,AFF_GIANT_STRENGTH))
    {
        if (!check_dispel(level,victim,skill_lookup("giant strength")))
        {
            if (victim != ch)
		        act( "Spell failed to make $N weaker.\n\r", ch, NULL, victim, TO_CHAR );
            send_to_char("You feel momentarily weaker.\n\r",victim);
            return;
        }

        act("$n is looking much weaker.",victim,NULL,NULL,TO_ROOM);
        //return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    /*    Max duration reduced from 49 ticks to 24 - Astark Oct 2012
          af.duration  = level / 2; */
    af.duration  = level / 4;
    af.location  = APPLY_STR;
    af.modifier  = -1 * (level / 2);
    af.bitvector = AFF_WEAKEN;
    affect_to_char( victim, &af );
    send_to_char( "You feel your strength slip away.\n\r", victim );
    act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);
    return;
}



/* RT recall spell is back */
void spell_word_of_recall( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    ROOM_INDEX_DATA *location;
    int chance;

    if (IS_NPC(victim))
        return;

    if ( !can_cast_transport(ch) )
        return;

    if ((location = get_room_index( ROOM_VNUM_TEMPLE)) == NULL)
    {
        send_to_char("You are completely lost.\n\r",victim);
        return;
    } 

    /*
       if (IS_SET(victim->in_room->room_flags,ROOM_NO_RECALL) ||
       IS_AFFECTED(victim,AFF_CURSE))
       {
       send_to_char("Your recall spell failed.\n\r",victim);
       return;
       }
     */

    if (NOT_AUTHED(victim))
    {
        send_to_char("You cannot recall until your character is authorized by the Immortals.\n\r",victim);
        return;
    }

    if (IS_TAG(victim))
    {
        send_to_char("You cannot recall while playing Freeze Tag.\n\r",victim);
        return;
    }

    if (!IS_NPC(victim) && in_pkill_battle(victim))    
    {
        send_to_char("You cannot recall during a pkill battle!\n\r",victim);
        return;
    }

    if (ch->pcdata != NULL && ch->pcdata->pkill_timer > 0)
    {
        send_to_char("Adrenaline is pumping!\n\r", ch);
        return;
    }

    if (IS_AFFECTED(victim, AFF_ENTANGLE))
    {
        chance = (number_percent ());
        if (  (chance) >  (get_curr_stat(victim, STAT_LUC)/10) )
        {
            send_to_char( "The plants entangling you hold you in place!\n\r", victim );
            return;
        }
    }

    /*
       if (victim->fighting != NULL)
       stop_fighting(victim,TRUE);
     */

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
            return;
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

    act("$n disappears.",victim,NULL,NULL,TO_ROOM);
    char_from_room(victim);
    char_to_room(victim,location);
    act("$n appears in the room.",victim,NULL,NULL,TO_ROOM);
    do_look(victim,"auto");

    /* -Rim 2/22/99 */
    if (victim->pet != NULL)
        do_recall(victim->pet,"");
}

/* Draconian & Necromancer spells are in breath.c --Bobble */

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = number_range( 25, 100 );
    if ( saves_spell( level, victim, DAM_PIERCE) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_PIERCE ,TRUE);
    return;
}

void spell_high_explosive(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = number_range( 30, 120 );
    if ( saves_spell( level, victim, DAM_PIERCE) )
        dam /= 2;
    full_dam( ch, victim, dam, sn, DAM_PIERCE ,TRUE);
    return;
}



/* Check number of charmees against cha - returns number of hitdice left over
 * Also send error message when this number is below required amount
 */
int check_cha_follow( CHAR_DATA *ch, int required )
{
    CHAR_DATA *check;
    int charmed=0;
    int max = ch->level * get_curr_stat(ch, STAT_CHA) / 40;

    for ( check=char_list ; check != NULL; check = check->next )
        if (IS_NPC(check) && IS_AFFECTED(check,AFF_CHARM) && check->master == ch && ch->pet != check)
            charmed += check->level;

    if (required > 0 && charmed + required > max)
        send_to_char("You are not charismatic enough to attract more followers.\n\r",ch);

    return UMAX(0, max - charmed);
}

void do_scribe( CHAR_DATA *ch, char *argument )
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
        if ( (scroll = create_object(get_obj_index(OBJ_VNUM_SCROLL), 0)) == NULL )
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

