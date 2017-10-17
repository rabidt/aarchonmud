/**************************************************************
                         playback.c

Written by Clayton Richey (Odoth/Vodur) <clayton.richey@gmail.com>
for Aarchon MUD
(aarchonmud.com:7000).
**************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <lua.h>
#include <lauxlib.h>
#include "merc.h"
#include "tables.h"
#include "lua_main.h"


/* local functions */
void playback_pers( CHAR_DATA *ch, PERS_HISTORY *history, sh_int entries );
void playback_to_char( CHAR_DATA *ch, COMM_HISTORY *history, sh_int entries );
void playback_clear( COMM_HISTORY *history );

#define MAX_COMM_HISTORY 300
/* Default number of results, needs to  be <=MAX_COMM_HISTORY */
#define DEFAULT_RESULTS 30

#define MAX_PERS_HISTORY 100 
#define DEFAULT_PERS 30


/* declare the actual structures we will use*/
COMM_HISTORY public_history={0, NULL, NULL};
COMM_HISTORY immtalk_history={0, NULL, NULL};
COMM_HISTORY savant_history={0, NULL, NULL};

COMM_ENTRY *comm_entry_new( void )
{
	COMM_ENTRY *entry=alloc_mem(sizeof(COMM_ENTRY));
	entry->next=NULL;
	entry->prev=NULL;
	entry->text=NULL;
	return entry;
}

PERS_ENTRY *pers_entry_new( void )
{
	PERS_ENTRY *entry=alloc_mem(sizeof(PERS_ENTRY));
	entry->next=NULL;
	entry->prev=NULL;
	entry->text=NULL;
	return entry;
}

PERS_HISTORY *pers_history_new( void )
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
	PERS_ENTRY *entry, *next;
	for ( entry = history->head ; entry ; entry=next )
	{
	    next=entry->next;
	    pers_entry_free( entry );
	}
	free_mem(history, sizeof(PERS_HISTORY) );
}

