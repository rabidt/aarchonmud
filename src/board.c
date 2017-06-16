/***************************************************************************
*  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*  Michael Seifert, Hans Henrik Starfeldt, Tom Madsen, and Katja Nyboe.   *
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
***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "buffer_util.h"

/*

  Note Board system, (c) 1995-96 Erwin S. Andreasen, erwin@pip.dknet.dk
  =====================================================================
  
  Basically, the notes are split up into several boards. The boards do 
  not exist physically, they can be read anywhere and in any position.
    
  Each of the note boards has its own file. Each of the boards can have
  its own "rights": who can read/write.
      
  Each character has an extra field added, namele the timestamp of the 
  last note read by him/her on a certain board.
        
  The note entering system is changed too, making it more interactive. 
  When entering a note, a character is put AFK and into a special CON_ 
  state.  Everything typed goes into the note.
          
  For the immortals it is possible to purge notes based on age. An Archive
  option is available which moves the notes older than X days into a special
  board. The file of this board should then be moved into some other directory
  during e.g. the startup script and perhaps renamed depending on date.
            
  Note that write_level MUST be >= read_level or else there will be strange
  output in certain functions.
              
  Board DEFAULT_BOARD must be at least readable by *everyone*.
                
*/ 

/* 
  This version modified December 1997 by Brian Castle, for use with
  ROM 2.4b4, running Lope's color code v1.2.  With this version, Erwin's
  color codes have been changed to Lope's, and all write_to_buffer() calls
  have been replaced with calls to act(), send_to_char(), or
  printf_to_char() in order to process Lope's color correctly.  Also, the
  editor has been enhanced from Erwin's version to include selected
  functionality from Ivan's OLC v1.6 and from the stock ROM 2.4b4 note system.

  More mods in June 1999: Special access levels, max message count. -Rim
*/

/* Local function prototypes (some are defined in merc.h) */
static void load_board (BOARD_DATA *board);
static void save_board (BOARD_DATA *board);

static void append_note (FILE *fp, NOTE_DATA *note);
static void unlink_note (BOARD_DATA *board, NOTE_DATA *note);
static void archive_note (BOARD_DATA *board, NOTE_DATA *pnote);

int board_number (const BOARD_DATA *board);
static NOTE_DATA* find_note (CHAR_DATA *ch, BOARD_DATA *board, int num);

static void show_note_to_char (CHAR_DATA *ch, NOTE_DATA *note, int num);
int unread_notes (CHAR_DATA *ch, BOARD_DATA *board);
static bool next_board (CHAR_DATA *ch);

static void do_nwrite (CHAR_DATA *ch, const char *argument);
static void do_nread (CHAR_DATA *ch, const char *argument);
static void do_nremove (CHAR_DATA *ch, const char *argument);
static void do_nlist (CHAR_DATA *ch, const char *argument);
static void do_ncatchup (CHAR_DATA *ch, const char *argument);
static void do_ncatchup_all (CHAR_DATA *ch);

DECLARE_DO_FUN(do_note);
DECLARE_DO_FUN(do_board);

/* External function declarations. */
DECLARE_DO_FUN( do_help );


#define BOARD_NONE 0
#define BOARD_NOACCESS -1
#define BOARD_NOTFOUND -1
#define BOARD_BUILDER 1
#define BOARD_PKILL 2

/* The prompt that the character is given after finishing a note with ~ or END */
const char * szFinishPrompt = "({+C{x)ontinue, ({+V{x)iew, ({+P{x)ost or ({+F{x)orget it?";

long last_note_stamp = 0; /* To generate unique timestamps on notes */

static bool next_board (CHAR_DATA *ch);
const static char* note_line = 
"===============================================================================\n\r";

/* In the section below, I made the following changes. Added Druids board, Mechanics board,
   Roleplay and Immortal hold notes for 300 days instead of 200. Immortal board holds 90
   notes instead of 30. News board has 500 days expiration instead of 200. Astark 1-2-13 */

   
/*  Short Name,     Long Name,             ReadLvl, WriteLvl,  Access, Dflt Recipient, Dflt Actn, Days Expir, Max Msg, NULL, FALSE */
BOARD_DATA boards[MAX_BOARD] =
{
   { "General",  "General discussion",           0,     2,     BOARD_NONE,    "all",     DEF_INCLUDE,  120,    200,    NULL, FALSE },
   { "Personal", "Personal messages",            0,     2,     BOARD_NONE,    "all",     DEF_EXCLUDE,  120,    500,    NULL, FALSE },
   { "Ideas",    "Suggestions for improvement",  0,     2,     BOARD_NONE,    "all",     DEF_NORMAL,   120,    200,    NULL, FALSE }, 
   { "Mechanics","Game Mechanics",               0,     2,     BOARD_NONE,    "all",     DEF_NORMAL,   120,    200,    NULL, FALSE },
   { "Bugs",     "Typos, bugs, errors",          0,     2,     BOARD_NONE,    "imm",     DEF_NORMAL,   120,    200,    NULL, FALSE },
   { "Pkill",    "Player killers only!",         0,     2,     BOARD_PKILL,   "pkill",   DEF_NORMAL,   120,    200,    NULL, FALSE }, 
   { "Roleplay", "Role Playing / Family notes",  0,     2,     BOARD_NONE,    "all",     DEF_NORMAL,   120,    200,    NULL, FALSE }, 
   { "Quest",    "Quest-related messages",       0,     2,     BOARD_NONE,    "all",     DEF_NORMAL,   120,    200,    NULL, FALSE },
   { "Builder",  "Area building",                0,     2,     BOARD_BUILDER, "builder", DEF_NORMAL,   120,    200,    NULL, FALSE }, 
   { "News",     "Announcements from Immortals", 0, DEMIGOD,   BOARD_NONE,    "all",     DEF_NORMAL,   120,    500,    NULL, FALSE },
   { "Immortal", "Immortals only",            DEMIGOD, DEMIGOD,BOARD_NONE,    "imm",     DEF_NORMAL,   120,    500,    NULL, FALSE },
   { "Penalty",  "Penalty notes (imm only)",     0, DEMIGOD,   BOARD_NONE,    "imm",     DEF_NORMAL,   120,    200,    NULL, FALSE },
};



