/*
 * tool for shifting an area's vnum range, e.g. to clone remort chambers
 * areas shifted must use relative vnums only in progs/triggers
 * be very careful on using this, since vnums in player/clan/? files are not shifted
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

extern bool exits_fixed;

/* local functions */
void shift_prog_list( PROG_LIST *list );

/* variables indicating the vnum shift and vnum range to be shifted
 */
static int vnum_shift = 0;
static int org_min_vnum = 0;
static int org_max_vnum = 0;

#define SHIFT(value) value = shift(value)

/* returns the shifted vnum */
int shift( int vnum )
{
    if ( IS_BETWEEN(org_min_vnum, vnum, org_max_vnum) )
	return vnum + vnum_shift;
    else
	return vnum;
}

/* returns weather an area already overlaps with given vnum range 
 * REQ: min_vnum <= max_vnum
 */
bool range_is_free( int min_vnum, int max_vnum )
{
    AREA_DATA *pArea;
    
    if ( min_vnum < 1 )
        return FALSE;
    
    for ( pArea = area_first; pArea; pArea = pArea->next )
        if ( pArea->max_vnum >= min_vnum && pArea->min_vnum <= max_vnum )
            return FALSE;

    return TRUE;
}

/* hash table updating */

void hash_remove_room( ROOM_INDEX_DATA *target )
{
    ROOM_INDEX_DATA *hash;
    int iHash;

    iHash = target->vnum % MAX_KEY_HASH;
    if ( (hash = room_index_hash[iHash]) == target )
	room_index_hash[iHash] = target->next;
    else
    {
	for ( ; hash != NULL; hash = hash->next )
	    if ( hash->next == target )
	    {
		hash->next = target->next;
		break;
	    }
	if ( hash == NULL )
	    bugf( "remove_room_hash: room not in hash table" );
    }
    target->next = NULL;
}

void hash_add_room( ROOM_INDEX_DATA *target )
{
    int iHash;

    if ( target->vnum > top_vnum_room )
        top_vnum_room = target->vnum;
    
    iHash	           = target->vnum % MAX_KEY_HASH;
    target->next           = room_index_hash[iHash];
    room_index_hash[iHash] = target;
}

void hash_remove_mob( MOB_INDEX_DATA *target )
{
    MOB_INDEX_DATA *hash;
    int iHash;

    iHash = target->vnum % MAX_KEY_HASH;
    if ( (hash = mob_index_hash[iHash]) == target )
	mob_index_hash[iHash] = target->next;
    else
    {
	for ( ; hash != NULL; hash = hash->next )
	    if ( hash->next == target )
	    {
		hash->next = target->next;
		break;
	    }
	if ( hash == NULL )
	    bugf( "remove_room_hash: room not in hash table" );
    }
    target->next = NULL;
}

void hash_add_mob( MOB_INDEX_DATA *target )
{
    int iHash;

    if ( target->vnum > top_vnum_mob )
        top_vnum_mob = target->vnum;
    
    iHash	           = target->vnum % MAX_KEY_HASH;
    target->next           = mob_index_hash[iHash];
    mob_index_hash[iHash]  = target;
}

void hash_remove_obj( OBJ_INDEX_DATA *target )
{
    OBJ_INDEX_DATA *hash;
    int iHash;

    iHash = target->vnum % MAX_KEY_HASH;
    if ( (hash = obj_index_hash[iHash]) == target )
	obj_index_hash[iHash] = target->next;
    else
    {
	for ( ; hash != NULL; hash = hash->next )
	    if ( hash->next == target )
	    {
		hash->next = target->next;
		break;
	    }
	if ( hash == NULL )
	    bugf( "remove_room_hash: room not in hash table" );
    }
    target->next = NULL;
}

void hash_add_obj( OBJ_INDEX_DATA *target )
{
    int iHash;

    if ( target->vnum > top_vnum_obj )
        top_vnum_obj = target->vnum;
    
    iHash	           = target->vnum % MAX_KEY_HASH;
    target->next           = obj_index_hash[iHash];
    obj_index_hash[iHash]  = target;
}

/* now the routines for shifting of the data types around */

/***************************** AREA ************************/

