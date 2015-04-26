/***************************************************************************
* MARRY.C written by Ryouga (<ryouga@jessi.indstate.edu>) for Vilaross Mud.*
* EotN Implementation, alterations and re-write by Voltec                  *
*    (Paul Seward <voltec@cyberdude.com>)                                  *
* Rewritten again by Rimbol (Brian Castle) for adaptation to Aarchon MUD.  *
***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"


DEF_DO_FUN(do_consent)
{
    if (IS_NPC(ch))
        return;
    
    if ( IS_SET(ch->act, PLR_CONSENT) )
    {
        send_to_char( "You no longer give consent to change your marital status.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_CONSENT);
        return;
    }
    
    send_to_char( "You have given consent to change your marital status.\n\r", ch);
    SET_BIT(ch->act, PLR_CONSENT);
    return;
}


DEF_DO_FUN(do_marry)
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;

    if (IS_NPC(ch))
        return;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: marry <char1> <char2>\n\r",ch);
        return;
    }

    if (!IS_IMMORTAL(ch) && !clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_marry)
    {
        send_to_char( "You do not have marrying power.\n\r", ch);
        return;
    }    

    if ( ((victim  = get_char_world(ch,arg1)) == NULL) ||
         ((victim2 = get_char_world(ch,arg2)) == NULL))
    {
        send_to_char("Both characters must be playing!\n\r", ch );
        return;
    }
    
    if ( IS_NPC(victim) || IS_NPC(victim2))
    {
        send_to_char("Mobs can't get married!\n\r", ch);
        return;
    }

    if (victim == ch || victim2 == ch)
    {
        send_to_char("You need to have someone else marry you.\n\r",ch);
        return;
    }

    if (!IS_IMMORTAL(ch) && ch->clan != victim->clan && ch->clan != victim2->clan)
    {
        printf_to_char(ch, "%ss may only marry their clan members.  Neither of those players are in your clan.\n\r",
            capitalize(clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].name));
        return;
    }

    if (!IS_SET(victim->act, PLR_CONSENT) || !IS_SET(victim2->act, PLR_CONSENT))
    {
        send_to_char( "They have not both given consent.\n\r", ch);
        return;
    }
    
    if (victim->pcdata->spouse > 0 || victim2->pcdata->spouse > 0)
    {
        send_to_char( "At least one of them is already married!\n\r", ch);
        return;
    }
    
    if (   (victim->level  < 15 && victim->pcdata->remorts  == 0) 
        || (victim2->level < 15 && victim2->pcdata->remorts == 0))
    {
        send_to_char( "They are not of the proper level to marry.\n\r", ch);
        return;
    }
    
/*  NO WAY!  I think same-sex marriages SHOULD be permitted!!!
    if (victim->pcdata->true_sex == victim2->pcdata->true_sex)
    {
        send_to_char("For roleplay purposes, same-sex marriages are not allowed.\n\r",ch);
        return;
    } */
    
    printf_to_char(victim, "%s has married you to %s!\n\r", ch->name, victim2->name);
    printf_to_char(victim2, "%s has married you to %s!\n\r", ch->name, victim->name);
    
    sprintf(log_buf, "%s has pronounced %s and %s married!\n\r", ch->name, victim->name, victim2->name);
    info_message(ch, log_buf, TRUE);
    
    victim->pcdata->spouse = str_dup(victim2->name);
    victim2->pcdata->spouse = str_dup(victim->name);

    REMOVE_BIT(victim->act, PLR_CONSENT);
    REMOVE_BIT(victim2->act, PLR_CONSENT);
    return;
}


DEF_DO_FUN(do_divorce)
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *victim2;

    if (IS_NPC(ch))
        return;

    if (!IS_IMMORTAL(ch))
    {
        send_to_char( "You do not have the power to divorce.\n\r", ch);
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: divorce <char1> <char2>\n\r",ch);
        return;
    }

    if ( ((victim  = get_char_world(ch,arg1)) == NULL) ||
         ((victim2 = get_char_world(ch,arg2)) == NULL))
    {
        send_to_char("Both characters must be playing!\n\r", ch );
        return;
    }
    
    if ( IS_NPC(victim) || IS_NPC(victim2))
    {
        send_to_char("Mobs can't get divorced!\n\r", ch);
        return;
    }

    if (!IS_SET(victim->act, PLR_CONSENT) || !IS_SET(victim2->act, PLR_CONSENT))
    {
        send_to_char( "They have not both given consent.\n\r", ch);
        return;
    }

    if (victim->pcdata->spouse && 
        (str_cmp(victim->pcdata->spouse, victim2->name) || 
         str_cmp(victim2->pcdata->spouse, victim->name)))
    {
        send_to_char("Those two players are not married to each other.\n\r",ch);
        return;
    }

    if (victim == ch || victim2 == ch)
    {
        send_to_char("You need to have someone else divorce you.\n\r",ch);
        return;
    }


    printf_to_char(victim, "%s has granted you a divorce from %s.\n\r", ch->name, victim2->name);
    printf_to_char(victim2, "%s has granted you a divorce from %s.\n\r", ch->name, victim->name);
    
    sprintf(log_buf, "%s has granted a divorce to %s and %s.\n\r", ch->name, victim->name, victim2->name);
    info_message(ch, log_buf, TRUE);

    free_string(victim->pcdata->spouse);
    victim->pcdata->spouse = NULL;

    free_string(victim2->pcdata->spouse);
    victim2->pcdata->spouse = NULL;

    REMOVE_BIT(victim->act, PLR_CONSENT);
    REMOVE_BIT(victim2->act, PLR_CONSENT);
    return;
}

void check_spouse( CHAR_DATA *ch )
{
    DESCRIPTOR_DATA *d = NULL;
    CHAR_DATA *spouse;

    if ( ch == NULL || ch->pcdata == NULL || ch->pcdata->spouse == NULL )
	return;
    
    /*
    if ( !pfile_exists(ch->pcdata->spouse) )
    {
	send_to_char( "Your spouse has vanished. You're single again.\n\r", ch );
	free_string( ch->pcdata->spouse );
	ch->pcdata->spouse = NULL;
	return;
    }
    */

    /* check if spouse exists and is married to char */
    if ( (spouse = get_player(ch->pcdata->spouse)) == NULL )
    {
	d=new_descriptor();
	if ( !load_char_obj(d, ch->pcdata->spouse, TRUE) )
	{
	    send_to_char( "Your spouse has vanished. You're single again.\n\r", ch );
	    free_string( ch->pcdata->spouse );
	    ch->pcdata->spouse = NULL;
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
	spouse = d->character;
    }
    
    if ( spouse->pcdata->spouse == NULL
	 || strcmp(spouse->pcdata->spouse, ch->name) )
    {
	send_to_char( "Your spouse isn't married to you. You're single again.\n\r", ch );
	free_string( ch->pcdata->spouse );
	ch->pcdata->spouse = NULL;
    }

    /* free descriptor if spouse was offline */
    if ( d != NULL )
    {
	free_char(spouse);
	free_descriptor(d);
    }
}
