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
#include <string.h>
#include <math.h>
#include <time.h>
#include <lua.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "buffer_util.h"
#include "religion.h"
#include "olc.h"
#include "mob_stats.h"
#include "lua_scripting.h"
#include "lua_arclib.h"
#include "mudconfig.h"
#include "warfare.h"
#include "special.h"

/* command procedures needed */
DECLARE_DO_FUN(do_quit      );
DECLARE_DO_FUN(do_morph     );
DECLARE_DO_FUN(do_flee      );
DECLARE_DO_FUN(do_stand     );

/*
 * Local functions.
 */
void affect_update( CHAR_DATA *ch );
void qset_update( CHAR_DATA *ch );
bool check_drown args((CHAR_DATA *ch));
int hit_gain    args( ( CHAR_DATA *ch ) );
int mana_gain   args( ( CHAR_DATA *ch ) );
int move_gain   args( ( CHAR_DATA *ch ) );
void    mobile_update   args( ( void ) );
void    mobile_timer_update args( ( void ) );
void    char_update args( ( void ) );
void    obj_update  args( ( void ) );
void    aggr_update args( ( void ) );
void    quest_update    args( ( void ) ); /* Vassago - quest.c */
void    sort_bounty   args( (SORT_TABLE *sort) );
void penalty_update (CHAR_DATA *ch);
ROOM_INDEX_DATA *find_jail_room(void);
void    msdp_update args( ( void ) );
void create_haunt( CHAR_DATA *ch );
void check_beast_mastery( CHAR_DATA *ch );
void check_shadow_companion( CHAR_DATA *ch );
void validate_all();
void check_clan_align( CHAR_DATA *gch );
void check_equipment_align( CHAR_DATA *gch );


/* used for saving */

int save_number = 0;



/*
 * Advancement stuff.
 */



void advance_level( CHAR_DATA *ch, bool hide )
{
    char buf[MAX_STRING_LENGTH];
    int add_prac;
    int played_now = ch->played + (int) (current_time - ch->logon);
    int time_since_last_level = played_now - ch->pcdata->last_level;

    ch->pcdata->last_level = played_now;

    if (! IS_SET(ch->act, PLR_TITLE))
    {
        sprintf( buf, "the %s",
                title_table [ch->class] [(ch->level+4-(ch->level+4)%5)/5]);
        set_title( ch, buf );
    }

    /* make sure player gets prac/trains only once (e.g. level-loss punishment) */
    if ( ch->pcdata->highest_level < ch->level )
    {

        /*  Commented out to replace with new stat gain formula, basing 
         *  gains on prime and secondary stats instead of just DIS - Astark 1-2-13
         *
         *  add_prac    = ch_dis_practice(ch); 
         */

        add_prac    = ch_prac_gains(ch, ch->level);
        // divide by 100, rounding randomly to preserve average
        add_prac = add_prac/100 + (chance(add_prac % 100) ? 1 : 0);
        ch->practice    += add_prac;
        ch->train       += 1 + UMAX(0, ch->level - LEVEL_MIN_HERO);
        ch->pcdata->highest_level = ch->level;
    }
    else
        add_prac = 0;

    check_achievement(ch);
    tattoo_modify_level( ch, ch->level - 1, ch->level );
    update_perm_hp_mana_move(ch);

    if (IS_HERO(ch))
    {
        ch->pcdata->condition[COND_DRUNK] = 0;
        ch->pcdata->condition[COND_FULL] = 0;
        ch->pcdata->condition[COND_THIRST] = -1;
        ch->pcdata->condition[COND_HUNGER] = -1;
        if (!hide)
            send_to_char("A bolt of energy from the gods has removed your hunger and thirst!\n\r",ch);
    }

    if (!hide)
    {
        sprintf(buf, "You gain %d practice%s.\n\r", add_prac, add_prac == 1 ? "" : "s");
        send_to_char( buf, ch );
    }
    
    // chance for restore, reaching 100% after 20-30 minutes
    int heal_chance = 10 + UMAX(0, 90 - ch->level) / 3 + time_since_last_level / 20;
    if ( per_chance(heal_chance) )
    {
        ch->hit = ch->max_hit;
        ch->mana = ch->max_mana;
        ch->move = ch->max_move;
        if (!hide)
            send_to_char("You feel yourself bursting with renewed energy!\n\r", ch);
    }
    
    return;
}   


void gain_exp( CHAR_DATA *ch, int gain)
{
    char buf[MAX_STRING_LENGTH];
    long field, max;

    if ( IS_NPC(ch) || IS_HERO(ch) )
        return;

    if ( IS_SET(ch->act,PLR_NOEXP) && gain > 0 )
        return;

    field = UMAX((ch_wis_field(ch)*gain)/100,0);
    gain-=field;

    max=exp_per_level(ch)+ch_dis_field(ch);
    if (ch->pcdata->field>max)
        send_to_char("Your mind is becoming overwhelmed with new information.\n\r",ch);
    max*=2;

    field = (field*(max-ch->pcdata->field))/(max);
    ch->pcdata->field+=(short)field;

    if ((field+gain)>0)
    {
        sprintf(buf, "You earn %d applied experience, and %ld field experience.\n\r",
                gain, field);
        send_to_char(buf,ch);
    }

    ch->exp = UMAX( exp_per_level(ch), ch->exp + gain );
    update_pc_level(ch);
}


void update_pc_level( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    
    if ( NOT_AUTHED(ch) && ch->exp >= exp_per_level(ch) * (ch->level+1)
            && ch->level >= LEVEL_UNAUTHED )
    {
        send_to_char("{RYou can not ascend to a higher level until you are authorized.{x\n\r", ch);
        ch->exp = (exp_per_level(ch) * (ch->level+1));
        return;
    }

    while ( !IS_HERO(ch) && ch->exp >= exp_per_level(ch) * (ch->level+1) )
    {
        send_to_char( "You raise a level!!  ", ch );
        ch->level += 1;
        update_lboard( LBOARD_LEVEL, ch, ch->level, 1);

        sprintf(buf,"%s has made it to level %d!",ch->name,ch->level);
        log_string(buf);
        info_message(ch, buf, FALSE);

        sprintf(buf,"$N has attained level %d!",ch->level);
        wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);

        advance_level(ch,FALSE);
    }

    return;
}

void update_field( CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    int gain,pos;

    if (!IS_NPC(ch) && ch->desc == NULL) /* Linkdead PC */
        return;

    gain = number_range(1, ch_int_field(ch));
    pos = ch->position;

    if (pos == POS_RESTING || pos == POS_SITTING) 
        gain*=2;
    else if (pos == POS_FIGHTING) 
        gain/=2;

    gain = UMIN(gain, ch->pcdata->field);
    ch->pcdata->field -= gain;

    ch->exp = UMAX(exp_per_level(ch), ch->exp + gain );

    if ( NOT_AUTHED(ch) && ch->exp >= exp_per_level(ch) * (ch->level+1)
            && ch->level >= LEVEL_UNAUTHED )
    {
        send_to_char("{RYou can not ascend to a higher level until you are authorized.{x\n\r", ch);
        ch->exp = (exp_per_level(ch) * (ch->level+1));
        return;
    }

    while ( !IS_HERO(ch) && ch->exp >= exp_per_level(ch) * (ch->level+1) )
    {
        send_to_char( "You raise a level!!  ", ch );
        ch->level += 1;
        update_lboard( LBOARD_LEVEL, ch, ch->level, 1);

        sprintf(buf,"%s has made it to level %d!",ch->name,ch->level);
        log_string(buf);
        info_message(ch, buf, FALSE);

        sprintf(buf,"$N has attained level %d!",ch->level);
        wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);

        advance_level(ch,FALSE);
    }
}

int gain_mod(int x)
{
    if (x<1000)
        return (x/40);
    if (x<5000)
        return (25+(x-1000)/160);
    return (50+(x-5000)/400);
}

/*
 * Regeneration stuff.
 */

struct race_type* get_morph_race_type( CHAR_DATA *ch );

/* adjustments which are the same for hp/mana/move */
int adjust_gain( CHAR_DATA *ch, int gain, int target )
{
    if ( target == APPLY_MANA )
        switch ( ch->position )
        {
            case POS_SLEEPING: gain += gain/2; break;
            case POS_RESTING: gain *= 2; break;
            case POS_FIGHTING: gain /= 2; break;
            default: break;
        }
    else
        switch ( ch->position )
        {
            case POS_SLEEPING: gain *= 2; break;
            case POS_RESTING: gain += gain/2; break;
            case POS_FIGHTING: gain /= 2; break;
            default: break;
        }
    gain /= 2;

    int regen = get_skill(ch, gsn_regeneration);
    if ( IS_AFFECTED(ch, AFF_REGENERATION) )
        regen = 100;
    gain += gain * regen / 200;

    /* encumberance can half healing speed */
    gain -= gain * get_encumberance( ch ) / 200;

    if ( IS_AFFECTED(ch, AFF_POISON) )
        gain /= 2;

    if ( IS_AFFECTED(ch, AFF_PLAGUE) )
        gain /= 2;

    if ( IS_AFFECTED(ch, AFF_HASTE)
            && !IS_SET(get_morph_race_type(ch)->affect_field, AFF_HASTE)
            && ch->fighting != NULL )
        gain /= 2 ;


    /* a bit randomness */
    gain = number_range( gain - gain/10, gain + gain/10 );

    /* PCs suffer from hunger and thirst */
    if ( !IS_NPC(ch) )
    {
        if ( ch->pcdata->condition[COND_HUNGER] > 0 
                && ch->pcdata->condition[COND_HUNGER] < 20 )
            gain /= 2;
        else if ( ch->pcdata->condition[COND_HUNGER]==0 )
            gain = 0;

        if ( ch->pcdata->condition[COND_THIRST] > 0 
                && ch->pcdata->condition[COND_THIRST] < 20 )
            gain /= 2;
        else if ( ch->pcdata->condition[COND_THIRST]==0 )
            gain = 0;


        /* PCs benefit from deep sleep */
        gain += gain * URANGE(0, ch->pcdata->condition[COND_DEEP_SLEEP], 10) / 5;
    }

    return gain;
}

int hit_gain( CHAR_DATA *ch )
{
    int gain;
    int ratio;

    if ( ch->in_room == NULL )
        return 0;

    /* no hp/mana healing in warfare */
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_WAR) )
    {
        if( ch->hit > ch->max_hit )
            ch->hit = ch->max_hit;
        return 0;
    }


    /* Increase the baseline gains by 33% - Astark 12-27-12 
       gain = (10 + ch->level) * get_curr_stat(ch, STAT_VIT); */
    gain = (10 + ch->level*4/3) * get_curr_stat(ch, STAT_VIT);
    if ( !IS_NPC(ch) )
        gain = gain * class_table[ch->class].hp_gain / 100;

    gain += gain * (get_skill(ch, gsn_fast_healing) + mastery_bonus(ch, gsn_fast_healing, 60, 100)) / 200;
    if ( ch->hit < ch->max_hit )
        check_improve(ch, gsn_fast_healing, TRUE, 5);

    /* healing ratio */
    ratio = ch->in_room->heal_rate;
    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        ratio = ratio * ch->on->value[3] / 100;

    if ( ratio >= 0 )
        gain = gain * ratio / 100;
    else
        if ( IS_NPC(ch) )
            return 0;
        else
            return UMAX( 1 - ch->hit, ch->level * ratio / 100 );

    /* general factor */
    gain = adjust_gain(ch, gain, APPLY_HIT );

    return UMIN( gain / 100, ch->max_hit - ch->hit );
}

int mana_gain( CHAR_DATA *ch )
{
    int gain;
    int ratio;

    if (ch->in_room == NULL)
        return 0;

    /* no hp/mana healing in warfare */
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_WAR) )
    {
        if( ch->mana > ch->max_mana )
            ch->mana = ch->max_mana;
        return 0;
    }


    /* Increase the baseline gains by 33% - Astark 12-27-12 
       gain = (10 + ch->level) * get_curr_stat(ch, STAT_INT); */
    gain = (10 + ch->level*4/3) * get_curr_stat(ch, STAT_INT);
    if ( !IS_NPC(ch) )
        gain = gain * class_table[ch->class].mana_gain / 100;

    if ( ch->position == POS_RESTING )
    {
        gain += gain * (get_skill(ch, gsn_meditation) + mastery_bonus(ch, gsn_meditation, 60, 100)) / 100;
        if ( ch->mana < ch->max_mana )
            check_improve(ch, gsn_meditation, TRUE, 3);
    }

    /* healing ratio */
    ratio = ch->in_room->mana_rate;
    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        ratio = ratio * ch->on->value[4] / 100;

    if ( ratio >= 0 )
        gain = gain * ratio / 100;
    else
        if ( IS_NPC(ch) )
            return 0;
        else
            return UMAX( -ch->mana, ch->level * ratio / 100 );

    /* general factor */
    gain = adjust_gain(ch, gain, APPLY_MANA);

    return UMIN( gain / 100, ch->max_mana - ch->mana );
}

