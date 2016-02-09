/* 
 * Religion Code
 * by Henning Koehler <koehlerh@in.tum.de>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "buffer_util.h"
#include "religion.h"
#include "simsave.h"
#include "warfare.h"

//void show_religion_syntax( CHAR_DATA *ch );
//void list_religions( CHAR_DATA *ch );
//void show_religion_info( RELIGION_DATA *rel, CHAR_DATA *ch );
//DECLARE_DO_FUN(do_religion_set);
//void free_prayer( CHAR_DATA *ch );
//#define REL_DEBUG

//static RELIGION_DATA *religion_list = NULL;

static RELIGION_RANK_DATA religion_ranks[RELIGION_MAX_RANK + 1] =
{
    {    0, "Neophyte"   },
    {   50, "Acolyte"    },
    {  125, "Follower"   },
    {  250, "Evangelist" },
    {  500, "Prophet"    },
    { 1000, "Saint"      }
};

/* some macros for loading from files */

//#if defined(KEY)
//#undef KEY
//#endif

//#define KEY( literal, field, value ) if ( !str_cmp( word, literal ) ) { field  = value; fMatch = TRUE; break; }

/* provided to free strings */
//#if defined(KEYS)
//#undef KEYS
//#endif

//#define KEYS( literal, field, value ) if ( !str_cmp( word, literal ) ) { free_string(field); field  = value; fMatch = TRUE; break; }

/* methods for religion_data */

