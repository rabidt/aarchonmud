/* magic tattoos that can be worn under equipment
 * tattoo affects increase as player advances in level
 * only add affect if no equipment worn over it
 * by Henning Koehler <koehlerh@in.tum.de>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "buffer_util.h"
#include "tattoo.h"
#include "tables.h"
#include "mudconfig.h"

#define TATTOO(ID)         tattoo_data_list[ID]
#define TATTOO_ID(ch,loc)  (ch)->pcdata->tattoos[loc]
#define NO_LOC(loc)        (loc < 0 || loc >= MAX_WEAR)
#define NO_ID(ID)          (ID < 0 || ID >= MAX_TATTOO)

/***************************** tattoo_list ***************************/

void add_tattoo( tattoo_list tl, int loc, int ID )
{
    if ( NO_LOC(loc) )
    {
	bug( "add_tattoo: invalid location (%d)", loc );
	return;
    }

    tl[loc] = ID;
}

void remove_tattoo( tattoo_list tl, int loc )
{
    if ( NO_LOC(loc) )
    {
	bug( "remove_tattoo: invalid location (%d)", loc );
	return;
    }
    
    tl[loc] = TATTOO_NONE;
}

int get_tattoo( tattoo_list tl, int loc )
{
    if ( NO_LOC(loc) )
    {
	bug( "get_tattoo: invalid location (%d)", loc );
	return TATTOO_NONE;
    }
    
    return tl[loc];
}

bool is_tattoo_list_empty( tattoo_list tl )
{
    int i;

    for ( i = 0; i < MAX_WEAR; i++ )
	if ( tl[i] != TATTOO_NONE )
	    return FALSE;

    return TRUE;
}

void clear_tattoos( tattoo_list tl )
{
    int i;

    for ( i = 0; i < MAX_WEAR; i++ )
	tl[i] = TATTOO_NONE;
}

char* print_tattoos( tattoo_list tl )
{
    static char buf[MSL];
    char nr_buf[10];
    int i;

    sprintf( buf, "%d", tl[0] );
    for ( i = 1; i < MAX_WEAR; i++ )
    {
	sprintf( nr_buf, " %d", tl[i] );
	strcat( buf, nr_buf );
    }
    
    return buf;
}

void bread_tattoos( RBUFFER *rbuf, tattoo_list tl )
{
    int i;

    for ( i = 0; i < MAX_WEAR; i++ )
	tl[i] = bread_number( rbuf );
}

/***************************** tattoo_data ***************************/

typedef struct tattoo_data TATTOO_DATA;
struct tattoo_data
{
    int vnum;
    int cost;
};

#define MAX_TATTOO 20
static const TATTOO_DATA tattoo_data_list[MAX_TATTOO] =
{
    { 10500, 500 }, // bear
    { 10501, 500 }, // snake
    { 10502, 500 }, // bunny
    { 10503, 500 }, // tiger
    { 10504, 500 }, // owl
    { 10505, 500 }, // lion
    { 10506, 500 }, // unicorn
    { 10507, 500 }, // eagle
    { 10508, 500 }, // dragon
    { 10509, 500 },  // tortoise

    { 10510, 2500 }, // grizzly
    { 10511, 2500 }, // python
    { 10512, 2500 }, // white rabbit
    { 10513, 2500 }, // sabretooth tiger
    { 10514, 2500 }, // fox
    { 10515, 2500 }, // sphinx
    { 10516, 2500 }, // elements
    { 10517, 2500 }, // phoenix
    { 10518, 2500 }, // greater wyrm
    { 10519, 2500 }  // guarding angel
};

