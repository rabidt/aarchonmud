/* grep command for finding obj/mobs/rooms with certain properties
 * by Henning Koehler <koehlerh@in.tum.de>
 */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "mob_stats.h"

void show_grep_syntax( CHAR_DATA *ch );
void grep_obj( CHAR_DATA *ch, const char *argument, int min_vnum, int max_vnum );
void grep_mob( CHAR_DATA *ch, const char *argument, int min_vnum, int max_vnum );
void grep_room( CHAR_DATA *ch, const char *argument, int min_vnum, int max_vnum );

typedef struct grep_data GREP_DATA;
struct grep_data
{
    GREP_DATA *next;
    sh_int stat;
    int value;
    char str_value[MIL];
    bool negate;
};

GREP_DATA* new_grep_data( sh_int stat, int value, const char *str_value, bool negate )
{
    GREP_DATA *gd = alloc_mem( sizeof(GREP_DATA) );

    if ( gd == NULL )
    {
	bugf( "new_grep_data: out of memory" );
	return NULL;
    }

    gd->next = NULL;
    gd->stat = stat;
    gd->value = value;
    gd->negate = negate;
    if ( str_value == NULL )
	gd->str_value[0] = '\0';
    else
	strcpy( gd->str_value, str_value );
    return gd;
}

void free_grep_data( GREP_DATA *gd )
{
    if ( gd != NULL )
	free_mem( gd, sizeof(GREP_DATA) );
}

void free_grep_list( GREP_DATA *gd )
{
    if ( gd != NULL )
    {
	free_grep_list( gd->next );
	free_grep_data( gd );
    }
} 

#define GREP_OBJ  0
#define GREP_MOB  1
#define GREP_ROOM 2