/*
RELIGION_DATA* new_religion()
{
    RELIGION_DATA *rel;
    int i;

#ifdef REL_DEBUG
    log_string( "new_religion: start" );
#endif

    rel = alloc_mem( sizeof(RELIGION_DATA) );
    
    rel->next = NULL;
    rel->name = NULL;
    rel->ID = 0;
    rel->min_align = -1000;
    rel->max_align = 1000;
    rel->altar_room_vnum = 0;
    rel->guard_vnum = 0;
    rel->relic_obj = NULL;
    rel->relic_vnum = 0;
    rel->relic_room_vnum = 0;
    rel->relic_bonus = 0;
    rel->god_power = 0;
//    rel->war_status = NULL;
    rel->god = NULL;
    for ( i = 0; i < MAX_PRIEST; i++ )
	rel->priest[i] = NULL;
    rel->follower = NULL;
    rel->conserve_at = 100;

#ifdef REL_DEBUG
    log_string( "new_religion: done" );
#endif

    return rel;
}

void free_religion( RELIGION_DATA* religion )
{
    int i;

#ifdef REL_DEBUG
    log_string( "free_religion: start" );
#endif

    if ( religion == NULL )
	return;

    free_string( religion->name );
//    free_religion_war_list( religion->war_status );
    free_string( religion->god );
    for ( i = 0; i < MAX_PRIEST; i++ )
	free_string( religion->priest[i] );
    free_follower_list( religion->follower );

    free_mem( religion, sizeof(RELIGION_DATA) );

#ifdef REL_DEBUG
    log_string( "free_religion: done" );
#endif

}

void religion_save_to_buffer( RELIGION_DATA *rel, DBUFFER *fp )
{
    int i;
    ROOM_INDEX_DATA *room;

#ifdef REL_DEBUG
    log_string( "religion_save_to_buffer: start" );
#endif

    if ( rel == NULL )
	return;

    bprintf( fp, "Name %s~\n", rel->name );
    bprintf( fp, "ID %d\n", rel->ID );
    bprintf( fp, "Align %d %d\n", rel->min_align, rel->max_align );
    bprintf( fp, "Altar %d\n", rel->altar_room_vnum );
    bprintf( fp, "Guard %d\n", rel->guard_vnum );
    bprintf( fp, "Relic %d\n", rel->relic_vnum );
    if ( (room = get_obj_room(rel->relic_obj)) != NULL )
	bprintf( fp, "RRoom %d\n", room->vnum );
    bprintf( fp, "Power %d\n", rel->god_power );

//    Don't bother saving war_status anymore, since wars are removed
//    if ( rel->war_status != NULL )
//    {
//	bprintf( fp, "Warstatus\n" );
//	religion_war_save_to_buffer( rel->war_status, fp );
//    }

    bprintf( fp, "God %s~\n", rel->god );
    for ( i = 0; i < MAX_PRIEST; i++ )
        if ( rel->priest[i] != NULL )
        {
            if ( i == 0 )
                bprintf( fp, "HPriest %s~\n", rel->priest[i] );
            else
                bprintf( fp, "Priest %s~\n", rel->priest[i] );
        }

    if ( rel->follower != NULL )
    {
	bprintf( fp, "Follower\n" );
	follower_save_to_buffer( rel->follower, fp );
    }

    bprintf( fp, "Consrv %d\n", rel->conserve_at );

    bprintf( fp, "End\n" );

#ifdef REL_DEBUG
    log_string( "religion_save_to_buffer: done" );
#endif

}

RELIGION_DATA* religion_load_from_file( FILE *fp )
{
    RELIGION_DATA *religion;
    FOLLOWER_DATA *last_fol = NULL;
    const char *word;
    bool fMatch;
    int priest_nr = 1;

#ifdef REL_DEBUG
    log_string( "religion_load_from_file: start" );
#endif

    religion = new_religion();
    while ( TRUE )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        
        switch ( UPPER(word[0]) )
	{
	case 'A':
	    if ( !str_cmp(word, "Align") )
	    {
		religion->min_align = fread_number(fp);
		religion->max_align = fread_number(fp);
		fMatch = TRUE;
		break;
	    }
	    KEY( "Altar", religion->altar_room_vnum, fread_number(fp) );
	    break;

	case 'C':
	    KEY( "Consrv", religion->conserve_at, fread_number(fp) );
	    break;

	case 'E':
	    if ( !str_cmp(word, "End") )
	    {

#ifdef REL_DEBUG
		log_string( "religion_load_from_file: done" );
#endif
		return religion;
	    }
	case 'F':
	    if ( !str_cmp(word, "Follower") )
	    {
		if ( last_fol == NULL )
		{
		    religion->follower = follower_load_from_file(religion, fp);
		    last_fol = religion->follower;
		}
		else
		{
		    last_fol->next = follower_load_from_file(religion, fp);
		    last_fol = last_fol->next;
		}
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'G':
	    KEYS( "God", religion->god, fread_string(fp) );
	    KEY( "Guard", religion->guard_vnum, fread_number(fp) );
	    break;
	case 'H':
	    KEYS( "HPriest", religion->priest[0], fread_string(fp) );
	    break;
	case 'I':
	    KEY( "ID", religion->ID, fread_number(fp) );
	    break;
	case 'N':
	    KEYS( "Name", religion->name, fread_string(fp) );
	    break;
	case 'P':
	    KEY( "Power", religion->god_power, fread_number(fp) ); 
	    if ( !str_cmp(word, "Priest") )
	    {
		if ( priest_nr < MAX_PRIEST )
		    religion->priest[priest_nr++] = fread_string(fp);
		else
		    free_string(fread_string(fp));
		fMatch = TRUE;
		break;
	    } 
	    break;
	case 'R':
	    KEY( "Relic", religion->relic_vnum, fread_number(fp) );
	    KEY( "RRoom", religion->relic_room_vnum, fread_number(fp) );
//	case 'W':
//	    if ( !str_cmp(word, "Warstatus") )
//	    {
//		religion->war_status = religion_war_load_from_file(fp);
//		fMatch = TRUE;
//		break;
//	    }
//	    break;
	}

	if ( !fMatch )
	{
	    bugf( "religion_load_file: no match: %s", word );
	    fread_to_eol( fp );
	}
    }

}

void religion_add_follower( RELIGION_DATA *religion, CHAR_DATA *ch )
{
    FOLLOWER_DATA *fol;

#ifdef REL_DEBUG
    log_string( "religion_add_follower: start" );
#endif

    if ( IS_NPC(ch) )
	return;

    if ( ch->pcdata->ch_rel != NULL
	&& ch->pcdata->ch_rel->religion != religion )
    {
	bugf( "religion_add_follower: %s already follows another religion",
	     ch->name );
	return;
    }

    if ( (fol = religion_get_follower(religion, ch->name)) == NULL )
    {
	fol = new_follower( religion, ch->name );
	fol->next = religion->follower;
	religion->follower = fol;
    }

    ch->pcdata->ch_rel = fol;

#ifdef REL_DEBUG
    log_string( "religion_add_follower: done" );
#endif

}

void religion_remove_follower( CHAR_DATA *ch )
{
    RELIGION_DATA *religion;
    FOLLOWER_DATA *fol, *next;

#ifdef REL_DEBUG
    log_string( "religion_remove_follower: start" );
#endif

    if ( IS_NPC(ch) || ch->pcdata->ch_rel == NULL )
	return;

    religion = ch->pcdata->ch_rel->religion;

    // remove from follower list
    if ( religion->follower == NULL )
	return;

    fol = religion->follower;
    if ( fol == ch->pcdata->ch_rel )
    {
	religion->follower = fol->next;
	free_follower( fol );
    }
    else
	for ( ; fol->next != NULL; fol = fol->next )
	    if ( fol->next == ch->pcdata->ch_rel )
	    {
		next = fol->next;
		fol->next = next->next;
		free_follower( next );
		break;
	    }
    ch->pcdata->ch_rel = NULL;

    religion_check_priest_exist( religion );

#ifdef REL_DEBUG
    log_string( "religion_remove_follower: done" );
#endif

}

FOLLOWER_DATA* religion_get_follower( RELIGION_DATA *religion, const char *name )
{
    FOLLOWER_DATA *fol;

#ifdef REL_DEBUG
    log_string( "religion_get_follower: start" );
#endif

    if ( name == NULL )
	return NULL;

    for ( fol = religion->follower; fol != NULL; fol = fol->next )
	if ( !str_cmp(fol->name, name) )
	    break;

#ifdef REL_DEBUG
    log_string( "religion_get_follower: done" );
#endif

    return fol;
}

void religion_check_priest_exist( RELIGION_DATA *religion )
{
    int i;

#ifdef REL_DEBUG
    log_string( "religion_check_priest_exist: start" );
#endif

    // check for priest position
    for ( i = 0; i < MAX_PRIEST; i++ )
	if ( religion->priest[i] != NULL
	     && religion_get_follower(religion, religion->priest[i]) == NULL )
	{
	    free_string( religion->priest[i] );
	    religion->priest[i] = NULL;
	}

#ifdef REL_DEBUG
    log_string( "religion_check_priest_exist: done" );
#endif

}

void religion_update_priests( RELIGION_DATA *religion )
{
    FOLLOWER_DATA *fol;
    const char *name;
    int i, priest_pos;
    int high_faith, priest_faith, faith;

#ifdef REL_DEBUG
    log_string( "religion_update_priest: start" );
#endif

    // ensure that priests are follower
    religion_check_priest_exist( religion );

    // update high priest position
    if ( religion->priest[0] == NULL )
	high_faith = 0;
    else
	high_faith = religion_get_follower(religion, religion->priest[0])->faith;

    for ( i = 1; i < MAX_PRIEST; i++ )
	if ( religion->priest[i] != NULL )
	{
	    faith = religion_get_follower(religion, religion->priest[i])->faith;
	    if ( faith > high_faith )
	    {
		// switch priest position
		high_faith = faith;
		name = religion->priest[0];
		religion->priest[0] = religion->priest[i];
		religion->priest[i] = name;
	    }
	}
    
    // check for new priest

    // search for free slot
    for ( i = 1; i < MAX_PRIEST; i++ )
	if ( religion->priest[i] == NULL )
	{
	    priest_faith = 0;
	    priest_pos = i;
	    break;
	}

    // if no empty position, find priest with weakest faith
    if ( i == MAX_PRIEST )
    {
	priest_faith = religion_get_follower(religion, religion->priest[1])->faith;
	priest_pos = 1;
	for ( i = 2; i < MAX_PRIEST; i++ )
	{
	    faith = religion_get_follower(religion, religion->priest[i])->faith;
	    if ( faith <= priest_faith )
	    {
		priest_faith = faith;
		priest_pos = i;
	    }
	}
    }
    
    // search non-priest follower with sufficiant rank and highest faith
    high_faith = priest_faith;
    name = NULL;
    for ( fol = religion->follower; fol != NULL; fol = fol->next )
	if ( fol->faith > high_faith
	     && !follower_is_priest(fol)
	     && follower_get_rank(fol) > RELIGION_RANK_NEO )
	{
	    high_faith = fol->faith;
	    free_string(name);
	    name = str_dup(fol->name);
	}

    // if success, change priest stat
    if ( name != NULL )
    {
	free_string( religion->priest[priest_pos] );
	religion->priest[priest_pos] = name;
    }

#ifdef REL_DEBUG
    log_string( "religion_update_priest: done" );
#endif

}

// remove auto-deleted followers
void religion_update_followers( RELIGION_DATA *religion )
{
    FOLLOWER_DATA *fol, *next;

#ifdef REL_DEBUG
    log_string( "religion_update_followers: start" );
#endif

    if ( (fol = religion->follower) == NULL )
	return;

    // check all but first follower
    while ( fol->next != NULL )
    {
	if ( pfile_exists(fol->next->name) )
	    fol = fol->next;
	else
	{
	    next = fol->next;
	    fol->next = next->next;
	    free_follower( next );
	}
    }

    // check first follower
    if ( !pfile_exists(religion->follower->name) )
    {
	fol = religion->follower;
	religion->follower = fol->next;
	free_follower( fol );
    }

    religion_check_priest_exist( religion );

#ifdef REL_DEBUG
    log_string( "religion_update_followers: done" );
#endif

}

// returns a relic to altar room if lying itmon
void religion_restore_relic( RELIGION_DATA *religion )
{
    ROOM_INDEX_DATA *altar_room, *room;
    
#ifdef REL_DEBUG
    log_string( "religion_restore_relic: start" );
#endif

    if ( religion->relic_obj == NULL
	 || religion->relic_obj->carried_by != NULL
	 || (altar_room = get_room_index(religion->altar_room_vnum)) == NULL )
	return;

    room = get_obj_room( religion->relic_obj );
    if ( room == NULL )
    {
	bugf( "religion_restore_relic: NULL room" );
	obj_to_room( religion->relic_obj, altar_room );
	return;
    }
    
    // check if room is itmon = no altar room
    if ( get_religion_of_altar(room) == NULL )
    {
	obj_from_room( religion->relic_obj );
	obj_to_room( religion->relic_obj, altar_room );
    }

#ifdef REL_DEBUG
    log_string( "religion_restore_relic: done" );
#endif

}

// ensure that a relic exists
void religion_create_relic( RELIGION_DATA *religion )
{
    OBJ_INDEX_DATA *relic_index;
    ROOM_INDEX_DATA *room;

#ifdef REL_DEBUG
    log_string( "religion_create_relic: start" );
#endif

    if ( religion->relic_obj != NULL )
	return;

    if ( (relic_index = get_obj_index(religion->relic_vnum)) == NULL )
	return;

    // ensure relic goes to a valid room
    if ( (room = get_room_index(religion->relic_room_vnum)) == NULL )
    {
	religion->relic_room_vnum = religion->altar_room_vnum;
	if ( (room = get_room_index(religion->relic_room_vnum)) == NULL )
	    return;
    }

    // create and place the relic
    religion->relic_obj = create_object( relic_index );
    obj_to_room( religion->relic_obj, room );

#ifdef REL_DEBUG
    log_string( "religion_create_relic: done" );
#endif

}

void religion_relic_damage( RELIGION_DATA *religion )
{
    OBJ_DATA *relic = religion->relic_obj;
    CHAR_DATA *victim;
    int dam;

#ifdef REL_DEBUG
    log_string( "religion_relic_damage: start" );
#endif

    if ( relic == NULL )
	return;

    if ( (victim = get_obj_char(relic)) == NULL )
	return;

    if ( IS_IMMORTAL(victim) )
	return;

    if ( IS_NPC(victim) )
    {
	act( "$n is slain by the power of $p.", victim, relic, NULL, TO_ROOM );
	raw_kill(victim, NULL, TRUE);
	return;
    }

    dam = 1000;
    if ( is_high_priest(victim) )
	dam /= 4;
    else if ( is_priest(victim) )
	dam /= 2;

    if ( victim->pcdata->ch_rel == NULL
	 || victim->pcdata->ch_rel->religion != religion
	 || follower_get_rank(victim->pcdata->ch_rel) == RELIGION_RANK_NEO )
	dam *= 2;

    full_dam( victim, victim, dam, skill_lookup("focus"), DAM_NONE, TRUE );

#ifdef REL_DEBUG
    log_string( "religion_relic_damage: done" );
#endif

}


// int religion_get_war_status( RELIGION_DATA *religion, RELIGION_DATA *opp )
// {
//     RELIGION_WAR_DATA *war;
// 
// #ifdef REL_DEBUG
//     log_string( "religion_get_war_status: start" );
// #endif
// 
//     for ( war = religion->war_status; war != NULL; war = war->next )
// 	if ( war->opp == opp )
// 	{
// #ifdef REL_DEBUG
// 	    log_string( "religion_get_war_status: done" );
// #endif
// 	    return war->status;
// 	}
// 
// #ifdef REL_DEBUG
//     log_string( "religion_get_war_status: done" );
// #endif
// 
//     return RELIGION_WAR_PEACE;
// }

// void religion_set_war_status( RELIGION_DATA *religion, RELIGION_DATA *opp, int status )
// {
//     RELIGION_WAR_DATA *war;
// 
// #ifdef REL_DEBUG
//     log_string( "religion_set_war_status: start" );
// #endif
// 
//     if ( religion == opp )
// 	return;
//     // peace is default - don't store
//     if ( status == RELIGION_WAR_PEACE )
//     {
// 	religion_remove_war_status( religion, opp );
// 	return;
//     }
    // search if opponent already in list
//    for ( war = religion->war_status; war != NULL; war = war->next )
// 	if ( war->opp == opp )
// 	{
//	    war->status = status;
// #ifdef REL_DEBUG
// 	    log_string( "religion_set_war_status: done" );
// #endif
//	    return;
//	}
//
//    // if not found, create new
//    war = new_religion_war( opp, status );
//    war->next = religion->war_status;
//    religion->war_status = war;
//
// #ifdef REL_DEBUG
//    log_string( "religion_set_war_status: done" );
// #endif
//
// }
//
// void religion_remove_war_status( RELIGION_DATA *religion, RELIGION_DATA *opp )
// {
//    RELIGION_WAR_DATA *war, *next;
//
// #ifdef REL_DEBUG
//     log_string( "religion_remove_war_status: start" );
// #endif
//
//    if ( religion->war_status == NULL )
// 	return;
//
//     if ( religion->war_status->opp == opp )
//     {
// 	war = religion->war_status;
// 	religion->war_status = war->next;
// 	free_religion_war( war );
// #ifdef REL_DEBUG
//     log_string( "religion_remove_war_status: done" );
// #endif
// 	return;
//     }
// 
//     for ( war = religion->war_status; war->next != NULL; war = war->next )
// 	if ( war->next->opp == opp )
// 	{
// 	    next = war->next;
// 	    war->next = next->next;
// 	    free_religion_war( next );
// #ifdef REL_DEBUG
//     log_string( "religion_remove_war_status: done" );
// #endif
// 	    return;
// 	}
// }

// methods for the list

MEMFILE* save_religions()
{
    RELIGION_DATA *rel;
    MEMFILE *fp;

#ifdef REL_DEBUG
    log_string( "save_religions: start" );
#endif

    // open file
    fp = memfile_new(RELIGION_FILE, 16*1024);
    if (fp == NULL)
    {
	bugf( "save_religions: couldn't open memory file for %s", RELIGION_FILE );
	return NULL;
    }

    // save the religions
    for ( rel = religion_list; rel != NULL; rel = rel->next )
    {
	bprintf( fp->buf, "#RELIGION\n" );
	religion_save_to_buffer( rel, fp->buf );
	bprintf( fp->buf, "\n" );
    }
    bprintf( fp->buf, "#END\n" );

    // check for overflow
    if (fp->buf->overflowed)
    {
	memfile_free(fp);
	return NULL;
    }

#ifdef REL_DEBUG
    log_string( "save_religions: done" );
#endif

    return fp;
}

void load_religions()
{
    RELIGION_DATA *rel, *rel_last = NULL;
    FILE *fp;
    const char *word;

#ifdef REL_DEBUG
    log_string( "load_religions: start" );
#endif

    religion_list = NULL;

    // open file
    if ( ( fp = fopen( RELIGION_FILE, "r" ) ) == NULL )
    {
	bugf( "load_religions: couldn't open %s", RELIGION_FILE );
	return;
    }

    // load the religions
    
    while ( TRUE )
    {
        word = feof( fp ) ? "#END" : fread_word( fp );
	if ( !strcmp(word, "#RELIGION") )
	{
	    rel = religion_load_from_file( fp );
	    if ( rel == NULL )
	    {
		bugf( "load_religions: NULL religion returned" );
		break;
	    }
	    if ( rel_last == NULL )
		religion_list = rel;
	    else
		rel_last->next = rel;
	    rel_last = rel;
	}
	else if ( !strcmp(word, "#END") )
	    break;
	else
	{
	    bugf( "load_religions: invalid section: %s", word );
	    break;
	}
    }    

    // close file
    fclose( fp );

    // fix the religion wars
    assign_religion_war_opp();

#ifdef REL_DEBUG
    log_string( "load_religions: done" );
#endif

}

void add_religion( RELIGION_DATA *religion )
{

#ifdef REL_DEBUG
    log_string( "add_religion: start" );
#endif

    religion->next = religion_list;
    religion_list = religion;

#ifdef REL_DEBUG
    log_string( "add_religion: done" );
#endif

}

void remove_religion( RELIGION_DATA *religion )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "remove_religion: start" );
#endif

    if ( religion_list == NULL )
    {
	bugf( "remove_religion: religion_list empty" );
	return;
    }
    if ( religion_list == religion )
	religion_list = religion_list->next;
    else
	for ( rel = religion_list; rel->next != NULL; rel = rel->next )
	    if ( rel->next == religion )
	    {
		rel->next = religion->next;
		break;
	    }

    // remove war status in other religions
    for ( rel = religion_list; rel != NULL; rel = rel->next )
    religion_remove_war_status( rel, religion );

    // free it
    free_religion( religion );

#ifdef REL_DEBUG
    log_string( "remove_religion: done" );
#endif

}

RELIGION_DATA* get_religion_by_name( const char *name )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "get_religion_by_name: start" );
#endif

    if ( name == NULL || name[0] == '\0' )
	return NULL;

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( is_name(name, rel->name) )
	{
	    return rel;
	}

    return NULL;
}

RELIGION_DATA* get_religion_by_ID( int ID )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "get_religion_by_ID: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( rel->ID == ID )
	{
#ifdef REL_DEBUG
	    log_string( "get_religion_by_ID: done" );
#endif
	    return rel;
	}

#ifdef REL_DEBUG
    log_string( "get_religion_by_ID: done" );
#endif

    return NULL;
}

void update_relic_bonus()
{
    RELIGION_DATA *rel, *rel_altar;

#ifdef REL_DEBUG
    log_string( "update_relic_bonus: start" );
#endif

    // clear old bonus
    for ( rel = religion_list; rel != NULL; rel = rel->next )
	rel->relic_bonus = 0;

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( rel->relic_obj != NULL )
	{
	    rel_altar = get_religion_of_altar( get_obj_room(rel->relic_obj) );
	    if ( rel_altar == rel )
		rel->relic_bonus += 10;
	    else if ( rel_altar != NULL )
	    {
		rel->relic_bonus -= 10;
		rel_altar->relic_bonus += 5;
	    }
	}

#ifdef REL_DEBUG
    log_string( "update_relic_bonus: done" );
#endif

}

// fix religion war lists after loading all religions
// void assign_religion_war_opp()
// {
//     RELIGION_DATA *rel;
//     RELIGION_WAR_DATA *war;
//    
// #ifdef REL_DEBUG
//     log_string( "assign_religion_war_opp: start" );
// #endif
// 
//    for ( rel = religion_list; rel != NULL; rel = rel->next )
// 	for ( war = rel->war_status; war != NULL; war = war->next )
// 	    war->opp = get_religion_by_ID( war->opp_ID );
//
// #ifdef REL_DEBUG
//     log_string( "assign_religion_war_opp: done" );
// #endif
//
// }

FOLLOWER_DATA* get_religion_follower_data( const char *name )
{
    FOLLOWER_DATA *fol;
    RELIGION_DATA *rel;
    
#ifdef REL_DEBUG
    log_string( "get_religion_follower_data: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( (fol = religion_get_follower(rel, name)) != NULL )
	    return fol;

#ifdef REL_DEBUG
    log_string( "get_religion_follower_data: done" );
#endif

    return NULL;
}

void all_religions( RELIGION_FUN *rel_fun )
{
    RELIGION_DATA *rel;
    
#ifdef REL_DEBUG
    log_string( "all_religions: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	rel_fun( rel );

#ifdef REL_DEBUG
    log_string( "all_religions: done" );
#endif

}

void update_priests()
{
    RELIGION_DATA *rel;
    
#ifdef REL_DEBUG
    log_string( "update_priests: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	religion_update_priests( rel );

#ifdef REL_DEBUG
    log_string( "update_priests: done" );
#endif

}

void update_followers()
{
    RELIGION_DATA *rel;
    
#ifdef REL_DEBUG
    log_string( "update_followers: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	religion_update_followers( rel );

#ifdef REL_DEBUG
    log_string( "update_followers: done" );
#endif

}

void relic_damage()
{
    RELIGION_DATA *rel;
    
#ifdef REL_DEBUG
    log_string( "relic_damage: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	religion_relic_damage( rel );

#ifdef REL_DEBUG
    log_string( "relic_damage: done" );
#endif

}

void create_relics()
{
    RELIGION_DATA *rel;
    
#ifdef REL_DEBUG
    log_string( "create_relics: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	religion_create_relic( rel );

#ifdef REL_DEBUG
    log_string( "create_relics: done" );
#endif

}
*/

