#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"

#define MAX_WEAPON_SUBTYPES 8
#define MAX_ARMOR_SUBTYPES 12

SMITH_DATA *smith_new( OBJ_DATA *obj );

DECLARE_DO_FUN(do_smith);
DECLARE_DO_FUN(do_save);


/* new stuff starts here */
void do_smith( CHAR_DATA *ch, char *argument )
{
    char arg1[MIL];

    argument=one_argument( argument, arg1);

    if ( !IS_SET(ch->in_room->room_flags, ROOM_BLACKSMITH))
    {
        send_to_char("There is no smithy here!\n\r", ch);
        return;
    }

    if ( arg1[0] == '\0' )
    {
        send_to_char("Which item should the smith work on?\n\r", ch );
        send_to_char("(smith give <obj name> to begin)\n\r", ch );
        return;
    }

    if ( !strcmp( arg1, "give" ) )
    {
        smith_give( ch, argument);
        return;
    } 

    if ( !strcmp( arg1, "cancel") )
    {
        if ( ch->pcdata->smith )
        {
            cancel_smith(ch);
            return;
        }
        else
        {
            send_to_char("You don't have any order to cancel!\n\r", ch );
            return;
        }
    }

    if ( !strcmp( arg1, "finish" ) )
    {
        smith_finish( ch );
        return;
    }

}

void smith_finish( CHAR_DATA *ch )
{
    if ( !ch->pcdata->smith )
    {
        send_to_char("Finish what?\n\r", ch);
        return;
    }

    printf_to_char( ch, "The smith returns %s to you with your modifications!\n\r",
            ch->pcdata->smith->new_obj->short_descr);
    obj_to_char( ch->pcdata->smith->new_obj, ch );

    if ( ch->pcdata->smith->old_obj )
        free_obj( ch->pcdata->smith->old_obj );

    smith_free( ch->pcdata->smith );
    ch->pcdata->smith = NULL ;

}
void smith_give( CHAR_DATA *ch, char *argument )
{

    if ( argument[0] == '\0' )
    {
        send_to_char("Give what?\n\r", ch);
        return;
    }
    if ( ch->pcdata->smith )
    {
        printf_to_char(ch,"The smith already has %s!\n\r",
                ch->pcdata->smith->old_obj->short_descr);
        return;
    }

    OBJ_DATA *obj = get_obj_carry( ch, argument, ch );
    if ( !obj )
    {
        send_to_char("You don't have that object!\n\r", ch );
        return;
    }

    ch->pcdata->smith = smith_new(obj);
    obj_from_char(obj);
    printf_to_char( ch, "You hand %s over to the smith.\n\r",
            obj->short_descr);

}


void cancel_smith( CHAR_DATA *ch )
{
    if ( ch->pcdata->smith )
    {
        printf_to_char( ch, "The smith returns %s to you unchanged.\n\r",
                ch->pcdata->smith->old_obj->short_descr);
        obj_to_char( ch->pcdata->smith->old_obj, ch );

        if ( ch->pcdata->smith->new_obj )
            free_obj( ch->pcdata->smith->new_obj );

        smith_free( ch->pcdata->smith );
        ch->pcdata->smith = NULL ;
    }
}


SMITH_DATA *smith_new( OBJ_DATA *obj )
{
    SMITH_DATA *sm;
    sm=alloc_mem( sizeof(SMITH_DATA) );
    sm->old_obj=obj;

    /* clone the heck out of this thing */
    sm->new_obj= create_object(sm->old_obj->pIndexData,0);
    clone_object(sm->old_obj,sm->new_obj);
    //recursive_clone(ch,obj,clone);

    sm->cost=0;

    return sm;
}

void smith_free( SMITH_DATA *sm )
{
    if ( sm == NULL )
        return;

    free_mem( sm, sizeof( SMITH_DATA ) );

}
