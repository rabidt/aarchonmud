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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <lua.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "religion.h"
#include "lua_arclib.h"
#include "lua_scripting.h"
#include "tattoo.h"
#include "songs.h"

bool commen_wear_pos( tflag wear_flag1, tflag wear_flag2 );
void show_affects(CHAR_DATA *ch, CHAR_DATA *to_ch, bool show_long, bool show_all);
void who_show_char( CHAR_DATA *ch, CHAR_DATA *wch, BUFFER *output );
void smash_beep_n_blink( char *str );
void smash_reserved_colcodes( char *str );
void print_affect( CHAR_DATA *ch, AFFECT_DATA *paf, FILE *fp );
void achievement_reward( CHAR_DATA *ch, int table_index);
void print_ach_rewards(CHAR_DATA *ch);
void print_stat_bars( CHAR_DATA *ch, BUFFER *output );

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

const char *  const   where_name  [] =
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
    
    // show item level and type
    if ( IS_SET(ch->comm, COMM_ITEMLEVEL) )
    {
        char lvlBuf[MSL];
        if ( obj->carried_by && obj->carried_by != ch )
                lvlBuf[0] = '\0';
        else if ( obj->wear_loc != WEAR_NONE )
            sprintf(lvlBuf, "(lvl %d) ", obj->level);
        else switch ( obj->wear_type )
        {
            case ITEM_NO_CARRY:
                lvlBuf[0] = '\0';
                break;
            case ITEM_CARRY:
                sprintf(lvlBuf, "(lvl %d %s) ", obj->level, flag_bit_name(type_flags, obj->item_type));
                break;
            case ITEM_WIELD:
                // weapon
                sprintf(lvlBuf, "(lvl %d %s%s) ",
                    obj->level,
                    IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) ? "2h-" : "",
                    flag_bit_name(weapon_class, obj->value[0]) );
                break;
            case ITEM_HOLD:
                if ( obj->item_type != ITEM_ARMOR
                    && obj->item_type != ITEM_JEWELRY
                    && obj->item_type != ITEM_TREASURE )
                {
                    // same as ITEM_CARRY
                    sprintf(lvlBuf, "(lvl %d %s) ", obj->level, flag_bit_name(type_flags, obj->item_type));
                    break;
                }
            default:
                sprintf(lvlBuf, "(lvl %d %s) ", obj->level, wear_bit_name(obj->wear_type));
                break;
        }
        strcat(buf, lvlBuf);
    }
    
    if ( IS_OBJ_STAT(obj, ITEM_INVIS)     )   strcat( buf, "(Invis) "     );
    if ( IS_AFFECTED(ch, AFF_DETECT_EVIL)
        && IS_OBJ_STAT(obj, ITEM_EVIL)   )   strcat( buf, "(Red Aura) "  );
    if (IS_AFFECTED(ch, AFF_DETECT_GOOD)
        &&  IS_OBJ_STAT(obj,ITEM_BLESS))          strcat(buf,"(Blue Aura) " );
    if ( IS_OBJ_STAT(obj, ITEM_MAGIC) )
    {
        // show magic item flag based on number of enchantments
        int ops = get_obj_ops_by_duration(obj, AFFDUR_DISENCHANTABLE);
        if ( ops > 12 )
            strcat(buf, "{M(Mythical){x ");
        else if ( ops > 8 )
            strcat(buf, "{G(Epic){x ");
        else if ( ops > 4 )
            strcat(buf, "{Y(Brilliant){x ");
        else if ( ops > 0 )
            strcat(buf, "{B(Magical){x ");
        else if ( IS_AFFECTED(ch, AFF_DETECT_MAGIC) )
            strcat(buf, "(Magical) ");
    }
    if ( IS_OBJ_STAT(obj, ITEM_GLOW)      )   strcat( buf, "(Glowing) "   );
    if ( IS_OBJ_STAT(obj, ITEM_DARK)      )   strcat( buf, "(Dark) "   );
    if ( IS_OBJ_STAT(obj, ITEM_HUM)       )   strcat( buf, "(Humming) "   );
    if ( IS_OBJ_STAT(obj, ITEM_HEAVY_ARMOR))  strcat( buf, "(Heavy) "     );
    if ( obj->timer == -1 && obj->item_type != ITEM_EXPLOSIVE )
        strcat( buf, "(Preserved) " );
    if ( show_empty_flag(obj) )
        strcat( buf, "(Empty) " );
    
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
    const char **prgpstrShow;
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
    prgpstrShow = alloc_mem( (size_t)count * sizeof(char *) );
    prgnShow    = alloc_mem( (size_t)count * sizeof(int)    );
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
    free_mem( prgpstrShow, (size_t)count * sizeof(char *) );
    free_mem( prgnShow,    (size_t)count * sizeof(int)    );
    
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
        if ( IS_AFFECTED(victim, AFF_PETRIFIED)   ) strcat( buf, "(Petrified) "  );
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
        strcat( buf, "\n\r");
	    send_to_char( buf, ch );
	    return;
	}
    }

    if ( victim->position == victim->start_pos && victim->long_descr[0] != '\0' )
    {
        strcat( buf, victim->long_descr );
        strcat( buf, "\n\r");
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
    static const char * const appear_str[] =
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

    sprintf( buf, "a %s %s %s %s%s%s%s%s",
	     size == SIZE_MEDIUM ? "medium-sized" : size_table[size].name,
	     appear_str[appearance],
	     sex == 0 ? "sexless" : sex_table[sex].name,
         IS_AFFECTED(ch, AFF_FLYING) ? "flying " : "",
	     race == 0 ? "being" : (check_shewolf ? "shewolf" : race_type->name),
             ch->stance == 0 ? "" : " in the ",
             ch->stance == 0 ? "" : stances[ch->stance].name,
             ch->stance == 0 ? "" : " stance" );
    
    return buf;
}

bool show_equipped_to_char( CHAR_DATA *victim, CHAR_DATA *ch, int slot, bool show_all )
{
    OBJ_DATA *obj = get_eq_char(victim, slot);
    int tattoo = get_tattoo_ch(victim, slot);
    const char *where_desc = where_name[slot];
    
    // show if wheapon is wielded 2-handed
    if ( slot == WEAR_WIELD && obj != NULL && is_wielding_twohanded(victim, obj) )
        where_desc = "<wielded 2h>        ";
    
    if ( obj != NULL && can_see_obj(ch, obj) )
    {
        ptc(ch, "%s%s", where_desc, format_obj_to_char(obj, ch, TRUE));
        if ( IS_OBJ_STAT(obj, ITEM_TRANSLUCENT_EX) && tattoo != TATTOO_NONE )
            ptc(ch, " above %s", tattoo_desc(tattoo));
        ptc(ch, "\n\r");
        return TRUE;
    }
    else if ( tattoo != TATTOO_NONE )
    {
        ptc(ch, "%s%s\n\r", where_desc, tattoo_desc(tattoo));
        return TRUE;
    }
    else if ( obj != NULL && victim == ch )
    {
        ptc(ch, "%s%s\n\r", where_desc, "something");
        return TRUE;
    }
    else if ( show_all )
    {
        ptc(ch, "%s%s\n\r", where_desc, "nothing");
    }
    return FALSE;
}

void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch, bool glance )
{
    char buf[MAX_STRING_LENGTH];
    int i, percent;
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
        act( "\n\r$N is using:", ch, NULL, victim, TO_CHAR );
        for ( i = 0; i < MAX_WEAR; i++ )
            show_equipped_to_char(victim, ch, i, FALSE);
    }
    
    /* show spell affects */
    if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
    {
	send_to_char("\n\rYou detect the following spells:\n\r", ch);
	show_affects(victim, ch, TRUE, FALSE);
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

DEF_DO_FUN(do_peek)
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

    WAIT_STATE(ch, skill_table[gsn_peek].beats);
    
    chance += (ch->level - victim->level) / 2;
    if ( number_percent() > chance )
    {
	send_to_char( "You failed.\n\r", ch );
    check_improve(ch,gsn_peek,FALSE,3);
	return;
    }

    check_improve(ch,gsn_peek,TRUE,3);
    /* money */
    sprintf( buf, "$N's wallet contains %ld gold and %ld silver coins.", victim->gold, victim->silver );
    act( buf, ch, NULL, victim, TO_CHAR );
    /* inventory */
    act_see( "$n peeks at your inventory.", ch, NULL, victim, TO_VICT );
    act( "You peek at $N's inventory:", ch, NULL, victim, TO_CHAR );
    show_list_to_char( victim->carrying, ch, TRUE, TRUE );
}

/* changes your scroll */
DEF_DO_FUN(do_scroll)
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
DEF_DO_FUN(do_social)
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
DEF_DO_FUN(do_socials)
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

DEF_DO_FUN(do_socials)
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

DEF_DO_FUN(do_motd)
{
    do_help(ch,"motd");
}

DEF_DO_FUN(do_imotd)
{  
    do_help(ch,"imotd");
}

DEF_DO_FUN(do_rules)
{
    do_help(ch,"rules");
}

DEF_DO_FUN(do_story)
{
    do_help(ch,"story");
}

DEF_DO_FUN(do_dirs)
{
    if (IS_NPC(ch))
        return;

    /* no args, just do "help dirs" */
    if (argument[0] == '\0')
    {
        do_help(ch, "dirs");
        return;
    }

    /* arg means search for match */
    BUFFER *output = new_buf();
    HELP_DATA *help_dirs = find_help_data(ch, "dirs", output);

    if (!help_dirs)
    {
        add_buf(output, "Couldn't find result for 'help dirs'; please contact an immortal.");
    }
    else
    {
        char *text = strdup(help_dirs->text); 
        const char *line = strtok(text, "\r");
        bool found = FALSE;

        while (line != NULL)
        {
            bool match = !str_infix(argument, line);

            if (match)
            {
                found = TRUE;
                add_buf(output, line);
                add_buf(output, "\r");
            }

            line = strtok(NULL, "\r");
        }

        free(text);

        if (!found)
        {
            addf_buf(output, "No dirs found for '%s'\n\r", argument);
        }
    }

    page_to_char(buf_string(output), ch);
    free_buf(output);
}

/*
DEF_DO_FUN(do_wizlist)
{
do_help(ch,"wizlist");
}
*/

/* RT this following section holds all the auto commands from ROM, as well as
replacements for config */

