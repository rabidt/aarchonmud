#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>

#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "warfare.h"
#include "dxport.h"

/* command procedures needed */
DECLARE_DO_FUN(do_help      );
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_skills    );
DECLARE_DO_FUN(do_outfit    );
DECLARE_DO_FUN(do_unread    );
DECLARE_DO_FUN(do_stats     );
DECLARE_DO_FUN(do_etls      );
DECLARE_DO_FUN(do_board     );
DECLARE_DO_FUN(do_showrace  );
DECLARE_DO_FUN(do_showsubclass);
DECLARE_DO_FUN(do_cmotd     );

extern bool            wizlock;        /* Game is wizlocked        */
extern bool            newlock;        /* Game is newlocked        */

void show_races_to_d( DESCRIPTOR_DATA *d );
void enter_game args((DESCRIPTOR_DATA *d));
void take_rom_basics args((DESCRIPTOR_DATA *d));
void take_class_defaults args((DESCRIPTOR_DATA *d));
void take_default_weapon args((DESCRIPTOR_DATA *d));
void newbie_alert args((DESCRIPTOR_DATA *d));
void take_default_stats args((CHAR_DATA *ch));
void auto_assign_stats args((CHAR_DATA *ch));
void skill_reimburse( CHAR_DATA *ch );

#define DECLARE_NANNY_FUN(func) bool func( DESCRIPTOR_DATA *d, const char *argument )
#define DEF_NANNY_FUN(func) bool func( DESCRIPTOR_DATA *d, const char *argument )

DECLARE_NANNY_FUN(get_name);
DECLARE_NANNY_FUN(get_old_password);
DECLARE_NANNY_FUN(confirm_new_name);
DECLARE_NANNY_FUN(get_new_password);
DECLARE_NANNY_FUN(confirm_new_password);
DECLARE_NANNY_FUN(nanny_remort_begin);
DECLARE_NANNY_FUN(get_new_race);
DECLARE_NANNY_FUN(get_new_sex);
DECLARE_NANNY_FUN(get_new_class);
DECLARE_NANNY_FUN(get_new_subclass);
DECLARE_NANNY_FUN(get_alignment);
DECLARE_NANNY_FUN(default_choice);
DECLARE_NANNY_FUN(gen_groups);
DECLARE_NANNY_FUN(pick_weapon);
DECLARE_NANNY_FUN(read_imotd);
DECLARE_NANNY_FUN(read_motd);
DECLARE_NANNY_FUN(break_connect);
DECLARE_NANNY_FUN(get_creation_mode);
DECLARE_NANNY_FUN(roll_stats);
DECLARE_NANNY_FUN(get_colour);

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, const char *argument )
{
      /* Delete leading spaces UNLESS character is writing a note */
      if (d->connected != CON_NOTE_TEXT)
      {
         while ( isspace(*argument) )
  	      argument++;
      }

	switch (creation_mode(d) )
	{

	default:
	    switch ( con_state(d) )
	    {
	    default:
		bug( "Nanny: bad d->connected %d.", d->connected );
		close_socket( d );
		return;
		
	    case CON_GET_NAME:
		get_name(d, argument);
		break;
		
        case CON_GET_OLD_PASSWORD:
            if ( get_old_password(d, argument) )
            {
                if ( IS_SET(d->character->act, PLR_REMORT_ROLL) )
                    remort_begin(d->character);
                else
                    read_imotd(d, argument);
            }
            break;
		
	    case CON_BREAK_CONNECT:
		if (break_connect(d, argument)) get_name(d, argument);
		break;
		
	    case CON_CONFIRM_NEW_NAME:
		if (confirm_new_name(d, argument))
		    {
			newbie_alert(d);
			get_new_password(d, argument);
		    }
		break;
		
	    case CON_GET_NEW_PASSWORD:
		if (get_new_password(d, argument)) confirm_new_password(d, argument);
		break;
		
	    case CON_CONFIRM_NEW_PASSWORD:
		if (confirm_new_password(d, argument)) get_colour(d, argument);
		break;
		
	    case CON_GET_COLOUR:
		if( get_colour(d,argument)) get_creation_mode(d, argument);
		break;

	    case CON_GET_CREATION_MODE:
		if (get_creation_mode(d, argument)) get_new_sex(d, argument);
		break;
		
	    case CON_READ_IMOTD:
		if (read_imotd(d, argument)) read_motd(d, argument);
		break;
		
	    case CON_READ_MOTD:
		if (read_motd(d, argument)) enter_game(d);
		break;
		
		/* states for new note system, (c)1995-96 erwin@pip.dknet.dk */
		/* ch MUST be PC here; have nwrite check for PC status! */
	    case CON_NOTE_TO:
		handle_con_note_to (d, argument);
		break;
		
	    case CON_NOTE_SUBJECT:
		handle_con_note_subject (d, argument);
		break; /* subject */
		
	    case CON_NOTE_EXPIRE:
		handle_con_note_expire (d, argument);
		break;
		
	    case CON_NOTE_TEXT:
		handle_con_note_text (d, argument);
		break;
		
	    case CON_NOTE_FINISH:
		handle_con_note_finish (d, argument);
		break;
		
		/* Penalty states - Rim 1/99 */
	    case CON_PENALTY_SEVERITY:
		penalty_severity(d, argument);
		break;
		
	    case CON_PENALTY_HOURS:
		penalty_hours(d, argument);
		break;
		
	    case CON_PENALTY_POINTS:
		penalty_points(d, argument);
		break;
		
	    case CON_PENALTY_PENLIST:
		penalty_penlist(d, argument);
		break;
		
	    case CON_PENALTY_CONFIRM:
		penalty_confirm(d, argument);
		break;
		
	    case CON_PENALTY_FINISH:
		penalty_finish(d, argument);
		break;

        case CON_LUA_HANDLER:
        case CON_LUA_PULSE_HANDLER:
        lua_con_handler(d, argument);
        break;
	    }	
	    break;
	    
	case CREATION_NORMAL:
	    switch ( con_state(d) )
		{
		    
		default:
		    bug( "Nanny: bad d->connected %d.", d->connected );
		    close_socket( d );
		    return;
		    
		case CON_GET_NEW_SEX:
		    if (get_new_sex(d, argument)) get_new_class(d, argument);
		    break;
		    
		case CON_GET_NEW_CLASS:
		    if (get_new_class(d, argument)) get_new_race(d, argument);
		    break;

		case CON_GET_NEW_RACE:
		    if (get_new_race(d, argument))
			{
                d->character->alignment = 0;
			    take_rom_basics(d);
			    take_class_defaults(d);
			    take_default_weapon(d);
			    take_default_stats(d->original ? d->original : d->character);
			    set_creation_state(d, CREATION_UNKNOWN);
			    read_imotd(d, argument);
			}
		    break;
		    
		}
	    break;
	    
	case CREATION_EXPERT:
	    switch ( con_state(d) )
		{
		    
		default:
		    bug( "Nanny: bad d->connected %d.", d->connected );
		    close_socket( d );
		    return;
		    
		case CON_GET_NEW_SEX:
		    if (get_new_sex(d, argument)) get_new_class(d, argument);
		    break;
		    
		case CON_GET_NEW_CLASS:
		    if (get_new_class(d, argument)) get_new_race(d, argument);
		    break;

		case CON_GET_NEW_RACE:
		    if (get_new_race(d, argument)) get_alignment(d, argument);
		    break;
		    
		case CON_GET_ALIGNMENT:
		    if (get_alignment(d, argument))
			{
			    take_rom_basics(d);
			    gen_groups(d, argument);
			}
		    break;
		    
		case CON_GEN_GROUPS:
		    if (gen_groups(d, argument)) pick_weapon(d, argument);
		    break;
		    
		case CON_PICK_WEAPON:
		    if (pick_weapon(d, argument)) roll_stats(d, argument);
		    break;
		    
		case CON_ROLL_STATS:
		    if (roll_stats(d, argument))
			{
			    set_creation_state(d, CREATION_UNKNOWN);
			    read_imotd(d, argument);
			}
		    break;
		    
		}
	    break;
	    
	case CREATION_REMORT:
	    switch ( con_state(d) )
		{
		    
		default:
		    bug( "Nanny: bad d->connected %d.", d->connected );
		    close_socket( d );
		    return;

        case CON_REMORT_BEGIN:
            nanny_remort_begin(d, argument);
            break;

        case CON_GET_NEW_SUBCLASS:
            if (get_new_subclass(d, argument)) get_new_race(d, argument);
            break;
            
		case CON_GET_NEW_RACE:
		    if (get_new_race(d, argument)) roll_stats(d, argument);
		    break;
		    
		case CON_ROLL_STATS:
		    if (roll_stats(d, argument))
			{
			    set_creation_state(d, CREATION_UNKNOWN);
			    remort_complete(d->character);
			}
		    break;
		    
		}
	    break;
	    
	}
	return;
}