/* append this note to the given file */
static void append_note (FILE *fp, NOTE_DATA *note)
{
   rfprintf (fp, "Sender  %s~\n", note->sender);
   rfprintf (fp, "Date    %s~\n", note->date);
   fprintf (fp, "Stamp   %ld\n", note->date_stamp);
   fprintf (fp, "Expire  %ld\n", note->expire);
   rfprintf (fp, "To      %s~\n", note->to_list);
   rfprintf (fp, "Subject %s~\n", note->subject);
   rfprintf (fp, "Text\n%s~\n\n", note->text);
}

/* Remove note from the list. Do not free note */
static void unlink_note (BOARD_DATA *board, NOTE_DATA *note)
{
   NOTE_DATA *p;
   
   if (board->note_first == note)
      board->note_first = note->next;
   else
   {
      for (p = board->note_first; p && p->next != note; p = p->next);
      if (!p)
         bug ("unlink_note: could not find note.",0);
      else
         p->next = note->next;
   }
}

/* Archive a note */
static void archive_note (BOARD_DATA *board, NOTE_DATA *pnote)
{
    FILE *fp_archive;
    char archive_name[200];
    
    sprintf (archive_name, "%s%s.old", NOTE_DIR, board->short_name);
    fp_archive = fopen (archive_name, "a");

    if (!fp_archive)
        bug("Could not open archive board for writing.",0);
    else
    {
        append_note (fp_archive, pnote);
        fclose (fp_archive);
    }
}      


/* Find the number of a board */
int board_number (const BOARD_DATA *board)
{
    int i;

    for (i = 0; i < MAX_BOARD; i++)
        if (board == &boards[i])
            return i;

    return -1;
}

/* Find a board number based on  a string */
int board_lookup (const char *name)
{
    int i;

    for (i = 0; i < MAX_BOARD; i++)
        if (!str_cmp (boards[i].short_name, name))
            return i;

    return BOARD_NOTFOUND;
}


/* Find the nth note on a board. Return NULL if ch has no access to that note */
static NOTE_DATA* find_note (CHAR_DATA *ch, BOARD_DATA *board, int num)
{
    int count = 0;
    NOTE_DATA *p;

    for (p = board->note_first; p ; p = p->next)
        if (++count == num)
            break;

    if ( (count == num) && is_note_to (ch, p))
        return p;
    else
        return NULL;

}

/* save a single board */
static void save_board (BOARD_DATA *board)
{
   FILE *fp;
   char filename[200];
   char buf[200];
   NOTE_DATA *note;
   
   sprintf (filename, "%s%s", NOTE_DIR, board->short_name);
   
   fp = fopen (filename, "w");
   if (!fp)
   {
      sprintf (buf, "Error writing to: %s", filename);
      bug (buf, 0);
   }
   else
   {
      for (note = board->note_first; note ; note = note->next)
         append_note (fp, note);
      
      fclose (fp);
   }
}

/* Save a note in a given board */
void finish_note (BOARD_DATA *board, NOTE_DATA *note)
{
   FILE *fp;
   NOTE_DATA *p, *q;
   char filename[200];
   int count = 1;
   
   /* The following is done in order to generate unique date_stamps */
   
   if (last_note_stamp >= current_time)
      note->date_stamp = ++last_note_stamp;
   else
   {
      note->date_stamp = current_time;
      last_note_stamp = current_time;
   }
   
   if (board->note_first) /* are there any notes in there now? */
   {
      for (p = board->note_first; p->next; p = p->next )
         count++; /* empty */

      while (count >= board->purge_count) /* Trim off any notes that violate the purge count */
      {
          q = board->note_first;
          archive_note(board, q);
          unlink_note(board, q);
          free_note (q);
          save_board(board);
          count--;
      }
      
      p->next = note;
   }
   else /* nope. empty list. */
      board->note_first = note;

   /* append note to note file */		
   sprintf (filename, "%s%s", NOTE_DIR, board->short_name);
   
   fp = fopen (filename, "a");
   if (!fp)
   {
      bug ("Could not open one of the note files in append mode",0);
      board->changed = TRUE; /* set it to TRUE hope it will be OK later? */
      return;
   }
   
   append_note (fp, note);
   fclose (fp);
}

/* Show one note to a character */
static void show_note_to_char (CHAR_DATA *ch, NOTE_DATA *note, int num)
{
   char buf[10*MAX_STRING_LENGTH];
   BUFFER *output;
   
   output = new_buf();
   
   /* Ugly colors ? */	
   sprintf (buf,
      "[{+%4d{x] {Y%s{x: {g%s{x\n\r"
      "{YDate{x:  %s\n\r"
      "{YTo{x:    %s\n\r"
      "{g==========================================================================={x\n\r"
      "%s\n\r"
      "{g==========================================================================={x\n\r",
      num, note->sender, note->subject,
      note->date,
      note->to_list,
      parse_url(note->text));

    add_buf(output, buf);
  
   page_to_char(buf_string(output),ch);
   free_buf(output);
}

/* Save changed boards */
void save_notes ( void )
{
   int i;
   
   for (i = 0; i < MAX_BOARD; i++)
      if (boards[i].changed) /* only save changed boards */
         save_board (&boards[i]);
}

/* Load a single board */
static void load_board (BOARD_DATA *board)
{
   FILE *fp;
   NOTE_DATA *last_note;
   char filename[200];
   
   sprintf (filename, "%s%s", NOTE_DIR, board->short_name);
   
   fp = fopen (filename, "r");
   
   /* Silently return */
   if (!fp)
      return;		
   
   /* Start note fetching. copy of db.c:load_notes() */
   
   last_note = NULL;
   
   for ( ; ; )
   {
      NOTE_DATA *pnote;
      char letter;
      
      do
      {
         letter = getc( fp );
         if ( feof(fp) )
         {
            fclose( fp );
            return;
         }
      }
      while ( isspace(letter) );
      ungetc( letter, fp );
      
      pnote             = alloc_perm( sizeof(*pnote) );
      
      if ( str_cmp( fread_word( fp ), "sender" ) )
         break;
      pnote->sender     = fread_string( fp );
      
      if ( str_cmp( fread_word( fp ), "date" ) )
         break;
      pnote->date       = fread_string( fp );
      
      if ( str_cmp( fread_word( fp ), "stamp" ) )
         break;
      pnote->date_stamp = fread_number( fp );
      
      if ( str_cmp( fread_word( fp ), "expire" ) )
         break;
      pnote->expire = fread_number( fp );
      
      if ( str_cmp( fread_word( fp ), "to" ) )
         break;
      pnote->to_list    = fread_string( fp );
      
      if ( str_cmp( fread_word( fp ), "subject" ) )
         break;
      pnote->subject    = fread_string( fp );
      
      if ( str_cmp( fread_word( fp ), "text" ) )
         break;
      pnote->text       = fread_string( fp );
      
      pnote->next = NULL; /* jic */
      
      /* Should this note be archived right now ? */
      if (pnote->expire < current_time)
      {
          archive_note(board, pnote);
          free_note (pnote);

          board->changed = TRUE;
          continue;
      }
      
      if ( board->note_first == NULL )
         board->note_first = pnote;
      else
         last_note->next     = pnote;
      
      last_note         = pnote;
   }
   
   bug( "Load_notes: bad key word.", 0 );
   return; /* just return */
}


