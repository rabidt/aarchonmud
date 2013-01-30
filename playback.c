/**************************************************************
                         playback.c

Written by Clayton Richey (Odoth/Vodur) <clayton.richey@gmail.com>
for Aarchon MUD
(aarchonmud.com:7000).
Version date: 1/30/2013


Summary:
This file contains log_chan and do_playback functions along with
structure definition/declarations neccessary for their use.

log_chan stores all neccessary info about a public channel 
communication for later playback.

do_playback is a command used to playback the saved comms.

As it is, it is set up to log all public channels in one 
COMM_HISTORY structure, and all immtalk in a 2nd. More can be
added through the follwing steps:

1. declare a new COMM_HISTORY structure
2. add cases as need in the switch statement in log_chan to 
   decide which channels go to it.
3. add calls to log_chan to the appropriate do_ function(s)
   in act_comm.c (see installation instructions)
4. duplicate the if (!strcmp(arg, "imm") section in do_playback


Other notes:
Invisible/astral/dark chars are stored as someone no matter what
and will appear as 'someone' on playback EXCEPT for imms with
holylight. This is not to be lazy but because it doesn't make
sense to see 'someone gossips', turn on detects then hit
playback and see who it was, and somehow tracking who had detects
when an invis person did a channel is unrealistic. 

Limitations:
Wizi levels are not taken into account. Holylight means you will
see the name in playback no matter what.

I probably forgot somethin, good luck!
***************************************************************

Installation instructions:

Add playback.c to makefile or duplicate its contents in
appropriate .c file.

Add these definitions to merc.h, or duplicate in act_comm.c
and playback.c
Was going to pass color and channel identifier separately but no
reason not to use color char for both!

#define CHAN_GOSSIP 'p'
#define CHAN_AUCTION 'a'
#define CHAN_MUSIC 'e'
#define CHAN_QUESTION 'q'
#define CHAN_ANSWER 'j'
#define CHAN_QUOTE 'h'
#define CHAN_GRATZ 'z'
#define CHAN_GAMETALK 'k'
#define CHAN_BITCH 'f'
#define CHAN_NEWBIE 'n'
#define CHAN_IMMTALK 'i'
*************
in act_comm.c

add a call to log_chan in each channel function fight after
call to makedrunk, or right after "You <channel>" message if
not. 
****
        sprintf( buf, "{fYou bitch {F'%s{F'{x\n\r", argument );
        send_to_char( buf, ch );

        argument = makedrunk(argument,ch);
+        sprintf(buf,"{f bitches {F'%s{F'",argument);
+        log_chan(ch,buf,CHAN_BITCH);
****
        sprintf( buf, "{kYou gametalk {K'%s{K'{x\n\r", argument );
        send_to_char( buf, ch );

        argument = makedrunk(argument,ch);
+        sprintf(buf,"{k gametalks {K'%s{K'",argument);
+        log_chan(ch,buf,CHAN_GAMETALK);
****
        sprintf( buf, "{zYou gratz {Z'%s{Z'{x\n\r", argument );
        send_to_char( buf, ch );

+        sprintf(buf,"{z gratzes {Z'%s{Z'", argument);
+        log_chan(ch,buf,CHAN_GRATZ);
****
        sprintf( buf, "{hYou quote {H'%s{H'{x\n\r", argument );
        send_to_char( buf, ch );

+        sprintf(buf,"{h quotes {H'%s{H'{x",argument);
+        log_chan(ch,buf,CHAN_QUOTE);
****
        sprintf( buf, "{qYou question {Q'%s{Q'{x\n\r", argument );
        send_to_char( buf, ch );

+        sprintf(buf,"{q questions {Q'%s{Q'", argument);
+        log_chan(ch,buf,CHAN_QUESTION);
****
        sprintf( buf, "{jYou answer {J'%s{J'{x\n\r", argument );
        send_to_char( buf, ch );

+        sprintf(buf,"{j answers {J'%s{J'",argument);
+        log_chan(ch,buf,CHAN_ANSWER);
****
        sprintf( buf, "{eYou MUSIC: {E'%s{E'{x\n\r", argument );
        send_to_char( buf, ch );

        argument = makedrunk(argument,ch);
+        sprintf(buf,"{e MUSIC: {E'%s{E'", argument);
+        log_chan(ch,buf,CHAN_MUSIC);
****
    sprintf( buf, "{i$n: {I%s{x", argument );
    act_new("{i$n: {I$t{x",ch,argument,NULL,TO_CHAR,POS_DEAD);

+    sprintf(buf,"{i: {I'%s{I'", argument);
+    log_chan(ch,buf,CHAN_IMMTALK);
****
        sprintf( buf, "{aYou auction {A'%s{A'{x\n\r", argument );
        send_to_char( buf, ch );

+        sprintf(buf,"{a auctions {A'%s{A'", argument);
+        log_chan(ch,buf,CHAN_AUCTION);

***********
in interp.c

    { "noreply",    do_noreply, POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
+    { "playback",   do_playback, POS_SLEEPING,   0,  LOG_NORMAL, 1, FALSE, FALSE},
***********
in interp.h

DECLARE_DO_FUN( do_play     );
+DECLARE_DO_FUN( do_playback );
DECLARE_DO_FUN( do_pload    );

**************************************************************/




