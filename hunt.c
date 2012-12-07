/*
 * SillyMUD Distribution V1.1b             (c) 1993 SillyMUD Developement
 * See license.doc for distribution terms.   SillyMUD is based on DIKUMUD
 *
 * Modifications by Rip in attempt to port to merc 2.1
 */

/*
 * Modified by Turtle for Merc22 (07-Nov-94)
 *
 * I got this one from ftp.atinc.com:/pub/mud/outgoing/track.merc21.tar.gz.
 * It cointained 5 files: README, hash.c, hash.h, skills.c, and skills.h.
 * I combined the *.c and *.h files in this hunt.c, which should compile
 * without any warnings or errors.
 */

/*
 * Some systems don't have bcopy and bzero functions in their linked libraries.
 * If compilation fails due to missing of either of these functions,
 * define NO_BCOPY or NO_BZERO accordingly.                 -- Turtle 31-Jan-95
#define NO_BCOPY
#define NO_BZERO
 */

/*
 * Further mods by Rimbol for port to ROM 2.4b4 (24-Jun-97)
 */

#if defined(WIN32)
#define NO_BCOPY
#define NO_BZERO
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"
#include "buffer_util.h"
#include "religion.h"

/* Command procedures needed */
DECLARE_DO_FUN(do_open  );
DECLARE_DO_FUN(do_say   );

void bcopy(register char *s1,register char *s2,int len);
void bzero(register char *sp,int len);
void do_hunt_relic( CHAR_DATA *ch );



struct hash_link
{
    int           key;
    struct hash_link  *next;
    void          *data;
};

struct hash_header
{
    int           rec_size;
    int           table_size;
    int           *keylist, klistsize, klistlen; /* this is really lame, AMAZINGLY lame */
    struct hash_link  **buckets;
};

#define WORLD_SIZE  30000
#define HASH_KEY(ht,key)((((unsigned int)(key))*17)%(ht)->table_size)



struct hunting_data
{
    char          *name;
    struct char_data  **victim;
};

struct room_q
{
    int       room_nr;
    struct room_q *next_q;
};

struct nodes
{
    int   visited;
    int   ancestor;
};

#define IS_DIR      (get_room_index(q_head->room_nr)->exit[i])
#define GO_OK       (!IS_SET( IS_DIR->exit_info, EX_CLOSED ))
#define GO_OK_SMARTER   1



#if defined( NO_BCOPY )
void bcopy(register char *s1,register char *s2,int len)
{
    while( len-- ) *(s2++) = *(s1++);
}
#endif

#if defined( NO_BZERO )
void bzero(register char *sp,int len)
{
    while( len-- ) *(sp++) = '\0';
}
#endif



void init_hash_table(struct hash_header *ht,int rec_size,int table_size)
{
    ht->rec_size  = rec_size;
    ht->table_size= table_size;
    ht->buckets   = (void*)calloc(sizeof(struct hash_link**),table_size);
    ht->keylist   = (void*)malloc(sizeof(ht->keylist)*(ht->klistsize=128));
    ht->klistlen  = 0;
}

void init_world(ROOM_INDEX_DATA *room_db[])
{
    /* zero out the world */
    bzero((char *)room_db,sizeof(ROOM_INDEX_DATA *)*WORLD_SIZE);
}

void destroy_hash_table(struct hash_header *ht,void (*gman)())
{
    int           i;
    struct hash_link  *scan,*temp;
    
    for(i=0;i<ht->table_size;i++)
        for(scan=ht->buckets[i];scan;)
        {
            temp = scan->next;
            (*gman)(scan->data);
            free(scan);
            scan = temp;
        }
        free(ht->buckets);
        free(ht->keylist);
}

void _hash_enter(struct hash_header *ht,int key,void *data)
{
    /* precondition: there is no entry for <key> yet */
    struct hash_link  *temp;
    int           i;
    
    temp      = (struct hash_link *)malloc(sizeof(struct hash_link));
    temp->key = key;
    temp->next    = ht->buckets[HASH_KEY(ht,key)];
    temp->data    = data;
    ht->buckets[HASH_KEY(ht,key)] = temp;
    if(ht->klistlen>=ht->klistsize)
    {
        ht->keylist = (void*)realloc(ht->keylist,sizeof(*ht->keylist)*
            (ht->klistsize*=2));
    }
    for(i=ht->klistlen;i>=0;i--)
    {
        if(ht->keylist[i-1]<key)
        {
            ht->keylist[i] = key;
            break;
        }
        ht->keylist[i] = ht->keylist[i-1];
    }
    ht->klistlen++;
}

