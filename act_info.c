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
#if !defined(WIN32)
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "religion.h"

bool commen_wear_pos( tflag wear_flag1, tflag wear_flag2 );
void show_affects(CHAR_DATA *ch, CHAR_DATA *to_ch, bool show_long, bool show_all);
void who_show_char( CHAR_DATA *ch, CHAR_DATA *wch, BUFFER *output );
void smash_beep_n_blink( char *str );
void smash_reserved_colcodes( char *str );
void show_affect( CHAR_DATA *ch, AFFECT_DATA *paf, bool say_it );
void print_affect( CHAR_DATA *ch, AFFECT_DATA *paf, FILE *fp );
void achievement_reward( CHAR_DATA *ch, int table_index);

/* command procedures needed */
DECLARE_DO_FUN( do_exits    );
DECLARE_DO_FUN( do_look     );
DECLARE_DO_FUN( do_help     );
DECLARE_DO_FUN( do_affects  );
DECLARE_DO_FUN(do_leadership);
DECLARE_DO_FUN(do_lore);
DECLARE_DO_FUN(do_appraise); 
DECLARE_DO_FUN(do_say);
DECLARE_DO_FUN(do_hunt);
DECLARE_DO_FUN(do_worth);
DECLARE_DO_FUN(do_attributes);
/* DECLARE_DO_FUN(do_combo); */

int get_pkgrade_level( int pts );

char *  const   where_name  [] =
{
    "<used as light>     ",
    "<worn on finger>    ",
    "<worn on finger>    ",
    "<worn around neck>  ",
    "<worn around neck>  ",
    "<worn on torso>     ",
    "<worn on head>      ",
    "<worn on legs>      ",
    "<worn on feet>      ",
    "<worn on hands>     ",
    "<worn on arms>      ",
    "<worn as shield>    ",
    "<worn about body>   ",
    "<worn about waist>  ",
    "<worn around wrist> ",
    "<worn around wrist> ",
    "<wielded>           ",
    "<held>              ",
    "<floating nearby>   ",
    "<secondary weapon>  ",
};


/* for do_count */
int max_on = 0;

/*
* Local functions.
*/
char *  format_obj_to_char  args( ( OBJ_DATA *obj, CHAR_DATA *ch,
                                 bool fShort ) );
void    show_list_to_char   args( ( OBJ_DATA *list, CHAR_DATA *ch,
                                 bool fShort, bool fShowNothing ) );
void    show_char_to_char_0 args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void    show_char_to_char_1 args( ( CHAR_DATA *victim, CHAR_DATA *ch, bool glance ) );
void    show_char_to_char   args( ( CHAR_DATA *list, CHAR_DATA *ch ) );
bool    check_blind     args( ( CHAR_DATA *ch ) );
char    get_pkflag          args( ( CHAR_DATA *ch, CHAR_DATA *wch ) );



char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
{
    static char buf[MAX_STRING_LENGTH];
    
    buf[0] = '\0';
    
    if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
        ||  (obj->description == NULL || obj->description[0] == '\0'))
        return buf;
    
    if ( IS_OBJ_STAT(obj, ITEM_INVIS)     )   strcat( buf, "(Invis) "     );
    if ( IS_AFFECTED(ch, AFF_DETECT_EVIL)
        && IS_OBJ_STAT(obj, ITEM_EVIL)   )   strcat( buf, "(Red Aura) "  );
    if (IS_AFFECTED(ch, AFF_DETECT_GOOD)
        &&  IS_OBJ_STAT(obj,ITEM_BLESS))          strcat(buf,"(Blue Aura) " );
    if ( IS_AFFECTED(ch, AFF_DETECT_MAGIC)
        && IS_OBJ_STAT(obj, ITEM_MAGIC)  )   strcat( buf, "(Magical) "   );
    if ( IS_OBJ_STAT(obj, ITEM_GLOW)      )   strcat( buf, "(Glowing) "   );
    if ( IS_OBJ_STAT(obj, ITEM_DARK)      )   strcat( buf, "(Dark) "   );
    if ( IS_OBJ_STAT(obj, ITEM_HUM)       )   strcat( buf, "(Humming) "   );
    
    if ( fShort )
    {
        if ( obj->short_descr != NULL )
            strcat( buf, obj->short_descr );
    }
    else
    {
        if ( obj->description != NULL)
            strcat( buf, obj->description );
    }
    
    return buf;
}



/*
* Show a list to a character.
* Can coalesce duplicated items.
*/

void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;
    char **prgpstrShow;
    int *prgnShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;
    
    if ( ch->desc == NULL )
        return;
    
        /*
        * Alloc space for output lines.
    */
    output = new_buf();
    
    count = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
        count++;
    prgpstrShow = alloc_mem( count * sizeof(char *) );
    prgnShow    = alloc_mem( count * sizeof(int)    );
    nShow   = 0;
    
    /*
    * Format the list of objects.
    */
    for ( obj = list; obj != NULL; obj = obj->next_content )
    { 
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )) 
        {
            pstrShow = format_obj_to_char( obj, ch, fShort );
            
            fCombine = FALSE;
            
            if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
            {
            /*
            * Look for duplicates, case sensitive.
            * Matches tend to be near end so run loop backwords.
                */
                for ( iShow = nShow - 1; iShow >= 0; iShow-- )
                {
                    if ( !strcmp( prgpstrShow[iShow], pstrShow ) )
                    {
                        prgnShow[iShow]++;
                        fCombine = TRUE;
                        break;
                    }
                }
            }
            
            /*
            * Couldn't combine, or didn't want to.
            */
            if ( !fCombine )
            {
                prgpstrShow [nShow] = str_dup( pstrShow );
                prgnShow    [nShow] = 1;
                nShow++;
            }
        }
    }
    
    /*
    * Output the formatted list.
    */
    for ( iShow = 0; iShow < nShow; iShow++ )
    {
        if (prgpstrShow[iShow][0] == '\0')
        {
            free_string(prgpstrShow[iShow]);
            continue;
        }
        
        if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
        {
            if ( prgnShow[iShow] != 1 )
            {
                sprintf( buf, "(%2d) ", prgnShow[iShow] );
                add_buf(output,buf);
            }
            else
            {
                add_buf(output,"     ");
            }
        }
        add_buf(output,prgpstrShow[iShow]);
        add_buf(output,"\n\r");
        free_string( prgpstrShow[iShow] );
    }
    
    if ( fShowNothing && nShow == 0 )
    {
        if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
            send_to_char( "     ", ch );
        send_to_char( "Nothing.\n\r", ch );
    }
    page_to_char(buf_string(output),ch);

    
    /*
    * Clean up.
    */
    free_buf(output);
    free_mem( prgpstrShow, count * sizeof(char *) );
    free_mem( prgnShow,    count * sizeof(int)    );
    
    return;
}