// password check that handles pwd = "" and similar cases correctly
bool check_password( const char *argument, const char *pwd )
{
    if ( !pwd || !strcmp(pwd, "") )
        return TRUE;
    if ( !argument )
        return FALSE;
    const char *encrypted = crypt(argument, pwd);
    return encrypted && (strcmp(encrypted, pwd) == 0);
}
    
bool is_reserved_name( const char *name )
{
  RESERVED_DATA *res;
  
  for (res = first_reserved; res; res = res->next)
    if ((*res->name == '*' && !str_infix(res->name+1, name)) ||
        !str_cmp(res->name, name))
      return TRUE;
  return FALSE;
}


/*
 * Parse a name for acceptability.
 */
bool check_parse_name( const char *name, bool newchar )
{
    char strsave[MAX_INPUT_LENGTH];

   /*
    * Reserved words.
    */
    if ( is_reserved_name(name) && newchar )
        return FALSE;
     
   /*
    if ( is_name( name, 
    "all auto immortal self someone something the you demise balance circle loner honor none questors") )
         
        return FALSE;
    */
     
   /*
    * Length restrictions.
    */
    if ( strlen(name) <  2 )
        return FALSE;
    
    if ( strlen(name) > 12 )
        return FALSE;
    
   /*
    * Alphanumerics only.
    * Lock out IllIll twits.
    */
    {
        const char *pc;
        bool fIll,adjcaps = FALSE,cleancaps = FALSE;
        unsigned int total_caps = 0;
        
        fIll = TRUE;
        for ( pc = name; *pc != '\0'; pc++ )
        {
            if ( !isalpha(*pc) )
                return FALSE;
            
            if ( isupper(*pc)) /* ugly anti-caps hack */
            {
                if (adjcaps)
                    cleancaps = TRUE;
                total_caps++;
                adjcaps = TRUE;
            }
            else
                adjcaps = FALSE;
            
            if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
                fIll = FALSE;
        }
        
        if ( fIll )
            return FALSE;
        
        if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
            return FALSE;
    }

   /*
    * Prevent players from naming themselves after mobs.
    */
    sprintf(strsave, "%s%s", GOD_DIR, capitalize ( name) );

    if (access(strsave, 0))
    {
        {
            extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
            MOB_INDEX_DATA *pMobIndex;
            int iHash;
            
            for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
            {
                for ( pMobIndex  = mob_index_hash[iHash];
                pMobIndex != NULL;
                pMobIndex  = pMobIndex->next )
                {
                    if ( is_name( name, pMobIndex->player_name ) )
                        return FALSE;
                }
            }
        }
    }
    
    return TRUE;
}
 
 
int con_state(DESCRIPTOR_DATA *d)
{
    return ( d->connected % MAX_CON_STATE );
}

int creation_mode(DESCRIPTOR_DATA *d)
{
    return ( (d->connected - (d->connected%MAX_CON_STATE)) / MAX_CON_STATE );
}

void set_con_state(DESCRIPTOR_DATA *d, int cstate)
{
    d->connected += cstate - d->connected%MAX_CON_STATE;
    return;
}

void set_creation_state(DESCRIPTOR_DATA *d, int cmode)
{
    d->connected = d->connected%MAX_CON_STATE + cmode*MAX_CON_STATE;
    return;
}

DEF_NANNY_FUN(get_name)
{
    char buf[MAX_STRING_LENGTH], argbuf[MIL];
    bool fOld;
    
    if (con_state(d)!=CON_GET_NAME)
    {
        set_con_state(d, CON_GET_NAME);
        write_to_buffer(d,"Name: ",0);
        return FALSE;
    }
    
    if ( argument[0] == '\0' )
    {
        close_socket( d );
        return FALSE;
    }
    
    //argument[0] = UPPER(argument[0]);
    strcpy(argbuf, argument);
    argbuf[0] = UPPER(argument[0]);
    argument = argbuf;

    fOld = load_char_obj( d, argument, FALSE );
    if ( !check_parse_name( argument, (bool)(!fOld) ) )
    {
        write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
        /* load_char_obj can load "default" char, so we should free
           it if so */
        if (d->character)
        {
            free_char(d->character);
            d->character=NULL;
        }
        return FALSE;
    }
    
   
    if (IS_SET(d->character->act, PLR_DENY))
    {
        sprintf( buf, "Denying access to %s@%s.", argument, d->host );
        log_string( buf );
        write_to_buffer( d, "You are denied access.\n\r", 0 );
        close_socket( d );
        return FALSE;
    }
    
    if (check_ban(d->host,BAN_PERMIT) && !IS_SET(d->character->act,PLR_PERMIT))
    {
        write_to_buffer(d,"Your site has been banned from this mud.\n\r",0);
        close_socket(d);
        return FALSE;
    }
    
    if ( check_reconnect( d, argument, FALSE ) )
    {
        fOld = TRUE;
    }
    else
    {
        if ( wizlock && !IS_IMMORTAL(d->character)) 
        {
            write_to_buffer( d, "The game is wizlocked.\n\r", 0 );
            close_socket( d );
            return FALSE;
        }
    }
    
    if ( fOld )
    {
        /* Old player */
        return get_old_password(d, argument);
    }
    else
    {
        /* New player */
        if (newlock)
        {
            write_to_buffer( d, "The game is newlocked.\n\r", 0 );
            close_socket( d );
            return FALSE;
        }
        
        if (check_ban(d->host,BAN_NEWBIES))
        {
            write_to_buffer(d,
                "New players are not allowed from your site.\n\r",0);
            close_socket(d);
            return FALSE;
        }
        
        return confirm_new_name(d, argument);
    }
    
    return FALSE;
}
 



DEF_NANNY_FUN(get_old_password)
{
	CHAR_DATA *ch = d->character;
	char buf[MAX_STRING_LENGTH];

	if (con_state(d) != CON_GET_OLD_PASSWORD)
	{
		set_con_state(d, CON_GET_OLD_PASSWORD);
		sprintf( buf, "Welcome back, %s.  What is your password? ", ch->name );
		write_to_buffer( d, buf, 0 );
        ProtocolNoEcho( d, true );

		return FALSE;
	}
	else
	{
		write_to_buffer( d, "\n\r", 2 );
        if ( !check_password(argument, ch->pcdata->pwd) )
		{
			write_to_buffer( d, "Wrong Password.\n\r", 0 );
			close_socket( d );
			return FALSE;
		}
 
        ProtocolNoEcho( d, false );

		if (check_playing(d,ch->name))
			return FALSE;

		if ( check_reconnect( d, ch->name, TRUE ) )
			return FALSE;

		sprintf( buf, "%s@%s has connected.", ch->name, d->host );
		log_string( buf );
		wiznet(buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));
        DXPORT_player_connect(ch->name, d->host, current_time);

		return TRUE;
	}
	
	return FALSE;
}



