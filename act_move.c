/***************************************************************************
*  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                         *
*  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
*  Chastain, Michael Quan, and Mitchell Tse.                              *
*                                                                         *
*  In order to use any part of this Merc Diku Mud, you must comply with   *
*  both the original Diku license in 'license.doc' as well the Merc       *
*  license in 'license.txt'.  In particular, you may not remove either of *
*  these copyright notices.                                               *
*                                                                         *
*  Much time and thought has gone into this software and you are          *
*  benefitting.  We hope that you share your changes too.  What goes      *
*  around, comes around.                                                  *
***************************************************************************/

/**************************************************************************
*   ROM 2.4 is copyright 1993-1996 Russ Taylor                            *
*   ROM has been brought to you by the ROM consortium                     *
*       Russ Taylor (rtaylor@efn.org)                                     *
*       Gabrielle Taylor                                                  *
*       Brian Moore (zump@rom.org)                                        *
*   By using this code, you have agreed to follow the terms of the        *
*   ROM license, in the file Rom24/doc/rom.license                        *
***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "tables.h"
#include "songs.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_recall    );
DECLARE_DO_FUN(do_stand     );
bool check_exit_trap_hit( CHAR_DATA *ch, int door, bool step_in );
void check_bleed( CHAR_DATA *ch, int dir );
static bool has_boat( CHAR_DATA *ch );

const char * const dir_name [MAX_DIR] =
{
   "north", "east", "south", "west", "up", "down", "northeast", "southeast", "southwest", "northwest"
};

const char * const dir_abbr [MAX_DIR] =
{
   "n", "e", "s", "w", "u", "d", "ne", "se", "sw", "nw"
};

bool is_direction( const char* argument )
{
    int i;
    for ( i = 0; i < MAX_DIR; i++ )
        if ( !strcmp(argument, dir_name[i]) || !strcmp(argument, dir_abbr[i]) )
            return TRUE;
    return FALSE;
}

const   sh_int  rev_dir     []      =
{
   2, 3, 0, 1, 5, 4, 8, 9, 6, 7
};

const   sh_int  movement_loss   [SECT_MAX]  =
{
   1, 2, 2, 3, 4, 6, 4, 6, 6, 10, 6, 4
};

bool can_move_room( CHAR_DATA *ch, ROOM_INDEX_DATA *to_room, bool show )
{
    if ( !to_room )
        return FALSE;
    
    if ( show && to_room->sector_type == SECT_AIR && !IS_AFFECTED(ch, AFF_FLYING) )
    {
        if ( show )
            send_to_char("You can't fly.\n\r", ch);
        return FALSE;
    }
    
    if ( IS_NPC(ch) && !IS_AFFECTED(ch, AFF_CHARM) && (IS_SET(to_room->room_flags, ROOM_NO_MOB) || IS_SET(to_room->room_flags, ROOM_SAFE)) )
    {
        if ( show )
            send_to_char("NPCs aren't allowed in there.\n\r", ch);
        return FALSE;
    }
    
    if ( IS_SET(to_room->room_flags, ROOM_BOX_ROOM) && (IS_NPC(ch) || !ch->pcdata->storage_boxes) )
    {
        if ( show )
            send_to_char("You have no business in there.\n\r", ch);
        return FALSE;
    }
        
    if ( !IS_NPC(ch) && room_is_private(to_room) && !is_room_owner(ch, to_room) )
    {
        if ( show )
            send_to_char("That room is private right now.\n\r", ch);
        return FALSE;
    }

    if ( !IS_NPC(ch) && area_full(to_room->area) && !(ch->in_room && ch->in_room->area == to_room->area) )
    {
        if ( show )
            send_to_char("That area is full right now.\n\r", ch);
        return FALSE;
    }
    
    // guilds and clan halls
    if ( !IS_NPC(ch) )
    {
        int iClass, iGuild;
        bool is_allowed = FALSE;
        bool is_guild = FALSE;

        for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
            for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)    
                if ( to_room->vnum == class_table[iClass].guild[iGuild] )
                {
                    is_guild = TRUE;
                    if ( iClass == ch->class )
                        is_allowed = TRUE;
                }

        if ( is_guild && !is_allowed )
        {
            if ( show )
                send_to_char("You aren't allowed in there.\n\r", ch);
            return FALSE;
        }
    
        if ( to_room->clan )
        {
            if ( !IS_NPC(ch) && ch->clan != to_room->clan )
            {
                if ( show )
                    printf_to_char(ch, "That area is for clan %s only.\n\r",
                        capitalize(clan_table[to_room->clan].name));
                return FALSE;
            }

            if ( ch->pcdata->clan_rank < to_room->clan_rank )
            {
                if ( show )
                    printf_to_char (ch, "That area is for %ss of clan %s only.\n\r",
                        capitalize(clan_table[to_room->clan].rank_list[to_room->clan_rank].name),
                        capitalize(clan_table[to_room->clan].name));
                return FALSE;
            }
        }
    }

    return TRUE;
}

bool can_move_dir( CHAR_DATA *ch, int dir, bool show )
{
    ROOM_INDEX_DATA *in_room = ch->in_room;
    EXIT_DATA *pexit = dir >= 0 ? in_room->exit[dir] : NULL;
    ROOM_INDEX_DATA *to_room = pexit ? pexit->u1.to_room : NULL;
    
    if ( !in_room 
         || !to_room 
         || !can_see_room(ch, to_room)
         || ( IS_SET( pexit->exit_info, EX_DORMANT ) 
              && !IS_IMMORTAL(ch) ) )
    {
        if ( show )
            send_to_char("Alas, you cannot go that way.\n\r", ch);
        return FALSE;
    }
    
    if ( IS_IMMORTAL(ch) )
        return TRUE;
    
    if ( IS_SET(pexit->exit_info, EX_CLOSED) && (IS_SET(pexit->exit_info, EX_NOPASS) || !IS_AFFECTED(ch, AFF_PASS_DOOR)) )
    {
        if ( show )
            act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
        return FALSE;
    }
    
    if ( !can_move_room(ch, to_room, show) )
        return FALSE;
    
    return TRUE;
}

int get_random_exit( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *in_room = ch->in_room;
    bool can_move[MAX_DIR];
    int exit_nr, dir, count = 0;

    if ( !in_room )
        return -1;
    
    for ( dir = 0; dir < MAX_DIR; dir++ )
        if ( (can_move[dir] = can_move_dir(ch, dir, FALSE)) )
            count++;
    
    if ( !count )
        return -1;
    
    exit_nr = number_range(1, count);
    for ( dir = 0; dir < MAX_DIR; dir++ )
        if ( can_move[dir] )
        {
            if ( --exit_nr == 0 )
                return dir;
        }
    
    // should never reach this point
    bugf("get_random_exit: not found where expected (room %d)", in_room->vnum);
    return -1;
}

/*
* Local functions.
*/
int find_door( CHAR_DATA *ch, const char *arg );
bool    has_key     args( ( CHAR_DATA *ch, int key ) );
bool check_drown args((CHAR_DATA *ch));
bool check_swim( CHAR_DATA *ch, ROOM_INDEX_DATA *to_room );

/* returns the direction the char moved to or -1 if he didn't move */
int move_char( CHAR_DATA *ch, int door, bool follow )
{
    CHAR_DATA *fch, *fch_next;
    ROOM_INDEX_DATA *in_room, *to_room;
    AREA_DATA *from_area;
    EXIT_DATA *pexit;
    char buf[MAX_STRING_LENGTH];
    int chance;
    
    if ( door < 0 || door >= MAX_DIR )
    {
        bug( "Do_move: bad door %d.", door );
        return -1;
    }
    
    if ( ch->in_room == NULL )
    {
        bugf( "move_char: NULL room" );
        return -1;
    }

    in_room = ch->in_room;

    if ( IS_AFFECTED(ch, AFF_INSANE) && number_bits(1) )
        door = get_random_exit(ch);
    
    pexit = door >= 0 ? in_room->exit[door] : NULL;
    to_room = pexit ? pexit->u1.to_room : NULL;
    
   /*
    * Exit trigger, if activated, bail out. Only PCs are triggered.
    */
    if ( !IS_NPC(ch) )
    {
        /* exit trigger might trans char to target room */
        if ( mp_exit_trigger(ch, door) )
            return ch->in_room == to_room ? door : -1;
        if ( !rp_move_trigger(ch, door) )
            return ch->in_room == to_room ? door : -1;
        if ( !rp_exit_trigger(ch) )
            return ch->in_room == to_room ? door : -1;
        if ( to_room && !ap_rexit_trigger(ch) )
            return ch->in_room == to_room ? door : -1;
        if ( to_room && !ap_exit_trigger(ch, to_room->area) )
            return ch->in_room == to_room ? door : -1;
        if ( to_room && !op_move_trigger(ch) )
            return ch->in_room == to_room ? door : -1;
    }

    if ( !can_move_dir(ch, door, TRUE) )
        return -1;
        
    if ( !IS_NPC(ch) && IS_SET(ch->pcdata->tag_flags, TAG_FROZEN) && IS_TAG(ch) )
    {
        send_to_char( "You've been frozen, you can't move!\n\r", ch );
        return -1;
    }

    if ( IS_SET(ch->penalty, PENALTY_JAIL) )
    {
        send_to_char( "You are chained to the floor.\n\r", ch );
        return -1;
    }
        
    if ( IS_AFFECTED(ch, AFF_ROOTS) )
    {
        send_to_char( "Your roots prevent you from moving.\n\r", ch );
        return -1;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL && in_room == ch->master->in_room )
    {
        send_to_char( "What?  And leave your beloved master?\n\r", ch );
        return -1;
    }

    if ( check_exit_trap_hit(ch, door, TRUE) )
        return -1;

    if ( !check_swim(ch, to_room) )
        return -1;

    // movement cost and lag
    if ( !NPC_ACT(ch, ACT_OBJ) )
    {
        int move = ( movement_loss[UMIN(SECT_MAX-1, in_room->sector_type)] + movement_loss[UMIN(SECT_MAX-1, to_room->sector_type)] ) / 2;

        // climbing
        int climb_cost = 0;
        if ( in_room->sector_type == SECT_MOUNTAIN )
            climb_cost += 2;
        if ( to_room->sector_type == SECT_MOUNTAIN )
            climb_cost += 2;
        if ( in_room->sector_type == SECT_HILLS )
            climb_cost += 1;
        if ( to_room->sector_type == SECT_HILLS )
            climb_cost += 1;
        
        if ( climb_cost > 0 )
        {
            if ( IS_AFFECTED(ch,AFF_FLYING) )
                move -= climb_cost;
            else if ( per_chance(get_skill(ch, gsn_climbing)) )
            {
                move -= climb_cost;
                check_improve(ch,gsn_climbing,TRUE,3);
            }
            else
                check_improve(ch,gsn_climbing,FALSE,3);
        }

        // swimming
        int deep_water = 0, under_water = 0;
        if ( in_room->sector_type == SECT_WATER_DEEP )
            deep_water++;
        if ( to_room->sector_type == SECT_WATER_DEEP )
            deep_water++;
        if ( in_room->sector_type == SECT_UNDERWATER )
            under_water++;
        if ( to_room->sector_type == SECT_UNDERWATER )
            under_water++;
        
        int swim_chance = get_skill_overflow(ch, gsn_swimming);
        if ( IS_AFFECTED(ch, AFF_FLYING) || has_boat(ch) || per_chance(swim_chance) )
            move -= deep_water;
        if ( per_chance(swim_chance) )
            move -= under_water;
        
        // safety-net
        move = UMAX(1, move);
            
        int waitpulse = 2 + move / 6;

        if ( IS_AFFECTED(ch, AFF_SLOW) )
        {
            move *= 2;
            waitpulse += 2;
        }

        if ( ch->slow_move > 0 )  /* For when a foot is chopped off */
        {
            move = 2 * (move + 1);
            waitpulse = 2 * (waitpulse + 1);
        }

        /* encumberance */
        {
            int encumber = get_encumberance(ch);
            move += (move * encumber + 99) / 100;
            waitpulse += (waitpulse * encumber + 99) / 100;
        }

        if ( IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_HASTE) )
            waitpulse /= 2;
        
        if ( IS_AFFECTED(ch, AFF_SNEAK) && !IS_SET(get_morph_race_type(ch)->affect_field, AFF_SNEAK) )
            move += 1;

        if ( ch->move < move )
        {
            send_to_char( "You are too exhausted.\n\r", ch );
            return -1;
        }

        WAIT_STATE(ch, waitpulse);
        ch->move -= move;
        
        if ( IS_AFFECTED(ch, AFF_SNEAK) )
            check_improve(ch, gsn_sneak, TRUE, 8);
    }
    
    if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
    {
        affect_strip( ch, gsn_hide );
        REMOVE_BIT( ch->affect_field, AFF_HIDE );
        send_to_char( "You come out of hiding.\n\r", ch );
    }
   
    int sneak_skill = (100 + get_skill(ch, gsn_sneak)) * (200 - get_heavy_armor_penalty(ch)) / 400;
    if ( !IS_AFFECTED(ch, AFF_ASTRAL) && ch->invis_level < LEVEL_HERO )
    {
        if ( !IS_AFFECTED(ch, AFF_SNEAK) )
            act( "$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM );
        else
        {
            sprintf( buf, "$n leaves %s.", dir_name[door] );
            for ( fch = in_room->people; fch != NULL; fch = fch_next )
            {
                fch_next = fch->next_in_room;
                chance = sneak_skill - get_skill(fch, gsn_alertness) / 3;
                if ( !per_chance(chance) )
                    act( buf, ch, NULL, fch, TO_VICT );
            }
        }
    }
    
    /* check fighting - no cross-room combat */
    if ( in_room != to_room )
    {
        stop_fighting( ch, TRUE );
        update_room_fighting( in_room );
        check_bleed( ch, door );
    }

    from_area=ch->in_room ? ch->in_room->area : NULL;

    char_from_room( ch );
    char_to_room( ch, to_room );
   
    if ( !IS_AFFECTED(ch, AFF_ASTRAL) && ch->invis_level < LEVEL_HERO )
    {
        if ( !IS_AFFECTED(ch, AFF_SNEAK) && !IS_TAG(ch) )
            act( "$n has arrived.", ch, NULL, NULL, TO_ROOM );
        else
        {
            for ( fch = to_room->people; fch != NULL; fch = fch_next )
            {
                fch_next = fch->next_in_room;
                chance = sneak_skill - get_skill(fch, gsn_alertness) / 3;
                if ( !per_chance(chance) )
                    act( "$n has arrived.", ch, NULL, fch, TO_VICT );
                }
        }
    }
   
   do_look( ch, "auto" );
   
   if (in_room == to_room) /* no circular follows */
       return door;
   
   for ( fch = in_room->people; fch != NULL; fch = fch_next )
   {
       fch_next = fch->next_in_room;
       // ensure that fch_next won't leave room by following fch - can cause follow bug
       // e.g. if B,D follow A, C follows B and room order is B-C-D when A leaves
       // invariant now is that move_char will move all ancestors based on this room's follow-graph
       while ( fch_next && fch_next->master != ch )
           fch_next = fch_next->next_in_room;
       
       if ( fch->master == ch && IS_AFFECTED(fch, AFF_CHARM) && fch->position < POS_STANDING )
           do_stand(fch, "");
       
       if ( fch->master == ch && fch->position == POS_STANDING && can_see_room(fch, to_room) )
       {
           if ( IS_SET(ch->in_room->room_flags, ROOM_LAW) && IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE) )
           {
               act("You can't bring $N into the city.", ch, NULL, fch, TO_CHAR);
               act("You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR);
               continue;
           }
           act( "You follow $N.", fch, NULL, ch, TO_CHAR );
           move_char( fch, door, TRUE );
       }
   }
   
   /* 
   * If someone is following the char, these triggers get activated
   * for the followers before the char, but it's safer this way...
   */
   if ( !IS_NPC( ch ) )
   {
       ap_enter_trigger( ch, from_area );
       ap_renter_trigger( ch );
       rp_enter_trigger( ch );
       op_greet_trigger( ch );
       mp_greet_trigger( ch );
   }

   if ( IS_NPC( ch ) && HAS_TRIGGER( ch, TRIG_ENTRY ) )
       mp_percent_trigger( ch, NULL, NULL, 0, NULL, 0, TRIG_ENTRY );

   /* mprog might have moved the char */
   if ( ch->in_room != to_room )
       return door;

   /*checks for a snare -- Siva*/
   if (IS_SET(ch->in_room->room_flags,ROOM_SNARE))
   {
       chance = (get_curr_stat(ch, STAT_AGI) + get_skill(ch, gsn_avoidance)) / 4;
       if (number_percent () < chance)
       {
           send_to_char ("You narrowly avoid a deadly snare!\n\r", ch);
       }
       else
       {
           int dr;
           in_room=ch->in_room;
           send_to_char ("You are caught in a snare!\n\r", ch);
           act( "$n is caught in a deadly snare!", ch, NULL, NULL, TO_ROOM );
           for ( dr = 0; dr<MAX_DIR; dr++ )
           {
               if ( ( pexit = in_room->exit[dr] ) != NULL
                   &&   pexit->u1.to_room != NULL
                   &&   pexit->u1.to_room != in_room )
               {
                   ch->in_room = pexit->u1.to_room;
                   act( "$n is caught in a nearby snare!", ch, NULL, NULL, TO_ROOM );
               }
           }
           WAIT_STATE(ch, PULSE_VIOLENCE);
           DAZE_STATE(ch, 2 * PULSE_VIOLENCE);
           check_lose_stance(ch);
           set_pos( ch, POS_RESTING );
           ch->in_room=in_room;
           REMOVE_BIT(ch->in_room->room_flags, ROOM_SNARE );  
       }
   }
   
   /* Check for peels, Tryste */
   if (IS_SET(ch->in_room->room_flags,ROOM_PEEL) && !IS_AFFECTED( ch, AFF_FLYING ))
   {
       chance = (get_curr_stat(ch, STAT_AGI) + get_skill(ch, gsn_avoidance)) / 4;
       if (number_percent () < chance)
       {
           send_to_char ("You step over a banana peel!\n\r", ch);
       }
       else
       {
           int dr;
           in_room=ch->in_room;
           send_to_char ("You slip on a banana peel!\n\r", ch);
           act( "$n slips on a banana peel! Point and Laugh!", ch, NULL, NULL,TO_ROOM);
           for ( dr = 0; dr<MAX_DIR; dr++ )
           {
               if ( ( pexit = in_room->exit[dr] ) != NULL
                   &&   pexit->u1.to_room != NULL   
                   &&   pexit->u1.to_room != in_room )
               {
                   ch->in_room = pexit->u1.to_room;
                   act( "$n slipped on a banana peel nearby!", ch, NULL, NULL, TO_ROOM);
               }
           }
           WAIT_STATE(ch, PULSE_VIOLENCE);
           DAZE_STATE(ch, 2 * PULSE_VIOLENCE);
           check_lose_stance(ch);
           set_pos( ch, POS_RESTING );
           ch->in_room=in_room;
           REMOVE_BIT(ch->in_room->room_flags, ROOM_PEEL );
       }
   }

   for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
   {
       fch_next = fch->next_in_room;
       if ( fch->stance == STANCE_AMBUSH
            && can_see(fch, ch)
            && fch->wait == 0
            && fch->position == POS_STANDING
            && per_chance(get_skill(fch, gsn_ambush))
            && !is_safe_spell(fch, ch, TRUE)
            && fch->hunting
            && (!strcmp(fch->hunting, "all") || is_name(fch->hunting, ch->name)) )
        {
            check_improve(fch, gsn_ambush, TRUE, 2);
            if ( is_ranged_weapon(get_eq_char(fch, WEAR_WIELD)) )
                snipe_char(fch, ch);
            else
                backstab_char(fch, ch);
            if ( fch->fighting == ch )
                multi_hit(fch, ch, TYPE_UNDEFINED);
            return door;
        }
   }
   
   return door;
}