DEF_DO_FUN(do_grep)
{
    char arg1[MIL];
    char arg2[MIL];
    int min_vnum, max_vnum;
    int grep_type;

    if ( ch->in_room == NULL || ch->in_room->area == NULL )
    {
	bug( "do_grep: NULL room or area", 0 );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    /* determin grep type */
    if ( !str_cmp(arg1, "obj") || !str_cmp(arg1, "object") )
	grep_type = GREP_OBJ;
    else if ( !str_cmp(arg1, "mob") || !str_cmp(arg1, "mobile") )
	grep_type = GREP_MOB;
    else if ( !str_cmp(arg1, "room") )
	grep_type = GREP_ROOM;
    else
    {
	show_grep_syntax( ch );
	return;
    }

    /* determin vnum range */
    if ( !str_cmp(arg2, "area") )
    {
	if ( get_trust(ch) < L8 &&
	     !IS_BUILDER(ch, ch->in_room->area) )
	{
	    send_to_char( "You're not a builder of this area.\n\r", ch );
	    return;
	}

	min_vnum = ch->in_room->area->min_vnum;
	max_vnum = ch->in_room->area->max_vnum;
    }
    else if ( !str_cmp(arg2, "world") )
    {
	if ( get_trust(ch) < L8 )
	{
	    send_to_char( "You can only grep within areas.\n\r", ch );
	    return;
	}

	min_vnum = 0;
	switch ( grep_type )
	{
	case GREP_OBJ: max_vnum = top_vnum_obj; break;
	case GREP_MOB: max_vnum = top_vnum_mob; break;
	case GREP_ROOM: max_vnum = top_vnum_room; break;
	default: return;
	}
    }
    else
    {
	show_grep_syntax( ch );
	return;
    }

    /* call subroutines */
    switch ( grep_type )
    {
    case GREP_OBJ: grep_obj( ch, argument, min_vnum, max_vnum ); break;
    case GREP_MOB: grep_mob( ch, argument, min_vnum, max_vnum ); break;
    case GREP_ROOM: grep_room( ch, argument, min_vnum, max_vnum ); break;
    default: return;
    }
} 

/* Have recently expanded on grep quite a bit. Newer flags include, obj nocost, 
   mob vuln, res, imm, shopmob, room name - Astark */

void show_grep_syntax( CHAR_DATA *ch )
{
    send_to_char( "Syntax: grep [obj|mob|room] [area|world] <stat> .. <stat>\n\r", ch );
    send_to_char( "        each <stat> may be preceeded with 'not'\n\r", ch );
    send_to_char( "obj stats: name    <name>\n\r", ch );
    send_to_char( "           type    <item type>\n\r", ch );
    send_to_char( "           cost    <minimum gold>\n\r", ch );
    send_to_char( "           ops     <minimum OPs>\n\r", ch );
    send_to_char( "           lvl     <minimum level>\n\r", ch );
    send_to_char( "           weight  <minimum weight>\n\r", ch );
    send_to_char( "           wear    <location>\n\r", ch );
    send_to_char( "           extra   <extra stat>\n\r", ch );
    send_to_char( "           rating  <rating nr>\n\r", ch );
    send_to_char( "           heal    <min ratio>\n\r", ch );
    send_to_char( "           spell   <name>\n\r", ch );
    send_to_char( "           aff     <affect type>\n\r", ch );
    send_to_char( "           trigger <oprog trigger type>\n\r", ch );
    send_to_char( "           oprog   <oprog vnum>\n\r", ch );
    send_to_char( "           addflag\n\r", ch );
    send_to_char( "           spec\n\r", ch );
    send_to_char( "           belowspec\n\r", ch );
    send_to_char( "           ingame\n\r", ch );
    send_to_char( "           combine\n\r", ch );
    send_to_char( "mob stats: name    <name>\n\r", ch );
    send_to_char( "           wealth  <minimum gold>\n\r", ch );
    send_to_char( "           act     <act flag>\n\r", ch );
    send_to_char( "           aff     <affect flag>\n\r", ch );
    send_to_char( "           off     <offense flag>\n\r", ch );
    send_to_char( "           lvl     <minimum level>\n\r", ch );
    send_to_char( "           trigger <oprog trigger type>\n\r", ch );
    send_to_char( "           mprog   <mprog vnum>\n\r", ch );
    send_to_char( "           specfun <any|spec_fun>\n\r", ch );
    send_to_char( "           align   <minimum alignment>\n\r", ch );
    send_to_char( "           spec\n\r", ch );
    send_to_char( "           vuln    <vulnerabilities>\n\r", ch );
    send_to_char( "           res     <resists>\n\r", ch );
    send_to_char( "           imm     <immunities>\n\r", ch );
    send_to_char( "           ingame\n\r", ch );
    send_to_char( "           shopmob <no arg - shows mobs with shops>\n\r", ch );
    send_to_char( "           race    <race name>\n\r", ch );
    send_to_char( "room stats: heal   <min ratio>\n\r", ch );
    send_to_char( "           flag    <room flag>\n\r", ch );
    send_to_char( "           sector  <sector>\n\r", ch );
    send_to_char( "           name    <name>\n\r", ch );
    send_to_char( "           ingame\n\r", ch );
}


#define MAX_MATCH 1000

/***************************** Objects *******************************/

#define GREP_OBJ_NAME      0
#define GREP_OBJ_TYPE      1
#define GREP_OBJ_COST      2
#define GREP_OBJ_OPS       3
#define GREP_OBJ_ADDFLAG   4
#define GREP_OBJ_SPEC      5
#define GREP_OBJ_LEVEL     6
#define GREP_OBJ_SPELL     7
#define GREP_OBJ_WEAR      8
#define GREP_OBJ_INGAME    9
#define GREP_OBJ_RATING   10
#define GREP_OBJ_HEAL     11
#define GREP_OBJ_AFF      12
#define GREP_OBJ_EXTRA    13
#define GREP_OBJ_COMBINE  14
#define GREP_OBJ_BELOW_SPEC 15
#define GREP_OBJ_WEIGHT   16
#define GREP_OBJ_OPROG    17
#define GREP_OBJ_TRIGGER  18
#define NO_SHORT_DESC "(no short description)"

/* parses argument into a list of grep_data */
GREP_DATA* parse_obj_grep( CHAR_DATA *ch, const char *argument )
{
    GREP_DATA *gd;
    char arg1[MIL] = "";
    char arg2[MIL] = "";
    int stat = -1,
	value = 0,
	negate = FALSE;
    
    argument = one_argument( argument, arg1 );

    /* allow negation */
    while ( !str_cmp(arg1, "not") )
    {
	negate = !negate;
	argument = one_argument( argument, arg1 );
    }
    
    /* first check stats without parameter.. */
    if ( !str_cmp(arg1, "addflag") )
    {
	stat = GREP_OBJ_ADDFLAG;
    }
    else if ( !str_cmp(arg1, "spec") )
    {
	stat = GREP_OBJ_SPEC;
    }
    else if ( !str_cmp(arg1, "belowspec") )
    {
        stat = GREP_OBJ_BELOW_SPEC;
    }
    else if ( !str_cmp(arg1, "ingame") )
    {
	stat = GREP_OBJ_INGAME;
    }
    else if ( !str_cmp(arg1, "combine") )
    {
        stat = GREP_OBJ_COMBINE;
    }
    /* ..now stats with parameter */
    else
    {
	argument = one_argument( argument, arg2 );
	if ( !str_cmp(arg1, "name") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What name do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    stat = GREP_OBJ_NAME;
	}
	else if ( !str_cmp(arg1, "type") || !str_cmp(arg1, "item"))
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What type do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, type_flags)) == NO_FLAG )
	    {
		send_to_char( "Please specify a valid item type.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_OBJ_TYPE;
	}
	else if ( !str_cmp(arg1, "cost") )
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the minimum cost in gold.\n\r", ch );
		return NULL;
	    }
	    if ( (value = atoi(arg2)) < 1 )
	    {
		send_to_char( "Minimum worth must be positive.\n\r", ch );
		return NULL;
	    }
	    value *= 100; // value in silver
	    stat = GREP_OBJ_COST;
	}
    else if ( !str_cmp(arg1, "weight") )
    {
        if ( !is_number(arg2) )
        {
            send_to_char( "Please specify the minimum weight in 1/10th of lbs.\n\r", ch );
            return NULL;
        }
        if ( (value = atoi(arg2)) < 1 )
        {
            send_to_char( "Minimum weight must be positive.\n\r", ch );
            return NULL;
        }
        stat = GREP_OBJ_WEIGHT;
    }
	else if ( !str_cmp(arg1, "ops") )
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the minimum OPs.\n\r", ch );
		return NULL;
	    }
	    value = atoi(arg2);
	    stat = GREP_OBJ_OPS;
	}
	else if ( !str_cmp(arg1, "lvl") || !str_cmp(arg1, "level"))
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the minimum level.\n\r", ch );
		return NULL;
	    }
	    value = atoi(arg2);
	    stat = GREP_OBJ_LEVEL;
	}
	else if ( !str_cmp(arg1, "wear") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What wear location do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, wear_types)) == NO_FLAG )
	    {
		send_to_char( "That affect doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_OBJ_WEAR;
	}
	else if ( !str_cmp(arg1, "spell") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What spell do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = spell_lookup(arg2)) < 0 )
	    {
		send_to_char( "That spell doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_OBJ_SPELL;
	}
	else if ( !str_cmp(arg1, "aff") || !str_cmp(arg1, "affect") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What affect do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, apply_flags)) == NO_FLAG )
	    {
		send_to_char( "That affect doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_OBJ_AFF;
	}
	else if ( !str_cmp(arg1, "extra") || !str_cmp(arg1, "stat") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What extra stat do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, extra_flags)) == NO_FLAG )
	    {
		send_to_char( "That extra stat doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_OBJ_EXTRA;
	}
	else if ( !str_cmp(arg1, "rating") )
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the rating nr.\n\r", ch );
		return NULL;
	    }
	    value = atoi(arg2);
	    stat = GREP_OBJ_RATING;
	}
	else if ( !str_cmp(arg1, "heal") )
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the minimum heal ratio.\n\r", ch );
		return NULL;
	    }
	    value = atoi(arg2);
	    stat = GREP_OBJ_HEAL;
	}
    else if ( !str_cmp(arg1, "trigger") )
    {
        if ( arg2[0] == '\0' )
        {
        send_to_char( "What trigger do you want to grep for?\n\r", ch );
        return NULL;
        }
        if ( (value = flag_lookup(arg2, oprog_flags)) == NO_FLAG )
        {
        send_to_char( "That trigger doesn't exist.\n\r", ch );
        return NULL;
        }
        stat = GREP_OBJ_TRIGGER;
    }
    else if ( !str_cmp(arg1, "oprog") )
    {
        if ( !is_number(arg2) )
        {
        send_to_char( "Please specify the mprog vnum.\n\r", ch );
        return NULL;
        }
        value = atoi(arg2);
        stat = GREP_OBJ_OPROG;
    }
	else
	{
	    show_grep_syntax( ch );
	    return NULL;
	}
    }

    /* ok, parsing went fine, so create the grep_data */
    gd = new_grep_data( stat, value, arg2, negate );

    /* now see if we need to do more parsing */
    if ( argument[0] != '\0' )
    {
	if ( (gd->next = parse_obj_grep(ch, argument)) == NULL )
	{
	    free_grep_data( gd );
	    return NULL;
	}
    }

    return gd;
}