void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH],message[MAX_STRING_LENGTH];
    
    buf[0] = '\0';
    
    if ( IS_SET(victim->comm,COMM_AFK     )   ) strcat( buf, "[AFK] "        );
    
    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_UNAUTHED))
        strcat( buf, "{Y[Pre-Auth] {x" );
    else if (!IS_NPC(victim) && victim->desc == NULL)
        strcat( buf, "(Linkdead) ");
    else if (!IS_NPC(victim) && IS_TAG(victim))
    {
        if ( IS_SET(victim->pcdata->tag_flags,TAG_RED)    ) strcat( buf,"{R[RED] {x"     );
        if ( IS_SET(victim->pcdata->tag_flags,TAG_BLUE)   ) strcat( buf,"{B[BLUE] {x"    );
        if ( IS_SET(victim->pcdata->tag_flags,TAG_FROZEN) ) strcat( buf,"{W[Frozen] {x"  );
    }
    else
    {
        if ( IS_SET(victim->penalty, PENALTY_JAIL)) strcat( buf, "[Jailed] "     );
        if ( IS_AFFECTED(victim, AFF_INVISIBLE)   ) strcat( buf, "(Invis) "      );
        if ( victim->invis_level > LEVEL_HERO     ) strcat( buf, "(Wizi) "       );
        if ( IS_AFFECTED(victim, AFF_HIDE)        ) strcat( buf, "(Hide) "       );
        if ( IS_AFFECTED(victim, AFF_SHELTER)     ) strcat( buf, "(Shelter) "    );
        if ( IS_AFFECTED(victim, AFF_ASTRAL)      ) strcat( buf, "(Astral) "     );  
        if ( IS_AFFECTED(victim, AFF_CHARM)       ) strcat( buf, "(Charmed) "    );
        if ( IS_AFFECTED(victim, AFF_PASS_DOOR)   ) strcat( buf, "(Translucent) ");
        if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) ) strcat( buf, "(Pink Aura) "  );
        if ( IS_EVIL(victim)
            &&   IS_AFFECTED(ch, AFF_DETECT_EVIL)     ) strcat( buf, "(Red Aura) "   );
        if ( IS_GOOD(victim)
            &&   IS_AFFECTED(ch, AFF_DETECT_GOOD)     ) strcat( buf, "(Golden Aura) ");
        if ( IS_AFFECTED(victim, AFF_SANCTUARY)   ) strcat( buf, "(White Aura) " );
        if ( IS_NPC(victim)
	     && IS_SET(victim->act, ACT_WIZI)     ) strcat( buf, "(Mob-Wizi) "   );
        if (IS_NPC(victim) && !IS_NPC(ch) &&
	    ch->pcdata->questmob > 0 && victim->pIndexData->vnum == ch->pcdata->questmob)
            strcat( buf, "[TARGET] ");
	if ( !is_mimic(victim) )
	{
	    if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER ) )
		strcat( buf, "(KILLER) "     );
	    if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF  ) )
		strcat( buf, "(THIEF) "      );
	}
    }

    /* mimics */
    if ( is_mimic(victim) )
    {
	MOB_INDEX_DATA *mimic = get_mimic(victim);
	if ( mimic != NULL && mimic->long_descr[0] != '\0' )
	{
	    strcat( buf, mimic->long_descr );
	    send_to_char( buf, ch );
	    return;
	}
    }

    if ( victim->position == victim->start_pos && victim->long_descr[0] != '\0' )
    {
        strcat( buf, victim->long_descr );
        send_to_char( buf, ch );
        return;
    }
    
    strcat( buf, PERS( victim, ch ) );
	if ( !IS_NPC(victim) && !IS_SET(ch->comm, COMM_BRIEF) 
        &&   victim->position == POS_STANDING && ch->on == NULL )
        strcat( buf, victim->pcdata->title );
    
    switch ( victim->position )
    {
    case POS_DEAD:     strcat( buf, " is DEAD!!" );              break;
    case POS_MORTAL:   strcat( buf, " is mortally wounded." );   break;
    case POS_INCAP:    strcat( buf, " is incapacitated." );      break;
    case POS_STUNNED:  strcat( buf, " is lying here stunned." ); break;
    case POS_SLEEPING: 
        if (victim->on != NULL)
        {
            if (I_IS_SET(victim->on->value[2],SLEEP_AT))
            {
                sprintf(message," is sleeping at %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else if (I_IS_SET(victim->on->value[2],SLEEP_ON))
            {
                sprintf(message," is sleeping on %s.",
                    victim->on->short_descr); 
                strcat(buf,message);
            }
            else
            {
                sprintf(message, " is sleeping in %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
        }
        else 
            strcat(buf," is sleeping here.");
        break;
    case POS_RESTING:  
        if (victim->on != NULL)
        {
            if (I_IS_SET(victim->on->value[2],REST_AT))
            {
                sprintf(message," is resting at %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else if (I_IS_SET(victim->on->value[2],REST_ON))
            {
                sprintf(message," is resting on %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else 
            {
                sprintf(message, " is resting in %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
        }
        else
            strcat( buf, " is resting here." );       
        break;
    case POS_SITTING:  
        if (victim->on != NULL)
        {
            if (I_IS_SET(victim->on->value[2],SIT_AT))
            {
                sprintf(message," is sitting at %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else if (I_IS_SET(victim->on->value[2],SIT_ON))
            {
                sprintf(message," is sitting on %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else
            {
                sprintf(message, " is sitting in %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
        }
        else
            strcat(buf, " is sitting here.");
        break;
    case POS_STANDING: 
        if (victim->on != NULL)
        {
            if (I_IS_SET(victim->on->value[2],STAND_AT))
            {
                sprintf(message," is standing at %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else if (I_IS_SET(victim->on->value[2],STAND_ON))
            {
                sprintf(message," is standing on %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else
            {
                sprintf(message," is standing in %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
        }
        else
            strcat( buf, " is here." );               
        break;
    case POS_FIGHTING:
        strcat( buf, " is here, fighting " );
        if ( victim->fighting == NULL )
            strcat( buf, "thin air??" );
        else if ( victim->fighting == ch )
            strcat( buf, "YOU!" );
        else if ( victim->in_room == victim->fighting->in_room )
        {
            strcat( buf, PERS( victim->fighting, ch ) );
            strcat( buf, "." );
        }
        else
            strcat( buf, "someone who left??" );
        break;
   }
   
   strcat( buf, "\n\r" );
   buf[0] = UPPER(buf[0]);
   send_to_char( buf, ch );
   return;
}

/*
char* get_disguise_name( CHAR_DATA *ch );
*/

char* char_look_info( CHAR_DATA *ch )
{
    static char buf[MSL];
    static char appear_str[10][100] =
    {
	"horrid",
	"disgusting",
	"ugly",
	"bad-looking",
	"average-looking",
	"good-looking",
	"pretty",
	"beautiful",
	"gorgeous",
	"astounding"
    };

    MOB_INDEX_DATA *mimic;
    struct race_type *race_type;
    int appearance, race, size, sex;
    /* Small hack to prevent female werewolves from being called 'wolfman' during daylight. */
    bool check_shewolf = ( ch->race == race_werewolf && ch->sex == SEX_FEMALE && 
             weather_info.sunlight != SUN_DARK && weather_info.sunlight != SUN_SET );

    appearance = (get_curr_stat(ch, STAT_CHA) - 1) / 20;
    if ( is_mimic(ch) )
    {
	if ( (mimic = get_mimic(ch)) == NULL )
	    return "!error!";

	/* mimic copies size, disguise don't */
	if ( is_affected(ch, gsn_mimic) )
	    size = mimic->size;
	else
	    size = ch->size;

	sex = mimic->sex;
	if ( sex == SEX_BOTH )
	    sex = SEX_NEUTRAL;

	race = mimic->race;
	race_type = &race_table[race];
    }
    else
    {
	race = ch->race;
	race_type = get_morph_race_type( ch );
	size = ch->size;
	sex = ch->sex;
    }

    /*
    if ( is_disguised(ch) )
	return get_disguise_name( ch );
    */

    sprintf( buf, "a %s %s %s %s",
	     size == SIZE_MEDIUM ? "medium-sized" : size_table[size].name,
	     appear_str[appearance],
	     sex == 0 ? "sexless" : sex_table[sex].name, 
	     race == 0 ? "being" : (check_shewolf ? "shewolf" : race_type->name) );
    
    return buf;
}


void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch, bool glance )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int i, tattoo;
    int percent;
    bool found;
    bool victim_is_obj = IS_NPC(victim) && IS_SET(victim->act, ACT_OBJ);
    
    if (ch == victim)
	act_see( "$n looks at $mself.",ch,NULL,NULL,TO_ROOM);
    else
    {
	act_see( "$n looks at you.", ch, NULL, victim, TO_VICT    );
	act_see( "$n looks at $N.",  ch, NULL, victim, TO_NOTVICT );
    }

    if ( !victim_is_obj )
    {
	/*
	if ( is_mimic(victim) )
	    send_to_char( "=== All mimicry!!! ===\n\r", ch );
	*/
	sprintf( buf, "%s is %s.\n\r", PERS(victim, ch), char_look_info(victim) );
	send_to_char( buf, ch );
    }

    if ( /*!is_disguised(victim) &&*/ !glance )
    {
	if ( is_mimic(victim) )
	{
	    MOB_INDEX_DATA *mimic = get_mimic(victim);
	    if ( mimic != NULL && mimic->description[0] != '\0' )
		send_to_char( mimic->description, ch );
	}
	else if ( victim->description[0] != '\0' )
	{
	    send_to_char( victim->description, ch );
	}
	else
	{
	    act( "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
	}
    }
    
    /* objects don't have health condition etc. to detect */
    if ( victim_is_obj )
	return;

    if ( victim->max_hit > 0 )
        percent = ( 100 * victim->hit ) / victim->max_hit;
    else
        percent = -1;
    
    strcpy( buf, PERS(victim, ch) );
    
    if (percent >= 100) 
        strcat( buf, " is in excellent condition.\n\r");
    else if (percent >= 90) 
        strcat( buf, " has a few scratches.\n\r");
    else if (percent >= 75) 
        strcat( buf," has some small wounds and bruises.\n\r");
    else if (percent >=  50)
        strcat( buf, " has quite a few wounds.\n\r");
    else if (percent >= 30)
        strcat( buf, " has some big nasty wounds and scratches.\n\r");
    else if (percent >= 15)
        strcat ( buf, " looks pretty hurt.\n\r");
    else if (percent >= 0 )
        strcat (buf, " is in awful condition.\n\r");
    else
        strcat(buf, " is bleeding to death.\n\r");
    
    buf[0] = UPPER(buf[0]);
    send_to_char( buf, ch );
    
    if ( !glance )
    {
	found = FALSE;
	for ( i = 0; i < MAX_WEAR; i++ )
	{
	    tattoo = get_tattoo_ch(victim, i);
	    if ( (obj = get_eq_char(victim, i)) != NULL
		 && can_see_obj(ch, obj) )
	    {
		if ( !found )
		{
		    send_to_char( "\n\r", ch );
		    act( "$N is using:", ch, NULL, victim, TO_CHAR );
		    found = TRUE;
		}
		send_to_char( where_name[i], ch );
		send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
		if ( CAN_WEAR(obj, ITEM_TRANSLUCENT) && tattoo != TATTOO_NONE )
		{
		    send_to_char( " above ", ch );
		    send_to_char( tattoo_desc(tattoo), ch );
		}
		send_to_char( "\n\r", ch );
	    }
	    else if ( tattoo != TATTOO_NONE )
	    {
		if ( !found )
		{
		    send_to_char( "\n\r", ch );
		    act( "$N is using:", ch, NULL, victim, TO_CHAR );
		    found = TRUE;
		}
		send_to_char( where_name[i], ch );
		send_to_char( tattoo_desc(tattoo), ch );
		send_to_char( "\n\r", ch );
		found = TRUE;
	    } 
	}
    }
    
    /* show spell affects */
    if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
    {
	send_to_char("\n\rYou detect the following spells:\n\r", ch);
	show_affects(victim, ch, FALSE, FALSE);
    }
    
    /*
    if ( victim != ch
        &&   !IS_NPC(ch)
        &&   number_percent( ) < get_skill(ch,gsn_peek))
    {
        send_to_char( "\n\rYou peek at the inventory:\n\r", ch );
        check_improve(ch,gsn_peek,TRUE,4);
        show_list_to_char( victim->carrying, ch, TRUE, TRUE );
    }
    */

    return;
}

bool check_see_eyes( CHAR_DATA *ch, CHAR_DATA *victim )
{
    bool success;

    if ( !IS_AFFECTED(victim, AFF_INFRARED) 
	 || can_see(ch, victim) )
	return FALSE;

    ch->in_room->light++;
    success = check_see( ch, victim );
    ch->in_room->light--;
    
    return success;
}

void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
    CHAR_DATA *rch;
    
    for ( rch = list; rch != NULL; rch = rch->next_in_room )
    {
        if ( rch == ch )
            continue;
        
        if ( get_trust(ch) < rch->invis_level)
            continue;
        
        if ( check_see( ch, rch ) )
        {
            show_char_to_char_0( rch, ch );
        }
        else if ( check_see_eyes(ch, rch) )
        {
            send_to_char( "You see glowing red eyes watching YOU!\n\r", ch );
        }
    }
    
    return;
} 

bool check_blind( CHAR_DATA *ch )
{
    
    if (!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT))
        return TRUE;
    
    if ( IS_AFFECTED(ch, AFF_BLIND) )
    { 
        send_to_char( "You can't see a thing!\n\r", ch ); 
        return FALSE; 
    }
    
    return TRUE;
}

void do_peek( CHAR_DATA *ch, char *argument )
{
    int chance;
    CHAR_DATA *victim;
    char buf[MSL];

    if ( IS_NPC(ch) )
	return;

    chance = get_skill( ch, gsn_peek );
    if ( chance < 1 )
    {
	send_to_char( "You don't know how to peek. Well, not in this way..\n\r", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Peek at whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, argument ) ) == NULL )
    {
	send_to_char( "Have to see them to peek at them.\n\r", ch );
	return;
    }

    chance += (ch->level - victim->level) / 2;
    if ( number_percent() > chance )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    check_improve(ch,gsn_peek,TRUE,4);
    /* money */
    sprintf( buf, "$N's wallet contains %d gold and %d silver coins.",
	     victim->gold, victim->silver );
    act( buf, ch, NULL, victim, TO_CHAR );
    /* inventory */
    act_see( "$n peeks at your inventory.", ch, NULL, victim, TO_VICT );
    act( "You peek at $N's inventory:", ch, NULL, victim, TO_CHAR );
    show_list_to_char( victim->carrying, ch, TRUE, TRUE );
}

/* changes your scroll */
void do_scroll(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;
    
    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        if (ch->lines == 0)
            send_to_char("You do not page long messages.\n\r",ch);
        else
        {
            sprintf(buf,"You currently display %d lines per page.\n\r",
                ch->lines + 2);
            send_to_char(buf,ch);
        }
        return;
    }
    
    if (!is_number(arg))
    {
        send_to_char("You must provide a number.\n\r",ch);
        return;
    }
    
    lines = atoi(arg);
    
    if (lines == 0)
    {
        send_to_char("Paging disabled.\n\r",ch);
        ch->lines = 0;
        return;
    }
    
    if (lines < 10 || lines > 100)
    {
        send_to_char("You must provide a reasonable number.\n\r",ch);
        return;
    }
    
    sprintf(buf,"Scroll set to %d lines.\n\r",lines);
    send_to_char(buf,ch);
    ch->lines = lines - 2;
}

/* execute a social - useful if there's a command with same name --Bobble */
void do_social(CHAR_DATA *ch, char *argument)
{
    char arg1[MIL];

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Which social do you want to perform?\n\r", ch );
	send_to_char( "(type 'socials' to get a list of all socials)\n\r", ch );
	return;
    }

    if ( !check_social(ch, arg1, argument) )
    {
	send_to_char( "That social doesn't exist.\n\r", ch );
	return;
    }
} 

/* RT does socials */
/* old do_socials
void do_socials(CHAR_DATA *ch, char *argument)
{
char buf[MAX_STRING_LENGTH];
int iSocial;
int col;

 col = 0;
 
  for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
  {
  sprintf(buf,"%-12s",social_table[iSocial].name);
  send_to_char(buf,ch);
  if (++col % 6 == 0)
  send_to_char("\n\r",ch);
  }
  
	  if ( col % 6 != 0)
      send_to_char("\n\r",ch);
      return;
} */

void do_socials(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;
    int iSocial;
    int col;
    
    output = new_buf();
    
    col = 0;
    
    for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
    {
        sprintf(buf,"%-12s",social_table[iSocial].name);
        add_buf(output,buf);
        if (++col % 6 == 0)
            add_buf(output, "\n\r");
    }
    
    if ( col % 6 != 0)
        add_buf(output, "\n\r");
    
    page_to_char(buf_string(output),ch);
    free_buf(output);
    
    return;
}


/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"motd");
}

void do_imotd(CHAR_DATA *ch, char *argument)
{  
    do_help(ch,"imotd");
}

void do_rules(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"rules");
}

void do_story(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"story");
}

/*
void do_wizlist(CHAR_DATA *ch, char *argument)
{
do_help(ch,"wizlist");
}
*/

/* RT this following section holds all the auto commands from ROM, as well as
replacements for config */

void do_autolist(CHAR_DATA *ch, char *argument)
{
    /* lists most player flags */
    if (IS_NPC(ch))
        return;
    
    send_to_char("{w  action       status{x\n\r",ch);
    send_to_char("{w---------------------{x\n\r",ch);
    
    send_to_char("autoassist     ",ch);
    if (IS_SET(ch->act,PLR_AUTOASSIST))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch); 
    
    send_to_char("autoexit       ",ch);
    if (IS_SET(ch->act,PLR_AUTOEXIT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    send_to_char("autogold       ",ch);
    if (IS_SET(ch->act,PLR_AUTOGOLD))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    send_to_char("autoloot       ",ch);
    if (IS_SET(ch->act,PLR_AUTOLOOT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    send_to_char("autosac        ",ch);
    if (IS_SET(ch->act,PLR_AUTOSAC))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    send_to_char("autosplit      ",ch);
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    send_to_char("autorescue     ",ch);
    if (IS_SET(ch->act,PLR_AUTORESCUE))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("compact mode   ",ch);
    if (IS_SET(ch->comm,COMM_COMPACT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    send_to_char("prompt         ",ch);
    if (IS_SET(ch->comm,COMM_PROMPT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    send_to_char("combine items  ",ch);
    if (IS_SET(ch->comm,COMM_COMBINE))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    if (IS_IMMORTAL(ch))
    {
        send_to_char("mudFTP         ",ch);
        if (IS_SET(ch->act,PLR_MUDFTP))
            send_to_char("ON\n\r",ch);
        else
            send_to_char("OFF\n\r",ch);
    }
    
    if (!IS_SET(ch->act,PLR_CANLOOT))
        send_to_char("Items you own are safe from thieves. (noloot)\n\r",ch);
    else 
        send_to_char("Items you own may be looted. (noloot)\n\r",ch);
    
    if (IS_SET(ch->act,PLR_NOSUMMON))
        send_to_char("You cannot be summoned. (nosum)\n\r",ch);
    else
        send_to_char("You can be summoned. (nosum)\n\r",ch);
    
    if (IS_SET(ch->act,PLR_NOCANCEL))
        send_to_char("You cannot be cancelled. (nocan)\n\r",ch);
    else
        send_to_char("You can be cancelled. (nocan)\n\r",ch);
    
    if (IS_SET(ch->act,PLR_NOFOLLOW))
        send_to_char("You do not welcome followers. (nofol)\n\r",ch);
    else
        send_to_char("You accept followers. (nofol)\n\r",ch);

    if (IS_SET(ch->act,PLR_NOLOCATE))
        send_to_char("You do not wish to be located. (noloc)\n\r",ch);
    else
        send_to_char("You wish to be located. (noloc)\n\r",ch);

    if (IS_SET(ch->act,PLR_NOACCEPT))
        send_to_char("You do not accept items from other players. (noacc)\n\r",ch);
    else
        send_to_char("You accept items from other players. (noacc)\n\r",ch);

    if (IS_SET(ch->act,PLR_NOSURR))
        send_to_char("You do not accept surrenders from other players. (nosurr)\n\r",ch);
    else
        send_to_char("You accept surrenders from other players. (nosurr)\n\r",ch);

    if (IS_SET(ch->act,PLR_NOEXP))
        send_to_char("You do not wish to gain experience points. (noexp)\n\r",ch);
    else
        send_to_char("You can gain experience points. (noexp)\n\r",ch);
}

void do_autoassist(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_AUTOASSIST))
    {
        send_to_char("Autoassist removed.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTOASSIST);
    }
    else
    {
        send_to_char("You will now assist when needed.\n\r",ch);
        SET_BIT(ch->act,PLR_AUTOASSIST);
    }
}

void do_autoexit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_AUTOEXIT))
    {
        send_to_char("Exits will no longer be displayed.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTOEXIT);
    }
    else
    {
        send_to_char("Exits will now be displayed.\n\r",ch);
        SET_BIT(ch->act,PLR_AUTOEXIT);
    }
}

void do_autogold(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_AUTOGOLD))
    {
        send_to_char("Autogold removed.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTOGOLD);
    }
    else
    {
        send_to_char("Automatic gold looting set.\n\r",ch);
        SET_BIT(ch->act,PLR_AUTOGOLD);
    }
}

void do_autoloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_AUTOLOOT))
    {
        send_to_char("Autolooting removed.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTOLOOT);
    }
    else
    {
        send_to_char("Automatic corpse looting set.\n\r",ch);
        SET_BIT(ch->act,PLR_AUTOLOOT);
    }
}

void do_autosac(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_AUTOSAC))
    {
        send_to_char("Autosacrificing removed.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTOSAC);
    }
    else
    {
        send_to_char("Automatic corpse sacrificing set.\n\r",ch);
        SET_BIT(ch->act,PLR_AUTOSAC);
    }
}

void do_autosplit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
    {
        send_to_char("Autosplitting removed.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTOSPLIT);
    }
    else
    {
        send_to_char("Automatic gold splitting set.\n\r",ch);
        SET_BIT(ch->act,PLR_AUTOSPLIT);
    }
}

void do_autorescue(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_AUTORESCUE))
    {
        send_to_char("Autorescue removed.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTORESCUE);
    }
    else
    {
        send_to_char("Your will now protect your friends.\n\r",ch);
        SET_BIT(ch->act,PLR_AUTORESCUE);
    }
}

void do_brief(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_BRIEF))
    {
        send_to_char("Full descriptions activated.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_BRIEF);
    }
    else
    {
        send_to_char("Short descriptions activated.\n\r",ch);
        SET_BIT(ch->comm,COMM_BRIEF);
    }
}

void do_compact(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMPACT))
    {
        send_to_char("Compact mode removed.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_COMPACT);
    }
    else
    {
        send_to_char("Compact mode set.\n\r",ch);
        SET_BIT(ch->comm,COMM_COMPACT);
    }
}

/* new show by Quirky */
void do_show(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    
    one_argument(argument, arg);
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Show what in your score?\n\r"
            "Syntax:  show <worth|attributes|percentages|affects>\n\r", ch );
        return;
    }
    
    if ( !str_cmp( arg, "affects" ) || !str_cmp( arg, "aff" ) )
    {
        if (IS_SET(ch->comm,COMM_SHOW_AFFECTS))
        {
            send_to_char("Affects will no longer be shown in score.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_SHOW_AFFECTS);
        }
        else
        {
            send_to_char("Affects will now be shown in score.\n\r",ch);
            SET_BIT(ch->comm,COMM_SHOW_AFFECTS);
        }
        return;
    }
    else if ( !str_cmp( arg, "worth" ))
    {
        if (IS_SET(ch->comm,COMM_SHOW_WORTH))
        {
            send_to_char("Worth will no longer be shown in score.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_SHOW_WORTH);
        }
        else
        {
            send_to_char("Worth will now be shown in score.\n\r",ch);
            SET_BIT(ch->comm,COMM_SHOW_WORTH);
        }
        return;
    }
    else if ( !str_cmp( arg, "att" ) ||  !str_cmp( arg, "attributes" ))
    {
        if (IS_SET(ch->comm,COMM_SHOW_ATTRIB))
        {
            send_to_char("Attributes will no longer be shown in  score.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_SHOW_ATTRIB);
        }
        else
        {
            send_to_char("Attributes will now be shown in score.\n\r",ch);
            SET_BIT(ch->comm,COMM_SHOW_ATTRIB);
        }
        return;
    }
    else if ( !str_cmp(arg, "per") || !str_cmp(arg, "percent") || !str_cmp(arg, "percentages") )
    {
        if ( IS_SET(ch->comm, COMM_SHOW_PERCENT) )
        {
            send_to_char("Percentages will no longer be shown in  score.\n\r",ch);
            REMOVE_BIT(ch->comm, COMM_SHOW_PERCENT);
        }
        else
        {
            send_to_char("Percentages will now be shown in score.\n\r",ch);
            SET_BIT(ch->comm, COMM_SHOW_PERCENT);
        }
        return;
    }
    else
    {
        send_to_char( "Show worth, attributes, percentages or affects in your score?\n\r", ch );
        send_to_char( "Syntax:  show <worth|att|per|aff>\n\r", ch );
        return;
    }
}

void do_mudftp(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (ch->pcdata->security < 1)
    {
        send_to_char("Insufficient OLC security.\n\r",ch);
        return;
    }
    
    if (IS_SET(ch->act,PLR_MUDFTP))
    {
        send_to_char("You will now use the normal editor to edit strings.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_MUDFTP);
    }
    else
    {
        send_to_char("You will now use the mudFTP protocol to edit strings.\n\r",ch);
        SET_BIT(ch->act,PLR_MUDFTP);
    }
}

#define MAX_PROMPT_LENGTH 110
void do_prompt(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char *temp;
    int chars = 0, noncol = 0;
    
    if ( argument[0] == '\0' )
    {
        if (IS_SET(ch->comm,COMM_PROMPT))
        {
            send_to_char("You will no longer see prompts.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_PROMPT);
        }
        else
        {
            send_to_char("You will now see prompts.\n\r",ch);
            SET_BIT(ch->comm,COMM_PROMPT);
        }
        return;
    }
    
    if( !strcmp( argument, "all" ) )
        strcpy( buf, PROMPT_DEFAULT );     
    else
    {
        if ( strlen_color(argument) > MAX_PROMPT_LENGTH )
        {
            for( temp = argument; *temp != '/0'; temp++ )
            {
                chars++;
                if( *temp == '{' )
                    noncol--;
                else noncol++;
                if( noncol > MAX_PROMPT_LENGTH )  break;
            }
            argument[chars] = '\0';

        }
        
        strcpy( buf, argument );
        smash_tilde( buf );
        if (str_suffix("%c",buf))
            strcat(buf," ");
    }
    
    free_string( ch->prompt );
    ch->prompt = str_dup( buf );
    sprintf(buf,"Prompt set to %s\n\r",ch->prompt );
    send_to_char(buf,ch);
    return;
}

void do_combine(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMBINE))
    {
        send_to_char("Long inventory selected.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_COMBINE);
    }
    else
    {
        send_to_char("Combined inventory selected.\n\r",ch);
        SET_BIT(ch->comm,COMM_COMBINE);
    }
}

void do_noloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_CANLOOT))
    {
        send_to_char("Items owned by you (ie. your corpse) now cannot be looted.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_CANLOOT);
    }
    else
    {
        send_to_char("Items owned by you (ie. your corpse) may now be picked up by anyone.\n\r",ch);
        SET_BIT(ch->act,PLR_CANLOOT);
    }
}

void do_nofollow(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_NOFOLLOW))
    {
        send_to_char("You now accept followers.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    }
    else
    {
        send_to_char("You no longer accept followers.\n\r",ch);
        SET_BIT(ch->act,PLR_NOFOLLOW);
		if (!IS_AFFECTED(ch, AFF_CHARM))
			die_follower( ch );
    }
}

void do_nosummon(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
        if (IS_SET(ch->imm_flags,IMM_SUMMON))
        {
            send_to_char("You are no longer immune to summon.\n\r",ch);
            REMOVE_BIT(ch->imm_flags,IMM_SUMMON);
        }
        else
        {
            send_to_char("You are now immune to summoning.\n\r",ch);
            SET_BIT(ch->imm_flags,IMM_SUMMON);
        }
    }
    else
    {
        if (IS_SET(ch->act,PLR_NOSUMMON))
        {
            send_to_char("You are no longer immune to summon.\n\r",ch);
            REMOVE_BIT(ch->act,PLR_NOSUMMON);
        }
        else
        {
            send_to_char("You are now immune to summoning.\n\r",ch);
            SET_BIT(ch->act,PLR_NOSUMMON);
        }
    }
}

void do_nocancel(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    else
    {
        if (IS_SET(ch->act,PLR_NOCANCEL))
        {
            send_to_char("You are no longer immune to cancellation.\n\r",ch);
            REMOVE_BIT(ch->act,PLR_NOCANCEL);
        }
        else
        {
            send_to_char("You are now immune to cancellation.\n\r",ch);
            SET_BIT(ch->act,PLR_NOCANCEL);
        }
    }
}

void do_nolocate(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    else
    {
        if (IS_SET(ch->act,PLR_NOLOCATE))
        {
            send_to_char("You wish to be located again.\n\r",ch);
            REMOVE_BIT(ch->act,PLR_NOLOCATE);
        }
        else
        {
            send_to_char("You no longer wish to be located.\n\r",ch);
            SET_BIT(ch->act,PLR_NOLOCATE);
        }
    }
}

void do_noaccept( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
        return;
    else
    {
        if (IS_SET(ch->act,PLR_NOACCEPT))
        {
            send_to_char("You now accept items from other players.\n\r",ch);
            REMOVE_BIT(ch->act,PLR_NOACCEPT);
        }
        else
        {
            send_to_char("You no longer accept items from other players.\n\r",ch);
            SET_BIT(ch->act,PLR_NOACCEPT);
        }
    }    
}

void do_nosurrender( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
        return;
    else
    {
        if (IS_SET(ch->act,PLR_NOSURR))
        {
            send_to_char("You now accept surrenders from other players.\n\r",ch);
            REMOVE_BIT(ch->act,PLR_NOSURR);
        }
        else
        {
            send_to_char("You no longer accept surrenders from other players.\n\r",ch);
            SET_BIT(ch->act,PLR_NOSURR);
        }
    }    
}

/* Lets players disable exp gains so that they can stay
   at a constant level - Astark 2-18-13 */

void do_noexp( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
        return;
    else
    {
        if (IS_SET(ch->act,PLR_NOEXP))
        {
            send_to_char("You can now gain experience points.\n\r",ch);
            REMOVE_BIT(ch->act,PLR_NOEXP);
        }
        else
        {
            send_to_char("You will no longer be able to gain experience points.\n\r",ch);
            SET_BIT(ch->act,PLR_NOEXP);
        }
    }    
}

/* added due to popular demand --Bobble */
void do_glance( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Glance at whom?\n\r", ch );
	return;
    }

    victim = get_char_room( ch, argument );
    if ( victim == NULL || !can_see(ch, victim) )
	send_to_char( "You don't see them here.\n\r", ch );
    else
        show_char_to_char_1( victim, ch, TRUE );
}

/* part of do_look: returns the object looked at if any */
OBJ_DATA* look_obj( CHAR_DATA *ch, char *argument )
{
    static char arg3[MSL];
    int number = number_argument(argument, arg3);
    int count = 0;
    OBJ_DATA *obj;
    char *pdesc;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {  /* player can see object */
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL )
                if (++count == number)
                {
                    if ( op_act_trigger(obj, ch, NULL, arg3, OTRIG_LOOK) )
                        send_to_char( pdesc, ch );
                    return obj;
                }
                else continue;
                
	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
		if (++count == number)
        {   
            if ( op_act_trigger(obj, ch, NULL, arg3, OTRIG_LOOK) )
                send_to_char( pdesc, ch );
            return obj;
		}
		else continue;
	    
	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
            if ( op_act_trigger(obj, ch, NULL, arg3, OTRIG_LOOK) )
            {
		        send_to_char( obj->description, ch );
		        send_to_char( "\n\r",ch);
            }
		    return obj;
		}
        }
    }
    
    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL )
                if (++count == number)
                {
                    if ( op_act_trigger(obj, ch, NULL, arg3, OTRIG_LOOK) )
                        send_to_char( pdesc, ch );
                    return obj;
                }
                
	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
		if (++count == number)
        {
            if ( op_act_trigger(obj, ch, NULL, arg3, OTRIG_LOOK) )
    		    send_to_char( pdesc, ch );
		    return obj;
		}
	    
	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
            if ( op_act_trigger(obj, ch, NULL, arg3, OTRIG_LOOK) )
            {
                send_to_char( obj->description, ch );
		        send_to_char("\n\r",ch);
            }
		    return obj;
		}
        }
    }

    return NULL;
}

void do_look( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char *pdesc;
    int door;
    int number,count;
    
    if ( ch->desc == NULL )
        return;
    
    if ( ch->position < POS_SLEEPING )
    {
        send_to_char( "You can't see anything but stars!\n\r", ch );
        return;
    }
    
    if ( ch->position == POS_SLEEPING )
    {
        send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
        return;
    }
    
    if ( !check_blind( ch ) )
        return;
    
    /* player might still be able to look at mobs with infravision --Bobble
    if ( !IS_NPC(ch)
	 && !IS_SET(ch->act, PLR_HOLYLIGHT)
	 && !IS_AFFECTED(ch, AFF_DARK_VISION)
	 && room_is_dark( ch->in_room ) )
    {
        send_to_char( "It is pitch black ... \n\r", ch );
        // added so that objects (i.e. glowing ones) are shown 
        show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE );
        show_char_to_char( ch->in_room->people, ch );
        return;
    }
    */
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    number = number_argument(arg1,arg3);
    count = 0;
    
    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
    {

	if ( !IS_NPC(ch)
	     && !IS_SET(ch->act, PLR_HOLYLIGHT)
	     && !IS_AFFECTED(ch, AFF_DARK_VISION)
	     && room_is_dark( ch->in_room ) )
	{
	    send_to_char( "It is pitch black ... \n\r", ch );
	    // added so that objects (i.e. glowing ones) are shown 
	    show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE );
	    show_char_to_char( ch->in_room->people, ch );
	    return;
	}

        /* 'look' or 'look auto' */
        sprintf( buf, "{o%s", ch->in_room->name );
        send_to_char( buf,ch);
        
        if ( IS_IMMORTAL(ch) || IS_BUILDER(ch, ch->in_room->area) )
        {
            sprintf( buf," [Room %d %s]", ch->in_room->vnum,
		     flag_bit_name(sector_flags, ch->in_room->sector_type) );
            send_to_char(buf,ch);
	    /*
	    if ( PLR_ACT(ch, PLR_HOLYLIGHT) )
	    {
		sprintf( buf,"\n\r[Sector: %s  Flags: %s]",
			 flag_bit_name(sector_flags, ch->in_room->sector_type),
			 flag_bits_name(room_flags, ch->in_room->room_flags) );
		send_to_char(buf,ch);
	    }
	    */
        }
        
        send_to_char( "{x\n\r", ch );
        
        if ( arg1[0] == '\0'
            || ( !IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF) ) )
        {
            send_to_char( "  ",ch);
            send_to_char( ch->in_room->description, ch );
        }
        
        if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT) )
        {
            send_to_char("\n\r",ch);
            do_exits( ch, "auto" );
        }
        
        show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE );
        show_char_to_char( ch->in_room->people,   ch );
        
        if (!IS_NPC(ch) && ch->hunting &&number_percent()<=get_skill(ch,gsn_stalk))
        {
            count=ch->wait;
            do_hunt(ch, ch->hunting);
	    ignore_invisible = FALSE;
            ch->wait=count;
            check_improve(ch,gsn_stalk,TRUE,4);
        }
        
        return;
    }
    
    if ( !str_cmp( arg1, "i" ) || !str_cmp(arg1, "in")  || !str_cmp(arg1,"on"))
    {
        /* 'look in' */
        if ( arg2[0] == '\0' )
        {
            send_to_char( "Look in what?\n\r", ch );
            return;
        }
        
        if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
        {
            send_to_char( "You do not see that here.\n\r", ch );
            return;
        }
        
        switch ( obj->item_type )
        {
        default:
            send_to_char( "That is not a container.\n\r", ch );
            break;
            
        case ITEM_DRINK_CON:
            if ( obj->value[1] <= 0 )
            {
                send_to_char( "It is empty.\n\r", ch );
                break;
            }
            
	    /*
            sprintf( buf, "It's %sfilled with a %s liquid.\n\r",
                obj->value[1] < obj->value[0] / 4
                ? "less than half-" :
            obj->value[1] < 3 * obj->value[0] / 4
                ? "about half-"     : "more than half-",
                liq_table[obj->value[2]].liq_color
                );
	    */
	    sprintf( buf, "It's filled with %d out of %d units of %s liquid.\n\r",
		     obj->value[1], obj->value[0],
		     liq_table[obj->value[2]].liq_color );

            send_to_char( buf, ch );
            break;
            
        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            if ( I_IS_SET(obj->value[1], CONT_CLOSED) )
            {
                send_to_char( "It is closed.\n\r", ch );
                break;
            }
            
            act( "$p holds:", ch, obj, NULL, TO_CHAR );
            show_list_to_char( obj->contains, ch, TRUE, TRUE );
            /* Show item count in storage boxes*/
            if (obj->pIndexData->vnum == OBJ_VNUM_STORAGE_BOX)
            {
                sh_int num_items=get_obj_number(obj);
                printf_to_char(ch,"\n\r%d %s.\n\r",num_items,
                        num_items==1?"item":"items");
            }
            break;
        }
        return;
    }
    
    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
        show_char_to_char_1( victim, ch, FALSE );
        return;
    }
    
    if ( look_obj(ch, arg1) != NULL )
	return;
    /*
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL )
                if (++count == number)
                {
                    send_to_char( pdesc, ch );
                    return;
                }
                else continue;
                
	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
		if (++count == number)
                {   
		    send_to_char( pdesc, ch );
		    return;
		}
		else continue;
	    
	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
		    send_to_char( obj->description, ch );
		    send_to_char( "\n\r",ch);
		    return;
		}
        }
    }
    
    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL )
                if (++count == number)
                {
                    send_to_char( pdesc, ch );
                    return;
                }
                
	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
		if (++count == number)
                {
		    send_to_char( pdesc, ch );
		    return;
		}
	    
	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
		    send_to_char( obj->description, ch );
		    send_to_char("\n\r",ch);
		    return;
		}
        }
    }
    */
    
    pdesc = get_extra_descr(arg3,ch->in_room->extra_descr);
    if (pdesc != NULL)
    {
        if (++count == number)
        {
            send_to_char(pdesc,ch);
            return;
        }
    }
    
    if (count > 0 && count != number)
    {
        if (count == 1)
            sprintf(buf,"You only see one %s here.\n\r",arg3);
        else
            sprintf(buf,"You only see %d of those here.\n\r",count);
        
        send_to_char(buf,ch);
        return;
    }
    
    if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) ) door = DIR_NORTH;
	   else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east"  ) ) door = DIR_EAST;
       else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) ) door = DIR_SOUTH;
       else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west"  ) ) door = DIR_WEST;
       else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up"    ) ) door = DIR_UP;
       else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down"  ) ) door = DIR_DOWN;
       else if ( !str_cmp( arg1, "ne" ) || !str_cmp( arg1, "northeast"  ) ) door = DIR_NORTHEAST;
       else if ( !str_cmp( arg1, "se" ) || !str_cmp( arg1, "southeast"  ) ) door = DIR_SOUTHEAST;
       else if ( !str_cmp( arg1, "sw" ) || !str_cmp( arg1, "southwest"  ) ) door = DIR_SOUTHWEST;
       else if ( !str_cmp( arg1, "nw" ) || !str_cmp( arg1, "northwest"  ) ) door = DIR_NORTHWEST;
       
       else
       {
           send_to_char( "You do not see that here.\n\r", ch );
           return;
       }
       
       /* 'look direction' */
       if ( ( pexit = ch->in_room->exit[door] ) == NULL )
       {
           send_to_char( "Nothing special there.\n\r", ch );
           return;
       }
       
       if ( pexit->description != NULL && pexit->description[0] != '\0' )
           send_to_char( pexit->description, ch );
       else
           send_to_char( "Nothing special there.\n\r", ch );
       
       if ( pexit->keyword    != NULL
           &&   pexit->keyword[0] != '\0'
           &&   pexit->keyword[0] != ' ' )
       {
           if ( IS_SET(pexit->exit_info, EX_CLOSED) )
           {
               act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
           }
           else if ( IS_SET(pexit->exit_info, EX_ISDOOR) )
           {
               act( "The $d is open.",   ch, NULL, pexit->keyword, TO_CHAR );
           }
       }
       
       return;
}

