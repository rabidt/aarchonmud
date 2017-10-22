/***************************************************************************
 *  File: string.c                                                         *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/


#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "lua_main.h"

const char * string_linedel( const char *, int );
const char * string_lineadd( const char *, const char *, int );
const char * numlineas( const char * );
const char * get_line( const char *str, char *buf );


/*****************************************************************************
 Name:      del_last_line
 Purpose:   Removes last line from string
 Called by: many
 ****************************************************************************/
const char * del_last_line( const char *string )
{
  char xbuf[4*MAX_STRING_LENGTH]; 
  return del_last_line_ext(string, xbuf);
}

const char * del_last_line_ext( const char *string, char *xbuf )
{
   int len;
   bool found = FALSE;
   
   xbuf[0] = '\0';
   if (string == NULL || string[0] == '\0')
      return(str_dup(xbuf));
   
   strcpy(xbuf, string);
   
   for (len = strlen(xbuf); len > 0; len--)
   {
      if (xbuf[len] == '\r')
      {
         if (!found) /* back it up */
         {
            if ( len > 0)
               len--;
            found = TRUE;
         }
         else /* found second one */
         {
            xbuf[len +1] = '\0';
            free_string(string);
            return( str_dup(xbuf));
         }
      }
   }
   xbuf[0] = '\0';
   free_string(string);
   return( str_dup(xbuf));
}


/*****************************************************************************
 Name:		string_edit
 Purpose:	Clears string and puts player into editing mode.
 Called by:	none
 ****************************************************************************/
void string_edit( CHAR_DATA *ch, const char **pString )
{
    send_to_char( "-========- Entering EDIT Mode -=========-\n\r", ch );
    send_to_char( "    Type .h on a new line for help\n\r", ch );
    send_to_char( " Terminate with a ~ or @ on a blank line.\n\r", ch );
    send_to_char( "-=======================================-\n\r", ch );

    if ( *pString != NULL )
        free_string(*pString);
    
    *pString = str_empty;

    ch->desc->pString = pString;

    return;
}



/*****************************************************************************
 Name:		string_append
 Purpose:	Puts player into append mode for given string.
 Called by:	(many)olc_act.c
 ****************************************************************************/
void string_append( CHAR_DATA *ch, const char **pString )
{
    send_to_char( "-=======- Entering APPEND Mode -========-\n\r", ch );
    send_to_char( "    Type .h on a new line for help\n\r", ch );
    send_to_char( " Terminate with a ~ or @ on a blank line.\n\r", ch );
    send_to_char( "-=======================================-\n\r", ch );
    
    if ( *pString == NULL )
    {
        *pString = str_dup( "" );
    }
    /* wackyhacky for syntax highlighting for lua scripts */
    PROG_CODE *mpc;
    switch( ch->desc->editor)
    {
        case ED_MPCODE:
             EDIT_MPCODE(ch, mpc);
             if (mpc->is_lua)
                dump_prog( ch, *pString, TRUE);
             else
                send_to_char_new( numlineas(*pString), ch, TRUE );/* RAW */
             break;
        case ED_APCODE:
        case ED_OPCODE:
        case ED_RPCODE:
            dump_prog( ch, *pString, TRUE);
            break;
        default:
            send_to_char_new( numlineas(*pString), ch, TRUE );/* RAW */
    } 
    
    if ( *(*pString + strlen( *pString ) - 1) != '\r' )
        send_to_char( "\n\r", ch );

    ch->desc->pString = pString;
    
    return;
}



/*****************************************************************************
 Name:		string_replace
 Purpose:	Substitutes one string for another.
 Called by:	string_add(string.c) (aedit_builder)olc_act.c.
 ****************************************************************************/
const char * string_replace( const char *orig, const char *old, const char *new )
{
  char xbuf[MAX_STRING_LENGTH];
  return string_replace_ext(orig, old, new, xbuf, MAX_STRING_LENGTH);
}

