/****************************************************************************
*  Automated Quest code written by Vassago of MOONGATE, moongate.ams.com    *
*  4000. Copyright (c) 1996 Ryan Addams, All Rights Reserved. Use of this   * 
*  code is allowed provided you add a credit line to the effect of:         *
*  "Quest Code (c) 1996 Ryan Addams" to your logon screen with the rest     *
*  of the standard diku/rom credits. If you use this or a modified version  *
*  of this code, let me know via email: moongate@moongate.ams.com. Further  *
*  updates will be posted to the rom mailing list. If you'd like to get     *
*  the latest version of quest.c, please send a request to the above add-   *
*  ress. Quest Code v2.01. Please do not remove this notice from this file. *
****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include "merc.h"
#include "special.h"
#include "lua_main.h"
#include "lua_arclib.h"
#include "mudconfig.h"
#include "olc.h"
#include "warfare.h"

DECLARE_DO_FUN( do_say );
DECLARE_DO_FUN( do_startwar );

/* Object vnums for Quest Rewards */

#define QUEST_ITEM1 10387 //Sword of Kings
#define QUEST_ITEM2 10389 //Staff of Ancients
#define QUEST_ITEM3 10386 
#define QUEST_ITEM4 10380 //Old Smote's
#define QUEST_ITEM5 10381 //Old Rimbol's
#define QUEST_ITEM6 10382 //Old Meph's
#define QUEST_ITEM7 10383 //Old Swayde
#define QUEST_ITEM8 10384 //Old Para
#define QUEST_ITEM9 10385 //Old Firewitch
#define QUEST_ITEM10 22050 //Old Lilith
#define QUEST_ITEM11 10388 //Old Quirky
#define QUEST_ITEM12 10398 //Old Siva
#define QUEST_ITEM13 22051 //Old Eris
#define QUEST_ITEM14 22052 //Old Bobble
#define QUEST_ITEM15 22053 //Old Drexl
#define QUEST_ITEM16 4700 //New Quirky
#define QUEST_ITEM17 4701 //New Lilith
#define QUEST_ITEM18 4702 //New Firewitch
#define QUEST_ITEM19 4703 //New Eris
#define QUEST_ITEM20 4704 //New Drexl
#define QUEST_ITEM21 4705 //New Parademia
#define QUEST_ITEM22 4706 //New Swayde
#define QUEST_ITEM23 4707 //New Siva
#define QUEST_ITEM24 4708 //New Mephison
#define QUEST_ITEM25 4709 //New Bobble
#define QUEST_ITEM26 4710 //New Rimbol
#define QUEST_ITEM27 4711 //New Smote
#define QUEST_ITEM28 4712 //Maedhros Leggings
#define QUEST_ITEM29 4726 //Quantum Shield
#define QUEST_ITEM30 4900 //Astark Light

/* Quest timer defines - Maedhros, Feb 8, 2007 */ 
 
#define QUEST_COUNTDOWN_MIN 30           /* Original value: 15 */
#define QUEST_COUNTDOWN_MAX 60           /* Original value: 30 */
#define QUEST_NEXTQUEST_MIN 1            /* Original value: 2 */
#define QUEST_NEXTQUEST_MAX 10           /* Original value: 10 */

/* quest item table */
struct quest_item
{
    int vnum;
    int cost;
    char name[MIL];
};
typedef struct quest_item QUEST_ITEM;

const QUEST_ITEM quest_item_table[] =
{
    { QUEST_ITEM1,  2500, "sword kings" },
    { QUEST_ITEM2,  2000, "staff ancient" },
    { QUEST_ITEM29, 2000, "quantum shield"},
    { QUEST_ITEM30, 2000, "astarks infinite inspiration"},
    { QUEST_ITEM27,  700, "smotes blessing" },
    { QUEST_ITEM26,  700, "rimbols orb insight" },
    { QUEST_ITEM25,  600, "bobbles bughouse suit" },
    { QUEST_ITEM24,   600, "mephistons identity crisis" },
    { QUEST_ITEM23,  600, "sivas mighty bracer" },
    { QUEST_ITEM22,   525, "swaydes cloak temptation" },
    { QUEST_ITEM21,   525, "parademias glove plague slain" },
    { QUEST_ITEM20,  525, "drexls skin malice" },
    { QUEST_ITEM28,  525, "maedhros leggings discord"},
    { QUEST_ITEM19,  425, "eris chastity belt" },
    { QUEST_ITEM18,   425, "firewitches ring fire" },
    { QUEST_ITEM17,  350, "liliths web lies" },
    { QUEST_ITEM16,  350, "quirkys clog calamity" },
/* Old versions of quest eq, can't be bought, sells for full price*/
    { QUEST_ITEM4,  1000, "" },
    { QUEST_ITEM5,  1000, "" },
    { QUEST_ITEM14,  850, "" },
    { QUEST_ITEM6,   850, "" },
    { QUEST_ITEM12,  850, "" },
    { QUEST_ITEM7,   750, "" },
    { QUEST_ITEM8,   750, "" },
    { QUEST_ITEM15,  750, "" },
    { QUEST_ITEM13,  600, "" },
    { QUEST_ITEM9,   600, "" },
    { QUEST_ITEM10,  500, "" },
    { QUEST_ITEM11,  500, "" },
    { 0, 0, "" }
};

char* list_quest_items( void )
{
    static char list_buf[MSL];
    char buf[MIL];
    const QUEST_ITEM *qi;
    OBJ_INDEX_DATA *obj;
    int i;
    char * wloc;

    list_buf[0] = '\0';
    for ( i = 0; quest_item_table[i].vnum != 0; i++ )
    {
        qi = &(quest_item_table[i]);
        if ( (obj = get_obj_index(qi->vnum)) == NULL || !strcmp(qi->name,"") )
            continue;

        if (obj->item_type == ITEM_LIGHT )
        {
            wloc = "<light>";
        }
        else
        {
            switch(obj->wear_type)
            {
                case ITEM_WEAR_FINGER: wloc = "<finger>"; break;
                case ITEM_WEAR_NECK:   wloc = "<neck>"; break;
                case ITEM_WEAR_TORSO:  wloc = "<torso>"; break;
                case ITEM_WEAR_HEAD:   wloc = "<head>"; break;
                case ITEM_WEAR_LEGS:   wloc = "<legs>"; break;
                case ITEM_WEAR_FEET:   wloc = "<feet>"; break;
                case ITEM_WEAR_HANDS:  wloc = "<hands>"; break;
                case ITEM_WEAR_ARMS:   wloc = "<arms>"; break;
                case ITEM_WEAR_SHIELD: wloc = "<shield>"; break;
                case ITEM_WEAR_ABOUT:  wloc = "<body>"; break;
                case ITEM_WEAR_WAIST:  wloc = "<waist>"; break;
                case ITEM_WEAR_WRIST:  wloc = "<wrist>"; break;
                case ITEM_WIELD:       wloc = "<weapon>"; break;
                case ITEM_HOLD:        wloc = "<held>"; break;
                case ITEM_WEAR_FLOAT:  wloc = "<floating>"; break;
                case ITEM_LIGHT:       wloc = "<light>"; break;
                default:               wloc = "<not defined>"; 
            }
        }

        //	sprintf( buf, "%5dqp..........%s\n\r", qi->cost, obj->short_descr );
        sprintf( buf, "%5dqp   %-10s    %s\n\r", qi->cost, wloc, obj->short_descr);
        strcat( list_buf, buf );
    }
    return list_buf;
}

