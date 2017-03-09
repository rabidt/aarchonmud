/* rewritten by Vodur 3/16/2013 */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"

#define SM_FUN( fun )    static void fun( CHAR_DATA *ch, const char *argument )
#define SM_SET_FUN( fun )    static void fun( CHAR_DATA *ch, const char *argument )
#define SM_PRICE_FUN( fun )  static void fun( CHAR_DATA *ch, int *gold, int *qp )
static SMITH_DATA *smith_new( OBJ_DATA *obj );

DECLARE_DO_FUN(do_smith);

typedef void SMITH_SET_FUN args( ( CHAR_DATA *ch, const char *argument) );
typedef void SMITH_FUN args( ( CHAR_DATA *ch, const char *argument ) );
typedef void SMITH_PRICE_FUN args( ( CHAR_DATA *ch, int *gold, int *qp ) );

/* local functions */
static bool check_smith_obj( CHAR_DATA *ch, OBJ_DATA *obj);
static void calc_smith_cost( CHAR_DATA *ch, int *gold, int *qp );
static bool try_pay_smith( CHAR_DATA *ch );
static void show_smith_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch );
static void smith_free( SMITH_DATA *sm );

struct smith_arg
{
    char * const    name;
    char * const    hint;
    SMITH_FUN *     fun;
};

struct smith_set_arg
{
    char * const            name;
    SMITH_SET_FUN *         fun;
    SMITH_PRICE_FUN *       price_fun;
    char * const            price_string;
};

SM_FUN( smith_give );
SM_FUN( smith_status);
SM_FUN( smith_set);
SM_FUN( smith_cancel);
SM_FUN( smith_finish);

static const struct smith_arg smith_arg_table[] =
{
    {   "give",     "Start your order [smith give <object>]",   smith_give      },
    {   "status",   "See the status of your order.",            smith_status    },
    {   "set",      "Set options on your order.",               smith_set       },
    {   "cancel",   "Cancel your order and retrieve your item.", smith_cancel    },
    {   "finish",   "Finish and pay for your order.",           smith_finish    },
    {   NULL,       NULL,                                       NULL            }
};

SM_SET_FUN( smith_set_short_descr);
SM_SET_FUN( smith_set_name);
SM_SET_FUN( smith_set_description);
SM_SET_FUN( smith_set_sticky);

SM_PRICE_FUN( smith_set_name_price );
SM_PRICE_FUN( smith_set_sticky_price);

static const struct smith_set_arg smith_set_table[] =
{
    {   "name",         smith_set_short_descr,  smith_set_name_price,
        "100qp, 5k gold (name, keywords, desc together)"                },
    {   "keywords",     smith_set_name,         NULL,                   
        "100qp, 5k gold (name, keywords, desc together)"                },
    {   "description",  smith_set_description,  NULL,                   
        "100qp, 5k gold (name, keywords, desc together)"                },
    {   "sticky",       smith_set_sticky,       smith_set_sticky_price,
        "Based on item power"                                           },
    {   NULL,           NULL,                   NULL,                  
        NULL                                                            }
};