DEF_DO_FUN(do_autolist)
{
    /* lists most player flags */
    if (IS_NPC(ch))
        return;
    
    send_to_char("{w  Command     Status     Description{x\n\r",ch);
    send_to_char("{w-------------------------------------------------------------------------------{x\n\r",ch);
    
    if (IS_SET(ch->act,PLR_AUTOASSIST))
        send_to_char("autoassist     ON     You will automatically assist your group in combat.\n\r",ch);
    else
        send_to_char("autoassist     OFF    You won't automatically assist your group in combat.\n\r",ch); 
    
    if (IS_SET(ch->act,PLR_AUTOEXIT))
        send_to_char("autoexit       ON     You will automatically see visible exits in rooms.\n\r",ch);
    else
        send_to_char("autoexit       OFF    You won't automatically see visible exits in rooms.\n\r",ch);
    
    if (IS_SET(ch->act,PLR_AUTOGOLD))
        send_to_char("autogold       ON     You will automatically loot gold from corpses.\n\r",ch);
    else
        send_to_char("autogold       OFF    You won't automatically loot gold from corpses.\n\r",ch);
    
    if (IS_SET(ch->act,PLR_AUTOLOOT))
        send_to_char("autoloot       ON     You will automatically loot equipment from corpses.\n\r",ch);
    else
        send_to_char("autoloot       OFF    You won't automatically loot equipment from corpses.\n\r",ch);

    if (IS_SET(ch->act,PLR_AUTORESCUE))
        send_to_char("autorescue     ON     You will automatically attempt to rescue group members.\n\r",ch);
    else
        send_to_char("autorescue     OFF    You won't automatically attempt to rescue group members.\n\r",ch);
    
    if (IS_SET(ch->act,PLR_AUTOSAC))
        send_to_char("autosac        ON     You will automatically sacrifice corpses when killing.\n\r",ch);
    else
        send_to_char("autosac        OFF    You won't automatically sacrifice corpses when killing.\n\r",ch);
    
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
        send_to_char("autosplit      ON     You will automatically share looted gold with your group.\n\r",ch);
    else
        send_to_char("autosplit      OFF    You won't automatically share looted gold with your group.\n\r",ch);


    send_to_char("\n\r",ch);


    if (IS_SET(ch->act,PLR_NOACCEPT))
        send_to_char("noaccept       ON     Players cannot give items to you.\n\r",ch);
    else
        send_to_char("noaccept       OFF    Players can give items to you.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOCANCEL))
        send_to_char("nocancel       ON     Players cannot cancel your spells.\n\r",ch);
    else
        send_to_char("nocancel       OFF    Players can cancel your spells.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOEXP))
        send_to_char("noexp          ON     You won't gain experience points.\n\r",ch);
    else
        send_to_char("noexp          OFF    You will gain experience points.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOFOLLOW))
        send_to_char("nofollow       ON     Players cannot follow you.\n\r",ch);
    else
        send_to_char("nofollow       OFF    Players can follow you.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOHELP))
        send_to_char("nohelp         ON     You won't receive help messages.\n\r",ch);
    else
        send_to_char("nohelp         OFF    You will receive help messages.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOLOCATE))
        send_to_char("nolocate       ON     Players cannot locate you with hunt / farsight.\n\r",ch);
    else
        send_to_char("nolocate       OFF    Players can locate you with hunt / farsight.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOLOOT))
        send_to_char("noloot         ON     Players cannot loot items from corpses you own.\n\r",ch);
    else 
        send_to_char("noloot         OFF    Players can loot items from corpses you own.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOSUMMON))
        send_to_char("nosummon       ON     Players cannot gate to or summon you.\n\r",ch);
    else
        send_to_char("nosummon       OFF    Players can gate to or summon you.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOSURR))
        send_to_char("nosurrender    ON     Players cannot surrender to you.\n\r",ch);
    else
        send_to_char("nosurrender    OFF    Players can surrender to you.\n\r",ch);


    send_to_char("\n\r",ch);   

   
    if (IS_SET(ch->comm,COMM_COMBINE))
        send_to_char("combine        ON     Your inventory will display a compacted list of items.\n\r",ch);
    else
        send_to_char("combine        OFF    Your inventory will display a full list of items.\n\r",ch);

    if (IS_SET(ch->comm,COMM_COMPACT))
        send_to_char("compact mode   ON     The blank line above your prompt will be displayed.\n\r",ch);
    else
        send_to_char("compact mode   OFF    The blank line above your prompt won't be displayed.\n\r",ch);
    
    if ( IS_SET(ch->comm, COMM_ITEMLEVEL) )
        send_to_char("itemlevel      ON     Items level and wear location will be displayed.\n\r", ch);
    else
        send_to_char("itemlevel      OFF    Items level and wear location will not be displayed.\n\r", ch);

    if (IS_SET(ch->comm,COMM_PROMPT))
        send_to_char("prompt         ON     Your prompt will be displayed.\n\r",ch);
    else
        send_to_char("prompt         OFF    Your prompt will not be displayed.\n\r",ch);

    send_to_char("\n\r",ch);
    send_to_char("Use the 'show' command for additional configuration options.\n\r",ch);   
   
}

// mama function for do_autoXYZ functions
static void auto_toggle(CHAR_DATA *ch, const char *argument, short flag, const char *on_msg, const char *off_msg)
{
    char arg[MIL];
    
    if ( IS_NPC(ch) )
        return;
    
    bool target = !IS_SET(ch->act, flag);
    argument = one_argument(argument, arg);
    
    if ( arg[0] != '\0' )
    {
        if ( !strcmp(arg, "on") )
            target = true;
        else if ( !strcmp(arg, "off") )
            target = false;
        else
        {
            send_to_char("Valid options are ON or OFF.\n\r", ch);
            return;
        }
    }
    
    if ( target )
    {
        ptc(ch, "%s.\n\r", on_msg);
        SET_BIT(ch->act, flag);
    }
    else
    {
        ptc(ch, "%s.\n\r", off_msg);
        REMOVE_BIT(ch->act, flag);
    }
}

DEF_DO_FUN(do_autoassist)
{
    auto_toggle(ch, argument, PLR_AUTOASSIST, "You will now assist when needed", "Autoassist removed");
}

DEF_DO_FUN(do_autoexit)
{
    auto_toggle(ch, argument, PLR_AUTOEXIT, "Exits will now be displayed", "Exits will no longer be displayed");
}

DEF_DO_FUN(do_autogold)
{
    auto_toggle(ch, argument, PLR_AUTOGOLD, "Automatic gold looting set", "Autogold removed");
}

DEF_DO_FUN(do_autoloot)
{
    auto_toggle(ch, argument, PLR_AUTOLOOT, "Automatic corpse looting set", "Autolooting removed");
}

DEF_DO_FUN(do_autosac)
{
    auto_toggle(ch, argument, PLR_AUTOSAC, "Automatic corpse sacrificing set", "Autosacrificing removed");
}

DEF_DO_FUN(do_autosplit)
{
    auto_toggle(ch, argument, PLR_AUTOSPLIT, "Automatic gold splitting set", "Autosplitting removed");
}

DEF_DO_FUN(do_autorescue)
{
    auto_toggle(ch, argument, PLR_AUTORESCUE, "You will now protect your friends", "Autorescue removed");
}

DEF_DO_FUN(do_noloot)
{
    auto_toggle(ch, argument, PLR_NOLOOT,
        "Items owned by you (i.e. your corpse) now cannot be looted",
        "Items owned by you (i.e. your corpse) may now be picked up by anyone");
}

DEF_DO_FUN(do_nofollow)
{
    auto_toggle(ch, argument, PLR_NOFOLLOW, "You no longer accept followers", "You now accept followers");
    if ( PLR_ACT(ch, PLR_NOFOLLOW) )
        die_follower( ch, false );
}

DEF_DO_FUN(do_nosummon)
{
    auto_toggle(ch, argument, PLR_NOSUMMON, "You are now immune to summoning", "You are no longer immune to summon");
}

DEF_DO_FUN(do_nocancel)
{
    auto_toggle(ch, argument, PLR_NOCANCEL, "You are now immune to cancellation", "You are no longer immune to cancellation");
}

DEF_DO_FUN(do_nolocate)
{
    auto_toggle(ch, argument, PLR_NOLOCATE, "You no longer wish to be located", "You wish to be located again");
}

DEF_DO_FUN(do_noaccept)
{
    auto_toggle(ch, argument, PLR_NOACCEPT, "You no longer accept items from other players", "You now accept items from other players");
}

DEF_DO_FUN(do_nosurrender)
{
    auto_toggle(ch, argument, PLR_NOSURR, "You no longer accept surrenders from other players", "You now accept surrenders from other players");
}

DEF_DO_FUN(do_noexp)
{
    auto_toggle(ch, argument, PLR_NOEXP, "You will no longer be able to gain experience points", "You can now gain experience points");
}

DEF_DO_FUN(do_nohelp)
{
    auto_toggle(ch, argument, PLR_NOHELP, "You will no longer see help messages", "You will now see help messages");
}

DEF_DO_FUN(do_brief)
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

DEF_DO_FUN(do_compact)
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
DEF_DO_FUN(do_show)
{
    char arg[MAX_STRING_LENGTH];
    
    one_argument(argument, arg);
    
    if ( arg[0] == '\0' )
    {
        send_to_char("{w  Command     Status     Description{x\n\r",ch);
        send_to_char("{w-------------------------------------------------------------------------------{x\n\r",ch);

        if (IS_SET(ch->comm,COMM_SHOW_AFFECTS))
            send_to_char("affects        ON     Affects will be shown in score.\n\r", ch);
        else
            send_to_char("affects        OFF    Affects won't be shown in score.\n\r", ch);

        if (IS_SET(ch->comm,COMM_SHOW_ATTRIB))
            send_to_char("attributes     ON     Attributes will be shown in score.\n\r", ch);
        else
            send_to_char("attributes     OFF    Attributes won't be shown in score.\n\r", ch);

        if ( IS_SET(ch->comm, COMM_SHOW_PERCENT) )
            send_to_char("percentages    ON     Percentages will be shown in score.\n\r", ch);
        else
            send_to_char("percentages    OFF    Percentages won't be shown in score.\n\r", ch);

        if ( IS_SET(ch->comm, COMM_SHOW_STATBARS) )
            send_to_char("statbars       ON     Statbars will be shown for attributes in score.\n\r", ch);
        else
            send_to_char("statbars       OFF    Statbars won't be shown for attributes in score.\n\r", ch);

        if (IS_SET(ch->comm,COMM_SHOW_WORTH))
            send_to_char("worth          ON     Worth will be shown in score.\n\r", ch);
        else
            send_to_char("worth          OFF    Worth won't be shown in score.\n\r", ch);

        send_to_char("\n\r",ch);
        send_to_char("Syntax: show <affects|attributes|percentages|statbars|worth>\n\r",ch);

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
            send_to_char("Attributes will no longer be shown in score.\n\r",ch);
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
            send_to_char("Percentages will no longer be shown in score.\n\r",ch);
            REMOVE_BIT(ch->comm, COMM_SHOW_PERCENT);
        }
        else
        {
            send_to_char("Percentages will now be shown in score.\n\r",ch);
            SET_BIT(ch->comm, COMM_SHOW_PERCENT);
        }
        return;
    }
    else if ( !str_cmp(arg, "stat") || !str_cmp(arg, "statbars") )
    {
        if ( IS_SET(ch->comm, COMM_SHOW_STATBARS) )
        {
            send_to_char("Statbars will no longer be shown for attributes in score.\n\r",ch);
            REMOVE_BIT(ch->comm, COMM_SHOW_STATBARS);
        }
        else
        {
            send_to_char("Statbars will now be shown for attributes in score.\n\r",ch);
            SET_BIT(ch->comm, COMM_SHOW_STATBARS);
        }
        return;
    }

    else
    {
        send_to_char( "Show attributes, affects, percentages, statbars, or worth in your score?\n\r", ch);
        send_to_char( "Syntax:  show <att|aff|per|stat|worth>\n\r", ch );
        return;
    }
}

#define MAX_PROMPT_LENGTH 110
DEF_DO_FUN(do_prompt)
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
        strcpy( buf, argument );
        if ( strlen_color(buf) > MAX_PROMPT_LENGTH )
        {
            for( temp = buf; *temp != '\0'; temp++ )
            {
                chars++;
                if( *temp == '{' )
                    noncol--;
                else noncol++;
                if( noncol > MAX_PROMPT_LENGTH )  break;
            }
            buf[chars] = '\0';
        }
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

DEF_DO_FUN(do_combine)
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

DEF_DO_FUN(do_itemlevel)
{
    if ( IS_SET(ch->comm, COMM_ITEMLEVEL) )
    {
        send_to_char("Item levels will no longer be displayed.\n\r", ch);
        REMOVE_BIT(ch->comm, COMM_ITEMLEVEL);
    }
    else
    {
        send_to_char("Item levels will now be displayed.\n\r", ch);
        SET_BIT(ch->comm, COMM_ITEMLEVEL);
    }
}

/* added due to popular demand --Bobble */
DEF_DO_FUN(do_glance)
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
    const char *pdesc;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj( ch, obj ) )
        {  /* player can see object */
            pdesc = get_extra_descr( arg3, obj->extra_descr );
            if ( pdesc != NULL )
            {
                if (++count == number)
                {
                    if ( op_act_trigger(obj, ch, NULL, arg3, OTRIG_LOOK) )
                        send_to_char( pdesc, ch );
                    return obj;
                }
                else continue;
            }

            pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
            if ( pdesc != NULL )
            {
                if (++count == number)
                {   
                    if ( op_act_trigger(obj, ch, NULL, arg3, OTRIG_LOOK) )
                        send_to_char( pdesc, ch );
                    return obj;
                }
                else continue;
            }

            if ( is_name( arg3, obj->name ) && ++count == number )
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

void show_content( CHAR_DATA *ch, OBJ_DATA *obj )
{
    switch ( obj->item_type )
    {
    default:
        send_to_char( "That is not a container.\n\r", ch );
        break;
        
    case ITEM_DRINK_CON:
        if ( obj->value[1] <= 0 )
            send_to_char( "It is empty.\n\r", ch );
        else
            ptc( ch, "It's filled with %d out of %d units of %s liquid.\n\r",
                obj->value[1], obj->value[0], liq_table[obj->value[2]].liq_color );
        break;
        
    case ITEM_CONTAINER:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
        if ( I_IS_SET(obj->value[1], CONT_CLOSED) )
            send_to_char( "It is closed.\n\r", ch );
        else
        {
            act( "$p holds:", ch, obj, NULL, TO_CHAR );
            show_list_to_char( obj->contains, ch, TRUE, TRUE );
            if ( obj->item_type == ITEM_CONTAINER )
            { 
                int num_items = get_obj_number(obj);
                printf_to_char(ch, "\n\r%d %s.\n\r", num_items, num_items == 1 ? "item" : "items");
            }
        }
        break;
    }
}

DEF_DO_FUN(do_look)
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    const char *pdesc;
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
        }
        
        send_to_char( "{x\n\r", ch );
        
        if ( arg1[0] == '\0'
            || ( !IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF) ) )
        {
            if (rp_look_trigger( ch ) )
            {
                send_to_char( "  ",ch);
                send_to_char( ch->in_room->description, ch );
            }
        }
        
        if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT) )
        {
            send_to_char("\n\r",ch);
            do_exits( ch, "auto" );
        }
        
        show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE );
        show_char_to_char( ch->in_room->people,   ch );
        
        // stalk "all" is special usage for ambush, shouldn't trigger hunt
        if ( !IS_NPC(ch) && ch->hunting && strcmp(ch->hunting, "all") )
        {
            // we may be lagged, so any hunting time is additive; stalk is handled in do_hunt
            int old_wait = ch->wait;
            ch->wait = 0;
            do_hunt(ch, ch->hunting);
            ignore_invisible = FALSE;
            ch->wait += old_wait;
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
        
        show_content(ch, obj);
        return;
    }
    
    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
        show_char_to_char_1( victim, ch, FALSE );
        return;
    }
    
    if ( look_obj(ch, arg1) != NULL )
	return;
    
    pdesc = get_extra_descr(arg3,ch->in_room->extra_descr);
    if (pdesc != NULL)
    {
        if (++count == number)
        {
            if ( rp_look_ed_trigger( ch, arg3 ) )
                send_to_char( pdesc, ch );
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
DEF_DO_FUN(do_read)
{
    do_look(ch,argument);
}

DEF_DO_FUN(do_examine)
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
        sprintf(buf, "It has a level requirement of %d.\n\r", obj->level);
        send_to_char( buf, ch );
	break;
    case ITEM_ARROWS:
        ptc(ch, "There are %d arrows in this pack.\n\r", obj->value[0]);
        ptc(ch, "It has a level requirement of %d.\n\r", obj->level);
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
            ptc(ch, "It has a level requirement of %d.\n\r", obj->level);
        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            show_content(ch, obj);
	    break;

	case ITEM_WEAPON:
        strcpy(buf, "It is ");
        switch (obj->value[0])
        {
        case(WEAPON_EXOTIC) : strcat(buf, "a weapon of some exotic type, ");  break;
        case(WEAPON_SWORD)  : strcat(buf, "a sword, ");  break;  
        case(WEAPON_DAGGER) : strcat(buf, "a dagger, "); break;
        case(WEAPON_SPEAR)  : strcat(buf, "a spear, "); break;
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
        if ( IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
           strcat(buf, "It requires two hands to wield.\n\r");
	send_to_char(buf,ch);
        sprintf(buf, "It has a level requirement of %d.\n\r", obj->level);
	send_to_char(buf,ch);
	break;

        case ITEM_ARMOR:
            ptc(ch, "It is %s armor.\n\r", IS_OBJ_STAT(obj, ITEM_HEAVY_ARMOR) ? "heavy" : "light");
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
        sprintf(buf, "It has a level requirement of %d.\n\r", obj->level);
	send_to_char(buf,ch);
	break;
    }
}



/*
* Thanks to Zrin for auto-exit part.
*/
DEF_DO_FUN(do_exits)
{
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
            &&   (! ( IS_SET(pexit->exit_info, EX_HIDDEN)
                      && IS_SET(pexit->exit_info, EX_CLOSED ) ) 
                  || IS_IMMORTAL(ch) ) 
            &&   (!IS_SET(pexit->exit_info, EX_DORMANT) || PLR_ACT(ch, PLR_HOLYLIGHT) ) )
        {
            found = TRUE;
            if ( fAuto )
            {
                if (IS_SET(pexit->exit_info, EX_DORMANT))
                {
                    sprintf( buf, "%s <%s>", buf, dir_name[door] );
                }
                else if (IS_SET(pexit->exit_info, EX_CLOSED))
                {
                    sprintf( buf, "%s (%s%s)", buf, 
                            IS_SET(pexit->exit_info, EX_HIDDEN) ? "*" : "",
                            dir_name[door] );
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


void display_affect(CHAR_DATA *to_ch, AFFECT_DATA *paf, AFFECT_DATA *paf_last, bool show_long)
{
    bool show_spell_name = TRUE;
    if (paf_last != NULL && paf->type == paf_last->type
        && ( paf->type != gsn_custom_affect 
        || !strcmp(paf->tag, paf_last->tag) ) )
    {
        if (show_long)
        {
            printf_to_char( to_ch, "                        ");
            show_spell_name = FALSE;
        }
        else
        {
            return;
        }
    }

    if (show_spell_name)
    {
        if (paf->type==gsn_custom_affect)
        {       
            ptc(to_ch, "Special: %-15s", paf->tag);
        }    /* More information for players regarding maledictions - Astark */
        else if (is_mental(paf->type))
        {
            printf_to_char( to_ch, "Mental : {M%-15s{x", skill_table[paf->type].name );
        }
        else if (is_curse(paf->type))
        {
            printf_to_char( to_ch, "Curse  : {r%-15s{x", skill_table[paf->type].name );
        }
        else if (is_disease(paf->type))
        {
            printf_to_char( to_ch, "Disease: {D%-15s{x", skill_table[paf->type].name );
        }
        else if (paf->type == gsn_poison || paf->type == gsn_paralysis_poison )
        {
            printf_to_char( to_ch, "Poison : {G%-15s{x", skill_table[paf->type].name );
        }
        else if (is_blindness(paf->type))
        {
            printf_to_char( to_ch, "Blind  : {c%-15s{x", skill_table[paf->type].name );
        }
        else if (!IS_SPELL(paf->type))
        {
            printf_to_char( to_ch, "Ability: %-15s", skill_table[paf->type].name );
        }
        else
        {
            printf_to_char( to_ch, "Spell  : %-15s", skill_table[paf->type].name );
        }
    }

    if (show_long)
    {
        // show aura resistances
        if ( paf->modifier == 0 && paf->where != TO_AFFECTS )
            printf_to_char( to_ch, ": grants %s ", to_bit_name(paf->where, paf->bitvector) );
        else
            printf_to_char( to_ch, ": modifies %s by %d ", affect_loc_name(paf->location), paf->modifier);
        if ( paf->duration == -1 )
            printf_to_char( to_ch, "indefinitely (Lvl %d)", paf->level );
        else
            // color-code spells about to expire
            printf_to_char( to_ch, "for %s%d hours{x (Lvl %d)", paf->duration < 5 ? "{R" : paf->duration < 10 ? "{y" : "", paf->duration, paf->level );
        if ( paf->type == gsn_mirror_image || paf->type == gsn_phantasmal_image )
            printf_to_char( to_ch, " with %d %s remaining", paf->bitvector, paf->bitvector == 1 ? "image" : "images");
    }

    send_to_char( "\n\r", to_ch );
}



/* displays the affects on ch to to_ch */
void show_affects(CHAR_DATA *ch, CHAR_DATA *to_ch, bool show_long, bool show_all)
{
    AFFECT_DATA *paf, *paf_last = NULL;
    bool debuff_exists = FALSE;
    
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if (is_offensive(paf->type))
        {
            debuff_exists = TRUE;
            break;
        }
    }

    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if (is_offensive(paf->type)) continue;
        if ( !show_all && !IS_SPELL(paf->type) )
            continue;

        display_affect(to_ch, paf, paf_last, show_long);

        paf_last = paf;
    }

    if (debuff_exists)
    {
        send_to_char("\n\rDebuffs:\n\r", to_ch);
    }
    
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if (!is_offensive(paf->type)) continue;
        if ( !show_all && !IS_SPELL(paf->type) )
            continue;

        display_affect(to_ch, paf, paf_last, show_long);

        paf_last = paf;
    }
}

DEF_DO_FUN(do_affects)
{
    int leadership = get_leadership_bonus(ch, FALSE);
    
    if ( ch->affected != NULL )
    {
        send_to_char( "You are affected by the following spells:\n\r", ch );
        bool show_long = ch->level >= 5 || (ch->pcdata && (ch->pcdata->remorts || ch->pcdata->ascents));
        show_affects(ch, ch, show_long, TRUE);
    }
    else 
        send_to_char("You are not affected by any spells.\n\r",ch);

    bool separated = FALSE;
    
    if ( leadership != 0 )
    {
        const char *header = separated ? "" : "\n\r";
        const char *type = leadership >= 0 ? "bonus" : "penalty";
        if ( ch->leader != NULL && ch->leader != ch && ch->leader->in_room == ch->in_room )
        {
            printf_to_char(ch, "%sYou receive a %d%% damage %s from %s's leadership.\n\r",
                header, leadership, type, PERS(ch->leader, ch)
            );
        }
        else
        {
            printf_to_char(ch, "%sYou receive a %d%% damage %s from your swagger.\n\r",
                header, leadership, type
            );
        }
        separated = TRUE;
    }
    
    if ( ch->hit_cap_delta || ch->mana_cap_delta || ch->move_cap_delta )
    {
        ptc(ch, "%sYou are limited to {%c%d{x hp {%c%d{x mn {%c%d{x mv.\n\r",
            separated ? "" : "\n\r",
            prompt_color_code(ch->prompt, 'H'), hit_cap(ch),
            prompt_color_code(ch->prompt, 'M'), mana_cap(ch),
            prompt_color_code(ch->prompt, 'V'), move_cap(ch));
        separated = TRUE;
    }
}

DEF_DO_FUN(do_leadership)
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

const char *  const   day_name    [] =
{
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
        "the Great Gods", "the Sun"
};

const char *  const   month_name  [] =
{
    "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
        "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
        "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
        "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

DEF_DO_FUN(do_time)
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



DEF_DO_FUN(do_weather)
{
    char buf[MAX_STRING_LENGTH];
    
    static const char * const sky_look[4] =
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
DEF_DO_FUN(do_help)
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
HELP_DATA* find_help_data( CHAR_DATA *ch, const char *argument, BUFFER *output )
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
        if ( is_name(argall, pHelp->keyword) && pHelp->to_delete == FALSE )
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
		    && !(IS_PLAYING(ch->desc->connected)) 
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

static void output_help(BUFFER *output, const char *text)
{
    char char_out[2] = {'\0', '\0'};
    const char *tag_cont_start = NULL;

    char tag_out[MSL];

    bool in_tag = false;

    const char *c = text;

    while (TRUE)
    {
        if (in_tag)
        {
            if (*c == '<' && *(c+1) == '/' && *(c+2) == 'h' && *(c+3) == '>')
            {
                if (!tag_cont_start)
                {
                    bugf("tag_cont_start NULL");
                    break;
                }
                
                int cont_length = c - tag_cont_start;
                snprintf(tag_out, sizeof(tag_out),
                    "\t<send href=\"help %.*s\">%.*s\t</send>",
                    cont_length,
                    tag_cont_start,
                    cont_length,
                    tag_cont_start);
                add_buf(output, tag_out);

                in_tag = FALSE;
                c += 4;
                tag_cont_start = NULL;
            }
            else if (*c == '\0' || *c == '\n')
            {
                // Something's wrong...just send it all out
                snprintf(tag_out, sizeof(tag_out),
                    "%.*s", (int)(c - tag_cont_start), tag_cont_start);
                add_buf(output, tag_out);

                in_tag = FALSE;
                // no pointer increment
                tag_cont_start = NULL;
            }
            else
            {
                c += 1;
            }
        }
        else
        {
            if (*c == '\0')
            {
                break;
            }
            if (*c == '<' && *(c+1) == 'h' && *(c+2) == '>')
            {
                in_tag = TRUE;
                c += 3;
                tag_cont_start = c;
            }
            else
            {
                char_out[0] = *c;
                add_buf(output, char_out);
                c += 1;
            }
        }
    }
}


DEF_DO_FUN(do_help)
{
    HELP_DATA *firstHelp;
    BUFFER *output;

    /* safety net */
    if ( IS_NPC(ch) )
	return;

    if ( argument[0] == '\0' )
    {
        argument = "summary";
    }
    
    // check for 'help search ...'
    {
        char arg1[MIL];
        const char *search_arg = one_argument(argument, arg1);

        if (!strcmp(arg1, "search"))
        {
            help_search(ch, search_arg);
            return;
        }
    }
    
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
	    output_help(output,firstHelp->text+1);
	else
	    output_help(output,firstHelp->text);
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
            pkflag = is_safe ? 'r' : 'R';
        
        if ( IS_SET(wch->act, PLR_PERM_PKILL) )
            pkflag = is_safe ? 'k' : 'K';

        if ( IS_SET(wch->act, PLR_HARDCORE) )
            pkflag = is_safe ? 'h' : 'H';
        
        /* Check if ch's clan has declared war on wch's clan, and vice-versa */
        if ((cw = clanwar_lookup(wch->clan, ch->clan)))
        {
            if (cw->status == CLANWAR_WAR)
                pkflag = is_safe ? 'w' : 'W';
            else if (cw->truce_timer > 0)
                pkflag = is_safe ? 't' : 'T';
        }
        if ((cw = clanwar_lookup(ch->clan, wch->clan)))
        {
            if (cw->status == CLANWAR_WAR)
                pkflag = is_safe ? 'w' : 'W';
            else if (cw->truce_timer > 0)
                pkflag = is_safe ? 't' : 'T';
        }
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
DEF_DO_FUN(do_whois)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
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
        
        if ( !(IS_PLAYING(d->connected)) )
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
    }
    else
    {
        page_to_char(buf_string(output),ch);
    }
    free_buf(output);
}

// for sorting the who_array
int who_compare( const void* a, const void* b )
{
    const CHAR_DATA *ch1 = *((const CHAR_DATA * const *) a);
    const CHAR_DATA *ch2 = *((const CHAR_DATA * const *) b);
    
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
    size_t who_count = 0;
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
DEF_DO_FUN(do_who)
{
    BUFFER *output;
    CHAR_DATA* who_array[MAX_WHO];
    int who_count, w;
    //RELIGION_DATA *rel = NULL;
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
    bool fClassRestrict = FALSE;
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    //bool fReligion = FALSE;
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
			//else if(!str_prefix(arg,"religion"))
			//    fReligion = TRUE;
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
				//if ( (rel=get_religion_by_name(arg)) == NULL )
				{
				    send_to_char("That's not a valid race, class or clan.\n\r",ch);
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
	     || ( fClassRestrict && !rgfClass[wch->clss] )
	     || ( fRaceRestrict && !rgfRace[wch->race])
	     //|| ( fReligion && (get_religion(wch)!=get_religion(ch)) )
	     || ( fClan && !is_same_clan(ch, wch))
	     || ( fClanRestrict && !rgfClan[wch->clan])
	     //|| ( rel != NULL && get_religion(wch) != rel )
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
    
    addf_buf(output, "\n\rPlayers found: %d\n\r", nMatch );
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}
#undef MAX_WHO

void who_show_char( CHAR_DATA *ch, CHAR_DATA *wch, BUFFER *output )
{
    char buf[MAX_STRING_LENGTH];
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
            class_table[wch->clss].who_name,
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
DEF_DO_FUN(do_count)
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

DEF_DO_FUN(do_inventory)
{
    char buf[MSL];

    sprintf( buf, "You are carrying %d / %d items:\n\r", 
        ch->carry_number, can_carry_n(ch));
    send_to_char(buf,ch);

    show_list_to_char( ch->carrying, ch, TRUE, TRUE );
    return;
}

DEF_DO_FUN(do_equipment)
{
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
        if ( show_equipped_to_char(ch, ch, iWear, all_slots) )
            found = TRUE;
    }
    if ( !found && !all_slots )
        send_to_char( "Nothing.\n\r", ch );
    ptc(ch, "\n\rYour current brightness level is %d%%.\n\r", light_status(ch));
    
    if ( !IS_SET(ch->act, PLR_NOHELP) )
    {
        send_to_char("\n\r", ch);
        do_eqhelp(ch,"");
    }
    
    int max_item_level = umd_max_item_level(ch);
    if ( ch->level < max_item_level )
        ptc(ch, "\n\rYou can use items of level %d or lower.\n\r", max_item_level);

    return;
}



DEF_DO_FUN(do_compare)
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
                && obj1->wear_type == obj2->wear_type )
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
    else if ( !(obj1->wear_type == obj2->wear_type) )
    {
        msg = "You can't compare $p and $P.";
    }
    else
    {
        if (obj1->item_type == ITEM_WEAPON && obj2->item_type == ITEM_WEAPON)
        {
            value1 += 4 * average_weapon_dam( obj1 );
            value2 += 4 * average_weapon_dam( obj2 );
        }
        // translucent eq has "hidden" ops
        if ( IS_OBJ_STAT(obj1, ITEM_TRANSLUCENT_EX) )
            value1 += get_translucency_spec_penalty( obj1->level );
        if ( IS_OBJ_STAT(obj2, ITEM_TRANSLUCENT_EX) )
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

DEF_DO_FUN(do_credits)
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

DEF_DO_FUN(do_where)
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
            if ( (IS_PLAYING(d->connected))
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




DEF_DO_FUN(do_consider)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int diff;
    
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



void set_title( CHAR_DATA *ch, const char *title )
{
    char buf[MAX_STRING_LENGTH];
    
    if ( IS_NPC(ch) )
    {
        bug( "Set_title: NPC.", 0 );
        return;
    }
    
    if ( title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' && title[0] != '\'' && title[0] != ' ' )
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


DEF_DO_FUN(do_title)
{
    char buf[MAX_STRING_LENGTH];
    char *temp;
    int chars = 0, noncol = 0;
    if ( IS_NPC(ch) )
        return;
    
    if ( argument[0] == '\0' )
    {
        sprintf( buf, "the %s",
            title_table [ch->clss] [(ch->level+4-(ch->level+4)%5)/5] );
        set_title( ch, buf );
        REMOVE_BIT(ch->act, PLR_TITLE);
        return;
    }
    
    strcpy(buf, argument);
    if ( strlen_color(buf) > 45 )
    {
        for( temp = buf; *temp != '\0' ; temp++ )
        {
            chars++;
            if ( *temp == '{' )
                noncol--;
            else noncol++;
            if ( noncol > 45 ) break;
        }
        buf[chars] = '\0';
    }
    
    smash_beep_n_blink( buf );
    smash_reserved_colcodes( buf );
    smash_tilde( buf );
    strcat (buf, " {x");
    set_title( ch, buf );
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


DEF_DO_FUN(do_description)
{
    char buf[MAX_STRING_LENGTH];
    
    if ( argument[0] != '\0' )
    {
        buf[0] = '\0';
        argument = smash_tilde_cc(argument);
        
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



DEF_DO_FUN(do_report)
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
DEF_DO_FUN(do_wimpy)
{
    char arg[MAX_INPUT_LENGTH];
    int wimpy;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        printf_to_char( ch, "Your current wimpy threshold is set to %d hp (%d%%).\n\r", (ch->wimpy*ch->max_hit/100), ch->wimpy );
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
        printf_to_char( ch, "You will now flee when dropping below %d hp (%d%%).\n\r", (wimpy*ch->max_hit/100), wimpy);
    return;
}

DEF_DO_FUN(do_calm)
{
    char arg[MAX_INPUT_LENGTH];
    int calm;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        printf_to_char( ch, "Your current calm threshold is set to %d moves (%d%%).\n\r", (ch->calm*ch->max_move/100), ch->calm );
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
        printf_to_char( ch, "You will now calm down when dropping below %d moves (%d%%).\n\r", (calm*ch->max_move/100), calm );
    return;
}

DEF_DO_FUN(do_password)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char *pwdnew;
    char *p;
    
    if ( IS_NPC(ch) )
        return;
    
    argument = one_argument_keep_case( argument, arg1 );
    argument = one_argument_keep_case( argument, arg2 );
    argument = one_argument_keep_case( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char( "Syntax: password <old> <new> <new>.\n\r", ch );
        return;
    }
    
    if ( !check_password(arg1, ch->pcdata->pwd) )
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

const char* wear_location_info( int pos )
{
    static char buf[MAX_STRING_LENGTH] = "";
    switch ( pos )
    {
        case ITEM_WEAR_SHIELD:
            return "It can be used as a shield.";
        case ITEM_HOLD:
            return "It can be held.";
        case ITEM_WIELD:
            return "It can be wielded.";
        case ITEM_WEAR_FLOAT:
            return "It would float nearby.";
        case ITEM_CARRY:
        case ITEM_NO_CARRY:
            return NULL;
        default:
            sprintf( buf, "It can be worn on the %s.", wear_bit_name(pos) );
            return buf;
    }
}

// target character level for which to lore an item
// certain lore info (tattoo bonus) varies with level worn
int get_lore_level( CHAR_DATA *ch, int obj_level )
{
    if ( !IS_NPC(ch) )
        return UMAX(obj_level, ch->level);
    if ( !ch->in_room )
        return obj_level;
    // find pc in room, if multiple we don't know and just pick first
    for ( ch = ch->in_room->people; ch; ch = ch->next_in_room )
        if ( !IS_NPC(ch) )
            return UMAX(obj_level, ch->level);
    return obj_level;
}

void say_basic_obj_data( CHAR_DATA *ch, OBJ_DATA *obj )
{
    char buf[MAX_STRING_LENGTH];
    int c;
    int ac = 0;

    sprintf( buf, "%s is %s %s with properties %s.", obj->short_descr,
            aan(item_name(obj->item_type)), item_name(obj->item_type), extra_bits_name(obj->extra_flags) );
    do_say(ch, buf);

    /*
       if (obj->owner != NULL)
       {
       sprintf( buf, "It is owned by %s.", obj->owner);
       do_say(ch, buf);
       }
       */

    sprintf( buf, "It weighs %d pounds, and its level of power is %d.",
            obj->weight / 10,
            obj->level );
    do_say(ch, buf);

    if ( IS_OBJ_STAT(obj, ITEM_TRANSLUCENT_EX) )
    {
        int lore_level = get_lore_level(ch, obj->level);
        int tattoo_percent = (int)(tattoo_bonus_factor(get_obj_tattoo_level(obj->level, lore_level)) * 100);
        sprintf(buf, "It's translucent, allowing tattoos to shine through (%d%% bonus).", tattoo_percent);
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
            sprintf( buf, "It can hold %d charges of level %d",
                    obj->value[1], obj->value[0] );

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
                case(WEAPON_SPEAR)  : strcat(buf, "a spear."); break;
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
            {
                const char *wear = wear_location_info(obj->wear_type);
                if ( wear )
                {
                    do_say(ch, wear);
                    ac = predict_obj_ac(obj, obj->wear_type);
                } 

                if ( ac > 0 )
                {
                    sprintf( buf, "It provides an armor class of %d.", ac );
                    do_say(ch, buf);
                }
                break;
            }
    }
}

/* same stupid thingy AGAIN.. */
void say_basic_obj_index_data( CHAR_DATA *ch, OBJ_INDEX_DATA *obj )
{
    char buf[MAX_STRING_LENGTH];
    int c;
    int ac = 0;

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

    sprintf( buf, "It weighs %d pounds, and its level of power is %d.",
            obj->weight / 10,
            obj->level );
    do_say(ch, buf);

    if ( IS_OBJ_STAT(obj, ITEM_TRANSLUCENT_EX) )
    {
        int lore_level = get_lore_level(ch, obj->level);
        int tattoo_percent = (int)(tattoo_bonus_factor(get_obj_tattoo_level(obj->level, lore_level)) * 100);
        sprintf(buf, "It's translucent, allowing tattoos to shine through (%d%% bonus).", tattoo_percent);
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
            sprintf( buf, "It can hold %d charges of level %d",
                    obj->value[1], obj->value[0] );

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
                case(WEAPON_SPEAR)  : strcat(buf, "a spear."); break;
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
            {
                const char *wear = wear_location_info(obj->wear_type);
                if ( wear )
                {
                    do_say(ch, wear);
                    ac = predict_obj_index_ac(obj, obj->wear_type);
                }
                if ( ac > 0 )
                {
                    sprintf( buf, "It provides an armor class of %d.", ac );
                    do_say(ch, buf);
                }
                break;
            }
    }
}

DEF_DO_FUN(do_lore)
{
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    OBJ_INDEX_DATA *org_obj;
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    int chance;
    int sn;
    int lore_skill, wlore_skill, mastery;
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
    mastery = get_mastery(ch, gsn_lore);
    weapon = org_obj->item_type == ITEM_WEAPON || org_obj->item_type == ITEM_ARROWS;
    sn = (weapon && wlore_skill) ? gsn_weapons_lore : gsn_lore;

    if ( lore_skill == 0)
    {
        if ( wlore_skill == 0)
        {
            send_to_char("You aren't versed in the lore of Aarchon.\n\r",ch);
            return;
        } 
        else if ( !weapon )
        {
            send_to_char("You only know lore regarding weapons.\n\r",ch);
            return;
        }
    }
    
    if ( weapon )
    {
        chance = lore_skill + (100 - lore_skill) * wlore_skill / 100;
        mastery = UMAX(mastery, get_mastery(ch, gsn_weapons_lore));
    }
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
        WAIT_STATE(ch, skill_table[sn].beats / (1 + mastery));
    
    act( "You try to figure out what $p is.", ch, obj, NULL, TO_CHAR );
    act( "$n tries to figure out what $p is.", ch, obj, NULL, TO_ROOM );
    
    /* first check if char knows ANYTHING */
    if ( !per_chance(chance) || IS_SET(org_obj->extra_flags, ITEM_NO_LORE) )
    {
        ch->mana -= skill_table[sn].min_mana / 2;
        send_to_char( "Hmm... you can't seem to place it from any tales you've heard.\n\r", ch );
        act("$n furrows $s brow.\n\r",ch,NULL,NULL,TO_ROOM);
        return;
    }
    
    ch->mana -= skill_table[sn].min_mana;

    if (!op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_LORE) )
        return;

    /* ok, he knows something.. */
    say_basic_obj_index_data( ch, org_obj );
   
    /* now we need to check which affects char knows.. */
    all_known = TRUE;
    
    /* Immortals and NPCs can finally lore random objs */
    if ( !IS_NPC(ch) && !IS_IMMORTAL(ch) )
    {
        // half failure chance for each individual affect
        chance = (chance + 100) / 2;
        for ( paf = org_obj->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->detect_level >= 0 )
            {
                if ( per_chance(chance - paf->detect_level) )
                    show_affect(ch, paf, TRUE);
                else
                    all_known = FALSE;
            }
        }
        if ( obj->affected )
            do_say(ch, "There appear to be additional enchantments on it.");
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

    /* now let's see if someone else learned something of it --Bobble */
    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( IS_NPC(rch) || !IS_AWAKE(rch) )
            continue;
        check_improve( rch, gsn_lore, TRUE, 3 );
        if ( weapon )
            check_improve( rch, gsn_weapons_lore, TRUE, 3 );
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

DEF_DO_FUN(do_appraise)
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
    
    WAIT_STATE(ch,skill_table[gsn_appraise].beats);
    check_improve(ch,gsn_appraise,TRUE,3);
    
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
DEF_DO_FUN(do_disguise)
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
	check_improve( ch, gsn_disguise, FALSE, 3 );
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

    check_improve( ch, gsn_disguise, TRUE, 3 );
}

DEF_DO_FUN(do_song_list)
{
    int i, skill, sn;
    char buf[MSL];

    send_to_char( "You know the following songs:\n\r", ch );

    for (i = 1; songs[i].name != NULL; i++)
    {
        sn = *(songs[i].gsn);
        skill = get_skill(ch, sn);
        if ( skill == 0 )
            continue;

        sprintf( buf, "%-18s %3d%%(%3d%%) %5dmn   %-5s   %s\n\r",
            songs[i].name,
            get_skill_prac(ch, sn), skill,
            song_cost(ch, i),
            songs[i].solo ? "solo" : "group",
            songs[i].instrumental ? "instrumental" : "");
        send_to_char( buf, ch );
    }
}


DEF_DO_FUN(do_stance_list)
{
    int i, skill, sn;
    char buf[MSL];

    send_to_char( "You know the following stances:\n\r", ch );

    for (i = 1; stances[i].name != NULL; i++)
    {
        sn = *(stances[i].gsn);
        skill = get_skill(ch, sn);
        if ( skill == 0 )
            continue;

        sprintf( buf, "%-18s %3d%%(%3d%%) %5dmv     %s %s   %s\n\r",
                stances[i].name,
                get_skill_prac(ch, sn), skill,
                stance_cost(ch, i),
                stances[i].weapon ? "w" : " ",
                stances[i].martial ? "m" : " ",
                flag_bit_name(damage_type, stances[i].type));
        send_to_char( buf, ch );
    }
}

/* gather information about current room */
DEF_DO_FUN(do_survey)
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
DEF_DO_FUN(do_score)
{
    char buf[MSL];
    char flagsbuf[MSL/4];
    char custombuf[MSL];
    char alignbuf[MSL/2];
    char positionbuf[MSL/4];
    char remortbuf[MSL/4]; 
    char temp[MSL];
    char encumberbuf[MSL];
    BUFFER *output;
    int align;
    int encumber;
    int hunger;
    int thirst;
    int drunk;
    int LENGTH = 76;
    int hp_cap, mana_cap, move_cap;
    bool hungry = FALSE;
    bool thirsty = FALSE;
    bool drunken = FALSE;

    if (IS_NPC(ch))
    {
        send_to_char("NPCs cannot use score.\n\r", ch);
        return;
    }
    
    /* These buffers help organize the output */

    /* custombuf - specifically a 'pflag' set by an imm */
    if ( ch->pcdata != NULL && ch->pcdata->customflag[0] != '\0' )
        sprintf( custombuf, "(%s) ", ch->pcdata->customflag );
    else
        custombuf[0] = '\0';

    /* flagsbuf - AFK, helper, rp, killer, thief, etc. */
    sprintf( flagsbuf, "%s%s%s%s%s",
        IS_SET(ch->comm, COMM_AFK) ? "[AFK] " : "",
        IS_SET(ch->act,PLR_HELPER) ? 
            (!IS_SET(ch->act, PLR_INACTIVE_HELPER) ? "({GH{CE{cL{GP{CE{cR{G!{x) " : "{G*{x ") : "",
        IS_SET(ch->act,PLR_RP) ? "[RP] " : "",
        IS_SET(ch->act,PLR_KILLER) ? "(KILLER) " : "",
        IS_SET(ch->act,PLR_THIEF) ? "(THIEF)" : "" );


    /* remortbuf - display either remort or trust value*/
    if( !IS_IMMORTAL(ch) )
        sprintf( remortbuf, "(Remort %d)", ch->pcdata->remorts );
    else if ( get_trust(ch) != ch->level || IS_IMMORTAL(ch))
        sprintf( remortbuf, "(Trust %d)", get_trust(ch) );


    /* alignbuf */
    align = ch->alignment;
    if( align > 900 )       sprintf( alignbuf, "{W%5d        (angelic){x ", align );
    else if( align >  700 ) sprintf( alignbuf, "{c%5d        (saintly){x ", align );
    else if( align >  350 ) sprintf( alignbuf, "{c%5d        (good){x    ", align );
    else if( align >  100 ) sprintf( alignbuf, "{m%5d        (kind){x    ", align );
    else if( align > -100 ) sprintf( alignbuf, "{m%5d        (neutral){x ", align );
    else if( align > -350 ) sprintf( alignbuf, "{m%5d        (mean){x    ", align );
    else if( align > -700 ) sprintf( alignbuf, "{r%5d        (evil){x    ", align );
    else if( align > -900 ) sprintf( alignbuf, "{r%5d        (demonic){x ", align );
    else                    sprintf( alignbuf, "{D%5d        (satanic){x ", align );

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

    /* encumberbuf - how encumbered is the character */
    if( (encumber = get_encumberance(ch)) > 0 )
    {
        if( encumber <= 25 )      sprintf( encumberbuf, "  {mmild{x" );
        else if( encumber <= 50 ) sprintf( encumberbuf, "  {wfair{x" );
        else if( encumber <= 75 ) sprintf( encumberbuf, "  {rhigh{x" );
        else sprintf( encumberbuf, "{Rsevere{x" );
    }
    else
        sprintf(encumberbuf, "  none");

    output = new_buf();


    /******* THE NEW, IMPROVED SCORE SCREEN - July 2014 *******/

    add_buf(output, "{D:============================================================================:{x\n\r");

    /* Show pflags, regular flags, name color, pre_title, name, title, etc. */
    sprintf(buf, "{D|{x %s%s%s%s%s{x%s", 
        flagsbuf, 
        custombuf, 
        ch->pcdata->name_color, 
        ch->pcdata->pre_title, 
        ch->name, 
        ch->pcdata->title);

    /* This line is used throughout to close each score line */
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    /* Level, Remorts/Trust, Clan Name, Clan Rank */
    sprintf(buf, "{D|{x Level: %3d %-11s    Clan: %s%13s{x        Rank: %s%12s  {x", 
        ch->level, 
        remortbuf,
        clan_table[ch->clan].active ? clan_table[ch->clan].who_color : "",
        clan_table[ch->clan].active ? clan_table[ch->clan].who_name : "None",
        clan_table[ch->clan].active ? clan_table[ch->clan].who_color : "",
        clan_table[ch->clan].active ? capitalize(clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].name) : "None");

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    /* Class, Race, Gender */
    sprintf(buf, "{D|{x Class: %11s        Race: %13s        Gender: %10s", 
        class_table[ch->clss].name, 
        race_table[ch->race].name,
        ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female" );

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );

    /* Remort, Ascent, Subclass */
    sprintf(buf, "{D|{x Sub: %13s        Ascent: %11d        Remort: %10d",
        subclass_table[ch->pcdata->subclass].name,
        ch->pcdata->ascents,
        ch->pcdata->remorts);

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );
    

    sprintf(buf, "{D|{x Dual: %12s        Age:    %5d years        Played:  %5d hrs",
        subclass_table[ch->pcdata->subclass2].name,
        get_age(ch),
        (ch->played + (int)(current_time - ch->logon))/3600);

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf( output, buf );


    /* Holy Light, Wizinvis and Incog Levels */
    if( IS_IMMORTAL(ch) )
    {
        sprintf( buf, "{D|{x Holylight:     {W%3s{x        Wizinvis:       {W%3d{x        Incognito:     {W%3d{x",
            IS_SET(ch->act, PLR_HOLYLIGHT) ? "ON" : "OFF", 
            IS_WIZI(ch) ? ch->invis_level : 0, 
            IS_INCOG(ch) ? ch->incog_level : 0 );

        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
            ; 
        strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );
    }

    add_buf(output, "{D:============================================================================:{x\n\r");


    /* Practices, Trains */
    if( !IS_HERO(ch) && ch->pcdata->highest_level <= ch->level )
        sprintf( temp, "   {cExpect to gain about{x %.2f {cnext level.", ch_prac_gains(ch, ch->level + 1)/100.0 );
    else
        temp[0] = '\0';

    sprintf( buf, "{D|{x Practices:   {C%5d{x     %s", ch->practice, temp );
    
    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    get_hmm_softcap( ch, &hp_cap, &mana_cap, &move_cap );
    sprintf( buf, "{D|{x Trains:      {C%5d        {cSpent:{x %d/%d {chp{x %d/%d {cmn{x %d/%d {cmv {cMAX %d{x",
        ch->train, ch->pcdata->trained_hit, hp_cap, ch->pcdata->trained_mana, 
        mana_cap, ch->pcdata->trained_move, move_cap, max_hmm_train(ch->level) );

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    /* Call the alignbuf here to show alignment */
    sprintf( buf, "{D|{x Alignment:   %s", alignbuf );

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    /* Cleaned up this section below but leaving commented out - Astark
    RELIGION_DATA *religion = NULL;

    if( ch->pcdata != NULL && (religion = get_religion(ch)) != NULL )
    {
        sprintf( temp, "Your religion allows %d to %d.", religion->min_align, religion->max_align );

        if( IS_IMMORTAL(ch) )
        {
            bool rel_at_altar = FALSE;
            if( get_religion_of_altar( get_obj_room(religion->relic_obj) ) == religion )
                rel_at_altar = TRUE;

            sprintf( buf, "{D|{x Faith Pool: {B%d{x", religion->god_power );
            sprintf( temp, "Relic: %s", !rel_at_altar ? "{Rmissing{x" : "safe" );
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

    */


    /* Mob kills, mob deaths, beheads */
    sprintf(buf, "{D|{x Mob Kills:  %6d        Mob Deaths:   %5d        Behead Count: %4d",
        ch->pcdata->mob_kills, 
        ch->pcdata->mob_deaths, 
        ch->pcdata->behead_cnt);

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    /* Warfare grade, kills and points */
    sprintf( buf, "{D|{x War Kills:  %6d        Warfare Grade:    %1s        Warfare Pts: %5d",
        ch->pcdata->war_kills, 
        pkgrade_table[get_pkgrade_level(ch->pcdata->warpoints)].grade,
        ch->pcdata->warpoints);

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    /* Pkills and Pkill Deaths */
    sprintf( buf, "{D|{x PKills:     %6d        PKill Deaths: %5d",
        ch->pcdata->pkill_count, 
        ch->pcdata->pkill_deaths);

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    add_buf(output, "{D:============================================================================:{x\n\r");


    /* Display pflag, but only if it exists */
    if( custombuf[0] != '\0' )
    {
        sprintf( buf, "{D|{x Your %sflag has %d hours remaining. ", 
            custombuf, 
            ch->pcdata->customduration );
        
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
            ; 
        strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );
    }


    /* Position and Stance */
    sprintf(buf, "{D|{x Position: %8s        Stance: {G%11s{x        Song: {G%11s{x",
        positionbuf, 
        ch->stance == STANCE_DEFAULT ? "None" : capitalize(stances[ch->stance].name),
        ch->song == SONG_DEFAULT ? "None" : capitalize(songs[ch->song].name) );

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );

    
    /* Clean this part up some more, but will only show when booleans are true */
    drunk  = ch->pcdata->condition[COND_DRUNK];
    thirst = ch->pcdata->condition[COND_THIRST];
    hunger = ch->pcdata->condition[COND_HUNGER];

    if( (drunk  >= 10 )              )  drunken = TRUE;
    if( (hunger >= 0 && hunger < 20) )  hungry  = TRUE;
    if( (thirst >= 0 && thirst < 20) )  thirsty = TRUE;

    if( drunken || hungry || thirsty )
    {
        sprintf(buf, "{D|{x Hungry:   %s        Thirsty: %s        Drunk: %s",
            hunger == 0 ? "{Rstarving{x" : hunger > 0 && hunger < 20 ? "     yes" : "    None",
            thirst == 0 ? "{Rdesiccated{x" : thirst > 0 && thirst < 20 ? "       yes" : "      None",
            drunk > 20 ? "{Rintoxicated{x" : drunk > 10 && drunk <= 20 ? "     buzzed" : "       None");

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );
    }


    /* Items carried, carry weight, encumbered */
    sprintf( buf, "{D|{x Items:   %-4d/%4d        Weight: %-5d/%5d        Encumbered: %s",
        ch->carry_number, 
        can_carry_n(ch), 
        (int)get_carry_weight(ch)/10,
        (int)can_carry_w(ch)/10,
        encumberbuf);

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    /* Morph information for races that utilize it */
    if ( MULTI_MORPH(ch) )
    {
        if( ch->pcdata->morph_race > 0 )
            sprintf( buf, "{D|{x Morph: {G%11s{x        Hours:  {G%d{x remaining",
                race_table[ch->pcdata->morph_race].name,  ch->pcdata->morph_time );
        else
            sprintf( buf, "{D|{x Morph:  {Gbasic form{x" );
        
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
            ; 
        strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );
    }
    else if( ch->race == race_naga )
    {
        if( ch->pcdata->morph_race == 0 )
            sprintf( buf, "{D|{x Morph:     {Gserpent{x" );
        else
            sprintf( buf, "{D|{x Morph:    {Ghumanoid{x" );
    
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
            ; 
        strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );
    }


    /* Show command currently actioned */
    if ( ch->pcdata->combat_action == NULL )
        sprintf( buf, "{D|{x Command Actioned:         {wNone{x");
    else
        sprintf( buf, "{D|{x Command Actioned:         {w%s{x", ch->pcdata->combat_action );

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf(output, buf );


    add_buf(output, "{D:============================================================================:{x\n\r");
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





DEF_DO_FUN(do_worth)
{
    char buf[MAX_STRING_LENGTH];
    int LENGTH = 76;

    if( !strcmp(argument,"for_score") )
    ;
    else send_to_char("{D:============================================================================:{x\n\r", ch);

    if ( !ch->pcdata )
    {
        sprintf(buf, "{D|{x You have {C%ld gold{x and {W%ld silver{x.", ch->gold, ch->silver );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
            ; 
        strcat( buf, "{D|{x\n\r" );
        send_to_char( buf, ch );
        send_to_char("{D:============================================================================:{x\n\r", ch);
        return;
    }
  

    /* Gold, Silver, Bank */ 
    sprintf(buf, "{D|{x Gold:    {Y%9d{x        Silver: {w%11d{x        In Bank: {Y%9d{x",
        (int)ch->gold,
        (int)ch->silver,
        (int)ch->pcdata->bank);

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" );
    send_to_char( buf, ch );


    /* ETL, Field, EXP */
    sprintf( buf, "{D|{x EXP to Lvl:   %4d        Field EXP:   %6d        Total Exp: %7d",
        (int)((ch->level + 1) * exp_per_level(ch) - ch->exp),
        (int)ch->pcdata->field,
        (int)ch->exp);

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" );
    send_to_char( buf, ch );


    /* Quest Points, Achievement Points, Storage Boxes */
    sprintf( buf, "{D|{x Quest Pts:   {B%5d{x        Faith:       {c%6d{x        Storage Boxes:   %-2d",
        ch->pcdata->questpoints,
        ch->pcdata->faith,
        ch->pcdata->storage_boxes);

    for( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" );
    send_to_char( buf, ch );



    if ( ch->pcdata->bounty > 0 )
    {
        sprintf( buf, "{D|{x There is a {Rbounty{x of {Y%d gold{x on your head.", ch->pcdata->bounty );
        for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
            ; 
        strcat( buf, "{D|{x\n\r" );
        send_to_char( buf, ch );
    }

    send_to_char("{D:============================================================================:{x\n\r", ch);

    return;
}

DEF_DO_FUN(do_attributes)
{
    /* Just a hack of score to give players a convenient way to view main attributes */
    /* Memnoch 02/98 */
    /* Updated spring 2004 by Quirky along with 'score' and 'worth' */

    char buf[MAX_STRING_LENGTH];
    BUFFER *output;
    int LENGTH = 76;
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
        add_buf(output, "{D:============================================================================:{x\n\r");


    /* Hitpoints (HP), Mana (MN), Move (MV) */
    sprintf( buf, "{D|{x {CHP:{x    {%c%5d{x/%5d        {CMana:{x   {%c%5d{x/%5d        {CMoves:{x {%c%5d{x/%5d",
        hp_col, ch->hit, ch->max_hit,
        mn_col, ch->mana, ch->max_mana, 
        mv_col, ch->move, ch->max_move );

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf( output, buf );

 
    /* Armor Class, Saves Magic, Saves Physical */
    sprintf( buf, "{D|{x {CArmor {CClass{x: %5d        {CSaves Phys{x:    %4d        {CSaves Magic{x:  %4d",
        GET_AC(ch),
        get_save(ch, TRUE),
        get_save(ch, FALSE));

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf( output, buf );


    /* Hitroll, Damroll, Wimpy, Calm */
    sprintf( buf, "{D|{x {CHitroll:{x      %4d        {CDamroll:{x       %4d        {CSpell Pierce{x: %4d",
        GET_HITROLL(ch),
        GET_DAMROLL(ch),
        get_spell_penetration(ch, ch->level));

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf( output, buf );

    /* Wimpy, Calm */
    sprintf( buf, "{D|{x {CWimpy:{x        %3d%%        {CCalm:{x          %3d%%        {CSpell Damage{x: %4d",
        ch->wimpy,
        ch->calm,
        get_spell_bonus_damage(ch, PULSE_VIOLENCE, TRUE, NULL));

    for ( ; strlen_color(buf) <= LENGTH; strcat( buf, " " ))
        ; 
    strcat( buf, "{D|{x\n\r" ); add_buf( output, buf );

    /* ** Stats ** */
    if (IS_SET(ch->comm, COMM_SHOW_STATBARS))
        print_stat_bars( ch, output );
    else
    {
        sprintf( buf, "{D|{x {cStr:{x %3d(%3d)  {cCon:{x %3d(%3d)  {cVit:{x %3d(%3d)  {cAgi:{x %3d(%3d)  {cDex:{x %3d(%3d)  {D|{x\n\r",
            ch->perm_stat[STAT_STR], get_curr_stat(ch,STAT_STR),
            ch->perm_stat[STAT_CON], get_curr_stat(ch,STAT_CON),
            ch->perm_stat[STAT_VIT], get_curr_stat(ch,STAT_VIT),
            ch->perm_stat[STAT_AGI], get_curr_stat(ch,STAT_AGI),
            ch->perm_stat[STAT_DEX], get_curr_stat(ch,STAT_DEX)  );
        add_buf( output, buf );

        sprintf( buf, "{D|{x {cInt:{x %3d(%3d)  {cWis:{x %3d(%3d)  {cDis:{x %3d(%3d)  {cCha:{x %3d(%3d)  {cLuc:{x %3d(%3d)  {D|{x\n\r",
            ch->perm_stat[STAT_INT], get_curr_stat(ch,STAT_INT),
            ch->perm_stat[STAT_WIS], get_curr_stat(ch,STAT_WIS),
            ch->perm_stat[STAT_DIS], get_curr_stat(ch,STAT_DIS),
            ch->perm_stat[STAT_CHA], get_curr_stat(ch,STAT_CHA),
            ch->perm_stat[STAT_LUC], get_curr_stat(ch,STAT_LUC)  );
        add_buf( output, buf );
    }

    add_buf(output, "{D:============================================================================:{x\n\r");
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
    const struct stat_type *stat;
    
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
        sprintf( buf, "{D|{x {c%s: {x%3d %s %3d => %3d   [{g%s{x]  {D|{x\n\r"
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
DEF_DO_FUN(do_percentages)
{
    BUFFER *output;
    int LENGTH = 77;

    output = new_buf();
    if ( strcmp(argument,"for_score") )
        add_buf(output, "{D:============================================================================:{x\n\r");

    // secondary and two-handed weapons
    addf_buf_pad(output, LENGTH, "{D|{x {cOffhand Attacks:{x %3d%%   {cTwohand Penalty:{x %3d%%        {cFocus Bonus:{x  %3d%%",
        offhand_attack_chance(ch, FALSE),
        get_twohand_penalty(ch, FALSE),
        get_focus_bonus(ch)
    );
    add_buf(output, "{D|{x\n\r");

    // dodge, parry, block
    addf_buf_pad(output, LENGTH, "{D|{x           {cDodge:{x %3d%%             {cParry:{x %3d%%              {cBlock:{x  %3d%%",
        dodge_chance(ch, ch->fighting, FALSE),
        parry_chance(ch, ch->fighting, FALSE),
        shield_block_chance(ch, FALSE)
    );
    add_buf(output, "{D|{x\n\r");
    
    int crit = critical_chance(ch, FALSE);
    int heavy_bonus = get_heavy_armor_bonus(ch);
    if ( crit || heavy_bonus )
    {
        addf_buf_pad(output, LENGTH, "{D|{x        {cCritical:{x %5.2f%%     {cHeavy Armor:{x %3d%%      {cHeavy Penalty:{x  %3d%%",
            crit / 20.0,
            heavy_bonus,
            get_heavy_armor_penalty(ch)
        );
        add_buf(output, "{D|{x\n\r");
    }
    
    int fade = fade_chance(ch);
    int misfade = misfade_chance(ch);
    if ( fade )
    {
        if ( misfade )
            addf_buf_pad(output, LENGTH, "{D|{x            {cFade:{x %3d%%/%d%%", fade, misfade);
        else
            addf_buf_pad(output, LENGTH, "{D|{x            {cFade:{x %3d%%", fade);
        add_buf(output, "{D|{x\n\r");
    }
    
    add_buf(output, "{D:============================================================================:{x\n\r");
    page_to_char(buf_string(output), ch);
    free_buf(output);

    return;
}

// breakdown of hp/mana/move calculation
DEF_DO_FUN(do_breakdown)
{
    if ( IS_NPC(ch) )
        return;
    
    ptc(ch, "%25s%16s%16s\n\r", "{rHitpts{x", "{BMana{x", "{cMoves{x");
    ptc(ch, "%-15s%6d%12d%12d\n\r", "Natural",
        ch->pcdata->perm_hit, ch->pcdata->perm_mana, ch->pcdata->perm_move);
    
    ptc(ch, "%-15s%6d%12d%12d\n\r", "Equipment",
        ch->pcdata->temp_hit, ch->pcdata->temp_mana, ch->pcdata->temp_move);
    
    int hero_bonus = get_hero_factor(modified_level(ch)) - 100;
    int hero_hit = ch->pcdata->temp_hit * hero_bonus / 100;
    int hero_mana = ch->pcdata->temp_mana * hero_bonus / 100;
    int hero_move = ch->pcdata->temp_move * hero_bonus / 100;
    if ( hero_hit || hero_mana || hero_move )
    {
        ptc(ch, "%-15s%6d%12d%12d       (%d%%)\n\r", "Hero-Bonus",
            hero_hit, hero_mana, hero_move, hero_bonus);
    }

    int train_hit  = ch->max_hit  - (ch->pcdata->perm_hit  + ch->pcdata->temp_hit  + hero_hit);
    int train_mana = ch->max_mana - (ch->pcdata->perm_mana + ch->pcdata->temp_mana + hero_mana);
    int train_move = ch->max_move - (ch->pcdata->perm_move + ch->pcdata->temp_move + hero_move);
    if ( train_hit || train_mana || train_move )
        ptc(ch, "%-15s%6d%12d%12d\n\r", "Training", train_hit, train_mana, train_move);
    
    int nat_sp = get_save(ch, TRUE) - ch->saving_throw;
    int nat_sm = get_save(ch, FALSE) - ch->saving_throw;
    int nat_hitroll = get_hitroll(ch) - ch->hitroll;
    int nat_damroll = get_damroll(ch) - ch->damroll;
    int nat_armor = get_ac(ch) - ch->armor;
    
    ptc(ch, "\n\r%25s%16s%16s%16s\n\r", "{CArmor{x", "{CSaves P/M{x", "{CHitroll{x", "{CDamroll{x");
    ptc(ch, "%-15s%6d%7d/%4d%12d%12d\n\r", "Natural",
        nat_armor, nat_sp, nat_sm, nat_hitroll, nat_damroll);
    
    ptc(ch, "%-15s%6d%7d/%4d%12d%12d\n\r", "Equip/Buffs",
        ch->armor, ch->saving_throw, ch->saving_throw, ch->hitroll, ch->damroll);
}

DEF_DO_FUN(do_helper)
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
DEF_DO_FUN(do_combo)
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

const char * const achievement_display [] =
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
        "Explored",
        "MA Skills",
        "GM Skills",
        "Retrain",
        "Hard Qsts",
        "Ascension"
};

DEF_DO_FUN(do_achievements)
{
    if ( IS_NPC(ch) )
        return;

    if (!strcmp( argument, "rewards") )
    {
        print_ach_rewards(ch);
        return;
    }

    char buf[MAX_STRING_LENGTH];
    int i;
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d=NULL;
    int col;
    int totalach = 0;
    int ltotal = 0;
    int utotal = 0;

    bool boss=FALSE;

    col = 0;

    if (argument[0] == '\0' )
    {
        victim = ch;
    }
    else if (!str_cmp(argument, "boss"))
    {
        do_achievements_boss( ch, ch );
        return;
    }
    else
    {
        char arg1[MIL];
        char arg2[MIL];
        char *vicarg;

        argument=one_argument( argument, arg1);
        argument=one_argument( argument, arg2);
        if (!str_cmp(arg1, "boss"))
        {
            boss=TRUE;
            vicarg=arg2;

            if (!str_cmp( arg2, "rewards"))
            {
                do_achievements_boss_reward( ch );
                return;
            }
        }
        else
        {
            vicarg=arg1;
        }

        d = new_descriptor();

        if ( !load_char_obj(d, vicarg, TRUE) )
        {
            send_to_char("Character not found.\n\r", ch);
            /* load_char_obj still loads "default" character
               even if player not found, so need to free it */
            if (d->character)
            {
                free_char(d->character);
                d->character=NULL;
            }
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

    if (boss)
    {
        do_achievements_boss( ch, victim);
    }
    else
    {
        BUFFER *output = new_buf();
        
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
        add_buf(output, "(Use 'achievement boss' to see boss achievements.)\n\r");
        page_to_char(buf_string(output),ch);
        free_buf(output);
    }

    /* if not self, need to free stuff */
    if ( d )
    {
        nuke_pets( d->character );
        free_char( d->character );
        free_descriptor( d );
    }
}

void print_ach_rewards(CHAR_DATA *ch)
{
	BUFFER *output;
	char header[MSL], buf[MSL];
	int i;
	 
	
	output = new_buf();
	int type=-1;
	sprintf(header, "\n\r{w%-10s %6s:{x {W%6s{x|{W%6s{x|{W%6s{x|{W%6s{x\n\r", "Type", "Limit", "QP", "Exp", "Gold", "AchP");

	for (i = 0; achievement_table[i].bit_vector != 0; i++)
	{
		const ACHIEVEMENT *entry = &achievement_table[i];
		
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

/* Give achievement to all PC group members in the room.
   Final hit may be from NPC */
void check_boss_achieve( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !IS_NPC( victim ) )
        return;

    BOSSACHV *ach = victim->pIndexData->boss_achieve;
    if ( !ach )
        return;

    BOSSREC *rec;
    CHAR_DATA *plr, *plr_next;


    /* achievement for all PC group members in the room */
    for ( plr=ch->in_room ? ch->in_room->people : NULL; plr; plr=plr_next )
    {
        plr_next=plr->next_in_room;

        if ( IS_NPC(plr) || !is_same_group( plr, ch ) )
            continue;

        /* check if already has it */
        bool found=FALSE;
        for ( rec = plr->pcdata->boss_achievements; rec; rec=rec->next )
        {
            if ( rec->vnum == victim->pIndexData->vnum )
            {
                found=TRUE;
                break;
            }
        }
        if ( found )
            continue;

        /* give the achievement */
        rec = alloc_BOSSREC();
        rec->vnum = victim->pIndexData->vnum;
        rec->timestamp = current_time; 

        rec->next = plr->pcdata->boss_achievements;
        plr->pcdata->boss_achievements = rec;

        /* do the rewards */
        plr->pcdata->questpoints += ach->quest_reward;
        plr->pcdata->bank += ach->gold_reward;
        gain_exp(plr, ach->exp_reward, TRUE);
        plr->pcdata->achpoints += ach->ach_reward;

        printf_to_char(plr, "--------------------------------------\n\r");
        printf_to_char(plr, "{wBoss Achievement unlocked{x.\n\r");
        send_to_char( "{wYour reward{x:\n\r",plr);
        if ( ach->gold_reward>0)
            printf_to_char(plr, "%6d gold\n\r", ach->gold_reward );
        if ( ach->quest_reward>0)
            printf_to_char(plr, "%6d quest points\n\r", ach->quest_reward);
        if (ach->exp_reward>0)
            printf_to_char(plr, "%6d experience points\n\r", ach->exp_reward);
        if (ach->ach_reward>0)
            printf_to_char(plr, "%6d achievement points\n\r", ach->ach_reward );
    }

    return;
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
		current = ch->pcdata->armageddon_won + ch->pcdata->clan_won + ch->pcdata->class_won + ch->pcdata->race_won + ch->pcdata->religion_won + ch->pcdata->gender_won + ch->pcdata->duel_won;
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
            case ACHV_MASKILLS:
                current = ch->pcdata->smc_mastered;
                break;
            case ACHV_GMSKILLS:
                current = ch->pcdata->smc_grandmastered;
                break;
            case ACHV_RETRAINED:
                current = ch->pcdata->smc_retrained;
                break;
            case ACHV_QHCOMP:
                current = ch->pcdata->quest_hard_success;
                break;
            case ACHV_ASCENSION:
                current = ch->pcdata->ascents;
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
        gain_exp(ch, achievement_table[table_index].exp_reward, TRUE);
        ch->pcdata->achpoints += achievement_table[table_index].ach_reward;
        //send_to_char("Achievement unlocked -- TEST.\n\r",ch);
        printf_to_char(ch, "--------------------------------------\n\r");
	printf_to_char(ch, "{wAchievement {R%s %d{w unlocked{x.\n\r", achievement_display[achievement_table[table_index].type], achievement_table[table_index].limit);
	send_to_char( "{wYour reward{x:\n\r",ch);
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
    printf_to_char(ch, "\n\r");
}



DEF_DO_FUN(do_count)
{
    int count, max;
    DESCRIPTOR_DATA *d;
	FILE *fp;

    count = 0;

    if ( IS_NPC(ch) || ch->desc == NULL )
    	return;

    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( IS_PLAYING(d->connected) && can_see( ch, d->character ) )
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




DEF_DO_FUN(do_classes)
{
    int class;

    printf_to_char(ch, "               Att  Def  Hp  Mana  Move prime secondary #Skl / Cost\n");
    for ( class = 0; class < MAX_CLASS; class++ )
    {
        // calculate number and cost of skills
        int sn, skill_count = 0, skill_cost = 0;
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name == NULL )
                break;
            if ( skill_table[sn].skill_level[class] <= LEVEL_HERO && skill_table[sn].rating[class] > 0 )
            {
                skill_count++;
                skill_cost += skill_table[sn].rating[class];
            }
        }
        // list details
        printf_to_char(ch, "%12s:  {r%3d  %3d  {B%3d  %3d  %3d  {y %s   %s %s  {g%4d / %d{x\n",
            class_table[class].name,
            class_table[class].attack_factor,
            class_table[class].defense_factor,
            class_table[class].hp_gain,
            class_table[class].mana_gain,
            class_table[class].move_gain,
            stat_table[class_table[class].attr_prime].abbreviation,
            stat_table[class_table[class].attr_second[0]].abbreviation,
            stat_table[class_table[class].attr_second[1]].abbreviation,
            skill_count, skill_cost
        );
    }
}

#ifdef LAG_FREE
DEF_DO_FUN(do_lagfree)
{
    if (IS_NPC(ch))
        return;

    if ( !ch->desc )
    {
        bugf("do_lagfree: no desc for %s", ch->name);
        return;
    }

    ch->desc->lag_free = !ch->desc->lag_free;

    ptc( ch, "Lag free mode: %s", ch->desc->lag_free ? "ON" : "OFF");
}
#endif




struct newbie_data
{
    int lvl;
    const char * area_name;
};

const struct newbie_data eq_data[] =
{
    {  1, "The Initiation"                                                  },
    {  5, "The Pirates Lair or The Palace Square Shops"                     },
    { 10, "Crystal Coast"                                                   },
    { 13, "Vorgath's Dungeon"                                               },
    { 15, "JROTC"                                                           },
    { 20, "Rovarok"                                                         },
    { 25, "NIMH"                                                            },
    { 30, "Mrem Village (ask for help), Dreamscape, or Kyoto Village"       },
    { 35, "Akyros Pharmaceuticals"                                          },
    { 42, "Square World"                                                    },
    { 50, "Dreamscape (fighters) or Logging Camp (casters)"                 },
    { 60, "Battlelords (The gear from the 'traveler' mobs)"                 },
    { 63, "Princess Bride"                                                  },
    { 70, "Abyss (ask for help)"                                            },
    { 75, "Spacehulk"                                                       },
    { 81, "Battlelords (The gear from non 'traveler' mobs)"                 },
    { 85, "Angel's Heaven"                                                  },
    { 90, "Mortal Kombat"                                                   },
    {  0, NULL                                                              }
};

DEF_DO_FUN(do_eqhelp)
{
    OBJ_DATA *obj;
    bool wield = FALSE;
    int curr_eq = 0;
    int sugg_eq;
    int i;
    int curr_wield = 0;
    int sugg_wield;

    /* First lets find where in the table the player is at */
    for ( i = 0 ; eq_data[i].area_name != NULL; i++ )
    {
        if (ch->level < eq_data[i].lvl)
            break;
    }

    /* We need to move down one position in the table to get the real value */

    i = i-1; 

    /* Now that we know the table position, lets find out what the power 
       of their EQ is based on its level */

    for ( obj = ch->carrying; obj != NULL ; obj = obj->next_content )
    {
        if ( obj->wear_loc != WEAR_NONE)
        {
            if (IS_SET(obj->extra_flags, ITEM_QUESTEQ) && obj->level <= 70)
                curr_eq += 70; /* Current value of QEQ in terms of level power */
            else
                curr_eq += obj->level;
        }

        if ( obj->wear_loc == WEAR_WIELD)
        {
            wield = TRUE;
            curr_wield = obj->level;
        }
    }

    /* We are going to assume that 15 slots of EQ are used to determine
       the power of the suggested EQ */

    sugg_eq = 15 * eq_data[i].lvl;
    sugg_wield = ch->level;
    
    /* Lets figure out the difference */

    int diff_percent = (200 * curr_eq + 1) / (sugg_eq * 2);
    int diff_percent2 = 100 - diff_percent;

    /* This is a test function to confirm our information 
    int diff = sugg_eq - curr_eq;
    sprintf(buf,"I_value=%d, Table_value=%d, curr_eq=%d, sugg_eq=%d, 
        difference=%d (%d%% of suggested, or %d%% weaker than suggested)\n\r\n\r", 
            i, eq_data[i].lvl, curr_eq, sugg_eq, diff, diff_percent, diff_percent2);
    send_to_char(buf,ch); */


    /* Now lets print the information to see our progress */

    printf_to_char(ch,"You {R%s{x \n\r", 
        diff_percent2 >= 20 ? "could use an equipment upgrade." : 
            "have equipment that suits your level.");

    printf_to_char(ch,"{yYour equipment is {R%d%%{y %s than expected for your level.{x\n\r", 
        diff_percent2 < 0 ? diff_percent2 * -1 : diff_percent2,
        diff_percent2 < 0 ? "stronger" : "weaker" );


    /* If the player has a weapon equipped we will tell them if it needs 
       improving. If not, we ignore this piece (i.e Monks) */

    if (wield == TRUE)
        if ((sugg_wield - curr_wield) > 5)
            printf_to_char(ch,"{yYour weapon is also {R%d{y levels below your current level.{x\n\r", sugg_wield - curr_wield);


    /* Don't use a shield without the skill ... */

    if (get_skill(ch,gsn_shield_block) < 1 && get_eq_char(ch,WEAR_SHIELD) != NULL)
        printf_to_char(ch,"{yYou are wearing a shield without the shield block skill.{x\n\r");

    
    /* Or focus ... */

    if (get_skill(ch,gsn_focus) < 1 && get_eq_char(ch,WEAR_HOLD) != NULL)
        printf_to_char(ch,"{yYou are using a held item without the focus skill.{x\n\r");

    /* Held item provides no focus if shield is worn
    if ( get_eq_char(ch,WEAR_SHIELD) != NULL && get_eq_char(ch,WEAR_HOLD) != NULL )
        printf_to_char(ch, "{yYou are holding an item while wearing a shield.{x\n\r");
    */
    
    /* Wrist shield too ... */

    if ((get_skill(ch,gsn_wrist_shield) < 1 && get_eq_char(ch,WEAR_SHIELD) != NULL) 
        && (get_eq_char(ch,WEAR_SECONDARY) != NULL 
        || get_eq_char(ch,WEAR_HOLD) != NULL))
            printf_to_char(ch,"{yYou are using a wrist shield without the wrist shield skill.{x\n\r");

    /* Only telling the player they need better equipment doesn't help much.
       We will tell them where to find it */

    if (diff_percent2 >= 20)
       printf_to_char(ch,"{yYou can find better equipment at %s.{x\n\r", eq_data[i].area_name);
    
}

bool can_take_subclass( int class, int subclass )
{
    return (subclass_table[subclass].base_classes & (1ul<<class)) != 0;
}

bool ch_can_take_subclass( CHAR_DATA *ch, int subclass )
{
    if ( !ch || !ch->pcdata )
        return FALSE;
    // cross-subclassing is possible after two ascents
    if ( ch->pcdata->ascents > 1 )
        return TRUE;
    return can_take_subclass(ch->clss, subclass);
}

bool ch_can_take_dual_subclass( CHAR_DATA *ch, int dual_subclass )
{
    if ( !ch || !ch->pcdata )
        return FALSE;
    if ( !ch->pcdata->subclass || ch->pcdata->subclass == dual_subclass )
        return FALSE;
    // dual-subclassing becomes available after 3 ascents
    return ch->pcdata->ascents > 2;
}

static void show_subclass_skills( CHAR_DATA *ch, int sc, int percent )
{
    int i;
    char skill_header[100];

    if ( percent == 100 )
        sprintf(skill_header, "Skill");
    else
        sprintf(skill_header, "Skill (%.12s)", subclass_table[sc].name);
    ptc(ch, "\n\r%-20s  Level  Percent\n\r", skill_header);

    for ( i = 0; i < MAX_SUBCLASS_SKILL; i++ )
    {
        if ( subclass_table[sc].skills[i] == NULL )
            break;
        int level = subclass_table[sc].skill_level[i] % 100;
        int min_ascent = 1 + subclass_table[sc].skill_level[i] / 100;
        int skill_percent = subclass_table[sc].skill_percent[i] * percent / 100;
        if ( min_ascent == 1 )
            ptc(ch, "%20s    %3d    %3d%%\n\r",
                subclass_table[sc].skills[i],
                level,
                skill_percent
            );
        else
            ptc(ch, "%20s    %3d    %3d%%   (A%d+)\n\r",
                subclass_table[sc].skills[i],
                level,
                skill_percent,
                min_ascent
            );
    }
}

static void show_subclass( CHAR_DATA *ch, int sc )
{
    int class;

    ptc(ch, "{BSubclass: %s{x (", subclass_table[sc].name);
    for ( class = 0; class < MAX_CLASS; class++ )
        if ( can_take_subclass(class, sc) )
            ptc(ch, " %s", class_table[class].name);
    ptc(ch, " )\n\r");

    ptc(ch, "\n\rSpecialty: %s\n\r", subclass_table[sc].specialty);
    show_subclass_skills(ch, sc, 100);
}

static void show_dual_subclass( CHAR_DATA *ch, int sc, int dual )
{
    ptc(ch, "{BSubclass: %s/%s{x\n\r", subclass_table[sc].name, subclass_table[dual].name);
    ptc(ch, "\n\rSpecialty: %s\n\r", subclass_table[sc].specialty);
    show_subclass_skills(ch, sc, 60);
    show_subclass_skills(ch, dual, 40);
}

DEF_DO_FUN(do_showsubclass)
{
    int sc, class;
    bool found = FALSE;
    
    if ( argument[0] == '\0' )
    {
        if (ch->pcdata->subclass == 0)
            send_to_char("Syntax: showsubclass <subclass|class|all>\n\r", ch);
        else if ( ch->pcdata->subclass2 == 0 )
            show_subclass(ch, ch->pcdata->subclass);
        else
            show_dual_subclass(ch, ch->pcdata->subclass, ch->pcdata->subclass2);
        return;
    }
    
    /*
    if ( !strcmp(argument, "all") )
    {
        for ( sc = 1; subclass_table[sc].name != NULL; sc++ )
        {
            if ( found )
                ptc(ch, "\n\r");
            else
                found = TRUE;
            show_subclass(ch, sc);
        }
        if ( !found )
            ptc(ch, "None found.\n\r");
        return;
    }
    */

    if ( !strcmp(argument, "all") )
    {
        for ( class = 0; class < MAX_CLASS; class++ )
        {
            bool first = TRUE;
            ptc(ch, "%12s:", class_table[class].name);
            for ( sc = 1; subclass_table[sc].name != NULL; sc++ )
                if ( can_take_subclass(class, sc) )
                {
                    ptc(ch, "%s %s", first ? "" : ",", subclass_table[sc].name);
                    first = FALSE;
                }
            ptc(ch, "\n\r");
        }
        return;
    }

    char arg1[MSL], arg2[MSL];
    if ( split_string(argument, '/', arg1, arg2) )
    {
        int sc1 = subclass_lookup(arg1);
        int sc2 = subclass_lookup(arg2);
        if ( sc1 > 0 && sc2 > 0 && sc1 != sc2 )
        {
            show_dual_subclass(ch, sc1, sc2);
            return;
        }
    }

    if ( (sc = subclass_lookup(argument)) > 0 )
    {
        show_subclass(ch, sc);
        return;
    }

    if ( (class = class_lookup(argument)) >= 0 )
    {
        for ( sc = 1; subclass_table[sc].name != NULL; sc++ )
            if ( can_take_subclass(class, sc) )
            {
                if ( found )
                    ptc(ch, "\n\r");
                else
                    found = TRUE;
                show_subclass(ch, sc);
            }
        if ( !found )
            ptc(ch, "None found.\n\r");
        return;
    }
    
    send_to_char("That's not a valid subclass, dual subclass or base class.\n\r", ch);
    send_to_char("Syntax: showsubclass <subclass|sc/sc|class|all>\n\r", ch);
}

/* do_tables stuff */
static void print_flag_table(CHAR_DATA *ch, const void *ptr)
{
    const struct flag_type *tbl = ptr;
    char buf[MSL];
    BUFFER *buffer=new_buf();

    int i;
    sprintf(buf, "%-20s %s\n\r", "Name", "Settable");
    add_buf( buffer, buf );
    add_buf( buffer, "----------------------------------------\n\r");
    for ( i=0; tbl[i].name ; i++ )
    {
        sprintf( buf, "%-20s %s\n\r", tbl[i].name, tbl[i].settable ? "TRUE" : "FALSE" );
        if (!add_buf( buffer, buf ))
        {
            bugf("Bad stuff happened");
            free_buf(buffer);
            return;
        }
    }

    page_to_char( buf_string(buffer), ch );
    free_buf(buffer);
    return;
}

static void print_item_table(CHAR_DATA *ch, const void *ptr)
{
    const struct item_type *tbl = ptr;
    ptc( ch, "Name\n\r");
    ptc( ch, "------------------------------\n\r");
    int i;
    for (i=0 ; tbl[i].name ; i++)
    {
        ptc(ch, "%s\n\r", tbl[i].name );
    }
}

static void print_attack_table(CHAR_DATA *ch, const void *ptr)
{
    const struct attack_type *tbl = ptr;
    ptc( ch, "%-20s %-20s %-20s\n\r", "Name", "Noun", "Damtype");
    ptc( ch, "--------------------------------------------------------------------------------\n\r");
    int i;
    for (i=0 ; tbl[i].name ; i++)
    {
        ptc(ch, "%-20s %-20s %-20s\n\r",
                tbl[i].name,
                tbl[i].noun,
                flag_bit_name(damage_type, tbl[i].damage) );
    }
}

static void print_liq_table(CHAR_DATA *ch, const void *ptr)
{
    const struct liq_type *tbl = ptr;
    ptc( ch, "%-20s %-20s %5s %5s %5s %5s %5s\n\r",
            "Name", "Color",
            "Proof", "Full", "Thrst", "Food", "Ssize");
    ptc( ch, "--------------------------------------------------------------------------------\n\r");
    int i;
    for (i=0 ; tbl[i].liq_name ; i++)
    {
        ptc( ch, "%-20s %-20s %5d %5d %5d %5d %5d\n\r",
                tbl[i].liq_name,
                tbl[i].liq_color,
                tbl[i].liq_affect[0],
                tbl[i].liq_affect[1],
                tbl[i].liq_affect[2],
                tbl[i].liq_affect[3],
                tbl[i].liq_affect[4] );
    }
}

static void print_stances(CHAR_DATA *ch, const void *ptr)
{
    const struct stance_type *tbl = ptr;
    ptc( ch, "%-18s %-10s %-16s %-3s %-3s %-4s\n\r",
            "Name",
            "Damtype",
            "Verb",
            "Hth",
            "Wpn",
            "Cost");
    ptc( ch, "--------------------------------------------------------------------------------\n\r");
    int i;
    for (i=0 ; tbl[i].name ; i++)
    {
        ptc( ch, "%-18s %-10s %-16s %-3s %-3s %4d\n\r",
                tbl[i].name,
                flag_bit_name(damage_type, tbl[i].type),
                tbl[i].verb,
                tbl[i].martial ? "YES" : "no",
                tbl[i].weapon ? "YES" : "no",
                tbl[i].cost);
    }

}

static void print_skill_table (CHAR_DATA *ch, const void *ptr)
{
    const struct skill_type *tbl = ptr;
    ptc( ch, "%3s %s\n\r", "SN","Name");
    int sn;
    for ( sn=0 ; tbl[sn].name ; sn++)
    {
        ptc( ch, "%3d %s\n\r",
                sn,
                tbl[sn].name);
    }
}

static void print_wiznet_table (CHAR_DATA *ch, const void *ptr)
{
    const struct wiznet_type *tbl = ptr;
    ptc( ch, "%3s %s\n\r", "Lvl", "Channel");

    int i;
    for ( i=0 ; tbl[i].name ; i++)
    {
        ptc( ch, "%3d %s\n\r",
                tbl[i].level,
                tbl[i].name);
    }
}

static void print_race_table (CHAR_DATA *ch, const void *ptr)
{
    const struct race_type *tbl = ptr;
    char buf[MSL];
    BUFFER *buffer = new_buf();

    int i;
    sprintf(buf, "%-20s %s\n\r", "Name", "PC");
    add_buf(buffer, buf);
    add_buf(buffer, "----------------------------------------\n\r");

    for (i=0; tbl[i].name; i++)
    {
        sprintf(buf, "%-20s %s\n\r", tbl[i].name, (tbl[i].pc_race) ? "TRUE" : "FALSE");
        if (!add_buf(buffer, buf))
        {
            bugf("BUFFER OVERFLOW???");
            free_buf(buffer);
            return;
        }
    }

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return;
}

static void print_title_table (CHAR_DATA *ch, const void *ptr)
{
    BUFFER *buffer = new_buf();

    const size_t class_cnt = sizeof(title_table) / sizeof(title_table[0]);
    const size_t title_cnt = sizeof(title_table[0]) / sizeof(title_table[0][0]);


    size_t i, j;
    for ( i=0 ; i < class_cnt ; ++i )
    {
        add_buf(buffer, "---------------------------------------------\n\r");
        add_buf(buffer, class_table[i].name );
        add_buf(buffer, " titles\n\r");
        add_buf(buffer, "---------------------------------------------\n\r");

        for ( j=0; j < title_cnt ; ++j )
        {
            add_buf(buffer, " ");
            add_buf(buffer, title_table[i][j]);
            add_buf(buffer, "\n\r");
        }
    }

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
}

#define PRFLAG( flgtbl, note ) { #flgtbl , print_flag_table, flgtbl, note, FALSE }

static struct
{
    const char * const name;
    void (* const printfun)(CHAR_DATA *, const void *);
    const void * const table;
    const char * const note;
    const bool mort; /* Whether mortals can view it */
} dotable_table[]=
{
    PRFLAG( area_flags, "" ),
    PRFLAG( exit_flags, "" ),
    PRFLAG( damage_type, "" ),
    PRFLAG( type_flags, "Item type flags." ),
    PRFLAG( affect_flags, "" ),
    PRFLAG( off_flags, "Offensive flags." ),
    PRFLAG( imm_flags, "Immune flags." ),
    PRFLAG( res_flags, "Resist flags." ),
    PRFLAG( vuln_flags, ""),
    PRFLAG( extra_flags, "Item extra flags." ),
    PRFLAG( wear_types, "" ),
    PRFLAG( room_flags, "" ),
    PRFLAG( wear_loc_flags, ""),
    PRFLAG( act_flags, ""),
    PRFLAG( plr_flags, ""),
    PRFLAG( form_flags, ""),
    PRFLAG( part_flags, ""),
    PRFLAG( comm_flags, ""),
    PRFLAG( mprog_flags, ""),
    PRFLAG( oprog_flags, ""),
    PRFLAG( aprog_flags, ""),
    PRFLAG( rprog_flags, ""),
    PRFLAG( sex_flags, ""),
    PRFLAG( door_resets, ""),
    PRFLAG( sector_flags, ""),
    PRFLAG( apply_flags, ""),
    PRFLAG( wear_loc_strings, ""),
    PRFLAG( container_flags, ""),
    PRFLAG( size_flags, ""),
    PRFLAG( weapon_class, ""),
    PRFLAG( weapon_type2, ""),
    PRFLAG( position_flags, ""),
    PRFLAG( portal_flags, ""),
    PRFLAG( furniture_flags, ""),
    PRFLAG( apply_types, ""),

    { "item_table",   print_item_table,   item_table,   "Item types.",      FALSE },
    { "attack_table", print_attack_table, attack_table, "Attack types.",    FALSE },
    { "liq_table",    print_liq_table,    liq_table,    "Liquid types.",    FALSE },
    { "stances",      print_stances,      stances,      "Stances.",         FALSE },
    { "skill_table",  print_skill_table,  skill_table,  "Skills.",          FALSE },
    { "wiznet_table", print_wiznet_table, wiznet_table, "Wiznet channels.", FALSE },
    { "race_table",   print_race_table,   race_table,   "Race table.",      FALSE },

    { "title_table",  print_title_table,  title_table,  "Title table.",     TRUE  },
    { NULL, NULL, NULL, NULL, FALSE }
};
#undef PRFLAG

DEF_DO_FUN(do_tables)
{
    int i;

    bool immort = IS_IMMORTAL(ch);

    if ( argument[0] == '\0' )
    {
        ptc( ch, "%-20s %s\n\r", "Table", "Note");
        ptc( ch, "-----------------------------------------------------------\n\r");
        for ( i=0; dotable_table[i].name ; i++ )
        {
            if ( dotable_table[i].mort || immort )
            {
                ptc(ch, "%-20s %s\n\r", 
                    dotable_table[i].name,
                    dotable_table[i].note );
            }
        }
        return;
    }

    for ( i=0; dotable_table[i].name ; i++ )
    {

        if ( (dotable_table[i].mort || immort) 
            && !str_prefix( argument, dotable_table[i].name ) )
        {
            dotable_table[i].printfun(
                    ch,
                    dotable_table[i].table);
            return;
        }
    }

    do_tables( ch, "");

}
