/* originally by Smote (?) for Aarchon MUD */
/* heavily reworked/rewritten by Vodur 3/16/2013 */

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

#define SMITH_ARG_FUN( fun )    void fun( CHAR_DATA *ch, char *argument )
#define SMITH_SET_FUN( fun )    void fun( CHAR_DATA *ch, char *argument )
SMITH_DATA *smith_new( OBJ_DATA *obj );

DECLARE_DO_FUN(do_smith);

typedef void SMITH_SET_FUN args( ( CHAR_DATA *ch, char *argument) );
typedef void SMITH_FUN args( ( CHAR_DATA *ch, char *argument ) );
#define DECLARE_SMITH_FUN( fun )        SMITH_FUN       fun
#define DECLARE_SMITH_SET_FUN( fun )    SMITH_SET_FUN   fun

/* local functions */
bool can_smith_obj( OBJ_DATA *obj);
int calc_smith_cost( CHAR_DATA *ch );
bool try_pay_smith( CHAR_DATA *ch );

struct smith_arg
{
    char * const    name;
    char * const    hint;
    SMITH_FUN *     fun;
};

struct smith_set_arg
{
    char * const    name;
    SMITH_SET_FUN * fun;
};

DECLARE_SMITH_FUN( smith_give );
DECLARE_SMITH_FUN( smith_status);
DECLARE_SMITH_FUN( smith_set);
DECLARE_SMITH_FUN( smith_cancel);
DECLARE_SMITH_FUN( smith_finish);

const struct smith_arg smith_arg_table[] =
{
    {   "give",     "Start your order [smith give <object>]",   smith_give      },
    {   "status",   "See the status of your order.",            smith_status    },
    {   "set",      "Set options on your order.",               smith_set       },
    {   "cancel",   "Cancel your order and retrive your item.", smith_cancel    },
    {   "finish",   "Finish and pay for your order.",           smith_finish    },
    {   NULL,       NULL,                                       NULL            }
};

DECLARE_SMITH_SET_FUN( smith_set_short_descr);
DECLARE_SMITH_SET_FUN( smith_set_name);
DECLARE_SMITH_SET_FUN( smith_set_description);

const struct smith_set_arg smith_set_table[] =
{
    {   "name",         smith_set_short_descr   },
    {   "keywords",     smith_set_name          },
    {   "description",  smith_set_description   },
    {   NULL,           NULL                    }
};




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
    
    struct smith_arg * arg_entry;

    int i;
    for ( arg_entry=&smith_arg_table[i=0] ; arg_entry->name ; arg_entry=&smith_arg_table[++i] )
    {
        if ( !strcmp( arg1, arg_entry->name) )
        {
            ( *(arg_entry->fun) )( ch, argument);
            return;
        }
    }

    /* didn't catch anything, let's display help */
    send_to_char( "Options:\n\r", ch );
    for ( arg_entry=&smith_arg_table[i=0] ; arg_entry->name ; arg_entry=&smith_arg_table[++i] )
    {
        printf_to_char(ch, "  %-15s%s\n\r", arg_entry->name, arg_entry->hint );
    } 
   
}


SMITH_ARG_FUN( smith_set)
{
    if ( !ch->pcdata->smith )
    {
        send_to_char("You don't have any order pending.\n\r", ch );
        return;
    }

    char arg1[MIL];
    argument=one_argument( argument, arg1);

    struct smith_set_arg * arg_entry;

    int i;
    for ( arg_entry=&smith_set_table[i=0] ; arg_entry->name ; arg_entry=&smith_set_table[++i] )
    {
        if ( !strcmp( arg1, arg_entry->name) )
        {
            ( *(arg_entry->fun) )( ch, argument);
            return;
        }
    }

    /* didn't catch anything, let's display help */
    send_to_char( "Options:\n\r", ch );
    for ( arg_entry=&smith_set_table[i=0] ; arg_entry->name ; arg_entry=&smith_set_table[++i] )
    {
        printf_to_char(ch, "  %s\n\r", arg_entry->name );
    }

   

}

void smith_set_name( CHAR_DATA *ch, char *argument )
{
    if ( !ch->pcdata->smith->new_obj )
    {
        bugf("smith_set_name: no new_obj found");
        return;
    }

    free_string(ch->pcdata->smith->new_obj->name);
    ch->pcdata->smith->new_obj->name = str_dup( argument );
}