bool match_grep_obj( GREP_DATA *gd, OBJ_INDEX_DATA *obj, char *info )
{
    char buf[MIL] = "",
	msg[MIL] = "";
    bool match = FALSE;
    int obj_value;
    AFFECT_DATA *aff;

    switch ( gd->stat )
    {
    case GREP_OBJ_NAME:
	match = is_name( gd->str_value, obj->name );
	break;
    case GREP_OBJ_TYPE:
	match = (obj->item_type == gd->value);
	break;
    case GREP_OBJ_COST:
        if ( obj->item_type == ITEM_MONEY )
	{
	    obj_value = obj->value[0] + obj->value[1] * 100;
	    obj_value = UMAX(obj_value, obj->cost);
	}
	else
	    obj_value = obj->cost;
	match = (obj_value >= gd->value);
	sprintf( buf, "(%d gold)", obj_value / 100 );
	strcat( info, buf );
	break;
    case GREP_OBJ_WEIGHT:
        obj_value = obj->weight;
        match = (obj_value >= gd->value);
        sprintf( buf, "(%d dlbs)", obj_value );
        strcat( info, buf );
        break;
    case GREP_OBJ_OPS:
	obj_value = get_obj_index_ops( obj );
	match = (obj_value >= gd->value);
	sprintf( buf, "(%d ops)", obj_value );
	strcat( info, buf );
	break;
    case GREP_OBJ_LEVEL:
	match = (obj->level >= gd->value);
	break;
    case GREP_OBJ_RATING:
	match = (obj->diff_rating == gd->value);
	break;
    case GREP_OBJ_WEAR:
	//match = IS_SET(obj->wear_flags, gd->value);
    match = (obj->wear_type == gd->value);
	break;
    case GREP_OBJ_EXTRA:
	match = IS_SET(obj->extra_flags, gd->value);
	break;
    case GREP_OBJ_SPELL:
	match = has_spell(obj, gd->value);
	break;
    case GREP_OBJ_AFF:
	match = has_affect( obj, gd->value, msg );
	if ( msg[0] != '\0' )
	{
	    sprintf( buf, "(%s)", msg );
	    strcat( info, buf );
	}
	break;
    case GREP_OBJ_HEAL:
	if ( obj->item_type != ITEM_FURNITURE )
	    break;
	match = (obj->value[3] >= gd->value
		 || obj->value[4] >= gd->value);
	sprintf( buf, "(heal=%d/%d)", obj->value[3], obj->value[4] );
	strcat( info, buf );
	break;
    case GREP_OBJ_ADDFLAG:
	/* look for special affect */
	for ( aff = obj->affected; aff != NULL; aff = aff->next )
	    if ( aff->bitvector > 0 )
	    {
		match = TRUE;
		sprintf( buf, "(%s)", to_bit_name(aff->where, aff->bitvector) );
		strcat( info, buf );
		break;
	    }
	break;
    case GREP_OBJ_SPEC:
	match = !is_obj_in_spec( obj, msg );
	if ( msg[0] != '\0' )
	{
	    sprintf( buf, "(%s)", msg );
	    strcat( info, buf );
	}
	break;
    case GREP_OBJ_BELOW_SPEC:
        match = is_obj_below_spec( obj, msg );
        if ( msg[0] != '\0' )
        {
            sprintf( buf, "(%s)", msg );
            strcat( info, buf );
        }
        break;
    case GREP_OBJ_INGAME:
	match = is_obj_ingame( obj );
	break;
    case GREP_OBJ_COMBINE:
        match = (obj->combine_vnum > 0);
        if (match)
        {
            sprintf( buf, "(combine=%d)", obj->combine_vnum );
            strcat( info, buf );
        }
        break;
    case GREP_OBJ_TRIGGER:
    match = IS_SET( obj->oprog_flags, gd->value );
    break;
    case GREP_OBJ_OPROG:
    match = has_oprog( obj, gd->value );
    break;
    default: 
	break;
    }

    if ( gd->negate )
	match = !match;

    if ( match && gd->next != NULL )
	match = match_grep_obj( gd->next, obj, info );
    return match;
}

void grep_obj( CHAR_DATA *ch, const char *argument, int min_vnum, int max_vnum )
{
    char buf[MSL], info[MSL];
    BUFFER *buffer;

    OBJ_INDEX_DATA *obj;
    GREP_DATA *gd;
    int vnum,
	nMatch = 0;

    if ( (gd = parse_obj_grep(ch, argument)) == NULL )
	return;
    
    /* now the actual grep */
    buffer = new_buf();
    add_buf( buffer, "[ vnum] lvl (info) name\n\r\n\r" );
    for ( vnum = min_vnum; vnum <= max_vnum; vnum++ )
    {
	if ( (obj = get_obj_index(vnum)) == NULL )
	    continue;

	/* filter out unused objects */
	if ( !strcmp(obj->short_descr, NO_SHORT_DESC) )
	    continue;

	/* did we find a match? */
	info[0] = '\0';
	if ( !match_grep_obj(gd, obj, info) )
	    continue;
	
	/* to prevent mega-spam */
	if ( ++nMatch > MAX_MATCH )
	{
	    add_buf( buffer, "Too many matches, aborting.\n\r" );
	    break;
	}

	/* now the message */
	if ( info[0] != '\0' )
	    sprintf( buf, "[%5d] %3d %s %s\n\r", 
		     vnum, obj->level, info, obj->short_descr );
	else
	    sprintf( buf, "[%5d] %3d %s\n\r", 
		     vnum, obj->level, obj->short_descr );

	add_buf( buffer, buf );
    }
    
    free_grep_list( gd );
    page_to_char( buf_string(buffer), ch );
    free_buf( buffer );
} 

/***************************** Mobiles *******************************/

#define GREP_MOB_NAME      0
#define GREP_MOB_WEALTH    1
#define GREP_MOB_SPEC      2
#define GREP_MOB_AFF       3
#define GREP_MOB_OFF       4
#define GREP_MOB_LEVEL     5
#define GREP_MOB_INGAME    6
#define GREP_MOB_MPROG     7
#define GREP_MOB_ACT       8
#define GREP_MOB_TRIGGER   9
#define GREP_MOB_VULN     10
#define GREP_MOB_RES      11
#define GREP_MOB_IMM      12
#define GREP_MOB_SHOPMOB  13
#define GREP_MOB_SPECFUN  14
#define GREP_MOB_ALIGN    15
#define GREP_MOB_RACE     16

