/* Countdown timers for progs */
/* Written by Vodur for Aarchon MUD
   Clayton Richey, clayton.richey@gmail.com
   */
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "timer.h"

#define GO_TYPE_UNDEFINED 0
#define GO_TYPE_CH 1
#define GO_TYPE_OBJ 2
#define GO_TYPE_AREA 3
#define GO_TYPE_ROOM 4

#define TM_UNDEFINED 0
#define TM_PROG      1
#define TM_LUAFUNC   2
#define TM_CFUNC     3

/* hide the struct implementation,
   we only want to manipulate nodes
   in this module */
struct timer_node
{
    struct timer_node *next;
    int tm_type;
    void *game_obj;
    void (*func)(void); /* pointer for C function */
    int go_type;
    int current; /* current val that gets decremented each second */
    bool unregistered; /* to mark for deletion */
    bool deleted; /* for debug stuff */
    const char *tag; /* used for unique tags in lua */
};


static TIMER_NODE *first_timer=NULL;

static void add_timer( TIMER_NODE *tmr);
static void free_timer_node( TIMER_NODE *tmr);
static TIMER_NODE *new_timer_node( void *gobj, void (*func)(), int go_type, int tm_type, int max, const char *tag );


void unregister_timer_node( TIMER_NODE *tmr )
{
//    if (tmr->unregistered)
//        bugf("unregistering already unregistered timer");
    tmr->unregistered=TRUE;
}

TIMER_NODE * register_c_timer( int value, TIMER_CFUN func)
{
    TIMER_NODE *tmr=new_timer_node( NULL, func, GO_TYPE_UNDEFINED, TM_CFUNC, value, NULL );
    add_timer(tmr);

    return tmr;
}

TIMER_NODE * register_lua_timer( int value, const char *tag)
{
    TIMER_NODE *tmr=new_timer_node( NULL, NULL, GO_TYPE_UNDEFINED, TM_LUAFUNC, value, tag );
    add_timer(tmr);
    
    return tmr;
}

/* unregister timer and return true if tag matches given tag, else return false*/
bool unregister_lua_timer( TIMER_NODE *tmr, const char *tag )
{
    if ( tag==NULL )
    {
        if ( tmr->tag != NULL )
        {
            return FALSE;
        }
        unregister_timer_node(tmr);
        return TRUE;
    }
    else if (!strcmp(tag, "*"))
    {
        unregister_timer_node(tmr);
        return TRUE;
    }
    else if ( !tmr->tag )
    {
        return FALSE;
    }
    else if ( !strcmp( tag, tmr->tag) )
    {
        unregister_timer_node(tmr);
        return TRUE;
    }

    return FALSE;
}

/* register on the list and also return a pointer to the node
*/
TIMER_NODE * register_ch_timer( CHAR_DATA *ch, int max )
{
    if (!valid_CH( ch ))
    {
        bugf("Trying to register timer for invalid CH");
        return NULL;
    }
    if (ch->must_extract)
    {
        bugf("Trying to register timer for CH pending extraction");
        return NULL;
    }
    if ( ch->trig_timer)
    {
        bugf("Tying to register timer for %s but already registered.", ch->name);
        return NULL;
    }

    TIMER_NODE *tmr=new_timer_node( (void *)ch, NULL, GO_TYPE_CH, TM_PROG, max, NULL);

    add_timer(tmr);

    ch->trig_timer=tmr;

    return tmr;

}

/* register on the list and also return a pointer to the node
*/
TIMER_NODE * register_obj_timer( OBJ_DATA *obj, int max )
{
    if ( obj->otrig_timer)
    {
        bugf("Tying to register timer for %s but already registered.", obj->name);
        return NULL;
    }

    TIMER_NODE *tmr=new_timer_node( (void *)obj, NULL, GO_TYPE_OBJ, TM_PROG, max, NULL);

    add_timer(tmr);

    obj->otrig_timer=tmr;

    return tmr;

}

/* register on the list and also return a pointer to the node
*/
TIMER_NODE * register_area_timer( AREA_DATA *area, int max )
{
    if ( area->atrig_timer)
    {
        bugf("Tying to register timer for %s but already registered.", area->name);
        return NULL;
    }

    TIMER_NODE *tmr=new_timer_node( (void *)area, NULL, GO_TYPE_AREA, TM_PROG, max, NULL);

    add_timer(tmr);

    area->atrig_timer=tmr;

    return tmr;

}

