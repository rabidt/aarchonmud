/* Countdown timers for progs */
/* Written by Vodur for Aarchon MUD
   Clayton Richey, clayton.richey@gmail.com
   */
#include <time.h>
#include <stdio.h>
#include "merc.h"
#include "timer.h"

#define TYPE_UNDEFINED 0
#define TYPE_CH 1
#define TYPE_OBJ 2
#define TYPE_AREA 3

#define TM_UNDEFINED 0
#define TM_PROG      1
#define TM_LUAFUNC   2

struct timer_node
{
    struct timer_node *next;
    struct timer_node *prev;
    int tm_type;
    void *game_obj;
    int go_type;
    int current; /* current val that gets decremented each second */
    bool unregistered; /* to mark for deletion */
};


TIMER_NODE *first_timer=NULL;

static void add_timer( TIMER_NODE *tmr);
static void free_timer_node( TIMER_NODE *tmr);
static TIMER_NODE *new_timer_node( void *gobj, int go_type, int tm_type, int max );
static void print_timer_list();

void * register_lua_timer( int value)
{
    TIMER_NODE *tmr=new_timer_node( NULL , TYPE_UNDEFINED, TM_LUAFUNC, value );
    add_timer(tmr);
    
    return (void *)tmr;
}

/* register on the list and also return a pointer to the node
   in the form of void */
void * register_CH_timer( CHAR_DATA *ch, int max )
{
    if ( ch->trig_timer)
    {
        bugf("Tying to register timer for %s but already registered.", ch->name);
        return NULL;
    }

    TIMER_NODE *tmr=new_timer_node( (void *)ch, TYPE_CH, TM_PROG, max);

    add_timer(tmr);

    ch->trig_timer=(void *)tmr;

    return (void *)tmr;

}

/* register on the list and also return a pointer to the node
   in the form of void */
void * register_OBJ_timer( OBJ_DATA *obj, int max )
{
    if ( obj->otrig_timer)
    {
        bugf("Tying to register timer for %s but already registered.", obj->name);
        return NULL;
    }

    TIMER_NODE *tmr=new_timer_node( (void *)obj, TYPE_OBJ, TM_PROG, max);

    add_timer(tmr);

    obj->otrig_timer=(void *)tmr;

    return (void *)tmr;

}

/* register on the list and also return a pointer to the node
   in the form of void */
void * register_AREA_timer( AREA_DATA *area, int max )
{
    if ( area->atrig_timer)
    {
        bugf("Tying to register timer for %s but already registered.", area->name);
        return NULL;
    }

    TIMER_NODE *tmr=new_timer_node( (void *)area, TYPE_AREA, TM_PROG, max);

    add_timer(tmr);

    area->atrig_timer=(void *)tmr;

    return (void *)tmr;

}

static void add_timer( TIMER_NODE *tmr)
{
    if (first_timer)
        first_timer->prev=tmr;
    tmr->next=first_timer;
    first_timer=tmr;

}

static void remove_timer( TIMER_NODE *tmr )
{
    if ( tmr->prev)
        tmr->prev->next=tmr->next;
    if ( tmr->next)
        tmr->next->prev=tmr->prev;
    if ( tmr==first_timer )
        first_timer=tmr->next;

    free_timer_node(tmr);
    return;
}

void unregister_CH_timer( CHAR_DATA *ch )
{
    if (!ch->trig_timer)
    {
        /* doesn't have one */
        return;
    }
    TIMER_NODE *tmr=(TIMER_NODE *)ch->trig_timer;

    tmr->unregistered=TRUE; /* queue it for removal next update */ 
    ch->trig_timer=NULL;
    return;
}

static void free_timer_node( TIMER_NODE *tmr)
{
    free_mem(tmr, sizeof(TIMER_NODE));
}

