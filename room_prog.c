#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <ctype.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"
#include "lua_scripting.h"


/*
 * A general purpose percentage trigger. Checks if a random percentage
 * number is less than trigger phrase
 */
static bool rp_percent_trigger(
        ROOM_INDEX_DATA *room, CHAR_DATA *ch1, CHAR_DATA *ch2, 
        OBJ_DATA *obj1, OBJ_DATA *obj2, int type)
{
    if ( !HAS_RTRIG(room, type) )
        return TRUE;

    PROG_LIST *prg;

    for ( prg = room->rprogs; prg != NULL; prg = prg->next )
    {
		if ( prg->trig_type == type
                && number_percent() <= atoi( prg->trig_phrase ) )
        {
            return lua_room_program( NULL, prg->vnum, prg->script->code, room, ch1, ch2, obj1, obj2, type, prg->script->security)
                && (ch1 ? !ch1->must_extract : TRUE )
                && (ch2 ? !ch2->must_extract : TRUE )
                && (obj1 ? !obj1->must_extract : TRUE )
                && (obj2 ? !obj2->must_extract : TRUE );
        }
    }
    return TRUE;
}

bool rp_enter_trigger( CHAR_DATA *ch )
{
    return rp_percent_trigger(
            ch->in_room, ch, NULL,
            NULL, NULL, RTRIG_ENTER);
}

bool rp_exit_trigger( CHAR_DATA *ch )
{
    return rp_percent_trigger(
            ch->in_room, ch, NULL,
            NULL, NULL, RTRIG_EXIT);
}

bool rp_look_trigger( CHAR_DATA *ch )
{
    return rp_percent_trigger(
            ch->in_room, ch, NULL,
            NULL, NULL, RTRIG_LOOK);
}

/* base function for exit specific triggers */
static bool exit_trigger( CHAR_DATA *ch, ROOM_INDEX_DATA *room, int door, int type )
{
    if (!room)
    {
        bugf("NULL room in exit_trigger");
        return TRUE;
    }

    if ( !HAS_RTRIG(room, type) )
        return TRUE;

    PROG_LIST *prg;

    const char *dirname=dir_name[door];
    for ( prg = room->rprogs ; prg ; prg = prg->next )
    {
        if ( prg->trig_type == type 
                && ( !strcmp(prg->trig_phrase, dirname ) )
                    || !strcmp(prg->trig_phrase, "*" ) )
        {
            return lua_room_program( dirname, prg->vnum, prg->script->code, room, ch, NULL, NULL, NULL, type, prg->script->security)
                && (ch ? !ch->must_extract : TRUE );
        }
    }

    return TRUE;
}

bool rp_open_trigger( CHAR_DATA *ch, int door )
{
    return exit_trigger( ch, ch->in_room, door, RTRIG_OPEN );
}

bool rp_close_trigger( CHAR_DATA *ch, int door )
{
    return exit_trigger( ch, ch->in_room, door, RTRIG_CLOSE );
}

bool rp_lock_trigger( CHAR_DATA *ch, int door )
{
    return exit_trigger( ch, ch->in_room, door, RTRIG_LOCK );
}

bool rp_unlock_trigger( CHAR_DATA *ch, int door )
{
    return exit_trigger( ch, ch->in_room, door, RTRIG_UNLOCK );
}

bool rp_move_trigger( CHAR_DATA *ch, int door )
{
    return exit_trigger( ch, ch->in_room, door, RTRIG_MOVE );
}


void rp_timer_trigger( ROOM_INDEX_DATA *room )
{
    if (!room)
    {
        bugf("NULL room in rp_timer_trigger");
        return;
    }

    PROG_LIST *prg;

    for ( prg=room->rprogs; prg != NULL; prg = prg->next )
    {
        if (prg->trig_type == RTRIG_TIMER)
        {
            lua_room_program( NULL, prg->vnum, prg->script->code, 
                   room,
                   NULL, NULL, NULL, NULL,
                   RTRIG_TIMER, prg->script->security);
            return;
        }
    }
}

void rprog_timer_init( ROOM_INDEX_DATA *room)
{
    if (!room)
    {
        bugf("NULL room in rprog_timer_init");
        return;
    }

    /* Set up timer stuff if not already */
    if (HAS_RTRIG(room, RTRIG_TIMER) && !room->rtrig_timer)
    {
        PROG_LIST *prg;
        for ( prg = room->rprogs; prg; prg= prg->next )
        {
            if ( prg->trig_type == RTRIG_TIMER )
            {
                if (!is_number(prg->trig_phrase))
                {
                    bugf("Bad timer phrase for room %d: %s, must be number.",
                            room->vnum, prg->trig_phrase);
                    return;
                }
                register_room_timer( room, atoi(prg->trig_phrase));
                return; /* only one allowed */
            }
        }
    }
}

void rprog_setup( ROOM_INDEX_DATA *room )
{
    /* initialize timer, may add more setup steps later */
    rprog_timer_init( room );
}