DEF_DO_FUN(do_smith)
{
    if (IS_NPC(ch))
        return;

    char arg1[MIL];

    argument=one_argument( argument, arg1);

    if ( !IS_SET(ch->in_room->room_flags, ROOM_BLACKSMITH))
    {
        send_to_char("There is no smithy here!\n\r", ch);
        return;
    }
    
    const struct smith_arg * arg_entry;

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


SM_FUN( smith_set)
{
    if ( !ch->pcdata->smith )
    {
        send_to_char("You don't have any order pending.\n\r", ch );
        return;
    }

    char arg1[MIL];
    argument=one_argument( argument, arg1);

    const struct smith_set_arg * arg_entry;

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
    ptc( ch, "  %-15s%s\n\r", "Setting", "Cost");
    for ( arg_entry=&smith_set_table[i=0] ; arg_entry->name ; arg_entry=&smith_set_table[++i] )
    {
        printf_to_char(ch, "  %-15s%s\n\r", arg_entry->name, arg_entry->price_string );
    }

   

}

SM_SET_FUN( smith_set_name)
{
    if ( !ch->pcdata->smith->new_obj )
    {
        bugf("smith_set_name: no new_obj found");
        return;
    }

    free_string(ch->pcdata->smith->new_obj->name);
    ch->pcdata->smith->new_obj->name = str_dup( argument );
}

SM_SET_FUN( smith_set_description )
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

SM_SET_FUN( smith_set_short_descr )
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

SM_SET_FUN( smith_set_sticky )
{
    if ( !ch->pcdata->smith->new_obj )
    {
        bugf("smith_set_sticky: no new_obj found");
        return;
    }
    if ( !strcmp( argument, "on" ) )
    {
        if (is_sticky_obj(ch->pcdata->smith->new_obj) )
        {
            send_to_char("It's already sticky.", ch);
            return;
        }
        else
        {
            SET_BIT(ch->pcdata->smith->new_obj->extra_flags, ITEM_STICKY);
            if ( ch->pcdata->smith->new_obj->owner )
            {
                free_string(ch->pcdata->smith->new_obj->owner);
            } 
            ch->pcdata->smith->new_obj->owner= str_dup( ch->name);
            return;
        }
    }
    else if ( !strcmp( argument, "off" ) )
    {
        if (!is_sticky_obj(ch->pcdata->smith->new_obj) )
        {
            send_to_char( "It's not sticky.", ch);
            return;
        }
        else
        {
            REMOVE_BIT(ch->pcdata->smith->new_obj->extra_flags, ITEM_STICKY);
            if ( ch->pcdata->smith->new_obj->owner )
            {
                free_string(ch->pcdata->smith->new_obj->owner);
                ch->pcdata->smith->new_obj->owner= NULL;
            }
            return;
        }
    }
    else
    {
        send_to_char(" Valid arguments: on, off\n\r", ch );
        return;
    }

}


SM_FUN( smith_status )
{
    if ( !ch->pcdata->smith )
    {
        send_to_char("Nothing to show status for!\n\r",ch);
        return;
    }

    send_to_char( "Original\n\r", ch);
    show_smith_obj_to_char( ch->pcdata->smith->old_obj, ch );
    send_to_char( "\n\r", ch );
    send_to_char( "Modified\n\r", ch );
    show_smith_obj_to_char( ch->pcdata->smith->new_obj, ch );
    send_to_char( "\n\r", ch );

    int qp, gold;
    calc_smith_cost( ch, &gold, &qp );
    ptc( ch, "Cost: %d gold, %d qp\n\r",gold, qp );

}

static void calc_smith_cost( CHAR_DATA *ch, int *gold, int *qp )
{

    *gold=0;
    *qp=0; 

    const struct smith_set_arg * arg_entry;
    int i;
    for ( arg_entry=&smith_set_table[i=0] ; arg_entry->name ; arg_entry=&smith_set_table[++i] )
    {
        int igold=0;
        int iqp= 0;

        if (arg_entry->price_fun)
            ( *(arg_entry->price_fun) ) (ch, &igold, &iqp );

        *qp+=iqp;
        *gold+=igold;

    } 
}

static void show_smith_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch )
{
    if ( !obj )
    {
        bugf("NULL obj in show_smith_obj_to_char!");
        return;
    }

    ptc( ch, "Keywords: %s\n\r", obj->name );
    ptc( ch, "Name: %s\n\r", obj->short_descr );
    ptc( ch, "Description: %s\n\r", obj->description );
    if (is_sticky_obj( obj ) )
        ptc(ch, "Sticky: on    Owner: %s\n\r", ( obj->owner ? obj->owner : "none") );
    else
        send_to_char( "Sticky: off\n\r", ch );
        
}

SM_FUN( smith_finish )
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

#ifdef SMITH_LOG
    char buf[MSL];
    int gold,qp;
    calc_smith_cost( ch, &gold, &qp );
    sprintf(buf, "%s paid %d gold and %d quest points to smith for %s (%d)",
           ch->name, gold, qp, ch->pcdata->smith->new_obj->short_descr, 
           ch->pcdata->smith->new_obj->pIndexData->vnum);
    log_string(buf);
#endif 

    if ( ch->pcdata->smith->old_obj )
        extract_obj( ch->pcdata->smith->old_obj );

    smith_free( ch->pcdata->smith );
    ch->pcdata->smith = NULL ;

}