bool create_quest_item( CHAR_DATA *ch, char *name, OBJ_DATA **obj )
{
    const QUEST_ITEM *qi;
    int i;

    for ( i = 0; quest_item_table[i].vnum != 0; i++ )
    {
	qi = &(quest_item_table[i]);
	if ( get_obj_index(qi->vnum) == NULL
	     || !is_name(name, qi->name) )
	    continue;

	/* ok, we found it */
	if ( ch->pcdata->questpoints < qi->cost )
	{
	    send_to_char( "You don't have enough quest points for that.\n\r", ch );
	    return TRUE;
	}

	*obj = create_object_vnum(qi->vnum);
	if ( *obj != NULL )
	{
	    ch->pcdata->questpoints -= qi->cost;
	    logpf( "%s bought %s for %dqp",
	            ch->name, remove_color((*obj)->short_descr), qi->cost );
	}
	else
	    bug( "create_quest_item: couldn't create obj %d", qi->vnum );
	return TRUE;
    }
    return FALSE;
}

bool sell_quest_item( CHAR_DATA *ch, OBJ_DATA *obj, CHAR_DATA *quest_man )
{
    const QUEST_ITEM *qi;
    int i, qp_gain;
    char buf[MSL];
    
    if ( ch == NULL || IS_NPC(ch) || obj == NULL )
        return FALSE;

    /* is obj a quest item? which? */
    for ( i = 0; quest_item_table[i].vnum != 0; i++ )
    {
	qi = &(quest_item_table[i]);
	if ( obj->pIndexData->vnum != qi->vnum )
	    continue;

	/* ok, we found it - owner? */
	if ( !is_obj_owner(ch, obj) )
	{
	    send_to_char( "You don't own that quest item!\n\r", ch );
	    return FALSE;
	}
	
	if ( !strcmp(qi->name,"") || cfg_refund_qeq )
	{
	    qp_gain = qi->cost;//refund full cost on old items
            do_say( quest_man, "As you like, I can refund you the whole cost!" );
	}
	else
	{
	    /* refund part of the price on current items*/
	    qp_gain = qi->cost * 9/10;
	    do_say( quest_man, "As you like, but I can't refund you the whole cost." );
	}
	sprintf( buf, "$N gives you %dqp for $p.", qp_gain );
	act( buf, ch, obj, quest_man, TO_CHAR );
	ch->pcdata->questpoints += qp_gain;
	logpf( "%s sold %s for %dqp",
	       ch->name, remove_color(obj->short_descr), qp_gain );
	extract_obj(obj);
	return TRUE;
    }
    act( "$p is not a quest item.", ch, obj, ch, TO_CHAR );
    return FALSE;
}

/* Object vnums for object quest 'tokens'. In Moongate, the tokens are
things like 'the Shield of Moongate', 'the Sceptre of Moongate'. These
items are worthless and have the rot-death flag, as they are placed
into the world when a player receives an object quest. */

#define QUEST_OBJQUEST1 10375
#define QUEST_OBJQUEST2 10376
#define QUEST_OBJQUEST3 10377
#define QUEST_OBJQUEST4 10378
#define QUEST_OBJQUEST5 10379

/* Local functions */

void generate_quest args(( CHAR_DATA *ch, CHAR_DATA *questman ));
void quest_update   args(( void ));
bool quest_level_diff   args(( CHAR_DATA *ch, int mlevel));
bool chance     args(( int num ));

/* Added for "hard" quest option -- Astark Feb2012 */
void generate_quest_hard args(( CHAR_DATA *ch, CHAR_DATA *questman ));
bool quest_level_diff_hard   args(( CHAR_DATA *ch, int mlevel));
/* End of hard quests */

int quest_timer;

/* CHANCE function. I use this everywhere in my code, very handy :> */
bool chance(int num)
{
    if (number_range(1,100) <= num)
	return TRUE;
    else 
	return FALSE;
}

bool per_chance(int num)
{
    if (number_range(1,100) <= num)
	return TRUE;
    else 
	return FALSE;
}

// division with randomized rounding
int rand_div(int divident, int divisor)
{
    // need to be careful with signs - C rounding is towards 0
    if ( divisor < 0 )
    {
        divisor = -divisor;
        divident = -divident;
    }
    
    int adjust = number_range(0, divisor-1);
    if ( divident > 0 )
        return (divident + adjust) / divisor;
    else
        return (divident - adjust) / divisor;
}

void show_quest_syntax( CHAR_DATA *ch )
{
    send_to_char("QUEST commands: POINTS INFO TIME REQUEST REQUESTHARD COMPLETE GIVEUP LIST BUY SELL REFUND.\n\r",ch);
    send_to_char("For more information, type 'HELP QUEST'.\n\r",ch);
}

