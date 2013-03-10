/* leaderboards for Aarchon MUD by Clayton Richey (Vodur) */
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "leaderboard.h"
#include "buffer_util.h"


msl_string lboard_names[MAX_LBOARD]=
{
	{"Mob Kills"},			/* LBOARD_MKILL */
	{"Quests Completed"},	/* LBOARD_QCOMP */
	{"Beheads"},			/* LBOARD_BHD   */
	{"Quest Points"},		/* LBOARD_QPNT  */
	{"War Kills"},			/* LBOARD_WKILL */
	{"Rooms Explored"},		/* LBOARD_EXPL  */
	{"Quests Failed"},		/* LBOARD_QFAIL */
	{"Levels Gained"},		/* LBOARD_LEVEL */
	{"Player Kills"}		/* LBOARD_PKILL */
};

LBOARD_TABLE_ENTRY lboard_daily[MAX_LBOARD]=
{
/*  enabled? */
	{ TRUE  , NULL }, /* LBOARD_MKILL */
	{ TRUE  , NULL }, /* LBOARD_QCOMP */
	{ FALSE , NULL }, /* LBOARD_BHD   */
	{ TRUE  , NULL }, /* LBOARD_QPNT  */
	{ FALSE , NULL }, /* LBOARD_WKILL */
	{ FALSE , NULL }, /* LBOARD_EXPL  */
	{ TRUE  , NULL }, /* LBOARD_QFAIL */
	{ TRUE  , NULL }, /* LBOARD_LEVEL */
	{ FALSE , NULL } /* LBOARD_PKILL */
};

LBOARD_TABLE_ENTRY lboard_weekly[MAX_LBOARD]=
{
/*  enabled? */
	{ TRUE  , NULL }, /* LBOARD_MKILL */
	{ TRUE  , NULL }, /* LBOARD_QCOMP */
	{ FALSE , NULL }, /* LBOARD_BHD   */
	{ TRUE  , NULL }, /* LBOARD_QPNT  */
	{ FALSE , NULL }, /* LBOARD_WKILL */
	{ FALSE , NULL }, /* LBOARD_EXPL  */
	{ TRUE  , NULL }, /* LBOARD_QFAIL */
	{ TRUE  , NULL }, /* LBOARD_LEVEL */
	{ FALSE , NULL } /* LBOARD_PKILL */
};

LBOARD_TABLE_ENTRY lboard_monthly[MAX_LBOARD]=
{
/*  enabled? */
	{ TRUE  , NULL }, /* LBOARD_MKILL */
	{ TRUE  , NULL }, /* LBOARD_QCOMP */
	{ FALSE , NULL }, /* LBOARD_BHD   */
	{ TRUE  , NULL }, /* LBOARD_QPNT  */
	{ FALSE , NULL }, /* LBOARD_WKILL */
	{ FALSE , NULL }, /* LBOARD_EXPL  */
	{ TRUE  , NULL }, /* LBOARD_QFAIL */
	{ TRUE  , NULL }, /* LBOARD_LEVEL */
	{ FALSE , NULL } /* LBOARD_PKILL */
};

LBOARD_TABLE_ENTRY lboard_overall[MAX_LBOARD]=
{
/*  enabled? */
	{ TRUE  , NULL }, /* LBOARD_MKILL */
	{ TRUE  , NULL }, /* LBOARD_QCOMP */
	{ TRUE  , NULL }, /* LBOARD_BHD   */
	{ FALSE , NULL }, /* LBOARD_QPNT  */
	{ TRUE  , NULL }, /* LBOARD_WKILL */
	{ TRUE  , NULL }, /* LBOARD_EXPL  */
	{ TRUE  , NULL }, /* LBOARD_QFAIL */
	{ FALSE , NULL }, /* LBOARD_LEVEL */
	{ TRUE  , NULL } /* LBOARD_PKILL */
};

LBOARD *daily_results=NULL;
LBOARD *weekly_results=NULL;
LBOARD *monthly_results=NULL;


time_t daily_reset=0;
time_t weekly_reset=0;
time_t monthly_reset=0;

/* allocate a new leaderboard entry
 */
LBOARD_ENTRY *lboard_entry_new( char *name, int value )
{
#ifdef LBOARD_DEBUG
    log_string("lboard_entry_new start");
#endif
  
  LBOARD_ENTRY *entry;
  entry = alloc_mem(sizeof(LBOARD_ENTRY));
  entry->value = value;
  entry->next = NULL;
  entry->previous = NULL;
  entry->name =str_dup(name);
  return entry;
}

void lboard_entry_free(LBOARD_ENTRY *entry)
{
	free_string(entry->name);
	free_mem(entry, sizeof(LBOARD_ENTRY) );
}


