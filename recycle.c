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

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"

void free_quest(QUEST_DATA *quest);

/* stuff for recyling notes */
NOTE_DATA *note_free;

/* Allocate memory for a new note or recycle (Erwin's version) */
NOTE_DATA *new_note ()
{
	NOTE_DATA *note;
	
	if (note_free)
	{
		note = note_free;
		note_free = note_free->next;
	}
	else
		note = alloc_mem (sizeof(NOTE_DATA));

	/* Zero all the field - Envy does not gurantee zeroed memory */ 
	note->next = NULL;
	note->sender = NULL;        
	note->expire = 0;
	note->to_list = NULL;
	note->subject = NULL;
	note->date = NULL;
	note->date_stamp = 0;
	note->text = NULL;

	  VALIDATE(note);   
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
	INVALIDATE(note);

	note->next = note_free;
	note_free   = note;
}

	
/* stuff for recycling ban structures */
BAN_DATA *ban_free;

BAN_DATA *new_ban(void)
{
	static BAN_DATA ban_zero;
	BAN_DATA *ban;

	if (ban_free == NULL)
	ban = alloc_perm(sizeof(*ban));
	else
	{
	ban = ban_free;
	ban_free = ban_free->next;
	}

	*ban = ban_zero;
	VALIDATE(ban);
	ban->name = &str_empty[0];
	return ban;
}

void free_ban(BAN_DATA *ban)
{
	if (!IS_VALID(ban))
	return;

	free_string(ban->name);
	INVALIDATE(ban);

	ban->next = ban_free;
	ban_free = ban;
}

/* stuff for recycling descriptors */
DESCRIPTOR_DATA *descriptor_free;

DESCRIPTOR_DATA *new_descriptor(void)
{
	static DESCRIPTOR_DATA d_zero;
	DESCRIPTOR_DATA *d;

	if (descriptor_free == NULL)
	d = alloc_perm(sizeof(*d));
	else
	{
	d = descriptor_free;
	descriptor_free = descriptor_free->next;
	}
	
	*d = d_zero;
	VALIDATE(d);
	
	d->connected    = CON_GET_NAME;
	d->showstr_head = NULL;
	d->showstr_point = NULL;
	d->outsize  = 2000;
	d->pEdit        = NULL;         /* OLC */
	d->pString  = NULL;         /* OLC */
	d->editor   = 0;            /* OLC */
	d->outbuf   = alloc_mem( d->outsize );
   
	return d;
}

void free_descriptor(DESCRIPTOR_DATA *d)
{
	if (!IS_VALID(d))
	return;

	free_string( d->host );
	free_string( d->username );
	free_string( d->ftp.data );
	free_string( d->ftp.filename );
	free_mem( d->outbuf, d->outsize );
	INVALIDATE(d);
	d->next = descriptor_free;
	descriptor_free = d;
}

/* stuff for recycling who_data */
WHO_DATA *who_data_free;

WHO_DATA *new_who_data(void)
{
	static WHO_DATA who_zero;
	WHO_DATA *who;

	if( who_data_free == NULL)
	  who = alloc_perm(sizeof(*who));
	else
	{
	  who = who_data_free;
	  who_data_free = who_data_free->next;
	}
	*who = who_zero;
	VALIDATE(who);
	return who;
}

void free_who_data(WHO_DATA *who)
{
	if (!IS_VALID(who))
	return;

	INVALIDATE(who);

	who->next = who_data_free;
	who_data_free = who;
} 


/* stuff for recycling gen_data */
GEN_DATA *gen_data_free;

GEN_DATA *new_gen_data(void)
{
	static GEN_DATA gen_zero;
	GEN_DATA *gen;

	if (gen_data_free == NULL)
	gen = alloc_perm(sizeof(*gen));
	else
	{
	gen = gen_data_free;
	gen_data_free = gen_data_free->next;
	}
	*gen = gen_zero;
	VALIDATE(gen);
	return gen;
}

void free_gen_data(GEN_DATA *gen)
{
	if (!IS_VALID(gen))
	return;

	INVALIDATE(gen);

	gen->next = gen_data_free;
	gen_data_free = gen;
} 

/* stuff for recycling extended descs */
EXTRA_DESCR_DATA *extra_descr_free;

EXTRA_DESCR_DATA *new_extra_descr(void)
{
	EXTRA_DESCR_DATA *ed;

	if (extra_descr_free == NULL)
	ed = alloc_perm(sizeof(*ed));
	else
	{
	ed = extra_descr_free;
	extra_descr_free = extra_descr_free->next;
	}

	ed->keyword = &str_empty[0];
	ed->description = &str_empty[0];
	VALIDATE(ed);
	return ed;
}

void free_extra_descr(EXTRA_DESCR_DATA *ed)
{
	if (!IS_VALID(ed))
	return;

	free_string(ed->keyword);
	free_string(ed->description);
	INVALIDATE(ed);
	
	ed->next = extra_descr_free;
	extra_descr_free = ed;
}


/* stuff for recycling affects */
AFFECT_DATA *affect_free;