bool check_drown(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *room=ch->in_room;
    int dam = 10 + ch->level;

    if (!IS_AFFECTED(ch, AFF_BREATHE_WATER) && !IS_IMMORTAL(ch))
    {
	if (ch->move > dam)
	{
	    send_to_char("You hold your breath.\n\r", ch);
	    ch->move -= dam;
	}
	else
	    full_dam(ch, ch, 2 * dam, gsn_swimming, DAM_DROWNING, TRUE);
    }
    return (ch->in_room != room);
}

static bool has_boat( CHAR_DATA *ch )
{
    OBJ_DATA *boat;
    for ( boat=ch->carrying; boat; boat=boat->next_content )
        if (boat->item_type == ITEM_BOAT)
            return TRUE;
    return FALSE;
}

bool check_swim( CHAR_DATA *ch, ROOM_INDEX_DATA *to_room )
{
    ROOM_INDEX_DATA *in_room = ch->in_room;
    int inwater = 0, towater = 0;
    
    if ( !in_room || !to_room || IS_NPC(ch) || IS_IMMORTAL(ch) )
        return TRUE;

    if ( (in_room->sector_type == SECT_WATER_DEEP) || (in_room->sector_type == SECT_UNDERWATER) )
        inwater = in_room->sector_type;

    if ( (to_room->sector_type == SECT_WATER_DEEP) || (to_room->sector_type == SECT_UNDERWATER) )
        towater = to_room->sector_type;

    if ( !inwater && !towater )
        return TRUE;
    
    if ( inwater != SECT_UNDERWATER && towater != SECT_UNDERWATER )
    {
        if ( IS_AFFECTED(ch, AFF_FLYING) || has_boat(ch))
            return TRUE;
    }

    int chance = (100 + get_skill(ch, gsn_swimming)) / 2;

    if ( !per_chance(chance) )
    {
        send_to_char("You paddle around and get nowhere.\n\r", ch);
        check_improve(ch, gsn_swimming, FALSE, 3);
        if ( inwater == SECT_UNDERWATER )
            check_drown(ch);
        return FALSE;
    }
    else
        check_improve(ch, gsn_swimming, TRUE, 3);
    
    if ( inwater != SECT_UNDERWATER && towater != SECT_UNDERWATER )
    {
        if ( !per_chance(chance) )
        {
            send_to_char("You fail to keep your head above water.\n\r", ch);
            if ( check_drown(ch) )
                return FALSE;
        }
    }
    else if ( !IS_AFFECTED(ch, AFF_BREATHE_WATER) )
    {
        send_to_char("You cant breathe!\n\r", ch);
        if ( check_drown(ch) )
            return FALSE;
    }

    return TRUE;
}

DEF_DO_FUN(do_north)
{
   move_char( ch, DIR_NORTH, FALSE );
   return;
}



DEF_DO_FUN(do_east)
{
   move_char( ch, DIR_EAST, FALSE );
   return;
}



DEF_DO_FUN(do_south)
{
   move_char( ch, DIR_SOUTH, FALSE );
   return;
}



DEF_DO_FUN(do_west)
{
   move_char( ch, DIR_WEST, FALSE );
   return;
}



DEF_DO_FUN(do_up)
{
   move_char( ch, DIR_UP, FALSE );
   return;
}



DEF_DO_FUN(do_down)
{
   move_char( ch, DIR_DOWN, FALSE );
   return;
}

DEF_DO_FUN(do_northeast)
{
   move_char( ch, DIR_NORTHEAST, FALSE );
   return;
}

DEF_DO_FUN(do_southeast)
{
   move_char( ch, DIR_SOUTHEAST, FALSE );
   return;
}

DEF_DO_FUN(do_southwest)
{
   move_char( ch, DIR_SOUTHWEST, FALSE );
   return;
}

DEF_DO_FUN(do_northwest)
{
   move_char( ch, DIR_NORTHWEST, FALSE );
   return;
}