/* allocate a new leaderboard
 */
LBOARD *lboard_new(int type)
{
#ifdef LBOARD_DEBUG
	log_string("lboard_new");
#endif
	LBOARD *board;
	board = alloc_mem(sizeof(LBOARD));
	board->board_name = &(lboard_names[type]);
	board->head = NULL;
	board->tail = NULL;
	return board;
}
void lboard_free(LBOARD *board)
{
	free_mem(board, sizeof(LBOARD) );
}

LBOARD_RESULT *lboard_result_new()
{
#ifdef LBOARD_DEBUG
	log_string("lboard_result_new");
#endif
	LBOARD_RESULT *result;
	result = alloc_mem(sizeof(LBOARD_RESULT));
	result->next = NULL;
	result->text = NULL;
	result->end_time = 0;
	return result;
}
void lboard_result_free(LBOARD_RESULT *result)
{
	free_mem(result, sizeof(LBOARD_RESULT) );
}

	
void update_lboard( int lboard_type, CHAR_DATA *ch, int current, int increment )
{
#ifdef LBOARD_DEBUG	
    log_string("update_lboard begin");
#endif
	if ( IS_IMMORTAL(ch) )
	{
		return;
	}
	LBOARD *daily=lboard_daily[lboard_type].board;
	LBOARD *weekly=lboard_weekly[lboard_type].board;
	LBOARD *monthly=lboard_monthly[lboard_type].board;
	LBOARD *overall=lboard_overall[lboard_type].board;
	
	if ( daily != NULL && increment != 0 )
		update_lboard_periodic( &daily, ch, increment);
	if ( weekly != NULL && increment != 0 )
		update_lboard_periodic( &weekly, ch, increment);
	if ( monthly != NULL && increment != 0 )
		update_lboard_periodic( &monthly, ch, increment);
	
	if ( overall != NULL )
		update_lboard_overall( &overall, ch, current,increment);
 }
 
 void update_lboard_periodic( LBOARD **board, CHAR_DATA *ch, int increment)
 {
	LBOARD_ENTRY *entry;
    entry = find_in_lboard( board, ch->name );

    if ( entry == NULL )
    {
		#ifdef LBOARD_DEBUG	
		log_string("update_lboard: entry == NULL");
		#endif
		add_to_lboard( board, ch->name, increment);
		return;
    }

	#ifdef LBOARD_DEBUG	
    log_string("entry->value += increment");
	#endif
    entry->value += increment;

    if ( entry->previous == NULL )
	return;
	
	update_lboard_order( board , &entry); 
}

void update_lboard_overall( LBOARD **board, CHAR_DATA *ch, int current, int increment)
 {
	LBOARD_ENTRY *entry;
    entry = find_in_lboard( board, ch->name );

    if ( entry == NULL )
    {
		#ifdef LBOARD_DEBUG	
		log_string("update_lboard_overall: entry == NULL");
		#endif
		
		if ( (*board)->tail == NULL )
		{
			#ifdef LBOARD_DEBUG	
			log_string("update_lboard_overall: empty board");
			#endif

			/* empty board, let's add it */
			entry=add_to_lboard( board, ch->name, current);
		}
		else
		{
			#ifdef LBOARD_DEBUG	
			log_string("update_lboard_overall: count entries");
			#endif

			/* special case for overall
			We only ever want 20 entries.
			Bump the tail from the list every time we 
			add somebody and it's already full*/
			int cnt=(*board)->tail->rank;
			if (cnt >= MAX_DISPLAY_ENTRY)
			{
				/* already at 20, let's see if this even qualifies */
				#ifdef LBOARD_DEBUG	
				log_string("update_lboard_overall: cnt >= MAX_DISPLAY_ENTRY");
				#endif

				if (current > (*board)->tail->value )
				{
					LBOARD_ENTRY *tmp;
					entry = add_to_lboard( board, ch->name, current);
					/* Means we need to kill the 20th entry (penultimate now)
						and push 21 (new one) up to 20 */
					tmp=(*board)->tail->previous;
					(*board)->tail->previous->previous->next=(*board)->tail;
					(*board)->tail->previous=(*board)->tail->previous->previous;
					
					(*board)->tail->rank=(*board)->tail->previous->rank+1;
					lboard_entry_free(tmp);
				}
				else
				{
					/*doesn't qualify for the board yet */
					return;
				}
			}
			else
			{
				#ifdef LBOARD_DEBUG	
				log_string("update_lboard_overall: cnt not >= MAX_DISPLAY_ENTRY");
				#endif

				/* board not full yet anyway */
				entry = add_to_lboard( board, ch->name, current);
			}
		}
    }
	else
	{
		/* entry already exists, update the value */
		#ifdef LBOARD_DEBUG	
		log_string("entry->value += increment");
		#endif
		entry->value = current;
	}
	
	if ( entry == NULL )
	{
		#ifdef LBOARD_DEBUG	
		log_string("update_lboard_overall: entry still NULL");
		#endif

		bugf("NULL entry!");
		return;
	}
    if ( entry->previous == NULL )
	{
		#ifdef LBOARD_DEBUG	
		log_string("update_lboard_overall: entry->previous NULL");
		#endif

		return;
	}
	
	update_lboard_order( board , &entry); 
}
	