/* parses argument into a list of grep_data */
GREP_DATA* parse_mob_grep( CHAR_DATA *ch, const char *argument )
{
    GREP_DATA *gd;
    char arg1[MIL] = "";
    char arg2[MIL] = "";
    int stat = -1,
	value = 0,
	negate = FALSE;
    
    argument = one_argument( argument, arg1 );
    
    /* allow negation */
    while ( !str_cmp(arg1, "not") )
    {
	negate = !negate;
	argument = one_argument( argument, arg1 );
    }

    /* first check stats without parameter.. */
    if ( !str_cmp(arg1, "spec") )
    {
	stat = GREP_MOB_SPEC;
    }
    else if ( !str_cmp(arg1, "ingame") )
    {
	stat = GREP_MOB_INGAME;
    }
    else if ( !str_cmp(arg1, "shopmob") )
    {
        stat = GREP_MOB_SHOPMOB;
    }
    /* ..now stats with parameter */
    else
    {
	argument = one_argument( argument, arg2 );

	if ( !str_cmp(arg1, "name") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What name do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    stat = GREP_MOB_NAME;
	}
	else if ( !str_cmp(arg1, "wealth") )
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the minimum worth in gold.\n\r", ch );
		return NULL;
	    }
	    if ( (value = atoi(arg2)) < 1 )
	    {
		send_to_char( "Minimum wealth must be positive.\n\r", ch );
		return NULL;
	    }
	    value *= 100; // value in silver
	    stat = GREP_MOB_WEALTH;
	}
	else if ( !str_cmp(arg1, "lvl") || !str_cmp(arg1, "level") )
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the minimum level.\n\r", ch );
		return NULL;
	    }
	    value = atoi(arg2);
	    stat = GREP_MOB_LEVEL;
	}
    else if ( !str_cmp(arg1, "align") || !str_cmp(arg1, "alignment") )
    {
        if ( !is_number(arg2) )
        {
            send_to_char( "Please specify the minimum alignment.\n\r", ch );
            return NULL;
        }
        value = atoi(arg2);
        stat = GREP_MOB_ALIGN;
    }
	else if ( !str_cmp(arg1, "aff") || !str_cmp(arg1, "affect"))
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What affect do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, affect_flags)) == NO_FLAG )
	    {
		send_to_char( "That affect doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_MOB_AFF;
	}
	else if ( !str_cmp(arg1, "off") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What offense flag do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, off_flags)) == NO_FLAG )
	    {
		send_to_char( "That offense flag doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_MOB_OFF;
	}
	else if ( !str_cmp(arg1, "act") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What act do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, act_flags)) == NO_FLAG )
	    {
		send_to_char( "That act flag doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_MOB_ACT;
	}
	else if ( !str_cmp(arg1, "trigger") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What trigger do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, mprog_flags)) == NO_FLAG )
	    {
		send_to_char( "That trigger doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_MOB_TRIGGER;
	}
	else if ( !str_cmp(arg1, "mprog") )
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the mprog vnum.\n\r", ch );
		return NULL;
	    }
	    value = atoi(arg2);
	    stat = GREP_MOB_MPROG;
	}
	else if ( !str_cmp(arg1, "vuln"))
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What vuln do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, vuln_flags)) == NO_FLAG )
	    {
		send_to_char( "That vuln doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_MOB_VULN;
	}
        else if ( !str_cmp(arg1, "res"))
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What resist do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, res_flags)) == NO_FLAG )
	    {
		send_to_char( "That resist doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_MOB_RES;
	}
        else if ( !str_cmp(arg1, "imm"))
	{
            if ( arg2[0] == '\0' )
	    {
		 send_to_char( "What resist do you want to grep for?\n\r", ch );
		 return NULL;
	    }
	    if ( (value = flag_lookup(arg2, imm_flags)) == NO_FLAG )
	    {
 		send_to_char( "That immunity doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_MOB_IMM;
	}
        else if ( !str_cmp(arg1, "race"))
        {
            if ( arg2[0] == '\0' )
	    {
		 send_to_char( "Which race do you want to grep for?\n\r", ch );
		 return NULL;
	    }
            if ( (value = race_lookup(arg2)) < 0 )
	    {
 		send_to_char( "That race doesn't exist.\n\r", ch );
		return NULL;
	    }
            stat = GREP_MOB_RACE;
	}

    else if ( !str_cmp(arg1, "specfun") )
    {
        if ( arg2[0] == '\0' )
        {
        send_to_char( "What special function do you want to grep for?\n\r", ch );
        return NULL;
        }
        stat = GREP_MOB_SPECFUN;
    }
	else
	{
	    show_grep_syntax( ch );
	    return NULL;
	}
    }

    /* ok, parsing went fine, so create the grep_data */
    gd = new_grep_data( stat, value, arg2, negate );

    /* now see if we need to do more parsing */
    if ( argument[0] != '\0' )
    {
	if ( (gd->next = parse_mob_grep(ch, argument)) == NULL )
	{
	    free_grep_data( gd );
	    return NULL;
	}
    }

    return gd;
}

bool match_grep_mob( GREP_DATA *gd, MOB_INDEX_DATA *mob, char *info )
{
    char buf[MIL] = "",
	msg[MIL] = "";
    bool match = FALSE;
    long wealth;

    switch ( gd->stat )
    {
    case GREP_MOB_NAME:
	match = is_name( gd->str_value, mob->player_name );
	break;
    case GREP_MOB_WEALTH:
        wealth = mob_base_wealth(mob);
	match = (wealth >= gd->value);
	if ( mob->pShop == NULL )
	    sprintf( buf, "(%ld gold)", wealth / 100 );
	else
	    sprintf( buf, "(S %ld gold)", wealth / 100 );
	strcat( info, buf );
	break;
    case GREP_MOB_AFF:
	match = IS_SET( mob->affect_field, gd->value );
	break;
    case GREP_MOB_OFF:
	match = IS_SET( mob->off_flags, gd->value );
	break;
    case GREP_MOB_ACT:
	match = IS_SET( mob->act, gd->value );
	break;
    case GREP_MOB_LEVEL:
	match = (mob->level >= gd->value);
	break;
    case GREP_MOB_ALIGN:
        match = (mob->alignment >= gd->value);
        break;
    case GREP_MOB_TRIGGER:
	match = IS_SET( mob->mprog_flags, gd->value );
	break;
    case GREP_MOB_MPROG:
	match = has_mprog( mob, gd->value );
	break;
    case GREP_MOB_SPEC:
	match = !is_mob_in_spec( mob, msg );
	if ( msg[0] != '\0' )
	{
	    sprintf( buf, "(%s)", msg );
	    strcat( info, buf );
	}
	break;
    case GREP_MOB_INGAME:
	match = is_mob_ingame( mob );
	break;
    case GREP_MOB_SHOPMOB:
	match = (mob->pShop != NULL );
	break;
    case GREP_MOB_VULN:
	match = IS_SET( mob->vuln_flags, gd->value );
	break;
    case GREP_MOB_RES:
	match = IS_SET( mob->res_flags, gd->value );
	break;
    case GREP_MOB_IMM:
	match = IS_SET( mob->imm_flags, gd->value );
	break;
    case GREP_MOB_RACE:
        match = (mob->race == gd->value );
        break;
    default: 
	break;
    case GREP_MOB_SPECFUN:
        match = has_special( mob, gd->str_value, msg );
        if ( msg[0] != '\0' )
        {
            sprintf( buf, "(%s)", msg );
            strcat( info, buf );
        }    
    }

    if ( gd->negate )
	match = !match;

    if ( match && gd->next != NULL )
	match = match_grep_mob( gd->next, mob, info );
    return match;
}

void grep_mob( CHAR_DATA *ch, const char *argument, int min_vnum, int max_vnum )
{
    char buf[MSL], info[MSL];
    BUFFER *buffer;

    MOB_INDEX_DATA *mob;
    GREP_DATA *gd;
    int vnum,
	nMatch = 0;

    if ( (gd = parse_mob_grep(ch, argument)) == NULL )
	return;

    /* now the actual grep */
    buffer = new_buf();
    add_buf( buffer, "[ vnum] lvl (info) name\n\r\n\r" );
    for ( vnum = min_vnum; vnum <= max_vnum; vnum++ )
    {
	if ( (mob = get_mob_index(vnum)) == NULL )
	    continue;

	/* filter out unused mobs */
	if ( !strcmp(mob->short_descr, NO_SHORT_DESC) )
	    continue;

	/* did we find a match? */
	info[0] = '\0';
	if ( !match_grep_mob(gd, mob, info) )
	    continue;
	
	/* to prevent mega-spam */
	if ( ++nMatch > MAX_MATCH )
	{
	    add_buf( buffer, "Too many matches, aborting.\n\r" );
	    break;
	}

	/* now the message */
	if ( info[0] != '\0' )
	    sprintf( buf, "[%5d] %3d %s %s\n\r", 
		     vnum, mob->level, info, mob->short_descr );
	else
	    sprintf( buf, "[%5d] %3d %s\n\r", 
		     vnum, mob->level, mob->short_descr );

	add_buf( buffer, buf );
    }
    
    free_grep_list( gd );
    page_to_char( buf_string(buffer), ch );
    free_buf( buffer );
} 

/***************************** Rooms *********************************/

#define GREP_ROOM_HEAL     0
#define GREP_ROOM_FLAG     2
#define GREP_ROOM_INGAME   1
#define GREP_ROOM_SECTOR   3
/* Added room name so we can find awesome rooms left behind by
   former immortals -Astark */
#define GREP_ROOM_NAME     4

/* parses argument into a list of grep_data */
GREP_DATA* parse_room_grep( CHAR_DATA *ch, const char *argument )
{
    GREP_DATA *gd;
    char arg1[MIL] = "";
    char arg2[MIL] = "";
    int stat = -1,
	value = 0,
	negate = FALSE;
    
    argument = one_argument( argument, arg1 );
    
    /* allow negation */
    while ( !str_cmp(arg1, "not") )
    {
	negate = !negate;
	argument = one_argument( argument, arg1 );
    }

    /* first check stats without parameter.. */
    if ( !str_cmp(arg1, "ingame") )
    {
	stat = GREP_ROOM_INGAME;
    }
    /* ..now stats with parameter */
    else
    {
	argument = one_argument( argument, arg2 );

	if ( !str_cmp(arg1, "heal") )
	{
	    if ( !is_number(arg2) )
	    {
		send_to_char( "Please specify the minimum heal ratio.\n\r", ch );
		return NULL;
	    }
	    value = atoi(arg2);
	    stat = GREP_ROOM_HEAL;
	}
	else if ( !str_cmp(arg1, "flag") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What room flag do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, room_flags)) == NO_FLAG )
	    {
		send_to_char( "That room flag doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_ROOM_FLAG;
	}
	else if ( !str_cmp(arg1, "sector") )
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What sector do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    if ( (value = flag_lookup(arg2, sector_flags)) == NO_FLAG )
	    {
		send_to_char( "That sector doesn't exist.\n\r", ch );
		return NULL;
	    }
	    stat = GREP_ROOM_SECTOR;
	}
	else if ( !str_cmp(arg1, "name") )    /* Search for room names - Astark */
	{
	    if ( arg2[0] == '\0' )
	    {
		send_to_char( "What room NAME do you want to grep for?\n\r", ch );
		return NULL;
	    }
	    stat = GREP_ROOM_NAME;
	}
	else
	{
	    show_grep_syntax( ch );
	    return NULL;
	}
    }

    /* ok, parsing went fine, so create the grep_data */
    gd = new_grep_data( stat, value, arg2, negate );

    /* now see if we need to do more parsing */
    if ( argument[0] != '\0' )
    {
	if ( (gd->next = parse_room_grep(ch, argument)) == NULL )
	{
	    free_grep_data( gd );
	    return NULL;
	}
    }

    return gd;
}

