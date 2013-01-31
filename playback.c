/**************************************************************
                         playback.c

Written by Clayton Richey (Odoth/Vodur) <clayton.richey@gmail.com>
for Aarchon MUD
(aarchonmud.com:7000).
Version date: 1/30/2013
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

#define MAX_PERS_HISTORY 20
#define DEFAULT_PERS_RESULTS 20




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
	free_string(entry->text);
	free_mem(entry, sizeof(PERS_ENTRY) );
}

void pers_history_free(PERS_HISTORY *history)
{
	free_mem(history, sizeof(PERS_HISTORY) );
}

void log_pers( PERS_HISTORY *history, char *text )
{
	PERS_ENTRY *entry=pers_entry_new();
	char time[MSL];
	char buf[MSL*2];
	
	strcpy(time, ctime( &current_time ) );
	time[strlen(time)-1] = '\0';
	sprintf(buf, "%s:::%s", time, text );
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
    
	if ( IS_NPC(ch) )
		return;
	
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
		COMM_HISTORY *history=NULL;
		PERS_HISTORY *phistory=NULL;
		
		if (!strcmp(arg, "imm" ) )
		{
			history=&immtalk_history;
		}
		else if (!strcmp(arg, "savant" ) )
		{
			history=&savant_history;
		}
		else if (!strcmp(arg, "gtell" ) )
		{
			phistory= ch->pcdata->gtell_history;
		}
		else if (!strcmp(arg, "tell" ) )
		{
			phistory= ch->pcdata->tell_history;
			ch->pcdata->new_tells=FALSE;
		}
		else if (!strcmp(arg, "clan" ) )
		{
			phistory= ch->pcdata->clan_history;
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
			if (history)
				playback_to_char( ch, history, DEFAULT_RESULTS );
			else if (phistory)
				playback_pers( ch, phistory);
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
    for ( entry=history->tail ; entry != NULL ; entry=entry->prev )
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
		send_to_char( entry->text, ch);
	}

}