/* The main quest function */
DEF_DO_FUN(do_quest)
{
    CHAR_DATA *questman;
    OBJ_DATA *obj=NULL;
    OBJ_INDEX_DATA *questinfoobj;
    MOB_INDEX_DATA *questinfo;
    ROOM_INDEX_DATA *questinforoom;
    AREA_DATA *questinfoarea;
    char buf [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    
    if (IS_NPC(ch))
    {
        send_to_char( "NPC's cannot quest!\n\r", ch );
        return;
    }
    
    if (arg1[0] == '\0')
    {
        show_quest_syntax(ch);
        return;
    }
    if (!strcmp(arg1, "info"))
    {
        if (IS_SET(ch->act, PLR_QUESTOR) || IS_SET(ch->act, PLR_QUESTORHARD))
        {
            if (ch->pcdata->questmob == -1 && ch->pcdata->questgiver->short_descr != NULL)
            {
                sprintf(buf, "Your quest is ALMOST complete!\n\rGet back to %s before your time runs out!\n\r",ch->pcdata->questgiver->short_descr);
                send_to_char(buf, ch);
            }
            else if (ch->pcdata->questobj > 0)
            {
                questinfoobj = get_obj_index(ch->pcdata->questobj);
		questinforoom = get_room_index(ch->pcdata->questroom);
		questinfoarea = get_area_data(ch->pcdata->questarea);
                if (questinfoobj != NULL && questinforoom != NULL && questinfoarea != NULL)
                {
                    sprintf(buf, "You are on a quest to recover the fabled %s!\n\r"
		    	"Look in the general area of %s for %s!\n\r", questinfoobj->name, 
			questinfoarea->name, questinforoom->name);
                    send_to_char(buf, ch);
                }
                else send_to_char("You aren't currently on a quest.\n\r",ch);
                return;
            }
            else if (ch->pcdata->questmob > 0)
            {
                questinfo = get_mob_index(ch->pcdata->questmob);
		questinforoom = get_room_index(ch->pcdata->questroom);
		questinfoarea = get_area_data(ch->pcdata->questarea);
                if (questinfo != NULL && questinforoom != NULL && questinfoarea != NULL)
                {
                    sprintf(buf, "You are on a quest to slay the dreaded %s!\n\r"
			"Look in the general area of %s for %s!\n\r",
			questinfo->short_descr, questinfoarea->name, questinforoom->name);
                    send_to_char(buf, ch);
                }
                else send_to_char("You aren't currently on a quest.\n\r",ch);
                return;
            }
        }
        else
            send_to_char("You aren't currently on a quest.\n\r",ch);
        return;
    }
    if (!strcmp(arg1, "points"))
    {
        sprintf(buf, "You have %d quest points.\n\r",ch->pcdata->questpoints);
        send_to_char(buf, ch);
        return;
    }
    else if (!strcmp(arg1, "giveup"))
    {

 /* Added the questorhard bitsector to be checked when giving up a quest --Astark Feb 2012 */

        if (!IS_SET(ch->act,PLR_QUESTOR) && !IS_SET(ch->act, PLR_QUESTORHARD))
        {
            send_to_char("You're not on a quest!\n\r",ch);
            return;
        }
        
        act("$n admits defeat and fails $s quest.",ch,NULL,NULL,TO_ROOM);
        send_to_char("You shamefully admit defeat before your questmaster.\n\r",ch);
        send_to_char("You will be given the chance to redeem yourself in 10 minutes.\n\r",ch);
        ch->pcdata->quest_failed++;
		update_lboard( LBOARD_QFAIL, ch, ch->pcdata->quest_failed, 1);

        if (IS_SET(ch->act,PLR_QUESTORHARD))
            ch->pcdata->quest_hard_failed++;

        ch->pcdata->questgiver = NULL;
        ch->pcdata->countdown = 0;
        ch->pcdata->questmob = 0;
        ch->pcdata->questobj = 0;
	ch->pcdata->questroom = 0;
	ch->pcdata->questarea = 0;
        REMOVE_BIT(ch->act, PLR_QUESTOR);
        REMOVE_BIT(ch->act, PLR_QUESTORHARD);
        
        /* Changed to include a define - Maedhros, Feb 7, 2006 */
	/* ch->pcdata->nextquest = 10; */
	ch->pcdata->nextquest = QUEST_NEXTQUEST_MAX;
        return;
    }
    else if (!strcmp(arg1, "time"))
    {
        if (!IS_SET(ch->act, PLR_QUESTOR) && !IS_SET(ch->act, PLR_QUESTORHARD))
        {
            send_to_char("You aren't currently on a quest.\n\r",ch);
            if (ch->pcdata->nextquest > 1)
            {
                sprintf(buf, "There are %d minutes remaining until you can go on another quest.\n\r",ch->pcdata->nextquest);
                send_to_char(buf, ch);
            }
            else if (ch->pcdata->nextquest == 1)
            {
                sprintf(buf, "There is less than a minute remaining until you can go on another quest.\n\r");
                send_to_char(buf, ch);
            }
        }
        else if (ch->pcdata->countdown > 0)
        {
            sprintf(buf, "Time left for current quest: %d\n\r",ch->pcdata->countdown);
            send_to_char(buf, ch);
        }
        return;
    }
    else if ( !strcmp(arg1, "refund") )
    {
        ptc(ch, "You currently get a %d%% refund when selling quest equipment.\n\r", cfg_refund_qeq ? 100 : 90);
        return;
    }
    
    /* Checks for a character in the room with spec_questmaster set. This special
    procedure must be defined in special.c. You could instead use an 
    ACT_QUESTMASTER flag instead of a special procedure. */
    
    for ( questman = ch->in_room->people; questman != NULL; questman = questman->next_in_room )
    {
        if (!IS_NPC(questman)) continue;
        if (questman->spec_fun == spec_questmaster) break;
    }
    
    if (questman == NULL || questman->spec_fun != spec_questmaster)
    {
        send_to_char("You can't do that here.\n\r",ch);
        return;
    }
    
    if ( questman->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        return;
    }
    
    ch->pcdata->questgiver = questman;
    
    /* And, of course, you will need to change the following lines for YOUR
    quest item information. Quest items on Moongate are unbalanced, very
    very nice items, and no one has one yet, because it takes awhile to
    build up quest points :> Make the item worth their while. */
    
    if (!strcmp(arg1, "list"))
    {
        act( "$n asks $N for a list of quest items.", ch, NULL, questman, TO_ROOM); 
        act ("You ask $N for a list of quest items.",ch, NULL, questman, TO_CHAR);
/*        sprintf(buf, "Current Quest Items available for Purchase:\n\r"); */
        sprintf(buf, "{w Cost   Wear Location   Name{x\n\r");
        strcat(buf, "-------------------------------\n\r");
	strcat( buf, list_quest_items() );
/*      strcat(buf, "500qp...........100,000 gold\n\r");  */
        strcat(buf, "  250qp.................50 practices\n\r");
	strcat(buf, "  200qp.................Change name 'color'.\n\r");
	strcat(buf, "  200qp.................Change pretitle (ptitle).\n\r");
        strcat(buf, "  100qp.................Experience (1/4 exp per level)\n\r");
	strcat(buf, "   50qp.................Warfare\n\r");
    strcat(buf, "   10qp.................Duel\n\r");
        strcat(buf, "\n\r");
        strcat(buf, "To buy an item, type 'QUEST BUY <item>'.\n\r");
        strcat(buf, "To see a list of items, type '\t<send href='help questitems'>{whelp questitems{x\t</send>'\n\r");
        send_to_char(buf, ch);
        return;
      }
    
    else if (!strcmp(arg1, "sell"))
    {
        if (arg2[0] == '\0')
        {
            send_to_char("To sell a quest item, type 'QUEST SELL <item>'.\n\r",ch);
            return;
        }
	if ( (obj = get_obj_carry(ch, arg2, ch)) == NULL )
        {
            send_to_char("You don't have that object.\n\r",ch);
            return;
        }
	sell_quest_item(ch, obj, questman);
	return;
      }
    else if (!strcmp(arg1, "buy"))
    {
        if (arg2[0] == '\0')
        {
            send_to_char("To buy an item, type 'QUEST BUY <item>'.\n\r",ch);
            return;
        }

	if ( create_quest_item(ch, arg2, &obj) )
	{
	    if ( obj == NULL )
		return;
	}
        else if (is_name(arg2, "practices pracs prac practice"))
        {
            if (ch->pcdata->questpoints >= 250)
            {
                ch->pcdata->questpoints -= 250;
                ch->practice += 50;
                act( "$N gives 50 practices to $n.", ch, NULL, questman, TO_ROOM );
                act( "$N gives you 50 practices.",   ch, NULL, questman, TO_CHAR );
                logpf("%s bought 50 practices for 250 qp", ch->name);
            }
            else
            {
                sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",ch->name);
                do_say(questman,buf);
            }
            return;
        }
/*      else if (is_name(arg2, "gold gp"))
        {
            if (ch->pcdata->questpoints >= 500)
            {
                ch->pcdata->questpoints -= 500;
                ch->gold += 100000;
                act( "$N gives 100,000 gold pieces to $n.", ch, NULL, questman, TO_ROOM );
                act( "$N has 100,000 in gold transfered from $s Swiss account to your balance.",   ch, NULL, questman, TO_CHAR );
                return;
            }
            else
            {
                sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",ch->name);
                do_say(questman,buf);
                return;
            }
        }*/
        else if (is_name(arg2, "experience xp"))
        {
          if ( IS_SET(ch->act,PLR_NOEXP))
          {
              send_to_char("Toggle 'noexp' to allow you to gain experience before purchasing this.\n\r",ch);
              return;
          }
            if (ch->pcdata->questpoints >= 100)
            {
                ch->pcdata->questpoints -= 100;
               // section of gain_exp reproduced here in order to bypass field exp -Vodur
               int gain = 1+exp_per_level(ch)/4;/* 1+ so whiners don't lose a couple of exp points per level  from rounding :)  -Vodur*/
               ch->exp = UMAX( exp_per_level(ch), ch->exp + gain );
               sprintf(buf, "You earn %d applied experience.\n\r", gain);
               send_to_char(buf,ch);
               logpf("%s bought %d experience for 100 qp", ch->name, gain);

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
               /*end of modified secion from gain_exp*/
               }

		
                act( "$N grants experience to $n.", ch, NULL, questman, TO_ROOM );
                act( "$N grants you experience.",   ch, NULL, questman, TO_CHAR );
            }
            else
            {
                sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",ch->name);
                do_say(questman,buf);
            }
            return;
        }
	else if (is_name(arg2, "warfare"))
	{
	    if (ch->pcdata->questpoints >= 50)
	    proc_startwar(ch, argument, TRUE);
	    else
	    {
		sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",ch->name);
		do_say(questman,buf);
	    }
	    return;
	}
    else if (is_name(arg2, "duel"))
    {
        if (ch->pcdata->questpoints >= 10)
            proc_startduel(ch, argument);
        else
        {
            sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.", ch->name);
            do_say(questman,buf);
        }
        return;
    }
            

		/* Added for Vodurs ptitle and color name 11/25/11 -- Maedhros */

	else if 
	(!strcmp(arg2,"color") )
        {
            if (*argument == '\0')
            {
                do_say(questman, "Use 'quest buy color list' to see your options.");
            }
            else if (!IS_IMMORTAL(ch) &&(ch->pcdata->questpoints < 200)
                && (strcmp(argument,"list") ) )
            {
                sprintf(buf, "Sorry, %s, but you dont have enough quest points for that.",ch->name);
                do_say(questman,buf);
            }
            else
                color_name(ch,argument,ch);
            return;
        }
        else if (!strcmp(arg2, "ptitle"))
	{
        quest_buy_ptitle( ch, argument);
        return; 
	}		 
		 /* End of Vodurs ptitle */

        else
        {
            sprintf(buf, "I don't have that item, %s.",ch->name);
            do_say(questman, buf);
        }
        if (obj != NULL)
        {
            act( "$N gives $p to $n.", ch, obj, questman, TO_ROOM );
            act( "$N gives you $p.",   ch, obj, questman, TO_CHAR );
            free_string(obj->owner);
            obj->owner = str_dup(ch->name);
            SET_BIT(obj->extra_flags, ITEM_STICKY);
            obj_to_char(obj, ch);
        }
        return;
      }
    else if (!strcmp(arg1, "request"))
    {
	if (ch->position < POS_RESTING)
	{
	    send_to_char("In your dreams, or what?\n\r",ch);
            return;
	}
		
        act( "$n asks $N for a quest.", ch, NULL, questman, TO_ROOM); 
        act ("You ask $N for a quest.",ch, NULL, questman, TO_CHAR);
        if (IS_SET(ch->act, PLR_QUESTOR) || IS_SET(ch->act, PLR_QUESTORHARD))
        {
            sprintf(buf, "But you're already on a quest!");
            do_say(questman, buf);
            return;
        }
        if (ch->pcdata->nextquest > 0 && !IS_IMMORTAL(ch))
        {
            sprintf(buf, "You're very brave, %s, but let someone else have a chance.",ch->name);
            do_say(questman, buf);
            sprintf(buf, "Come back later.");
            do_say(questman, buf);
            return;
        }
        
        sprintf(buf, "Thank you, brave %s!",ch->name);
        do_say(questman, buf);
        ch->pcdata->questmob = 0;
        ch->pcdata->questobj = 0;
	ch->pcdata->questroom = 0;
	ch->pcdata->questarea = 0;
        /* How about ch->pcdata->nextquest = 0;, for Immortals? - Maedhros */ 

        generate_quest(ch, questman);
        
        if (ch->pcdata->questmob > 0 || ch->pcdata->questobj > 0)
        {
	    /* This line was removed (commented out), and similar lines moved to the bottom
	     * of each of the mob and item quest creation sections in generate_quest(), so
	     * that the quest item's timer could be based on the countdown timer, and scaled
	     * so that the item barely outlasts the countdown timer - Maedhros, Feb 7, 2006.
	     *
	     *  ch->pcdata->countdown = number_range(15,30);
	     */

	    SET_BIT(ch->act, PLR_QUESTOR);
            sprintf(buf, "You have %d minutes to complete this quest.",ch->pcdata->countdown);
            do_say(questman, buf);
            sprintf(buf, "May the gods go with you!");
            do_say(questman, buf);
        }
        return;
      }
/* Used for requesting difficult quests. The code here is nearly
   the same as up above but a separate function is used. Could be
   solved I think by adding another argument to the quest request
   command but this was the easiest solution for me. 
   -- Astark Feb 2012 */

    else if (!strcmp(arg1, "requesthard"))
    {
      if (ch->position >= POS_RESTING)
      {
        act( "$n asks $N for a quest.", ch, NULL, questman, TO_ROOM); 
        act ("You ask $N for a quest.",ch, NULL, questman, TO_CHAR);
        if (IS_SET(ch->act, PLR_QUESTOR) || IS_SET(ch->act, PLR_QUESTORHARD))
        {
            sprintf(buf, "But you're already on a quest!");
            do_say(questman, buf);
            return;
        }
        if (ch->pcdata->nextquest > 0 && !IS_IMMORTAL(ch))
        {
            sprintf(buf, "You're very brave, %s, but let someone else have a chance.",ch->name);
            do_say(questman, buf);
            sprintf(buf, "Come back later.");
            do_say(questman, buf);
            return;
        }
        
        sprintf(buf, "Thank you, brave %s!",ch->name);
        do_say(questman, buf);
        ch->pcdata->questmob = 0;
        ch->pcdata->questobj = 0;
	ch->pcdata->questroom = 0;
	ch->pcdata->questarea = 0;
        
        generate_quest_hard(ch, questman);
        
        if (ch->pcdata->questmob > 0 || ch->pcdata->questobj > 0)
        {

 /* Sets a different bit (questorhard) to differentiate between
    easy and hard quests. -- Astark feb 2012 */

            SET_BIT(ch->act, PLR_QUESTORHARD);
            sprintf(buf, "You have %d minutes to complete this quest.",ch->pcdata->countdown);
            do_say(questman, buf);
            sprintf(buf, "May the gods go with you!");
            do_say(questman, buf);
        }
        return;
      }
      else
      {
          send_to_char("In your dreams, or what?\n\r",ch);
          return;
      }
    }
    else if (!strcmp(arg1, "complete"))
    {
        act( "$n informs $N $e has completed $s quest.", ch, NULL, questman, TO_ROOM); 
        act ("You inform $N you have completed $s quest.",ch, NULL, questman, TO_CHAR);
        
        if ( !IS_SET(ch->act, PLR_QUESTOR) && !IS_SET(ch->act, PLR_QUESTORHARD) )
        {
            sprintf(buf, "What? You aren't on any quest! Get lost!");
            do_say(questman, buf);
            return;
        }

        int reward_silver = 0, reward_points = 0, reward_prac = 0, reward_exp = 0;
        int luck = ch_luc_quest(ch);
        OBJ_DATA *quest_obj = get_char_obj_vnum(ch, ch->pcdata->questobj);
        int prac_chance = IS_SET(ch->act, PLR_QUESTORHARD) ? 20 : 15;

        // reward_points
        int reward_points_min = 0, reward_points_max = 0;
        if ( IS_SET(ch->act, PLR_QUESTORHARD) )
        {
            /* The modulo functions below are modeled after the fractional 'practices' function.
               With this change, you are no longer required to hit a breakpoint before having a
               chance to earn extra QP */
            reward_points_min = get_curr_stat(ch, STAT_CHA) / 12;
            reward_points_min += chance((get_curr_stat(ch, STAT_CHA) % 12)*100/12) ? 1 : 0;

            reward_points_max = 20 + luck;
            reward_points_max += chance((get_curr_stat(ch, STAT_LUC) % 6)*100/6) ? 1 : 0;
        }
        else
        {
            reward_points_min = get_curr_stat(ch, STAT_CHA) / 15;
            reward_points_min += chance((get_curr_stat(ch, STAT_CHA) % 15)*100/15) ? 1 : 0; 

            reward_points_max = 10 + luck;
            reward_points_max += chance((get_curr_stat(ch, STAT_LUC) % 6)*100/6) ? 1 : 0;
        }
        reward_points = number_range(reward_points_min, reward_points_max);
        
        if ( IS_AFFECTED(ch, AFF_FORTUNE) )
        {
            prac_chance += 5;
            int second_roll = number_range(reward_points_min, reward_points_max);
            if ( second_roll > reward_points )
            {
                ptc(ch, "{YFortuna smiles on you.{x\n\r");
                reward_points = second_roll;
            }
        }
        
        // kill mob quest (completed)
        if ( ch->pcdata->questmob == -1 )
        {
            if ( IS_SET(ch->act, PLR_QUESTORHARD) )
            {
                reward_silver = number_range( 15*ch->level, 50*ch->level*luck );
                if ( per_chance(prac_chance) )
                    reward_prac = 3 + number_range(1, luck/2);
                reward_exp = number_range(50, 100+luck);
                ch->pcdata->quest_hard_success++;
            }
            else
            {
                reward_silver = number_range( 1, 12*ch->level*luck );
                if ( per_chance(prac_chance) )
                    reward_prac = number_range(1, luck/2);
                reward_exp = number_range(10, 20+luck);
            }
        }
        // collect X of ages quest (completed)
        else if ( ch->pcdata->questobj > 0 && quest_obj != NULL )
        {
            act("You hand $p to $N.", ch, quest_obj, questman, TO_CHAR);
            act("$n hands $p to $N.", ch, quest_obj, questman, TO_ROOM);
            extract_obj(quest_obj);
            
            if ( IS_SET(ch->act, PLR_QUESTORHARD) )
            {
                reward_silver = number_range( ch->level, 30*ch->level*luck );
                if ( per_chance(prac_chance) )
                    reward_prac = 3 + number_range(1, luck/2);
                reward_exp = number_range(10, 20+luck);
                ch->pcdata->quest_hard_success++;
            }
            else
            {
                reward_silver = number_range( 1, 12*ch->level*luck );
                if ( per_chance(prac_chance) )
                    reward_prac = number_range(1, luck/2);
                reward_exp = number_range(10, 20+luck);
            }
        }
        else
        {
            sprintf(buf, "You haven't completed the quest yet, but there is still time!");
            do_say(questman, buf);
            return;
        }
        
        sprintf(buf, "Congratulations on completing your quest!");
        do_say(questman, buf);
        
        // general adjustments
        //reward_points += reward_points * get_religion_bonus(ch) / 100;
        if ( cfg_enable_qp_mult )
        {
            reward_points = (int)(reward_points * cfg_qp_mult );
            if ( cfg_show_qp_mult )
            {
                sprintf(buf, "There's currently a qp bonus of %d%%!", (int)((cfg_qp_mult*100)-99.5));
                do_say(questman, buf );
            }

        }

        // notify of rewards
        sprintf(buf,"As a reward, I am giving you %d quest points, and %d silver.", reward_points, reward_silver);
        do_say(questman, buf);
        if ( reward_prac > 0 )
            printf_to_char(ch, "You gain %d practice%s!\n\r", reward_prac, reward_prac > 1 ? "s" : "" );

        // hand out rewards
        add_money_mixed(ch, reward_silver, questman);
        ch->pcdata->questpoints += reward_points;
        logpf("quest complete: awarded %d qp to %s.", reward_points, ch->name);
        ch->practice += reward_prac;
        gain_exp(ch, reward_exp, TRUE);
        
        // cleanup
        REMOVE_BIT(ch->act, PLR_QUESTOR);
        REMOVE_BIT(ch->act, PLR_QUESTORHARD);
        ch->pcdata->questgiver = NULL;
        ch->pcdata->countdown = 0;
        ch->pcdata->questmob = 0;
        ch->pcdata->questobj = 0;
        ch->pcdata->questroom = 0;
        ch->pcdata->questarea = 0;
        ch->pcdata->nextquest = QUEST_NEXTQUEST_MAX;

        // tracking
        ch->pcdata->quest_success++;
        check_achievement(ch);
        update_lboard( LBOARD_QPNT, ch, ch->pcdata->questpoints, reward_points );
        update_lboard( LBOARD_QCOMP, ch, ch->pcdata->quest_success, 1);
        
        return;
    }
    else
        show_quest_syntax(ch);
}

