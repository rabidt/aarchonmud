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
#include "warfare.h"
#include "lua_scripting.h"
#include "recycle.h"

ROOM_INDEX_DATA  *get_random_room_range(CHAR_DATA *ch, int min_vnum, int max_vnum);

/* command procedures needed */
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_stand     );

/* random room generation procedure */
ROOM_INDEX_DATA  *get_random_room(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *room;
    
    for ( ; ; )
    {
	room = get_random_room_range( ch, 0, 65535 );
	if ( room != NULL && is_room_ingame( room) )
	    break;
    }
    
    return room;
}


/* warroom random room generation procedure */
ROOM_INDEX_DATA  *get_random_war_room(CHAR_DATA *ch)
{
    return get_random_room_range( ch, WAR_ROOM_FIRST, WAR_ROOM_LAST );
}

ROOM_INDEX_DATA  *get_random_room_area(CHAR_DATA *ch)
{
    if ( ch->in_room == NULL )
	return NULL;
    else
	return get_random_room_range( ch, ch->in_room->area->min_vnum, 
				      ch->in_room->area->max_vnum );
}

ROOM_INDEX_DATA  *get_random_room_range(CHAR_DATA *ch, int min_vnum, int max_vnum)
{
    ROOM_INDEX_DATA *room;
    int i;

    for ( i = 0; i < 10000; i++ )
    {
        room = get_room_index( number_range( min_vnum, max_vnum ) );
        if ( room != NULL )
            if ( can_see_room(ch,room)
                //&&   !room_is_private(room)
                &&   can_move_room(ch, room, FALSE)
                &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
                &&   !IS_SET(room->room_flags, ROOM_SOLITARY) 
                &&   !IS_SET(room->room_flags, ROOM_SAFE) 
                &&   !IS_SET(room->room_flags, ROOM_JAIL) 
                &&   !IS_SET(room->room_flags, ROOM_NO_TELEPORT)
                &&   !(IS_NPC(ch) 
		       && IS_SET(ch->act,ACT_AGGRESSIVE) 
		       && IS_SET(room->room_flags,ROOM_LAW)))
                break;
    }

    if ( i == 10000 )
    {
	bugf( "get_random_room_range: no room found (%d-%d)", min_vnum, max_vnum );
	return NULL;
    }
    
    return room;
}