void update_lboard_order( LBOARD **board, LBOARD_ENTRY **entry)
{
	#ifdef LBOARD_DEBUG
	log_string("update_lboard_order: start");
	#endif
	
	#ifdef LBOARD_DEBUG
	char buf[MSL];
	sprintf(buf, "entry->value : %d",(*entry)->value);
	log_string(buf);
	sprintf(buf, "entry->previous->value : %d",(*entry)->previous->value);
	log_string(buf);
	
/*	LBOARD_ENTRY *temp;
	for (temp=(*board)->head ; temp!= NULL ; temp=temp->next)
	{
		sprintf( buf, "%d %s %d", temp->rank, temp->name, temp->value );
		log_string(buf);
	}*/
	#endif
	
	while ( (*entry)->value > (*entry)->previous->value )
    {
		#ifdef LBOARD_DEBUG	
		log_string("Reordering linked list.");
		#endif
		LBOARD_ENTRY *one,*two,*three,*four;
		/* Represents the new order
		   Essentially we are swapping "two" and "three"
		   */
		one = (*entry)->previous->previous;
		two = (*entry);
		three = (*entry)->previous;
		four = (*entry)->next;

		/* need to account for head and tail switching */
	
		if ( one != NULL )
		{
			one->next = two;
			two->rank = one->rank+1;
		}
		else
		{
			/* This means three is current head */
			/* so let's make two the head */
			(*board)->head = two;
			two->rank= 1;
		}
	
		two->previous = one;
		two->next = three;
		

		three->previous = two;
		three->next = four;
		three->rank = two->rank+1;

		if ( four != NULL )
		{
			four->previous = three;
		}
		else
		{
			/* This means two is current tail */
			/* so let's make three the tail */
			(*board)->tail=three;
		}
		if ( (*entry)->previous == NULL )
			break;
	}
    return;

}

void remove_from_all_lboards( char *name )
{
	int i;
	
	for (i=0; i < MAX_LBOARD; i++)
	{
		if ( lboard_daily[i].board != NULL )
			remove_from_lboard( &(lboard_daily[i].board), name );
	}

	for (i=0; i < MAX_LBOARD; i++)
	{
		if ( lboard_weekly[i].board != NULL )
			remove_from_lboard( &(lboard_weekly[i].board), name );
	}
	
	for (i=0; i < MAX_LBOARD; i++)
	{
		if ( lboard_monthly[i].board != NULL )
			remove_from_lboard( &(lboard_monthly[i].board), name );
	}
	
	for (i=0; i < MAX_LBOARD; i++)
	{
		if ( lboard_overall[i].board != NULL )
		remove_from_lboard( &(lboard_overall[i].board), name );
	}
	
}


void remove_from_lboard( LBOARD **board, char *name )
{
	/* board is the address of the local pointer
	   *board is the value passed (an address)
	   **board is the value that the address passed points to*/
				
	LBOARD_ENTRY *entry = find_in_lboard( board, name);
	
	if ( entry == NULL)
		return;
	
	if ( (*board)->head == entry )
	{
		(*board)->head = entry->next;
	}
	else
	{
		entry->previous->next=entry->next;
	}
	
	if ( entry->next != NULL )
	{
		entry->next->previous=entry->previous;
	}
	
	LBOARD_ENTRY *temp;
	for ( temp=entry->next ; temp != NULL; temp=temp->next )
	{
		if ( temp->previous == NULL)
			temp->rank=1;
		else
			temp->rank = temp->previous->rank + 1;
	}
		
	lboard_entry_free( entry );
	return;
}