/* methods for religion_war_data */

// RELIGION_WAR_DATA* new_religion_war( RELIGION_DATA *opp, int status )
// {
//     RELIGION_WAR_DATA *war;
// 
// #ifdef REL_DEBUG
//     log_string( "new_religion_war: start" );
// #endif
// 
//     war = alloc_mem( sizeof(RELIGION_WAR_DATA) );
// 
//     war->next = NULL;
//     war->opp = opp;
//     war->status = status;
// 
// #ifdef REL_DEBUG
//     log_string( "new_religion_war: done" );
// #endif
// 
//     return war;
// }
// 
// void free_religion_war( RELIGION_WAR_DATA *war )
// {
//     free_mem( war, sizeof(RELIGION_WAR_DATA) );
// }
// 
// void free_religion_war_list( RELIGION_WAR_DATA *war )
// {
//     RELIGION_WAR_DATA *next;
// 
// #ifdef REL_DEBUG
//     log_string( "free_religion_war_list: start" );
// #endif
// 
//     while ( war != NULL )
//     {
// 	next = war->next;
// 	free_religion_war( war );
// 	war = next;
//     }
// 
// #ifdef REL_DEBUG
//     log_string( "free_religion_war_list: done" );
// #endif
// 
// }
// 
// void religion_war_save_to_buffer( RELIGION_WAR_DATA *war, DBUFFER *fp )
// {
// 
// #ifdef REL_DEBUG
//    log_string( "religion_war_save_to_buffer: start" );
// #endif
// 
//     while ( war != NULL )
//     {
// 	bprintf( fp, "War %d %d\n", war->opp->ID, war->status );
// 	war = war->next;
//     }
//     bprintf( fp, "End\n" );
// 
// #ifdef REL_DEBUG
//     log_string( "religion_war_save_to_buffer: done" );
// #endif
// 
// }
// 
// RELIGION_WAR_DATA* religion_war_load_from_file( FILE *fp )
// {
//     RELIGION_WAR_DATA *war, *new_war;
//     char *word;
// 
// #ifdef REL_DEBUG
//     log_string( "religion_war_load_from_file: start" );
// #endif
// 
//     war = NULL;
//     while ( TRUE )
//     {
//         word = feof( fp ) ? "End" : fread_word( fp );
// 	if ( !strcmp(word, "War") )
// 	{
// 	    new_war = new_religion_war( NULL, 0 );
// 	    new_war->opp_ID = fread_number( fp );
// 	    new_war->status = fread_number( fp );
// 	    new_war->next = war;
// 	    war = new_war;
// 	}
// 	else if ( !strcmp(word, "End") )
// 	    break;
// 	else
// 	{
// 	    bugf( "religion_war_load_from_file: invalid match: %s", word );
// 	    break;
// 	}
//     }    
// 
// #ifdef REL_DEBUG
//     log_string( "religion_war_load_from_file: done" );
// #endif
// 
//     return war;
// }

// methods for follower_data
/*
FOLLOWER_DATA* new_follower( RELIGION_DATA *religion, const char *name )
{
    FOLLOWER_DATA *fol;

#ifdef REL_DEBUG
    log_string( "new_follower: start" );
#endif

    fol = alloc_mem( sizeof(FOLLOWER_DATA) );

    fol->next = NULL;
    fol->religion = religion;
    fol->name = str_dup(name);
    fol->faith = 0;
    fol->favour = 0;
    fol->join_time = current_time;

#ifdef REL_DEBUG
    log_string( "new_follower: done" );
#endif

    return fol;
}

void free_follower( FOLLOWER_DATA *fol )
{
    CHAR_DATA *ch;

#ifdef REL_DEBUG
    log_string( "free_follower: start" );
#endif

    // make sure that online player drop their follower data
    for ( ch = char_list; ch != NULL; ch = ch->next )
	if ( ch->pcdata != NULL && ch->pcdata->ch_rel == fol )
	{
	    ch->pcdata->ch_rel = NULL;
	    break;
	}
    
    free_string( fol->name );
    free_mem( fol, sizeof(FOLLOWER_DATA) );

#ifdef REL_DEBUG
    log_string( "free_follower: done" );
#endif

}

void free_follower_list( FOLLOWER_DATA *list )
{
    FOLLOWER_DATA *next;

#ifdef REL_DEBUG
    log_string( "free_follower_list: start" );
#endif

    while ( list != NULL )
    {
	next = list->next;
	free_follower( list );
	list = next;
    }

#ifdef REL_DEBUG
    log_string( "free_follower_list: done" );
#endif

}

void follower_save_to_buffer( FOLLOWER_DATA *list, DBUFFER *fp )
{
    FOLLOWER_DATA *fol;

#ifdef REL_DEBUG
    log_string( "follower_save_to_buffer: start" );
#endif

    for ( fol = list; fol != NULL; fol = fol->next )
	bprintf( fp, "Fol %s~ %d %d %ld\n",
		 fol->name, fol->faith, fol->join_time, fol->favour );
    bprintf( fp, "End\n" );

#ifdef REL_DEBUG
    log_string( "follower_save_to_buffer: done" );
#endif

}

FOLLOWER_DATA* follower_load_from_file( RELIGION_DATA *religion, FILE *fp )
{
    FOLLOWER_DATA *fol, *new_fol;
    const char *word;

#ifdef REL_DEBUG
    log_string( "follower_load_from_file: start" );
#endif

    fol = NULL;
    while ( TRUE )
    {
        word = feof( fp ) ? "End" : fread_word( fp );
	if ( !strcmp(word, "Fol") )
	{
	    new_fol = new_follower( religion, fread_string(fp) );
	    new_fol->faith = fread_number( fp );
	    new_fol->join_time = fread_number( fp );
	    new_fol->favour = fread_number( fp );
	    new_fol->next = fol;
	    fol = new_fol;
	}
	else if ( !strcmp(word, "End") )
	    return fol;
	else
	{
	    bugf( "follower_load_from_file: invalid match: %s", word );
	    break;
	}
    }

#ifdef REL_DEBUG
    log_string( "follower_load_from_file: done" );
#endif

    return fol;
}

int follower_get_rank( FOLLOWER_DATA *fol )
{
    int rank;
    int fol_time;

#ifdef REL_DEBUG
    log_string( "follower_get_rank: start" );
#endif

    fol_time = current_time - fol->join_time;

    for ( rank = RELIGION_MAX_RANK; rank > 0; rank-- )
	if ( religion_ranks[rank].min_time <= fol_time
	     && religion_ranks[rank].min_faith <= fol->faith )
	    break;

#ifdef REL_DEBUG
    log_string( "follower_get_rank: done" );
#endif

    return rank;
}

bool follower_is_priest( FOLLOWER_DATA *fol )
{
    int i;

#ifdef REL_DEBUG
    log_string( "follower_is_priest: start" );
#endif

    if ( fol == NULL )
	return FALSE;

    for ( i = 0; i < MAX_PRIEST; i++ )
	if ( fol->religion->priest[i] != NULL
	     && !strcmp(fol->religion->priest[i], fol->name) )
	    return TRUE;

    return FALSE;
}


// general methods concerning religion

RELIGION_DATA *get_religion( CHAR_DATA *ch )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "get_religion: start" );
#endif

    if ( IS_IMMORTAL(ch) )
    {
	for ( rel = religion_list; rel != NULL; rel = rel->next )
	    if ( rel->god != NULL && !strcmp(rel->god, ch->name) )
		return rel;
	return NULL;
    }

    if ( ch->pcdata != NULL && ch->pcdata->ch_rel != NULL )
	return ch->pcdata->ch_rel->religion;
    else
	return NULL;
}

bool is_priest( CHAR_DATA *ch )
{

#ifdef REL_DEBUG
    log_string( "is_priest: start" );
#endif

    if ( ch->pcdata != NULL && ch->pcdata->ch_rel != NULL )
	return follower_is_priest( ch->pcdata->ch_rel );
    else
	return FALSE;
}

bool is_high_priest( CHAR_DATA *ch )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "is_high_priest: start" );
#endif

    rel = get_religion( ch );
    if ( rel != NULL && rel->priest[0] != NULL )
	return strcmp( rel->priest[0], ch->name ) == 0;
    else
	return FALSE;
}

bool is_religion_opp( CHAR_DATA *ch, CHAR_DATA *opp )
{
    FOLLOWER_DATA *ch_fol, *opp_fol;

#ifdef REL_DEBUG
    log_string( "is_religion_opp: start" );
#endif

    if ( ch->pcdata == NULL || ch->pcdata->ch_rel == NULL
	 || opp->pcdata == NULL || opp->pcdata->ch_rel == NULL )
	return FALSE;
	 
    ch_fol = ch->pcdata->ch_rel;
    opp_fol = opp->pcdata->ch_rel;

    if ( follower_get_rank(ch_fol) == RELIGION_RANK_NEO
	 || follower_get_rank(opp_fol) == RELIGION_RANK_NEO )
	return FALSE;

// Instead of checking war status to see if the person is in an opposing religion,
// we check simply whether they are of high enough rank to steal a relic (i.e. higher
// than Neophyte).  If those qualifications above didn't disqualify this person, then
// they are of a potentially dangerous religion.  This check is made when temple guards
// attack followers of a different religion.  A neophyte, or non-religious person, gets
// transferred out of the temple immediately.  Anyone else is considered hostile.
//    return religion_get_war_status(ch_fol->religion, opp_fol->religion)
//	== RELIGION_WAR_WAR
//	|| religion_get_war_status(opp_fol->religion, ch_fol->religion)
//	== RELIGION_WAR_WAR;
    return TRUE;
}

bool carries_relic( CHAR_DATA *ch )
{
    RELIGION_DATA *rel;
    
#ifdef REL_DEBUG
    log_string( "carries_relic: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( get_obj_char(rel->relic_obj) == ch )
	    return TRUE;
    
    return FALSE;
}

void check_religion_align( CHAR_DATA *ch )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "check_religion_align: start" );
#endif

    if ( IS_IMMORTAL(ch) || (rel = get_religion(ch)) == NULL )
	return;

    if ( !IS_BETWEEN(rel->min_align, ch->alignment, rel->max_align) )
    {
	send_to_char( "Your alignment opposes your religion.\n\r", ch );
	remove_priest( ch );
	send_to_char( "You lose your faith.\n\r", ch );
	religion_remove_follower( ch );
    }

#ifdef REL_DEBUG
    log_string( "check_religion_align: done" );
#endif

}

void remove_priest( CHAR_DATA *ch )
{
    RELIGION_DATA *rel;
    int i;

#ifdef REL_DEBUG
    log_string( "remove_priest: start" );
#endif

    if ( IS_IMMORTAL(ch) || (rel = get_religion(ch)) == NULL )
	return;

    for ( i = 0; i < MAX_PRIEST; i++ )
	if ( rel->priest[i] != NULL && !str_cmp(rel->priest[i], ch->name) )
	{
	    free_string( rel->priest[i] );
	    rel->priest[i] = NULL;
	    send_to_char( "You lose your priest status.\n\r", ch );
	    return;
	}
}

// returns the corresponding religion if room is alter-room
RELIGION_DATA *get_religion_of_altar( ROOM_INDEX_DATA *room )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "get_religion_of_altar: start" );
#endif

    if ( room == NULL )
	return NULL;
    
    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( rel->altar_room_vnum == room->vnum )
	    return rel;

    return NULL;
}

// returns the corresponding religion if mob is temple-guard
RELIGION_DATA *get_religion_of_guard( CHAR_DATA *guard )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "get_religion_of_guard: start" );
#endif

    if ( guard == NULL || guard->pIndexData == NULL )
	return NULL;
    
    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( rel->guard_vnum != 0 )
	    if ( rel->guard_vnum == guard->pIndexData->vnum
		 || rel->guard_vnum == guard->pIndexData->group )
		return rel;

    return NULL;
}
*/
int get_religion_rank_number( const char *name )
{
    int i;

    for (i=0; i <= RELIGION_MAX_RANK; i++)
    {
        if (!strcmp(name, religion_ranks[i].name))
        {
            return i;
        }
    }
    return -1;
}
    
