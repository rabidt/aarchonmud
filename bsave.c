/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
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

/***************************************************************************
*   ROM 2.4 is copyright 1993-1996 Russ Taylor             *
*   ROM has been brought to you by the ROM consortium          *
*       Russ Taylor (rtaylor@efn.org)                  *
*       Gabrielle Taylor                           *
*       Brian Moore (zump@rom.org)                     *
*   By using this code, you have agreed to follow the terms of the     *
*   ROM license, in the file Rom24/doc/rom.license             *
***************************************************************************/

/* 
   changed to buffered save by Henning Koehler (aka Bobble)
*/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "buffer_util.h"
#include "simsave.h"
#include "religion.h"
#include "lua_main.h"
#include "lua_arclib.h"

extern  int     _filbuf         args( (FILE *) );

int fingertime;
/*int rename(const char *oldfname, const char *newfname);*/
void mem_save_storage_box( CHAR_DATA *ch );
char *time_format args((time_t, char *));
void save_quest( CHAR_DATA *ch, DBUFFER *buf );

char *print_flags(int flag)
{
    int count, pos = 0;
    static char buf[52];
    
    
    for (count = 0; count < 32;  count++)
    {
        if (I_IS_SET(flag, 1<<count))
        {
            if (count < 26)
                buf[pos] = 'A' + count;
            else
                buf[pos] = 'a' + (count - 26);
            pos++;
        }
    }
    
    if (pos == 0)
    {
        buf[pos] = '0';
        pos++;
    }
    
    buf[pos] = '\0';
    
    return buf;
}

static int count_objects( OBJ_DATA *obj_list )
{
    int count = 0;
    while ( obj_list )
    {
        count += 1 + count_objects(obj_list->contains);
        obj_list = obj_list->next_content;
    }
    return count;
}

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST    100
static  OBJ_DATA *  rgObjNest   [MAX_NEST];

/*
 * Local functions.
 */
void    bwrite_char args( ( CHAR_DATA *ch,  DBUFFER *buf ) );
void    bwrite_obj  args( ( CHAR_DATA *ch,  OBJ_DATA  *obj,
			    DBUFFER *buf, int iNest ) );
void    bwrite_pet  args( ( CHAR_DATA *pet, DBUFFER *buf) );
void    bread_char  args( ( CHAR_DATA *ch,  RBUFFER *buf ) );
void    bread_pet   args( ( CHAR_DATA *ch,  RBUFFER *buf ) );
void    bread_obj   args( ( CHAR_DATA *ch,  RBUFFER *buf, OBJ_DATA *storage_box ) );


#define LOAD_COLOUR( field ) ch->pcdata->field[1] = bread_number( buf );if( ch->pcdata->field[1] == 0 ){ch->pcdata->field[1] = 8;} if( ch->pcdata->field[1] >= 100 ){ch->pcdata->field[1] -= 100;ch->pcdata->field[2] = 1;}else{ch->pcdata->field[2] = 0;}if ( ch->pcdata->field[1] >= 10 ){ch->pcdata->field[1] -= 10;ch->pcdata->field[0] = 1;}else{ch->pcdata->field[0] = 0;}


/* version number to keep track with changes in pfile format or pfile age 
 * (e.g. for moneywipe) --Bobble */
#define CURR_PFILE_VERSION 3 
static int pfile_version = 0;

#define VER_FLAG_CHANGE  1
#define VER_MONEY_WIPE   2
#define VER_EXP_RAISE    3

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
MEMFILE* mem_save_char_obj( CHAR_DATA *ch )
{
    char strsave[MAX_INPUT_LENGTH];
    MEMFILE *mf;
#if defined(SIM_DEBUG)
   log_string("mem_save_char_obj: start");
#endif
    
    if ( ch->desc != NULL && ch->desc->original != NULL )
        ch = ch->desc->original;

    if ( IS_NPC(ch) )
        return NULL;    
    
#if defined(unix)
    /* create god log */
    if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL)
    {
        FILE *fp;
        sprintf(strsave, "%s%s",GOD_DIR, capitalize(ch->name));
        if ((fp = fopen(strsave,"w")) == NULL)
        {
            bug("mem_save_char_obj: fopen",0);
            log_error(strsave);
        }
        else
	{
	    fprintf(fp,"Lev %2d Trust %2d  %s%s\n",
		    ch->level, get_trust(ch), ch->name, ch->pcdata->title);
	    fclose( fp );
	}
    }
#endif
    
    /* alloc memory file */
    strcpy(strsave, capitalize(ch->name));
    /* 16k should do for most players; 
       if not, the buffer will expand automatically */
    mf = memfile_new(strsave, 16*1024);
    if (mf == NULL)
    {
      char msg[MSL];
      sprintf(msg, "mem_save_char_obj: couldn't open memory file for %s", strsave);
      bug(msg, 0);
      return NULL;
    }

    /* record obj count to track vanishing eq bug */
    if ( ch->carrying == NULL )
        logpf("mem_save_char_obj: %s carries no objects", ch->name);

    /* now save to memory file */
    bprintf( mf->buf, "#VER %d\n", CURR_PFILE_VERSION );
    bwrite_char( ch, mf->buf );
    if ( ch->carrying != NULL )
      bwrite_obj( ch, ch->carrying, mf->buf, 0 );

    /* a little safety in case crash or copyover while smithing */
    if ( ch->pcdata->smith )
        bwrite_obj( ch, ch->pcdata->smith->old_obj, mf->buf, 0 );

    /* save the pets */
    if ( ch->pet != NULL )
    {
        CHAR_DATA *pet = ch->pet;
        bwrite_pet(pet, mf->buf);
        if ( pet->carrying != NULL )
            bwrite_obj(pet, pet->carrying, mf->buf, 0);
    }
    bprintf( mf->buf, "#END\n" );

    /* mem_save_storage_box will now make the storage_box mf and add it to
      box_mf_list*/
    mem_save_storage_box(ch);

    /* check for overflow */
    if (mf->buf->overflowed)
    {
      memfile_free(mf);
      return NULL;
    }

#if defined(SIM_DEBUG)
   log_string("mem_save_char_obj: done");
#endif
    return mf;
}

/* Saves storage box for char if any and adds to box_mf_list */
void mem_save_storage_box( CHAR_DATA *ch )
{
    char strsave[MAX_INPUT_LENGTH];
    MEMFILE *mf;
    sh_int i;
#if defined(SIM_DEBUG)
   log_string("mem_save_storage_box: start");
#endif

    if ( IS_NPC(ch) )
        return;


/* If they don't have any storage boxes at all or none are loaded
   then don't need to save.
   Old box mfs will be on box_mf_list */

    if (ch->pcdata->storage_boxes<1 || ch->pcdata->box_data[0] == NULL)
    {
           return;
    }
    /* alloc memory file */
    sprintf(strsave,"%s_box", capitalize(ch->name));
    /* 16k should do for most players;
       if not, the buffer will expand automatically */
    mf = memfile_new(strsave, 16*1024);
    if (mf == NULL)
    {
      char msg[MSL];
      sprintf(msg, "mem_save_storage_box: couldn't open memory file for %s", strsave);
      bug(msg, 0);
      return;
    }

/*  loop from 1 to ch->pcdata->storage_boxes
    write identifying stuff for box
*/
    for (i=1;i<=ch->pcdata->storage_boxes;i++)
    {
        bprintf( mf->buf, "#BOX %d\n", i );
	if (ch->pcdata->box_data[i-1]->contains != NULL)
          bwrite_obj(ch,ch->pcdata->box_data[i-1]->contains, mf->buf, 0);
    }
    bprintf( mf->buf, "#END\n" );

    /* check for overflow */
    if (mf->buf->overflowed)
    {
      memfile_free(mf);
      return;
    }

#if defined(SIM_DEBUG)
   log_string("mem_save_storage_box: done");
#endif
    remove_from_box_list(mf->filename);
    mf->next = box_mf_list;
    box_mf_list = mf;

} 

/*
 * Write the char.
 */
