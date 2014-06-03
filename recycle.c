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

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "lua_arclib.h"

void free_quest(QUEST_DATA *quest);

/* Allocate memory for a new note or recycle (Erwin's version) */
NOTE_DATA *new_note ()
{
	NOTE_DATA *note;
	note = lua_new_ud(type_NOTE_DATA);
	return note;
}

void free_note(NOTE_DATA *note)
{
	if (!IS_VALID(note))
	return;

	free_string( note->text    );
	free_string( note->subject );
	free_string( note->to_list );
	free_string( note->date    );
	free_string( note->sender  );
    
    lua_free_ud( note );
}

	
BAN_DATA *new_ban(void)
{
	BAN_DATA *ban;

	ban = lua_new_ud( type_BAN_DATA );
	ban->name = &str_empty[0];
	return ban;
}

void free_ban(BAN_DATA *ban)
{
	if (!IS_VALID(ban))
	    return;

	free_string(ban->name);

	ban->next = NULL;
    lua_free_ud( ban );
}

DESCRIPTOR_DATA *new_descriptor(void)
{
	DESCRIPTOR_DATA *d;

	d = lua_new_ud( type_DESCRIPTOR_DATA );
	
	d->connected    = CON_GET_NAME;
	d->outsize  = 2000;
	d->outbuf   = alloc_mem( d->outsize );
    d->pProtocol= ProtocolCreate();

    d->lua.interpret=FALSE;
    d->lua.incmpl=FALSE;

#ifdef LAG_FREE
    d->lag_free=FALSE;
#endif
   
	return d;
}

void free_descriptor(DESCRIPTOR_DATA *d)
{
	if (!IS_VALID(d))
	return;

    lua_unregister_desc(d);
	free_string( d->host );
	free_mem( d->outbuf, d->outsize );
    ProtocolDestroy( d->pProtocol );
	d->next = NULL;
    lua_free_ud( d );
}

GEN_DATA *new_gen_data(void)
{
	GEN_DATA *gen;

	gen = lua_new_ud( type_GEN_DATA );
	return gen;
}

void free_gen_data(GEN_DATA *gen)
{
	if (!IS_VALID(gen))
	return;

	gen->next = NULL;
    lua_free_ud( gen );
} 

EXTRA_DESCR_DATA *new_extra_descr(void)
{
	EXTRA_DESCR_DATA *ed;

	ed = lua_new_ud( type_EXTRA_DESCR_DATA );
	
    ed->keyword = &str_empty[0];
	ed->description = &str_empty[0];
	return ed;
}

void free_extra_descr(EXTRA_DESCR_DATA *ed)
{
	if (!IS_VALID(ed))
	return;

	free_string(ed->keyword);
	free_string(ed->description);
	ed->next = NULL;
    lua_free_ud( ed );
}

AFFECT_DATA *new_affect(void)
{
	AFFECT_DATA *af;
	af = lua_new_ud( type_AFFECT_DATA );
	return af;
}

void free_affect(AFFECT_DATA *af)
{
	if (!IS_VALID(af))
	return;

	af->next = NULL; 
    lua_free_ud( af );
}

OBJ_DATA *new_obj(void)
{
	OBJ_DATA *obj;
	obj = lua_new_ud( type_OBJ_DATA );
	return obj;
}

void free_obj(OBJ_DATA *obj)
{
	AFFECT_DATA *paf, *paf_next;
	EXTRA_DESCR_DATA *ed, *ed_next;

	if (!IS_VALID(obj))
	return;

    object_count--;

	for (paf = obj->affected; paf != NULL; paf = paf_next)
	{
	    paf_next = paf->next;
	    free_affect(paf);
	}
	obj->affected = NULL;

	for (ed = obj->extra_descr; ed != NULL; ed = ed_next )
	{
	    ed_next = ed->next;
	    free_extra_descr(ed);
	 }
	 obj->extra_descr = NULL;
   
	free_string( obj->name        );
	free_string( obj->description );
	free_string( obj->short_descr );
	free_string( obj->owner     );
    free_string( obj->material  );

    LUA_EXTRA_VAL *luaval;
    LUA_EXTRA_VAL *luaval_next=NULL;
    for ( luaval=obj->luavals; luaval; luaval=luaval_next )
    {
        luaval_next=luaval->next;
        free_luaval(luaval);
    }

    obj->next=NULL;
    lua_free_ud( obj );
}