void shift_area_data( AREA_DATA *area )
{
    if ( area == NULL )
    {
	bugf( "shift_area: NULL pointer given" );
	return;
    }

    SHIFT( area->min_vnum );
    SHIFT( area->max_vnum );
    SET_BIT( area->area_flags, AREA_CHANGED );
}

/***************************** ROOM ************************/

void shift_reset_list( RESET_DATA *reset )
{
    for ( ; reset != NULL; reset = reset->next )
	switch ( reset->command )
	{
	case 'M':
	case 'O':
	case 'P':
	    SHIFT( reset->arg3 ); 
	    /* no break here! */
	case 'E':
	case 'G':
	    SHIFT( reset->arg1 ); 
	    break;
	default:
	    break;
	}
}

void shift_exit( EXIT_DATA *exit )
{
    if ( exit != NULL )
    {
	SHIFT( exit->key );
	if ( !exits_fixed )
	    SHIFT( exit->u1.vnum );
    }
}
    
void shift_room( ROOM_INDEX_DATA *room )
{
    int door;

    if ( room == NULL )
    {
	bugf( "shift_room: NULL pointer given" );
	return;
    }

    /* remove room from hash table, shift, and add back */
    hash_remove_room( room );
    SHIFT( room->vnum );
    hash_add_room( room );

    /* shift exits and resets as well */
    for ( door = 0; door < MAX_DIR; door++ )
	if ( room->exit[door] != NULL )
	    shift_exit( room->exit[door] );

    shift_reset_list( room->reset_first );

    if ( room->rprogs != NULL )
        shift_prog_list( room->rprogs );
}

/***************************** MOB *************************/

void shift_shop( SHOP_DATA *shop )
{
    if ( shop != NULL )
	SHIFT(shop->keeper);
}

void shift_prog_list( PROG_LIST *list )
{
    for ( ; list != NULL; list = list->next )
	SHIFT( list->vnum );
}

void shift_mob( MOB_INDEX_DATA *mob )
{
    if ( mob == NULL )
    {
	bugf( "shift_mob: NULL pointer given" );
	return;
    }
    
    /* remove mob from hash table, shift, and add back */
    hash_remove_mob( mob );
    SHIFT( mob->vnum );
    hash_add_mob( mob );

    if ( mob->pShop != NULL )
	shift_shop( mob->pShop );
    
    if ( mob->mprogs != NULL )
	shift_prog_list( mob->mprogs );
}

/***************************** OBJ *************************/

void shift_obj( OBJ_INDEX_DATA *obj )
{
    if ( obj == NULL )
    {
	bugf( "shift_obj: NULL pointer given" );
	return;
    }
    
    /* remove obj from hash table, shift, and add back */
    hash_remove_obj( obj );
    SHIFT( obj->vnum );
    hash_add_obj( obj );

    SHIFT( obj->combine_vnum );

    /* portals */
    if ( obj->item_type == ITEM_PORTAL )
	SHIFT( obj->value[3] );
    /* container locks */
    if ( obj->item_type == ITEM_CONTAINER )
	SHIFT( obj->value[2] );

    if ( obj->oprogs != NULL )
    shift_prog_list( obj->oprogs );
}

/***************************** MPROGS **********************/

void shift_prog_code( PROG_CODE *prog )
{
    if ( prog == NULL )
    {
	bugf( "shift_prog: NULL pointer given" );
	return;
    }

    SHIFT( prog->vnum );
}

/* now the big mama functions */

