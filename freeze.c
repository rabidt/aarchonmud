#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

/* Freeze Tag, by Nebseni of Clandestine MUD.  Adaptation to Aarchon source by Rimbol. */

/* 
   Set these VNUMs to the min/max room number to randomly locate players
   in the freeze tag zone.
 */
int ftag_table[4][2]=
{
	{32500,32548},
	{17353,17380},
	{50,81},
	{17200,17352}
};

/* Local declarations. */
void show_ftag_status( CHAR_DATA *ch, bool final );
void give_ftag_reward(int team, int amount);
void check_team_frozen ( CHAR_DATA *ch );

int ftag_reward = 0;
int ftag_chamber=0;
int ftag_next=0;
bool going=FALSE;

DECLARE_DO_FUN( do_transfer );
DECLARE_DO_FUN( do_ftag );
DECLARE_DO_FUN( do_look );

DEF_DO_FUN(do_red)
{
    DESCRIPTOR_DATA *d;
    char buf [MAX_STRING_LENGTH];
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  red <message>\n\r", ch );
        return;
    }
    
    if (IS_NPC(ch) || (!IS_IMMORTAL(ch) && !IS_TAG(ch)))
    {
        send_to_char( "You must be a Freeze Tag player to use this channel.\n\r", ch );
        return;
    }
    
    if (!IS_IMMORTAL(ch) && !IS_SET(ch->pcdata->tag_flags,TAG_RED))
    {
        send_to_char("You must be on the {RRed{x team to use this channel.\n\r",ch);
        return;
    }
    
    
    sprintf(buf,"{R[RED] %s{x: %s\n\r",ch->name,argument);
    wiznet(buf, ch, NULL, WIZ_FTAG, 0, 0);
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (d->connected == CON_PLAYING)
            &&   IS_SET(d->character->pcdata->tag_flags,TAG_RED)) 
        {
            send_to_char(buf, d->character);
        }
    }
}

DEF_DO_FUN(do_blue)
{
    DESCRIPTOR_DATA *d;
    char buf [MAX_STRING_LENGTH];
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  blue <message>\n\r", ch );
        return;
    }
    
    if (IS_NPC(ch) || (!IS_IMMORTAL(ch) && !IS_TAG(ch)))
    {
        send_to_char( "You must be a Freeze Tag player to use this channel.\n\r", ch );
        return;
    }
    
    if (!IS_IMMORTAL(ch) && !IS_SET(ch->pcdata->tag_flags,TAG_BLUE))
    {
        send_to_char("You must be on the {BBlue{x team to use this channel.\n\r",ch);
        return;
    }
    
    sprintf(buf,"{B[BLUE] %s{x: %s\n\r",ch->name,argument);
    wiznet(buf, ch, NULL, WIZ_FTAG, 0, 0);
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (d->connected == CON_PLAYING)
            &&   IS_SET(d->character->pcdata->tag_flags,TAG_BLUE)) 
        {
            send_to_char(buf, d->character);
        }
    }
}

/* Added by Rimbol. */
void give_ftag_reward(int team, int amount)
{
    DESCRIPTOR_DATA *d;
    int value;
    
    if (team != TAG_BLUE && team != TAG_RED)
        return;
    
    value = URANGE(0, amount, 20);
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (d->connected == CON_PLAYING)
	    &&	 IS_TAG(d->character)
            &&   IS_SET(d->character->pcdata->tag_flags, team) )
        {
            if (IS_SET(d->character->pcdata->tag_flags, TAG_FROZEN)) 
            {
                printf_to_char(d->character, "You have received %d Quest points!\n\r", value/2);
                d->character->pcdata->questpoints += (value / 2);
            }
            else
            {
                printf_to_char(d->character, "You have received %d Quest points!\n\r", value);
                d->character->pcdata->questpoints += value;
            }
        }
    }
}