const char * string_replace_ext( const char * orig, const char * old, const char * new, char *xbuf, int xbuf_length )
{
    int i;

    xbuf[0] = '\0';
    strcpy( xbuf, orig );

    /* Suggested fix by: Nebseni [nebseni@clandestine.bcn.net] */
    if ( strstr( orig, old ) != NULL 
       && strlen( orig ) 
          - strlen( strstr( orig, old ) ) 
          + strlen( new ) < (unsigned int)xbuf_length )
    {
        i = strlen( orig ) - strlen( strstr( orig, old ) );
        xbuf[i] = '\0';
        strcat( xbuf, new );
        strcat( xbuf, &orig[i + (int)strlen( old )] );
        free_string( orig );
    }

    return str_dup( xbuf );
}



/*****************************************************************************
 Name:		string_add
 Purpose:	Interpreter for string editing.
 Called by:	game_loop_xxxx(comm.c).
 ****************************************************************************/
void string_add( CHAR_DATA *ch, const char *argument )
{
   char buf[MAX_STRING_LENGTH], argbuf[MSL];
   
   /*
    * Thanks to James Seng
    */
   argument = smash_tilde_cpy( argbuf, argument );

   if ( !str_cmp(argument, ".q") || *argument == '~' || *argument == '@' )
   {
      if ( ch->desc->editor == ED_MPCODE ) /* for mobprogs */
      {
         PROG_CODE *mpc;
         EDIT_MPCODE(ch, mpc);
         fix_mprog_mobs( ch, mpc);
      }
      else if ( ch->desc->editor == ED_OPCODE ) /* for objprogs */
      {
          PROG_CODE *opc;
          EDIT_OPCODE(ch, opc);
          fix_oprog_objs( ch, opc);
      }
      else if ( ch->desc->editor == ED_APCODE ) /* for areaprogs */
      {
          PROG_CODE *apc;
          EDIT_APCODE(ch, apc);
          fix_aprog_areas(ch, apc);
      }
      else if ( ch->desc->editor == ED_RPCODE ) /* for roomprogs */
      {
          PROG_CODE *rpc;
          EDIT_RPCODE(ch, rpc);
          fix_rprog_rooms(ch, rpc);
      }
                                                   

      
      ch->desc->pString = NULL;

      if (ch->pcdata->new_penalty != NULL)
          penalty_finish(ch->desc, "");
      return;
   }

   if ( *argument == '.' )
   {
      char arg1 [MAX_INPUT_LENGTH];
      char arg2 [MAX_INPUT_LENGTH];
      char arg3 [MAX_INPUT_LENGTH];
      char tmparg3 [MAX_INPUT_LENGTH];
      
      argument = one_argument( argument, arg1 );
      argument = first_arg( argument, arg2, FALSE );
      strcpy( tmparg3, argument );
      argument = first_arg( argument, arg3, FALSE );

      if ( !str_cmp( arg1, ".c" ) )
      {
         send_to_char( "String cleared.\n\r", ch );
         free_string(*ch->desc->pString);
         *ch->desc->pString = str_dup( "" );
         
         return;
      }
      
      if ( !str_cmp( arg1, ".s" ) )
      {
        send_to_char( "String so far:\n\r", ch );
        /* wackyhacky for syntax highlighting for lua scripts */
        PROG_CODE *mpc;
        switch( ch->desc->editor)
        {
            case ED_MPCODE:
                 EDIT_MPCODE(ch, mpc);
                 if (mpc->is_lua)
                    dump_prog( ch, *ch->desc->pString, TRUE);
                 else
                    send_to_char_new( numlineas(*ch->desc->pString), ch, TRUE );/* RAW */
                 break;
            case ED_APCODE:
            case ED_OPCODE:
            case ED_RPCODE:
                dump_prog( ch, *ch->desc->pString, TRUE);
                break;
            default: 
                send_to_char_new( numlineas(*ch->desc->pString), ch, TRUE );/* RAW */
        }
        return;
      }
      
      if ( !str_cmp( arg1, ".r" ) )
      {
         if ( arg2[0] == '\0' )
         {
            send_to_char(
               "usage:  .r \"old string\" \"new string\"\n\r", ch );
            return;
         }
         
         *ch->desc->pString =
            string_replace( *ch->desc->pString, arg2, arg3 );
         sprintf( buf, "'%s' replaced with '%s'.\n\r", arg2, arg3 );
         send_to_char( buf, ch );
         return;
      }

      if ( !str_cmp( arg1, ".d" ) )
      {
         *ch->desc->pString = del_last_line( *ch->desc->pString );
         send_to_char( " Line removed.\n\r", ch );
         return;
      }
      
      if ( !str_cmp( arg1, ".f" ) )
      {
         *ch->desc->pString = format_string( *ch->desc->pString );
         send_to_char( "String formatted.\n\r", ch );
         return;
      }

      if ( !str_cmp( arg1, ".ld" ) )
      {
         *ch->desc->pString = string_linedel( *ch->desc->pString, atoi(arg2) );
         send_to_char( "Line deleted.\n\r", ch );
         return;
      }
      
      if ( !str_cmp( arg1, ".li" ) )
      {
         *ch->desc->pString = string_lineadd( *ch->desc->pString, tmparg3, atoi(arg2) );
         send_to_char( "Line inserted.\n\r", ch );
         return;
      }
      
      if ( !str_cmp( arg1, ".lr" ) )
      {
         *ch->desc->pString = string_linedel( *ch->desc->pString, atoi(arg2) );
         *ch->desc->pString = string_lineadd( *ch->desc->pString, tmparg3, atoi(arg2) );
         send_to_char( "Line replaced.\n\r", ch );
         return;
      }

      if ( !str_cmp( arg1, ".h" ) )
      {
         send_to_char( "Sedit help (commands on blank line):   \n\r", ch );
         send_to_char( ".r 'old' 'new'   - replace a substring \n\r", ch );
         send_to_char( "                   (requires '', \"\") \n\r", ch );
         send_to_char( ".d               - delete last line    \n\r", ch );
         send_to_char( ".h               - get help (this info)\n\r", ch );
         send_to_char( ".s               - show string so far  \n\r", ch );
         send_to_char( ".f               - (word wrap) string  \n\r", ch );
         send_to_char( ".c               - clear string so far \n\r", ch );
         send_to_char( ".ld <num>        - delete line number <num>\n\r", ch );
         send_to_char( ".li <num> <str>  - add <str> as line <num>\n\r", ch );
         send_to_char( ".lr <num> <str>  - replace line <num> with <str>\n\r", ch );
         send_to_char( ".q OR @          - end string          \n\r", ch );
         return;
      }
   
      send_to_char( "SEdit:  Invalid dot command.\n\r", ch );
      return;
   }
   
   strcpy( buf, *ch->desc->pString );
   
   /*
    * Truncate strings to MAX_STRING_LENGTH.
    * --------------------------------------
    */
   if ( strlen( buf ) + strlen( argument ) >= ( MAX_STRING_LENGTH - 4 ) )
   {
      send_to_char( "String too long, last line skipped.\n\r", ch );
      
      /* Force character out of editing mode. */
      ch->desc->pString = NULL;
      return;
   }
   
   strcat( buf, argument );
   strcat( buf, "\n\r" );
   free_string( *ch->desc->pString );
   *ch->desc->pString = str_dup( buf );
   return;
}