int move_gain( CHAR_DATA *ch )
{
    int gain;
    int ratio;

    if (ch->in_room == NULL)
        return 0;

    /* Increase the baseline gains by 33% - Astark 12-27-12 
       gain = (20 + ch->level) * get_curr_stat(ch, STAT_VIT); */
    gain = (20 + ch->level*4/3) * get_curr_stat(ch, STAT_VIT);
    if ( !IS_NPC(ch) )
        gain = gain * class_table[ch->class].move_gain / 100;

    gain += gain * (get_skill(ch, gsn_endurance) + mastery_bonus(ch, gsn_endurance, 60, 100)) / 200;
    if ( ch->move < ch->max_move )
        check_improve(ch, gsn_endurance, TRUE, 6);

    /* healing ratio */
    ratio = ch->in_room->heal_rate;
    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        ratio = ratio * ch->on->value[3] / 100;

    if ( ratio >= 0 )
        gain = gain * ratio / 100;
    else
        if ( IS_NPC(ch) )
            return 0;
        else
            return UMAX( -ch->move, ch->level * ratio / 100 );

    /* general factor */
    gain = adjust_gain(ch, gain, APPLY_MOVE);

    return UMIN( gain / 100, ch->max_move - ch->move );
}

/* old stuff 
   int hit_gain( CHAR_DATA *ch )
   {
   int gain;
   int number;

   if (ch->in_room == NULL)
   return 0;

   if ( IS_NPC(ch) )
   {
   gain =  5 + ch->level;
   if (IS_AFFECTED(ch,AFF_REGENERATION))
   gain *= 2;

   switch(ch->position)
   {
   default :       
   gain /= 2;          
   break;
   case POS_SLEEPING:  
   gain = 3 * gain/2;
   break;
   case POS_RESTING:
   break;
   case POS_FIGHTING:  gain /= 3;
   break;
   }
   if (ch->song_hearing == gsn_lust_life)
   gain += number_range(1,song_level(ch->in_room->singer,gsn_lust_life))*gain/100;


   }
   else
   {
   gain = UMAX(3,get_curr_stat(ch,STAT_VIT)/4 - 5 + ch->level/2); 
   gain = gain * class_table[ch->class].hp_gain / 100;
   number = number_percent();
   if (number < get_skill(ch,gsn_fast_healing))
   {
   if ((ch->hit < ch->max_hit) && (number_bits(4)==0))
   check_improve(ch,gsn_fast_healing,TRUE,10);
   } else number =0;
   if (ch->pcdata->condition[COND_SMOKE]<0||ch->fighting!=NULL)
   gain += (number+ch->pcdata->condition[COND_SMOKE]) * gain / 100;
   else
   gain += (number+ch->pcdata->condition[COND_SMOKE]/2) * gain / 100;

   if (ch->song_hearing == gsn_lust_life)
   gain += number_range(1,song_level(ch->in_room->singer,gsn_lust_life))*gain/100;

   switch ( ch->position )
   {
   default:        gain /= 4;          break;
   case POS_SLEEPING:                  break;
   case POS_RESTING:   gain /= 2;          break;
   case POS_FIGHTING:  gain /= 6;          break;
   }

   if (ch->pcdata->condition[COND_HUNGER]>0 && ch->pcdata->condition[COND_HUNGER]<20)
   gain /= 2;
   else if (ch->pcdata->condition[COND_HUNGER]==0)
   if (ch->hit == 0)
   gain = 0;
   else
   gain /= 4;

   if (ch->pcdata->condition[COND_THIRST]>0 &&  ch->pcdata->condition[COND_THIRST] < 20 )
   gain /= 2;
   else if (ch->pcdata->condition[COND_THIRST]==0)
   gain = 0;

}

gain = gain * ch->in_room->heal_rate / 100;

if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
    gain = gain * ch->on->value[3] / 100;

if ( IS_AFFECTED(ch, AFF_POISON) )
    gain /= 4;

if (IS_AFFECTED(ch, AFF_PLAGUE))
    gain /= 8;

    if ((IS_AFFECTED(ch,AFF_HASTE) && !IS_SET(race_table[ch->race].affect_field, AFF_HASTE)) 
            ||  IS_AFFECTED(ch,AFF_SLOW))
    gain /=2 ;

    number = (number_percent());
if (number < get_skill(ch,gsn_regeneration))
{
    gain += number * gain / 200;
    if ((ch->hit < ch->max_hit) && (number_bits(4)==0))
        check_improve(ch,gsn_regeneration,TRUE,10);
}

gain += gain_mod(ch->max_hit)*gain/100;

return UMIN(gain, ch->max_hit - ch->hit);
}



int mana_gain( CHAR_DATA *ch )
{
    int gain;
    int number;

    if (ch->in_room == NULL)
        return 0;

    if ( IS_NPC(ch) )
    {
        gain = 5 + ch->level;
        switch (ch->position)
        {
            default:        gain /= 2;      break;
            case POS_SLEEPING:  gain = 3 * gain/2;  break;
            case POS_RESTING:               break;
            case POS_FIGHTING:  gain /= 3;      break;
        }
        if (ch->song_hearing == gsn_lust_life)
            gain += number_range(1,song_level(ch->in_room->singer,gsn_lust_life))*gain/100;
    }
    else
    {
        gain = (2*get_curr_stat(ch,STAT_INT) + ch->level) / 8;

        if (ch->position == POS_RESTING &&
                number_percent() < get_skill(ch,gsn_meditation))
        {
            if ((ch->mana < ch->max_mana) && (number_bits(4)==0))
                check_improve(ch,gsn_meditation,TRUE,10);
            gain *= 6;
        }

        if (ch->pcdata->condition[COND_SMOKE]<0||ch->fighting!=NULL)
            gain += (ch->pcdata->condition[COND_SMOKE]) * gain / 100;
        else
            gain += (ch->pcdata->condition[COND_SMOKE]/2) * gain / 100;

        if (ch->song_hearing == gsn_lust_life)
            gain += number_range(1,song_level(ch->in_room->singer,gsn_lust_life))*gain/100;

        if (ch->song_singing != song_null)
            gain /= 2;

        gain = class_table[ch->class].mana_gain*gain/100;

        switch ( ch->position )
        {
            default:        gain /= 4;          break;
            case POS_SLEEPING:                  break;
            case POS_RESTING:   gain /= 2;          break;
            case POS_FIGHTING:  gain /= 6;          break;
        }

        if (ch->pcdata->condition[COND_HUNGER]>0 && ch->pcdata->condition[COND_HUNGER]<20)
            gain /= 2;
        else if (ch->pcdata->condition[COND_HUNGER]==0)
            gain /= 4;

        if (ch->pcdata->condition[COND_THIRST]>0 &&  ch->pcdata->condition[COND_THIRST] < 20 )
            gain /= 2;
        else if (ch->pcdata->condition[COND_THIRST]==0)
            gain /= 4;

    }

    gain = gain * ch->in_room->mana_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[4] / 100;

    if ( IS_AFFECTED( ch, AFF_POISON ) )
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if ((IS_AFFECTED(ch,AFF_HASTE) && !IS_SET(race_table[ch->race].affect_field, AFF_HASTE)) 
            ||  IS_AFFECTED(ch,AFF_SLOW))
        gain /=2 ;

    number = number_percent();
    if (number < get_skill(ch,gsn_regeneration))
    {
        gain += number * gain / 400;
        if ((ch->mana < ch->max_mana) && (number_bits(4)==0))
            check_improve(ch,gsn_regeneration,TRUE,10);
    }

    gain += gain_mod(ch->max_mana)*gain/100;

    return UMIN(gain, ch->max_mana - ch->mana);
}



int move_gain( CHAR_DATA *ch )
{
    int gain,number;

    if (ch->in_room == NULL)
        return 0;

    if ( IS_NPC(ch) )
    {
        gain = ch->level;
        if (ch->song_hearing == gsn_lust_life)
            gain += number_range(1,song_level(ch->in_room->singer,gsn_lust_life))*gain/100;
    }
    else
    {
        gain = UMAX(40,get_curr_stat(ch,STAT_VIT) + ch->level)/8; 
        if ((number=number_percent()) < get_skill(ch,gsn_endurance))
        {
            if ((ch->move < ch->max_move) && (number_bits(4)==0))
                check_improve(ch,gsn_endurance,TRUE,10);
        }else number=0;
        if (ch->pcdata->condition[COND_SMOKE]<0||ch->fighting!=NULL)
            gain += (number+ch->pcdata->condition[COND_SMOKE]) * gain / 100;
        else
            gain += (number+ch->pcdata->condition[COND_SMOKE]/2) * gain / 100;

        if (ch->song_hearing == gsn_lust_life)
            gain += number_range(1,song_level(ch->in_room->singer,gsn_lust_life))*gain/100;

        switch ( ch->position )
        {
            case POS_SLEEPING: gain += get_curr_stat(ch,STAT_VIT)/4;      break;
            case POS_RESTING:  gain += get_curr_stat(ch,STAT_VIT) / 8;  break;
        }

        if (ch->pcdata->condition[COND_HUNGER]>0 && ch->pcdata->condition[COND_HUNGER]<20)
            gain /= 2;
        else if (ch->pcdata->condition[COND_HUNGER]==0)
            if (ch->move == 0)
                gain = 0;
            else
                gain /= 4;

        if (ch->pcdata->condition[COND_THIRST]>0 &&  ch->pcdata->condition[COND_THIRST] < 20 )
            gain /= 2;
        else if (ch->pcdata->condition[COND_THIRST]==0)
            gain = 0;

    }

    if (get_carry_weight(ch) > can_carry_w(ch))
        gain/=2;

    gain = gain * ch->in_room->heal_rate/100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[3] / 100;

    if ( IS_AFFECTED(ch, AFF_POISON) )
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if ((IS_AFFECTED(ch,AFF_HASTE) && !IS_SET(race_table[ch->race].affect_field, AFF_HASTE)) 
            ||  IS_AFFECTED(ch,AFF_SLOW))
        gain /=2 ;

    number = number_percent();
    if (number < get_skill(ch,gsn_regeneration))
    {
        gain += number * gain / 400;
        if ((ch->mana < ch->max_mana) && (number_bits(4)==0))
            check_improve(ch,gsn_regeneration,TRUE,10);
    }

    gain += gain_mod(ch->max_move)*gain/100;

    return UMIN(gain, ch->max_move - ch->move);
}
*/

bool starvation_immune( CHAR_DATA *ch )
{
    return IS_NPC(ch) || NOT_AUTHED(ch) || IS_HERO(ch) || IS_SET(ch->form, FORM_CONSTRUCT) || IS_AFFECTED(ch, AFF_ROOTS);
}

void gain_condition( CHAR_DATA *ch, int iCond, int value )
{
    int condition;

    if ( value == 0 || IS_NPC(ch) )
        return;

    if ( (condition = ch->pcdata->condition[iCond]) < 0 )
        return;

    if ( starvation_immune(ch) && (iCond == COND_HUNGER || iCond == COND_THIRST) && value < 0 )
        return;
    
    ch->pcdata->condition[iCond] = URANGE( 0, condition + value, 72 );
    
    if ( ch->pcdata->condition[iCond] == 0 )
    {
        switch ( iCond )
        {
            case COND_HUNGER:
                send_to_char( "You are starving.\n\r",  ch );
                break;

            case COND_THIRST:
                send_to_char( "You are desiccated.\n\r", ch );
                break;

            case COND_DRUNK:
                if (condition != 0)
                    send_to_char( "You are sober.\n\r", ch);
                break;
        }
    }
    else if ( ch->pcdata->condition[iCond] < 20 )
    {
        switch ( iCond )
        {
            case COND_HUNGER:
                send_to_char( "You are hungry.\n\r",  ch );
                break;

            case COND_THIRST:
                send_to_char( "You are thirsty.\n\r", ch );
                break;
        }
    }

    return;
}

void update_learning( CHAR_DATA *ch )
{
    if ( !ch || !ch->pcdata )
        return;
    
    int sn;
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        // resets (on average) ever minute while fighting, every 5 minutes while not
        if ( per_chance(50) && (ch->fighting || per_chance(20)) )
            ch->pcdata->ready2learn[sn] = TRUE;
    }
}