/* RT Enter portals */
DEF_DO_FUN(do_enter)
{    
	ROOM_INDEX_DATA *location; 
	bool stay_area = FALSE;
    AREA_DATA *from_area;

	if ( ch->fighting != NULL ) 
	return;

	/* nifty portal stuff */
	if (argument[0] != '\0')
	{
		ROOM_INDEX_DATA *old_room;
	OBJ_DATA *portal;
	CHAR_DATA *fch, *fch_next;

    old_room = ch->in_room;
    from_area= old_room ? old_room->area : NULL;

	portal = get_obj_list( ch, argument,  ch->in_room->contents );
	
	if (portal == NULL)
	{
		send_to_char("You don't see that here.\n\r",ch);
		return;
	}

	if (portal->item_type != ITEM_PORTAL 
	    ||  (I_IS_SET(portal->value[1], EX_CLOSED) && !IS_TRUSTED(ch,LEVEL_IMMORTAL)))
	{
		send_to_char("You can't seem to find a way in.\n\r",ch);
		return;
	}

	if (!IS_TRUSTED(ch,LEVEL_IMMORTAL) && !I_IS_SET(portal->value[2],GATE_NOCURSE)
	&&  (IS_AFFECTED(ch,AFF_CURSE)
	||  (IS_SET(old_room->room_flags,ROOM_NO_RECALL) && !I_IS_SET(portal->value[2],GATE_IGNORE_NO_RECALL))))
	{
		send_to_char("Something prevents you from leaving...\n\r",ch);
		return;
	}

	if ( I_IS_SET(portal->value[2],GATE_ASTRAL) )
	{
	   if( !IS_AFFECTED( ch, AFF_DETECT_ASTRAL ) && !IS_AFFECTED( ch, AFF_ASTRAL )  )
	   {
	      send_to_char( "You don't see that here.\n\r", ch );
	      return;
	   }
	   if( !IS_AFFECTED( ch, AFF_ASTRAL ) )
	   {
	      send_to_char( "You see the way, but you cannot bring your physical form through it...\n\r", ch );
	      return;
	   }
	}

	if ( carries_relic(ch) )
	{
	    send_to_char( "Not with a relic!\n\r", ch );
	    return;
	}

	stay_area = I_IS_SET(portal->value[2],GATE_STAY_AREA) != 0;

	if (I_IS_SET(portal->value[2],GATE_RANDOM) || portal->value[3] == -1)
	{
	    if ( stay_area )
		location = get_random_room_area(ch);
	    else
		location = get_random_room(ch);
	    portal->value[3] = location->vnum; /* for record keeping :) */
	}
	else if (I_IS_SET(portal->value[2],GATE_WARFARE) || portal->value[3] == -1)
	{
		location = get_random_war_room(ch);
		portal->value[3] = location->vnum; /* for record keeping :) */
	}
	else if ( I_IS_SET(portal->value[2],GATE_BUGGY) && (per_chance(5) || (IS_AFFECTED(ch, AFF_CURSE) && per_chance(20))) )
	{
	    if ( stay_area )
		location = get_random_room_area(ch);
	    else
		location = get_random_room(ch);
	}
	else
		location = get_room_index(portal->value[3]);

	if (location == NULL
	||  location == old_room
	||  !can_see_room(ch,location)
    ||  !can_move_room(ch,location,false))
	//||  (room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR)))
	{
	   act("$p doesn't seem to go anywhere.",ch,portal,NULL,TO_CHAR);
	   return;
	}

	if (IS_NPC(ch) && IS_SET(ch->act,ACT_AGGRESSIVE)
	    &&  IS_SET(location->room_flags,ROOM_LAW))
	{
	    send_to_char("Something prevents you from leaving...\n\r",ch);
	    return;
	}

	if ( stay_area && location->area != ch->in_room->area )
	{
	   act("$p is too far from it's destination.",ch,portal,NULL,TO_CHAR);
	   return;
	}

	if ( check_item_trap_hit(ch, portal) )
	    return;

    /* check for exit triggers */
    if ( !IS_NPC(ch) )
    {
        if ( !op_percent_trigger( NULL, portal, NULL, ch, NULL, OTRIG_ENTER ) )
            return;
        if ( !rp_exit_trigger(ch) )
            return;
        if ( !ap_rexit_trigger(ch) )
            return;
        if ( !ap_exit_trigger(ch, location->area) )
            return;
        if ( !op_move_trigger(ch) )
            return;
    }  

	act("$n steps into $p.",ch,portal,NULL,TO_ROOM);
	
	if (I_IS_SET(portal->value[2],GATE_NORMAL_EXIT))
		act("You enter $p.",ch,portal,NULL,TO_CHAR);
	else
		act("You walk through $p and find yourself somewhere else...",
			ch,portal,NULL,TO_CHAR); 

	char_from_room(ch);
	char_to_room(ch, location);

	if (I_IS_SET(portal->value[2],GATE_GOWITH)) /* take the gate along */
	{
		obj_from_room(portal);
		obj_to_room(portal,location);
	}

	if (I_IS_SET(portal->value[2],GATE_NORMAL_EXIT))
		act("$n has arrived.",ch,portal,NULL,TO_ROOM);
	else
		act("$n has arrived through $p.",ch,portal,NULL,TO_ROOM);

	do_look(ch,"auto");

	/* charges */
	if (portal->value[0] > 0)
	{
		portal->value[0]--;
		if (portal->value[0] == 0)
		portal->value[0] = -1;
	}

	/* protect against circular follows */
	if (old_room == location)
		return;

		for ( fch = old_room->people; fch != NULL; fch = fch_next )
		{
			fch_next = fch->next_in_room;

			if (portal == NULL || portal->value[0] == -1) 
		/* no following through dead portals */
				continue;
 
			if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM)
			&&   fch->position < POS_STANDING)
				do_stand(fch,"");

			if ( fch->master == ch && fch->position == POS_STANDING)
			{
 
				if (IS_SET(ch->in_room->room_flags,ROOM_LAW)
				&&  (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE)))
				{
					act("You can't bring $N into the city.",
						ch,NULL,fch,TO_CHAR);
					act("You aren't allowed in the city.",
						fch,NULL,NULL,TO_CHAR);
					continue;
				}
 
				act( "You follow $N.", fch, NULL, ch, TO_CHAR );
				do_enter(fch,argument);
			}
		}

	if (portal != NULL && portal->value[0] == -1)
	{
		act("$p fades out of existence.",ch,portal,NULL,TO_CHAR);
		if (ch->in_room == old_room)
		act("$p fades out of existence.",ch,portal,NULL,TO_ROOM);
		else if (old_room->people != NULL)
		{
		act("$p fades out of existence.", 
			old_room->people,portal,NULL,TO_CHAR);
		act("$p fades out of existence.",
			old_room->people,portal,NULL,TO_ROOM);
		}
		extract_obj(portal);
	}
 
	/* 
	 * If someone is following the char, these triggers get activated
	 * for the followers before the char, but it's safer this way...
	 */
	if ( !IS_NPC( ch ) )
    {
        ap_enter_trigger( ch, from_area );
        ap_renter_trigger( ch );
        rp_enter_trigger( ch );
        op_greet_trigger( ch );
		mp_greet_trigger( ch );
    }
	if ( IS_NPC( ch ) && HAS_TRIGGER( ch, TRIG_ENTRY ) )
		mp_percent_trigger( ch, NULL, NULL, 0, NULL,0, TRIG_ENTRY );
 
	return;
	}

	send_to_char("Nope, can't do it.\n\r",ch);
	return;
}