static const bool tattoo_wear[MAX_WEAR] =
{
    FALSE, // LIGHT
    TRUE,  // WEAR_FINGER_L
    TRUE,  // WEAR_FINGER_R
    TRUE,  // WEAR_NECK_1
    TRUE,  // WEAR_NECK_2
    TRUE,  // WEAR_TORSO
    TRUE,  // WEAR_HEAD
    TRUE,  // WEAR_LEGS
    TRUE,  // WEAR_FEET
    TRUE,  // WEAR_HANDS
    TRUE,  // WEAR_ARMS
    FALSE, // WEAR_SHIELD
    FALSE, // WEAR_ABOUT
    TRUE,  // WEAR_WAIST
    TRUE,  // WEAR_WRIST_L
    TRUE,  // WEAR_WRIST_R
    FALSE, // WEAR_WIELD
    FALSE, // WEAR_HOLD
    FALSE, // WEAR_FLOAT
    FALSE  // WEAR_SECONDARY
};

bool is_tattoo_loc( int loc )
{
    if ( NO_LOC(loc) )
    {
	bug( "is_tattoo_loc: invalid location (%d)", loc );
	return FALSE;
    }

    return tattoo_wear[loc];
}

OBJ_INDEX_DATA* tattoo_obj( int ID )
{
    if ( ID == TATTOO_NONE )
	return NULL;

    if ( NO_ID(ID) )
    {
	bug( "tattoo_obj: invalid ID (%d)", ID );
	return NULL;
    }

    return get_obj_index( tattoo_data_list[ID].vnum );
}

const char* tattoo_desc( int ID )
{
    OBJ_INDEX_DATA *obj;

    if ( (obj = tattoo_obj(ID)) == NULL )
	return "";
    else
	return obj->short_descr;
}

const char* tattoo_name( int ID )
{
    OBJ_INDEX_DATA *obj;

    if ( (obj = tattoo_obj(ID)) == NULL )
	return "";
    else
	return obj->name;
}

int tattoo_cost( int ID )
{
    if ( NO_ID(ID) )
    {
	bug( "tattoo_cost: invalid ID (%d)", ID );
	return 0;
    }

    return tattoo_data_list[ID].cost;
}

int tattoo_id( const char *name )
{
    OBJ_INDEX_DATA *obj;
    int ID;

    for ( ID = 0; ID < MAX_TATTOO; ID++ )
    {
	obj = tattoo_obj( ID );
	if ( obj != NULL && is_exact_name(name, obj->name) )
	    return ID;
    }
    return TATTOO_NONE;
}

/***************************** general *******************************/ 

float tattoo_bonus_factor( float level )
{
    if ( level < 90 )
        return (level + 10) / 100;
    else
        return 1 + (level - 90) / 10;
}

AFFECT_DATA* tattoo_affect( AFFECT_DATA *aff, float level, bool basic )
{
    static AFFECT_DATA taff;
    float factor;

    /* (mis)use detect-level to mark basic bonus :) */
    if ( aff->detect_level == -1 )
    {
        if ( basic )
            return aff;
        else
            factor = 0;
    }
    else
    {
        if ( basic )
            factor = 0;
        else
            factor = tattoo_bonus_factor(level);
    }
    memcpy( &taff, aff, sizeof(AFFECT_DATA) );
    taff.next = NULL;
    taff.modifier = (int)(aff->modifier * factor);

    return &taff;
}

void tattoo_modify_ID( CHAR_DATA *ch, int ID, float level, bool fAdd, bool drop, bool basic )
{
    AFFECT_DATA *aff;
    OBJ_INDEX_DATA *obj;

    if ( NO_ID(ID) )
    {
	bug( "tattoo_modify_ID: invalid ID (%d)", ID );
	return;
    }
    
    if ( (obj = tattoo_obj(ID)) == NULL )
	return;

    for ( aff = obj->affected; aff != NULL; aff = aff->next )
	affect_modify_new( ch, tattoo_affect(aff, level, basic), fAdd, drop );
}

void tattoo_modify_equip( CHAR_DATA *ch, int loc, bool fAdd, bool drop, bool basic )
{
    int ID;

    if ( IS_NPC(ch) )
        return;

    if ( NO_LOC(loc) )
    {
        bug( "tattoo_modify_equip: invalid location (%d)", loc );
        return;
    }

    if ( (ID = tattoo_bonus_ID(ch, loc)) != TATTOO_NONE )
    {
        float tattoo_level = get_tattoo_level( ch, loc, ch->level );
        tattoo_modify_ID( ch, ID, tattoo_level, fAdd, drop, basic );
    }
}

