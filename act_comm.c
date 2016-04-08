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
**************************************************************************/

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
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "special.h"
//#include "religion.h"
#include "simsave.h"
#include "interp.h"
#include "warfare.h"

/* command procedures needed */
DECLARE_DO_FUN(do_quit  );
DECLARE_DO_FUN(do_groups    );
void war_remove( CHAR_DATA *ch, bool killed );
void smash_beep_n_blink( char *str );
void quit_char( CHAR_DATA *ch );
void try_set_leader( CHAR_DATA *ch, CHAR_DATA *victim );
void change_leader( CHAR_DATA *old_leader, CHAR_DATA *new_leader );
void open_chat_tag( CHAR_DATA *ch );
void close_chat_tag( CHAR_DATA *ch );

#define ALTER_COLOUR( type ) \
if( !str_prefix( argument, "red" ) ) {ch->pcdata->type[0] = NORMAL;ch->pcdata->type[1] = RED;} \
else if ( !str_prefix( argument, "hi-red" ) ) {ch->pcdata->type[0] = BRIGHT;ch->pcdata->type[1] = RED;} \
else if ( !str_prefix( argument, "green" ) ) {ch->pcdata->type[0] = NORMAL;ch->pcdata->type[1] = GREEN;} \
else if ( !str_prefix( argument, "hi-green" ) ) {ch->pcdata->type[0] = BRIGHT;ch->pcdata->type[1] = GREEN;} \
else if ( !str_prefix( argument, "yellow" ) ) {ch->pcdata->type[0] = NORMAL;ch->pcdata->type[1] = YELLOW;} \
else if ( !str_prefix( argument, "hi-yellow" ) ) {ch->pcdata->type[0] = BRIGHT;ch->pcdata->type[1] = YELLOW;} \
else if ( !str_prefix( argument, "blue" ) ) {ch->pcdata->type[0] = NORMAL;ch->pcdata->type[1] = BLUE;} \
else if ( !str_prefix( argument, "hi-blue" ) ) {ch->pcdata->type[0] = BRIGHT;ch->pcdata->type[1] = BLUE;} \
else if ( !str_prefix( argument, "magenta" ) ) {ch->pcdata->type[0] = NORMAL;ch->pcdata->type[1] = MAGENTA;} \
else if ( !str_prefix( argument, "hi-magenta" ) ) {ch->pcdata->type[0] = BRIGHT;ch->pcdata->type[1] = MAGENTA;} \
else if ( !str_prefix( argument, "cyan" ) ) {ch->pcdata->type[0] = NORMAL;ch->pcdata->type[1] = CYAN;} \
else if ( !str_prefix( argument, "hi-cyan" ) ) {ch->pcdata->type[0] = BRIGHT;ch->pcdata->type[1] = CYAN;} \
else if ( !str_prefix( argument, "white" ) ){ch->pcdata->type[0] = NORMAL;ch->pcdata->type[1] = WHITE;} \
else if ( !str_prefix( argument, "hi-white" ) ) {ch->pcdata->type[0] = BRIGHT;ch->pcdata->type[1] = WHITE;} \
else if ( !str_prefix( argument, "grey" ) ) {ch->pcdata->type[0] = BRIGHT;ch->pcdata->type[1] = BLACK;} \
else if ( !str_prefix( argument, "clear" ) ) {ch->pcdata->type[0] = NORMAL;ch->pcdata->type[1] = COLOUR_NONE;} \
else if ( !str_prefix( argument, "beep" ) ) {ch->pcdata->type[2] = 1;} \
else if ( !str_prefix( argument, "nobeep" ) ) {ch->pcdata->type[2] = 0;} \
else { send_to_char_bw( "Unrecognized colour. Unchanged.\n\r", ch ); return;}


DEF_DO_FUN(do_delet)
{
    send_to_char("You must type the full command to delete yourself.\n\r",ch);
}

DEF_DO_FUN(do_delete)
{
    char filename[MAX_INPUT_LENGTH];
    char arg1[MIL], arg2[MIL];
    
    if ( IS_NPC(ch) )
        return;

    argument = one_argument_keep_case( argument, arg1 );
    argument = one_argument_keep_case( argument, arg2 );
    
    if (ch->pcdata->confirm_delete)
    {
        if ( !strcmp(arg1, "confirm") )
        {
	    /* extra password check to prevent deleting by trigger-abuse */
        if ( !check_password(arg2, ch->pcdata->pwd) )
	    {
		send_to_char( "Wrong password, try again.\n\r", ch );
		return; 
	    }

            wiznet("$N turns $Mself into line noise.",ch,NULL,0,0,0);
	    logpf( "%s has deleted.", ch->name );
            stop_fighting(ch,TRUE);
            
            if (ch->level >= LEVEL_IMMORTAL)
            {
                update_wizlist(ch, 1);
            }            
            
            remort_remove(ch, FALSE);
	    if ( IS_AUTHED(ch) )
		add_auto_auth( ch->name );
            remove_from_auth( ch->name );
	    //remove_from_all_lboards( ch->name);
            rank_available(ch->clan, ch->pcdata->clan_rank, 0);
	    //religion_remove_follower( ch );
            sprintf( filename, "%s", capitalize( ch->name ) );
	    quit_char( ch );
	    unlink_pfile( filename );
            return;
        }
        else
        {
            send_to_char("Delete status removed.\n\r",ch);
            ch->pcdata->confirm_delete = FALSE;
            return;
        }
    }
    
    if (arg1[0] != '\0')
    {
        send_to_char("Just type delete. No argument.\n\r",ch);
        return;
    }
    
    send_to_char("Type 'delete confirm <password>' to confirm your wish to delete yourself.\n\r",ch);
    send_to_char("WARNING: This command is irreversible!\n\r",ch);
    send_to_char("Typing delete with any other argument will abort the operation.\n\r", ch);
    ch->pcdata->confirm_delete = TRUE;
    wiznet("$N is contemplating deletion.",ch,NULL,0,0,get_trust(ch));
}


void print_pub_chan( sh_int sn, CHAR_DATA *ch )
{
    const CHANNEL *chan = &(public_channel_table[sn]);
    char buf[MSL];
    sprintf(buf, "{%c%s{%c",
		chan->prime_color,
		chan->name,
		chan->second_color);
    while (strlen_color(buf) < 15)
	strcat(buf, " ");

    strcat(buf, IS_SET( ch->comm, chan->offbit) ? "OFF" : "ON");
    strcat(buf, "\n\r");

    send_to_char(buf, ch);
}

/* RT code to display channel status */

DEF_DO_FUN(do_channels)
{   
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char("   channel     status\n\r",ch);
    send_to_char("---------------------\n\r",ch);
    
    if (NOT_AUTHED(ch))
    {
        send_to_char("{1INFO messages{x  ",ch);
        if (!IS_SET(ch->comm,COMM_NOINFO))
            send_to_char("{2ON{x\n\r",ch);
        else
            send_to_char("{2OFF{x\n\r",ch);
    }
    else
    {
        send_to_char("{1INFO messages{x  ",ch);
        if (!IS_SET(ch->comm,COMM_NOINFO))
            send_to_char("{2ON{x\n\r",ch);
        else
            send_to_char("{2OFF{x\n\r",ch);
        
    print_pub_chan(sn_gossip, ch);
    print_pub_chan(sn_auction, ch);
    print_pub_chan(sn_music, ch);
    print_pub_chan(sn_question, ch);
    print_pub_chan(sn_quote, ch);
    print_pub_chan(sn_gratz, ch);
    print_pub_chan(sn_gametalk, ch);
    print_pub_chan(sn_bitch, ch);
    print_pub_chan(sn_newbie, ch);
 
        if( ch->clan > 0 )
	{
          send_to_char("{lClan{x           ",ch);
          if (!IS_SET(ch->comm,COMM_NOCLAN))
            send_to_char("{LON{x\n\r",ch);
          else
            send_to_char("{LOFF{x\n\r",ch);
	}

	/*
	if( get_religion(ch) != NULL )
	{
          send_to_char("{9Proclaim{x       ",ch);
          if (!IS_SET(ch->comm,COMM_NOREL))
            send_to_char("{0ON{x\n\r",ch);
          else
            send_to_char("{0OFF{x\n\r",ch);
	}
	*/

        send_to_char("{5War{x            ",ch);
        if (!IS_SET(ch->comm,COMM_NOWAR))
            send_to_char("{6ON{x\n\r", ch);
        else
            send_to_char("{6OFF{x\n\r", ch);
        
        if (is_granted_name(ch,"immtalk") || is_granted_name(ch,":"))
        {
	    print_pub_chan(sn_immtalk, ch);
        }
        
        if (is_granted_name(ch,"savantalk"))
        {
	    print_pub_chan(sn_savantalk, ch);
        }
        
	if (IS_SET(ch->penalty,PENALTY_NOSHOUT))
	    send_to_char("{uYou cannot {Ushout{u.{x\n\r",ch);
	else
	{
            send_to_char("{uShouts{x         ",ch);
            if (!IS_SET(ch->comm,COMM_SHOUTSOFF))
                send_to_char("{UON{x\n\r",ch);
            else
                send_to_char("{UOFF{x\n\r",ch);
	}
    }
    
    if (IS_SET(ch->penalty,PENALTY_NOTELL))
        send_to_char("{tYou cannot use {Ttells{t.{x\n\r",ch);
    else
    {
        send_to_char("{tDeaf to tells{x  ",ch);
        if (!IS_SET(ch->comm,COMM_DEAF))
            send_to_char("{TOFF{x\n\r",ch);
        else
            send_to_char("{TON{x\n\r",ch);
    }

    send_to_char("Quiet mode     ",ch);
    if (IS_SET(ch->comm,COMM_QUIET))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
    
    if (IS_SET(ch->comm,COMM_AFK))
        send_to_char("You are AFK.\n\r",ch);

    if (IS_SET(ch->comm,COMM_BUSY))
        send_to_char("You claim to be too busy to receive tells.\n\r",ch);
    
    if (ch->lines != PAGELEN)
    {
        if (ch->lines)
        {
            sprintf(buf,"You display %d lines of scroll.\n\r",ch->lines+2);
            send_to_char(buf,ch);
        }
        else
            send_to_char("Scroll buffering is off.\n\r",ch);
    }

    ptc( ch, "Your chat window is turned %s.\n\r",
            ( ch->pcdata && ch->pcdata->guiconfig.chat_window ) ? "ON" : "OFF" );
    
    if (ch->prompt != NULL)
    {
        sprintf(buf,"Your current prompt is: %s\n\r",ch->prompt);
        send_to_char(buf,ch);
    }
    
    if (IS_SET(ch->penalty,PENALTY_NOCHANNEL))
        send_to_char("You cannot use ANY channels.\n\r",ch);
    
    if (IS_SET(ch->penalty,PENALTY_NOEMOTE))
        send_to_char("You cannot show emotions.\n\r",ch);
    
    if (IS_SET(ch->penalty,PENALTY_NONOTE))
        send_to_char("You cannot write notes.\n\r",ch);
    
}

/* RT deaf blocks out all shouts */

DEF_DO_FUN(do_deaf)
{
    
    if (IS_SET(ch->comm,COMM_DEAF))
    {
        send_to_char("You can now hear tells again.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_DEAF);
    }
    else
    {
        send_to_char("From now on, you won't hear tells.\n\r",ch);
        SET_BIT(ch->comm,COMM_DEAF);
    }
}

/* RT quiet blocks out all communication */

DEF_DO_FUN(do_quiet)
{
    if (IS_SET(ch->comm,COMM_QUIET))
    {
        send_to_char("Quiet mode removed.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_QUIET);
    }
    else
    {
        send_to_char("From now on, you will only hear says and emotes.\n\r",ch);
        SET_BIT(ch->comm,COMM_QUIET);
    }
}

/* afk command */

DEF_DO_FUN(do_afk)
{

    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->comm,COMM_AFK))
    {
        send_to_char("AFK mode removed.\n\r",ch);
        if (ch->pcdata->new_tells)
            send_to_char("Type 'playback tell' to see missed tells.\n\r", ch );
        REMOVE_BIT(ch->comm,COMM_AFK);
    }
    else
    {
        send_to_char("You are now in AFK mode.\n\r",ch);
        SET_BIT(ch->comm,COMM_AFK);
    }
}

/* busy command; marks you "busy" instead of afk so you don't have to receive tells */
/* related to autobusy command, which makes you busy during fights only, and "unbusy" afterwards */
DEF_DO_FUN(do_busy)
{

    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->comm,COMM_BUSY))
    {
        send_to_char("BUSY mode removed.\n\r",ch);
        if (ch->pcdata->new_tells)
            send_to_char("Type 'playback tell' to see missed tells.\n\r", ch );
        REMOVE_BIT(ch->comm,COMM_BUSY);
    }
    else
    {
        send_to_char("You are now in BUSY mode.\n\r",ch);
        SET_BIT(ch->comm,COMM_BUSY);
    }
}


DEF_DO_FUN(do_replay)
{
	do_playback( ch, "tell" );
}