/* stuff for handling portal spell target locations */

PORTAL_DATA *portal_list = NULL;

void save_portal_list()
{
    PORTAL_DATA *portal;
    FILE *fp;

    if ( (fp = fopen(PORTAL_FILE, "w")) == NULL )
    {
	bugf( "save_portal_list: fopen failed" );
	log_error( PORTAL_FILE );
    }
    else
    {
	/* save the portal locations */
	for ( portal = portal_list; portal != NULL; portal = portal->next )
	    rfprintf( fp, "#%d %s~\n\r", portal->vnum, portal->name );
	fprintf( fp, "END\n\r" );
	fclose( fp );
    }
}

void load_portal_list()
{
    PORTAL_DATA *portal, *portal_last;
    FILE *fp;

    if ( portal_list != NULL )
    {
	bugf( "load_portal_list: portal_list not empty" );
	return;
    }

    if ( (fp = fopen(PORTAL_FILE, "r")) == NULL )
    {
	bugf( "load_portal_list: fopen failed" );
	log_error( PORTAL_FILE );
    }
    else
    {
	/* load the portal locations */
	portal_last = NULL;
	while ( TRUE )
	{
	    if ( fread_letter( fp ) != '#' )
		break;
	    
	    portal = new_portal();
	    portal->vnum = fread_number( fp );
	    portal->name = str_dup( fread_string(fp) );

	    if ( portal_last == NULL )
		portal_list = portal;
	    else
		portal_last->next = portal;
	    portal_last = portal;
	}
	
	fclose( fp );
    }
}

ROOM_INDEX_DATA* get_portal_room( const char *name )
{
    PORTAL_DATA *portal;

    for ( portal = portal_list; portal != NULL; portal = portal->next )
	if ( !strcmp(name, portal->name) )
	    return get_room_index( portal->vnum );

    return NULL;
}