int find_door( CHAR_DATA *ch, const char *arg )
{
   EXIT_DATA *pexit;
   int door;
   
   if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) ) door = DIR_NORTH;
   else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east"  ) ) door = DIR_EAST;
   else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) ) door = DIR_SOUTH;
   else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west"  ) ) door = DIR_WEST;
   else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up"    ) ) door = DIR_UP;
   else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down"  ) ) door = DIR_DOWN;
   else if ( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast"  ) ) door = DIR_NORTHEAST;
   else if ( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast"  ) ) door = DIR_SOUTHEAST;
   else if ( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest"  ) ) door = DIR_SOUTHWEST;
   else if ( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest"  ) ) door = DIR_NORTHWEST;
   else if ( !str_cmp( arg, "door" ) )
   {
	  int count = 0, onedoor = 0;
	  for ( door = 0; door < MAX_DIR; door++ )
		 if ( (pexit = ch->in_room->exit[door]) != NULL
		 && IS_SET(pexit->exit_info, EX_ISDOOR) 
         && !IS_SET(pexit->exit_info, EX_DORMANT) )
		 {
			count++;
			onedoor = door;
		 }
	  if ( count == 0 )
	  {
		 send_to_char( "There are no doors here.\n\r", ch );
		 return -1;
	  }
	  else if ( count == 1 )
		 return onedoor;
	  else
	  {
		 send_to_char( "Which door?  Syntax: <command> <direction>\n\r", ch );
		 return -1;
	  }
   }
   else
   {
	  for ( door = 0; door <MAX_DIR; door++ )
	  {
		 if ( ( pexit = ch->in_room->exit[door] ) != NULL
			&&   IS_SET(pexit->exit_info, EX_ISDOOR)
			&&   pexit->keyword != NULL
			&&   is_name( arg, pexit->keyword ) 
            &&   !IS_SET(pexit->exit_info, EX_DORMANT) )
			return door;
	  }
	  act( "I see no $T here.", ch, NULL, arg, TO_CHAR );
	  return -1;
   }
   
   if ( ( pexit = ch->in_room->exit[door] ) == NULL || IS_SET(pexit->exit_info, EX_DORMANT) )
   {
	  act( "I see no door $T here.", ch, NULL, arg, TO_CHAR );
	  return -1;
   }
   
   if ( !IS_SET(pexit->exit_info, EX_ISDOOR) )
   {
	  send_to_char( "There's an exit but not a door.\n\r", ch );
	  return -1;
   }
   
   return door;
}



DEF_DO_FUN(do_open)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int door;
   
   one_argument( argument, arg );
   
   if ( arg[0] == '\0' )
   {
	  send_to_char( "Open what?\n\r", ch );
	  return;
   }
   
   if ( !is_direction(arg) && (obj = get_obj_here(ch, arg)) != NULL )
   {
	  /* open portal */
	  if (obj->item_type == ITEM_PORTAL)
	  {

		 if (!I_IS_SET(obj->value[1], EX_ISDOOR))
		 {
			send_to_char("You can't do that.\n\r",ch);
			return;
		 }
		 
		 if (!I_IS_SET(obj->value[1], EX_CLOSED))
		 {
			send_to_char("It's already open.\n\r",ch);
			return;
		 }
		 
		 if (I_IS_SET(obj->value[1], EX_LOCKED))
		 {
			send_to_char("It's locked.\n\r",ch);
			return;
		 }
		 
		 if ( check_item_trap_hit(ch, obj) )
		     return;

		 I_REMOVE_BIT(obj->value[1], EX_CLOSED);
		 act("You open $p.",ch,obj,NULL,TO_CHAR);
		 act("$n opens $p.",ch,obj,NULL,TO_ROOM);
		 return;
	  }
	  
	  /* 'open object' */
	  if ( obj->item_type != ITEM_CONTAINER)
	  { send_to_char( "That's not a container.\n\r", ch ); return; }
	  if ( !I_IS_SET(obj->value[1], CONT_CLOSED) )
	  { send_to_char( "It's already open.\n\r",      ch ); return; }
	  if ( !I_IS_SET(obj->value[1], CONT_CLOSEABLE) )
	  { send_to_char( "You can't do that.\n\r",      ch ); return; }
	  if ( I_IS_SET(obj->value[1], CONT_LOCKED) )
	  { send_to_char( "It's locked.\n\r",            ch ); return; }
	  
	  if ( check_item_trap_hit(ch, obj) )
	      return;

      if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_OPEN) )
          return;

	  I_REMOVE_BIT(obj->value[1], CONT_CLOSED);
	  act("You open $p.",ch,obj,NULL,TO_CHAR);
	  act( "$n opens $p.", ch, obj, NULL, TO_ROOM );
	  return;
   }
   
   if ( ( door = find_door( ch, arg ) ) >= 0 )
   {
	  /* 'open door' */
	  ROOM_INDEX_DATA *to_room;
	  EXIT_DATA *pexit;
	  EXIT_DATA *pexit_rev;
	  
	  pexit = ch->in_room->exit[door];
	  if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	  { send_to_char( "It's already open.\n\r",      ch ); return; }
	  if (  IS_SET(pexit->exit_info, EX_LOCKED) )
	  { send_to_char( "It's locked.\n\r",            ch ); return; }

	  if ( check_exit_trap_hit(ch, door, FALSE) )
	      return;

      if ( !rp_open_trigger( ch, door ) )
          return;
	  
	  REMOVE_BIT(pexit->exit_info, EX_CLOSED);
	  act( "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	  send_to_char( "Ok.\n\r", ch );
	  
	  /* open the other side */
	  if ( ( to_room   = pexit->u1.to_room            ) != NULL
		 &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
		 &&   pexit_rev->u1.to_room == ch->in_room )
	  {
		 CHAR_DATA *rch;
		 
		 REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
		 for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
			act( "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
	  }
   }
   
   return;
}



DEF_DO_FUN(do_close)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int door;
   
   one_argument( argument, arg );
   
   if ( arg[0] == '\0' )
   {
	  send_to_char( "Close what?\n\r", ch );
	  return;
   }
   
   if ( !is_direction(arg) && (obj = get_obj_here(ch, arg)) != NULL )
   {
	  /* portal stuff */
	  if (obj->item_type == ITEM_PORTAL)
	  {
		 
		 if (!I_IS_SET(obj->value[1],EX_ISDOOR)
			||   I_IS_SET(obj->value[1],EX_NOCLOSE))
		 {
			send_to_char("You can't do that.\n\r",ch);
			return;
		 }
		 
		 if (I_IS_SET(obj->value[1],EX_CLOSED))
		 {
			send_to_char("It's already closed.\n\r",ch);
			return;
		 }
		 
		 I_SET_BIT(obj->value[1],EX_CLOSED);
		 act("You close $p.",ch,obj,NULL,TO_CHAR);
		 act("$n closes $p.",ch,obj,NULL,TO_ROOM);
		 return;
	  }
	  
	  /* 'close object' */
	  if ( obj->item_type != ITEM_CONTAINER )
	  { send_to_char( "That's not a container.\n\r", ch ); return; }
	  if ( I_IS_SET(obj->value[1], CONT_CLOSED) )
	  { send_to_char( "It's already closed.\n\r",    ch ); return; }
	  if ( !I_IS_SET(obj->value[1], CONT_CLOSEABLE) )
	  { send_to_char( "You can't do that.\n\r",      ch ); return; }
	  
	  I_SET_BIT(obj->value[1], CONT_CLOSED);
	  act("You close $p.",ch,obj,NULL,TO_CHAR);
	  act( "$n closes $p.", ch, obj, NULL, TO_ROOM );
	  return;
   }
   
   if ( ( door = find_door( ch, arg ) ) >= 0 )
   {
	  /* 'close door' */
	  ROOM_INDEX_DATA *to_room;
	  EXIT_DATA *pexit;
	  EXIT_DATA *pexit_rev;
	  
	  pexit   = ch->in_room->exit[door];
	  if ( IS_SET(pexit->exit_info, EX_CLOSED) )
	  { send_to_char( "It's already closed.\n\r",    ch ); return; }

          if ( IS_SET(pexit->exit_info, EX_NOCLOSE) )
          { send_to_char( "It can't be closed.\n\r",     ch ); return; }
      if ( !rp_close_trigger( ch, door ) )
          return;
	  
	  SET_BIT(pexit->exit_info, EX_CLOSED);
	  act( "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	  send_to_char( "Ok.\n\r", ch );
	  
	  /* close the other side */
	  if ( ( to_room   = pexit->u1.to_room            ) != NULL
		 &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
		 &&   pexit_rev->u1.to_room == ch->in_room )
	  {
		 CHAR_DATA *rch;
		 
		 SET_BIT( pexit_rev->exit_info, EX_CLOSED );
		 for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
			act( "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
	  }
   }
   
   return;
}


OBJ_DATA *find_key( CHAR_DATA *ch, int key )
{
   OBJ_DATA *obj;
   
   for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
   {
	  if ( obj->pIndexData->vnum == key )
		 return obj;
   }
   return NULL;
}

bool has_key( CHAR_DATA *ch, int key )
{
   OBJ_DATA *obj=NULL;
   
   for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
   {
	  if ( obj->pIndexData->vnum == key )\
		 return TRUE;
   }
   
   return FALSE;
}



DEF_DO_FUN(do_lock)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj, *key;
   int door;
   
   one_argument( argument, arg );
   
   if ( arg[0] == '\0' )
   {
	  send_to_char( "Lock what?\n\r", ch );
	  return;
   }

   if ( !is_direction(arg) && (obj = get_obj_here(ch, arg)) != NULL )
   {
	  /* portal stuff */
	  if (obj->item_type == ITEM_PORTAL)
	  {

		 if (!I_IS_SET(obj->value[1],EX_ISDOOR)
			||  I_IS_SET(obj->value[1],EX_NOCLOSE))
		 {
			send_to_char("You can't do that.\n\r",ch);
			return;
		 }
		 if (!I_IS_SET(obj->value[1],EX_CLOSED))
		 {
			send_to_char("It's not closed.\n\r",ch);
			return;
		 }
		 
		 if (obj->value[4] < 1 || I_IS_SET(obj->value[1],EX_NOLOCK))
		 {
			send_to_char("It can't be locked.\n\r",ch);
			return;
		 }
		 
		 if (!has_key(ch,obj->value[4]))
		 {
			send_to_char("You lack the key.\n\r",ch);
			return;
		 }
		 else 
			 key = find_key(ch, obj->value[4]);            
		 
		 if (I_IS_SET(obj->value[1],EX_LOCKED))
		 {
			send_to_char("It's already locked.\n\r",ch);
			return;
		 }
		 
		 I_SET_BIT(obj->value[1],EX_LOCKED);
		 act("You lock $p.",ch,obj,NULL,TO_CHAR);
		 act("$n locks $p.",ch,obj,NULL,TO_ROOM);
		 if (IS_SET(key->extra_flags, ITEM_ONE_USE))
		 {
		    act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
		    act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
            extract_obj(key);
		 }
		 return;
	  }
	  
	  /* 'lock object' */
	  if ( obj->item_type != ITEM_CONTAINER )
	  { send_to_char( "That's not a container.\n\r", ch ); return; }
	  if ( !I_IS_SET(obj->value[1], CONT_CLOSED) )
	  { send_to_char( "It's not closed.\n\r",        ch ); return; }
	  if ( obj->value[2] < 1 )
	  { send_to_char( "It can't be locked.\n\r",     ch ); return; }

	  if ( !has_key( ch, obj->value[2] ) )
	  { send_to_char( "You lack the key.\n\r",       ch ); return; }
	  else key = find_key(ch, obj->value[2]);

	  if ( I_IS_SET(obj->value[1], CONT_LOCKED) )
	  { send_to_char( "It's already locked.\n\r",    ch ); return; }

	  I_SET_BIT(obj->value[1], CONT_LOCKED);
	  act("You lock $p.",ch,obj,NULL,TO_CHAR);
	  act( "$n locks $p.", ch, obj, NULL, TO_ROOM );
	  if (IS_SET(key->extra_flags, ITEM_ONE_USE))
	  {
	    act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
	    act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
        extract_obj(key);
	  }
	  
	  return;
   }
   
   if ( ( door = find_door( ch, arg ) ) >= 0 )
   {
	  /* 'lock door' */
	  ROOM_INDEX_DATA *to_room;
	  EXIT_DATA *pexit;
	  EXIT_DATA *pexit_rev;
	  
	  pexit   = ch->in_room->exit[door];
	  if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	  { send_to_char( "It's not closed.\n\r",        ch ); return; }
	  if ( pexit->key < 1 )
	  { send_to_char( "It can't be locked.\n\r",     ch ); return; }
	  if ( !has_key( ch, pexit->key) )
	  { send_to_char( "You lack the key.\n\r",       ch ); return; }
	  else key = find_key(ch,pexit->key);

	  if ( IS_SET(pexit->exit_info, EX_LOCKED) )
	  { send_to_char( "It's already locked.\n\r",    ch ); return; }
	  
      if (!rp_lock_trigger( ch, door ) )
          return;

	  SET_BIT(pexit->exit_info, EX_LOCKED);
	  send_to_char( "*Click*\n\r", ch );
	  act( "$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	  if (IS_SET(key->extra_flags, ITEM_ONE_USE))
		{
		act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
		act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
        extract_obj(key);
		}
	  
	  /* lock the other side */
	  if ( ( to_room   = pexit->u1.to_room            ) != NULL
		 &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
		 &&   pexit_rev->u1.to_room == ch->in_room )
	  {
		 SET_BIT( pexit_rev->exit_info, EX_LOCKED );
	  }
   }
   
   return;
}



DEF_DO_FUN(do_unlock)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj, *key;
   int door;
   
   one_argument( argument, arg );
   
   if ( arg[0] == '\0' )
   {
	  send_to_char( "Unlock what?\n\r", ch );
	  return;
   }
   
   if ( !is_direction(arg) && (obj = get_obj_here(ch, arg)) != NULL )
   {
	  /* portal stuff */
	  if (obj->item_type == ITEM_PORTAL)
	  {
		 if (!I_IS_SET(obj->value[1],EX_ISDOOR))
		 {
			send_to_char("You can't do that.\n\r",ch);
			return;
		 }
		 
		 if (!I_IS_SET(obj->value[1],EX_CLOSED))
		 {
			send_to_char("It's not closed.\n\r",ch);
			return;
		 }

         if (!I_IS_SET(obj->value[1],EX_LOCKED))
         {
            send_to_char("It's not locked.\n\r",ch);
            return;
         }
		 
		 if (obj->value[4] < 1)
		 {
			send_to_char("It can't be unlocked.\n\r",ch);
			return;
		 }
		  
		 if (!has_key(ch,obj->value[4]))
		 {
			send_to_char("You lack the key.\n\r",ch);
			return;
		 }
		 else 
		 {
			 key = find_key(ch, obj->value[4]);            
		 }

		 if (!I_IS_SET(obj->value[1],EX_LOCKED))
		 {
			send_to_char("It's already unlocked.\n\r",ch);
			return;
		 }
		 
		I_REMOVE_BIT(obj->value[1],EX_LOCKED);
		act("You unlock $p.",ch,obj,NULL,TO_CHAR);
		act("$n unlocks $p.",ch,obj,NULL,TO_ROOM);
		if (IS_SET(key->extra_flags, ITEM_ONE_USE))
			{
			act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
			act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
            extract_obj(key);
			}
		
		return;
	  }
	  
	  /* 'unlock object' */
	  if ( obj->item_type != ITEM_CONTAINER )
	  { send_to_char( "That's not a container.\n\r", ch ); return; }
	  if ( !I_IS_SET(obj->value[1], CONT_CLOSED) )
	  { send_to_char( "It's not closed.\n\r",        ch ); return; }
      if ( !I_IS_SET(obj->value[1], CONT_LOCKED) )
      { send_to_char( "It's not locked.\n\r",        ch ); return; }
	  if ( obj->value[2] < 1 )
	  { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
	  if ( !has_key( ch, obj->value[2] ) )
	  { send_to_char( "You lack the key.\n\r",       ch ); return; }
	  else
	     key = find_key( ch, obj->value[2]);
	  if ( !I_IS_SET(obj->value[1], CONT_LOCKED) )
	  { send_to_char( "It's already unlocked.\n\r",  ch ); return; }

      if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_UNLOCK) )
        return;
	  
	  I_REMOVE_BIT(obj->value[1], CONT_LOCKED);
	  act("You unlock $p.",ch,obj,NULL,TO_CHAR);
	  act( "$n unlocks $p.", ch, obj, NULL, TO_ROOM );
		if (IS_SET(key->extra_flags, ITEM_ONE_USE))
			{
			act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
			act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
            extract_obj(key);
			}
	  return;
   }
   
   if ( ( door = find_door( ch, arg ) ) >= 0 )
   {
	  /* 'unlock door' */
	  ROOM_INDEX_DATA *to_room;
	  EXIT_DATA *pexit;
	  EXIT_DATA *pexit_rev;
	  
	  pexit = ch->in_room->exit[door];
	  if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	  { send_to_char( "It's not closed.\n\r",        ch ); return; }
      if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
      { send_to_char( "It's not locked.\n\r",        ch ); return; }
	  if ( pexit->key < 0 )
	  { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
	  if ( !has_key( ch, pexit->key) )
	  { send_to_char( "You lack the key.\n\r",       ch ); return; }
	  else
	    key = find_key( ch, pexit->key );
	  if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
	  { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
	  
      if ( !rp_unlock_trigger( ch, door ) )
          return;

	  REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	  send_to_char( "*Click*\n\r", ch );
	  act( "$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
		if (IS_SET(key->extra_flags, ITEM_ONE_USE))
			{
			act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
			act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
            extract_obj(key);
			}
	  /* unlock the other side */
	  if ( ( to_room   = pexit->u1.to_room            ) != NULL
		 &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
		 &&   pexit_rev->u1.to_room == ch->in_room )
	  {
		 REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
	  }
   }
   
   return;
}

DEF_DO_FUN(do_estimate)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument( argument, arg );

    if ((chance = get_skill(ch, gsn_estimate)) == 0)
    {
	send_to_char ( "Try consider until you truly learn to estimate a foe.\n\r", ch);
	return;
    }

    if ( arg[0] == '\0' )
    {
	send_to_char( "Estimate who or what?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, argument ) ) == NULL )
    {
	send_to_char( "Have to see them to estimate them.\n\r", ch );
	return;
    }

    if (!IS_NPC(victim))
    {
	send_to_char("That being is completely unpredictable.\n\r",ch); 
	return;
    }
   
    if ( IS_SET(victim->act, ACT_OBJ) )
    {
	act( "$N is an object.", ch, NULL, victim, TO_CHAR); 
	return;
    }

    int ch_level = ch->level + mastery_bonus(ch, gsn_estimate, 40, 60);
    if ( (ch_level + chance) < victim->level )
    {
	sprintf( buf, "%s is too powerful for you to identify.\n\r" , victim->short_descr);
	send_to_char(buf, ch);
	return;
    }

    sprintf( buf, "You size up %s.\n\r", victim->short_descr );
    send_to_char( buf, ch );
    WAIT_STATE(ch, skill_table[gsn_estimate].beats);

    /* do now so we can just return later */
    check_improve(ch, gsn_estimate, TRUE, 3);
	
    /* some simple info */
    sprintf( buf, "%s is %s.\n\r", victim->short_descr, char_look_info(victim) );
    send_to_char( buf, ch );

    /* high level races are harder to judge */
    if ( ch_level < victim->level )
        chance -= (victim->level - ch_level) / 4;

    /* let's see how much more we find out */
    int knowledge = 0;
    while ( knowledge < 20 && per_chance(chance) )
        knowledge++;

    if ( knowledge-- > 0 )
    {
        ptc( ch, "This being is level: %d\n\r", victim->level );
    }
    
    if ( knowledge-- > 0 )
    {
        ptc( ch, "Str: %d(%d)  Con: %d(%d)  Vit: %d(%d)  Agi: %d(%d)  Dex: %d(%d)\n\r",
            victim->perm_stat[STAT_STR], get_curr_stat(victim, STAT_STR),
            victim->perm_stat[STAT_CON], get_curr_stat(victim, STAT_CON),
            victim->perm_stat[STAT_VIT], get_curr_stat(victim, STAT_VIT),
            victim->perm_stat[STAT_AGI], get_curr_stat(victim, STAT_AGI),
            victim->perm_stat[STAT_DEX], get_curr_stat(victim, STAT_DEX)
            );
        ptc( ch, "Int: %d(%d)  Wis: %d(%d)  Dis: %d(%d)  Cha: %d(%d)  Luc: %d(%d)\n\r",
            victim->perm_stat[STAT_INT], get_curr_stat(victim, STAT_INT),
            victim->perm_stat[STAT_WIS], get_curr_stat(victim, STAT_WIS),
            victim->perm_stat[STAT_DIS], get_curr_stat(victim, STAT_DIS),
            victim->perm_stat[STAT_CHA], get_curr_stat(victim, STAT_CHA),
            victim->perm_stat[STAT_LUC], get_curr_stat(victim, STAT_LUC)
            );
    }

    if ( knowledge-- > 0 )
    {
        ptc( ch, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d\n\r",
            victim->hit,    victim->max_hit,
            victim->mana,   victim->max_mana,
            victim->move,   victim->max_move
            );
        ptc( ch, "Armor: %d\n\r", GET_AC(victim) );
    }

    if ( knowledge-- > 0 )
    {
        ptc( ch, "Hit: %d  Dam: %d  Saves: %d  Physical: %d\n\r",
            GET_HITROLL(victim), GET_DAMROLL(victim), get_save(victim, FALSE), get_save(victim, TRUE)
            );
        ptc( ch, "Damage: %dd%d  Type: %s\n\r",
            victim->damage[DICE_NUMBER], victim->damage[DICE_TYPE],
            attack_table[victim->dam_type].noun
            );
    }

    if ( knowledge-- > 0 )
    {
        ptc(ch, "Knows how to: %s\n\r", off_bits_name(victim->off_flags));
    }

    if ( knowledge-- > 0 )
    {
        ptc(ch, "Fights in stance: %s\n\r", stances[victim->stance].name);
    }

    // slayers are especially good at estimating weaknesses
    if ( check_skill(ch, gsn_exploit_weakness) )
        knowledge = 3;
    
    if ( knowledge-- > 0 )
    {
        ptc(ch, "Immune to: %s\n\r", imm_bits_name(victim->imm_flags));
    }
  
    if ( knowledge-- > 0 )
    {
        ptc(ch, "Resistant to: %s\n\r", imm_bits_name(victim->res_flags));
    }

    if ( knowledge-- > 0 )
    {
        ptc(ch, "Vulnerable to: %s\n\r", imm_bits_name(victim->vuln_flags));
    }

}

DEF_DO_FUN(do_shoot_lock)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;
    int skill;
   
    one_argument( argument, arg );
   
    if (get_weapon_sn(ch) != gsn_gun)
    {
        send_to_char("You need a gun to shoot a lock.\n\r", ch);
        return;
    }

    obj = (get_eq_char(ch, WEAR_WIELD));
    if (IS_SET(obj->extra_flags, ITEM_JAMMED))
    {
        send_to_char( "Not with a jammed gun.\n\r", ch);
        return;
    }
        
    if ( arg[0] == '\0' )
    {
        send_to_char( "Shoot what now?\n\r", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_shoot_lock].beats );
   
    skill = get_skill(ch,gsn_shoot_lock) * (ch->level + get_curr_stat(ch, STAT_LUC) + 200)/500;
   
    /*
    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        send_to_char("But you could damage whatever's inside!\n\r",ch);
        return;
    }
    */
         
    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'pick door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;
	  
        pexit = ch->in_room->exit[door];
        
        /* different difficulties */
        if (IS_SET(pexit->exit_info,EX_INFURIATING))
            skill -= 50;
        else if (IS_SET(pexit->exit_info,EX_HARD))
            skill -= 25;
        else if (IS_SET(pexit->exit_info,EX_EASY))
            skill += 25;
          
        if ( !per_chance(skill) )
        {
            send_to_char( "You miss the lock completely.\n\r", ch);
	        check_improve(ch,gsn_shoot_lock,FALSE,1);
	        return;
        }  
        
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
        { 
            send_to_char( "It's not closed, why shoot the lock?\n\r", ch ); 
            return; 
        }
        
        if ( pexit->key < 0 && !IS_IMMORTAL(ch))
        { 
            send_to_char( "Your gun is useless against this strong lock.\n\r", ch ); 
            return; 
        }
        
        if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
        { 
            send_to_char( "It was already unlocked, but . . .\n\r", ch ); 
            return; 
        }
        
        if ( IS_SET(pexit->exit_info, EX_PICKPROOF))
        { 
            send_to_char( "Nope, shooting the lock did no good.\n\r", ch ); 
            return; 
        }
	  
	    REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	    send_to_char( "*BLAM!*\n\r", ch );
	    act( "$n blows away the lock on the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	    check_improve(ch,gsn_shoot_lock,TRUE,1);
	  
	    /* pick the other side */
	    if ( ( to_room   = pexit->u1.to_room            ) != NULL
		    &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
		    &&   pexit_rev->u1.to_room == ch->in_room )
	    {
		    REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
	    }
    }
    return;
}

DEF_DO_FUN(do_unjam)
{
    OBJ_DATA *obj;
    int skill;
    
    /* find out what */
    if (argument[0] != '\0')
    {
	if ( (obj = get_obj_carry(ch, argument, ch)) == NULL )
	{
	    send_to_char("You don't have that in inventory.\n\r",ch);
	    return;
	}
    }
    else
    {
	/* check first primary, then secondary weapon */
	if ( (obj = get_eq_char(ch, WEAR_WIELD)) == NULL )
	{
	    send_to_char("You don't wield anything.\n\r",ch);
	    return;
	}
	if ( !IS_OBJ_STAT(obj,ITEM_JAMMED) )
	{
	    //act( "$p isn't jammed(?)", ch, obj, NULL, TO_CHAR );
	    obj = get_eq_char(ch, WEAR_SECONDARY);
	}
    }

    if ( obj == NULL || !IS_OBJ_STAT(obj,ITEM_JAMMED) )
    {
	send_to_char( "Your weapon isn't jammed.\n\r",ch);
	return;
    }
    
    /* ok, we have a jammed weapon now */

    if ((skill = get_skill(ch,gsn_unjam)) < 1)
    {
	send_to_char("You're not really that sure what causes a jam.\n\r",ch);
	return;
    }

    /*
    if (!IS_OBJ_STAT(obj,ITEM_JAMMED))
    {
	act("$p is not jammed.",ch,obj,NULL,TO_CHAR);
	return;
    }
    */

    if (number_percent() < (skill/3) + (get_curr_stat(ch, STAT_LUC)/12))  
    {
	REMOVE_BIT(obj->extra_flags,ITEM_JAMMED);
	act("$n unjams $s weapon!",ch,obj,NULL,TO_ROOM);
	act("You unjam $p.",ch,obj,NULL,TO_CHAR);
	check_improve(ch,gsn_unjam,TRUE,2);
	WAIT_STATE(ch,skill_table[gsn_unjam].beats);
	return;
    }
    else
    {
	act("You fail to unjam $p.",ch,obj,NULL,TO_CHAR);
	check_improve(ch,gsn_unjam,FALSE,2);
	WAIT_STATE(ch,skill_table[gsn_unjam].beats);
	return;
    }
	
    /*
    act("You can't unjam $p.",ch,obj,NULL,TO_CHAR);
    */
}

DEF_DO_FUN(do_set_snare)
{

    int chance;
    ROOM_INDEX_DATA *in_room;  

    in_room = ch->in_room;

    if (((ch->in_room->sector_type) == SECT_INSIDE)  
	|| ((ch->in_room->sector_type) == SECT_CITY))
    {
	send_to_char("This place is unsuitable for a snare.\n\r",ch);
	return;
    }

    if ((chance = (get_skill(ch, gsn_set_snare))) !=0 )
	{
	    if ((number_percent ()) < (chance))
		{
		    send_to_char("You set up a snare.\n\r", ch);
		    act("$n sets a snare.", ch, NULL, NULL, TO_ROOM);
		    SET_BIT( in_room->room_flags, ROOM_SNARE );   
		    check_improve(ch, gsn_set_snare, TRUE, 3); 
		}
	    
	    else
		{
		    send_to_char("Your snare doesn't quite work.\n\r", ch);
		    check_improve(ch, gsn_set_snare, FALSE, 3); 
		}
	    
	}
    else
	send_to_char ("You're not sure how to set a snare.\n\r", ch);
    return;
}

DEF_DO_FUN(do_peel)
{

    int chance;
    ROOM_INDEX_DATA *in_room;
    
    in_room = ch->in_room;

    if (((ch->in_room->sector_type) == SECT_INSIDE)
	|| ((ch->in_room->sector_type) == SECT_CITY))
	{
	    send_to_char("Give a hoot, don't litter!\n\r",ch);
	    return;
	}
    
    if ((chance = (get_skill(ch, gsn_peel))) !=0 )
        {
	    if ((number_percent ()) < (chance))
                {
		    send_to_char("A banana now lies in wait for some poor sap.\n\r", ch);
		    act("$n drops a banana peel, hope nobody slips.", ch, NULL,
			NULL, TO_ROOM);
		    SET_BIT( in_room->room_flags, ROOM_PEEL );
		    check_improve(ch, gsn_peel, TRUE, 3);
                }
	    
	    else
                {
		    send_to_char("The peel sticks to your hand.\n\r", ch);
		    check_improve(ch, gsn_peel, FALSE, 3);
                }
	    
	}
    else
        send_to_char ("You don't like bananas, hence you don't have any peels.\n\r", ch);
    return;
}


DEF_DO_FUN(do_pick)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    int door;
    int skill;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Pick what?\n\r", ch );
        return;
    }

    skill = get_skill(ch,gsn_pick_lock) * (ch->level + get_curr_stat(ch, STAT_DEX) + 200)/500;

    if (skill == 0)
    {
        send_to_char("You don't know how to pick locks.\n\r", ch );
        return;
    }
    
    if( !str_prefix(arg,"nort") )
        { do_pick(ch,"north"); return; }
    if( !str_prefix(arg,"sout") )
        { do_pick(ch,"south"); return; }
    if( !str_prefix(arg,"eas") )
        { do_pick(ch,"east"); return; }
    if( !str_prefix(arg,"wes") )
        { do_pick(ch,"west"); return; }
    if( !str_cmp(arg,"nw") )
        { do_pick(ch,"northwest"); return; }
    if( !str_cmp(arg,"ne") )
        { do_pick(ch,"northeast"); return; }
    if( !str_cmp(arg,"se") )
        { do_pick(ch,"southeast"); return; }
    if( !str_cmp(arg,"sw") )
        { do_pick(ch,"southwest"); return; }
    
   
    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
    
            /* look for guards */
            for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
            {
                if ( IS_NPC(gch) && IS_AWAKE(gch) && !IS_AFFECTED(gch, AFF_CHARM) && ch->level + 5 < gch->level )
                {
                    act( "$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR );
                    return;
                }
            }
    
            if (!I_IS_SET(obj->value[1],EX_ISDOOR))
            {   
                send_to_char("You can't do that.\n\r",ch);
                return;
            }
            
            if (!I_IS_SET(obj->value[1],EX_LOCKED))
            {
                send_to_char("It's not locked.\n\r",ch);
                return;
            }
            
            if (!I_IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's not closed.\n\r",ch);
                return;
            }
            
            if (obj->value[4] < 0)
            {
                send_to_char("It can't be unlocked.\n\r",ch);
                return;
            }
            
            if ( I_IS_SET(obj->value[1],EX_PICKPROOF) )
            {
                send_to_char( "You can't seem to figure out how to pick this lock.\n\r", ch );
                return;
            }
            
            /* different difficulties */
            if (I_IS_SET(obj->value[1],EX_INFURIATING))
                skill -= 50;
            else if (I_IS_SET(obj->value[1],EX_HARD))
                skill -= 25;
            else if (I_IS_SET(obj->value[1],EX_EASY))
                skill += 25;
            
            
            if ( number_percent() > skill )
            {
                send_to_char( "You failed.\n\r", ch);
                check_improve(ch,gsn_pick_lock,FALSE,1);
                WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );
                return;
            }
            
            I_REMOVE_BIT(obj->value[1],EX_LOCKED);
            act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
            act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
            check_improve(ch,gsn_pick_lock,TRUE,1);
            WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );
            return;
        }
        
         if ( obj->carried_by == NULL )
        {
            /* look for guards */
            for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
            {
                if ( IS_NPC(gch) && IS_AWAKE(gch) && !IS_AFFECTED(gch, AFF_CHARM) && ch->level + 5 < gch->level )
                {
                    act( "$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR );
                    return;
                }
            }
        }
        else
        {
            /* 'pick object' */
            if ( obj->item_type != ITEM_CONTAINER )
                { send_to_char( "That's not a container.\n\r", ch ); return; }
            if ( !I_IS_SET(obj->value[1], CONT_CLOSED) )
                { send_to_char( "It's not closed.\n\r",        ch ); return; }
            if ( obj->value[2] < 0 )
                { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
            if ( !I_IS_SET(obj->value[1], CONT_LOCKED) )
                { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
            if ( I_IS_SET(obj->value[1],CONT_PICKPROOF) )
                { send_to_char( "You can't seem to figure out how to pick this lock.\n\r", ch ); return; }
            
            if (I_IS_SET(obj->value[1],CONT_INFURIATING))
                skill -= 50;
            else if (I_IS_SET(obj->value[1],CONT_HARD))
                skill -= 25;
            else if (I_IS_SET(obj->value[1],CONT_EASY))
                skill += 25;
            if ( number_percent() > skill )
            {
                send_to_char( "You failed.\n\r", ch);
                check_improve(ch,gsn_pick_lock,FALSE,1);
                WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );
                return;
            }
        }
         
        I_REMOVE_BIT(obj->value[1], CONT_LOCKED);
        act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
        act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
        check_improve(ch,gsn_pick_lock,TRUE,1);
        WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );
        return;
    }
    
    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'pick door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;
        
        /* look for guards */
        for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
        {
            if ( IS_NPC(gch) && IS_AWAKE(gch) && !IS_AFFECTED(gch, AFF_CHARM) && ch->level + 5 < gch->level )
            {
                act( "$N is standing too close to the lock.",
                ch, NULL, gch, TO_CHAR );
                return;
            }
        }
        
        pexit = ch->in_room->exit[door];
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( pexit->key < 0 && !IS_IMMORTAL(ch))
            { send_to_char( "It can't be picked.\n\r",     ch ); return; }
        if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
        
        if ( IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
            { send_to_char( "You can't seem to figure out how to pick this lock.\n\r", ch ); return; }
        
        /* different difficulties */
        if (IS_SET(pexit->exit_info,EX_INFURIATING))
            skill -= 50;
        else if (IS_SET(pexit->exit_info,EX_HARD))
            skill -= 25;
        else if (IS_SET(pexit->exit_info,EX_EASY))
            skill += 25;
        
        if (  number_percent() > skill )
        {
            send_to_char( "You failed.\n\r", ch);
            check_improve(ch,gsn_pick_lock,FALSE,1);
            WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );
            return;
        }
        
        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char( "*Click*\n\r", ch );
        act( "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );
        check_improve(ch,gsn_pick_lock,TRUE,1);
        
        /* pick the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
            &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
            &&   pexit_rev->u1.to_room == ch->in_room )
        {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }   
        
    return;
}


DEF_DO_FUN(do_stand)
{
   OBJ_DATA  *obj = NULL;
   CHAR_DATA *gch = NULL;
   
   if (argument[0] != '\0')
   {
	  if (ch->position == POS_FIGHTING)
	  {
		 send_to_char("Maybe you should finish fighting first?\n\r",ch);
		 return;
	  }
	  obj = get_obj_list(ch,argument,ch->in_room->contents);
	  if (obj == NULL)
	  {
		 send_to_char("You don't see that here.\n\r",ch);
		 return;
	  }
	  if (obj->item_type != ITEM_FURNITURE
		 ||  (!I_IS_SET(obj->value[2],STAND_AT)
		 &&   !I_IS_SET(obj->value[2],STAND_ON)
		 &&   !I_IS_SET(obj->value[2],STAND_IN)))
	  {
		 send_to_char("You can't seem to find a place to stand.\n\r",ch);
		 return;
	  }
	  if (ch->on != obj && count_users(obj) >= obj->value[0])
	  {
		 act_new("There's no room to stand on $p.",
			ch,obj,NULL,TO_CHAR,POS_DEAD);
		 return;
	  }
	  ch->on = obj;
   }
   
   switch ( ch->position )
   {
   case POS_SLEEPING:
	  if ( IS_AFFECTED(ch, AFF_SLEEP) )
	  { send_to_char( "You can't wake up!\n\r", ch ); return; }
	  
      if ( ch->on && !op_percent_trigger( NULL, ch->on, NULL, ch, NULL, OTRIG_WAKE) )
      {
          return;
      }
	  if (obj == NULL)
	  {
		 send_to_char( "You wake and stand up.\n\r", ch );
		 act( "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM );
		 ch->on = NULL;
	  }
	  else if (I_IS_SET(obj->value[2],STAND_AT))
	  {
		 act_new("You wake and stand at $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
		 act("$n wakes and stands at $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],STAND_ON))
	  {
		 act_new("You wake and stand on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
		 act("$n wakes and stands on $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else 
	  {
		 act_new("You wake and stand in $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
		 act("$n wakes and stands in $p.",ch,obj,NULL,TO_ROOM);
	  }

	 if ( IS_AFFECTED(ch, AFF_SHELTER) )
	     for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
		 if (is_same_group( gch, ch ))
		 {
		     affect_strip ( gch, gsn_shelter    );
		     REMOVE_AFFECT ( gch, AFF_SHELTER );
		     set_pos(gch, POS_STANDING);
		     if (gch == ch)
			 send_to_char("You have revealed the shelter!\n",ch);
		     else
			 act_new("The group's shelter was revealed by $N.", gch, NULL, ch, TO_CHAR, POS_STANDING);
		 }
	  
        set_pos(ch, POS_STANDING);
          if (!IS_NPC(ch))
          {
             ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
          }
	  do_look(ch,"auto");
	  break;
	  
   case POS_RESTING: case POS_SITTING:
      if ( ch->on && !op_percent_trigger( NULL, ch->on, NULL, ch, NULL, OTRIG_WAKE) )
      {
          return;
      }
	  if (obj == NULL)
	  {
		 send_to_char( "You stand up.\n\r", ch );
		 act( "$n stands up.", ch, NULL, NULL, TO_ROOM );
		 ch->on = NULL;
	  }
	  else if (I_IS_SET(obj->value[2],STAND_AT))
	  {
		 act("You stand at $p.",ch,obj,NULL,TO_CHAR);
		 act("$n stands at $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],STAND_ON))
	  {
		 act("You stand on $p.",ch,obj,NULL,TO_CHAR);
		 act("$n stands on $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else
	  {
		 act("You stand in $p.",ch,obj,NULL,TO_CHAR);
		 act("$n stands on $p.",ch,obj,NULL,TO_ROOM);
	  }
        set_pos(ch, POS_STANDING);
      provoke_attacks(ch);  
	  break;
	  
   case POS_STANDING:
	  send_to_char( "You are already standing.\n\r", ch );
	  break;
	  
   case POS_FIGHTING:
	  send_to_char( "You are already fighting!\n\r", ch );
	  break;
   }
   
   return;
}



DEF_DO_FUN(do_rest)
{
   OBJ_DATA *obj = NULL;
   CHAR_DATA *gch = NULL;
   
   if (ch->position == POS_FIGHTING)
   {
	  send_to_char("You are already fighting!\n\r",ch);
	  return;
   }
   
   /* okay, now that we know we can rest, find an object to rest on */
   if (argument[0] != '\0')
   {
	  obj = get_obj_list(ch,argument,ch->in_room->contents);
	  if (obj == NULL)
	  {
		 send_to_char("You don't see that here.\n\r",ch);
		 return;
	  }
   }
   else obj = ch->on;
   
   if (obj != NULL)
   {
	  if (obj->item_type != ITEM_FURNITURE
		 ||  (!I_IS_SET(obj->value[2],REST_ON)
		 &&   !I_IS_SET(obj->value[2],REST_IN)
		 &&   !I_IS_SET(obj->value[2],REST_AT)))
	  {
		 send_to_char("You can't rest on that.\n\r",ch);
		 return;
	  }
	  
	  if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
	  {
		 act_new("There's no more room on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
		 return;
	  }

      if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_REST) )
            return;
	  
	  ch->on = obj;
   }
   
   switch ( ch->position )
   {
   case POS_SLEEPING:
	  if (IS_AFFECTED(ch,AFF_SLEEP))
	  {
		 send_to_char("You can't wake up!\n\r",ch);
		 return;
	  }
	  
	  if (obj == NULL)
	  {
		 send_to_char( "You wake up and start resting.\n\r", ch );
		 act ("$n wakes up and starts resting.",ch,NULL,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],REST_AT))
	  {
		 act_new("You wake up and rest at $p.",
			ch,obj,NULL,TO_CHAR,POS_SLEEPING);
		 act("$n wakes up and rests at $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],REST_ON))
	  {
		 act_new("You wake up and rest on $p.",
			ch,obj,NULL,TO_CHAR,POS_SLEEPING);
		 act("$n wakes up and rests on $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else
	  {
		 act_new("You wake up and rest in $p.",
			ch,obj,NULL,TO_CHAR,POS_SLEEPING);
		 act("$n wakes up and rests in $p.",ch,obj,NULL,TO_ROOM);
	  }
	  ch->position = POS_RESTING;
      {
          if (!IS_NPC(ch))
          {
             ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
          }
       }

         if ( IS_AFFECTED(ch, AFF_SHELTER) )
                 for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
                         if (is_same_group( gch, ch ))
                         {
                                 affect_strip ( gch, gsn_shelter    );
                                 REMOVE_AFFECT ( gch, AFF_SHELTER );
				 set_pos(gch, POS_STANDING);
                                 if (gch == ch)
                                         send_to_char("You have revealed the shelter!\n",ch);
                                 else
                                         act_new("The group's shelter was revealed by $N.", gch, NULL, ch, TO_CHAR, POS_STANDING);

                         }
	  break;
	  
   case POS_RESTING:
	  send_to_char( "You are already resting.\n\r", ch );
	  break;
	  
   case POS_STANDING:
	  if (obj == NULL)
	  {
		 send_to_char( "You rest.\n\r", ch );
		 act( "$n sits down and rests.", ch, NULL, NULL, TO_ROOM );
	  }
	  else if (I_IS_SET(obj->value[2],REST_AT))
	  {
		 act("You sit down at $p and rest.",ch,obj,NULL,TO_CHAR);
		 act("$n sits down at $p and rests.",ch,obj,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],REST_ON))
	  {
		 act("You sit on $p and rest.",ch,obj,NULL,TO_CHAR);
		 act("$n sits on $p and rests.",ch,obj,NULL,TO_ROOM);
	  }
	  else
	  {
		 act("You rest in $p.",ch,obj,NULL,TO_CHAR);
		 act("$n rests in $p.",ch,obj,NULL,TO_ROOM);
	  }
	  ch->position = POS_RESTING;
          if (!IS_NPC(ch))
          {
             ch->pcdata->condition[COND_DEEP_SLEEP] = 0; 
          }
	  break;
	  
   case POS_SITTING:
	  if (obj == NULL)
	  {
		 send_to_char("You rest.\n\r",ch);
		 act("$n rests.",ch,NULL,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],REST_AT))
	  {
		 act("You rest at $p.",ch,obj,NULL,TO_CHAR);
		 act("$n rests at $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],REST_ON))
	  {
		 act("You rest on $p.",ch,obj,NULL,TO_CHAR);
		 act("$n rests on $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else
	  {
		 act("You rest in $p.",ch,obj,NULL,TO_CHAR);
		 act("$n rests in $p.",ch,obj,NULL,TO_ROOM);
	  }
	  ch->position = POS_RESTING;
          if (!IS_NPC(ch))
          {
             ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
          }
	  break;
   }
   
   
   return;
}


DEF_DO_FUN(do_sit)
{
    OBJ_DATA *obj = NULL;
    CHAR_DATA *gch = NULL;
   
    if ( ch->position == POS_FIGHTING )
    {
        send_to_char("Maybe you should finish this fight first?\n\r",ch);
        return;
    }
   
    /* okay, now that we know we can sit, find an object to sit on */
    if ( argument[0] != '\0' )
    {
        obj = get_obj_list(ch, argument, ch->in_room->contents);
        if (obj == NULL)
        {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    }
    else
        obj = ch->on;
   
    /* check if we can sit down */
    if ( ch->position == POS_SITTING && ch->on == obj )
    {
        if ( obj )
            act_new("You are already sitting on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
        else
            send_to_char("You are already sitting down.\n\r", ch);
        return;
    }
   
    if ( ch->position == POS_SLEEPING && IS_AFFECTED(ch, AFF_SLEEP) )
    {
        send_to_char("You can't wake up!\n\r",ch);
        return;       
    }
   
    if ( obj != NULL )
    {
        if ( obj->item_type != ITEM_FURNITURE
            || (!I_IS_SET(obj->value[2], SIT_ON)
            &&  !I_IS_SET(obj->value[2], SIT_IN)
            &&  !I_IS_SET(obj->value[2], SIT_AT)) )
        {
            send_to_char("You can't sit on that.\n\r", ch);
            return;
        }
        
        if ( obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0] )
        {
            act_new("There's no more room on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            return;
        }
        
        if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_SIT) )
            return;
        
        ch->on = obj;
    }
    
    int old_pos = ch->position;
    ch->position = POS_SITTING;
   
    /* ok, we already sat down, now just send feedback */
    if ( old_pos == POS_SLEEPING )
    {
        if ( obj == NULL )
        {
            send_to_char( "You wake and sit up.\n\r", ch );
            act( "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
        }
        else if ( I_IS_SET(obj->value[2], SIT_AT) )
        {
            act_new("You wake and sit at $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
        }
        else if ( I_IS_SET(obj->value[2],SIT_ON) )
        {
            act_new("You wake and sit on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
        }
        else
        {
            act_new("You wake and sit in $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            act("$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM);
        }
        
        if ( IS_AFFECTED(ch, AFF_SHELTER) )
            for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
                if ( is_same_group(gch, ch) )
                {
                    affect_strip(gch, gsn_shelter);
                    REMOVE_AFFECT(gch, AFF_SHELTER);
                    if ( gch == ch )
                        send_to_char("You have revealed the shelter!\n", ch);
                    else
                        act_new("The group's shelter was revealed by $N.", gch, NULL, ch, TO_CHAR, POS_RESTING);
                }
        
          if (!IS_NPC(ch))
          {
             ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
          }
    }
    else
    {
        if ( obj == NULL )
        {
            act("$n sits down on the ground.", ch, NULL, NULL, TO_ROOM);
            send_to_char("You sit down.\n\r", ch);
        }
        else if ( I_IS_SET(obj->value[2], SIT_AT) )
        {
            act("You sit at $p.", ch, obj, NULL, TO_CHAR);
            act("$n sits at $p.", ch, obj, NULL, TO_ROOM);
        }
        else if ( I_IS_SET(obj->value[2], SIT_ON) )
        {
            act("You sit on $p.", ch, obj, NULL, TO_CHAR);
            act("$n sits on $p.", ch, obj, NULL, TO_ROOM);
        }
        else
        {
            act("You sit down in $p.", ch, obj, NULL, TO_CHAR);
            act("$n sits down in $p.", ch, obj, NULL, TO_ROOM);
        }
    }
    return;
}


DEF_DO_FUN(do_sleep)
{
   OBJ_DATA *obj = NULL;
   
   switch ( ch->position )
   {
   case POS_SLEEPING:
	  send_to_char( "You are already sleeping.\n\r", ch );
	  break;
	  
   case POS_RESTING:
   case POS_SITTING:
   case POS_STANDING: 
	  if (argument[0] == '\0' && ch->on == NULL)
	  {
		 send_to_char( "You go to sleep.\n\r", ch );
		 act( "$n goes to sleep.", ch, NULL, NULL, TO_ROOM );
		 ch->position = POS_SLEEPING;
	  }
	  else  /* find an object and sleep on it */
	  {
		 if (argument[0] == '\0')
			obj = ch->on;
		 else
			obj = get_obj_list( ch, argument,  ch->in_room->contents );
		 
		 if (obj == NULL)
		 {
			send_to_char("You don't see that here.\n\r",ch);
			return;
		 }
		 if (obj->item_type != ITEM_FURNITURE
			||  (!I_IS_SET(obj->value[2],SLEEP_ON) 
			&&   !I_IS_SET(obj->value[2],SLEEP_IN)
			&&   !I_IS_SET(obj->value[2],SLEEP_AT)))
		 {
			send_to_char("You can't sleep on that!\n\r",ch);
			return;
		 }
		 
		 if (ch->on != obj && count_users(obj) >= obj->value[0])
		 {
			act_new("There is no room on $p for you.",
			   ch,obj,NULL,TO_CHAR,POS_DEAD);
			return;
		 }
		 
         if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_SLEEP) )
            return;

		 ch->on = obj;
		 if (I_IS_SET(obj->value[2],SLEEP_AT))
		 {
			act("You go to sleep at $p.",ch,obj,NULL,TO_CHAR);
			act("$n goes to sleep at $p.",ch,obj,NULL,TO_ROOM);
		 }
		 else if (I_IS_SET(obj->value[2],SLEEP_ON))
		 {
			act("You go to sleep on $p.",ch,obj,NULL,TO_CHAR);
			act("$n goes to sleep on $p.",ch,obj,NULL,TO_ROOM);
		 }
		 else
		 {
			act("You go to sleep in $p.",ch,obj,NULL,TO_CHAR);
			act("$n goes to sleep in $p.",ch,obj,NULL,TO_ROOM);
		 }
		 ch->position = POS_SLEEPING;
	  }
      if (ch->song != 0)
      {
         ch->song = SONG_DEFAULT;
         remove_bard_song_group(ch);
         send_to_char("You are no longer singing.\n\r", ch);
      }
	  break;
	  
   case POS_FIGHTING:
	  send_to_char( "You are already fighting!\n\r", ch );
	  break;
   }
   
   return;
}



DEF_DO_FUN(do_wake)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   
   one_argument( argument, arg );
   if ( arg[0] == '\0' )
   { do_stand( ch, argument ); return; }
   
   if ( !IS_AWAKE(ch) )
   { send_to_char( "You are asleep yourself!\n\r",       ch ); return; }
   
   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   { send_to_char( "They aren't here.\n\r",              ch ); return; }
   
   if ( IS_AWAKE(victim) )
   { act( "$N is already awake.", ch, NULL, victim, TO_CHAR ); return; }
   
   if ( IS_AFFECTED(victim, AFF_SLEEP) )
   { act( "You can't wake $M!",   ch, NULL, victim, TO_CHAR );  return; }
   
   act_new( "$n wakes you.", ch, NULL, victim, TO_VICT,POS_SLEEPING );
   do_stand(victim,"");
   return;
}


DEF_DO_FUN(do_sneak)
{
    AFFECT_DATA af;
    
    if ( !strcmp(argument, "stop") )
    {
	if ( !is_affected(ch, gsn_sneak) )
	    send_to_char( "You aren't sneaking.\n\r", ch );
	else
	{
	    affect_strip( ch, gsn_sneak );
	    send_to_char( "You stop sneaking.\n\r", ch );
	}
	return;
    }

    if (!IS_NPC(ch) && IS_TAG(ch))
    {
        send_to_char("You cannot sneak while playing Freeze Tag.\n\r",ch);
        return;
    }

    if (ch->fighting != NULL)
    {
        send_to_char("You are fighting!\n\r",ch);
        return;
    }
    
    if (IS_AFFECTED(ch,AFF_SNEAK))
    {
        send_to_char("You are already sneaking!\n\r",ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_ASTRAL) )
    {
        send_to_char("Can't sneak around the astral plane.\n\r",ch);
        return;
    }

    send_to_char( "You start to move silently.\n\r", ch );

    af.where     = TO_AFFECTS;
    af.type      = gsn_sneak;
    af.level     = ch->level;
    af.duration  = -1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SNEAK;
    affect_to_char( ch, &af );
        
    return;
}

int get_hips_skill( CHAR_DATA *ch )
{
    if ( room_is_dim(ch->in_room) || IS_AFFECTED(ch, AFF_SHROUD) )
        return get_skill(ch, gsn_hips);
    else if ( ch->stance == STANCE_SHADOWWALK )
        return get_skill(ch, gsn_hips) * get_skill(ch, gsn_shadowwalk) / 100;
    else
        return 0;
}

DEF_DO_FUN(do_hide)
{
    AFFECT_DATA af;
    int hips_skill = get_hips_skill(ch);
    bool hips = FALSE;
    
    send_to_char( "You attempt to hide.\n\r", ch );
    
 /* Commented out because I believe this was changed long ago - Astark 1-7-13
  *    if (IS_REMORT(ch))
  *    {
  *        send_to_char("There is noplace to hide in remort.\n\r",ch);
  *        return;
  *    } 
  */

    if (IS_NOHIDE(ch))
    {
        send_to_char("There is no place to hide.\n\r",ch);
        return;
    }
    if (IS_TAG(ch))
    {
        send_to_char("There is noplace to hide in freeze tag.\n\r",ch);
        return;
    }
    
    if ( IS_AFFECTED(ch, AFF_HIDE) )
    {
        send_to_char( "Shh..you're already hiding! Don't blow your cover!\n\r", ch );
        return;
    }
    
    if ( IS_AFFECTED(ch, AFF_ASTRAL) )
    {
        send_to_char("There is no hiding on the Astral plane!\n\r",ch);
        return;
    }

    if (ch->fighting != NULL)
    {
        if ( hips_skill == 0 )
        {
            send_to_char("I think your opponent sees you.\n\r",ch);
            return;
        }
        hips = TRUE;
    }
    
    WAIT_STATE( ch, skill_table[gsn_hide].beats );
    if ( per_chance(50 + get_skill(ch, gsn_hide) / 2) )
    {
        // hips: need to apply affect first to get check_see to work
        af.where     = TO_AFFECTS;
        af.type      = gsn_hide;
        af.level     = ch->level; 
        af.duration  = -1;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_HIDE;
        affect_to_char( ch, &af );
        if ( hips )
        {
            if ( !per_chance(hips_skill) )
            {
                send_to_char("You fail to conceal yourself with shadows.\n\r", ch);
                affect_strip(ch, gsn_hide);
                return;
            }
            CHAR_DATA *opp;
            for ( opp = ch->in_room->people; opp != NULL; opp = opp->next_in_room )
            {
                if ( !is_opponent(opp, ch) )
                    continue;
                if ( check_see_combat(opp, ch) )
                {
                    act("$N spots you and your attempt to hide fails.", ch, NULL, opp, TO_CHAR);
                    affect_strip(ch, gsn_hide);
                    return;
                }
            }
            // successful hide-in-plain-sight, end all combat with ch
            act("$n vanishes into the shadows.", ch, NULL, NULL, TO_ROOM);
            stop_fighting(ch, TRUE);
        }
        send_to_char("You successfully hide.\n\r",ch); 
        check_improve(ch,gsn_hide,TRUE,3);
        // hips grants lag-free hide if successful
        if ( per_chance(hips_skill) )
            ch->wait = 0;
    }
    else
    {
        send_to_char("Everyone laughs at your pathetic attempt to hide.\n\r",ch); 
        check_improve(ch,gsn_hide,FALSE,3);
    }
    
    return;
}

void make_visible( CHAR_DATA *ch )
{
   affect_strip ( ch, gsn_hide        );
   affect_strip_flag(ch, AFF_INVISIBLE);
   affect_strip ( ch, gsn_astral      );
   affect_strip ( ch, gsn_shelter     );
   REMOVE_AFFECT( ch, AFF_HIDE        );
   REMOVE_AFFECT( ch, AFF_INVISIBLE   );
   REMOVE_AFFECT( ch, AFF_ASTRAL      );
   REMOVE_AFFECT( ch, AFF_SHELTER     );
}

/*
* Contributed by Alander.
*/
DEF_DO_FUN(do_visible)
{
    make_visible( ch );
    send_to_char( "Ok.\n\r", ch );
    return;
}

DEF_DO_FUN(do_recall)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *location;
    AREA_DATA *from_area;
    int chance;
    int room=ROOM_VNUM_RECALL;
    int move_cost;
    const char *god_name;
    
    if (IS_REMORT(ch))
    {
        send_to_char("Not from remort, chucklehead.\n\r",ch);
        return;
    }
    
    
    if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET))
    {
        send_to_char("Only players can recall.\n\r",ch);
        return;
    }

    if (NOT_AUTHED(ch))
    {
        send_to_char("You cannot recall until your character is authorized by the Immortals.\n\r",ch);
        return;
    }

    if (!IS_NPC(ch) && IS_TAG(ch))
    {
        send_to_char("You cannot recall while playing Freeze Tag.\n\r",ch);
        return;
    }

    
    if (!IS_NPC(ch) && in_pkill_battle(ch))
    {
        send_to_char("You cannot recall during a pkill battle!\n\r",ch);
        return;
    }

    if (ch->pcdata != NULL && ch->pcdata->pkill_timer > 0)
    {
        send_to_char("Adrenaline is pumping!\n\r", ch);
        return;
    }

    /*
    if ( carries_relic(ch) )
    {
        send_to_char( "Not with a relic!\n\r", ch );
        return;
    }
    */

    god_name = get_god_name(ch);
    sprintf(buf, "$n prays to %s for transportation!", god_name);
    act( buf, ch, 0, 0, TO_ROOM );
    
    if (!IS_NPC(ch) && ch->pcdata->clan_rank>1)
        room=clan_table[ch->clan].hall;
    
    if ( ( location = get_room_index( room) ) == NULL )
    {
        send_to_char( "You are completely lost.\n\r", ch );
        return;
    }
    
    if (IS_NPC(ch)&&IS_SET(ch->act,ACT_PET)&&ch->master)
        location=ch->master->in_room;
    
    if ( ch->in_room == location )
    {
        send_to_char("You are already home.\n\r", ch);
        return;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) )
    {
        printf_to_char(ch, "%s has forsaken you.\n\r", god_name );
        return;
    }
    
    if ( IS_AFFECTED(ch, AFF_ENTANGLE) )
    {
        chance = (number_percent ());
        if (  (chance) >  (get_curr_stat(ch, STAT_LUC)/10) )
        {
            send_to_char( "The plants entangling you hold you in place!\n\r", ch );
            return;
        }
    }

    if ( IS_AFFECTED(ch, AFF_ROOTS) )
    {
        send_to_char( "Not with your roots sunk into the ground!\n\r", ch );
        return;
    }

    /* This vnum specification is for Bastion. Cheaper to recall from Bastion
       than other areas. Added to help newbies who get lost in our huge city. */

    if ( IS_NPC(ch) )
        move_cost = 0;
    else if ( ch->in_room->area->min_vnum == 10200 )
        move_cost = ch->level * 5/2;
    else
        move_cost = ch->level * 5;

    if ( ch->move < move_cost )
    {
	send_to_char("You are too tired to recall.\n\r", ch);
	return;
    }

    if ( ( victim = ch->fighting ) != NULL )
    {
        int lose,skill;
        
        skill = 50 + (get_curr_stat(ch, STAT_LUC) - get_curr_stat(victim, STAT_LUC)) / 5;

        if ( (lose = get_skill(victim, gsn_entrapment)) )
		{
			skill -= lose/3;
			check_improve(victim, gsn_entrapment, TRUE, 1);
		}

		if (victim->stance == STANCE_AMBUSH)
			skill /= 2;
        
        if (number_percent() > skill/3)
        {
            WAIT_STATE( ch, 6 );
            sprintf( buf, "%s doesn't answer your prayer.\n\r", god_name);
            send_to_char( buf, ch );
            return;
        }
    }

    /* Check for area triggers, they might prevent recall */
    if (!IS_NPC(ch) )
    {
        if ( !ap_recall_trigger(ch) )
            return;
        if ( !rp_exit_trigger(ch) )
            return;
        if ( !ap_rexit_trigger(ch) )
            return;
        if ( !ap_exit_trigger(ch, location->area) )
            return;
    }

    if (victim)
    {
        if (!IS_HERO(ch))
        {
            int lose = (ch->desc != NULL) ? 25 : 50;
            gain_exp( ch, 0 - lose );
            sprintf( buf, "You recall from combat!  You lose %d exps.\n\r", lose );
            send_to_char( buf, ch );
        }
        else
            send_to_char("You recall from combat!\n\r",ch);

        stop_fighting( ch, TRUE );
        
    }
    

    // misgate chance when cursed but not normally
    location = room_with_misgate(ch, location, 0);

    from_area=ch->in_room ? ch->in_room->area : NULL;
    

    ch->move -= move_cost;
    act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, location );
    act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );
    
    if (ch->pet != NULL)
        do_recall(ch->pet,"");
    
    if (!IS_NPC(ch) )
    {
        ap_enter_trigger(ch, from_area);
        ap_renter_trigger(ch);
        rp_enter_trigger(ch);
    }
    return;
}