const char *parse_url( const char *argument )
{
    const char open[]="\t<a href=\"";
    const char mid[]= "\">";
    const char close[]="\t</a>";

    char *url;
    if ( ! (    ( url=strstr(argument, "http://" ) )
             || ( url=strstr(argument, "https://") )
             || ( url=strstr(argument, "www."    ) ) ) )
    return argument;
    
    static char rtn[MSL*10];
    int rtnIndex;

    for ( rtnIndex=0 ; ; argument++)
    {
        if ( argument==url )
        {
            strncpy( &rtn[rtnIndex], open, strlen(open));
            rtnIndex+=strlen(open);

            char *cptr;
            for (cptr=url; *cptr != ' ' && *cptr !='\0' && *cptr !='\n' && *cptr !='\r'; cptr++)
            {
                rtn[rtnIndex++]=*cptr;
            }

            strncpy( &rtn[rtnIndex], mid, strlen(mid));
            rtnIndex+=strlen(mid);

            for (argument=url; *argument != ' ' && *argument !='\0' ; argument++)
            {
                rtn[rtnIndex++]=*argument;
            }

            strncpy( &rtn[rtnIndex], close, strlen(close));
            rtnIndex+=strlen(close);

        }
        
        if ( *argument == '\0' )
        {
            rtn[rtnIndex++]='\0';
            break;
        }

        rtn[rtnIndex++]=*argument;
    }

    return rtn;
}

/* RT chat replaced with ROM gossip */
void public_channel( const CHANNEL *chan, CHAR_DATA *ch, const char *argument )
{
	if ( chan == NULL )
	{
		bugf("NULL channel sent to public_channel.");
		return;
	}
	
    char buf[MAX_STRING_LENGTH], arg_buf[MSL];
    DESCRIPTOR_DATA *d;
    bool found;
    sh_int pos;
    
    if (NOT_AUTHED(ch))
    {
        send_to_char("You can't use that channel yet.\n\r", ch);
        return;
    }
    
    if (argument[0] == '\0' )
    {
        if (IS_SET(ch->comm,chan->offbit))
        {
		printf_to_char(ch, "{%c%s channel{x is now {%cON{x.\n\r", chan->prime_color, chan->name, chan->second_color);
            REMOVE_BIT(ch->comm,chan->offbit);
        }
        else
        {
		printf_to_char(ch, "{%c%s channel{x is now {%cOFF{x.\n\r", chan->prime_color, chan->name, chan->second_color);
            SET_BIT(ch->comm, chan->offbit);
        }
    }
    else  /* channel message sent, turn channel on if it isn't already */
    {
        if (!IS_NPC(ch) && ch->level < chan->min_level && ch->pcdata->remorts == 0 && ch->pcdata->ascents == 0 )
        {
	    printf_to_char( ch, "You can't use %s channel until level %d.\n\r", chan->name, chan->min_level);
            return;
        }
        
        if (IS_SET(ch->comm,COMM_QUIET))
        {
            send_to_char("You must turn off quiet mode first.\n\r",ch);
            return;
        }
        
        if (IS_SET(ch->penalty,PENALTY_NOCHANNEL))
        {
            send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
            return;
            
        }
        
	    /* If it's off, turn it on, and TELL them */
        if (IS_SET( ch->comm, chan->offbit) )
        {
            REMOVE_BIT(ch->comm, chan->offbit);
            printf_to_char(ch, "{%c%s channel{x is now {%cON{x.\n\r", 
                    chan->prime_color, 
                    chan->name, 
                    chan->second_color);
        }

        strcpy(arg_buf, argument);
        smash_beep_n_blink(arg_buf);
        argument=parse_url(arg_buf);
        
        sprintf( buf, "{%cYou %s {%c'%s{%c'{x\n\r", chan->prime_color, chan->first_pers, chan->second_color, argument , chan->second_color);
        show_image_to_char( ch, buf );
        send_to_char( buf, ch );
        if ( ch->pcdata && ch->pcdata->guiconfig.chat_window )
        {
            open_chat_tag( ch );
            send_to_char( buf, ch );
            close_chat_tag( ch );
        }

        argument = makedrunk(argument,ch);

        sprintf(buf,"{%c %s {%c'%s{%c'", chan->prime_color, chan->third_pers, chan->second_color, argument, chan->second_color);
        log_chan(ch, buf, *(chan->psn));

        // public channels show character name
        sprintf(buf,"{%c%s{%c %s {%c'$t{%c'{x", chan->prime_color, IS_NPC(ch) ? ch->short_descr : ch->name,
            chan->prime_color, chan->third_pers, chan->second_color, chan->second_color);
        
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            CHAR_DATA *victim;
            
            victim = d->original ? d->original : d->character;
            
            if ( (IS_PLAYING(d->connected)) &&
                d->character != ch &&
                !IS_SET(victim->comm,chan->offbit) &&
                !IS_SET(victim->comm,COMM_QUIET) &&
                !NOT_AUTHED(victim) &&
		(chan->check == NULL ? TRUE : (*(chan->check))(victim) ) ) /* special check for certain channels */
            {
                found = FALSE;
                for (pos = 0; pos < MAX_FORGET; pos++)
                {
                    if (IS_NPC(victim))
                        break;
                    if (victim->pcdata->forget[pos] == NULL)
                        break;
                    if (!str_cmp(ch->name,victim->pcdata->forget[pos]))
                        found = TRUE; 
                }
                if (!found)
                {
                    //sprintf(buf,"{%c$n{%c %s {%c'$t{%c'{x", chan->prime_color, chan->prime_color, chan->third_pers, chan->second_color, chan->second_color);
                    act_new( buf,
                        ch, argument, d->character, TO_VICT, POS_DEAD);

                    if ( d->character->pcdata && 
                            d->character->pcdata->guiconfig.chat_window )
                    {
                        open_chat_tag( d->character );
                        act_new( buf,
                            ch,argument, d->character, TO_VICT, POS_DEAD);

                        close_chat_tag( d->character );
                    }
                }
            }
        }
    }
}

DEF_DO_FUN(do_gossip)
{
	public_channel( &public_channel_table[sn_gossip], ch, argument );
}


DEF_DO_FUN(do_newbie)
{
	public_channel( &public_channel_table[sn_newbie], ch, argument );
}



DEF_DO_FUN(do_bitch)
{
	public_channel( &public_channel_table[sn_bitch], ch, argument );
}

DEF_DO_FUN(do_gametalk)
{
	public_channel( &public_channel_table[sn_gametalk], ch, argument );
}
/* Function info_message is called in a fashion similar to wiznet to
announce various events to players. -- Rimbol */
void info_message( CHAR_DATA *ch, const char *argument, bool show_to_char )
{
    info_message_new( ch, argument, show_to_char, FALSE );
}

/* extended version that can show message only to players who see
 * the character - needed for login info */
void info_message_new( CHAR_DATA *ch, const char *argument, bool show_to_char, bool check_visible )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    
    if (ch && IS_IMMORTAL(ch) && ch->invis_level > LEVEL_HERO)
        return;
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        victim = d->original ? d->original : d->character;
        
        if (!victim)
            continue;
        
        if (!show_to_char && victim == ch) /* If ch == NULL, this will be false */
            continue;

	if ( check_visible && !check_see( victim, ch ) )
	    continue;
        
        /* Don't send automatic info messages to unauthed players. */
        if (!ch && NOT_AUTHED(victim))
            continue;
        
            /* If message was generated by an unauthed player, show only to other
        unauthed players and imms. */
        if (ch && NOT_AUTHED(ch) && !IS_IMMORTAL(victim) && !NOT_AUTHED(victim))
            continue;
        
        /* If message was generated by an authed player, do not show to unauthed players. */
        if (ch && !NOT_AUTHED(ch) && NOT_AUTHED(victim))
            continue;
        
        if ( (IS_PLAYING(d->connected)) &&
            !IS_SET(victim->comm,COMM_NOINFO) &&
            !IS_SET(victim->comm,COMM_QUIET) )
        {
            sprintf(buf, "{1[INFO]{2: %s\n{x", argument);
            act_new( buf, victim, NULL, NULL, TO_CHAR, POS_SLEEPING );

            if ( victim->pcdata && victim->pcdata->guiconfig.chat_window )
            {
                open_chat_tag( victim );
                act_new( buf, victim, NULL, NULL, TO_CHAR, POS_SLEEPING );
                close_chat_tag( victim );
            }
        }
    }
}

DEF_DO_FUN(do_gratz)
{
	public_channel( &public_channel_table[sn_gratz], ch, argument );
}

DEF_DO_FUN(do_quote)
{
	public_channel( &public_channel_table[sn_quote], ch, argument );
}

/* RT question channel */
DEF_DO_FUN(do_question)
{
	public_channel( &public_channel_table[sn_question], ch, argument );
}

/* RT answer channel - uses same line as questions */
DEF_DO_FUN(do_answer)
{
	public_channel( &public_channel_table[sn_answer], ch, argument );
}

/* RT music channel */
DEF_DO_FUN(do_music)
{
	public_channel( &public_channel_table[sn_music], ch, argument );
}

/* clan channels */
DEF_DO_FUN(do_clantalk)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found;
    sh_int pos;
    
    if (NOT_AUTHED(ch) || IS_NPC(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if (!is_clan(ch) || !clan_table[ch->clan].active)
    {
        send_to_char("You aren't in a clan.\n\r",ch);
        return;
    }
    
    if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_use_clantalk)
    {
        send_to_char("Your rank is not high enough to use the clan channel.\n\r",ch);
        return;
    }
    
    if ( argument[0] == '\0' )
    {
        if (IS_SET(ch->comm,COMM_NOCLAN))
        {
            send_to_char("{lClan channel{x is now {LON{x.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_NOCLAN);
        }
        else
        {
            send_to_char("{lClan channel{x is now {LOFF{x.\n\r",ch);
            SET_BIT(ch->comm,COMM_NOCLAN);
        }
        return;
    }
    
    if (IS_SET(ch->penalty,PENALTY_NOCHANNEL))
    {
        send_to_char("The gods have revoked your channel privileges.\n\r",ch);
        return;
    }
    
    REMOVE_BIT(ch->comm,COMM_NOCLAN);

    argument = parse_url(argument);
    
    sprintf( buf, "{lYou clan {L'%s{L'\n\r{x", argument );
    send_to_char( buf, ch );
	if ( !IS_NPC(ch) )
		log_pers(ch->pcdata->clan_history, buf);
    if ( ch->pcdata && ch->pcdata->guiconfig.chat_window )
    {
        open_chat_tag( ch );
        send_to_char( buf, ch );
        close_chat_tag( ch );
    }

    argument = makedrunk(argument,ch);
    /* ACT is just unneccessary overhead here! Memnoch 03/98 */
    
    sprintf(buf,"{l%s {lclans {L'%s{L'{x\n\r",ch->name,argument);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (IS_PLAYING(d->connected)) &&
            d->character != ch && !IS_NPC(d->character) &&
            is_same_clan(ch,d->character) &&
            clan_table[d->character->clan].rank_list[d->character->pcdata->clan_rank].can_use_clantalk &&
            !IS_SET(d->character->comm,COMM_NOCLAN) &&
            !IS_SET(d->character->comm,COMM_QUIET) &&
            !NOT_AUTHED(d->character) )
        {
            found = FALSE;
            for (pos = 0; pos < MAX_FORGET; pos++)
            {
                if (IS_NPC(d->character))
                    break;
                if (d->character->pcdata->forget[pos] == NULL)
                    break;
                if (!str_cmp(ch->name,d->character->pcdata->forget[pos]))
                    found = TRUE; 
            }
            if (!found)
            {
                send_to_char(buf,d->character);
				if ( !IS_NPC(ch) )
					log_pers(d->character->pcdata->clan_history, buf);
                if ( d->character->pcdata &&
                        d->character->pcdata->guiconfig.chat_window )
                {
                    open_chat_tag( d->character );
                    send_to_char( buf, d->character );
                    close_chat_tag( d->character );
                }
            }
        }
    }
    
    return;
}

/* religion channels */
/*
DEF_DO_FUN(do_religion_talk)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found;
    sh_int pos;
    RELIGION_DATA *rel;
    
    if ( NOT_AUTHED(ch) )
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if ( IS_NPC(ch) )
    {
	if ( (rel = get_religion_of_guard(ch)) == NULL )
	    return;
    }
    else if ( (rel = get_religion(ch)) == NULL )
    {
        send_to_char("You aren't religious.\n\r",ch);
        return;
    }
    
    if ( !IS_NPC(ch) && !IS_IMMORTAL(ch) && !is_religion_member(ch) )
    {
        send_to_char( "Your rank is not high enough to use the religion channel.\n\r", ch);
        return;
    }
    
    if ( argument[0] == '\0' )
    {
        if (IS_SET(ch->comm,COMM_NOREL))
        {
            send_to_char("{lReligion channel{x is now {LON{x.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_NOREL);
        }
        else
        {
            send_to_char("{lReligion channel{x is now {LOFF{x.\n\r",ch);
            SET_BIT(ch->comm,COMM_NOREL);
        }
        return;
    }
    
    if (IS_SET(ch->penalty,PENALTY_NOCHANNEL))
    {
        send_to_char("The gods have revoked your channel privileges.\n\r",ch);
        return;
    }
    
    REMOVE_BIT(ch->comm, COMM_NOREL);
    
    sprintf( buf, "{9You proclaim {0'%s{0'\n\r{x", argument );
    send_to_char( buf, ch );
    argument = makedrunk(argument,ch);
    
    if ( IS_NPC(ch) )
	sprintf(buf,"{9%s proclaims {0'%s{0'{x\n\r",ch->short_descr,argument);
    else
	sprintf(buf,"{9%s proclaims {0'%s{0'{x\n\r",ch->name,argument);


    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (IS_PLAYING(d->connected)) &&
	     d->character != ch && !IS_NPC(d->character) &&
	     get_religion(d->character) == rel &&
	     is_religion_member(d->character) &&
            !IS_SET(d->character->comm,COMM_NOREL) &&
            !IS_SET(d->character->comm,COMM_QUIET) &&
            !NOT_AUTHED(d->character) )
        {
            found = FALSE;
            for (pos = 0; pos < MAX_FORGET; pos++)
            {
                if (IS_NPC(d->character))
                    break;
                if (d->character->pcdata->forget[pos] == NULL)
                    break;
                if (!str_cmp(ch->name,d->character->pcdata->forget[pos]))
                    found = TRUE; 
            }
            if (!found)
            {
                send_to_char(buf,d->character);
            }
        }
    }
    
    return;
}
*/

