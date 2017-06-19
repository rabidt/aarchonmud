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
#include "interp.h"

/*
 * A general purpose percentage trigger. Checks if a random percentage
 * number is less than trigger phrase
 */
bool op_percent_trigger(
        const char *trigger, 
        OBJ_DATA *obj, OBJ_DATA *obj2, CHAR_DATA *ch1, CHAR_DATA *ch2, 
        int type )
{
    if (!obj)
    {
        bugf("NULL obj passed to op_percent_trigger");
        return TRUE;
    }

    if ( !IS_VALID(obj) || !HAS_OTRIG(obj, type) )
        return TRUE;

    PROG_LIST *prg;

    for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
    {
        if ( prg->trig_type == type
                && number_percent() <= atoi( prg->trig_phrase ) )
        {
            return ( lua_obj_program( g_mud_LS, trigger, prg->vnum, prg->script->code, obj, obj2, ch1, ch2, type, prg->script->security)
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

bool op_act_trigger(OBJ_DATA *obj, CHAR_DATA *ch1, CHAR_DATA *ch2, const char *trigger, int type)
{
    PROG_LIST *prg;

    for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
    {
        if ( prg->trig_type == type
                /* should be case-insensitive --Bobble
                   && strstr( argument, prg->trig_phrase ) != NULL )
                 */
            && ( strstr(cap_all(trigger), cap_all(prg->trig_phrase)) != NULL
                    ||   !strcmp(prg->trig_phrase, "*") ) )
                    {
                        return (lua_obj_program( g_mud_LS, trigger, prg->vnum, prg->script->code, obj, NULL, ch1, NULL, type, prg->script->security)
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

/* similar to act trigger but it needs its own logic in the end */
bool op_command_trigger( CHAR_DATA *ch, int cmd, const char *argument )
{
    PROG_LIST *prg;
    OBJ_DATA *obj;
    OBJ_DATA *next_obj;
    bool continu;

    if ( !ch->in_room )
    {
        bugf("op_command_trigger: ch->in_room NULL for %s", ch->name);
        return TRUE;
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_COMMAND) )
        {
            for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
            {
                if ( prg->trig_type == OTRIG_COMMAND && !str_cmp(cmd_table[cmd].name, prg->trig_phrase) )
                {
                    continu = lua_obj_program(g_mud_LS, cmd_table[cmd].name, prg->vnum, prg->script->code, obj, NULL, ch, NULL, OTRIG_COMMAND, prg->script->security);
                    if ( !continu )
                        return FALSE;
                }
            }
        }
    }

    for ( obj = ch->carrying; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_COMMAND) )
        {
            for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
            {
                if ( prg->trig_type == OTRIG_COMMAND && !str_cmp(cmd_table[cmd].name, prg->trig_phrase) )
                {
                    continu = lua_obj_program(g_mud_LS, cmd_table[cmd].name, prg->vnum, prg->script->code, obj, NULL, ch, NULL, OTRIG_COMMAND, prg->script->security);
                    if ( !continu )
                        return FALSE;
                }
            }
        }
    }

    return TRUE;
}

/* similar to act trigger, but we need to return whether or not any matching trigger was found */
bool op_try_trigger( const char *trigger, CHAR_DATA *ch )
{
    PROG_LIST *prg;
    OBJ_DATA *obj;
    OBJ_DATA *next_obj;
    bool found = FALSE;
    bool continu=TRUE;

    if ( !ch->in_room )
    {
        bugf("op_try_trigger: ch->in_room NULL for %s", ch->name);
        return FALSE;
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_TRY) )
        {
            for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
            {
                if ( prg->trig_type == OTRIG_TRY 
                    && ( strstr(cap_all(trigger), cap_all(prg->trig_phrase)) != NULL                      
                        ||   !strcmp(prg->trig_phrase, "*") ) )
                {
                    found = TRUE;
                    continu = lua_obj_program( g_mud_LS, trigger, prg->vnum, prg->script->code, obj, NULL, ch, NULL, OTRIG_TRY, prg->script->security);

                    if (!continu)
                        return found;
                }
            }
        }
    }

    for ( obj = ch->carrying; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_TRY) )
        {
            for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
            {
                if ( prg->trig_type == OTRIG_TRY 
                    && ( strstr(cap_all(trigger), cap_all(prg->trig_phrase)) != NULL                      
                        ||   !strcmp(prg->trig_phrase, "*") ) )
                {
                    found = TRUE;
                    continu = lua_obj_program( g_mud_LS, trigger, prg->vnum, prg->script->code, obj, NULL, ch, NULL, OTRIG_TRY, prg->script->security);

                    if (!continu)
                        return found;
                }
            }
        }
    }

    return found;
}


bool op_move_trigger( CHAR_DATA *ch )
{
    OBJ_DATA *obj, *next_obj;
    bool rtn=TRUE;

    for ( obj = ch->carrying ; obj != NULL; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_MOVE) )
        {
            rtn = rtn && op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_MOVE); 
        }
    }

    return rtn;
}

void op_speech_trigger( const char *argument, CHAR_DATA *ch )
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
            op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_GREET);
        }
    }
}

void op_fight_trigger( CHAR_DATA *ch, CHAR_DATA *vic )
{
    OBJ_DATA *obj;
    OBJ_DATA *next_obj;

    for ( obj = ch->carrying ; obj ; obj = next_obj )
    {
        next_obj = obj->next_content;

        /* only checking worn and wielded items */
        if ( obj->wear_loc != WEAR_NONE && HAS_OTRIG(obj, OTRIG_FIGHT) )
        {
            op_percent_trigger(NULL, obj, NULL, ch, vic, OTRIG_FIGHT);
        }
    }
}

void op_death_trigger( CHAR_DATA *ch, CHAR_DATA *vic )
{
    OBJ_DATA *obj, *next_obj;

    for ( obj = ch->carrying ; obj ; obj = next_obj )
    {
        next_obj = obj->next_content;

        if ( HAS_OTRIG(obj, OTRIG_DEATH) )
        {
            op_percent_trigger(NULL, obj, NULL, ch, vic, OTRIG_DEATH);
        }
    }
}

bool op_merge_trigger( CHAR_DATA *ch, OBJ_DATA *obj1, OBJ_DATA *obj2)
{
    if (!op_percent_trigger(NULL, obj1, obj2, ch, NULL, OTRIG_MERGE))
        return FALSE;

    if (!op_percent_trigger(NULL, obj2, obj1, ch, NULL, OTRIG_MERGE))
        return FALSE;

    return TRUE;
}

bool op_prehit_trigger( OBJ_DATA *obj, CHAR_DATA *ch, CHAR_DATA *vic, int damage)
{
    char damstr[MSL];
    sprintf( damstr, "%d", damage);
    return op_percent_trigger(damstr, obj, NULL, ch, vic, OTRIG_PREHIT);
}

void op_timer_trigger( OBJ_DATA *obj )
{
    PROG_LIST *prg;

    for ( prg=obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
    {
        if (prg->trig_type == OTRIG_TIMER)
        {
            lua_obj_program( g_mud_LS, NULL, prg->vnum, prg->script->code, 
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
        PROG_LIST *prg;
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
                register_obj_timer( obj, atoi(prg->trig_phrase));
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