LBOARD *add_to_lboard( LBOARD **board, char *name, int increment )
{
	#ifdef LBOARD_DEBUG	
    log_string("add_to_lboard start");
	#endif
    /* Let's assume we always add them at the bottom of the
       board */
	LBOARD_ENTRY *entry = lboard_entry_new( name, increment);
	
	if ((*board)==NULL)
	{
		bugf("Null board in add_to_lboard.");
		return;
	}
    
	if ( (*board)->head == NULL )
    {
		#ifdef LBOARD_DEBUG	
		log_string("add_to_lboard: board is empty");
		#endif
		(*board)->head=entry;
		(*board)->tail=entry;
		entry->rank=1;
    } 
    else
    {	
		#ifdef LBOARD_DEBUG	
		log_string("add_to_lboard: board isn't emtpy");
		#endif
		(*board)->tail->next=entry;
		entry->previous=(*board)->tail;
		(*board)->tail=entry;
		
		entry->rank = entry->previous->rank+1;
    }
    return entry;
}

LBOARD_ENTRY *find_in_lboard( LBOARD **board, char *name )
{
	#ifdef LBOARD_DEBUG	
    log_string("find_in_lboard start");
	#endif
	LBOARD_ENTRY *entry;
	
	if ( (*board)->head == NULL )
		return NULL;
	
	if ( name==NULL )
	{
		bugf("NULL name sent to find_in_lboard.");
		return NULL;
	}
	
    for ( entry=(*board)->head ; entry != NULL ; entry = entry->next )
    {
		if ( entry->name == NULL)
		{
			bugf("NULL entry name in find_in_lboard");
			return NULL;
		}
		if ( !strcmp( capitalize(name), capitalize(entry->name) ) )
		{
			#ifdef LBOARD_DEBUG	
			log_string("find_in_lboard: entry found");
			#endif
			return entry;
		}
    }
    return NULL;
}


/* do_lboard and related helper functions for displaying data to characters */
void do_lboard( CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	char arg3[MIL];
	
	argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
	argument = one_argument( argument, arg3 );
	
	if (arg1[0] == '\0')
	{
		print_all_lboard_lists_to_char( ch );
		return;
	}
	
	LBOARD_TABLE_ENTRY *table;
	
	if ( arg1[0]== 'd' )
	{
		table=lboard_daily;
	}
	else if ( arg1[0]== 'w' )
	{
		table=lboard_weekly;
	}
	else if ( arg1[0]== 'm' )
	{
		table=lboard_monthly;
	}
	else if ( arg1[0]== 'o' )
	{
		table=lboard_overall;
	}
	else
	{
		send_to_char("Invalid argument.\n\r", ch);
		return;
	}
	
	if (arg2[0] == '\0')
	{
		print_lboard_list_to_char( table , ch );
		return;
	}

	int index=atoi(arg2);
	/* atoi returns 0 for non valid integer */
	
	if (index > 0 )
	{
		/* some nasty magic because we printed them in sequence, skipping NULL boards */
		int num=0;
		int i;
		bool found=FALSE;
		for (i=0; i<MAX_LBOARD ; i++)
		{
			if (table[i].enabled == TRUE)
			{
				num++;
				if ( num==index )
				{
					found=TRUE;
					break;
				}
					
			}
		}
		
		if ( !found || table[i].board == NULL )
		{
			send_to_char("That board doesn't exist!\n\r",ch);
			return;
		}
		
		
		char name[MSL];
		if ( arg3[0] == '\0' ) /* no arg3, let's print their status */
		{
			strcpy(name, ch->name);
		}
		else
		{
			arg3[0]=UPPER(arg3[0]);
			strcpy(name, arg3);
		}
		
		LBOARD_ENTRY *entry=find_in_lboard( &(table[i].board), name );
				
		if ( entry != NULL ) /* print with entry->rank highlighted. Won't actually highlight if it's over 20 anyway */
			print_lboard_to_char( table[i].board , ch, MAX_DISPLAY_ENTRY, entry->rank );
		else
			print_lboard_to_char( table[i].board , ch, MAX_DISPLAY_ENTRY, 0 ); /* highlight 0 means no highlight since it loops from 1 to MAX_DISPLAY_ENTRY */
			
		if ( entry == NULL)
		{
			printf_to_char( ch, "No score for %s.\n\r" , name);
		}
		else if ( entry->rank > MAX_DISPLAY_ENTRY)
		{
			printf_to_char( ch, "{W%3d: %-25s %10d{x\n\r", entry->rank,  entry->name, entry->value);
		}

		return;
	}
	else
	{
		send_to_char("Invalid number, try again.\n\r", ch);
		return;
	}

		
}