DEF_DO_FUN(do_portal)
{
    char buf[MSL],
	arg1[MIL], arg2[MIL], arg3[MIL];
    PORTAL_DATA *portal, *portal_prev;
    BUFFER *output;
    ROOM_INDEX_DATA *room;
    int vnum;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    
    if ( !strcmp(arg1, "list") )
    {
	output = new_buf();
	add_buf( output, "The following portal locations exist:\n\r" );
	add_buf( output, "[ vnum area                ] name\n\r" );
	for ( portal = portal_list; portal != NULL; portal = portal->next )
	{
	    room = get_room_index( portal->vnum );
	    sprintf( buf, "[%5d %-20s] %s\n\r", portal->vnum,
		     room == NULL ? "!!! no room !!!" : room->area->name,
		     portal->name );
	    add_buf( output, buf );
	}
	page_to_char( buf_string(output), ch );
	free_buf(output);
	return;
    }
    else if ( !strcmp(arg1, "add") )
    {
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "You must specify a name for the portal location.\n\r", ch );
	    return;
	}
	
	if ( arg3[0] == '\0' )
	{
	    send_to_char( "You must specify the room vnum.\n\r", ch );
	    return;
	}

	if ( !is_number(arg3) )
	{
	    send_to_char( "That's not a number.\n\r", ch );
	    return;
	}
	
	vnum = atoi( arg3 );
	if ( get_room_index(vnum) == NULL )
	{
	    send_to_char( "That room doesn't exist.\n\r", ch );
	    return;
	}

	if ( get_portal_room(arg2) != NULL )
	{
	    send_to_char( "That portal location already exists.\n\r", ch );
	    return;
	}

	/* all fine, now create the portal lcoation */
	portal = new_portal();
	portal->vnum = vnum;
	portal->name = str_dup(arg2);

	/* insert sorted by vnum */
	if ( portal_list == NULL || portal_list->vnum > vnum )
	{
	    portal->next = portal_list;
	    portal_list = portal;
	}
	else
	{
	    for ( portal_prev = portal_list; portal_prev->next != NULL;
		  portal_prev = portal_prev->next )
		if ( portal_prev->next->vnum > vnum )
		    break;

	    portal->next = portal_prev->next;
	    portal_prev->next = portal;
	}

	send_to_char( "Portal location added.\n\r", ch );
	save_portal_list();
	return;
    }
    else if ( !strcmp(arg1, "remove") )
    {
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Which portal location do you want to remove?\n\r", ch );
	    return;
	}

	/* find portal */
	portal_prev = NULL;
	for ( portal = portal_list; portal != NULL; portal = portal->next )
	{
	    if ( !strcmp(arg2, portal->name) )
		break;
	    portal_prev = portal;
	}
	
	if ( portal == NULL )
	{
	    send_to_char( "There is no portal location with this name.\n\r", ch );
	    return;
	}

	if ( portal == portal_list )
	    portal_list = portal->next;
	else
	    portal_prev->next = portal->next;
	free_portal( portal );

	send_to_char( "Portal removed.\n\r", ch );
	save_portal_list();
	return;
    }
    else
    {
	send_to_char( "Syntax: portal list\n\r", ch );
	send_to_char( "        portal add <name> <room vnum>\n\r", ch );
	send_to_char( "        portal remove <name>\n\r", ch );
	return;
    }
}

/* called by portal spell */
void show_portal_names( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *room;
    PORTAL_DATA *portal;
    BUFFER *output;
    char buf[MSL];

    output = new_buf();
    add_buf( output, "The following portal locations exist:\n\r" );
    add_buf( output, "{w Area Name                    Portal Name{x\n\r" );
    add_buf( output, "{w-----------------------------------------{x\n\r" );
    for ( portal = portal_list; portal != NULL; portal = portal->next )
    {
        room = get_room_index( portal->vnum );
        sprintf( buf, "[%-27s] %s\n\r", 
            remove_color(room == NULL ? "!!! no room !!!" : room->area->name),
	    portal->name );
	    add_buf( output, buf );
    }

    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}