bool match_grep_room( GREP_DATA *gd, ROOM_INDEX_DATA *room, char *info )
{
    char buf[MIL] = "";
    bool match = FALSE;

    switch ( gd->stat )
    {
    case GREP_ROOM_FLAG:
	match = IS_SET( room->room_flags, gd->value );
	break;
    case GREP_ROOM_SECTOR:
	match = (room->sector_type == gd->value);
	break;
    case GREP_ROOM_HEAL:
	match = (room->heal_rate >= gd->value
		 || room->mana_rate >= gd->value);
	sprintf( buf, "(heal=%d/%d)", room->heal_rate, room->mana_rate );
	strcat( info, buf );
	break;
    case GREP_ROOM_INGAME:
	match = is_room_ingame( room );
	break;
  /* We can find room names now */
    case GREP_ROOM_NAME:
        match = is_name( gd->str_value, remove_color(room->name) );
	break;
    default: 
	break;
    }

    if ( gd->negate )
	match = !match;

    if ( match && gd->next != NULL )
	match = match_grep_room( gd->next, room, info );
    return match;
}

void grep_room( CHAR_DATA *ch, const char *argument, int min_vnum, int max_vnum )
{
    char buf[MSL], info[MSL];
    BUFFER *buffer;

    ROOM_INDEX_DATA *room;
    GREP_DATA *gd;
    int vnum,
	nMatch = 0;

    if ( (gd = parse_room_grep(ch, argument)) == NULL )
	return;

    /* now the actual grep */
    buffer = new_buf();
    add_buf( buffer, "[ vnum] (info) name\n\r\n\r" );
    for ( vnum = min_vnum; vnum <= max_vnum; vnum++ )
    {
	if ( (room = get_room_index(vnum)) == NULL )
	    continue;

	/* did we find a match? */
	info[0] = '\0';
	if ( !match_grep_room(gd, room, info) )
	    continue;
	
	/* to prevent mega-spam */
	if ( ++nMatch > MAX_MATCH )
	{
	    add_buf( buffer, "Too many matches, aborting.\n\r" );
	    break;
	}

	/* now the message */
	if ( info[0] != '\0' )
	    sprintf( buf, "[%5d] %s %s\n\r", 
		     vnum, info, room->name );
	else
	    sprintf( buf, "[%5d] %s\n\r", 
		     vnum, room->name );

	add_buf( buffer, buf );
    }
    
    free_grep_list( gd );
    page_to_char( buf_string(buffer), ch );
    free_buf( buffer );
} 