TIMER_NODE * register_room_timer( ROOM_INDEX_DATA *room, int max )
{
    if ( room->rtrig_timer)
    {
        bugf("Trying to register timer for room %d but already registered.", room->vnum);
        return NULL;
    }

    TIMER_NODE *tmr=new_timer_node( (void *)room, NULL, GO_TYPE_ROOM, TM_PROG, max, NULL);

    add_timer(tmr);

    room->rtrig_timer=tmr;

    return tmr;
}


static void add_timer( TIMER_NODE *tmr)
{
    tmr->next=first_timer;
    first_timer=tmr;
}

void unregister_ch_timer( CHAR_DATA *ch )
{
    if (!ch->trig_timer)
    {
        /* doesn't have one */
        return;
    }
    TIMER_NODE *tmr=ch->trig_timer;

    unregister_timer_node(tmr);
    ch->trig_timer=NULL;
    return;
}

void unregister_obj_timer( OBJ_DATA *obj )
{
    if (!obj->otrig_timer)
    {
        /* doesn't have one */
        return;
    }
    TIMER_NODE *tmr=obj->otrig_timer;

    unregister_timer_node(tmr);
    obj->otrig_timer=NULL;
    return;
}

static void free_timer_node( TIMER_NODE *tmr)
{
    free_string(tmr->tag);

    /* debuggg */
    tmr->tag=NULL;
    tmr->next=NULL;

    tmr->deleted=TRUE;
    /* debuggg */
    free_mem(tmr, sizeof(TIMER_NODE));
}

static TIMER_NODE *new_timer_node( void *gobj, void (*func)(), int go_type, int tm_type, int seconds, const char *tag )
{
    TIMER_NODE *new=alloc_mem(sizeof(TIMER_NODE));
    new->next=NULL;
    new->tm_type=tm_type;
    new->game_obj=gobj;
    new->func=func;
    new->go_type=go_type;
    new->current=seconds;
    new->unregistered=FALSE;
    new->deleted=FALSE;
    new->tag=str_dup(tag);
    return new;
}

/* see if everything that should have a running timer does have a running timer */
static void timer_debug()
{
    CHAR_DATA *ch;
    OBJ_DATA *obj;
    AREA_DATA *area;
    //ROOM_INDEX_DATA *room;

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if (!IS_NPC(ch))
            continue;

        if (HAS_TRIGGER(ch, TRIG_TIMER) && !ch->must_extract && !ch->trig_timer)
        {
            bugf("timer_debug: no timer running for mob %d, re-initializing", 
                    ch->pIndexData->vnum ); 
            mprog_timer_init(ch);
        }
    }

    for ( obj=object_list ; obj ; obj=obj->next )
    {
        if (HAS_OTRIG(obj, OTRIG_TIMER) && !obj->must_extract && !obj->otrig_timer)
        {
            bugf("timer_debug: no timer running for obj %d, re-initializing",
                    obj->pIndexData->vnum );
            oprog_timer_init(obj);
        }
    }

    for ( area=area_first ; area ; area=area->next )
    {
        if (HAS_ATRIG(area, ATRIG_TIMER) && !area->atrig_timer)
        {
            bugf("timer_debug: no timer running for area %s, re-initializing",
                    area->name );
            aprog_timer_init(area);
        }
    }

    /* let's not do rooms for now */
}