void print_all_lboard_lists_to_char( CHAR_DATA *ch )
{
	int i;
	LBOARD *board;
	
	send_to_char("[--- DAILY BOARDS -----]\n\r", ch );
	print_lboard_list_to_char( lboard_daily, ch);
	printf_to_char(ch, "Resets on: %s\n\r", ctime(&daily_reset) );

	send_to_char("[--- WEEKLY BOARDS  ---]\n\r", ch );
	print_lboard_list_to_char( lboard_weekly, ch);
	printf_to_char(ch, "Resets on: %s\n\r", ctime(&weekly_reset) );
	
	send_to_char("[--- MONTHLY BOARDS ---]\n\r", ch );
	print_lboard_list_to_char( lboard_monthly, ch);
	printf_to_char(ch, "Resets on: %s\n\r", ctime(&monthly_reset) );

	send_to_char("[--- OVERALL BOARDS ---]\n\r", ch );
	print_lboard_list_to_char( lboard_overall, ch);
	
}

void print_lboard_list_to_char( LBOARD_TABLE_ENTRY *table, CHAR_DATA *ch )
{
	int i;
	int num=1;
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( table[i].board == NULL )
			continue;
		/* print as i+1, we'll index as arg-1 when the time comes */
		printf_to_char( ch, "%4d: %-25s\n\r", num, table[i].board->board_name);
		num++;
	}

}

void print_lboard_to_char( LBOARD *board , CHAR_DATA *ch, int entries, int highlight_rank )
{
	if ( board == NULL )
	{
		bugf("NULL board in print_lboard_to_char");
		return;
	}

	LBOARD_ENTRY *entry;
	
	printf_to_char(ch,"%s Leaderboard\n\r", board->board_name);

	if ( board->head == NULL)
	{
		send_to_char("No entries.\n\r",ch);
		return;
	}
	
	printf_to_char( ch, "Rank %-25s %10s\n\r", "Player", "Value");
	int j=1;
	for ( entry=board->head ; entry != NULL ; entry = entry->next )
	{	
		if ( j > entries )
			break;
		if (j==highlight_rank)
		{
			printf_to_char( ch, "{W%3d: %-25s %10d{x\n\r", entry->rank,  entry->name, entry->value);
		}
		else
		{
			printf_to_char( ch, "%3d: %-25s %10d\n\r", entry->rank,  entry->name, entry->value);
		}
		j++;
	}
	send_to_char("\n\r",ch);
}	


/* Saving and loading functions */
MEMFILE* save_lboards()
{
    MEMFILE *fp;

#ifdef LBOARD_DEBUG	
    log_string( "save_lboards: start" );
#endif

    /* open file */
    fp = memfile_new(LBOARD_FILE, 16*1024);
    if (fp == NULL)
    {
		bugf( "save_lboards: couldn't open memory file for %s", LBOARD_FILE );
		return NULL;
    }

    /* save the religions */
	
    sh_int i;
	
	bprintf( fp->buf, "#DAILY %d\n", daily_reset );
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( lboard_daily[i].board != NULL )
			lboard_save_to_buffer( lboard_daily[i].board, i, fp->buf );
    }
	bprintf( fp->buf, "EndSect\n" );
	
	bprintf( fp->buf, "#WEEKLY %d\n", weekly_reset );
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( lboard_weekly[i].board != NULL )
			lboard_save_to_buffer( lboard_weekly[i].board, i, fp->buf );
    }
	bprintf( fp->buf, "EndSect\n" );
	
	bprintf( fp->buf, "#MONTHLY %d\n", monthly_reset );
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( lboard_monthly[i].board != NULL )
			lboard_save_to_buffer( lboard_monthly[i].board, i, fp->buf );
    }
	bprintf( fp->buf, "EndSect\n" );
	
	bprintf( fp->buf, "#OVERALL\n" );
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( lboard_overall[i].board != NULL )
			lboard_save_to_buffer( lboard_overall[i].board, i, fp->buf );
    }
	bprintf( fp->buf, "EndSect\n" );
	
	bprintf( fp->buf, "#END\n" );
	/* check for overflow */
	if (fp->buf->overflowed)
	{
		memfile_free(fp);
		return NULL;
	}

#ifdef LBOARD_DEBUG	
    log_string( "save_lboards: done" );
#endif

    return fp;
}

void lboard_save_to_buffer( LBOARD *board, int type, DBUFFER *fp )
{
	LBOARD_ENTRY *entry;
	
	if ( board != NULL )
	{
		bprintf( fp, "#LBOARD %d\n", type );
				
		for ( entry=board->head ; entry != NULL ; entry=entry->next )
		{
			bprintf( fp, "Entry %s %d\n", entry->name, entry->value );
		}
	
	bprintf( fp, "End\n" );
	}
	else
	{
		bugf("lboard_save_to_buffer: NULL board");
	}
	
#ifdef LBOARD_DEBUG	
    log_string( "lboard_save_to_buffer: done" );
#endif

}