/* miscellanous methods */
bool is_area_ingame( AREA_DATA *area )
{
    return !IS_SET(area->area_flags, AREA_REMORT)
        && area->security > 4 
        && area->security < 9;
}

bool is_obj_ingame( OBJ_INDEX_DATA *obj )
{
    return obj->level < LEVEL_IMMORTAL
	&& is_area_ingame(obj->area);
}

bool is_mob_ingame( MOB_INDEX_DATA *mob )
{
    return is_area_ingame( mob->area );
}

bool is_room_ingame( ROOM_INDEX_DATA *room )
{
    return !IS_SET(room->room_flags, ROOM_GODS_ONLY)
	&& !IS_SET(room->room_flags, ROOM_IMP_ONLY)
    && is_area_ingame(room->area);
}

bool is_mob_in_spec( MOB_INDEX_DATA *mob, char *msg )
{
    /* remort has no specs */
    if ( IS_SET(mob->area->area_flags, AREA_REMORT) )
	return TRUE;

    /* check level */
    if ( mob->level < 1 || mob->level > 200 )
    {
	sprintf( msg, "lvl=%d", mob->level );
	return FALSE;
    }

    /* check wealth */

    if ( mob->wealth_percent > 200 )
    {
        sprintf( msg, "wealth=%d%%", mob->wealth_percent );
        return FALSE;
    }

    /* check hp */
    if ( mob->hitpoint_percent != 100 )
    {
        sprintf( msg, "hp=%d%%", mob->hitpoint_percent );
        return FALSE;
    }

    /* check damage */
    if ( mob->damage_percent != 100 )
    {
        sprintf( msg, "damage=%d%%", mob->damage_percent );
        return FALSE;
    }

    /* check hitroll */
    if ( mob->hitroll_percent != 100 )
    {
        sprintf( msg, "hitroll=%d%%", mob->hitroll_percent );
        return FALSE;
    }

    /* otherwise it's ok */
    return TRUE;
}

int get_affect_cap( int location, int level )
{
    switch ( location )
    {
    case APPLY_STR:
    case APPLY_CON:
    case APPLY_VIT:
    case APPLY_AGI:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_DIS:
    case APPLY_CHA:
    case APPLY_LUC:
    case APPLY_STATS:
        return level < 90 ? 10 : level - 80;
    case APPLY_MANA:
    case APPLY_HIT:
    case APPLY_MOVE:
        return level < 90 ? 2 * (level + 10) : 10 * (level - 70);
    case APPLY_HITROLL:
    case APPLY_DAMROLL:
        return level < 90 ? (level + 10) / 10 : level - 80;
    case APPLY_AC:
        return level < 90 ? -(level + 10) : -10 * (level - 80);
    case APPLY_SAVES:
        return level < 90 ? -(level + 10) / 10 : -(level - 80);
    default:
        return 0;
    }
}

bool is_basic_stat( int location )
{
    switch ( location )
    {
    case APPLY_STR:
    case APPLY_CON:
    case APPLY_VIT:
    case APPLY_AGI:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_DIS:
    case APPLY_CHA:
    case APPLY_LUC:
        return TRUE;
    default:
        return FALSE;
    }
}

bool is_affect_cap_hard( int location )
{
    switch ( location )
    {
    case APPLY_STR:
    case APPLY_CON:
    case APPLY_VIT:
    case APPLY_AGI:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_DIS:
    case APPLY_CHA:
    case APPLY_LUC:
    case APPLY_STATS:
        return TRUE;
    default:
        return FALSE;
    }
}

float get_affect_ops( AFFECT_DATA *aff, int level )
{
    float factor = 0;
    float result, max_ops;

    switch ( aff->location )
    {
    case APPLY_STR:
    case APPLY_CON:
    case APPLY_VIT:
    case APPLY_AGI:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_DIS:
    case APPLY_CHA:
    case APPLY_LUC: factor = 0.25; break;
    case APPLY_STATS: factor = 2.5; break;
    case APPLY_SKILLS: factor = 10; break;
    case APPLY_MANA:
    case APPLY_HIT:
    case APPLY_MOVE: factor = 0.1; break;
    case APPLY_HITROLL:
    case APPLY_DAMROLL: factor = 1; break;
    case APPLY_AC: factor = -0.1; break;
    case APPLY_SAVES:
    case APPLY_SAVING_ROD:
    case APPLY_SAVING_PETRI:
    case APPLY_SAVING_BREATH:
    case APPLY_SAVING_SPELL: factor = -1; break;
    default: factor = 0; break;
    }

    result = aff->modifier * factor;
    max_ops = get_affect_cap( aff->location, level ) * factor;

    // negative stats only give 2/3 their OPs value
    if ( result < 0 )
        result = UMAX(result, -max_ops) * 2/3;
    // bonuses in excess of the soft-cap cost 1/3 extra
    else if ( result > max_ops && !is_affect_cap_hard(aff->location) )
        result += (result - max_ops) / 3;

    if ( aff->where == TO_AFFECTS )
    {
        switch (aff->bitvector)
        {
            case AFF_NONE: break;
            case AFF_HASTE: result += 50; break;
            case AFF_BERSERK:
            case AFF_PROTECT_MAGIC: result += 25; break;
            case AFF_FLYING:
            case AFF_SHIELD:
            case AFF_BATTLE_METER:
            case AFF_DETECT_INVIS:
            case AFF_DETECT_HIDDEN:
            case AFF_BREATHE_WATER: result += 20; break;
            case AFF_DARK_VISION: result += 15; break;
            case AFF_INFRARED:
            case AFF_DETECT_MAGIC:
            case AFF_DETECT_GOOD:
            case AFF_DETECT_EVIL: result += 10; break;
            default: result += 1000; break; // not allowed
        }
    }
    
    return result;
}