DEF_NANNY_FUN(confirm_new_name)
{
	char buf[MAX_STRING_LENGTH];

	if (con_state(d)!=CON_CONFIRM_NEW_NAME)
	{
		sprintf( buf, "Did I get that right, %s (Y/N)? ", argument );
		write_to_buffer( d, buf, 0 );
		set_con_state(d, CON_CONFIRM_NEW_NAME);
		return FALSE;
	}

	switch ( *argument )
	{
	case 'y': case 'Y':
		return TRUE;

	case 'n': case 'N':
		free_char( d->character );
		d->character = NULL;
		return get_name(d, argument);

	default:
		write_to_buffer( d, "Please type Yes or No? ", 0 );
		break;
	}
	
	return FALSE;
}




DEF_NANNY_FUN(get_new_password)
{
	char *pwdnew;
	char *p;
	CHAR_DATA *ch = d->character;
	char buf[MAX_STRING_LENGTH];

	if (con_state(d) != CON_GET_NEW_PASSWORD)
	{
        ProtocolNoEcho( d, true );
		sprintf( buf, "Ah, a new soul.  Welcome to your new home, %s.\n\rPlease enter a password for your new character: ",
		d->character->name );
		write_to_buffer( d, buf, 0 );
		set_con_state(d, CON_GET_NEW_PASSWORD);
		return FALSE;
	}

	write_to_buffer( d, "\n\r", 2 );

	if ( strlen(argument) < 5 )
	{
		write_to_buffer( d,
		"Password must be at least five characters long.\n\rPassword: ",
		0 );
		return FALSE;
	}

	pwdnew = crypt( argument, ch->name );
	for ( p = pwdnew; *p != '\0'; p++ )
	{
		if ( *p == '~' )
		{
		write_to_buffer( d,
			"New password not acceptable, try again.\n\rPassword: ",
			0 );
		return FALSE;
		}
	}

	free_string( ch->pcdata->pwd );
	ch->pcdata->pwd = str_dup( pwdnew );

	return TRUE;
}




DEF_NANNY_FUN(confirm_new_password)
{
	CHAR_DATA *ch=d->character;

	if (con_state(d) != CON_CONFIRM_NEW_PASSWORD)
	{
		write_to_buffer( d, "Please retype password: ", 0 );
		set_con_state(d, CON_CONFIRM_NEW_PASSWORD);
		return FALSE;
	}

	write_to_buffer( d, "\n\r", 2 );

    if ( !check_password(argument, ch->pcdata->pwd) )
	{
		write_to_buffer( d, "Passwords don't match, please retype it.\n\r", 0 );
		return (get_new_password(d, argument));
	}

    ProtocolNoEcho( d, false );

	return TRUE;
}


DEF_NANNY_FUN(get_colour)
{
	CHAR_DATA *ch = d->character;

	if (con_state(d) != CON_GET_COLOUR)
	{
		write_to_buffer(d,"Would you like COLOUR activated (Y/N) ? ",0);
		set_con_state(d, CON_GET_COLOUR);
		return FALSE;
	}

	switch ( argument[0] )
	{
	case 'y': case 'Y': 	SET_BIT( ch->act, PLR_COLOUR );
				return TRUE;
	case 'n': case 'N':	/* do nothing */
				return TRUE;
	default:
		write_to_buffer( d, "Please answer yes or no.\n\rWould you like COLOUR activated? ", 0 );
		break;
	}
	return FALSE;
}

DEF_NANNY_FUN(get_creation_mode)
{
	char arg[MAX_STRING_LENGTH];
	char msg[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH*2];
	char *pbuff;

	if (con_state(d) != CON_GET_CREATION_MODE)
	{
		write_to_buffer(d,"\n\r",0);
		do_help(d->character,"header creation");
		sprintf( msg, "{CWhich creation option do you choose(normal, expert)?{x " );

		pbuff = buffer;
		colourconv( pbuff, msg, d->character );
		write_to_buffer(d, buffer, 0);

		set_con_state(d, CON_GET_CREATION_MODE);
		return FALSE;
	}

	one_argument(argument,arg);

	if (!strcmp(arg, "normal"))
	{
		set_creation_state(d, CREATION_NORMAL);
		return TRUE;
	}

	if (!strcmp(arg, "expert"))
	{
		set_creation_state(d, CREATION_EXPERT);
		return TRUE;
	}
	if (!strcmp(arg, "help"))
	{
		do_help(d->character,"header creation");
		return FALSE;
	}

	write_to_buffer(d,"That isn't a valid choice.\n\r",0);

        sprintf( msg, "{CWhich creation option do you choose(normal, expert)?{x " );
        pbuff = buffer;
        colourconv( pbuff, msg, d->character ); 
        write_to_buffer(d, buffer ,0);

	return FALSE;
}
 
DEF_NANNY_FUN(nanny_remort_begin)
{
    CHAR_DATA *ch = d->character;
    
    // allow change of subclass for the first few remorts so players can test all
    if ( ch->pcdata->ascents > 0 && ch->pcdata->remorts <= subclass_count(ch->class) )
        return get_new_subclass(d, argument);
    else
        return get_new_race(d, argument);
}

DEF_NANNY_FUN(get_new_race)
{
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *ch = d->character;
    int race, i;
    char msg[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH*2];
    char *pbuff; 
    
    if (con_state(d) != CON_GET_NEW_RACE)
    {
	do_help(d->character, "race help");

	write_to_buffer(d,"The following races are available:\n\r",0);
	show_races_to_d(d);

	if (ch->pcdata->remorts>0)
	    write_to_buffer(d, "Type HELP REMORTRACE for information on remort races.\n\r ",0);

	sprintf( msg, "{CWhat is your race (for more information type HELP, STATS, or ETLS)? {x" );
	pbuff = buffer;
	colourconv( pbuff, msg, d->character );
	write_to_buffer(d,buffer,0);

	set_con_state(d, CON_GET_NEW_RACE);
	return FALSE;
    }

    one_argument(argument,arg);
    

    if (!strcmp(arg,"help"))
    {
    argument = one_argument(argument,arg);
	if (argument[0] == '\0')
	    do_help(ch,"race help");
	else
	    do_help(ch,argument);
	if (ch->pcdata->remorts>0)
	    write_to_buffer(d, "Type HELP REMORTRACE for information on remort races.\n\r ",0);

	sprintf( msg, "{CWhat is your race (for more information type HELP, STATS, or ETLS)? {x" );
	pbuff = buffer;
	colourconv( pbuff, msg, d->character );
	write_to_buffer(d,buffer,0);

	return FALSE;
    }

    if (!strcmp(arg,"stats"))
    {
    	argument = one_argument(argument,arg);
    	if (argument[0] == '\0')
    	{
			char r[6];
			sprintf( r, "< r%d", ch->pcdata->remorts );
			do_stats(ch, r);
		}
		else
		{
			do_stats(ch, argument);
		}

	if (ch->pcdata->remorts>0)
	    write_to_buffer(d, "Type HELP REMORTRACE for information on remort races.\n\r ",0);

	sprintf( msg, "{CWhat is your race (for more information type HELP, STATS, or ETLS)? {x" );
	pbuff = buffer;
	colourconv( pbuff, msg, d->character );
	write_to_buffer(d,buffer,0);

	return FALSE;
    }
    
    if (!strcmp(arg,"etls"))
    {
    	argument = one_argument(argument,arg);
    	if (argument[0] == '\0')
    	{
			char r[6];
			sprintf( r, "< r%d", ch->pcdata->remorts );
			do_etls(ch, r);
		}
		else
		{
			do_etls(ch, argument);
		}

	if (ch->pcdata->remorts>0)
	    write_to_buffer(d, "Type HELP REMORTRACE for information on remort races.\n\r ",0);

	sprintf( msg, "{CWhat is your race (for more information type HELP, STATS, or ETLS)? {x" );
	pbuff = buffer;
	colourconv( pbuff, msg, d->character );
	write_to_buffer(d,buffer,0);

	return FALSE;
    }

    if (!strcmp(arg,"showrace"))
    {
    	argument = one_argument(argument,arg);
    	
        if (argument[0])
        {
            do_showrace(ch, argument);
        }
        else
        {
            send_to_char("Syntax: showrace <pc race>\n\r", ch);
        }

    	return FALSE;
    }

    race = race_lookup(argument);

    if ( race == 0 
	 || !race_table[race].pc_race
	 || pc_race_table[race].remorts > ch->pcdata->remorts
	 || strcmp(argument, pc_race_table[race].name) )
    {
	write_to_buffer(d,"That is not a valid race.\n\r",0);
	write_to_buffer(d,"The following races are available:\n\r",0);
	show_races_to_d(d);

	if (ch->pcdata->remorts>0)
	    write_to_buffer(d, "Type HELP REMORTRACE for information on remort races.\n\r ",0);

	sprintf( msg, "{CWhat is your race (for more information type HELP, STATS, or ETLS)? {x" );
	pbuff = buffer;
	colourconv( pbuff, msg, d->character );
	write_to_buffer(d,buffer,0);

	return FALSE;
    }

    if (pc_race_table[race].gender != SEX_BOTH)
    {
	ch->sex = pc_race_table[race].gender;
    }

    ch->race = race;

    /* strip affects */
    while ( ch->affected )
	affect_remove( ch, ch->affected );
    
    /* initialize stats */
    flag_copy( ch->affect_field, race_table[race].affect_field );
    flag_copy( ch->imm_flags,    race_table[race].imm );
    flag_copy( ch->res_flags,    race_table[race].res );
    flag_copy( ch->vuln_flags,   race_table[race].vuln );
    flag_copy( ch->form,         race_table[race].form );
    flag_copy( ch->parts,        race_table[race].parts );
    
    /* add skills */
    for (i = 0; i < pc_race_table[race].num_skills; i++)
    {
	group_add(ch,pc_race_table[race].skills[i],FALSE);
    }

    /* add cost */
    ch->pcdata->points =0;
    ch->size = pc_race_table[race].size;
    
    sprintf( msg, "\n\r     {cFor your first incarnation, you have chosen to be %s %s.{x\n\r\n\r",
        aan(pc_race_table[race].name), pc_race_table[race].name );
    pbuff = buffer;
    colourconv( pbuff, msg, d->character );
    write_to_buffer(d, buffer, 0);

    return TRUE;
}