DEF_DO_FUN(do_immtalk)
{
	public_channel( &public_channel_table[sn_immtalk], ch, argument );
}

bool check_immtalk( CHAR_DATA *ch )
{
	return (is_granted_name(ch,"immtalk") || is_granted_name(ch,":"));
}

/* Function do_info corresponds to the player command info (no
argument) which toggles COMM_NOINFO.  As with the other
channels, if COMM_NOINFO or COMM_QUIET is set, INFO messages
will not be sent to that player.  -- Rimbol */
DEF_DO_FUN(do_info)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    
    if (argument[0] == '\0' )
    {
        if (IS_SET(ch->comm,COMM_NOINFO))
        {
            send_to_char("{1INFO channel{x is now {2ON{x.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_NOINFO);
        }
        else
        {
            send_to_char("{1INFO channel{x is now {2OFF{x.\n\r",ch);
            SET_BIT(ch->comm,COMM_NOINFO);
        }
    }
    else
        if ( get_trust(ch) >= VICEARCHON )
        {
            for ( d = descriptor_list; d != NULL; d = d->next )
            {
                victim = d->original ? d->original : d->character;
                
                if ((IS_PLAYING(d->connected)) &&
                    !IS_SET(victim->comm,COMM_NOINFO) &&
                    !IS_SET(victim->comm,COMM_QUIET) )
                {
                    sprintf(buf, "{1[INFO]{2: %s\n{x", parse_url(argument));
                    act_new( buf, victim, NULL, NULL, TO_CHAR, POS_SLEEPING );
                    
                    if ( victim->pcdata && victim->pcdata->guiconfig.chat_window )
                    {
                        open_chat_tag( victim );
                        act_new( buf, victim, NULL, NULL, TO_CHAR, POS_SLEEPING );
                        close_chat_tag( victim );
                    }
                }
            }
        }
        else
            do_groups(ch, argument);
}


DEF_DO_FUN(do_say)
{
    const char *mid1, *mid2, *scan;
    char buf[MAX_STRING_LENGTH];
    
    if (is_affected(ch, gsn_slash_throat) && !IS_IMMORTAL(ch))
    {
       send_to_char("The cut in your throat prevents you from speaking.\n\r",ch);
       return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Say what?\n\r", ch );
        return;
    }
    
    for (scan = argument; *scan; scan++);
    switch (*(--scan)) {
    case '?':
        mid1="ask";
        mid2 = "asks";
        break;
    case '!':
        mid1="exclaim";
        mid2 = "exclaims";
        break;
    case '(':
        scan--;
        if(*scan == ':' || *scan == ';' || *scan == '8' || *scan == '=')
        {
            mid1= "pout";
            mid2 = "pouts";
        }
        else
        {
            mid1="say";
            mid2 = "says";
        }
        break;
        /*
        case ')':
        scan--;
        if(*scan == ':' || *scan == ';' || *scan == '8' || *scan == '=')
        {
        mid1= "say with a grin";
        mid2 = "says with a grin";
        }
        else
        {
        mid1="say";
        mid2 = "says";
        }
        break;
        */
    default:
        mid1="say";
        mid2 = "says";
        break;
    }
    
    if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
    {
        affect_strip( ch, gsn_hide );
        REMOVE_BIT( ch->affect_field, AFF_HIDE );
        send_to_char( "You come out of hiding.\n\r", ch );
    }
    
    sprintf(buf, "{sYou %s {S'$T{S'{x", mid1);
    
    nt_act( buf, ch, NULL, parse_url(argument), TO_CHAR );
    argument = makedrunk(argument,ch);
    sprintf(buf, "{s$n {s%s {S'$T{S'{x", mid2);
    if (NOT_AUTHED(ch))
        nt_act( buf, ch, NULL, parse_url(argument), TO_ROOM_UNAUTHED );
    else
        nt_act( buf, ch, NULL, parse_url(argument), TO_ROOM );

    if ( !IS_NPC(ch) )
    {
        CHAR_DATA *mob, *mob_next;
        for ( mob = ch->in_room->people; mob != NULL; mob = mob_next )
        {
            mob_next = mob->next_in_room;
            if ( can_trigger(mob, TRIG_SPEECH) )
                mp_act_trigger( argument, mob, ch, NULL,0, NULL,0, TRIG_SPEECH );
        }

        op_speech_trigger( argument,ch);
    }
    return;
}


DEF_DO_FUN(do_shout)
{
    DESCRIPTOR_DATA *d;
    bool found;
    sh_int pos;
    char arg_buf[MSL];

    if (NOT_AUTHED(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if (argument[0] == '\0' )
    {
        if (IS_SET(ch->comm,COMM_SHOUTSOFF))
        {
            send_to_char("You can hear {ushouts{x again.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_SHOUTSOFF);
        }
        else
        {
            send_to_char("You will no longer hear {ushouts{x.\n\r",ch);
            SET_BIT(ch->comm,COMM_SHOUTSOFF);
        }
        return;
    }
    if ( !IS_NPC(ch) && ch->level < 3 && ch->pcdata->remorts == 0 )
    {
        send_to_char( "Your lungs aren't strong enough to shout.\n\r", ch );
        return;
    }
    
    if ( IS_SET(ch->penalty,PENALTY_NOSHOUT) )
    {
        send_to_char( "You can't shout.\n\r", ch );
        return;
    }
    
    if ( IS_SET(ch->penalty,PENALTY_NOCHANNEL) )
    {
        send_to_char( "The gods have revoked your channel privledges.\n\r", ch );
        return;
    }
    
    REMOVE_BIT(ch->comm,COMM_SHOUTSOFF);
    
    if (is_affected(ch, gsn_slash_throat) && !IS_IMMORTAL(ch))
    {
       send_to_char("The cut in your throat prevents you from speaking.\n\r",ch);
       return;
    }

    WAIT_STATE( ch, 12 );
    
    strcpy(arg_buf, argument);
    smash_beep_n_blink(arg_buf);
    act( "{uYou shout {U'$T{U'{x", ch, NULL, arg_buf, TO_CHAR );
    argument = makedrunk(arg_buf, ch);
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA *victim;
        
        victim = d->original ? d->original : d->character;
        
        if ((IS_PLAYING(d->connected)) &&
            d->character != ch &&
            !IS_SET(victim->comm, COMM_SHOUTSOFF) &&
            !IS_SET(victim->comm, COMM_QUIET) &&
            !NOT_AUTHED(victim) )
        {
            found = FALSE;  
            for (pos = 0; pos < MAX_FORGET; pos++)
            {
                if (IS_NPC(victim))
                    break;
                if (victim->pcdata->forget[pos] == NULL)
                    break;
                if (!str_cmp(ch->name,victim->pcdata->forget[pos]))
                    found = TRUE; 
            }
            if (!found)
            {
                act("{u$n{u shouts {U'$t{U'{x",ch,argument,d->character,TO_VICT);
            }
        }
    }
    
    return;
}

void act_tell_char( CHAR_DATA *ch, CHAR_DATA *victim, const char *argument )
{
    char buf[MAX_STRING_LENGTH];

    sprintf( buf, "{t$n {ttells you {T'%s{T'{x", argument );
	nt_act( buf, ch, NULL, victim, TO_VICT );
}

void tell_char( CHAR_DATA *ch, CHAR_DATA *victim, const char *argument )
{
    char buf[MAX_STRING_LENGTH];
    bool found;
    int pos;

    
    if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF))
        && !IS_IMMORTAL(ch))
    {
        act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR );
        return;
    }
    
    if (victim->desc != NULL && !IS_NPC(victim))
    {
        found = FALSE;  
        for (pos = 0; pos < MAX_FORGET; pos++)
        {
            if (victim->pcdata->forget[pos] == NULL)
                break;
            if (!str_cmp(ch->name,victim->pcdata->forget[pos]))
                found = TRUE; 
        }
        
        if (found)
        {
            printf_to_char(ch, "%s is ignoring you.\n\r", victim->name);
            return;
        }
    }

	
	sprintf( buf, "{tYou tell %s {T'%s{T'{x\n\r", ( IS_NPC(victim) ? victim->short_descr : victim->name ), argument );
	send_to_char( buf, ch );

    if ( ch->pcdata && ch->pcdata->guiconfig.chat_window )
    {
        open_chat_tag( ch );
        send_to_char( buf, ch );
        close_chat_tag( ch );
    }

	if ( !IS_NPC(ch) )
		log_pers(ch->pcdata->tell_history, buf);
	argument = makedrunk(argument,ch);
	
	
        /* send as regular */
        sprintf(buf,"{t%s {ttells you {T'%s{T'{x\n\r", ( IS_NPC(ch) ? ch->short_descr : ch->name ), argument);

        /* we'll add to history whether they're available or not */
        if (!IS_NPC(victim) )
        {
                log_pers(victim->pcdata->tell_history, buf);
        }

    if ( victim->desc == NULL && !IS_NPC(victim))
    {
        act("$N seems to have misplaced $S link...$E'll get your tell upon returning.",
            ch,NULL,victim,TO_CHAR);
		victim->pcdata->new_tells=TRUE;
    }
	else if (IS_SET(victim->comm,COMM_AFK))
    {
	/* NPC's shouldn't be able to go AFK, but .. safety is worth the extra check! */
        if (!IS_NPC(victim))
        {
            act("$E is AFK, and may view your tell upon returning.",ch,NULL,victim,TO_CHAR);
			victim->pcdata->new_tells=TRUE;
        }
    }
	else if ( !IS_NPC(victim) /* switched imms */
	 && victim->desc != NULL
	 && IS_WRITING_NOTE(victim->desc->connected) )
    {
        act("$N is writing a note, but your tell will go through when $E finishes.",
            ch,NULL,victim,TO_CHAR);
		victim->pcdata->new_tells=TRUE;
    }
    else
    {
	    /* send as regular */
	    send_to_char(buf, victim);
        if ( victim->pcdata && victim->pcdata->guiconfig.chat_window )
        {
            open_chat_tag( victim );
            send_to_char( buf, victim );
            close_chat_tag( victim );
        }
    }   
	
    
	
    if( victim != ch )
        victim->reply = ch;
    
    if ( !IS_NPC(ch) && IS_NPC(victim) && HAS_TRIGGER(victim,TRIG_SPEECH) )
        mp_act_trigger( argument, victim, ch, NULL,0, NULL,0, TRIG_SPEECH );
}

DEF_DO_FUN(do_tell)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    
    if ( IS_SET(ch->penalty,PENALTY_NOTELL) || IS_SET(ch->comm,COMM_DEAF))
    {
        send_to_char( "Your message didn't get through.\n\r", ch );
        return;
    }
    
    if ( IS_SET(ch->comm, COMM_QUIET) )
    {
        send_to_char( "You must turn off quiet mode first.\n\r", ch);
        return;
    }
    
    if (IS_SET(ch->comm,COMM_DEAF))
    {
        send_to_char("You must turn off deaf mode first.\n\r",ch);
        return;
    }
    
    argument = one_argument( argument, arg );
    
    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        send_to_char( "Tell whom what?\n\r", ch );
        return;
    }
    
    /*
    * Can tell to PC's anywhere, but NPC's only in same room.
    * -- Furey
    */
    helper_visible = TRUE;
    victim = get_char_world( ch, arg );

    if ( victim == NULL )
	 /* no harm in sending tells to un-authed..
	 || ( IS_NPC(victim) && victim->in_room != ch->in_room ) 
	 || (!NOT_AUTHED(ch) && NOT_AUTHED(victim) && !IS_IMMORTAL(ch) )
	 */
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( NOT_AUTHED(ch) && !NOT_AUTHED(victim)
	 && !IS_IMMORTAL(victim) )
    {
        send_to_char( "They can't hear you because you are not authorized.\n\r", ch);
        send_to_char( "To answer a tell sent to you, use 'reply'.\n\r", ch);
        return;
    }
    
    tell_char( ch, victim, parse_url(argument) );

}