/* RT added back for the hell of it */
void do_read (CHAR_DATA *ch, char *argument )
{
    do_look(ch,argument);
}

void do_examine( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Examine what?\n\r", ch );
        return;
    }
    
    if ( (obj = look_obj(ch, arg)) == NULL )
    {
	send_to_char( "You don't see that here.\n\r", ch );
	return;
    }

    if (obj->owner != NULL)
    {
        sprintf( buf, "A marking says it is owned by %s.\n\r", obj->owner);
        send_to_char( buf, ch );
    }

    switch ( obj->item_type )
    {
    default:
	break;
	
    case ITEM_ARROWS:
	sprintf( buf, "There are %d arrows in this pack.\n\r", obj->value[0] );
	send_to_char( buf, ch );
	break;

    case ITEM_MONEY:
	if (obj->value[0] == 0)
        {
	    if (obj->value[1] == 0)
		sprintf(buf,"Odd...there's no coins in the pile.\n\r");
	    else if (obj->value[1] == 1)
		sprintf(buf,"Wow. One gold coin.\n\r");
	    else
		sprintf(buf,"There are %d gold coins in the pile.\n\r",
			obj->value[1]);
	}
	else if (obj->value[1] == 0)
        {
	    if (obj->value[0] == 1)
		sprintf(buf,"Wow. One silver coin.\n\r");
	    else
		sprintf(buf,"There are %d silver coins in the pile.\n\r",
			obj->value[0]);
	}
	else
	    sprintf(buf,
		    "There are %d gold and %d silver coins in the pile.\n\r",
		    obj->value[1],obj->value[0]);
	send_to_char(buf,ch);
	break;
	
        case ITEM_DRINK_CON:
        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            sprintf(buf,"in %s",argument);
            do_look( ch, buf );
	    break;

	case ITEM_WEAPON:
        strcpy(buf, "It is ");
        switch (obj->value[0])
        {
        case(WEAPON_EXOTIC) : strcat(buf, "a weapon of some exotic type, ");  break;
        case(WEAPON_SWORD)  : strcat(buf, "a sword, ");  break;  
        case(WEAPON_DAGGER) : strcat(buf, "a dagger, "); break;
        case(WEAPON_SPEAR)  : strcat(buf, "a spear or staff, "); break;
        case(WEAPON_MACE)   : strcat(buf, "a mace or club, ");   break;
        case(WEAPON_AXE)    : strcat(buf, "an axe, ");       break;
        case(WEAPON_FLAIL)  : strcat(buf, "a flail, ");  break;
        case(WEAPON_WHIP)   : strcat(buf, "a whip, ");       break;
        case(WEAPON_POLEARM): strcat(buf, "a polearm, ");    break;
        case(WEAPON_GUN)    : strcat(buf, "a gun, ");    break;
        case(WEAPON_BOW)    : strcat(buf, "a bow, ");    break;
        default             : strcat(buf, "a weapon of some unknown type, "); break;
        }
	if( obj->weight < 10 ) strcat(buf, "and it is very lightweight.\n\r");
	 else if( obj->weight < 40 ) strcat(buf, "and it is relatively lightweight.\n\r");
	 else if( obj->weight < 70 ) strcat(buf, "and it is easy to carry.\n\r");
	 else if( obj->weight < 100 ) strcat(buf, "and it is of medium weight.\n\r");
	 else if( obj->weight < 180 ) strcat(buf, "and it has a good weight to it.\n\r");
	 else if( obj->weight < 260 ) strcat(buf, "and it is quite massive.\n\r");
	 else strcat(buf, "and it is extremely heavy.\n\r");
	send_to_char(buf,ch);
	break;

	case ITEM_ARMOR:
	strcpy(buf, "It looks like it could be ");

	if( CAN_WEAR(obj,ITEM_WEAR_FINGER) )
	    strcat(buf, "worn on the finger.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_NECK) )
	    strcat(buf, "worn around the neck.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_TORSO) )
	    strcat(buf, "worn on the torso.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_HEAD) )
	    strcat(buf, "worn on the head.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_LEGS) )
	    strcat(buf, "worn on the legs.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_FEET) )
	    if( obj->pIndexData->vnum != 10415 )	/* Shaft's Lost Left Shoe! */
	        strcpy(buf, "It looks like they can be worn on the feet.\n\r");
	    else
		strcat(buf, "worn on the left foot.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_HANDS) )
	    strcpy(buf, "It looks like they can be worn on the hands.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_ARMS) )
	    strcpy(buf, "It looks like they can be worn on the arms.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_SHIELD) )
	    strcat(buf, "used as a shield.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_ABOUT) )
	    strcat(buf, "worn about the body.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_WAIST) )
	    strcat(buf, "worn about the waist.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_WRIST) )
	    strcat(buf, "worn on the wrist.\n\r");
	else if( CAN_WEAR(obj,ITEM_WEAR_FLOAT) )
	    strcat(buf, "something that would float nearby.\n\r");
	else if( CAN_WEAR(obj,ITEM_HOLD) )
	    strcat(buf, "held in the hand, to focus magic.\n\r");
	else
	    strcat(buf, "used as armor, but you can't figure out how.\n\r");
	send_to_char(buf,ch);
	break;
    }
}



/*
* Thanks to Zrin for auto-exit part.
*/
void do_exits( CHAR_DATA *ch, char *argument )
{
    extern char * const dir_name[];
    char buf[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    int door;
    
    fAuto  = !str_cmp( argument, "auto" );
    
    if ( !check_blind( ch ) )
        return;
    
    if (fAuto)
        sprintf(buf,"{O[Exits:");
    else if (IS_IMMORTAL(ch))
        sprintf(buf,"Obvious exits from room %d:\n\r",ch->in_room->vnum);
    else
        sprintf(buf,"Obvious exits:\n\r");
    
    found = FALSE;
    for ( door = 0; door < MAX_DIR; door++ )
    {
        if ( ( pexit = ch->in_room->exit[door] ) != NULL
            &&   pexit->u1.to_room != NULL
            &&   can_see_room(ch,pexit->u1.to_room) 
            &&   (!IS_SET(pexit->exit_info, EX_CLOSED) || IS_IMMORTAL(ch) ) )
        {
            found = TRUE;
            if ( fAuto )
            {
                if (IS_SET(pexit->exit_info, EX_CLOSED))
                {
                    sprintf( buf, "%s (%s)", buf, dir_name[door] );
                }
                else
                {
                    sprintf( buf, "%s %s", buf, dir_name[door] );
                }
            }
            else
            {
                sprintf( buf + strlen(buf), "%-5s - %s",
                    capitalize( dir_name[door] ),
                    room_is_dark( pexit->u1.to_room )
                    ?  "Too dark to tell"
                    : pexit->u1.to_room->name
                    );
                if (IS_IMMORTAL(ch))
                    sprintf(buf + strlen(buf), 
                    " (room %d)\n\r",pexit->u1.to_room->vnum);
                else
                    sprintf(buf + strlen(buf), "\n\r");
            }
        }
    }
    
    if ( !found )
        strcat( buf, fAuto ? " none" : "None.\n\r" );
    
    if ( fAuto )
        strcat( buf, "]\n\r" );
    
    send_to_char( buf, ch );
    send_to_char( "{x", ch);
    return;
}


/* displays the affects on ch to to_ch */
void show_affects(CHAR_DATA *ch, CHAR_DATA *to_ch, bool show_long, bool show_all)
{
    AFFECT_DATA *paf, *paf_last = NULL;
    char buf[MAX_STRING_LENGTH];
    
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if ( !show_all && !IS_SPELL(paf->type) )
            continue;

        if (paf_last != NULL && paf->type == paf_last->type)
            if (show_long)
                printf_to_char( to_ch, "                      ");
            else
                continue;
        else
            printf_to_char( to_ch, "Spell: %-15s", skill_table[paf->type].name );

        if (show_long)
        {
            printf_to_char( to_ch, ": modifies %s by %d ", affect_loc_name( paf->location ), paf->modifier);
            if ( paf->duration == -1 )
                printf_to_char( to_ch, "indefinitely (Lvl %d)", paf->level );
            else
                // color-code spells about to expire
                printf_to_char( to_ch, "for %s%d hours{x (Lvl %d)", paf->duration < 5 ? "{R" : paf->duration < 10 ? "{y" : "", paf->duration, paf->level );
            if ( paf->type == gsn_mirror_image || paf->type == gsn_phantasmal_image )
                printf_to_char( to_ch, " with %d %s remaining", paf->bitvector, paf->bitvector == 1 ? "image" : "images");
        }

        send_to_char( "\n\r", to_ch );
        paf_last = paf;
    }
}

void do_affects( CHAR_DATA *ch, char *argument )
{
    int leadership = get_leadership_bonus(ch, FALSE);
    
    if ( ch->affected != NULL )
    {
        send_to_char( "You are affected by the following spells:\n\r", ch );
        show_affects(ch, ch, ch->level >= 5, TRUE);
    }
    else 
        send_to_char("You are not affected by any spells.\n\r",ch);

    if (ch->leader != NULL && ch->leader != ch && ch->leader->in_room == ch->in_room)
        printf_to_char(ch, "You receive a %d%% damage %s from %s's leadership.\n\r",
            leadership,
            leadership >= 0 ? "bonus" : "penalty",
            PERS(ch->leader, ch)
        );
}

void do_leadership( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *fol;
    bool found = FALSE;
    
    if (ch->in_room == NULL)
        return;
    
    for ( fol = ch->in_room->people; fol != NULL; fol = fol->next_in_room )
        if ( fol != ch && fol->leader == ch )
        {
            int leadership = get_leadership_bonus(fol, FALSE);
            found = TRUE;
            printf_to_char(ch, "Your leadership grants a %d%% damage %s to %s.\n\r",
                leadership,
                leadership >= 0 ? "bonus" : "penalty",
                PERS(fol, ch)
            );
        }
    
    if (!found)
        send_to_char("You don't have any followers here.\n\r", ch);

    int max = cha_max_follow(ch);
    int charmed = cha_cur_follow(ch);
    printf_to_char(ch, "You control %d / %d levels worth of creatures (%d remaining).\n\r", charmed, max, max-charmed);
}

char *  const   day_name    [] =
{
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
        "the Great Gods", "the Sun"
};

char *  const   month_name  [] =
{
    "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
        "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
        "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
        "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

void do_time( CHAR_DATA *ch, char *argument )
{
    extern char str_boot_time[];
    char buf[MAX_STRING_LENGTH];
    char *suf;
    int day;
    
    day     = time_info.day + 1;
    
    if ( day > 4 && day <  20 ) suf = "th";
	   else if ( day % 10 ==  1       ) suf = "st";
       else if ( day % 10 ==  2       ) suf = "nd";
       else if ( day % 10 ==  3       ) suf = "rd";
       else                             suf = "th";
       
       sprintf( buf,
           "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\r",
           (time_info.hour % 12 == 0) ? 12 : time_info.hour %12,
           time_info.hour >= 12 ? "pm" : "am",
           day_name[day % 7],
           day, suf,
           month_name[time_info.month]);
       send_to_char(buf,ch);
       sprintf(buf,"Aarchon started up at %s\n\rThe system time is %s\n\r",
           str_boot_time,
           (char *) ctime( &current_time )
           );
       
       send_to_char( buf, ch );
       return;
}



void do_weather( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    
    static char * const sky_look[4] =
    {
        "cloudless",
            "cloudy",
            "rainy",
            "lit by flashes of lightning"
    };
    
    if ( !IS_OUTSIDE(ch) )
    {
        send_to_char( "You can't see the weather indoors.\n\r", ch );
        return;
    }
    
    sprintf( buf, "The sky is %s and %s.\n\r",
        sky_look[weather_info.sky],
        weather_info.change >= 0
        ? "a warm southerly breeze blows"
        : "a cold northern gust blows"
        );
    send_to_char( buf, ch );
    return;
}

bool is_command( char *arg )
{
    int cmd;
    
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
        if ( UPPER(arg[0]) == UPPER(cmd_table[cmd].name[0])
            && is_exact_name( cmd_table[cmd].name, arg ) )
            return TRUE;
        
        return FALSE;
}

#if 0
void do_help( CHAR_DATA *ch, char *argument )
{
    HELP_DATA *pHelp;
    BUFFER *output;
    bool found = FALSE;
    char argall[MAX_INPUT_LENGTH],argone[MAX_INPUT_LENGTH];
    int level;
    
    output = new_buf();
    
    if ( argument[0] == '\0' )
        argument = "summary";
    
    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    
    while (argument[0] != '\0' )
    {
        argument = one_argument(argument,argone);
        if (argall[0] != '\0')
            strcat(argall," ");
        strcat(argall,argone);
    }
    
    for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
    {
        level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;
        
        /* If a mortal has been granted access to an imm command, allow them to
        view the associated help file.  Otherwise, the level rules apply. */
        if ( level > get_trust( ch )
            && ( ( level <  LEVEL_IMMORTAL && !is_granted_name(ch,pHelp->keyword) )
            ||   ( level >= LEVEL_IMMORTAL && !is_command( pHelp->keyword ) ) ) )
            continue;
        
        if ( level >= LEVEL_IMMORTAL 
            && is_command( pHelp->keyword )
            && !is_granted_name(ch, pHelp->keyword) ) 
            continue;
        
        if ( is_name(argall, pHelp->keyword) && pHelp->delete == FALSE )
        {
            /* add seperator if found */
            if (found)
                add_buf(output,
                "\n\r============================================================\n\r\n\r");
            if ( pHelp->level >= 0 && str_cmp( argall, "imotd" ) )
            {
                add_buf(output,pHelp->keyword);
                add_buf(output,"\n\r");
            }
            
            /*
            * Strip leading '.' to allow initial blanks.
            */
            if ( pHelp->text[0] == '.' )
                add_buf(output,pHelp->text+1);
            else
                add_buf(output,pHelp->text);
            found = TRUE;
            
            /* small hack :) */
            if (ch->desc != NULL 
                && ch->desc->connected != CON_PLAYING 
                && !IS_WRITING_NOTE(ch->desc->connected) 
                && ch->desc->connected != CON_GEN_GROUPS)
                break;
        }
    }
    
    if (!found)
        send_to_char( "No help on that word.\n\r", ch );
    else
        page_to_char(buf_string(output),ch);
    free_buf(output);
}
#endif

/* searches for a unique help_data
 * if not found or not unique prints info to output and returns NULL
 */
HELP_DATA* find_help_data( CHAR_DATA *ch, char *argument, BUFFER *output )
{
    HELP_DATA *pHelp, *firstHelp = NULL;
    char argall[MAX_INPUT_LENGTH], argone[MAX_INPUT_LENGTH], arg[MIL];
    int level, number, count = 0;
    bool exact_number = FALSE;
    
    number = number_argument( argument, arg );
    if ( strcmp(argument, arg) )
    {
	exact_number = TRUE;
	argument = arg;
    }

    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    
    while (argument[0] != '\0' )
    {
        argument = one_argument(argument,argone);
        if (argall[0] != '\0')
            strcat(argall," ");
        strcat(argall,argone);
    }

    for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
    {
        level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;
        
        /* If a mortal has been granted access to an imm command, allow them to
        view the associated help file.  Otherwise, the level rules apply. */
        if ( level > get_trust( ch ) )
            continue;
 	/* creates problems --Bobble
        if ( level > get_trust( ch )
	     && ( (level < LEVEL_IMMORTAL && !is_granted_name(ch,pHelp->keyword) )
		  || (level >= LEVEL_IMMORTAL && !is_command(pHelp->keyword)) ) )
            continue;
        
        if ( level >= LEVEL_IMMORTAL 
            && is_command( pHelp->keyword )
            && !is_granted_name(ch,pHelp->keyword) ) 
            continue;
	*/
        if ( is_name(argall, pHelp->keyword) && pHelp->delete == FALSE )
        {
	    /* help found */
	    count++;

	    if ( exact_number )
	    {
		if ( count == number )
		{
		    firstHelp = pHelp;
		    break;
		}
	    }
	    else 
	    {
		if ( count == 1 )
		    firstHelp = pHelp;
		else
		{
		    if ( count == 2 )
		    {
			add_buf( output,
				 "Several help topics have been found for this keyword. To read the\n\r"
				 "second item on the list you can type, help 2.(keyword). You may also\n\r"
				 "put the full line in quotes (like help 'experience').\n\r"
				 "====================================================================\n\r" );
			add_buf( output, firstHelp->keyword );
			add_buf( output, "\n\r" );
			firstHelp = NULL;
		    }
		    add_buf( output, pHelp->keyword );
		    add_buf( output, "\n\r" );
		}
		
		/* small hack :) */
		if (ch->desc != NULL
		    && ch->desc->connected != CON_PLAYING 
		    && !IS_WRITING_NOTE(ch->desc->connected) 
		    && ch->desc->connected != CON_GEN_GROUPS)
		    break;
	    }
	}
    }

    /* now let's see what we found */
    if ( firstHelp == NULL && (exact_number || count <= 1) )
	add_buf( output, "No help on that word.\n\r" );

    return firstHelp;
}

void do_help( CHAR_DATA *ch, char *argument )
{
    HELP_DATA *firstHelp;
    BUFFER *output;

    /* safety net */
    if ( IS_NPC(ch) )
	return;

    if ( argument[0] == '\0' )
        argument = "summary";
    
    output = new_buf();

    if ( (firstHelp = find_help_data(ch, argument, output)) != NULL )
    {
    
	/* display the help */
	if ( firstHelp->level >= 0 && str_cmp( argument, "imotd" ) )
	{
	    add_buf(output,firstHelp->keyword);
	    add_buf(output,"\n\r");
	}
            
	/*
	 * Strip leading '.' to allow initial blanks.
	 */
	if ( firstHelp->text[0] == '.' )
	    add_buf(output,firstHelp->text+1);
	else
	    add_buf(output,firstHelp->text);
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);
}

char get_pkflag( CHAR_DATA *ch, CHAR_DATA *wch )
{
    char pkflag;
    CLANWAR_DATA *cw;
    
    if (NOT_AUTHED(wch))
        pkflag = 'N';
    else
    {
	/* name compare for finger command */
	bool is_safe = !is_exact_name(ch->name, wch->name) && is_always_safe(ch, wch);

        pkflag = ' ';
	if ( IS_SET(wch->act, PLR_RP) )
	    if (is_safe)
		pkflag = 'r';
	    else 
		pkflag = 'R';

	if ( IS_SET(wch->act, PLR_PERM_PKILL) )
	    if (is_safe)
		pkflag = 'k';
	    else 
		pkflag = 'K';

	if ( IS_SET(wch->act, PLR_HARDCORE) )
	    if (is_safe)
		pkflag = 'h';
	    else 
		pkflag = 'H';
        
        /* Check if ch's clan has declared war on wch's clan, and
        vice-versa */
	if ((cw = clanwar_lookup(wch->clan, ch->clan)))
	    if (cw->status == CLANWAR_WAR)
		if (is_safe)
		    pkflag = 'w';
		else 
		    pkflag = 'W';
	    else if (cw->truce_timer > 0)
		if (is_safe)
		    pkflag = 't';
		else
		    pkflag = 'T';
        if ((cw = clanwar_lookup(ch->clan, wch->clan)))
            if (cw->status == CLANWAR_WAR)
                if (is_safe)
                    pkflag = 'w';
                else 
                    pkflag = 'W';
                else if (cw->truce_timer > 0)
                    if (is_safe)
                        pkflag = 't';
                    else
                        pkflag = 'T';

	/* religion war supercedes all else, and so its signs are posted last --
	   NO! religion war made nonexistant due to too much similarity to clanwar :P
	if ( is_religion_opp(ch,wch) )
		if (is_safe)
		    pkflag = 'w';
		else
		    pkflag = 'W';
	*/

    }
    return pkflag;
}    


/* whois command */
void do_whois (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    char levelbuf[8];
	char *racestr;
    char clanbuf[MAX_STRING_LENGTH];
    char custombuf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;
    
    
    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("You must provide a name.\n\r",ch);
        return;
    }
    
    output = new_buf();
    
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        CHAR_DATA *wch;
        
        if ( !(d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected)) )
            continue;
        
        wch = ( d->original != NULL ) ? d->original : d->character;
        
        if ( !can_see(ch,wch) && !IS_ACTIVE_HELPER(wch) )
            continue;
        
        if (!str_prefix(arg,wch->name))
        {
            found = TRUE;
	    who_show_char( ch, wch, output );
	}
    }
    
    if (!found)
    {
        send_to_char("No one of that name is playing.\n\r",ch);
        return;
    }
    
    page_to_char(buf_string(output),ch);
    free_buf(output);
    
}

// for sorting the who_array
int who_compare( const void* a, const void* b )
{
    CHAR_DATA *ch1 = *((CHAR_DATA**) a);
    CHAR_DATA *ch2 = *((CHAR_DATA**) b);
    
    // high-level characters go first
    if ( ch1->level > ch2->level )
        return -1;
    if ( ch1->level < ch2->level )
        return 1;

    if ( ch1->clan < ch2->clan )
        return -1;
    if ( ch1->clan > ch2->clan )
        return 1;

    // same for clan rank - higher ranks first
    if ( ch1->pcdata->clan_rank > ch2->pcdata->clan_rank )
        return -1;
    if ( ch1->pcdata->clan_rank < ch2->pcdata->clan_rank )
        return 1;

    return strcmp(ch1->name, ch2->name);
}

/*
 * returns number of characters returned in who_array
 */
#define MAX_WHO 64
int create_who_array( CHAR_DATA **who_array )
{
    int who_count = 0;
    DESCRIPTOR_DATA *desc;
    
    // gather all characters we want to show
    for ( desc = descriptor_list; desc != NULL && who_count < MAX_WHO; desc = desc->next )
    {
        if ( IS_PLAYING(desc->connected) && desc->character != NULL )
        {
            who_array[who_count++] = DESC_PC(desc);
        }
    }
    // now sort it
    qsort(&who_array[0], who_count, sizeof(CHAR_DATA*), &who_compare);
    
    return who_count;
}

/*
* New 'who' command originally by Alander of Rivers of Mud.
* Reworked by Quirky!  in August of 2002.  Fixed more by Quirky!  in June of 2003.
*/
void do_who( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char *racestr;
    char clanbuf[MAX_STRING_LENGTH];
    char levelbuf[8];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    CHAR_DATA* who_array[MAX_WHO];
    int who_count, w;
    RELIGION_DATA *rel = NULL;
    int iClass;
    int iRace;
    int iClan;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    bool rgfClan[MAX_CLAN];
    char custombuf[MAX_STRING_LENGTH];
    bool fClassRestrict = FALSE;
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    bool fReligion = FALSE;
    bool fRaceRestrict = FALSE;
    bool fImmortalOnly = FALSE;
    bool fHelperOnly = FALSE;
    bool fRemortOnly = FALSE;
    bool fPkillOnly = FALSE;
    bool fTagAll = FALSE;
    bool fTagRed = FALSE;
    bool fTagBlue = FALSE;
    bool fFemale = FALSE;
    bool fMale = FALSE;
    bool fNeuter = FALSE;

    /*
    * Set default arguments.
    */
    iLevelLower    = 0;
    iLevelUpper    = MAX_LEVEL;
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        rgfClass[iClass] = FALSE;
    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
        rgfRace[iRace] = FALSE;
    for (iClan = 0; iClan < MAX_CLAN; iClan++)
        rgfClan[iClan] = FALSE;


        /*
        * Parse arguments.
       */
    nNumber = 0;
    for ( ;; )
    {
        char arg[MAX_STRING_LENGTH];
        
        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' )
            break;
        
        if ( is_number( arg ) )
        {
            switch ( ++nNumber )
            {
            case 1: iLevelLower = atoi( arg ); break;
            case 2: iLevelUpper = atoi( arg ); break;
            default:
                send_to_char( "Only two level numbers allowed.\n\r", ch );
                return;
            }
        }
        else
        {
	  /*
	   * Look for classes to turn on.
	   */
	    if (!str_prefix(arg,"immortals"))
	    {
		fImmortalOnly = TRUE;
	    }
	    else if (!str_prefix(arg,"helper"))
	    {
		fHelperOnly = TRUE;
	    }
	    else if (!str_prefix(arg,"pkill"))
	    {
		fPkillOnly = TRUE;
	    }
	    else if (!str_prefix(arg,"remort"))
	    {
		fRemortOnly = TRUE;
	    }
	    else if (!str_prefix(arg,"female"))
	    {
		fFemale = TRUE;
	    }
	    else if (!str_prefix(arg,"male"))
	    {
		fMale = TRUE;
	    }
	    else if (!str_prefix(arg,"neuter") || !str_prefix(arg,"sexless"))
	    {
		fNeuter = TRUE;
	    }
	    else if (!str_prefix(arg,"tag"))
	    {
		fTagAll = TRUE;
	    }
	    else if (!str_prefix(arg,"blue"))
	    {
		fTagBlue = TRUE;
	    }
	    else if (!str_prefix(arg,"red"))
	    {
		fTagRed = TRUE;
	    }
	    else
            {
		iClass = class_lookup(arg);
		if (iClass == -1)
		{
		    iRace = race_lookup(arg);
		    if (iRace == 0 || iRace >= MAX_PC_RACE)
		    {
			if (!str_prefix(arg,"clan"))
			    fClan = TRUE;
			else if(!str_prefix(arg,"religion"))
			    fReligion = TRUE;
			else
			{
			    iClan = clan_lookup(arg);
			    if (iClan)
			    {
				fClanRestrict = TRUE;
				rgfClan[iClan] = TRUE;
			    }
			    else				   
			    {
				if ( (rel=get_religion_by_name(arg)) == NULL )
				{
				    send_to_char("That's not a valid race, class, clan or religion.\n\r",ch);
				    return;
				}
			    }
			}
		    }
		    else
		    {
			fRaceRestrict = TRUE;
			rgfRace[iRace] = TRUE;
		    }
		}
		else
		{
		    fClassRestrict = TRUE;
		    rgfClass[iClass] = TRUE;
		}
	    }        
        }
    }
    
    /*
    * Now show matching chars.
    */
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf();

    who_count = create_who_array( who_array );
    for ( w = 0; w < who_count; w++ )
    {
        CHAR_DATA *wch = who_array[w];
                
        if ( !can_see(ch,wch) )
            continue;
        
        if ( wch->level < iLevelLower
	     ||  IS_NPC(ch) /* Sanity check */
	     ||   wch->level > iLevelUpper
	     || ( fImmortalOnly  && wch->level < LEVEL_IMMORTAL )
	     || ( fHelperOnly  && !IS_HELPER(wch) && !IS_ACTIVE_HELPER(wch) )
	     || ( fPkillOnly && !IS_SET(wch->act,PLR_PERM_PKILL)
		  && (is_always_safe(ch, wch) || ch == wch || IS_IMMORTAL(ch)) )
	     || ( fRemortOnly && wch->pcdata->remorts == 0 )
	     || ( fClassRestrict && !rgfClass[wch->class] )
	     || ( fRaceRestrict && !rgfRace[wch->race])
	     || ( fReligion && (get_religion(wch)!=get_religion(ch)) )
	     || ( fClan && !is_same_clan(ch, wch))
	     || ( fClanRestrict && !rgfClan[wch->clan])
	     || ( rel != NULL && get_religion(wch) != rel )
	     || ( fTagRed && !IS_SET(wch->pcdata->tag_flags,TAG_RED) )
	     || ( fTagBlue && !IS_SET(wch->pcdata->tag_flags,TAG_BLUE) )
	     || ( fTagAll && !IS_TAG(wch) )
	     || ( fFemale && wch->sex != 2 )
	     || ( fMale && wch->sex != 1 )
	     || ( fNeuter && wch->sex != 0 )		)
            continue;

        nMatch++;
        
        who_show_char( ch, wch, output );
    }
    
    sprintf( buf2, "\n\rPlayers found: %d\n\r", nMatch );
    add_buf(output,buf2);
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}
#undef MAX_WHO

void who_show_char( CHAR_DATA *ch, CHAR_DATA *wch, BUFFER *output )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char clanbuf[MAX_STRING_LENGTH];
    char custombuf[MAX_STRING_LENGTH];
    char levelbuf[8];
    char *racestr;

    switch(wch->level)
    {
    case MAX_LEVEL - 0 : sprintf(levelbuf, "IMP"); break;
    case MAX_LEVEL - 1 : sprintf(levelbuf, "ARC"); break;
    case MAX_LEVEL - 2 : sprintf(levelbuf, "ARC"); break;
    case MAX_LEVEL - 3 : sprintf(levelbuf, "VAR"); break;
    case MAX_LEVEL - 4 : sprintf(levelbuf, "VAR"); break;
    case MAX_LEVEL - 5 : sprintf(levelbuf, "GOD"); break;
    case MAX_LEVEL - 6 : sprintf(levelbuf, "GOD"); break;
    case MAX_LEVEL - 7 : sprintf(levelbuf, "DEM"); break;
    case MAX_LEVEL - 8 : sprintf(levelbuf, "DEM"); break;
    case MAX_LEVEL - 9 : sprintf(levelbuf, "SAV"); break;
    default : sprintf(levelbuf, "%3d", wch->level); break;
    }
    
    if (clan_table[wch->clan].active)
	sprintf(clanbuf, "[%s%s-%s{x] ", 
		clan_table[wch->clan].who_color,
		clan_table[wch->clan].who_name,
		clan_table[wch->clan].rank_list[wch->pcdata->clan_rank].who_name);
    else
	clanbuf[0] = '\0';
    
    if (wch->pcdata->customflag[0]!='\0')
	sprintf(custombuf, "(%s) ", wch->pcdata->customflag);
    else
	custombuf[0] = '\0';
    
    if (wch->race >= MAX_PC_RACE)
	racestr = "      ";
    else if (wch->race == race_werewolf)
	if ( weather_info.sunlight == SUN_SET
	     ||   weather_info.sunlight == SUN_DARK )
	    racestr = "Wolf  ";
	else
	    racestr = "Were  ";
    else
	racestr = pc_race_table[wch->race].who_name;

    /* a little formatting */
    sprintf(buf, "[%s %6s %s %c] %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s{x%s\n\r",
            levelbuf, racestr,
            class_table[wch->class].who_name,
            get_pkflag(ch, wch),

            clanbuf,

            IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
	    IS_HELPER(wch) ? (ch->pcdata->remorts > 0 || IS_IMMORTAL(ch) || !IS_ACTIVE_HELPER(wch)) ?
	                                                    "{G*{x " : "({GH{CE{cL{GP{CE{cR{G!{x) " : "",
	    IS_SET(wch->act, PLR_RP) ? "[RP] " : "",
            wch->incog_level >= LEVEL_HERO ? "(Incog) ": "",
            wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
            IS_SET(wch->act, PLR_WAR) ? "{y[War] {x" : "",
            IS_SET(wch->pcdata->tag_flags,TAG_RED) ? "{R[RED]{x " : 
	                 (IS_SET(wch->pcdata->tag_flags,TAG_BLUE) ? "{B[BLUE]{x " : ""),
            IS_SET(wch->pcdata->tag_flags,TAG_FROZEN) ? "{W[Frozen]{x " : "",
            custombuf,
            IS_SET(wch->act,PLR_KILLER) ? "(KILLER) " : "",
            IS_SET(wch->act,PLR_THIEF) ? "(THIEF) " : "",
            wch->pcdata->name_color, wch->pcdata->pre_title,
	    wch->name, IS_NPC(wch) ? "" : wch->pcdata->title);
    add_buf(output,buf);
}

/*
void do_count ( CHAR_DATA *ch, char *argument )
{
    int count;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    
    count = 0;
    
    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( d->connected == CON_PLAYING && can_see( ch, d->character ) )
            count++;
        
        max_on = UMAX(count,max_on);
        
        if (max_on == count)
            sprintf(buf,"There are %d characters on, the most so far today.\n\r",
            count);
        else
            sprintf(buf,"There are %d characters on, the most on today was %d.\n\r",
            count,max_on);
        
        send_to_char(buf,ch);
}
*/

void do_inventory( CHAR_DATA *ch, char *argument )
{
    char buf[MSL];

    sprintf( buf, "You are carrying %d / %d items:\n\r", 
        ch->carry_number, can_carry_n(ch));
    send_to_char(buf,ch);

    show_list_to_char( ch->carrying, ch, TRUE, TRUE );
    return;
}

void do_equipment( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    int iWear;
    bool found, all_slots;

    if ( argument[0] != '\0' )
	if ( !strcmp(argument, "all") )
	    all_slots = TRUE;
	else
	{
	    send_to_char( "Syntax: equipment or equipment all\n\r", ch );
	    return;
	}
    else
	all_slots = FALSE;
    
    send_to_char( "You are using:\n\r", ch );
    found = FALSE;
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
	int tattoo = get_tattoo_ch(ch, iWear);

        if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
	{
	    if ( tattoo != TATTOO_NONE )
	    {
		send_to_char( where_name[iWear], ch );
		send_to_char( tattoo_desc(tattoo), ch );
		send_to_char( "\n\r", ch );
		found = TRUE;
	    } 
	    else if ( all_slots )
	    {
		send_to_char( where_name[iWear], ch );
		send_to_char( "nothing.\n\r", ch );
	    }
            continue;
	}

        send_to_char( where_name[iWear], ch );
        if ( can_see_obj( ch, obj ) )
        {
            send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
	    if( CAN_WEAR(obj,ITEM_TRANSLUCENT) && tattoo != TATTOO_NONE )
	    {
		send_to_char( " above ", ch );
		send_to_char( tattoo_desc(tattoo), ch );
	    }
            send_to_char( "\n\r", ch );
        }
        else
        {
            if( tattoo != TATTOO_NONE )
                send_to_char( tattoo_desc(tattoo), ch );
	    else
        	send_to_char( "something.", ch );
            send_to_char( "\n\r", ch );
        }
        found = TRUE;
    }
    
    if ( !found && !all_slots )
        send_to_char( "Nothing.\n\r", ch );

    return;
}



void do_compare( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
        send_to_char( "Compare what to what?\n\r", ch );
        return;
    }
    
    if ( ( obj1 = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
        send_to_char( "You do not have that item.\n\r", ch );
        return;
    }
    
    if (arg2[0] == '\0')
    {
        for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
        {
            if (obj2->wear_loc != WEAR_NONE
                &&  can_see_obj(ch,obj2)
                &&  obj1->item_type == obj2->item_type
                &&  commen_wear_pos(obj1->wear_flags,obj2->wear_flags) )
                break;
        }
        
        if (obj2 == NULL)
        {
            send_to_char("You aren't wearing anything comparable.\n\r",ch);
            return;
        }
    } 
    
    else if ( (obj2 = get_obj_carry(ch,arg2,ch) ) == NULL )
    {
        send_to_char("You do not have that item.\n\r",ch);
        return;
    }
    
    msg     = NULL;
    value1 = get_obj_ops( obj1 );
    value2 = get_obj_ops( obj2 );
    
    if ( obj1 == obj2 )
    {
        msg = "You compare $p to itself.  It looks about the same.";
    }
    else if ( !commen_wear_pos(obj1->wear_flags, obj2->wear_flags) )
    {
        msg = "You can't compare $p and $P.";
    }
    else
    {
        if (obj1->item_type == ITEM_WEAPON && obj1->item_type == ITEM_WEAPON)
        {
            value1 += 4 * average_weapon_dam( obj1 );
            value2 += 4 * average_weapon_dam( obj2 );
        }
        // translucent eq has "hidden" ops
        if ( CAN_WEAR(obj1, ITEM_TRANSLUCENT) )
            value1 += get_translucency_spec_penalty( obj1->level );
        if ( CAN_WEAR(obj2, ITEM_TRANSLUCENT) )
            value2 += get_translucency_spec_penalty( obj2->level );
    }
    
    if ( msg == NULL )
    {
        if ( value1 == value2 ) msg = "$p and $P look about the same.";
        else if ( value1  > value2 ) msg = "$p looks better than $P.";
        else                         msg = "$p looks worse than $P.";
    }
    
    act( msg, ch, obj1, obj2, TO_CHAR );
    return;
}

bool commen_wear_pos( tflag wear_flag1, tflag wear_flag2 )
{
    int pos;
    for ( pos = 1; pos < FLAG_MAX_BIT; pos++ )
	if ( pos != ITEM_TAKE && pos != ITEM_NO_SAC && pos != ITEM_TRANSLUCENT
	     && IS_SET( wear_flag1, pos )
	     && IS_SET( wear_flag2, pos ) )
	    return TRUE;
    return FALSE;
}

void do_credits( CHAR_DATA *ch, char *argument )
{
    do_help( ch, "diku" );
    return;
}

bool can_locate( CHAR_DATA *ch, CHAR_DATA *victim )
{
    return IS_NPC( ch )
	|| IS_NPC( victim )
	|| IS_SET( victim->act, PLR_WAR )
	|| !IS_SET( victim->act, PLR_NOLOCATE )
	|| !is_always_safe( ch, victim );
}

void do_where( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    bool found;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Players near you:\n\r", ch );
        found = FALSE;
        for ( d = descriptor_list; d; d = d->next )
        {
            if ( (d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected))
		 && ( victim = d->character ) != NULL
		 &&   !IS_NPC(victim)
		 &&   victim->in_room != NULL
		 &&   !IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
		 &&   (is_room_owner(ch,victim->in_room) 
		      || !room_is_private(victim->in_room))
		 &&   victim->in_room->area == ch->in_room->area
		 &&   can_see( ch, victim )
		 &&   can_locate( ch, victim ) )
            {
                found = TRUE;
                sprintf( buf, "%-28s %s\n\r",
                    victim->name, victim->in_room->name );
                send_to_char( buf, ch );
            }
        }
        if ( !found )
            send_to_char( "None\n\r", ch );
    }
    else
    {
        found = FALSE;
        for ( victim = char_list; victim != NULL; victim = victim->next )
        {
            if ( victim->in_room != NULL
                &&   victim->in_room->area == ch->in_room->area
                &&   can_see( ch, victim )
		&&   can_locate( ch, victim )
                &&   is_name( arg, victim->name ) )
            {
                found = TRUE;
                sprintf( buf, "%-28s %s\n\r",
                    PERS(victim, ch), victim->in_room->name );
                send_to_char( buf, ch );
                break;
            }
        }
        if ( !found )
            act( "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
    }
    
    return;
}




void do_consider( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int diff, armor, hp, dam, level;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Consider killing whom?\n\r", ch );
        return;
    }
    
    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They're not here.\n\r", ch );
        return;
    }
    
    if (is_safe(ch,victim))
    {
        send_to_char("Don't even think about it.\n\r",ch);
        return;
    }
    
    diff = level_power(victim) - level_power(ch);
    if ( IS_NPC(victim) )
    {
	/* NPCs are weaker than players */
	diff -= 10 + victim->level/5;
	/* unless they have special abilities.. */
	if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
	    diff += 5 + victim->level/10;
	if ( IS_AFFECTED(victim, AFF_HASTE) )
	    diff += 3 + victim->level/20;
	if ( IS_SET(victim->off_flags, OFF_FAST) )
	    diff += 2 + victim->level/20;
    }

    /*
    if (IS_NPC(victim))
    {
        level = victim->level;
        armor = (victim->armor[0]+victim->armor[1]+victim->armor[2]+victim->armor[3])/4;
        hp = victim->hit;
        dam = (victim->damage[0]*(victim->damage[1]+1))/2 + GET_DAMROLL(victim) + GET_HITROLL(victim);
        diff += ((hp-level*level)/(level+1))/(3+level/15);
        diff += (dam-level)/(2+level/30);
        diff += (100 - armor - 6*level)/(level+10);
    }
    */

    /* reduce diff a bit to make difference more distinguishable */
    diff = diff * 2/3;

    if ( diff <= -50 ) msg = "One displeased glance could send $N into cardiac arrest.";
    else if ( diff <= -36 ) msg = "You could squash $N like a bloated tick.";
    else if ( diff <= -24 ) msg = "$N wets $Mself.";
    else if ( diff <= -18 ) msg = "You can kill $N naked and weaponless.";
    else if ( diff <= -12 ) msg = "$N is nothing to break a sweat over.";
    else if ( diff <=  -8 ) msg = "$N is no match for you.";
    else if ( diff <=  -4 ) msg = "$N looks like an easy kill.";
    else if ( diff <=   3 ) msg = "The perfect match!";
    else if ( diff <=   8 ) msg = "$N says 'Do you feel lucky, punk?'";
    else if ( diff <=   12 ) msg = "$N laughs at you mercilessly.";
    else if ( diff <=   18 ) msg = "You'd be a hero or a fool to attack $N.";
    else if ( diff <=   24 ) msg = "$N would have cream of $n for lunch.";
    else if ( diff <=   36 ) msg = "What are you, nuts?";
    else if ( diff <=   50 ) msg = "Surely the gods would not show mercy for such lunacy.";
    else                    msg = "Death will thank you for your gift.";
       
       act( msg, ch, NULL, victim, TO_CHAR );
       return;
}



void set_title( CHAR_DATA *ch, char *title )
{
    char buf[MAX_STRING_LENGTH];
    
    if ( IS_NPC(ch) )
    {
        bug( "Set_title: NPC.", 0 );
        return;
    }
    
    if ( title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' && title[0] != '\'' )
    {
        buf[0] = ' ';
        strcpy( buf+1, title );
    }
    else
    {
        strcpy( buf, title );
    }
    
    free_string( ch->pcdata->title );
    ch->pcdata->title = str_dup( buf );
    return;
}


void do_title( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char *temp;
    int chars = 0, noncol = 0;
    if ( IS_NPC(ch) )
        return;
    
    if ( argument[0] == '\0' )
    {
        sprintf( buf, "the %s",
            title_table [ch->class] [(ch->level+4-(ch->level+4)%5)/5] );
        set_title( ch, buf );
        REMOVE_BIT(ch->act, PLR_TITLE);
        return;
    }
    
    if ( strlen_color(argument) > 45 )
    {
	for( temp = argument; *temp != '\0' ; temp++ )
	{
	    chars++;
	    if( *temp == '{' )
		noncol--;
	    else noncol++;
	    if( noncol > 45 )  break;
	}
	argument[chars] = '\0';
    }
    
    smash_beep_n_blink( argument );
    smash_reserved_colcodes( argument );
    smash_tilde( argument );
    strcat (argument, " {x");
    set_title( ch, argument );
    SET_BIT(ch->act, PLR_TITLE);
    send_to_char( "Ok.\n\r", ch );
}

/* Called from do_title and public channels, this removes annoying beeps and blinks */
void smash_beep_n_blink( char *str )
{
    for( ; *str != '\0'; str++ )
    {
        if ( *str == '{' )
	{
	    str++;
	    if( *str == '%' || *str == '*' )
		*str = 'x';
	}
    }
    return;
}

/* Called ONLY from do_title, this prevents people from using the colour codes used
   by the channels .. mostly to prevent stray beeps from being annoying :P */

void smash_reserved_colcodes( char *str )
{
    for( ; *str != '\0'; str++ )
    {
        if ( *str == '{' )
	{
	    str++;
	    if( *str != 'r' && *str != 'R' && *str != 'g' && *str != 'G'
			&& *str != 'c' && *str != 'C' && *str != 'b' && *str != 'B'
			&& *str != 'm' && *str != 'M' && *str != 'y' && *str != 'Y'
			&& *str != 'w' && *str != 'W' && *str != 'D' && *str != '{'
			&& *str != 'x' && *str != '+' && *str != 'v' )
		*str = 'x';
	}
    }
    return;
}


void do_description( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    
    if ( argument[0] != '\0' )
    {
        buf[0] = '\0';
        smash_tilde( argument );
        
        if (argument[0] == '-')
        {
            int len;
            bool found = FALSE;
            
            if (ch->description == NULL || ch->description[0] == '\0')
            {
                send_to_char("No lines left to remove.\n\r",ch);
                return;
            }
            
            strcpy(buf,ch->description);
            
            for (len = strlen(buf); len > 0; len--)
            {
                if (buf[len] == '\r')
                {
                    if (!found)  /* back it up */
                    {
                        if (len > 0)
                            len--;
                        found = TRUE;
                    }
                    else /* found the second one */
                    {
                        buf[len + 1] = '\0';
                        free_string(ch->description);
                        ch->description = str_dup(buf);
                        send_to_char( "Your description is:\n\r", ch );
                        send_to_char( ch->description ? ch->description : 
                        "(None).\n\r", ch );
                        return;
                    }
                }
            }
            buf[0] = '\0';
            free_string(ch->description);
            ch->description = str_dup(buf);
            send_to_char("Description cleared.\n\r",ch);
            return;
        }
        if ( argument[0] == '+' )
        {
            if ( ch->description != NULL )
                strcat( buf, ch->description );
            argument++;
            while ( isspace(*argument) )
                argument++;
        }
        
        if ( strlen(buf) >= 1024)
        {
            send_to_char( "Description too long.\n\r", ch );
            return;
        }
        
        strcat( buf, argument );
        strcat( buf, "\n\r" );
        free_string( ch->description );
        ch->description = str_dup( buf );
    }
    
    send_to_char( "Your description is:\n\r", ch );
    send_to_char( ch->description ? ch->description : "(None).\n\r", ch );
    return;
}



void do_report( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];
    
    sprintf( buf,
        "You say 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'\n\r",
        ch->hit,  ch->max_hit,
        ch->mana, ch->max_mana,
        ch->move, ch->max_move,
        ch->exp   );
    
    send_to_char( buf, ch );
    
    sprintf( buf, "$n says 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'",
        ch->hit,  ch->max_hit,
        ch->mana, ch->max_mana,
        ch->move, ch->max_move,
        ch->exp   );
    
    act( buf, ch, NULL, NULL, TO_ROOM );
    
    return;
}



/*
* 'Wimpy' originally by Dionysos.
*/
void do_wimpy( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int wimpy;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        printf_to_char( ch, "Your current wimpy threshold is set to %d%%.\n\r", ch->wimpy );
        printf_to_char( ch, "To change type: wimpy <percentage>\n\r" );
        return;
    }
    else
        wimpy = atoi( arg );
    
    wimpy = URANGE(0, wimpy, 100);
    
    ch->wimpy = wimpy;
    if ( wimpy == 0 )
        printf_to_char( ch, "You will stand your ground no matter what.\n\r");
    else if ( wimpy == 100 )
        printf_to_char( ch, "You will run at the first hint of danger.\n\r");
    else 
        printf_to_char( ch, "You will now flee when dropping below %d%% of your hit points.\n\r", wimpy );
    return;
}

void do_calm( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int calm;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        printf_to_char( ch, "Your current calm threshold is set to %d%%.\n\r", ch->calm );
        printf_to_char( ch, "To change type: calm <percentage>\n\r" );
        return;
    }
    else
        calm = atoi( arg );
    
    calm = URANGE(0, calm, 100);
    
    ch->calm = calm;
    if ( calm == 0 )
        printf_to_char( ch, "You won't calm down till you drop.\n\r");
    else if ( calm == 100 )
        printf_to_char( ch, "You will always be calm.\n\r");
    else 
        printf_to_char( ch, "You will now calm down when dropping below %d%% of your moves.\n\r", calm );
    return;
}