static bool try_pay_smith( CHAR_DATA *ch )
{
    /* insert payment logic here */
    int qp,gold;
    calc_smith_cost( ch, &gold, &qp);

    if ( gold == 0 && qp == 0 )
    {
        send_to_char( "You haven't made any changes!\n\r", ch );
        return FALSE;
    }

    if ( ch->gold < gold || ch->pcdata->questpoints < qp )
    {
        send_to_char( "You can't afford it!\n\r", ch );
        return FALSE;
    }
    else
    {
        ch->gold -= gold;
        ch->pcdata->questpoints -= qp;
        ptc(ch, "You pay %d gold and %d quest points to the smith.\n\r", gold, qp);

        return TRUE;
    }
}


SM_FUN( smith_give )
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

    if ( !check_smith_obj(ch, obj) )
    {
        return;
    }

    ch->pcdata->smith = smith_new(obj);
    obj_from_char(obj);

    act( "You hand $p over to the smith.", ch, obj, NULL, TO_CHAR    );
    act( "$n hands $p over to the smith.", ch, obj, NULL, TO_ROOM    );

#ifdef SMITH_LOG
    char buf[MSL];
    sprintf( buf, "%s gave %s (%d) to smith.", ch->name,
            obj->short_descr, obj->pIndexData->vnum);
    log_string(buf);
#endif
}

static bool check_smith_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( obj->item_type == ITEM_CONTAINER )
    {
        if ( obj->contains ) /* not empty*/
        {
            printf_to_char( ch, "%s must be empty.\n\r", obj->short_descr );
            return FALSE;
        }
    }
    if ( IS_SET( obj->pIndexData->extra_flags, ITEM_QUESTEQ ) ) /* check the proto */
    {
        send_to_char( "You can't alter quest equipment!\n\r", ch);
        return FALSE;
    }
    return TRUE;
}

SM_FUN( smith_cancel )
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
#ifdef SMITH_LOG
    char buf[MSL];
    sprintf(buf, "%s got %s (%d) back from smith.",
           ch->name, ch->pcdata->smith->old_obj->short_descr,
           ch->pcdata->smith->old_obj->pIndexData->vnum);
    log_string(buf);
#endif

        if ( ch->pcdata->smith->new_obj )
            extract_obj( ch->pcdata->smith->new_obj );

        smith_free( ch->pcdata->smith );
        ch->pcdata->smith = NULL ;
    }
}


static SMITH_DATA *smith_new( OBJ_DATA *obj )
{
    SMITH_DATA *sm;
    sm=alloc_mem( sizeof(SMITH_DATA) );
    sm->old_obj=obj;

    /* clone the heck out of this thing */
    sm->new_obj= create_object(sm->old_obj->pIndexData);
    clone_object(sm->old_obj,sm->new_obj);
    //recursive_clone(ch,obj,clone);

    return sm;
}

static void smith_free( SMITH_DATA *sm )
{
    if ( sm == NULL )
        return;

    free_mem( sm, sizeof( SMITH_DATA ) );

}

SM_PRICE_FUN( smith_set_name_price )
{
    if ( str_cmp( ch->pcdata->smith->old_obj->short_descr, ch->pcdata->smith->new_obj->short_descr )
        || str_cmp( ch->pcdata->smith->old_obj->name, ch->pcdata->smith->new_obj->name)
        || str_cmp( ch->pcdata->smith->old_obj->description, ch->pcdata->smith->new_obj->description ) )
    {
        *qp=100;
        *gold=5000;
    }
    else
    {
        *qp=0;
        *gold=0;
    }
}

SM_PRICE_FUN( smith_set_sticky_price )
{
    OBJ_DATA *old=ch->pcdata->smith->old_obj;
    OBJ_DATA *new=ch->pcdata->smith->new_obj;

    if ( ( is_sticky_obj( old ) 
           != is_sticky_obj( new ) )
         ||
         ( old->owner
           != new->owner ) )
    {
        int ops = UMAX(get_obj_ops(new), get_obj_spec(new));
        if ( IS_OBJ_STAT(new, ITEM_TRANSLUCENT_EX) )
            ops += get_translucency_spec_penalty( new->level );
        *gold= ops * UMAX(4, ops-20)/4 * 75;
        *qp= ops * UMAX(4, ops-20)/16;

    }
    else
    {
        *qp=0;
        *gold=0;
    }
    
}

