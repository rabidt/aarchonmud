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
bool op_percent_trigger(
        OBJ_DATA *obj, OBJ_DATA *obj2, CHAR_DATA *ch1, CHAR_DATA *ch2, int type)
{
    if ( !HAS_OTRIG(obj, type) )
        return TRUE;

    OPROG_LIST *prg;

    for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
    {
        if ( prg->trig_type == type
                && number_percent() <= atoi( prg->trig_phrase ) )
        {
            return ( lua_obj_program( NULL, prg->vnum, prg->script->code, obj, obj2, ch1, ch2, type, prg->script->security)
                     && ( obj ? !obj->must_extract : TRUE )
                     && ( obj2 ? !obj2->must_extract : TRUE )
                     && ( ch1 ? !ch1->must_extract : TRUE )
                     && ( ch2 ? !ch2->must_extract : TRUE ) ); 
        }
    }
    return ( TRUE
             && ( obj ? !obj->must_extract : TRUE )
             && ( obj2 ? !obj2->must_extract : TRUE )
             && ( ch1 ? !ch1->must_extract : TRUE )
             && ( ch2 ? !ch2->must_extract : TRUE ) );
}

bool op_act_trigger(
        OBJ_DATA *obj, CHAR_DATA *ch1, CHAR_DATA *ch2, char *trigger, int type)
{
    OPROG_LIST *prg;

    for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
    {
        if ( prg->trig_type == type
                /* should be case-insensitive --Bobble
                   && strstr( argument, prg->trig_phrase ) != NULL )
                 */
            && ( strstr(cap_all(trigger), cap_all(prg->trig_phrase)) != NULL
                    ||   !strcmp(prg->trig_phrase, "*") ) )
                    {
                        return (lua_obj_program( trigger, prg->vnum, prg->script->code, obj, NULL, ch1, NULL, type, prg->script->security)
                               && ( obj ? !obj->must_extract : TRUE )
                               && ( ch1 ? !ch1->must_extract : TRUE )
                               && ( ch2 ? !ch2->must_extract : TRUE ) );
                    }
    }
    return (TRUE
            && ( obj ? !obj->must_extract : TRUE )
            && ( ch1 ? !ch1->must_extract : TRUE )
            && ( ch2 ? !ch2->must_extract : TRUE ) );
}

bool op_try_trigger( char* argument, CHAR_DATA *ch )
{
    OBJ_DATA *obj;
    OBJ_DATA *next_obj;
    bool found = FALSE;

    if ( !ch->in_room )
    {
        bugf("op_try_trigger: ch->in_room NULL for %s", ch->name);
        return;
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_TRY) )
        {
            op_act_trigger(obj, ch, NULL, argument, OTRIG_TRY); 
            found = TRUE;
        }
    }

    for ( obj = ch->carrying; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_TRY) )
        {
            op_act_trigger(obj, ch, NULL, argument, OTRIG_TRY); 
            found = TRUE;
        }
    }

    return found;
}

void op_speech_trigger( char *argument, CHAR_DATA *ch )
{
    OBJ_DATA *obj;
    OBJ_DATA *next_obj;
    
    if ( !ch->in_room )
    {
        bugf("op_speech_trigger: ch->in_room NULL for %s", ch->name);
        return;
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_SPEECH) )
        {
            op_act_trigger(obj, ch, NULL, argument, OTRIG_SPEECH);
        }
    }

    for ( obj = ch->carrying; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_SPEECH) )
        {
            op_act_trigger(obj, ch, NULL, argument, OTRIG_SPEECH);
        }
    }
}

void op_greet_trigger( CHAR_DATA *ch )
{
    OBJ_DATA *obj;
    OBJ_DATA *next_obj;

    if ( !ch->in_room )
    {
        bugf("op_greet_trigger: ch->in_room NULL for %s", ch->name);
        return;
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_GREET) )
        {
            op_percent_trigger(obj, NULL, ch, NULL, OTRIG_GREET);
        }
    }
}

void op_timer_trigger( OBJ_DATA *obj )
{
    OPROG_LIST *prg;

    for ( prg=obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
    {
        if (prg->trig_type == OTRIG_TIMER)
        {
            lua_obj_program( NULL, prg->vnum, prg->script->code, 
                    obj, NULL, NULL, NULL, 
                    OTRIG_TIMER, prg->script->security);
            return;
        }
    }
}

void oprog_timer_init( OBJ_DATA *obj)
{
    /* Set up timer stuff if not already */
    if (HAS_OTRIG(obj, OTRIG_TIMER) && !obj->otrig_timer)
    {
        OPROG_LIST *prg;
        for ( prg = obj->pIndexData->oprogs; prg; prg= prg->next )
        {
            if ( prg->trig_type == OTRIG_TIMER )
            {
                if (!is_number(prg->trig_phrase))
                {
                    bugf("Bad timer phrase for object %d: %s, must be number.",
                            obj->pIndexData->vnum, prg->trig_phrase);
                    return;
                }
                register_OBJ_timer( obj, atoi(prg->trig_phrase));
                return; /* only one allowed */
            }
        }
    }
}

void oprog_setup( OBJ_DATA *obj )
{
    /* initialize timer, may add more setup steps later */
    oprog_timer_init( obj );
}