ROOM_INDEX_DATA *room_find(ROOM_INDEX_DATA *room_db[],int key)
{
    return((key<WORLD_SIZE&&key>-1)?room_db[key]:0);
}

void *hash_find(struct hash_header *ht,int key)
{
    struct hash_link *scan;
    
    scan = ht->buckets[HASH_KEY(ht,key)];
    
    while(scan && scan->key!=key)
        scan = scan->next;
    
    return scan ? scan->data : NULL;
}

int room_enter(ROOM_INDEX_DATA *rb[],int key,ROOM_INDEX_DATA *rm)
{
    ROOM_INDEX_DATA *temp;
    
    temp = room_find(rb,key);
    if(temp) return(0);
    
    rb[key] = rm;
    return(1);
}

int hash_enter(struct hash_header *ht,int key,void *data)
{
    void *temp;
    
    temp = hash_find(ht,key);
    if(temp) return 0;
    
    _hash_enter(ht,key,data);
    return 1;
}

ROOM_INDEX_DATA *room_find_or_create(ROOM_INDEX_DATA *rb[],int key)
{
    ROOM_INDEX_DATA *rv;
    
    rv = room_find(rb,key);
    if(rv) return rv;
    
    rv = (ROOM_INDEX_DATA *)malloc(sizeof(ROOM_INDEX_DATA));
    rb[key] = rv;
    
    return rv;
}

void *hash_find_or_create(struct hash_header *ht,int key)
{
    void *rval;
    
    rval = hash_find(ht, key);
    if(rval) return rval;
    
    rval = (void*)malloc(ht->rec_size);
    _hash_enter(ht,key,rval);
    
    return rval;
}

int room_remove(ROOM_INDEX_DATA *rb[],int key)
{
    ROOM_INDEX_DATA *tmp;
    
    tmp = room_find(rb,key);
    if(tmp)
    {
        rb[key] = 0;
        free(tmp);
    }
    return(0);
}

void *hash_remove(struct hash_header *ht,int key)
{
    struct hash_link **scan;
    
    scan = ht->buckets+HASH_KEY(ht,key);
    
    while(*scan && (*scan)->key!=key)
        scan = &(*scan)->next;
    
    if(*scan)
    {
        int       i;
        struct hash_link  *temp, *aux;
        
        temp  = (*scan)->data;
        aux   = *scan;
        *scan = aux->next;
        free(aux);
        
        for(i=0;i<ht->klistlen;i++)
            if(ht->keylist[i]==key)
                break;
            
            if(i<ht->klistlen)
            {
                bcopy((char *)ht->keylist+i+1,(char *)ht->keylist+i,(ht->klistlen-i)
                    *sizeof(*ht->keylist));
                ht->klistlen--;
            }
            
            return temp;
    }
    
    return NULL;
}

void room_iterate(ROOM_INDEX_DATA *rb[],void (*func)(),void *cdata)
{
    register int i;
    
    for(i=0;i<WORLD_SIZE;i++)
    {
        ROOM_INDEX_DATA *temp;
        
        temp = room_find(rb,i);
        if(temp) (*func)(i,temp,cdata);
    }
}

void hash_iterate(struct hash_header *ht,void (*func)(),void *cdata)
{
    int i;
    
    for(i=0;i<ht->klistlen;i++)
    {
        void      *temp;
        register int  key;
        
        key = ht->keylist[i];
        temp = hash_find(ht,key);
        (*func)(key,temp,cdata);
        if(ht->keylist[i]!=key) /* They must have deleted this room */
            i--;              /* Hit this slot again. */
    }
}



int exit_ok( EXIT_DATA *pexit )
{
    ROOM_INDEX_DATA *to_room;
    
    if ( ( pexit == NULL )
        ||   ( to_room = pexit->u1.to_room ) == NULL )
        return 0;
    
    return 1;
}