void check_team_frozen ( CHAR_DATA *ch )
{
    DESCRIPTOR_DATA *d;
    char buf [MAX_STRING_LENGTH];
    int count = 0;
    int ch_team = 0;
    
    if (IS_SET(ch->pcdata->tag_flags, TAG_BLUE))
        ch_team = TAG_BLUE;
    else
        ch_team = TAG_RED;
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (d->connected == CON_PLAYING) 
	    &&	 IS_TAG(d->character)
            &&   IS_SET(d->character->pcdata->tag_flags, (ch_team == TAG_BLUE) ? TAG_RED : TAG_BLUE))
        {
            count++;
            if (!IS_SET(d->character->pcdata->tag_flags, TAG_FROZEN))
                return;
        }
    }
    
    going=FALSE;
    
    sprintf(buf, "The %s team has won Freeze Tag!", 
        (ch_team == TAG_BLUE) ? "{BBlue{x" : "{RRed{x");
    info_message(ch, buf, TRUE);
    show_ftag_status(ch, TRUE);
    give_ftag_reward((ch_team == TAG_BLUE) ? TAG_BLUE : TAG_RED, 
        (ftag_reward < 0) ? count*2 : ftag_reward);
    do_ftag( NULL, "reset all" );
    return;
}

/* Added by Rimbol. */
void ftag_reset_player(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char buf [MAX_STRING_LENGTH];
    
    if ((victim = get_char_world(ch, argument)) == NULL)
    {
        send_to_char( "No such player.\n\r", ch );
        return;
    }
    if ((victim->desc == NULL) || IS_NPC(victim))
    {
        send_to_char("No such player.\n\r",ch);
        return;
    }
    if (victim->desc->connected != CON_PLAYING)
    {
        send_to_char("That character is not playing.\n\r",ch);
        return;
    }
    
    if (IS_TAG(victim))
    {
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_PLAYING);
        if (!IS_NPC(victim))
        {
            sprintf(buf, "%s %d", victim->name, ROOM_VNUM_TEMPLE);
            do_transfer(ch,buf);
        }
    }
    
    if ( IS_SET(victim->pcdata->tag_flags,TAG_FROZEN))
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_FROZEN);
    if ( IS_SET(victim->pcdata->tag_flags,TAG_RED))
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_RED);
    if ( IS_SET(victim->pcdata->tag_flags,TAG_BLUE))
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_BLUE);
    
    send_to_char( "You have been removed from Freeze Tag.\n\r", victim );
    
}