void do_password( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;
    
    if ( IS_NPC(ch) )
        return;
    
    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    /*
    pArg = arg1;
    while ( isspace(*argument) )
        argument++;
    
    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;
    
    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';
    
    pArg = arg2;
    while ( isspace(*argument) )
        argument++;
    
    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;
    
    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';
    */
    argument = one_argument_keep_case( argument, arg1 );
    argument = one_argument_keep_case( argument, arg2 );
    argument = one_argument_keep_case( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char( "Syntax: password <old> <new> <new>.\n\r", ch );
        return;
    }
    
    if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
    {
        WAIT_STATE( ch, 40 );
        send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
        return;
    }
    
    if ( strlen(arg2) < 5 )
    {
        send_to_char(
            "New password must be at least five characters long.\n\r", ch );
        return;
    }

    if ( strcmp(arg2, arg3) )
    {
        send_to_char( "The two typings of the new password don't match.\n\r", ch );
        return;
    }
    
    /*
    * No tilde allowed because of player file format.
    */
    pwdnew = crypt( arg2, ch->name );
    for ( p = pwdnew; *p != '\0'; p++ )
    {
        if ( *p == '~' )
        {
            send_to_char(
                "New password not acceptable, try again.\n\r", ch );
            return;
        }
    }
    
    free_string( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );
    send_to_char( "Ok.\n\r", ch );
    return;
}