int get_obj_index_ops( OBJ_INDEX_DATA *obj )
{
    AFFECT_DATA *aff;
    float sum = 0;

    if ( obj == NULL )
	return 0;

    /* affects */
    for ( aff = obj->affected; aff != NULL; aff = aff->next )
        sum += get_affect_ops( aff, obj->level );

    /* weapon flags */
    if ( obj->item_type == ITEM_WEAPON )
    {
	if ( I_IS_SET(obj->value[4], WEAPON_FLAMING) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_FROST) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_VAMPIRIC) )
	    sum += 10;
	if ( I_IS_SET(obj->value[4], WEAPON_SHARP) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_VORPAL) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_SHOCKING) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_POISON) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_MANASUCK) )
	    sum += 10;
	if ( I_IS_SET(obj->value[4], WEAPON_MOVESUCK) )
	    sum += 10;
	if ( I_IS_SET(obj->value[4], WEAPON_DUMB) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_PUNCTURE) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_PARALYSIS_POISON) )
	    sum += 5;
	if ( I_IS_SET(obj->value[4], WEAPON_STORMING) )
	    sum += 15;
    }

    return (int) (sum);
}

int get_obj_ops( OBJ_DATA *obj )
{
    AFFECT_DATA *aff;
    float sum = 0;

    if ( obj == NULL )
	return 0;

    /* base ops */
    sum = get_obj_index_ops( obj->pIndexData );

    /* affects */
    for ( aff = obj->affected; aff != NULL; aff = aff->next )
        // only permanent affects count
        if ( aff->duration < 0 )
            sum += get_affect_ops( aff, obj->level );

    return (int) (sum);
}

// used for enchanting items
int get_obj_ops_by_duration( OBJ_DATA *obj, int duration )
{
    AFFECT_DATA *aff;
    float sum = 0;

    if ( obj == NULL )
        return 0;

    /* affects */
    for ( aff = obj->affected; aff != NULL; aff = aff->next )
        if ( aff->duration == duration )
            sum += get_affect_ops( aff, obj->level );

    return (int) (sum);
}

int get_translucency_spec_penalty( int level )
{
    if ( level < 90 )
        return (10 + level) / 5;
    else
        return 20 + 2 * (level - 90);
}

int get_obj_index_spec( OBJ_INDEX_DATA *obj, int level )
{
    int spec;

    if ( obj == NULL )
        return 0;

    if ( level < 90 )
    {
        spec = 50 + (level * 19/3 + (30+level) * obj->diff_rating)/3;
        // we don't use get_translucency_spec_penalty to avoid rounding issues
        //if ( CAN_WEAR(obj, ITEM_TRANSLUCENT) )
        if (IS_OBJ_STAT(obj, ITEM_TRANSLUCENT_EX))
            spec -= 2 * (10 + level);
        spec /= 10;
    }
    else /* These are objects above level 90 */
    {
        spec = 24 + 2 * (level - 90) + 4 * obj->diff_rating;
        //if ( CAN_WEAR(obj, ITEM_TRANSLUCENT) )
        if (IS_OBJ_STAT(obj, ITEM_TRANSLUCENT_EX))
            spec -= 20 + 2 * (level - 90);
    }
    
    // bonuses for align restrictions
    if ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) )
        spec += 1;
    if ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) )
        spec += 1;
    if ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) )
        spec += 1;
    
    // bonuses for class restrictions
    int class_count = classes_can_use(obj->extra_flags);
    if ( class_count <= MAX_CLASS * 2/3 )
        spec += 1;
    if ( class_count <= MAX_CLASS * 1/3 )
        spec += 1;
    
    // bonus for randomness
    if ( IS_OBJ_STAT(obj, ITEM_RANDOM)
      || IS_OBJ_STAT(obj, ITEM_RANDOM_PHYSICAL)
      || IS_OBJ_STAT(obj, ITEM_RANDOM_CASTER))
    {
        int ops = get_obj_index_ops(obj);
        if ( ops <= spec/2 )
            spec += 1;
        if ( ops == 0 )
            spec += 1;
    }
    
    // penalties for certain benefitial properties
    if ( IS_OBJ_STAT(obj, ITEM_BURN_PROOF) )
        spec -= 1;
    
    return spec;
}

int get_obj_spec( OBJ_DATA *obj )
{
    if ( obj == NULL )
        return 0;

    return get_obj_index_spec(obj->pIndexData, obj->level);
}

int weapon_dam_spec( int level, int weapon_class, bool twohanded )
{
    int spec = 0;
    
    if ( level < 90 )
    {
        if ( twohanded )
            spec = (level + 10) * 9/10;
        else
            spec = (level + 10) * 6/10;
    }
    else
    {
        if ( twohanded )
            spec = (level - 60) * 3;
        else
            spec = (level - 60) * 2;
    }
    
    // bows are effectively two-handed and provoke counter-attacks
    // this is compensated by dealing massive damage
    if ( weapon_class == WEAPON_BOW )
        spec *= 2.5;
    
    return spec;
}

int weapon_index_dam_spec( OBJ_INDEX_DATA *obj )
{
    int spec = weapon_dam_spec(obj->level, obj->value[0], IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS));
    
    if ( IS_OBJ_STAT(obj, ITEM_NONMETAL) )
        spec -= 1;
    
    return spec;
}

int average_weapon_dam( OBJ_DATA *obj )
{
    return obj->value[1] * (obj->value[2] + 1) / 2;
}

int average_weapon_index_dam( OBJ_INDEX_DATA *obj )
{
    return obj->value[1] * (obj->value[2] + 1) / 2;
}

bool can_wear( OBJ_INDEX_DATA *obj )
{
    //if ( !CAN_WEAR(obj, ITEM_TAKE) )
    if (obj->item_type == ITEM_NO_CARRY)
        return FALSE;
    
    if ( obj->item_type == ITEM_LIGHT )
        return TRUE;
        
    if ( CAN_WEAR(obj, ITEM_WEAR_FINGER)
        || CAN_WEAR(obj, ITEM_WEAR_NECK)
        || CAN_WEAR(obj, ITEM_WEAR_TORSO)
        || CAN_WEAR(obj, ITEM_WEAR_HEAD)
        || CAN_WEAR(obj, ITEM_WEAR_LEGS)
        || CAN_WEAR(obj, ITEM_WEAR_FEET)
        || CAN_WEAR(obj, ITEM_WEAR_HANDS)
        || CAN_WEAR(obj, ITEM_WEAR_ARMS)
        || CAN_WEAR(obj, ITEM_WEAR_ABOUT)
        || CAN_WEAR(obj, ITEM_WEAR_WAIST)
        || CAN_WEAR(obj, ITEM_WEAR_WRIST)
        || CAN_WEAR(obj, ITEM_WEAR_FLOAT)
        || CAN_WEAR(obj, ITEM_WEAR_SHIELD)
        || CAN_WEAR(obj, ITEM_HOLD)
        || CAN_WEAR(obj, ITEM_WIELD))
        return TRUE;

    return FALSE;
}