CHAR_DATA *new_char (void)
{
	CHAR_DATA *ch;
	int i;

	ch = lua_new_ud( type_CHAR_DATA ); 
	
	ch->name                    = &str_empty[0];
	ch->short_descr             = &str_empty[0];
	ch->long_descr              = &str_empty[0];
	ch->description             = &str_empty[0];
	ch->prompt                  = &str_empty[0];
	ch->prefix          = &str_empty[0];
	ch->logon                   = current_time;
	ch->lines                   = PAGELEN;
	for (i = 0; i < 4; i++)
		ch->armor[i]            = 100;
	ch->hunting					= NULL;
	ch->aggressors				= NULL;
    ch->pet                     = NULL;
	ch->position                = POS_STANDING;
	ch->hit                     = 100;
	ch->max_hit                 = 100;
	ch->mana                    = 100;
	ch->max_mana                = 100;
	ch->move                    = 100;
	ch->max_move                = 100;
	ch->stance=0;
	ch->just_killed = FALSE;
	ch->must_extract = FALSE;
    ch->trig_timer              = NULL;
    ch->luavals                 = NULL;
	
	for (i = 0; i < MAX_STATS; i ++)
	{
		ch->perm_stat[i] = 50;
		ch->mod_stat[i] = 0;
	}

	return ch;
}


void free_char (CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;


	if (!IS_VALID(ch))
	    return;

	if (IS_NPC(ch))
    {
	   mobile_count--;
    }
	/* Erwin's suggested fix to light problem */ 
	else if ( ch->in_room != NULL )
	{
	   /*log_string( "Setting ch->in_room to null for light bug.");*/
	   ch->in_room = NULL;
	}    

	for (obj = ch->carrying; obj != NULL; obj = obj_next)
	{
	obj_next = obj->next_content;
	extract_obj(obj);
	}

	for (paf = ch->affected; paf != NULL; paf = paf_next)
	{
	    paf_next = paf->next;
	    affect_remove(ch,paf);
	}

	free_string(ch->hunting);
	forget_attacks(ch);

	free_string(ch->name);
	free_string(ch->short_descr);
	free_string(ch->long_descr);
	free_string(ch->description);
	free_string(ch->prompt);
	free_string(ch->prefix);

    LUA_EXTRA_VAL *luaval;
    LUA_EXTRA_VAL *luaval_next=NULL;
    for ( luaval=ch->luavals; luaval; luaval=luaval_next )
    {
        luaval_next=luaval->next;
        free_luaval( luaval );
    }

	if (ch->pcdata != NULL)
	    free_pcdata(ch->pcdata);

	ch->next = NULL; 

    lua_free_ud( ch );
	return;
}

PC_DATA *new_pcdata(void)
{
    int alias, i;
    
    PC_DATA *pcdata;
    
    pcdata = lua_new_ud( type_PC_DATA );
    
    for (alias = 0; alias < MAX_ALIAS; alias++)
    {
        pcdata->alias[alias] = NULL;
        pcdata->alias_sub[alias] = NULL;
    }
    
    for (i = 0; i < MAX_STATS; i ++)
    {
        pcdata->original_stats[i] = 50;
        pcdata->history_stats[i] = 0;
    }
    
    pcdata->gtell_history	    = pers_history_new();
    pcdata->tell_history	    = pers_history_new();
    pcdata->clan_history	    = pers_history_new();
    pcdata->explored = (EXPLORE_DATA *)calloc(1, sizeof(*(pcdata->explored) ) ); //Allocate explored data

    return pcdata;
}
	