void say_basic_obj_data( CHAR_DATA *ch, OBJ_DATA *obj )
{
    char buf[MAX_STRING_LENGTH], arg[MAX_INPUT_LENGTH];
    int c, pos;

    sprintf( buf, "The %s is %s.",
	     item_name(obj->item_type),
	     extra_bits_name(obj->extra_flags) );
    do_say(ch, buf);
    
    /*
    if (obj->owner != NULL)
    {
        sprintf( buf, "It is owned by %s.", obj->owner);
        do_say(ch, buf);
    }
    */

    sprintf( buf, "It weighs %d, and its level of power is %d.",
	     obj->weight / 10,
	     obj->level );
    do_say(ch, buf);
    
    if ( CAN_WEAR(obj, ITEM_TRANSLUCENT) )
    {
       /* Loring translucent objects while confused no longer crashes the MUD */
       /*         /* Jan 13, 2006 - Elik */
       /* 
        *         do_say( ch, "It's translucent, allowing tattoos to shine through." );
        *                 */

	sprintf(buf, "It's translucent, allowing tattoos to shine through.");
	do_say(ch, buf);
    }
    
    switch ( obj->item_type )
    {
    case ITEM_LIGHT:
        if ( obj->value[2] >= 0 )
            sprintf( buf, "It has %d hours of light remaining.", obj->value[2] );
        else
            sprintf( buf, "It is an infinite light source." );
        do_say( ch, buf );
        break;
    case ITEM_ARROWS:
	sprintf( buf, "It contains %d arrows.", obj->value[0] );
	do_say( ch, buf );
	if ( obj->value[1] > 0 )
	{
	    sprintf( buf, "Each arrow deals %d extra %s damage.",
		     obj->value[1], flag_bit_name(damage_type, obj->value[2]) );
	    do_say( ch, buf );
	}
        break;
    case ITEM_SCROLL: 
    case ITEM_POTION:
    case ITEM_PILL:
        sprintf( buf, "It has level %d spells of:", obj->value[0] );
        
        for ( c=1; c<5; c++)
            if ( obj->value[c] >= 0 && obj->value[c] < MAX_SKILL )
            {
                strcat( buf, " '");
                strcat( buf, skill_table[obj->value[c]].name);
                strcat( buf, "'");
            }
            
            strcat( buf, ".");
            do_say(ch, buf);
            
            break;
            
    case ITEM_WAND: 
    case ITEM_STAFF: 
        sprintf( buf, "It has %d charges of level %d",
            obj->value[2], obj->value[0] );
        
        if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
        {
            strcat( buf, " '");
            strcat( buf, skill_table[obj->value[3]].name);
            strcat( buf, "'");
        }
        
        strcat( buf, ".");
        do_say(ch, buf);
        
        break;
        
    case ITEM_DRINK_CON:
	/*
        sprintf(buf,"It holds %s-colored %s.",
            liq_table[obj->value[2]].liq_color,
            liq_table[obj->value[2]].liq_name);
        do_say(ch, buf);
        */
        break;
        
    case ITEM_CONTAINER:
        sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s",
            obj->value[0], obj->value[3], cont_bits_name(obj->value[1]));
        do_say(ch, buf);
        
        if (obj->value[4] != 100)
        {
            sprintf(buf,"Weight multiplier: %d%%",
                obj->value[4]);
            do_say(ch, buf);
            
        }
        break;
       
    case ITEM_WEAPON:
        strcpy(buf, "The weapon is ");
        switch (obj->value[0])
        {
        case(WEAPON_EXOTIC) : strcat(buf, "of some exotic type.");  break;
        case(WEAPON_SWORD)  : strcat(buf, "a sword.");  break;  
        case(WEAPON_DAGGER) : strcat(buf, "a dagger."); break;
        case(WEAPON_SPEAR)  : strcat(buf, "a spear or staff."); break;
        case(WEAPON_MACE)   : strcat(buf, "a mace or club.");   break;
        case(WEAPON_AXE)    : strcat(buf, "an axe.");       break;
        case(WEAPON_FLAIL)  : strcat(buf, "a flail.");  break;
        case(WEAPON_WHIP)   : strcat(buf, "a whip.");       break;
        case(WEAPON_POLEARM): strcat(buf, "a polearm.");    break;
        case(WEAPON_GUN)    : strcat(buf, "a gun.");    break;
        case(WEAPON_BOW)    : strcat(buf, "a bow.");    break;
        default             : strcat(buf, "of some unknown type."); break;
        }
        do_say(ch, buf);
        
	sprintf(buf,"It does %s damage of %dd%d (average %d).",
		attack_table[obj->value[3]].noun,
		obj->value[1],obj->value[2],
		(1 + obj->value[2]) * obj->value[1] / 2);
        do_say(ch, buf);
        
        
        if (obj->value[4])  /* weapon flags */
        {
            sprintf(buf, "Weapons flags: %s",
		    weapon_bits_name(obj->value[4]));
            do_say(ch, buf);
            
        }
        break;
        
    case ITEM_ARMOR:
	sprintf( buf, "" );
	for( pos = 1; pos < FLAG_MAX_BIT; pos++ )
	{
		if( !IS_SET(obj->wear_flags, pos) )
		    continue;

		if( !strcmp( wear_bit_name(pos), "shield" ) )
		    sprintf( buf, "It is used as a shield." );
        	else if( !strcmp( wear_bit_name(pos), "float" ) )
		    sprintf( buf, "It would float nearby." );
		else if ( pos != ITEM_TAKE && pos != ITEM_NO_SAC && pos != ITEM_TRANSLUCENT )
		    sprintf( buf, "It is worn on the %s.", wear_bit_name(pos) );
	}
	do_say(ch, buf);

        sprintf( buf, 
            "It provides an armor class of %d pierce, %d bash, %d slash, and %d vs. magic.", 
            obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
        do_say(ch, buf);
            
        break;
   }
}

/* same stupid thingy AGAIN.. */
void say_basic_obj_index_data( CHAR_DATA *ch, OBJ_INDEX_DATA *obj )
{
    char buf[MAX_STRING_LENGTH], arg[MAX_INPUT_LENGTH];
    int c;
    int pos;

    sprintf( buf, "The %s is %s.",
	     item_name(obj->item_type),
	     extra_bits_name(obj->extra_flags) );
    do_say(ch, buf);
    
    /*
    if (obj->owner != NULL)
    {
        sprintf( buf, "It is owned by %s.", obj->owner);
        do_say(ch, buf);
    }
    */

    sprintf( buf, "It weighs %d, and its level of power is %d.",
	     obj->weight / 10,
	     obj->level );
    do_say(ch, buf);
    
    if ( CAN_WEAR(obj, ITEM_TRANSLUCENT) )
    {
	sprintf( buf, "It's translucent, allowing tattoos to shine through."); 
	do_say( ch, buf );
    }
    
    switch ( obj->item_type )
    {
    case ITEM_LIGHT:
        if ( obj->value[2] >= 0 )
            sprintf( buf, "It has %d hours of light remaining.", obj->value[2] );
        else
            sprintf( buf, "It is an infinite light source." );
        do_say( ch, buf );
        break;
    case ITEM_ARROWS:
	sprintf( buf, "It contains %d arrows.", obj->value[0] );
	do_say( ch, buf );
	if ( obj->value[1] > 0 )
	{
	    sprintf( buf, "Each arrow deals %d extra %s damage.",
		     obj->value[1], flag_bit_name(damage_type, obj->value[2]) );
	    do_say( ch, buf );
	}
        break;
    case ITEM_SCROLL: 
    case ITEM_POTION:
    case ITEM_PILL:
        sprintf( buf, "It has level %d spells of:", obj->value[0] );
        
        for ( c=1; c<5; c++)
            if ( obj->value[c] >= 0 && obj->value[c] < MAX_SKILL )
            {
                strcat( buf, " '");
                strcat( buf, skill_table[obj->value[c]].name);
                strcat( buf, "'");
            }
            
            strcat( buf, ".");
            do_say(ch, buf);
            
            break;
            
    case ITEM_WAND: 
    case ITEM_STAFF: 
        sprintf( buf, "It has %d charges of level %d",
            obj->value[2], obj->value[0] );
        
        if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
        {
            strcat( buf, " '");
            strcat( buf, skill_table[obj->value[3]].name);
            strcat( buf, "'");
        }
        
        strcat( buf, ".");
        do_say(ch, buf);
        
        break;
        
    case ITEM_DRINK_CON:
	/*
        sprintf(buf,"It holds %s-colored %s.",
            liq_table[obj->value[2]].liq_color,
            liq_table[obj->value[2]].liq_name);
        do_say(ch, buf);
        */
        break;
        
    case ITEM_CONTAINER:
        sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s",
            obj->value[0], obj->value[3], cont_bits_name(obj->value[1]));
        do_say(ch, buf);
        
        if (obj->value[4] != 100)
        {
            sprintf(buf,"Weight multiplier: %d%%",
                obj->value[4]);
            do_say(ch, buf);
            
        }
        break;
        
    case ITEM_WEAPON:
        strcpy(buf, "The weapon is ");
        switch (obj->value[0])
        {
        case(WEAPON_EXOTIC) : strcat(buf, "of some exotic type.");  break;
        case(WEAPON_SWORD)  : strcat(buf, "a sword.");  break;  
        case(WEAPON_DAGGER) : strcat(buf, "a dagger."); break;
        case(WEAPON_SPEAR)  : strcat(buf, "a spear or staff."); break;
        case(WEAPON_MACE)   : strcat(buf, "a mace or club.");   break;
        case(WEAPON_AXE)    : strcat(buf, "an axe.");       break;
        case(WEAPON_FLAIL)  : strcat(buf, "a flail.");  break;
        case(WEAPON_WHIP)   : strcat(buf, "a whip.");       break;
        case(WEAPON_POLEARM): strcat(buf, "a polearm.");    break;
        case(WEAPON_GUN)    : strcat(buf, "a gun.");    break;
        case(WEAPON_BOW)    : strcat(buf, "a bow.");    break;
        default             : strcat(buf, "of some unknown type."); break;
        }
        do_say(ch, buf);
        
	sprintf(buf,"It does %s damage of %dd%d (average %d).",
		attack_table[obj->value[3]].noun ,
		obj->value[1],obj->value[2],
		(1 + obj->value[2]) * obj->value[1] / 2);
        do_say(ch, buf);
        
        
        if (obj->value[4])  /* weapon flags */
        {
            sprintf(buf, "Weapons flags: %s",
		    weapon_bits_name(obj->value[4]));
            do_say(ch, buf);
            
        }
        break;
        
    case ITEM_ARMOR:

	sprintf( buf, "" );
	for( pos = 1; pos < FLAG_MAX_BIT; pos++ )
	{
		if( !IS_SET(obj->wear_flags, pos) )
		    continue;

		if( !strcmp( wear_bit_name(pos), "shield" ) )
		    sprintf( buf, "It is used as a shield." );
        	else if( !strcmp( wear_bit_name(pos), "float" ) )
		    sprintf( buf, "It would float nearby." );
		else if ( pos != ITEM_TAKE && pos != ITEM_NO_SAC && pos != ITEM_TRANSLUCENT )
		    sprintf( buf, "It is worn on the %s.", wear_bit_name(pos) );
	}
	do_say(ch, buf);

        sprintf( buf, 
            "It provides an armor class of %d pierce, %d bash, %d slash, and %d vs. magic.", 
            obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
        do_say(ch, buf);
            
            break;
   }
}

void do_lore ( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    OBJ_INDEX_DATA *org_obj;
    char buf[MAX_STRING_LENGTH], arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    int chance;
    int sn, c;
    int lore_skill, wlore_skill;
    bool weapon, all_known;
    
    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("Lore what?\n\r",ch);
        return;
    }
    
    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        if ((obj = get_obj_wear(ch, arg)) == NULL)
        {
            send_to_char("You aren't carrying that.\n\r",ch);
            return;
        }
    }
    org_obj = obj->pIndexData;
    
    lore_skill = get_skill(ch,gsn_lore);
    wlore_skill = get_skill(ch,gsn_weapons_lore);
    weapon = org_obj->item_type == ITEM_WEAPON || org_obj->item_type == ITEM_ARROWS;
    sn = weapon ? gsn_weapons_lore : gsn_lore;

    if ( lore_skill == 0)
        if ( wlore_skill == 0)
        {
            send_to_char("You aren't versed in the lore of Aarchon.\n\r",ch);
            return;
        } 
	else if (!weapon)
        {
            send_to_char("You only know lore regarding weapons.\n\r",ch);
            return;
        }
    
    if ( weapon )
	chance = lore_skill + wlore_skill - lore_skill * wlore_skill / 100;
    else
	chance = lore_skill;

    if ( ch->mana < skill_table[sn].min_mana && skill_table[sn].min_mana > 0 )
    {
        send_to_char("You don't have the presence of mind now.\n\r",ch);
        return;
    }

    if ( IS_IMMORTAL(ch) || IS_NPC(ch) )
	chance = 100;
    else
	WAIT_STATE(ch,skill_table[sn].beats);
    
    act( "You try to figure out what $p is.", ch, obj, NULL, TO_CHAR );
    act( "$n tries to figure out what $p is.", ch, obj, NULL, TO_ROOM );
    
    /* first check if char knows ANYTHING */
    if ( number_percent() > chance
	 || IS_SET(org_obj->extra_flags, ITEM_NO_LORE) )
    {
	ch->mana -= skill_table[sn].min_mana / 2;
	/*
        check_improve(ch,gsn_lore,FALSE,2);
	if ( weapon )
	    check_improve(ch,gsn_weapons_lore,FALSE,2);
	*/
	/* "cant" changed to "can't" - Elik, Jan 16, 2006 */
        send_to_char( "Hmm... you can't seem to place it from any tales you've heard.\n\r", ch );
        act("$n furrows $s brow.\n\r",ch,NULL,NULL,TO_ROOM);
        return;
    }
    
    ch->mana -= skill_table[sn].min_mana;
    /*
    check_improve(ch,gsn_lore,FALSE,2);
    if ( weapon )
	check_improve(ch,gsn_weapons_lore,FALSE,2);
    */

    /* ok, he knows something.. */
    say_basic_obj_index_data( ch, org_obj );
   
    /* now we need to check which affects char knows.. */
    all_known = TRUE;
    
    /* Immortals and NPCs can finally lore random objs */
    if ( !IS_NPC(ch) && !IS_IMMORTAL(ch) )
    {
    for ( paf = org_obj->affected; paf != NULL; paf = paf->next )
      if ( paf->detect_level >= 0 )
	if ( number_percent() <= chance - paf->detect_level )
	  show_affect(ch, paf, TRUE);
	else
	  all_known = FALSE;
    }
    else
    {
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
	    show_affect(ch, paf, TRUE);

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
	    show_affect(ch, paf, TRUE);    
    }

    if ( !all_known )
    {
	send_to_char( "You have a feeling that there might be something else that you didn't remember.\n\r", ch );
	act( "$n scratches $s chin in contemplation.", ch, NULL, NULL, TO_ROOM );
    }

    /* lore won't work with special enchantments */
    /*
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
      show_affect(ch, paf, TRUE);
    */

    /* now let's see if someone else learned something of it --Bobble */
    if ( IS_NPC(ch) )
	return; // prevent easy learning by spamming sage
    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
	if ( IS_NPC(rch) || !IS_AWAKE(rch) || rch == ch )
	    continue;
	check_improve( rch, gsn_lore, 2, TRUE );
	if ( weapon )
	    check_improve( rch, gsn_weapons_lore, 2, TRUE );
    }
}