void show_races_to_d( DESCRIPTOR_DATA *d )
{
	int race;
	char colour;
	char msg[MAX_STRING_LENGTH];
	char tmp[20];
	char buffer[MAX_STRING_LENGTH*2];
	char *pbuff;	/* pointer to buff */

	sprintf( msg, "[ " );
	for ( race = 1; race_table[race].name != NULL; race++ )
	{
	    if (!race_table[race].pc_race)
		break;
	    if (pc_race_table[race].remorts > d->character->pcdata->remorts)
		break;
	    switch( pc_race_table[race].remorts )
	    {
		case 0:  colour = 'W'; break;
		case 1:  colour = 'Y'; break;
		case 2:  colour = 'G'; break;
		case 3:  colour = 'C'; break;
		case 4:  colour = 'B'; break;
		case 5:  colour = 'M'; break;
		case 6:  colour = 'R'; break;
		case 7:  colour = 'Y'; break;
		case 8:  colour = 'G'; break;
		case 9:  colour = 'C'; break;
		case 10: colour = 'B'; break;
		default: colour = 'D';
	    }
	    //sprintf( msg, "%s{%c%s ", msg,colour, race_table[race].name );
            sprintf( tmp, "{%c%s ", colour, race_table[race].name );
            strcat( msg, tmp );

	}
	strcat( msg, "{x]\n\r" );

	pbuff = buffer;
	colourconv( pbuff, msg, d->character );
	write_to_buffer(d,buffer,0);
}


DEF_NANNY_FUN(get_new_sex)
{
	CHAR_DATA *ch = d->character;
	char msg[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH*2];
	char *pbuff; 

	if (con_state(d)!=CON_GET_NEW_SEX)
	{
		sprintf( msg, "\n\r{CWhat is your sex (M/F)?{x " );
		pbuff = buffer;
		colourconv( pbuff, msg, d->character );
		write_to_buffer( d, buffer, 0 );

		set_con_state(d, CON_GET_NEW_SEX);
		return FALSE;
	}

	switch ( argument[0] )
	{
	case 'm': case 'M': ch->sex = SEX_MALE;    
				ch->pcdata->true_sex = SEX_MALE;
				return TRUE;
	case 'f': case 'F': ch->sex = SEX_FEMALE; 
				ch->pcdata->true_sex = SEX_FEMALE;
				return TRUE;
	default:
		sprintf( msg, "That's not a sex.\n\r{CWhat is your sex (M/F)?{x " );
		pbuff = buffer;
		colourconv( pbuff, msg, d->character );
		write_to_buffer( d, buffer, 0 );

		break;
	}

	return FALSE;
}




DEF_NANNY_FUN(get_new_class)
{
	int i;
	char buf[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];
	char *pbuff;
	char arg[MAX_STRING_LENGTH];
	CHAR_DATA *ch = d->character;

	if (con_state(d)!=CON_GET_NEW_CLASS)
	{
		write_to_buffer( d, "\n\r", 0 );
		do_help(ch, "classes");

		strcpy( buf, "The following classes are available:\n\r[ {W" );
		for ( i = 0; i < MAX_CLASS; i++ )
		{
			if ( i > 0 )
			strcat( buf, " " );
			strcat( buf, class_table[i].name );
		}
//		strcat( buf, "{x ]\n\r{CChoose a class(for more information type HELP, STATS, or ETLS):{x" );
		strcat( buf, "{x ]\n\r{CChoose a class(for more information type HELP <CLASS NAME>, HELP STATS or HELP ETLS):{x" ); 
		pbuff = buffer;
		colourconv( pbuff, buf, d->character );
		write_to_buffer( d, buffer, 0 );

		set_con_state(d, CON_GET_NEW_CLASS);

		return FALSE;
	}

        argument = one_argument(argument,arg);
	if (!strcmp(arg,"help"))
	{
		if (argument[0] == '\0')
		{
			do_help(ch,"classes");
		}
		else
		{
			// Special case help assassin to go to help assassin class instead
			// of showing help assassination.
			if (!str_prefix(argument, "assassin"))
			{
				do_help(ch, "assassin class");
			}
			else
			{
				do_help(ch,argument);
			}	
		}

//		sprintf( buf, "{CWhat is your class (for more information type HELP, STATS, or ETLS)?{x" );
		sprintf( buf, "\n\r{CWhat is your class (for more information type HELP <CLASS NAME>, HELP STATS or HELP ETLS)?{x" ); 
		pbuff = buffer;
		colourconv( pbuff, buf, d->character );
		write_to_buffer( d, buffer, 0 );

		return FALSE;
	}

	if (!strcmp(arg,"stats"))
	{
		if (argument[0] == '\0')
		{
			char r[6];
			sprintf( r, "< r%d", ch->pcdata->remorts );
			do_stats(ch,r);
		}
		else
		{
			do_stats(ch,argument);
		}

//		sprintf( buf, "{CWhat is your class (for more information type HELP, STATS, or ETLS)?{x" );
                sprintf( buf, "\n\r{CWhat is your class (for more information type HELP <CLASS NAME>, HELP STATS or HELP ETLS)?{x" ); 
		pbuff = buffer;
		colourconv( pbuff, buf, d->character );
		write_to_buffer( d, buffer, 0 );

		return FALSE;
	}

	if (!strcmp(arg,"etls"))
	{
		if (argument[0] == '\0')
		{
			char r[6];
			sprintf( r, "< r%d", ch->pcdata->remorts );
			do_etls(ch,r);
		}
		else
		{
			do_etls(ch,argument);
		}

//		sprintf( buf, "{CWhat is your class (for more information type HELP, STATS, or ETLS)?{x" );
                sprintf( buf, "\n\r{CWhat is your class (for more information type HELP <CLASS NAME>, HELP STATS or HELP ETLS)?{x" ); 
		pbuff = buffer;
		colourconv( pbuff, buf, d->character );
		write_to_buffer( d, buffer, 0 );

		return FALSE;
	}

	if (!strcmp(arg,"showrace"))
	{
        if (argument[0])
        {
            do_showrace(ch, argument);
        }
        else
        {
            send_to_char("Syntax: showrace <pc race>\n\r", ch);
        }

		return FALSE;
	}

	i = class_lookup(arg);

	if ( i == -1 )
	{
		write_to_buffer( d, "That's not a class.\n\r", 0 );

		strcpy( buf, "The following classes are available: [ {W" );
		for ( i = 0; i < MAX_CLASS; i++ )
		{
			if ( i > 0 )
			strcat( buf, " " );
			strcat( buf, class_table[i].name );
		}

//		strcat( buf, "{CWhat is your class(for more information type HELP, STATS, or ETLS)?{x" );
                strcat( buf, "\n\r{CWhat is your class (for more information type HELP <CLASS NAME>, HELP STATS or HELP ETLS)?{x" ); 
		pbuff = buffer;
		colourconv( pbuff, buf, d->character );
		write_to_buffer( d, buffer, 0 );

		return FALSE;
	}

	ch->class = i;

    sprintf( buf, "\n\r     {cYou have chosen to be %s %s.{x\n\r\n\r", aan(class_table[i].name), class_table[i].name );
    pbuff = buffer;
    colourconv( pbuff, buf, d->character );
    write_to_buffer(d, buffer, 0);

	return TRUE;
}