/* Initialize structures. Load all boards. */
void load_boards ( void )
{
   int i;
   
   for (i = 0; i < MAX_BOARD; i++)
      load_board (&boards[i]);
}

/* Returns TRUE if the specified note is addressed to ch */
bool is_note_to (CHAR_DATA *ch, NOTE_DATA *note)
{
   if (IS_NPC(ch))
      return FALSE;

   if (!str_cmp (ch->name, note->sender))
      return TRUE;

   if (is_exact_name ("all", note->to_list))
      return TRUE;

   if ( (IS_SET(ch->act, PLR_PERM_PKILL) || IS_IMMORTAL(ch))
      && is_exact_name("pkill", note->to_list) )
      return TRUE;

   if ( (IS_SET(ch->act, PLR_HELPER) || IS_IMMORTAL(ch))
	&& ( is_exact_name("helper", note->to_list) ||
	     is_exact_name("helpers", note->to_list)) )
      return TRUE;

   if ( (ch->pcdata->security > 0 && ch->level >= SAVANT)
      && is_exact_name("builder", note->to_list) )
      return TRUE;
   
   if (IS_IMMORTAL(ch) && ( 
      is_exact_name ("imm", note->to_list) ||
      is_exact_name ("imms", note->to_list) ||
      is_exact_name ("immortal", note->to_list) ||
      is_exact_name ("god", note->to_list) ||
      is_exact_name ("gods", note->to_list) ||
      is_exact_name ("immortals", note->to_list)))
      return TRUE;

   if ((get_trust(ch) == MAX_LEVEL) && (
      is_exact_name ("imp", note->to_list) ||
      is_exact_name ("imps", note->to_list) ||
      is_exact_name ("implementor", note->to_list) ||
      is_exact_name ("implementors", note->to_list)))
      return TRUE;
   
   if ( ch->clan 
       && clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_note 
       && is_exact_name(clan_table[ch->clan].name, note->to_list) )
      return TRUE;

   /*
   if ( (rel = get_religion(ch)) != NULL
	&& is_religion_member(ch)
	&& is_exact_name(rel->name, note->to_list) )
      return TRUE;
   */
   
   if (is_exact_name (ch->name, note->to_list))
      return TRUE;
   
   /* Allow a note to e.g. 40 to send to characters level 40 and above */		
   /*
   if (is_number(note->to_list) && get_trust(ch) >= atoi(note->to_list))
      return TRUE;
   */
   return FALSE;
}

/* Return the number of unread notes 'ch' has in 'board' */
/* Returns BOARD_NOACCESS if ch has no access to board */
int unread_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
   NOTE_DATA *note;
   time_t last_read;
   int count = 0;
   
   if (IS_NPC(ch))
       return BOARD_NOACCESS;

   if (board->read_level > get_trust(ch))
      return BOARD_NOACCESS;

   if (board->special_access != BOARD_NONE && !IS_IMMORTAL(ch)) /* Imms ignore these */
   {
       if (board->special_access == BOARD_PKILL && !IS_SET(ch->act, PLR_PERM_PKILL))
           return BOARD_NOACCESS;
       if (board->special_access == BOARD_BUILDER && ch->pcdata->security <= 0)
           return BOARD_NOACCESS;
   }
   
   last_read = ch->pcdata->last_note[board_number(board)];
   
   for (note = board->note_first; note; note = note->next)
      if (is_note_to(ch, note) && !is_exact_name(ch->name, note->sender) && ((long)last_read < (long)note->date_stamp))
         count++;
      
   return count;
}


/*
 * COMMANDS
 */

/* Start writing a note */
static void do_nwrite (CHAR_DATA *ch, const char *argument)
{
   char *strtime;
   char buf[200];
   
   if (IS_NPC(ch)) /* NPC cannot post notes */
      return;
   
   if (IS_SET(ch->penalty, PENALTY_NONOTE))
   {
       send_to_char("Your note writing privileges have been revoked.\n\r", ch);
       return;
   }

   if (get_trust(ch) < ch->pcdata->board->write_level)
   {
      send_to_char ("You cannot post notes on this board.\n\r",ch);
      return;
   }
   
   /* continue previous note, if any text was written*/ 
   if (ch->pcdata->in_progress && (!ch->pcdata->in_progress->text))
   {
      send_to_char ("Your note in progress was cancelled because you had not yet written any text.\n\r\n\r", ch);
      free_note (ch->pcdata->in_progress);		              
      ch->pcdata->in_progress = NULL;
   }
   
   
   if (!ch->pcdata->in_progress)
   {
      ch->pcdata->in_progress = new_note();
      ch->pcdata->in_progress->sender = str_dup (ch->name);
      
      /* convert to ascii. ctime returns a string which last character is \n, so remove that */	
      strtime = ctime (&current_time);
      strtime[strlen(strtime)-1] = '\0';
      
      ch->pcdata->in_progress->date = str_dup (strtime);
   }
   
   act ("{G$n starts writing a note.{x", ch, NULL, NULL, TO_ROOM);
   
   /* Begin writing the note ! */
   sprintf (buf, "You are now %s a note on the {+%s{x board.\n\r",
      ch->pcdata->in_progress->text ? "continuing" : "posting",
      ch->pcdata->board->short_name);
   send_to_char (buf,ch);
   
   sprintf (buf, "{YFrom{x:    %s\n\r\n\r", ch->name);
   send_to_char (buf,ch);
   
   if (!ch->pcdata->in_progress->text) /* Are we continuing an old note or not? */
   {
      switch (ch->pcdata->board->force_type)
      {
      case DEF_NORMAL:
         sprintf (buf, "If you press Return, default recipient \"{+%s{x\" will be chosen.\n\r",
            ch->pcdata->board->names);
         break;
      case DEF_INCLUDE:
         sprintf (buf, "The recipient list MUST include \"{+%s{x\". If not, it will be added automatically.\n\r",
            ch->pcdata->board->names);
         break;
         
      case DEF_EXCLUDE:
         sprintf (buf, "The recipient of this note must NOT include: \"{+%s{x\".",
            ch->pcdata->board->names);
         
         break;
      }			
      
      send_to_char (buf,ch);
      send_to_char ("\n\r{YTo{x:      ",ch);
      
      ch->desc->connected = CON_NOTE_TO;
      /* nanny takes over from here */
      
   }
   else /* we are continuing, print out all the fields and the note so far*/
   {
      printf_to_char( ch,"{YTo{x:      %s\n\r"
         "{YExpires{x: %s\n\r"
         "{YSubject{x: %s\n\r", 
         ch->pcdata->in_progress->to_list,
         ctime(&ch->pcdata->in_progress->expire),
         ch->pcdata->in_progress->subject);
      
      send_to_char ("\n\rEnter text. Type {+END{x, {+@{x, or {+.q{x to finish, {+.h{x for help.\n\r", ch);
      send_to_char ( note_line, ch );
      page_to_char (ch->pcdata->in_progress->text, ch);
      
      ch->desc->connected = CON_NOTE_TEXT;		            
      
   }
   
}