/* returns ID of tattoo of ch at loc provided it adds an affect */
int tattoo_bonus_ID( CHAR_DATA *ch, int loc )
{
    OBJ_DATA *obj;

    if ( IS_NPC(ch) )
        return TATTOO_NONE;

    if ( (obj = get_eq_char(ch, loc)) != NULL && !CAN_WEAR(obj, ITEM_TRANSLUCENT) )
        return TATTOO_NONE;

    return TATTOO_ID(ch, loc);
}

float get_obj_tattoo_level( int obj_level, int level )
{
    // average of level and object level for translucent equipment
    return (level + obj_level) / 2.0;
}

float get_tattoo_level( CHAR_DATA *ch, int loc, int level )
{
    OBJ_DATA *obj = get_eq_char(ch, loc);
    
    // full level if no equipment worn over it
    if ( !obj )
        return level;
    
    return get_obj_tattoo_level(obj->level, level);
}

void tattoo_modify_level( CHAR_DATA *ch, int old_level, int new_level )
{
    int loc, ID;

    if ( IS_NPC(ch) )
        return;

    for ( loc = 0; loc < MAX_WEAR; loc++ )
    {
        ID = tattoo_bonus_ID( ch, loc );        
        if ( ID != TATTOO_NONE )
        {
            float old_tattoo_level = get_tattoo_level( ch, loc, old_level );
            float new_tattoo_level = get_tattoo_level( ch, loc, new_level );
            tattoo_modify_ID( ch, ID, old_tattoo_level, FALSE, FALSE, FALSE );
            tattoo_modify_ID( ch, ID, new_tattoo_level, TRUE, FALSE, FALSE );
        }
    }

    check_drop_weapon( ch );
}

void tattoo_modify_reset( CHAR_DATA *ch )
{
    int loc, ID;

    if ( IS_NPC(ch) )
        return;

    for ( loc = 0; loc < MAX_WEAR; loc++ )
    {
        // level-based bonus (only for translucent eq)
        if ( (ID = tattoo_bonus_ID(ch, loc)) != TATTOO_NONE )
        {
            float tattoo_level = get_tattoo_level( ch, loc, ch->level );
            tattoo_modify_ID( ch, ID, tattoo_level, TRUE, FALSE, FALSE );
        }
        // add additional bonus (regardless of eq and level)
        if ( (ID = TATTOO_ID(ch, loc)) != TATTOO_NONE )
            tattoo_modify_ID( ch, ID, ch->level, TRUE, FALSE, TRUE );
    }
}

int get_tattoo_ch( CHAR_DATA *ch, int loc )
{
    if ( IS_NPC(ch) )
	return TATTOO_NONE;
    else
	return get_tattoo( ch->pcdata->tattoos, loc );
}

/***************************** do_functions **************************/

void show_tattoos( CHAR_DATA *ch )
{
    OBJ_INDEX_DATA *obj;
    char buf[MSL];
    int ID;

    send_to_char( "The following tattoos are available:\n\r\n\r", ch );
    for ( ID = 0; ID < MAX_TATTOO; ID++ )
    {
	if ( (obj = tattoo_obj(ID)) == NULL )
	    continue;

	sprintf( buf, "%-10s: %d qp\n\r",
		 capitalize(obj->name), tattoo_cost(ID) );
	send_to_char( buf, ch );
    }
}

void show_tattoo_syntax( CHAR_DATA *ch )
{
    send_to_char( "Syntax: tattoo list\n\r", ch );
    send_to_char( "        tattoo loc\n\r", ch );
    send_to_char( "        tattoo buy <location> <name>\n\r", ch );
    send_to_char( "        tattoo remove <location>\n\r", ch );
}