static int nanny_show_subclasses( CHAR_DATA *ch )
{
    int sc;
    int count = subclass_count(ch->class);
    
    ptc(ch, "The following subclasses are available:\n\r[{W");
    for ( sc = 1; subclass_table[sc].name != NULL; sc++ )
    {
        if ( ch_can_take_subclass(ch, sc) )
            ptc(ch, " %s", subclass_table[sc].name);
    }
    ptc(ch, "{x ]\n\r");
    if ( count == ch->pcdata->remorts )
        ptc(ch, "{RWARNING: This time your choice of subclass will be final!{x\n\r");
    ptc(ch, "{CChoose a subclass (for more information type HELP <SUBCLASS NAME>):{x");
    return count;
}

DEF_NANNY_FUN(get_new_subclass)
{
    int sc;
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *ch = d->character;

    if ( con_state(d) != CON_GET_NEW_SUBCLASS )
    {
        ptc(ch, "\n\r");
        do_help(ch, "subclasses");
        nanny_show_subclasses(ch);

        set_con_state(d, CON_GET_NEW_SUBCLASS);
        return FALSE;
    }

    argument = one_argument(argument, arg);
    
    if ( !strcmp(arg, "help") )
    {
        if ( argument[0] == '\0' )
            do_help(ch, "subclasses");
        else
            do_showsubclass(ch, argument);
        ptc(ch, "{CChoose a subclass (for more information type HELP <SUBCLASS NAME>):{x");
        return FALSE;
    }
    
    sc = subclass_lookup(arg);

    if ( sc == 0 )
    {
        if ( arg[0] )
            ptc(ch, "That's not a subclass.\n\r");
        int count = nanny_show_subclasses(ch);
        // safety-net
        if ( count == 0 )
        {
            ptc(ch, "\n\r{RNo subclasses available. Moving on.{x\n\r");
            return TRUE;
        }
        return FALSE;
    }
    if ( !ch_can_take_subclass(ch, sc) )
    {
        ptc(ch, "You do not qualify for that subclass.\n\r");
        nanny_show_subclasses(ch);
        return FALSE;
    }

    ch->pcdata->subclass = sc;
    ptc(ch, "\n\r     {cYou have chosen to be %s %s.{x\n\r\n\r", aan(subclass_table[sc].name), subclass_table[sc].name);
    
    return TRUE;
}


DEF_NANNY_FUN(get_alignment)
{
	CHAR_DATA *ch=d->character;
    char msg[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH*2]="";
	char *pbuff; 
	char arg[MAX_STRING_LENGTH];

	if (con_state(d)!= CON_GET_ALIGNMENT)
	{
		write_to_buffer( d, "You may be angelic, saintly, good, kind, neutral, mean, evil, demonic, or satanic.\n\r",0);
		sprintf( msg, "{CWhich alignment (angelic/saintly/good/kind/neutral/mean/evil/demonic/satanic)?{x " );
		pbuff = buffer;
		colourconv( pbuff, msg, d->character );
		write_to_buffer( d, buffer, 0 );

		set_con_state(d, CON_GET_ALIGNMENT);
		return FALSE;
	}

    one_argument(argument,arg);
   
    if (!strcmp(arg,"help"))
    {
	argument = one_argument(argument,arg);
	if (argument[0] == '\0')
	    do_help(ch,"alignment");
	else
	    do_help(ch,argument);

	pbuff = buffer;
    sprintf( msg, "{CWhich alignment (angelic/saintly/good/kind/neutral/mean/evil/demonic/satanic)?{x " );
	colourconv( pbuff, msg, d->character );
	write_to_buffer(d,buffer,0);
	return FALSE;
    } 
    else if (!strcmp(arg,"angelic"))
    {
        ch->alignment = 1000;
	sprintf( msg, "\n\r     {cYou have chosen to start with angelic alignment.{x\n\r" );
    }
    else if (!strcmp(arg,"saintly"))
    {
	ch->alignment = 750;
	sprintf( msg, "\n\r     {cYou have chosen to start with saintly alignment.{x\n\r" );
    }
    else if (!strcmp(arg,"good"))
    {
	ch->alignment = 500;
	sprintf( msg, "\n\r     {cYou have chosen to start with good alignment.{x\n\r" );
    }
    else if (!strcmp(arg,"kind"))
    {
	ch->alignment = 250;
	sprintf( msg, "\n\r     {cYou have chosen to start with kind alignment.{x\n\r" );
    }
    else if (!strcmp(arg,"neutral"))
    {
	ch->alignment = 0;
	sprintf( msg, "\n\r     {cYou have chosen to start with neutral alignment.{x\n\r" );
    }
    else if (!strcmp(arg,"mean"))
    {
	ch->alignment = -250;
	sprintf( msg, "\n\r     {cYou have chosen to start with mean alignment.{x\n\r" );
    }
    else if (!strcmp(arg,"evil"))
    {
	ch->alignment = -500;
	sprintf( msg, "\n\r     {cYou have chosen to start with evil alignment.{x\n\r" );
    }
    else if (!strcmp(arg,"demonic"))
    {
	ch->alignment = -750;
	sprintf( msg, "\n\r     {cYou have chosen to start with demonic alignment.{x\n\r" );
    }
    else if (!strcmp(arg,"satanic"))
    {
	ch->alignment = -1000;
	sprintf( msg, "\n\r     {cYou have chosen to start with satanic alignment.{x\n\r" );
    }
    else
    {
	write_to_buffer(d,"That's not a valid alignment.\n\r",0);
	write_to_buffer( d, "You may be angelic, saintly, good, kind, neutral, mean, evil, demonic, or satanic.\n\r",0);
	sprintf( msg, "{CWhich alignment (angelic/saintly/good/kind/neutral/mean/evil/demonic/satanic)?{x " );
	pbuff = buffer;
	colourconv( pbuff, msg, d->character );
	write_to_buffer( d, buffer, 0 );
	return FALSE;
    }


	pbuff = buffer;
	colourconv( pbuff, msg, d->character );
	write_to_buffer( d, buffer, 0 );

	write_to_buffer(d,"\n\r",0);

	return TRUE;
}


void take_rom_basics(DESCRIPTOR_DATA *d)
{
	CHAR_DATA *ch=d->character;

	group_add(ch,class_table[ch->class].base_group,FALSE);
	
	return;
}	


void take_class_defaults(DESCRIPTOR_DATA *d)
{
	group_add(d->character,class_table[d->character->class].default_group,TRUE);
	return;
}