void load_lboards()
{
    LBOARD *board;
    FILE *fp;
    char *word;
    bool fMatch;

#ifdef LBOARD_DEBUG	
    log_string( "load_lboards: start" );
#endif

     /* open file */
    fclose( fpReserve );
    if ( ( fp = fopen( LBOARD_FILE, "r" ) ) == NULL )
    {
		bugf( "load_lboards: couldn't open %s", LBOARD_FILE );
		fpReserve = fopen( NULL_FILE, "r" );
		return;
    }

    /* load the lboards */
	int type;	
	struct lboard_table_entry *table;
	
    while ( TRUE )
    {
        word = feof( fp ) ? "#END" : fread_word( fp );
		if ( !strcmp(word, "#DAILY") )
		{
			#ifdef LBOARD_DEBUG
			log_string("daily");
			#endif
			daily_reset= (time_t)fread_number( fp );
			table=lboard_daily;
		}
		else if ( !strcmp(word, "#WEEKLY") )
		{
			#ifdef LBOARD_DEBUG
			log_string("weekly");
			#endif
			weekly_reset= (time_t)fread_number( fp );
			table=lboard_weekly;
		}
		else if ( !strcmp(word, "#MONTHLY") )
		{
			monthly_reset= (time_t)fread_number( fp );
			table=lboard_monthly;
		}
		else if ( !strcmp(word, "#OVERALL") )
		{
			table=lboard_overall;
		}
		else if ( !strcmp(word, "#LBOARD") )
		{
			if ( table == NULL )
			{
				bugf( "load_lboards: NULL table pointer" );
				break;
			}
			
			type=fread_number( fp );
			if (table[type].enabled == TRUE)
			{
				table[type].board=lboard_load_from_file( fp, type );
			}
			else
			{
				bugf("Found board that is not enabled.");
				lboard_load_from_file( fp, type );
			}
		}
		else if ( !strcmp(word, "EndSect") )
		{
			int cnt;
			
			for ( cnt=0 ; cnt < MAX_LBOARD ; cnt++ )
			{
				if ( table[cnt].enabled == TRUE && table[cnt].board == NULL )
				{
					bugf("Board didn't load, creating new one.");
					table[cnt].board=lboard_new(cnt);
				}
			}
		}
		else if ( !strcmp(word, "#END") )
			break;
		else
		{
			bugf( "load_lboards: invalid section: %s", word );
			break;
		}
    }    

    /* close file */
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );  



    check_lboard_reset();

#ifdef LBOARD_DEBUG	
    log_string( "load_lboards: done" );
#endif

}

LBOARD* lboard_load_from_file( FILE *fp, int type )
{
    LBOARD *board;
    char *word;
    bool fMatch;
	char *name;
	int val;

#ifdef LBOARD_DEBUG	
    log_string( "lboard_load_from_file: start" );
#endif
	board=lboard_new(type);
	
	while ( TRUE )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
		if (!strcmp(word, "End") )
		{
			return board;
		}
		else if ( !strcmp(word, "Entry") )
		{
			name=fread_word( fp );
			val=fread_number( fp );
			add_to_lboard( &board, name, val );
		}
	}
}

MEMFILE* save_lboard_results()
{
    MEMFILE *fp;

#ifdef LBOARD_DEBUG	
    log_string( "save_lboard_results: start" );
#endif

    /* open file */
    fp = memfile_new(LBOARD_RESULT_FILE, 16*1024);
    if (fp == NULL)
    {
		bugf( "save_lboard_results: couldn't open memory file for %s", LBOARD_RESULT_FILE );
		return NULL;
    }

    /* save the results */
	
    LBOARD_RESULT *result;
	
	bprintf( fp->buf, "#DAILY\n" );
	for ( result=daily_results ; result != NULL ; result=result->next )
	{
		bprintf( fp->buf, "#RESULT %d\n", result->end_time );
		bprintf( fp->buf, "%s~", result->text );
		bprintf( fp->buf, "End" );
		bprintf( fp->buf, "\n" );
    }

	bprintf( fp->buf, "#WEEKLY\n" );
	for ( result=weekly_results ; result != NULL ; result=result->next )
	{
		bprintf( fp->buf, "#RESULT %d\n", result->end_time );
		bprintf( fp->buf, "%s~", result->text );
		bprintf( fp->buf, "End" );
		bprintf( fp->buf, "\n" );
    }

	bprintf( fp->buf, "#MONTHLY\n" );
	for ( result=monthly_results ; result != NULL ; result=result->next )
	{
		bprintf( fp->buf, "#RESULT %d\n", result->end_time );
		bprintf( fp->buf, "%s~", result->text );
		bprintf( fp->buf, "End" );
		bprintf( fp->buf, "\n" );
    }
	
	bprintf( fp->buf, "#END\n" );
	/* check for overflow */
	if (fp->buf->overflowed)
	{
		memfile_free(fp);
		return NULL;
	}

#ifdef LBOARD_DEBUG	
    log_string( "save_lboard_results: done" );
#endif

    return fp;
}