bool is_guild_room( int vnum )
{
    int iClass, iGuild;

    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
	for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)    
	    if ( vnum == class_table[iClass].guild[iGuild] )
		return TRUE;

    return FALSE;
}

void generate_quest(CHAR_DATA *ch, CHAR_DATA *questman)
{
    CHAR_DATA *victim;
    MOB_INDEX_DATA *vsearch;
    ROOM_INDEX_DATA *room;
    OBJ_DATA *questitem;
    char buf [MAX_STRING_LENGTH];
    long mcounter;
    int mob_vnum;
    bool found = FALSE;
 
    if ( ch == NULL || IS_NPC(ch) )
	return;

    /*  Randomly selects a mob from the world mob list. If you don't
    want a mob to be selected, make sure it is immune to summon.
    Or, you could add a new mob flag called ACT_NOQUEST. The mob
    is selected for both mob and obj quests, even tho in the obj
    quest the mob is not used. This is done to assure the level
    of difficulty for the area isn't too great for the player. */
    
    for (mcounter = 0; mcounter < 10000; mcounter ++)
    {
        mob_vnum = number_range(50, 32600);
        
        if ( (vsearch = get_mob_index(mob_vnum) ) != NULL )
        {
            if ( !quest_level_diff(ch, vsearch->level)
		 || vsearch->pShop != NULL
		 /* || IS_SET(vsearch->imm_flags, IMM_SUMMON) */
		 || IS_SET(vsearch->act, ACT_TRAIN)
                 || IS_SET(vsearch->act, ACT_HARD_QUEST)
		 || IS_SET(vsearch->act, ACT_PRACTICE)
		 || IS_SET(vsearch->act, ACT_IS_HEALER)
		 || IS_SET(vsearch->act, ACT_IS_CHANGER)
		 || IS_SET(vsearch->act, ACT_PET)
		 || IS_SET(vsearch->act, ACT_SAFE)
		 || IS_SET(vsearch->act, ACT_WIZI)
		 || IS_SET(vsearch->act, ACT_NO_QUEST)
		 || IS_SET(vsearch->affect_field, AFF_CHARM)
		 || IS_SET(vsearch->affect_field, AFF_INVISIBLE)
		 || IS_SET(vsearch->affect_field, AFF_ASTRAL)
		 || IS_SET(vsearch->area->area_flags, AREA_REMORT)
		 || IS_SET(vsearch->area->area_flags, AREA_NOQUEST)
         || area_full(vsearch->area)
		 || (IS_SET(vsearch->imm_flags, IMM_WEAPON)
		     && IS_SET(vsearch->imm_flags, IMM_MAGIC))
		 || chance(60))
            {
                vsearch = NULL;
                continue;
            }
            
            if (( victim = get_mob_vnum_world( mob_vnum ) ) == NULL
                || ( room = victim->in_room ) == NULL
                || (IS_GOOD(ch) && IS_GOOD(victim))
                || !is_room_ingame(victim->in_room)
		|| is_guild_room( room->vnum )
                || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
                || IS_SET(victim->in_room->room_flags, ROOM_JAIL)
                || IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
                || IS_SET(victim->in_room->room_flags, ROOM_NO_QUEST)
                || IS_SET(victim->in_room->room_flags, ROOM_GODS_ONLY)
                || IS_SET(victim->in_room->room_flags, ROOM_IMP_ONLY)
                || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
                || (ch->level < LEVEL_HERO && 
                    IS_SET(victim->in_room->room_flags, ROOM_HEROES_ONLY))
                || (ch->level > 5 && 
                    IS_SET(victim->in_room->room_flags, ROOM_NEWBIES_ONLY)) )
                continue;
            
	    found = TRUE;
            break;
        }
    }
    
    if ( !found || chance(20-ch_luc_quest(ch)) )
    {
        sprintf(buf, "I'm sorry, I don't have a good quest for you right now.");
        do_say(questman, buf);
        sprintf(buf, "Please come back in a few minutes, maybe something will come up.");
        do_say(questman, buf);
        
        /* Changed to include a define - Maedhros, Feb 7, 2006 */
	/* ch->pcdata->nextquest = 2; */
	ch->pcdata->nextquest = QUEST_NEXTQUEST_MIN;
        return;
    }
    
    /*  40% chance it will send the player on a 'recover item' quest. */
    
    if (chance(40) || (ch->level < 10 && !IS_SET(victim->in_room->room_flags, ROOM_NEWBIES_ONLY)) )
    {
        int objvnum = 0;
        
        switch(number_range(0,4))
        {
        case 0:
            objvnum = QUEST_OBJQUEST1;
            break;
            
        case 1:
            objvnum = QUEST_OBJQUEST2;
            break;
            
        case 2:
            objvnum = QUEST_OBJQUEST3;
            break;
            
        case 3:
            objvnum = QUEST_OBJQUEST4;
            break;
            
        case 4:
            objvnum = QUEST_OBJQUEST5;
            break;
        }
        
        questitem = create_object_vnum(objvnum);
    
        /* Removed (commented out) the following line, and replaced it with the next two,
	 * one of which was previously outside of generate_quest(), so that the quest item
	 * timer could be based on countdown, and also scaled, since countdown runs more
	 * slowly than the quest item timer. - Maedhros, Feb 7, 2006.
	 *
         * questitem->timer=30;
         */ 
        ch->pcdata->countdown = number_range(QUEST_COUNTDOWN_MIN, QUEST_COUNTDOWN_MAX);	
	/* ch->pcdata->countdown = number_range(15, 30); */ 
	questitem->timer = (ch->pcdata->countdown * 2) + 1;

        obj_to_room(questitem, room);
        ch->pcdata->questobj = questitem->pIndexData->vnum;
        
        sprintf(buf, "Vile pilferers have stolen %s from the royal treasury!",questitem->short_descr);
        do_say(questman, buf);
        do_say(questman, "My court wizardess, with her magic mirror, has pinpointed its location.");
        
        /* I changed my area names so that they have just the name of the area
        and none of the level stuff. You may want to comment these next two
        lines. - Vassago */
        
        sprintf(buf, "Look in the general area of %s for %s!",room->area->name, room->name);
	ch->pcdata->questroom = room->vnum;
	ch->pcdata->questarea = room->area->vnum;
        do_say(questman, buf);
        return;
    }
    
    /* Quest to kill a mob */
    
    else 
    {
        switch(number_range(0,1))
        {
        case 0:
            sprintf(buf, "An enemy of mine, %s, is making vile threats against the crown.",victim->short_descr);
            do_say(questman, buf);
            sprintf(buf, "This threat must be eliminated!");
            do_say(questman, buf);
            break;
            
        case 1:
            sprintf(buf, "Aarchon's most heinous criminal, %s, has escaped from the dungeon!",victim->short_descr);
            do_say(questman, buf);
            /* Typo fix: "civillians" to "civilians" - Maedhros, Jan 16, 2006 */ 
	    sprintf(buf, "Since the escape, %s has murdered %d civilians!",victim->short_descr, number_range(2,20));
            do_say(questman, buf);
            do_say(questman,"The penalty for this crime is death, and you are to deliver the sentence!");
            break;
        }
        
        if (room->name != NULL)
        {
            sprintf(buf, "Seek %s out somewhere in the vicinity of %s!",victim->short_descr,room->name);
            do_say(questman, buf);
            
            /* I changed my area names so that they have just the name of the area
            and none of the level stuff. You may want to comment these next two
            lines. - Vassago */
            
            sprintf(buf, "That location is in the general area of %s.",room->area->name);
	    ch->pcdata->questroom = room->vnum;
	    ch->pcdata->questarea = room->area->vnum;
            do_say(questman, buf);
        }
        ch->pcdata->questmob = victim->pIndexData->vnum;

        /* Added this line.  It replaces an identical line that was just outside of the call
	 * to generate_quest().  This was done so that the quest item timer could be based on
	 * the countdown timer, easily. - Maedhros, Feb 7, 2006
	 */
        ch->pcdata->countdown = number_range(QUEST_COUNTDOWN_MIN, QUEST_COUNTDOWN_MAX); 	
        /* ch->pcdata->countdown = number_range(15, 30); */

    }
    return;
}