void take_default_weapon(DESCRIPTOR_DATA *d)
{
	int i;
	char msg[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH*2];
	char *pbuff; 

	for(i=0; weapon_table[i].name!=NULL; i++)
		if (weapon_table[i].vnum == class_table[d->character->class].weapon)
			break;

	if (d->character->class == class_lookup("monk"))
	{
		sprintf( msg, "     {cYou have chosen to begin fighting barehanded.{x\n\r\n\r" );
		pbuff = buffer;
		colourconv( pbuff, msg, d->character );
		write_to_buffer(d,buffer,0);
		d->character->pcdata->learned[gsn_hand_to_hand] = 40;
	}
	else
	{
		sprintf( msg, "     {cYou have chosen to begin with a %s in hand.{x\n\r\n\r", weapon_table[i].name );
		pbuff = buffer;
		colourconv( pbuff, msg, d->character );
		write_to_buffer(d,buffer,0);
		d->character->pcdata->learned[*weapon_table[i].gsn] = 40;
	}
}


void newbie_alert(DESCRIPTOR_DATA *d)
{
    char buf[MAX_STRING_LENGTH];
    PENALTY_DATA *p;

    /* If there are any residual penalties from a previous use of this player
       name, remove them */
    for (p = penalty_list; p ; p = p->next)
        if (!str_cmp(d->character->name, p->victim_name))
        {
            delete_penalty_node(p);
            save_penalties();
        }

    sprintf( buf, "%s@%s new player.", d->character->name, d->host );
    log_string( buf );
    wiznet("Newbie alert!  $N sighted.",d->character,NULL,WIZ_NEWBIE,0,0);
    wiznet(buf,NULL,NULL,WIZ_SITES,0,0);
    return;
}


DEF_NANNY_FUN(default_choice)
{
	char msg[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH*2];
	char *pbuff; 

	if (con_state(d) != CON_DEFAULT_CHOICE)
	{
		sprintf(msg, "Do you wish to customize this character?\n\r"
			"Customization takes time, but allows a wider range of skills and abilities.\n\r"
			"{CCustomize (Y/N)?{x " );
		pbuff = buffer;
		colourconv( pbuff, msg, d->character ); 
		write_to_buffer(d, buffer ,0);

		set_con_state(d, CON_DEFAULT_CHOICE);
		return FALSE;		
	}

	write_to_buffer(d,"\n\r",2);
	switch ( argument[0] )
	{
	case 'y': case 'Y': 
		return gen_groups(d, argument);
	case 'n': case 'N':
		take_class_defaults(d); 
		return TRUE;
	default:
		sprintf(msg, "{CPlease answer yes/no (Y/N)?{x ");
		pbuff = buffer;
		colourconv( pbuff, msg, d->character ); 
		write_to_buffer(d, buffer ,0);
		break;
	}
	
	return FALSE;
}




DEF_NANNY_FUN(gen_groups)
{
	CHAR_DATA *ch=d->character;
	char buf[MAX_STRING_LENGTH];

	if (con_state(d) != CON_GEN_GROUPS)
	{
		ch->gen_data = new_gen_data();
		ch->gen_data->points_chosen = ch->pcdata->points;
		do_help(ch,"header group list");
		list_group_costs(ch);
        //write_to_buffer(d,"You already have the following skills:\n\r",0);
        //do_skills(ch,"");
        //do_spells(ch,"");
		send_to_char("{CList, learned, premise, add, drop, info, help, or done?{x ",ch);
		set_con_state(d, CON_GEN_GROUPS);
		return FALSE;		
	}

	send_to_char("\n\r",ch);
	if (!str_cmp(argument,"done"))
	{
		sprintf(buf,"     {cYou have used %d creation points,{x\n\r",ch->pcdata->points);
		send_to_char(buf,ch);
		free_gen_data(ch->gen_data);
		ch->gen_data = NULL;
		return TRUE;
	}

	if (!parse_gen_groups(ch,argument))
		send_to_char("Thats not a valid choice.\n\r",ch);

	send_to_char("{CList, learned, premise, add, drop, info, help, or done?{x ",ch);
	
	return FALSE;
}




DEF_NANNY_FUN(pick_weapon)
{
	int w, weapon;
	CHAR_DATA *ch=d->character;
	char msg[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH*2];
        char *pbuff;    /* pointer to buff */

	if (con_state(d) != CON_PICK_WEAPON)
	{
		write_to_buffer( d, "\n\r", 2 );
		write_to_buffer(d,
		"Please pick a weapon from the following choices:\n\r",0);

	        sprintf( msg, "[{W" );

		for ( w = 0; weapon_table[w].name != NULL; w++)
		{
		if ( ch->pcdata->learned[*weapon_table[w].gsn] > 0 
			&& skill_table[*weapon_table[w].gsn].skill_level[ch->class] == 1 )
			{
			    strcat(msg, " ");
			    strcat(msg,weapon_table[w].name);
			}
	        }
		if ( ch->pcdata->learned[gsn_hand_to_hand] > 0
			&& skill_table[gsn_hand_to_hand].skill_level[ch->class] == 1 )
		    strcat(msg," unarmed");
	        strcat( msg, "{x ]\n\r" );

        	pbuff = buffer;
	        colourconv( pbuff, msg, d->character );
        	write_to_buffer(d,buffer,0);

		sprintf(msg,"{CYour choice of weapon (Press enter to take your class default)?{x ");
        	pbuff = buffer;
	        colourconv( pbuff, msg, d->character );
		write_to_buffer(d,buffer,0);
		set_con_state(d, CON_PICK_WEAPON);
		return FALSE;
	}

	write_to_buffer(d,"\n\r",2);

	if (argument[0]=='\0')
	{
		take_default_weapon(d);
		return TRUE;
	}		

	weapon = weapon_lookup(argument);
	if (weapon == -1 || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0)
	{
		if (strcmp(argument, "unarmed")||
			ch->pcdata->learned[gsn_hand_to_hand]<=0)
		{
			write_to_buffer(d, "That's not a valid selection. Choices are:\n\r",0);

		        sprintf( msg, "[ {W" );
			for ( w = 0; weapon_table[w].name != NULL; w++)
			{
				if (ch->pcdata->learned[*weapon_table[w].gsn] > 0 
				&& skill_table[*weapon_table[w].gsn].skill_level[ch->class] == 1)
				{
				    strcat(msg,weapon_table[w].name);
				    strcat(msg, " ");
				}
				if (ch->pcdata->learned[gsn_hand_to_hand]>0)
				    strcat(msg,"unarmed");
		    }
		        strcat( msg, "{x ]\n\r" );

        		pbuff = buffer;
		        colourconv( pbuff, msg, d->character );

			strcat(msg,"{CYour choice of weapon (Press enter to take your class default)?{x ");
        		pbuff = buffer;
		        colourconv( pbuff, msg, d->character );
			write_to_buffer(d,buffer,0);
			return FALSE;
		}
	}

	if (weapon==-1)
	{
		sprintf( msg, "     {cYou have chosen to begin fighting barehanded.{x\n\r" );
		pbuff = buffer;
		colourconv( pbuff, msg, d->character );
		write_to_buffer(d,buffer,0);
		ch->pcdata->learned[gsn_hand_to_hand] = 40;
	}
	else
	{
		sprintf( msg, "     {cYou have chosen to begin with a %s in hand.{x\n\r", weapon_table[weapon].name );
		pbuff = buffer;
		colourconv( pbuff, msg, d->character );
		write_to_buffer(d,buffer,0);
		ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
	}

	write_to_buffer(d,"\n\r",2);
	return TRUE;
}