DEF_DO_FUN(do_reply)
{
    CHAR_DATA *victim;
    
    if ( IS_SET(ch->penalty,PENALTY_NOTELL) )
    {
        send_to_char( "Your message didn't get through.\n\r", ch );
        return;
    }
    
    if ( ( victim = ch->reply ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    /*
    if (NOT_AUTHED(ch) && !NOT_AUTHED(victim) && !IS_IMMORTAL(victim) )
    {
    send_to_char( "They can't hear you because you are not authorized.\n\r", ch);
    return;
    }
    */
    
    helper_visible = TRUE;

    tell_char( ch, victim, argument );
}

DEF_DO_FUN(do_yell)
{
    DESCRIPTOR_DATA *d;
    bool found;
    sh_int pos;
    char arg_buf[MSL];
    
    if (NOT_AUTHED(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if ( !IS_NPC(ch) && ch->level < 3 && ch->pcdata->remorts == 0 )
    {
        send_to_char( "Your lungs aren't strong enough to yell.\n\r", ch );
        return;
    }
    
    if ( IS_SET(ch->penalty,PENALTY_NOSHOUT) )
    {
        send_to_char( "You can't yell.\n\r", ch );
        return;
    }
    
    if (is_affected(ch, gsn_slash_throat) && !IS_IMMORTAL(ch))
    {
       send_to_char("The cut in your throat prevents you from speaking.\n\r",ch);
       return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Yell what?\n\r", ch );
        return;
    }
    
    strcpy(arg_buf, argument);
    smash_beep_n_blink(arg_buf);
    argument = arg_buf;
        
    nt_act("{uYou yell {U'$t{U'{x",ch,argument,NULL,TO_CHAR);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ((IS_PLAYING(d->connected))
            &&   d->character != ch
            &&   d->character->in_room != NULL
            &&   d->character->in_room->area == ch->in_room->area
            &&   !IS_SET(d->character->comm,COMM_QUIET) &&
            !NOT_AUTHED(d->character)  )
        {
            found = FALSE; 
            for (pos = 0; pos < MAX_FORGET; pos++)
            {
                if (IS_NPC(d->character))
                    break;
                if (d->character->pcdata->forget[pos] == NULL)
                    break;
                if (!str_cmp(ch->name,d->character->pcdata->forget[pos]))
                    found = TRUE; 
            }
            if (!found)
            {
                nt_act("{u$n{u yells {U'$t{U'{x",ch,argument,d->character,TO_VICT);
            }
        }
    }
    
    return;
}

DEF_DO_FUN(do_emote)
{

    bool keep_space = (argument[0] != '\'');

    if ( !IS_NPC(ch) && IS_SET(ch->penalty,PENALTY_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }
    
    if (NOT_AUTHED(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }
    
    if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
    {
        affect_strip( ch, gsn_hide );
        REMOVE_BIT( ch->affect_field, AFF_HIDE );
        send_to_char( "You come out of hiding.\n\r", ch );
    }
    
    nt_act(keep_space ? "$n $T" : "$n$T", ch, NULL, argument, TO_ROOM );
    nt_act(keep_space ? "$n $T" : "$n$T", ch, NULL, argument, TO_CHAR );
    return;
}

DEF_DO_FUN(do_pmote)
{
    CHAR_DATA *vch;
    const char *letter, *name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    size_t matches = 0;
    bool keep_space = (argument[0] != '\'');
    
    if ( !IS_NPC(ch) && IS_SET(ch->penalty,PENALTY_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }
    
    if ( IS_AFFECTED( ch, AFF_HIDE ) && !IS_AFFECTED( ch, AFF_SNEAK ) )
    {
        affect_strip( ch, gsn_hide );
        REMOVE_BIT( ch->affect_field, AFF_HIDE );
        send_to_char( "You come out of hiding.\n\r", ch );
    }
    
    nt_act(keep_space ? "$n $t" : "$n$t", ch, argument, NULL, TO_CHAR );
    
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;
        
        if ((letter = strstr(argument,vch->name)) == NULL)
        {
            nt_act(keep_space ? "$N $t" : "$N$t",vch,argument,ch,TO_CHAR);
            continue;
        }
        
        strcpy(temp,argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        name = vch->name;
        
        for (; *letter != '\0'; letter++)
        {
            if ((*letter == '\'') && 
				(matches == strlen(vch->name)))
            {
                strcat(temp,"r");
                continue;
            }
            
            if (*letter == 's' && matches == strlen(vch->name))
            {
                matches = 0;
                continue;
            }
            
            if (matches == strlen(vch->name))
            {
                matches = 0;
            }
            
            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == strlen(vch->name))
                {
                    strcat(temp,"you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat(last,letter,1);
                continue;
            }
            
            matches = 0;
            strcat(temp,last);
            strncat(temp,letter,1);
            last[0] = '\0';
            name = vch->name;
        }
        
        nt_act(keep_space ? "$N $t" : "$N$t",vch,temp,ch,TO_CHAR);
    }
    
    return;
}


/*
* All the posing stuff.
*/

#define MAX_POSE 17

struct  pose_table_type
{
    char *  message[8];
};

const   struct  pose_table_type pose_table  [MAX_POSE]  =
{
    {
        {
            "You show your bulging muscles.",
                "$n shows $s bulging muscles.",
                "You perform a small card trick.",
                "$n performs a small card trick.",
                "You feel very holy.",
                "$n looks very holy.",
                "You sizzle with energy.",
                "$n sizzles with energy.",
        }
    },
        
    {
        {
            "You crack nuts between your fingers.",
                "$n cracks nuts between $s fingers.",
                "You wiggle your ears alternately.",
                "$n wiggles $s ears alternately.",
                "You nonchalantly turn wine into water.",
                "$n nonchalantly turns wine into water.",
                "You turn into a butterfly, then return to your normal shape.",
                "$n turns into a butterfly, then returns to $s normal shape.",
        }
    },
        
    {
        {
            "You grizzle your teeth and look mean.",
                "$n grizzles $s teeth and looks mean.",
                "You nimbly tie yourself into a knot.",
                "$n nimbly ties $mself into a knot.",
                "A halo appears over your head.",
                "A halo appears over $n's head.",
                "Blue sparks fly from your fingers.",
                "Blue sparks fly from $n's fingers.",
        }
    },
        
    {
        {
            "You hit your head, and your eyes roll.",
                "$n hits $s head, and $s eyes roll.",
                "You juggle with daggers, apples, and eyeballs.",
                "$n juggles with daggers, apples, and eyeballs.",
                "You recite words of wisdom.",
                "$n recites words of wisdom.",
                "Little red lights dance in your eyes.",
                "Little red lights dance in $n's eyes.",
        }
    },
        
    {
        {
            "Crunch, crunch -- you munch a bottle.",
                "Crunch, crunch -- $n munches a bottle.",
                "You steal the underwear off every person in the room.",
                "Your underwear is gone!  $n stole it!",
                "Deep in prayer, you levitate.",
                "Deep in prayer, $n levitates.",
                "A slimy green monster appears before you and bows.",
                "A slimy green monster appears before $n and bows.",
        }
    },
        
    {
        {
            "... 98, 99, 100 ... you do pushups.",
                "... 98, 99, 100 ... $n does pushups.",
                "The dice roll ... and you win again.",
                "The dice roll ... and $n wins again.",
                "An angel consults you.",
                "An angel consults $n.",
                "You turn everybody into a little pink elephant.",
                "You are turned into a little pink elephant by $n.",
        }
    },
        
    {
        {
            "Arnold Schwarzenegger admires your physique.",
                "Arnold Schwarzenegger admires $n's physique.",
                "You count the money in everyone's pockets.",
                "Check your money, $n is counting it.",
                "Your body glows with an unearthly light.",
                "$n's body glows with an unearthly light.",
                "A small ball of light dances on your fingertips.",
                "A small ball of light dances on $n's fingertips.",
        }
    },
        
    {
        {
            "Watch your feet, you are juggling granite boulders.",
                "Watch your feet, $n is juggling granite boulders.",
                "You balance a pocket knife on your tongue.",
                "$n balances a pocket knife on your tongue.",
                "A spot light hits you.",
                "A spot light hits $n.",
                "Smoke and fumes leak from your nostrils.",
                "Smoke and fumes leak from $n's nostrils.",
        }
    },
        
    {
        {
            "Oomph!  You squeeze water out of a granite boulder.",
                "Oomph!  $n squeezes water out of a granite boulder.",
                "You produce a coin from everyone's ear.",
                "$n produces a coin from your ear.",
                "Everyone levitates as you pray.",
                "You levitate as $n prays.",
                "The light flickers as you rap in magical languages.",
                "The light flickers as $n raps in magical languages.",
        }
    },
        
    {
        {
            "You pick your teeth with a spear.",
                "$n picks $s teeth with a spear.",
                "You step behind your shadow.",
                "$n steps behind $s shadow.",
                "A cool breeze refreshes you.",
                "A cool breeze refreshes $n.",
                "Your head disappears.",
                "$n's head disappears.",
        }
    },
        
    {
        {
            "Everyone is swept off their feet by your hug.",
                "You are swept off your feet by $n's hug.",
                "Your eyes dance with greed.",
                "$n's eyes dance with greed.",
                "The sun pierces through the clouds to illuminate you.",
                "The sun pierces through the clouds to illuminate $n.",
                "A fire elemental singes your hair.",
                "A fire elemental singes $n's hair.",
        }
    },
        
    {
        {
            "Your karate chop splits a tree.",
                "$n's karate chop splits a tree.",
                "You deftly steal everyone's weapon.",
                "$n deftly steals your weapon.",
                "The ocean parts before you.",
                "The ocean parts before $n.",
                "The sky changes color to match your eyes.",
                "The sky changes color to match $n's eyes.",
        }
    },
        
    {
        {
            "A strap of your armor breaks over your mighty thews.",
                "A strap of $n's armor breaks over $s mighty thews.",
                "The Grey Mouser buys you a beer.",
                "The Grey Mouser buys $n a beer.",
                "A thunder cloud kneels to you.",
                "A thunder cloud kneels to $n.",
                "The stones dance to your command.",
                "The stones dance to $n's command.",
        }
    },
        
    {
        {
            "A boulder cracks at your frown.",
                "A boulder cracks at $n's frown.",
                "Everyone's pocket explodes with your fireworks.",
                "Your pocket explodes with $n's fireworks.",
                "The Burning Man speaks to you.",
                "The Burning Man speaks to $n.",
                "The heavens and grass change colour as you smile.",
                "The heavens and grass change colour as $n smiles.",
        }
    },
        
    {
        {
            "Mercenaries arrive to do your bidding.",
                "Mercenaries arrive to do $n's bidding.",
                "Everyone discovers your dagger a centimeter from their eye.",
                "You discover $n's dagger a centimeter from your eye.",
                "An eye in a pyramid winks at you.",
                "An eye in a pyramid winks at $n.",
                "Everyone's clothes are transparent, and you are laughing.",
                "Your clothes are transparent, and $n is laughing.",
        }
    },
        
    {
        {
            "Four matched Percherons bring in your chariot.",
                "Four matched Percherons bring in $n's chariot.",
                "Where did you go?",
                "Where did $n go?",
                "Valentine Michael Smith offers you a glass of water.",
                "Valentine Michael Smith offers $n a glass of water.",
                "A black hole swallows you.",
                "A black hole swallows $n.",
        }
    },
        
    {
        {
            "Atlas asks you to relieve him.",
                "Atlas asks $n to relieve him.",
                "Click.",
                "Click.",
                "A god appears and gives you a staff.",
                "A god appears and gives $n a staff.",
                "The world shimmers in time with your whistling.",
                "The world shimmers in time with $n's whistling.",
        }
    }
        
};


DEF_DO_FUN(do_pose)
{
    int level;
    int pose;
	int clas, warrior=0, thief=0, cleric=0, mage=0;
    
    if ( IS_NPC(ch) )
        return;

	switch (ch->class)
	{
	case 0: case 4: warrior = 10; break;
	case 1: thief = 10; break;
	case 2: cleric = 10; break;
	case 3: mage = 10; break;
	case 5: warrior = 9; mage = 2; break;
	case 6: warrior = 9; cleric = 3; break;
	case 7: warrior = 7; thief = 7; break;
	case 8: warrior = 7; thief = 6; mage = 2; break;
	case 9: warrior = 5; cleric = 8; break;
	case 10: cleric = 7; mage = 7; break;
	case 11: mage = 9; thief = 4; break;
	case 12: warrior = 6; thief = 6; break;
	case 13: warrior = 5; thief = 5; cleric = 5; mage = 5; break;
	case 14: mage = 9; cleric = 3; break;
    }

	clas = number_range(1, warrior+thief+cleric+mage);
	if (clas <= warrior)
	{
		clas = 0;
		level = warrior;
	} else {
	clas-=warrior;
	if (clas <= thief)
	{
		clas = 1;
		level = thief;
	} else {
	clas-=thief;
	if (clas <= cleric)
	{
		clas = 2;
		level = cleric;
	} else {
	clas-=cleric;
	if (clas <= mage)
	{
		clas = 3;
		level = mage;
	} else
		return;
	}}}

	level = MAX_POSE*((ch->level+2*ch->pcdata->remorts) * (2+level))/1400;
	level = URANGE(0, level, MAX_POSE-1);
    pose  = number_range(0, level);

    act(pose_table[pose].message[2*clas+0], ch, NULL, NULL, TO_CHAR );
    act(pose_table[pose].message[2*clas+1], ch, NULL, NULL, TO_ROOM );
}

void log_bugs(char *str)
{
    FILE * fp = fopen("buglog.txt", "a");

    if(fp == NULL) 
        return;
}    

DEF_DO_FUN(do_bug)
{
    //append_file( ch, BUG_FILE, argument );
    //send_to_char( "Bug logged.\n\r", ch );
    send_to_char("Please report bugs on the Bugs board, using the {gboard{x and {gnote{x commands.\n\r", ch);
    return;
}

DEF_DO_FUN(do_typo)
{
    //append_file( ch, TYPO_FILE, argument );
    //send_to_char( "Typo logged.\n\r", ch );
    send_to_char("Please report typos on the Bugs board, using the {gboard{x and {gnote{x commands.\n\r", ch);
    return;
}

DEF_DO_FUN(do_rent)
{
    send_to_char( "There is no rent here.  Just quit.\n\r", ch );
    return;
}

DEF_DO_FUN(do_qui)
{
    send_to_char( "If you want to QUIT, you have to spell it out.\n\r", ch );
    return;
}

DEF_DO_FUN(do_quit)
{
    if ( IS_NPC(ch) || ch->pcdata == NULL )
        return;
    
    if ((ch->position == POS_FIGHTING) || ch->fighting)
    {
        send_to_char( "No way! You are fighting.\n\r", ch );
        return;
    }
    
    if ( ch->position  < POS_STUNNED  )
    {
        send_to_char( "You're not DEAD yet.\n\r", ch );
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
	send_to_char("Not unless your master tells you to quit!\n\r", ch);
	return;
    }

    if (ch->pcdata->pkill_timer > 0)
    {
        send_to_char( "You can't quit right now.\n\r", ch);
        return;
    }

    if (IS_SET( ch->act, PLR_WAR ))
   {
     send_to_char( "You cannot quit out of a warfare!\n\r", ch);
     return;
   }

    quit_char( ch );
}

void quit_char( CHAR_DATA *ch )
{
    DESCRIPTOR_DATA *d,*d_next;
    int id;
    AUTH_LIST *old_auth;    
    
    if (IS_SET( ch->act, PLR_WAR ))
        war_remove( ch, FALSE );
    
    if ( !NOT_AUTHED( ch ) ) 
        remove_from_auth( ch->name );
    else
    {
        old_auth = get_auth_name( ch->name );
        if( old_auth != NULL 
            && (old_auth->state == AUTH_ONLINE || old_auth->state == AUTH_LINK_DEAD) )
            old_auth->state = AUTH_OFFLINE; /* Logging off */
    }
    
    send_to_char("Have a good journey.\n\r",ch);
    sprintf( log_buf, "%s has quit.", ch->name );
    log_string( log_buf );
    wiznet("$N rejoins the real world.",ch,NULL,WIZ_LOGINS,0,get_trust(ch));
    
    sprintf(log_buf, "%s has left the game.", ch->name);
    info_message_new(ch, log_buf, FALSE, FALSE);

    strcat(log_buf, "\n");
    /*
    * After extract_char the ch is no longer valid!
    */
    
    /* Free note that might be there somehow */
    if (ch->pcdata->in_progress)
        free_note (ch->pcdata->in_progress);
    
    /* check if char had quest */
    if ( IS_SET(ch->act, PLR_QUESTOR) || IS_SET(ch->act, PLR_QUESTORHARD) )
    {
	REMOVE_BIT( ch->act, PLR_QUESTOR );
	REMOVE_BIT( ch->act, PLR_QUESTORHARD );
	ch->pcdata->quest_failed++;
    }

    ap_quit_trigger(ch);
    if (!IS_SET(ch->in_room->room_flags, ROOM_BOX_ROOM))
       quit_save_char_obj( ch );
    else
    {
       char_from_room(ch);/*this will force a quit_save_char_obj,
                            don't want to save twice
                            -Vodur*/
       char_to_room( ch, get_room_index( ROOM_VNUM_RECALL ));
    }

    remove_bounty(ch);
    id = ch->id;
    d = ch->desc;
    extract_char( ch, TRUE );
    if ( d != NULL )
        close_socket( d );
    else
        logpf("quit_char: no descriptor for %s", ch->name);
    
    /* toast evil cheating bastards */
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        CHAR_DATA *tch;
        
        d_next = d->next;
        tch = d->original ? d->original : d->character;
        if (tch && tch->id == id)
        {
            bugf("quit_char: %s still referenced via descriptor", tch->name);
            extract_char(tch,TRUE);
            close_socket(d);
        }
    }
    
    return;
}

DEF_DO_FUN(do_save)
{
    send_to_char("Your character is saved automatically here.\n\r", ch);
}

DEF_DO_FUN(do_follow)
{
    /* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Follow whom?\n\r", ch );
        return;
    }
    
    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL )
    {
        act( "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
        return;
    }
    
    if ( victim == ch )
    {
        if ( ch->master == NULL )
        {
            send_to_char( "You already follow yourself.\n\r", ch );
            return;
        }
        stop_follower(ch);
        return;
    }
    
    if ( (PLR_ACT(ch, PLR_WAR) || PLR_ACT(victim, PLR_WAR))
	 && !is_same_team(ch, victim) )
    {
	send_to_char("You cannot group during Warfare.\n\r",ch);
	return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOFOLLOW) && !IS_IMMORTAL(ch))
    {
        act("$N doesn't seem to want any followers.\n\r",
            ch,NULL,victim, TO_CHAR);
        return;
    }
    
    REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    
    if ( ch->master != NULL )
        stop_follower( ch );
    
    add_follower( ch, victim );
    return;
}


void add_follower( CHAR_DATA *ch, CHAR_DATA *master )
{
    if ( ch->master != NULL )
    {
        bug( "Add_follower: non-null master.", 0 );
        return;
    }
    
    ch->master        = master;
    ch->leader        = NULL;
    
    act_see( "$n now follows you.", ch, NULL, master, TO_VICT );
    act( "You now follow $N.",  ch, NULL, master, TO_CHAR );
    
    return;
}



void stop_follower( CHAR_DATA *ch )
{
    CHAR_DATA *master;
    bool show_master = FALSE;

    if ( (master = ch->master) == NULL )
    {
        bug( "Stop_follower: null master.", 0 );
        return;
    }

    if ( is_same_group(ch, master) )
	show_master = TRUE;

    affect_strip_flag( ch, AFF_CHARM );
    
    if (ch->master->pet == ch)
        ch->master->pet = NULL;
    
    ch->master = NULL;
    ch->leader = NULL;

    if ( show_master )
	act( "$n stops following you.",     ch, NULL, master, TO_VICT    );
    act( "You stop following $N.",      ch, NULL, master, TO_CHAR    );

    return;
}

/* nukes charmed monsters and pets */
void nuke_pets( CHAR_DATA *ch )
{
    CHAR_DATA *pet;
    
    if ((pet = ch->pet) != NULL)
    {
        stop_follower(pet);
        if (pet->in_room != NULL)
            act("$N slowly fades away.",ch,NULL,pet,TO_NOTVICT);
        extract_char(pet,TRUE);
    }
    ch->pet = NULL;
    
    return;
}



void die_follower( CHAR_DATA *ch, bool preservePets )
{
    CHAR_DATA *fch;
    
    if (ch->master != NULL)
    {   
        if (ch->master->pet == ch)
            ch->master->pet = NULL;
        stop_follower( ch );
    }
    
    ch->leader = NULL;
    
    for ( fch = char_list; fch != NULL; fch = fch->next )
    {
        if ( preservePets && IS_NPC(fch) && IS_AFFECTED(fch, AFF_CHARM) )
            continue;
        if ( fch->master == ch )
            stop_follower( fch );
        if ( fch->leader == ch )
            fch->leader = NULL;
    }
    
    return;
}

DEF_DO_FUN(do_order)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;
    
    argument = one_argument( argument, arg );
    one_argument(argument,arg2);
    
    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        send_to_char( "Order whom to do what?\n\r", ch );
        return;
    }
    
    if ( IS_AFFECTED( ch, AFF_CHARM ) )
    {
        send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
        return;
    }
    
    if (!can_order(arg2, NULL))
    {
        send_to_char("You cannot order that.\n\r",ch);
        return;
    }
    
    if ( !str_cmp( arg, "all" ) )
    {
        fAll   = TRUE;
        victim = NULL;
    }
    else
    {
        fAll   = FALSE;
        if ( ( victim = get_char_room( ch, arg ) ) == NULL )
        {
            send_to_char( "They aren't here.\n\r", ch );
            return;
        }
        
        if ( victim == ch )
        {
            send_to_char( "Aye aye, right away!\n\r", ch );
            return;
        }
        
        if ( !IS_AFFECTED(victim, AFF_CHARM)
	     || victim->master != ch
	     || IS_IMMORTAL(victim)
	     || !can_order(arg2, victim) )
        {
            send_to_char( "Do it yourself!\n\r", ch );
            return;
        }
    }
    
    found = FALSE;
    for ( och = ch->in_room->people; och != NULL; och = och_next )
    {
        och_next = och->next_in_room;
        
        if ( IS_AFFECTED(och, AFF_CHARM)
	     && och->master == ch
	     && (fAll || och == victim) )
        {
            found = TRUE;
	    if ( can_order(arg2, och) )
	    {
		sprintf( buf, "$n orders you to '%s'.", argument );
		act( buf, ch, NULL, och, TO_VICT );
		interpret( och, argument );
	    }
        }
    }
    
    if ( found )
    {
        WAIT_STATE(ch,PULSE_VIOLENCE);
        send_to_char( "Ok.\n\r", ch );
    }
    else
        send_to_char( "You have no followers here.\n\r", ch );
    return;
}

const char* ch_name( CHAR_DATA *ch )
{
    if ( !ch )
        return "";
    return IS_NPC(ch) ? remove_color(ch->short_descr) : ch->name;
}

void show_group_member( CHAR_DATA *ch, CHAR_DATA *gch )
{
    char buf[MAX_STRING_LENGTH];
    char hp_col, mn_col, mv_col;   /* Colours that vary depending on the group member's current hp/mana/mv */
   
    hp_col = (gch->hit >= gch->max_hit) ? 'W' :
        (gch->hit >= gch->max_hit*.85) ? 'G' :
        (gch->hit >= gch->max_hit*.66) ? 'g' :
        (gch->hit >= gch->max_hit*.50) ? 'Y' :
        (gch->hit >= gch->max_hit*.33) ? 'y' :
        (gch->hit >= gch->max_hit*.16) ? 'R' : 'r';

    mn_col = (gch->mana >= gch->max_mana) ? 'W' :
        (gch->mana >= gch->max_mana*.85) ? 'G' :
        (gch->mana >= gch->max_mana*.66) ? 'g' :
        (gch->mana >= gch->max_mana*.50) ? 'Y' :
        (gch->mana >= gch->max_mana*.33) ? 'y' :
        (gch->mana >= gch->max_mana*.16) ? 'R' : 'r';

    mv_col = (gch->move >= gch->max_move) ? 'W' :
        (gch->move >= gch->max_move*.85) ? 'G' :
        (gch->move >= gch->max_move*.66) ? 'g' :
        (gch->move >= gch->max_move*.50) ? 'Y' :
        (gch->move >= gch->max_move*.33) ? 'y' :
        (gch->hit >= gch->max_hit*.16) ? 'R' : 'r';

    sprintf( buf,
        "[%3d %.3s] %-18s {%c%5d{x/%-5d hp {%c%5d{x/%-5d mn {%c%5d{x/%-5d mv  %s%s%s%s%s%s%s%s %5d etl\n\r",
        gch->level,
        !IS_NPC(gch) ? class_table[gch->class].who_name : IS_AFFECTED(gch, AFF_CHARM) ? ch_name(gch->leader) : "Mob",
        ch_name(gch),
        hp_col, gch->hit,   gch->max_hit,
        mn_col, gch->mana,  gch->max_mana,
        mv_col, gch->move,  gch->max_move,
       /* Shows what spells you can help your group with */
        NPC_OFF(gch, OFF_RESCUE) || PLR_ACT(gch, PLR_AUTORESCUE) ? "{WR{x" : " ",
        is_affected(gch, gsn_bless) || is_affected(gch, gsn_prayer) ? "{WB{x" : get_skill(ch, gsn_bless) > 1 ? "{Rb{x" : " ",
        IS_AFFECTED(gch, AFF_FLYING) ? "{WF{x" : get_skill(ch, gsn_fly) > 1 ? "{Rf{x" : " ",
        IS_AFFECTED(gch, AFF_GIANT_STRENGTH) ? "{WG{x" : get_skill(ch, gsn_giant_strength) > 1 ? "{Rg{x" : " ",
        IS_AFFECTED(gch, AFF_HASTE) ? "{WH{x" : !IS_AFFECTED(gch, AFF_SLOW) && get_skill(ch, gsn_haste) > 1 ? "{Rh{x" : " ",
        IS_AFFECTED(gch, AFF_SANCTUARY) ? "{WS{x" : get_skill(ch, gsn_sanctuary) > 1 ? "{Rs{x" : " ",
        is_affected(gch, gsn_war_cry) ? "{WW{x" : get_skill(ch, gsn_war_cry) > 1 ? "{Rw{x" : " ",
        IS_AFFECTED(gch, AFF_BERSERK) ? "{WZ{x" : get_skill(ch, gsn_frenzy) > 1 ? "{Rz{x" : " ",

        (IS_NPC(gch) || IS_HERO(gch)) ? 0 : (gch->level+1) * exp_per_level(gch) - gch->exp
    );
    send_to_char( buf, ch );
}

DEF_DO_FUN(do_group)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    const char *remain;

    remain = one_argument( argument, arg );
    
    // show group
    if ( arg[0] == '\0' )
    {
        CHAR_DATA *gch;
        CHAR_DATA *leader;
        
        leader = (ch->leader != NULL) ? ch->leader : ch;
        sprintf( buf, "%s's group:\n\r", leader->name );
        send_to_char( buf, ch );

        // show group members in room first to ensure same targeting order as for other commands
        if ( ch->in_room != NULL )
        {
            for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
            {
                if ( is_same_group( gch, ch ) )
                    show_group_member( ch, gch );
            }
        }
        // afterwards pick up all group members not in room
        for ( gch = char_list; gch != NULL; gch = char_list_next_char(gch) )
        {
            if ( is_same_group( gch, ch ) && ch->in_room != gch->in_room)
                show_group_member( ch, gch );
        }
        return;
    }
    
    // remove from group
    if ( !strcmp(arg, "remove") )
    {
        victim = get_char_group( ch, remain );
        if ( !victim )
        {
            send_to_char( "Nobody like that in your group.\n\r", ch );
            return;
        }
        if ( victim->leader != ch )
        {
            act_new("$N does not listen to you.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
            return;
        }
        stop_follower(victim);
        act_new("$n removes $N from $s group.",ch,NULL,victim,TO_NOTVICT,POS_RESTING);
        act_new("$n removes you from $s group.",ch,NULL,victim,TO_VICT,POS_SLEEPING);
        act_new("You remove $N from your group.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
        return;
    }
    
    if ( ch->leader != NULL && ch->leader != ch  )
    {
        send_to_char( "Only the leader may add group members or pass on leadership!\n\r", ch );
        return;
    }
    
    // pass on leadership
    if ( !strcmp(arg, "leader") )
    {
        victim = get_char_group( ch, remain );
        if ( !victim )
        {
            send_to_char( "Nobody like that in your group.\n\r", ch );
            return;
        }
        try_set_leader( ch, victim );
        return;
    }
    
    // add to group
    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        // show syntax
        send_to_char("  group <player> - add a player to your group\n\r"
                     "  group leader <player> - pass leadership of your group to a player\n\r"
                     "  group remove <player> - remove a member from your group\n\r", ch);
        //send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( victim->master != ch && ch != victim )
    {
        act_new("$N isn't following you.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
        return;
    }
    
    if ( (PLR_ACT(ch, PLR_WAR) || PLR_ACT(victim, PLR_WAR)) && !is_same_team(ch, victim) )
    {
        send_to_char("You cannot group during Warfare.\n\r",ch);
        return;
    }
    
    if ( is_same_group( victim, ch ) )
    {
        act_new("$N is already in your group.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
        return;
    }

    if ( ch == victim )
    {
        send_to_char("You can't add or remove yourself from your own group.\n\r",ch);
        return;
    }
    
    act_new("$N joins $n's group.",ch,NULL,victim,TO_NOTVICT,POS_RESTING);
    act_new("You join $n's group.",ch,NULL,victim,TO_VICT,POS_SLEEPING);
    act_new("$N joins your group.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
    change_leader(victim, ch);

    if ( ch != victim && is_same_player(ch, victim) )
    {
        sprintf( buf, "Multiplay: %s joins %s's group", victim->name, ch->name );
        wiznet(buf, ch, NULL, WIZ_CHEAT, 0, LEVEL_IMMORTAL);
    }
    return;
}

void try_set_leader( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch )
    {
        bug("Null ch in try_set_leader.",0);
        return;
    }

    if ( !victim )
    {
        bug( "Null victim in try_set_leader.",0);
        return;
    }

    if (IS_NPC(ch))
        return;

    if (IS_NPC(victim))
    {
        send_to_char( "Leader must be a player.", ch);
        return;
    }

    if ( victim->leader != ch )
    {
        send_to_char( "You can't pass leadership to somebody you don't lead.\n\r", ch );
        return;
    }

    change_leader(ch, victim);
}

void change_leader( CHAR_DATA *old_leader, CHAR_DATA *new_leader )
{
    CHAR_DATA *gch;
    for ( gch = char_list; gch != NULL; gch = gch->next )
    {
        if ( IS_NPC(gch) )
            continue;
        if ( gch->leader == old_leader || gch == old_leader )
        {
            if ( gch == new_leader )
            {
                gch->leader = NULL;
                gch->master = NULL;
                ptc( gch, "You now lead the group.\n\r");
            }
            else
            {
                gch->leader = new_leader;
                gch->master = new_leader;
                ptc( gch, "%s now leads your group.\n\r", new_leader->name );
            }
        }
    }
}

/*
* 'Split' originally by Gnort, God of Chaos.
*/
DEF_DO_FUN(do_split)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount_gold = 0, amount_silver = 0;
    int share_gold, share_silver;
    int extra_gold, extra_silver;
    
    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' )
    {
        send_to_char( "Split how much?\n\r", ch );
        return;
    }
    
    amount_silver = atoi( arg1 );
    
    if (arg2[0] != '\0')
        amount_gold = atoi(arg2);
    
    if ( amount_gold < 0 || amount_silver < 0)
    {
        send_to_char( "Your group wouldn't like that.\n\r", ch );
        return;
    }
    
    if ( amount_gold == 0 && amount_silver == 0 )
    {
        send_to_char( "You hand out zero coins, but no one notices.\n\r", ch );
        return;
    }
    
    if ( ch->gold <  amount_gold || ch->silver < amount_silver)
    {
        send_to_char( "You don't have that much to split.\n\r", ch );
        return;
    }
    
    members = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group( gch, ch ) && !IS_AFFECTED(gch,AFF_CHARM))
            members++;
    }
    
    if ( members < 2 )
    {
        send_to_char( "Just keep it all.\n\r", ch );
        return;
    }
    
    share_silver = amount_silver / members;
    extra_silver = amount_silver % members;
    
    share_gold   = amount_gold / members;
    extra_gold   = amount_gold % members;
    
    if ( share_gold == 0 && share_silver == 0 )
    {
        send_to_char( "Don't even bother, cheapskate.\n\r", ch );
        return;
    }
    
    ch->silver  -= amount_silver;
    ch->silver  += share_silver + extra_silver;
    ch->gold    -= amount_gold;
    ch->gold    += share_gold + extra_gold;
    
    if (share_silver > 0)
    {
        sprintf(buf,
            "You split %d silver coins. Your share is %d silver.\n\r",
            amount_silver,share_silver + extra_silver);
        send_to_char(buf,ch);
    }
    
    if (share_gold > 0)
    {
        sprintf(buf,
            "You split %d gold coins. Your share is %d gold.\n\r",
            amount_gold,share_gold + extra_gold);
        send_to_char(buf,ch);
    }
    
    if (share_gold == 0)
    {
        sprintf(buf,"$n splits %d silver coins. Your share is %d silver.",
            amount_silver,share_silver);
    }
    else if (share_silver == 0)
    {
        sprintf(buf,"$n splits %d gold coins. Your share is %d gold.",
            amount_gold,share_gold);
    }
    else
    {
        sprintf(buf,
            "$n splits %d silver and %d gold coins, giving you %d silver and %d gold.\n\r",
            amount_silver,amount_gold,share_silver,share_gold);
    }
    
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( gch != ch && is_same_group(gch,ch) && !IS_AFFECTED(gch,AFF_CHARM))
        {
            act( buf, ch, NULL, gch, TO_VICT );
            gch->gold += share_gold;
            gch->silver += share_silver;
        }
    }
    
    return;
}

DEF_DO_FUN(do_gtell)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *gch;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Tell your group what?\n\r", ch );
        return;
    }
    
    if ( IS_SET( ch->penalty,PENALTY_NOTELL ) )
    {
        send_to_char( "Your message didn't get through!\n\r", ch );
        return;
    }
    sprintf( buf, "{3You tell the group, {4'%s'{x\n\r", parse_url(argument) );
    send_to_char( buf, ch );
	if ( !IS_NPC(ch) )
		log_pers( ch->pcdata->gtell_history, buf);
    if ( ch->pcdata && ch->pcdata->guiconfig.chat_window )
    {
        open_chat_tag( ch );
        send_to_char( buf, ch );
        close_chat_tag( ch );
    }
    argument = makedrunk(argument,ch);
    
    for ( gch = char_list; gch != NULL; gch = char_list_next_char(gch) )
    {
        if ( is_same_group( gch, ch ) )
		{
            //nt_act_new( "{3$n{3 tells the group {4'$t'{x", ch, argument, gch, TO_VICT, POS_SLEEPING );
			//sprintf(buf, "{3%s{3 tells the group {4'%s'{x\n\r", get_mimic_PERS_new( ch, gch, 0), argument );
			sprintf(buf, "{3%s{3 tells the group {4'%s'{x\n\r", (IS_NPC(ch)?ch->short_descr:ch->name), parse_url(argument) );
			if (gch != ch)
			{
				send_to_char(buf, gch);
				if ( !IS_NPC(gch) )
					log_pers( gch->pcdata->gtell_history, buf);

                if ( gch->pcdata && gch->pcdata->guiconfig.chat_window )
                {
                    open_chat_tag( gch );
                    send_to_char( buf, gch );
                    close_chat_tag( gch );
                }
			}
		}
    }
    
    return;
}

CHAR_DATA *ultimate_master( CHAR_DATA *ch )
{
    while ( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL )
    {
        // safety net since we're getting crashes
        if ( !valid_CH(ch->master) || ch->master->must_extract )
        {
            bugf("ultimate_master: invalid master for %s", IS_NPC(ch) ? ch->short_descr : ch->name);
            ch->master = NULL;
        }
        else
            ch = ch->master;
    }
    return ch;
}

/*
* It is very important that this be an equivalence relation:
* (1) A ~ A
* (2) if A ~ B then B ~ A
* (3) if A ~ B  and B ~ C, then A ~ C
*/
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch )
{
    if ( ach == NULL || bch == NULL)
        return FALSE;
    
    // safety net
    if ( !valid_CH(ach) || !valid_CH(bch) || ach->must_extract || bch->must_extract )
        return FALSE;

    if ( IS_NPC(ach) && IS_NPC(bch)
            && !IS_AFFECTED(ach, AFF_CHARM) && !IS_AFFECTED(bch, AFF_CHARM)
            && ach->group > 0 && ach->group == bch->group )
        return TRUE;

    /* charmies belong to same group as master */
    ach = ultimate_master(ach);
    bch = ultimate_master(bch);
    
    if ( ach->leader != NULL ) ach = ach->leader;
    if ( bch->leader != NULL ) bch = bch->leader;
    return ach == bch;
}