/*
 * Thanks to Kalgen for the new procedure (no more bug!)
 * Original wordwrap() written by Surreality.
 */
/*****************************************************************************
 Name:		format_string
 Purpose:	Special string formating and word-wrapping.
 Called by:	string_add(string.c) (many)olc_act.c
 ****************************************************************************/
const char* format_string( const char *oldstring /*, bool fSpace */)
{
  char xbuf[MAX_STRING_LENGTH];
  char xbuf2[MAX_STRING_LENGTH];
  const char *oc;
  char *rdesc;
  int i=0, noncol=0, pos=0;
  bool cap=TRUE;
  
  xbuf[0]=xbuf2[0]=0;
  
  i=0;
  
  /* First major loop reads in the oldstring, and removes excess spaces */
  for (oc = oldstring; *oc; oc++)
  {
    if (*oc=='\n')
    {
      if (xbuf[i-1] != ' ')
      {
        xbuf[i]=' ';
        i++;
      }
    }
    else if (*oc=='\r') ;
    else if (*oc==' ')
    {
      if (xbuf[i-1] != ' ')
      {
        xbuf[i]=' ';
        i++;
      }
    }
    else if (*oc==')')
    {
      if (xbuf[i-1]==' ' && xbuf[i-2]==' ' && 
          (xbuf[i-3]=='.' || xbuf[i-3]=='?' || xbuf[i-3]=='!'))
      {
        xbuf[i-2]=*oc;
        xbuf[i-1]=' ';
        xbuf[i]=' ';
        i++;
      }
      else
      {
        xbuf[i]=*oc;
        i++;
      }
    }
    else if (*oc=='.' || *oc=='?' || *oc=='!') {
      if (xbuf[i-1]==' ' && xbuf[i-2]==' ' && 
          (xbuf[i-3]=='.' || xbuf[i-3]=='?' || xbuf[i-3]=='!')) {
        xbuf[i-2]=*oc;
        if (*(oc+1) != '\"')
        {
          xbuf[i-1]=' ';
          xbuf[i]=' ';
          i++;
        }
        else
        {
          xbuf[i-1]='\"';
          xbuf[i]=' ';
          xbuf[i+1]=' ';
          i+=2;
          oc++;
        }
      }
      else
      {
        xbuf[i]=*oc;
        if (*(oc+1) != '\"')
        {
          xbuf[i+1]=' ';
          //xbuf[i+2]=' ';
          //i += 3;
	  i +=2;
        }
        else
        {
          xbuf[i+1]='\"';
          xbuf[i+2]=' ';
          //xbuf[i+3]=' ';
          //i += 4;
	  i += 3;
          oc++;
        }
      }
      cap = TRUE;
    }
    else
    {
      xbuf[i]=*oc;
      if ( cap )
        {
          cap = FALSE;
          xbuf[i] = UPPER( xbuf[i] );
        }
      i++;
    }
  }
  xbuf[i]=0;
  strcpy(xbuf2,xbuf);
  
  rdesc=xbuf2;
  
  xbuf[0]=0;
  
  /* Second major loop measures line length, wraps to 76 chars */
  for ( ; ; )
  {
    /* First, determines if the current text is less than one line long. */
    /* While we're at it, we can find the position of the 76th noncolourcode character. */
    noncol = 0;
    for ( pos=0; pos<MAX_STRING_LENGTH; pos++ )
    {
      if (!*(rdesc+pos)) break;

      if (*(rdesc+pos) == '{')
        noncol--;
      else noncol++;

      if( noncol > 76 ) break;
    }
    if ( noncol < 77 )
    {
      break;
    }

    /* From the 76th position (73rd if this is the first line) count back to a space */
    for (i=(xbuf[0]?pos:pos-3) ; i ; i--)
    {
      if (*(rdesc+i)==' ') break;
    }
    /* If we have found a space on the line (i.e. i != 0) ... add the line return */
    if (i)
    {
      *(rdesc+i)=0;
      strcat(xbuf,rdesc);
      strcat(xbuf,"\n\r");
      rdesc += i+1;
      while (*rdesc == ' ') rdesc++;
    }
    else
    {
      bug ("No spaces", 0);
      *(rdesc+75)=0;
      strcat(xbuf,rdesc);
      strcat(xbuf,"-\n\r");
      rdesc += 76;
    }
  }
  while (*(rdesc+i) && (*(rdesc+i)==' '||
                        *(rdesc+i)=='\n'||
                        *(rdesc+i)=='\r'))
    i--;
  *(rdesc+i+1)=0;
  strcat(xbuf,rdesc);
  if (xbuf[strlen_color(xbuf)-2] != '\n')
    strcat(xbuf,"\n\r");

  free_string(oldstring);
  return(str_dup(xbuf));
}