void donothing()
{
    return;
}

int find_path( int in_room_vnum, int out_room_vnum, CHAR_DATA *ch, 
	       int depth, int in_zone )
{
    struct room_q     *tmp_q, *q_head, *q_tail;
    struct hash_header    x_room;
    int           i, tmp_room, count=0, thru_doors;
    ROOM_INDEX_DATA   *herep;
    ROOM_INDEX_DATA   *startp;
    EXIT_DATA     *exitp;
    
    if ( depth < 0 )
    {
        thru_doors = TRUE;
        depth = -depth;
    }
    else
    {
        thru_doors = FALSE;
    }
    
    startp = get_room_index( in_room_vnum );
    
    init_hash_table( &x_room, sizeof(int), 2048 );
    hash_enter( &x_room, in_room_vnum, (void *) - 1 );
    
    /* initialize queue */
    q_head = (struct room_q *) malloc(sizeof(struct room_q));
    q_tail = q_head;
    q_tail->room_nr = in_room_vnum;
    q_tail->next_q = 0;
    
    while(q_head)
    {
        herep = get_room_index( q_head->room_nr );
        /* for each room test all directions */
        if( herep->area == startp->area || !in_zone )
        {
        /* only look in this zone...
	   saves cpu time and  makes world safer for players  */
            for( i = 0; i < MAX_DIR; i++ )
            {
                exitp = herep->exit[i];
                if( exit_ok(exitp) && ( thru_doors ? GO_OK_SMARTER : GO_OK ) )
                {
                    /* next room */
                    tmp_room = herep->exit[i]->u1.to_room->vnum;
                    if( tmp_room != out_room_vnum )
                    {
                    /* shall we add room to queue ?
                        count determines total breadth and depth */
                        if( !hash_find( &x_room, tmp_room )
                            && ( count < depth ) )
                            /* && !IS_SET( RM_FLAGS(tmp_room), DEATH ) ) */
                        {
                            count++;
                            /* mark room as visted and put on queue */
                            
                            tmp_q = (struct room_q *)
                                malloc(sizeof(struct room_q));
                            tmp_q->room_nr = tmp_room;
                            tmp_q->next_q = 0;
                            q_tail->next_q = tmp_q;
                            q_tail = tmp_q;
                            
                            /* ancestor for first layer is the direction */
                            hash_enter( &x_room, tmp_room,
                                ((int)hash_find(&x_room,q_head->room_nr)
                                == -1) ? (void*)(i+1)
                                : hash_find(&x_room,q_head->room_nr));
                        }
                    }
                    else
                    {
                        /* have reached our goal so free queue */
                        tmp_room = q_head->room_nr;
                        for(;q_head;q_head = tmp_q)
                        {
                            tmp_q = q_head->next_q;
                            free(q_head);
                        }
                        /* return direction if first layer */
                        if ((int)hash_find(&x_room,tmp_room)==-1)
                        {
                            if (x_room.buckets)
                            {
                                /* junk left over from a previous track */
                                destroy_hash_table(&x_room, donothing);
                            }
                            return(i);
                        }
                        else
                        {
                            /* else return the ancestor */
                            int i;
                            
                            i = (int)hash_find(&x_room,tmp_room);
                            if (x_room.buckets)
                            {
                                /* junk left over from a previous track */
                                destroy_hash_table(&x_room, donothing);
                            }
                            return( -1+i);
                        }
                    }
                }
            }
        }
        
        /* free queue head and point to next entry */
        tmp_q = q_head->next_q;
        free(q_head);
        q_head = tmp_q;
    }
    
    /* couldn't find path */
    if( x_room.buckets )
    {
        /* junk left over from a previous track */
        destroy_hash_table( &x_room, donothing );
    }
    return -1;
}

bool is_wilderness( int sector )
{
    return sector != SECT_CITY
	&& sector != SECT_INSIDE;

    /*
    switch ( sector )
    {
    case SECT_FIELD:
    case SECT_FOREST:
    case SECT_HILLS:
    case SECT_MOUNTAIN:
	return TRUE;
    default:
	return FALSE;
    }
    */
}