DEF_DO_FUN(do_ftag)
{
    DESCRIPTOR_DATA *d;
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf [MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    bool fRed = FALSE;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0'
        ||  (arg2[0] == '\0' 
        && (!str_cmp( arg1, "red" ) || !str_cmp( arg1, "blue" ) || !str_cmp(arg1, "reset") || !str_cmp(arg1, "admit"))))
    {
        send_to_char( "Syntax: ftag auto            - Splits everyone in the room evenly into teams.\n\r", ch );
        send_to_char( "        ftag red  <player>   - Add <player> to the red team.\n\r", ch );
        send_to_char( "        ftag blue <player>   - Add <player> to the blue team.\n\r", ch );
        send_to_char( "        ftag admit <player>  - Admit <player> to a game in progress.\n\r",ch);
        send_to_char( "        ftag chamber <chamber>- Sets the current freeze tag chamber.\n\r", ch );
        send_to_char( "        ftag start <amount>  - Starts the game, qp reward = <amount>.\n\r",ch);
        send_to_char( "        ftag reset all       - End game and return all players to healer.\n\r", ch );
        send_to_char( "        ftag reset <player>  - Remove <player> from the game.\n\r", ch );
        return;
    }
    
    if ( !str_cmp( arg1, "chamber" ) )
    {
        if (is_number(arg2))
            ftag_next = URANGE(0, atoi(arg2)-1, 3);
        
        send_to_char("Chamber 1: The original vanilla chamber.\n\r",ch);
        send_to_char("Chamber 2: A medium sized chamber.\n\r",ch);
        send_to_char("Chamber 3: Another medium sized chamber.\n\r",ch);
        send_to_char("Chamber 4: {DShadow Arena{x A huge chamber for big games.\n\r",ch);
        
        sprintf(buf, "Current freezetag chamber set to %d.\n\r", ftag_next+1);
        send_to_char(buf,ch);
        return;
    }
    else if ( !str_cmp( arg1, "start" ) )
    {
        if (is_number(arg2))
            ftag_reward = URANGE(0, atoi(arg2), 30);
        else
            ftag_reward = -1;
        
        ftag_chamber=ftag_next;
        going=TRUE;
        
        sprintf(buf, "%s has started a game of Freeze Tag!", capitalize(ch->name));
        info_message(ch, buf, TRUE);
        
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ((d->connected != CON_PLAYING) || IS_NPC(d->character))
                continue;
            
            if ( IS_TAG(d->character) )
            {
                sprintf(buf,"%s %d",d->character->name,
                    number_range(ftag_table[ftag_chamber][0],ftag_table[ftag_chamber][1]));
                REMOVE_BIT(d->character->pcdata->tag_flags,TAG_FROZEN);
                do_transfer(ch,buf);
                send_to_char( "{WFreeze Tag has started!{x\n\r", d->character );
            }
        }
        return;
    } 
    else if ( !str_cmp( arg1, "auto" ) )
    {
        for ( victim = ch->in_room->people; victim != NULL; 
        victim = victim->next_in_room )
        {
            if ( victim == ch || IS_NPC(victim))
                continue;
            
            if ((fRed = !fRed))
            {
                sprintf(buf,"red %s",victim->name);
            }
            else
            {
                sprintf(buf,"blue %s",victim->name);
            }
            
            do_ftag(ch,buf);
        }
        return;
    }   
    else if ( !str_cmp( arg1, "red" ) )
    {
        if ( ( victim = get_char_world( ch, arg2 ) ) == NULL )
        {
            send_to_char( "They aren't playing.\n\r", ch );
            return;
        }
        
        if (IS_NPC(victim))
        {
            send_to_char("No NPC's.\n\r",ch);
            return;
        }
        
        SET_BIT(victim->pcdata->tag_flags,TAG_PLAYING);
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_FROZEN);
        SET_BIT(victim->pcdata->tag_flags,TAG_RED);
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_BLUE);
        affect_strip(victim, gsn_sneak);
        REMOVE_BIT( victim->affect_field, AFF_SNEAK );
	affect_strip( victim, gsn_disguise );
	affect_strip( victim, gsn_mimic );
	make_visible( victim );
        act( "You are on the {RRED{x team!", ch, NULL, victim, TO_VICT );
        act( "$N is on the {RRED{x team!",   ch, NULL, victim, TO_NOTVICT );
        act( "$N is on the {RRED{x team!",   ch, NULL, victim, TO_CHAR );
    }
    else if ( !str_cmp( arg1, "blue" ) )
    {
        if ( ( victim = get_char_world( ch, arg2 ) ) == NULL )
        {
            send_to_char( "They aren't playing.\n\r", ch );
            return;
        }
        
        if (IS_NPC(victim))
        {
            send_to_char("No NPC's.\n\r",ch);
            return;
        }
        
        SET_BIT(victim->pcdata->tag_flags,TAG_PLAYING);
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_FROZEN);
        SET_BIT(victim->pcdata->tag_flags,TAG_BLUE);
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_RED);
        affect_strip(victim, gsn_sneak);
        REMOVE_BIT( victim->affect_field, AFF_SNEAK );
	affect_strip( victim, gsn_disguise );
	affect_strip( victim, gsn_mimic );
	make_visible( victim );
        act( "You are on the {BBLUE{x team!", ch, NULL, victim, TO_VICT );
        act( "$N is on the {BBLUE{x team!",   ch, NULL, victim, TO_NOTVICT );
        act( "$N is on the {BBLUE{x team!",   ch, NULL, victim, TO_CHAR );
    }
    else if (!str_cmp(arg1, "admit")) /* Added by Rimbol. */
    {
        if ((victim = get_char_world( ch, arg2 ) ) == NULL)
        {
            send_to_char( "They aren't playing.\n\r", ch );
            return;
        }
        
        if (IS_NPC(victim))
        {
            send_to_char("No NPC's.\n\r",ch);
            return;
        }
        
        if (!IS_SET(victim->pcdata->tag_flags, TAG_BLUE) && !IS_SET(victim->pcdata->tag_flags, TAG_RED))
        {
            send_to_char("You must first set that player to {RRed{x or {BBlue{x.\n\r",ch);
            return;
        }
        
        sprintf(buf,"%s %d",victim->name,
            number_range(ftag_table[ftag_chamber][0],ftag_table[ftag_chamber][1]));
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_FROZEN);
        do_transfer(ch,buf);
        
        if (IS_SET(victim->pcdata->tag_flags, TAG_BLUE))
            sprintf(buf, "%s has been admitted to the {BBlue{x team!\n\r", victim->name);
        else
            sprintf(buf, "%s has been admitted to the {RRed{x team!\n\r", victim->name);
        do_blue(ch, buf);
        do_red(ch, buf);
        
        printf_to_char(ch, "%s has been admitted.\n\r", victim->name);
    }
    else if ( !str_cmp( arg1, "reset" ) )
    {
        if (!str_cmp(arg2, "all"))
        {
            going=FALSE;
            for ( d = descriptor_list; d != NULL; d = d->next )
            {
                if ((d->connected != CON_PLAYING) || IS_NPC(d->character))
                    continue;
                
                if ( IS_TAG(d->character) )
                {
                    REMOVE_BIT(d->character->pcdata->tag_flags,TAG_PLAYING);
                    char_from_room( d->character );
                    char_to_room( d->character, get_room_index( ROOM_VNUM_TEMPLE ) );
                    do_look( d->character, "" );
                }
                
                if ( IS_SET(d->character->pcdata->tag_flags,TAG_FROZEN))
                    REMOVE_BIT(d->character->pcdata->tag_flags,TAG_FROZEN);
                if ( IS_SET(d->character->pcdata->tag_flags,TAG_RED))
                    REMOVE_BIT(d->character->pcdata->tag_flags,TAG_RED);
                if ( IS_SET(d->character->pcdata->tag_flags,TAG_BLUE))
                    REMOVE_BIT(d->character->pcdata->tag_flags,TAG_BLUE);
                send_to_char( "Freeze Tag has been reset.\n\r", d->character );
            }
            if ( ch != NULL )
                send_to_char( "All players reset.\n\r", ch );
            return;
        }
        else
            ftag_reset_player(ch, arg2);
    }
    else
        send_to_char("Command failed, check the syntax.\n\r",ch);
    
    return;
}

