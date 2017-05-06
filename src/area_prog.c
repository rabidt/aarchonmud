#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <ctype.h>
#include <lua.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"
#include "lua_scripting.h"


/*
 * A general purpose percentage trigger. Checks if a random percentage
 * number is less than trigger phrase
 */
static bool ap_percent_trigger(AREA_DATA *area, CHAR_DATA *ch1, int type)
{
    if ( !HAS_ATRIG(area, type) )
        return TRUE;

    PROG_LIST *prg;

    for ( prg = area->aprogs; prg != NULL; prg = prg->next )
    {
		if ( prg->trig_type == type
                && number_percent() <= atoi( prg->trig_phrase ) )
        {
            return lua_area_program( NULL, prg->vnum, prg->script->code, area, ch1, type, prg->script->security)
                && ( ch1 ? !ch1->must_extract : TRUE);
        }
    }
    return TRUE;
}

bool ap_death_trigger(CHAR_DATA *ch)
{
    if ( !ch->in_room)
    {
        bugf("ap_death_trigger: in_room NULL for %s", ch->name);
        return TRUE;
    }
    if ( !HAS_ATRIG(ch->in_room->area, ATRIG_DEATH) )
        return TRUE;

    return ap_percent_trigger( ch->in_room->area, ch, ATRIG_DEATH);
}

bool ap_rexit_trigger(CHAR_DATA *ch)
{
	if ( !ch->in_room)
	{
		bugf("ap_rexit_trigger: in_room NULL for %s", ch->name);
		return TRUE;
	}
	if ( !HAS_ATRIG(ch->in_room->area, ATRIG_REXIT) )
		return TRUE;
	
	return ap_percent_trigger( ch->in_room->area, ch, ATRIG_REXIT);

}

bool ap_exit_trigger(CHAR_DATA *ch, AREA_DATA *to_area)
{
	
	if ( !ch->in_room)
	{
		bugf("ap_exit_trigger: in_room NULL for %s", ch->name);
		return TRUE;
	}
	if ( !HAS_ATRIG(ch->in_room->area, ATRIG_EXIT) )
		return TRUE;
	
	if ( to_area == ch->in_room->area )
		return TRUE;
	
	return ap_percent_trigger( ch->in_room->area, ch, ATRIG_EXIT);
}

bool ap_renter_trigger(CHAR_DATA *ch)
{
	
	if ( !ch->in_room)
	{
		bugf("ap_renter_trigger: in_room NULL for %s", ch->name);
		return TRUE;
	}
	if ( !HAS_ATRIG(ch->in_room->area, ATRIG_RENTER) )
		return TRUE;
	
	return ap_percent_trigger( ch->in_room->area, ch, ATRIG_RENTER);
}

bool ap_enter_trigger(CHAR_DATA *ch, AREA_DATA *from_area)
{
	
	if ( !ch->in_room)
	{
		bugf("ap_enter_trigger: in_room NULL for %s", ch->name);
		return TRUE;
	}
	if ( !HAS_ATRIG(ch->in_room->area, ATRIG_ENTER) )
		return TRUE;
	
	if ( from_area == ch->in_room->area )
		return TRUE;
	
	return ap_percent_trigger( ch->in_room->area, ch, ATRIG_ENTER);
}

bool ap_prereset_trigger( AREA_DATA *area)
{
    return ap_percent_trigger( area, NULL, ATRIG_PRERESET);
}

void ap_postreset_trigger( AREA_DATA *area)
{
    ap_percent_trigger( area, NULL, ATRIG_POSTRESET);

    return;
}

void ap_boot_trigger( void )
{

	AREA_DATA *area;
	
	for ( area=area_first ; area; area=area->next)
	{
		if ( !HAS_ATRIG( area, ATRIG_BOOT) )
			continue;
		ap_percent_trigger( area, NULL, ATRIG_BOOT);
	}
	
	return;
}

void ap_shutdown_trigger( void )
{
	AREA_DATA *area;
	
	for ( area=area_first ; area ; area=area->next)
	{
		if ( !HAS_ATRIG( area, ATRIG_SHUTDOWN) )
			continue;
		ap_percent_trigger( area, NULL, ATRIG_SHUTDOWN);
	}
	
	return;
}

void ap_quit_trigger(CHAR_DATA *ch)
{
	ROOM_INDEX_DATA *room;
	
	room=ch->in_room;
		
	if ( !room)
	{
		bugf("ap_quit_trigger: in_room NULL for %s", ch->name);
		return;
	}
	if ( !HAS_ATRIG(room->area, ATRIG_QUIT) )
		return;
	
	ap_percent_trigger( room->area, ch, ATRIG_QUIT);
}

void ap_connect_trigger(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *room;

    room=ch->in_room;

    if (!room)
    {
        bugf("ap_connect_trigger: in_room NULL for %s", ch->name);
        return;
    }
    if ( !HAS_ATRIG(room->area, ATRIG_CONNECT) )
        return;

    ap_percent_trigger( room->area, ch, ATRIG_CONNECT);
}

void ap_void_trigger(CHAR_DATA *ch)
{

	if ( !ch->in_room)
	{
		bugf("ap_void_trigger: in_room NULL for %s", ch->name);
		return;
	}
	if ( !HAS_ATRIG(ch->in_room->area, ATRIG_VOID) )
		return;
	
	ap_percent_trigger( ch->in_room->area, ch, ATRIG_VOID);
}

bool ap_unvoid_trigger( CHAR_DATA *ch)
{

	if ( !ch->in_room)
	{
		bugf("ap_unvoid_trigger: in_room NULL for %s", ch->name);
		return TRUE;
	}
	if ( !HAS_ATRIG(ch->in_room->area, ATRIG_UNVOID) )
		return TRUE;
		
	return ap_percent_trigger( ch->in_room->area, ch, ATRIG_UNVOID);
}

bool ap_recall_trigger( CHAR_DATA *ch)
{

    if ( !ch->in_room)
    {
        bugf("ap_recall_trigger: in_room NULL for %s", ch->name);
        return TRUE;
    }
    if ( !HAS_ATRIG(ch->in_room->area, ATRIG_RECALL) )
        return TRUE;

    return ap_percent_trigger( ch->in_room->area, ch, ATRIG_RECALL);
}

void ap_timer_trigger( AREA_DATA *area )
{
    PROG_LIST *prg;

    for ( prg=area->aprogs; prg != NULL; prg = prg->next )
    {
        if (prg->trig_type == ATRIG_TIMER)
        {
            lua_area_program( NULL, prg->vnum, prg->script->code, 
                    area, NULL, ATRIG_TIMER, prg->script->security);
            return;
        }
    }
}

void aprog_timer_init( AREA_DATA *area)
{
    /* Set up timer stuff if not already */
    if (HAS_ATRIG(area, ATRIG_TIMER) && !area->atrig_timer)
    {
        PROG_LIST *prg;
        for ( prg = area->aprogs; prg; prg= prg->next )
        {
            if ( prg->trig_type == ATRIG_TIMER )
            {
                if (!is_number(prg->trig_phrase))
                {
                    bugf("Bad timer phrase for area %s: %s, must be number.",
                            area->name, prg->trig_phrase);
                    return;
                }
                register_area_timer( area, atoi(prg->trig_phrase));
                return; /* only one allowed */
            }
        }
    }
}

void aprog_setup( AREA_DATA *area )
{
    /* initialize timer, may add more setup steps later */
    aprog_timer_init( area );
}
