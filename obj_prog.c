#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <ctype.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"


/*
 * A general purpose percentage trigger. Checks if a random percentage
 * number is less than trigger phrase
 */
bool op_percent_trigger(
    OBJ_DATA *obj, CHAR_DATA *ch1, CHAR_DATA *ch2, int type)
{
    OPROG_LIST *prg;

    for ( prg = obj->pIndexData->oprogs; prg != NULL; prg = prg->next )
    {
        if ( prg->trig_type == type
           && number_percent() <= atoi( prg->trig_phrase ) )
        {
            return lua_obj_program( prg->vnum, prg->code, obj, ch1, ch2);
        }
    }
    return TRUE;
}
#if 0
bool op_give_trigger(
    OBJ_DATA *obj, CHAR_DATA *giver, CHAR_DATA *receiver)
{
    OPROG_LIST *prg;

    for ( prg=obj->pIndexData->oprogs ; prg ; prg = prg->next )
    {
        if ( prg->trig_type == OTRIG_GIVE )
        {
            return lua_obj_program( prg->vnum, prg->code, obj, giver, receiver);
        }
    }
    return TRUE;
}

bool op_drop_trigger(
    OBJ_DATA *obj, CHAR_DATA *dropper)
{
    OPROG_LIST *prg;

    for ( prg=obj->pIndexData->oprogs ; prg ; prg = prg->next )
    {
        if ( prg->trig_type == OTRIG_DROP )
        {
            return lua_obj_program( prg->vnum, prg->code, obj, dropper, NULL);
        }
    }
    return TRUE;
}

bool op_eat_trigger(
    OBJ_DATA *obj, CHAR_DATA *eater)
{
    OPROG_LIST *prg;

    for ( prg=obj->pIndexData->oprogs ; prg ; prg = prg->next )
    {
        if ( prg->trig_type == OTRIG_EAT )
        {
            return lua_obj_program( prg->vnum, prg->code, obj, eater, NULL);
        }
    }
    return TRUE;
}

bool op_sacrifice_trigger(
    OBJ_DATA *obj, CHAR_DATA *saccer)
{
    OPROG_LIST *prg;

    for ( prg=obj->pIndexData->oprogs ; prg ; prg = prg->next )
    {
        if ( prg->trig_type == OTRIG_SACRIFICE )
        {
            return lua_obj_program( prg->vnum, prg->code, obj, saccer, NULL);
        }
    }
    return TRUE;
}

bool op_wear_trigger(
    OBJ_DATA *obj, CHAR_DATA *wearer)
{
    OPROG_LIST *prg;

    for ( prg=obj->pIndexData->oprogs ; prg ; prg = prg->next )
    {
        if ( prg->trig_type == OTRIG_WEAR )
        {
            return lua_obj_program( prg->vnum, prg->code, obj, wearer, NULL);
        }
    }
    return TRUE;
}    
#endif
