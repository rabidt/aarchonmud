/*
  Tools to handle buffered simultanious player saves
  by Henning Koehler <koehlerh@in.tum.de>
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "buffer_util.h"
#include "religion.h"


bool boxtemp = FALSE;//track if there are temp box files needing to be moved
/* player files kept in memory
 */
MEMFILE *player_quit_list = NULL;
MEMFILE *player_save_list = NULL;
MEMFILE *box_mf_list     = NULL;
MEMFILE *other_save_list = NULL;

/* states for player saves */
#define SAVE_STATE_SIMSAVE  0
#define SAVE_STATE_TEMPSAVE 1
#define SAVE_STATE_TEMPCOPY 2
#define SAVE_STATE_NOSAVE   3

static int player_save_state = SAVE_STATE_SIMSAVE;
static bool bootup_temp_clean_done = FALSE;
bool ready_to_save( CHAR_DATA *ch );
MEMFILE *memfile_from_list( const char *filename, MEMFILE *list );
void sim_save_to_mem();
void mem_sim_save_other();
void sim_save_other();

/* perform one step in the continious player autosave
 */
void handle_player_save()
{
  MEMFILE *mf;
  MEMFILE *box_mf;
  char command[MSL];
  char buf[MSL];
#if defined(SIM_DEBUG)
  sprintf(command, "handle_player_save: start, state = %d", player_save_state);
  log_string(command);
#endif

  switch (player_save_state)
  {

  case SAVE_STATE_SIMSAVE:
    sim_save_to_mem();
    if (player_save_list != NULL)
    {
      player_save_state = SAVE_STATE_TEMPSAVE;
      /* save other non-player files to memory */
      mem_sim_save_other();
      /* clear temp directory */
      if (!bootup_temp_clean_done)
      {
        sprintf(command, "rm -f %s*", PLAYER_TEMP_DIR);
        if ( system(command) == -1 )
        {
            bugf("handle_player_save: failed to execute command '%s'", command);
            exit(1);
        }
        sprintf(command, "rm -f %s*", BOX_TEMP_DIR);
        if ( system(command) == -1 )
        {
            bugf("handle_player_save: failed to execute command '%s'", command);
            exit(1);
        }
        bootup_temp_clean_done = TRUE;
      }
    }
    break;

  case SAVE_STATE_TEMPSAVE:
    /* char might have deleted => player_save_list is empty */
    if (player_save_list != NULL)
    {
	mf = player_save_list;
	player_save_list = player_save_list->next;
	if (!save_to_dir( mf, PLAYER_TEMP_DIR ))
	{
	    bugf( "handle_player_save: couldn't save %s, exit to avoid corruption",
		  mf->filename );
	    /* we don't want corrupt player files */
	    exit(1);
	}
       
       /* See if corresponding box in box_mf_list */
       sprintf(buf, "%s_box", mf->filename);
       if ( (box_mf = memfile_from_list(buf,box_mf_list)) )
       {
	   boxtemp = TRUE;
           if (!save_to_dir(box_mf, BOX_TEMP_DIR))
           {
               bugf( "handle_player_save: couldn't save %s's box (%s), exit to avoid corruption",
                mf->filename, box_mf->filename );
            /* we don't want corrupt box files */
               exit(1);
           }

           remove_from_box_list(box_mf->filename);
       }
       memfile_free( mf );
    }
    if (player_save_list == NULL )
      player_save_state = SAVE_STATE_TEMPCOPY;
    break;

  case SAVE_STATE_TEMPCOPY:
    sprintf(command, "mv %s* %s", PLAYER_TEMP_DIR, PLAYER_DIR);
    if ( system(command) == -1 )
    {
        bugf("handle_player_save: failed to execute command '%s'", command);
        exit(1);
    }
    if (boxtemp)
    {
      sprintf(command, "mv %s* %s", BOX_TEMP_DIR, BOX_DIR);
      if ( system(command) == -1 )
      {
          bugf("handle_player_save: failed to execute command '%s'", command);
          exit(1);
      }
      boxtemp=FALSE;
    }

    /* save remort etc. files as well */
    sim_save_other();

    player_save_state = SAVE_STATE_SIMSAVE;
    break;
    
  case SAVE_STATE_NOSAVE:
    break;

  default:
    bug("handle_player_save: illegal state %d, resetting", player_save_state);
    player_save_state = SAVE_STATE_SIMSAVE;
  }

#if defined(SIM_DEBUG)
  log_string("handle_player_save: done");
#endif
}

