/**************************************************************
                         playback.c

Written by Clayton Richey (Odoth/Vodur) <clayton.richey@gmail.com>
for Aarchon MUD
(aarchonmud.com:7000).
Version date: 1/31/2013
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

#define MAX_PERS_HISTORY 25


/* declare the actual structures we will use*/
COMM_HISTORY public_history={0, NULL, NULL};
COMM_HISTORY immtalk_history={0, NULL, NULL};
COMM_HISTORY savant_history={0, NULL, NULL};

COMM_ENTRY *comm_entry_new()
{
	COMM_ENTRY *entry=alloc_mem(sizeof(COMM_ENTRY));
	entry->next=NULL;
	entry->prev=NULL;
	entry->text=NULL;
	return entry;
}

PERS_ENTRY *pers_entry_new()
{
	PERS_ENTRY *entry=alloc_mem(sizeof(PERS_ENTRY));
	entry->next=NULL;
	entry->prev=NULL;
	entry->text=NULL;
	return entry;
}

PERS_HISTORY *pers_history_new()
{
	PERS_HISTORY *history=alloc_mem(sizeof(PERS_HISTORY));
	history->head=NULL;
	history->tail=NULL;
	history->size=0;
	return history;
}

void comm_entry_free(COMM_ENTRY *entry)
{
	free_string(entry->timestamp);
	free_string(entry->text);
	free_string(entry->mimic_name);
	free_string(entry->name);
	free_mem(entry, sizeof(COMM_ENTRY) );
}

void pers_entry_free(PERS_ENTRY *entry)
{
	log_string(entry->text);
	free_string(entry->text);
	free_mem(entry, sizeof(PERS_ENTRY) );
}

void pers_history_free(PERS_HISTORY *history)
{
	PERS_ENTRY *entry, *next;
	for ( entry = history->head ; entry ; entry=next )
	{
	    next=entry->next;
	    pers_entry_free( entry );
	}
	free_mem(history, sizeof(PERS_HISTORY) );
}

void log_pers( PERS_HISTORY *history, char *text )
{
	PERS_ENTRY *entry=pers_entry_new();
	char time[MSL];
	char buf[MSL*2];
	
	strcpy(time, ctime( &current_time ) );
	time[strlen(time)-1] = '\0';
	sprintf(buf, "%s::%s", time, text );
	entry->text=str_dup(buf);
	
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
	
	if ( history->size > MAX_PERS_HISTORY )
	{
		/* It's over full, pop off the tail */
		PERS_ENTRY *destroy=history->tail;
		history->tail=destroy->prev;
		destroy->prev->next=NULL;
		history->size -= 1;
		pers_entry_free(destroy);
	}
		
}