// not as strong as grouped, but generally considered an ally
bool is_allied( CHAR_DATA *ach, CHAR_DATA *bch )
{
    if ( ach == NULL || bch == NULL)
        return FALSE;

    if ( is_same_group(ach, bch) )
        return TRUE;
    
    if ( IS_NPC(ach) && IS_NPC(bch) && ach->pIndexData == bch->pIndexData )
        return TRUE;
    
    return FALSE;
}

/*
* Colour setting and unsetting, way cool, Lope Oct '94
* (most of) Lope's V2.0 update added to Aarchon Sept '98 by Quirky
*/
DEF_DO_FUN(do_colour)
{
    char   arg[ MAX_STRING_LENGTH ];
    
    if( IS_NPC( ch ) )
    {
        send_to_char_bw( "Colour is not available for NPC's.\n\r", ch );
        return;
    }
    
    argument = one_argument( argument, arg );
    
    if( !*arg )
    {
        if( !IS_SET( ch->act, PLR_COLOUR ) )
        {
            SET_BIT( ch->act, PLR_COLOUR );
            send_to_char( "{bC{ro{yl{co{mu{gr{x is now {rON{x, Way Cool!\n\r", ch );
            send_to_char( "Further syntax:\n\r  colour {c<{xfield{c> <{xcolour{c>{x"
                "  colour {c<{xfield{c>{x {cbeep{x|{cnobeep{x\n\r", ch );
        }
        else
        {
            send_to_char_bw( "Colour is now OFF, <sigh>\n\r", ch );
            REMOVE_BIT( ch->act, PLR_COLOUR );
        }
        return;
    }
    
    if (!str_cmp( arg, "verbatim" ) )
    {
        ch->desc->pProtocol->verbatim = !ch->desc->pProtocol->verbatim;
        TOGGLE_BIT( ch->act, PLR_COLOUR_VERBATIM );
        if ( IS_SET( ch->act, PLR_COLOUR_VERBATIM ) )
            send_to_char( "Color codes are now displayed {rverbatim{x.\n\r", ch );
        else 
            send_to_char( "Color codes are no longer displayed {rverbatim{x.\n\r", ch );
        return;
    }
    
    if (!str_cmp( arg, "default" ) )
    {
        default_colour( ch );
        send_to_char_bw( "Colour setting to default values.\n\r", ch );
        return;
    }
    
    if (!str_cmp( arg, "all" ) )
    {
        all_colour( ch, argument );
        return;
    }
    
    if (!str_cmp( arg, "gossip" ) )
    {
        ALTER_COLOUR( gossip )
    }
    else if (!str_cmp( arg, "gossip_text" ) )
    {
        ALTER_COLOUR( gossip_text )
    }
    else if (!str_cmp( arg, "auction" ) )
    {
        ALTER_COLOUR( auction )
    }
    else if (!str_cmp( arg, "auction_text" ) )
    {
        ALTER_COLOUR( auction_text )
    }
    else if (!str_cmp( arg, "music" ) )
    {
        ALTER_COLOUR( music )
    }
    else if (!str_cmp( arg, "music_text" ) )
    {
        ALTER_COLOUR( music_text )
    }
    else if (!str_cmp( arg, "question" ) )
    {
        ALTER_COLOUR( question )
    }
    else if (!str_cmp( arg, "question_text" ) )
    {
        ALTER_COLOUR( question_text )
    }
    else if (!str_cmp( arg, "answer" ) )
    {
        ALTER_COLOUR( answer )
    }
    else if (!str_cmp( arg, "answer_text" ) )
    {
        ALTER_COLOUR( answer_text )
    }
    else if (!str_cmp( arg, "quote" ) )
    {
        ALTER_COLOUR( quote )
    }
    else if (!str_cmp( arg, "quote_text" ) )
    {
        ALTER_COLOUR( quote_text )
    }
    else if (!str_cmp( arg, "gratz" ) )
    {
        ALTER_COLOUR( gratz )
    }
    else if (!str_cmp( arg, "gratz_text" ) )
    {
        ALTER_COLOUR( gratz_text )
    }
    else if (!str_cmp( arg, "immtalk" ) )
    {
        ALTER_COLOUR( immtalk )
    }
    else if (!str_cmp( arg, "immtalk_text" ) )
    {
        ALTER_COLOUR( immtalk_text )
    }
    else if (!str_cmp( arg, "savantalk" ) )
    {
        ALTER_COLOUR( savantalk )
    }
    else if (!str_cmp( arg, "savantalk_text" ) )
    {
        ALTER_COLOUR( savantalk_text )
    }
    else if (!str_cmp( arg, "info" ) )
    {
        ALTER_COLOUR( info )
    }
    else if (!str_cmp( arg, "info_text" ) )
    {
        ALTER_COLOUR( info_text )
    }
    else if (!str_cmp( arg, "gametalk" ) )
    {
        ALTER_COLOUR( gametalk )
    }
    else if (!str_cmp( arg, "gametalk_text" ) )
    {
        ALTER_COLOUR( gametalk_text )
    }
    else if (!str_cmp( arg, "bitch" ) )
    {
        ALTER_COLOUR( bitch )
    }
    else if (!str_cmp( arg, "bitch_text" ) )
    {
        ALTER_COLOUR( bitch_text )
    }
    else if (!str_cmp( arg, "clan" ) )
    {
        ALTER_COLOUR( clan )
    }
    else if (!str_cmp( arg, "clan_text" ) )
    {
        ALTER_COLOUR( clan_text )
    }
    else if (!str_cmp( arg, "newbie" ) )
    {
        ALTER_COLOUR( newbie )
    }
    else if (!str_cmp( arg, "newbie_text" ) )
    {
        ALTER_COLOUR( newbie_text )
    }
    else if (!str_cmp( arg, "say" ) )
    {
        ALTER_COLOUR( say )
    }
    else if (!str_cmp( arg, "say_text" ) )
    {
        ALTER_COLOUR( say_text )
    }
    else if (!str_cmp( arg, "shouts" ) )
    {
        ALTER_COLOUR( shouts )
    }
    else if (!str_cmp( arg, "shouts_text" ) )
    {
        ALTER_COLOUR( shouts_text )
    }
    else if (!str_cmp( arg, "tells" ) )
    {
        ALTER_COLOUR( tells )
    }
    else if (!str_cmp( arg, "tell_text" ) )
    {
        ALTER_COLOUR( tell_text )
    }
    else if (!str_cmp( arg, "gtell" ) )
    {
        ALTER_COLOUR( gtell )
    }
    else if (!str_cmp( arg, "gtell_text" ) )
    {
        ALTER_COLOUR( gtell_text )
    }
    else if (!str_cmp( arg, "wiznet" ) )
    {
        ALTER_COLOUR( wiznet )
    }
    else if (!str_cmp( arg, "room_title" ) )
    {
        ALTER_COLOUR( room_title )
    }
    else if (!str_cmp( arg, "room_exits" ) )
    {
        ALTER_COLOUR( room_exits )
    }
    else if (!str_cmp( arg, "warfare" ) )
    {
        ALTER_COLOUR( warfare )
    }
    else if (!str_cmp( arg, "warfare_text" ) )
    {
        ALTER_COLOUR( warfare_text )
    }
    else if (!str_cmp( arg, "proclaim" ) )
    {
	ALTER_COLOUR( proclaim )
    }
    else if (!str_cmp( arg, "proclaim_text" ) )
    {
	ALTER_COLOUR( proclaim_text )
    }
    else
    {
        send_to_char_bw( "Unrecognized. Colour not changed.\n\r", ch );
        return;
    }
    
    send_to_char_bw( "New Colour Parameter set.\n\r", ch );
    return;
    
}