/* Bobble: used by do_lore & spell_identify */
void show_affect( CHAR_DATA *ch, AFFECT_DATA *paf, bool say_it )
{
  char buf[MSL];

  if ( paf->location != APPLY_NONE && paf->modifier != 0 )
    {
      if ( paf->duration > -1)
	sprintf( buf, "Affects %s by %d for %d hours.", 
		 affect_loc_name( paf->location ), paf->modifier, paf->duration );
      else
	sprintf( buf, "Affects %s by %d.",
		 affect_loc_name( paf->location ), paf->modifier );
      if (say_it)
	do_say(ch, buf);
      else
      {
	send_to_char(buf, ch);
	send_to_char("\n\r", ch);
      }
    }
  
  if (paf->bitvector && paf->where != TO_SPECIAL)
    {
      switch(paf->where)
	{
	case TO_AFFECTS:
	  sprintf(buf,"It adds %s affect.", affect_bit_name(paf->bitvector));
	  break;
	case TO_OBJECT:
	  sprintf(buf,"It adds %s object flag.", extra_bit_name(paf->bitvector));
	  break;
	case TO_WEAPON:
	  sprintf(buf,"It adds %s weapon flags.", weapon_bits_name(paf->bitvector));
	  break;
	case TO_IMMUNE:
	  sprintf(buf,"It grants immunity to %s.", imm_bit_name(paf->bitvector));
	  break;
	case TO_RESIST:
	  sprintf(buf,"It bestows resistance to %s.", imm_bit_name(paf->bitvector));
	  break;
	case TO_VULN:
	  sprintf(buf,"It inflicts vulnerability to %s.", imm_bit_name(paf->bitvector));
	  break;
	default:
	  sprintf(buf,"Unknown bit %d: %d\n\r", paf->where, paf->bitvector);
	  break;
	}
      if (say_it)
	do_say(ch, buf);
      else
      {
	send_to_char(buf, ch);
	send_to_char("\n\r", ch);
      }
    }
} 

void do_appraise ( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH], arg[MAX_INPUT_LENGTH];
    int chance, value, range;
    
    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("Appraise what?\n\r",ch);
        return;
    }
    
    if ((obj = get_obj_carry(ch,arg,ch)) == NULL)
    {
        send_to_char("You aren't carrying that.\n\r",ch);
        return;
    }
    
    if ( (chance = get_skill(ch,gsn_appraise)) == 0 )
    {
        send_to_char("You wouldn't be able to make a guess at its value.\n\r",ch);
        return;
    }
    
    /*
    if (ch->mana < 500/chance)
    {
        send_to_char("You are too distracted.\n\r",ch);
        return;
    }
    */
    
    WAIT_STATE(ch,skill_table[gsn_appraise].beats);
    check_improve(ch,gsn_appraise,TRUE,2);
    /*ch->mana -= 500/chance;*/
    
    value = obj->cost;
    range = value * (100 - chance)/100;
    value = number_range(value-range, value+range);
    
    sprintf( buf,"You appraise %s to be worth about %d silver.\n\r",
	     obj->short_descr, value );
    send_to_char( buf, ch );
    
    return;
}

bool is_disguised( CHAR_DATA *ch )
{
    return is_affected( ch, gsn_disguise );
}

/*
#define MAX_DISGUISE 10
char* get_disguise_name( CHAR_DATA *ch )
{
    static char disguise_names[MAX_DISGUISE][100] =
    {
	"a city guard member",
	"an old man",
	"an old woman",
	"a poor beggar",
	"an innocent peasant",
	"a pregnant woman",
	"a young man",
	"a young woman",
	"a wealthy nobleman",
	"a noble lady",
    };

    AFFECT_DATA *af;
    int index;
    
    for ( af = ch->affected; af != NULL; af = af->next )
	if ( af->type == gsn_disguise )
	    break;

    if ( af == NULL )
	return "not disguised";

    index = af->modifier - 1;
    if ( index < 0 || index >= MAX_DISGUISE )
	return "a bug";
    else
	return disguise_names[index];
}
*/

#define DISGUISE_MIN_VNUM 20
#define DISGUISE_MAX_VNUM 29

MOB_INDEX_DATA* get_disguise_vnum( char *name )
{
    MOB_INDEX_DATA *mob;
    int vnum;

    for ( vnum = DISGUISE_MIN_VNUM; vnum <= DISGUISE_MAX_VNUM; vnum++ )
    {
	mob = get_mob_index( vnum );
	if ( mob == NULL )
	    continue;

	if ( is_name(name, mob->player_name) )
	    return mob;
    }

    return NULL;
}

void show_disguise_list( CHAR_DATA *ch )
{
    MOB_INDEX_DATA *mob;
    int vnum;

    send_to_char( "The following disguises are available:\n\r", ch );
    for ( vnum = DISGUISE_MIN_VNUM; vnum <= DISGUISE_MAX_VNUM; vnum++ )
    {
	if ( (mob = get_mob_index(vnum)) != NULL )
	{
	    send_to_char( mob->player_name, ch );
	    send_to_char( "\n\r", ch );
	}
    }
    
}

/* disguise yourself to avoid killer/thief flag detection */
void do_disguise( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
    char buf[MSL], arg1[MIL];
    int skill;
    MOB_INDEX_DATA *mob;

    one_argument( argument, arg1 );

    if ( (skill = get_skill(ch, gsn_disguise)) < 1 )
    {
	send_to_char( "You change your socks, hoping noone will recognize you anymore.\n\r", ch );
	return;
    }

    if ( !str_cmp(arg1, "remove") )
    {
	if ( is_disguised(ch) )
	{
	    affect_strip( ch, gsn_disguise );
	    send_to_char( "You remove your disguise again.\n\r", ch );
	}
	else
	    send_to_char( "You aren't disguised.\n\r", ch ); 
	return;
    }

    if ( !IS_NPC(ch) && IS_TAG(ch) )
    {
	send_to_char( "You cannot use a disguise in freeze tag.\n\r", ch );
	return;
    }

    if ( !str_cmp(arg1, "list") )
    {
	show_disguise_list( ch );
	return;
    }

    if ( is_disguised(ch) )
    {
	send_to_char( "You are already disguised. Type 'disguise remove' to return to your normal looks.\n\r", ch );
	return;
    }

    if ( arg1[0] == '\0' )
    {
	send_to_char( "What do you want to disguise yourself as?\n\r", ch );
	return;
    }

    if ( (mob = get_disguise_vnum(arg1)) == NULL )
    {
	send_to_char( "You can't disguise as that.\n\r", ch );
	show_disguise_list( ch );
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_disguise].beats );
    if ( number_percent() > skill )
    {
	send_to_char( "Hmm.. nope. You will still get recognized.\n\r", ch );
	check_improve( ch, gsn_disguise, FALSE, 1 );
	return;
    }

    sprintf( buf, "You disguise yourself as %s.\n\r", mob->short_descr );
    send_to_char( buf, ch );
    sprintf( buf, "$n disguises $mself as %s.", mob->short_descr );
    act( buf, ch, NULL, NULL, TO_ROOM );

    af.type      = gsn_disguise;
    af.level     = ch->level;
    af.duration  = get_duration(gsn_disguise, ch->level);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.where     = TO_SPECIAL;
    af.bitvector = mob->vnum;
    affect_to_char( ch, &af );

    check_improve( ch, gsn_disguise, TRUE, 1 );
}

void do_stance_list( CHAR_DATA *ch, char *argument )
{
    int i, skill;
    char buf[MSL];

    send_to_char( "You know the following stances:\n\r", ch );

    for (i = 1; stances[i].name != NULL; i++)
    {
	skill = get_skill(ch, *(stances[i].gsn));
	if ( skill == 0 )
	    continue;

	sprintf( buf, "%-20s %3d%%(%3d%%) %10dmv     %s %s\n\r",
		 stances[i].name,
		 ch->pcdata->learned[*(stances[i].gsn)], skill,
		 stance_cost(ch, i),
		 stances[i].weapon ? "w" : " ",
		 stances[i].martial ? "m" : " ");
	send_to_char( buf, ch );
    }
}

/* gather information about current room */
void do_survey( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *room = ch->in_room;
    char buf[MSL], *buf2;
    int skill = get_skill( ch, gsn_survey );

    if ( room == NULL )
	return;
    
    if ( room_is_dark(room) )
	buf2 = "dark";
    else if ( room_is_dim(room) )
	buf2 = "dim";
    else
	buf2 = "well-lit";

   /* A little feature for blind players */
    sprintf( buf, "The area you are in is : %s{x\n\r", ch->in_room->area->name);
        send_to_char(buf, ch);

    sprintf( buf, "Your immediate surrounding is %s and %s.\n\r", 
	     flag_bit_name(sector_flags, room->sector_type), buf2 );
    send_to_char( buf, ch );
    if ( IS_SET(room->room_flags, ROOM_INDOORS) )
	send_to_char( "It is protected from the weather.\n\r", ch );

    WAIT_STATE( ch, skill_table[gsn_survey].beats );

    /* more info with skill */
    if ( number_percent() < skill )
    {
	if ( room->heal_rate < 0 )
	    send_to_char( "Your wounds will grow worse here.\n\r", ch );
	else if ( room->heal_rate == 0 )
	    send_to_char( "Your wounds will not heal here.\n\r", ch );
	else if ( room->heal_rate < 100 )
	    send_to_char( "Your wounds will heal slowly here.\n\r", ch );
	else if ( room->heal_rate > 100 )
	    send_to_char( "Your wounds will heal quickly here.\n\r", ch );

	if ( room->mana_rate < 0 )
	    send_to_char( "Your mind will grow dim here.\n\r", ch );
	else if ( room->mana_rate == 0 )
	    send_to_char( "Your mind will not clear here.\n\r", ch );
	else if ( room->mana_rate < 100 )
	    send_to_char( "Your mind will clear slowly here.\n\r", ch );
	else if ( room->mana_rate > 100 )
	    send_to_char( "Your mind will clear quickly here.\n\r", ch );

	if ( IS_SET(room->room_flags, ROOM_DARK) )
	    send_to_char( "It's always dark here.\n\r", ch );
	if ( IS_SET(room->room_flags, ROOM_SAFE) )
	    send_to_char( "It seems safe to rest.\n\r", ch );
 	if ( IS_SET(room->room_flags, ROOM_NO_SCOUT) )
	    send_to_char( "It seems hard to find.\n\r", ch );
 	if ( IS_SET(room->room_flags, ROOM_LAW) )
	    send_to_char( "The law is held up here.\n\r", ch );
 	if ( IS_SET(room->room_flags, ROOM_ARENA) )
	    send_to_char( "You can fight safely here.\n\r", ch );

	check_improve( ch, gsn_survey, TRUE, 3 );
    }
    else
	check_improve( ch, gsn_survey, FALSE, 3 );
}


/*  NEW do_score, do_attributes, and do_worth by Quirky in May 03 */
void do_score( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH], flagsbuf[MAX_STRING_LENGTH/4];
    char custombuf[MAX_STRING_LENGTH];
    char alignbuf[MAX_STRING_LENGTH/2], positionbuf[MAX_STRING_LENGTH/4];
    char remortbuf[MAX_STRING_LENGTH/4], temp[MAX_STRING_LENGTH];
    BUFFER *output;
    RELIGION_DATA *religion = NULL;
    int align, thirst, hunger, encumber;
    int LENGTH = 75;


    /* Added by maedhros to toggle old score */

    if (IS_SET(ch->togg, TOGG_OLDSCORE))
    {
      do_oldscore (ch, "oldscore");
      return;
    }
    else

    
    /* setup */

    /* custombuf */
    if ( ch->pcdata != NULL && ch->pcdata->customflag[0] != '\0' )
        sprintf( custombuf, "(%s) ", ch->pcdata->customflag );
    else
        sprintf( custombuf, "" );

    /* flagsbuf */
    if ( !IS_NPC(ch) )
    {
        sprintf( flagsbuf, "%s%s%s%s%s",
            IS_SET(ch->comm, COMM_AFK) ? "[AFK] " : "",
            IS_SET(ch->act,PLR_HELPER) ? 
                (!IS_SET(ch->act, PLR_INACTIVE_HELPER) ? "({GH{CE{cL{GP{CE{cR{G!{x) " : "{G*{x ") : "",
            IS_SET(ch->act,PLR_RP) ? "[RP] " : "",
            IS_SET(ch->act,PLR_KILLER) ? "(KILLER) " : "",
            IS_SET(ch->act,PLR_THIEF) ? "(THIEF)" : "" );
    }
    else
        sprintf( flagsbuf, "" );

    /* remortbuf */
    if( ch->pcdata != NULL )
    {
        if( !IS_IMMORTAL(ch) )
            sprintf( remortbuf, " (Remort %d)", ch->pcdata->remorts );
        else if ( get_trust(ch) != ch->level )
            sprintf( remortbuf, " (Trust %d)", get_trust(ch) );
        else sprintf( remortbuf, "" );
    } 
    else
        sprintf( remortbuf, "" );

    /* alignbuf */
    align = ch->alignment;
    if( align > 900 )       sprintf( alignbuf, "{b%-5d (angelic){x ", align );
    else if( align >  700 ) sprintf( alignbuf, "{b%-5d (saintly){x ", align );
    else if( align >  350 ) sprintf( alignbuf, "{b%-5d (good){x    ", align );
    else if( align >  100 ) sprintf( alignbuf, "{m%-5d (kind){x    ", align );
    else if( align > -100 ) sprintf( alignbuf, "{m%-5d (neutral){x ", align );
    else if( align > -350 ) sprintf( alignbuf, "{m%-5d (mean){x    ", align );
    else if( align > -700 ) sprintf( alignbuf, "{r%-5d (evil){x    ", align );
    else if( align > -900 ) sprintf( alignbuf, "{r%-5d (demonic){x ", align );
    else                    sprintf( alignbuf, "{r%-5d (satanic){x ", align );

    /* positionbuf */
    switch( ch->position )
    {
    case POS_DEAD:	sprintf( positionbuf, "DEAD" );  break;
    case POS_MORTAL:	sprintf( positionbuf, "mortally wounded" );  break;
    case POS_INCAP:	sprintf( positionbuf, "incapacitated" );  break;
    case POS_STUNNED:	sprintf( positionbuf, "stunned" );  break;
    case POS_SLEEPING:	sprintf( positionbuf, "sleeping" );  break;
    case POS_RESTING:	sprintf( positionbuf, "resting" );  break;
    case POS_SITTING:	sprintf( positionbuf, "sitting" );  break;
    case POS_STANDING:	sprintf( positionbuf, "standing" );  break;
    case POS_FIGHTING:	sprintf( positionbuf, "fighting" );  break;
    }


    output = new_buf();

    /******* THE NEW, IMPROVED SCORE SCREEN *******/

    add_buf(output, "{D:===========================================================================:{x\n\r");
    /* Changed to allow for pretitle -- Maedhros 11/12/11 */
    /* sprintf(buf, "{D|{x %s%s%s%s{x%s", flagsbuf, custombuf,IS_NPC(ch) ? "" : ch->pcdata->title ); */
    sprintf(buf, "{D|{x %s%s%s%s%s{x%s", flagsbuf, custombuf,IS_NPC(ch) ? "" : ch->pcdata->name_color, IS_NPC(ch)?"":ch->pcdata->pre_title, ch->name,IS_NPC(ch) ? "" : ch->pcdata->title);

    /* Only add the ending {D|{x if the line is short enough, otherwise just go to newline */
    if( strlen_color(buf) <= LENGTH )
    {
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
    }
    else strcat( buf, "\n\r" );
    add_buf(output, buf );

    /* Level, Remorts/Trust,  Clan Details */
    sprintf(buf, "{D|{x Level: %d%s", ch->level, remortbuf );

    if( !IS_NPC(ch) && clan_table[ch->clan].active )
    {
        for( ; strlen_color(buf) <= 24; strcat(buf, " ") );
        sprintf(temp, "Clan: %s%s{x",
            clan_table[ch->clan].who_color,
            clan_table[ch->clan].who_name   );
        for( ; strlen_color(temp) <= 19; strcat(temp, " ") );
        strcat( buf, temp );
        sprintf(temp, "Rank: %s%s{x",
            clan_table[ch->clan].who_color,
            capitalize(clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].name) );
        strcat( buf, temp );
    }
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf(output, buf );

    /* Class, Race, Sex */
    sprintf(buf, "{D|{x Class: %s", IS_NPC(ch) ? "mobile" : class_table[ch->class].name );
    for( ; strlen_color(buf) <= 24; strcat(buf, " ") );
    sprintf(temp, "Race: %s", race_table[ch->race].name );
    for( ; strlen_color(temp) <= 19; strcat(temp, " ") );
    strcat( buf, temp );
    sprintf(temp, "Gender: %s",
         ch->sex == 0 ? "{msexless{x" : ch->sex == 1 ? "{bmale{x" : "{rfemale{x" );
    strcat( buf, temp );
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf(output, buf );

    add_buf(output, "{D:===========================================================================:{x\n\r");

    /* Some players-only stuff */
    if( !IS_NPC(ch) )
    {
        /* Age, hours */
        sprintf(buf, "{D|{x Age:  %d years  (Played: %d hours)",
            get_age(ch), (ch->played + (int)(current_time - ch->logon))/3600 );
        for( ; strlen_color(buf) <= 40; strcat(buf, " ") );

        /* on same line as above, Marriage details */
        if( ch->pcdata->spouse )
            sprintf( temp, "You are married to %s.", ch->pcdata->spouse );
        else
            sprintf( temp, "You are single." );
        strcat( buf, temp );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );

        /* Imm stuff */
        if( IS_IMMORTAL(ch) )
        {
            sprintf( buf, "{D|{x Holylight:  %s   ", IS_SET(ch->act, PLR_HOLYLIGHT) ? "{WON{x" : "{WOFF{x" );
            if( ch->invis_level )
            {
                sprintf( temp, "Wizinvis: level {W%d{x   ", ch->invis_level );
                strcat( buf, temp );
            }
            if( ch->incog_level )
            {
                sprintf( temp, "Incognito: level {W%d{x", ch->incog_level );
                strcat( buf, temp );
            }
            for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
            strcat( buf, "{D|{x\n\r" );
            add_buf( output, buf );
        }

        /* Practices, trains */
        if( !IS_HERO(ch) && ch->pcdata->highest_level <= ch->level )
            sprintf( temp, "{c(Expect to gain about{x %.2f {cnext level.)", ch_prac_gains(ch, ch->level + 1)/100.0 );
        else
            sprintf( temp, "" );

        sprintf( buf, "{D|{x Practices:  {C%-5d{x  %s", ch->practice, temp );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );

        int hp_cap, mana_cap, move_cap;
        get_hmm_softcap( ch, &hp_cap, &mana_cap, &move_cap );
        sprintf( buf, "{D|{x Trains:     {C%-5d {cSpent:{x %d/%d {chp,{x %d/%d {cmn,{x %d/%d {cmv  {c(MAX %d){x",
            ch->train, ch->pcdata->trained_hit, hp_cap, ch->pcdata->trained_mana, mana_cap, ch->pcdata->trained_move, move_cap, max_hmm_train(ch->level) );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );

    }   /* end of "some players-only stuff" section */

    sprintf( buf, "{D|{x Alignment:  %s ", alignbuf );

    /* Religion Stuff, same line as Align since they are related .. plus another line below */
    if( ch->pcdata != NULL && (religion = get_religion(ch)) != NULL )
    {
        /* Finishing the "Align" line with some Religion stuff */
        for ( ; strlen_color(buf) <= 40; strcat( buf, " " ));
        sprintf( temp, "Your religion allows %d to %d.", religion->min_align, religion->max_align );
        strcat( buf, temp );

        /* Split the two Religion lines with the proper formatting stuff */
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );

        /* Second line of Religion Stuff */
        if( IS_IMMORTAL(ch) )
        {
            bool rel_at_altar = FALSE;
            if( get_religion_of_altar( get_obj_room(religion->relic_obj) ) == religion )
                rel_at_altar = TRUE;

            sprintf( buf, "{D|{x Faith Pool: {B%d{x", religion->god_power );
            for( ; strlen_color(buf) <= 40; strcat(buf, " ") );
            sprintf( temp, "Relic: %s", !rel_at_altar ? "{Rmissing{x" : "safe" );
            strcat( buf, temp );
        }
        else
        {
            char *rank = get_ch_rank_name(ch);

            sprintf( buf, "{D|{x Faith in %s: {B%5d{x (%s%s of %s) ",
                religion->god, get_faith(ch), rank,
                is_priest(ch) ? (is_high_priest(ch) ? " & High Priest" : " & Priest") : "",
                religion->name );
        }
    }
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );
    /* end of align and religion stuff */

    /* More player-only stuff */
    if( ch->pcdata != NULL )
    {
        if( !IS_IMMORTAL(ch) )
        {
            sprintf( temp, "Mobkills:   %d kills, %d deaths",
	        ch->pcdata->mob_kills, ch->pcdata->mob_deaths );
	    for ( ; strlen_color(temp) <= 47; strcat( temp, " " ));
                sprintf(buf, "{D| {x %s Beheads: %d",
	        temp, ch->pcdata->behead_cnt );

	    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " "));
	    strcat( buf, "{D|{x\n\r" );
	    add_buf(output, buf);

	    /* Warfare grade and number of kills */
	    sprintf( buf, "{D|{x   Warfare:  Grade {W<{x%s{W>{x (%d points from %d warkills)",
		pkgrade_table[get_pkgrade_level(ch->pcdata->warpoints)].grade,
		ch->pcdata->warpoints, ch->pcdata->war_kills );
	    for( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
	    strcat( buf, "{D|{x\n\r" );
	    add_buf( output, buf );

        /* Pkill status */
        /* This section has been moved from up above to make the PKill
           count display below warfare grade. Astark Oct 2012 */
        if( IS_SET(ch->act, PLR_PERM_PKILL) )
        {
			sprintf( buf, "{D|{x   %s:  Kills {W<{R%d{W>{x ",
				IS_SET(ch->act, PLR_HARDCORE) ? "PK (HC)" : "  Pkill",
				ch->pcdata->pkill_count);
			for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
            strcat( buf, "{D|{x\n\r" );
            add_buf( output, buf );
        }

            /* ** Pflag hours remaining ** */
            if( custombuf[0] != '\0' )
            {
                sprintf( buf, "{D|{x Your %sflag has %d hours remaining. ", custombuf, ch->pcdata->customduration );
                for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
                strcat( buf, "{D|{x\n\r" );
                add_buf( output, buf );
            }

        } /* end of some mortal-only stuff */

    } /* end of more players-only stuff */

    /* ** Position, drunk, hunger, thirst, smoke ** */
    sprintf( buf, "{D|{x You are %s", positionbuf );
    if( ch->pcdata != NULL )
    {
        bool hungry = FALSE;
        bool thirsty = FALSE;

        if( ch->pcdata->condition[COND_DRUNK] > 10 )
            strcat( buf, ", drunk" );
        else if( ch->pcdata->condition[COND_DRUNK] > 20 )
            strcat( buf, ", VERY drunk" );

        thirst = ch->pcdata->condition[COND_THIRST];
        hunger = ch->pcdata->condition[COND_HUNGER];

        if( (hunger >= 0 && hunger < 20) )  hungry = TRUE;
        if( (thirst >= 0 && thirst < 20) )  thirsty = TRUE;

        if( hungry || thirsty ) strcat( buf, ", feeling " );

        if( hungry )
        {
            if( hunger == 0 )
                strcat( buf, "{rvery{x hungry" );
            else if( (hunger > 0 && hunger < 20) )
                strcat( buf, "hungry" );
        }

        if( thirsty )
        {
            if( hungry ) strcat( buf, " and " );

            if( thirst == 0 )
                strcat( buf, "{rvery{x thirsty" );
            else if( (thirst > 0 && thirst < 20) )
                strcat( buf, "thirsty" );
        }

        /* Smoke code, not fully implemented yet */
        if( ch->pcdata->condition[COND_SMOKE] < 0 )
            strcat( buf, ", and you need a smoke" );
    }
    strcat( buf, "." );
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );

    /* ** Carry details ** */
    sprintf( buf, "{D|{x Carrying: %d/%d items (%d/%d pounds).",
        ch->carry_number, can_carry_n(ch), get_carry_weight(ch)/10, can_carry_w(ch)/10 );
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );

    /* ** Encumberance ** */
    if( (encumber = get_encumberance(ch)) > 0 )
    {
        if( encumber <= 25 )      sprintf( buf, "{D|{x You are slightly encumbered." );
        else if( encumber <= 50 ) sprintf( buf, "{D|{x You are seriously encumbered." );
        else if( encumber <= 75 ) sprintf( buf, "{D|{x You are heavily encumbered." );
        else   sprintf( buf, "{D|{x {RYou can hardly move under the weight you carry!{x" );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );
    }

    /* ** More players-only stuff ** */
    if( ch->pcdata != NULL )
    {
        /* Morph info, and there's also room for STANCE details here */
        if( ch->race == race_doppelganger )
        {
            if( ch->pcdata->morph_race > 0 )
                sprintf( buf, "{D|{x Morph race: {G%s{x with {G%d{x hours remaining.   ",
                    race_table[ch->pcdata->morph_race].name,  ch->pcdata->morph_time );
            else
                sprintf( buf, "{D|{x You are currently in {Gbasic form{x.   " );

            if( ch->stance != 0 )
            {
                sprintf( temp, "Stance: {G%s{x", capitalize(stances[ch->stance].name) );
                strcat( buf, temp );
            }

            for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
            strcat( buf, "{D|{x\n\r" );
            add_buf( output, buf );
        }
        else if( ch->race == race_naga )
        {
            if( ch->pcdata->morph_race == 0 )
                sprintf( buf, "{D|{x You are currently in {Gserpent form{x.   " );
            else
                sprintf( buf, "{D|{x You are currently in humanoid form.   " );

            if( ch->stance != 0 )
            {
                sprintf( temp, "Stance: {G%s{x", capitalize(stances[ch->stance].name) );
                strcat( buf, temp );
            }

            for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
            strcat( buf, "{D|{x\n\r" );
            add_buf( output, buf );
        }
        else if( ch->stance != 0 )
        {
            sprintf( buf, "{D|{x Stance: {G%s{x", capitalize(stances[ch->stance].name) );
            for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
            strcat( buf, "{D|{x\n\r" );
            add_buf( output, buf );
        }

    }

    add_buf(output, "{D:===========================================================================:{x\n\r");
    page_to_char(buf_string(output),ch);
    free_buf(output);


    if ( IS_SET(ch->comm, COMM_SHOW_WORTH) )
        do_worth(ch, "for_score");
    if ( IS_SET(ch->comm, COMM_SHOW_ATTRIB) )
        do_attributes(ch, "for_score");
    if ( IS_SET(ch->comm, COMM_SHOW_PERCENT) )
        do_percentages(ch, "for_score");
    if ( !IS_NPC(ch) && ch->penalty )
        show_penalties_by_player(ch, ch->name, TIME_PLAYED(ch), 2);
    if ( IS_SET(ch->comm, COMM_SHOW_AFFECTS) )
        do_affects(ch, "for_score");

    return;
}