void shift_area( AREA_DATA *area, int shift, bool area_only )
{
    ROOM_INDEX_DATA *room;
    MOB_INDEX_DATA *mob;
    OBJ_INDEX_DATA *obj;
    PROG_CODE *prog;
    int index;

    if ( area == NULL )
    {
        bugf( "shift_area: NULL pointer given" );
        return;
    }

    if ( !range_is_free(area->min_vnum + shift, area->max_vnum + shift) )
    {
        bugf( "shift_area: range not free (%d - %d -> %d)",
                area->min_vnum, area->max_vnum, area->min_vnum + shift );
        return;
    }

    /* set global shift parameters */
    org_min_vnum = area->min_vnum;
    org_max_vnum = area->max_vnum;
    vnum_shift = shift;

    /* do the shift on everything needed */
    if ( area_only )
    {
        shift_area_data( area );
        for ( index = org_min_vnum; index <= org_max_vnum; index++ )
        {
            if ( (room = get_room_index(index)) != NULL )
                shift_room( room );
            if ( (mob = get_mob_index(index)) != NULL )
                shift_mob( mob );
            if ( (obj = get_obj_index(index)) != NULL )
                shift_obj( obj );
        }
    }
    else
    {
        for ( area = area_first; area != NULL; area = area->next )
            shift_area_data( area );

        for ( index = 1; index <= top_vnum_room; index++ )
            if ( (room = get_room_index(index)) != NULL )
                shift_room( room );

        for ( index = 1; index <= top_vnum_mob; index++ )
            if ( (mob = get_mob_index(index)) != NULL )
                shift_mob( mob );

        for ( index = 1; index <= top_vnum_obj; index++ )
            if ( (obj = get_obj_index(index)) != NULL )
                shift_obj( obj );
    }

    for ( prog = mprog_list; prog != NULL; prog = prog->next )
        shift_prog_code( prog );
    for ( prog = oprog_list; prog != NULL; prog = prog->next )
        shift_prog_code( prog );
    for ( prog = aprog_list; prog != NULL; prog = prog->next )
        shift_prog_code( prog );
    for ( prog = rprog_list; prog != NULL; prog = prog->next )
        shift_prog_code( prog );

    if ( area->aprogs != NULL )
        shift_prog_list( area->aprogs );
}

DEF_DO_FUN(do_ashift)
{
    char buf[MSL];
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];

    AREA_DATA *area;
    int shift;
    bool area_only;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0'
	 || !is_number(arg1) || !is_number(arg2) )
    {
	send_to_char( "Syntax: ashift <area vnum> <new min_vnum> <area|world>\n\r", ch );
	send_to_char( "Make sure that the area uses relative vnums in progs.\n\r", ch );
	return;
    }

    if ( (area = get_area_data(atoi(arg1))) == NULL )
    {
         send_to_char( "That area vnum does not exist.\n\r", ch );
         return;
    }

    if ( !strcmp(arg3, "area") )
	area_only = TRUE;
    else if ( !strcmp(arg3, "world") )
	area_only = FALSE;
    else
    {
	send_to_char( "Third parameter must be either 'area' or 'world'.\n\r", ch );
	send_to_char( " area: change shifted area only\n\r", ch );
	send_to_char( "world: adjust all references to shifted area\n\r", ch );
	return;
    }
    
    shift = atoi(arg2) - area->min_vnum;

    if ( !range_is_free(area->min_vnum + shift, area->max_vnum + shift) )
    {
         send_to_char( "That vnum range is already in use.\n\r", ch );
         return;
    }

    sprintf( buf, "Shifting area %s from %d - %d to %d - %d.\n\r",
	     area->name, area->min_vnum, area->max_vnum,
	     area->min_vnum + shift, area->max_vnum + shift );
    log_string( buf );
    send_to_char( buf, ch );

    shift_area( area, shift, area_only );
    send_to_char( "Done.\n\r", ch );
}

/***************************** RVNUMS **********************/

/* some stuff for changing absolute into relative vnums */