/* some mobiles need to update more often
 * could be cpu intensive..
 */
void mobile_special_update( void )
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;

    /* only one last_mprog message for better performance */
    sprintf( last_mprog, "mobile_special_update" );

    /* go through mob list */
    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;

        if ( ch->spec_fun == NULL || !IS_NPC(ch) || ch->in_room == NULL || IS_AFFECTED(ch, AFF_CHARM) || IS_AFFECTED(ch, AFF_PETRIFIED) )
            continue;

        /* Examine call for special procedure */
        if ( is_wait_based(ch->spec_fun) && ch->wait == 0 )
        {
            (*ch->spec_fun)( ch );
        }
    }
    sprintf( last_mprog, "(Finished) mobile_special_update" );
}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update( void )
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    EXIT_DATA *pexit;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    int door;
    bool success;

    /* Examine all mobs. */
    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;

        if ( !IS_NPC(ch) || ch->in_room == NULL || IS_AFFECTED(ch, AFF_CHARM) )
            continue;

        if (ch->in_room->area->empty && !IS_SET(ch->act,ACT_UPDATE_ALWAYS))
            continue;

        /* Examine call for special procedure */
        if ( ch->wait == 0 && !IS_AFFECTED(ch, AFF_PETRIFIED) )
        {
            if ( ch->fighting && is_wimpy(ch) )
            {
                if ( ch->position >= POS_FIGHTING )
                    do_flee(ch, "");
                continue;
            }
            else if ( ch->spec_fun != NULL && !is_wait_based(ch->spec_fun) )
            {
                /* update the last_mprog log */
                sprintf( last_mprog, "mob %d at %d %s",
                        ch->pIndexData->vnum,
                        ch->in_room ? ch->in_room->vnum : 0,
                        spec_name_lookup(ch->spec_fun) );

                success = (*ch->spec_fun)( ch );

                /* update the last_mprog log */
                sprintf( last_mprog, "(Finished) mob %d at %d %s",
                        ch->pIndexData->vnum,
                        ch->in_room ? ch->in_room->vnum : 0,
                        spec_name_lookup(ch->spec_fun) );

                if ( success )
                    continue;
            }
        }

        if (ch->pIndexData->pShop != NULL) /* give him some gold */
        {
            long base_wealth = mob_base_wealth(ch->pIndexData);
            if ((ch->gold * 100 + ch->silver) < base_wealth)
            {
                ch->gold += base_wealth * number_range(1,20)/5000000;
                ch->silver += base_wealth * number_range(1,20)/50000;
            }
        }

        /* Delay */
        if ( HAS_TRIGGER(ch, TRIG_DELAY) && ch->mprog_delay > 0 )
        {
            if ( --ch->mprog_delay <= 0 )
            {
                mp_percent_trigger(ch, NULL, NULL,0, NULL,0, TRIG_DELAY);
                continue;
            }
        } 
        /*
         * Check random triggers only if mobile still in default position
         */
        if ( ch->position == ch->pIndexData->default_pos )
        {
            if ( HAS_TRIGGER( ch, TRIG_RANDOM) )
            {
                if( mp_percent_trigger( ch, NULL, NULL,0, NULL,0, TRIG_RANDOM ) )
                    continue;
            }
        } else if ( ch->position == POS_RESTING && ch->wait == 0 && !IS_AFFECTED(ch, AFF_PETRIFIED) )
        {
            do_stand(ch, "");
            WAIT_STATE(ch, PULSE_VIOLENCE/2);
        }

        /* This if check was added to make mobs that were recently disarmed
           have a chance to re-equip their weapons. The mobile_update gets
           called pretty frequently, and this check will make sure the mobs
           are indeed fighting before having them equip anything. It's possible
           that a few rare mobs are holding weapons that they aren't meant to
           equip, but we'll cross that bridge if and when we come to it 
           - Astark 1-7-13 
           - 6-7-14, enabling this code finally - Astark */

        if ( ch->position == POS_FIGHTING && number_bits( 2 ) == 0 )
        {
            for ( obj = ch->carrying; obj != NULL; obj = obj_next )
            {
              obj_next = obj->next_content;
              if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) && obj->item_type == ITEM_WEAPON )
                  wear_obj( ch, obj, FALSE );
            }
        } 


        /* That's all for sleeping / busy monster, and empty zones */
        if ( ch->position != POS_STANDING )
            continue;

        /* Scavenge */
        if ( IS_SET(ch->act, ACT_SCAVENGER)
                &&   ch->in_room->contents != NULL
                &&   !IS_SET(ch->in_room->room_flags, ROOM_DONATION)
                &&   number_bits( 6 ) == 0 )
        {
            OBJ_DATA *obj;
            OBJ_DATA *obj_best;
            int max;

            max         = 1;
            obj_best    = 0;
            for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
            {
                if ( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC
                        || IS_BETWEEN(10375,obj->pIndexData->vnum,10379) ) /* quest objs */
                    continue;
                if ( CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj,FALSE)
                        && obj->cost > max  && obj->cost > 0)
                {
                    obj_best    = obj;
                    max         = obj->cost;
                }
            }

            if ( obj_best )
            {
                obj_from_room( obj_best );
                obj_to_char( obj_best, ch );
                act( "$n gets $p.", ch, obj_best, NULL, TO_ROOM );
            }
        }

        if (ch->pIndexData->vnum==MOB_VNUM_ZOMBIE &&
                !IS_AFFECTED(ch, AFF_CHARM))
            SET_BIT(ch->act, ACT_AGGRESSIVE);

        /* Wander */
        if ( !IS_SET(ch->act, ACT_SENTINEL) 
                && number_bits(3) == 0
                && (door = number_bits(5)) < MAX_DIR
                && (pexit = ch->in_room->exit[door]) != NULL
                && pexit->u1.to_room != NULL
                && !IS_SET(pexit->exit_info, EX_CLOSED)
                && !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
                && (!IS_SET(ch->act, ACT_STAY_AREA)
                    || pexit->u1.to_room->area == ch->in_room->area ) 
                && (!IS_SET(ch->act, ACT_OUTDOORS)
                    || !IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)) 
                && (!IS_SET(ch->act, ACT_INDOORS)
                    || IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)))
        {
            move_char( ch, door, FALSE );
        }
    }

    return;
}

void mobile_timer_update( void )
{
    CHAR_DATA *ch;

    /* go through mob list */
    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if (ch->desc == NULL)
        {
            ch->wait = UMAX(0, ch->wait - 1);
            ch->daze = UMAX(0, ch->daze - 1);
        }
    }
    
    return;
}

/*
 * Update the weather.
 */
void weather_update( void )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int diff;

    buf[0] = '\0';

    switch ( ++time_info.hour )
    {
        case  5:
            weather_info.sunlight = SUN_RISE;
            strcat( buf, "The sun rises in the east.\n\r" );
            for ( d = descriptor_list; d != NULL; d = d->next )
            {
                if ( d->character != NULL && IS_AFFECTED( d->character, AFF_DARKNESS ) )
                {
                    send_to_char("The rising of the sun drains the energy from your body.\n\r",d->character); 
                    REMOVE_BIT( d->character->affect_field, AFF_DARKNESS );
                    affect_strip(d->character, gsn_blessed_darkness);
                }
            }
            break;

        case  6:
            weather_info.sunlight = SUN_LIGHT;
            for (d=descriptor_list; d!=NULL; d=d->next)
                if (d->character && (d->character->race == race_werewolf)
                        && (IS_PLAYING(d->connected)))
                {
                    if (d->connected == CON_PLAYING)
                    {
                        send_to_char("The light burns your eyes and your head clears.\n\r",
                                d->character);
                        if ( IS_AWAKE(d->character) )
                            check_social(d->character, "groan", "");
                    }
                }

            strcat( buf, "The day has begun.\n\r" );
            break;

        case 19:
            weather_info.sunlight = SUN_SET;
            for (d=descriptor_list; d!=NULL; d=d->next)
                if (d->character && (d->character->race == race_werewolf)
                        && (IS_PLAYING(d->connected)))
                {
                    if ( d->connected == CON_PLAYING )
                    {
                        send_to_char("You unsheath your claws and fur sprouts from your flesh.\n\r",
                                d->character);
                        if ( IS_AWAKE(d->character) )
                            check_social(d->character, "howl", "");
                    }
                }

            strcat( buf, "The sun slowly disappears in the west.\n\r" );
            break;

        case 20:
            weather_info.sunlight = SUN_DARK;
            strcat( buf, "The night has begun.\n\r" );
            break;

        case 24:
            time_info.hour = 0;
            time_info.day++;
            break;
    }

    if ( time_info.day   >= 35 )
    {
        time_info.day = 0;
        time_info.month++;
    }

    if ( time_info.month >= 17 )
    {
        time_info.month = 0;
        time_info.year++;
    }

    /*
     * Weather change.
     */
    if ( time_info.month >= 9 && time_info.month <= 16 )
        diff = weather_info.mmhg >  985 ? -2 : 2;
    else
        diff = weather_info.mmhg > 1015 ? -2 : 2;

    weather_info.change   += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    weather_info.change    = UMAX(weather_info.change, -12);
    weather_info.change    = UMIN(weather_info.change,  12);

    weather_info.mmhg += weather_info.change;
    weather_info.mmhg  = UMAX(weather_info.mmhg,  960);
    weather_info.mmhg  = UMIN(weather_info.mmhg, 1040);

    switch ( weather_info.sky )
    {
        default: 
            bug( "Weather_update: bad sky %d.", weather_info.sky );
            weather_info.sky = SKY_CLOUDLESS;
            break;

        case SKY_CLOUDLESS:
            if ( weather_info.mmhg <  990
                    || ( weather_info.mmhg < 1010 && number_bits( 2 ) == 0 ) )
            {
                strcat( buf, "The sky is getting cloudy.\n\r" );
                weather_info.sky = SKY_CLOUDY;
            }
            break;

        case SKY_CLOUDY:
            if ( weather_info.mmhg <  970
                    || ( weather_info.mmhg <  990 && number_bits( 2 ) == 0 ) )
            {
                strcat( buf, "It starts to rain.\n\r" );
                weather_info.sky = SKY_RAINING;
            }

            if ( weather_info.mmhg > 1030 && number_bits( 2 ) == 0 )
            {
                strcat( buf, "The clouds disappear.\n\r" );
                weather_info.sky = SKY_CLOUDLESS;
            }
            break;

        case SKY_RAINING:
            if ( weather_info.mmhg <  970 && number_bits( 2 ) == 0 )
            {
                strcat( buf, "Lightning flashes in the sky.\n\r" );
                weather_info.sky = SKY_LIGHTNING;
            }

            if ( weather_info.mmhg > 1030
                    || ( weather_info.mmhg > 1010 && number_bits( 2 ) == 0 ) )
            {
                strcat( buf, "The rain stopped.\n\r" );
                weather_info.sky = SKY_CLOUDY;
            }
            break;

        case SKY_LIGHTNING:
            if ( weather_info.mmhg > 1010
                    || ( weather_info.mmhg >  990 && number_bits( 2 ) == 0 ) )
            {
                strcat( buf, "The lightning has stopped.\n\r" );
                weather_info.sky = SKY_RAINING;
                break;
            }
            break;
    }

    if ( buf[0] != '\0' )
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->connected == CON_PLAYING
                    && d->editor == ED_NONE
                    && IS_OUTSIDE(d->character)
                    && IS_AWAKE(d->character) )
                send_to_char( buf, d->character );
        }
    }

    return;
}

/* makes sure that every victim is fighting */
void update_fighting( void )
{
    CHAR_DATA *fch;

    for ( fch = char_list; fch != NULL; fch = fch->next )
    {
        if ( fch->fighting == NULL )
            continue;

        if ( fch->fighting->fighting == NULL )
            set_fighting_new( fch->fighting, fch, FALSE );

        /* fix against invalid positions */
        if ( fch->position == POS_STANDING )
            fch->position = POS_FIGHTING;
    }
}

/* makes sure that every victim in a room is fighting */
void update_room_fighting( ROOM_INDEX_DATA *room )
{
    CHAR_DATA *fch;

    if (room == NULL)
    {
        bug( "update_room_fighting: NULL pointer", 0 );
        return;
    }

    for ( fch = room->people; fch != NULL; fch = fch->next_in_room )
        if ( fch->fighting != NULL && fch->fighting->fighting == NULL )
            set_fighting_new( fch->fighting, fch, FALSE );
}