/* This gives players much more difficult quests than they'd normally
receive. They also will be rewarded appropriately due to the QUESTORHARD
flag they are assigned. Allows for invisible mobs and for mobs with the
flag "ACT hard_quest", even if they're in a no_quest room/area  -- Astark Feb2012 */

void generate_quest_hard(CHAR_DATA *ch, CHAR_DATA *questman)
{
    CHAR_DATA *victim;
    MOB_INDEX_DATA *vsearch;
    ROOM_INDEX_DATA *room;
    OBJ_DATA *questitem;
    char buf [MAX_STRING_LENGTH];
    long mcounter;
    int mob_vnum;
    bool found = FALSE;
    
    if ( ch == NULL || IS_NPC(ch) )
	return;

    /*  Randomly selects a mob from the world mob list. If you don't
    want a mob to be selected, make sure it is immune to summon.
    Or, you could add a new mob flag called ACT_NOQUEST. The mob
    is selected for both mob and obj quests, even tho in the obj
    quest the mob is not used. This is done to assure the level
    of difficulty for the area isn't too great for the player. */
    
    for (mcounter = 0; mcounter < 10000; mcounter ++)
    {
        mob_vnum = number_range(50, 32600);
        
        if ( (vsearch = get_mob_index(mob_vnum) ) != NULL )
        {
            if ( !quest_level_diff_hard(ch, vsearch->level)
		 || vsearch->pShop != NULL
		 /* || IS_SET(vsearch->imm_flags, IMM_SUMMON) */
		 || IS_SET(vsearch->act, ACT_TRAIN)
		 || IS_SET(vsearch->act, ACT_PRACTICE)
		 || IS_SET(vsearch->act, ACT_IS_HEALER)
		 || IS_SET(vsearch->act, ACT_IS_CHANGER)
		 || IS_SET(vsearch->act, ACT_PET)
		 || IS_SET(vsearch->act, ACT_SAFE)
		 || IS_SET(vsearch->act, ACT_WIZI)
		 || IS_SET(vsearch->act, ACT_NO_QUEST)
		 || IS_SET(vsearch->affect_field, AFF_CHARM)
		 || IS_SET(vsearch->affect_field, AFF_ASTRAL)
		 || IS_SET(vsearch->area->area_flags, AREA_REMORT)
		 || (IS_SET(vsearch->area->area_flags, AREA_NOQUEST) && !IS_SET(vsearch->act, ACT_HARD_QUEST))
         || area_full(vsearch->area)
		 || (IS_SET(vsearch->imm_flags, IMM_WEAPON)
		     && IS_SET(vsearch->imm_flags, IMM_MAGIC))
		 || chance(60))
            {
                vsearch = NULL;
                continue;
            }
            
            if (( victim = get_mob_vnum_world( mob_vnum ) ) == NULL
                || ( room = victim->in_room ) == NULL
                || (IS_GOOD(ch) && IS_GOOD(victim))
                || !is_room_ingame(victim->in_room)
		|| is_guild_room( room->vnum )
                || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
                || IS_SET(victim->in_room->room_flags, ROOM_JAIL)
                || IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
                || (IS_SET(victim->in_room->room_flags, ROOM_NO_QUEST) && !IS_SET(victim->act, ACT_HARD_QUEST))
                || IS_SET(victim->in_room->room_flags, ROOM_GODS_ONLY)
                || IS_SET(victim->in_room->room_flags, ROOM_IMP_ONLY)
                || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
                || (ch->level < LEVEL_HERO && 
                    IS_SET(victim->in_room->room_flags, ROOM_HEROES_ONLY))
                || (ch->level > 5 && 
                    IS_SET(victim->in_room->room_flags, ROOM_NEWBIES_ONLY)) )
                continue;
            
	    found = TRUE;
            break;
        }
    }
    
    if ( !found || chance(20-ch_luc_quest(ch)) )
    {
        sprintf(buf, "I'm sorry, I don't have a good quest for you right now.");
        do_say(questman, buf);
        sprintf(buf, "Please come back in a few minutes, maybe something will come up.");
        do_say(questman, buf);

        /* Changed to include a define - Maedhros, Feb 7, 2006 */
	/* ch->pcdata->nextquest = 2; */

	ch->pcdata->nextquest = QUEST_NEXTQUEST_MIN;
        return;
    }
    
    /*  30% chance it will send the player on a 'recover item' quest. */
    
    if (chance(30) || (ch->level < 10 && !IS_SET(victim->in_room->room_flags, ROOM_NEWBIES_ONLY)) )
    {
        int objvnum = 0;
        
        switch(number_range(0,4))
        {
        case 0:
            objvnum = QUEST_OBJQUEST1;
            break;
            
        case 1:
            objvnum = QUEST_OBJQUEST2;
            break;
            
        case 2:
            objvnum = QUEST_OBJQUEST3;
            break;
            
        case 3:
            objvnum = QUEST_OBJQUEST4;
            break;
            
        case 4:
            objvnum = QUEST_OBJQUEST5;
            break;
        }
        
        questitem = create_object_vnum(objvnum);
    
        /* Removed (commented out) the following line, and replaced it with the next two,
	 * one of which was previously outside of generate_quest(), so that the quest item
	 * timer could be based on countdown, and also scaled, since countdown runs more
	 * slowly than the quest item timer. - Maedhros, Feb 7, 2006.
	 *
         * questitem->timer=30;
         */ 

        ch->pcdata->countdown = number_range(QUEST_COUNTDOWN_MIN, QUEST_COUNTDOWN_MAX);	

 	/* ch->pcdata->countdown = number_range(15, 30); */ 

	questitem->timer = (ch->pcdata->countdown * 2) + 1;

        obj_to_room(questitem, room);
        ch->pcdata->questobj = questitem->pIndexData->vnum;
        
        sprintf(buf, "Vile pilferers have stolen %s from the royal treasury!",questitem->short_descr);
        do_say(questman, buf);
        do_say(questman, "My court wizardess, with her magic mirror, has pinpointed its location.");
        
        /* I changed my area names so that they have just the name of the area
        and none of the level stuff. You may want to comment these next two
        lines. - Vassago */
        
        sprintf(buf, "Look in the general area of %s for %s!",room->area->name, room->name);
	ch->pcdata->questroom = room->vnum;
	ch->pcdata->questarea = room->area->vnum;
        do_say(questman, buf);
        return;
    }
    
    /* Quest to kill a mob */
    
    else 
    {
        switch(number_range(0,1))
        {
        case 0:
            sprintf(buf, "An enemy of mine, %s, is making vile threats against the crown.",victim->short_descr);
            do_say(questman, buf);
            sprintf(buf, "This threat must be eliminated!");
            do_say(questman, buf);
            break;
            
        case 1:
            sprintf(buf, "Aarchon's most heinous criminal, %s, has escaped from the dungeon!",victim->short_descr);
            do_say(questman, buf);
            sprintf(buf, "Since the escape, %s has murdered %d civillians!",victim->short_descr, number_range(2,20));
            do_say(questman, buf);
            do_say(questman,"The penalty for this crime is death, and you are to deliver the sentence!");
            break;
        }
        
        if (room->name != NULL)
        {
            sprintf(buf, "Seek %s out somewhere in the vicinity of %s!",victim->short_descr,room->name);
            do_say(questman, buf);
            
            /* I changed my area names so that they have just the name of the area
            and none of the level stuff. You may want to comment these next two
            lines. - Vassago */
            
            sprintf(buf, "That location is in the general area of %s.",room->area->name);
	    ch->pcdata->questroom = room->vnum;
	    ch->pcdata->questarea = room->area->vnum;
            do_say(questman, buf);
        }
        ch->pcdata->questmob = victim->pIndexData->vnum;
        ch->pcdata->countdown = number_range(QUEST_COUNTDOWN_MIN, QUEST_COUNTDOWN_MAX); 
    }
    return;
}
/* Level differences to search for. Moongate has 350
   levels, so you will want to tweak these greater or
   less than statements for yourself. - Vassago */