/*****************************************************************************
 Name:		force_wrap
 Purpose:	Wrap string to 80 characters per line.  Snippet by Jon Franz.
 Called by:	handle_con_note_text in board.c
 ****************************************************************************/
/* improved version by Bobble */
char *force_wrap( const char *old_string )
{
   int i, i_old;
   int last_return, last_blank, colour_count;
   char c;
   static char xbuf[MAX_STRING_LENGTH];
   xbuf[0]='\0';
   i = 0;
   last_return = -1;
   last_blank = -1;
   colour_count = 0;
   
   memset(xbuf, '\0', MAX_STRING_LENGTH);

   for (i_old = 0; (c = old_string[i_old]) != '\0'; i_old++)
   {
      if ( c == '\n' || c == '\r' )
	  last_return = i;

      /* the following is for Lope's color code support */
      if ( c == '{' )
         colour_count += 2;

      if ( c == ' ' )
	  last_blank = i;
      
      if (i == last_return+80+colour_count )
      {
	  /* can we wrap smoothly? */
	  if ( last_blank > last_return )
	  {
	      /* jump back to last blank, skip it on copy */
	      i_old += last_blank - i + 1;
	      c = old_string[i_old];
	      i = last_blank;
	  }

         xbuf[i++] = '\n';
         xbuf[i++] = '\r';
         last_return = i-1;
      }

      xbuf[i] = c;
      if (c == '~')
      {
         xbuf[i] = '\0';
         break;
      }
      if (xbuf[i] == '\0')
      {
         break;
      }
      i++; 
   }
   xbuf[i] = '\0'; /* make sure we end xbuf with a null, or else we're screwed -
                   since xbuf is static, it get reused every time this function is
                   called, allowing for previous string to bleed into new ones
                   unless we force the end with a null...*/
   return xbuf; /* throw it back! and we're done */
}