bool is_obj_in_spec( OBJ_INDEX_DATA *obj, char *msg )
{
    int value, spec;
    AFFECT_DATA *aff;

    if ( obj->level >= LEVEL_IMMORTAL || !can_wear(obj) )
        return TRUE;

    /* check ops */
    spec = get_obj_index_spec( obj, obj->level );
    value = get_obj_index_ops( obj );
    if ( value > spec )
    {
	sprintf( msg, "ops=%d/%d", value, spec );
	return FALSE;
    }

    // APPLY_STATS counts towards the cap of STR...LUC
    int all_stats = 0;
    for ( aff = obj->affected; aff != NULL; aff = aff->next )
        if ( aff->location == APPLY_STATS )
            all_stats += aff->modifier;
    
    /* affects - checks for hard caps per guidelines */
    for ( aff = obj->affected; aff != NULL; aff = aff->next )
    {
        if ( is_affect_cap_hard(aff->location) )
        {
            spec = get_affect_cap( aff->location, obj->level );
            value = aff->modifier + (is_basic_stat(aff->location) ? all_stats : 0);
            if ( value > spec )
            {
                sprintf( msg, "%s = %d/%d", name_lookup(aff->location, apply_flags), value, spec );
                return FALSE;
            }
        }
    }
    
    // affects - check for duplicates circumventing soft/hardcaps
    int last_loc = -1;
    for ( aff = obj->affected; aff != NULL; aff = aff->next )
    {
        if ( aff->where == TO_OBJECT && aff->location != -1)
        {
            if ( aff->location == last_loc )
            {
                sprintf( msg, "duplicates" );
                return FALSE;
            }
            last_loc = aff->location;
        }
    }
    
    /* check arrows */
    if ( obj->item_type == ITEM_ARROWS )
    {
	int maxnr, maxdam;
	maxnr = 100 + 50*obj->diff_rating;
	maxdam = 10 + obj->level + 5 * obj->diff_rating;
	if ( obj->value[0] > maxnr )
	{
	    sprintf( msg, "nr=%d/%d", obj->value[0], maxnr );
	    return FALSE;
	}
	if ( obj->value[1] > maxdam )
	{
	    sprintf( msg, "dam=%d/%d", obj->value[1], maxdam );
	    return FALSE;
	}
    }

    /* check weapon damage */
    if ( obj->item_type == ITEM_WEAPON )
    {
        spec = weapon_index_dam_spec(obj);
        value = average_weapon_index_dam(obj);
        if ( value > spec )
        {
            sprintf( msg, "dam=%d/%d", value, spec );
            return FALSE;
        }
    }

    /* check armor class */
    if ( obj->item_type == ITEM_ARMOR )
    {
	spec = (obj->level * 2 + 8) / 9;
	value = obj->value[0];
	if ( value > spec )
	{
	    sprintf( msg, "ac=%d/%d", value, spec );
	    return FALSE;
	}
    }

    return TRUE;
}

bool is_obj_below_spec( OBJ_INDEX_DATA *obj, char *msg )
{
    int value, spec;
    AFFECT_DATA *aff;

    if ( obj->level >= LEVEL_IMMORTAL || obj->level == 0 || !can_wear(obj) )
        return FALSE;
    
    /* check ops */
    spec = get_obj_index_spec( obj, obj->level );
    value = get_obj_index_ops( obj );
    // ignore objects with no bonuses at all
    if ( 0 < value && value < spec && !IS_SET(obj->extra_flags, ITEM_RANDOM) 
      && !IS_SET(obj->extra_flags, ITEM_RANDOM_PHYSICAL) 
      && !IS_SET(obj->extra_flags, ITEM_RANDOM_CASTER) )
    {
        sprintf( msg, "ops=%d/%d", value, spec );
        return TRUE;
    }
    
    /* check penalties */
    for ( aff = obj->affected; aff != NULL; aff = aff->next )
    {
        spec = get_affect_cap( aff->location, obj->level );
        value = aff->modifier;
        int factor = spec < 0 ? -1 : 1; // saves & AC

        // below negative spec
        if ( value*factor < -spec*factor )
        {
            sprintf( msg, "%s=%d/%d", name_lookup(aff->location, apply_flags), value, spec );
            return TRUE;
        }
    }
    
    /* check weapon damage */
    if ( obj->item_type == ITEM_WEAPON )
    {
        spec = weapon_index_dam_spec(obj);
        value = average_weapon_index_dam(obj);
        if ( value < spec )
        {
            sprintf( msg, "dam=%d/%d", value, spec );
            return TRUE;
        }
    }
    
    return FALSE;
}

bool has_mprog( MOB_INDEX_DATA *mob, int vnum )
{
    PROG_LIST *mprog;

    for ( mprog = mob->mprogs; mprog != NULL; mprog = mprog->next )
	if ( mprog->vnum == vnum )
	    return TRUE;
    return FALSE;
}

bool has_oprog( OBJ_INDEX_DATA *obj, int vnum )
{
    PROG_LIST *oprog;

    for ( oprog = obj->oprogs; oprog != NULL; oprog = oprog->next )
    if ( oprog->vnum == vnum )
        return TRUE;
    return FALSE;
}

bool has_special( MOB_INDEX_DATA *mob, char *spec_name, char *msg )
{
    const char *mob_spec_name = spec_name_lookup(mob->spec_fun);

    if ( mob_spec_name == NULL )
        return FALSE;

    if ( !strcmp(spec_name, "any") )
    {
        sprintf( msg, "%s", mob_spec_name );
        return TRUE;
    }

    return strcmp(spec_name, mob_spec_name) == 0;
}

bool has_spell( OBJ_INDEX_DATA *obj, int ID )
{
    if ( obj->item_type == ITEM_SCROLL
	 || obj->item_type == ITEM_PILL
	 || obj->item_type == ITEM_POTION )
    {
	return obj->value[1] == ID
	    || obj->value[2] == ID
	    || obj->value[3] == ID
	    || obj->value[4] == ID;
    }
    else if ( obj->item_type == ITEM_WAND
	      || obj->item_type == ITEM_STAFF )
    {
	return obj->value[3] == ID;
    }
    else
	return FALSE;
}

bool has_affect( OBJ_INDEX_DATA *obj, int loc, char *msg )
{
    AFFECT_DATA *aff;

    for ( aff = obj->affected; aff != NULL; aff = aff->next )
	if ( aff->location == loc )
	{
	    sprintf( msg, "%d %s", aff->modifier,
		     flag_bit_name(apply_flags, aff->location) );
	    return TRUE;
	}
    return FALSE;
}