void bwrite_char( CHAR_DATA *ch, DBUFFER *buf )
{
    AFFECT_DATA *paf;
    int sn, gn, pos, i;
    tflag saveact;
    GRANT_DATA *gran;

    
    flag_copy( saveact, ch->act );
    REMOVE_BIT(saveact, PLR_QUESTOR);
    REMOVE_BIT(saveact, PLR_QUESTORHARD);
    REMOVE_BIT(saveact, PLR_WAR);
    
    bprintf( buf, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER" );
    
    bprintf( buf, "Name %s~\n",  ch->name        );

    bprintf( buf, "Id   %ld\n", ch->id           );

    bprintf( buf, "LogO %ld\n",  current_time        );
    
    bprintf( buf, "LastH %s~\n", ch->pcdata->last_host );
    
    bprintf( buf, "Vers %d\n",   5           );
    
    if (ch->short_descr[0] != '\0')
        bprintf( buf, "ShD  %s~\n",  ch->short_descr );
    
    if( ch->long_descr[0] != '\0')
        bprintf( buf, "LnD  %s~\n",  ch->long_descr  );
    
    if (ch->description[0] != '\0')
        bprintf( buf, "Desc %s~\n",  ch->description );
    
    if (ch->prompt != NULL || !str_cmp(ch->prompt, PROMPT_DEFAULT))
        bprintf( buf, "Prom %s~\n",      ch->prompt      );
    
    bprintf( buf, "Race %s~\n", pc_race_table[ch->race].name );

    if ( ch->pcdata->morph_race > 0 )
	bprintf( buf, "Morph %s~ %d\n",
		 pc_race_table[ch->pcdata->morph_race].name,
		 ch->pcdata->morph_time );
    
    if (ch->clan)
    {
        bprintf( buf, "Clan %s~\n",clan_table[ch->clan].name);
        bprintf( buf, "CRank %s~\n",clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].name);
    }

    for (i = 1; i < MAX_CLAN; i++)
        if (ch->pcdata->invitation[i] != NULL)
            bprintf(buf, "Invite %s %s~\n", clan_table[i].name, ch->pcdata->invitation[i]);
    

    bprintf( buf, "Sex  %d\n",   ch->sex         );
    
    bprintf( buf, "Cla  %d\n",   ch->class       );
    
    bprintf( buf, "Levl %d\n",   ch->level       );
    
    if (ch->trust != 0)
        bprintf( buf, "Tru  %d\n",   ch->trust   );
    
    bprintf( buf, "Sec  %d\n",    ch->pcdata->security  );  /* OLC */
    
    bprintf( buf, "Plyd %d\n",
        ch->played + (int) (current_time - ch->logon)   );

    //bprintf( buf, "Pray %d\n", ch->pcdata->prayed_at );
    if ( has_god(ch) )
        bprintf(buf, "God  %s~\n", get_god_name(ch));
    if ( ch->pcdata->faith > 0 )
        bprintf(buf, "Faith %d\n", ch->pcdata->faith);
    if ( ch->pcdata->religion_rank > 0 )
        bprintf(buf, "RRank %d\n", ch->pcdata->religion_rank);
    
    bprintf( buf, "Scro %d\n",   ch->lines       );
    
    bprintf( buf, "Room %d\n",
	( ch->in_room == get_room_index( ROOM_VNUM_LIMBO ) && ch->was_in_room != NULL )
        ? ch->was_in_room->vnum
        : (ch->in_room == NULL || IS_TAG(ch))
        ? ROOM_VNUM_TEMPLE : ch->in_room->vnum );
    
    bprintf( buf, "HMV  %d %d %d %d %d %d\n",
        ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
    
    if (ch->gold > 0)
        bprintf( buf, "Gold %ld\n",    ch->gold        );
    else
        bprintf( buf, "Gold %d\n", 0           ); 
    
    if (ch->silver > 0)
        bprintf( buf, "Silv %ld\n",ch->silver        );
    else
        bprintf( buf, "Silv %d\n",0          );
    
    bprintf( buf, "Bnty %d\n",   ch->pcdata->bounty  );
    
    bprintf( buf, "Demerit %d\n", ch->pcdata->demerit_points);
    
    /* save crime list */
    if ( ch->pcdata->crimes != NULL )
    {
	CRIME_DATA *crime;
	for ( crime = ch->pcdata->crimes; crime != NULL; crime = crime->next )
	    bprintf( buf, "Crime %s~ %s~ %ld %d\n",
		     crime->name,
		     crime->imm_name,
		     crime->timestamp,
		     crime->forgive ? 1 : 0 );
    }

    if ( ch->pcdata->authed_by )
        bprintf( buf, "AuthedBy %s~\n",	ch->pcdata->authed_by);
    
    if ( ch->pcdata->spouse )
        bprintf(buf, "Spouse %s~\n", ch->pcdata->spouse);
    
    bprintf( buf, "Exp  %d\n",   ch->exp         );
    
    if ( !flag_is_empty(ch->act) )
        bprintf( buf, "Act  %s\n",   print_tflag(saveact));
    
    /* no longer needed --Bobble
    if ( !flag_is_empty(ch->affect_field) )
	bprintf( buf, "AfBy %s\n", print_tflag(ch->affect_field) );
    */
    
    bprintf( buf, "Comm %s\n",       print_tflag(ch->comm));
    
    if ( !flag_is_empty(ch->wiznet) )
        bprintf( buf, "Wizn %s\n",   print_tflag(ch->wiznet));

    if ( !flag_is_empty(ch->gag) )
	bprintf( buf, "Gag  %s\n", print_tflag(ch->gag));
    
    if (ch->invis_level)
        bprintf( buf, "Invi %d\n",   ch->invis_level );
    
    if (ch->incog_level)
        bprintf(buf,"Inco %d\n",ch->incog_level);
    
    bprintf( buf, "Pos  %d\n",   
        ch->position == POS_FIGHTING ? POS_STANDING : ch->position );
    
    if (ch->practice != 0)
        bprintf( buf, "Prac %d\n",   ch->practice    );
    
    if (ch->train != 0)
        bprintf( buf, "Trai %d\n",   ch->train   );
    
    bprintf( buf, "Field %d\n",  ch->pcdata->field);
    
    if (ch->saving_throw != 0)
        bprintf( buf, "Save  %d\n",  ch->saving_throw);
    
    bprintf( buf, "Alig  %d\n",  ch->alignment       );
    
    if (ch->hitroll != 0)
        bprintf( buf, "Hit   %d\n",  ch->hitroll );
    
    if (ch->damroll != 0)
        bprintf( buf, "Dam   %d\n",  ch->damroll );
    
    bprintf( buf, "AC %d\n",   
        ch->armor);
    
    if (ch->wimpy !=0 )
        bprintf( buf, "Wimp  %d\n",  ch->wimpy   );
    
    if ( ch->calm != 0 )
        bprintf( buf, "Calm  %d\n",  ch->calm );

    bprintf( buf, "NewAttr %d %d %d %d %d %d %d %d %d %d\n",
        ch->perm_stat[STAT_STR],
        ch->perm_stat[STAT_CON],
        ch->perm_stat[STAT_VIT],
        ch->perm_stat[STAT_AGI],
        ch->perm_stat[STAT_DEX],
        ch->perm_stat[STAT_INT],
        ch->perm_stat[STAT_WIS],
        ch->perm_stat[STAT_DIS],
        ch->perm_stat[STAT_CHA],
        ch->perm_stat[STAT_LUC] );
    
    bprintf (buf, "NewAMod %d %d %d %d %d %d %d %d %d %d\n",
        ch->mod_stat[STAT_STR],
        ch->mod_stat[STAT_CON],
        ch->mod_stat[STAT_VIT],
        ch->mod_stat[STAT_AGI],
        ch->mod_stat[STAT_DEX],
        ch->mod_stat[STAT_INT],
        ch->mod_stat[STAT_WIS],
        ch->mod_stat[STAT_DIS],
        ch->mod_stat[STAT_CHA],
        ch->mod_stat[STAT_LUC] );
    
    bprintf (buf, "AOrg %d %d %d %d %d %d %d %d %d %d\n",
        ch->pcdata->original_stats[STAT_STR],
        ch->pcdata->original_stats[STAT_CON],
        ch->pcdata->original_stats[STAT_VIT],
        ch->pcdata->original_stats[STAT_AGI],
        ch->pcdata->original_stats[STAT_DEX],
        ch->pcdata->original_stats[STAT_INT],
        ch->pcdata->original_stats[STAT_WIS],
        ch->pcdata->original_stats[STAT_DIS],
        ch->pcdata->original_stats[STAT_CHA],
        ch->pcdata->original_stats[STAT_LUC] );
    
    bprintf (buf, "AHis %d %d %d %d %d %d %d %d %d %d\n",
        ch->pcdata->history_stats[STAT_STR],
        ch->pcdata->history_stats[STAT_CON],
        ch->pcdata->history_stats[STAT_VIT],
        ch->pcdata->history_stats[STAT_AGI],
        ch->pcdata->history_stats[STAT_DEX],
        ch->pcdata->history_stats[STAT_INT],
        ch->pcdata->history_stats[STAT_WIS],
        ch->pcdata->history_stats[STAT_DIS],
        ch->pcdata->history_stats[STAT_CHA],
        ch->pcdata->history_stats[STAT_LUC] );
    
    if ( IS_NPC(ch) )
    {
        bprintf( buf, "Vnum %d\n",   ch->pIndexData->vnum    );
    }
    else
    {
        bprintf( buf, "Pass %s~\n",  ch->pcdata->pwd     );
    
	bprintf( buf, "PKPoints %d\n",   ch->pcdata->pkpoints );
        
        bprintf( buf, "PKCount %d\n",    ch->pcdata->pkill_count );
	bprintf( buf, "PKExpire %ld\n",  ch->pcdata->pkill_expire);

        bprintf( buf, "Remort %d\n",  ch->pcdata->remorts);
        bprintf( buf, "Ascent %d\n",  ch->pcdata->ascents);
        if ( ch->pcdata->subclass )
            bprintf( buf, "Subclass %s~\n", subclass_table[ch->pcdata->subclass].name );
        
        if (ch->pcdata->bamfin[0] != '\0')
            bprintf( buf, "Bin  %s~\n",  ch->pcdata->bamfin);
        
        if (ch->pcdata->bamfout[0] != '\0')
            bprintf( buf, "Bout %s~\n",  ch->pcdata->bamfout);
        
        if (ch->pcdata->customflag[0] != '\0')
        {
            bprintf( buf, "CFlag %s~\n", ch->pcdata->customflag);
            bprintf( buf, "CDur %d\n", ch->pcdata->customduration);
        }
	bprintf( buf, "NmCol %s~\n", ch->pcdata->name_color);
	bprintf( buf, "Pretitle %s~\n", ch->pcdata->pre_title);
        
        // strip single leading ' ' character that gets added automatically by set_title
        bprintf( buf, "Titl %s~\n", ch->pcdata->title[0] == ' ' ? ch->pcdata->title+1 : ch->pcdata->title );
        
        bprintf( buf, "Pnts %d\n",       ch->pcdata->points      );
        
        bprintf( buf, "TSex %d\n",   ch->pcdata->true_sex    );
        
        bprintf( buf, "LLev %d\n",   ch->pcdata->last_level  );

	if (ch->pcdata->highest_level != ch->level)
        bprintf( buf, "HLev %d\n",   ch->pcdata->highest_level  );
        
        if (ch->pcdata->trained_hit)
	    bprintf(buf, "THit %d\n", ch->pcdata->trained_hit);
        
	if (ch->pcdata->trained_mana)
	    bprintf(buf, "TMan %d\n", ch->pcdata->trained_mana);
        
	if (ch->pcdata->trained_move)
	    bprintf(buf, "TMov %d\n", ch->pcdata->trained_move);
        
	bprintf( buf, "Cnd  %d %d %d %d\n",
            ch->pcdata->condition[0],
            ch->pcdata->condition[1],
            ch->pcdata->condition[2],
            ch->pcdata->condition[3] );

    bprintf( buf, "Stance %d\n", ch->stance );

    bprintf( buf, "GuiC %d %d %d\n", ch->pcdata->guiconfig.chat_window,
                                     ch->pcdata->guiconfig.show_images,
                                     ch->pcdata->guiconfig.image_window );

       /*
        * Write Colour Config Information.
        */
        bprintf( buf, "Coloura   %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->gossip[2],
            ch->pcdata->gossip[0],
            ch->pcdata->gossip[1],
            ch->pcdata->auction[2],
            ch->pcdata->auction[0],
            ch->pcdata->auction[1],
            ch->pcdata->music[2],
            ch->pcdata->music[0],
            ch->pcdata->music[1],
            ch->pcdata->question[2],
            ch->pcdata->question[0],
            ch->pcdata->question[1],
            ch->pcdata->answer[2],
            ch->pcdata->answer[0],
            ch->pcdata->answer[1] );
     
        bprintf( buf, "Colourb   %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->quote[2],
            ch->pcdata->quote[0],
            ch->pcdata->quote[1],
            ch->pcdata->gratz[2],
            ch->pcdata->gratz[0],
            ch->pcdata->gratz[1],
            ch->pcdata->immtalk[2],
            ch->pcdata->immtalk[0],
            ch->pcdata->immtalk[1],
            ch->pcdata->shouts[2],
            ch->pcdata->shouts[0],
            ch->pcdata->shouts[1],
            ch->pcdata->tells[2],
            ch->pcdata->tells[0],
            ch->pcdata->tells[1]);
        
        bprintf( buf, "Colourc   %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->info[2],
            ch->pcdata->info[0],
            ch->pcdata->info[1],
            ch->pcdata->gametalk[2],
            ch->pcdata->gametalk[0],
            ch->pcdata->gametalk[1],
            ch->pcdata->bitch[2],
            ch->pcdata->bitch[0],
            ch->pcdata->bitch[1],
            ch->pcdata->newbie[2],
            ch->pcdata->newbie[0],
            ch->pcdata->newbie[1],
            ch->pcdata->clan[2],
            ch->pcdata->clan[0],
            ch->pcdata->clan[1] );
        
        bprintf( buf, "Colourd   %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->say[2],
            ch->pcdata->say[0],
            ch->pcdata->say[1],
            ch->pcdata->gtell[2],
            ch->pcdata->gtell[0],
            ch->pcdata->gtell[1],
            ch->pcdata->room_title[2],
            ch->pcdata->room_title[0],
            ch->pcdata->room_title[1],
            ch->pcdata->room_exits[2],
            ch->pcdata->room_exits[0],
            ch->pcdata->room_exits[1],
            ch->pcdata->wiznet[2],
            ch->pcdata->wiznet[0],
            ch->pcdata->wiznet[1] );
        
        bprintf( buf, "Coloure   %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->gossip_text[2],
            ch->pcdata->gossip_text[0],
            ch->pcdata->gossip_text[1],
            ch->pcdata->auction_text[2],
            ch->pcdata->auction_text[0],
            ch->pcdata->auction_text[1],
            ch->pcdata->music_text[2],
            ch->pcdata->music_text[0],
            ch->pcdata->music_text[1],
            ch->pcdata->question_text[2],
            ch->pcdata->question_text[0],
            ch->pcdata->question_text[1],
            ch->pcdata->answer_text[2],
            ch->pcdata->answer_text[0],
            ch->pcdata->answer_text[1] );
        
        bprintf( buf, "Colourf   %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->quote_text[2],
            ch->pcdata->quote_text[0],
            ch->pcdata->quote_text[1],
            ch->pcdata->gratz_text[2],
            ch->pcdata->gratz_text[0],
            ch->pcdata->gratz_text[1],
            ch->pcdata->immtalk_text[2],
            ch->pcdata->immtalk_text[0],
            ch->pcdata->immtalk_text[1],
            ch->pcdata->shouts_text[2],
            ch->pcdata->shouts_text[0],
            ch->pcdata->shouts_text[1],
            ch->pcdata->tell_text[2],
            ch->pcdata->tell_text[0],
            ch->pcdata->tell_text[1]);
        
        bprintf( buf, "Colourg   %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->info_text[2],
            ch->pcdata->info_text[0],
            ch->pcdata->info_text[1],
            ch->pcdata->gametalk_text[2],
            ch->pcdata->gametalk_text[0],
            ch->pcdata->gametalk_text[1],
            ch->pcdata->bitch_text[2],
            ch->pcdata->bitch_text[0],
            ch->pcdata->bitch_text[1],
            ch->pcdata->newbie_text[2],
            ch->pcdata->newbie_text[0],
            ch->pcdata->newbie_text[1],
            ch->pcdata->clan_text[2],
            ch->pcdata->clan_text[0],
            ch->pcdata->clan_text[1] );
        
        bprintf( buf, "Colourh   %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->say_text[2],
            ch->pcdata->say_text[0],
            ch->pcdata->say_text[1],
            ch->pcdata->gtell_text[2],
            ch->pcdata->gtell_text[0],
            ch->pcdata->gtell_text[1],
            ch->pcdata->warfare[2],
            ch->pcdata->warfare[0],
            ch->pcdata->warfare[1],
            ch->pcdata->warfare_text[2],
            ch->pcdata->warfare_text[0],
            ch->pcdata->warfare_text[1]);

        bprintf( buf, "Colouri   %d%d%d %d%d%d %d%d%d %d%d%d\n",
            ch->pcdata->savantalk[2],
            ch->pcdata->savantalk[0],
            ch->pcdata->savantalk[1],
            ch->pcdata->savantalk_text[2],
            ch->pcdata->savantalk_text[0],
            ch->pcdata->savantalk_text[1],
	    ch->pcdata->proclaim[2],
	    ch->pcdata->proclaim[0],
	    ch->pcdata->proclaim[1],
	    ch->pcdata->proclaim_text[2],
	    ch->pcdata->proclaim_text[0],
	    ch->pcdata->proclaim_text[1]);




        for (gran = ch->pcdata->granted; gran != NULL; gran = gran->next)
            bprintf(buf, "Icmd '%s' %d\n", gran->name, gran->duration);

	if(ch->pcdata->explored->set > 0 )
	{	EXPLORE_HOLDER *pExp;

		bprintf(buf, "ExploredN %d\n", ch->pcdata->explored->set);
		for(pExp = ch->pcdata->explored->buckets ; pExp ; pExp = pExp->next )
			bprintf(buf, "%d %d\n", pExp->mask, pExp->bits );
		bprintf(buf, "-1 -1\n" );
	}

    /* boss achievements */
    struct boss_achieve_record *rec;
    for ( rec = ch->pcdata->boss_achievements ; rec; rec=rec->next )
    {
        bprintf( buf, "BAch %d %d\n", rec->vnum, rec->timestamp );
    }


        /* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (ch->pcdata->alias[pos] == NULL
                ||  ch->pcdata->alias_sub[pos] == NULL)
                break;
            
            bprintf(buf,"Alias %s %s~\n",ch->pcdata->alias[pos],
                ch->pcdata->alias_sub[pos]);
        }
	/* write default command */
	if ( ch->pcdata->combat_action != NULL )
	    bprintf(buf, "Action %s~\n", ch->pcdata->combat_action);

        /* Save note board status */
        /* Save number of boards in case that number changes */
        bprintf (buf, "Boards       %d ", MAX_BOARD);
        
        for (i = 0; i < MAX_BOARD; i++)
            bprintf (buf, "%s %ld ", boards[i].short_name, ch->pcdata->last_note[i]);
        bprintf (buf, "\n");
        
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0 )
            {
                bprintf( buf, "Sk %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn].name );
                if ( ch->pcdata->mastered[sn] > 0 )
                    bprintf( buf, "Ma %d '%s'\n", ch->pcdata->mastered[sn], skill_table[sn].name );
            }
        }
        
        for ( gn = 0; gn < MAX_GROUP; gn++ )
        {
            if ( group_table[gn].name != NULL && ch->pcdata->group_known[gn])
            {
                bprintf( buf, "Gr '%s'\n",group_table[gn].name);
            }
        }
    }
    
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if (paf->type < 0 || paf->type>= MAX_SKILL)
            continue;

        if (paf->type == gsn_custom_affect)
        {
            bprintf( buf, "AffCust %s~ %3d %3d %3d %3d %3d %10d\n",
                    paf->tag,
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector
                   );
        }
        else
        {
            bprintf( buf, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                    skill_table[paf->type].name,
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector
                   );
        }
    }

    if (ch->pcdata->achievements != 0)
        bprintf(buf, "Achv %s\n", print_tflag(ch->pcdata->achievements));

    if (ch->pcdata->achpoints != 0)
        bprintf(buf, "AchPts %d\n",      ch->pcdata->achpoints );

    if (ch->pcdata->questpoints != 0)
        bprintf( buf, "QuestPnts %d\n",  ch->pcdata->questpoints );

    if (ch->pcdata->nextquest != 0)
        bprintf( buf, "QuestNext %d\n",  ch->pcdata->nextquest   );
    else if (ch->pcdata->countdown != 0)
        bprintf( buf, "QuestNext %d\n",  10              );
    
    if (ch->pcdata->bank > 0)    
        bprintf( buf, "Bank %ld\n",    ch->pcdata->bank        );
    else
        bprintf( buf, "Bank %d\n", 0           ); 
    bprintf( buf, "Bhds %d\n",    ch->pcdata->behead_cnt        );
    bprintf( buf, "Boxes %d\n",   ch->pcdata->storage_boxes	);
    
    bprintf( buf, "WarGenoWon %d\n", ch->pcdata->armageddon_won );
    bprintf( buf, "WarGenoLost %d\n", ch->pcdata->armageddon_lost );
    bprintf( buf, "WarGenoKills %d\n", ch->pcdata->armageddon_kills );
    bprintf( buf, "WarClanWon %d\n", ch->pcdata->clan_won );
    bprintf( buf, "WarClanLost %d\n", ch->pcdata->clan_lost );
    bprintf( buf, "WarClanKills %d\n", ch->pcdata->clan_kills );
    bprintf( buf, "WarRaceWon %d\n", ch->pcdata->race_won );
    bprintf( buf, "WarRaceLost %d\n", ch->pcdata->race_lost );
    bprintf( buf, "WarRaceKills %d\n", ch->pcdata->race_kills );
    bprintf( buf, "WarClassWon %d\n", ch->pcdata->class_won );
    bprintf( buf, "WarClassLost %d\n", ch->pcdata->class_lost );
    bprintf( buf, "WarClassKills %d\n", ch->pcdata->class_kills );
    bprintf( buf, "WarPoints %d\n", ch->pcdata->warpoints );
    bprintf( buf, "WarTotalKills %d\n", ch->pcdata->war_kills );
    bprintf( buf, "WarGenderKills %d\n", ch->pcdata->gender_kills );
    bprintf( buf, "WarGenderWon %d\n", ch->pcdata->gender_won );
    bprintf( buf, "WarGenderLost %d\n", ch->pcdata->gender_lost );
    bprintf( buf, "WarReligionKills %d\n", ch->pcdata->religion_kills );
    bprintf( buf, "WarReligionWon %d\n", ch->pcdata->religion_won );
    bprintf( buf, "WarReligionLost %d\n", ch->pcdata->religion_lost );
    bprintf( buf, "WarDuelKills %d\n", ch->pcdata->duel_kills );
    bprintf( buf, "WarDuelWon %d\n", ch->pcdata->duel_won );
    bprintf( buf, "WarDuelLost %d\n", ch->pcdata->duel_lost );
    bprintf( buf, "MobKills %d\n", ch->pcdata->mob_kills );
    bprintf( buf, "MobDeaths %d\n", ch->pcdata->mob_deaths );
    bprintf( buf, "QuestsFailed %d\n", ch->pcdata->quest_failed );
    bprintf( buf, "QuestsSuccess %d\n", ch->pcdata->quest_success );
    bprintf( buf, "QuestsHardSuccess %d\n", ch->pcdata->quest_hard_success );
    bprintf( buf, "QuestsHardFailed %d\n", ch->pcdata->quest_hard_failed );
    bprintf( buf, "PkillDeaths %d\n", ch->pcdata->pkill_deaths );

    save_quest( ch, buf );

    if ( !is_tattoo_list_empty(ch->pcdata->tattoos) )
	bprintf( buf, "Tattoos %s\n", print_tattoos(ch->pcdata->tattoos) );

    bprintf( buf, "Smc %d %d %d\n", ch->pcdata->smc_mastered, ch->pcdata->smc_grandmastered, ch->pcdata->smc_retrained );

    LUA_EXTRA_VAL *luaval;
    for ( luaval=ch->luavals ; luaval; luaval=luaval->next )
    {
        if (!luaval->persist)
            continue;
        bprintf( buf, "LuaVal %d %s~ %s~\n",
                luaval->type,
                luaval->name,
                luaval->val);
    }
    
    const char *luaconfig=save_luaconfig( ch );
    if (luaconfig)
    {
        bprintf( buf, "LuaCfg %s~\n", luaconfig );
    }

    const char *ptitles=save_ptitles( ch );
    if (ptitles)
    {
        bprintf( buf, "Ptitles %s~\n", ptitles );
        free_string(ptitles);
    }
    
    
    bprintf( buf, "End\n\n" );
    return;
}