const char* get_religion_rank_name( int rank )
{
#ifdef REL_DEBUG
    log_string( "get_religion_rank_name: start" );
#endif

    rank = URANGE(0, rank, RELIGION_MAX_RANK);
    return religion_ranks[rank].name;
}

const char* get_ch_rank_name( CHAR_DATA *ch )
{
#ifdef REL_DEBUG
    log_string( "get_ch_rank_name: start" );
#endif

    if ( ch->pcdata == NULL || ch->pcdata->god_name[0] == '\0' )
        return "None";

    return get_religion_rank_name( ch->pcdata->religion_rank );
}

/*
int get_religion_bonus( CHAR_DATA *ch )
{
    RELIGION_DATA *rel = get_religion( ch );

#ifdef REL_DEBUG
    log_string( "get_religion_bonus: start" );
#endif

    if ( rel == NULL || !is_religion_member(ch) )
	return 0;
    else
	if ( !IS_BETWEEN(rel->min_align, ch->alignment, rel->max_align) )
	    return UMIN( 0, rel->relic_bonus );
	else
	    return rel->relic_bonus;
}
*/

void gain_faith( CHAR_DATA *ch, int gain )
{

#ifdef REL_DEBUG
    log_string( "gain_faith: start" );
#endif

    if ( gain <= 0 )
        return;

    if ( ch != NULL && ch->pcdata != NULL )
    {
        ch->pcdata->faith += gain;
        //ch->pcdata->ch_rel->religion->god_power += gain;
        send_to_char( "You feel more faithful.\n\r", ch );
        int next_rank = ch->pcdata->religion_rank + 1;
        if ( next_rank <= RELIGION_MAX_RANK && ch->pcdata->faith >= religion_ranks[next_rank].min_faith )
            ptc(ch, "You are ready to advance to the rank of %s.\n\r", get_religion_rank_name(next_rank));
    }

#ifdef REL_DEBUG
    log_string( "gain_faith: done" );
#endif

}

const char* get_god_name( CHAR_DATA *ch )
{
    
#ifdef REL_DEBUG
    log_string( "get_god_name: start" );
#endif

    if ( has_god(ch) )
       return ch->pcdata->god_name;
    
    const char *god_name = clan_table[ch->clan].patron;
    if ( god_name == NULL || god_name[0] == '\0' )
        return "Rimbol";
    else
        return god_name;
}

bool has_god( CHAR_DATA *ch ) {
    return ch->pcdata && ch->pcdata->god_name[0] != '\0';
}

int get_faith( CHAR_DATA *ch )
{

#ifdef REL_DEBUG
    log_string( "get_faith: start" );
#endif

    if ( ch->pcdata == NULL )
        return 0;
    else
        return ch->pcdata->faith;
}