/* do an immediate save for all players
 */
void force_full_save()
{
  bool nosave = (player_save_state == SAVE_STATE_NOSAVE);
#if defined(SIM_DEBUG)
   log_string("force_full_save: start");
#endif
  if (nosave)
    player_save_state = SAVE_STATE_SIMSAVE;
  else
    /* flush pending saves */
    while (player_save_state != SAVE_STATE_SIMSAVE)
      handle_player_save();

  /* do a full save */
  handle_player_save();
  while (player_save_state != SAVE_STATE_SIMSAVE)
    handle_player_save();

  /* reset old state */
  if (nosave)
    player_save_state = SAVE_STATE_NOSAVE;
#if defined(SIM_DEBUG)
   log_string("force_full_save: done");
#endif
}

/* do a final save of all players, then stop all autosaving;
 * used when shutting down the mud
 */
void final_player_save()
{
#if defined(SIM_DEBUG)
   log_string("final_player_save: start");
#endif
  if (player_save_state == SAVE_STATE_NOSAVE)
    return;
  force_full_save();
  /* stop all autosaving */
  player_save_state = SAVE_STATE_NOSAVE;
  /* call any aprog shutdown scripts */
  ap_shutdown_trigger();
#if defined(SIM_DEBUG)
   log_string("final_player_save: done");
#endif
}

/* save all players online to player_save_list and
 * move all files in player_quit_list to player_save_list
 */
void sim_save_to_mem()
{
  MEMFILE *mf;
  //DESCRIPTOR_DATA *d;
  CHAR_DATA *ch;
#if defined(SIM_DEBUG)
   log_string("sim_save_to_mem: start");
#endif

  if (player_save_list != NULL)
  {
    bug("sim_save_to_mem: player_save_list not empty", 0);
    return;
  }
  
  /* move files in player_quit_list to player_save_list */
  player_save_list = player_quit_list;
  player_quit_list = NULL;

  /* save players online */
  //for (d = descriptor_list; d != NULL; d = d->next)
  for ( ch = char_list; ch != NULL; ch = ch->next )
  {
      if ( IS_NPC(ch) || !ready_to_save(ch) )
	  continue;
      /* valid character found, save it */
      mf = mem_save_char_obj( ch );
      if (mf == NULL)
      {
	  bug("sim_save_to_mem: out of memory, save aborted", 0);
	  return;
      }
      
      remove_from_save_list(mf->filename);
      /* add pfile to player_save_list */
      mf->next = player_save_list;
      player_save_list = mf;
  }

#if defined(SIM_DEBUG)
   log_string("sim_save_to_mem: done");
#endif
}

/* return wether a descriptor is ready for a player save 
 * be careful not to skip any valid players, this would open
 * loopholes for duping
 */
bool ready_to_save( CHAR_DATA *ch )
{
    if ( IS_NPC(ch) )
	return FALSE;

    if (ch == NULL)
    {
	bug("ready_to_save: NULL pointer given", 0);
	return FALSE;
    }

    /* player may have quit and is waiting for delayed extraction
     * Note: when last player quits, extraction is delayed until someone logs in
     */
    if ( ch->must_extract )
        return FALSE;
    
    /* chars without desc are playing or note-writing, else they would have been
     * removed at link-closing time
     */
    if ( ch->desc == NULL )
	return TRUE;

    if ( IS_PLAYING(ch->desc->connected) ) 
        return TRUE;
    else
        return FALSE;
}

bool pfile_exists( const char *name )
{
    char filename[MIL];
    FILE *fp;

    if ( name == NULL )
	return FALSE;

    sprintf( filename, "%s%s", PLAYER_DIR, capitalize(name) );
    fp = fopen( filename, "r" );
    if ( fp )
	fclose( fp );
    
    return fp != NULL;
}

/*
 * Load a char and inventory into a new ch structure.
 * the correct file is searched as follows:
 * 1.) player_quit_list
 * 2.) player_save_list
 * 3.) temp player directory
 * 4.) player directory
 */