// maximum remort race you can morph into
int morph_power( CHAR_DATA *ch )
{
    if ( IS_NPC(ch) )
        return -1;

    if ( ch->race == race_doppelganger || ch->race == race_naga || ch->race == race_werewolf )
        return ch->pcdata->remorts;

    if ( ch->race == race_rakshasa )
        return ch->pcdata->remorts - 1;

    return -1;
}

void dragonborn_rebirth( CHAR_DATA *ch )
{
    // safety-net
    if ( ch->race != race_dragonborn || IS_NPC(ch) )
    {
        bugf("dragonborn_rebirth: invalid character %s", ch->name);
        return;
    }
    
    // randomly chose a new color
    int morph_race = number_range(MORPH_DRAGON_RED, MORPH_DRAGON_WHITE);
    // ensure we don't get the same color a before
    while ( morph_race == ch->pcdata->morph_race )
        morph_race = number_range(MORPH_DRAGON_RED, MORPH_DRAGON_WHITE);
    
    logpf("dragonborn_rebirth: setting morph race for %s to %s", ch->name, morph_race_table[morph_race].name);
    ptc(ch, "You have been reborn into a %s.\n\r", morph_race_table[morph_race].name);
    ch->pcdata->morph_race = morph_race;
    ch->pcdata->morph_time = -1;
    morph_update(ch);
}