DEF_NANNY_FUN(roll_stats)
{
	CHAR_DATA *ch=d->character;

	if (con_state(d) != CON_ROLL_STATS)
	{
		ch->gen_data = new_gen_data();
		do_help(ch,"header rollstat");
		parse_roll_stats(ch, "reroll default");
		send_to_char("{CShow, reroll, assign, unassign, default, help, or done?{x ",ch);
		set_con_state(d, CON_ROLL_STATS);
		return FALSE;
	}

	send_to_char("\n\r",ch);

	if (!str_cmp(argument,"done"))
	{
		if (ch->gen_data->unused_die[0]!=-1)
		{
			send_to_char("You haven't finished setting your priorities.  Finish assigning them or type default.\n\r",ch);
			return FALSE;
		}
		free_gen_data(ch->gen_data);
		ch->gen_data = NULL;
		return TRUE;
	}

    if (!str_cmp(argument,"default"))
    {
        send_to_char("     {cThe game has assigned dice for you based on your class and race.{x\n\r",ch);
        auto_assign_stats(ch);
        calc_stats(ch);
        show_dice(ch);
        send_to_char("{CShow, reroll, assign, unassign, default, help, or done?{x ",ch);
        return FALSE;
    }

	if (!parse_roll_stats(ch,argument))
		send_to_char("Thats not a valid choice.\n\r",ch);

	send_to_char("{CShow, reroll, assign, unassign, default, help, or done?{x ",ch);
	
	return FALSE;
}



DEF_NANNY_FUN(read_imotd)
{
    if (!is_granted_name(d->character,"imotd"))
		return read_motd(d, argument);
	
	if (con_state(d) != CON_READ_IMOTD)
	{
		set_con_state(d, CON_READ_IMOTD);
		do_help( d->character, "imotd" );
		return FALSE;
	}

	return TRUE;
}




DEF_NANNY_FUN(read_motd)
{
	if (con_state(d) != CON_READ_MOTD)
	{
		set_con_state(d, CON_READ_MOTD);
		do_help( d->character, "motd" );
		return FALSE;
	}

	return TRUE;
}


DEF_NANNY_FUN(break_connect)
{
	DESCRIPTOR_DATA *d_old, *d_next;

	if (con_state(d)!=CON_BREAK_CONNECT)
	{
		write_to_buffer( d, "That character is already playing.\n\r",0);
		write_to_buffer( d, "Do you wish to connect anyway (Y/N)?",0);
		set_con_state(d, CON_BREAK_CONNECT);
		return FALSE;
	}
	else
	switch( *argument )
	{
	case 'y' : case 'Y':
		for ( d_old = descriptor_list; d_old != NULL; d_old = d_next )
		{
		d_next = d_old->next;
		if (d_old == d || d_old->character == NULL)
			continue;

		if (str_cmp(d->character->name,d_old->original ?
			d_old->original->name : d_old->character->name))
			continue;

		close_socket(d_old);
		}
		if (check_reconnect(d,d->character->name,TRUE))
			return FALSE;
		write_to_buffer(d,"Reconnect attempt failed.\n\r",0);
			if ( d->character != NULL )
			{
				free_char( d->character );
				d->character = NULL;
			}
		return TRUE;

	case 'n' : case 'N':
			if ( d->character != NULL )
			{
				free_char( d->character );
				d->character = NULL;
			}
		return TRUE;

	default:
		write_to_buffer(d,"Please type Y or N? ",0);
		break;
	}

	return FALSE;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, const char *name, bool fConn )
{
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH];
    
    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if ( ch->must_extract )
            continue;
        
        if ( !IS_NPC(ch)
            &&   (!fConn || ch->desc == NULL)
            &&   !str_cmp( d->character->name, ch->name ) )
        {
            if ( fConn == FALSE )
            {
                free_string( d->character->pcdata->pwd );
                d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
            }
            else
            {
                /* if there was a pet in the pfile then it
                   was loaded up and we need to kill it */
                if (d->character->pet)
                {
                    /* extract_char  because it's on the char_list */
                    extract_char( d->character->pet, TRUE );
                    
                }

                /* free_char because it's not on the char_list */
                free_char( d->character );
                d->character = ch;
                ch->desc     = d;
                ch->timer    = 0;

                send_to_char( "Reconnecting.\n\r", ch );
                if (ch->pcdata->new_tells)
                    send_to_char( "Type 'playback tell' to see missed tells.\n\r", ch );
                act( "$n has reconnected.", ch, NULL, NULL, TO_ROOM );

                sprintf( buf, "%s@%s reconnected.", ch->name, d->host );
                log_string( buf );
                wiznet("$N groks the fullness of $S link.",
                    ch,NULL,WIZ_LINKS,0,0);
                d->connected = CON_PLAYING;
                MXPSendTag( d, "<VERSION>" );
                gui_login_setup( d->character );
       /* Removed by Vodur, messes with box saving
          no downside to keeping it on the list*/
	//	remove_from_quit_list( ch->name );

                /* Inform the character of a note in progress and the possibility of continuation */ 
                if (ch->pcdata->in_progress)
                    send_to_char ("You have a note in progress. Type NOTE WRITE to continue it.\n\r",ch);
                
                check_auth_state( ch );
            }
            return TRUE;
        }
    }
    return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing( DESCRIPTOR_DATA *d, const char *name )
{
	DESCRIPTOR_DATA *dold;

	for ( dold = descriptor_list; dold; dold = dold->next )
	{
	if ( dold != d
	&&   dold->character != NULL
	&&   dold->connected != CON_GET_NAME
	&&   dold->connected != CON_GET_OLD_PASSWORD
	&&   !str_cmp( name, dold->original
			 ? dold->original->name : dold->character->name ) )
	{
		break_connect(d, "");
		return TRUE;
	}
	}

	return FALSE;
}

struct quote_type
{
    const char * 	text;
};

static const struct quote_type quote_table[MAX_QUOTES] =
{
    { "{WYou will move and regenerate slower while encumbered (carrying too much weight).{x\n\r"            }, /* 1 */
    { "{WMaledictions such as poison and plague will slow down your HP, mana, and move regeneration.{x\n\r" },
    { "{WBeing affected by 'fly' reduces movement points used, and moves you faster between rooms.{x\n\r"   },
    { "{WThe lemonade fountain 2N from Palace Square (recall) will satisfy thirst and hunger.{x\n\r"        },
    { "{WDon't forget to eat and drink. Being hungry or thirsty reduces your regeneration rates.{x\n\r"     }, /* 5 */
    { "{WYou can heal faster by sleeping or resting at the Palace Square or The Temple.{x\n\r"              },
    { "{WBe sure to practice your new skills and spells after leveling up.{x\n\r"                           },
    { "{WYou can earn extra gold by selling items that you don't need or have outgrown.{x\n\r"              },
    { "{WDeposit your gold at the bank NW,W from Palace Square. It will stay safe in the bank.{x\n\r"       },
    { "{WLenny the banker can exchange your silver for gold. (give <amount> silver lenny).{x\n\r"           }, /* 10 */
    { "{WMost storage containers will reduce the effective weight of the items in your inventory.{x\n\r"    },
    { "{WType AUTOLIST to see what commands you can automate (such as looting corpses with autoloot.{x\n\r" },
    { "{WYou can automate a single combat command using ACTION. Read 'help action' for more info.{x\n\r"    },
    { "{WThe CONSIDER command will give you a -rough- idea of how hard a fight may be.{x\n\r"               }, /* 15 */
    { "{WThere are maps available on our website of many areas. http://www.aarchonmud.com{x "               }, 
    { "{WThe Wandering Sages that roam Bastion will lore any item for you. Just hand it to them!{x\n\r"     },
    { "{WIf you lose your newbie guide, you can buy another one at Herman the Gnome (SE,E){x\n\r"           },
    { "{WTake time to explore. There are many mini-quests throughout Aarchon.{x\n\r"                        }, /* 20 */
    { "{WField experience is EXP that you absorb over time. Read 'help field' for more info.{x\n\r"         },
    { "{WDid you know, that if you type '!' it will repeat the last command you typed?{x\n\r"               },
    { "{WTyping '&' will clear your command queue. Useful for when you spam too many commands!{x\n\r"       },
    { "{WVisit the mystic NW of recall for a free god blessing. Read 'help god bless' for more info.{x\n\r" },
    { "{WThe 'finger' command allows you to look up detailed information about players.{x\n\r"              },
    { "{WWant to try out another class? You don't need to delete. Just make a second character!{x\n\r"      },
    { "{WThe donation pit 'down' from recall often contains items you can use or sell for extra gold.{x\n\r"},
    { "{WMercy the Healer at The Temple can spell you up for a fee. Type 'spellup' in her room for a list.{x\n\r"},
    { "{WType 'identify <item name>' at any shop to have the shopkeeper lore the item for a nominal fee.{x\n\r"},
    { "{WUse the 'gag' command to disable extraneous combat messages.{x\n\r"                                },
    { "{WYou can disable these automated tips by typing 'nohelp', or see more by typing 'tips'.{x\n\r"      },
    { "{WFind lights, containers, food, and drink at Herman's shop, SE, E from recall.{x\n\r"               },
    { "{WThe 'dirs' command or 'help dirs' will give you a list of directions to all areas in the game.{x\n\r"}
};