bool load_char_obj( DESCRIPTOR_DATA *d, const char *name, bool char_only )
{
  MEMFILE *mf;
  DBUFFER *buf;
  FILE *fp;
  char strsave[MAX_INPUT_LENGTH];
  char filename[MAX_INPUT_LENGTH];
  char bug_buf[MSL];
  bool found_in_mem;

#if defined(SIM_DEBUG)
   log_string("load_char_obj: start");
#endif
  strcpy(filename, capitalize(name));
  /* search player_quit_list */
  for (mf = player_quit_list; mf != NULL; mf = mf->next)
    if (!strcmp(mf->filename, filename))
      {
//      log_string("load_char_obj: found in player_quit_list");
        break;
      }

  /* search player_save_list */
  if (mf == NULL)
    for (mf = player_save_list; mf != NULL; mf = mf->next)
      if (!strcmp(mf->filename, filename))
      {
//     log_string("load_char_obj: found in player_save_list");
       break;

      }

  found_in_mem = (mf != NULL);

  /* search temp player directory */
  if (mf == NULL)
  {
#if defined(SIM_DEBUG)
   log_string("load_char_obj: search temp player dir");
#endif
    sprintf( strsave, "%s%s", PLAYER_TEMP_DIR, filename );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
      buf = load_file_to_buffer( fp );
      fclose( fp );
      if (buf == NULL)
      {
	sprintf( bug_buf, "load_char_obj: error loading %s", strsave );
	bug( bug_buf, 0 );
	return FALSE;
      }
      mf = memfile_wrap_buffer( filename, buf );
    }
  }

  /* search player directory */
  if (mf == NULL)
  {
#if defined(SIM_DEBUG)
   log_string("load_char_obj: search player dir");
#endif
    sprintf( strsave, "%s%s", PLAYER_DIR, filename );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
      buf = load_file_to_buffer( fp );
      fclose( fp );
      if (buf == NULL)
      {
	sprintf( bug_buf, "load_char_obj: error loading %s", strsave );
	bug( bug_buf, 0 );
	return FALSE;
      }
      mf = memfile_wrap_buffer( filename, buf );
    }
  }
  
#if defined(SIM_DEBUG)
   log_string("load_char_obj: search done");
#endif
  if (mf == NULL)
  {
    /* load default character */
    mf = memfile_wrap_buffer( filename, NULL );
    mem_load_char_obj( d, mf, char_only );
    memfile_wrap_free( mf );
    return FALSE;
  }

  /* player file found, now try to load player from it */
  mem_load_char_obj( d, mf, char_only );
  if (!found_in_mem)
    memfile_free( mf );
  return TRUE;
}

bool load_storage_boxes(CHAR_DATA *ch )
{
  MEMFILE *mf;
  DBUFFER *buf;
  FILE *fp;
  char strsave[MAX_INPUT_LENGTH];
  char filename[MAX_INPUT_LENGTH];
  char bug_buf[MSL];
  bool found_in_mem;
  sh_int i;

 if (IS_NPC(ch))
 {
     bugf("load_storage_boxes called on NPC",0);
     return FALSE;
 }

  if (ch->pcdata->storage_boxes<1)
    return FALSE;

  send_to_char("As you enter, an employee brings in your boxes and sets them before you.\n\r",ch);
  for (i=1;i<=ch->pcdata->storage_boxes;i++)
  {
      ch->pcdata->box_data[i-1] = create_object_vnum(OBJ_VNUM_STORAGE_BOX);
      obj_to_room( ch->pcdata->box_data[i-1], ch->in_room);
  }
  

#if defined(SIM_DEBUG)
   log_string("load_storage_box: start");
#endif
  sprintf(filename, "%s_box", capitalize(ch->name));

  mf=memfile_from_list(filename, box_mf_list);

  found_in_mem = (mf != NULL);

  /* search temp player directory */
  if (mf == NULL)
  {
#if defined(SIM_DEBUG)
   log_string("load_storager_box: search temp player dir");
#endif
    sprintf( strsave, "%s%s", BOX_TEMP_DIR, filename );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
      buf = load_file_to_buffer( fp );
      fclose( fp );
      if (buf == NULL)
      {
        sprintf( bug_buf, "load_storage_box: error loading %s", strsave );
		        bug( bug_buf, 0 );
        return FALSE;
      }
      mf = memfile_wrap_buffer( filename, buf );
    }
  }

  /* search player directory */
  if (mf == NULL)
  {
#if defined(SIM_DEBUG)
   log_string("load_storage_box: search player dir");
#endif
    sprintf( strsave, "%s%s", BOX_DIR, filename );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
      buf = load_file_to_buffer( fp );
      fclose( fp );
      if (buf == NULL)
      {
        sprintf( bug_buf, "load_storage_box: error loading %s", strsave );
        bug( bug_buf, 0 );
        return FALSE;
      }
      mf = memfile_wrap_buffer( filename, buf );
    }
  }