void load_lboard_results()
{
    LBOARD *board;
    FILE *fp;
    char *word;
    bool fMatch;

#ifdef LBOARD_DEBUG	
    log_string( "load_lboard_results: start" );
#endif

    //religion_list = NULL;

    /* open file */
    fclose( fpReserve );
    if ( ( fp = fopen( LBOARD_RESULT_FILE, "r" ) ) == NULL )
    {
		bugf( "load_lboard_results: couldn't open %s", LBOARD_RESULT_FILE );
		fpReserve = fopen( NULL_FILE, "r" );
		return;
    }

    /* load the lboards */
    int i;
	int max;
	LBOARD_RESULT **results_list;
	LBOARD_RESULT *temp_list;
	
    while ( TRUE )
    {
        word = feof( fp ) ? "#END" : fread_word( fp );
		if ( !strcmp(word, "#DAILY") )
		{
			#ifdef LBOARD_DEBUG
			log_string("daily");
			#endif
			results_list = &daily_results;
		}
		else if ( !strcmp(word, "#WEEKLY") )
		{
			#ifdef LBOARD_DEBUG
			log_string("weekly");
			#endif
			results_list = &weekly_results;		
		}
		else if ( !strcmp(word, "#MONTHLY") )
		{
			results_list = &monthly_results;		
		}
		else if ( !strcmp(word, "#RESULT") )
		{
			if ( results_list == NULL )
			{
				bugf( "load_lboard_results: NULL lboard_list" );
				break;
			}
			LBOARD_RESULT *rslt= lboard_result_new();
			rslt->end_time = fread_number( fp );
			rslt->text = fread_string( fp );

			LBOARD_RESULT *tmp=(*results_list);
			if (tmp==NULL)
			{
				(*results_list)=rslt;
				rslt->next=NULL;
			}
			else
			{
				while(1)
				{
					if (tmp->next==NULL)
					{
						tmp->next=rslt;
						rslt->next=NULL;
						break;
					}
					else
						tmp=tmp->next;
				}
			}

		}
		else if ( !strcmp(word, "End") )
		{
			//nuffin
		}
		else if ( !strcmp(word, "#END") )
			break;
		else
		{
			bugf( "load_lboards: invalid section: %s", word );
			break;
		}
    }    

    /* close file */
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );  

#ifdef LBOARD_DEBUG	
    log_string( "load_lboards: done" );
#endif

}

/* Functions related to resetting/clearing */
LBOARD_RESULT *make_result( time_t reset, LBOARD_TABLE_ENTRY *table)
{
	char text[MSL];
	char buf[MSL];
	int i,j;
	LBOARD_RESULT *rslt=lboard_result_new();
	rslt->end_time = reset;
	strcpy(text,"");
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( table[i].board == NULL )
			continue;
		strcat(text, table[i].board->board_name);
		strcat(text, "\n\r");
		sprintf( buf , "Rank %-25s %10s\n\r", "Player", "Value");
		strcat( text, buf);
		LBOARD_ENTRY *entry = table[i].board->head;
		for ( j=0; j<RESULT_NUM_RANK ; j++ )
		{
			if ( entry == NULL )
				break;
			
			sprintf( buf, "%3d: %-25s %10d\n\r", entry->rank, entry->name, entry->value);
			strcat( text, buf);
			entry=entry->next;
		}
		strcat( text, "\n\r" );
		rslt->text= str_dup( text );
		
	}
	return rslt;
}

void reset_daily_lboards()
{
#ifdef LBOARD_DEBUG	
    log_string( "reset_daily_lboards: start" );
#endif
	LBOARD_RESULT *rslt;
	
	if ( daily_reset != 0 ) /* Don't make a result if reset time not set */
	    rslt=make_result( daily_reset, lboard_daily);
	
	rslt->next = daily_results;
	daily_results = rslt;	
	
	int i;
	do
	{
		i=1;
		rslt=daily_results;
		LBOARD_RESULT *next=rslt->next;
		if ( next == NULL)
			break;
		else
			i++;
		
		while ( TRUE )
		{
			if ( next->next != NULL )
			{
				i++;
				rslt=next;
				next=rslt->next;
			}
			else
				break;
		}
		if ( i > MAX_LBOARD_RESULT )
		{
			lboard_result_free(next);
			rslt->next=NULL;
			i--;
		}
		
	} while ( i > MAX_LBOARD_RESULT );
	
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( lboard_daily[i].board != NULL )
		reset_lboard( &(lboard_daily[i].board) );
	}
	
	struct tm *timeinfo;
	timeinfo=localtime( &current_time);
	timeinfo->tm_hour=0;
	timeinfo->tm_min=0;
	timeinfo->tm_sec=0;
	timeinfo->tm_mday+=1;
	daily_reset=mktime(timeinfo);
}