bool quest_level_diff( CHAR_DATA *ch, int mlevel)
{
    int clevel = ch->level + 2 * ch->pcdata->remorts;
    int min_level = URANGE( 1, clevel - 20, 90 );

    return IS_BETWEEN( min_level, mlevel, min_level + 30 );
}

bool quest_level_diff_hard( CHAR_DATA *ch, int mlevel)
{
    // mobs for hard quests are higher level by 10 + 33%
    // so e.g. a range of 90-120 goes up to 130-170
    return quest_level_diff(ch, (mlevel - 10) * 3/4);
}


/* Called from update_handler() by pulse_area */

void quest_update(void)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;
    
    quest_timer++;
     
    if (quest_timer%2) return;

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if (d->character != NULL && !IS_NPC(d->character)
            && (IS_PLAYING(d->connected)))
        {
            
            ch = d->character;
            
            if (ch->pcdata->nextquest > 0)
            {
                ch->pcdata->nextquest--;
                if (ch->pcdata->nextquest == 0)
                    send_to_char("You may now quest again.\n\r",ch);
            }
            else if (IS_SET(ch->act,PLR_QUESTOR) || IS_SET(ch->act, PLR_QUESTORHARD))
            {
                if (--ch->pcdata->countdown <= 0)
                {
                    char buf [MAX_STRING_LENGTH];
                   
		    /* Changed to include a define - Maedhros, Feb 7, 2006 */
		    /* ch->pcdata->nextquest = 10; */
                    ch->pcdata->nextquest = QUEST_NEXTQUEST_MAX;
                    sprintf(buf, "You have run out of time for your quest!\n\rYou may quest again in %d minutes.\n\r",ch->pcdata->nextquest);
                    send_to_char(buf, ch);
                    REMOVE_BIT(ch->act, PLR_QUESTOR);
                    REMOVE_BIT(ch->act, PLR_QUESTORHARD);
                    ch->pcdata->quest_failed++;
					update_lboard( LBOARD_QFAIL, ch, ch->pcdata->quest_failed, 1);
                    ch->pcdata->questgiver = NULL;
                    ch->pcdata->countdown = 0;
                    ch->pcdata->questmob = 0;
                }
                else if (ch->pcdata->countdown < 6)
                    send_to_char("Better hurry, you're almost out of time for your quest!\n\r",ch);
            }
        }
    }
    return;
}