bool is_pet_storage( ROOM_INDEX_DATA *room )
{
    ROOM_INDEX_DATA *shop_room;
    
    if (room == NULL)
        return FALSE;
    
    if ( (shop_room = get_room_index(room->vnum - 1)) == NULL )
        return FALSE;
    
    return IS_SET(shop_room->room_flags, ROOM_PET_SHOP);
}

/*
 * Update all chars, including mobs.
 */
void char_update( void )
{   
    static int curr_tick=0;
    CHAR_DATA *ch;
    bool healmessage;

    /* update save counter */
    save_number++;
    if (save_number > 29)
        save_number = 0;

    curr_tick=(curr_tick+1)%4;

    /*update_fighting();*/

    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if ( !valid_CH(ch) )
        {
            bugf("Invalid ch in char_update (%d). Removing from list.",
                    ch->pIndexData ? ch->pIndexData->vnum : 0 );
            /* invalid should mean already freed, just kill it from the list */
            char_from_char_list(ch);
            continue;
        }
        
        if (ch->must_extract)
            continue;

        if ( !IS_NPC(ch) )
            update_learning(ch);
        
        /* Check for natural resistance */
        affect_strip (ch, gsn_natural_resistance);
        if ( get_skill(ch, gsn_natural_resistance) > 0)
        {
            int bonus = ch->level * (get_skill(ch, gsn_natural_resistance) + mastery_bonus(ch, gsn_natural_resistance, 60, 100)) / 500;
            AFFECT_DATA af;
            af.where    = TO_AFFECTS;
            af.type     = gsn_natural_resistance;
            af.level    = ch->level;
            af.location  = APPLY_SAVES;
            af.duration = -1;
            af.modifier = -bonus;
            af.bitvector= 0;
            affect_to_char(ch,&af);
            check_improve( ch, gsn_natural_resistance, TRUE, 8 );
        } 

        /* Check for iron hide */
        affect_strip (ch, gsn_iron_hide);
        if ( get_skill(ch, gsn_iron_hide) > 0)
        {   
            int bonus = ch->level * (get_skill(ch, gsn_iron_hide) + mastery_bonus(ch, gsn_iron_hide, 60, 100)) / 50;
            AFFECT_DATA af;
            af.where    = TO_AFFECTS;
            af.type     = gsn_iron_hide;
            af.level    = ch->level;
            af.location  = APPLY_AC;
            af.duration = -1;
            af.modifier = -bonus;
            af.bitvector=0;
            affect_to_char(ch,&af);
            check_improve( ch, gsn_iron_hide, TRUE, 8 );
        } 


        if ( ch->position >= POS_STUNNED )
        {
            if (IS_NPC(ch))
            {
                /* check to see if we need to go home */
                if ( !IS_SET(ch->act, ACT_TRAVELLER) && ch->zone != NULL 
                        && ch->zone != ch->in_room->area
                        && ch->desc == NULL &&  ch->fighting == NULL 
                        && !IS_AFFECTED(ch,AFF_CHARM) && number_percent() < 5)
                {
                    act("$n wanders on home.",ch,NULL,NULL,TO_ROOM);
                    extract_char(ch,TRUE);
                    continue;
                }

                if (ch->pIndexData->vnum == MOB_VNUM_ZOMBIE
                        && !IS_AFFECTED(ch, AFF_ANIMATE_DEAD))
                {
                    act("$n crumbles into dust.",ch,NULL,NULL,TO_ROOM);
                    drop_eq( ch );
                    extract_char(ch,TRUE);
                    continue;
                }

                if ( ch->pIndexData->vnum == MOB_VNUM_GHOST
                        && ch->desc == NULL && ch->fighting == NULL 
                        && number_bits(3)==0 )
                {
                    act("$n vanishes into the ether.",ch,NULL,NULL,TO_ROOM);
                    extract_char(ch,TRUE);
                    continue;
                }

                if ( IS_SET(ch->act, ACT_PET) && (ch->leader == NULL || !IS_AFFECTED(ch,AFF_CHARM))
                        && ch->desc == NULL && ch->fighting == NULL 
                        && !is_pet_storage(ch->in_room)
                        && number_bits(3)==0 )
                {
                    act("$n wanders off.",ch,NULL,NULL,TO_ROOM);
                    extract_char(ch,TRUE);
                    continue;
                }

                if (ch->aggressors)
                    update_memory(ch);

                if (IS_SET(ch->off_flags, OFF_DISTRACT))
                    REMOVE_BIT(ch->off_flags, OFF_DISTRACT);

                /* have sleeping mobs wake up */
                if ( ch->position == POS_SLEEPING
                        && ch->pIndexData->default_pos != POS_SLEEPING
                        && !IS_AFFECTED(ch, AFF_SLEEP)
                        && !IS_AFFECTED(ch, AFF_CHARM) )
                {
                    act( "$n wakes up.", ch, NULL, NULL, TO_ROOM );
                    set_pos( ch, ch->pIndexData->default_pos );
                }

                /* re-hide mobs */
                if ( ch->position == POS_STANDING
                        && ch->fighting == NULL
                        && IS_SET(ch->pIndexData->affect_field, AFF_HIDE) )
                    SET_AFFECT( ch, AFF_HIDE );
            }
            
            if ( ch->fighting == NULL && IS_SET(race_table[ch->race].affect_field, AFF_INVISIBLE)
                && !IS_AFFECTED(ch, AFF_INVISIBLE) && IS_AFFECTED(ch, AFF_SNEAK) )
            {
                SET_AFFECT(ch, AFF_INVISIBLE);
                send_to_char("You turn invisible once more.\n\r", ch);
            }

            if (!IS_NPC(ch) && ch->desc == NULL) /* Linkdead PC */
                ; /* Skip updates */
            else
            {
                /* werewolfs can change at certain ticks */
                if ( !IS_NPC(ch) && ch->race == race_werewolf )
                    morph_update( ch );

                if ( !IS_NPC(ch) && (ch->position == POS_SLEEPING || ch->position == POS_RESTING) )
                {
                    healmessage = (ch->hit < ch->max_hit || ch->mana < ch->max_mana ||
                            ch->move < ch->max_move);
                }
                else healmessage = FALSE;

                /* PCs heal always to consider negative healing ratio */
                if ( !IS_NPC(ch) || ch->hit < ch->max_hit )
                {
                    // calculate hit_gain first, as it could modify ch->hit
                    // happens if fast healing skill improves, and the xp causes level-up and renewal
                    int gain = hit_gain(ch);
                    ch->hit += gain;
                }

                if ( !IS_NPC(ch) || ch->mana < ch->max_mana )
                {
                    int gain = mana_gain(ch);
                    ch->mana += gain;
                }

                if ( !IS_NPC(ch) || ch->move < ch->max_move )
                {
                    int gain = move_gain(ch);
                    ch->move += gain;
                }

                if (healmessage)
                {
                    if( ch->hit < ch->max_hit || ch->mana < ch->max_mana ||
                            ch->move < ch->max_move )
                        send_to_char("You feel better.\n\r", ch);
                    else
                        send_to_char("You are fully healed.\n\r", ch);
                }
            }
        }

        if ( ch->position == POS_STUNNED )
            update_pos( ch );

        if ( !IS_NPC(ch) && ch->pcdata->customduration > 0)
        {
            ch->pcdata->customduration--;

            if (ch->pcdata->customduration == 0)
            {
                send_to_char("Your flag has worn off.\n\r",ch);
                free_string(ch->pcdata->customflag);
                ch->pcdata->customflag=str_dup("");
                ch->pcdata->customduration=0;
            }
        }

        /* If a character stays asleep without being waken up, they can fall into a deep sleep.
           This is then used in the int hit_gain/mana_gain/move_gain to give the player a 
           bonus to their regeneration. Added by Astark - September 2012 */
        if (!IS_NPC(ch))
        {
            if ((ch->position == POS_SLEEPING) && ch->pcdata->condition[COND_DEEP_SLEEP] < 10)
            {
                ch->pcdata->condition[COND_DEEP_SLEEP] += 1;
                send_to_char("You fall into a deeper sleep.\n\r",ch);
            }
            else if (ch->position != POS_SLEEPING)
                ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
        }

        if ( !IS_NPC(ch) && ch->level < LEVEL_IMMORTAL )
        {
            OBJ_DATA *obj;

            if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
                    &&   obj->item_type == ITEM_LIGHT
                    &&   obj->value[2] > 0 )
            {
                if ( --obj->value[2] == 0 && ch->in_room != NULL )
                {
                    --ch->in_room->light;
                    act( "$p goes out.", ch, obj, NULL, TO_ROOM );
                    act( "$p flickers and goes out.", ch, obj, NULL, TO_CHAR );
                    extract_obj( obj );
                }
                else if ( obj->value[2] <= 5 && ch->in_room != NULL)
                    act("$p flickers.",ch,obj,NULL,TO_CHAR);
            }

            update_field(ch);

            if (IS_IMMORTAL(ch))
                ch->timer = 0;

            if ( ++ch->timer >= 12 )
            {
                if ( ch->was_in_room == NULL && ch->in_room != NULL && !in_pkill_battle(ch))
                {
                    war_remove( ch, FALSE );
                    ch->was_in_room = ch->in_room;
                    if (ch->fighting != NULL)
                        stop_fighting( ch, TRUE );
                    
                    ap_void_trigger( ch );

                    act( "$n disappears into the void.",
                            ch, NULL, NULL, TO_ROOM );
                    if ( IS_SET( ch->in_room->room_flags, ROOM_BOX_ROOM))
                        send_to_char("An employee screams \"You're taking too long!\"\n\rThe angry employee carries you away to Palace Square.\n\r",ch);

                    send_to_char( "You disappear into the void.\n\r", ch );

                    char_from_room( ch );
                    char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
                }

                if ( ch->timer > 30 )
                {
                    do_quit( ch, "" );
                    continue;
                }
            }

            if ((ch->desc == NULL) || IS_WRITING_NOTE(ch->desc->connected)) {}
            else if ((save_number % 2) &&
                    (ch->pcdata->condition[COND_HUNGER]<50) &&
                    (ch->pcdata->condition[COND_HUNGER]>0) &&
                    (number_percent() < get_skill(ch, gsn_sustenance)))
            {
                /* Skip food/drink changes this round due to sustenance. */
                check_improve(ch,gsn_sustenance,TRUE,6);
            }
            else
            {
                gain_condition( ch, COND_FULL, (ch->size > SIZE_MEDIUM) ? -2 : -1 );
                gain_condition( ch, COND_DRUNK,  -1 );

                if ( !starvation_immune(ch) )
                {
                    gain_condition( ch, COND_THIRST, 
                            IS_AFFECTED(ch, AFF_BREATHE_WATER) ? 
                            -1 : -(curr_tick%2) );

                    if ((ch->pcdata->condition[COND_HUNGER]>=20) || curr_tick>2)
                        gain_condition( ch, COND_HUNGER, 
                                ch->size > SIZE_MEDIUM ? -1 : -(curr_tick%2));

                    if ((ch->pcdata->condition[COND_HUNGER]==0) && (ch->level>4))
                    {
                        if (ch->move>ch->max_move/50+2)
                            ch->move-=ch->max_move/50+2;
                        else if (ch->move>0)
                        {
                            ch->move=0;
                            send_to_char("You collapse from hunger.\n\r", ch);
                        }
                        else if (ch->hit>ch->max_hit/30+2)
                            ch->hit-=ch->max_hit/30+2;
                        else if (ch->hit>-5)
                        {
                            ch->hit= -5;
                            send_to_char("You pass out from starvation.\n\r", ch);
                            update_pos(ch);
                        }
                    }

                    if ((ch->pcdata->condition[COND_THIRST]==0) && (ch->level>4))
                    {
                        if (ch->move>ch->max_move/30+2)
                            ch->move-=ch->max_move/30+2;
                        else if (ch->move>0)
                        {
                            ch->move=0;
                            send_to_char("You collapse from thirst.\n\r", ch);
                        }
                        else if (ch->hit>ch->max_hit/20+2)
                            ch->hit-=ch->max_hit/20+2;
                        else if (ch->hit>-5)
                        {
                            ch->hit= -5;
                            send_to_char("You pass out from dehydration.\n\r", ch);
                            update_pos(ch);
                        }
                    }

                }
            }
            
            if ( ch->pcdata->prayer_request )
            {
                ch->pcdata->prayer_request->ticks--;
                if ( ch->pcdata->prayer_request->ticks == 0 )
                    grant_prayer(ch);
            }
        }


        if (!IS_NPC(ch))
        {
            if (ch->pcdata->morph_time>0)
            {
                ch->pcdata->morph_time--;
                if (ch->pcdata->morph_time==0)
                    do_morph(ch, "");
            }

            /* Apply penalties - valid for imms as well as morts, so not in previous section */
            penalty_update(ch);

            if ((ch->in_room->sector_type == SECT_UNDERWATER) &&
                    !((ch->desc == NULL) || IS_WRITING_NOTE(ch->desc->connected)))
                check_drown(ch);
        }

        /* update granted commands */
        if (!IS_NPC(ch) && ch->pcdata->granted != NULL)
        {
            GRANT_DATA *gran, *gran_next, *gran_prev;

            gran_prev = ch->pcdata->granted;

            for (gran = ch->pcdata->granted; gran != NULL; gran = gran_next)
            {
                gran_next = gran->next;

                if (gran->duration > 0) 
                    gran->duration--;

                if (gran->duration == 0)
                {
                    if (gran == ch->pcdata->granted)
                        ch->pcdata->granted = gran_next;
                    else
                        gran_prev->next = gran->next;

                    printf_to_char(ch, "Your time runs out on the %s command.\n\r",gran->name);
                    free_string(gran->name);
                    free_mem(gran,sizeof(*gran));
                    gran = NULL;
                }

                if (gran != NULL) 
                    gran_prev = gran;
            }
        }

        affect_update( ch );
        qset_update( ch );

        if ( IS_AFFECTED(ch, AFF_HAUNTED) )
            create_haunt( ch );
        if ( !IS_NPC(ch) )
        {
            check_beast_mastery( ch );
            //check_shadow_companion(ch);
        }
    }

    return;
}

