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
#include <lua.h>
#include "merc.h"
#include "recycle.h"
#include "lua_main.h"
#include "lua_arclib.h"


static void free_pcdata(PC_DATA *pcdata);

/* stuff for recyling notes */
NOTE_DATA *note_free;

/* Allocate memory for a new note or recycle (Erwin's version) */
NOTE_DATA *new_note ( void )
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
static BAN_DATA *ban_free;

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

DESCRIPTOR_DATA *new_descriptor(void)
{
	DESCRIPTOR_DATA *d;

	d = alloc_DESCRIPTOR_DATA();
	
    VALIDATE(d);
	
	d->connected    = CON_GET_NAME;
	d->showstr_head = NULL;
	d->showstr_point = NULL;
	d->outsize  = 2000;
	d->pEdit        = NULL;         /* OLC */
	d->pString  = NULL;         /* OLC */
	d->editor   = 0;            /* OLC */
	d->outbuf   = alloc_mem( d->outsize );
    d->pProtocol= ProtocolCreate();
    d->host_proxied = FALSE;

    d->con_cb_data = NULL;

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
    
    if (d->character)
    {
        d->character->desc = NULL;
    }
    lua_unregister_desc(d);
	free_string( d->host );
	free_mem( d->outbuf, d->outsize );
    ProtocolDestroy( d->pProtocol );

    if ( d->con_cb_data )
    {
        CON_CB_DATA_free( d->con_cb_data );
        d->con_cb_data = NULL;
    }

    INVALIDATE(d);
    d->next = NULL;
    dealloc_DESCRIPTOR_DATA( d );
}

/* stuff for recycling gen_data */
static GEN_DATA *gen_data_free;

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


AFFECT_DATA *new_affect(void)
{
	AFFECT_DATA *af;

	af = alloc_AFFECT_DATA();

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
	af->next = NULL;
    dealloc_AFFECT_DATA( af );
}

OBJ_DATA *new_obj(void)
{
	OBJ_DATA *obj;

	obj = alloc_OBJ_DATA();
	VALIDATE(obj);
    obj->must_extract=FALSE;
    obj->otrig_timer=NULL;
    obj->luavals=NULL;

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

	INVALIDATE(obj);

	obj->next   = NULL;
    dealloc_OBJ_DATA( obj );
}


CHAR_DATA *new_char (void)
{
	CHAR_DATA *ch;
	int i;

	ch = alloc_CHAR_DATA();

	VALIDATE(ch);
	ch->name                    = &str_empty[0];
	ch->short_descr             = &str_empty[0];
	ch->long_descr              = &str_empty[0];
	ch->description             = &str_empty[0];
	ch->prompt                  = &str_empty[0];
	ch->prefix          = &str_empty[0];
	ch->logon                   = current_time;
	ch->lines                   = PAGELEN;
	ch->armor                   = 0;
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

    if (ch->desc)
    {
        ch->desc->character = NULL;
    }

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

    affect_unfreeze_sn(ch, 0); // to ensure frozen affects get deallocated as well
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

	INVALIDATE(ch);

    dealloc_CHAR_DATA( ch );
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
    
    pcdata->boss_achievements         = NULL;
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
    pcdata->duel_won                = 0;
    pcdata->duel_lost               = 0;
    pcdata->war_kills               = 0;
    pcdata->warpoints               = 0;
    pcdata->mob_kills               = 0;
    pcdata->mob_deaths              = 0;
    pcdata->quest_failed            = 0;
    pcdata->quest_success           = 0;
    pcdata->quest_hard_success      = 0;
    pcdata->quest_hard_failed       = 0;
    pcdata->gender_kills            = 0;
    pcdata->gender_lost             = 0;
    pcdata->gender_won              = 0;
    pcdata->gtell_history           = pers_history_new();
    pcdata->tell_history            = pers_history_new();
    pcdata->clan_history            = pers_history_new();
    pcdata->explored = (EXPLORE_DATA *)calloc(1, sizeof(*(pcdata->explored) ) ); //Allocate explored data
    
    pcdata->god_name = &str_empty[0];
    pcdata->faith = 0;
    pcdata->religion_rank = 0;

    pcdata->ptitles = new_luaref();

    VALIDATE(pcdata);

    return pcdata;
}
	

static void free_pcdata(PC_DATA *pcdata)
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
    free_string(pcdata->god_name);
    
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

    BOSSREC * rec, * rec_next;
    for ( rec = pcdata->boss_achievements ; rec ; rec=rec_next )
    {
        rec_next=rec->next;
        dealloc_BOSSREC( rec );
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

    free_luaref(g_mud_LS, pcdata->ptitles);

    INVALIDATE(pcdata);
    pcdata->next = pcdata_free;
    pcdata_free = pcdata;
    
    return;
}

/* stuff for recycling quests */
static QUEST_DATA *quest_free;

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
static PORTAL_DATA *portal_free;

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
static long    last_pc_id;
static long    last_mob_id;

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

static MEM_DATA *mem_data_free;

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
    memory->ally_reaction = 0;
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

PROG_LIST *new_mprog(void)
{
   PROG_LIST *mp;

   mp = alloc_MPROG_LIST();
   
   mp->vnum             = 0;
   mp->trig_type        = 0;
   mp->script           = NULL;
   VALIDATE(mp);
   return mp;
}

void free_mprog(PROG_LIST *mp)
{
   if (!IS_VALID(mp))
	  return;

   INVALIDATE(mp);
   mp->next = NULL;
   dealloc_MPROG_LIST( mp );
}

PROG_LIST *new_oprog(void)
{
   PROG_LIST *op;

   op = alloc_OPROG_LIST();
   
   op->vnum             = 0;
   op->trig_type        = 0;
   op->script           = NULL;
   VALIDATE(op);
   return op;
}

void free_oprog(PROG_LIST *op)
{
   if (!IS_VALID(op))
      return;

   INVALIDATE(op);
   op->next = NULL; 
   dealloc_OPROG_LIST( op );
}

PROG_LIST *new_aprog(void)
{
   PROG_LIST *ap;

   ap = alloc_APROG_LIST(); 

   ap->vnum             = 0;
   ap->trig_type        = 0;
   ap->script           = NULL;
   VALIDATE(ap);
   return ap;
}

void free_aprog(PROG_LIST *ap)
{
   if (!IS_VALID(ap))
      return;

   INVALIDATE(ap);
   ap->next = NULL;
   dealloc_APROG_LIST( ap );
}

PROG_LIST *new_rprog(void)
{
    PROG_LIST *rp;
    
    rp = alloc_RPROG_LIST(); 

    rp->vnum        = 0;
    rp->trig_type   = 0;
    rp->script      = NULL;
    VALIDATE(rp);
    return rp;
}

void free_rprog(PROG_LIST *rp)
{
    if (!IS_VALID(rp))
        return;

    INVALIDATE(rp);
    rp->next = NULL;
    dealloc_RPROG_LIST( rp );
}

static HELP_AREA * had_free;

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

HELP_DATA * new_help ( void )
{
   HELP_DATA * help;
   
   help       = alloc_HELP_DATA();

   help->level   = 0;
   help->keyword = str_dup("");
   help->text    = str_dup("");
   help->next    = NULL;
   help->next_area = NULL;

   return help;
}

	
static SORT_TABLE *sort_free;

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
static WIZ_DATA *wiz_free;

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
static CRIME_DATA *crime_free;

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