void do_worth( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH], temp[MAX_STRING_LENGTH];
    int LENGTH = 75;
    int level_soon = ch->level + ch->exp;
    int totalquests;

    if( !strcmp(argument,"for_score") )
    ;
    else send_to_char("{D:===========================================================================:{x\n\r", ch);

    if ( !ch->pcdata )
    {
        sprintf(buf, "{D|{x You have {C%ld gold{x and {W%ld silver{x.", ch->gold, ch->silver );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        send_to_char( buf, ch );
        send_to_char("{D:===========================================================================:{x\n\r", ch);
        return;
    }
  
    /* *** MONEY *** */

    /* First, some formatting ... want to make things line up :P */
    sprintf( temp, "{Y%ld gold{x, {W%ld silver{x", ch->gold, ch->silver );
    for ( ; strlen_color(temp) <= 35; strcat( temp, " " ));

    sprintf( buf, "{D|{x Assets:     %s  Bank: %ld gold",
        temp, ch->pcdata->bank );

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    send_to_char( buf, ch );

    if( ch->pcdata->house > 0 )
    {
        sprintf( buf, "{D|{x             Your house is worth %dk gold.", ch->pcdata->house/1000 );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        send_to_char( buf, ch );
    }

    /* *** EXPERIENCE *** */
    sprintf( buf, "{D|{x Experience: %d real exp", ch->exp );
    if( ch->pcdata->field > 0 )
    {
	sprintf( temp, " and %d field", ch->pcdata->field );
        strcat( buf, temp );
    }
    if ( !IS_HERO(ch) )
    {
        sprintf( temp, " (%d exp to reach lvl %d)",
            (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp, ch->level + 1 );
        strcat( buf, temp );
    }
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    send_to_char( buf, ch );

    /* ACHIEVEMENT POINTS  -- Astark Sep 2012 */

    sprintf( buf, "{D|{x You have    {c%d{x achievement point%s",
                ch->pcdata->achpoints, ch->pcdata->achpoints == 1 ? "" : "s" );
    for( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    send_to_char( buf, ch );


    /* *** QUESTING *** */
    totalquests = ch->pcdata->quest_success + ch->pcdata->quest_failed;

    /* Bit o formatting to match with the Assets line :P */
    sprintf( temp, "You have {B%d{x quest point%s",
                ch->pcdata->questpoints, ch->pcdata->questpoints == 1 ? "" : "s" );
    for( ; strlen_color(temp) <= 35; strcat( temp, " " ));

    sprintf( buf, "{D|{x Quests:     %s  %d/%d (%3.1f%%)", temp,
                ch->pcdata->quest_success, totalquests, ch->pcdata->quest_success == 0 ? 0 :
                (float)ch->pcdata->quest_success * 100 / (float)totalquests );

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    send_to_char( buf, ch );

    if ( ch->pcdata->bounty > 0 )
    {
        sprintf( buf, "{D|{x There is a bounty of %d gold on your head.", ch->pcdata->bounty );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        send_to_char( buf, ch );
    }

    send_to_char("{D:===========================================================================:{x\n\r", ch);

    return;
}



void do_attributes( CHAR_DATA *ch, char *argument )
{
    /* Just a hack of score to give players a convenient way to view main attributes */
    /* Memnoch 02/98 */
    /* Updated spring 2004 by Quirky along with 'score' and 'worth' */

    char buf[MAX_STRING_LENGTH], temp[MAX_STRING_LENGTH];
    BUFFER *output;
    int LENGTH = 75;
    char hp_col, mn_col, mv_col;   /* Colours that vary depending on current hp/mana/mv */

    hp_col = (ch->hit == ch->max_hit)    ? 'W' :
            (ch->hit >= ch->max_hit*.85) ? 'G' :
            (ch->hit >= ch->max_hit*.66) ? 'g' :
            (ch->hit >= ch->max_hit*.50) ? 'Y' :
            (ch->hit >= ch->max_hit*.33) ? 'y' :
            (ch->hit >= ch->max_hit*.16) ? 'R' : 'r';
    
    mn_col = (ch->mana == ch->max_mana)    ? 'W' :
            (ch->mana >= ch->max_mana*.85) ? 'G' :
            (ch->mana >= ch->max_mana*.66) ? 'g' :
            (ch->mana >= ch->max_mana*.50) ? 'Y' :
            (ch->mana >= ch->max_mana*.33) ? 'y' :
            (ch->mana >= ch->max_mana*.16) ? 'R' : 'r';

    mv_col = (ch->move == ch->max_move)    ? 'W' :
            (ch->move >= ch->max_move*.85) ? 'G' :
            (ch->move >= ch->max_move*.66) ? 'g' :
            (ch->move >= ch->max_move*.50) ? 'Y' :
            (ch->move >= ch->max_move*.33) ? 'y' :
            (ch->hit >= ch->max_hit*.16) ? 'R' : 'r';

    output = new_buf();
    if( strcmp(argument,"for_score") )
        add_buf(output, "{D:===========================================================================:{x\n\r");

    /* ** hp, mana, mv ** */
    sprintf( buf, "{D|{x {CHp:{x {%c%d{x/%d  {CMana:{x {%c%d{x/%d  {CMoves:{x {%c%d{x/%d",
        hp_col, ch->hit, ch->max_hit,
        mn_col, ch->mana, ch->max_mana, mv_col, ch->move, ch->max_move );
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );

    /* ** Wimpy (way better on its own line) ** */
    sprintf( buf, "{D|{x {cWimpy:{x %d%%  {cCalm:{x %d%%", ch->wimpy, ch->calm );
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );

    /* ** Stats ** */
    if (IS_SET(ch->togg, TOGG_STATBARS))
        print_stat_bars( ch, output );
    else
    {
        sprintf( buf, "{D|{x {bStr:{x %3d(%3d)  {bCon:{x %3d(%3d)  {bVit:{x %3d(%3d)  {bAgi:{x %3d(%3d)  {bDex:{x %3d(%3d) {D|{x\n\r",
            ch->perm_stat[STAT_STR], get_curr_stat(ch,STAT_STR),
            ch->perm_stat[STAT_CON], get_curr_stat(ch,STAT_CON),
            ch->perm_stat[STAT_VIT], get_curr_stat(ch,STAT_VIT),
            ch->perm_stat[STAT_AGI], get_curr_stat(ch,STAT_AGI),
            ch->perm_stat[STAT_DEX], get_curr_stat(ch,STAT_DEX)  );
        add_buf( output, buf );

        sprintf( buf, "{D|{x {bInt:{x %3d(%3d)  {bWis:{x %3d(%3d)  {bDis:{x %3d(%3d)  {bCha:{x %3d(%3d)  {bLuc:{x %3d(%3d) {D|{x\n\r",
            ch->perm_stat[STAT_INT], get_curr_stat(ch,STAT_INT),
            ch->perm_stat[STAT_WIS], get_curr_stat(ch,STAT_WIS),
            ch->perm_stat[STAT_DIS], get_curr_stat(ch,STAT_DIS),
            ch->perm_stat[STAT_CHA], get_curr_stat(ch,STAT_CHA),
            ch->perm_stat[STAT_LUC], get_curr_stat(ch,STAT_LUC)  );
        add_buf( output, buf );
    }
    /* ** Armor Class ** */
    if( IS_NPC(ch) || ch->level >= 25 )
    {
        sprintf( buf, "{D|{x      {CA{crmor {CC{class:{x %5d, %d, %d, %d {c(pierce, bash, slash, magic){x",
            GET_AC(ch,AC_PIERCE), GET_AC(ch,AC_BASH), GET_AC(ch,AC_SLASH), GET_AC(ch,AC_EXOTIC) );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );

        /* ** Hitroll, damroll ** */
        sprintf( buf, "{D|{x {CHit{croll, {CDam{croll:{x %5d, %d     {CSaves, Physical:{x %5d, %d",
            GET_HITROLL(ch),  GET_DAMROLL(ch), get_save(ch, FALSE), get_save(ch, TRUE) );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );
    }
    else
    {
        int i;
        int avg = 0;
        char rating[MAX_STRING_LENGTH/4];

        avg = (GET_AC(ch,0) + GET_AC(ch,1) + GET_AC(ch,2) + GET_AC(ch,3) )/4;

        if( avg >= 101 )        sprintf( rating, "shameful"  );
            else if( avg >= 80 )    sprintf( rating, "terrible"  );
            else if( avg >= 60 )    sprintf( rating, "very weak" );
            else if( avg >= 40 )    sprintf( rating, "weak"      );
            else if( avg >= 20 )    sprintf( rating, "so-so"     );
            else if( avg >= 0 )     sprintf( rating, "passable"  );
            else if( avg >= -20 )   sprintf( rating, "decent"    );
            else if( avg >= -40 )   sprintf( rating, "good"      );
            else if( avg >= -60 )   sprintf( rating, "great"     );
            else if( avg >= -80 )   sprintf( rating, "superb"    );
            else if( avg >= -100 )  sprintf( rating, "excellent" );
            else if( avg >= -150 )  sprintf( rating, "amazing!"  );
            else                             sprintf( rating, "fantastic!" );

        sprintf( buf, "{D|{x {CA{crmor {CC{class (Avg):{x %d (%s)   {CHit{croll, {CDam{croll:{x %d, %d",
            avg, rating, GET_HITROLL(ch), GET_DAMROLL(ch) );

        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );
    }

    add_buf(output, "{D:===========================================================================:{x\n\r");
    page_to_char(buf_string(output),ch);
    free_buf(output);

    return;
}

/* Show Str..Luc bonuses as a bar graph -- Bobble 02/2013 */
void print_stat_bars( CHAR_DATA *ch, BUFFER *output )
{
    const int MAX_COST = 8;
    char bar_buf[256], buf[MAX_STRING_LENGTH];
    int bar_next;
    int si, bonus, cost, partial, i;
    bool color_switched;
    struct stat_type *stat;
    
    for (si = 0; si < MAX_STATS; si++)
    {
        stat = &stat_table[si];
        bonus = ch->mod_stat[stat->stat];
        // generate string for bar graph
        bar_next = 0;
        color_switched = FALSE;
        for (cost = 1; cost <= MAX_COST; cost++)
        {
            for (i = 0; i < 5; i++)
            {
                partial = URANGE(0, bonus, cost);
                bonus -= partial;
                if (partial < cost && !color_switched)
                {
                    // red color
                    bar_buf[bar_next++] = '{';
                    bar_buf[bar_next++] = 'r';
                    color_switched = TRUE;
                }
                bar_buf[bar_next++] = '0' + partial;
            }
            if (cost < MAX_COST)
                bar_buf[bar_next++] = ' ';
        }
        // terminate string
        bar_buf[bar_next++] = 0;
        // now send it to output
        sprintf( buf, "{D|{x {b%s: {x%3d %s %3d => %3d  [{g%s{x]  {D|{x\n\r"
            , stat_table[si].abbreviation
            , ch->perm_stat[stat->stat]
            , ch->mod_stat[stat->stat] < 0 ? "-" : "+"
            , ABS(ch->mod_stat[stat->stat])
            , get_curr_stat(ch,stat->stat)
            , bar_buf
        );
        add_buf( output, buf );        
    }
}

// show offhand attack and other percentages
void do_percentages( CHAR_DATA *ch, char *argument )
{
    BUFFER *output;
    int LENGTH = 76;

    output = new_buf();
    if ( strcmp(argument,"for_score") )
        add_buf(output, "{D:===========================================================================:{x\n\r");

    // secondary and two-handed weapons
    add_buff_pad(output, LENGTH, "{D|{x {cOffhand Attacks:{x %3d%%   {cTwohand Bonus:{x %3d%%     {cFocus Bonus:{x %3d%%",
        offhand_attack_chance(ch, FALSE),
        get_twohand_bonus(ch, get_eq_char(ch, WEAR_WIELD), FALSE),
        get_focus_bonus(ch)
    );
    add_buf(output, "{D|{x\n\r");

    // dodge, parry, block
    add_buff_pad(output, LENGTH, "{D|{x           {cDodge:{x %3d%%           {cParry:{x %3d%%           {cBlock:{x %3d%%",
        dodge_chance(ch, ch->fighting, FALSE),
        parry_chance(ch, ch->fighting, FALSE),
        shield_block_chance(ch, FALSE)
    );
    add_buf(output, "{D|{x\n\r");
    
    add_buf(output, "{D:===========================================================================:{x\n\r");
    page_to_char(buf_string(output), ch);
    free_buf(output);

    return;
}

void do_helper( CHAR_DATA *ch, char *argument )
{
        CHAR_DATA *victim;
        char arg[MIL];

        if( IS_IMMORTAL(ch) )
        {
            argument = one_argument( argument, arg );

            if( arg[0] == '\0' )
            {
                send_to_char( "Set whose helper flag?\n\r", ch );
                return;
            }

            if( (victim = get_char_world(ch,arg)) == NULL )
            {
                send_to_char( "They aren't here.\n\r", ch );
                return;
            }
            else
            {
                if( IS_SET(victim->act, PLR_HELPER) )
                {
                    REMOVE_BIT( victim->act, PLR_HELPER );
                    REMOVE_BIT( victim->act, PLR_INACTIVE_HELPER );
                    act( "$N's {GH{CE{cL{GP{CE{cR{x status removed.", ch, NULL, victim, TO_CHAR );
                    act( "Your {GH{CE{cL{GP{CE{cR{x status has been removed.", ch, NULL, victim, TO_VICT );
                    return;
                }
                else
                {
                    SET_BIT( victim->act, PLR_HELPER );
                    REMOVE_BIT( victim->act, PLR_INACTIVE_HELPER );
                    act( "$N is now a {GH{CE{cL{GP{CE{cR{x.", ch, NULL, victim, TO_CHAR );
                    act( "You are now a {GH{CE{cL{GP{CE{cR{x!", ch, NULL, victim, TO_VICT );
                    return;
                }
            }
        }

        if( !IS_HELPER(ch) )
        {
            SET_BIT( ch->act, PLR_INACTIVE_HELPER );
            return;
        }
        else
        {
            send_to_char( "Helper status set to 'active'.  Type 'helper' again to toggle.\n\r", ch );
            REMOVE_BIT( ch->act, PLR_INACTIVE_HELPER );
            return;
        }
}


void print_affect( CHAR_DATA *ch, AFFECT_DATA *paf, FILE *fp )
{
  char buf[MSL];
  bool say_it = FALSE;

  if ( paf->location != APPLY_NONE && paf->modifier != 0 )
    {
      if ( paf->duration > -1)
        fprintf( fp, "%s by %d for %d hours.",
                 affect_loc_name( paf->location ), paf->modifier, paf->duration );
      else
        fprintf( fp, "%s\n\r%d",
                 affect_loc_name( paf->location ), paf->modifier );
      if (say_it)
        do_say(ch, buf);
      else
      {
        //send_to_char(buf, ch);
        fprintf( fp, "\n\r");
      }
    }

  if (paf->bitvector && paf->where != TO_SPECIAL)
    {
      switch(paf->where)
        {
        case TO_AFFECTS:
          fprintf( fp, "Adds%s", affect_bit_name(paf->bitvector));
          break;
        case TO_OBJECT:
          fprintf( fp, "It adds %s object flag.", extra_bit_name(paf->bitvector));
          break;
        case TO_WEAPON:
          fprintf( fp, "It adds %s weapon flags.", weapon_bits_name(paf->bitvector));
          break;
        case TO_IMMUNE:
          fprintf( fp, "It grants immunity to %s.", imm_bit_name(paf->bitvector));
          break;
        case TO_RESIST:
          fprintf( fp, "It bestows resistance to %s.", imm_bit_name(paf->bitvector));
          break;
        case TO_VULN:
          fprintf( fp, "It inflicts vulnerability to %s.", imm_bit_name(paf->bitvector));
          break;
        default:
          fprintf( fp, "Unknown bit %d: %d\n\r", paf->where, paf->bitvector);
          break;
        }
      if (say_it)
        do_say(ch, buf);
      else
      {
        //send_to_char(buf, ch);
        fprintf( fp, "\n\r");
      }
    }
}

/*
void do_combo( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( ch->combo_points > 0 )
    {
        sprintf( buf, "You have %d combo point(s).\n\r", ch->combo_points );
        send_to_char( buf, ch );
    }
    else
    {
    send_to_char("You do not have any combo points at this time.\n\r", ch);
    }

    return;
} 
*/

msl_string achievement_display [] =
{
        "none",
        "Level",
        "M.Kills",
        "Remorts",
        "Q.Complete",
        "Warkills",
        "War Wins",
        "Beheads",
        "Pkills",
        "Age",
        "Max HP",
        "Max Mana",
        "Max Moves",
        "Explored"
};


