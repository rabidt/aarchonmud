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
#include "lua_main.h"
#include "lua_scripting.h"
#include "interp.h"


/*
 * A general purpose percentage trigger. Checks if a random percentage
 * number is less than trigger phrase
 */
static bool rp_percent_trigger(
        ROOM_INDEX_DATA *room, CHAR_DATA *ch1, CHAR_DATA *ch2, 
        OBJ_DATA *obj1, OBJ_DATA *obj2, 
        const char *text1,
        int type)
{
    if ( !HAS_RTRIG(room, type) )
        return TRUE;

    PROG_LIST *prg;

    for ( prg = room->rprogs; prg != NULL; prg = prg->next )
    {
		if ( prg->trig_type == type
                && number_percent() <= atoi( prg->trig_phrase ) )
        {
            return lua_room_program( g_mud_LS, NULL, prg->vnum, prg->script->code, room, ch1, ch2, obj1, obj2, text1, type, prg->script->security)
                && (ch1 ? !ch1->must_extract : TRUE )
                && (ch2 ? !ch2->must_extract : TRUE )
                && (obj1 ? !obj1->must_extract : TRUE )
                && (obj2 ? !obj2->must_extract : TRUE );
        }
    }
    return TRUE;
}

static bool rp_act_trigger(
        ROOM_INDEX_DATA *room, 
        CHAR_DATA *ch1, CHAR_DATA *ch2, 
        OBJ_DATA *obj1, OBJ_DATA *obj2,
        const char *text1,
        const char *trigger, int type)
{
    if ( !HAS_RTRIG(room, type) )
        return TRUE;

    PROG_LIST *prg;

    for ( prg = room->rprogs; prg != NULL; prg = prg->next )
    {
        if (prg->trig_type == type
            && ( strstr(cap_all(trigger), cap_all(prg->trig_phrase)) != NULL
                ||  !strcmp(prg->trig_phrase, "*") ) )
        {
            return lua_room_program( g_mud_LS, NULL, prg->vnum, prg->script->code, room, ch1, ch2, obj1, obj2, text1, type, prg->script->security)
                && (ch1 ? !ch1->must_extract : TRUE )
                && (ch2 ? !ch2->must_extract : TRUE )
                && (obj1 ? !obj1->must_extract : TRUE )
                && (obj2 ? !obj2->must_extract : TRUE );
        }
    }
    return TRUE;
}

/* similar to act trigger but it needs its own logic in the end */
bool rp_command_trigger( CHAR_DATA *ch, int cmd, const char *argument )
{
    if ( !ch->in_room )
    {
        bugf( "rp_command_trigger: no room for %s",
                ch->name);
        return FALSE;
    }

    if ( !HAS_RTRIG(ch->in_room, RTRIG_COMMAND) )
        return TRUE;

    PROG_LIST *prg;

    for ( prg = ch->in_room->rprogs; prg ; prg = prg->next )
    {
        if ( prg->trig_type == RTRIG_COMMAND
                && !str_cmp( cmd_table[cmd].name, prg->trig_phrase ) )
        {
            return lua_room_program( g_mud_LS, cmd_table[cmd].name, 
                    prg->vnum, prg->script->code,
                    ch->in_room, ch, NULL, NULL, NULL, argument, 
                    RTRIG_COMMAND, 
                    prg->script->security)
                && (ch ? !ch->must_extract : TRUE );
        }
    }
    return TRUE;
}

bool rp_prereset_trigger( ROOM_INDEX_DATA *room )
{
    return rp_percent_trigger( room, NULL, NULL, NULL, NULL, NULL, RTRIG_PRERESET);
}

void rp_postreset_trigger( ROOM_INDEX_DATA *room )
{
    rp_percent_trigger( room, NULL, NULL, NULL, NULL, NULL, RTRIG_POSTRESET);
    return;
}

void rp_connect_trigger( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *room;

    room=ch->in_room;

    if (!room)
    {
        bugf("rp_connect_trigger: in_room NULL for %s", ch->name);
        return;
    }
    if ( !HAS_RTRIG(room, RTRIG_CONNECT) )
        return;

    rp_percent_trigger( room, ch, NULL, NULL, NULL, NULL, RTRIG_CONNECT);
} 

/* returns whether a trigger was found */
bool rp_try_trigger( const char *argument, CHAR_DATA *ch )
{
    if ( !ch->in_room )
    {
        bugf( "rp_try_trigger: no room for %s",
                ch->name);
        return FALSE;
    }

    if ( !HAS_RTRIG( ch->in_room, RTRIG_TRY ) )
        return FALSE; 

    rp_act_trigger( ch->in_room,
            ch, NULL,
            NULL, NULL,
            NULL,
            argument, RTRIG_TRY );

    return TRUE;

}

bool rp_enter_trigger( CHAR_DATA *ch )
{
    return rp_percent_trigger(
            ch->in_room, ch, NULL,
            NULL, NULL, NULL, RTRIG_ENTER);
}

bool rp_exit_trigger( CHAR_DATA *ch )
{
    return rp_percent_trigger(
            ch->in_room, ch, NULL,
            NULL, NULL, NULL, RTRIG_EXIT);
}

bool rp_look_ed_trigger( CHAR_DATA *ch, const char *ed )
{
    return rp_act_trigger( ch->in_room,
            ch, NULL,
            NULL, NULL,
            NULL,
            ed, RTRIG_LOOK );
}

bool rp_look_trigger( CHAR_DATA *ch )
{
    return rp_percent_trigger(
            ch->in_room, ch, NULL,
            NULL, NULL, NULL, RTRIG_LOOK);
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
        if ( (prg->trig_type == type && !strcmp(prg->trig_phrase, dirname))
                || !strcmp(prg->trig_phrase, "*") )
        {
            return lua_room_program( g_mud_LS, dirname, prg->vnum, prg->script->code, room, ch, NULL, NULL, NULL, NULL, type, prg->script->security)
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
            lua_room_program( g_mud_LS, NULL, prg->vnum, prg->script->code, 
                   room,
                   NULL, NULL, NULL, NULL, NULL,
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