void do_hunt( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int direction, chance;
    bool fArea;
    
    one_argument( argument, arg );
    
    if ( !strcmp(arg, "relic") )
    {
	do_hunt_relic( ch );
	return;
    }

    if ( !IS_NPC(ch) && get_skill(ch, gsn_hunt)==0 )
    {
        send_to_char("You wouldn't know how to track.\n\r", ch);
        return;
    }
    
    if (IS_TAG(ch))
    {
        send_to_char( "You can't hunt during Freeze Tag!\n\r", ch );
        return;
    }

    if ( arg[0] == '\0' )
    {
        send_to_char( "Whom are you trying to hunt?\n\r", ch );
        return;
    }
    
    /* only imps can hunt to different areas */
    fArea = ( get_trust(ch) < ARCHON );
    
    /* invisible victims leave tracks as well */
    ignore_invisible = TRUE;

    if ( fArea )
        victim = get_char_area( ch, arg );
    else
        victim = get_char_world( ch, arg );
    
    if ( victim == NULL || !can_locate(ch, victim) )
    {
        send_to_char("No-one around by that name.\n\r", ch );
        return;
    }
    
    if ( IS_NPC(victim) && IS_SET(victim->act, ACT_OBJ) )
    {
	send_to_char( "You can't hunt objects!\n\r", ch );
	return;
    }

   /*
    * Deduct some movement.
    */
    if( ch->move > 2 )
        ch->move -= 3;
    else
    {
        send_to_char( "You're too exhausted to hunt anyone!\n\r", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_hunt].beats );
    
    /* let's not make the hunter visible */
    ignore_invisible = FALSE;
    act( "$n carefully sniffs the air.", ch, NULL, NULL, TO_ROOM );
    ignore_invisible = TRUE;

    if ( ch->in_room == victim->in_room )
    {
        act( "$N is here!", ch, NULL, victim, TO_CHAR );
        return;
    }
    
    if ( IS_SET(victim->act, ACT_NO_TRACK) )
    {
        act( "$N has left no discernable trail.", ch, NULL, victim, TO_CHAR );     
        return;
    }
    
    if ( IS_AFFECTED(victim, AFF_NO_TRACE)
	 && is_wilderness(ch->in_room->sector_type) )           
    {
	act( "$N has left no trace of $S passing.", ch, NULL, victim, TO_CHAR );     
	return;
    }
    
    direction = find_path( ch->in_room->vnum, victim->in_room->vnum,
        ch, -40000, fArea );
    
    if( direction == -1 )
    {
        act( "You couldn't find a path to $N from here.",
            ch, NULL, victim, TO_CHAR );
        return;
    }
    
    if( direction < 0 || direction > 9 )
    {
        send_to_char( "Hmm... Something seems to be wrong.\n\r", ch );
        return;
    }
    
   /*
    * Give a random direction if the player misses the die roll.
    */
    if( number_percent () > (IS_NPC(ch) ? 75 : get_skill(ch,gsn_hunt)))
    {
        do
        {
            direction = number_door();
        }
        while( ( ch->in_room->exit[direction] == NULL )
            || ( ch->in_room->exit[direction]->u1.to_room == NULL) );
    }
    
    if ((chance=get_skill(victim, gsn_elude)) != 0)
    {
        if (number_percent() < chance/2)
        {
            do
            {
                direction = number_door();
            }
            while( ( ch->in_room->exit[direction] == NULL )
                || ( ch->in_room->exit[direction]->u1.to_room == NULL) );
            if (number_percent () < (chance/4))
            {
                send_to_char("Someone is hunting you down.\n\r", victim);
                check_improve(victim, gsn_elude, TRUE, 8);
            }
        }
        else check_improve(victim, gsn_elude, FALSE, 8);
    }
    
   /*
    * Display the results of the search.
    */
    sprintf( buf, "$N is %s from here.", dir_name[direction] );
    check_improve( ch, gsn_hunt, TRUE, 3 );
    act( buf, ch, NULL, victim, TO_CHAR );
    
    return;
}