/* save a player's quest status */
void save_quest( CHAR_DATA *ch, DBUFFER *buf )
{
    QUEST_DATA *qdata;

    if ( ch == NULL )
    {
	bug( "save_quest: NULL character given", 0 );
	return;
    }

    if ( ch->pcdata == NULL )
	return;

/* This information is written to the pfile. bwrite_char actually calls
this function instead of doing it all manually. Makes sense because a loop
is needed to ensure all qsets are written. There is no need to write the
limit on the timer into the pfile because we are just going to set the timer
to the correct number. The limit will start counting again when the player is
online. */ 

    for ( qdata = ch->pcdata->qdata; qdata != NULL; qdata = qdata->next )
	bprintf( buf, "QStat %d %d %d\n", qdata->id, qdata->status, qdata->timer );
}

/* write a pet */
void bwrite_pet( CHAR_DATA *pet, DBUFFER *buf)
{
    AFFECT_DATA *paf;
    
    bprintf(buf,"#PET\n");
    
    bprintf(buf,"Vnum %d\n",pet->pIndexData->vnum);
    
    bprintf(buf,"Name %s~\n", pet->name);
    bprintf(buf,"LogO %ld\n", current_time);
    if (pet->short_descr != pet->pIndexData->short_descr)
        bprintf(buf,"ShD  %s~\n", pet->short_descr);
    if (pet->long_descr != pet->pIndexData->long_descr)
        bprintf(buf,"LnD  %s~\n", pet->long_descr);
    if (pet->description != pet->pIndexData->description)
        bprintf(buf,"Desc %s~\n", pet->description);
    if (pet->race != pet->pIndexData->race)
        bprintf(buf,"Race %s~\n", race_table[pet->race].name);
    if (pet->clan)
        bprintf( buf, "Clan %s~\n",clan_table[pet->clan].name);
    bprintf(buf,"Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
        bprintf(buf,"Levl %d\n", pet->level);
    bprintf(buf, "HMV  %d %d %d %d %d %d\n",
        pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);
    if (pet->gold > 0)
        bprintf(buf,"Gold %ld\n",pet->gold);
    if (pet->silver > 0)
        bprintf(buf,"Silv %ld\n",pet->silver);
    if (pet->exp > 0)
        bprintf(buf, "Exp  %d\n", pet->exp);
    if ( !flag_equal(pet->act, pet->pIndexData->act) )
        bprintf(buf, "Act  %s\n", print_tflag(pet->act));
    if ( !flag_equal(pet->affect_field, pet->pIndexData->affect_field) )
        bprintf(buf, "AfBy %s\n", print_tflag(pet->affect_field));
    if (pet->comm != 0)
        bprintf(buf, "Comm %s\n", print_tflag(pet->comm));
    bprintf(buf,"Pos  %d\n",
	    pet->position == POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
        bprintf(buf, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
        bprintf(buf, "Alig %d\n", pet->alignment);
    bprintf( buf, "Hit  %d\n", pet->hitroll );
    bprintf( buf, "DamDice %d %d\n", pet->damage[DICE_NUMBER], pet->damage[DICE_TYPE] );
    bprintf(buf, "Dam  %d\n", pet->damroll);
    bprintf(buf, "AC %d\n",
        pet->armor);
    bprintf( buf, "Attr %d %d %d %d %d %d %d %d %d %d\n",
        pet->perm_stat[STAT_STR],
        pet->perm_stat[STAT_CON],
        pet->perm_stat[STAT_VIT],
        pet->perm_stat[STAT_AGI],
        pet->perm_stat[STAT_DEX],
        pet->perm_stat[STAT_INT],
        pet->perm_stat[STAT_WIS],
        pet->perm_stat[STAT_DIS],
        pet->perm_stat[STAT_CHA],
        pet->perm_stat[STAT_LUC] );
    
    bprintf (buf, "AMod %d %d %d %d %d %d %d %d %d %d\n",
        pet->mod_stat[STAT_STR],
        pet->mod_stat[STAT_CON],
        pet->mod_stat[STAT_VIT],
        pet->mod_stat[STAT_AGI],
        pet->mod_stat[STAT_DEX],
        pet->mod_stat[STAT_INT],
        pet->mod_stat[STAT_WIS],
        pet->mod_stat[STAT_DIS],
        pet->mod_stat[STAT_CHA],
        pet->mod_stat[STAT_LUC] );
    
    for ( paf = pet->affected; paf != NULL; paf = paf->next )
    {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;

        if (paf->type == gsn_custom_affect)
        {
            bprintf(buf, "AffCust %s~ %3d %3d %3d %3d %10d\n",
                    paf->tag,
                    paf->where, paf->level, paf->duration, paf->modifier,
                    paf->location, paf->bitvector);
            continue;
        }
        /* don't save permanent affects - they will be added automatically */
        if ( paf->duration == -1 )
            continue;

        bprintf(buf, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                skill_table[paf->type].name,
                paf->where, paf->level, paf->duration, paf->modifier,paf->location,
                paf->bitvector);
    }
    
    bprintf(buf,"End\n");
    return;
}

/*
 * Write an object and its contents.
 */
void bwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, DBUFFER *buf, int iNest )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
    
   /*
    * Slick recursion to write lists backwards,
    *   so loading them will load in forwards order.
    */
    if ( obj->next_content != NULL )
        bwrite_obj( ch, obj->next_content, buf, iNest );
    
   /*
    * Castrate storage characters.
    */
    if ( is_drop_obj(obj)
	 || (obj->item_type == ITEM_KEY && !is_remort_obj(obj))
	 || obj->item_type == ITEM_TRASH )
        return;
    
    bprintf( buf, "#O\n" );
    bprintf( buf, "Vnum %d\n",   obj->pIndexData->vnum        );
    if (obj->owner)
        bprintf(buf, "Owner %s~\n", obj->owner);
    bprintf( buf, "Nest %d\n",   iNest            );
    
    /* these data are only used if they do not match the defaults */

    /* Bobble: only save name & description to allow restring but avoid cheating
     * extra flags can be legally changed, thus must be saved
     * condition & durability aren't used anyway(?!)
     */
    
    if ( obj->name != obj->pIndexData->name)
        bprintf( buf, "Name %s~\n",  obj->name            );
    if ( obj->short_descr != obj->pIndexData->short_descr)
        bprintf( buf, "ShD  %s~\n",  obj->short_descr         );
    if ( obj->description != obj->pIndexData->description)
        bprintf( buf, "Desc %s~\n",  obj->description         );
    if ( !flag_equal(obj->extra_flags, obj->pIndexData->extra_flags) )
        bprintf( buf, "ExtF %s\n",   print_tflag(obj->extra_flags) );
    if ( obj->material != obj->pIndexData->material)
        bprintf( buf, "Mat %s~\n",   obj->material         );

    /*
    if ( obj->durability != obj->pIndexData->durability)
        bprintf( buf, "Dur  %d\n",   obj->durability );
    if ( obj->clan != obj->pIndexData->clan)
        bprintf( buf, "Clan %s~\n",clan_table[obj->clan].name);
    if ( obj->rank != obj->pIndexData->rank)
        bprintf( buf, "CRank %s~\n",clan_table[obj->clan].rank_list[obj->rank].name);
    if ( obj->condition != obj->pIndexData->condition)
        bprintf( buf, "Cond %d\n",   obj->condition           );
    */
    
    /* variable data */
    
    bprintf( buf, "Wear %d\n",   obj->wear_loc                );
    if (obj->level != obj->pIndexData->level)
        bprintf( buf, "Lev  %d\n",   obj->level           );
    if (obj->timer != 0)
        bprintf( buf, "Time %d\n",   obj->timer       );
    /*
    bprintf( buf, "Cost %d\n",   obj->cost            );
    */
    if (obj->value[0] != obj->pIndexData->value[0]
        ||  obj->value[1] != obj->pIndexData->value[1]
        ||  obj->value[2] != obj->pIndexData->value[2]
        ||  obj->value[3] != obj->pIndexData->value[3]
        ||  obj->value[4] != obj->pIndexData->value[4]) 
        bprintf( buf, "Val  %d %d %d %d %d\n",
        obj->value[0], obj->value[1], obj->value[2], obj->value[3],
        obj->value[4]        );
    
    switch ( obj->item_type )
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL:
        if ( obj->value[1] > 0 )
        {
            bprintf( buf, "Spell 1 '%s'\n", 
                skill_table[obj->value[1]].name );
        }
        
        if ( obj->value[2] > 0 )
        {
            bprintf( buf, "Spell 2 '%s'\n", 
                skill_table[obj->value[2]].name );
        }
        
        if ( obj->value[3] > 0 )
        {
            bprintf( buf, "Spell 3 '%s'\n", 
                skill_table[obj->value[3]].name );
        }
        
        break;
        
    case ITEM_STAFF:
    case ITEM_WAND:
        if ( obj->value[3] > 0 )
        {
            bprintf( buf, "Spell 3 '%s'\n", 
                skill_table[obj->value[3]].name );
        }
        
        break;
    }
    
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;

        if (paf->type == gsn_custom_affect)
        {
            bprintf( buf, "AffCust %s~ %3d %3d %3d %3d %3d %10d\n",
                    paf->tag,
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector
                   );
        }
        else
        {
            bprintf( buf, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                    skill_table[paf->type].name,
                    paf->where,
                    paf->level,
                    paf->duration,
                    paf->modifier,
                    paf->location,
                    paf->bitvector
                   );
        }
    }
    
    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
        bprintf( buf, "ExDe %s~ %s~\n",
            ed->keyword, ed->description );
    }

    LUA_EXTRA_VAL *luaval;
    for ( luaval = obj->luavals; luaval; luaval=luaval->next )
    {
        if (!luaval->persist)
            continue;
        bprintf( buf, "LuaVal %d %s~ %s~\n", 
                luaval->type,
                luaval->name, 
                luaval->val );
    }
    
    bprintf( buf, "End\n\n" );
    
    if ( obj->contains != NULL )
        bwrite_obj( ch, obj->contains, buf, iNest + 1 );
    
    return;
}

/*
 * Load a char and inventory into a new ch structure.
 */