AFFECT_DATA *new_affect(void)
{
	static AFFECT_DATA af_zero;
	AFFECT_DATA *af;

	if (affect_free == NULL)
	af = alloc_perm(sizeof(*af));
	else
	{
	af = affect_free;
	affect_free = affect_free->next;
	}

	*af = af_zero;

	VALIDATE(af);
	af->next = NULL;
	af->detect_level = 0;
	
	return af;
}

void free_affect(AFFECT_DATA *af)
{
	if (!IS_VALID(af))
	return;

	INVALIDATE(af);
	af->next = affect_free;
	affect_free = af;
}

/* stuff for recycling objects */
OBJ_DATA *obj_free;

OBJ_DATA *new_obj(void)
{
	static OBJ_DATA obj_zero;
	OBJ_DATA *obj;

	if (obj_free == NULL)
	obj = alloc_perm(sizeof(*obj));
	else
	{
	obj = obj_free;
	obj_free = obj_free->next;
	}
	*obj = obj_zero;
	VALIDATE(obj);

	return obj;
}

void free_obj(OBJ_DATA *obj)
{
	AFFECT_DATA *paf, *paf_next;
	EXTRA_DESCR_DATA *ed, *ed_next;

	if (!IS_VALID(obj))
	return;

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
	INVALIDATE(obj);

	obj->next   = obj_free;
	obj_free    = obj; 
}


/* stuff for recyling characters */
CHAR_DATA *char_free;

CHAR_DATA *new_char (void)
{
	static CHAR_DATA ch_zero;
	CHAR_DATA *ch;
	int i;

	if (char_free == NULL)
	ch = alloc_perm(sizeof(*ch));
	else
	{
	ch = char_free;
	char_free = char_free->next;
	}

	*ch             = ch_zero;
	VALIDATE(ch);
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
	   mobile_count--;

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


	if (ch->pcdata != NULL)
	    free_pcdata(ch->pcdata);

	ch->next = char_free;
	char_free  = ch;

	INVALIDATE(ch);
	return;
}

PC_DATA *pcdata_free;

PC_DATA *new_pcdata(void)
{
    int alias, i;
    
    static PC_DATA pcdata_zero;
    PC_DATA *pcdata;
    
    if (pcdata_free == NULL)
        pcdata = alloc_perm(sizeof(*pcdata));
    else
    {
        pcdata = pcdata_free;
        pcdata_free = pcdata_free->next;
    }
    
    *pcdata = pcdata_zero;
    
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
    
    pcdata->buffer = new_buf();
    pcdata->pkill_count = 0;
    pcdata->pkill_deaths = 0;
    pcdata->pkpoints = 0;
    pcdata->remorts = 0;
    pcdata->authed_by = NULL;
    
    pcdata->armageddon_won            = 0;
    pcdata->armageddon_lost           = 0;
    pcdata->clan_won                = 0;
    pcdata->clan_lost               = 0;
    pcdata->race_won                = 0;
    pcdata->race_lost               = 0;
    pcdata->class_won               = 0;
    pcdata->class_lost              = 0;
    pcdata->total_wars              = 0;
    pcdata->war_kills               = 0;
    pcdata->warpoints               = 0;
    pcdata->mob_kills               = 0;
    pcdata->mob_deaths              = 0;
    pcdata->quest_failed            = 0;
    pcdata->quest_success           = 0;
    pcdata->gender_kills            = 0;
    pcdata->gender_lost             = 0;
    pcdata->gender_won              = 0;
    pcdata->explored = (EXPLORE_DATA *)calloc(1, sizeof(*(pcdata->explored) ) ); //Allocate explored data
    VALIDATE(pcdata);

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
    free_buf(pcdata->buffer);

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
        for(pExp = pcdata->explored->bits ; pExp ; pExp = e_next )
        {    e_next = pExp->next;
             free(pExp);
        }
    }

    INVALIDATE(pcdata);
    pcdata->next = pcdata_free;
    pcdata_free = pcdata;
    
    return;
}

/* stuff for recycling quests */
QUEST_DATA *quest_free;

QUEST_DATA *new_quest(void)
{
    static QUEST_DATA quest_zero;
    QUEST_DATA *quest;
    
    if (quest_free == NULL)
	quest = alloc_perm(sizeof(*quest));
    else
    {
	quest = quest_free;
	quest_free = quest_free->next;
    }
    
    *quest = quest_zero;
    
    VALIDATE(quest);
    return quest;
}

void free_quest(QUEST_DATA *quest)
{
    if (!IS_VALID(quest))
	return;
    
    INVALIDATE(quest);
    quest->next = quest_free;
    quest_free = quest;
}

/* stuff for recycling portals */
PORTAL_DATA *portal_free;

PORTAL_DATA *new_portal( void )
{
    PORTAL_DATA *portal;
    
    if (portal_free == NULL)
	portal = alloc_perm(sizeof(*portal));
    else
    {
	portal = portal_free;
	portal_free = portal_free->next;
    }
    
    portal->vnum = 0;
    portal->name = &str_empty[0];
    
    VALIDATE(portal);
    return portal;
}