/* create a ghost that haunts the char */
void create_haunt( CHAR_DATA *ch )
{
    CHAR_DATA *mob;
    int level, rand, i;

    /* only chance to be haunted.. unless you're sleeping ;P */
    if ( ch->position != POS_SLEEPING && number_bits(2) )
        return;

    if ( (mob = create_mobile(get_mob_index(MOB_VNUM_GHOST))) == NULL ) 
        return;

    /* small tiny ghost or big nasty? */
    level = 200;
    for (i = 0; i < 3; i++)
    {
        rand = number_range( 1, ch->level * 2 );
        level = UMIN(level, rand);            
    }

    set_mob_level( mob, level );
    char_to_room( mob, ch->in_room );

    act( "A ghost has come to haunt you!", ch, NULL, mob, TO_CHAR );
    act( "A ghost has come to haunt $n!", ch, NULL, mob, TO_ROOM );

    multi_hit( mob, ch, TYPE_UNDEFINED );
}

/*
   void spread_affect( CHAR_DATA *ch, AFFECT_DATA *af, int dam_type, char *msg )
   {
   CHAR_DATA *rch;
   bool addflag;

   if ( ch == NULL || ch->in_room == NULL || af == NULL )
   return;

   addflag = af->where == TO_AFFECTS && af->bitvector != 0;

   for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
   {
   if ( rch == ch
   || IS_IMMORTAL(ch)
   || addflag && IS_AFFECTED(rch, af->bitvector)
   || is_affected(rch, af->type)
   || saves_spell(af->level, rch, dam_type)
   || number_bits(4) )
   continue;

// spread the affect
affect_to_char( rch, af );
send_to_char( msg, rch );
}
}
 */


/* Each time the qsets update (each tick), it will check to see if an
   hour has passed. If it has, then the timer is reduced by 1 (hour), and
   the limit is reset. This means that each hour the timer is dropped by
   1 point. - Astark October 2012 */ 

void qset_update( CHAR_DATA *ch )
{
    QUEST_DATA *qdata;

    if ( ch == NULL )
    {
        bugf( "qset_update: NULL character given" );
        return;
    }

    if ( IS_NPC(ch) || ch->pcdata == NULL )
        return;

    /* add quest data if not found */
    for ( qdata = ch->pcdata->qdata; qdata != NULL; qdata = qdata->next )
    {	
        if ( qdata->timer > 0 && (qdata->limit < current_time) )
        {
            /* Changed this value from 55 (55 seconds or about 1 real life minute) to
               3600 seconds or about 1 real life hour - Astark 1-3-13 */
            qdata->limit = current_time + 3600; /* 1 real life hour */ 
            qdata->timer -= 1;
        }
    }
}


/* update the affects on a character */
void affect_update( CHAR_DATA *ch )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    if ( ch == NULL || ch->in_room == NULL )
        return;

    // may recover faster from maledictions than normal while resting
    // single check for all affects, as one skill may add multiple affects which should have same duration
    bool malediction_recovery = FALSE;
    if ( ch->position < POS_FIGHTING && !ch->fighting && number_range(1, MAX_CURRSTAT) <= get_curr_stat(ch, STAT_VIT) )
        malediction_recovery = TRUE;
    
    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
        paf_next    = paf->next;
        if ( paf->duration > 0 )
        {
            paf->duration--;
            if ( paf->duration > 0 && malediction_recovery && is_offensive(paf->type) )
                paf->duration--;
            if (number_range(0,4) == 0 && paf->level > 0)
                paf->level--;  /* spell strength fades with time */
        }
        else if ( paf->duration < 0 )
            ;
        else
        {
            if ( paf_next == NULL
                    ||   paf_next->type != paf->type
                    ||   paf_next->duration > 0 )
            {
                if ( paf->type == gsn_custom_affect )
                {
                    ptc( ch, "Your %s affect has worn off.\n\r", paf->tag );
                }
                else if ( paf->type > 0 && skill_table[paf->type].msg_off )
                {
                    send_to_char( skill_table[paf->type].msg_off, ch );
                    send_to_char( "\n\r", ch );
                }
            }

            affect_remove( ch, paf );
        }
    }

    /* decompose */
    if ( is_affected(ch, gsn_decompose) )
        decompose_update(ch, 0);

    /*
     *   Careful with the damages here,
     *   MUST NOT refer to ch after damage taken,
     *   as it may be lethal damage (on NPC).
     */

    if (IS_AFFECTED(ch, AFF_PLAGUE) && ch != NULL)
    {
        AFFECT_DATA *af, plague;
        CHAR_DATA *vch;
        int dam;

        act("$n writhes in agony as plague sores erupt from $s skin.",
                ch,NULL,NULL,TO_ROOM);
        send_to_char("You writhe in agony from the plague.\n\r",ch);

        af = affect_find_flag( ch->affected, AFF_PLAGUE );

        if (af == NULL)
        {
            REMOVE_BIT(ch->affect_field,AFF_PLAGUE);
            return;
        }

        if ( af->level > 1
                && !IS_AFFECTED(ch, AFF_NECROSIS)
                && !IS_SET(ch->in_room->room_flags, ROOM_SAFE) )
        {

            plague.where        = TO_AFFECTS;
            plague.type         = gsn_plague;
            plague.level        = af->level - 1; 
            plague.duration     = get_duration(gsn_plague, plague.level);
            plague.location     = APPLY_STR;
            plague.modifier     = -5;
            plague.bitvector    = AFF_PLAGUE;

            for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
            {
                if ( !saves_spell(vch, NULL, plague.level, DAM_DISEASE)
                        &&  !IS_IMMORTAL(vch)
                        &&  !(IS_NPC(vch) && IS_SET(vch->act, ACT_OBJ))
                        &&  !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(4) == 0 )
                {
                    send_to_char("You feel hot and feverish.\n\r",vch);
                    act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM);
                    affect_join(vch,&plague);
                }
            }
        }

        dam = UMIN(ch->level,af->level/5+1);
        if ( IS_AFFECTED(ch, AFF_NECROSIS) )
            dam *= 4;
        ch->mana = UMAX(ch->mana - dam, 0);
        ch->move = UMAX(ch->move - dam, 0);
        deal_damage( ch, ch, dam, gsn_plague, DAM_DISEASE, FALSE, FALSE);
        if ( IS_DEAD(ch) )
            return;
    }

    if (IS_AFFECTED(ch, AFF_LAUGH) && ch != NULL)
    {
        AFFECT_DATA *af, laugh;
        CHAR_DATA *vch;

        /*
           for ( af = ch->affected; af != NULL; af = af->next )
           {
           if ((af->type == gsn_laughing_fit))
           break;
           }
         */
        af = affect_find_flag( ch->affected, AFF_LAUGH );

        if (af == NULL)
        {
            REMOVE_BIT(ch->affect_field, AFF_LAUGH);
            return;
        }

        if (af->level <= 1)
            return;

        if (ch->position == POS_STANDING)
        {
            act( "$n falls down, laughing hysterically.",ch,NULL,NULL,TO_ROOM);
            send_to_char( "You fall down laughing.\n\r",ch);
            set_pos( ch, POS_SITTING );
            check_lose_stance(ch);
        }
        else
        {
            act("$n laughs like a madman, wonder what's so funny?",ch,NULL,NULL,TO_ROOM);
            send_to_char( "You throw your head back and laugh like a crazy madman!\n\r",ch );
        }

        affect_strip(ch, gsn_laughing_fit);
        laugh.where        = TO_AFFECTS;
        laugh.type         = gsn_laughing_fit;
        laugh.level        = af->level - 1;
        laugh.duration     = af->duration;
        laugh.modifier     = af->modifier - 1;
        laugh.bitvector    = AFF_LAUGH;

        laugh.location    = APPLY_STR;
        affect_to_char(ch, &laugh);
        laugh.location    = APPLY_HITROLL;
        affect_to_char(ch, &laugh);
        laugh.location    = APPLY_INT;
        affect_to_char(ch, &laugh);

        if( !IS_SET(ch->in_room->room_flags, ROOM_SAFE) )
        {
            laugh.where        = TO_AFFECTS;
            laugh.type         = gsn_laughing_fit;
            laugh.level        = af->level - 1;
            laugh.modifier     = -2;
            laugh.bitvector    = AFF_LAUGH;

            for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
            {
                if (!saves_spell(vch, NULL, laugh.level - 2, DAM_MENTAL)
                        &&  !IS_IMMORTAL(vch)
                        &&  !IS_AFFECTED(vch,AFF_LAUGH) && number_bits(4) == 0)
                {
                    send_to_char("You feel light headed and giddy!\n\r",vch);
                    act("$n gets a dumb grin on $s face and starts to giggle!",vch,NULL,NULL,TO_ROOM);
                    laugh.duration     = number_range(1,laugh.level);

                    laugh.location     = APPLY_STR;
                    affect_to_char(vch, &laugh);

                    laugh.location     = APPLY_HITROLL;
                    affect_to_char(vch, &laugh);

                    laugh.location     = APPLY_INT;
                    affect_to_char(vch, &laugh);
                }
            } // end of for loop

        } // end of ROOM_SAFE check

        ch->move = UMAX(ch->move - 5, 0);
        DAZE_STATE(ch, 3 * PULSE_VIOLENCE);

    }

    if ( IS_AFFECTED(ch, AFF_POISON) && ch != NULL
            &&   !IS_AFFECTED(ch,AFF_SLOW))
    {
        AFFECT_DATA *poison;

        poison = affect_find(ch->affected,gsn_poison);

        if (poison != NULL)
        {
            act( "$n shivers and suffers.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You shiver and suffer.\n\r", ch );
            deal_damage(ch,ch,poison->level/10 + 1,gsn_poison, DAM_POISON,FALSE,FALSE);
        }
    }

    if ( ch->position == POS_INCAP && number_range(0,1) == 0)
    {
        damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE,FALSE);
    }
    else if ( ch->position == POS_MORTAL )
    {
        damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE,FALSE);
    }
    else
        update_pos( ch );
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{   
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf, *paf_next;
    ROOM_INDEX_DATA *room;
    bool is_remort;

    for ( obj = object_list; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next;

        if ( !valid_OBJ(obj) )
        {
            bugf("Invalid obj in obj_update (%d). Removing from list.", obj->pIndexData->vnum);
            /* invalid should mean already freed, just kill it from the list */
            obj_from_object_list(obj);
            continue;
        }

        CHAR_DATA *rch;
        const char *message = "";

        if (obj->must_extract)
            continue;

        /* go through affects and decrement */
        for ( paf = obj->affected; paf != NULL; paf = paf_next )
        {
            paf_next    = paf->next;
            if ( paf->duration > 0 )
            {
                paf->duration--;
                if (number_range(0,4) == 0 && paf->level > 0)
                    paf->level--;  /* spell strength fades with time */
            }
            else if ( paf->duration < 0 )
                ;
            else
            {
                if ( paf_next == NULL
                        ||   paf_next->type != paf->type
                        ||   paf_next->duration > 0 )
                {
                    if ( paf->type > 0 && skill_table[paf->type].msg_obj )
                    {
                        if (obj->carried_by != NULL)
                        {
                            rch = obj->carried_by;
                            act(skill_table[paf->type].msg_obj,
                                    rch,obj,NULL,TO_CHAR);
                        }
                        if (obj->in_room != NULL 
                                && obj->in_room->people != NULL)
                        {
                            rch = obj->in_room->people;
                            act(skill_table[paf->type].msg_obj,
                                    rch,obj,NULL,TO_ALL);
                        }
                    }
                }

                affect_remove_obj( obj, paf );
            }
        }

        if ( !op_percent_trigger( NULL, obj, NULL, NULL, NULL, OTRIG_RAND) )
            continue;

        if ( obj->timer <= 0 || --obj->timer > 0 )
            continue;

        if (in_donation_room(obj))
            message = "$p shimmers and fades away.";
        else
            switch ( obj->item_type )
            {
                default:              message = "$p crumbles into dust.";  break;
                case ITEM_FOUNTAIN:   message = "$p dries up.";         break;
                case ITEM_CORPSE_NPC: message = "$p decays into dust."; break;
                case ITEM_CORPSE_PC:  message = "$p decays into dust."; break;
                case ITEM_EXPLOSIVE:  
                {
                    ROOM_INDEX_DATA *room=get_obj_room(obj);
                    if (!room)
                    {
                        bugf("No room for %d.", obj->pIndexData->vnum);
                        break;
                    }
                    if ( IS_SET(room->room_flags, ROOM_SAFE)
                            ||  IS_SET(room->room_flags, ROOM_LAW) )
                        message = "$p is whisked away swiftly by an imp.";
                    else if (number_percent() <= 10)
                        message = "$p sputters a little and dies out.";
                    else
                    {
                        message = "$p explodes violently, throwing you to the floor!";
                        explode(obj);
                    }
                    break;
                }

                case ITEM_FOOD:       message = "$p decomposes.";   break;
                /* pots shouldn't have timers set anymore - Vodur Jan2014 */
                case ITEM_POTION:     message = "$p has evaporated from disuse.";   
                                      break;
                case ITEM_TRASH:      message = "$p collapses into nothingness."; break;
                case ITEM_PORTAL:     message = "$p fades out of existence."; break;
                case ITEM_CONTAINER: 
                                      if (CAN_WEAR(obj,ITEM_WEAR_FLOAT))
                                          /*
                                             if (obj->contains)
                                             message = 
                                             "$p flickers and vanishes, spilling its contents on the floor.";
                                             else
                                           */
                                          message = "$p flickers and vanishes.";
                                      else
                                          message = "$p crumbles into dust.";
                                      break;
            }

        if ( obj->carried_by != NULL )
        {
            if (IS_NPC(obj->carried_by) 
                    &&  obj->carried_by->pIndexData->pShop != NULL)
                obj->carried_by->silver += obj->cost/5;
            else
            {
                act( message, obj->carried_by, obj, NULL, TO_CHAR );
                if ( obj->wear_loc == WEAR_FLOAT)
                    act(message,obj->carried_by,obj,NULL,TO_ROOM);
            }
        }
        else if ( obj->in_room != NULL
                && ( rch = obj->in_room->people ) != NULL )
        {
            act( message, rch, obj, NULL, TO_ROOM );
            act( message, rch, obj, NULL, TO_CHAR );
        }

        /* make sure items won't get lost in remort due to corpse crumbling */
        room = get_obj_room( obj );
        is_remort = room != NULL && IS_SET( room->area->area_flags, AREA_REMORT );

        if ( (obj->item_type == ITEM_CORPSE_PC
                    || obj->item_type == ITEM_CONTAINER
                    || is_remort )
                && obj->contains)
        {   /* save the contents */
            OBJ_DATA *t_obj, *next_obj;

            for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
            {
                next_obj = t_obj->next_content;
                obj_from_obj(t_obj);

                if (obj->in_obj) /* in another object */
                    obj_to_obj(t_obj,obj->in_obj);

                else if (obj->carried_by)  /* carried */
                    /*
                       if (obj->wear_loc == WEAR_FLOAT)
                       if (obj->carried_by->in_room == NULL)
                       extract_obj(t_obj);
                       else
                       obj_to_room(t_obj,obj->carried_by->in_room);
                       else
                     */
                    obj_to_char(t_obj,obj->carried_by);
                else if (obj->in_room == NULL)  /* destroy it */
                {
                    bugf("obj_update: destroying container %s (%d) but nowhere to dump contents.", obj->name, obj->pIndexData->vnum);
                    // immediate extraction could cause obj_next to become invalid
                    t_obj->must_extract = TRUE;
                }

                else /* to a room */
                {
                    /* A container in a donation room has expired.
                       Its contents are being dumped to the floor, and so
                       should be given a donation timer as well. */
                    if (in_donation_room(obj))
                    {
                        if (t_obj->timer)
                            SET_BIT(t_obj->extra_flags,ITEM_HAD_TIMER);
                        else
                            t_obj->timer = number_range(100,200);
                    }
                    obj_to_room(t_obj,obj->in_room);
                }
            }
        }

        // immediate extraction could cause obj_next to become invalid
        obj->must_extract = TRUE;
    }

    return;
}