void do_hunt_relic( CHAR_DATA *ch )
{
    RELIGION_DATA *rel;
    ROOM_INDEX_DATA *room;
    char buf[MAX_STRING_LENGTH];
    int direction;
    bool fArea;

    if ( !is_priest(ch) && !IS_IMMORTAL(ch) )
    {
	send_to_char( "Only priests can hunt for relics.\n\r", ch );
	return;
    }

    if ( (rel = get_religion(ch)) == NULL )
    {
	send_to_char( "You're not religious.\n\r", ch );
	return;
    }

    if ( rel->relic_obj == NULL )
    {
	send_to_char( "Your religion has no relic.\n\r", ch );
	return;
    }

    if ( (room = get_obj_room(rel->relic_obj)) == NULL )
    {
	send_to_char( "The relic can't be located.\n\r", ch );
	return;
    }
	
    if ( room == ch->in_room )
    {
	act( "$p is here!", ch, rel->relic_obj, NULL, TO_CHAR );
	return;
    }

    fArea = !IS_IMMORTAL(ch) && !is_high_priest(ch);
    direction = find_path( ch->in_room->vnum, room->vnum,
        ch, -40000, fArea );
    
    if( direction == -1 )
    {
        act( "You couldn't find a path to $p from here.",
            ch, rel->relic_obj, NULL, TO_CHAR );
        return;
    }
    
    if( direction < 0 || direction > 9 )
    {
        send_to_char( "Hmm... Something seems to be wrong.\n\r", ch );
        return;
    }
    
   /*
    * Display the results of the search.
    */
    sprintf( buf, "$p is %s from here.", dir_name[direction] );
    act( buf, ch, rel->relic_obj, NULL, TO_CHAR );
}

/* 'hunts' for a room */
void do_scout( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *target;
    int direction, chance;
    bool fArea;
    int skill, sn;
    
    /*one_argument( argument, arg );*/
    strcpy( arg, argument );
    
    if ( is_wilderness(ch->in_room->sector_type) )
    {
	if ( (skill = get_skill(ch, gsn_pathfind)) > 0 )
	    sn = gsn_pathfind;
	else
	{
	    send_to_char( "You wouldn't know how to scout in the wilderness.\n\r", ch );
	    return;
	}
    }
    else
    {
	if ( (skill = get_skill(ch, gsn_streetwise)) > 0 )
	    sn = gsn_streetwise;
	else
	{
	    send_to_char( "You wouldn't know how to scout in the city.\n\r", ch );
	    return;
	}
    }

    if ( arg[0] == '\0' )
    {
        send_to_char( "Which location are you trying to find?\n\r", ch );
        return;
    }
    
    /* only imps can scout to different areas */
    fArea = ( get_trust(ch) < ARCHON );
    
    if ( fArea )
	target = get_room_area( ch->in_room->area, arg );
    else
	target = get_room_world( arg );

    if ( target == NULL )
    {
        send_to_char( "There's no such location around.\n\r", ch );
        return;
    }
    
    if ( ch->in_room == target )
    {
        send_to_char( "You're already there.\n\r", ch );
        return;
    }
    
    if ( IS_SET(target->room_flags, ROOM_NO_SCOUT) )
    {
        send_to_char( "You have no clue how to get there.\n\r", ch );
        return;
    }

   /*
    * Deduct some movement.
    */
    if ( ch->move > 2 )
        ch->move -= 3;
    else
    {
        send_to_char( "You're too exhausted to scout any further!\n\r", ch );
        return;
    }
    
    act( "$n carefully examines the landscape.", ch, NULL, NULL, TO_ROOM );
    WAIT_STATE( ch, skill_table[sn].beats );
    direction = find_path( ch->in_room->vnum, target->vnum,
        ch, -40000, fArea );
    
    if( direction == -1 )
    {
	sprintf( buf, "You couldn't find a path to %s from here.\n\r",
		 target->name );
	send_to_char( buf, ch );
        return;
    }
    
    if( direction < 0 || direction > 9 )
    {
        send_to_char( "Hmm... Something seems to be wrong.\n\r", ch );
        return;
    }
    
   /*
    * Give a random direction if the player misses the die roll.
    */
    if( number_percent() > skill )
    {
        do
        {
            direction = number_door();
        }
        while( ( ch->in_room->exit[direction] == NULL )
            || ( ch->in_room->exit[direction]->u1.to_room == NULL) );
    }
    
   /*
    * Display the results of the search.
    */
    sprintf( buf, "%s is %s from here.\n\r", target->name, dir_name[direction] );
    send_to_char( buf, ch );
    check_improve( ch, sn, TRUE, 3 );
    
    return;
}