/*
bool is_relic_obj( OBJ_DATA *obj )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "is_relic_obj: start" );
#endif

    if ( obj == NULL )
	return FALSE;
    
    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( rel->relic_obj == obj )
	    return TRUE;

    return FALSE;
}

void free_relic( OBJ_DATA *obj )
{
    RELIGION_DATA *rel;

#ifdef REL_DEBUG
    log_string( "free_relic: start" );
#endif

    for ( rel = religion_list; rel != NULL; rel = rel->next )
	if ( rel->relic_obj == obj )
	    rel->relic_obj = NULL;

#ifdef REL_DEBUG
    log_string( "free_relic: done" );
#endif

}

bool is_religion_member( CHAR_DATA *ch )
{
    if ( get_religion(ch) == NULL )
	return FALSE;

    if ( IS_IMMORTAL(ch) )
	return TRUE;

    if ( ch->pcdata == NULL
	 || ch->pcdata->ch_rel == NULL )
	return FALSE;

    if ( follower_get_rank(ch->pcdata->ch_rel) == RELIGION_RANK_NEO )
	return FALSE;
    else
	return TRUE;
}

double adjust_align_change( CHAR_DATA *ch, double change )
{
    FOLLOWER_DATA *fol;
    int ideal_align;
    double rank_factor;

    if ( ch->pcdata == NULL || (fol = ch->pcdata->ch_rel) == NULL )
	return change;

    // calculate "ideal alignment" for the religion
    if ( fol->religion->min_align == -1000
	 && fol->religion->max_align == 1000 )
	ideal_align = 0;
    else if ( fol->religion->min_align == -1000 )
	ideal_align = -1000;
    else if ( fol->religion->max_align == 1000 )
	ideal_align = 1000;
    else
	ideal_align = (fol->religion->min_align + fol->religion->max_align) / 2;

    // lower/speed alignment change depending on rank
    rank_factor = 1 + 0.2 * follower_get_rank( fol );
    if ( (ch->alignment - ideal_align) * change < 0 )
	return change * rank_factor;
    else
	return change / rank_factor;
}

// the big user-interface

DEF_DO_FUN(do_religion)
{
    RELIGION_DATA *rel;
    CHAR_DATA *victim;
    int ID = 0;
    char arg1[MIL];
    char buf[MSL];

#ifdef REL_DEBUG
    log_string( "do_religion: start" );
#endif

    if ( IS_NPC(ch) )
	return;

    argument = one_argument( argument, arg1 );

    if ( !strcmp(arg1, "list") )
	list_religions( ch );
    else if ( !strcmp(arg1, "info") )
    {
	if ( argument[0] == '\0' )
	{
	    if ( (rel = get_religion(ch)) == NULL )
	    {
		send_to_char( "Which religion do you want to know about?\n\r", ch );
		return;
	    }
	}
	else if ( (rel = get_religion_by_name(argument)) == NULL )
	{
	    send_to_char( "That religion doesn't exist.\n\r", ch );
	    return;
	}
	// show the info
	show_religion_info( rel, ch );
    }
    else if ( !strcmp(arg1, "join") )
    {

	if ( NOT_AUTHED(ch) )
	{
	    send_to_char( "You aren't authorized yet.\n\r", ch );
	    return;
	}
	if ( IS_IMMORTAL(ch) )
	{
	    send_to_char( "Only mortals can join a religion.\n\r", ch );
	    return;
	}
	if ( get_religion(ch) != NULL )
	{
	    send_to_char( "You are already worshipping someone.\n\r", ch );
	    return;
	}
	if ( IS_SET(ch->act, PLR_WAR) )
	{
	    send_to_char( "Wait till the war is over.\n\r", ch );
	    return;
	}
	if ( argument[0] == '\0' )
	{
	    send_to_char( "Which religion do you wish to join?\n\r", ch );
	    return;
	}
	if ( (rel = get_religion_by_name(argument)) == NULL )
	{
	    send_to_char( "That religion doesn't exist.\n\r", ch );
	    return;
	}
	if ( !IS_BETWEEN(rel->min_align, ch->alignment, rel->max_align) )
	{
	    send_to_char( "You don't have the proper alignment.\n\r", ch );
	    return;
	}
	// join
	religion_add_follower( rel, ch );
	sprintf( buf, "You join %s.\n\r", rel->name ); 
	send_to_char( buf, ch );
    }
    else if ( !strcmp(arg1, "reject") )
    {
	if ( IS_IMMORTAL(ch) )
	{
	    send_to_char( "Hello? You're the GOD!\n\r", ch );
	    return;
	}
	if ( (rel = get_religion(ch)) == NULL )
	{
	    send_to_char( "You aren't worshipping anyone.\n\r", ch );
	    return;
	}
	if ( IS_SET(ch->act, PLR_WAR) )
	{
	    send_to_char( "Wait till the war is over.\n\r", ch );
	    return;
	}
	if ( strcmp(argument, "confirm") )
	{
	    send_to_char( "Are you sure you want to do that?\n\r", ch );
	    send_to_char( "If yes, type 'religion reject confirm'.\n\r", ch );
	    return;
	}
	// reject
	religion_remove_follower( ch );
	send_to_char( "You lose your faith.\n\r", ch );
    }
//    else if ( !strcmp(arg1, "war") )
//    {
//	do_religion_war( ch, argument );
//    }
    else if ( !strcmp(arg1, "start") )
    {
	if ( !IS_IMMORTAL(ch) || ch->level < L4 )
	{
	    send_to_char( "Only gods can start a religion.\n\r", ch );
	    return;
	}
	if ( argument[0] == '\0' )
	{
	    send_to_char( "You must provide a name.\n\r", ch );
	    return;
	}
	if ( (get_religion_by_name(argument)) != NULL )
	{
	    send_to_char( "That religion already exists.\n\r", ch );
	    return;
	}
	if ( get_religion(ch) != NULL )
	{
	    send_to_char( "You are already being worshipped.\n\r", ch );
	    return;
	}
	// find an unused ID
	while ( get_religion_by_ID(ID) != NULL )
	    ID++;
	// create the new religion
	rel = new_religion();
	rel->name = str_dup(smash_tilde_cc(argument));
	rel->god = str_dup( ch->name );
	rel->ID = ID;
	rel->altar_room_vnum = ch->in_room->vnum;
	add_religion( rel );
	send_to_char( "Religion created.\n\r", ch );
    }
    else if ( !strcmp(arg1, "remove") )
    {
	if ( !IS_IMMORTAL(ch) )
	{
	    send_to_char( "Only gods can remove a religion.\n\r", ch );
	    return;
	}
	if ( argument[0] == '\0' )
	{
	    send_to_char( "You must provide a name.\n\r", ch );
	    return;
	}
	if ( (rel = get_religion_by_name(argument)) == NULL )
	{
	    send_to_char( "That religion doesn't exist.\n\r", ch );
	    return;
	}
	if ( get_religion(ch) != rel && get_trust(ch) < ML )
	{
	    send_to_char( "You don't head that religion.\n\r", ch );
	    return;
	}
	// remove it
	remove_religion( rel );
	send_to_char( "Religion removed.\n\r", ch );
    }
    else if ( !strcmp(arg1, "boot") )
    {
	if ( !IS_IMMORTAL(ch) )
	{
	    send_to_char( "Only gods can boot followers.\n\r", ch );
	    return;
	}
	if ( argument[0] == '\0' )
	{
	    send_to_char( "You must provide a name.\n\r", ch );
	    return;
	}
	if ( (victim = get_char_world(ch, argument)) == NULL )
	{
	    send_to_char( "That player does not exist.\n\r", ch );
	    return;
	}
	if ( (rel = get_religion(victim)) == NULL )
	{
	    send_to_char( "That player doesn't follow any god.\n\r", ch );
	    return;
	}
	if ( get_religion(ch) != rel && get_trust(ch) < ML )
	{
	    send_to_char( "That player doesn't follow you.\n\r", ch );
	    return;
	}
	// remove it
	religion_remove_follower( victim );
	send_to_char( "Follower removed.\n\r", ch );
	send_to_char( "Your god has crushed your beliefs.\n\r", victim );
    }
    else if ( !strcmp(arg1, "set") )
    {
	do_religion_set( ch, argument );
    }
    else if ( !strcmp(arg1, "favour") || !strcmp(arg1, "favor") )
    {
	int fav;

	if( !IS_IMMORTAL(ch) )
	{
	    if( (rel = get_religion(ch)) != NULL )
	    {
	        if( ch->pcdata->ch_rel->favour >= 5 )
	            sprintf( buf, "%s thinks very highly of you.\n\r", rel->god );
		else if( ch->pcdata->ch_rel->favour >= 1 )
		    sprintf( buf, "%s thinks highly of you.\n\r", rel->god );
		else if( ch->pcdata->ch_rel->favour == 0 )
		    sprintf( buf, "You have not displeased %s...yet.\n\r", rel->god );
		else if( ch->pcdata->ch_rel->favour >= -10 )
		    sprintf( buf, "%s frowns upon you.\n\r", rel->god );
	        else if( ch->pcdata->ch_rel->favour >= -50 )
		    sprintf( buf, "You have fallen out of favour with %s.\n\r", rel->god );
		else
		    sprintf( buf, "You have gravely displeased %s.\n\r", rel->god );
		send_to_char( buf, ch );
	    } else
		send_to_char( "You have not chosen a god to follow.\n\r", ch );
	    return;
	}

	if( argument[0] == '\0' )
	{
	    send_to_char( "Check whose favour? Or grant how much favour to whom?\n\r", ch );
	    send_to_char( "Syntax:  religion favour <name> [-100<=number<=+10]\n\r", ch );
	    return;
	}
	
	argument = one_argument( argument, arg1 );

	if( (victim = get_char_world(ch,arg1)) == NULL || IS_NPC(victim) )
	{
	    send_to_char( "That player does not exist.\n\r", ch );
	    return;
	}
	
	argument = one_argument( argument, arg1 );

	if( !is_number(arg1) )
	{
	    send_to_char( "Syntax:  religion favour <name> [-100<=number<=+10]\n\r", ch );
	    return;
	}

	fav = atoi( arg1 );
	victim->pcdata->ch_rel->favour = fav;
	sprintf( buf, "%s's favour is now set to %d.\n\r", victim->name, fav );
	send_to_char( buf, ch );
	return;
    }
    else
	show_religion_syntax( ch );

#ifdef REL_DEBUG
    log_string( "do_religion: done" );
#endif

}

void show_religion_syntax( CHAR_DATA *ch )
{

#ifdef REL_DEBUG
    log_string( "show_religion_syntax: start" );
#endif

    send_to_char( "Syntax: religion list\n\r", ch );
    send_to_char( "        religion info <name>\n\r", ch );
    send_to_char( "        religion join <name>\n\r", ch );
    send_to_char( "        religion reject\n\r", ch );
//    send_to_char( "        religion war <status> <name>\n\r", ch );
    if ( IS_IMMORTAL(ch) )
    {
    send_to_char( "        religion start <name>\n\r", ch );
    send_to_char( "        religion remove <name>\n\r", ch );
    send_to_char( "        religion boot <name>\n\r", ch );
    send_to_char( "        religion set <field> <value>\n\r", ch );
    send_to_char( "        religion favor/favour <name> [-100<=number<=+10]\n\r", ch );
    }
    else
        send_to_char( "        religion favor/favour (shows how favoured you are by your god)\n\r", ch );

#ifdef REL_DEBUG
    log_string( "show_religion_syntax: done" );
#endif

}

void list_religions( CHAR_DATA *ch )
{
    RELIGION_DATA *rel;
    char buf[MSL];

#ifdef REL_DEBUG
    log_string( "list_religions: start" );
#endif

    send_to_char( "The following religions exist:\n\r", ch );
    for ( rel = religion_list; rel != NULL; rel = rel->next )
    {
	sprintf( buf, "%-20s: %s\n\r", rel->god, rel->name );
	send_to_char( buf, ch );
    }

#ifdef REL_DEBUG
    log_string( "list_religions: done" );
#endif

}

void show_religion_info( RELIGION_DATA *rel, CHAR_DATA *ch )
{
    BUFFER *buffer;
    char buf[MSL];
    OBJ_INDEX_DATA *relic;
    FOLLOWER_DATA *fol;
    int i, rank;
    bool priest_found = FALSE;

#ifdef REL_DEBUG
    log_string( "show_religion_info: start" );
#endif

    buffer = new_buf();

    sprintf( buf, "Religion %s:\n\r\n\r", rel->name );
    add_buf( buffer, buf );
    sprintf( buf, "God: %s\n\r", rel->god );
    add_buf( buffer, buf );
    sprintf( buf, "Alignment: %d to %d\n\r", rel->min_align, rel->max_align );
    add_buf( buffer, buf );
    if ( (relic = get_obj_index(rel->relic_vnum)) != NULL )
    {
	sprintf( buf, "Relic: %s\n\r", relic->short_descr );
	add_buf( buffer, buf );
    }
    if ( IS_IMMORTAL(ch) )
    {
	sprintf( buf, "Relic Vnum: %d\n\r", rel->relic_vnum );
	add_buf( buffer, buf );
	sprintf( buf, "Relic Bonus: %d%%\n\r", rel->relic_bonus );
	add_buf( buffer, buf );
	sprintf( buf, "Power: %d\n\r", rel->god_power );
	add_buf( buffer, buf );
	sprintf( buf, "Altar Room: %d\n\r", rel->altar_room_vnum );
	add_buf( buffer, buf );
	sprintf( buf, "Temple Guard Vnum: %d\n\r", rel->guard_vnum );
	add_buf( buffer, buf );
    }
    else
    {
	// Pictoral representation of the relic bonus
	char stars[20];
	int i, relics = 0;
	bool own = FALSE;
	RELIGION_DATA *r;

	for( r = religion_list; r != NULL; r = r->next )
	{
	    if( rel == get_religion_of_altar(get_obj_room(r->relic_obj)) )
	    {
		if( r == rel )	own = TRUE;
		else		relics++;
	    }
	}

	if( own )
	    sprintf( stars, "{Y*{R" );
	else
	    sprintf( stars, "{R" );

	for( i=0; i<relics; i++ )
	    strcat( stars, "*" );
	strcat( stars, "{x" );

	sprintf( buf, "Relic Power: %s\n\r", stars );
	add_buf( buffer, buf );
    }

    if ( rel->priest[0] != NULL )
    {
	sprintf( buf, "High Priest: %s\n\r", rel->priest[0] );
	add_buf( buffer, buf );
    }

    add_buf( buffer, "Priests: " );
    for ( i = 1; i < MAX_PRIEST; i++ )
	if ( rel->priest[i] != NULL )
	{
	    priest_found = TRUE;
	    sprintf( buf, " %s", rel->priest[i] );
	    add_buf( buffer, buf );
	}
    if ( !priest_found )
	add_buf( buffer, " none" );

    // show the followers, sorted by rank
    for ( rank = RELIGION_MAX_RANK; rank >= 0; rank-- )
    {
	sprintf( buf, "\n\r========== %-10s ==========",
		 get_religion_rank_name(rank) );
	add_buf( buffer, buf );
	i = 0;
	for ( fol = rel->follower; fol != NULL; fol = fol->next )
	    if ( follower_get_rank(fol) == rank )
	    {
		if ( i++ % 5 == 0 )
		    add_buf( buffer, "\n\r" );
		sprintf( buf, "%-15s", fol->name );
		add_buf( buffer, buf );
	    }
    }
    add_buf( buffer, "\n\r" );

    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);

#ifdef REL_DEBUG
    log_string( "show_religion_info: done" );
#endif

}
*/