/*
 * Used above in string_add.  Because this function does not
 * modify case if fCase is FALSE and because it understands
 * parenthesis, it would probably make a nice replacement
 * for one_argument.
 */
/*****************************************************************************
 Name:		first_arg
 Purpose:	Pick off one argument from a string and return the rest.
 		Understands quates, parenthesis (barring ) ('s) and
 		percentages.
 Called by:	string_add(string.c)
 ****************************************************************************/
const char *first_arg( const char *argument, char *arg_first, bool fCase )
{
    char cEnd;

    while ( *argument == ' ' )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"'
      || *argument == '%'  || *argument == '(' )
    {
        if ( *argument == '(' )
        {
            cEnd = ')';
            argument++;
        }
        else cEnd = *argument++;
    }

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
    if ( fCase ) *arg_first = LOWER(*argument);
            else *arg_first = *argument;
	arg_first++;
	argument++;
    }
    *arg_first = '\0';

    while ( *argument == ' ' )
	argument++;

    return argument;
}

const char *string_linedel( const char *string, int line )
{
   const char *strtmp = string;
   char buf[MAX_STRING_LENGTH];
   int cnt = 1, tmp = 0;
   
   buf[0] = '\0';
   
   for ( ; *strtmp != '\0'; strtmp++ )
   {
      if ( cnt != line )
         buf[tmp++] = *strtmp;
      
      if ( *strtmp == '\n' )
      {
         if ( *(strtmp + 1) == '\r' )
         {
            if ( cnt != line )
               buf[tmp++] = *(++strtmp);
            else
               ++strtmp;
         }
         
         cnt++;
      }
   }
   
   buf[tmp] = '\0';
   
   free_string(string);
   return str_dup(buf);
}

const char *string_lineadd( const char *string, const char *newstr, int line )
{
   const char *strtmp = string;
   int cnt = 1, tmp = 0;
   bool done = FALSE;
   char buf[MAX_STRING_LENGTH];
   
   buf[0] = '\0';
   
   for ( ; *strtmp != '\0' || (!done && cnt == line); strtmp++ )
   {
      if ( cnt == line && !done )
      {
         strcat( buf, newstr );
         strcat( buf, "\n\r" );
         tmp += (int)strlen(newstr) + 2;
         cnt++;
         done = TRUE;
      }
      
      buf[tmp++] = *strtmp;
      
      if ( done && *strtmp == '\0' )
         break;
      
      if ( *strtmp == '\n' )
      {
         if ( *(strtmp + 1) == '\r' )
            buf[tmp++] = *(++strtmp);
         
         cnt++;
      }
      
      buf[tmp] = '\0';
   }
   
   free_string(string);
   return str_dup(buf);
}

/* buf queda con la linea sin \n\r */
const char *get_line( const char *str, char *buf )
{
   int tmp = 0;
   bool found = FALSE;
   
   while ( *str )
   {
      if ( *str == '\n' )
      {
         found = TRUE;
         break;
      }
      
      buf[tmp++] = *(str++);
   }
   
   if ( found )
   {
      if ( *(str + 1) == '\r' )
         str += 2;
      else
         str += 1;
   } /* para que quedemos en el inicio de la prox linea */
   
   buf[tmp] = '\0';
   
   return str;
}

