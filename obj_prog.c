#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <ctype.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"


bool op_give_trigger(
    OBJ_DATA *obj, CHAR_DATA *giver, CHAR_DATA *receiver)
{
    char buf[MSL], *p;
    OPROG_LIST *prg;

    for ( prg = obj->pIndexData->oprogs ; prg ; prg = prg->next )
    {
        if ( prg->trig_type == OTRIG_GIVE )
        {
            p = prg->trig_phrase;
            /*
             * Vnum argument
             */
            if ( is_r_number( p ) )
            {
                if ( IS_NPC( receiver ) 
                   && receiver->pIndexData->vnum==r_atoi_obj(obj, p) )
                {
                    lua_obj_program( prg->vnum, prg->code, obj, giver, receiver);
                    return TRUE;
                }
            }
            /*
             * Character name argument, e.g. 'dragon'
             */
            else
            {
                while (*p)
                {
                    p=one_argument( p, buf);

                    if ( is_name( buf, receiver->name ) 
                        || !str_cmp( "all", buf)
                        || !str_cmp( "*", buf) )
                    {
                        log_string(prg->code);
                        lua_obj_program( prg->vnum, prg->code, obj, giver, receiver);
                        return TRUE;
                    }
                }
            }
        }
    }
}

bool op_drop_trigger(
    OBJ_DATA *obj, CHAR_DATA *dropper)
{
    OPROG_LIST *prg;

    for ( prg=obj->pIndexData->oprogs ; prg ; prg = prg->next )
    {
        if ( prg->trig_type == OTRIG_DROP )
        {
            /* TBC make a percent trigger for oprogs */
            lua_obj_program( prg->vnum, prg->code, obj, dropper, NULL);
            return TRUE;
        }
    }
}



    