void reset_weekly_lboards()
{
#ifdef LBOARD_DEBUG	
    log_string( "reset_weekly_lboards: start" );
#endif
	LBOARD_RESULT *rslt;
	
	if ( weekly_reset != 0 ) /* Don't make a result if reset time not set */
	    rslt=make_result( weekly_reset, lboard_weekly);
	
	rslt->next = weekly_results;
	weekly_results = rslt;

	int i;
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( lboard_weekly[i].board != NULL )
			reset_lboard( &(lboard_weekly[i].board) );
	}
	
	struct tm *timeinfo;
	timeinfo=localtime( &current_time);
	timeinfo->tm_hour=0;
	timeinfo->tm_min=0;
	timeinfo->tm_sec=0;
	timeinfo->tm_mday += 7 - timeinfo->tm_wday;
	weekly_reset=mktime(timeinfo);
	
}

void reset_monthly_lboards()
{
#ifdef LBOARD_DEBUG	
    log_string( "reset_monthly_lboards: start" );
#endif

	LBOARD_RESULT *rslt;
	if ( daily_reset != 0 ) /* Don't make a result if reset time not set */
	    rslt=make_result( monthly_reset, lboard_monthly);
	
	rslt->next = monthly_results;
	monthly_results = rslt;

	int i;
	
	for ( i=0 ; i<MAX_LBOARD ; i++ )
	{
		if ( lboard_monthly[i].board != NULL )
			reset_lboard( &(lboard_monthly[i].board) );
	}

	struct tm *timeinfo;
	timeinfo=localtime( &current_time);
	timeinfo->tm_hour=0;
	timeinfo->tm_min=0;
	timeinfo->tm_sec=0;
	timeinfo->tm_mon +=1;
	timeinfo->tm_mday= 1;	
	monthly_reset=mktime(timeinfo);
	 
}

void reset_lboard( LBOARD **board)
{
#ifdef LBOARD_DEBUG	
    log_string( "reset_lboard: start" );
#endif
	LBOARD_ENTRY *entry;
	
	for ( entry=(*board)->head ; entry != NULL ; entry=entry->next )
	{
		if (entry->previous != NULL)
			lboard_entry_free(entry->previous);
	}
	
	(*board)->head = NULL;
	(*board)->tail = NULL;
}

void do_lhistory( CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	
	argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
	
	LBOARD_RESULT *result;


	if ( arg1[0] == '\0')
	{
		send_to_char("lhistory [daily|weekly|monthly] <index>\n\r",ch);
		return;
	}
	else if ( arg1[0]=='d' )
	{
		result=daily_results;
	}
	else if ( arg1[0]=='w' )
	{
		result=weekly_results;
	}
	else if ( arg1[0]=='m' )
	{
		result=monthly_results;
	}
	else
	{
		send_to_char("lhistory [d|w|m] <index>\n\r",ch);
		return;
	}
	
	if ( arg2[0] == '\0' )
	{
		int i=1;
		for ( ; result != NULL ; result=result->next )
		{
			printf_to_char(ch, "%3d: %-25s\n\r", i, ctime(&(result->end_time)) );
			i++;
		}
		return;
	}
	
	int index = atoi( arg2 );
	if ( index > 0 )
	{
		int i=1;
		for ( result; result != NULL ; result=result->next )
		{
			if ( i == index)
			{
				printf_to_char(ch, "Ended:%s\n\r%s\n\r", ctime(&(result->end_time)), result->text );
				return;
			}
			i++;
		}
		send_to_char("Invalid index.\n\r",ch);
		return;
	}
	else
	{
		send_to_char("Invalid index.\n\r",ch);
		return;
	}
}

void check_lboard_reset()
{
        if (current_time > daily_reset)
        {
                #ifdef LBOARD_DEBUG
                log_string("reset daily boards");
                #endif
                reset_daily_lboards();
        }
        if (current_time > weekly_reset)
        {
                #ifdef LBOARD_DEBUG
                log_string("reset weekly boards");
                #endif
                reset_weekly_lboards();
        }
        if (current_time > monthly_reset)
        {
                #ifdef LBOARD_DEBUG
                log_string("reset monthly boards");
                #endif
                reset_monthly_lboards();
        }
}


