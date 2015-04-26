/*
 * tool for cloning areas, e.g. remort chambers
 * areas cloned must use relative vnums only in mprogs/triggers
 * by Henning Koehler <koehlerh@in.tum.de>
 */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"

#define SET_STR(target, source) {free_string(target);target=str_dup(source)}

/* variables indicating the required vnum shift for area cloning and cloned area
 * must be initialized before the cloning methods are called
 */
static int vnum_shift = 0;
static AREA_DATA *cloned_area = NULL;

int clone_shift( int vnum )
{
    if ( IS_BETWEEN(cloned_area->min_vnum, vnum, cloned_area->max_vnum) )
	return vnum + vnum_shift;
    else
	return vnum;
}

void add_area_world( AREA_DATA *area )
{
    area_last->next = area;
    area_last = area;

    area->vnum = top_area++;
}

void add_room_world( ROOM_INDEX_DATA *room )
{
    int iHash;

    if ( room->vnum > top_vnum_room )
        top_vnum_room = room->vnum;
    
    iHash			= value % MAX_KEY_HASH;
    room->next			= room_index_hash[iHash];
    room_index_hash[iHash]	= room;
}

/* clones the given area data using vnum_shift */
void clone_area( AREA_DATA *pArea )
{
    AREA_DATA *clone;
    char buf[MSL];

    if ( pArea == NULL )
    {
	bugf( "clone_area: NULL pointer given" );
	return;
    }

    /* create clone */
    clone = alloc_AREA_DATA();
    clone->next = NULL;
    clone->helps = NULL;
    clone->name = str_dup( pArea->name );
    clone->credits = str_dup( pArea->credits );
    clone->age = 0;
    clone->nplayer = 0;
    clone->reset_time = pArea->reset_time;
    clone->min_vnum = pArea->min_vnum + vnum_shift;
    clone->max_vnum = pArea->max_vnum + vnum_shift;
    clone->save = TRUE;
    clone->empty = TRUE;
    clone->builders = str_dup( pArea->builders );
    flag_copy( clone->area_flags, pArea->area_flags );
    clone->security = pArea->security;

    /* add to area_list */
    add_area_world( pArea );

    /* new (hopefully unique) default filename by vnum */
    sprintf( buf, "clone%d.are", clone->vnum );
    pArea->file_name = str_dup( buf );
}

EXTRA_DESC_DATA* clone_extra_list( EXTRA_DESC_DATA *extra )
{
    EXTRA_DESC_DATA *clone;
    
    if ( extra == NULL )
	return NULL;

    clone = new_extra_descr();
    SET_STR( clone->keyword, extra->keyword );
    SET_STR( clone->description, extra->description );

    clone->next = clone_extra_list( extra->next );
    return clone;
}

RESET_DATA* clone_reset_list( RESET_DATA *reset )
{
    RESET_DATA *clone;

    if ( reset == NULL )
	return NULL;

    /* basic copy */
    clone = new_reset_data();
    clone->command = reset->command;
    clone->arg1 = reset->arg1;
    clone->arg2 = reset->arg2;
    clone->arg3 = reset->arg3;
    clone->arg4 = reset->arg4;

    /* vnum shifts where needed */
    switch ( clone->command )
    {
    case 'M':
    case 'O':
    case 'P':
	clone->arg3 = clone_shift( clone->arg3 ); 
	/* no break here! */
    case E:
    case G:
	clone->arg1 = clone_shift( clone->arg1 ); 
	break;
    default:
	break;
    }

    clone->next = clone_reset_list( reset->next );
    return clone;
}