void hunt_victim( CHAR_DATA *ch )
{
    int       dir, chance;
    bool      found;
    CHAR_DATA *tmp;
    CHAR_DATA *victim;

    if( ch == NULL || ch->hunting == NULL || !IS_NPC(ch) )
        return;
    
   /*
    * Make sure the victim still exists.
    */
    for( found = 0, tmp = char_list; tmp; tmp = tmp->next )
        if (!str_cmp(ch->hunting, tmp->name))
	{
            found = 1;
	    break;
	}

    victim = tmp;
        
    ignore_invisible = TRUE;
    if ( !found || !can_see( ch, victim ))
    {
	ignore_invisible = FALSE;
	do_say( ch, "Damn!  My prey is gone!!" );
	stop_hunting(ch);
        return;
    }
    ignore_invisible = FALSE;

    if ( ch->in_room == victim->in_room )
    {
	/* mob might have hunted victim down but now can't see it */
	if ( !can_see(ch, victim) )
	{
	    act( "$n sniffs the air, then get a red glare in $s eyes.",
		 ch, NULL, victim, TO_ROOM );
	    act( "You sniff the air and notice that $N must be nearby.",
		 ch, NULL, victim, TO_CHAR );
	    return;
	}

	act( "$n glares at $N and says, 'Ye shall DIE!'",
	     ch, NULL, victim, TO_NOTVICT );
	act( "$n glares at you and says, 'Ye shall DIE!'",
	     ch, NULL, victim, TO_VICT );
	act( "You glare at $N and say, 'Ye shall DIE!",
	     ch, NULL, victim, TO_CHAR);
	multi_hit( ch, victim, TYPE_UNDEFINED );
	stop_hunting(ch);
	return;
    }

    if (IS_SET(ch->act, ACT_SENTINEL))
	return;
    
    WAIT_STATE( ch, skill_table[gsn_hunt].beats );

    /* mobs hunt in area -> saves cpu time */
    dir = find_path( ch->in_room->vnum, victim->in_room->vnum,
		     ch, -4000, TRUE );
    
    if( dir < 0 || dir > 9 )
    {
	act( "$n says 'Damn!  Lost $M!'", ch, NULL, ch->hunting, TO_ROOM );
	stop_hunting(ch);
	return;
    }

    chance = (ch->level - victim->level)/2 + 75;
        
    if ( IS_AFFECTED(victim, AFF_NO_TRACE)
	 && is_wilderness(ch->in_room->sector_type) )           
    {
	act( "$N has left no trace of $S passing.", ch, NULL, victim, TO_CHAR );     
	return;
    }
    
    if (get_skill(victim, gsn_elude) != 0)
    {
        int eludeskill;
	eludeskill = get_skill(victim, gsn_elude) - chance/2;
        if (number_percent() < eludeskill)
        {
			chance = 0;
            if (number_percent () < (eludeskill/4))
            {
                send_to_char("Someone is hunting you down.\n\r", victim);
                check_improve(victim, gsn_elude, TRUE, 10);
            }
        }
        else check_improve(victim, gsn_elude, FALSE, 10);
    }
    
    /*
     * Give a random direction if the mob misses the die roll.
     */
    if( number_percent () > chance )        /* @ 25% */
    {
	do
        {
	    dir = number_door();
	}
	while( ( ch->in_room->exit[dir] == NULL )
	       || ( ch->in_room->exit[dir]->u1.to_room == NULL ) );
    }
        
        
    if( IS_SET( ch->in_room->exit[dir]->exit_info, EX_CLOSED ) )
    {
	do_open( ch, (char *) dir_name[dir] );
	return;
    }
        
    move_char( ch, dir, FALSE );
    return;
}