/* replaces absolute vnums by relative vnums in given string */
char* rel_string( const char *str, int min_vnum, int max_vnum )
{
    static char rel_str[MSL];
    char last_word[MIL], c;
    int lw_index = 0,
	rel_index = 0,
	str_index = 0,
	i, value;
    bool reading_int = TRUE;

    if ( str == NULL )
	return NULL;

    do
    {
	c = str[str_index++];

	/* digit while reading int */
	if ( reading_int && '0' <= c && c <= '9' )
	{
	    last_word[lw_index++] = c;
	}
	else
	{
	    /* seperator */
	    if ( c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0' )
	    {
		if ( reading_int && lw_index > 0)
		{
		    /* relativate number and add to rel_str */
		    last_word[lw_index] = '\0';
		    value = atoi( last_word );
		    if ( IS_BETWEEN(min_vnum, value, max_vnum) )
			sprintf( last_word, "r%d", value - min_vnum );
		    for ( i = 0; last_word[i] != '\0'; i++ )
			rel_str[rel_index++] = last_word[i];
		    lw_index = 0;
		}
		else
		    reading_int = TRUE;
	    }
	    /* just some char */
	    else
	    {
		if ( reading_int )
		{
		    /* add remaining string */
		    for ( i = 0; i < lw_index; i++ )
			rel_str[rel_index++] = last_word[i];
		    lw_index = 0;
		    reading_int = FALSE;
		}
	    }
	    rel_str[rel_index++] = c;
	}
    } while ( c != '\0' );

    return rel_str;
}

void rel_prog_list( PROG_LIST *list, int min_vnum, int max_vnum )
{
    char *rel;

    while ( list != NULL )
    {
	if ( list->trig_type == TRIG_GIVE )
	{
	    rel = rel_string( list->trig_phrase, min_vnum, max_vnum );
	    if ( strcmp(rel, list->trig_phrase) )
	    {
		free_string( list->trig_phrase );
		list->trig_phrase = str_dup( rel );
	    }
	}
	list = list->next;
    }
}

void rel_prog_code( PROG_CODE *prog, int min_vnum, int max_vnum )
{
    char *rel;

    if ( prog == NULL )
	return;

    if ( !prog->is_lua && IS_BETWEEN(min_vnum, prog->vnum, max_vnum) )
    {
	rel = rel_string( prog->code, min_vnum, max_vnum );
	if ( strcmp(rel, prog->code) )
	{
	    free_string( prog->code );
	    prog->code = str_dup( rel );
	}
    }
}

void rel_area( AREA_DATA *area )
{
    MOB_INDEX_DATA *mob;
    OBJ_INDEX_DATA *oid;
    ROOM_INDEX_DATA *rid;
    PROG_CODE *prog;
    int index;

    for ( index = area->min_vnum; index <= area->max_vnum; index++ )
    {
        if ( (mob = get_mob_index(index)) != NULL )
            rel_prog_list( mob->mprogs, area->min_vnum, area->max_vnum );
        if ( (oid = get_obj_index(index)) != NULL )
            rel_prog_list( oid->oprogs, area->min_vnum, area->max_vnum );
        if ( (rid = get_room_index(index)) != NULL )
            rel_prog_list( rid->rprogs, area->min_vnum, area->max_vnum );

        rel_prog_list( area->aprogs, area->min_vnum, area->max_vnum );

    }

    for ( prog = mprog_list; prog != NULL; prog = prog->next )
        rel_prog_code( prog, area->min_vnum, area->max_vnum );

    for ( prog = oprog_list; prog != NULL; prog = prog->next )
        rel_prog_code( prog, area->min_vnum, area->max_vnum );

    for ( prog = aprog_list; prog != NULL; prog = prog->next )
        rel_prog_code( prog, area->min_vnum, area->max_vnum );

    for ( prog = rprog_list; prog != NULL; prog = prog->next )
        rel_prog_code( prog, area->min_vnum, area->max_vnum );


    SET_BIT( area->area_flags, AREA_CHANGED );
}

DEF_DO_FUN(do_rvnum)
{
    char buf[MSL];
    char arg1[MIL];
    AREA_DATA *area;
    
    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Syntax: rvnum <area vnum>\n\r", ch );
	send_to_char( "        rvnum this\n\r", ch );
	return;
    }


    if ( !str_cmp(arg1, "this") )
	area = ch->in_room->area;
    else if ( !is_number(arg1) )
    {
	send_to_char( "Syntax: rvnum <area vnum>\n\r", ch );
	send_to_char( "        rvnum this\n\r", ch );
	return;
    }
    else if ( (area = get_area_data(atoi(arg1))) == NULL )
    {
         send_to_char( "That area vnum does not exist.\n\r", ch );
         return;
    }

    if ( !IS_BUILDER(ch, area) )
    {
         send_to_char( "You are not a builder in this area.\n\r", ch );
         return;
    }

    sprintf( buf, "Making vnums in progs & triggers in area %s relative.\n\r",
	     area->name );
    log_string( buf );
    send_to_char( buf, ch );
    rel_area( area );
    send_to_char( "Done.\n\r", ch );
}