/* Read next note in current group. If no more notes, go to next board */
static void do_nread (CHAR_DATA *ch, const char *argument)
{
    NOTE_DATA *p;
    int count = 0, number;
    time_t *last_note = &ch->pcdata->last_note[board_number(ch->pcdata->board)];

    if (is_number (argument))
    {
        number = atoi(argument);

        for (p = ch->pcdata->board->note_first; p; p = p->next)
            if (++count == number)
                break;

        if (!p || !is_note_to(ch, p))
            send_to_char ("No such note.\n\r",ch);
        else
        {
            show_note_to_char (ch,p,count);
            *last_note =  UMAX (*last_note, p->date_stamp);
        }
    }
    else /* just next one */
    {
        char buf[200];

        count = 1;
        for (p = ch->pcdata->board->note_first; p ; p = p->next, count++)
            if ((p->date_stamp > *last_note) && is_note_to(ch,p) && !is_exact_name(ch->name, p->sender))
            {
                show_note_to_char (ch,p,count);
                /* Advance if new note is newer than the currently newest for that char */
                *last_note =  UMAX (*last_note, p->date_stamp);
                return;
            }

        send_to_char ("No new notes on this board.\n\r",ch);

        if (ch->pcdata->in_progress)
            sprintf (buf, "You have a note in progress on this board.\n\r" );
        else if (next_board (ch))
            sprintf (buf, "Changed to next board, %s.\n\r", ch->pcdata->board->short_name );
        else
            sprintf (buf, "There are no more boards.\n\r");

        send_to_char (buf,ch);
    }
}

/* Remove a note */
static void do_nremove (CHAR_DATA *ch, const char *argument)
{
   NOTE_DATA *p;
   
   if (!is_number(argument))
   {
      send_to_char ("Remove which note?\n\r",ch);
      return;
   }
   
   p = find_note (ch, ch->pcdata->board, atoi(argument));
   if (!p)
   {
      send_to_char ("No such note.\n\r",ch);
      return;
   }
   
   if (str_cmp(ch->name,p->sender) && (get_trust(ch) < ARCHON))
   {
      send_to_char ("You are not authorized to remove this note.\n\r",ch);
      return;
   }
   
   unlink_note (ch->pcdata->board,p);
   free_note (p);
   send_to_char ("Note removed!\n\r",ch);
   
   save_board(ch->pcdata->board); /* save the board */
}


/* List all notes or if argument given, list N of the last notes */
/* Shows REAL note numbers! */
static void do_nlist (CHAR_DATA *ch, const char *argument)
{
   int count= 0, show = 0, num = 0, has_shown = 0;
   time_t last_note;
   NOTE_DATA *p;
   char buf[MAX_STRING_LENGTH];
   char ts_buf[80]; /* New, for dates */
   BUFFER *output;

   output = new_buf();
   
   if (is_number(argument))	 /* first, count the number of notes */
   {
      show = atoi(argument);
      
      for (p = ch->pcdata->board->note_first; p; p = p->next)
         if (is_note_to(ch,p))
            count++;
   }
   
   add_buf(output,"{+Notes on this board:{x\n\r");
   add_buf(output,"{rNum> Author        Date        Subject{x\n\r");
   
   last_note = ch->pcdata->last_note[board_number (ch->pcdata->board)];
   
   for (p = ch->pcdata->board->note_first; p; p = p->next)
   {
      num++;
      if (is_note_to(ch,p))
      {
         has_shown++; /* note that we want to see X VISIBLE note, not just last X */
         if (!show || ((count-show) < has_shown))
         {
          /* The next line here is used to display the date properly - Vodur 1-7-13 */
	    strftime(ts_buf,sizeof(ts_buf),"%x", localtime(&(p->date_stamp)));
            sprintf (buf, "{+%3d{x>{B%c{Y%-13s{x{y%-12s %s{x \n\r",
               num, 
               (last_note < p->date_stamp && !is_exact_name(ch->name, p->sender)) ? '*' : ' ',
               p->sender, 
	       ts_buf,
               p->subject);
            add_buf(output,buf);
         }
      }
   }
   if (num == 0)
      add_buf(output,"{+There are currently no notes addressed to you on this board.{x\n\r");

   page_to_char(buf_string(output),ch);
   free_buf(output);

   return;
   
}