void do_stalk( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch)||get_skill(ch, gsn_stalk)==0)
    {
        send_to_char("You wouldnt know how to stalk someone.\n\r",ch);
        return;
    }
    
    if (argument[0]=='\0')
    {
        send_to_char("You stop stalking.\n\r",ch);
        if (ch->hunting)
        {
            free_string(ch->hunting);
            ch->hunting=NULL;
        }
        return;
    }
    
    if (ch->hunting) free_string(ch->hunting);
    ch->hunting = str_dup( argument );
    
    if (ch->fighting == NULL)
	do_hunt(ch, argument);
}


void set_hunting(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (ch->hunting)
		free_string(ch->hunting);

	ch->hunting = str_dup(victim->name);
}

void stop_hunting(CHAR_DATA *ch)
{
	if (ch->hunting)
		free_string(ch->hunting);

	ch->hunting = NULL;
}

void remember_attack(CHAR_DATA *ch, CHAR_DATA *victim, int dam)
{
	MEM_DATA *m, *m_last, *m_new;

	if (!IS_NPC(ch) || IS_NPC(victim) || dam<1)
		return;

	m_last = NULL;
	for (m=ch->aggressors; m; m = m->next)
	{
		m_last = m;
		if (m->id == victim->id)
		{
			m->reaction += dam;
			return;
		}
	}

	m_new = new_mem_data();
	m_new->id = victim->id;
	m_new->reaction = dam;

	if (m_last)
	    m_last->next = m_new;
	else
	    ch->aggressors = m_new;
}

void update_memory(CHAR_DATA *ch)
{
	CHAR_DATA *victim;
	DESCRIPTOR_DATA *d;
	MEM_DATA *m, *m_next, *m_last;

	if (IS_SET(ch->off_flags, OFF_HUNT) && !ch->hunting)
		for (m=ch->aggressors; m && !ch->hunting; m = m->next)
			for (d = descriptor_list; d && !ch->hunting; d = d->next)
				if ((victim=d->character) && !IS_NPC(victim) &&
						(victim->id == m->id) &&
						(victim->in_room->area == ch->in_room->area) &&
						can_see(ch, victim))
					set_hunting(ch, victim);

	if (ch->hit >= ch->max_hit)
		forget_attacks(ch);
	else
	{
		m_last = NULL;
		for (m=ch->aggressors; m; m = m_next)
		{
			m_next = m->next;
			m->reaction -= 1 + ch->level/10;
			if (m->reaction<=0)
			{
				if (m_last)
					m_last->next = m_next;
				else
					ch->aggressors = m_next;

				free_mem_data(m);
			}
			else
				m_last = m;
		}
	}
}

/* forget specific attacker */
void forget_attacker(CHAR_DATA *ch, CHAR_DATA *attacker)
{
    MEM_DATA *m, *m_next;

    if ( !IS_NPC(ch) || IS_NPC(attacker) || ch->aggressors == NULL )
	return;
    
    m = ch->aggressors;
    if ( m->id == attacker->id )
    {
	m_next = m->next;
	free_mem_data(m);
	ch->aggressors = m_next;
	return;
    }

    for ( ; m->next != NULL; m = m->next )
	if (m->next->id == attacker->id)
	{
	    m_next = m->next->next;
	    free_mem_data(m->next);
	    m->next = m_next;
	    return;
	}
}

void forget_attacks(CHAR_DATA *ch)
{
	MEM_DATA *m, *m_next;

	for (m=ch->aggressors; m; m=m_next)
	{
		m_next = m->next;
		free_mem_data(m);
	}
	ch->aggressors = NULL;
}

int check_anger(CHAR_DATA *ch, CHAR_DATA *victim)
{
    MEM_DATA *m;

    if (!IS_NPC(ch) || IS_NPC(victim) || IS_SET(ch->act, ACT_WIMPY))
	return 0;

    for (m=ch->aggressors; m; m=m->next)
	if (m->id == victim->id)
	    return 1;

    return 0;
}

int get_reaction( CHAR_DATA *ch, CHAR_DATA *victim )
{
    MEM_DATA *m;

    for (m=ch->aggressors; m; m=m->next)
	if (m->id == victim->id)
	    return m->reaction;

    return 0;
}