DEF_DO_FUN(do_morph)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int race;

	one_argument(argument, arg);

	if (IS_NPC(ch))
		send_to_char("You pretend you're a doppelganger.\n\r", ch);
	else if ( MULTI_MORPH(ch) )
	{
        if ( arg[0] == '\0' )
        {
            if ( ch->pcdata->morph_race )
            {
                WAIT_STATE(ch, PULSE_VIOLENCE);
                send_to_char("You revert to your original form.\n\r", ch);
                ch->pcdata->morph_time = 0;
                ch->pcdata->morph_race = 0;
                morph_update( ch );
            }
            else
                send_to_char("You must select a victim or race to emulate.\n\r", ch);
            return;
        }
        
        if ( (victim = get_char_room(ch, arg)) )
            race = victim->race;
        else
            race = pc_race_lookup(arg);
        
        if ( !race )
        {
            send_to_char("That's not a valid victim or race.\n\r", ch);
            return;
        }

        if ( race == ch->race )
        {
            send_to_char("You are already of that race.\n\r", ch);
            return;
        }

		if ( !race_table[race].pc_race || pc_race_table[race].remorts > morph_power(ch) )
		{
			sprintf(buf, "You can't morph into a %s.\n\r",
				race_table[race].name);
			send_to_char(buf, ch);
			return;
		}

        int base_cost = (pc_race_table[race].remorts + 5) * (20 + ch->level) * (victim ? 1 : 2);
        int mana_cost = base_cost * 0.4; // min mana factor for any class
        int move_cost = base_cost * 0.7; // min move factor for any class
        
        if ( ch->mana < mana_cost || ch->move < move_cost )
		{
			sprintf(buf,"You need %d mana and %d move to morph into a %s.\n\r",
                mana_cost, move_cost, race_table[race].name);
			send_to_char(buf, ch);
			return;
		}
		else
		{
			WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
			sprintf(buf, "You morph into a %s.\n\r", race_table[race].name);
			send_to_char(buf, ch);
            ch->mana -= mana_cost;
            ch->move -= move_cost;
			ch->pcdata->morph_race = race;
			ch->pcdata->morph_time = get_duration_by_type(DUR_EXTREME, ch->level);
			morph_update( ch );
		}
	}
	else if (ch->race == race_naga)
	{
		if (ch->pcdata->morph_race)
		{
		    WAIT_STATE(ch, PULSE_VIOLENCE);
		    send_to_char("You morph back into your natural serpent form.\n\r", ch);
		    ch->pcdata->morph_race = 0;
		    ch->pcdata->morph_time = 0;
		}
		else
		{
		    WAIT_STATE(ch, PULSE_VIOLENCE);
		    send_to_char("You morph into humanoid form.\n\r", ch);
		    ch->pcdata->morph_race = race_naga;
		    ch->pcdata->morph_time = 10;
		}
		morph_update( ch );
	}
	else if (ch->race == race_werewolf)
		send_to_char("You can't control your lycanthropy.\n\r", ch);
    else if ( ch->race == race_dragonborn )
    {
        if ( IS_AFFECTED(ch, AFF_CHARM) )
        {
            ptc(ch, "You cannot rebirth in your current state.\n\r");
            return;
        }
        
        if ( strcmp(arg, "rebirth") )
        {
            ptc(ch, "To rebirth into a different random color, type 'morph rebirth'.\n\r");
            ptc(ch, "{RWarning: This will cost you 1 train of hp/mana/move each.{x\n\r");
            return;
        }
        
        // need to be at full strength to rebirth
        if ( ch->hit < ch->max_hit
            || ch->pcdata->trained_hit < 1
            || ch->pcdata->trained_mana < 1
            || ch->pcdata->trained_move < 1 )
        {
            ptc(ch, "You must be at full health and sufficiently trained to survive a rebirth.\n\r");
            return;
        }
        
        // ok, good to go
        ptc(ch, "You feel your health, mental powers and endurance dwindle.\n\r");
        ch->pcdata->trained_hit--;
        ch->pcdata->trained_mana--;
        ch->pcdata->trained_move--;
        update_perm_hp_mana_move(ch);
        dragonborn_rebirth(ch);
        ch->hit = 1;
    }
	else
		send_to_char("You pretend you're a doppelganger.\n\r", ch);
}