#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"


#define MAX_COMM_HISTORY 70
/* Default number of results, needs to  be <=MAX_COMM_HISTORY */
#define DEFAULT_RESULTS 35 

typedef struct comm_history_entry COMM_ENTRY;
typedef struct comm_history_type COMM_HISTORY;

struct comm_history_entry
{
    COMM_ENTRY *next;
    COMM_ENTRY *prev;

    char *timestamp;
    char channel;
    char *text;
    bool invis;
    char *mimic_name;
    char *name;
};

struct comm_history_type
{
    sh_int size;
    COMM_ENTRY *head; /* most recent */
    COMM_ENTRY *tail; /* oldest */
};


/* declare the actual structures we will use*/
COMM_HISTORY public_history;
COMM_HISTORY immtalk_history;
COMM_HISTORY savant_history;

COMM_ENTRY *comm_entry_new()
{
	return alloc_mem(sizeof(COMM_ENTRY));
}

void comm_entry_free(COMM_ENTRY *entry)
{
	free_string(entry->timestamp);
	free_string(entry->text);
	free_string(entry->mimic_name);
	free_string(entry->name);
	free_mem(entry, sizeof(COMM_ENTRY) );
}

void log_chan(CHAR_DATA * ch, char * text , char channel)
{
    char buf[MSL];

    /* write all the info for the new entry */
	COMM_ENTRY *entry=comm_entry_new();
	
    entry->text = str_dup(text) ;
    entry->channel = channel;
    entry->timestamp= str_dup(ctime( &current_time ));
    //have to add the EOL to timestamp or it won't have one, weird
    entry->timestamp[strlen(entry->timestamp)-1] = '\0';

    if (!IS_NPC(ch))
      sprintf(buf,"%s%s", ch->pcdata->pre_title,ch->name);
    else
      sprintf(buf,"%s",ch->short_descr);

    entry->name = str_dup(buf);
    //check visibility
    entry->invis=(IS_AFFECTED(ch, AFF_ASTRAL) || IS_AFFECTED(ch, AFF_INVISIBLE) || IS_WIZI(ch) || room_is_dark( ch->in_room));
    //now mimic
    if (is_mimic(ch))
    {
        MOB_INDEX_DATA *mimic;
		mimic = get_mimic(ch);
        if (mimic != NULL)
         entry->mimic_name = str_dup(mimic->short_descr);
    }
    else 
     entry->mimic_name = NULL;




    /* Assign the correct history based on which channel.
    All public channels using public_history, immtalk uses
    immtalk history, etc. */
    	
	COMM_HISTORY * history;    
    switch (channel)
    {
		case CHAN_IMMTALK:
			history=&immtalk_history;
			break;
		case CHAN_SAVANT:
			history=&savant_history;
			break;
		default:
			history=&public_history;
    }
	
	/* add it to the history */
	if ( history->head == NULL )
	{
		/* empty history */
		history->head=entry;
		history->tail=entry;
		history->size=1;
		entry->prev=NULL;
		entry->next=NULL;
	}
	else
	{
		entry->next=history->head;
		history->head->prev=entry;
		history->head=entry;
		history->size+=1;
	}
	
	if ( history->size > MAX_COMM_HISTORY )
	{
		/* It's over full, pop off the tail */
		COMM_ENTRY *destroy=history->tail;
		history->tail=destroy->prev;
		destroy->prev->next=NULL;
		history->size -= 1;
		comm_entry_free(destroy);
	}
		
}