void mem_load_char_obj( DESCRIPTOR_DATA *d, MEMFILE *mf, bool char_only )
{
    CHAR_DATA *ch;
    int stat;
    int i, iNest;
    RBUFFER *buf;
    tflag comm_default = { COMM_COMBINE, COMM_NOBITCH, COMM_PROMPT, COMM_SHOW_WORTH, 
			   COMM_SHOW_ATTRIB };

#if defined(SIM_DEBUG)
   log_string("mem_load_char_obj: start");
#endif
    ch = new_char();
    ch->pcdata = new_pcdata();
    
    d->character            = ch;
    ch->desc                = d;
    ch->name                = str_dup( mf->filename );
    ch->id              = get_pc_id();
    ch->race                = race_lookup("human");
    flag_clear( ch->act ); SET_BIT( ch->act, PLR_NOSUMMON );
    bit_list_to_tflag( comm_default );
    flag_copy( ch->comm, comm_default );
    flag_clear( ch->gag );
    ch->prompt = str_dup( PROMPT_DEFAULT );       
    ch->pcdata->confirm_delete      = FALSE;
    ch->pcdata->confirm_pkill     = FALSE;
    ch->pcdata->pkpoints     = 0;
    ch->pcdata->pkill_count  = 0;
    ch->pcdata->field        = 0;
    ch->pcdata->remorts =0;
    ch->pcdata->smith = NULL;
    ch->pcdata->new_penalty = NULL;
    ch->pcdata->granted = NULL;
    
    flag_clear( ch->penalty );

    /* Every character starts at default board from login.  This board
       should be read_level 0!     */   
    ch->pcdata->board       = &boards[DEFAULT_BOARD];
    
    ch->pcdata->last_note[DEFAULT_BOARD] = 0; /*?*/
    ch->pcdata->in_progress = NULL;/*?*/
    
    ch->pcdata->pwd         = str_dup( "" );
    ch->pcdata->bamfin          = str_dup( "" );
    ch->pcdata->bamfout         = str_dup( "" );
    ch->pcdata->title           = str_dup( "" );
    for (stat =0; stat < MAX_STATS; stat++)
        ch->perm_stat[stat]     = 50;
    for (stat =0; stat < MAX_STATS; stat++)
        ch->pcdata->history_stats[stat]     = 0;
    ch->pcdata->bounty          = 0;
    ch->pcdata->demerit_points  = 0;
    ch->slow_move = 0;

    ch->pcdata->bounty_sort         = NULL;
    ch->pcdata->condition[COND_THIRST]  = 72; 
    ch->pcdata->condition[COND_FULL]    = 72;
    ch->pcdata->condition[COND_HUNGER]  = 72;
    ch->pcdata->security        = 0;    /* OLC */
    ch->pcdata->clan_rank = 0;
    ch->pcdata->customduration = 0;
    ch->pcdata->customflag = str_dup(""); 
    ch->pcdata->authed_by  = NULL;
    ch->pcdata->spouse = NULL;
    ch->pcdata->trained_hit = 0;
    ch->pcdata->trained_mana = 0;
    ch->pcdata->trained_move = 0; 
    ch->pcdata->highest_level = 0;
    ch->pcdata->qdata = NULL;
    clear_tattoos( ch->pcdata->tattoos );
    //ch->pcdata->ch_rel = get_religion_follower_data( ch->name );
    //ch->pcdata->prayed_at = current_time - 24*PULSE_TICK;
    //ch->pcdata->prayer_request = NULL;
    ch->pcdata->combat_action = NULL;
    ch->pcdata->name_color = str_dup("");
    ch->pcdata->pre_title = str_dup("");

    for (i = 0; i < MAX_CLAN; i++)
        ch->pcdata->invitation[i] = NULL;
    
    default_colour( ch );
    
    pfile_version = 0;
    
    /* Bobble: even if buffer is NULL, default init had to be done */
    if (mf->buf == NULL)
      return;

    buf = read_wrap_buffer(mf->buf);

        for ( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;

        for ( ; ; )
        {
            char letter;
            const char *word;
            
            letter = bread_letter( buf );
            if ( letter == '*' )
            {
                bread_to_eol( buf );
                continue;
            }
            
            if ( letter != '#' )
            {
                bug( "mem_load_char_obj: # not found.", 0 );
                break;
            }
            
            word = bread_word( buf );
            if      ( !str_cmp( word, "VER"    ) ) pfile_version = bread_number ( buf );
            else if ( !str_cmp( word, "PLAYER" ) )
            {
                bread_char(ch, buf);
                if ( char_only )
                    break;
            }
            else if ( !str_cmp( word, "OBJECT" ) ) bread_obj  ( ch->pet ? ch->pet : ch, buf, NULL );
            else if ( !str_cmp( word, "O"      ) ) bread_obj  ( ch->pet ? ch->pet : ch, buf, NULL );
            else if ( !str_cmp( word, "PET"    ) ) bread_pet  ( ch, buf );
            else if ( !str_cmp( word, "END"    ) ) break;
            else
            {
                bug( "mem_load_char_obj: bad section.", 0 );
                break;
            }
        }

    // copy verbatim settings from char to descriptor
    if ( d->pProtocol )
        d->pProtocol->verbatim = IS_SET(ch->act, PLR_COLOUR_VERBATIM);
        
    /* record obj count to track vanishing eq bug */
    if ( !char_only )
        logpf("mem_load_char_obj: %s carries %d objects", ch->name, count_objects(ch->carrying));
        
    /* initialize race */
    if (ch->race == 0)
        ch->race = race_lookup("human");
        
        ch->size = pc_race_table[ch->race].size;
        ch->dam_type = 17; /*punch */
        
        for (i = 0; i < pc_race_table[ch->race].num_skills; i++)
        {
            group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
        }
	
        if (pc_race_table[ch->race].gender==0)
	    ch->sex = SEX_NEUTRAL;

	morph_update( ch );
    fix_ptitles( ch );
    
    read_wrap_free(buf);
#if defined(SIM_DEBUG)
    log_string("mem_load_char_obj: done");
#endif
}

void mem_load_storage_box( CHAR_DATA *ch, MEMFILE *mf )
{
    int iNest;
    int box_number = 0;
    RBUFFER *buf;
    buf = read_wrap_buffer(mf->buf);


        for ( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;

        for ( ; ; )
        {
            char letter;
            const char *word;

            letter = bread_letter( buf );
            if ( letter == '*' )
            {
                bread_to_eol( buf );
                continue;
            }

            if ( letter != '#' )
            {
                bug( "mem_load_storage_box: # not found.", 0 );
                break;
            }

            word = bread_word( buf );
	    if (!str_cmp( word, "BOX") ) 
              box_number = bread_number(buf);

            else if ( !str_cmp( word, "OBJECT" ) ) 
	      bread_obj( ch, buf,ch->pcdata->box_data[box_number-1]);

            else if ( !str_cmp( word, "O"      ) ) 
	      bread_obj  ( ch, buf,ch->pcdata->box_data[box_number-1]);

            else if ( !str_cmp( word, "END"    ) ) break;
            else
            {
                bug( "mem_load_storage_box: bad section.", 0 );
                break;
            }
        }

}

/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value ) if ( !str_cmp( word, literal ) ) { field  = value; fMatch = TRUE; break; }

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS( literal, field, value ) if ( !str_cmp( word, literal ) ) { free_string(field); field  = value; fMatch = TRUE; break; }

/* flag reading special */
#define KEYF( literal, field) if ( !str_cmp( word, literal ) ) { bread_tflag(buf, field); fMatch = TRUE; break; }

/* read and discard (gobble) old value */
#define GOBBLE( literal, value ) if ( !str_cmp(word, literal) ) { value; fMatch = TRUE; break; }

void bread_char( CHAR_DATA *ch, RBUFFER *buf )
{
    char str_buf[MAX_STRING_LENGTH];
    const char *word;
    bool fMatch;
    int count = 0;
    int lastlogoff = current_time;
    double percent;
    
#if defined(SIM_DEBUG)
   log_string("bread_char: start");
#endif
    sprintf(str_buf,"Loading %s.",ch->name);
    log_string(str_buf);
    
    for ( ; ; )
    {
        word   = beof( buf ) ? "End" : bread_word( buf );
        fMatch = FALSE;
        
        switch ( UPPER(word[0]) )
        {
        case '*':
            fMatch = TRUE;
            bread_to_eol( buf );
            break;
            
        case 'A':
            KEYF( "Achv", ch->pcdata->achievements );
            KEYF( "Act", ch->act );
	    //REMOVE_BIT(ch->act, PLR_CONSENT);

            KEYF( "AffectedBy",  ch->affect_field );
            KEYF( "AfBy",    ch->affect_field );
            KEY( "AchPts",   ch->pcdata->achpoints, bread_number( buf ) );
            KEY( "Alignment",   ch->alignment,      bread_number( buf ) );
            KEY( "Alig",    ch->alignment,      bread_number( buf ) );
            KEYS( "AuthedBy",	ch->pcdata->authed_by,	bread_string( buf ) );
            KEYS( "Action", ch->pcdata->combat_action,	bread_string( buf ) );
            
            if (!str_cmp( word, "Alia"))
            {
                if (count >= MAX_ALIAS)
                {
                    bread_to_eol(buf);
                    fMatch = TRUE;
                    break;
                }
                
                ch->pcdata->alias[count]    = str_dup(bread_word(buf));
                ch->pcdata->alias_sub[count]    = str_dup(bread_word(buf));
                count++;
                fMatch = TRUE;
                break;
            }
            
            if (!str_cmp( word, "Alias"))
            {
                if (count >= MAX_ALIAS)
                {
                    bread_to_eol(buf);
                    fMatch = TRUE;
                    break;
                }
                
                ch->pcdata->alias[count]        = str_dup(bread_word(buf));
                ch->pcdata->alias_sub[count]    = bread_string(buf);
                count++;
                fMatch = TRUE;
                break;
            }
            
            if (!str_cmp( word, "AC") )
            {
                ch->armor=bread_number(buf);
                fMatch = TRUE;
                break;
            }
            
            if (!str_cmp(word,"ACs"))
            {
                /* old format */
                ch->armor=bread_number(buf);
                bread_to_eol(buf);

                fMatch = TRUE;
                break;
            }
            
	    /* outdated --Bobble
            if (!str_cmp(word, "AffD"))
            {
                AFFECT_DATA *paf;
                int sn;
                
                paf = new_affect();
                
                sn = skill_lookup(bread_word(buf));
                if (sn < 0)
                    bug("Bread_char: unknown skill.",0);
                else
                    paf->type = sn;
                
                paf->level  = bread_number( buf );
                paf->duration   = bread_number( buf );
                paf->modifier   = bread_number( buf );
                paf->location   = bread_number( buf );
                paf->bitvector  = bread_number( buf );
                paf->next   = ch->affected;
                ch->affected    = paf;
                fMatch = TRUE;
                break;
            }
	    */            

            if (!str_cmp(word, "Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
                
                paf = new_affect();
                
                sn = skill_lookup(bread_word(buf));
                if (sn < 0)
                    bug("Bread_char: unknown skill.",0);
                else
                    paf->type = sn;
                
                paf->where  = bread_number(buf);
                paf->level      = bread_number( buf );
                paf->duration   = bread_number( buf );
                paf->modifier   = bread_number( buf );
                paf->location   = bread_number( buf );
                paf->bitvector  = bread_number( buf );
                if ( pfile_version < VER_FLAG_CHANGE )
                    FLAG_CONVERT( paf->bitvector );
                ch->affected = affect_insert(ch->affected, paf);
                fMatch = TRUE;
                break;
            }

            if (!str_cmp(word, "AffCust"))
            {
                AFFECT_DATA *paf;
                
                paf = new_affect();

                paf->type = gsn_custom_affect;
                paf->tag = bread_string( buf );
                paf->where = bread_number(buf);
                paf->level = bread_number(buf);
                paf->duration = bread_number(buf);
                paf->modifier = bread_number(buf);
                paf->location = bread_number(buf);
                paf->bitvector = bread_number(buf);
                paf->next = ch->affected;
                ch->affected = paf;
                fMatch = TRUE;
                break;
            } 
            
            if ( !str_cmp( word, "AttrMod"  ) || !str_cmp(word,"AMod"))
            {
                int stat;
                for (stat = 0; stat < 5; stat ++)
                    ch->mod_stat[stat] = bread_number(buf);
                for (stat=0; stat<MAX_STATS; stat++)
                    ch->mod_stat[stat] = 0;
                
                fMatch = TRUE;
                break;
            }
            
            if ( !str_cmp( word, "AttrPerm" ) || !str_cmp(word,"Attr"))
            {
                int stat;
                
                for (stat = 0; stat < 5; stat++)
                {
                    ch->perm_stat[stat] = bread_number(buf);
                    ch->train+=ch->perm_stat[stat]-7;
                }
                if (ch->race == race_lookup("mutant")) ch->train+=21;
                for (stat = 0; stat < MAX_STATS; stat++)
                {
                    ch->perm_stat[stat] = (pc_race_table[ch->race].max_stats[stat]
                        + pc_race_table[ch->race].min_stats[stat])/2
                        + number_range(0,10) + class_bonus(ch->class, stat);
                    ch->pcdata->original_stats[stat] = ch->perm_stat[stat];
                }
                fMatch = TRUE;
                break;
            }
            
            if ( !str_cmp(word,"AOrg"))
            {
                int stat;
                for (stat = 0; stat < MAX_STATS; stat ++)
                    ch->pcdata->original_stats[stat] = bread_number(buf);
                fMatch = TRUE;
                break;
            }
            
            if ( !str_cmp(word,"AHis"))
            {
                int stat;
                for (stat = 0; stat < MAX_STATS; stat ++)
                    ch->pcdata->history_stats[stat] = bread_number(buf);
                fMatch = TRUE;
                break;
            }
            
            KEY( "Ascent", ch->pcdata->ascents, bread_number(buf) );
            
            break;
            
    case 'B':
        if (!str_cmp(word, "BAch" ) )
        {
            BOSSREC * rec = alloc_BOSSREC();
            rec->vnum=bread_number(buf);
            rec->timestamp=bread_number(buf);

            rec->next = ch->pcdata->boss_achievements;
            ch->pcdata->boss_achievements = rec;
            fMatch = TRUE;
            break;
        }

        KEYS( "Bamfin",  ch->pcdata->bamfin, bread_string( buf ) );
        KEYS( "Bamfout", ch->pcdata->bamfout,    bread_string( buf ) );
        KEY( "Bank",    ch->pcdata->bank,       bread_number( buf ) );       
	    KEY( "Bhds",    ch->pcdata->behead_cnt, bread_number(buf));
        KEYS( "Bin",     ch->pcdata->bamfin, bread_string( buf ) );
        KEYS( "Bout",    ch->pcdata->bamfout,    bread_string( buf ) );
	    KEY( "Boxes",   ch->pcdata->storage_boxes, bread_number(buf) );
        KEY( "Bnty",  ch->pcdata->bounty,   bread_number( buf ) );
        
        /* Read in board status */      
        if (!str_cmp(word, "Boards" ))
        {
            int i,num = bread_number (buf); /* number of boards saved */
            const char *boardname;
            
            for (; num ; num-- ) /* for each of the board saved */
            {
                boardname = bread_word (buf);
                i = board_lookup (boardname); /* find board number */
                
                if (i == BOARD_NOTFOUND) /* Does board still exist ? */
                {
                    sprintf (str_buf, "bread_char: %s had unknown board name: %s. Skipped.", ch->name, boardname);                 
                    log_string (str_buf);
                    bread_number (buf); /* read last_note and skip info */
                }
                else /* Save it */
                    ch->pcdata->last_note[i] = bread_number (buf);
            }     /* for */
            fMatch = TRUE;
            break;
        } /* Boards */
        
        break;
        
    case 'C':
        KEY( "Class",   ch->class,      bread_number( buf ) );
        KEY( "Cla",     ch->class,      bread_number( buf ) );
        KEY( "Calm",    ch->calm,       bread_number( buf ) );
        if ( !str_cmp(word, "Clan") )
        {
            const char *temp=bread_string(buf);
            ch->clan=clan_lookup(temp);
            free_string(temp);
            fMatch=TRUE;
            break;
        }
        KEYS( "CFlag",   ch->pcdata->customflag, bread_string(buf));
        KEY( "CDur",    ch->pcdata->customduration, bread_number(buf));
        
        if ( !str_cmp(word,"Cond"))
        {
            ch->pcdata->condition[0] = bread_number( buf );
            ch->pcdata->condition[1] = bread_number( buf );
            ch->pcdata->condition[2] = bread_number( buf );
            ch->pcdata->condition[3] = bread_number( buf );
            /*ch->pcdata->condition[4] =*/ bread_number( buf );
            /*ch->pcdata->condition[5] =*/ bread_number( buf );
            fMatch = TRUE;
            break;
        }
        if (!str_cmp(word,"Cnd"))
        {
            ch->pcdata->condition[0] = bread_number( buf );
            ch->pcdata->condition[1] = bread_number( buf );
            ch->pcdata->condition[2] = bread_number( buf );
            ch->pcdata->condition[3] = bread_number( buf );
            fMatch = TRUE;
            break;
        }
        KEYF("Comm",     ch->comm ); 
        if (!str_cmp(word, "CRank") )
        {
            const char *temp=bread_string(buf);
            ch->pcdata->clan_rank=clan_rank_lookup(ch->clan, temp);
            free_string(temp);
            fMatch=TRUE;
            break;
        }

	if ( !str_cmp(word, "Crime") )
	{
	    CRIME_DATA *cr, *newcr;
	    /* read in crime */
	    newcr = new_crime();
	    newcr->name = bread_string( buf );
	    newcr->imm_name = bread_string( buf );
	    newcr->timestamp = bread_number( buf );
	    newcr->forgive = bread_number( buf );
	    /* add at end of crime list to preserve order */
	    if ( ch->pcdata->crimes == NULL )
		ch->pcdata->crimes = newcr;
	    else
	    {
		for ( cr = ch->pcdata->crimes; cr->next != NULL; cr = cr->next )
		    ;
		cr->next = newcr;
	    }
	    fMatch = TRUE;
	    break;
	}
        if (!str_cmp( word, "Coloura" ) )
        {
            LOAD_COLOUR( gossip )
                LOAD_COLOUR( auction )
                LOAD_COLOUR( music )
                LOAD_COLOUR( question )
                LOAD_COLOUR( answer )
                fMatch = TRUE;
            break;
        }
        if (!str_cmp( word, "Colourb" ) )
        {
            LOAD_COLOUR( quote )
                LOAD_COLOUR( gratz )
                LOAD_COLOUR( immtalk )
                LOAD_COLOUR( shouts )
                LOAD_COLOUR( tells )
                fMatch = TRUE;
            break;
        }
        if (!str_cmp( word, "Colourc" ) )
        {
            LOAD_COLOUR( info )
                LOAD_COLOUR( gametalk )
                LOAD_COLOUR( bitch )
                LOAD_COLOUR( newbie )
                LOAD_COLOUR( clan )
                fMatch = TRUE;
            break;
        }
        if (!str_cmp( word, "Colourd" ) )
        {
            LOAD_COLOUR( say )
                LOAD_COLOUR( gtell )
                LOAD_COLOUR( room_title )
                LOAD_COLOUR( room_exits )
                LOAD_COLOUR( wiznet )
                fMatch = TRUE;
            break;
        }
        if (!str_cmp( word, "Coloure" ) )
        {
            LOAD_COLOUR( gossip_text )
                LOAD_COLOUR( auction_text )
                LOAD_COLOUR( music_text )
                LOAD_COLOUR( question_text )
                LOAD_COLOUR( answer_text )
                fMatch = TRUE;
            break;
        }
        if (!str_cmp( word, "Colourf" ) )
        {
            LOAD_COLOUR( quote_text )
                LOAD_COLOUR( gratz_text )
                LOAD_COLOUR( immtalk_text )
                LOAD_COLOUR( shouts_text )
                LOAD_COLOUR( tell_text )
                fMatch = TRUE;
            break;
        }
        if (!str_cmp( word, "Colourg" ) )
        {
            LOAD_COLOUR( info_text )
                LOAD_COLOUR( gametalk_text )
                LOAD_COLOUR( bitch_text )
                LOAD_COLOUR( newbie_text )
                LOAD_COLOUR( clan_text )
                fMatch = TRUE;
            break;
        }
        if (!str_cmp( word, "Colourh" ) )
        {
            LOAD_COLOUR( say_text )
                LOAD_COLOUR( gtell_text )
                LOAD_COLOUR( warfare )
                LOAD_COLOUR( warfare_text )
                fMatch = TRUE;
            break;
        }
        if (!str_cmp( word, "Colouri" ) )
        {
                LOAD_COLOUR( savantalk )
                LOAD_COLOUR( savantalk_text )
		LOAD_COLOUR( proclaim )
		LOAD_COLOUR( proclaim_text )
                fMatch = TRUE;
            break;
        }
        
        break;
        
    case 'D':
        KEY( "Damroll", ch->damroll,        bread_number( buf ) );
        KEY( "Dam",     ch->damroll,        bread_number( buf ) );
        KEYS( "Description", ch->description,    bread_string( buf ) );
        KEYS( "Desc",    ch->description,    bread_string( buf ) );
        KEY( "Demerit", ch->pcdata->demerit_points, bread_number( buf ) );
        break;
        
    case 'E':
        if ( !str_cmp( word, "End" ) )
        {
	    /*
            percent=(double)(current_time-lastlogoff);
            percent/=3600.0;
            percent*=.000158;
            ch->pcdata->bank+=(int)(ch->pcdata->bank*percent);
	    */

            /* adjust hp mana move up  -- here for speed's sake */
            percent = (current_time - lastlogoff) * 25 / ( 2 * 60 * 60);
            percent = UMIN(percent,100);
            fingertime=lastlogoff;
            
            if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
                &&  !IS_AFFECTED(ch,AFF_PLAGUE))
            {
                gain_hit(ch, (ch->max_hit - ch->hit) * percent / 100);
                gain_mana(ch, (ch->max_mana - ch->mana) * percent / 100);
                gain_move(ch, (ch->max_move - ch->move) * percent / 100);
            }

	    /* highest level not always saved */
	    ch->pcdata->highest_level = UMAX(ch->pcdata->highest_level, ch->level);

	    /* money wipe */
	    if ( pfile_version < VER_MONEY_WIPE )
	    {
		ch->gold = 0;
		ch->silver = 0;
		ch->pcdata->bank = 0;
		ch->pcdata->bounty = 0;
	    }
	    /* increased etl for some players */
	    if ( pfile_version < VER_EXP_RAISE )
	    {
		set_level_exp( ch );
	    }

            return;
        }


        KEY( "Exp",     ch->exp,        bread_number( buf ) );
		
		/* New ExploredN keyword to be used after version changeover (from bugged version to non bugged version).
		   Older Explored keyword will recalculate explored->set based on number of entries.*/
        if(!str_cmp(word, "Explored") )
		{	
			int mask;
			int bits;
			EXPLORE_HOLDER *pExp;
			
			bread_number(buf); // This is "set". Don't want it, going to recalculate
			int set=0;
			while(1)
			{	
				mask = bread_number(buf);
				bits = bread_number(buf);
				if(mask == -1)
					break;
				if(mask < 0 )
					continue; /*old junk data, we don't want or count this */
				for(pExp = ch->pcdata->explored->buckets ; pExp ; pExp = pExp->next )
					if(pExp->mask == mask)
						break;
				if(!pExp)
				{	pExp = (EXPLORE_HOLDER *)calloc(1, sizeof(*pExp) );
					pExp->next = ch->pcdata->explored->buckets;
					ch->pcdata->explored->buckets = pExp;
					pExp->mask = mask;
					pExp->bits = bits;
				}
				int bit;
				for ( bit=0 ; bit <32 ; bit++ )
				{
					if ( ( ( bits >> bit) & 1 ) == 1 )
						set++;
				}
			}
			ch->pcdata->explored->set=set;
			fMatch = TRUE;
            break;
			
		}
		else if(!str_cmp(word, "ExploredN") )
		{
			ch->pcdata->explored->set=bread_number(buf);
			int mask;
			int bits;
		
			EXPLORE_HOLDER *pExp;
			
			while(1)
			{	mask = bread_number(buf);
				bits = bread_number(buf);
				if(mask == -1)
					break;
				for(pExp = ch->pcdata->explored->buckets ; pExp ; pExp = pExp->next )
					if(pExp->mask == mask)
						break;
				if(!pExp)
				{	pExp = (EXPLORE_HOLDER *)calloc(1, sizeof(*pExp) );
					pExp->next = ch->pcdata->explored->buckets;
					ch->pcdata->explored->buckets = pExp;
					pExp->mask = mask;
					pExp->bits = bits;
				}
			}
			fMatch = TRUE;
            break;
		}
        break;
        
    case 'F':
        KEY( "Faith",   ch->pcdata->faith,  bread_number(buf));
        KEY( "Field",   ch->pcdata->field,  bread_number(buf));
        
    case 'G':
        KEYF( "Gag",     ch->gag );
        KEYS( "God",    ch->pcdata->god_name,   bread_string(buf) );
        KEY( "Gold",    ch->gold,       bread_number( buf ) );
        if ( !str_cmp( word, "Group" )  || !str_cmp(word,"Gr"))
        {
            int gn;
            const char *temp;
            
            temp = bread_word( buf ) ;
            gn = group_lookup(temp);
            /* gn    = group_lookup( bread_word( buf ) ); */
            if ( gn < 0 )
            {
                if ( strcmp(temp, "rom basics") )
                    bugf("bread_char: unknown group '%s'.", temp);
            }
            else
                gn_add(ch,gn);
            fMatch = TRUE;
            break;
        }
        /* old */
        KEY( "Gui", ch->pcdata->guiconfig.chat_window, bread_number( buf ) );
        /* new */
        if ( !str_cmp( word, "GuiC" ) )
        {
            ch->pcdata->guiconfig.chat_window = bread_number( buf );
            ch->pcdata->guiconfig.show_images = bread_number( buf );
            ch->pcdata->guiconfig.image_window = bread_number( buf );
            fMatch = TRUE;
            break;
        }

        break;
        
    case 'H':
        KEY( "Hitroll", ch->hitroll,        bread_number( buf ) );
        KEY( "Hit",     ch->hitroll,        bread_number( buf ) );
        KEY( "HLev",    ch->pcdata->highest_level, bread_number( buf ) );
        	
        if ( !str_cmp( word, "HpManaMove" ) || !str_cmp(word,"HMV"))
        {
            ch->hit     = bread_number( buf );
            ch->max_hit = bread_number( buf );
            ch->mana    = bread_number( buf );
            ch->max_mana    = bread_number( buf );
            ch->move    = bread_number( buf );
            ch->max_move    = bread_number( buf );
            fMatch = TRUE;
            break;
        }
        
        break;
        
    case 'I':
        KEY( "Id",      ch->id,         bread_number( buf ) );
        KEY( "InvisLevel",  ch->invis_level,    bread_number( buf ) );
        KEY( "Inco",    ch->incog_level,    bread_number( buf ) );
        KEY( "Invi",    ch->invis_level,    bread_number( buf ) );

        if (!str_cmp(word, "Invite" ))
        {
            int i = 0;

            i = clan_lookup(bread_word(buf));

            if (i > 0)
                ch->pcdata->invitation[i] = bread_string(buf);
                                       
            fMatch = TRUE;
            break;
        }

        if ( !str_cmp( word, "Icmd" ) )
        {
            GRANT_DATA *gran;
            int cmd;
            
            gran = alloc_mem(sizeof(*gran));
            gran->name = str_dup(bread_word(buf));
            gran->duration = bread_number(buf);
            gran->next = NULL;
            gran->do_fun = NULL;
            
            for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
                if ( gran->name[0] == cmd_table[cmd].name[0]
                    &&  is_exact_name( gran->name, cmd_table[cmd].name ) )
                {
                    gran->do_fun = cmd_table[cmd].do_fun;
                    gran->level = cmd_table[cmd].level;
                    break;
                }
                
                if (gran->do_fun == NULL)
                {
                    sprintf(str_buf,"Grant: Command %s not found in pfile for %s",
			    gran->name,ch->name);
                    log_string(str_buf);
		    /* drop the grant */
		    free_string(gran->name);
		    free_mem(gran,sizeof(*gran));
                }
                else
		{
		    gran->next = ch->pcdata->granted;
		    ch->pcdata->granted = gran;
		}
                
                fMatch = TRUE;
                break;
        }
        
        break;
        
    case 'L':
        KEY( "LastLevel",   ch->pcdata->last_level, bread_number( buf ) );
        KEYS( "LastH",  ch->pcdata->last_host, bread_string(buf));
        KEY( "LLev",    ch->pcdata->last_level, bread_number( buf ) );
        KEY( "Level",   ch->level,      bread_number( buf ) );
        KEY( "Lev",     ch->level,      bread_number( buf ) );
        KEY( "Levl",    ch->level,      bread_number( buf ) );
        KEY( "LogO",    lastlogoff,     bread_number( buf ) );
        KEYS( "LongDescr",   ch->long_descr,     bread_string( buf ) );
        KEYS( "LnD",     ch->long_descr,     bread_string( buf ) );

        if ( !strcmp( word, "LuaCfg") )
        {
            const char *temp = bread_string( buf );
            load_luaconfig( ch, temp );
            free_string( temp );
            fMatch=TRUE;
            break;
        }

        if ( !strcmp( word, "LuaVal") )
        {
            LUA_EXTRA_VAL *luaval;
            int type=bread_number( buf );
            const const char *name= bread_string( buf );
            const const char *val = bread_string( buf );
            luaval=new_luaval( type, name, val, TRUE );

            luaval->next=ch->luavals;
            ch->luavals=luaval;
            fMatch=TRUE;
            break;
        }

        break;
        
    case 'M':
        KEY( "MobKills",ch->pcdata->mob_kills,  bread_number( buf ) );
        KEY( "MobDeaths",ch->pcdata->mob_deaths,         bread_number( buf ) );
	if ( !str_cmp(word, "Morph") )
	{
        const char *temp=bread_string(buf);
	    ch->pcdata->morph_race = race_lookup( temp );
        free_string(temp);
	    ch->pcdata->morph_time = bread_number( buf );
	    fMatch = TRUE;
	    break;
	}

        if ( !str_cmp(word, "Mastery") || !str_cmp(word,"Ma") )
        {
            int value = bread_number(buf);
            const char *temp = bread_word(buf);
            int sn = skill_lookup(temp);

            if ( sn < 0 )
                bugf("bread_char: unknown mastery skill '%s'", temp);
            else
                ch->pcdata->mastered[sn] = value;
            fMatch = TRUE;
            break;
        }

    case 'N':
        KEYS( "Name",   ch->name,       bread_string( buf ) );
        if ( !str_cmp(word,"NewAMod"))
        {
            int stat;
            for (stat = 0; stat < MAX_STATS; stat ++)
                ch->mod_stat[stat] = bread_number(buf);
            fMatch = TRUE;
            break;
        }
        
        if ( !str_cmp(word,"NewAttr"))
        {
            int stat;
            
            for (stat = 0; stat < MAX_STATS; stat++)
                ch->perm_stat[stat] = bread_number(buf);
            fMatch = TRUE;
            break;
        }
	KEYS( "NmCol", ch->pcdata->name_color, bread_string( buf) );
        break;
        
    case 'P':
        KEYS( "Password",    ch->pcdata->pwd,    bread_string( buf ) );
        KEYS( "Pass",    ch->pcdata->pwd,    bread_string( buf ) );
        KEY( "PKPoints", ch->pcdata->pkpoints, bread_number( buf ) );
	    KEY( "PkillDeaths", ch->pcdata->pkill_deaths, bread_number( buf ) );
        KEY( "Played",  ch->played,     bread_number( buf ) );
        KEY( "Plyd",    ch->played,     bread_number( buf ) );
        KEY( "Points",  ch->pcdata->points, bread_number( buf ) );
        KEY( "Pnts",    ch->pcdata->points, bread_number( buf ) );
        KEY( "Position",    ch->position,       bread_number( buf ) );
        KEY( "Pos",     ch->position,       bread_number( buf ) );
        KEY( "Practice",    ch->practice,       bread_number( buf ) );
        KEY( "Prac",    ch->practice,       bread_number( buf ) );
	    KEYS( "Pretitle",ch->pcdata->pre_title, bread_string(buf));
        if ( !strcmp( word, "Ptitles") )
        {
            const char *temp = bread_string( buf );
            load_ptitles( ch, temp );
            free_string( temp );
            fMatch=TRUE;
            break;
        }
        KEYS( "Prompt",      ch->prompt,             bread_string( buf ) );
        KEYS( "Prom",    ch->prompt,     bread_string( buf ) );

    /*
	if ( !str_cmp( word, "Pray" ) )
	{
	    ch->pcdata->prayed_at = bread_number( buf );
	    fMatch = TRUE;
	    break;
	}
	*/

	if ( !str_cmp( word, "PKCount" ) )
	{
	    ch->pcdata->pkill_count  = bread_number( buf );
	    if( ch->pcdata->pkpoints == 0 )
	        ch->pcdata->pkpoints = ch->pcdata->pkill_count;
	    fMatch = TRUE;
	    break;
	}
	
	KEY( "PKExpire", ch->pcdata->pkill_expire, bread_number( buf) );

        break;
        
    case 'Q':
        KEY( "QuestPnts",   ch->pcdata->questpoints,        bread_number( buf ) );
        KEY( "QuestNext",   ch->pcdata->nextquest,          bread_number( buf ) );
        KEY( "QuestsSuccess",ch->pcdata->quest_success,         bread_number( buf ) );
        KEY( "QuestsHardSuccess", ch->pcdata->quest_hard_success, bread_number( buf ) );
        KEY( "QuestsHardFailed", ch->pcdata->quest_hard_failed, bread_number( buf ) );
        KEY( "QuestsFailed",ch->pcdata->quest_failed,       bread_number( buf ) );

	if ( !str_cmp(word, "QStat") )
	{
 	    int id, status, timer;
            time_t limit;
	    id = bread_number( buf );
	    status = bread_number( buf );  
            timer = bread_number( buf ) ;
            if (timer > 0)
                /* This decrements the qset timer by 1 point for each hour that
                   you have been logged off. - Astark */
                timer -= ((current_time - lastlogoff) / 3600) ;  
            if (timer < 0)
                timer = 0;
                /* This resets the limit to 1 hour from the time of login, so that
                   it will start to decrement every 1 hour again. - Astark */
            limit = current_time + 3600;

            set_quest_status( ch, id, status, timer, limit);
	    fMatch = TRUE;
	    break;
	}

        break;
        
        
    case 'R':
        if (!str_cmp(word, "Race") )
        {
            const char *temp=bread_string(buf);
            ch->race=race_lookup(temp);
            free_string(temp);
            fMatch=TRUE;
            break;
        }
        KEY( "Remort",  ch->pcdata->remorts,    bread_number(buf));
        
        if ( !str_cmp( word, "Room" ) )
        {
            ch->in_room = get_room_index( bread_number( buf ) );
            if ( ch->in_room == NULL )
                ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
            fMatch = TRUE;
            break;
        }
        KEY( "RRank",   ch->pcdata->religion_rank,  bread_number(buf) );
        
        break;
        
    case 'S':
        KEY( "SavingThrow", ch->saving_throw,   bread_number( buf ) );
        KEY( "Save",    ch->saving_throw,   bread_number( buf ) );
        KEY( "Scro",    ch->lines,      bread_number( buf ) );
        KEY( "Sex",     ch->sex,        bread_number( buf ) );
        KEYS( "ShortDescr",  ch->short_descr,    bread_string( buf ) );
        KEYS( "ShD",     ch->short_descr,    bread_string( buf ) );
        KEY( "Sec",         ch->pcdata->security,   bread_number( buf ) );   /* OLC */
        KEY( "Silv",        ch->silver,             bread_number( buf ) );
        KEYS( "Spouse",	ch->pcdata->spouse,	bread_string( buf ) );
        
        
        if ( !str_cmp( word, "Skill" ) || !str_cmp(word,"Sk"))
        {
            int sn;
            int value;
            const char *temp;
            
            value = bread_number( buf );
            temp = bread_word( buf ) ;
            sn = skill_lookup(temp);
            /* sn    = skill_lookup( bread_word( buf ) ); */
            if ( sn < 0 )
            {
                if ( strcmp(temp, "recall") )
                    bugf("bread_char: unknown skill '%s'.", temp);
            }
            else
                ch->pcdata->learned[sn] = value;
            fMatch = TRUE;
            break;
        }

        if ( !str_cmp(word, "SkillMasteryCount") || !str_cmp(word, "Smc") )
        {
            ch->pcdata->smc_mastered = bread_number(buf);
            ch->pcdata->smc_grandmastered = bread_number(buf);
            ch->pcdata->smc_retrained = bread_number(buf);
            fMatch = TRUE;
            break;
        }

        KEY( "Stance",  ch->stance, bread_number( buf ) );
        
        if ( !str_cmp(word, "Subclass") )
        {
            const char *temp = bread_string(buf);
            ch->pcdata->subclass = subclass_lookup(temp);
            free_string(temp);
            fMatch = TRUE;
            break;
        }
        
        break;
        
    case 'T':
        KEYF( "Togg",	ch->togg	);
        KEY( "TrueSex",     ch->pcdata->true_sex,   bread_number( buf ) );
        KEY( "TSex",    ch->pcdata->true_sex,   bread_number( buf ) );
        KEY( "Trai",    ch->train,      bread_number( buf ) );
        KEY( "Trust",   ch->trust,      bread_number( buf ) );
        KEY( "Tru",     ch->trust,      bread_number( buf ) );
	KEY( "THit",	ch->pcdata->trained_hit, bread_number(buf));
	KEY( "TMan",	ch->pcdata->trained_mana, bread_number(buf));
	KEY( "TMov",	ch->pcdata->trained_move, bread_number(buf));
        
        if ( !str_cmp( word, "Title" )  || !str_cmp( word, "Titl"))
        {
            const char *temp=bread_string(buf);
            set_title(ch, temp);
            free_string(temp);
            fMatch = TRUE;
            break;
        }
	
	if ( !str_cmp(word, "Tattoos") )
	{
	    bread_tattoos( buf, ch->pcdata->tattoos );
            fMatch = TRUE;
            break;
	}
        
        break;
        
    case 'V':
        KEY( "Version",     ch->version,        bread_number ( buf ) );
        KEY( "Vers",    ch->version,        bread_number ( buf ) );
        if ( !str_cmp( word, "Vnum" ) )
        {
            ch->pIndexData = get_mob_index( bread_number( buf ) );
            fMatch = TRUE;
            break;
        }
        break;
        
    case 'W':
        KEY( "Wimpy",   ch->wimpy,      bread_number( buf ) );
        KEY( "Wimp",    ch->wimpy,      bread_number( buf ) );
        KEYF( "Wizn",    ch->wiznet );
        KEY( "WarGenoWon",  ch->pcdata->armageddon_won,       bread_number( buf ) );
        KEY( "WarGenoLost", ch->pcdata->armageddon_lost,      bread_number( buf ) );
        KEY( "WarGenoKills",ch->pcdata->armageddon_kills,     bread_number( buf ) );
        KEY( "WarClanWon",  ch->pcdata->clan_won,           bread_number( buf ) );
        KEY( "WarClanLost", ch->pcdata->clan_lost,          bread_number( buf ) );
        KEY( "WarClanKills",ch->pcdata->clan_kills,         bread_number( buf ) );
        KEY( "WarRaceWon",  ch->pcdata->race_won,           bread_number( buf ) );
        KEY( "WarRaceLost", ch->pcdata->race_lost,          bread_number( buf ) );
        KEY( "WarRaceKills",ch->pcdata->race_kills,         bread_number( buf ) );
        KEY( "WarClassWon", ch->pcdata->class_won,          bread_number( buf ) );
        KEY( "WarClassLost",ch->pcdata->class_lost,         bread_number( buf ) );
        KEY( "WarClassKills",ch->pcdata->class_kills,       bread_number( buf ) );
        KEY( "WarPoints",ch->pcdata->warpoints,             bread_number( buf ) );
        GOBBLE( "WarTotalWars", bread_number(buf) );
        KEY( "WarGenderKills",ch->pcdata->gender_kills,     bread_number( buf ) );
        KEY( "WarGenderWon",ch->pcdata->gender_won,         bread_number( buf ) );
        KEY( "WarGenderLost",ch->pcdata->gender_lost,       bread_number( buf ) );
        KEY( "WarReligionKills",ch->pcdata->religion_kills, bread_number( buf ) );
        KEY( "WarReligionWon",ch->pcdata->religion_won,     bread_number( buf ) );
        KEY( "WarReligionLost",ch->pcdata->religion_lost,   bread_number( buf ) );
        KEY( "WarDuelKills", ch->pcdata->duel_kills,        bread_number( buf ) );
        KEY( "WarDuelWon", ch->pcdata->duel_won,            bread_number( buf ) );
        KEY( "WarDuelLost", ch->pcdata->duel_lost,          bread_number( buf ) );

	if ( !str_cmp( word, "WarTotalKills" ) )
	{
	    ch->pcdata->war_kills = bread_number( buf );
	    if( ch->pcdata->warpoints == 0 )
	        ch->pcdata->warpoints = ch->pcdata->war_kills;
	    fMatch = TRUE;
	    break;
	}

        break;
    }
	
    if ( !fMatch )
    {
        bug( "Bread_char: no match.", 0 );
        bug( word, 0 );
        bread_to_eol( buf );
    }
    }

#if defined(SIM_DEBUG)
   log_string("bread_char: done");
#endif

}

/* load a pet from the forgotten reaches */
void bread_pet( CHAR_DATA *ch, RBUFFER *buf )
{
    const char *word;
    CHAR_DATA *pet;
    bool fMatch;
    int lastlogoff = current_time;
    int percent;
    
    /* first entry had BETTER be the vnum or we barf */
    word = beof(buf) ? "END" : bread_word(buf);
    if (!str_cmp(word,"Vnum"))
    {
        int vnum;
        
        vnum = bread_number(buf);
        if (get_mob_index(vnum) == NULL)
        {
            bug("Bread_pet: bad vnum %d.",vnum);
            pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
        }
        else
            pet = create_mobile(get_mob_index(vnum));
    }
    else
    {
        bug("Bread_pet: no vnum in file.",0);
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }
    
    for ( ; ; )
    {
        word    = beof(buf) ? "END" : bread_word(buf);
        fMatch = FALSE;
        
        switch (UPPER(word[0]))
        {
        case '*':
            fMatch = TRUE;
            bread_to_eol(buf);
            break;
            
        case 'A':
            KEYF( "Act",     pet->act );
            KEYF( "AfBy",    pet->affect_field );
            KEY( "Alig",    pet->alignment,     bread_number(buf));
            
            if (!str_cmp(word,"ACs"))
            {
                /* old format */
                pet->armor=bread_number(buf);
                bread_to_eol(buf);

                fMatch = TRUE;
                break;
            }
            
	    /* outdated --Bobble
            if (!str_cmp(word,"AffD"))
            {
                AFFECT_DATA *paf;
                int sn;
                
                paf = new_affect();
                
                sn = skill_lookup(bread_word(buf));
                if (sn < 0)
                    bug("Bread_char: unknown skill.",0);
                else
                    paf->type = sn;
                
                paf->level  = bread_number(buf);
                paf->duration   = bread_number(buf);
                paf->modifier   = bread_number(buf);
                paf->location   = bread_number(buf);
                paf->bitvector  = bread_number(buf);
                paf->next   = pet->affected;
                pet->affected   = paf;
                fMatch      = TRUE;
                break;
            }
	    */
            
            if (!str_cmp(word,"Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
                
                paf = new_affect();
                
                sn = skill_lookup(bread_word(buf));
                if (sn < 0)
                    bug("Bread_char: unknown skill (%d).", sn);
                else
                    paf->type = sn;
                
                paf->where  = bread_number(buf);
                paf->level      = bread_number(buf);
                paf->duration   = bread_number(buf);
                paf->modifier   = bread_number(buf);
                paf->location   = bread_number(buf);
                paf->bitvector  = bread_number(buf);
		if ( pfile_version < VER_FLAG_CHANGE )
		    FLAG_CONVERT( paf->bitvector );
                paf->next       = pet->affected;
                pet->affected   = paf;
                fMatch          = TRUE;
                break;
            }

            if (!str_cmp(word, "AffCust"))
            {
                AFFECT_DATA *paf;

                paf = new_affect();

                paf->type=gsn_custom_affect;
                paf->tag = bread_string(buf);
                paf->where=bread_number(buf);
                paf->level=bread_number(buf);
                paf->duration=bread_number(buf);
                paf->modifier=bread_number(buf);
                paf->location=bread_number(buf);
                paf->bitvector=bread_number(buf);
                paf->next=pet->affected;
                pet->affected=paf;
                fMatch=TRUE;
                break;
            }
            
            if (!str_cmp(word,"AMod"))
            {
                int stat;
                
                for (stat = 0; stat < MAX_STATS; stat++)
                    pet->mod_stat[stat] = bread_number(buf);
                fMatch = TRUE;
                break;
            }
            
            if (!str_cmp(word,"Attr"))
            {
                int stat;
                
                for (stat = 0; stat < MAX_STATS; stat++)
                    pet->perm_stat[stat] = bread_number(buf);
                fMatch = TRUE;
                break;
            }
            break;
            
        case 'C':
            if (!str_cmp(word, "Clan") )
            {
                const char *temp=bread_string(buf);
                pet->clan=clan_lookup(temp);
                free_string(temp);
                fMatch=TRUE;
                break;
            }
            KEYF( "Comm",   pet->comm );
            break;
            
        case 'D':
	    if ( !strcmp(word, "DamDice") )
	    {
		pet->damage[DICE_NUMBER] = bread_number( buf );
		pet->damage[DICE_TYPE] = bread_number( buf );
		fMatch = TRUE;
		break;
	    }
            KEY( "Dam",    pet->damroll,       bread_number(buf));
            KEYS( "Desc",   pet->description,   bread_string(buf));
            break;
            
        case 'E':
            if (!str_cmp(word,"End"))
            {
                pet->leader = ch;
                pet->master = ch;
                ch->pet = pet;
                /* adjust hp mana move up  -- here for speed's sake */
                percent = (current_time - lastlogoff) * 25 / ( 2 * 60 * 60);
                
                if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
                    &&  !IS_AFFECTED(ch,AFF_PLAGUE))
                {
                    percent = UMIN(percent,100);
                    pet->hit    += (pet->max_hit - pet->hit) * percent / 100;
                    pet->mana   += (pet->max_mana - pet->mana) * percent / 100;
                    pet->move   += (pet->max_move - pet->move)* percent / 100;
                }

		/* money wipe */
		if ( pfile_version < VER_MONEY_WIPE )
		{
		    pet->gold = 0;
		    pet->silver = 0;
		}

                return;
            }
            KEY( "Exp",    pet->exp,       bread_number(buf));
            break;
            
        case 'G':
            KEY( "Gold",   pet->gold,      bread_number(buf));
            break;
            
        case 'H':
            KEY( "Hit",    pet->hitroll,       bread_number(buf));
            
            if (!str_cmp(word,"HMV"))
            {
                pet->hit    = bread_number(buf);
                pet->max_hit    = bread_number(buf);
                pet->mana   = bread_number(buf);
                pet->max_mana   = bread_number(buf);
                pet->move   = bread_number(buf);
                pet->max_move   = bread_number(buf);
                fMatch = TRUE;
                break;
            }
            break;
            
        case 'L':
            KEY( "Levl",   pet->level,     bread_number(buf));
            KEYS( "LnD",    pet->long_descr,    bread_string(buf));
            KEY( "LogO",   lastlogoff,     bread_number(buf));
            break;
            
        case 'N':
            KEYS( "Name",   pet->name,      bread_string(buf));
            break;
            
        case 'P':
            KEY( "Pos",    pet->position,      bread_number(buf));
            break;
            
        case 'R':
            if (!str_cmp(word, "Race") )
            {
                const char *temp=bread_string(buf);
                pet->race=race_lookup(temp);
                free_string(temp);
                fMatch=TRUE;
                break;
            }
            break;
            
        case 'S' :
            KEY( "Save",    pet->saving_throw,  bread_number(buf));
            KEY( "Sex",     pet->sex,       bread_number(buf));
            KEYS( "ShD",     pet->short_descr,   bread_string(buf));
            KEY( "Silv",        pet->silver,            bread_number( buf ) );
            break;
            
            if ( !fMatch )
            {
                bug("Bread_pet: no match.",0);
                bread_to_eol(buf);
            }
            
        }
    }
}

extern  OBJ_DATA    *obj_free;

void bread_obj( CHAR_DATA *ch, RBUFFER *buf,OBJ_DATA *storage_box )
{
    OBJ_DATA *obj;
    const char *word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool make_new;    /* update object */
    bool ignore_affects = FALSE; /* catch for old pfiles */
    bool done = FALSE;
    
    fVnum = FALSE;
    obj = NULL;
    first = TRUE;  /* used to counter buf offset */
    make_new = FALSE;
    
    word   = beof( buf ) ? "End" : bread_word( buf );
    if (!str_cmp(word,"Vnum" ))
    {
        int vnum;
        first = FALSE;  /* buf will be in right place */
        
        vnum = bread_number( buf );
        if (  get_obj_index( vnum )  == NULL )
        {
            bug( "Bread_obj: bad vnum %d.", vnum );
        }
        else
        {
            obj = create_object_vnum(vnum);
        }
        
    }
    
    if (obj == NULL)  /* either not found or old style */
    {
        obj = new_obj();
        obj->name       = str_dup( "" );
        obj->short_descr    = str_dup( "" );
        obj->description    = str_dup( "" );
    }
    
    fNest       = FALSE;
    fVnum       = TRUE;
    iNest       = 0;
    

    while ( !done )
    {
        if (first)
            first = FALSE;
        else
            word   = beof( buf ) ? "End" : bread_word( buf );
        fMatch = FALSE;
        
        switch ( UPPER(word[0]) )
        {
        case '*':
            fMatch = TRUE;
            bread_to_eol( buf );
            break;
            
        case 'A':
	    /* outdated --Bobble
            if (!str_cmp(word,"AffD"))
            {
                AFFECT_DATA *paf;
                int sn;
                
                paf = new_affect();
                
                sn = skill_lookup(bread_word(buf));
                if (sn < 0)
                    bug("Bread_obj: unknown skill.",0);
                else
                    paf->type = sn;
                
                paf->level  = bread_number( buf );
                paf->duration   = bread_number( buf );
                paf->modifier   = bread_number( buf );
                paf->location   = bread_number( buf );
                paf->bitvector  = bread_number( buf );
		paf->next   = obj->affected;

		if (ignore_affects)
		    free_affect( paf );
		else
		    obj->affected = paf;

                fMatch      = TRUE;
                break;
            }
	    */
            if (!str_cmp(word,"Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
                
                paf = new_affect();
                
                sn = skill_lookup(bread_word(buf));
                if (sn < 0)
                    bug("Bread_obj: unknown skill.",0);
                else
                    paf->type = sn;
                
                paf->where  = bread_number( buf );
                paf->level      = bread_number( buf );
                paf->duration   = bread_number( buf );
                paf->modifier   = bread_number( buf );
                paf->location   = bread_number( buf );
                paf->bitvector  = bread_number( buf );
                if ( pfile_version < VER_FLAG_CHANGE )
                    FLAG_CONVERT( paf->bitvector );
                if (ignore_affects)
                    free_affect( paf );
                else
                    obj->affected = affect_insert( obj->affected, paf );

                fMatch          = TRUE;
                break;
            }

            if (!str_cmp(word, "AffCust"))
            {
                AFFECT_DATA *paf;

                paf=new_affect();

                paf->type=gsn_custom_affect;
                paf->tag=bread_string(buf);
                paf->where=bread_number(buf);
                paf->level=bread_number(buf);
                paf->duration=bread_number(buf);
                paf->modifier=bread_number(buf);
                paf->location=bread_number(buf);
                paf->bitvector=bread_number(buf);

                if (ignore_affects)
                   free_affect( paf );
                else
                   obj->affected = affect_insert( obj->affected, paf );
                
                fMatch=TRUE;
                break;
            }

            break;
            
        case 'C':
        /*
            KEY( "Cond",    obj->condition, bread_number( buf ) );
            KEY( "Cost",    obj->cost,      bread_number( buf ) );
	    */
        if ( !str_cmp(word, "Cost") || !str_cmp(word, "Cond") )
        {
            /* ignore cost and condition */
            bread_number( buf );
            fMatch = TRUE;
            break;
        }
        if (!str_cmp(word, "Clan") )
        {
            const char *temp=bread_string(buf);
            obj->clan=clan_lookup(temp);
            free_string(temp);
            fMatch=TRUE;
            break;
        }
        if (!str_cmp(word, "CRank") )
        {
            const char *temp=bread_string(buf);
            obj->rank=clan_rank_lookup(obj->clan, temp);
            free_string(temp);
            fMatch=TRUE;
            break;
        }
            break;
            
        case 'D':
            KEYS( "Description", obj->description,   bread_string( buf ) );
            KEYS( "Desc",    obj->description,   bread_string( buf ) );
            /*
            KEY( "Dur",    obj->durability,   bread_number( buf ) );
            */
            if ( !str_cmp( word, "Dur" ) )
            {
                /* ignore durability */
                bread_number( buf );
                fMatch = TRUE;
                break;
            }
            break;
            
        case 'E':
            
            if ( !str_cmp( word, "Enchanted"))
            {
	        /* old pfile, affects are invalid */ 
	        ignore_affects = TRUE;
                fMatch  = TRUE;
                break;
            }
            
            KEYF( "ExtraFlags",  obj->extra_flags );
            KEYF( "ExtF",    obj->extra_flags );
            
            if ( !str_cmp( word, "ExtraDescr" ) || !str_cmp(word,"ExDe"))
            {
                EXTRA_DESCR_DATA *ed;
                
                ed = new_extra_descr();
                
                ed->keyword     = bread_string( buf );
                ed->description     = bread_string( buf );
                ed->next        = obj->extra_descr;
                obj->extra_descr    = ed;
                fMatch = TRUE;
                break;
            }
            
            if ( !str_cmp( word, "End" ) )
            {
                fMatch = TRUE;
                done = TRUE;
                if ( !fNest || ( fVnum && obj->pIndexData == NULL ) )
                {
                    bug( "Bread_obj: incomplete object.", 0 );
                    free_obj(obj);
                    /* make sure the nesting is cleared up too */
                    if ( rgObjNest[iNest] == obj )
                        rgObjNest[iNest] = NULL;
                    obj = NULL;
                    break;
                }
                else
                {
                    if ( !fVnum )
                    {
                        free_obj( obj );
                        obj = create_object_vnum(OBJ_VNUM_DUMMY);
                    }

                    if (make_new)
                    {
                        int wear;
                        
                        wear = obj->wear_loc;
                        extract_obj(obj);
                        
                        obj = create_object(obj->pIndexData);
                        obj->wear_loc = wear;
                    }

		    /* money wipe */
		    if ( pfile_version < VER_MONEY_WIPE
			 && obj->item_type == ITEM_MONEY )
		    {
                        obj->value[0] = 0;
                        obj->value[1] = 0;
		    }

                    if ( iNest == 0 || rgObjNest[iNest-1] == NULL )
                       if (storage_box != NULL)
                           obj_to_obj( obj, storage_box);
                       else
                            obj_to_char( obj, ch );
                    else
                        obj_to_obj( obj, rgObjNest[iNest-1] );
                }
            }
            break;
            
        case 'I':
            KEY( "ItemType",    obj->item_type,     bread_number( buf ) );
            KEY( "Ityp",    obj->item_type,     bread_number( buf ) );
            break;
            
        case 'L':
            KEY( "Level",   obj->level,     bread_number( buf ) );
            KEY( "Lev",     obj->level,     bread_number( buf ) );
            if ( !strcmp( word, "LuaVal" ) )
            {
                LUA_EXTRA_VAL *luaval;
                int type=bread_number( buf );
                const char *name=bread_string( buf );
                const char *val=bread_string( buf );
                luaval=new_luaval( type, name, val, TRUE );

                luaval->next=obj->luavals;
                obj->luavals=luaval;
                fMatch=TRUE;
                break;
            }
            break;
            
        case 'M':
            KEYS( "Mat",     obj->material,  bread_string(buf));
            break;
            
        case 'N':
            KEYS( "Name",    obj->name,      bread_string( buf ) );
            
            if ( !str_cmp( word, "Nest" ) )
            {
                iNest = bread_number( buf );
                if ( iNest < 0 || iNest >= MAX_NEST )
                {
                    bug( "Bread_obj: bad nest %d.", iNest );
                }
                else
                {
                    rgObjNest[iNest] = obj;
                    fNest = TRUE;
                }
                fMatch = TRUE;
                break;
            }
            break;
            
        case 'O':
            KEYS( "Owner",    obj->owner,      bread_string( buf ) );
                break;
            break;
            
            
        case 'S':
            KEYS( "ShortDescr",  obj->short_descr,   bread_string( buf ) );
            KEYS( "ShD",     obj->short_descr,   bread_string( buf ) );
            
            if ( !str_cmp( word, "Spell" ) )
            {
                int iValue;
                int sn;
                
                iValue = bread_number( buf );
                sn     = skill_lookup( bread_word( buf ) );
                if ( iValue < 0 || iValue > 3 )
                {
                    bug( "Bread_obj: bad iValue %d.", iValue );
                }
                else if ( sn < 0 )
                {
                    bug( "Bread_obj: unknown skill.", 0 );
                }
                else
                {
                    obj->value[iValue] = sn;
                }
                fMatch = TRUE;
                break;
            }
            
            break;
            
        case 'T':
            KEY( "Timer",   obj->timer,     bread_number( buf ) );
            KEY( "Time",    obj->timer,     bread_number( buf ) );
            break;
            
        case 'V':
            if ( !str_cmp( word, "Values" ) || !str_cmp(word,"Vals"))
            {
                obj->value[0]   = bread_number( buf );
                obj->value[1]   = bread_number( buf );
                obj->value[2]   = bread_number( buf );
                obj->value[3]   = bread_number( buf );
                if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
                    obj->value[0] = obj->pIndexData->value[0];
                fMatch      = TRUE;
                break;
            }
            
            if ( !str_cmp( word, "Val" ) )
            {
                obj->value[0]   = bread_number( buf );
                obj->value[1]   = bread_number( buf );
                obj->value[2]   = bread_number( buf );
                obj->value[3]   = bread_number( buf );
                obj->value[4]   = bread_number( buf );
                fMatch = TRUE;
                break;
            }
            
            if ( !str_cmp( word, "Vnum" ) )
            {
                int vnum;
                
                vnum = bread_number( buf );
                if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL )
                    bug( "Bread_obj: bad vnum %d.", vnum );
                else
                    fVnum = TRUE;
                fMatch = TRUE;
                break;
            }
            break;
            
        case 'W':
            /* handle old format with wear flags */
            if (!str_cmp( word, "WearFlags")
                    || !str_cmp( word, "WeaF"))
            {
                tflag wear;
                bread_tflag( buf, wear);
                int wear_type = (IS_SET(wear, ITEM_TAKE_OLD)) ?
                                 ITEM_CARRY : ITEM_NO_CARRY;

                int i;
                for ( i=0 ; wear_types[i].name ; i++ )
                {
                    int bit = wear_types[i].bit;
                    if (bit == ITEM_TRANSLUCENT_OLD
                            || bit == ITEM_NO_SAC_OLD
                            || bit == ITEM_TAKE_OLD )
                        continue;
                    if (IS_SET(wear, bit))
                    {
                        wear_type = bit;
                        break;
                    }
                }
                obj->wear_type = wear_type;

                fMatch = TRUE;
                break;
            }
            KEY( "WearLoc", obj->wear_loc,      bread_number( buf ) );
            KEY( "Wear",    obj->wear_loc,      bread_number( buf ) );
            KEY( "Weight",  obj->weight,        bread_number( buf ) );
            KEY( "Wt",      obj->weight,        bread_number( buf ) );
            break;
            
    }
    
    if ( !fMatch )
    {
        bug( "Bread_obj: no match.", 0 );
        bread_to_eol( buf );
    }
    }

    /* in case of old format, copy over extra flags that came from wear flags*/
    if ( obj ) {
        if ( IS_OBJ_STAT(obj->pIndexData, ITEM_TRANSLUCENT_EX) )
            SET_BIT(obj->extra_flags, ITEM_TRANSLUCENT_EX);
        if ( IS_OBJ_STAT(obj->pIndexData, ITEM_NO_SAC_EX) )
            SET_BIT(obj->extra_flags, ITEM_NO_SAC_EX);
    }
}



DEF_DO_FUN(do_finger)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char levelbuf[16];
    char clanbuf[MAX_STRING_LENGTH];
    char custombuf[MAX_STRING_LENGTH];
    char immbuf[16];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *wch;
    char pkstring[5];
    
    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("You must provide a name.\n\r",ch);
        return;
    }
    
    d = new_descriptor();
    
    if ( !load_char_obj(d, argument, TRUE) )
    {
        send_to_char("Character not found.\n\r", ch);
        /* load_char_obj still loads "default" character
           even if player not found, so need to free it */
        if (d->character)
        {
            free_char(d->character);
            d->character=NULL;
        }
        free_descriptor(d);
        return;
    }
    wch = d->character;
    output = new_buf();
    
    if (wch->pcdata->customflag[0]!='\0')
        sprintf(custombuf, "(%s) ", wch->pcdata->customflag);
    else
        custombuf[0] = '\0';
    
    if ( wch->level >= LEVEL_IMMORTAL )
        switch(wch->level)
    {
      case MAX_LEVEL - 0 : sprintf(immbuf, "IMPLEMENTOR"); break;
      case MAX_LEVEL - 1 : sprintf(immbuf, "ARCHON"); break;
      case MAX_LEVEL - 2 : sprintf(immbuf, "ARCHON"); break;
      case MAX_LEVEL - 3 : sprintf(immbuf, "VICE-ARCHON"); break;
      case MAX_LEVEL - 4 : sprintf(immbuf, "VICE-ARCHON"); break;
      case MAX_LEVEL - 5 : sprintf(immbuf, "GOD"); break;
      case MAX_LEVEL - 6 : sprintf(immbuf, "GOD"); break;
      case MAX_LEVEL - 7 : sprintf(immbuf, "DEMIGOD"); break;
      case MAX_LEVEL - 8 : sprintf(immbuf, "DEMIGOD"); break;
      case MAX_LEVEL - 9 : sprintf(immbuf, "SAVANT"); break;
      default : sprintf(immbuf, " "); break;
    }
    
    sprintf(levelbuf, "Level {c%d{x", wch->level);
    
    if (clan_table[wch->clan].active)
        sprintf(clanbuf, " [%s%s-%s{x] ", 
        clan_table[wch->clan].who_color,
        clan_table[wch->clan].who_name,
        clan_table[wch->clan].rank_list[wch->pcdata->clan_rank].who_name);
    else
        clanbuf[0] = '\0';
    
               
    if ( IS_SET(wch->act, PLR_PERM_PKILL) )
        sprintf( pkstring, "[%c]", get_pkflag(ch, wch) );
    else
        sprintf( pkstring, " " );
    
    
    /* AND NOW the formatting! */
    
    sprintf(buf,"{D\n\r:===================================================================:{x\n\r" );
    add_buf( output, buf );
    
    /* **  RP, Helper, Killer, Thief, pflag, name, title ** */
    sprintf(buf, "  %s%s%s%s%s%s%s%s{x%s\n\r",
            IS_SET(wch->act,PLR_RP) ? "(RP) " : "",
            IS_SET(wch->act,PLR_HELPER) ? "({GH{CE{cL{GP{CE{cR{x) " : "",
            IS_SET(wch->act,PLR_KILLER) ? "(KILLER) " : "",
            IS_SET(wch->act,PLR_THIEF) ? "(THIEF) " : "",
            custombuf,
	    IS_NPC(wch)?"":wch->pcdata->name_color,
	    IS_NPC(wch)?"":wch->pcdata->pre_title,
            wch->name, IS_NPC(wch) ? "" : wch->pcdata->title);
    add_buf( output, buf );
    
    sprintf(buf,"{D:===================================================================:{x\n\r" );
    add_buf( output, buf );
    
    /* ** Incog, Wizi, AFK, Levelbuf, Sex, Race, Class, pkill ** */
    sprintf(buf, "{D|{x %s%s%s%s %s %s %s.            %s%s",
        get_trust(ch) >= wch->incog_level &&
        wch->incog_level >= LEVEL_HERO ? "(Incog) ": "",
        get_trust(ch) >= wch->invis_level &&
        wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
        IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
        levelbuf,
        wch->sex == 0 ? "sexless" : wch->sex == 1 ? "male" : "female",
        wch->race < MAX_PC_RACE ? pc_race_table[wch->race].name : "     ",
        class_table[wch->class].name,
        clanbuf,
        pkstring);
    for ( ; strlen_color(buf) <= 67; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );

    /* revised religion */
    sprintf( buf, "{D|{x God:     %-11s Rank: %-15s",
             has_god(wch) ? get_god_name(wch) : "None",
             get_ch_rank_name(wch) );
    
    if( wch->pcdata && wch->pcdata->spouse )
        sprintf( buf2, "Spouse: %-12s", wch->pcdata->spouse );
    else
        sprintf( buf2, "Spouse: None" );
        
    strcat( buf, buf2 );
    
    for( ; strlen_color(buf) <= 67; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );
    
    /* Remorts, Age, Hours, Bounty */
    sprintf(buf, "{D|{x ");
    if ( wch->level <= LEVEL_HERO )
    {
        sprintf(buf2, "Remorts: {c%-2d{x          Age: %-3d",
            wch->pcdata->remorts,
            get_age(wch));
        strcat( buf, buf2 );
    }
    else
    {
        sprintf( buf2, " *** %s ***      ", immbuf );
        strcat( buf, buf2 );
    }
    
    if ( get_trust(ch) > LEVEL_IMMORTAL )
    {
        sprintf( buf2, "             Hours: {c%d{x   ", ((int)wch->played)/3600);
        strcat( buf, buf2 );
    }
    if ( wch->pcdata->bounty )
    {
        sprintf( buf2, "     Bounty: %d", wch->pcdata->bounty );
        strcat( buf, buf2 );
    }
    for ( ; strlen_color(buf) <= 67; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );
    
    /* ascent and subclass */
    if ( wch->level <= LEVEL_HERO && wch->pcdata->ascents > 0 )
    {
        sprintf(buf, "{D|{x ");
        sprintf(buf2, "Ascents: {c%-2d{x     Subclass: %s",
            wch->pcdata->ascents,
            subclass_table[wch->pcdata->subclass].name);
        strcat( buf, buf2 );
        for ( ; strlen_color(buf) <= 67; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );        
    }
    
    /* Last on */
    if ( wch->level < LEVEL_IMMORTAL || IS_IMMORTAL(ch) )
    {
        if ( IS_IMMORTAL(wch) && ch->level <= wch->level )
            ;  /* Do nothing. */
        else
        {
        sprintf(buf, "{D|{x Date Last On: %s    ",
            time_format(fingertime, custombuf));
        for ( ; strlen_color(buf) <= 67; strcat( buf, " " ));
        strcat( buf, "{D|{x\n\r" );
        add_buf( output, buf );
        }
    }
	
    /* Date Created */
    sprintf(buf, "{D|{x Date Created: %s   ",
	    time_format(wch->id, custombuf));
    for ( ; strlen_color(buf) <= 67; strcat( buf, " " ));
    strcat( buf, "{D|{x\n\r" );
    add_buf( output, buf );

    if ( get_trust(ch) > GOD )
    {
        if (IS_IMMORTAL(wch) && ch->level <= wch->level)
        {
            ; /* Do nothing */
        }
        else
        {
            sprintf(buf, "{D|{x Last host: 	<send 'pgrep %15s'>%s	</send>", wch->pcdata->last_host, wch->pcdata->last_host);
            for ( ; strlen_color(buf) <= 106; strcat( buf, " " ));
            strcat( buf, "{D|{x\n\r" );
            add_buf( output, buf );
        }
    }
    
    
    if (wch->level <= LEVEL_HERO)
    {
        int war;

        int wars_won = wch->pcdata->armageddon_won
            + wch->pcdata->clan_won
            + wch->pcdata->class_won
            + wch->pcdata->race_won
            + wch->pcdata->religion_won
            + wch->pcdata->gender_won
            + wch->pcdata->duel_won;
        int wars_lost = wch->pcdata->armageddon_lost
            + wch->pcdata->clan_lost
            + wch->pcdata->class_lost
            + wch->pcdata->race_lost
            + wch->pcdata->religion_lost
            + wch->pcdata->gender_lost
            + wch->pcdata->duel_lost;
        
        /*
        if( wch->pcdata->pkpoints == 0 )
            pk = get_pkgrade_level(wch->pcdata->pkill_count);
        else
            pk = get_pkgrade_level(wch->pcdata->pkpoints);
        */

	if( wch->pcdata->warpoints == 0 )
	    war = get_pkgrade_level(wch->pcdata->war_kills);
	else
	    war = get_pkgrade_level(wch->pcdata->warpoints);

        sprintf(buf,"{D:===================================================================:{x\n\r" );
        add_buf( output, buf );
     
        sprintf(buf,
	    "{D|{x                         {D|{x Warfare Grade: {W<<%s{W>>{x  Total Wars: %5d{x {D|{x\n\r",
			pkgrade_table[war].grade, wars_won + wars_lost );
        add_buf( output, buf ); 
         
        sprintf(buf,
	    "{D|{x          Pkills:  {C%5d{x {D|{x                   {D|=={xWON{D=|={xLOST{D=|={xKILLS{D=|{x\n\r",
			wch->pcdata->pkill_count );
        add_buf( output, buf );

        sprintf(buf,
	    "{D|{x     Mobs Killed: {R%6d{x {D|{x      {rA{Drmageddons:{x {D|{x {r%4d{x {D|{x {r%4d{x {D|{x {r%5d{x {D|{x\n\r",
             wch->pcdata->mob_kills, wch->pcdata->armageddon_won, wch->pcdata->armageddon_lost, wch->pcdata->armageddon_kills );
        add_buf( output, buf ); 

        sprintf(buf,
	    "{D|{x         Beheads:  {R%5d{D |{x        {yC{Dlan {yW{Dars:{x {D|{x {y%4d{x {D|{x {y%4d{x {D|{x {y%5d{x {D|{x\n\r",
            wch->pcdata->behead_cnt, wch->pcdata->clan_won, wch->pcdata->clan_lost, wch->pcdata->clan_kills );
        add_buf( output, buf );
            
        sprintf(buf,
	    "{D|=========================|{x       {gC{Dlass {gW{Dars:{x {D|{x {g%4d{x {D|{x {g%4d{x {D|{x {g%5d{x {D|{x\n\r",
	    wch->pcdata->class_won, wch->pcdata->class_lost, wch->pcdata->class_kills );
        add_buf( output, buf );
            
        sprintf(buf,
	    "{D|{x Quests Complete: %6d {D|{x        {cR{Dace {cW{Dars:{x {D|{x {c%4d{x {D|{x {c%4d{x {D|{x {c%5d{x {D|{x\n\r", 
	    wch->pcdata->quest_success, wch->pcdata->race_won, wch->pcdata->race_lost, wch->pcdata->race_kills );
        add_buf( output, buf );
            
        sprintf(buf,
	    "{D|{x Hard Qst Complt: %6d {D|{x      {bR{Delign {bW{Dars:{x {D|{x {b%4d{x {D|{x {b%4d{x {D|{x {b%5d{x {D|{x\n\r",
	    wch->pcdata->quest_hard_success, wch->pcdata->religion_won, wch->pcdata->religion_lost, wch->pcdata->religion_kills );
	add_buf( output, buf );

	sprintf(buf,
	    "{D|{x   Quests Failed:  %5d {D|{x      {mG{Dender {mW{Dars:{x {D|{x {m%4d{x {D|{x {m%4d{x {D|{x {m%5d{x {D|{x\n\r",
		wch->pcdata->quest_failed, wch->pcdata->gender_won, wch->pcdata->gender_lost, wch->pcdata->gender_kills );
	add_buf( output, buf );

    sprintf( buf,
        "{D|{x                         {D|{x        {DDuel Wars: | %4d | %4d | %5d |{x\n\r",
        wch->pcdata->duel_won, wch->pcdata->duel_lost, wch->pcdata->duel_kills);
    add_buf( output, buf );

	sprintf(buf,
	    "{D|{x Percent Success: %5.1f%% {D|{x           TOTALS:{x {D|{x%5d{x {D|{x%5d{x {D|{x%6d{x {D|{x\n\r",
            wch->pcdata->quest_success == 0 ? 0 : (float)wch->pcdata->quest_success * 100 /
            (float)(wch->pcdata->quest_failed + wch->pcdata->quest_success),
            wars_won, wars_lost,
	    wch->pcdata->war_kills );
        add_buf( output, buf );
        
    }
    
    sprintf(buf,"{D:===================================================================:{x\n\r" );
    add_buf( output, buf );
    
    page_to_char(buf_string(output),ch);
    free_buf(output);
    
    nuke_pets(wch);
    free_char(wch);
    free_descriptor(d);
    
    return;
}