/* update all stats that might have changed after morphing */
void morph_update( CHAR_DATA *ch )
{
    struct race_type *race = get_morph_race_type( ch );
    struct pc_race_type *pc_race = get_morph_pc_race_type( ch );

    if ( IS_NPC(ch) )
	return;

    if ( race == NULL || pc_race == NULL )
    {
	bugf( "morph_update: NULL race on %s", ch->name );
	return;
    }

    flag_copy( ch->form, race->form );
    flag_copy( ch->parts, race->parts );
    ch->size = pc_race->size;
    if ( IS_SET(ch->parts, PART_CLAWS) )
	ch->dam_type = 5; /* claw */
    else
	ch->dam_type = 17; /* punch */

    update_flags( ch );
    update_perm_hp_mana_move( ch );
}

void trap_damage( CHAR_DATA *ch, bool can_behead )
{
    ROOM_INDEX_DATA *was_in_room = ch->in_room;

    int dam_type = number_range(DAM_BASH, DAM_POISON);
    int dam = 100 + dice( ch->level, 20 );

    if ( can_behead )
    {
        // no beheading in remort - instead damage is increased
        if ( ch->pcdata && IS_REMORT(ch) )
            dam += dam * (ch->pcdata->remorts + 1) / 5;
        else if ( number_bits(4) == 0 )
        {
            act( "Your head is chopped right off!", ch, NULL, NULL, TO_CHAR );
            act( "$n's head is chopped right off!", ch, NULL, NULL, TO_ROOM );
            behead( ch, ch );
            return;
        }
    }
    
    full_dam( ch, ch, dam, 0, dam_type, FALSE);
    
    if ( ch->in_room != was_in_room )
	return;

    /* extra effect depending on damage type */
    switch ( dam_type )
    {
    case DAM_FIRE:
	fire_effect( ch, ch->level, dam, TARGET_CHAR );
	break;
    case DAM_COLD:
	cold_effect( ch, ch->level, dam, TARGET_CHAR );
	break;
    case DAM_LIGHTNING:
	shock_effect( ch, ch->level, dam, TARGET_CHAR );
	break;
    case DAM_ACID:
	acid_effect( ch, ch->level, dam, TARGET_CHAR );
	break;
    case DAM_POISON:
	poison_effect( ch, ch->level, dam, TARGET_CHAR );
	break;
    default:
	DAZE_STATE( ch, 6 * PULSE_VIOLENCE );
	break;
    }
}