static TIMER_NODE *new_timer_node( void *gobj, int go_type, int tm_type, int seconds )
{
    TIMER_NODE *new=alloc_mem(sizeof(TIMER_NODE));
    new->next=NULL;
    new->prev=NULL;
    new->tm_type=tm_type;
    new->game_obj=gobj;
    new->go_type=go_type;
    new->current=seconds;
    new->unregistered=FALSE;
    return new;
}

/* Should be called every second */
/* need to solve the problem of gobj destruction and unregistering
   screwing up loop iteration and crashing mud */
/* what happens when tmr_next is destroyed by current tmr */
/* condition 1
   tmr_next gobj is destroyed by prog of tmr
   tmr_next is unregistered and destroyed but we are still
   pointing to it
   how do we prevent this happening?
   set tmr-next AFTER the prog processes but before we destroy tmr

   but what if tmr gobj was destroyed during the prog?
   we could set tmr next at the top of the loop but also
   re-set it after the prog just in case

   this would work in cases where only tmr_next was destroyed
   or only tmr was destroyed, but what if both were destroyed?

   we probably need to unregister timers outside of progs
   ( similar to must_extract ) to avoid all of these shenanigans.
   simply set the must_extract bit. if we destroyed something
   we already looped past, we grab it on the next timer_update.
   if we destroyed something not looped yet, we just cleanly
   unregister/free it when its turn is up */

void timer_update()
{
    TIMER_NODE *tmr, *tmr_next;
    CHAR_DATA *ch;
    OBJ_DATA *obj;
    AREA_DATA *area;

    for (tmr=first_timer ; tmr ; tmr=tmr_next)
    {
        tmr_next=tmr->next;

        if ( tmr->unregistered )
        {
            /* it was unregistered since the last update
               we need to kill it cleanly */
            remove_timer( tmr );
            continue;
        }

        tmr->current-=1;
        if (tmr->current <= 0)
        {
            switch(tmr->tm_type)
            {
                case TM_PROG:
                    switch( tmr->go_type )
                    {
                        case TYPE_CH: 
                            ch=(CHAR_DATA *)(tmr->game_obj);
                            if (!IS_VALID(ch))
                            {
                                /* Shouldn't happen since we unregister
                                   on extract */
                                bugf("timer_update: invalid ch %s", ch->name);
                                break;
                            }
                            mp_timer_trigger( ch );
                            /* repeating timer, set it up again */
                            if (IS_VALID(ch))
                            {
                                ch->trig_timer=NULL;
                                mprog_timer_init( ch );
                            }
                            break;

                        case TYPE_OBJ:
                            obj=(OBJ_DATA *)(tmr->game_obj);
                            if (!IS_VALID(obj))
                            {
                                bugf("timer_update: invalid obj %s", obj->name);
                                break;
                            }
                            op_timer_trigger( obj );
                            if (IS_VALID(obj))
                            {
                                obj->otrig_timer=NULL;
                                oprog_timer_init( obj );
                            }
                            break;

                        case TYPE_AREA:
                            /* no need for valid check on areas */
                            area=(AREA_DATA *)(tmr->game_obj);
                            ap_timer_trigger( area );
                            area->atrig_timer=NULL;
                            aprog_timer_init( area );
                            break;

                        default:
                            bugf("Bad stuff.");
                            return;
                    }
                    break;
                case TM_LUAFUNC:
                    run_delayed_function(tmr);
                    break;
                default:
                    bugf("You broke it.");
            }
            /* it fired, kill it */
            remove_timer( tmr );
        }
    }
}

static void print_timer_list()
{
    TIMER_NODE *tmr;
    int i=1;
    for ( tmr=first_timer; tmr; tmr=tmr->next )
    {
        bugf("%d %s", i,
            tmr->tm_type == TM_LUAFUNC ? "luafunc" :
            tmr->go_type == TYPE_CH ? ((CHAR_DATA *)(tmr->game_obj))->name :
            tmr->go_type == TYPE_OBJ ? ((OBJ_DATA *)(tmr->game_obj))->name :
            "unknown");
        i++;
    }
    return;

}