void do_playback(CHAR_DATA *ch, char * argument)
{
    
    BUFFER *output;
    char arg[MSL];
    sh_int arg_number;

    argument = one_argument(argument,arg);
    
    if ( arg[0] == '\0')
    {
		playback_to_char( ch, &public_history, DEFAULT_RESULTS);
		return;
    }
    else if (is_number(arg))
    {
        arg_number = atoi(arg);
        if (arg_number > MAX_COMM_HISTORY || arg_number < 1)
        {
            printf_to_char(ch, "Argument should be a number from 1 to %d.\n\r",MAX_COMM_HISTORY);
            return;
        }
        else
		{
			playback_to_char( ch, &public_history, arg_number );
			return;
		}
    }
	else if (!strcmp(arg, "clear") && !IS_NPC(ch))
    {
		playback_clear( ch, &public_history );
    }
	else
	{
		/* specify channel and argument */
		COMM_HISTORY *history;
		if (!strcmp(arg, "imm" ) )
		{
			history=&immtalk_history;
		}
		else if (!strcmp(arg, "savant" ) )
		{
			history=&savant_history;
		}
		else
		{
			send_to_char("Bad syntax.\n\r",ch);
			return;
		}
		
		char arg2[MSL];
		argument=one_argument(argument,arg2);
		if (arg2[0] == '\0')
		{
			playback_to_char( ch, history, DEFAULT_RESULTS );
			return;
		}
		else if (is_number(arg2))
		{	
			arg_number = atoi(arg2);
			if (arg_number > MAX_COMM_HISTORY || arg_number < 1)
			{
				printf_to_char(ch, "Argument should be a number from 1 to %d.\n\r",MAX_COMM_HISTORY);
				return;
			}
			else
			{
				playback_to_char( ch, history, arg_number );
				return;
			}
		}
		else if (!strcmp(arg2, "clear") && !IS_NPC(ch))
		{
			playback_clear( ch, history );
		}
	}
}     

void playback_clear( CHAR_DATA *ch, COMM_HISTORY *history)
{
	COMM_ENTRY *destroy=history->head;
	if (destroy==NULL)
		return;
	
	history->head=destroy->next;
	if (history->head!=NULL)
		history->head->prev=NULL;
	
	history->size-=1;
	
	comm_entry_free(destroy);
}


void playback_to_char( CHAR_DATA *ch, COMM_HISTORY *history, sh_int entries )
{
	char buf[2*MSL];
	BUFFER *output;
	output = new_buf();
	COMM_ENTRY *entry;
	int entry_num=0;
    for ( entry=history->head ; entry != NULL ; entry=entry->next )
    {
		entry_num+=1;
		if ( entry_num > entries )
			break;
			
        if (entry->timestamp != NULL && entry->text != NULL && !((entry->channel == CHAN_BITCH && IS_SET(ch->comm,COMM_NOBITCH))))
        {
            if (IS_SET(ch->act, PLR_HOLYLIGHT) && entry->mimic_name != NULL)
             sprintf(buf,"%s::%s(%s{x){%c%s%s",entry->timestamp,
                                         entry->channel==CHAN_NEWBIE?"{n[Newbie] ":"",
                                         entry->name,
                                         entry->channel,
                                         entry->mimic_name,
                                         entry->text);
            else if (entry->invis==TRUE && !IS_SET(ch->act, PLR_HOLYLIGHT))
             sprintf(buf, "%s::%s{%c%s%s", entry->timestamp,
                                         entry->channel==CHAN_NEWBIE?"{n[Newbie] ":"",
                                         entry->channel,
                                         "someone",
                                         entry->text);
            else if (entry->mimic_name != NULL)
             sprintf(buf,"%s::%s{%c%s%s", entry->timestamp,
                                        entry->channel==CHAN_NEWBIE?"{n[Newbie] ":"",
                                        entry->channel,
                                        entry->mimic_name,
                                        entry->text);
            else
             sprintf(buf,"%s::%s{%c%s%s", entry->timestamp,
                                        entry->channel==CHAN_NEWBIE?"{n[Newbie] ":"",
                                        entry->channel,
                                        entry->name,
                                        entry->text);
            add_buf(output,buf);
            add_buf(output,"{x\n\r");
        }
    }
	
    page_to_char(buf_string(output),ch);
    free_buf(output);
}