EXIT_DATA* get_revers_exit( ROOM_INDEX_DATA *pRoom, int door, bool changed );

/* check if player is hit by an exit trap */
bool check_exit_trap_hit( CHAR_DATA *ch, int door, bool step_in )
{
    EXIT_DATA *exit, *rev_exit;
    ROOM_INDEX_DATA *in_room;
    int chance;

    if ( IS_NPC(ch) )
	return FALSE;

    if ( (in_room = ch->in_room) == NULL
	 || (exit = in_room->exit[door]) == NULL
	 || !IS_SET(exit->exit_info, EX_TRAPPED) )
	return FALSE;

    if ( step_in )
    {
	chance = get_skill( ch, gsn_avoidance ) / 3 + get_curr_stat( ch, STAT_LUC ) / 6;
	if ( number_percent() <= chance )
	{
	    act( "You narrowly avoid a trap!", ch, NULL, NULL, TO_CHAR );
	    act( "$n narrowly avoids a trap!", ch, NULL, NULL, TO_ROOM );
	    return FALSE;
	}

	act( "You step into a trap! Ouch, that HURT!!!", ch, NULL, NULL, TO_CHAR );
	act( "$n steps into a trap! Ouch, that HURT!!!", ch, NULL, NULL, TO_ROOM );
    }
    else
    {
	act( "You trigger a trap! Ouch, that HURT!!!", ch, NULL, NULL, TO_CHAR );
	act( "$n triggers a trap! Ouch, that HURT!!!", ch, NULL, NULL, TO_ROOM );
    }

    WAIT_STATE( ch, PULSE_VIOLENCE );

    /* remove the trap */
    REMOVE_BIT( exit->exit_info, EX_TRAPPED );
    /* on revers exit too */
    if ( (rev_exit = get_revers_exit(in_room, door, FALSE)) != NULL )
	REMOVE_BIT( rev_exit->exit_info, EX_TRAPPED );
    
    /* damage */
    trap_damage( ch, step_in );

    return TRUE;
}

/* check if player is hit by an item trap */
bool check_item_trap_hit( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( IS_NPC(ch) )
	return FALSE;

    if ( obj == NULL || !IS_SET(obj->extra_flags, ITEM_TRAPPED) )
	return FALSE;

    act( "You trigger a trap on $p! Ouch, that HURT!!!", ch, obj, NULL, TO_CHAR );
    act( "$n triggers a trap on $p! Ouch, that HURT!!!", ch, obj, NULL, TO_ROOM );
    WAIT_STATE( ch, PULSE_VIOLENCE );

    /* remove the trap */
    REMOVE_BIT( obj->extra_flags, ITEM_TRAPPED );
    
    /* damage */
    trap_damage( ch, FALSE );

    return TRUE;
}

