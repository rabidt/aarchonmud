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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"
#include "buffer_util.h"

/* Command procedures needed */
DECLARE_DO_FUN(do_open  );
DECLARE_DO_FUN(do_say   );

//void do_hunt_relic( CHAR_DATA *ch );

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
#define HASH_KEY(ht,key) ( (((unsigned int)(key)) * 17) % (unsigned int)((ht)->table_size))



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



void init_hash_table(struct hash_header *ht,int rec_size,size_t table_size)
{
    ht->rec_size  = rec_size;
    ht->table_size= table_size;
    ht->buckets   = (void*)calloc(sizeof(struct hash_link**),table_size);
    ht->keylist   = (void*)malloc(sizeof(ht->keylist) * (size_t)(ht->klistsize=128));
    ht->klistlen  = 0;
}

void destroy_hash_table(struct hash_header *ht,void (*gman)(void *))
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
            (unsigned)(ht->klistsize*=2));
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

void *hash_find(struct hash_header *ht,int key)
{
    struct hash_link *scan;
    
    scan = ht->buckets[HASH_KEY(ht,key)];
    
    while(scan && scan->key!=key)
        scan = scan->next;
    
    return scan ? scan->data : NULL;
}

int hash_enter(struct hash_header *ht,int key,void *data)
{
    void *temp;
    
    temp = hash_find(ht,key);
    if(temp) return 0;
    
    _hash_enter(ht,key,data);
    return 1;
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
            bcopy((char *)ht->keylist+i+1,(char *)ht->keylist+i,(size_t)(ht->klistlen-i)
                    *sizeof(*ht->keylist));
            ht->klistlen--;
        }

        return temp;
    }

    return NULL;
}

#if 0
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

#endif



int exit_ok( EXIT_DATA *pexit )
{
    ROOM_INDEX_DATA *to_room;
    
    if ( ( pexit == NULL )
        ||   ( to_room = pexit->u1.to_room ) == NULL )
        return 0;
    
    return 1;
}

void donothing(void *data)
{
    return;
}

// encode distance + initial direction in a single int
#define DD(dir, dist) ((dir) + ((dist) << 4))
#define DD_DIR(dd) ((dd) & 15)
#define DD_DIST(dd) ((dd) >> 4)
// maximal number of exists to search from a room, including portals (easier than list)
#define MAX_BRANCH (MAX_DIR+10)