/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update( void )
{
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;
    int chance, vch_cha;

    for ( wch = char_list; wch != NULL; wch = wch_next )
    {
        wch_next = wch->next;
        if ( IS_NPC(wch)
                ||   wch->level >= LEVEL_IMMORTAL
                ||   wch->in_room == NULL 
                ||   wch->in_room->area->empty)
            continue;

        for ( ch = wch->in_room->people; ch != NULL; ch = ch_next )
        {
            int count;

            ch_next = ch->next_in_room;

            if ( !IS_NPC(ch)
                    ||   ch->wait > 0
                    ||   (!IS_SET(ch->act, ACT_AGGRESSIVE) && !(ch->aggressors))
                    ||   (IS_SET(ch->in_room->room_flags,ROOM_SAFE)
                        && !IS_SET(ch->act, ACT_IGNORE_SAFE) )
                    ||   IS_AFFECTED(ch,AFF_CALM)
                    ||   ch->fighting != NULL
                    ||   IS_SET(ch->off_flags, OFF_DISTRACT)
                    ||   IS_AFFECTED(ch, AFF_CHARM)
                    ||   ch->position < POS_STANDING
                    ||   ( IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch) )
                    ||   !can_see( ch, wch )
                    ||   number_bits(1) == 0)
                continue;

            /*
             * Ok we have a 'wch' player character and a 'ch' npc aggressor.
             * Now make the aggressor fight a RANDOM pc victim in the room,
             *   giving each 'vch' an equal chance of selection.
             */
            count   = 0;
            victim  = NULL;
            for ( vch = wch->in_room->people; vch != NULL; vch = vch_next )
            {
                vch_next = vch->next_in_room;
                vch_cha = level_power(vch) + ch_cha_aggro(vch);
                if ( is_affected(vch, gsn_disguise) )
                    vch_cha += get_skill(vch, gsn_disguise) / 5;

                if ( !IS_NPC(vch)
                        && vch->level < LEVEL_IMMORTAL
                        && ch->level >= vch_cha - 5
                        && ( !IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch) )
                        && !(IS_GOOD(ch) && IS_GOOD(vch))
                        && check_see( ch, vch )
                        && (IS_SET(ch->act, ACT_AGGRESSIVE) || check_anger(ch, vch)))
                {
                    if ( number_range( 0, count ) == 0 )
                    {
                        victim = vch;
                        count++;
                    }
                }
            }

            if ( victim == NULL )
                continue;

            if (((chance= get_skill(victim, gsn_soothe))!=0) &&
                    (!IS_SET(ch->form, FORM_SENTIENT)))
            {
                chance -= get_curr_stat(ch,STAT_INT)/5;
                chance += get_curr_stat(victim,STAT_CHA)/5;
                chance += (victim->level - ch->level)/2;

                if (number_percent() < chance)
                {
                    act( "You soothe $n with your peaceful presence.", ch, NULL, victim, TO_VICT );
                    act( "$N soothes you with $s peaceful presence.", ch, NULL, victim, TO_CHAR );
                    act( "$N soothes $n with $s peaceful presence.", ch, NULL, victim, TO_NOTVICT );
                    // apply calm effect
                    AFFECT_DATA af;
                    af.where = TO_AFFECTS;
                    af.type = gsn_soothe;
                    af.level = victim->level;
                    af.duration = get_duration(gsn_soothe, victim->level);
                    af.location = APPLY_HITROLL;
                    af.modifier = -5;
                    af.bitvector = AFF_CALM;
                    affect_to_char(ch, &af);
                    forget_attacks( ch );
                    check_improve(victim,gsn_soothe,TRUE,3);
                    continue;
                }
                act( "You fail to soothe $n.", ch, NULL, victim, TO_VICT );
                check_improve(victim,gsn_soothe,FALSE,3);
            }

            if ( IS_SET(ch->off_flags, OFF_BACKSTAB) )
            {
                backstab_char( ch, victim );
                continue;
            }

            if ((chance= get_skill(victim, gsn_avoidance))!=0)
            {
                chance += get_curr_stat(victim,STAT_AGI)/4;
                chance -= get_curr_stat(ch,STAT_AGI)/4;
                chance += (victim->level - ch->level *3)/8;

                if (number_percent() <chance)
                {
                    act( "You avoid $n!",  ch, NULL, victim, TO_VICT    );
                    act( "$N avoids you!", ch, NULL, victim, TO_CHAR    );
                    act( "$N avoids $n!",  ch, NULL, victim, TO_NOTVICT );
                    check_improve(victim,gsn_avoidance,TRUE,3);
                    continue;
                }
            }

            victim = check_bodyguard(ch, victim);
            multi_hit (ch, victim, TYPE_UNDEFINED); 
        }
    }
    return;
}

/* resets just_killed flag */
void death_update( void )
{
    CHAR_DATA *ch;

    for ( ch = char_list; ch != NULL; ch = ch->next )
        ch->just_killed = FALSE;
}