DEF_DO_FUN(do_disarm_trap)
{
    int door = 0, skill;
    EXIT_DATA *exit = NULL, *rev_exit;
    OBJ_DATA *obj;
    bool disarm_exit;

    if ( (skill = get_skill(ch, gsn_disarm_trap)) < 1 )
    {
	send_to_char( "You don't know how to disarm traps.\n\r", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Which trap do you want to disarm?\n\r", ch );
	return;
    }

    /* which item or exit are we disarming? */
    if ( (obj = get_obj_here(ch, argument)) == NULL )
    {
        disarm_exit = TRUE;

        /* is argument a direction? */
        door = find_door( ch, argument );
        if ( door < 0 )
            return;

        if ( (exit = ch->in_room->exit[door]) == NULL )
        {
            bugf( "do_disarm_trap: NULL exit %d in room %d",
            door, ch->in_room->vnum );
            return;
        }

        if ( !IS_SET(exit->exit_info, EX_TRAPPED) )
        {
            send_to_char( "That exit isn't trapped.\n\r", ch );
            return;
        }
    }
    else
    {
        disarm_exit = FALSE;
        
        if ( !IS_SET(obj->extra_flags, ITEM_TRAPPED) )
        {
            act( "$p isn't trapped.", ch, obj, NULL, TO_CHAR );
            return;
        }
    }

    /* ok, we have a trap to disarm */ 
    WAIT_STATE( ch, skill_table[gsn_disarm_trap].beats );
    if ( number_percent() > skill )
    {
	send_to_char( "You failed to disarm the trap.\n\r", ch );
	return;
    }

    /* remove the trap */
    if ( disarm_exit )
    {
	REMOVE_BIT( exit->exit_info, EX_TRAPPED );
	/* on revers exit too */
	if ( (rev_exit = get_revers_exit(ch->in_room, door, FALSE)) != NULL )
	    REMOVE_BIT( rev_exit->exit_info, EX_TRAPPED );
    }
    else
    {
	REMOVE_BIT( obj->extra_flags, ITEM_TRAPPED );
    }

    act( "You carefully disarm the trap.", ch, NULL, NULL, TO_CHAR );
    act( "$n carefully disarms a trap.", ch, NULL, NULL, TO_ROOM );
}


DEF_DO_FUN(do_root)
{
    char arg[MIL];
    AFFECT_DATA af;

    argument = one_argument( argument, arg );

    if ( !strcmp(arg, "remove") )
    {
	if ( !IS_AFFECTED(ch, AFF_ROOTS) )
	{
	    send_to_char( "You haven't sunk your roots into the ground.\n\r", ch );
	    return;
	}
	affect_strip_flag( ch, AFF_ROOTS );
	act( "You twist your roots out of the ground.", ch, NULL, NULL, TO_CHAR );
	act( "$n twists $s roots out of the ground.", ch, NULL, NULL, TO_ROOM );
	WAIT_STATE( ch, PULSE_VIOLENCE );
	return;
    }

    if ( strcmp(arg, "grow") )
    {
	send_to_char( "Syntax: root [grow|remove]\n\r", ch );
	return;
    }

    if ( !IS_SET(ch->form, FORM_PLANT) )
    {
	send_to_char( "You're not a plant.\n\r", ch );
	return;
    }

    switch( ch->in_room->sector_type )
    {
    case SECT_INSIDE:
    case SECT_CITY:
    case SECT_WATER_DEEP:
    case SECT_UNDERWATER:
    case SECT_AIR:
	send_to_char( "You can't sink your roots into the ground here.\n\r", ch );
	return;
    default:
	break;
    }

    if ( IS_AFFECTED(ch, AFF_ROOTS) )
    {
	send_to_char( "You have already sunk your roots into the ground.\n\r", ch );
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_entrapment; // need SOME skill..
    af.level     = ch->level;
    af.duration  = -1;
    af.location  = APPLY_AGI;
    af.modifier  = -10;
    af.bitvector = AFF_ROOTS;
    affect_to_char( ch, &af );
    
    act( "You sink your roots into the ground.", ch, NULL, NULL, TO_CHAR );
    act( "$n sinks $s roots into the ground.", ch, NULL, NULL, TO_ROOM );
    WAIT_STATE( ch, PULSE_VIOLENCE );
}

/* checks if a character loses blood */


void check_bleed( CHAR_DATA *ch, int dir )
{
    OBJ_DATA *blood;
    char buf[MSL];

    if ( ch == NULL || ch->in_room == NULL )
	return;

    if ( IS_IMMORTAL(ch)
	 || NPC_ACT(ch, ACT_OBJ)
	 || NPC_ACT(ch, ACT_WIZI)
	 || IS_UNDEAD(ch)
	 || IS_SET(ch->form, FORM_PLANT) 
	 || IS_AFFECTED(ch, AFF_ASTRAL) )
	return;

    if ( ch->in_room->sector_type == SECT_AIR )
	return;

    int hp_below_min = ch->max_hit/4 - ch->hit;
    if ( !is_affected(ch,gsn_rupture) && (hp_below_min <= 0 || saves_physical(ch, NULL, hp_below_min/10, DAM_OTHER)) )
        return;

    /* create blood object */
    if ( (blood = create_object_vnum(OBJ_VNUM_BLOOD)) == NULL )
        return;

    /* add direction hint */
    sprintf( buf, "{rA trail of blood leads %s.{x", dir_name[dir] );
    free_string( blood->description );
    blood->description = str_dup( buf );
    /* shouldn't lie around for all time */
    blood->timer = 10;
    obj_to_room( blood, ch->in_room );
    send_to_char( "{rYou leave a trail of blood.{x\n\r", ch );
}




bool explored_vnum(CHAR_DATA *ch, int vnum)
{	
	int mask = vnum / 32; //Get which bucket the bit is in
	int bit = vnum % 32; //Get which bit to set, 0 to 31
	EXPLORE_HOLDER *pExp; //The buckets bucket.
#ifdef EXPLORE_DEBUG
	send_to_char("explored_vnum: start\n\r",ch);
	printf_to_char(ch,"mask: %d\n\rbit: %d\n\r", mask,bit);
#endif
	
	for(pExp = ch->pcdata->explored->buckets ; pExp ; pExp = pExp->next ) //Iterate through the buckets
	{	
		if(pExp->mask != mask)
			continue;
		//Found the right bucket, might be explored.
		if ( ( ( pExp->bits >> bit) & 1 ) == 1 ) 
		{
#ifdef EXPLORE_DEBUG
			send_to_char("IS_SET_EXPLORE returned true\n\r",ch);
#endif
			return TRUE;
		}
		return FALSE; //Return immediately. This value wont be in any other bucket.
	}
	return FALSE;
#ifdef EXPLORE_DEBUG
	send_to_char("explored_vnum: finish\n\r",ch);
#endif
}
//Explore a vnum. Assume it's not explored and just set it.
void explore_vnum(CHAR_DATA *ch, int vnum )
{	int mask = vnum / 32; //Get which bucket it will be in
	int bit = vnum % 32; // Get which bit to set, 0 to 31
	EXPLORE_HOLDER *pExp; //The buckets bucket.

#ifdef EXPLORE_DEBUG
	send_to_char("explore_vnum: start\n\r",ch);
	printf_to_char(ch,"mask: %d\n\rbit: %d\n\r", mask,bit);
#endif

	//Find the bucket.
	for(pExp = ch->pcdata->explored->buckets ; pExp ; pExp = pExp->next )
		if(pExp->mask == mask)
			break;

	if(!pExp) //If it's null, bucket not found, we'll whip one up.
	{
#ifdef EXPLORE_DEBUG	
		send_to_char("creating pExp\n\r", ch);
#endif
		pExp = (EXPLORE_HOLDER *)calloc(sizeof(*pExp), 1); //Alloc and zero
		pExp->mask = mask;
		pExp->next = ch->pcdata->explored->buckets; //Add to
		ch->pcdata->explored->buckets = pExp;       //the list
	}
	
	pExp->bits = pExp->bits | ( 1 << bit) ;
	ch->pcdata->explored->set++; //Tell how many rooms we've explored
	update_lboard( LBOARD_EXPL, ch, ch->pcdata->explored->set, 1);
#ifdef EXPLORE_DEBUG
	send_to_char("explore_vnum: finish\n\r",ch);
#endif
}


//Explore a vnum.
void check_explore( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoom )
{	
	if(IS_NPC(ch) ) return;
#ifdef EXPLORE_DEBUG
	send_to_char("check_explore: start\n\r",ch);
#endif
	
	if(explored_vnum(ch, pRoom->vnum) )
		return;

	explore_vnum(ch, pRoom->vnum);
#ifdef EXPLORE_DEBUG
	send_to_char("check_explore: finish\n\r",ch);
#endif
}

DEF_DO_FUN(do_explored)
{	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "You have explored %d room%s!{x\r\n", ch->pcdata->explored->set, (ch->pcdata->explored->set == 1 ? "" : "s") );
	send_to_char(buf,ch);

//This shows all the rooms they've explored. Probably don't need mortals seeing this, and for immortals... it'd get really spammy. Mostly an example.
/*	for(pExp = ch->pcdata->explored->bits ; pExp ; pExp = pExp->next )
	{	for(bit = 1 ; bit <= 32 ; ++bit )
		{	if(IS_SET(pExp->bits, (1 << bit) ) )
			{	sprintf(buf, "[%-5d]", (pExp->mask * 32 + bit) );
				send_to_char(buf,ch);
			}
		}
		send_to_char("\r\n",ch);
	}
*/
	return;
}

//RUN COMMAND by SIVA 2/14/04
DEF_DO_FUN(do_run)
{
	//local variables
	CHAR_DATA *wch;
	const char *p;	//pointer to iterate argument
	int i; //counter for for loops
	int last = 0; //holds last character of argument
	int par_counter = 0; // for turning chars into ints	
	int move_counter = 0;
	char nbr_parser[3] = "\0";

	p = argument;
	
	//Not for the pkillers! (change this if flag is different)
	if (IS_SET (ch->act, PLR_KILLER))
		{
		send_to_char("Running is for cowards. Killers stand and fight!\r\n", ch);
		WAIT_STATE(ch, 8);
		return;
		}

	if (IS_SET (ch->act, PLR_THIEF))
		{
		send_to_char("The weight of your crimes prevents you from running.\r\n", ch);
		WAIT_STATE(ch, 8);
		return;
		}

	//add similar check to prevent use during warfare!
	//add similar check to prevent use during freeze tag!

	//do we have valid string of input?
	if (argument[0] == '\0')
		{
		send_to_char("Run where?\r\n", ch);
		return;
		}
	else if (strlen (argument) > MAX_INPUT_LENGTH)
		{
		send_to_char("Requested path too long. Try something shorter.\r\n", ch);
		return;
		}

	//make sure argument is valid
	for (i=0; p[i]; i++)
		{
			switch(p[i]) {
			case 'n':
			case 's':
			case 'e':
			case 'w':
			case 'u':
			case 'd':
			case 'N':
			case 'S':
			case 'E':
			case 'W':
				{
				if (nbr_parser[0] != '\0') 
					{
					strcpy (nbr_parser, "\0");
					}
				break;
				}
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				{
				if (nbr_parser[0] == '\0') par_counter = 0;
				nbr_parser[par_counter] = p[i];
				par_counter += 1;
				if ( (atoi(nbr_parser) > 99) || (atoi(nbr_parser) < 1) )
					{
					send_to_char("Only numbers 1 through 99 are acceptable for \"run\".\r\n", ch);
					return;
					}
				break;
				}
			default:
				{
				send_to_char("Invalid run directions. Try again.\r\n", ch);
				return;
				}
			} //end switch
		last = i;	
		} //end for loop

	//If last character of argument is a number, that doesn't work either!
	if ( isdigit(p[last]) )
				{
				send_to_char("A number must be followed by a direction for \"run\".\r\n", ch);
				return;
				}

	//reset number parser for actual movements
	par_counter = 0;
	nbr_parser[0] = '\0';
	nbr_parser[1] = '\0';
		
	//execute the run movements
	for (i = 0; p[i]; i++)
	{
		//make a number from the string
		if (isdigit(p[i])) 
		{
			nbr_parser[par_counter] = p[i];
			par_counter++;
			continue;
		}
		move_counter = 1;

		if (	(!isdigit(p[i])) && (nbr_parser[0] != '\0')	) 
			{
			move_counter = (atoi(nbr_parser));
			nbr_parser[0] = '\0';
			nbr_parser[1] = '\0';
			par_counter = 0;
			}
			
		
			for ( ; move_counter > 0; move_counter--)
				{
				if (p[i] == 'n') do_north(ch, NULL);
				else if (p[i] == 's') do_south(ch, NULL);
				else if (p[i] == 'e') do_east(ch, NULL);
				else if (p[i] == 'w') do_west(ch, NULL);
				else if (p[i] == 'u') do_up(ch, NULL);
				else if (p[i] == 'd') do_down(ch, NULL);
				else if (p[i] == 'd') do_down(ch, NULL);
				else if (p[i] == 'N') 
					{
					if (p[i+1] == 'E') do_northeast(ch, NULL);
					else if (p[i+1] == 'W') do_northwest(ch, NULL);
					}
				else if (p[i] == 'S') 
					{
					if (p[i+1] == 'E') do_southeast(ch, NULL);
					else if (p[i+1] == 'W') do_southwest(ch, NULL);
					}

				//check for aggro mobs, can't run past them!
				for (wch = ch->in_room->people; wch != NULL; wch = wch->next_in_room)
					{
					if (IS_SET (wch->act, ACT_AGGRESSIVE) 
					&& ch->level < LEVEL_IMMORTAL
					&& ch->level <= wch->level - 5
					&& IS_AWAKE (wch)
					&& can_see (wch, ch)   )
						{
						send_to_char("{yPotential aggression stops you in your tracks!{x\r\n", ch);
						return;
						}
					} //end of aggro for
				WAIT_STATE(ch, 8);
				
				} //end of for
		
	} //end of big for
	return;
} // end of run function

