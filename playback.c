/**************************************************************
                         playback.c

Written by Clayton Richey (Odoth/Vodur) <clayton.richey@gmail.com>
for Aarchon MUD
(aarchonmud.com:7000).
Version date: 9/16/2011


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
Invisbile/astra/dark chars are stored as someone no matter what
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
//Default number of results, needs to  be <=MAX_COMM_HISTORY
#define DEFAULT_RESULTS 35 

/* for cleaner code without adding unnecessary pointers*/
#define ENTRY history->entry
#define POSITION history->position
 
/* add these definitions to merc.h, or duplicate in act_comm.c
and playback.c
Was going to pass color and channel identifier separately but no
reason not to use color for both!

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
#define CHAN_INFO '1'
#define CHAN_SAVANT '7'

*/
typedef struct comm_history_entry COMM_ENTRY;
typedef struct comm_history_type COMM_HISTORY;

struct comm_history_entry
{
    char *timestamp;
    char channel;
    char *text;
    bool invis;
    char *mimic_name;
    char *name;
};

struct comm_history_type
{
    sh_int position;
    COMM_ENTRY entry[MAX_COMM_HISTORY];
};


/* declare the actual structures we will use*/
COMM_HISTORY public_history;
COMM_HISTORY immtalk_history;
COMM_HISTORY savant_history;

void log_chan(CHAR_DATA * ch, char * text , char channel)
{

    COMM_HISTORY * history;    
    char buf[MSL];

    if IS_NPC(ch)
      return;

    /* Assign the correct history based on which channel.
    All public channels using public_history, immtalk uses
    immtalk history, etc. */
    //to be lazy make public the default and speciffor anything else
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

    /* Free all the strings before we overwrite an entry */
    if (ENTRY[POSITION].text != NULL)
     free_string(ENTRY[POSITION].text);
    if (ENTRY[POSITION].timestamp != NULL)
     free_string(ENTRY[POSITION].timestamp);
    if (ENTRY[POSITION].name != NULL)
     free_string(ENTRY[POSITION].name);
    if (ENTRY[POSITION].mimic_name != NULL)
     free_string(ENTRY[POSITION].mimic_name);


    /* write all the info for the new entry */
    ENTRY[POSITION].text = str_dup(text) ;
    ENTRY[POSITION].channel = channel;
    ENTRY[POSITION].timestamp= str_dup(ctime( &current_time ));
    //have to add the EOL to timestamp or it won't have one, weird
    ENTRY[POSITION].timestamp[strlen(ENTRY[POSITION].timestamp)-1] = '\0';
    sprintf(buf,"%s%s", ch->pcdata->pre_title, ch->name);
    ENTRY[POSITION].name = str_dup(buf);
    //check visibility
    ENTRY[POSITION].invis=(IS_AFFECTED(ch, AFF_ASTRAL) || IS_AFFECTED(ch, AFF_INVISIBLE) || IS_WIZI(ch) || room_is_dark( ch->in_room));
    //now mimic
    if (is_mimic(ch))
    {
        MOB_INDEX_DATA *mimic;
	mimic = get_mimic(ch);
        if (mimic != NULL)
         ENTRY[POSITION].mimic_name = strdup(mimic->short_descr);
    }
    else 
     ENTRY[POSITION].mimic_name = NULL;

   /* increment position, loop it back if we're out of space and begin
   overwriting the oldest entries*/
    POSITION += 1;
    if (POSITION == MAX_COMM_HISTORY)
     POSITION = 0;

}