// return direction indicating first edge of shortest path
int find_path( int in_room_vnum, int out_room_vnum, bool in_zone, int max_depth, int *distance )
{
    struct room_q *tmp_q, *q_head, *q_tail;
    struct hash_header x_room;
    int i, tmp_room;
    bool thru_doors = TRUE;
    ROOM_INDEX_DATA *herep, *startp;
    EXIT_DATA *exitp;
    // destinations from current room
    int branch_rooms[MAX_BRANCH];
    
    startp = get_room_index( in_room_vnum );
    
    init_hash_table( &x_room, sizeof(int), 2048 );
    hash_enter( &x_room, in_room_vnum, (void *) DD(0,0) );
    
    /* initialize queue */
    q_head = (struct room_q *) malloc(sizeof(struct room_q));
    q_tail = q_head;
    q_tail->room_nr = in_room_vnum;
    q_tail->next_q = 0;
    
    while(q_head)
    {
        herep = get_room_index( q_head->room_nr );
        /* for each room test all directions */
        if ( herep && (herep->area == startp->area || !in_zone) )
        {
            // direction and distance of successors
            // direction is that of ancestor (unless first step, then updated below)
            int dir_dist = (int)(intptr_t)hash_find(&x_room, q_head->room_nr);
            int dist = DD_DIST(dir_dist) + 1;
            int dir = DD_DIR(dir_dist);
            // regular exits
            for( i = 0; i < MAX_DIR; i++ )
            {
                exitp = herep->exit[i];
                if ( exit_ok(exitp) && ( thru_doors ? GO_OK_SMARTER : GO_OK ) )
                    branch_rooms[i] = herep->exit[i]->u1.to_room->vnum;
                else
                    branch_rooms[i] = 0;
            }
            // portals
            OBJ_DATA *portal = herep->contents;
            while ( i < MAX_BRANCH )
            {
                // no objects left => clear remainder of array
                if ( portal == NULL )
                {
                    branch_rooms[i++] = 0;
                    continue;
                }
                if ( portal->item_type == ITEM_PORTAL && portal->value[3] > 0 )
                    branch_rooms[i++] = portal->value[3];
                portal = portal->next_content;
            }
            // handle all "exits" that we found
            for( i = 0; i < MAX_BRANCH; i++ )
            {
                if ( branch_rooms[i] > 0 )
                {
                    /* next room */
                    tmp_room = branch_rooms[i];
                    // don't use ancestor direction if it's the first step we're taking
                    if ( dist == 1 )
                        dir = (i < MAX_DIR ? i : DIR_PORTAL);

                    if( tmp_room != out_room_vnum )
                    {
                        // shall we add room to queue?
                        if ( !hash_find(&x_room, tmp_room) && dist < max_depth )
                        {
                            /* mark room as visted and put on queue */
                            tmp_q = (struct room_q *) malloc(sizeof(struct room_q));
                            tmp_q->room_nr = tmp_room;
                            tmp_q->next_q = 0;
                            q_tail->next_q = tmp_q;
                            q_tail = tmp_q;
                            //logpf("Hunting: #%d -> #%d (%d), dir=%d, dist=%d", q_head->room_nr, tmp_room, i, dir, dist);
                            hash_enter( &x_room, tmp_room, (void *)(intptr_t) DD(dir,dist) );
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
                        if (x_room.buckets)
                        {
                            /* junk left over from a previous track */
                            destroy_hash_table(&x_room, donothing);
                        }
                        // return of distance is optional
                        if ( distance != NULL )
                            *distance = dist;
                        return dir;
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

// formulas used for max distance & skill reduction by hunt, pathfinding, etc.
int hunt_max_distance(int skill, int mastery)
{
    if ( mastery >= 2 )
        return skill / 2;
    else
        return skill / 3;
}

int hunt_fail_chance(int skill, int distance, int mastery)
{
    if ( mastery >= 1 )
        return (100-skill) + distance;
    else
        return (100-skill) + 2 * distance;
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

DEF_DO_FUN(do_hunt)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int direction, chance, skill;
    
    one_argument( argument, arg );
    
    /*
    if ( !strcmp(arg, "relic") )
    {
	do_hunt_relic( ch );
	return;
    }
    */

    if ( (skill = get_skill(ch, gsn_hunt)) == 0 )
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
    
    /* invisible victims leave tracks as well */
    ignore_invisible = TRUE;

    victim = get_char_world( ch, arg );
    
    if ( victim == NULL || !can_locate(ch, victim) )
    {
        send_to_char("No-one around by that name.\n\r", ch );
        return;
    }

    if ( !is_room_ingame(victim->in_room) && is_room_ingame(ch->in_room))
    {
        send_to_char("No-one around by that name.\n\r", ch);
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
    int cost = 10 - mastery_bonus(ch, gsn_stalk, 6, 9);
    if( ch->move >= cost )
        ch->move -= cost;
    else
    {
        send_to_char( "You're too exhausted to hunt anyone!\n\r", ch );
        return;
    }

    if ( !per_chance(get_skill(ch, gsn_stalk)) )
    {
        WAIT_STATE(ch, skill_table[gsn_hunt].beats);
        check_improve(ch, gsn_stalk, FALSE, 4);
    }
    else
        check_improve(ch, gsn_stalk, TRUE, 4);
    
    /* let's not make the hunter visible */
    ignore_invisible = FALSE;
    act( "$n carefully sniffs the air.", ch, NULL, NULL, TO_ROOM );
    ignore_invisible = TRUE;

    if ( ch->in_room == victim->in_room )
    {
        act( "$N is here!", ch, NULL, victim, TO_CHAR );
        return;
    }

    // allow hunting of any target, overwriting elude etc.
    bool stalk_any = per_chance(get_skill_overflow(ch, gsn_stalk));
    
    if ( !stalk_any && IS_SET(victim->act, ACT_NO_TRACK) )
    {
        act( "$N has left no discernable trail.", ch, NULL, victim, TO_CHAR );     
        return;
    }
    
    if ( !stalk_any && IS_AFFECTED(victim, AFF_NO_TRACE) && is_wilderness(ch->in_room->sector_type) )
    {
	act( "$N has left no trace of $S passing.", ch, NULL, victim, TO_CHAR );     
	return;
    }
    
    int mastery = get_mastery(ch, gsn_hunt);
    int max_depth = IS_IMMORTAL(ch) ? 100 : hunt_max_distance(skill, mastery);
    int distance = 0;
    direction = find_path( ch->in_room->vnum, victim->in_room->vnum, FALSE, max_depth, &distance );
    
    if( direction == -1 )
    {
        act( "You couldn't find a path to $N from here.",
            ch, NULL, victim, TO_CHAR );
        return;
    }
    
    if ( direction == DIR_PORTAL )
    {
        send_to_char( "The trail suddenly ends here.\n\r", ch );
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
    int fail_chance = hunt_fail_chance(skill, distance, mastery);
    int reroll_chance = (100 - fail_chance) * get_skill_overflow(ch, gsn_hunt) / 100;
    if ( !IS_IMMORTAL(ch) && per_chance(fail_chance) && !per_chance(reroll_chance) )
    {
        do
        {
            direction = number_door();
        }
        while( ( ch->in_room->exit[direction] == NULL )
            || ( ch->in_room->exit[direction]->u1.to_room == NULL) );
    }
    
    if ( !stalk_any && (chance=get_skill(victim, gsn_elude)) > 0 )
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
                check_improve(victim, gsn_elude, TRUE, 3);
            }
        }
        else check_improve(victim, gsn_elude, FALSE, 3);
    }
    
   /*
    * Display the results of the search.
    */
    snprintf( buf, sizeof(buf), "$N is %s from here.", dir_name[direction] );
    check_improve( ch, gsn_hunt, TRUE, 3 );
    act( buf, ch, NULL, victim, TO_CHAR );
    
    return;
}

/*
void do_hunt_relic( CHAR_DATA *ch )
{
    RELIGION_DATA *rel;
    ROOM_INDEX_DATA *room;
    char buf[MAX_STRING_LENGTH];
    int direction;

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

    int max_depth = (IS_IMMORTAL(ch) || is_high_priest(ch)) ? 100 : ch->level / 2;
    direction = find_path( ch->in_room->vnum, room->vnum, FALSE, max_depth, NULL );
    
    if( direction == -1 )
    {
        act( "You couldn't find a path to $p from here.",
            ch, rel->relic_obj, NULL, TO_CHAR );
        return;
    }
    
    if ( direction == DIR_PORTAL )
    {
        send_to_char( "The trail suddenly ends here.\n\r", ch );
        return;
    }
    
    if( direction < 0 || direction > 9 )
    {
        send_to_char( "Hmm... Something seems to be wrong.\n\r", ch );
        return;
    }
    
    // Display the results of the search.
    snprintf( buf, sizeof(buf), "$p is %s from here.", dir_name[direction] );
    act( buf, ch, rel->relic_obj, NULL, TO_CHAR );
}
*/

/* 'hunts' for a room */
DEF_DO_FUN(do_scout)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *target;
    int direction;
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
    
    target = get_room_area( ch->in_room->area, arg );
    if ( !target )
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
    if ( ch->move > 9 )
        ch->move -= 10;
    else
    {
        send_to_char( "You're too exhausted to scout any further!\n\r", ch );
        return;
    }
    
    act( "$n carefully examines the landscape.", ch, NULL, NULL, TO_ROOM );
    WAIT_STATE( ch, skill_table[sn].beats );
    int mastery = get_mastery(ch, sn);
    int max_depth = IS_IMMORTAL(ch) ? 100 : hunt_max_distance(skill, mastery);
    int distance = 0;
    direction = find_path( ch->in_room->vnum, target->vnum, FALSE, max_depth, &distance );
    
    if( direction == -1 )
    {
	snprintf( buf, sizeof(buf), "You couldn't find a path to %s from here.\n\r",
		 target->name );
	send_to_char( buf, ch );
        return;
    }
    
    if ( direction == DIR_PORTAL )
    {
        send_to_char( "The trail suddenly ends here.\n\r", ch );
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
    if ( !IS_IMMORTAL(ch) && per_chance(hunt_fail_chance(skill, distance, mastery)) )
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
    snprintf( buf, sizeof(buf), "%s is %s from here.\n\r", target->name, dir_name[direction] );
    send_to_char( buf, ch );
    check_improve( ch, sn, TRUE, 3 );
    
    return;
}

void hunt_victim( CHAR_DATA *ch )
{
    int       dir, chance;
    bool      found = FALSE;
    CHAR_DATA *victim;

    if( ch == NULL || ch->hunting == NULL || !IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM))
        return;
    
   /*
    * Make sure the victim still exists.
    */
    for( victim = char_list; victim != NULL; victim = victim->next )
    {
        if ( victim->must_extract || victim->in_room == NULL )
            continue;
        if ( !str_cmp(ch->hunting, victim->name) )
        {
            found = TRUE;
            break;
        }
    }

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
	    act( "$n sniffs the air, then gets a red glare in $s eyes.",
		 ch, NULL, victim, TO_ROOM );
	    act( "You sniff the air and notice that $N must be nearby.",
		 ch, NULL, victim, TO_CHAR );
	    return;
	}

        if ( IS_SET(ch->form, FORM_SENTIENT) )
        {
            act( "$n glares at $N and says, 'Ye shall DIE!'", ch, NULL, victim, TO_NOTVICT );
            act( "$n glares at you and says, 'Ye shall DIE!'", ch, NULL, victim, TO_VICT );
            act( "You glare at $N and say, 'Ye shall DIE!", ch, NULL, victim, TO_CHAR);
        }
        else
        {
            act( "$n growls at $N and attacks!'", ch, NULL, victim, TO_NOTVICT );
            act( "$n growls at you and attacks!'", ch, NULL, victim, TO_VICT );
            act( "You growl at $N and attack!", ch, NULL, victim, TO_CHAR);
        }
	multi_hit( ch, victim, TYPE_UNDEFINED );
	stop_hunting(ch);
	return;
    }

    if (IS_SET(ch->act, ACT_SENTINEL))
	return;
    
    WAIT_STATE( ch, skill_table[gsn_hunt].beats );

    /* mobs hunt in area -> saves cpu time and prevents wandering off */
    dir = find_path( ch->in_room->vnum, victim->in_room->vnum, TRUE, UMAX(5, ch->level/5), NULL);
    
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
                check_improve(victim, gsn_elude, TRUE, 3);
            }
        }
        else check_improve(victim, gsn_elude, FALSE, 3);
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
	do_open( ch, dir_name[dir] );
	return;
    }
        
    move_char( ch, dir, FALSE );
    return;
}