/* catch up with some notes */
static void do_ncatchup (CHAR_DATA *ch, const char *argument)
{
    if (argument[0] != '\0')
    {
        if ( strcmp( argument, "all" ) )
        {
            send_to_char( "Invalid argument.", ch );
            return;
        }
        else
        {
            do_ncatchup_all( ch );
            return;
        }
    }

    /* else no arg */
   NOTE_DATA *p;
   
   /* Find last note */	
   for (p = ch->pcdata->board->note_first; p && p->next; p = p->next);
   
   if (!p)
      send_to_char ("Alas, there are no notes in that board.\n\r",ch);
   else
   {
      ch->pcdata->last_note[board_number(ch->pcdata->board)] = p->date_stamp;
      send_to_char ("All messages skipped.\n\r",ch);
   }
}

static void do_ncatchup_all ( CHAR_DATA *ch)
{
    int i=0;
    for ( ch->pcdata->board=&boards[0] ; ++i < MAX_BOARD ; ch->pcdata->board=&boards[i] )
    {
        ptc( ch, "Catching up %s notes.\n\r", boards[i].short_name );
        do_ncatchup( ch, "" );
    }
    
    ch->pcdata->board=&boards[0];
       
}

/* Dispatch function for backwards compatibility */
DEF_DO_FUN(do_note)
{
   char arg[MAX_INPUT_LENGTH];
   
   if (IS_NPC(ch))
      return;

   if (NOT_AUTHED(ch))
   {
       send_to_char("You cannot access notes until you are authorized.\n\r",ch);
       return;
   }

   /*
   if (IS_SET(ch->penalty, PENALTY_NONOTE))
   {
       send_to_char("Your note privileges have been revoked.\n\r", ch);
       return;
   }
   */
   
   argument = one_argument (argument, arg);
   
   if ((!arg[0]) || (!str_cmp(arg, "read"))) /* 'note' or 'note read X' */
      do_nread (ch, argument);
   
   else if (!str_cmp (arg, "list"))
      do_nlist (ch, argument);
   
   else if (!str_cmp (arg, "write") || !str_cmp (arg, "to") || !str_cmp (arg, "subject"))
      do_nwrite (ch, argument);
   
   else if (!str_cmp (arg, "remove"))
      do_nremove (ch, argument);
   
   else if (!str_cmp (arg, "catchup"))
      do_ncatchup (ch, argument);
   
   else if (!str_cmp (arg, "help"))
      do_help (ch, "note");
   
   else 
      send_to_char("Usage:  note                 - Read next new note.  If none, try next board.\n\r"
      "        note read <number>   - Read a specific note on the current board.\n\r"
      "        note write           - Create a new note on the current board.\n\r"
      "        note list            - List all notes on the current board.\n\r"
      "        note remove <number> - Remove a specific note on the current board.\n\r"
      "        note catchup         - Skip all unread notes on the current board.\n\r"
      "        note catchup all     - Skip all unread notes on all boards.\n\r"
      "        note help            - Same as HELP NOTE.\n\r",ch);
   
}

/* Show all accessible boards with their numbers of unread messages OR
   change board. New board name can be given as a number or as a name (e.g.
   board personal or board 4 */
DEF_DO_FUN(do_board)
{
   int i, count, number;
   char buf[200];
   
   if (IS_NPC(ch))
      return;

   if (NOT_AUTHED(ch))
   {
       send_to_char("You cannot access notes until you are authorized.\n\r",ch);
       return;
   }

   if (IS_SET(ch->penalty, PENALTY_NONOTE))
   {
	   send_to_char("Your note privileges have been revoked.\n\r", ch);
	   return;
   }

   if (!argument[0]) /* show boards */
   {
      int unread = 0, total_notes = 0;
      NOTE_DATA *p;

      count = 1;
      send_to_char ("{BNum         Name Unread/Total  Description{x\n\r"
         "{r=== ============ ============= ==========={x\n\r",ch);
      for (i = 0; i < MAX_BOARD; i++)
      {
         unread = unread_notes (ch,&boards[i]); /* how many unread notes? */
         if (unread != BOARD_NOACCESS)
         { 
            total_notes = 0;
            for (p = boards[i].note_first; p; p = p->next)
               if (is_note_to(ch,p))
                  total_notes++;      
               
            printf_to_char (ch, "{+%2d{x> {G%12s{x [%s%4d{g / %4d{x] {y%s{x\n\r", 
               count, boards[i].short_name, unread ? "{r" : "{g", 
               unread, total_notes, boards[i].long_name);
            count++;
         } /* if has access */
         
      } /* for each board */
      
      sprintf (buf, "\n\rYour current board is {+%s{x.\n\r", ch->pcdata->board->short_name);
      send_to_char (buf,ch);
      
      /* Inform of rights */		
      if (ch->pcdata->board->read_level > get_trust(ch))
         send_to_char ("You cannot read or write notes on this board.\n\r",ch);
      else if (ch->pcdata->board->write_level > get_trust(ch))
         send_to_char ("You can only read notes from this board.\n\r",ch);
      else
         send_to_char ("You can both read and write on this board.\n\r",ch);
      
      return;			
   } /* if empty argument */
   
   if (ch->pcdata->in_progress)
   {
      send_to_char ("Please finish your interrupted note first.\n\r",ch);
      return;
   }
   
   /* Change board based on its number */
   if (is_number(argument))
   {
       count = 0;
       number = atoi(argument);
       for (i = 0; i < MAX_BOARD; i++)
           if (unread_notes(ch,&boards[i]) != BOARD_NOACCESS)		
               if (++count == number)
                   break;

       if (count == number) /* found the board.. change to it */
       {
           ch->pcdata->board = &boards[i];
           sprintf (buf, "Current board changed to {+%s{x. %s.\n\r",boards[i].short_name,
                   (get_trust(ch) < boards[i].write_level) 
                   ? "You can only read here" 
                   : "You can both read and write here");
           send_to_char (buf,ch);
       }			
       else /* so such board */
           send_to_char ("No such board.\n\r",ch);

       return;
   }
   
   /* Non-number given, find board with that name */
   
   for (i = 0; i < MAX_BOARD; i++)
       /* BC: Using str_prefix() vs. str_cmp() here for easier user input. */
       if (!str_prefix(argument, boards[i].short_name))
           break;

   if (i == MAX_BOARD)
   {
       send_to_char ("No such board.\n\r",ch);
       return;
   }

   /* Does ch have access to this board? */	
   if (unread_notes(ch,&boards[i]) == BOARD_NOACCESS)
   {
       send_to_char ("No such board.\n\r",ch);
       return;
   }

   ch->pcdata->board = &boards[i];
   sprintf (buf, "Current board changed to {+%s{x. %s.\n\r",boards[i].short_name,
           (get_trust(ch) < boards[i].write_level) 
           ? "You can only read here"  
           : "You can both read and write here");
   send_to_char (buf,ch);
}