#if defined(SIM_DEBUG)
   log_string("load_storage_box: search done");
#endif
  if (mf == NULL)
  {
    /* load default character */
    mf = memfile_wrap_buffer( filename, NULL );
//    mem_load_storage_box( ch, mf );
    bug("no storage box found in memory or file",0);
    memfile_wrap_free( mf );
    return FALSE;
  }
  
  /* box file found, now try to load player from it */
  mem_load_storage_box( ch, mf );
  if (!found_in_mem)
    memfile_free( mf );
  return TRUE;
}

void unload_storage_boxes( CHAR_DATA *ch)
{
        sh_int i;

        for (i=0;i<ch->pcdata->storage_boxes;i++)
        {
            extract_obj(ch->pcdata->box_data[i]);
            ch->pcdata->box_data[i]=NULL;
        }
}


/* saves a character to player_quit_list 
 */
void quit_save_char_obj( CHAR_DATA *ch )
{
  MEMFILE *mf;
#if defined(SIM_DEBUG)
   log_string("quit_save_char_obj: start");
#endif
  mf = mem_save_char_obj( ch );
  if (mf == NULL)
  {
    bug("quit_save_char_obj: out of memory", 0);
    return;
  }
  /* if already in player_quit_list, remove old entry */
  /* can happen to ploaded chars */
  remove_from_quit_list( ch->name );
  /* add to list */
  mf->next = player_quit_list;
  player_quit_list = mf;
  
#if defined(SIM_DEBUG)
   log_string("quit_save_char_obj: done");
#endif
}
/*
 * removes a file from a list;
 * returns wether file was found and removed
 */
bool remove_from_list( const char *name, MEMFILE **list )
{
  MEMFILE *mf, *last_mf;
  char filename[MAX_INPUT_LENGTH];
#if defined(SIM_DEBUG)
  char log_buf[MSL];
  sprintf(log_buf, "remove_from_list: start (%s)", name);
  log_string(log_buf);
#endif

  if (name == NULL || name[0] == '\0')
  {
    bug("remove_from_list: invalid name given", 0);
    return FALSE;
  }
  if ( (*list) == NULL)
    return FALSE;

  strcpy(filename, capitalize(name));

  /* special case if first in list */
  last_mf = (*list);
  if (!strcmp(last_mf->filename, filename))
  {
    (*list) = last_mf->next;
    memfile_free(last_mf);
    return TRUE;
  }

  mf = last_mf->next;
  while (mf != NULL)
  {
    if (!strcmp(mf->filename, filename))
    {
      last_mf->next = mf->next;
      memfile_free(mf);
      return TRUE;
    }
    last_mf = mf;
    mf = mf->next;
  }

  return FALSE;
}

/*
 * removes a file from box_mf_list;
 * returns wether file was found and removed
 */
bool remove_from_box_list( const char *name )
{
#if defined(SIM_DEBUG)
  char log_buf[MSL];
  sprintf(log_buf, "remove_from_box_list: start (%s)", name);
  log_string(log_buf);
#endif
  return remove_from_list( name, &box_mf_list );
}

/*
 * removes a file from the player_quit_list;
 * returns wether file was found and removed
 */

bool remove_from_quit_list( const char *name )
{
#if defined(SIM_DEBUG)
  char log_buf[MSL];
  sprintf(log_buf, "remove_from_quit_list: start (%s)", name);
  log_string(log_buf);
#endif
  return remove_from_list( name, &player_quit_list );
}

/*
 * removes a file from the player_save_list;
 * returns wether file was found and removed
 */