/* checks for quest completed by the killing of victim by ch */
void check_kill_quest_completed( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA *gch;

    if ( !IS_NPC(victim) )
	return;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( IS_NPC(gch) )
            continue;

	if ( (IS_SET(gch->act, PLR_QUESTOR) || IS_SET(gch->act, PLR_QUESTORHARD))
	     && gch->pcdata->questmob == victim->pIndexData->vnum )
	{
	    send_to_char("You have almost completed your QUEST!\n\r",gch);
	    send_to_char("Return to the questmaster before your time runs out!\n\r",gch);
	    gch->pcdata->questmob = -1;
	}
    }    
}

/* 
 * mprog controlled quest code
 * by Henning Koehler <koehlerh@in.tum.de>
 */

#define QUEST_STATUS_NONE (0)
#define QSET_TIMER_NONE   (1) 
/* returns the quest status of a character for the quest identified by id
 * or QUEST_STATUS_NONE if no quest status found
 */
int quest_status( CHAR_DATA *ch, int id )
{
    QUEST_DATA *qdata;

    if ( ch == NULL )
    {
	bug( "quest_status: NULL character given for quest %d", id );
	return QUEST_STATUS_NONE;
    }

    if ( IS_NPC(ch) || ch->pcdata == NULL )
	return QUEST_STATUS_NONE;
    
    for ( qdata = ch->pcdata->qdata; qdata != NULL; qdata = qdata->next )
	if ( qdata->id == id )
	    return qdata->status;

    return QUEST_STATUS_NONE;
}