void clone_room( ROOM_INDEX_DATA *room )
{
    ROOM_INDEX_DATA *clone;

    if ( room == NULL )
    {
	bugf( "clone_room: NULL pointer given" );
	return;
    }

    /* create room */
    clone              = new_room_index();
    clone->vnum        = room->vnum + vnum_shift;
    clone->area        = get_vnum_area( clone->vnum );

    /* copy simple data */
    SET_STR( clone->name, room->name );
    SET_STR( clone->description, room->description );
    SET_STR( clone->owner, room->owner );
    flag_copy( clone->room_flags, room->room_flags );
    clone->sector_type = room->sector_type;
    clone->heal_rate   = room->heal_rate;
    clone->mana_rate   = room->mana_rate;
    clone->clan        = room->clan;
    clone->clan_rank   = room->clan_rank;

    /* clone extra descriptions & resets */
    clone->extra_descr = clone_extra_list( room->extra_descr );
    clone->reset_first = clone_reset_list( room->reset_first );

    /* exits are not cloned here as target rooms might not exit yet */
    
    add_room_world( room );
}

void clone_exits( ROOM_INDEX_DATA *room )
{
    ROOM_INDEX_DATA *clone, *to_room;
    EXIT_DATA *room_exit, *clone_exit;
    int door;

    if ( room == NULL )
    {
	bugf( "clone_exits: NULL pointer given" );
	return;
    }
    
    if ( (clone = get_room_index(room->vnum + vnum_shift)) == NULL )
    {
	bug( "clone_exits: clone room not found (%d)",  );
	return;
    }

    for ( door = 0; door < MAX_DIR; door++ )
	if ( (room_exit = room->exit[door]) != NULL )
	{
	    if ( clone->exit[door] != NULL )
	    {
		bugf( "clone_exits: exit %d in room %d already exists",
		      door, clone->vnum );
		return;
	    }
	    /* get original to_room */
	    if ( (to_room = room_exit->u1.to_room) == NULL )
		continue;
	    /* get cloned to_room */
	    if ( (to_room = get_room_index(clone_shift(to_room->vnum))) == NULL )
	    {
		bugf( "clone_exits: exit %d in room %d leads to uncloned room",
		      door, clone->vnum );
		return;
	    }
	    /* clone the exit */
	    clone_exit = new_exit();
	    clone_exit->u1.to_room = to_room;
	    flag_copy( clone_exit->exit_info, room_exit->exit_info );
	    flag_copy( clone_exit->rs_flags, room_exit->rs_flags );
	    clone_exit->key = shift_if_area( room_exit->key, room->area );
	    SET_STR( clone_exit->keyword, room_exit->keyword );
	    SET_STR( clone_exit->description, room_exit->description );
	    clone_exit->orig_door = room_exit->orig_door;
	    clone->exit[door] = clone_exit;
	}
}
    
void clone_mob( MOB_INDEX_DATA *mob )
{
    MOB_INDEX_DATA *clone;
    int i;

    if ( mob == NULL )
    {
	bugf( "clone_mob: NULL pointer given" );
	return;
    }
    
    /* create mob */
    clone              = new_mob_index();
    clone->vnum        = mob->vnum + vnum_shift;
    clone->area        = get_vnum_area( clone->vnum );

    /* copy simple data */
    clone->spec_fun = mob->spec_fun; 
    clone->group = mob->group;
    clone->new_format = mob->new_format;
    SET_STR( clone->player_name, mob->player_name );
    SET_STR( clone->short_descr, mob->short_descr );
    SET_STR( clone->long_descr, mob->long_descr );
    SET_STR( clone->description, mob->description );
    flag_copy( clone->act, mob->act );
    flag_copy( clone->affect_field, mob->affect_field );
    clone->alignment = mob->alignment;
    clone->level = mob->level;
    clone->hitroll = mob->hitroll;
    for ( i = 0; i < 3; i++ )
    {
	clone->hit[i] = mob->hit[i];
	clone->mana[i] = mob->mana[i];
	clone->damage[i] = mob->damage[i];
    }
    for ( i = 0; i < 4; i++ )
	clone->ac[i] = mob->ac[i];
    clone-> = mob->dam_type;
    flag_copy( clone->off_flags, mob->off_flags );
    flag_copy( clone->imm_flags, mob->imm_flags );
    flag_copy( clone->res_flags, mob->res_flags );
    flag_copy( clone->vuln_flags, mob->vuln_flags );
    clone-> = mob->;
    clone-> = mob->;
    clone-> = mob->;

}