/* Added by Rimbol. */
void show_ftag_status( CHAR_DATA *ch, bool final )
{
    DESCRIPTOR_DATA *d;
    char buf [MAX_STRING_LENGTH];
    int bPlay = 0, bFrozen = 0;
    int rPlay = 0, rFrozen = 0;
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (d->connected == CON_PLAYING) && IS_TAG(d->character) )
        {
            if (IS_SET(d->character->pcdata->tag_flags, TAG_BLUE))
            {
                bPlay++;
                if (IS_SET(d->character->pcdata->tag_flags, TAG_FROZEN)) 
                    bFrozen++;
            }
            
            if (IS_SET(d->character->pcdata->tag_flags, TAG_RED))
            {
                rPlay++;
                if (IS_SET(d->character->pcdata->tag_flags, TAG_FROZEN))
                    rFrozen++;
            }
        }
    }
    
    if (final)
    {
        sprintf(buf, " {RRed Team{x Score:  {+%d Playing, %d Frozen{x", rPlay, rFrozen);
        info_message(ch, buf, TRUE);
        sprintf(buf, "{BBlue Team{x Score:  {+%d Playing, %d Frozen{x", bPlay, bFrozen);
        info_message(ch, buf, TRUE);
    }
    else
    {
        printf_to_char(ch, " {RRed Team{x Score:  {+%d Playing, %d Frozen{x\n\r", rPlay, rFrozen);
        printf_to_char(ch, "{BBlue Team{x Score:  {+%d Playing, %d Frozen{x\n\r", bPlay, bFrozen);
    }
    
    return;
}