void smith_set_description( CHAR_DATA *ch, char *argument )
{
    if ( !ch->pcdata->smith->new_obj )
    {
        bugf("smith_set_description: no new_obj found");
        return;
    }

    char buf[MIL];
    sprintf( buf, "%s{x", argument);
    free_string(ch->pcdata->smith->new_obj->description);
    ch->pcdata->smith->new_obj->description = str_dup( buf );
}

void smith_set_short_descr( CHAR_DATA *ch, char *argument )
{
    if ( !ch->pcdata->smith->new_obj )
    {
        bugf("smith_set_short_descr: no new_obj found");
        return;
    }
    
    char buf[MIL];
    sprintf( buf, "%s{x", argument );
    free_string(ch->pcdata->smith->new_obj->short_descr);
    ch->pcdata->smith->new_obj->short_descr = str_dup( buf );
}



SMITH_ARG_FUN( smith_status )
{
    if ( !ch->pcdata->smith )
    {
        send_to_char("Nothing to show status for!\n\r",ch);
        return;
    }

    send_to_char( "Original\n\r", ch);
    show_smith_obj_to_char( ch->pcdata->smith->old_obj, ch );
    send_to_char( "\n\r", ch );
    send_to_char( "Altered\n\r", ch );
    show_smith_obj_to_char( ch->pcdata->smith->new_obj, ch );
    send_to_char( "\n\r", ch );

    ptc( ch, "Cost: %d\n\r", calc_smith_cost( ch ) );

}

int calc_smith_cost( CHAR_DATA *ch )
{
    /* insert some calculation stuff here */
    /* tbc */
    return 0;
}

void show_smith_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch )
{
    if ( !obj )
    {
        bugf("NULL obj in show_smith_obj_to_char!");
        return;
    }

    ptc( ch, "Keywords: %s\n\r", obj->name );
    ptc( ch, "Name: %s\n\r", obj->short_descr );
    ptc( ch, "Description: %s\n\r", obj->description );
}

SMITH_ARG_FUN( smith_finish )
{
    if ( !ch->pcdata->smith )
    {
        send_to_char("Finish what?\n\r", ch);
        return;
    }

    if ( !try_pay_smith( ch ) )
    {
        return;
    }

    act( "The smith hands $p back to you with modifications!", 
            ch, ch->pcdata->smith->new_obj, NULL, TO_CHAR    );
    act( "The smith hands $p back to $n with modifications!", 
            ch, ch->pcdata->smith->new_obj, NULL, TO_ROOM    );
    obj_to_char( ch->pcdata->smith->new_obj, ch );

    if ( ch->pcdata->smith->old_obj )
        extract_obj( ch->pcdata->smith->old_obj );

    smith_free( ch->pcdata->smith );
    ch->pcdata->smith = NULL ;

}

bool try_pay_smith( CHAR_DATA *ch )
{
    /* insert payment logic here */
    /* tbc */
    return TRUE;
}


SMITH_ARG_FUN( smith_give )
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

    if ( !can_smith_obj(obj) )
    {
        send_to_char("The smith can't accept that object.\n\r", ch);
        return;
    }

    ch->pcdata->smith = smith_new(obj);
    obj_from_char(obj);

    act( "You hand $p over to the smith.", ch, obj, NULL, TO_CHAR    );
    act( "$n hands $p over to the smith.", ch, obj, NULL, TO_ROOM    );

}

bool can_smith_obj( OBJ_DATA *obj )
{
    if ( obj->item_type == ITEM_CONTAINER )
    {
        return FALSE;
    }
    return TRUE;
}

SMITH_ARG_FUN( smith_cancel )
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

void cancel_smith( CHAR_DATA *ch )
{
    if ( ch->pcdata->smith )
    {
        act( "The smith hands $p back to you.", 
                ch, ch->pcdata->smith->old_obj, NULL, TO_CHAR    );
        act( "The smith hands $p back to $n.", 
                ch, ch->pcdata->smith->old_obj, NULL, TO_ROOM    );
        
        obj_to_char( ch->pcdata->smith->old_obj, ch );

        if ( ch->pcdata->smith->new_obj )
            extract_obj( ch->pcdata->smith->new_obj );

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

    return sm;
}

void smith_free( SMITH_DATA *sm )
{
    if ( sm == NULL )
        return;

    free_mem( sm, sizeof( SMITH_DATA ) );

}
