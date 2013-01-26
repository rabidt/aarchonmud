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


LBOARD *lboard_daily[MAX_LBOARD_DAILY];
LBOARD *lboard_weekly[MAX_LBOARD_WEEKLY];
LBOARD *lboard_monthly[MAX_LBOARD_MONTHLY];
LBOARD *lboard_overall[MAX_LBOARD_OVERALL];

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
  strcpy(entry->name, name);
  return entry;
}

/* allocate a new leaderboard
 */
LBOARD *lboard_new()
{
#ifdef LBOARD_DEBUG
	log_string("lboard_new");
#endif
	LBOARD *board;
	board = alloc_mem(sizeof(LBOARD));
	board->head = NULL;
	board->tail = NULL;
	return board;
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
	
void update_lboard( int lboard_type, CHAR_DATA *ch, int current, int increment )
{
#ifdef LBOARD_DEBUG	
    log_string("update_lboard begin");
#endif
	if ( IS_IMMORTAL(ch) )
	{
		return;
	}
	LBOARD *daily=NULL, *weekly=NULL, *monthly=NULL, *overall=NULL;
	
	switch (lboard_type)
	{
		case LBOARD_MKILL:
			daily=lboard_daily[LBOARD_MKILL_DAILY];
			weekly=lboard_weekly[LBOARD_MKILL_WEEKLY];
			monthly=lboard_monthly[LBOARD_MKILL_MONTHLY];
			overall=lboard_overall[LBOARD_MKILL_OVERALL];
			break;
		case LBOARD_QCOMP:
			daily=lboard_daily[LBOARD_QCOMP_DAILY];
			weekly=lboard_weekly[LBOARD_QCOMP_WEEKLY];
			monthly=lboard_monthly[LBOARD_QCOMP_MONTHLY];
			overall=lboard_overall[LBOARD_QCOMP_OVERALL];
			break;
		case LBOARD_QFAIL:
			daily=lboard_daily[LBOARD_QFAIL_DAILY];
			weekly=lboard_weekly[LBOARD_QFAIL_WEEKLY];
			monthly=lboard_monthly[LBOARD_QFAIL_MONTHLY];
			overall=lboard_overall[LBOARD_QFAIL_OVERALL];
			break;
		
		/* periodic only */
		case LBOARD_QPNT:
			daily=lboard_daily[LBOARD_QPNT_DAILY];
			weekly=lboard_weekly[LBOARD_QPNT_WEEKLY];
			monthly=lboard_monthly[LBOARD_QPNT_MONTHLY];
			break;

		/* overall only */
		case LBOARD_BHD:
			overall=lboard_overall[LBOARD_BHD_OVERALL];
			break;
		case LBOARD_WKILL:
			overall=lboard_overall[LBOARD_WKILL_OVERALL];
			break;
		case LBOARD_EXPL:
			overall=lboard_overall[LBOARD_EXPL_OVERALL];
			break;


	}
	
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
    entry = find_in_lboard( &(*board), ch->name );

    if ( entry == NULL )
    {
		#ifdef LBOARD_DEBUG	
		log_string("update_lboard: entry == NULL");
		#endif
		add_to_lboard( &(*board), ch->name, increment);
		return;
    }

	#ifdef LBOARD_DEBUG	
    log_string("entry->value += increment");
	#endif
    entry->value += increment;

    if ( entry->previous == NULL )
	return;
	
	update_lboard_order( &(*board) , &entry); 
}

void update_lboard_overall( LBOARD **board, CHAR_DATA *ch, int current, int increment)
 {
	LBOARD_ENTRY *entry;
    entry = find_in_lboard( &(*board), ch->name );

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
			entry=add_to_lboard( &(*board), ch->name, current);
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
			if (cnt > 19)
			{
				/* already at 20, let's see if this even qualifies */
				#ifdef LBOARD_DEBUG	
				log_string("update_lboard_overall: cnt > 19");
				#endif

				if (current > (*board)->tail->value )
				{
					LBOARD_ENTRY *tmp;
					entry = add_to_lboard( &(*board), ch->name, current);
					/* Means we need to kill the 20th entry (penultimate now)
						and push 21 (new one) up to 20 */
					tmp=(*board)->tail->previous;
					(*board)->tail->previous->previous->next=(*board)->tail;
					(*board)->tail->previous=(*board)->tail->previous->previous;
					
					(*board)->tail->rank=(*board)->tail->previous->rank+1;
					free_mem(tmp, sizeof(LBOARD_ENTRY) );
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
				log_string("update_lboard_overall: cnt not > 20");
				#endif

				/* board not full yet anyway */
				entry = add_to_lboard( &(*board), ch->name, current);
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
	
	update_lboard_order( &(*board) , &entry); 
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
		
    for ( entry=(*board)->head ; entry != NULL ; entry = entry->next )
    {
		if ( !strcmp( name, entry->name ) )
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
	
	LBOARD **board_array;
	int max;
	if ( !strcmp( arg1, "daily" ) )
	{
		board_array=lboard_daily;
		max=MAX_LBOARD_DAILY;
	}
	else if ( !strcmp ( arg1, "weekly" ) )
	{
		board_array=lboard_weekly;
		max=MAX_LBOARD_WEEKLY;
	}
	else if ( !strcmp ( arg1, "monthly" ) )
	{
		board_array=lboard_monthly;
		max=MAX_LBOARD_MONTHLY;
	}
	else if ( !strcmp ( arg1, "overall" ) )
	{
		board_array=lboard_overall;
		max=MAX_LBOARD_OVERALL;
	}
	else
	{
		send_to_char("Invalid argument.\n\r", ch);
		return;
	}
	
	if (arg2[0] == '\0')
	{
		print_lboard_list_to_char( board_array , max, ch );
		return;
	}

	int index=atoi(arg2);
	/* atoi returns 0 for non valid integer */
	/* we printed as index+1 so need to account for that here */
	if (index > 0 && index < (max+1) )
	{
		char name[MSL];
		if ( arg3[0] == '\0' ) /* no arg3, let's print their status */
		{
			strcpy( name, ch->name);
		}
		else
		{
			strcpy( name, capitalize(arg3) );
		}
		
		LBOARD_ENTRY *entry=find_in_lboard( &board_array[index-1], name );
		
		if ( entry != NULL ) /* print with entry->rank highlighted. Won't actually highlight if it's over 20 anyway */
			print_lboard_to_char( board_array[index-1] , ch, MAX_DISPLAY_ENTRY, entry->rank );
		else
			print_lboard_to_char( board_array[index-1] , ch, MAX_DISPLAY_ENTRY, 0 ); /* highlight 0 means no highlight since it loops from 1 to MAX_DISPLAY_ENTRY */
			
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
	
	send_to_char("[---DAILY BOARDS ---]\n\r", ch );
	print_lboard_list_to_char( lboard_daily, MAX_LBOARD_DAILY, ch);
	printf_to_char(ch, "Resets on: %s\n\r", ctime(&daily_reset) );

	send_to_char("[---WEEKLY BOARDS ---]\n\r", ch );
	print_lboard_list_to_char( lboard_weekly, MAX_LBOARD_WEEKLY, ch);
	printf_to_char(ch, "Resets on: %s\n\r", ctime(&weekly_reset) );
	
	send_to_char("[---MONTHLY BOARDS ---]\n\r", ch );
	print_lboard_list_to_char( lboard_monthly, MAX_LBOARD_MONTHLY, ch);
	printf_to_char(ch, "Resets on: %s\n\r", ctime(&monthly_reset) );

	send_to_char("[---OVERALL BOARDS ---]\n\r", ch );
	print_lboard_list_to_char( lboard_overall, MAX_LBOARD_OVERALL, ch);
	
}

void print_lboard_list_to_char( LBOARD **list, int max, CHAR_DATA *ch )
{
	int i;
	for ( i=0 ; i<max ; i++ )
	{
		if ( list[i] == NULL )
		{
			bugf("NULL daily board in print_lboard_list_to_char.");
			return;
		}
		/* print as i+1, we'll index as arg-1 when the time comes */
		printf_to_char( ch, "%4d: %-25s\n\r", i+1, list[i]->board_name);
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
	for ( i=0 ; i<MAX_LBOARD_DAILY ; i++ )
	{
		bprintf( fp->buf, "#LBOARD\n" );
		lboard_save_to_buffer( lboard_daily[i], fp->buf );
		bprintf( fp->buf, "\n" );
    }
	
	bprintf( fp->buf, "#WEEKLY %d\n", weekly_reset );
	for ( i=0 ; i<MAX_LBOARD_WEEKLY ; i++ )
	{
		bprintf( fp->buf, "#LBOARD\n" );
		lboard_save_to_buffer( lboard_weekly[i], fp->buf );
		bprintf( fp->buf, "\n" );
    }
	
	bprintf( fp->buf, "#MONTHLY %d\n", monthly_reset );
	for ( i=0 ; i<MAX_LBOARD_MONTHLY ; i++ )
	{
		bprintf( fp->buf, "#LBOARD\n" );
		lboard_save_to_buffer( lboard_monthly[i], fp->buf );
		bprintf( fp->buf, "\n" );
    }
	
	bprintf( fp->buf, "#OVERALL\n" );
	for ( i=0 ; i<MAX_LBOARD_OVERALL ; i++ )
	{
		bprintf( fp->buf, "#LBOARD\n" );
		lboard_save_to_buffer( lboard_overall[i], fp->buf );
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
    log_string( "save_lboards: done" );
#endif

    return fp;
}

void lboard_save_to_buffer( LBOARD *board, DBUFFER *fp )
{
	LBOARD_ENTRY *entry;
	
    if ( board == NULL )
		return;
	
	bprintf( fp, "BoardName %s~\n", board->board_name);
	
	for ( entry=board->head ; entry != NULL ; entry=entry->next )
	{
		bprintf( fp, "Entry %s %d\n", entry->name, entry->value );
	}
	
	bprintf( fp, "End\n" );

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

    //religion_list = NULL;

    /* open file */
    fclose( fpReserve );
    if ( ( fp = fopen( LBOARD_FILE, "r" ) ) == NULL )
    {
		bugf( "load_lboards: couldn't open %s", LBOARD_FILE );
		fpReserve = fopen( NULL_FILE, "r" );
		return;
    }

    /* load the lboards */
    int i;
	int max;
	LBOARD **lboard_array;
    while ( TRUE )
    {
        word = feof( fp ) ? "#END" : fread_word( fp );
		if ( !strcmp(word, "#DAILY") )
		{
			#ifdef LBOARD_DEBUG
			log_string("daily");
			#endif
			daily_reset= (time_t)fread_number( fp );
			lboard_array=lboard_daily;
			i=0;
			max= MAX_LBOARD_DAILY;
		}
		else if ( !strcmp(word, "#WEEKLY") )
		{
			#ifdef LBOARD_DEBUG
			log_string("weekly");
			#endif
			weekly_reset= (time_t)fread_number( fp );
			lboard_array=lboard_weekly;
			i=0;
			max= MAX_LBOARD_WEEKLY;
		}
		else if ( !strcmp(word, "#MONTHLY") )
		{
			monthly_reset= (time_t)fread_number( fp );
			lboard_array=lboard_monthly;
			i=0;
			max= MAX_LBOARD_MONTHLY;
		}
		else if ( !strcmp(word, "#OVERALL") )
		{
			lboard_array=lboard_overall;
			i=0;
			max= MAX_LBOARD_OVERALL;
		}
		else if ( !strcmp(word, "#LBOARD") )
		{
			if ( lboard_array == NULL )
			{
				bugf( "load_lboards: NULL lboard_array" );
				break;
			}
			lboard_array[i]=lboard_load_from_file( fp );
			if ( lboard_array == NULL )
			{
				bugf( "load_lboards: NULL lboard returned" );
				break;
			}
			i++;
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

LBOARD* lboard_load_from_file( FILE *fp )
{
    LBOARD *board;
    char *word;
    bool fMatch;
	char *name;
	int val;

#ifdef LBOARD_DEBUG	
    log_string( "lboard_load_from_file: start" );
#endif
	board=lboard_new();
	
	while ( TRUE )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
		if (!strcmp(word, "End") )
			return board;
		else if ( !strcmp(word, "BoardName") )
		{
			strcpy( board->board_name, fread_string(fp) );
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
		bprintf( fp->buf, "\n" );
    }

	bprintf( fp->buf, "#MONTHLY\n" );
	for ( result=monthly_results ; result != NULL ; result=result->next )
	{
		bprintf( fp->buf, "#RESULT %d\n", result->end_time );
		bprintf( fp->buf, "%s~", result->text );
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

			rslt->next=(*results_list);
			(*results_list)=rslt;
		}
		else if ( !strcmp(word, "End") )
		{
			/* need to reverse the list */
			LBOARD_RESULT *ptr=(*results_list);
			LBOARD_RESULT *temp;
			LBOARD_RESULT *previous=NULL;
			while ( ptr != NULL )
			{
				temp=ptr->next;
				ptr->next=previous;
				previous=ptr;
				ptr=temp;
			}
			(*results_list)=previous;
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
LBOARD_RESULT *make_result( time_t reset, LBOARD **board_array, int maxboard )
{
	char text[MSL];
	char buf[MSL];
	int i,j;
	LBOARD_RESULT *rslt=lboard_result_new();
	rslt->end_time = reset;
	strcpy(text,"");
	for ( i=0 ; i<maxboard ; i++ )
	{
		strcat(text, board_array[i]->board_name);
		strcat(text, "\n\r");
		sprintf( buf , "Rank %-25s %10s\n\r", "Player", "Value");
		strcat( text, buf);
		LBOARD_ENTRY *entry = board_array[i]->head;
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
	
	rslt=make_result( daily_reset, lboard_daily, MAX_LBOARD_DAILY);
	
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
			free_mem( next, sizeof(LBOARD_RESULT) );
			rslt->next=NULL;
			i--;
		}
		
	} while ( i > MAX_LBOARD_RESULT );
	
	
	for ( i=0 ; i<MAX_LBOARD_DAILY ; i++ )
	{
		reset_lboard( &(lboard_daily[i]) );
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
	
	rslt=make_result( weekly_reset, lboard_weekly, MAX_LBOARD_WEEKLY);
	
	rslt->next = weekly_results;
	weekly_results = rslt;

	int i;
	for ( i=0 ; i<MAX_LBOARD_WEEKLY ; i++ )
	{
		reset_lboard( &(lboard_weekly[i]) );
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
	rslt=make_result( monthly_reset, lboard_monthly, MAX_LBOARD_MONTHLY);
	
	rslt->next = monthly_results;
	monthly_results = rslt;

	int i;
	
	for ( i=0 ; i<MAX_LBOARD_MONTHLY ; i++ )
	{
		reset_lboard( &(lboard_monthly[i]) );
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
			free_mem( entry->previous, sizeof(LBOARD_ENTRY) );
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
	else if ( !strcmp( arg1, "daily" ) )
	{
		result=daily_results;
	}
	else if ( !strcmp( arg1, "weekly" ) )
	{
		result=weekly_results;
	}
	else if ( !strcmp( arg1, "monthly" ) )
	{
		result=monthly_results;
	}
	else
	{
		send_to_char("lhistory [daily|weekly|monthly] <index>\n\r",ch);
		return;
	}
	
	if ( arg2[0] == '\0' )
	{
		int i=1;
		for ( ; result != NULL ; result=result->next )
		{
			printf_to_char(ch, "%3d: %-25s", i, ctime(&(result->end_time)) );
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