DEF_DO_FUN(do_tag)
{
    char arg [MAX_INPUT_LENGTH], temp[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char buf [MAX_STRING_LENGTH];
    
    argument = one_argument( argument, temp );
    
    number_argument(temp, arg); /* Strip off numeric references from argument,
                                   to cut down on triggers. -Rim */
    
    if ( (!IS_IMMORTAL(ch) && !IS_TAG(ch) && !going) || IS_NPC(ch) )
    {
        send_to_char( "You're not playing Freeze Tag.\n\r", ch );
        return;
    }
    
    if ( !str_cmp(arg, "status") )
    {
        show_ftag_status(ch, FALSE);
        return;
    }
    
    if ( arg[0] == '\0' || (arg[0]<='9' && arg[0]>='0') || arg[1]=='.')
    {
        send_to_char("Syntax:  tag status   - Display current status of game.\n\r",ch);
        send_to_char("      :  tag <player> - Tag another player, or un-freeze a teammate!\n\r",ch);
        return;
    }
    
    if (ch->in_room == NULL
        || ch->in_room->vnum < ftag_table[ftag_chamber][0]
        || ch->in_room->vnum > ftag_table[ftag_chamber][1])
    {
        send_to_char("You cannot tag outside the Freeze Tag arena.\n\r",ch);
        return;
    }
    
    if ((victim = get_char_room( ch, arg ) ) == NULL)
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

	if (IS_NPC(victim))
	{
		send_to_char("You can't tag an NPC.\n\r", ch);
		return;
	}
    
    if ( victim == ch )
    {
        send_to_char( "You tag yourself.  How amusing.\n\r", ch );
        return;
    }
    
    if (!IS_TAG(victim))
    {
        send_to_char( "They're not playing Freeze Tag.\n\r", ch );
        return;
    }
    
    if (IS_SET(ch->pcdata->tag_flags,TAG_FROZEN))
    {
        send_to_char( "You can't tag, you're frozen!\n\r", ch );
        return;
    }
    
    act( "$n tags you!", ch, NULL, victim, TO_VICT );
    act( "$n tags $N!",  ch, NULL, victim, TO_NOTVICT );
    act( "You tag $N!",  ch, NULL, victim, TO_CHAR );
    
    if (IS_SET(victim->pcdata->tag_flags,TAG_FROZEN))
    {
        REMOVE_BIT(victim->pcdata->tag_flags,TAG_FROZEN);
        act( "You are no longer frozen!", ch, NULL, victim, TO_VICT );
        act( "$N is no longer frozen!",   ch, NULL, victim, TO_NOTVICT );
        act( "$N is no longer frozen!",   ch, NULL, victim, TO_CHAR );
        sprintf(buf,"Wiznet:{%c%s{x was un-frozen by {%c%s{x.", 
            IS_SET(victim->pcdata->tag_flags, TAG_BLUE) ?  'B' : 'R', 
            capitalize(victim->name),
            IS_SET(ch->pcdata->tag_flags, TAG_BLUE) ? 'B' : 'R', 
            capitalize(ch->name));
        wiznet(buf, ch, NULL, WIZ_FTAG, 0, 0);
        
    }
    else
    {
        SET_BIT(victim->pcdata->tag_flags,TAG_FROZEN);
        act( "You are frozen!", ch, NULL, victim, TO_VICT );
        act( "$N is frozen!",   ch, NULL, victim, TO_NOTVICT );
        act( "$N is frozen!",   ch, NULL, victim, TO_CHAR );
        sprintf(buf,"Wiznet: {%c%s{x was tagged by {%c%s{x.", 
            IS_SET(victim->pcdata->tag_flags, TAG_BLUE) ?  'B' : 'R', 
            capitalize(victim->name),
            IS_SET(ch->pcdata->tag_flags, TAG_BLUE) ?  'B' : 'R', 
            capitalize(ch->name));
        wiznet(buf, ch, NULL, WIZ_FTAG, 0, 0);
        check_team_frozen( ch );
    }
    
    return;
}