/* Send a note to someone on the personal board */
void personal_message (const char *sender, const char *to, const char *subject, const int expire_days, const char *text)
{
   make_note ("Personal", sender, to, subject, expire_days, text);
}

void make_note (const char* board_name, const char *sender, const char *to, const char *subject, const int expire_days, const char *text)
{
   int board_index = board_lookup (board_name);
   BOARD_DATA *board;
   NOTE_DATA *note;
   char *strtime;
   
   if (board_index == BOARD_NOTFOUND)
   {
      bug ("make_note: board not found",0);
      return;
   }
   
   if (strlen(text) > MAX_NOTE_TEXT)
   {
      bug ("make_note: text too long (%d bytes)", strlen(text));
      return;
   }
   
   
   board = &boards [board_index];
   
   note = new_note(); /* allocate new note */
   
   note->sender = str_dup (sender);
   note->to_list = str_dup(to);
   note->subject = str_dup (subject);
   note->expire = current_time + expire_days * 60 * 60 * 24;
   note->text = str_dup (text);
   
   /* convert to ascii. ctime returns a string which last character is \n, so remove that */	
   strtime = ctime (&current_time);
   strtime[strlen(strtime)-1] = '\0';
   
   note->date = str_dup (strtime);
   
   finish_note (board, note);
   
}

/* tries to change to the next accessible board */
static bool next_board (CHAR_DATA *ch)
{
   int i = board_number (ch->pcdata->board) + 1;
   
   while ((i < MAX_BOARD) && (unread_notes(ch,&boards[i]) == BOARD_NOACCESS))
      i++;
   
   if (i == MAX_BOARD)
      return FALSE;
   else
   {
      ch->pcdata->board = &boards[i];
      return TRUE;
   }
}

void handle_con_note_to (DESCRIPTOR_DATA *d, const char * argument)
{
   char buf [MAX_INPUT_LENGTH];
   CHAR_DATA *ch = d->character;
   
   if (!ch->pcdata->in_progress)
   {
      d->connected = CON_PLAYING;
      bug ("board: In CON_NOTE_TO, but no note in progress",0);
      return;
   }
   
   strcpy (buf, argument);
   smash_tilde (buf); /* change ~ to - as we save this field as a string later */
   
   switch (ch->pcdata->board->force_type)
   {
   case DEF_NORMAL: /* default field */
      if (!buf[0]) /* empty string? */
      {
         ch->pcdata->in_progress->to_list = str_dup (ch->pcdata->board->names);
         printf_to_char(ch, "Assumed default recipient: {+%s{x\n\r", ch->pcdata->board->names);
      }
      else
         ch->pcdata->in_progress->to_list = str_dup (buf);
      
      break;
      
   case DEF_INCLUDE: /* forced default */
      if (!is_exact_name (ch->pcdata->board->names, buf))
      {
          strcat (buf, " ");
          strcat (buf, ch->pcdata->board->names);
          ch->pcdata->in_progress->to_list = str_dup(buf);

          printf_to_char(ch,"\n\rYou did not specify %s as recipient, so it was automatically added.\n\r"
                  "{YNew To{x:  %s\n\r",
                  ch->pcdata->board->names, ch->pcdata->in_progress->to_list);
      }
      else
          if (!buf[0])
          {
              send_to_char("You must specify a recipient.\n\r"
                      "{YTo{x:      ",ch);
              return;
          }
          else
              ch->pcdata->in_progress->to_list = str_dup (buf);
      break;
         
   case DEF_EXCLUDE: /* forced exclude */
      if (is_exact_name (ch->pcdata->board->names, buf))
      {
          printf_to_char(ch, "You are not allowed to send notes to %s on this board. Try again.\n\r"
                  "{YTo{x:      ", ch->pcdata->board->names);
          return; /* return from nanny, not changing to the next state! */
      }
      else
          if (!buf[0])
          {
              send_to_char("You must specify a recipient.\n\r"
                      "{YTo{x:      ",ch);
              return;
          }
          else
              ch->pcdata->in_progress->to_list = str_dup (buf);
      break;
         
   }		
   
   send_to_char("{Y\n\rSubject{x: ", ch);
   d->connected = CON_NOTE_SUBJECT;
}

void handle_con_note_subject (DESCRIPTOR_DATA *d, const char * argument)
{
   char buf [MAX_INPUT_LENGTH];
   CHAR_DATA *ch = d->character;
   
   if (!ch->pcdata->in_progress)
   {
      d->connected = CON_PLAYING;
      bug ("board: In CON_NOTE_SUBJECT, but no note in progress",0);
      return;
   }
   
   strcpy (buf, argument);
   smash_tilde (buf); /* change ~ to - as we save this field as a string later */
   
   /* Do not allow empty subjects */
   
   if (!buf[0])		
   {
      send_to_char("Please find a meaningful subject!\n\r", ch);
      send_to_char("{YSubject{x: ", ch);
   }
   else  if (strlen(buf)>60)
   {
      send_to_char ("No, no. This is just the Subject. You're not writing the note yet.\n\r", ch);
   }
   else
      /* advance to next stage */
   {
      ch->pcdata->in_progress->subject = str_dup(buf);
      if (IS_IMMORTAL(ch)) /* immortals get to choose number of expire days */
      {
         printf_to_char(ch, "\n\rHow many days do you want this note to expire in?\n\r"
            "Press Enter for default value for this board, {+%d{x days.\n\r"
            "{YExpire{x:  ",
            ch->pcdata->board->purge_days);
         d->connected = CON_NOTE_EXPIRE;
      }
      else
      {
         ch->pcdata->in_progress->expire = 
            current_time + ch->pcdata->board->purge_days * 24L * 3600L;				
         printf_to_char(ch, "This note will expire %s\r",ctime(&ch->pcdata->in_progress->expire));
         send_to_char ("\n\rEnter text. Type {+END{x or {+.q{x to finish, {+.h{x for help.\n\r", ch );
	 send_to_char( note_line, ch );
         
         d->connected = CON_NOTE_TEXT;
      }
   }
}