/* qset timer -- this ignores the qSTATUS but checks qTIMER based on the
 * ID that was provided. If qset timer is 0, then it returns QSET_TIMER_NONE
 */
int qset_timer( CHAR_DATA *ch, int id )
{
    QUEST_DATA *qdata;

    if ( ch == NULL )
    {
	bug( "qset_timer: NULL character given for quest %d", id );
	return 0;
    }

    if ( IS_NPC(ch) || ch->pcdata == NULL )
	return 0;
    
    for ( qdata = ch->pcdata->qdata; qdata != NULL; qdata = qdata->next )
	if ( qdata->id == id )
	    return qdata->timer;

    return 0;
}


/* clear all quests in range
 * doesn't call set_quest_status to be faster
 */
void clear_quests( CHAR_DATA *ch, int min_id, int max_id )
{
    QUEST_DATA *qdata, *last;

    if ( ch == NULL )
    {
	bug( "clear_quests: NULL character given", 0 );
	return;
    }

    if ( ch->pcdata == NULL || ch->pcdata->qdata == NULL )
	return;

    /* check quests that come after first */
    last = ch->pcdata->qdata;
    qdata = last->next;
    while ( qdata != NULL )
	if ( IS_BETWEEN(min_id, qdata->id, max_id) )
	{
	    last->next = qdata->next;
	    free_quest( qdata );
	    qdata = last->next;
	}
	else
	{
	    last = qdata;
	    qdata = last->next;
	}

    /* check first quest */
    qdata = ch->pcdata->qdata;
    if ( IS_BETWEEN(min_id, qdata->id, max_id) )
    {
	ch->pcdata->qdata = qdata->next;
	free_quest( qdata );
    }
}

/* clears all quests with id in vnum range of area */
void clear_area_quests( CHAR_DATA *ch, AREA_DATA *area )
{
    if ( area == NULL )
    {
	bug( "clear_area_quests: NULL area", 0 );
	return;
    }
    clear_quests( ch, area->min_vnum, area->max_vnum );    
}

void show_quests( CHAR_DATA *ch, CHAR_DATA *to_ch )
{
    char buf[MSL];
    QUEST_DATA *qdata;

    if ( ch == NULL || to_ch == NULL )
    {
	bug( "show_quests: NULL pointer", 0 );
	return;
    }
    
    if ( ch->pcdata == NULL || ch->pcdata->qdata == NULL )
    {
	send_to_char( "No Quests.\n\r", to_ch );
	return;
    }

    send_to_char( "Quest     Status  Timer\n\r", to_ch );
    send_to_char( "=======================\n\r", to_ch );
    
    for ( qdata = ch->pcdata->qdata; qdata != NULL; qdata = qdata->next )
    {
	sprintf( buf, "%-10d%-9d%d\n\r", qdata->id, qdata->status, qdata->timer);
	send_to_char( buf, to_ch );
    }
}

void show_luavals( CHAR_DATA *ch, CHAR_DATA *to_ch )
{
    LUA_EXTRA_VAL *luaval;

    if ( ch == NULL || to_ch == NULL )
    {
        bugf( "show_luavals: NULL pointer" );
        return;
    }

    if ( !ch->luavals )
    {
        send_to_char( "No luavals.\n\r", to_ch );
        return;
    }

    ptc( to_ch, "%-20s %-20s %s\n\r", "Name", "Value", "Persist" );
    send_to_char( "=================================================\n\r", to_ch );

    for ( luaval = ch->luavals ; luaval ; luaval = luaval->next )
    {
        ptc( to_ch, "%-20s %-20s %s\n\r",
                luaval->name,
                luaval->val,
                luaval->persist ? "TRUE" : "FALSE" );
    }
    return;
}

bool color_name( CHAR_DATA *ch, const char *argument, CHAR_DATA *victim )
{
  char arg2 [MAX_INPUT_LENGTH];
  char buf [3] = "";

  argument = one_argument( argument, arg2);

  if (victim == NULL)
    victim=ch;

  if ( !strcmp(arg2, "" ))
  {
    send_to_char("Available colors: {rred{x, {yyellow{x, {ggreen{x, {Bhi-blue{x, {mmagenta{x, {wwhite{x, {ccyan{x and {Dgrey{x.  \n\r'clear' argument removes current color at no cost. \n\r",ch);
    return FALSE;
  }

  if ( IS_NPC(victim) )
  {
    return FALSE;
  }

  if (!strcmp(arg2, "clear"))
  {
    free_string(victim->pcdata->name_color);
    victim->pcdata->name_color = str_dup("");
    send_to_char("Name color cleared.\n\r",ch);
    return TRUE;
  }

  if (!strcmp(arg2, "red"))
  {
    strcpy(buf,"{r");
  }

  if (!strcmp(arg2, "magenta"))
  {
    strcpy(buf,"{m");
  }

  if (!strcmp(arg2, "white"))
  {
    strcpy(buf,"{w");
  }

  if (!strcmp(arg2, "hi-blue"))
  {
    strcpy(buf,"{B");
  }
  
  if (!strcmp(arg2, "green"))
  {
    strcpy(buf,"{g");
  }

  if (!strcmp(arg2, "yellow"))
  {
    strcpy(buf, "{y");
  }

  if (!strcmp(arg2, "cyan"))
  {
    strcpy(buf,"{c");
  }

  if (!strcmp(arg2, "grey"))
  {
    strcpy(buf, "{D");
  }

  if (!strcmp(arg2, "gray"))
  {
    strcpy(buf, "{D");
  }

  if (!strcmp(buf, ""))
  {
	  send_to_char("Available colors: {rred{x, {yyellow{x, {ggreen{x, {Bhi-blue{x, {mmagenta{x, {wwhite{x, {ccyan{x and {Dgrey{x.  \n\r'clear' argument removes current color at no cost. \n\r",ch);
	 
    return FALSE;
  }

  if (!strcmp(victim->pcdata->name_color,buf))
  {
    send_to_char("That color is already set!\n\r",ch);
    return FALSE;
  }

  free_string( victim->pcdata->name_color );
  victim->pcdata->name_color = str_dup(buf);
  send_to_char("Name color changed.\n\r",ch);
  
  if (!IS_IMMORTAL(ch))
  {
    victim->pcdata->questpoints -= 200;
  }

  return TRUE;
}


void set_quest_status( CHAR_DATA *ch, int id, int status, int timer, int limit )
{
    QUEST_DATA *qdata, *last;

    if ( ch == NULL )
    {
	bug( "set_quest_status: NULL character given for quest %d", id );
	return;
    }

    if ( IS_NPC(ch) || ch->pcdata == NULL )
	return;

    /* remove quest data if found */
    if ( status == QUEST_STATUS_NONE )
    {
	qdata = ch->pcdata->qdata;
	if ( qdata == NULL )
	    return;
	if ( qdata->id == id )
	{
	    ch->pcdata->qdata = qdata->next;
	    free_quest( qdata );
	    return;
	}
	/* search for it */
	last = qdata;
	for ( qdata = qdata->next; qdata != NULL; last = qdata, qdata = qdata->next )
	    if ( qdata->id == id )
	    {
		last->next = qdata->next;
		free_quest( qdata );
		return;
	    }
	return;
    }

    /* add quest data if not found */
    for ( qdata = ch->pcdata->qdata; qdata != NULL; qdata = qdata->next )
	if ( qdata->id == id )
	{
	    qdata->status = status;
            qdata->timer = timer; /* Number of hours before it hits 0 */
            qdata->limit = current_time + 3600; /* 1 hour before timer will drop 1 point. */
	    return;
	}

    /* create new quest data */
    qdata = new_quest();
    qdata->next = ch->pcdata->qdata;
    ch->pcdata->qdata = qdata;
    qdata->id = id;
    qdata->status = status;
    qdata->timer = timer;
}