void do_achievements( CHAR_DATA *ch, char *argument )
{
	if ( IS_NPC(ch) )
		return;

	if (!strcmp( argument, "rewards") )
	{
		print_ach_rewards(ch);
		return;
	}
	
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char buf2[MSL];
    int i;
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    BUFFER *output;
    int col;
    int totalach = 0;
    int ltotal = 0;
    int utotal = 0;

    col = 0;

    output = new_buf();

    if (argument[0] == '\0')
    {
	victim = ch;
    }
    else
    {
    	d = new_descriptor();
    
    	if (!load_char_obj(d, argument))
    	{
           send_to_char("Character not found.\n\r", ch);
           free_descriptor(d);
           return;
    	}
        victim = d->character;
    }

    if (victim == NULL)
    {
        send_to_char("Player not found.\n\r",ch);
        return;
    }

    sprintf(buf, "\n\r");
    add_buf(output,buf);
    sprintf(buf, "{WAchievements for %s\n\r", victim->name);
    add_buf(output,buf);
    add_buf(output,"{w----------------------------\n\r");
    for (i = 0; achievement_table[i].bit_vector != 0; i++)
    {
  	sprintf(buf, "{w%-10s %6d: ", achievement_display[achievement_table[i].type], achievement_table[i].limit);
	add_buf(output, buf);
        totalach += 1;

	if (IS_SET(victim->pcdata->achievements, achievement_table[i].bit_vector))
        {
            add_buf(output,"{yAchvd{x");
            utotal += 1;
        }
        else
        {
            add_buf(output,"{DLockd{x");
            ltotal += 1;
        }
	col +=1;
	if ( col % 3 == 0 )
    	  add_buf(output, "\n\r");
	else
	  add_buf(output, " | ");

    }
    if ( col % 3 != 0 )
      add_buf(output, "\n\r");

    sprintf( buf, "{w\n\rTotal Achievements: %d, Total Unlocked: %d, Total Locked: %d{x\n\r", totalach, utotal, ltotal);
    add_buf(output,buf);
	add_buf(output, "(Use 'achievement rewards' to see rewards table.)\n\r");
    page_to_char(buf_string(output),ch);
    free_buf(output);
	
}

void print_ach_rewards(CHAR_DATA *ch)
{
	BUFFER *output;
	char buf[MSL];
	char header[MSL];
	int i;
	 
	
	output = new_buf();
	int type=-1;
	sprintf(header, "\n\r{w%-10s %6s:{x {W%6s{x|{W%6s{x|{W%6s{x|{W%6s{x\n\r", "Type", "Limit", "QP", "Exp", "Gold", "AchP");

	for (i = 0; achievement_table[i].bit_vector != 0; i++)
	{
		ACHIEVEMENT *entry=&achievement_table[i];
		
		if ( entry->type != type )
		{
			type= entry->type;
			add_buf(output, header);
		}
		
		sprintf(buf, "{w%-10s %6d{x: {y%6d{x|{c%6d{x|{y%6d{x|{c%6d{x\n\r", achievement_display[entry->type], entry->limit, entry->quest_reward, entry->exp_reward, entry->gold_reward, entry->ach_reward);
		add_buf(output,buf);
	}
	page_to_char( buf_string(output), ch );
	free_buf( output );
		

}



/* For achievement rewards... This gets called at certain times (level up, quest complete, etc. )--Vodur / Astark 3/19/12 */

void check_achievement( CHAR_DATA *ch )
{
    int i;
    int current;

/*loop through the whole achievement table starting from index 0 and keep going
  until the type is NULL (this will always be the last entry)*/

   for (i = 0; achievement_table[i].bit_vector != 0; i++)
    {	
        /* Recommendation from Vodur to make the check more efficient. No point in checking
           that in which we've already earned. Added 1-15-13 */
        if (IS_SET(ch->pcdata->achievements, achievement_table[i].bit_vector)) 
            continue;

	/*for whatever index we're on, is the type "level"? if so, we check limit vs ch-> level. We'll have other if checks for other types
          so we'll know what to check the 'limit' against*/
	current=0;//so numbers don't carry over from previous loops
	switch (achievement_table[i].type)
	{
	    case ACHV_LEVEL:
		current = ch->level;
		break;
	    case ACHV_MKILL:
		current = ch->pcdata->mob_kills;
		break;
	    case ACHV_REMORT:
		current = ch->pcdata->remorts;
		break;
	    case ACHV_QCOMP:
		current = ch->pcdata->quest_success;
		break;
	    case ACHV_WKILL:
		current = ch->pcdata->war_kills;
		break;
	    case ACHV_WWIN:
		current = ch->pcdata->armageddon_won + ch->pcdata->clan_won + ch->pcdata->class_won + ch->pcdata->race_won + ch->pcdata->religion_won + ch->pcdata->gender_won;
		break;
	    case ACHV_BEHEAD:
		current = ch->pcdata->behead_cnt;
		break;
	    case ACHV_PKILL:
		current = ch->pcdata->pkill_count;
		break;
	    case ACHV_AGE:
		current = get_age(ch);
		break;
	    case ACHV_MAXHP:
		current = ch->max_hit;
		break;
	    case ACHV_MAXMN:
		current = ch->max_mana;
		break;
	    case ACHV_MAXMV:
		current = ch->max_move;
		break;
            case ACHV_EXPLORED:
                current = ch->pcdata->explored->set;
                break;
	    default:
		bug("Invalid achievement entry. Check achievement type", 0);
		//bug message here
    	}
    	
        if ( current >= achievement_table[i].limit)
          if (!IS_SET(ch->pcdata->achievements, achievement_table[i].bit_vector))
            achievement_reward(ch, i); 
       
    }
}

void achievement_reward( CHAR_DATA *ch, int table_index)
{
   char buf[MSL];

   /* This function gives people rewards based on what table_index is given*/

    if (ch == NULL)
    {
        bug("Null Character while trying to reward for an achievement.", 0);
        return;
    }
    else
    {    
	flag_set(ch->pcdata->achievements, achievement_table[table_index].bit_vector);

        ch->pcdata->questpoints += achievement_table[table_index].quest_reward;
        ch->pcdata->bank += achievement_table[table_index].gold_reward;
        gain_exp(ch, achievement_table[table_index].exp_reward);
        ch->pcdata->achpoints += achievement_table[table_index].ach_reward;
        //send_to_char("Achievement unlocked -- TEST.\n\r",ch);
	printf_to_char(ch, "Achievement %s %d unlocked.\n\r", achievement_display[achievement_table[table_index].type], achievement_table[table_index].limit);
	send_to_char( "Your reward:\n\r",ch);
	if (achievement_table[table_index].gold_reward>0)
	  printf_to_char(ch, "%6d gold\n\r", achievement_table[table_index].gold_reward);
	if (achievement_table[table_index].quest_reward>0)
	  printf_to_char(ch, "%6d quest points\n\r", achievement_table[table_index].quest_reward);
	if (achievement_table[table_index].exp_reward>0)
	  printf_to_char(ch, "%6d experience points\n\r", achievement_table[table_index].exp_reward);
	if (achievement_table[table_index].ach_reward>0)
	  printf_to_char(ch, "%6d achievement points\n\r", achievement_table[table_index].ach_reward);
	//this later
/*	if (achivement_table[table_index].obj_reward !=0)
	{
	   blah blah blah make an obj based on vnum and give to char
	   and send a message
	}
	*/
        
    }
}



void do_count ( CHAR_DATA *ch, char *argument )
{
    int count, max;
    DESCRIPTOR_DATA *d;
	FILE *fp;

    count = 0;

    if ( IS_NPC(ch) || ch->desc == NULL )
    	return;

    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( d->connected == CON_PLAYING && can_see( ch, d->character ) )
	    count++;

    max_on = UMAX(count,max_on);

	if ( ( fp = fopen( MAX_WHO_FILE,"r" ) ) == NULL )
	{
	    log_string("Error reading from maxwho.txt");
	    return;
	}
	max = fread_number( fp );
	fclose(fp);

	if ( max_on > max )
	{
		if ( ( fp = fopen( MAX_WHO_FILE,"w" ) ) == NULL )
		{
		    log_string("Error writing to maxwho.txt");
		    return;
		}
		fprintf( fp, "%d\n", max_on );
		fclose(fp);
	}

	ptc(ch,"The largest number of active players today was %d.\n\r", max_on );
	ptc(ch,"The largest number of active players ever was %d.\n\r", max );

      /* Added a check to make the word character plural only if there is more
         than 1 visible character - Astark 1-15-13 */
        if (count == 1)
            ptc(ch,"You can see %d character.\n\rSome characters may be invisible to you.\n\r\n\r", count );
        else
            ptc(ch,"You can see %d characters.\n\rSome characters may be invisible to you.\n\r\n\r", count );
}


void do_toggle( CHAR_DATA *ch, char *argument )
{
  char arg[MIL];
  one_argument( argument, arg);
  int ti, toggle;

  if (argument[0] == '\0')
  {
    send_to_char("\n\r{cToggleable Commands{x / {cCurrent Setting{x\n\r",ch);
    send_to_char("{c-------------------{x   {c----------------{x\n\r",ch);
    for (ti=0; togg_flags[ti].name != NULL; ti++)
    {
        printf_to_char(ch, "{w%18s    %s{x\n\r",
            togg_flags[ti].name
            , IS_SET(ch->togg,togg_flags[ti].bit) ? "ON" : "OFF"
        );
    }
    return;
  }

  toggle = flag_lookup(argument, togg_flags);
  if (toggle == NO_FLAG)
  {
      printf_to_char(ch, "Unknown toggle '%s'. Type toggle without arguments for available toggles.", argument);
      return;
  }
  
  // all good, let's do it
  TOGGLE_BIT(ch->togg, toggle);
  printf_to_char(ch, "Toggled %s.\n\r", IS_SET(ch->togg,toggle) ? "ON" : "OFF");

  return;
}
/* NEW worth function by Quirky: July 6, 1998 */
void do_oldworth( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;
    output = new_buf();

    if (IS_NPC(ch))
    {
        sprintf(buf, "Gold: %d, Silver: %d.\n\r",ch->gold, ch->silver );
        add_buf(output, buf);
    }

    sprintf(buf, "Gold: %d, Silver: %d.\n\r",ch->gold, ch->silver );
    add_buf(output, buf);
    sprintf(buf, "Bank: %d gold\n\r", ch->pcdata->bank);
    add_buf(output, buf);
    sprintf(buf, "Real Experience: %d\n\r", ch->exp);
    add_buf(output, buf);
    sprintf(buf, "Field Experience: %d\n\r", ch->pcdata->field);
    add_buf(output, buf);
    if ( !IS_NPC(ch) && !IS_HERO(ch) )
    {
        sprintf( buf, "Exp to level: %d\n\r",
            (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp);
        add_buf(output, buf);
    }
    sprintf(buf, "Quest Points: %d\n\r", ch->pcdata->questpoints);
    add_buf(output, buf);
    if ( !IS_NPC(ch) && ch->pcdata->bounty > 0 )
    {
        sprintf( buf, "Bounty on you: %d gold\n\r", ch->pcdata->bounty);
        add_buf(output, buf);
    }


    page_to_char(buf_string(output),ch);
    free_buf(output);

}



void do_oldattributes(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;

    output = new_buf();
    
    sprintf( buf,"{cHit{x: %d/%d  {cMana{x: %d/%d  {cMoves{x: %d/%d\n\r",
        ch->hit,  ch->max_hit,
        ch->mana, ch->max_mana,
        ch->move, ch->max_move);    
    add_buf(output, buf);
    
    sprintf( buf,
        "{BStr{x: %3d(%3d)  {BCon{x: %3d(%3d)  {BVit{x: %3d(%3d)  {BAgi{x: %3d(%3d)  {BDex{x: %3d(%3d)\n\r",
        ch->perm_stat[STAT_STR],
        get_curr_stat(ch,STAT_STR),
        ch->perm_stat[STAT_CON],
        get_curr_stat(ch,STAT_CON),
        ch->perm_stat[STAT_VIT],
        get_curr_stat(ch,STAT_VIT),
        ch->perm_stat[STAT_AGI],
        get_curr_stat(ch,STAT_AGI),
        ch->perm_stat[STAT_DEX],
        get_curr_stat(ch,STAT_DEX) );
    add_buf(output, buf);
    
    sprintf( buf,
        "{BInt{x: %3d(%3d)  {BWis{x: %3d(%3d)  {BDis{x: %3d(%3d)  {BCha{x: %3d(%3d)  {BLuc{x: %3d(%3d)\n\r",
        ch->perm_stat[STAT_INT],
        get_curr_stat(ch,STAT_INT),
        ch->perm_stat[STAT_WIS],
        get_curr_stat(ch,STAT_WIS),
        ch->perm_stat[STAT_DIS],
        get_curr_stat(ch,STAT_DIS),
        ch->perm_stat[STAT_CHA],
        get_curr_stat(ch,STAT_CHA),
        ch->perm_stat[STAT_LUC],
        get_curr_stat(ch,STAT_LUC) );
    add_buf(output, buf);
    
    sprintf( buf,"Armor {cpierce{x: %d {cbash{x: %d {cslash{x: %d {cmagic{x: %d\n\r",
            GET_AC(ch,AC_PIERCE),
            GET_AC(ch,AC_BASH),
            GET_AC(ch,AC_SLASH),
            GET_AC(ch,AC_EXOTIC));
    add_buf(output, buf);

       
    sprintf( buf, "Hitroll: %d , Damroll: %d\n\r",
        GET_HITROLL(ch), GET_DAMROLL(ch) );
    add_buf(output, buf);
        
    sprintf( buf, "Saves: %d , Physical: %d\n\r", get_save(ch, FALSE), get_save(ch, TRUE));
    add_buf(output, buf);
        

    page_to_char(buf_string(output),ch);
    free_buf(output);

}

/* NEW score function by Quirky, July 6 1998 */
void do_oldscore( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH], custombuf[MAX_STRING_LENGTH];
    BUFFER *output;
    int trustlevel, thirst, hunger, encumber;

    output = new_buf();

    if (IS_NPC(ch))
    {
        do_oldscore( ch, "" );
        return;
    }

    if (ch->pcdata->customflag[0]!='\0')
        sprintf(custombuf, "(%s) ", ch->pcdata->customflag);
    else
        custombuf[0] = '\0';

    sprintf( buf, "Name and Title: %s%s%s%s%s%s\n\r",
        IS_SET(ch->comm,COMM_AFK) ? "[AFK] " : "",
        custombuf,
        IS_SET(ch->act, PLR_KILLER) ? "(KILLER) " : "",
        IS_SET(ch->act, PLR_THIEF) ? "(THIEF) " : "",
        ch->name, IS_NPC(ch) ? "" : ch->pcdata->title);
    add_buf(output, buf);
    sprintf( buf, "Level: {c%d{x\n\r", ch->level );
    add_buf(output, buf);
    sprintf( buf, "Sex: %s   Age: %d years (%d hours)\n\r",
        ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female",
        get_age(ch), ( ch->played + (int)(current_time - ch->logon) ) / 3600 );
    add_buf(output, buf);
    sprintf( buf, "Race: %s   Class: %s   Clan: %s%c%s\n\r",
        race_table[ch->race].name,
        IS_NPC(ch) ? "mobile" : class_table[ch->class].name,
        (clan_table[ch->clan].active && !IS_NPC(ch)) ?  clan_table[ch->clan].who_name : "none",
        clan_table[ch->clan].active ?  '-' : ' ',
        (clan_table[ch->clan].active && !IS_NPC(ch)) ?  clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].who_name : "");
    add_buf(output, buf);

    if ( !IS_IMMORTAL(ch) && get_religion(ch) != NULL )
    {
        sprintf( buf, "God: %s   Rank: %s   Faith: %d\n\r",
                 get_god_name(ch), get_ch_rank_name(ch), get_faith(ch) );
        add_buf(output, buf);
    }
    sprintf(buf, "Trains: %d\n\r", ch->train);
    add_buf(output, buf);
    sprintf(buf, "Practices: %d\n\r", ch->practice);
    add_buf(output, buf);
    sprintf(buf, "Carrying: %d/%d items.\n\r", ch->carry_number, can_carry_n(ch) );
    add_buf(output, buf);
    sprintf( buf, "Weight: %d/%d pounds.\n\r", get_carry_weight(ch) / 10, can_carry_w(ch) / 10 );
    add_buf(output, buf);
    /* display trains spent --Bobble */
    /* marker for astark , check trains */
    sprintf( buf, "Trains spent:  hp: %d mana: %d move: %d max: %d\n\r",
             ch->pcdata->trained_hit,
             ch->pcdata->trained_mana,
             ch->pcdata->trained_move,
             max_hmm_train( ch->level ) );
    add_buf(output, buf);
    sprintf( buf, "Wimpy set to %d%% hit points.\n\r", ch->wimpy );
    add_buf(output, buf);
    sprintf( buf, "Calm set to %d%% moves.\n\r", ch->calm );
    add_buf(output, buf);

    switch ( ch->position )
    {
    case POS_DEAD:
        sprintf(buf, "You are DEAD\n\r");
        add_buf(output, buf);
        break;
    case POS_MORTAL:
        sprintf(buf, "You are mortally wounded\n\r");
        add_buf(output, buf);
        break;
    case POS_INCAP:
        sprintf(buf, "You are incapacitated\n\r");
        add_buf(output, buf);
        break;
    case POS_STUNNED:
        sprintf(buf, "You are stunned\n\r");
        add_buf(output, buf);
        break;
    case POS_SLEEPING:
        sprintf(buf, "You are sleeping\n\r");
        add_buf(output, buf);
        break;
    case POS_RESTING:
        sprintf(buf, "You are resting\n\r");
        add_buf(output, buf);
        break;
    case POS_SITTING:
        sprintf(buf, "You are sitting\n\r");
        add_buf(output, buf);
        break;
    case POS_STANDING:
        sprintf(buf, "You are standing\n\r");
        add_buf(output, buf);
        break;
    case POS_FIGHTING:
        sprintf(buf, "You are fighting\n\r");
        add_buf(output, buf);
        break;
    }

    if ( ch->pcdata->condition[COND_DRUNK] > 10 )
        sprintf(buf, "You are drunk\n\r");
        add_buf(output, buf);

    thirst = ch->pcdata->condition[COND_THIRST];
    hunger = ch->pcdata->condition[COND_HUNGER];

    if ( (thirst >= 0 && thirst<20) && (hunger >= 0 && hunger < 20))
    {
        sprintf(buf, "You are hungry and thirsty\n\r");
        add_buf(output, buf);
    }
    else if ( thirst >= 0 && thirst<20)
    {
        sprintf(buf, "You are thirsty\n\r");
        add_buf(output, buf);
    }
    else if ( hunger >= 0 && hunger < 20)
    {
        sprintf(buf, "You are hungry\n\r");
        add_buf(output, buf);
    }

    /* encumberance */
    if ( (encumber = get_encumberance(ch)) > 0 )
    {
        if ( encumber <= 25 )
            sprintf(buf, "You are slightly encumbered.\n\r");
        else if ( encumber <= 50 )
            sprintf(buf, "You are seriously encumbered.\n\r");
        else if ( encumber <= 75 )
            sprintf(buf, "You are heavily encumbered.\n\r");
        else
            sprintf(buf, "You can hardly move under the weight you carry!\n\r");
            
        add_buf(output,buf);
    }

    if (ch->stance != 0)
    {
        sprintf(buf, "Stance: %s\n\r", stances[ch->stance].name );
        add_buf(output,buf);
    }

    /* morphing info */
    if ( ch->race == race_doppelganger )
    {
        if ( ch->pcdata->morph_race > 0 )
        {
            sprintf(buf, "You have morphed into a %s for %d remaining hours.\n\r",
                    race_table[ch->pcdata->morph_race].name,
                    ch->pcdata->morph_time );
            add_buf(output,buf);
        }
        else
        {
            sprintf(buf, "You are in basic form.\n\r");
            add_buf(output,buf);
        }
    }
    if ( ch->race == race_naga )
    {
        if ( ch->pcdata->morph_race == 0 )
        {
            sprintf(buf, "You are currently in serpent form.\n\r");
            add_buf(output,buf);
        }
        else
        {
            sprintf(buf, "You are currently in humanoid form.\n\r");
            add_buf(output,buf);
        }
    }

    
    int align;
    align = ch->alignment;
    if( align > 900 )       sprintf( buf, "Alignment: %-5d (angelic)\n\r", align );
    else if( align >  700 ) sprintf( buf, "Alignment: %-5d (saintly)\n\r", align );
    else if( align >  350 ) sprintf( buf, "Alignment: %-5d (good)\n\r", align );
    else if( align >  100 ) sprintf( buf, "Alignment: %-5d (kind)\n\r", align );
    else if( align > -100 ) sprintf( buf, "Alignment: %-5d (neutral)\n\r", align );
    else if( align > -350 ) sprintf( buf, "Alignment: %-5d (mean)\n\r", align );
    else if( align > -700 ) sprintf( buf, "Alignment: %-5d (evil)\n\r", align );
    else if( align > -900 ) sprintf( buf, "Alignment: %-5d (demonic)\n\r", align );
    else                    sprintf( buf, "Alignment: %-5d (satanic)\n\r", align );

    add_buf(output, buf);
    
    if (IS_SET(ch->act, PLR_PERM_PKILL))
    {
        sprintf(buf, "You are a %s player killer, with %d kills.\n\r",
            IS_SET(ch->act, PLR_HARDCORE) ? "hardcore" : "permanent",
            ch->pcdata->pkill_count);
        add_buf(output, buf);
    }

    /* Warfare grade and number of kills */
    sprintf( buf, "{xWarfare:  Grade <%s> (%d points from %d warkills)\n\r",
        pkgrade_table[get_pkgrade_level(ch->pcdata->warpoints)].grade,
	    ch->pcdata->warpoints, ch->pcdata->war_kills );
    add_buf(output, buf);
	   

    if ( ch->pcdata->spouse )
       sprintf(buf, "You are married to %s.\n\r", ch->pcdata->spouse);
    else
       sprintf(buf, "You are not married.\n\r");
           
    add_buf(output, buf);       
           

    if ( ch->pcdata->customflag[0] != '\0' )
    {
        sprintf( buf, "You are marked with a (%s) flag for %d hours.\n\r",
           ch->pcdata->customflag, ch->pcdata->customduration );
        add_buf(output, buf);
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);

    if ( IS_SET(ch->comm,COMM_SHOW_WORTH) )
       do_oldworth(ch, "");
    else
       send_to_char("Type {Wshow worth{x to include exp and money info in score.\n\r", ch );

    if ( IS_SET(ch->comm,COMM_SHOW_ATTRIB))
       do_oldattributes(ch, "");
    else
       send_to_char("Type {Wshow att{x to include hp,moves,mana,stats,armor,etc. in score\n\r", ch );

    if ( IS_SET(ch->comm,COMM_SHOW_AFFECTS))
       do_affects(ch, "");

    if (!IS_NPC(ch) && ch->penalty)
       show_penalties_by_player(ch, ch->name, TIME_PLAYED(ch), 2);

}