/* Should be called every second */
void timer_update()
{
    TIMER_NODE *tmr, *tmr_next, *tmr_prev;
    CHAR_DATA *ch;
    OBJ_DATA *obj;
    AREA_DATA *area;
    ROOM_INDEX_DATA *room;

    tmr_prev=NULL;
    for (tmr=first_timer ; tmr ; tmr=tmr_next)
    {
        tmr_next=tmr->next;

        if (tmr->deleted)
        {
            bugf("Deleted timer in timer list!!!");
            return;
        }

        if ( tmr->unregistered )
        {
            /* it was unregistered since the last update
               we need to kill it cleanly */
            if (tmr == first_timer)
            {
                first_timer=tmr_next;
            }
            else
            {
                tmr_prev->next = tmr_next;
            }
            free_timer_node(tmr);

            continue;
        }
        tmr_prev=tmr;

        tmr->current-=1;
        if (tmr->current <= 0)
        {
            switch(tmr->tm_type)
            {
                case TM_PROG:
                    switch( tmr->go_type )
                    {
                        case GO_TYPE_CH: 
                            ch=(CHAR_DATA *)(tmr->game_obj);
                            if (ch->must_extract)
                                break;

                            if (!valid_CH( ch ) )
                            {
                                /* Shouldn't happen ever */
                                bugf("timer_update: invalid ch");
                                break;
                            }
                            mp_timer_trigger( ch );
                            /* repeating timer, set it up again */
                            if (valid_CH( ch ) && !ch->must_extract)
                            {
                                ch->trig_timer=NULL;
                                mprog_timer_init( ch );
                            }
                            break;

                        case GO_TYPE_OBJ:
                            obj=(OBJ_DATA *)(tmr->game_obj);
                            if (!valid_OBJ( obj ) )
                            {
                                bugf("timer_update: invalid obj");
                                break;
                            }
                            op_timer_trigger( obj );
                            if (valid_OBJ( obj ) && !obj->must_extract)
                            {
                                obj->otrig_timer=NULL;
                                oprog_timer_init( obj );
                            }
                            break;

                        case GO_TYPE_AREA:
                            /* no need for valid check on areas */
                            area=(AREA_DATA *)(tmr->game_obj);
                            ap_timer_trigger( area );
                            area->atrig_timer=NULL;
                            aprog_timer_init( area );
                            break;

                        case GO_TYPE_ROOM:
                            room=(ROOM_INDEX_DATA *)(tmr->game_obj);
                            if (!valid_ROOM( room ))
                            {
                                bugf("timer_update: invalid room");
                                break;
                            }
                            rp_timer_trigger( room );
                            room->rtrig_timer=NULL;
                            rprog_timer_init( room );
                            break;

                        default:
                            bugf("Invalid type in timer update: %d.", tmr->go_type);
                            unregister_timer_node(tmr);
                            return;
                    }
                    break;
                case TM_LUAFUNC:
                    run_delayed_function(tmr);
                    break;
                case TM_CFUNC:
                    tmr->func();
                    break;
                default:
                    bugf("Invalid timer type: %d", tmr->tm_type);
                    unregister_timer_node(tmr);
                    return;
            }
            /* it fired, make sure it's unregistered */
            if (!tmr->unregistered)
                unregister_timer_node(tmr);
        }
    }

    /* DEBUGGGGGG */
    timer_debug();
    /* DEBUGGGGGG */
}

char * print_timer_list()
{
    static char buf[MSL*4];
    TIMER_NODE *tmr;
    strcpy(buf, "");
    int i=1;
    int unregcnt=0;
    for ( tmr=first_timer; tmr; tmr=tmr->next )
    {
        if ( tmr->unregistered )
        {
            unregcnt++;
            continue;
        }
        else if ( tmr->tm_type==TM_PROG && !valid_UD( tmr->game_obj ) )
        {
            bugf("Invalid game_obj in print_timer_list.");
            continue;
        } 
        sprintf(buf, "%s\n\r%d %s %d %s", buf, i,
            tmr->tm_type == TM_LUAFUNC ? "luafunc" :
            tmr->tm_type == TM_CFUNC ? "cfunc" :
            tmr->go_type == GO_TYPE_CH ? ((CHAR_DATA *)(tmr->game_obj))->name :
            tmr->go_type == GO_TYPE_OBJ ? ((OBJ_DATA *)(tmr->game_obj))->name :
            tmr->go_type == GO_TYPE_AREA ? ((AREA_DATA *)(tmr->game_obj))->name :
            tmr->go_type == GO_TYPE_ROOM ? ((ROOM_INDEX_DATA *)(tmr->game_obj))->name :
            "unknown",
            tmr->current,
            tmr->tag ? tmr->tag : "none");
        i++;
    }
    strcat( buf, "\n\r");
    sprintf(buf, "%s\n\rUnregistered timers (pending removal): %d\n\r", buf, unregcnt);
    return buf;

}