void free_portal( PORTAL_DATA *portal )
{
    if (!IS_VALID(portal))
	return;
    
    free_string( portal->name );

    INVALIDATE(portal);
    portal->next = portal_free;
    portal_free = portal;
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

MEM_DATA *mem_data_free;

/* procedures and constants needed for buffering */

BUFFER *buf_free;

MEM_DATA *new_mem_data(void)
{
	MEM_DATA *memory;
  
	if (mem_data_free == NULL)
	memory = alloc_mem(sizeof(*memory));
	else
	{
	memory = mem_data_free;
	mem_data_free = mem_data_free->next;
	}

	memory->next = NULL;
	memory->id = 0;
	memory->reaction = 0;
	memory->when = 0;
	VALIDATE(memory);

	return memory;
}

void free_mem_data(MEM_DATA *memory)
{
	if (!IS_VALID(memory))
	return;

	memory->next = mem_data_free;
	mem_data_free = memory;
	INVALIDATE(memory);
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

	if (buf_free == NULL) 
	buffer = alloc_perm(sizeof(*buffer));
	else
	{
	buffer = buf_free;
	buf_free = buf_free->next;
	}

	buffer->next    = NULL;
	buffer->state   = BUFFER_SAFE;
	buffer->size    = get_size(BASE_BUF);

	buffer->string  = alloc_mem(buffer->size);
	buffer->string[0]   = '\0';
	VALIDATE(buffer);

	return buffer;
}

BUFFER *new_buf_size(int size)
{
	BUFFER *buffer;
 
	if (buf_free == NULL)
		buffer = alloc_perm(sizeof(*buffer));
	else
	{
		buffer = buf_free;
		buf_free = buf_free->next;
	}
 
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
	VALIDATE(buffer);
 
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
	INVALIDATE(buffer);

	buffer->next  = buf_free;
	buf_free      = buffer;
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

/* stuff for recycling mobprograms */
MPROG_LIST *mprog_free;
 
MPROG_LIST *new_mprog(void)
{
   static MPROG_LIST mp_zero;
   MPROG_LIST *mp;

   if (mprog_free == NULL)
	   mp = alloc_perm(sizeof(*mp));
   else
   {
	   mp = mprog_free;
	   mprog_free=mprog_free->next;
   }

   *mp = mp_zero;
   mp->vnum             = 0;
   mp->trig_type        = 0;
   mp->code             = str_dup("");
   VALIDATE(mp);
   return mp;
}

void free_mprog(MPROG_LIST *mp)
{
   if (!IS_VALID(mp))
	  return;

   INVALIDATE(mp);
   mp->next = mprog_free;
   mprog_free = mp;
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

	
SORT_TABLE *sort_free;

SORT_TABLE *new_sort(void)
{
   static SORT_TABLE *sort;

   if (sort_free == NULL) 
	  sort = alloc_perm(sizeof(*sort));
   else
   {
	  sort = sort_free;
	  sort_free = sort_free->next;
   }
   
   VALIDATE(sort);
   
   return sort;
}

void free_sort(SORT_TABLE *sort)
{
	if (!IS_VALID(sort))
	return;

	INVALIDATE(sort);

	sort->next  = sort_free;
	sort_free      = sort;
}

/* stuff for recycling wizlist structures */
WIZ_DATA *wiz_free;

WIZ_DATA *new_wiz(void)
{
    static WIZ_DATA wiz_zero;
    WIZ_DATA *wiz;
    
    if (wiz_free == NULL)
        wiz = alloc_perm(sizeof(*wiz));
    else
    {
        wiz = wiz_free;
        wiz_free = wiz_free->next;
    }
    
    *wiz = wiz_zero;
    VALIDATE(wiz);
    wiz->name = &str_empty[0];
    return wiz;
}

void free_wiz(WIZ_DATA *wiz)
{
    if (!IS_VALID(wiz))
        return;
    
    free_string(wiz->name);
    INVALIDATE(wiz);
    
    wiz->next = wiz_free;
    wiz_free = wiz;
}


/* Recycle crime list */
CRIME_DATA *crime_free;

CRIME_DATA *new_crime(void)
{
    static CRIME_DATA crime_zero;
    CRIME_DATA *crime;
    
    if (crime_free == NULL)
        crime = alloc_perm(sizeof(*crime));
    else
    {
        crime = crime_free;
        crime_free = crime_free->next;
    }
    
    *crime = crime_zero;

    VALIDATE(crime);

    crime->name      = NULL;
    crime->desc      = NULL;
    crime->imm_name  = NULL;
    crime->timestamp = 0;
    crime->forgive   = 0;
    return crime;
}

void free_crime(CRIME_DATA *crime)
{
    if (!IS_VALID(crime))
        return;
    
    free_string(crime->name);
    free_string(crime->desc);
    free_string(crime->imm_name);
    INVALIDATE(crime);
    
    crime->next = crime_free;
    crime_free = crime;
}