DEF_DO_FUN(do_bounty)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *hunter;
    SORT_TABLE *entry;
    
    if ( ch == NULL || ch->in_room == NULL )
	return;

    if ( ch->class == class_lookup("assassin") || IS_IMMORTAL(ch) )
        hunter = ch;
    else 
    {
        for ( hunter = ch->in_room->people; hunter != NULL; hunter = hunter->next_in_room )
        {
            if (!IS_NPC(hunter)) 
                continue;

            if ( hunter->spec_fun == spec_bounty_hunter ) 
                break;
        }
        
        if ( hunter == NULL || hunter->spec_fun != spec_bounty_hunter )
        {
            send_to_char("There is no bounty hunter here.\n\r",ch);
            return;
        }
        
        if ( hunter->fighting != NULL)
        {
            send_to_char("Wait until the fighting stops.\n\r",ch);
            return;
        }
        
        if ( !can_see(hunter, ch) )
        {
            do_say(hunter, "Where the heck are you, buddy?");
            return;
        }
    }
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( !str_cmp( arg1, "list" ) )
    {
        if ( bounty_table == NULL )
        {
            send_to_char("Nobody online has a bounty.\n\r",ch);
            return;
        }
        for (entry = bounty_table; entry != NULL; entry = entry->next)
        {
	    if ( entry->owner == NULL || entry->owner->pcdata == NULL )
	    {
		bugf( "bounty list: NULL owner or pcdata" );
		return;
	    }

            sprintf(buf, "%s has a bounty of %d gold.\n\r",
		    entry->owner->name, entry->owner->pcdata->bounty);
            send_to_char(buf, ch);

            if (entry->next == bounty_table) 
                return;
        }
    }
    
    if ( IS_REMORT(ch) )
    {
	send_to_char( "Not during remort!\n\r", ch );
	return;
    }

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Place a bounty on whose head?\n\rSyntax: bounty <victim> <amount>  or bounty list\n\r", ch );
        return;
    }
    
    /* trick to prevent detection of imms */
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL 
	 || (IS_IMMORTAL(victim) && (IS_INCOG(victim) || IS_WIZI(victim))) )
    {
        send_to_char( "They are currently not logged in!\n\r", ch );
        return;
    }

    if ( IS_IMMORTAL(victim) )
    {
        send_to_char( "You won't get rid of us that easily.. ;P\n\r", ch );
        return;
    }
    
    if (!str_cmp(arg2, "payoff"))
    {
        if (IS_NPC(victim)) 
            return;

        if (victim->pcdata->bounty < 1)
        {
            if (ch == hunter)
                send_to_char("That person doesn't have a bounty.\n\r", hunter );
            else
                do_say(hunter, "That person doesn't have a bounty." );

            victim->pcdata->bounty = 0;
            update_bounty( victim );
            return;
        }

        if (victim->pcdata->bounty > ch->gold)
        {
            if (ch == hunter)
                send_to_char("You don't have enough gold to pay it off.\n\r",hunter);
            else
                do_say(hunter, "You don't have enough gold to pay it off.");

            return;
        }

        ch->gold -= victim->pcdata->bounty;
        victim->pcdata->bounty = 0;
        sprintf(buf, "%s's bounty was paid off.", victim->name);
        update_bounty( victim );
        info_message(ch, buf, TRUE);
        return;
    }
    
    if (ch == victim)
    {
        if (ch == hunter)
            send_to_char("What're you, some kind of sicko?\n\r", hunter);
        else
            do_say(hunter, "What're you, some kind of sicko?");
        return;
    }
    
    if (IS_NPC(victim))
    {
        send_to_char( "You cannot put a bounty on NPCs!\n\r", ch );
        return;
    }
    
    if ( is_number( arg2 ) )
    {
        int amount, old_bounty;

        amount   = atoi(arg2);
        
	/* Curb the excessive use of the bounty command by making it more expensive. */
        if ( amount < 1000 )
	{
            if (ch == hunter)
	        send_to_char( "You may only place a bounty (or raise one) with 1000 gold or more.\n\r", hunter );
            else
	        do_say(hunter, "Keep your damn pocket change!  Put down 1000 gold or more, and then we'll talk business." );
            return;
	}
        
        if ( ch->gold < amount )
        {
            if (ch == hunter)
	        send_to_char( "You don't have that much gold!\n\r", hunter );
            else
	        do_say(hunter, "You don't have that much gold!" );
            return;
        }
        
        ch->gold -= amount;
        old_bounty = victim->pcdata->bounty;
        victim->pcdata->bounty += amount;

        sprintf( buf, "You have placed a %d gold bounty on %s.\n\r",
            amount, victim->name);
        send_to_char(buf,ch);
        
        sprintf(buf, "%s now has a bounty of %d gold.", victim->name,victim->pcdata->bounty );
        do_say(hunter, buf);
        
        if (old_bounty == 0)
            sprintf(buf, "%s has put a %d gold bounty on %s's head.", ch->name, amount, 
               victim->name);
        else
            sprintf(buf, "%s has increased the bounty on %s's head from %d to %d gold.",
               ch->name, victim->name, old_bounty, old_bounty+amount);
        
        info_message(ch, buf, FALSE);
        
        update_bounty(victim);
    }
    
    return;
}