/* delayed removal of purged chars */
void extract_update( void )
{
    // extraction of containers or character with pets may trigger extraction of the next object or object in the list
    // hence we start new after any (potentially recursive) extraction
    CHAR_DATA *ch = char_list;
    while ( ch )
    {
        if ( !valid_CH(ch) )
        {
            bugf("Invalid ch in extract_update" );
            /* invalid should mean already freed, just kill it from the list */
            char_from_char_list(ch);
            ch = char_list;
        }
        else if ( ch->must_extract )
        {
            char_from_char_list(ch);
            free_char(ch);
            ch = char_list;
        }
        else
        {
            ch = ch->next;
        }
    }

    OBJ_DATA *obj = object_list;
    while ( obj )
    {  
        if ( !valid_OBJ(obj) )
        {
            bugf("Invalid obj in extract_update" );
            /* invalid should mean already freed, just kill it from the list */
            obj_from_object_list(obj);
            obj = object_list;
        }
        else if ( obj->must_extract )
        {
            extract_obj(obj);
            obj = object_list;
        }
        else
        {
            obj = obj->next;
        }
    }
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler( void )
{
    static  int     pulse_area;
    static  int     pulse_mobile;
    static  int     pulse_mobile_special;
    static  int     pulse_violence;
    static  int     pulse_point;
    static  int     pulse_save = 3; // "= 3" to reduce CPU peeks
    static  int     pulse_herb;
    static  int     pulse_msdp;
    static  int     pulse_timer;
    static bool hour_update = TRUE;
    static bool minute_update = TRUE;
    /* if nobody is logged on, update less to safe CPU power */
    bool update_all = (descriptor_list != NULL );

    if ( --pulse_timer <= 0 )
    {
        pulse_timer     = PULSE_TIMER_TRIG;
        timer_update();
    }

    if ( --pulse_msdp <= 0 )
    {
        pulse_msdp      = PULSE_PER_SECOND;
        msdp_update();
    } 

    if ( --pulse_save <= 0 )
    {
        pulse_save = PULSE_SAVE;
        handle_player_save();
    }

    if ( update_all && --pulse_area <= 0 )
    {
        pulse_area  = PULSE_AREA;
        /* number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 ); */
        area_update ( FALSE );
        remort_update();
    }

    if ( update_all && --pulse_herb <= 0 )
    {
        pulse_herb  = PULSE_HERB;
        reset_herbs_world();
    }

    if ( update_all )
    {
        mobile_timer_update();
        if ( --pulse_mobile <= 0 )
        {
            pulse_mobile         = PULSE_MOBILE;
            mobile_update   ( );
        }
        if ( --pulse_mobile_special <= 0 )
        {
            pulse_mobile_special = PULSE_MOBILE_SPECIAL;
            mobile_special_update   ( );
        }
    }

    if ( update_all && --pulse_violence <= 0 )
    {
        pulse_violence  = PULSE_VIOLENCE;
        violence_update ( );
        /* relics */
        all_religions( &religion_relic_damage );
    }

    if ( update_all && --pulse_point    <= 0 )
    {
        wiznet("TICK!",NULL,NULL,WIZ_TICKS,0,0);
        pulse_point     = PULSE_TICK;
        /* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */

        auth_update();
        weather_update  ( );
        char_update ( );
        /* obj_update  ( ); */  /* Original spot for obj_update() */

        war_update  ( );  
        quest_update( );  
        obj_update();     /* Added this here - Elik, Feb 8, 2006 */ 
        /* clan_update(); */
        all_religions( &religion_create_relic );
        update_relic_bonus();
    }

    /* check lboard reset times once a minute
       could check once an hour or even once a day 
       but 'current_time % HOUR' doesn't account for local time
       so doesn't synch up. */
    if ( current_time % MINUTE == 0 )
    {
        if ( minute_update )
        {
            check_lboard_reset();
        }
    }
    else
        minute_update=TRUE;

    /* update some things once per hour */
    if ( current_time % HOUR == 0 )
    {
        if ( hour_update )
        {
            /* update herb_resets every 6 hours */
            if ( current_time % (6*HOUR) == 0 )
                update_herb_reset();

            /* update priests once per day */
            if ( current_time % DAY == 0 )
            {
                all_religions( &religion_update_followers );
                all_religions( &religion_update_priests );
                all_religions( &religion_restore_relic );
            }
        }
        hour_update = FALSE;
    }
    else
        hour_update = TRUE;

    if ( update_all )
    {
        update_fighting();
        aggr_update();
        death_update();
        extract_update();
        cleanup_uds();
        validate_all();
    }

    tail_chain( );
    return;
}

void deal_bomb_damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam )
{
    bool lethal = ch->in_room == victim->in_room;
    deal_damage(ch, victim, dam, gsn_ignite, MIX_DAMAGE(DAM_BASH, DAM_FIRE), TRUE, lethal);
}

/* Explosives by Rimbol.  Original idea from Wurm codebase. */
void explode(OBJ_DATA *obj)
{
    OBJ_DATA *original_obj = NULL;
    CHAR_DATA *rch, *victim = NULL;
    CHAR_DATA *owner = NULL;
    ROOM_INDEX_DATA *room;
    char buf[MSL];
    long int dam;
    bool contained = FALSE;

    if ( (room = get_obj_room(obj)) == NULL )
    {
        bugf( "explode: no room for %d", obj->pIndexData->vnum);
        return;
    }
    // need to have an owner to damage anyone
    if ( (owner = get_player(obj->owner)) == NULL )
    {
        sprintf(buf, "%s explodes harmlessly.", obj->short_descr);
        recho(buf, room);
        return;
    }
    
    dam = dice(obj->value[0], obj->value[1]);

    if ( obj->in_obj )
    {
        contained = TRUE;
        dam /= 2;
        original_obj = obj;
        while (obj->in_obj)
            obj = obj->in_obj;
    }
    
    recho("=== KA-BOOOOM!!! ===", room);
    
    if ( (victim = obj->carried_by) != NULL )
    {
        if ( contained )
        {
            act("$p explodes in your hands!  Some of the blast is absorbed by $P.", obj->carried_by, original_obj, obj, TO_CHAR);
            act("$p explodes in $n's hands!  Some of the blast is absorbed by $P.", obj->carried_by, original_obj, obj, TO_ROOM);
        }
        else
        {
            act("$p explodes in your hands!", obj->carried_by, obj, NULL, TO_CHAR);
            act("$p explodes in $n's hands!", obj->carried_by, obj, NULL, TO_ROOM);
        }
        deal_bomb_damage(owner, victim, dam);
        set_pos(victim, POS_RESTING);
        return;
    }
    else
    {
        if ( contained )
        {
            sprintf(buf, "%s explodes inside %s!  Some of the blast is absorbed.", original_obj->short_descr, obj->short_descr);
            recho(buf, room);
        }
        dam *= 0.5 * AREA_SPELL_FACTOR;
    }
    
    // no damage in safe rooms
    if ( IS_SET(room->room_flags, ROOM_SAFE) )
        return;
    
    // now the damage
    for ( rch = room->people; rch; rch = rch->next_in_room )
    {
        if ( is_safe_spell(owner, rch, FALSE) )
            continue;

        deal_bomb_damage(owner, rch, dam);
        set_pos(rch, POS_RESTING);
    }
}

void update_bounty(CHAR_DATA *ch)
{
    SORT_TABLE *sort;   

    if ((sort = ch->pcdata->bounty_sort) == NULL)
    {
        if (ch->pcdata->bounty == 0)
            return;

        ch->pcdata->bounty_sort = new_sort();
        sort = ch->pcdata->bounty_sort;
        sort->score = ch->pcdata->bounty;
        sort->owner = ch;
        if (bounty_table == NULL)
        {
            sort->next = sort;
            sort->prev = sort;
            bounty_table = sort;
            return;
        }
        else
        {
            sort->next = bounty_table;
            sort->prev = bounty_table->prev;
            sort->prev->next = sort;
            bounty_table->prev = sort;
        }

    }
    else
    {
        if ( (ch->pcdata->bounty == 0) || !IS_SET(ch->act, PLR_PERM_PKILL) )
        {
            remove_bounty(ch);
            return;
        }

        sort->score = ch->pcdata->bounty;
    }

    sort_bounty(sort);

    return;
}

void sort_bounty(SORT_TABLE * sort)
{
    SORT_TABLE * temp;

    if ((sort->prev->score < sort->score)&&(sort!=bounty_table))
    {
        temp=sort->prev;
        if (temp != sort->next)
        {
            sort->prev = temp->prev;
            temp->next = sort->next;
            sort->next->prev = temp;
            sort->next = temp;
            temp->prev->next = sort;
            temp->prev = sort;
        }
        if (bounty_table == temp)
            bounty_table = sort;
        sort_bounty(sort);
    }
    else if ((sort->next->score > sort->score)&&(sort->next != bounty_table))
    {
        temp=sort->next;
        sort->next = temp->next;
        temp->prev = sort->prev;
        sort->prev = temp;
        temp->next = sort;
        if (bounty_table == sort)
            bounty_table = temp;
        sort_bounty(sort);
    }

    return;
}

void remove_bounty(CHAR_DATA *ch)
{
    SORT_TABLE *sort = ch->pcdata->bounty_sort;
    if (sort == NULL) return;

    if (bounty_table == sort)
    {
        bounty_table = sort->next;
        if (bounty_table == sort)
            bounty_table = NULL;
    }

    sort->prev->next = sort->next;
    sort->next->prev = sort->prev;

    free_sort(sort);
    ch->pcdata->bounty_sort = NULL;

    return;
}

void change_align (CHAR_DATA *ch, int change_by)
{
    double change = (double)change_by;
    double align;
    char buf[60];

    if ((change_by == 0) || (ch == NULL)) return;

    if (IS_REMORT(ch) 
            && ((change_by < 0 && IS_GOOD(ch)) 
                || (change_by > 0 && IS_EVIL(ch))))
        return;

    change_by = adjust_align_change( ch, change_by );

    align = (double)URANGE(-1000, ch->alignment + change_by, 1000);

    /* change of align speed halves as align goes vs. max/min */
    if ((change_by > 0) && (align > 350))
        change *= (1650 - align) / 1300.0;
    else if ((change_by < 0) && (align < -350))
        change *= (1650 + align) / 1300.0;

    change = URANGE(-1000 - ch->alignment, change, 1000 - ch->alignment);

    /* make sure small loss isn't nullified */
    if ( change < 0 && change > -1 )
        change = -1;

    ch->alignment += (short)change;

    if (change == 0) return;

    if (change > 0) {
        if (change > 600)
            send_to_char("You renounce your evil ways.\n\r", ch);
        else if (change > 250)       
            send_to_char("You repent your sins.\n\r", ch);
        else if (change > 100)
            send_to_char("You decide to live a better life.\n\r", ch);
        else if (change > 40)
            send_to_char("Your conscience is surfacing.\n\r", ch);
        else if (change > 5)
            send_to_char("You have a twinge of guilt.\n\r", ch);
        else send_to_char("Your alignment rises.\n\r", ch);
    } 
    else if (change < -600)
        send_to_char("You renounce conventional morality.\n\r", ch); 
    else if (change < -250)
        send_to_char("You relax into a life of sin.\n\r", ch);
    else if (change < -100)
        send_to_char("You are becoming a worse person.\n\r", ch);
    else if (change < -40)
        send_to_char("You stifle your conscience.\n\r", ch);
    else if (change < -5)
        send_to_char("You have an evil thought.\n\r", ch);
    else send_to_char("Your alignment slips.\n\r", ch);

    if (!IS_HERO(ch))
    {
        int exp_loss = 0;

        if ( change > 0 && !IS_GOOD(ch) )
            exp_loss = (int)(change * (1000 - ch->alignment) / 2000.0);
        else if ( change < 0 && !IS_EVIL(ch) )
            exp_loss = (int)(-change * (1000 + ch->alignment) / 2000.0);
            
        gain_exp(ch, -exp_loss);

        if (exp_loss > 4)
        {
            sprintf(buf, "You lose %d experience.\n\r", exp_loss);
            send_to_char(buf,ch);
        }       
    }

    check_religion_align( ch );
    check_clan_align( ch );
    check_equipment_align( ch );
    return;
}

void drop_align( CHAR_DATA *ch )
{
    if ( IS_REMORT(ch) && IS_GOOD(ch) ) 
        return;

    if ( ch->alignment > -1000 )
        ch->alignment -= 1;

    check_religion_align( ch );
    check_clan_align( ch );
    check_equipment_align( ch );
    return;
}

void check_clan_align( CHAR_DATA *gch )
{
    if (!IS_NPC(gch) && !IS_IMMORTAL(gch) &&
            (  gch->alignment < clan_table[gch->clan].min_align 
               || gch->alignment > clan_table[gch->clan].max_align))
    {
        send_to_char("Your alignment has made you unwelcome in your clan!\n\r", gch);
        sprintf(log_buf, "%s has become too %s for clan %s!",
                gch->name, 
                gch->alignment < clan_table[gch->clan].min_align ? "{rEvil{x" : "{wGood{x", 
                capitalize(clan_table[gch->clan].name));

        info_message(gch, log_buf, TRUE);

        gch->clan = 0;
        gch->pcdata->clan_rank = 0;

        check_clan_eq(gch);
    }
    return;
}

void check_equipment_align( CHAR_DATA *gch )
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    for ( obj = gch->carrying; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;
        if ( obj->wear_loc == WEAR_NONE )
            continue;

        if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(gch)    )
                ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(gch)    )
                ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(gch) ) )
        {
            act( "You are zapped by $p.", gch, obj, NULL, TO_CHAR );
            act( "$n is zapped by $p.",   gch, obj, NULL, TO_ROOM );
            obj_from_char( obj );
            obj_to_char( obj, gch );
        }        
    }
    return;
}