void free_pcdata(PC_DATA *pcdata)
{
    int alias, i;
    GRANT_DATA *gran, *gran_next;
    QUEST_DATA *qdata;
    
    if (!IS_VALID(pcdata))
        return;
    
    free_string(pcdata->pwd);
    free_string(pcdata->bamfin);
    free_string(pcdata->bamfout);
    free_string(pcdata->title);
    free_string(pcdata->authed_by);
   // free_buf(pcdata->buffer);

    for (i = 0; i < MAX_CLAN; i++)
        free_string(pcdata->invitation[i]);
    
    for (alias = 0; alias < MAX_ALIAS; alias++)
    {
        free_string(pcdata->alias[alias]);
        free_string(pcdata->alias_sub[alias]);
    }
    free_string(pcdata->combat_action);
    free_string(pcdata->name_color);
    free_string(pcdata->pre_title);
    free_string(pcdata->last_host);
    free_string(pcdata->customflag);
    free_string(pcdata->spouse);

    
    for (i=0; i<MAX_FORGET ; i++)
    {
        free_string(pcdata->forget[i]);
    }

    for (gran = pcdata->granted; gran != NULL; gran = gran_next)
    {
        gran_next = gran->next;
        free_string(gran->name);
        free_mem(gran,sizeof(*gran));
    }

    for ( qdata = pcdata->qdata; qdata != NULL; qdata = pcdata->qdata )
    {
	pcdata->qdata = qdata->next;
	free_quest( qdata );
    }

    {   EXPLORE_HOLDER *pExp, *e_next;
        for(pExp = pcdata->explored->buckets ; pExp ; pExp = e_next )
        {    e_next = pExp->next;
             free(pExp);
        }
    }

    pers_history_free(pcdata->gtell_history);
    pers_history_free(pcdata->tell_history);
    pers_history_free(pcdata->clan_history);

    CRIME_DATA *crime, *crime_next;
    for ( crime=pcdata->crimes; crime ; crime=crime_next )
    {
        crime_next=crime->next;
        free_crime(crime);
    }

    lua_free_ud( pcdata );
    
    return;
}

QUEST_DATA *new_quest(void)
{
    QUEST_DATA *quest;
	quest=lua_new_ud( type_QUEST_DATA ); 
    return quest;
}

void free_quest(QUEST_DATA *quest)
{
    if (!IS_VALID(quest))
	return;
    
    quest->next = NULL;
    lua_free_ud( quest );
}

PORTAL_DATA *new_portal( void )
{
    PORTAL_DATA *portal;
	portal = lua_new_ud( type_PORTAL_DATA );
    portal->name = &str_empty[0];
    return portal;
}

void free_portal( PORTAL_DATA *portal )
{
    if (!IS_VALID(portal))
	return;
    
    free_string( portal->name );

    portal->next = NULL;
    lua_free_ud( portal );
}

/* stuff for setting ids */
long    last_pc_id;
long    last_mob_id;

long get_pc_id(void)
{
	int val;

	val = (current_time <= last_pc_id) ? last_pc_id + 1 : current_time;
	last_pc_id = val;
	return val;
}

long get_mob_id(void)
{
	last_mob_id++;
	return last_mob_id;
}

MEM_DATA *new_mem_data(void)
{
	MEM_DATA *memory;
	memory = lua_new_ud( type_MEM_DATA );
	return memory;
}

void free_mem_data(MEM_DATA *memory)
{
	if (!IS_VALID(memory))
	return;

	memory->next = NULL;
    lua_free_ud( memory );
}



/* buffer sizes */
/* Added 32767 on 12-21-12 - Astark. Not certain what affects this will
   have on memory usage, but it allows a full output from the alist
   command using the new formatting */
const int buf_size[MAX_BUF_LIST] =
{
	16,32,64,128,256,1024,2048,4096,8192,16384,32767
};

/* local procedure for finding the next acceptable size */
/* -1 indicates out-of-boundary error */
int get_size (int val)
{
	int i;

	for (i = 0; i < MAX_BUF_LIST; i++)
	if (buf_size[i] >= val)
	{
		return buf_size[i];
	}
	
	return -1;
}

BUFFER *new_buf()
{
	BUFFER *buffer;

	buffer = lua_new_ud( type_BUFFER );

	buffer->next    = NULL;
	buffer->state   = BUFFER_SAFE;
	buffer->size    = get_size(BASE_BUF);

	buffer->string  = alloc_mem(buffer->size);
	buffer->string[0]   = '\0';

	return buffer;
}

BUFFER *new_buf_size(int size)
{
	BUFFER *buffer;
 
	buffer = lua_new_ud( type_BUFFER ); 
 
	buffer->next        = NULL;
	buffer->state       = BUFFER_SAFE;
	buffer->size        = get_size(size);
	if (buffer->size == -1)
	{
		bug("new_buf: buffer size %d too large.",size);
		exit(1);
	}
	buffer->string      = alloc_mem(buffer->size);
	buffer->string[0]   = '\0';
	return buffer;
}


void free_buf(BUFFER *buffer)
{
	if (!IS_VALID(buffer))
	return;

	free_mem(buffer->string,buffer->size);
	buffer->string = NULL;
	buffer->size   = 0;
	buffer->state  = BUFFER_FREED;

	buffer->next  = NULL;
    lua_free_ud( buffer );
}