const char * makedrunk (const char *string, CHAR_DATA * ch)
{
    /* This structure defines all changes for a character */
    struct struckdrunk drunk[] =
    {
        {3, 10,
        {"a", "a", "a", "A", "aa", "ah", "Ah", "ao", "aw", "oa", "ahhhh"}},
        {8, 5,
        {"b", "b", "b", "B", "B", "vb"}},
        {3, 5,
        {"c", "c", "C", "cj", "sj", "zj"}},
        {5, 3,
        {"d", "d", "D", "duh"}},
        {3, 4,
        {"e", "e", "eh", "E", "*hic*"}},
        {4, 6,
        {"f", "f", "ff", "fff", "fFf", "F", "FriGGiN"}},
        {8, 3,
        {"g", "g", "G", "gG"}},
        {9, 7,
        {"h", "h", "hh", "hhh", "Hhh", "HhH", "H", ""}},
        {7, 6,
        {"i", "i", "Iii", "ii", "iI", "Ii", "I"}},
        {9, 6,
        {"j", "j", "jj", "Jj", "jJ", "J", "jizzow"}},
        {7, 3,
        {"k", "k", "K", "&*$#@%"}},
        {3, 3,
        {"l", "l", "L", "LI"}},
        {5, 8,
        {"m", "m", "mm", "mmm", "mmmm", "mmmmm", "MmM", "mM", "M"}},
        {6, 6,
        {"n", "n", "nn", "Nn", "nnn", "nNn", "N"}},
        {3, 6,
        {"o", "o", "ooo", "ao", "aOoo", "Ooo", "ooOo"}},
        {3, 2,
        {"p", "p", "P"}},
        {5, 5,
        {"q", "q", "Q", "ku", "ququ", "kukeleku"}},
        {4, 3,
        {"r", "r", "R", ""}},
        {2, 5,
        {"s", "ss", "Z", "SsS", "sSzz", "sSsSZs"}},
        {5, 3,
        {"t", "t", "T", ""}},
        {3, 6,
        {"u", "u", "uh", "Uh", "UuH", "uhhU", "uhhuh"}},
        {4, 3,
        {"v", "v", "V", "s0noFa"}},
        {4, 5,
        {"w", "w", "W", "ew", "weee", "WHOOP"}},
        {5, 6,
        {"x", "x", "X", "ks", "iks", "kz", "xz"}},
        {3, 3,
        {"y", "y", "Y", "yE"}},
        {2, 9,
        {"z", "z", "Z", "zZ", "szz", "sZZz", "ZSz", "ZzzZz", "Zzz", "Zsszzsz", }}
    };
    
    static char buf[MSL];
    char temp;
    int pos, drunklevel, randomnum;
        
    if ( string[0] == '\0' )
        return string;
    
    if ( IS_AFFECTED(ch, AFF_INSANE) )
    {
        strcpy(buf, string);
        // garble up character order using "reverse bubble-sort" - a "bobble sort" ;)
        pos = 0;
        while ( buf[pos+1] != '\0' )
        {
            if ( !number_bits(2) )
            {
                temp = buf[pos];
                buf[pos] = buf[pos+1];
                buf[pos+1] = temp;
                if ( pos > 0 )
                    pos--;
                else
                    pos++;
            }
            else
                pos++;
        }
        return buf;
    }
    
    /* Check how drunk a person is... */
    if (IS_NPC(ch))
        drunklevel = 0;
    else
        drunklevel = ch->pcdata->condition[COND_DRUNK]/2+1;
    
    if (drunklevel > 1)
    {
        pos=0;
        do
        {
            temp = *string;
            if ((temp >= 'a') && (temp <= 'z'))
                temp += 'A'-'a';
            if ((temp >= 'A') && (temp <= 'Z'))
            {
                if (drunklevel > drunk[temp - 'A'].min_drunk_level+number_range(0,4))
                {
                    randomnum = number_range (0, UMIN(1+drunk[temp - 'A'].number_of_rep, drunklevel-drunk[temp-'A'].min_drunk_level));
                    if (randomnum == 1+drunk[temp-'A'].number_of_rep)
                    {
                        if ((drunklevel + number_range(0,25))>50)
                            temp = 'A' + number_range(0,25);
                        randomnum = number_range(0, drunk[temp-'A'].number_of_rep);
                    }
                    strcpy (&buf[pos], drunk[temp - 'A'].replacement[randomnum]);
                    pos += strlen (drunk[temp - 'A'].replacement[randomnum]);
                }
                else
                    buf[pos++] = *string;
            }
            else
            {
                if ((temp >= '0') && (temp <= '9'))
                {
                    temp = (30 + temp - '0' + number_range (-drunklevel/3, drunklevel/3))%10 + '0';
                    buf[pos++] = temp;
                }
                else
                    buf[pos++] = *string;
            }
        }
        while (*string++);
        buf[pos] = '\0';          /* Mark end of the string... */
        return(buf);
    }
    return (string);
}