// chance to summon best pet with beast mastery
void check_beast_mastery( CHAR_DATA *ch )
{
    AFFECT_DATA af;
    CHAR_DATA *mob;
    MOB_INDEX_DATA *mobIndex;
    char buf[MAX_STRING_LENGTH];
    int mlevel, sector;
    int skill = get_skill(ch, gsn_beast_mastery);

    if ( skill == 0 )
        return;

    // safety net
    if ( ch->in_room == NULL )
        return;

    // must be in the wild for this to work
    sector = ch->in_room->sector_type;
    if ( sector == SECT_INSIDE || sector == SECT_CITY )
        return;

    // must be playing and not in warfare
    if ( IS_SET( ch->act, PLR_WAR ) || ch->desc == NULL || !IS_PLAYING(ch->desc->connected))
        return;

    // only a chance to happen each tick - more likely in the forest, less likely during combat
    if ( number_bits(2)
            || (sector != SECT_FOREST && number_bits(1))
            || (ch->fighting != NULL && number_bits(2))
            || !chance(skill) )
        return;

    // must not have a pet already, and must accept them
    if ( ch->pet != NULL || IS_SET(ch->act, PLR_NOFOLLOW))
        return;

    if ( (mobIndex = get_mob_index(MOB_VNUM_BEAST)) == NULL )
        return;

    mob = create_mobile(mobIndex);

    mlevel = dice(1,3) + ch->level * (80 + skill) / 200;
    mlevel = URANGE(1, mlevel, ch->level);
    set_mob_level( mob, mlevel );

    sprintf(buf,"This wild animal follows %s.\n\r", ch->name);
    free_string(mob->description);
    mob->description = str_dup(buf);

    char_to_room( mob, ch->in_room );

    send_to_char( "A wild animal comes up to you and starts following you around.\n\r", ch );
    act( "A wild animal trods up and follows $n.", ch, NULL, NULL, TO_ROOM );

    add_follower( mob, ch );
    mob->leader = ch;
    af.where     = TO_AFFECTS;
    af.type      = gsn_beast_mastery;
    af.level     = ch->level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( mob, &af );
    SET_BIT(mob->act, ACT_PET);
    ch->pet = mob;

    return;
}

// chance to summon shadow companion pet
/*
void check_shadow_companion( CHAR_DATA *ch )
{
    AFFECT_DATA af;
    CHAR_DATA *mob;
    MOB_INDEX_DATA *mobIndex;
    char buf[MAX_STRING_LENGTH];
    int mlevel;
    int skill = get_skill(ch, gsn_shadow_companion);

    if ( !per_chance(skill) )
        return;

    // safety net
    if ( ch->in_room == NULL )
        return;

    // must be in shadowy area for this to work
    if ( !room_is_dim(ch->in_room) )
        return;

    // must be playing and not in warfare
    if ( IS_SET(ch->act, PLR_WAR) || ch->desc == NULL || !IS_PLAYING(ch->desc->connected) )
        return;

    // only a chance to happen each tick, less likely during combat
    if ( number_bits(2) || (ch->fighting != NULL && number_bits(2)) )
        return;

    // must not have a pet already, and must accept them
    if ( ch->pet != NULL || IS_SET(ch->act, PLR_NOFOLLOW) )
        return;

    if ( (mobIndex = get_mob_index(MOB_VNUM_SHADOW)) == NULL )
        return;

    mob = create_mobile(mobIndex);

    mlevel = dice(1,3) + ch->level * (80 + skill) / 200;
    mlevel = URANGE(1, mlevel, ch->level);
    set_mob_level( mob, mlevel );

    sprintf(buf,"This shadow follows %s.\n\r", ch->name);
    free_string(mob->description);
    mob->description = str_dup(buf);

    char_to_room( mob, ch->in_room );

    send_to_char( "A shadow materializes and starts following you around.\n\r", ch );
    act( "A shadow materializes and follows $n.", ch, NULL, NULL, TO_ROOM );

    add_follower( mob, ch );
    mob->leader = ch;
    af.where     = TO_AFFECTS;
    af.type      = gsn_shadow_companion;
    af.level     = ch->level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( mob, &af );
    SET_BIT(mob->act, ACT_PET);
    ch->pet = mob;

    return;
}
*/

void msdp_update( void )
{
    DESCRIPTOR_DATA *d;
    int PlayerCount = 0;

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
    if ( d->character && IS_PLAYING(d->connected) && !IS_NPC(d->character) )
        {
            char buf[MAX_STRING_LENGTH];
            CHAR_DATA *pOpponent = d->character->fighting;
            ROOM_INDEX_DATA *pRoom = d->character->in_room;
            AFFECT_DATA *paf;

            ++PlayerCount;

            MSDPSetString( d, eMSDP_CHARACTER_NAME, d->character->name );
            MSDPSetNumber( d, eMSDP_ALIGNMENT, d->character->alignment );
            MSDPSetNumber( d, eMSDP_EXPERIENCE, d->character->exp );
            MSDPSetNumber( d, eMSDP_EXPERIENCE_MAX, exp_per_level(d->character) );
            MSDPSetNumber( d, eMSDP_EXPERIENCE_TNL, ((d->character->level + 1) *
               exp_per_level(d->character) - d->character->exp ) );

            MSDPSetNumber( d, eMSDP_HEALTH, d->character->hit );
            MSDPSetNumber( d, eMSDP_HEALTH_MAX, d->character->max_hit );
            MSDPSetNumber( d, eMSDP_LEVEL, d->character->level );
/*
            MSDPSetNumber( d, eMSDP_RACE, TBD );
            MSDPSetNumber( d, eMSDP_CLASS, TBD );
*/
            MSDPSetNumber( d, eMSDP_MANA, d->character->mana );
            MSDPSetNumber( d, eMSDP_MANA_MAX, d->character->max_mana );
            MSDPSetNumber( d, eMSDP_WIMPY, d->character->wimpy );
            MSDPSetNumber( d, eMSDP_PRACTICE, d->character->practice );
            MSDPSetNumber( d, eMSDP_MONEY, d->character->gold );
            MSDPSetNumber( d, eMSDP_MOVEMENT, d->character->move );
            MSDPSetNumber( d, eMSDP_MOVEMENT_MAX, d->character->max_move );
            MSDPSetNumber( d, eMSDP_HITROLL, GET_HITROLL(d->character) );
            MSDPSetNumber( d, eMSDP_DAMROLL, GET_DAMROLL(d->character) );
            //MSDPSetNumber( d, eMSDP_AC, GET_AC(d->character) );
            MSDPSetNumber( d, eMSDP_STR, get_curr_stat(d->character, STAT_STR) );
            MSDPSetNumber( d, eMSDP_INT, get_curr_stat(d->character, STAT_INT) );
            MSDPSetNumber( d, eMSDP_WIS, get_curr_stat(d->character, STAT_WIS) );
            MSDPSetNumber( d, eMSDP_DEX, get_curr_stat(d->character, STAT_DEX) );
            MSDPSetNumber( d, eMSDP_CON, get_curr_stat(d->character, STAT_CON) );
            MSDPSetNumber( d, eMSDP_STR_PERM, d->character->perm_stat[STAT_STR] );
            MSDPSetNumber( d, eMSDP_INT_PERM, d->character->perm_stat[STAT_INT] );
            MSDPSetNumber( d, eMSDP_WIS_PERM, d->character->perm_stat[STAT_WIS] );
            MSDPSetNumber( d, eMSDP_DEX_PERM, d->character->perm_stat[STAT_DEX] );
            MSDPSetNumber( d, eMSDP_CON_PERM, d->character->perm_stat[STAT_CON] );

            /* This would be better moved elsewhere */
            if ( pOpponent != NULL )
            {
                int hit_points = (pOpponent->hit * 100) / pOpponent->max_hit;
                MSDPSetNumber( d, eMSDP_OPPONENT_HEALTH, hit_points );
                MSDPSetNumber( d, eMSDP_OPPONENT_HEALTH_MAX, 100 );
                MSDPSetNumber( d, eMSDP_OPPONENT_LEVEL, pOpponent->level );
                MSDPSetString( d, eMSDP_OPPONENT_NAME, pOpponent->name );
            }
            else /* Clear the values */
            {
                MSDPSetNumber( d, eMSDP_OPPONENT_HEALTH, 0 );
                MSDPSetNumber( d, eMSDP_OPPONENT_LEVEL, 0 );
                MSDPSetString( d, eMSDP_OPPONENT_NAME, "" );
            }

            /* Only update room stuff if they've changed room */
            /* we're not sending vnums yet -Vodur */
            //if ( pRoom && pRoom->vnum != d->pProtocol->pVariables[eMSDP_ROOM_VNUM]->ValueInt )
            if ( pRoom )
            {
                int i; /* Loop counter */
                buf[0] = '\0';

                for ( i = DIR_NORTH; i < MAX_DIR; ++i )
                {
                    if ( pRoom->exit[i] != NULL )
                    {
                        const char MsdpVar[] = { (char)MSDP_VAR, '\0' };
                        const char MsdpVal[] = { (char)MSDP_VAL, '\0' };
                        extern char *const dir_name[];

                        strcat( buf, MsdpVar );
                        strcat( buf, dir_name[i] );
                        strcat( buf, MsdpVal );

                        if ( IS_SET(pRoom->exit[i]->exit_info, EX_CLOSED) )
                            strcat( buf, "C" );
                        else /* The exit is open */
                            strcat( buf, "O" );
                    }
                }

                if ( pRoom->area != NULL )
                    MSDPSetString( d, eMSDP_AREA_NAME, pRoom->area->name );

                MSDPSetString( d, eMSDP_ROOM_NAME, pRoom->name );
                MSDPSetTable( d, eMSDP_ROOM_EXITS, buf );
                //MSDPSetNumber( d, eMSDP_ROOM_VNUM, pRoom->vnum );
            }
/*
            MSDPSetNumber( d, eMSDP_WORLD_TIME, d->character-> );
*/

            buf[0] = '\0';
            for ( paf = d->character->affected; paf; paf = paf->next )
            {
                char skill_buf[MAX_STRING_LENGTH];
                sprintf( skill_buf, "%c%s%c%d",
                    (char)MSDP_VAR, skill_table[paf->type].name, 
                    (char)MSDP_VAL, paf->duration );
                strcat( buf, skill_buf );
            }
            MSDPSetTable( d, eMSDP_AFFECTS, buf );

            MSDPUpdate( d );
        }
    }

    /* Ideally this should be called once at startup, and again whenever 
     * someone leaves or joins the mud.  But this works, and it keeps the 
     * snippet simple.  Optimise as you see fit.
     */
    MSSPSetPlayers( PlayerCount );
}

// data consistency checks for detecting corruption early
// extend as needed for debugging, but keep it fast
void validate_all()
{
    CHAR_DATA *ch, *ch_next, *dch, *lch;
    DESCRIPTOR_DATA *desc, *desc_next;
    
    // characters
    for ( ch = char_list; ch; ch = ch_next )
    {
        ch_next = ch->next;
        if ( !valid_CH(ch) )
        {
            bugf("validate_all: invalid ch in char_list (%s)", ch->name);
            char_from_char_list(ch);
            continue;
        }
        if ( ch->master && !valid_CH(ch->master) )
        {
            bugf("validate_all: invalid ch->master (%s)", ch->name);
            ch->master = NULL;
            continue;
        }
        if ( ch->leader && !valid_CH(ch->leader) )
        {
            bugf("validate_all: invalid ch->leader (%s)", ch->name);
            ch->leader = NULL;
            continue;
        }
        if ( ch->pet )
        {
            if ( !valid_CH(ch->pet) )
            {
                bugf("validate_all: invalid ch->pet (%s)", ch->name);
                ch->pet = NULL;
                continue;
            }
            lch = ch->pet->leader;
            if ( ch != lch )
            {
                bugf("validate_all: ch != ch->pet->leader (%s != %s)", ch->name, !lch ? "NULL" : !valid_CH(lch) ? "invalid" : lch->name);
                ch->pet->leader = NULL;
                ch->pet = NULL;
                continue;
            }
        }
        if ( ch->desc )
        {
            if ( !valid_DESCRIPTOR(ch->desc) )
            {
                bugf("validate_all: invalid ch->desc (%s)", ch->name);
                ch->desc = NULL;
                continue;
            }
            dch = ch->desc->character;
            if ( ch != dch )
            {
                bugf("validate_all: ch != ch->desc->ch (%s != %s)", ch->name, !dch ? "NULL" : !valid_CH(dch) ? "invalid" : dch->name);
                ch->desc->character = NULL;
                ch->desc = NULL;
                continue;
            }
        }
    }
    // descriptors
    for ( desc = descriptor_list; desc; desc = desc_next )
    {
        desc_next = desc->next;
        if ( !valid_DESCRIPTOR(desc) )
        {
            bugf("validate_all: invalid desc in descriptor_list (%s)", desc->host);
            desc_from_descriptor_list(desc);
            continue;
        }
        if ( (dch = desc->character) )
        {
            if ( !valid_CH(dch) )
            {
                bugf("validate_all: invalid desc->character (%s, %s)", desc->host, dch->name);
                desc->character = NULL;
                continue;
            }
            if ( desc != dch->desc )
            {
                bugf("validate_all: desc != desc->character->desc (%s, %s != %s)", dch->name, desc->host, !dch->desc ? "NULL" : !valid_DESCRIPTOR(dch->desc) ? "invalid" : dch->desc->host);
                desc->character->desc = NULL;
                desc->character = NULL;
                continue;
            }
        }
    }
}