void log_pers( PERS_HISTORY *history, const char *text )
{
	PERS_ENTRY *entry=pers_entry_new();
	char time[MSL];
	char buf[MSL*2];
	
	strcpy(time, ctime( &current_time ) );
	time[strlen(time)-1] = '\0';
	snprintf(buf, sizeof(buf), "%s::%s", time, text );
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

void add_to_comm_history ( COMM_HISTORY *history, COMM_ENTRY *entry ) 
{
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

void log_chan( CHAR_DATA * ch, const char *text , sh_int channel )
{
    /* write all the info for the new entry */
    COMM_ENTRY *entry=comm_entry_new();
	
    entry->text = str_dup(text) ;
    entry->channel = channel;
    entry->timestamp = trim_realloc(str_dup(ctime(&current_time)));

    if (!IS_NPC(ch))
    {
        entry->name = str_dup(ch->name);
    }
    else
    {
        entry->name = str_dup(ch->short_descr);
    }

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
	

    add_to_comm_history( history, entry );
		
}

DEF_DO_FUN(do_playback)
{
    
    if ( IS_NPC(ch) )
		return;
	
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
	
	/* if we're here, it's a COMM_HISTORY, not PERS_HISTORY, check for 2nd arg */
	argument=one_argument( argument, arg );
	
	if ( arg[0] == '\0' )
	{
        if (phistory)
        {
            playback_pers( ch, phistory, ch->lines == 0 ? DEFAULT_PERS : ch->lines );
            return;
        }
        else
        {
		    playback_to_char( ch, history, ch->lines == 0 ? DEFAULT_RESULTS : ch->lines );
		    return;
        }
	}
    if (is_number(arg))
    {
        arg_number = atoi(arg);
        if (arg_number > MAX_COMM_HISTORY || arg_number < 1)
        {
            printf_to_char(ch, "Argument should be a number from 1 to %d.\n\r",
                    phistory ? MAX_PERS_HISTORY : MAX_COMM_HISTORY);
            return;
        }
        else
        {
            if (phistory)
            {
                playback_pers( ch, phistory, arg_number);
                return;
            }
            else
            {
                playback_to_char( ch, history, arg_number );
                return;
            }
        }
    }
    else if (!strcmp(arg, "clear") && immortal && history )
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
            snprintf(buf, sizeof(buf),"%s::{%c%s%s", 
                    entry->timestamp,
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

void playback_pers( CHAR_DATA *ch, PERS_HISTORY *history, sh_int entries)
{
	if (history==NULL)
	{	
		bugf("NULL history passed to playback_pers.");
		return;
	}
	if ( history->tail == NULL )
		return;
		
	PERS_ENTRY *entry;
	
    entry=history->tail;
    if ( history->size > entries )
    {
       int i;
       for ( i = history->size - entries ; i>0 ; i-- )
           entry=entry->prev; /* just fast forwarding */
    }

	for ( ; entry != NULL ; entry=entry->prev )
	{
		send_to_char(  entry->text, ch );
	}
}

void push_comm_history( lua_State *LS, COMM_HISTORY *history )
{
    lua_newtable( LS );
    int index=1;

    COMM_ENTRY *entry;

    for ( entry=history->tail ; entry ; entry=entry->prev )
    {
        lua_newtable( LS );

        lua_pushstring( LS, entry->timestamp );
        lua_setfield( LS, -2, "timestamp" );

        lua_pushstring( LS,
                public_channel_table[entry->channel].name );
        lua_setfield( LS, -2, "channel" );

        lua_pushstring( LS, entry->text );
        lua_setfield( LS, -2, "text" );

        lua_pushstring( LS, entry->name );
        lua_setfield( LS, -2, "name" );

        lua_rawseti( LS, -2, index++ );
    }
}

static void load_comm_history( lua_State *LS, COMM_HISTORY *history )
{
    int n=lua_objlen( LS, -1 );
    int i;
    COMM_ENTRY *en;

    for ( i=1 ; i<=n ; i++ )
    {
        lua_rawgeti( LS, -1, i );

        if ( lua_isnil( LS, -1 ) )
        {
            bugf( "Bad juju." );
            break;
        }

        en=comm_entry_new();

        lua_getfield( LS, -1, "timestamp" );
        if ( !lua_isnil( LS, -1 ) )
        {
            en->timestamp=str_dup(luaL_checkstring( LS, -1));
        }
        lua_pop( LS, 1);

        lua_getfield( LS, -1, "channel" );
        const char *chan=luaL_checkstring( LS, -1 );
        int sn;
        for ( sn=0 ; public_channel_table[sn].name ; sn++ )
        {
            if ( !strcmp( chan, public_channel_table[sn].name) )
            {
                en->channel=sn;
                break;
            }
        }
        lua_pop( LS, 1);

        lua_getfield( LS, -1, "text" );
        en->text=str_dup(luaL_checkstring( LS, -1));
        lua_pop( LS, 1);

        lua_getfield( LS, -1, "name" );
        en->name=str_dup(luaL_checkstring( LS, -1));
        lua_pop( LS, 1);

        add_to_comm_history( history, en );
        lua_pop( LS, 1 );
    }
    lua_pop( LS, 1 );
}
static int L_save_comm_histories( lua_State *LS )
{
    lua_getglobal( LS, "save_comm" );
    lua_pushliteral( LS, "public_history" );
    push_comm_history( LS, &public_history );
    lua_call( LS, 2, 0 );

    lua_getglobal( LS, "save_comm" );
    lua_pushliteral( LS, "immtalk_history" );
    push_comm_history( LS, &immtalk_history );
    lua_call( LS, 2, 0 );

    lua_getglobal( LS, "save_comm" );
    lua_pushliteral( LS, "savant_history" );
    push_comm_history( LS, &savant_history );
    lua_call( LS, 2, 0 );

    return 0;
}

static int L_load_comm_histories( lua_State *LS )
{
    lua_getglobal( LS, "load_comm" );
    lua_pushliteral( LS, "public_history" );
    lua_call( LS, 1, 1 );
    load_comm_history( LS, &public_history );

    lua_getglobal( LS, "load_comm" );
    lua_pushliteral( LS, "immtalk_history" );
    lua_call( LS, 1, 1 );
    load_comm_history( LS, &immtalk_history );

    lua_getglobal( LS, "load_comm" );
    lua_pushliteral( LS, "savant_history" );
    lua_call( LS, 1, 1 );
    load_comm_history( LS, &savant_history );

    return 0;
} 
void save_comm_histories( void )
{
    PERF_PROF_ENTER( pr_, "save_comm_histories" );
    lua_pushcfunction( g_mud_LS, L_save_comm_histories );
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with L_save_comm_histories:\n %s",
                lua_tostring(g_mud_LS, -1));
        PERF_PROF_EXIT( pr_ );
        return;
    }
    PERF_PROF_EXIT( pr_ );
}

void load_comm_histories( void )
{
    lua_pushcfunction( g_mud_LS, L_load_comm_histories );
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with L_load_comm_histories:\n %s",
                lua_tostring(g_mud_LS, -1));
        return;
    }
}