DEF_DO_FUN(do_stalk)
{
    if ( IS_NPC(ch) )
        return;
    
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
    
    if ( !strcmp(argument, "all") )
        send_to_char("You prepare to ambush anyone who comes along.\n\r", ch);
    else if ( ch->fighting == NULL )
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

static int get_assist_count( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA *helper;
    int assist_percent = 0;
    
    if ( victim->in_room == NULL || ch->in_room == NULL || victim->in_room != ch->in_room )
        return 0;
    
    for ( helper = victim->in_room->people; helper; helper = helper->next_in_room )
    {
        if ( helper == ch || helper == victim || helper->fighting == NULL
            || !IS_NPC(helper) || IS_SET(helper->act, ACT_NOEXP)
            || !is_same_group(helper->fighting, victim) )
            continue;
        // we have an NPC assisting the victim - but may not count as 100%, depending on level
        int assist_value = 100 + 5 * (helper->level - ch->level);
        assist_percent += UMAX(0, assist_value);
    }
    return rand_div(assist_percent, 100);
}

void remember_attack(CHAR_DATA *ch, CHAR_DATA *victim, int dam)
{
	MEM_DATA *m, *m_last, *m_new;

    // players are responsible for their charmies
    victim = get_local_leader(victim);
    
	if (!IS_NPC(ch) || IS_NPC(victim) || dam<1)
		return;

	m_last = NULL;
	for (m=ch->aggressors; m; m = m->next)
	{
		m_last = m;
		if (m->id == victim->id)
		{
			m->reaction += dam;
            m->ally_reaction += dam * get_assist_count(ch, victim);
			return;
		}
	}

	m_new = new_mem_data();
	m_new->id = victim->id;
	m_new->reaction = dam;
    m_new->ally_reaction = dam * get_assist_count(ch, victim);

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

int get_ally_reaction( CHAR_DATA *ch, CHAR_DATA *victim )
{
    MEM_DATA *m;

    for ( m = ch->aggressors; m; m=m->next )
        if ( m->id == victim->id )
            return m->ally_reaction;

    return 0;
}