void handle_con_note_expire(DESCRIPTOR_DATA *d, const char * argument)
{
    CHAR_DATA *ch = d->character;
    char buf[MAX_STRING_LENGTH];
    time_t expire;
    int days;

    if (!ch->pcdata->in_progress)
    {
        d->connected = CON_PLAYING;
        bug ("board: In CON_NOTE_EXPIRE, but no note in progress",0);
        return;
    }

    /* Numeric argument. no tilde smashing */
    strcpy (buf, argument);
    if (!buf[0]) /* assume default expire */
        days = 	ch->pcdata->board->purge_days;
    else /* use this expire */
        if (!is_number(buf))
        {
            send_to_char("Write the number of days!\n\r", ch);
            send_to_char("{YExpire{x:  ", ch);
            return;
        }
        else
        {
            days = atoi (buf);
            if (days <= 0)
            {
                send_to_char("This is a positive MUD. Use positive numbers only! :)\n\r", ch);
                send_to_char("{YExpire{x:  ", ch);
                return;
            }
        }

    expire = current_time + (days*24L*3600L); /* 24 hours, 3600 seconds */

    ch->pcdata->in_progress->expire = expire;

    /* note that ctime returns XXX\n so we only need to add an \r */

    send_to_char ("\n\rEnter text. Type {+END{x, {+@{x, or {+.q{x to finish, {+.h{x for help.\n\r", ch );
    send_to_char( note_line, ch );

    d->connected = CON_NOTE_TEXT;
}

void handle_con_note_text (DESCRIPTOR_DATA *d, const char * argument)
{
    CHAR_DATA *ch = d->character;
    char letter[4*MAX_STRING_LENGTH];
    char xbuf[MAX_NOTE_TEXT];
    char line[MAX_PROTOCOL_BUFFER]; // same size as d->inbuf

    if (!ch->pcdata->in_progress)
    {
        d->connected = CON_PLAYING;
        bug ("board: In CON_NOTE_TEXT, but no note in progress",0);
        return;
    }

    int line_ind=0;
    int buf_ind=0;
    int shift_ind=0;

    for ( buf_ind=0; d->inbuf[buf_ind] != '\0'; )
    {
        /* grab the first line */
        line_ind=0;
        bool noline=FALSE;
        while (TRUE)
        {
            char c=d->inbuf[buf_ind];
            if (c=='\r' || c=='\n')
            {
                /* finish off the line */
                bool got_n = FALSE, got_r=FALSE;

                for (;d->inbuf[buf_ind] == '\r' || d->inbuf[buf_ind] == '\n';buf_ind++)
                {
                    if (d->inbuf[buf_ind] == '\r' && got_r++)
                        break; /*while*/

                    else if (d->inbuf[buf_ind] == '\n' && got_n++)
                        break; /*while*/
                }

                shift_ind=buf_ind;

                line[line_ind]='\0';
                break; /*while*/
            }
            else if (c=='\0')
            {
                noline=TRUE;
                line[line_ind++]='\0';
                buf_ind++;
                break; /*while*/
            }
            else
            {
                line[line_ind++]=c;
                buf_ind++;
            }
        }

        if (noline)
            break; /*for*/

        /* first check for paging */
        if ( d->showstr_point )
        {
            show_string( d, line );
            continue;
        }

        /* does it match any commands? if so, process them */
        /* check for EndOfNote marker */
        if (!str_cmp(line, ".q") || 
                !str_cmp(line, "END") || 
                !str_cmp(line, "@") ||
                !str_cmp(line, "~"))
        {
            printf_to_char (ch, "\n\r\n\r%s\n\r", szFinishPrompt);
            d->connected = CON_NOTE_FINISH;
            break; /*for*/
        }

        smash_tilde (line); /* smash it now */
        /* This bit of code is based on string_add() from Ivan's OLC 1.6. -BC */
        if (line[0] == '.' )
        {
            char arg1 [MAX_INPUT_LENGTH];
            char arg2 [MAX_INPUT_LENGTH];
            char arg3 [MAX_INPUT_LENGTH];
            const char *buffer;

            buffer = one_argument( line, arg1 );
            buffer = first_arg( buffer, arg2, FALSE );
            buffer = first_arg( buffer, arg3, FALSE );

            if ( !str_cmp( arg1, ".h" ) )
            {
                send_to_char("{+Editor functions:{x\n\r"
                        "  {y.h{x - {+Display this help.{x\n\r"
                        "  {y.q{x - {+Stop writing and abort or post note.{x\n\r"
                        "  {y.d{x - {+Delete last line.{x\n\r"
                        "  {y.c{x - {+Clear the note text.{x\n\r"
                        "  {y.s{x - {+Show note so far.{x\n\r"
                        "  {y.r{x - {+Replace first occurrence of 'string1' with 'string2'.{x\n\r"
                        "  {y.p{x - {+Pause - stop writing for a moment.{x\n\r",ch);
                continue;
            }

            if ( !str_cmp( arg1, ".p" ) )
            {
                d->connected = CON_PLAYING;
                send_to_char("{+Pausing note in progress.  Type 'NOTE WRITE' to continue.\n\r{x",ch);
                break; /*for*/
            }

            if ( !str_cmp( arg1, ".r" ) )
            {
                if ( arg2[0] == '\0' )
                {
                    send_to_char("{+Usage{x:  .r \"{rold string{x\" \"{gnew string{x\"\n\r", ch );
                    continue; /*for*/
                }

                if ( !ch->pcdata->in_progress->text )
                {
                    ptc( ch, "You haven't written a thing!\n\r");
                    continue; /*for*/
                }

                ch->pcdata->in_progress->text = string_replace_ext( ch->pcdata->in_progress->text, arg2, arg3,
                        xbuf, MAX_NOTE_TEXT);

                printf_to_char( ch, "'%s' has been replaced with '%s'.\n\r", arg2, arg3 );
                continue; /*for*/
            }

            if ( !str_cmp( arg1, ".s" ) )
            {
                if (ch->pcdata->in_progress->text == NULL)
                {
                    send_to_char("You haven't written a thing!\n\r",ch);
                    continue; /*for*/
                }

                send_to_char ("{gText of your note so far:{x\n\r",ch);
                send_to_char( note_line, ch );

                page_to_char( ch->pcdata->in_progress->text, ch );
                continue; /*for*/
            }

            if ( !str_cmp( arg1, ".d" ) )
            {
                if (ch->pcdata->in_progress->text == NULL)
                {
                    send_to_char("You haven't written a thing!\n\r",ch);
                    continue; /*for*/
                }

                ch->pcdata->in_progress->text = del_last_line_ext(ch->pcdata->in_progress->text, xbuf);
                printf_to_char( ch, "Line deleted.\n\r", arg2, arg3 );
                continue; /*for*/
            }

            if ( !str_cmp( arg1, ".c") )
            {
                if (ch->pcdata->in_progress->text == NULL)
                {
                    send_to_char("You haven't written a thing!\n\r",ch);
                    continue; /*for*/
                }

                free_string(ch->pcdata->in_progress->text);
                ch->pcdata->in_progress->text = NULL;
                ptc( ch, "Note cleared.\n\r");
                continue; /*for*/
            }
        }

        /* Normal text has been received.  Copy to temp buffer, 
           add a new line, and copy back. */
        if (ch->pcdata->in_progress->text)
        {
            strcpy (letter, ch->pcdata->in_progress->text);
            free_string (ch->pcdata->in_progress->text);
            ch->pcdata->in_progress->text = NULL; /* be sure we don't free it twice */
        }
        else
            strcpy (letter, "");

        /* Check for overflow */

        if ((strlen(letter) + strlen (line)) > MAX_NOTE_TEXT)
        { /* Note too long, take appropriate steps */
            send_to_char ("Note too long!\n\r", ch);
            free_note (ch->pcdata->in_progress);
            ch->pcdata->in_progress = NULL;			/* important */
            d->connected = CON_PLAYING;
            return;			
        }

        if (strlen_color(line) > MAX_LINE_LENGTH)
            send_to_char ("Line exceeds 80 characters. It has been wrapped.\n\r",ch);

        /* Add new line to the buffer */   
        strcat (letter, force_wrap(line));
        strcat (letter, "\n\r");

        /* allocate dynamically */		
        ch->pcdata->in_progress->text = str_dup (letter);
    }

    /* we need to shift the d->inbuf appropriately */
    /* similar to read_from_buffer */
    int j;
    for ( j = 0; ( d->inbuf[j] = d->inbuf[shift_ind+j] ) != '\0'; j++ )
        ;
    return;
}