void show_tattoo_loc( CHAR_DATA *ch )
{
    char buf[MSL];
    int loc;

    send_to_char( "Tattoos can be worn on the following locations:\n\r", ch );
    for ( loc = 0; loc < MAX_WEAR; loc++ )
	if ( is_tattoo_loc(loc) )
	{
	    sprintf( buf, "  %s\n\r", flag_bit_name(wear_loc_flags, loc) );
	    send_to_char( buf, ch );
	}
}

DEF_DO_FUN(do_tattoo)
{
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];
    int loc, ID, cost;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( !IS_SET(ch->in_room->room_flags, ROOM_TATTOO_SHOP) )
    {
	send_to_char( "You'll have to find a tattoo shop first.\n\r", ch );
	return;
    }

    if ( !strcmp(arg1, "list") )
    {
	show_tattoos( ch );
    }
    else if ( !strcmp(arg1, "loc") || !strcmp(arg1, "location"))
    {
	show_tattoo_loc( ch );
    }
    else if ( !strcmp(arg1, "buy") )
    {
	if ( (loc = flag_lookup(arg2, wear_loc_flags)) == NO_FLAG
	     || !is_tattoo_loc(loc) )
	{
	    send_to_char( "That's not a valid location.\n\r", ch );
	    return;
	}

	if ( get_eq_char(ch, loc) != NULL )
	{
	    send_to_char( "You must remove your armor first!\n\r", ch );
	    return;
	}

	if ( get_tattoo_ch(ch, loc) != TATTOO_NONE )
	{
	    send_to_char( "You must remove your old tattoo first!\n\r", ch );
	    return;
	}

	if ( (ID = tattoo_id(arg3)) == TATTOO_NONE )
	{
	    send_to_char( "That's not a valid tattoo.\n\r", ch );
	    return;
	}

	if ( (cost = tattoo_cost(ID)) > ch->pcdata->questpoints )
	{
	    send_to_char( "You don't have enough quest points.\n\r", ch );
	    return;
	}

	/* ok, let's add the tattoo */
	add_tattoo( ch->pcdata->tattoos, loc, ID );
	tattoo_modify_equip( ch, loc, TRUE, FALSE, TRUE );
	tattoo_modify_equip( ch, loc, TRUE, FALSE, FALSE );
	
	logpf( "%s bought tattoo '%s' at %s for %d qp",
	       ch->name, tattoo_name(ID), flag_bit_name(wear_loc_flags, loc),cost );
	ch->pcdata->questpoints -= cost;
	send_to_char( "Enjoy your new tattoo!\n\r", ch );
    }
    else if ( !strcmp(arg1, "remove") )
    {
	if ( (loc = flag_lookup(arg2, wear_loc_flags)) == NO_FLAG
	     || !is_tattoo_loc(loc) )
	{
	    send_to_char( "That's not a valid location.\n\r", ch );
	    return;
	}

	if ( get_eq_char(ch, loc) != NULL )
	{
	    send_to_char( "You must remove your armor first!\n\r", ch );
	    return;
	}

	if ( (ID = get_tattoo_ch(ch, loc)) == TATTOO_NONE )
	{
	    send_to_char( "You don't have a tattoo there!\n\r", ch );
	    return;
	}

	/* ok, let's remove the tattoo */
	tattoo_modify_equip( ch, loc, FALSE, TRUE, TRUE );
	tattoo_modify_equip( ch, loc, FALSE, TRUE, FALSE );
	remove_tattoo( ch->pcdata->tattoos, loc );
    if ( cfg_refund_tattoos )
        cost = tattoo_cost(ID);
    else
        cost = tattoo_cost(ID) * 9/10;

	logpf( "%s removed tattoo '%s' at %s for %d qp",
	       ch->name, tattoo_name(ID), flag_bit_name(wear_loc_flags, loc), cost );
	ch->pcdata->questpoints += cost;
	printf_to_char( ch, "Tattoo removed. Refunded %d quest points.\n\r", cost );
    }
    else
	show_tattoo_syntax( ch );
}