// char* war_status_name( int status )
// {
// 
// #ifdef REL_DEBUG
//     log_string( "war_status_name: start" );
// #endif
// 
//     switch( status )
//     {
//     case RELIGION_WAR_PEACE: return "{Cpeace{x";
//     case RELIGION_WAR_WAR: return "{Rwar{x";
//     default: return "error";
//     }
// }
// 
// int get_war_status_by_name( char *name )
// {
// 
// #ifdef REL_DEBUG
//     log_string( "get_war_status_by_name: start" );
// #endif
// 
//     if ( !strcmp(name, "peace" ) )
// 	return RELIGION_WAR_PEACE;
//     if ( !strcmp(name, "war" ) )
// 	return RELIGION_WAR_WAR;
//     return -1; /* not found */ */
// }
// 
// void do_religion_war( CHAR_DATA *ch, char *argument )
// {
//     RELIGION_DATA *rel, *opp;
//     char arg1[MIL];
//     char buf[MSL];
//     int status,other;
// 
// #ifdef REL_DEBUG
//     log_string( "do_religion_war: start" );
// #endif
// 
//     argument = one_argument( argument, arg1 );
// 
//     if ( !strcmp(arg1, "status") )
//     {
// 	if ( (rel = get_religion_by_name(argument)) == NULL
// 	     && (rel = get_religion(ch)) == NULL )
// 	{
// 	    send_to_char( "You must specify a valid religion.\n\r", ch );
// 	    return;
// 	}
// 
// 	sprintf( buf, "War status for %s:\tVersus %s:\n\r", rel->name, rel->name );
// 	send_to_char( buf, ch );
// 	for ( opp = religion_list; opp != NULL; opp = opp->next )
// 	{
// 	    if ( opp == rel )
// 		continue;
// 	    status = religion_get_war_status( rel, opp );
// 	    other = religion_get_war_status( opp, rel );
// 	    sprintf( buf, "%22s:    %s\t\t%s\n\r", opp->name, war_status_name(status), war_status_name(other) );
// 	    send_to_char( buf, ch );
// 	}
//     }
//     else if ( (status = get_war_status_by_name(arg1)) != -1 )
//     {
// 	if ( !IS_IMMORTAL(ch) && !is_high_priest(ch) )
// 	{
// 	    send_to_char( "Only gods or high priests can declare war.\n\r", ch );
// 	    return;
// 	}
// 
// 	if ( (rel = get_religion(ch)) == NULL )
// 	{
// 	    send_to_char( "You're not even religious.\n\r", ch );
// 	    return;
// 	}
// 
// 	if ( (opp = get_religion_by_name(argument)) == NULL )
// 	{
// 	    send_to_char( "You must specify a valid religion.\n\r", ch );
// 	    return;
// 	}
// 
// 	if ( opp == rel )
// 	{
// 	    send_to_char( "You should always be at peace with yourself.\n\r", ch );
// 	    return;
// 	}
// 
// 	if ( status == religion_get_war_status(rel, opp) )
// 	{
// 	    send_to_char( "That wouldn't change anything.\n\r", ch );
// 	    return;
// 	}
// 	
// 	/* change the status */
// 	religion_set_war_status( rel, opp, status );
// 	send_to_char( "Ok.\n\r", ch );
// 	sprintf( buf, "%s are now at %s with %s.", rel->name, arg1, opp->name );
// 	info_message( ch, buf, TRUE );
//     }
//     else
//     {
// 	send_to_char( "Options: status <name>\n\r", ch );
// 	if ( IS_IMMORTAL(ch) || is_high_priest(ch) )
// 	{
// 	send_to_char( "         peace <name>\n\r", ch );
// 	send_to_char( "         war <name>\n\r", ch );
// 	}
//     }
// 
// #ifdef REL_DEBUG
//     log_string( "do_religion_war: done" );
// #endif
// 
// }

/*
DEF_DO_FUN(do_religion_set)
{
    RELIGION_DATA *rel;
    char arg1[MIL], arg2[MIL], arg3[MIL];
    int value1, value2;

#ifdef REL_DEBUG
    log_string( "do_religion_set: start" );
#endif

    if ( !IS_IMMORTAL(ch) )
    {
	send_to_char( "Only gods can set stats for their religion.\n\r", ch );
	return;
    }
    
    if ( (rel = get_religion(ch)) == NULL )
    {
	send_to_char( "You're not being worshipped.\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( !strcmp(arg1, "align") )
    {
	if ( !is_number(arg2) || !is_number(arg3) )
	{
	    send_to_char( "Syntax: align <min align> <max align>\n\r", ch );
	    return;
	}
	value1 = atoi(arg2);
	value2 = atoi(arg3);
	if ( -1000 > value1 || value1 > value2 || value2 > 1000 )
	{
	    send_to_char( "Alignment range is -1000 to 1000.\n\r", ch );
	    return;
	}
	// set it
	rel->min_align = value1;
	rel->max_align = value2;
	send_to_char( "Alignment range set.\n\r", ch );
    }
    else if ( !strcmp(arg1, "altar") )
    {
	if ( !is_number(arg2) )
	{
	    send_to_char( "Syntax: altar <vnum>\n\r", ch );
	    return;	    
	}
	value1 = atoi(arg2);
	if ( get_room_index(value1) == NULL )
	{
	    send_to_char( "That room doesn't exist.\n\r", ch );
	    return;
	}
	// set it
	rel->altar_room_vnum = value1;
	send_to_char( "Altar room set.\n\r", ch );
    }
    else if ( !strcmp(arg1, "guard") )
    {
	if ( !is_number(arg2) )
	{
	    send_to_char( "Syntax: guard <vnum>\n\r", ch );
	    return;	    
	}
	value1 = atoi(arg2);
	if ( get_mob_index(value1) == NULL )
	{
	    send_to_char( "That mobile doesn't exist.\n\r", ch );
	    return;
	}
	// set it
	rel->guard_vnum = value1;
	send_to_char( "Temple Guard set.\n\r", ch );
    }
    else if ( !strcmp(arg1, "power") )
    {
        if ( ch->level < MAX_LEVEL - 2 )
        {
            send_to_char( "This action is reserved for 108+ IMMs", ch );
        }
        else
        {
            if ( !is_number(arg2) )
            {
                send_to_char( "Syntax: power <value>\n\r", ch );
                return;	    
            }
            value1 = atoi(arg2);
            // set it
            rel->god_power = value1;
            send_to_char( "Power set.\n\r", ch );
        }
    }
    else if ( !strcmp(arg1, "relic") )
    {
	if ( !is_number(arg2) )
	{
	    send_to_char( "Syntax: relic <vnum>\n\r", ch );
	    return;	    
	}
	value1 = atoi(arg2);
	if ( get_obj_index(value1) == NULL )
	{
	    send_to_char( "That object doesn't exist.\n\r", ch );
	    return;
	}
	// set it
	rel->relic_vnum = value1;
	send_to_char( "Relic set.\n\r", ch );
    }
    else if ( !strcmp(arg1, "conserve") )
    {
	if( !is_number(arg2) )
	{
	    send_to_char( "Syntax:  religion set conserve <cutoff value for prayer granting>\n\r", ch );
	    return;
	}
	value1 = atoi(arg2);
	rel->conserve_at = value1;
	sprintf( arg3, "Prayers will not be granted if your faith pool has less than %d points.\n\r", value1 );
	send_to_char( arg3, ch );
    }
    else
    {
	send_to_char( "Syntax: align <min_align> <max_align>\n\r", ch );
	send_to_char( "        altar <vnum>\n\r", ch );
	send_to_char( "        guard <vnum>\n\r", ch );
    send_to_char( "        power <value>\n\r", ch );
	send_to_char( "        relic <vnum>\n\r", ch );
	send_to_char( "        conserve <cutoff value for prayer granting>\n\r", ch );
    }

#ifdef REL_DEBUG
    log_string( "do_religion_set: done" );
#endif

}
*/

/* curses and blessings */

typedef bool GOD_FUN( CHAR_DATA *ch, CHAR_DATA *victim, const char *god_name, sh_int duration );
typedef struct god_action GOD_ACTION;

struct god_action
{
    const char *name;
    GOD_FUN *fun;
    bool negative;
    const char *desc;
};

GOD_ACTION god_table[] =
{
    /* name,        function,       negative,   description */
    { "confuse",    &god_confuse,   TRUE,       "insanity (short duration)" },
    { "curse",      &god_curse,     TRUE,       "curse" },
    { "plague",     &god_plague,    TRUE,       "plague" },
    { "bless",      &god_bless,     FALSE,      "improved hitroll & stats, heroism & death's door" },
    { "slow",       &god_slow,      TRUE,       "slow & weaken" },
    { "speed",      &god_speed,     FALSE,      "improved AGI & DEX, haste & mantra" },
    { "heal",       &god_heal,      FALSE,      "improved VIT, heals over time" },
    { "enlighten",  &god_enlighten, FALSE,      "improved INT, +50% exp gain and faster learning" },
    { "protect",    &god_protect,   FALSE,      "improved AC & saves, sanctuary & protection from magic" },
    { "fortune",    &god_fortune,   FALSE,      "improved CHA & LUC, +50% gold, +5% chance to gain practices" },
    { "haunt",      &god_haunt,     TRUE,       "send ghosts to haunt" },
    { "cleanse",    &god_cleanse,   FALSE,      "remove divine curses" },
    { "defy",       &god_defy,      TRUE,       "remove divine blessings" },
    { NULL }
};

void show_god_syntax( CHAR_DATA *ch )
{
    int i;

    send_to_char( "Syntax: god <action> <name> [duration]\n\r", ch );
    send_to_char( "        god <action> all [duration]\n\r", ch );
    send_to_char( "        Duration is optional.\n\r", ch );
    send_to_char( "Valid actions and cost:\n\r", ch );

    for ( i = 0; god_table[i].name != NULL; i++ )
    {
        ptc( ch, "%-10s: %s\n\r", god_table[i].name, god_table[i].desc );
    }
}

/*
void show_pray_syntax( CHAR_DATA *ch )
{
    int i;
    char buf[MSL];

    send_to_char( "Syntax:  pray for <blessing>\n\r", ch );
    send_to_char( "         pray for <blessing/curse> on <player>\n\r", ch );
    send_to_char( "The following blesses/curses are available:\n\r\n\r", ch );

    for ( i = 0; god_table[i].name != NULL; i++ )
    {
	sprintf( buf, "%-10s (%s): %s\n\r",
		 god_table[i].name, god_table[i].mean ? "curse" : "bless", god_table[i].desc );
	send_to_char( buf, ch );
    }

}
*/