bool remove_from_save_list( const char *name )
{
#if defined(SIM_DEBUG)
  char log_buf[MSL];
  sprintf(log_buf, "remove_from_save_list: start (%s)", name);
  log_string(log_buf);
#endif
  return remove_from_list( name, &player_save_list );
}

/* returns wether a file of name <filename> is in <list>
 */
bool memfile_in_list( const char *filename, MEMFILE *list )
{
  MEMFILE *mf;
#if defined(SIM_DEBUG)
   log_string("memfile_in_list: start");
#endif
  if (filename == NULL || filename[0] == '\0')
  {
    bug("memfile_in_list: invalid filename", 0);
    return FALSE;
  }

  for (mf = list; mf != NULL; mf = mf->next)
    if (!strcmp(mf->filename, filename))
      return TRUE;

  return FALSE;
}

MEMFILE *memfile_from_list( const char *filename, MEMFILE *list )
{
   MEMFILE *mf;
#if defined(SIM_DEBUG)
   log_string("memfile_from_list: start");
#endif
  if (filename == NULL || filename[0] == '\0')
  {
    bug("memfile_from_list: invalid filename", 0);
    return FALSE;
  }

  for (mf = list; mf != NULL; mf = mf->next)
    if (!strcmp(mf->filename, filename))
      return mf;

  return NULL;
}

/* save other data to memory for simultanious saving
 */
void mem_sim_save_other()
{
    MEMFILE *mf;
    int i;
#if defined(SIM_DEBUG)
   log_string("mem_sim_save_other: start");
#endif
    /* check if list was cleared properly */
    if ( other_save_list != NULL )
    {
	bug( "mem_sim_save_other: other_save_list != NULL", 0 );
	return;
    }

    /* remort */
    mf = remort_mem_save();
    if ( mf != NULL )
    {
	mf->next = other_save_list;
	other_save_list = mf;
    }

    /* clans */
    for (i = 0; i < MAX_CLAN; i++)
        if (clan_table[i].changed == TRUE)
	{
            mf = mem_save_clan_file(i);
	    if ( mf != NULL )
	    {
		mf->next = other_save_list;
		other_save_list = mf;
	    }
	}

    /* religion */
    mf = save_religions();
    if ( mf != NULL )
    {
	mf->next = other_save_list;
	other_save_list = mf;
    }

    /* leaderboards */
    save_lboards();

    /* changelog */
    save_changelog();

    /* playback */
    save_comm_histories();
   /* mf = save_lboards();
    {
        mf->next = other_save_list;
        other_save_list = mf;
    }*/

    /* leaderboard results */
    /*mf = save_lboard_results();
    {
        mf->next = other_save_list;
        other_save_list = mf;
    }*/

    /* mudconfig */
    save_mudconfig();

#if defined(SIM_DEBUG)
   log_string("mem_sim_save_other: done");
#endif
}

/* save files in other_save_list to disk */
void sim_save_other()
{
    MEMFILE *mf;

#if defined(SIM_DEBUG)
   log_string("sim_save_other: start");
#endif
    while ( other_save_list != NULL )
    {
	mf = other_save_list;
	other_save_list = mf->next;
	save_to_dir( mf, "" );
	memfile_free( mf );
    }
#if defined(SIM_DEBUG)
   log_string("sim_save_other: done");
#endif
}

int unlink_pfile( const char *filename )
{
    char strsave[MIL];
    char buf[MIL];
    int result;

#if defined(SIM_DEBUG)
   log_string("unlink_pfile: start");
#endif
    remove_from_quit_list( filename );
    remove_from_save_list( filename );
    sprintf( buf, "%s_box", filename);
    remove_from_box_list( buf);
    sprintf( strsave, "%s%s", PLAYER_DIR, filename );
    result = unlink(strsave);
    sprintf( strsave, "%s%s", PLAYER_TEMP_DIR, filename );
    unlink(strsave);
    /*Kill boxes when deleting too*/
    sprintf( strsave, "%s%s", BOX_DIR, buf );
    unlink(strsave);
    sprintf( strsave, "%s%s", BOX_TEMP_DIR, buf );
    unlink(strsave);
#if defined(SIM_DEBUG)
   log_string("unlink_pfile: end");
#endif
    return result;
}