const char* numlineas( const char *string )
{
   int cnt = 1;
   static char buf[MAX_STRING_LENGTH*2];
   char buf2[MAX_STRING_LENGTH], tmpb[MAX_STRING_LENGTH];
   
   buf[0] = '\0';
   
   while ( *string )
   {
      string = get_line( string, tmpb );
      sprintf( buf2, "%2d. %s\n\r", cnt++, tmpb );
      strcat( buf, buf2 );
   }
   
   return buf;
   
}


/*
 * Same as capitalize but changes the pointer's data.
 * Used in olc_act.c in aedit_builder.
 */
char * string_proper( char * argument )
{
    char *s;

    s = argument;

    while ( *s != '\0' )
    {
        if ( *s != ' ' )
        {
            *s = UPPER(*s);
            while ( *s != ' ' && *s != '\0' )
                s++;
        }
        else
        {
            s++;
        }
    }

    return argument;
}

/* truncate an optionally colored string to a certain length 
   */
const char *truncate_color_string( const char *argument, int limit )
{
    static SR_BUF sr_buf;
    char *rtn = next_sr_buf( &sr_buf );

    if ( strlen_color(argument) <= limit )
        return argument;
    else if ( strlen(argument) > MSL) 
    {
        bugf("truncate_color_string received string with length > MSL");
        return "ERROR"; /* So it won't crash */
    }

    int i=0;
    int len=0;
    for ( i=0 ; len < limit ; i++ ) 
    {
        rtn[i]=*(argument+i);
        len++;

        if (rtn[i] == '{')
        {
            i++;
            rtn[i]=*(argument+i);

            if (rtn[i] != '{')
                len--;
        }

    }

    rtn[i]='\0';

    return rtn;
}

const char *format_color_string( const char *argument, int width )
{
    static SR_BUF sr_buf;
    char *rtn = next_sr_buf( &sr_buf );

    int lencolor=strlen_color(argument);
    if ( lencolor > width )
        return truncate_color_string( argument, width );
    else if ( lencolor == width ) /* just in case */
        return argument;

    int i=0;
    int len=0;

    for (  ; *(argument+i) != '\0' ; i++ )
    {
        rtn[i]=*(argument+i);
        len++;

        if (rtn[i] == '{')
        {
            i++;

            if ( *(argument+i) == '\0')
                break;

            rtn[i]=*(argument+i);

            if (rtn[i] != '{')
                len--;
        }
    }

    for ( ; len < width ; i++ )
    {
       rtn[i]=' ';
       len++;
    }

    rtn[i]='\0';

    return rtn;
}

/*
 * This is a modified version of a function written by Wreck.
 * Original at http://http://dark.nrg.dtu.dk/~wreck/mud/code/color.txt
 *						Dennis Reed
 */
int strlen_color( const char *argument )
{
    const char *str;
    int  length;
    
    if ( argument == NULL || argument[0] == '\0' )
        return 0;
    
    length = 0;
    str = argument;
    
    while ( *str != '\0' )
    {
        if ( *str != '{' )
        {
            str++;
            length++;
            continue;
        }
        
        if (*(++str) == '{')
            length++;
        
        str++;
    }
    
    return length;
}


/* center() 
 * Copyright 1997-1998 by Bobby J. Bailey <wordweaver@usa.net>
 * Color code support by Rimbol, 6/99.
 */

/* CENTER Function
 * Syntax: center( "argument", width, "fill" ) AHA, NO, this is WRONG,
 * the syntax should have one char, for the fill, not a string.
 *
 * This function will center "argument" in a string <width> characters
 * wide, using the "fill" character for padding on either end of the
 * string when centering. If "argument" is longer than <width>, the
 * "argument" is returned unmodified.
 *
 * Example:
 *     center( "CENTER", 20, "=" )
 * Will return:
 *     "=======CENTER======="
 */