bool add_buf(BUFFER *buffer, char *string)
{
	int len;
	char *oldstr;
	int oldsize;

	oldstr = buffer->string;
	oldsize = buffer->size;

	if (buffer->state == BUFFER_OVERFLOW) /* don't waste time on bad strings! */
	return FALSE;

	len = strlen(buffer->string) + strlen(string) + 1;

	while (len >= buffer->size) /* increase the buffer size */
	{
	buffer->size    = get_size(buffer->size + 1);
	{
		if (buffer->size == -1) /* overflow */
		{
		buffer->size = oldsize;
		buffer->state = BUFFER_OVERFLOW;
		bug("buffer overflow past size %d",buffer->size);
		return FALSE;
		}
	}
	}

	if (buffer->size != oldsize)
	{
	buffer->string  = alloc_mem(buffer->size);

	strcpy(buffer->string,oldstr);
	free_mem(oldstr,oldsize);
	}

	strcat(buffer->string,string);
	return TRUE;
}


void clear_buf(BUFFER *buffer)
{
	buffer->string[0] = '\0';
	buffer->state     = BUFFER_SAFE;
}


char *buf_string(BUFFER *buffer)
{
	return buffer->string;
}

PROG_LIST *new_mprog(void)
{
   PROG_LIST *mp;
   mp = lua_new_ud( type_MPROG_LIST );
   return mp;
}

void free_mprog(PROG_LIST *mp)
{
   if (!IS_VALID(mp))
	  return;

   mp->next = NULL;
   lua_free_ud( mp );
}

PROG_LIST *new_oprog(void)
{
   PROG_LIST *op;
   op = lua_new_ud( type_OPROG_LIST ); 
   return op;
}

void free_oprog(PROG_LIST *op)
{
   if (!IS_VALID(op))
      return;

   op->next = NULL;
   lua_free_ud( op );
}

PROG_LIST *new_aprog(void)
{
   PROG_LIST *ap;
   ap = lua_new_ud( type_APROG_LIST );
   return ap;
}

void free_aprog(PROG_LIST *ap)
{
   if (!IS_VALID(ap))
      return;

   ap->next = NULL;
   lua_free_ud( ap );
}

PROG_LIST *new_rprog(void)
{
    PROG_LIST *rp;
    rp = lua_new_ud( type_RPROG_LIST );
    return rp;
}

void free_rprog(PROG_LIST *rp)
{
    if (!IS_VALID(rp))
        return;

    rp->next = NULL;
    lua_free_ud( rp );
}

HELP_AREA * had_free;

HELP_AREA * new_had ( void )
{
   HELP_AREA * had;
   
   if ( had_free )
   {
	  had       = had_free;
	  had_free  = had_free->next;
   }
   else
	  had       = alloc_perm( sizeof( *had ) );
   
   return had;
}

HELP_DATA * help_free;

HELP_DATA * new_help ( void )
{
   HELP_DATA * help;
   
   if ( help_free == NULL )
	  help       = alloc_perm( sizeof( *help ) );
   else
   {
	  help       = help_free;
	  help_free = help_free->next;
   }

   help->level   = 0;
   help->keyword = str_dup("");
   help->text    = str_dup("");
   help->next    = NULL;
   help->next_area = NULL;

   return help;
}

SORT_TABLE *new_sort(void)
{
    SORT_TABLE *sort = lua_new_ud( type_SORT_TABLE );
    return sort;
}

void free_sort(SORT_TABLE *sort)
{
	if (!IS_VALID(sort))
	return;

	sort->next  = NULL;
    lua_free_ud( sort );
}

WIZ_DATA *new_wiz(void)
{
    WIZ_DATA *wiz;
    
    wiz = lua_new_ud( type_WIZ_DATA );
    wiz->name = &str_empty[0];
    return wiz;
}

void free_wiz(WIZ_DATA *wiz)
{
    if (!IS_VALID(wiz))
        return;
    
    free_string(wiz->name);
    wiz->next = NULL;
    lua_free_ud( wiz );
}

CRIME_DATA *new_crime(void)
{
    CRIME_DATA *crime;
    crime = lua_new_ud( type_CRIME_DATA );
    return crime;
}

void free_crime(CRIME_DATA *crime)
{
    if (!IS_VALID(crime))
        return;
    
    free_string(crime->name);
    free_string(crime->desc);
    free_string(crime->imm_name);
    
    crime->next = NULL;
    lua_free_ud( crime );
}