void do_playback(CHAR_DATA *ch, char * argument)
{
    COMM_HISTORY *history;
    BUFFER *output;
    char color;
    sh_int pos;
    char buf[2*MSL];
    char arg[MSL];
    sh_int arg_number;

    argument = one_argument(argument,arg);
    
    if ( arg[0] == '\0')
    {
	history = &public_history;
	pos = POSITION;
	pos += MAX_COMM_HISTORY - DEFAULT_RESULTS;
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
	    history = &public_history;
            pos = POSITION;
            pos += MAX_COMM_HISTORY-arg_number;
	}
    }
    else if (!strcmp(arg,"imm") && IS_IMMORTAL(ch))
    {
	history = &immtalk_history;
	pos = POSITION;
	argument = one_argument(argument,arg);
	if ( arg[0] == '\0')
	 pos += MAX_COMM_HISTORY - DEFAULT_RESULTS;
	else if (is_number(arg))
	{
	    arg_number = atoi(arg);
            if (arg_number > MAX_COMM_HISTORY || arg_number < 1)
            {
            	printf_to_char(ch, "Argument should be a number from 1 to %d.\n\r",MAX_COMM_HISTORY);
            return;
            }
	    else
	     pos += MAX_COMM_HISTORY-arg_number;
    	}

    }
    else if (!strcmp(arg,"savant") && IS_IMMORTAL(ch))
    {
        history = &savant_history;
        pos = POSITION;
        argument = one_argument(argument,arg);
        if ( arg[0] == '\0')
         pos += MAX_COMM_HISTORY - DEFAULT_RESULTS;
        else if (is_number(arg))
        {
            arg_number = atoi(arg);
            if (arg_number > MAX_COMM_HISTORY || arg_number < 1)
            {
                printf_to_char(ch, "Argument should be a number from 1 to %d.\n\r",MAX_COMM_HISTORY);
            return;
            }
            else
             pos += MAX_COMM_HISTORY-arg_number;
        }

    }
    else
    {
	send_to_char("Bad syntax.\n\r",ch);
	return;
    } 
    if (pos > MAX_COMM_HISTORY - 1)
     pos -= MAX_COMM_HISTORY;

    output = new_buf();
    for ( ; ; )

    {
        if (ENTRY[pos].timestamp != NULL && ENTRY[pos].text != NULL && !((ENTRY[pos].channel == CHAN_BITCH && IS_SET(ch->comm,COMM_NOBITCH))))
        {
            if (IS_SET(ch->act, PLR_HOLYLIGHT) && ENTRY[pos].mimic_name != NULL)
             sprintf(buf,"%s::%s(%s{x){%c%s%s",ENTRY[pos].timestamp,
                                         ENTRY[pos].channel==CHAN_NEWBIE?"{n[Newbie] ":"",
                                         ENTRY[pos].name,
                                         ENTRY[pos].channel,
                                         ENTRY[pos].mimic_name,
                                         ENTRY[pos].text);
            else if (ENTRY[pos].invis==TRUE && !IS_SET(ch->act, PLR_HOLYLIGHT))
             sprintf(buf, "%s::%s{%c%s%s", ENTRY[pos].timestamp,
                                         ENTRY[pos].channel==CHAN_NEWBIE?"{n[Newbie] ":"",
                                         ENTRY[pos].channel,
                                         "someone",
                                         ENTRY[pos].text);
            else if (ENTRY[pos].mimic_name != NULL)
             sprintf(buf,"%s::%s{%c%s%s", ENTRY[pos].timestamp,
                                        ENTRY[pos].channel==CHAN_NEWBIE?"{n[Newbie] ":"",
                                        ENTRY[pos].channel,
                                        ENTRY[pos].mimic_name,
                                        ENTRY[pos].text);
            else
             sprintf(buf,"%s::%s{%c%s%s", ENTRY[pos].timestamp,
                                        ENTRY[pos].channel==CHAN_NEWBIE?"{n[Newbie] ":"",
                                        ENTRY[pos].channel,
                                        ENTRY[pos].name,
                                        ENTRY[pos].text);
            add_buf(output,buf);
            add_buf(output,"{x\n\r");
        }
        if ((pos == POSITION-1) || ((POSITION == 0) && (pos == MAX_COMM_HISTORY-1)))
         break;
        pos = (pos == MAX_COMM_HISTORY-1)?0:pos+1;
    }
    page_to_char(buf_string(output),ch);
    free_buf(output);
}