/* RT auction rewritten in ROM style */
DEF_DO_FUN(do_auction)
{
	public_channel( &public_channel_table[sn_auction], ch, argument );
}

/* Sardonic 10/99 */
DEF_DO_FUN(do_savantalk)
{
   public_channel( &public_channel_table[sn_savantalk], ch, argument );
}
bool check_savant( CHAR_DATA *ch )
{
	return (is_granted_name(ch,"savantalk"));
}

/* Gag code by Bobble */
void print_gag(char* info_str, long value, CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    if (IS_SET(ch->gag, value))
        sprintf(buf, "%s: ON\n\r", info_str);
    else
        sprintf(buf, "%s: OFF\n\r", info_str);
    send_to_char(buf, ch);
}

bool check_gag_arg(const char* arg, const char* cmp_str, long value, CHAR_DATA *ch)
{
    if (!strcmp(arg, cmp_str))
    {
        if (IS_SET(ch->gag, value))
            REMOVE_BIT(ch->gag, value);
        else
            SET_BIT(ch->gag, value);
        send_to_char("Gagging toggled.\n\r", ch);
        return TRUE;
    }
    else
        return FALSE;
}

/* Toggle gagging options. */
DEF_DO_FUN(do_gag)
{
    if ( argument[0] == '\0' )
    {
        send_to_char("Information Gagging:\n\r", ch);
        print_gag("   miss", GAG_MISS, ch);
        print_gag("  wflag", GAG_WFLAG, ch);
        print_gag("   fade", GAG_FADE, ch);
        print_gag("  bleed", GAG_BLEED, ch);
        print_gag(" immune", GAG_IMMUNE, ch);
        print_gag("  equip", GAG_EQUIP, ch);
        print_gag("   aura", GAG_AURA, ch);
        print_gag("sunburn", GAG_SUNBURN, ch);
        print_gag(" damage", GAG_DAMAGE, ch);
        print_gag(" effect", GAG_EFFECT, ch);
        return;
    }
    
    if (check_gag_arg(argument, "miss", GAG_MISS, ch))
        return;
    if (check_gag_arg(argument, "wflag", GAG_WFLAG, ch))
        return;
    if (check_gag_arg(argument, "fade", GAG_FADE, ch))
        return;
    if (check_gag_arg(argument, "bleed", GAG_BLEED, ch))
        return;
    if (check_gag_arg(argument, "immune", GAG_IMMUNE, ch))
        return;
    if (check_gag_arg(argument, "equip", GAG_EQUIP, ch))
        return;
    if (check_gag_arg(argument, "aura", GAG_AURA, ch))
        return;
    if (check_gag_arg(argument, "sunburn", GAG_SUNBURN, ch))
        return;
    if (check_gag_arg(argument, "damage", GAG_DAMAGE, ch))
        return;
    if (check_gag_arg(argument, "effect", GAG_EFFECT, ch))
        return;
    
    send_to_char("Syntax: gag\n\r", ch);
    send_to_char("Syntax: gag [miss|wflag|fade|bleed|immune|equip|sunburn|damage|effect]\n\r", ch);
}

DEF_DO_FUN(do_try)
{
    if (IS_NPC(ch))
        return;

    if (argument[0] == '\0')
        send_to_char("Try to do what?\n\r", ch);
    else
    {
        bool found=FALSE;
        found = mp_try_trigger(argument, ch);
        found = op_try_trigger(argument, ch) | found;
        found = rp_try_trigger(argument, ch) | found;

        if (!found)
            send_to_char("That didn't work.\n\r", ch);
    }

}

DEF_DO_FUN(do_turn_in)
{
    CHAR_DATA *judge;
    char buf[MSL];
    int qp, gold;

    if ( IS_NPC(ch) )
	return;

    if ( !IS_SET(ch->act, PLR_KILLER) && !IS_SET(ch->act, PLR_THIEF) )
    {
	send_to_char( "Noble of you, but you aren't a criminal.\n\r", ch );
	return;
    }

    for ( judge = ch->in_room->people; judge != NULL; judge = judge->next_in_room )
	if ( IS_NPC(judge) && IS_SET(judge->act, ACT_JUDGE) )
	    break;
	
    if ( judge == NULL )
    {
	send_to_char( "If you want to turn yourself in, seek out a judge!\n\r", ch );
	return;
    }

    if ( !can_see(judge, ch) )
    {
	act( "$N can't see you!\n\r", ch, NULL, judge, TO_CHAR );
	return;
    }

    send_to_char( "You turn yourself in for the crimes you committed.\n\r", ch );
    
    /* calculate fine in quest points and gold */
    qp = 24;
    gold = 1000;

    act( "$N takes a look at your criminal record.", ch, NULL, judge, TO_CHAR );
    act( "$N takes a look at $n's criminal record.", ch, NULL, judge, TO_NOTVICT );
    sprintf( buf, "It seems you have broken the law %s!", ch->name );
    do_say( judge, buf );
    sprintf( buf, "I hereby sentence you to %d gold and %d hours of social work!",
	     gold, qp );
    do_say( judge, buf );

    if ( ch->gold < gold )
    {
	do_say( judge, "Return to me when you have the money." );
	return;
    }
    if ( ch->pcdata->questpoints < qp )
    {
	do_say( judge, "Return to me when you have completed the work." );
	return;
    }
    do_say( judge, "I don't want to see you back here anytime soon, you hear me?!" );

    ch->gold -= gold;
    ch->pcdata->questpoints -= qp;
    REMOVE_BIT( ch->act, PLR_KILLER );
    REMOVE_BIT( ch->act, PLR_THIEF  );
}

DEF_DO_FUN(do_action)
{
    char buf[MSL];

    if ( ch->pcdata == NULL )
	return;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax: action <command>\n\r", ch );
	send_to_char( "        action clear\n\r\n\r", ch );

	/* show current setting */
	if ( ch->pcdata->combat_action == NULL )
	    send_to_char( "You don't have any combat action set.\n\r", ch );
	else
	{
	    sprintf( buf, "Your current setting is '%s'.\n\r", ch->pcdata->combat_action );
	    send_to_char( buf, ch );
	}

	return;
    }

    /* old setting needs clearing either way.. */
    free_string( ch->pcdata->combat_action );
    ch->pcdata->combat_action = NULL;

    if ( !strcmp(argument, "clear") )
    {
	send_to_char( "Combat action cleared.\n\r", ch );
    }
    else
    {
	ch->pcdata->combat_action = str_dup( argument );
	send_to_char( "Combat action set.\n\r", ch );
    }
}

DEF_DO_FUN(do_noreply)
{
    CHAR_DATA *found_ch;
    char buf[MAX_STRING_LENGTH];

    if( argument[0] == '\0' )
    {
	send_to_char( "Whose replies would you like to divert?\n\r", ch );
	return;
    }
    else if( ( found_ch = get_char_world(ch,argument) ) != NULL )
    {
	if( found_ch->reply != ch )
	{
	    send_to_char( "That person is already not replying to you.\n\r", ch );
	    return;
	}
	sprintf( buf, "%s's replies are now diverted.\n\r", found_ch->name );
	send_to_char( buf, ch );
	found_ch->reply = NULL;
	return;
    }
    else
    {
	send_to_char( "Character not found.\n\r", ch );
	return;
    }
}

void open_chat_window( CHAR_DATA *ch )
{
    ptc( ch, "\t<FRAME Name=\"Comm\" INTERNAL Align=\"right\">");
}

void close_chat_window( CHAR_DATA *ch )
{
    ptc( ch, "\t<FRAME Name=\"Comm\" Action=\"CLOSE\">");
} 

void open_image_window( CHAR_DATA *ch )
{
    ptc( ch, "\t<FRAME Name=\"Images\" INTERNAL Align=\"top\">");
}

void close_image_window( CHAR_DATA *ch )
{
    ptc( ch, "\t<FRAME Name=\"Images\" Action=\"CLOSE\">");
}

void gui_login_setup( CHAR_DATA *ch )
{
    const char *client= ch->desc ? ch->desc->pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString : "";

    if ( ch->pcdata->guiconfig.chat_window )
    {
        open_chat_window( ch );
    }
    else if ( strstr(client, "mudportal" ) )
    {
        // always use chatwindow in mudportal cause i said so
        ch->pcdata->guiconfig.chat_window=TRUE;
        open_chat_window( ch );
        ptc( ch, "{CI see you're using mudportal. Chat window enabled.{x\n\r" );
    }

    if ( strstr( client, "zmud") ||
         strstr( client, "zMUD") ||
         strstr( client, "cmud") ||
         strstr( client, "CMUD") )
    {
        ptc( ch, "{CI see you're using %s. Type '\t(guiconfig\t)' to see some special options.{x\n\r", client);
    }
}

void open_imagewin_tag( CHAR_DATA *ch )
{
    ptc( ch, "\t<DEST Images>" );
}

void close_imagewin_tag( CHAR_DATA *ch )
{
    ptc( ch, "\t</DEST>" );
}

void open_chat_tag( CHAR_DATA *ch )
{
    ptc( ch, "\t<DEST Comm>" );
}

void close_chat_tag( CHAR_DATA *ch )
{
    ptc( ch, "\t</DEST>" );
}

DEF_DO_FUN(do_guiconfig)
{
    if (IS_NPC(ch))
        return;

    if ( !strcmp( argument, "chat" ) )
    {
        ch->pcdata->guiconfig.chat_window = !ch->pcdata->guiconfig.chat_window;
        ptc( ch, "Your chat window is turned %s.\n\r",
                ch->pcdata->guiconfig.chat_window ? "ON" : "OFF" );

        if ( ch->pcdata->guiconfig.chat_window )
        {
            open_chat_window( ch );
        }
        else
        {
            close_chat_window( ch );
        }
        return;
    }
    else if ( !strcmp( argument, "images" ) )
    {
        ch->pcdata->guiconfig.show_images = !ch->pcdata->guiconfig.show_images;
        ptc( ch, "Display images is turned %s.\n\r",
                ch->pcdata->guiconfig.show_images ? "ON" : "OFF" );
        return; 
    }
    else if ( !strcmp( argument, "imagewin" ) )
    {
        
        ch->pcdata->guiconfig.image_window = !ch->pcdata->guiconfig.image_window;
        ptc( ch, "Your image window is turned %s.\n\r",
                ch->pcdata->guiconfig.image_window ? "ON" : "OFF" );

        if ( ch->pcdata->guiconfig.image_window )
        {
            open_image_window( ch );
        }
        else
        {
            close_image_window( ch );
        }
        return;
    }

    ptc( ch, "Your chat window is turned %s.\n\r",
        ( ch->pcdata && ch->pcdata->guiconfig.chat_window ) ? "ON" : "OFF" );
    ptc( ch, "'guiconfig chat' to toggle\n\r" );

    ptc( ch, "Display images is %s.\n\r",
        ( ch->pcdata && ch->pcdata->guiconfig.show_images ) ? "ON" : "OFF" );
    ptc( ch, "'guiconfig images' to toggle\n\r" );

    ptc( ch, "Image window is %s.\n\r",
        ( ch->pcdata && ch->pcdata->guiconfig.image_window ) ? "ON" : "OFF" );
    ptc( ch, "'guiconfig imagewin' to toggle\n\r" );

}