const char * center( const char *argument, int width, char fill )
{
    char	buf[MSL];
    static char buf2[MSL];
    int		length; /* Length without color codes */
    int		lead_chrs;
    int		trail_chrs;
    
    if ( !argument )
    {
        sprintf( log_buf, "ERROR! Please note an imm if you see this message.\n\rPlease include EXACTLY what you did before you got this message.\n\r");
        return log_buf;
    }
    
    memset(buf, '\0', MSL);
	memset(buf2, '\0', MSL); /* Wipe the buffer */

    length = strlen_color( argument );
    
    if ( length >= width )
        return argument;
    
    lead_chrs = (int)((width / 2) - (length / 2) + .5);
    memset( buf2, fill, (size_t)lead_chrs );

    strcat( buf2, argument);

    trail_chrs = width - lead_chrs - length;
    if( trail_chrs > 0 )
    {
        memset( buf, fill, (size_t)trail_chrs );
        strcat( buf2, buf );
    }
    
    return buf2;
}

/* Color code aware left pad function.  -Rimbol 6/99 */
const char *lpad( const char *argument, int width, char fill )
{
    static char buf2[MSL];
    int		length; /* Length without color codes */
    int		lead_chrs;
    
    if ( !argument )
    {
        sprintf( log_buf, "ERROR! Please note an imm if you see this message.\n\rPlease include EXACTLY what you did before you got this message.\n\r");
        return log_buf;
    }
    
    memset(buf2, '\0', MSL); /* Wipe the buffer */

    length = strlen_color( argument );
    
    if ( length >= width )
        return argument;
    
    lead_chrs = width - length;
    memset( buf2, fill, (size_t)lead_chrs );

    strcat( buf2, argument);

    return buf2;
}

/* Color code aware right pad function.  -Rim 6/99 */
const char *rpad( const char *argument, int width, char fill )
{
    char	buf[MSL];
    static char buf2[MSL];
    int		length; /* Length without color codes */
    int		trail_chrs;
    
    if ( !argument )
    {
        sprintf( log_buf, "ERROR! Please note an imm if you see this message.\n\rPlease include EXACTLY what you did before you got this message.\n\r");
        return log_buf;
    }
    
    memset(buf, '\0', MSL);
	memset(buf2, '\0', MSL); /* Wipe the buffer */

    length = strlen_color( argument );
    
    if ( length >= width )
        return argument;
    
    strcat( buf2, argument);

    trail_chrs = width - length;

    if( trail_chrs > 0 )
    {
        memset( buf, fill, (size_t)trail_chrs );
        strcat( buf2, buf );
    }
    
    return buf2;
}

/* return wether a string 'looks' empty */
bool is_empty_string( const char *s )
{
    if ( s == NULL )
	return TRUE;

    while ( *s != '\0' )
    {
	if ( *s == '{' )
	{
	    s++;
	    if ( *s == '\0' )
		return FALSE;
	}
	else if ( !isspace(*s) )
	    return FALSE;
	s++;
    }

    return TRUE;
}

const char* ltrim(const char *s)
{
    while ( isspace(*s) )
        s++;
    return s;
}

const char* aan(const char *s)
{
    switch (*s) {
        case 'a':
        case 'A':
        case 'e':
        case 'E':
        case 'i':
        case 'I':
        case 'o':
        case 'O':
        case 'u':
        case 'U':
            return "an";
        default:
            return "a";
    }
}

bool is_alpha_string( const char *s )
{
    while ( *s != '\0' )
        if ( !isalpha(*(s++)) )
            return FALSE;
    return TRUE;
}

// find color code used for displaying variable in prompt
char prompt_color_code( const char *prompt, char var )
{
    if ( !prompt )
        return 'x';
    
    char color_code = 'x';
    while ( *prompt != '\0' && *(prompt+1) != '\0' )
    {
        switch ( *prompt )
        {
            case '{':
                color_code = *(++prompt);
                break;
            case '%':
                if ( *(++prompt) == var )
                    return color_code;
            default:
                break;
        }
        prompt++;
    }
    
    return color_code;
}

// returns true if s contains split_char
// in that case, also splits s and returns parts in prefix and suffix
bool split_string( const char *s, char split_char, char *prefix, char *suffix )
{
    size_t split_pos;

    for ( split_pos = 0; s[split_pos] != '\0'; split_pos++ )
        if ( s[split_pos] == split_char )
            break;
    if ( s[split_pos] == '\0' )
        return false;

    strncpy(prefix, s, split_pos);
    prefix[split_pos] = '\0';
    strcpy(suffix, s + split_pos + 1);

    return true;
}