void log_chan(CHAR_DATA * ch, char * text , sh_int channel)
{
    char buf[MSL];

    /* write all the info for the new entry */
    COMM_ENTRY *entry=comm_entry_new();
	
    entry->text = str_dup(text) ;
    entry->channel = channel;
    entry->timestamp= str_dup(ctime( &current_time ));
    /*have to add the EOL to timestamp or it won't have one, weird*/
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
    
    if (channel == sn_immtalk)
	history=&immtalk_history;
    else if ( channel == sn_savantalk )
	history=&savant_history;
    else
	history=&public_history;
	


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
    
    if ( IS_NPC(ch) )
		return;
	
    BUFFER *output;
    char arg[MSL];
    sh_int arg_number;
    bool immortal=IS_IMMORTAL(ch);
    bool savant= ( ch->level >= SAVANT );

    argument = one_argument(argument,arg);
    
    if ( arg[0] == '\0')
    {
	send_to_char("playback [public|tell|gtell|clan", ch);
	if ( immortal )
		send_to_char("|imm",ch);
	if ( savant )
		send_to_char("|savant",ch);
	send_to_char("] <#>\n\r",ch);
	return;
    }
	
	COMM_HISTORY *history=NULL;
	PERS_HISTORY *phistory=NULL;
	
	if ( arg[0] == 'p' )
	{
		history=&public_history;
	}
	else if ( arg[0] == 't' )
	{
		phistory=ch->pcdata->tell_history;
		ch->pcdata->new_tells=FALSE;
	}
	else if ( arg[0] == 'g' )
	{
		phistory=ch->pcdata->gtell_history;
	}
	else if ( arg[0] == 'c' )
	{
		phistory=ch->pcdata->clan_history;
	}
	else if ( immortal  && arg[0] == 'i' )
	{
		history=&immtalk_history;
	}
	else if ( savant && arg[0] == 's' )
	{
		history=&savant_history;
	}
	else
	{
		send_to_char("Invalid syntax.\n\r", ch);
		return;
	}
	
	
	if (phistory)
	{
		playback_pers( ch, phistory);
		return;
	}
	else if (!history)
	{
		bugf("history and phistory NULL in do_playback");
		return;
	}
	
	/* if we're here, it's a COMM_HISTORY, not PERS_HISTORY, check for 2nd arg */
	argument=one_argument( argument, arg );
	
	if ( arg[0] == '\0' )
	{
		playback_to_char( ch, history, DEFAULT_RESULTS);
		return;
	}
	if (is_number(arg))
	{
        	arg_number = atoi(arg);
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
	else if (!strcmp(arg, "clear") && immortal )
	{
		playback_clear( history );
		return;
	}
	else
	{
		send_to_char("Invalid syntax.\n\r",ch);
		return;
	}
}     

void playback_clear( COMM_HISTORY *history)
{
	COMM_ENTRY *destroy=history->head;
	if (destroy==NULL)
		return;
	else if ( destroy==history->tail ) /* head and tail the same, 1 entry */
	{
		history->head=NULL;
		history->tail=NULL;
		history->size=0;
	}
	else
	{
		history->head=destroy->next;
		if (history->head!=NULL)
			history->head->prev=NULL;
		history->size-=1;
	}
	comm_entry_free(destroy);
}


void playback_to_char( CHAR_DATA *ch, COMM_HISTORY *history, sh_int entries )
{
	if (history == NULL)
	{
		bugf("NULL history passed to playback_to_char.");
		return;
	}
	COMM_ENTRY *entry;
	entry=history->tail;
	if ( entry == NULL)
		return;
		
	char buf[2*MSL];
	BUFFER *output;
	output = new_buf();
		
	if ( entries < history->size )
	{
		entry=history->head;
		int i;
		for ( i=1; i < entries && entry != NULL; )
		{
		
			if ( !IS_CHAN_OFF(ch, (entry->channel)) )
			  i++;
			entry=entry->next;

		}
	}
	
    for ( ; entry != NULL ; entry=entry->prev )
    {
			
        if (! IS_CHAN_OFF(ch, (entry->channel)) )
        {
            if (IS_SET(ch->act, PLR_HOLYLIGHT) && entry->mimic_name != NULL)
             sprintf(buf,"%s::(%s{x){%c%s%s",entry->timestamp,
                                         entry->name,
                                         public_channel_table[entry->channel].prime_color,
                                         entry->mimic_name,
                                         entry->text);
            else if (entry->invis==TRUE && !IS_SET(ch->act, PLR_HOLYLIGHT))
             sprintf(buf, "%s::{%c%s%s", entry->timestamp,
                                         entry->channel,
                                         "someone",
                                         entry->text);
            else if (entry->mimic_name != NULL)
             sprintf(buf,"%s::{%c%s%s", entry->timestamp,
                                        public_channel_table[entry->channel].prime_color,
                                        entry->mimic_name,
                                        entry->text);
            else
             sprintf(buf,"%s::{%c%s%s", entry->timestamp,
                                        public_channel_table[entry->channel].prime_color,
                                        entry->name,
                                        entry->text);
            add_buf(output,buf);
            add_buf(output,"{x\n\r");
        }
    }
	
    page_to_char(buf_string(output),ch);
    free_buf(output);
}

void playback_pers( CHAR_DATA *ch, PERS_HISTORY *history)
{
	if (history==NULL)
	{	
		bugf("NULL history passed to playback_pers.");
		return;
	}
	if ( history->tail == NULL )
		return;
		
	PERS_ENTRY *entry;
	
	for ( entry=history->tail ; entry != NULL ; entry=entry->prev )
	{
		send_to_char(  entry->text, ch );
	}

}