void handle_con_note_finish (DESCRIPTOR_DATA *d, const char * argument)
{
   
   CHAR_DATA *ch = d->character;
   
   if (!ch->pcdata->in_progress)
   {
      d->connected = CON_PLAYING;
      bug ("board: In CON_NOTE_FINISH, but no note in progress",0);
      return;
   }
   
   switch (tolower(argument[0]))
   {
         case 'c': /* keep writing */
	     send_to_char ("Continuing note...\n\r", ch);
	     d->connected = CON_NOTE_TEXT;
	     break;
            
         case 'v': /* view note so far */
            if (ch->pcdata->in_progress->text)
            {
               send_to_char ("{gText of your note so far:{x\n\r",ch);
               send_to_char ( note_line, ch);
               send_to_char (ch->pcdata->in_progress->text, ch);
               printf_to_char(ch, "\n\r\n\r%s\n\r",szFinishPrompt);
            }
            else
               printf_to_char (ch, "You haven't written a thing!\n\r\n\r%s\n\r", szFinishPrompt);
            break;
            
         case 'p': /* post note */
            if (ch->pcdata->in_progress->text)
            {
               finish_note (ch->pcdata->board, ch->pcdata->in_progress);
               mail_notify(ch, ch->pcdata->in_progress, ch->pcdata->board);
               send_to_char ("Note posted.\n\r", ch);
            }
            else
            {
               send_to_char("You haven't written anything.\n\r",ch);
            }
            
            d->connected = CON_PLAYING;
            /* remove AFK status */
            ch->pcdata->in_progress = NULL;
            act ("{G$n finishes $s note.{x" , ch, NULL, NULL, TO_ROOM);
            break;
            
         case 'f':
            send_to_char ("Note cancelled!\n\r", ch);
            free_note (ch->pcdata->in_progress);
            ch->pcdata->in_progress = NULL;
            d->connected = CON_PLAYING;
            /* remove afk status */
            break;
            
         default: /* invalid response */
            printf_to_char (ch, "Huh? Valid answers are:\n\r\n\r%s\n\r", szFinishPrompt);
   }

   if ( d->connected == CON_PLAYING
	&& ch->pcdata->new_tells )
       send_to_char("Type 'playback tell' to see missed tells.\n\r", ch );
}

/* Announces new mail to online recipients.  Toggled off and on just like other
   INFO messages with COMM_NOINFO. -Rimbol */
void mail_notify( CHAR_DATA *ch, NOTE_DATA *pnote, BOARD_DATA *board )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *recip;
   char buf[MAX_STRING_LENGTH];

   sprintf(buf, "{1[INFO]{2: Mail time! %s has sent you a new note on the %s board.{x", ch->name, capitalize(board->short_name));
   for ( d = descriptor_list; d != NULL; d = d->next )
   {
      recip = d->original ? d->original : d->character;
      
      if (!recip)
         continue;

      if (ch == recip)
         continue;

      if (board->read_level > get_trust(recip))
          continue;
      
      if (board->special_access != BOARD_NONE && !IS_IMMORTAL(recip))
      {
          if (board->special_access == BOARD_PKILL && !IS_SET(recip->act, PLR_PERM_PKILL))
              continue;
          if (board->special_access == BOARD_BUILDER && recip->pcdata->security <= 0)
              continue;
      }

	  if ((IS_PLAYING(d->connected) )
      && !NOT_AUTHED(recip)
	  && !IS_SET(recip->comm,COMM_NOINFO)
	  && !IS_SET(recip->comm,COMM_QUIET) )
		 if (is_note_to( recip, pnote ))
         {
			act_new(buf, recip, NULL, NULL, TO_CHAR, POS_SLEEPING );
            if ( recip->pcdata && USE_CHAT_WIN(recip) )
            {
                ptc( recip, "\t<DEST Comm>" );
                act_new(buf, recip, NULL, NULL, TO_CHAR, POS_SLEEPING );
                ptc( recip, "\t</DEST>" );
            }
         }
   }
}