void do_loginquote( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    int number;

    number = number_range( 1, MAX_QUOTES);

    sprintf ( buf, "\n\r%s\n\r", quote_table[number].text);
    send_to_char ( buf, ch );
    return;
}

void enter_game ( DESCRIPTOR_DATA *d )
{
	CHAR_DATA *ch = d->character;
	char buf[MAX_STRING_LENGTH];

	if ( ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0')
	{
		write_to_buffer( d, "Warning! Null password!\n\r", 0 );
		write_to_buffer( d, "Please report old password with bug.\n\r",0);
		write_to_buffer( d,
			"Type 'password null <new password>' to fix.\n\r",0);
	}

	write_to_buffer( d, "\n\rWelcome to Aarchon!  May your time here be pleasantly wasted.\n\r",	0 );
    char_list_insert(ch);
	d->connected    = CON_PLAYING;
      /* Removed by Vodur, messes with box saving
          no downside to keeping it on the list*/
	//remove_from_quit_list( ch->name );
	
	free_string(ch->pcdata->last_host);
	ch->pcdata->last_host = str_dup(d->host);

	reset_char(ch);

	/* assassins are ALWAYS pkillers */
	if (ch->class == class_lookup("assassin"))
	    SET_BIT(ch->act, PLR_PERM_PKILL);
	/* Set expire date for PK chars with none set yet */
	if ( IS_SET( ch->act, PLR_PERM_PKILL) 
	   &&  ch->pcdata->pkill_expire == 0 )
		reset_pkill_expire(ch);

	/* fix in case someone was in warfare when crash happened */
	if ( !IS_IMMORTAL(ch) && ch->in_room != NULL
	     && ch->in_room->area->min_vnum == WAR_ROOM_FIRST )
	{
	    ch->in_room = NULL;
	}

	if ( ch->level == 0 )
	{
	    ch->level   = 1;
	    ch->exp     = exp_per_level(ch);
	    ch->pcdata->highest_level = 1;

	    update_perm_hp_mana_move(ch);

	    ch->hit     = ch->max_hit;
	    ch->mana    = ch->max_mana;
	    ch->move    = ch->max_move;
	    ch->silver  = 50;

        ch->train = MAX_CP - ch->pcdata->points;
	    ch->practice = 5;

	    SET_BIT(ch->act, PLR_AUTOEXIT);
	    SET_BIT(ch->act, PLR_AUTOLOOT);
	    SET_BIT(ch->act, PLR_AUTOGOLD);
	    SET_BIT(ch->act, PLR_AUTOSPLIT);
        SET_BIT(ch->act, PLR_AUTOASSIST);
        ch->wimpy = 20;
        ch->calm = 20;

	    sprintf( buf, "the %s",
		     title_table [ch->class] [(ch->level+4-(ch->level+4)%5)/5]);
	    set_title( ch, buf );

	    do_outfit(ch,"");
	    if (wait_for_auth == AUTH_STATUS_ENABLED)
	    {
		char_to_room( ch, get_room_index( ROOM_VNUM_AUTH_START ) );
		if ( !check_auto_auth(ch->name) )
		{
		    ch->pcdata->auth_state = 0;
		    SET_BIT(ch->act, PLR_UNAUTHED);
		    add_to_auth( ch );
		}
		else if ( !ch->pcdata->authed_by )
		    ch->pcdata->authed_by = str_dup( "auto" );
	    }
	    else if (wait_for_auth == AUTH_STATUS_IMM_ON)
	    {
		DESCRIPTOR_DATA *desc;
		bool found = FALSE;

		for ( desc = descriptor_list; desc; desc = desc->next )
		    if ( (IS_PLAYING(desc->connected) )
			 && desc->character
			 && IS_IMMORTAL(desc->character))
			found = TRUE;

		if (found)
		{
		    char_to_room( ch, get_room_index( ROOM_VNUM_AUTH_START ) );
		    if ( !check_auto_auth(ch->name) )
		    {
			ch->pcdata->auth_state = 0;
			SET_BIT(ch->act, PLR_UNAUTHED);
			add_to_auth( ch );
		    }
		    else if ( !ch->pcdata->authed_by )
			ch->pcdata->authed_by = str_dup( "auto" );
		}
		else
		    char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
	    }
	    else
		char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );

	    send_to_char("\n\r",ch);
	    do_help(ch,"NEWBIE INFO");
	    send_to_char("\n\r",ch);
	}
	else if ( ch->in_room != NULL )
	{
        if ( IS_SET(ch->in_room->room_flags, ROOM_BOX_ROOM)
            || room_is_private(ch->in_room)
            || area_full(ch->in_room->area) )
            char_to_room(ch, get_room_index(ROOM_VNUM_RECALL));
	    else
	        char_to_room( ch, ch->in_room );
	}
	else if ( IS_IMMORTAL(ch) )
	{
	    char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
	}
	else
	{
	    char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
	}
	
	update_bounty(ch);

	wiznet("$N has left real life behind.",ch,NULL,
	       WIZ_LOGINS,WIZ_SITES,get_trust(ch));

	if ( !IS_IMMORTAL(ch) )
	{
	    sprintf(buf, "%s has decided to join us.", ch->name);
	    info_message_new(ch, buf, FALSE, FALSE);
	}
    MXPSendTag( d, "<VERSION>" );
    gui_login_setup( d->character );
	/* Prevents pkillers from immediately attacking upon login.
	   Will annoy people who use their alts to watch for targets! Q, Nov 2002 */
	if ( ch->pcdata != NULL )
	    ch->pcdata->pkill_timer = -10 * PULSE_VIOLENCE;

	/* check for multiplay */
	/*
	if ( !IS_IMMORTAL(ch) )
	{
	    DESCRIPTOR_DATA *desc;
	    for ( desc = descriptor_list; desc != NULL; desc = desc->next)
		if ( desc->character != NULL
		     && desc->character != ch
		     && !IS_IMMORTAL(desc->character)
		     && is_same_player(desc->character, ch)
		     && (desc->connected == CON_PLAYING
			 || IS_WRITING_NOTE(desc->connected)) )
		{
		    sprintf( buf, "Multiplay: %s and %s have same host",
			     ch->name, desc->character->name );
		    wiznet(buf, ch, NULL, WIZ_CHEAT, 0, LEVEL_IMMORTAL);
		}
	}
	*/

	act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
	do_look( ch, "auto" );
        //do_loginquote(ch);
	if (ch->pet != NULL)
	{
		char_to_room(ch->pet,ch->in_room);
		act("$n appears in the room.",ch->pet,NULL,NULL,TO_ROOM);
	}
      do_board(ch,"");
      penalty_update(ch);

    check_auth_state( ch );
    /* char might be morphed => update (includes flag & hp_mana_move update) */
    morph_update( ch );
    check_spouse( ch );

    // reimburse players for lost skills
    skill_reimburse(ch);
    // update 'random' objects character is carrying
    OBJ_DATA *obj;
    for ( obj = ch->carrying; obj; obj = obj->next_content )
        check_reenchant_obj(obj);
    
    if ( is_clan(ch) )
        do_cmotd(ch, "");

    if ( IS_IMMORTAL(ch))
    {
        // Refresh imm commands.
        login_grant(ch);
    }

    /* and finally, run connect triggers if any */
    ap_connect_trigger(ch);
    rp_connect_trigger(ch);
    return;
}