DEF_DO_FUN(do_god)
{
    DESCRIPTOR_DATA *d;
    char arg1[MIL], arg2[MIL], arg3[MIL];
    CHAR_DATA *victim;
    bool all = FALSE;
    int i;

    if ( !IS_IMMORTAL(ch) )
    {
        send_to_char( "You have no godly powers!\n\r", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    
    /* find command */
    for ( i = 0; god_table[i].name != NULL; i++ )
        if ( !strcmp(arg1, god_table[i].name) )
            break;

    if ( god_table[i].name == NULL )
    {
        show_god_syntax( ch );
        return;
    }

    if ( arg2[0] == '\0' )
    {
        send_to_char( "Whom do you want to use your powers on?\n\r", ch );
        return;
    }
    else if ( !strcmp(arg2, "all") || !strcmp(arg2, "world") )
        all = TRUE;

    /* find victim */
    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( IS_PLAYING(d->connected) )
        {
            victim = original_char( d->character );

            if ( victim == NULL
                || IS_NPC(victim)
                || IS_IMMORTAL(victim)
                || is_in_remort(victim)
                || (IS_SET(victim->act,PLR_WAR) && !in_religion_war(victim)) )
                continue;

            if ( all || is_name(arg2, victim->name) )
            {
                // See if we have a defined duration.
                sh_int duration = GOD_FUNC_DEFAULT_DURATION;
                if ( is_number(arg3) )
                {
                    duration = atoi(arg3);
                }

                /* execute command */
                (*god_table[i].fun)(ch, victim, "", duration);
                if ( !all ) return;
            }
        }

    if ( all )
        send_to_char( "Ok.\n\r", ch );
    else
        send_to_char( "Not possible -- victim is either in remort, warfare, or is nonexistant.\n\r", ch );
}

#define DEF_GOD_FUN(fun) bool fun( CHAR_DATA *ch, CHAR_DATA *victim, const char *god_name, sh_int duration )

DEF_GOD_FUN( god_bless )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_bless) )
    {
        if (ch)
            act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_bless;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_HITROLL;
    af.modifier  = 100;
    af.bitvector = AFF_DEATHS_DOOR;
    affect_to_char( victim, &af );
    
    af.location  = APPLY_STATS;
    af.bitvector = AFF_HEROISM;
    af.modifier  = 40;
    affect_to_char( victim, &af );

    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You bless $N.", ch, NULL, victim, TO_CHAR );
    act( "$N blesses you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s blesses you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_curse )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_curse) )
    {
        if (ch)
	act("$N is already cursed by the gods.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_curse;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_HITROLL;
    af.modifier  = -100;
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );
    
    af.location  = APPLY_SAVES;
    af.modifier  *= -1;
    affect_to_char( victim, &af );

    if( ch != NULL )
    {
    act( "You curse $N.", ch, NULL, victim, TO_CHAR );
    act( "$N curses you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s curses you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_heal )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_bless) )
    {
        if (ch)
	act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_bless;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_VIT;
    af.modifier  = 50;
    af.bitvector = AFF_HEAL;
    affect_to_char( victim, &af );
    
    if( ch != NULL )
    {
    act( "You begin to heal $N.", ch, NULL, victim, TO_CHAR );
    act( "$N begins to heal you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s begins to heal you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_speed )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_bless) )
    {
	if( ch )
	act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    affect_strip( victim, gsn_slow );

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_bless;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_AGI;
    af.modifier  = 50;
    af.bitvector = AFF_HASTE;
    affect_to_char( victim, &af );
    
    af.location  = APPLY_DEX;
    af.bitvector = AFF_MANTRA;
    affect_to_char( victim, &af );

    if( ch != NULL )
    {
    act( "You speed $N up.", ch, NULL, victim, TO_CHAR );
    act( "$N speeds you up.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s speeds you up.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_slow )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_curse) )
    {
	if( ch )
	act("$N is already cursed by the gods.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    affect_strip( victim, gsn_haste );

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_curse;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_AGI;
    af.modifier  = -50;
    af.bitvector = AFF_SLOW;
    affect_to_char( victim, &af );
    
    af.location  = APPLY_DEX;
    af.bitvector = AFF_WEAKEN;
    affect_to_char( victim, &af );

    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You slow and weaken $N.", ch, NULL, victim, TO_CHAR );
    act( "$N slows and weakens you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s slows and weakens you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_cleanse )
{
    // duration is ignored
    char buf[MSL];

    if ( !is_affected(victim, gsn_god_curse) )
    {
        if (ch)
	act( "$N isn't cursed by the gods.", ch, NULL, victim, TO_CHAR );
	return FALSE;
    }

    affect_strip( victim, gsn_god_curse );

    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You cleanse $N.", ch, NULL, victim, TO_CHAR );
    act( "$N cleanses you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s cleanses you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_defy )
{
    // duration is ignored
    char buf[MSL];

    if ( !is_affected(victim, gsn_god_bless) )
    {
        if (ch)
	act( "$N isn't blessed by the gods.", ch, NULL, victim, TO_CHAR );
	return FALSE;
    }

    affect_strip( victim, gsn_god_bless );

    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You defy $N's blessings.", ch, NULL, victim, TO_CHAR );
    act( "$N defies your blessings.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s defies your blessings.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_enlighten )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_bless) )
    {
        if (ch)
            act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_bless;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_INT;
    af.modifier  = 50;
    af.bitvector = AFF_LEARN;
    affect_to_char( victim, &af );
    
    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
        act( "You enlighten $N.", ch, NULL, victim, TO_CHAR );
        act( "$N enlightens you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	    if( god_name[0] == '\0' )
	        god_name = "Rimbol";
            sprintf( buf, "%s enlightens you.\n\r", god_name );
	    send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_protect )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim,gsn_god_bless) )
    {   
        if (ch)
            act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);

        return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = gsn_god_bless;
    af.level	 = 100;
    af.duration	 = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.modifier	 = -1000;
    af.location	 = APPLY_AC;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char( victim, &af );
    
    af.location  = APPLY_SAVES;
    af.bitvector = AFF_PROTECT_MAGIC;
    af.modifier  = -100;
    affect_to_char( victim, &af );

    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You protect $N.", ch, NULL, victim, TO_CHAR );
    act( "$N protects you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s protects you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_fortune )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_bless) )
    {
        if (ch)
	act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_bless;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_CHA;
    af.modifier  = 50;
    af.bitvector = AFF_FORTUNE;
    affect_to_char( victim, &af );
    af.location  = APPLY_LUC;
    af.modifier  = 50;
    affect_to_char( victim, &af );
    
    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You grant good fortune to $N.", ch, NULL, victim, TO_CHAR );
    act( "$N grants you good fortune.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s grants you good fortune.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_haunt )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_curse) )
    {
        if (ch)
	act("$N is already cursed by the gods.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_curse;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_LUC;
    af.modifier  = -50;
    af.bitvector = AFF_HAUNTED;
    affect_to_char( victim, &af );
    
    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You send ghosts to haunt $N.", ch, NULL, victim, TO_CHAR );
    act( "$N sends ghosts after you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s sends ghosts after you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_plague )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_curse) )
    {
        if (ch)
	act("$N is already cursed by the gods.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_curse;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 100;
    af.location  = APPLY_VIT;
    af.modifier  = -50;
    af.bitvector = AFF_PLAGUE;
    affect_to_char( victim, &af );
    
    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You bring the plague down on $N.", ch, NULL, victim, TO_CHAR );
    act( "$N brings the plague down on you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s brings the plague down on you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}

DEF_GOD_FUN( god_confuse )
{
    AFFECT_DATA af;
    char buf[MSL];

    if ( is_affected(victim, gsn_god_curse) )
    {
        if (ch)
	act("$N is already cursed by the gods.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_god_curse;
    af.level     = 100;
    af.duration  = duration != GOD_FUNC_DEFAULT_DURATION ? duration : 10;
    af.location  = APPLY_INT;
    af.modifier  = -50;
    af.bitvector = AFF_INSANE;
    affect_to_char( victim, &af );
    
    /* When called through auto-granting, there is no ch */
    if( ch != NULL )
    {
    act( "You confuse $N.", ch, NULL, victim, TO_CHAR );
    act( "$N confuses you.", victim, NULL, ch, TO_CHAR );
    }
    else
    {
	if( god_name[0] == '\0' )
	    god_name = "Rimbol";
        sprintf( buf, "%s confuses you.\n\r", god_name );
	send_to_char( buf, victim );
    }

    return TRUE;
}


/* Syntax:  pray for bless/speed/cleanse etc (on self)
            pray for bless/etc on <player>
	    pray for curse/defy/etc on <player>
 -- by Quirky, July/Aug 2003 */
/*
DEF_DO_FUN(do_prayer)
{
    char arg1[MIL], arg2[MIL];
    PRAYER_DATA *prayer;
    RELIGION_DATA *rel;
    RELIGION_DATA *vrel;
    char buf[MSL];
    int i, chance = 0;
    CHAR_DATA *victim;
    bool enlighten = FALSE, in_temple = FALSE, ch_is_priest = FALSE;
    int ticks;

    if( IS_NPC(ch) || ch->pcdata == NULL )
    {
	send_to_char( "NPCs shouldn't bother praying.\n\r", ch );
	return;
    }

    rel = get_religion(ch);

    if( IS_IMMORTAL(ch) )
    {
	if( rel == NULL )
	{
	    send_to_char( "You don't have a religion.\n\r", ch );
	    return;
	}

	if( argument[0] == '\0' )
	{
	    send_to_char( "Syntax:  prayer <character> grant/deny\n\r", ch );
	    return;
	}

        if( !str_cmp( argument, "list" ) )
        {
	    int i;

	    send_to_char( "The syntax that mortals use to pray:  {cpray [for] <god-action> [on <target>]{x\n\r", ch );

	    send_to_char( "Options for god-action:\n\r", ch );
	    for ( i = 0; god_table[i].name != NULL; i++ )
	    {
	        sprintf( buf, "%-10s (%s): %s\n\r",
	             god_table[i].name, god_table[i].mean ? "mean" : "kind", god_table[i].desc );
	        send_to_char( buf, ch );
	    }
	    send_to_char( "God-actions marked as 'mean' are less likely to be automatically granted.\n\r", ch );
	    return;
        }


	// Handling of prayer <char> grant/deny
	argument = one_argument( argument, buf );

	if( (victim = get_char_world(ch,buf)) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );	    
	    return;
	}

	if( (prayer = victim->pcdata->prayer_request) == NULL )
	{
	    send_to_char( "No prayer is pending.\n\r", ch );
	    return;
	}

	vrel = get_religion(victim);

	if( rel != vrel && get_trust(ch) < ML )
	{
	    send_to_char( "That prayer is not for you.\n\r", ch );
	    return;
	}

	argument = one_argument( argument, buf );
	if( !str_cmp(buf, "deny") )
	{
	    sprintf( buf, "{0$N's prayer (%s %s) denied.{x", god_table[prayer->prayer_num].name, prayer->victim->name );
	    wiznet( buf, victim, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );

	    free_mem( victim->pcdata->prayer_request, sizeof(PRAYER_DATA) );
	    victim->pcdata->prayer_request = NULL;
	    return;
	}

	if( !str_cmp(buf, "grant") )
	{
	    // Grant it!!
	    grant_prayer(victim);
	    return;
	}

	send_to_char( "Syntax:  prayer <character> grant/deny\n\r", ch );
	return;
    }

    if( argument[0] == '\0' )
    {
	send_to_char( "Pray what? ('prayer list' for options.)\n\r", ch );
	return;
    }

    if( !str_cmp( argument, "list" ) )
    {
	int i;

	send_to_char( "Syntax: pray [for] <god-action> [on <target>]\n\r", ch );

	send_to_char( "Options for god-action:\n\r", ch );
	for ( i = 0; god_table[i].name != NULL; i++ )
	{
	    sprintf( buf, "%-10s (%s): %s\n\r",
	         god_table[i].name, god_table[i].mean ? "mean" : "kind", god_table[i].desc );
	    send_to_char( buf, ch );
	}
	send_to_char( "Note that the god-actions marked as 'mean' are less likely to be granted.\n\r", ch );
	return;
    }

    
    // Now:  send a message to the imms, and then make an attempt to auto-interpret the prayer.
    // Lag will discourage its use except when people really need to use it (same delay as 'shout', 3 pulses).

    // In the room, the prayer is overheard, but not distinctly.
    act( "$n mumbles a little prayer to the powers that be.", ch, NULL, NULL, TO_ROOM );

    WAIT_STATE( ch, 12 );
    ch->pcdata->prayed_at = current_time;

    argument = one_argument( argument, arg1 );

    // Flexible syntax ... may choose to use, or omit, the word "for".
    // Echoes are slightly different when the "for" is included.
    if( !str_cmp( arg1, "for" ) )
    {
	// Echo to self:
	sprintf( buf, "{9You pray for {0%s.{x{x\n\r", argument );
	send_to_char( buf, ch );

	// Imms get the message:
	sprintf( buf, "{9$N is praying for {0%s{x{x", argument );
	wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );

	// Reassign arg1 so that it contains the blessing/curse name.
	argument = one_argument( argument, arg1 );
    }
    else
    {
	// Echo to self:
	sprintf( buf, "{9You pray: {0%s %s{x{x\n\r", arg1, argument );
	send_to_char( buf, ch );

	// Imms get the message:
	sprintf( buf, "{9$N is praying: {0%s %s{x{x", arg1, argument );
	wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
    }

    // Now, attempt to interpret the prayer.
    // Some failed checks will be informative to the person praying.
    // All failed checks will be informative to any immortals listening,
    // and if anyone chooses to react to the prayer, they may do so in any way.

    if( rel == NULL )
    {
	sprintf( buf, "{0...but $N is not in a religion.{x" );
	wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	return;
    }
    else if ( !IS_BETWEEN(rel->min_align, ch->alignment, rel->max_align) )
    {
	sprintf( buf, "{0...but $N is not in the alignment range for %s.{x", rel->name );
	wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	return;
    }

    // For what does the mortal pray?
    for( i=0; god_table[i].name != NULL; i++ )
    {
	// Small hack to allow the spelling of enlighten to be extended, or even shortened...
	// mostly so one can "pray for enlightenment" if they choose to spell it out.
	// Also, toggles enlighten to TRUE for later checks (enlighten is easiest to grant).

	if( !str_cmp(god_table[i].name, "enlighten") && !str_prefix(arg1, "enlightenment") )
	{
	    enlighten = TRUE;
	    break;
	}
	else if( !str_cmp(arg1, god_table[i].name) )
	    break;
    }

    // Prayer type not found
    if( god_table[i].name == NULL )
    {
	sprintf( buf, "{0...but there is no such blessing or curse.{x" );
	wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	return;
    }

    // Conserve faith when god_power is lower than an imm-settable religion_data variable:
    if( rel->god_power - god_table[i].cost < rel->conserve_at )
    {
	sprintf( buf, "{0...but the religion is conserving faith.{x" );
	wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	return;
    }

    // Do not allow brand-new neophytes to make demands! :P
    if( get_faith(ch) < god_table[i].cost )
    {
	send_to_char( "Be patient little Neophyte, your prayers will be answered once you have more faith.\n\r", ch );
	sprintf( buf, "{0...but $E has not yet earned enough faith to ask for this %s.",
		god_table[i].mean ? "curse" : "blessing" );
	wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	return;
    }

    if ( find_path(ch->in_room->vnum, rel->altar_room_vnum, TRUE, 10, NULL) != -1 )
        in_temple = TRUE;

    argument = one_argument( argument, arg2 );

    // Flexible syntax ... may omit "on".
    // If you include an "on", then you must include a target afterwards (i.e. arg2 can't be null).
    // If no target specified, the prayer is wasted :P  Ha ha, that's what you get for being stupid.
    if( !str_cmp(arg2, "on") )
    {
	argument = one_argument( argument, arg2 );
	if( arg2[0] == '\0' )
	{
	    sprintf( buf, "{0Pray for %s on whom?{x\n\r", god_table[i].name );
	    send_to_char( buf, ch );
	    sprintf( buf, "{0...but $E failed to specify a target.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}
    }

    // Prayer for something upon oneself
    if( arg2[0] == '\0'
        || !str_cmp(arg2,"me") || !str_cmp(arg2,"myself") || !str_cmp(arg2,"self") || !str_cmp(arg2,ch->name) )
    {
	victim = ch;

	// Players should welcome blessings, not beg for their removal!
	if( !strcmp(god_table[i].name, "defy") )
	{
	    sprintf( buf, "{0...but mortals should welcome blessings, not beg for their removal!{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	// Players should not waste their god(ess)'s points on cursing oneself.
	// Just deny them all.  Any imm who sees their message may manually place it, if they choose.
	if( god_table[i].mean )
	{
	    sprintf( buf, "{0...but what a pointless waste of faith!  Grant this request manually, if you choose.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	if( !strcmp(god_table[i].name, "cleanse") && !is_affected(ch, gsn_god_curse) )
	{
	    send_to_char( "{0...but you are not cursed.{x\n\r", ch );
	    sprintf( buf, "{0...but $N is not cursed.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	if( is_affected(ch, gsn_god_bless) )
	{
	    send_to_char( "{0...but you are already blessed.{x\n\r", ch );
	    sprintf( buf, "{0...but $N is already blessed.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	// If the requested prayer is not enlighten, and the praying person is not
	// in his/her temple, there is a 10% chance of auto-denying the prayer.
	if( !enlighten && !in_temple && number_range(0,9)==0 )
	{
	    sprintf( buf, "{0...but $E got unlucky, and the prayer was lost in the wind.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}
    }
    else
    {
	// Can't hide from the gods, and thus can't hide from prayer requests.
	// Abuse of this should be limited by the "one prayer per day" requirement.
	ignore_invisible = TRUE;

	if( (victim = get_char_world( ch, arg2 )) == NULL
	    || IS_NPC(victim) || victim->pcdata == NULL )
	{
	    send_to_char( "{0...but no target by that name was found.{x\n\r", ch );
	    sprintf( buf, "{0...but no target by that name was found.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	if( IS_IMMORTAL(victim) && !can_see(ch,victim) )
	{
	    send_to_char( "{0...but no target by that name was found.{x\n\r", ch );
	    sprintf( buf, "{0...but only mortals can be targetted by prayers.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	if( IS_IMMORTAL(victim) && can_see(ch,victim) )
	{
	    send_to_char( "{0...but you may only target mortals in your prayers.{x\n\r", ch );
	    sprintf( buf, "{0...but only mortals can be targetted by prayers.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
            return;
	}

	if( !str_cmp(god_table[i].name, "defy") && !is_affected(victim, gsn_god_bless) )
	{
            send_to_char( "{0...but that person is not affected by a divine blessing.{x\n\r", ch );
            sprintf( buf, "{0...but the target is not divinely blessed.{x" );
            wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
            return;
        }

   	if( !str_cmp(god_table[i].name, "cleanse") && !is_affected(victim, gsn_god_curse) )
        {
            send_to_char( "{0...but that person is not affected by a divine curse.{x\n\r", ch );
            sprintf( buf, "{0...but the target is not divinely cursed.{x" );
            wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
            return;
        }
    }

    // Necessary information, before we can continue

    vrel = get_religion(victim);

    ch_is_priest = is_priest(ch);

    if( god_table[i].mean )
    {
	// Can't curse people in your own religion (gods may choose to grant nonetheless)
	if( rel == vrel )
	{
	    sprintf( buf, "{0...but %s probably wouldn't like that.{x\n\r", rel->god );
	    send_to_char( buf, ch );
	    sprintf( buf, "{0...but $E should treat the people of $S religion with respect.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	// Neophytes cannot pray for curses on people.
	if( follower_get_rank(ch->pcdata->ch_rel) == RELIGION_RANK_NEO )
	{
	    sprintf( buf, "{0...but Neophytes are not considered educated enough to choose curses appropriately.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	// Curses cannot be requested on people who are too much lower in level than oneself.
	if( ch->level - victim->level > 10 )
	{
	    sprintf( buf, "{0...but the target is too much lower in level than $N.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	    return;
	}

	chance = 0;   // Base chance of success for a curse.

	// Modify chance of success.

	if( in_temple )
	    chance += 10;   // Temple improves chances a bit.

	if( ch_is_priest )
	    chance += 10;  // Priests have a bit of extra success.

	// Now see if the victim deserves the curse :P  If so, the odds are greatly raised.

	// Is the victim carrying ch's relic, or in the altar room ready to steal it?!
	// Is the victim on his way into the altar room, i.e. is he in the temple?
	if( rel->relic_obj->carried_by == victim || victim->in_room->vnum == rel->altar_room_vnum )
	    chance += 50;
	else if( find_path(victim->in_room->vnum, rel->altar_room_vnum, TRUE, 10, NULL) != -1 )
	    chance += 45;

	// Is the victim a member of the religion which currently possesses ch's relic?
	// Increased chance of success, but still the curse cannot be guaranteed.
    ROOM_INDEX_DATA *rel_room = get_obj_room(rel->relic_obj);
	if( vrel != NULL && rel_room != NULL && rel_room->vnum == vrel->altar_room_vnum )
	    chance += 20;
    }
    else
    {
	chance = 40;  // Base chance of success for a benevolent prayer.

	if( ch_is_priest && victim != ch && rel == vrel )
	    chance += 25;

	if( enlighten )
	    chance += 25;

	if( in_temple )
	    chance += 10;
    }

    // The chance of your prayer being granted (either curse or bless) is modified by the rank you hold.
    switch( follower_get_rank(ch->pcdata->ch_rel) )
    {
	case RELIGION_RANK_NEO:   // case 0:
	    chance += 5; break;   // Total of 70% for enlighten, 80% if in temple
	case 1:
	    chance += 10; break;
	case 2:
	    chance += 15; break;
	case 3:
	    chance += 20; break;
	case 4:
	    chance += 25; break;
	case 5:
	    chance += 30; break;
	default:
	    chance = 0; break;
    }

    // Chance is also modified by how (un)favoured you are by your god..
    //this value can raise or lower your chances by up to 25.
    chance += get_favour(ch);

	    sprintf( buf, "{0...chance is equal to %d.{x", chance );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );

    // "Roll the dice!!!", see if the odds are in the person's favour!
    if( chance < number_percent() )
    {
	sprintf( buf, "{0...but $E was unlucky, and $S prayer got lost in the wind.{x" );
	wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );
	return;
    }

    // Now indicate to the imms that this prayer will be granted shortly,
    // unless s/he intervenes and denies the prayer.

    /// How long is 'shortly'?
    if( enlighten || in_temple )
	ticks = 1;
    else
	ticks = 2;

    sprintf( buf, "{0This prayer will be granted automatically after %d full tick%s, drawing from %s's faith pool.{x",
								ticks,	(ticks>1)?"s":"", rel->god );
    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );

    sprintf( buf, "{0%s may choose to grant it sooner, or deny it, using 'prayer <char> grant/deny'.{x", rel->god );
    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );

    ch->pcdata->prayer_request = alloc_mem( sizeof(PRAYER_DATA) );
    ch->pcdata->prayer_request->prayer_num = i;
    ch->pcdata->prayer_request->victim = victim;
    ch->pcdata->prayer_request->ticks = ticks;
}


int get_favour( CHAR_DATA *ch )
{
	// Make similar to get_faith(ch)
	return ch->pcdata->ch_rel->favour;
}

void grant_prayer( CHAR_DATA *ch )
{
	RELIGION_DATA *rel = get_religion(ch);
	PRAYER_DATA *prayer = ch->pcdata->prayer_request;
	char buf[MAX_STRING_LENGTH];

	if( rel == NULL )
	{ 
	    free_prayer(ch);
	    return;
	}

	if( prayer == NULL ) return;

	if( get_god_name(ch) == NULL )
	{ 
	    free_prayer(ch);
	    return;
	}

	if( (*god_table[prayer->prayer_num].fun)( NULL, prayer->victim, get_god_name(ch), GOD_FUNC_DEFAULT_DURATION ) )
	{
	    send_to_char( "Your prayer has been granted.\n\r", ch );

	    sprintf( buf, "{0$N's prayer (%s %s) has been granted.{x", god_table[prayer->prayer_num].name, prayer->victim->name );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );

	    rel->god_power -= god_table[prayer->prayer_num].cost;

	    free_prayer(ch);
	}
	else
	{
	    sprintf( buf, "{0$N's prayer has not been granted.{x" );
	    wiznet( buf, ch, NULL, WIZ_RELIGION, 0, LEVEL_IMMORTAL );

	    send_to_char( "Your prayer could not be granted.\n\r", ch );
	    free_prayer(ch);
	}
	return;
}

void free_prayer( CHAR_DATA *ch )
{
    free_mem( ch->pcdata->prayer_request, sizeof(PRAYER_DATA) );
    ch->pcdata->prayer_request = NULL;
}
*/

// used by chosen subclass
DEF_DO_FUN( do_channel )
{
    AFFECT_DATA *paf = affect_find(ch->affected, gsn_divine_channel);
    int i;
    
    if ( paf == NULL )
    {
        send_to_char("You have no divine energy to channel.\n\r", ch);
        return;
    }
    
    if ( is_affected(ch, gsn_god_bless) )
    {
        send_to_char("You already have divine favour.\n\r", ch);
        return;
    }
    
    for ( i = 0; god_table[i].name; i++ )
    {
        // only blessings, not curses
        if ( god_table[i].negative )
            continue;
        
        if ( !strcmp(god_table[i].name, argument) )
            break;
    }
    if ( !god_table[i].name )
    {
        send_to_char("You may channel the following blessings:", ch);
        for ( i = 0; god_table[i].name; i++ )
            if ( !god_table[i].negative )
                ptc(ch, " %s", god_table[i].name);
        send_to_char("\n\r", ch);
        return;
    }
    
    int chance = -paf->modifier;
    WAIT_STATE(ch, PULSE_VIOLENCE);
    affect_strip(ch, gsn_divine_channel);
    
    const char *god_name = get_god_name(ch);
    
    if ( per_chance(chance) )
        (*god_table[i].fun)(NULL, ch, god_name, GOD_FUNC_DEFAULT_DURATION);
    else
        ptc(ch, "%s doesn't answer your call.\n\r", god_name);
}
