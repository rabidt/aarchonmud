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
#include "merc.h"
#include "tables.h"
#include <malloc.h>

/* command procedures needed */
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_recall    );
DECLARE_DO_FUN(do_stand     );
void  one_hit   args(( CHAR_DATA *ch, CHAR_DATA *victim, int dt, bool secondary ));
bool  in_pkill_battle args( ( CHAR_DATA *ch ) );
bool check_exit_trap_hit( CHAR_DATA *ch, int door, bool step_in );
bool check_item_trap_hit( CHAR_DATA *ch, OBJ_DATA *obj );
void morph_update( CHAR_DATA *ch );
void check_bleed( CHAR_DATA *ch, int dir );


char *  const   dir_name    []      =
{
   "north", "east", "south", "west", "up", "down", "northeast", "southeast",
   "southwest", "northwest"
};

const   sh_int  rev_dir     []      =
{
   2, 3, 0, 1, 5, 4, 8, 9, 6, 7
};

const   sh_int  movement_loss   [SECT_MAX]  =
{
   1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6
};



/*
* Local functions.
*/
int find_door   args( ( CHAR_DATA *ch, char *arg ) );
bool    has_key     args( ( CHAR_DATA *ch, int key ) );
bool check_drown args((CHAR_DATA *ch));

/* returns the direction the char moved to or -1 if he didn't move */
int move_char( CHAR_DATA *ch, int door, bool follow )
{
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    CHAR_DATA *old_singer;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    bool singing_in_room;
    char buf[MAX_STRING_LENGTH];
    int chance, d, inwater, towater;
    
    if ( door < 0 || door >= MAX_DIR )
    {
        bug( "Do_move: bad door %d.", door );
        return -1;
    }
    
    in_room = ch->in_room;

    if ( ch->in_room == NULL )
    {
	bugf( "move_char: NULL room" );
	return -1;
    }

    if (IS_AFFECTED(ch, AFF_INSANE) && number_bits(1))
        for ( chance = 0; chance < 8; chance++ )
        {  
            d = number_door( );
            if ( ( pexit = in_room->exit[d] ) == 0
                ||   pexit->u1.to_room == NULL
                ||   IS_SET(pexit->exit_info, EX_CLOSED)
                || ( IS_NPC(ch)
                &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) 
        /* Check added so that mobs can't flee into a safe room. Causes problems
           with resets, quests, and leveling - Astark Dec 2012 */
                ||   IS_SET(pexit->u1.to_room->room_flags, ROOM_SAFE) ) ) 
                continue;
            door=d;
            break;
        }
        
   /*
    * Exit trigger, if activated, bail out. Only PCs are triggered.
    */
    if ( !IS_NPC(ch) && mp_exit_trigger( ch, door ) )
    {
	/* exit trigger might trans char to target room */
	if ( in_room->exit[door] != NULL
	     && ch->in_room == in_room->exit[door]->u1.to_room )
	    return door;
	else
	    return -1;
    }
        
    in_room = ch->in_room;
    if ( ( pexit   = in_room->exit[door] ) == NULL
        ||   ( to_room = pexit->u1.to_room   ) == NULL 
        ||   !can_see_room(ch,pexit->u1.to_room))
    {
        send_to_char( "Alas, you cannot go that way.\n\r", ch );
        return -1;
    }
        
    if (!IS_NPC(ch) && IS_SET(ch->pcdata->tag_flags, TAG_FROZEN) && IS_TAG(ch))
    {
        send_to_char( "You've been frozen, you can't move!\n\r", ch );
        return -1;
    }

    if (IS_SET(ch->penalty, PENALTY_JAIL))
    {
        send_to_char("You are chained to the floor.\n\r",ch);
        return -1;
    }
        
    if ( IS_AFFECTED(ch, AFF_ROOTS) )
    {
	send_to_char( "Your roots prevent you from moving.\n\r", ch );
	return -1;
    }

    if (IS_SET(pexit->exit_info, EX_CLOSED)
        &&  (!IS_AFFECTED(ch, AFF_PASS_DOOR) || IS_SET(pexit->exit_info,EX_NOPASS))
        &&   !IS_TRUSTED(ch,LEVEL_IMMORTAL))
    {
        act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
        return -1;
    }
        
    if ( IS_AFFECTED(ch, AFF_CHARM)
        &&   ch->master != NULL
        &&   in_room == ch->master->in_room )
    {
        send_to_char( "What?  And leave your beloved master?\n\r", ch );
        return -1;
    }

    if ( IS_SET( to_room->room_flags, ROOM_BOX_ROOM))
    {
        if (IS_NPC(ch))
          return -1;
        if (ch->pcdata->storage_boxes<1)
        {
            send_to_char("You have no business in there.\n\r",ch);
            return -1;
        }
    }
        
    if ( !is_room_owner(ch,to_room) && room_is_private( to_room ) )
    {
        send_to_char( "That room is private right now.\n\r", ch );
        return -1;
    }
        
    if ( !IS_NPC(ch) )
    {
        int iClass, iGuild;
        int move, waitpulse;
        int can_go=0;
        int cant_go=0;
          
        for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        {
            for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)    
            {
                if ( to_room->vnum == class_table[iClass].guild[iGuild] )
                {
                    if(iClass==ch->class)
                        can_go=1;
                    else
                        cant_go=1;
                }
            }
        }

            
        if ((cant_go==1) && (can_go==0))
        {
            send_to_char("You aren't allowed in there.\n\r", ch);
            return -1;
        }
            
        if ( !IS_IMMORTAL(ch) && !(IS_NPC(ch)&&IS_SET(ch->act,ACT_PET)))     
            if ( to_room->clan > 0)
            {
                if ( ch->clan != to_room->clan )
                {
                    printf_to_char (ch, "That area is for clan %s only.\n\r",
                        capitalize(clan_table[to_room->clan].name));
                    return -1;
                }
                
                if (!IS_NPC(ch) && to_room->clan_rank>0 && ch->pcdata->clan_rank<to_room->clan_rank)
                {
                    printf_to_char (ch, "That area is for %ss of clan %s only.\n\r",
                        capitalize(clan_table[to_room->clan].rank_list[to_room->clan_rank].name),
                        capitalize(clan_table[to_room->clan].name));
                    return -1;
                }
            }
                
        if ( in_room->sector_type == SECT_AIR
            ||   to_room->sector_type == SECT_AIR )
        {
            if ( !IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
            {
                send_to_char( "You can't fly.\n\r", ch );
                return -1;
            }
        }

	if ( check_exit_trap_hit(ch, door, TRUE) )
	    return;
	
	if (!IS_IMMORTAL(ch))
	{
	    if ((in_room->sector_type == SECT_WATER_DEEP) ||
		(in_room->sector_type == SECT_UNDERWATER))
		inwater = in_room->sector_type;
	    else
		inwater = 0;
	    
	    if ((to_room->sector_type == SECT_WATER_DEEP) ||
		(to_room->sector_type == SECT_UNDERWATER))
		towater = to_room->sector_type;
	    else
		towater = 0;

	    if ((inwater == 0) && (to_room->sector_type == SECT_WATER_SHALLOW) &&
		IS_SET(ch->vuln_flags, VULN_DROWNING) &&
		!IS_AFFECTED(ch, AFF_FLYING))
	    {
		OBJ_DATA *obj;
		
		for (obj=ch->carrying; obj; obj=obj->next_content)
		    if (obj->item_type == ITEM_BOAT)
			break;
		if (obj==NULL)
		{
		    send_to_char("You wade into the water despite your phobia.\n\r", ch);
		    if (check_drown(ch)) return -1;
		}
	    }

	    while (inwater || towater)
	    {
		if ((inwater < SECT_UNDERWATER) && (towater < SECT_UNDERWATER))
		    {
			OBJ_DATA *obj;
			if (IS_AFFECTED(ch, AFF_FLYING)) break;
			
			for (obj=ch->carrying; obj; obj=obj->next_content)
			    if (obj->item_type == ITEM_BOAT)
				break;
			if (obj) break;
		    }

			chance = get_skill(ch, gsn_swimming);
			if (IS_SET(ch->vuln_flags, VULN_DROWNING))
				chance -= 15;

			if (number_percent()*2>(chance+100))
			{
			    send_to_char("You paddle around and get nowhere.\n\r", ch);
			    if (inwater == SECT_UNDERWATER)
				check_drown(ch);                
			    return -1;
			}
			if ((inwater<SECT_UNDERWATER) && (towater<SECT_UNDERWATER))
			if (number_percent()>chance)
			{
			    send_to_char("You fail to keep your head above water.\n\r", ch);
			    if (check_drown(ch)) return -1;
			    check_improve(ch,gsn_swimming,FALSE,2);  
			}
			else
			{
			    check_improve(ch,gsn_swimming,TRUE,3);
			    if (IS_SET(ch->vuln_flags, VULN_DROWNING))
			    {
				send_to_char("You hate water!\n\r", ch);
				if (check_drown(ch)) return -1;
			    }
			}
			break;
		}

		if (((inwater == SECT_UNDERWATER) || (towater == SECT_UNDERWATER))
				&& !IS_AFFECTED(ch, AFF_BREATHE_WATER))
		{
			send_to_char("You cant breathe!\n\r", ch);
			if (check_drown(ch)) return -1;
		}
		}       
                
        move = 2*(movement_loss[UMIN(SECT_MAX-1, in_room->sector_type)]
            + movement_loss[UMIN(SECT_MAX-1, to_room->sector_type)])/3;
        
		if (((in_room->sector_type==SECT_MOUNTAIN) ||
				(in_room->sector_type==SECT_HILLS) ||
				(to_room->sector_type==SECT_MOUNTAIN) ||
				(to_room->sector_type==SECT_HILLS))
			&& (get_skill(ch, gsn_climbing) > (number_percent()/2))
			&& (!IS_AFFECTED(ch,AFF_FLYING)))
            {
                if (in_room->sector_type == SECT_MOUNTAIN) 
                    move -=2;
		else if (in_room->sector_type == SECT_HILLS)
		    move -=1;

                if (to_room->sector_type == SECT_MOUNTAIN) 
                    move -=2;
		else if (to_room->sector_type == SECT_HILLS)
		    move -=1;

                check_improve(ch,gsn_climbing,TRUE,6);  
            }
                    
        waitpulse=2+move/6;

	if (IS_AFFECTED(ch,AFF_SLOW))
	{
	    move *=2;
	    waitpulse+=2;
	}

	if (ch->slow_move > 0)  /* For when a foot is chopped off */
	{
	    move = 2 * (move + 1);
	    waitpulse = 2 * (waitpulse + 1);
	}

	/* encumberance */
	{
	    int encumber = get_encumberance( ch );
	    move += (move * encumber + 99) / 100;
	    waitpulse += (waitpulse * encumber + 99) / 100;
	}

        if (IS_AFFECTED(ch,AFF_FLYING) || IS_AFFECTED(ch,AFF_HASTE))
            waitpulse/=2;

        if ( ch->move < move )
        {
            send_to_char( "You are too exhausted.\n\r", ch );
            return -1;
        }
                 
	WAIT_STATE(ch, waitpulse);
        ch->move -= move;
    }
    
    if ( IS_AFFECTED(ch, AFF_HIDE ) && !IS_AFFECTED(ch, AFF_SNEAK) )
    {
        affect_strip( ch, gsn_hide );
        REMOVE_BIT( ch->affect_field, AFF_HIDE );
        send_to_char( "You come out of hiding.\n\r", ch );
    }
   
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
		chance = get_skill(ch, gsn_sneak) - get_skill(fch, gsn_alertness)/3;
		if ( number_percent() > chance )
		    act( buf, ch, NULL, fch, TO_VICT );
	    }
	}
    }
    
    old_singer = in_room->singer;
    singing_in_room = (old_singer != NULL) && (to_room->singer == NULL)
        && (to_room != in_room);
    
    if (singing_in_room)
       in_room->singer = NULL;
   
    /* check fighting - no cross-room combat */
    if ( in_room != to_room )
    {
	stop_fighting( ch, TRUE );
	update_room_fighting( in_room );
	check_bleed( ch, door );
    }

/*storage box stuff*/
/*put this in char_to_room func
   if ( IS_SET(ch->in_room->room_flags, ROOM_BOX_ROOM) && ch->pcdata->storage_boxes>0)
   {
        quit_save_storage_box(ch);
	send_to_char( "As you leave the room, an employee takes your boxes back down to the basement.\n\r",ch);
 put this in quit_save_storage_box(ch)
	int i;
	for (i=0;i<ch->pcdata->storage_boxes;i++)
	{
	    extract_obj(ch->pcdata->box_data[i]);
	    ch->pcdata->box_data[i]=NULL;
	}

    }*/
	


    char_from_room( ch );
    char_to_room( ch, to_room );
   
   
    if (ch->song_singing != song_null)
    {
	sprintf( buf, "%s has arrived, singing %s.\n\r",
		 ch->name, skill_table[ch->song_singing].msg_off);
	act(buf, ch, NULL, NULL, TO_ROOM);
    }
    else if ( !IS_AFFECTED(ch, AFF_ASTRAL) && ch->invis_level < LEVEL_HERO )
    {
	if ( !IS_AFFECTED(ch, AFF_SNEAK) && !IS_TAG(ch) )
	    act( "$n has arrived.", ch, NULL, NULL, TO_ROOM );
	else
	{
	    for ( fch = to_room->people; fch != NULL; fch = fch_next )
	    {
		fch_next = fch->next_in_room;
		chance = get_skill(ch, gsn_sneak) - get_skill(fch, gsn_alertness)/3;
		if ( number_percent() > chance )
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
       
       if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM) 
           &&   fch->position < POS_STANDING)
           do_stand(fch,"");
       
       if ( fch->master == ch && fch->position == POS_STANDING 
           &&   can_see_room(fch,to_room))
       {
           
           if (IS_SET(ch->in_room->room_flags,ROOM_LAW)
               &&  (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE)))
           {
               act("You can't bring $N into the city.",
                   ch,NULL,fch,TO_CHAR);
               act("You aren't allowed in the city.",
                   fch,NULL,NULL,TO_CHAR);
               continue;
           }
           
           act( "You follow $N.", fch, NULL, ch, TO_CHAR );
           move_char( fch, door, TRUE );
       }
   }
   
   if (singing_in_room)
   {
       in_room->singer = old_singer;
       if (old_singer->in_room == to_room)
       {
           old_singer->in_room = in_room;
           song_from_room(old_singer);
           old_singer->in_room = to_room;
           song_to_room(old_singer);
       }
       else
       {
           old_singer->in_room = to_room;
           to_room->singer = old_singer;
           song_from_room(old_singer);
           old_singer->in_room = in_room;
       }
   }
   
   /* 
   * If someone is following the char, these triggers get activated
   * for the followers before the char, but it's safer this way...
   */
   if ( IS_NPC( ch ) && HAS_TRIGGER( ch, TRIG_ENTRY ) )
       mp_percent_trigger( ch, NULL, NULL, NULL, TRIG_ENTRY );
   if ( !IS_NPC( ch ) )
       mp_greet_trigger( ch );

   /* mprog might have moved the char */
   if ( ch->in_room != to_room )
       return door;

   /* storage box stuff */
//   if ( IS_SET(ch->in_room->room_flags, ROOM_BOX_ROOM))  
//	load_storage_boxes(ch);
//put this in char_to_room func   
   /*checks for a snare -- Siva*/
   if (IS_SET(ch->in_room->room_flags,ROOM_SNARE))
   {
       chance = 0;
       chance += get_curr_stat(ch,STAT_AGI)/6;
       chance += get_curr_stat(ch,STAT_DEX)/10;
       chance += get_skill(ch, gsn_avoidance)/5;
       chance += (get_skill(ch, gsn_duck)/5);
       chance += (get_skill(ch, gsn_dodge)/10);
       if (number_percent () < chance)
       {
           send_to_char ("You narrowly avoid a deadly snare!\n\r", ch);
           REMOVE_BIT(ch->in_room->room_flags, ROOM_SNARE );   
       }
       else
       {
           in_room=ch->in_room;
           send_to_char ("You are caught in a snare!\n\r", ch);
           act( "$n is caught in a deadly snare!", ch, NULL, NULL, TO_ROOM );
           for ( door = 0; door<MAX_DIR; door++ )
           {
               if ( ( pexit = in_room->exit[door] ) != NULL
                   &&   pexit->u1.to_room != NULL
                   &&   pexit->u1.to_room != in_room )
               {
                   ch->in_room = pexit->u1.to_room;
                   act( "$n is caught in a nearby snare!", ch, NULL, NULL, TO_ROOM );
               }
           }
           DAZE_STATE(ch, 6 * PULSE_VIOLENCE);
           set_pos( ch, POS_RESTING );
           ch->in_room=in_room;
           REMOVE_BIT(ch->in_room->room_flags, ROOM_SNARE );  
       }
   }
   
   /* Check for peels, Tryste */
   if (IS_SET(ch->in_room->room_flags,ROOM_PEEL) && !IS_AFFECTED( ch, AFF_FLYING ))
   {
       chance = 0;   
       chance += get_curr_stat(ch,STAT_AGI)/6;
       chance += get_curr_stat(ch,STAT_DEX)/10;
       chance += get_skill(ch, gsn_avoidance)/5;
       chance += (get_skill(ch, gsn_dodge)/10);  
       if (number_percent () < chance)
       {
           send_to_char ("You step over a banana peel!\n\r", ch);
       }
       else
       {
           in_room=ch->in_room;
           send_to_char ("You slip on a banana peel!\n\r", ch);
           act( "$n slips on a banana peel! Point and Laugh!", ch, NULL, NULL,TO_ROOM);
           for ( door = 0; door<MAX_DIR; door++ )
           {
               if ( ( pexit = in_room->exit[door] ) != NULL
                   &&   pexit->u1.to_room != NULL   
                   &&   pexit->u1.to_room != in_room )
               {
                   ch->in_room = pexit->u1.to_room;
                   act( "$n slipped on a banana peel nearby!", ch, NULL, NULL, TO_ROOM);
               }
           }
           DAZE_STATE(ch, 6 * PULSE_VIOLENCE);   
           set_pos( ch, POS_SITTING );
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
	    && number_percent() <= get_skill(fch, gsn_ambush)
	    && !is_safe_spell(fch, ch, TRUE)
	    && (IS_NPC(fch) || fch->hunting==NULL || is_name(fch->hunting, ch->name)))
       {
           check_improve(fch, gsn_ambush, TRUE, 2);
	   backstab_char( fch, ch );
	   if (fch->fighting == ch)
	       multi_hit(fch, ch, TYPE_UNDEFINED);
	   /*
	   if ( check_see(ch, fch) && number_bits(1) )
	   {
	       act( "$N spots you!", fch, NULL, ch, TO_CHAR );
	       damage( fch, ch, 0, gsn_backstab, DAM_NONE, TRUE);
	   }
	   else
	   {
	       one_hit(fch, ch, gsn_backstab, FALSE);
	       if (get_eq_char( fch, WEAR_SECONDARY ) != NULL)
		   one_hit(fch, ch, gsn_backstab, TRUE);
	       if ( !stop_attack(fch, ch) )
		   check_assassinate( fch, ch, get_eq_char(fch, WEAR_WIELD), 4 );
	       if (fch->fighting == ch)
		   multi_hit(fch, ch, TYPE_UNDEFINED);
	   }
	   */
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


void do_north( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_NORTH, FALSE );
   return;
}



void do_east( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_EAST, FALSE );
   return;
}



void do_south( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_SOUTH, FALSE );
   return;
}



void do_west( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_WEST, FALSE );
   return;
}



void do_up( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_UP, FALSE );
   return;
}



void do_down( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_DOWN, FALSE );
   return;
}

void do_northeast( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_NORTHEAST, FALSE );
   return;
}

void do_southeast( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_SOUTHEAST, FALSE );
   return;
}

void do_southwest( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_SOUTHWEST, FALSE );
   return;
}

void do_northwest( CHAR_DATA *ch, char *argument )
{
   move_char( ch, DIR_NORTHWEST, FALSE );
   return;
}


int find_door( CHAR_DATA *ch, char *arg )
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
		 && IS_SET(pexit->exit_info, EX_ISDOOR) )
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
			&&   is_name( arg, pexit->keyword ) )
			return door;
	  }
	  act( "I see no $T here.", ch, NULL, arg, TO_CHAR );
	  return -1;
   }
   
   if ( ( pexit = ch->in_room->exit[door] ) == NULL )
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



void do_open( CHAR_DATA *ch, char *argument )
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
   
   if( !str_prefix(arg,"nort") )
	{ do_open(ch,"north"); return; }
   if( !str_prefix(arg,"sout") )
	{ do_open(ch,"south"); return; }
   if( !str_prefix(arg,"eas") )
	{ do_open(ch,"east"); return; }
   if( !str_prefix(arg,"wes") )
	{ do_open(ch,"west"); return; }
   if( !str_cmp(arg,"nw") )
	{ do_open(ch,"northwest"); return; }
   if( !str_cmp(arg,"ne") )
	{ do_open(ch,"northeast"); return; }
   if( !str_cmp(arg,"se") )
	{ do_open(ch,"southeast"); return; }
   if( !str_cmp(arg,"sw") )
	{ do_open(ch,"southwest"); return; }

   if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
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



void do_close( CHAR_DATA *ch, char *argument )
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
   
   if( !str_prefix(arg,"nort") )
	{ do_close(ch,"north"); return; }
   if( !str_prefix(arg,"sout") )
	{ do_close(ch,"south"); return; }
   if( !str_prefix(arg,"eas") )
	{ do_close(ch,"east"); return; }
   if( !str_prefix(arg,"wes") )
	{ do_close(ch,"west"); return; }
   if( !str_cmp(arg,"nw") )
	{ do_close(ch,"northwest"); return; }
   if( !str_cmp(arg,"ne") )
	{ do_close(ch,"northeast"); return; }
   if( !str_cmp(arg,"se") )
	{ do_close(ch,"southeast"); return; }
   if( !str_cmp(arg,"sw") )
	{ do_close(ch,"southwest"); return; }

   if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
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
	  if ( obj->pIndexData->vnum == key )
		 return TRUE;
   }
   
   return FALSE;
}



void do_lock( CHAR_DATA *ch, char *argument )
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

   if( !str_prefix(arg,"nort") )
	{ do_lock(ch,"north"); return; }
   if( !str_prefix(arg,"sout") )
	{ do_lock(ch,"south"); return; }
   if( !str_prefix(arg,"eas") )
	{ do_lock(ch,"east"); return; }
   if( !str_prefix(arg,"wes") )
	{ do_lock(ch,"west"); return; }
   if( !str_cmp(arg,"nw") )
	{ do_lock(ch,"northwest"); return; }
   if( !str_cmp(arg,"ne") )
	{ do_lock(ch,"northeast"); return; }
   if( !str_cmp(arg,"se") )
	{ do_lock(ch,"southeast"); return; }
   if( !str_cmp(arg,"sw") )
	{ do_lock(ch,"southwest"); return; }
   
   if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
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
		 
		 if (obj->value[4] < 0 || I_IS_SET(obj->value[1],EX_NOLOCK))
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
		    obj_from_char(key);
		    act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
		    act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
		 }
		 return;
	  }
	  
	  /* 'lock object' */
	  if ( obj->item_type != ITEM_CONTAINER )
	  { send_to_char( "That's not a container.\n\r", ch ); return; }
	  if ( !I_IS_SET(obj->value[1], CONT_CLOSED) )
	  { send_to_char( "It's not closed.\n\r",        ch ); return; }
	  if ( obj->value[2] < 0 )
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
	    obj_from_char(key);
	    act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
	    act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
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
	  if ( pexit->key < 0 )
	  { send_to_char( "It can't be locked.\n\r",     ch ); return; }
	  if ( !has_key( ch, pexit->key) )
	  { send_to_char( "You lack the key.\n\r",       ch ); return; }
	  else key = find_key(ch,pexit->key);

	  if ( IS_SET(pexit->exit_info, EX_LOCKED) )
	  { send_to_char( "It's already locked.\n\r",    ch ); return; }
	  
	  SET_BIT(pexit->exit_info, EX_LOCKED);
	  send_to_char( "*Click*\n\r", ch );
	  act( "$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	  if (IS_SET(key->extra_flags, ITEM_ONE_USE))
		{
		obj_from_char(key);
		act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
		act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
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



void do_unlock( CHAR_DATA *ch, char *argument )
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
   
   if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
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
		 
		 if (obj->value[4] < 0)
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
			obj_from_char(key);
			act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
			act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
			}
		
		return;
	  }
	  
	  /* 'unlock object' */
	  if ( obj->item_type != ITEM_CONTAINER )
	  { send_to_char( "That's not a container.\n\r", ch ); return; }
	  if ( !I_IS_SET(obj->value[1], CONT_CLOSED) )
	  { send_to_char( "It's not closed.\n\r",        ch ); return; }
	  if ( obj->value[2] < 0 )
	  { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
	  if ( !has_key( ch, obj->value[2] ) )
	  { send_to_char( "You lack the key.\n\r",       ch ); return; }
	  else
	     key = find_key( ch, obj->value[2]);
	  if ( !I_IS_SET(obj->value[1], CONT_LOCKED) )
	  { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
	  
	  I_REMOVE_BIT(obj->value[1], CONT_LOCKED);
	  act("You unlock $p.",ch,obj,NULL,TO_CHAR);
	  act( "$n unlocks $p.", ch, obj, NULL, TO_ROOM );
		if (IS_SET(key->extra_flags, ITEM_ONE_USE))
			{
			obj_from_char(key);
			act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
			act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
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
	  if ( pexit->key < 0 )
	  { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
	  if ( !has_key( ch, pexit->key) )
	  { send_to_char( "You lack the key.\n\r",       ch ); return; }
	  else
	    key = find_key( ch, pexit->key );
	  if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
	  { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
	  
	  REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	  send_to_char( "*Click*\n\r", ch );
	  act( "$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
		if (IS_SET(key->extra_flags, ITEM_ONE_USE))
			{
			obj_from_char (key);     
			act("{c*POOF* $p disappears from your hand!{x",ch,key,NULL,TO_CHAR);
			act("{c*POOF* $p disappears from $n's hand!{x",ch,key,NULL,TO_ROOM);
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

#define NLRETURN if ( number_percent() > chance ) \
                 { send_to_char("\n\r", ch); return; }
void do_estimate( CHAR_DATA *ch, char *argument )
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

    if ( (ch->level + chance) < victim->level )
    {
	sprintf( buf, "%s is too powerful for you to identify.\n\r" , victim->short_descr);
	send_to_char(buf, ch);
	return;
    }

    sprintf( buf, "You size up %s.\n\r", victim->short_descr );
    send_to_char( buf, ch );
    WAIT_STATE(ch, skill_table[gsn_estimate].beats);

    /* do now so we can just return later */
    check_improve(ch, gsn_estimate, TRUE, 1);
	
    /* some simple info */
    sprintf( buf, "%s is %s.\n\r", victim->short_descr, char_look_info(victim) );
    send_to_char( buf, ch );

    /* high level races are harder to judge */
    if ( ch->level < victim->level )
	chance -= (victim->level - ch->level)/4;

    /* let's see how much more we find out */

    NLRETURN

    sprintf( buf, "This being is level: %d\n\r", victim->level );       
    send_to_char( buf, ch );
	  
    NLRETURN

    sprintf( buf,
	     "Str: %d(%d)  Con: %d(%d)  Vit: %d(%d)  Agi: %d(%d)  Dex: %d(%d)\n\r",
	     victim->perm_stat[STAT_STR],
	     get_curr_stat(victim,STAT_STR),
	     victim->perm_stat[STAT_CON],
	     get_curr_stat(victim,STAT_CON),
	     victim->perm_stat[STAT_VIT],
	     get_curr_stat(victim,STAT_VIT),
	     victim->perm_stat[STAT_AGI],
	     get_curr_stat(victim,STAT_AGI),
	     victim->perm_stat[STAT_DEX],
	     get_curr_stat(victim,STAT_DEX)
	     );
    send_to_char( buf, ch );

    sprintf( buf,
	     "Int: %d(%d)  Wis: %d(%d)  Dis: %d(%d)  Cha: %d(%d)  Luc: %d(%d)\n\r",
	     victim->perm_stat[STAT_INT],
	     get_curr_stat(victim,STAT_INT),
	     victim->perm_stat[STAT_WIS],
	     get_curr_stat(victim,STAT_WIS),
	     victim->perm_stat[STAT_DIS],
	     get_curr_stat(victim,STAT_DIS),
	     victim->perm_stat[STAT_CHA],
	     get_curr_stat(victim,STAT_CHA),
	     victim->perm_stat[STAT_LUC],
	     get_curr_stat(victim,STAT_LUC)
	     );
    send_to_char( buf, ch );

    NLRETURN

    sprintf( buf, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d\n\r",
	     victim->hit,         victim->max_hit,
	     victim->mana,        victim->max_mana,
	     victim->move,        victim->max_move
	     );
    send_to_char( buf, ch );

    sprintf( buf, "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
	     GET_AC(victim,AC_PIERCE), GET_AC(victim,AC_BASH),
	     GET_AC(victim,AC_SLASH),  GET_AC(victim,AC_EXOTIC)
	     );
    send_to_char(buf,ch);

    NLRETURN

    sprintf( buf, "Hit: %d  Dam: %d  Saves: %d\n\r",
	     GET_HITROLL(victim), GET_DAMROLL(victim), get_save(victim)
	     );
    send_to_char( buf, ch );

    if ( victim->pIndexData->new_format)
    {
	sprintf( buf, "Damage: %dd%d  Type: %s\n\r",
		 victim->damage[DICE_NUMBER], victim->damage[DICE_TYPE],
		 attack_table[victim->dam_type].noun
		 );
	send_to_char(buf,ch);
    }
	
    NLRETURN

    sprintf(buf, "Knows how to: %s\n\r", off_bits_name(victim->off_flags));
    send_to_char(buf,ch);

    NLRETURN

    sprintf(buf, "Fights in stance: %s\n\r", stances[victim->stance].name );
    send_to_char(buf,ch);

    NLRETURN

    sprintf(buf, "Immune to: %s\n\r", imm_bits_name(victim->imm_flags));
    send_to_char(buf,ch);
  
    NLRETURN
    
    sprintf(buf, "Resistant to: %s\n\r", imm_bits_name(victim->res_flags));
    send_to_char(buf,ch);

    NLRETURN
    
    sprintf(buf, "Vulnerable to: %s\n\r", imm_bits_name(victim->vuln_flags));
    send_to_char(buf,ch);

}
#undef NLRETURN

void do_shoot_lock( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int door;
   
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
   
   if ( !IS_NPC(ch) && number_percent( ) > get_skill(ch,gsn_shoot_lock))
   {
	  send_to_char( "You miss the lock completely.\n\r", ch);
	  check_improve(ch,gsn_shoot_lock,FALSE,2);
	  return;
   }

   if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
   {
       send_to_char("But you could damage whatever's inside!\n\r",ch);
       return;
   }
   
   if ( ( door = find_door( ch, arg ) ) >= 0 )
   {
       /* 'pick door' */
       ROOM_INDEX_DATA *to_room;
       EXIT_DATA *pexit;
       EXIT_DATA *pexit_rev;
	  
       pexit = ch->in_room->exit[door];
       if ( !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
       { send_to_char( "It's not closed, why shoot the lock?\n\r", ch ); return; }
       if ( pexit->key < 0 && !IS_IMMORTAL(ch))
       { send_to_char( "Your gun is useless against this strong lock.\n\r", ch ); return; }
       if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
       { send_to_char( "It was already unlocked, but . . .\n\r", ch ); return; }
       if ( IS_SET(pexit->exit_info, EX_PICKPROOF)
	    || IS_SET(pexit->exit_info, EX_HARD)
	    || IS_SET(pexit->exit_info, EX_INFURIATING) )
       { send_to_char( "Nope, shooting the lock did no good.\n\r", ch ); return; }
	  
	  REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	  send_to_char( "*BLAM!*\n\r", ch );
	  act( "$n blows away the lock on the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	  check_improve(ch,gsn_shoot_lock,TRUE,2);
	  
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

void do_unjam(CHAR_DATA *ch, char *argument)
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
	check_improve(ch,gsn_unjam,TRUE,4);
	WAIT_STATE(ch,skill_table[gsn_unjam].beats);
	return;
    }
    else
    {
	act("You fail to unjam $p.",ch,obj,NULL,TO_CHAR);
	check_improve(ch,gsn_unjam,FALSE,4);
	WAIT_STATE(ch,skill_table[gsn_unjam].beats);
	return;
    }
	
    /*
    act("You can't unjam $p.",ch,obj,NULL,TO_CHAR);
    */
}

void do_set_snare ( CHAR_DATA *ch, char *argument)
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
		    check_improve(ch, gsn_set_snare, TRUE, 2); 
		}
	    
	    else
		{
		    send_to_char("Your snare doesn't quite work.\n\r", ch);
		    check_improve(ch, gsn_set_snare, FALSE, 2); 
		}
	    
	}
    else
	send_to_char ("You're not sure how to set a snare.\n\r", ch);
    return;
}

void do_peel ( CHAR_DATA *ch, char *argument)
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
		    check_improve(ch, gsn_peel, TRUE, 2);
                }
	    
	    else
                {
		    send_to_char("The peel sticks to your hand.\n\r", ch);
		    check_improve(ch, gsn_peel, FALSE, 2);
                }
	    
	}
    else
        send_to_char ("You don't like bananas, hence you don't have any peels.\n\r", ch);
    return;
}


void do_pick( CHAR_DATA *ch, char *argument )
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
   
   WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );
   
   /* look for guards */
   for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
   {
	  if ( IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level )
	  {
		 act( "$N is standing too close to the lock.",
			ch, NULL, gch, TO_CHAR );
		 return;
	  }
   }
   
   skill = get_skill(ch,gsn_pick_lock) * (ch->level + get_curr_stat(ch, STAT_DEX) + 200)/500;
   
   if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
   {
	  /* portal stuff */
	  if (obj->item_type == ITEM_PORTAL)
	  {
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
		     check_improve(ch,gsn_pick_lock,FALSE,2);
		     return;
		 }
		 
		 I_REMOVE_BIT(obj->value[1],EX_LOCKED);
		 act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
		 act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
		 check_improve(ch,gsn_pick_lock,TRUE,2);
		 return;
	  }
	  	  
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
	      check_improve(ch,gsn_pick_lock,FALSE,2);
	      return;
	  }
	  
	  I_REMOVE_BIT(obj->value[1], CONT_LOCKED);
	  act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
	  act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
	  check_improve(ch,gsn_pick_lock,TRUE,2);
	  return;
   }
   
   if ( ( door = find_door( ch, arg ) ) >= 0 )
   {
	  /* 'pick door' */
	  ROOM_INDEX_DATA *to_room;
	  EXIT_DATA *pexit;
	  EXIT_DATA *pexit_rev;
	  
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
	      check_improve(ch,gsn_pick_lock,FALSE,2);
	      return;
	  }

	  REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	  send_to_char( "*Click*\n\r", ch );
	  act( "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	  check_improve(ch,gsn_pick_lock,TRUE,2);
	  
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




void do_stand( CHAR_DATA *ch, char *argument )
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
	  
	  ch->position = POS_STANDING;
          ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
	  do_look(ch,"auto");
	  break;
	  
   case POS_RESTING: case POS_SITTING:
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
	  ch->position = POS_STANDING;
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



void do_rest( CHAR_DATA *ch, char *argument )
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
          ch->pcdata->condition[COND_DEEP_SLEEP] = 0;

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
	  /*ch->stance = 0;*/
	  ch->position = POS_RESTING;
          ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
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
          ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
	  break;
   }
   
   
   return;
}


void do_sit (CHAR_DATA *ch, char *argument )
{
   OBJ_DATA *obj = NULL;
   CHAR_DATA *gch = NULL;
   
   if (ch->position == POS_FIGHTING)
   {
	  send_to_char("Maybe you should finish this fight first?\n\r",ch);
	  return;
   }
   
   /* okay, now that we know we can sit, find an object to sit on */
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
		 ||  (!I_IS_SET(obj->value[2],SIT_ON)
		 &&   !I_IS_SET(obj->value[2],SIT_IN)
		 &&   !I_IS_SET(obj->value[2],SIT_AT)))
	  {
		 send_to_char("You can't sit on that.\n\r",ch);
		 return;
	  }
	  
	  if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
	  {
		 act_new("There's no more room on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
		 return;
	  }
	  
	  ch->on = obj;
   }
   switch (ch->position)
   {
   case POS_SLEEPING:
	  if (IS_AFFECTED(ch,AFF_SLEEP))
	  {
		 send_to_char("You can't wake up!\n\r",ch);
		 return;
	  }
	  
	  if (obj == NULL)
	  {
		 send_to_char( "You wake and sit up.\n\r", ch );
		 act( "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
	  }
	  else if (I_IS_SET(obj->value[2],SIT_AT))
	  {
		 act_new("You wake and sit at $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
		 act("$n wakes and sits at $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],SIT_ON))
	  {
		 act_new("You wake and sit on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
		 act("$n wakes and sits at $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else
	  {
		 act_new("You wake and sit in $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
		 act("$n wakes and sits in $p.",ch,obj,NULL,TO_ROOM);
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
	  ch->position = POS_SITTING;
          ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
	  break;
   case POS_RESTING:
	  if (obj == NULL)
		 send_to_char("You stop resting.\n\r",ch);
	  else if (I_IS_SET(obj->value[2],SIT_AT))
	  {
		 act("You sit at $p.",ch,obj,NULL,TO_CHAR);
		 act("$n sits at $p.",ch,obj,NULL,TO_ROOM);
	  }
	  
	  else if (I_IS_SET(obj->value[2],SIT_ON))
	  {
		 act("You sit on $p.",ch,obj,NULL,TO_CHAR);
		 act("$n sits on $p.",ch,obj,NULL,TO_ROOM);
	  }
	  ch->position = POS_SITTING;
          ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
	  break;
   case POS_SITTING:
	  send_to_char("You are already sitting down.\n\r",ch);
	  break;
   case POS_STANDING:
	  if (obj == NULL)
	  {
		 send_to_char("You sit down.\n\r",ch);
		 act("$n sits down on the ground.",ch,NULL,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],SIT_AT))
	  {
		 act("You sit down at $p.",ch,obj,NULL,TO_CHAR);
		 act("$n sits down at $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else if (I_IS_SET(obj->value[2],SIT_ON))
	  {
		 act("You sit on $p.",ch,obj,NULL,TO_CHAR);
		 act("$n sits on $p.",ch,obj,NULL,TO_ROOM);
	  }
	  else
	  {
		 act("You sit down in $p.",ch,obj,NULL,TO_CHAR);
		 act("$n sits down in $p.",ch,obj,NULL,TO_ROOM);
	  }
	  ch->position = POS_SITTING;
          ch->pcdata->condition[COND_DEEP_SLEEP] = 0;
	  /*ch->stance = 0;*/
	  break;
   }
   return;
}


void do_sleep( CHAR_DATA *ch, char *argument )
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
		 /*ch->stance = 0;*/
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
		 /*ch->stance = 0;*/
		 ch->position = POS_SLEEPING;
	  }
	  break;
	  
   case POS_FIGHTING:
	  send_to_char( "You are already fighting!\n\r", ch );
	  break;
   }
   
   return;
}



void do_wake( CHAR_DATA *ch, char *argument )
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
   do_rest(victim,"");
   return;
}


void do_sneak( CHAR_DATA *ch, char *argument )
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
    
    if (IS_REMORT(ch))
    {
        send_to_char("There is noplace to hide in remort.\n\r",ch);
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

    send_to_char( "You attempt to move silently.\n\r", ch );

    WAIT_STATE( ch, skill_table[gsn_sneak].beats );
    if ( number_percent( ) < get_skill(ch,gsn_sneak))
    {
        send_to_char("You sure are sneaky.\n\r",ch);
        check_improve(ch,gsn_sneak,TRUE,3);
        af.where     = TO_AFFECTS;
        af.type      = gsn_sneak;
        af.level     = ch->level; 
        af.duration  = ch->level;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_SNEAK;
        affect_to_char( ch, &af );
    }
    else
    {
        send_to_char("You don't have that sneaky feeling.\n\r",ch); 
        check_improve(ch,gsn_sneak,FALSE,3);
    }
    return;
}

void do_hide( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
    
    send_to_char( "You attempt to hide.\n\r", ch );
    
    if (IS_REMORT(ch))
    {
        send_to_char("There is noplace to hide in remort.\n\r",ch);
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
        send_to_char("I think your opponent sees you.\n\r",ch);
        return;
    }
    
    WAIT_STATE( ch, skill_table[gsn_hide].beats );
    if ( number_percent( ) < get_skill(ch,gsn_hide))
    {
        af.where     = TO_AFFECTS;
        af.type      = gsn_hide;
        af.level     = ch->level; 
        af.duration  = ch->level;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_HIDE;
        affect_to_char( ch, &af );
        send_to_char("You successfully hide.\n\r",ch); 
        check_improve(ch,gsn_hide,TRUE,3);
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
   affect_strip ( ch, gsn_invis       );
   affect_strip ( ch, gsn_mass_invis  );
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
void do_visible( CHAR_DATA *ch, char *argument )
{
    make_visible( ch );
    send_to_char( "Ok.\n\r", ch );
    return;
}

void do_recall( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *location;
    int chance;
    int room=ROOM_VNUM_RECALL;
    int move_cost;
    char *god_name;
    
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

    if ( carries_relic(ch) )
    {
        send_to_char( "Not with a relic!\n\r", ch );
        return FALSE;
    }

    if ( (god_name = get_god_name(ch)) == NULL )
    {
	god_name = clan_table[ch->clan].patron;
	if ( god_name == NULL || god_name[0] == '\0' )
	    god_name = "Rimbol";
    }

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
        return;
    
    if ( IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
        ||   IS_AFFECTED(ch, AFF_CURSE))
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

    move_cost = ch->level * 5;

    if ( !IS_NPC(ch) && ch->move < move_cost)
    {
	send_to_char("You are too tired to recall.\n\r", ch);
	return;
    }
    
    if ( ( victim = ch->fighting ) != NULL )
    {
        int lose,skill;
        
        skill = get_skill(ch,gsn_recall);
        skill += get_curr_stat(ch, STAT_LUC)/5;
		skill -= get_curr_stat(victim, STAT_LUC)/4;

		if (lose = get_skill(victim, gsn_entrapment))
		{
			skill -= lose/3;
			check_improve(victim, gsn_entrapment, TRUE, 10);
		}

		if (victim->stance == STANCE_AMBUSH)
			skill /= 2;
        
        if (number_percent() > skill/3)
        {
            check_improve(ch,gsn_recall,FALSE,6);
            WAIT_STATE( ch, 6 );
            sprintf( buf, "%s doesn't answer your prayer.\n\r", god_name);
            send_to_char( buf, ch );
            return;
        }

        if (!IS_HERO(ch))
        {
            lose = (ch->desc != NULL) ? 25 : 50;
            gain_exp( ch, 0 - lose );
            sprintf( buf, "You recall from combat!  You lose %d exps.\n\r", lose );
            send_to_char( buf, ch );
        }
        else
            send_to_char("You recall from combat!\n\r",ch);

        check_improve(ch,gsn_recall,TRUE,4);
        stop_fighting( ch, TRUE );
        
    }
    
    ch->move -= move_cost;
    act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, location );
    act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );
    
    if (ch->pet != NULL)
        do_recall(ch->pet,"");
    
    return;
}

void do_morph(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int race, cost;

	one_argument(argument, arg);

	if (IS_NPC(ch))
		send_to_char("You pretend you're a doppelganger.\n\r", ch);
	else if (ch->race == race_doppelganger)
	{
		if ((arg[0]=='\0') && ch->pcdata->morph_race)
		{
			WAIT_STATE(ch, PULSE_VIOLENCE);
			send_to_char("You revert to your original form.\n\r", ch);
			ch->pcdata->morph_time = 0;
			ch->pcdata->morph_race = 0;
			morph_update( ch );
			return;
		}

		if (arg[0]=='\0' || (victim=get_char_room(ch, arg))==NULL)
		{
			send_to_char("You must select a victim to emulate.\n\r", ch);
			return;
		}

		if ( (race=victim->race) == race_doppelganger )
		{
			send_to_char("Your victim is a doppelganger too.\n\r", ch);
			return;
		}

		if (!race_table[race].pc_race ||
		    (ch->pcdata->remorts<pc_race_table[race].remorts))
		{
			sprintf(buf, "You cant morph into a %s.\n\r",
				race_table[race].name);
			send_to_char(buf, ch);
			return;
		}

		cost = (pc_race_table[race].remorts + 1) * ch->level;
		if ( (ch->mana < cost ) || (ch->move < 2*cost) )
		{
			sprintf(buf,"You need %d mana and %d move to morph into a %s.\n\r",
					cost, 2*cost, race_table[race].name);
			send_to_char(buf, ch);
			return;
		}
		else
		{
			WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
			sprintf(buf, "You morph into a %s.\n\r", race_table[race].name);
			send_to_char(buf, ch);
			ch->move-=2*cost;
			ch->mana-=cost;
			ch->pcdata->morph_race = race;
			ch->pcdata->morph_time = ch->level*2 + 10;
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
		else if ( ch->move < (cost = 10) )
		{
		    send_to_char( "You are too exhausted to morph into humanoid form.\n\r", ch );
		    return;
		}
		else
		{
		    WAIT_STATE(ch, PULSE_VIOLENCE);
		    send_to_char("You morph into humanoid form.\n\r", ch);
		    ch->pcdata->morph_race = race_naga;
		    ch->pcdata->morph_time = 10;
		    ch->move -= cost;
		}
		morph_update( ch );
	}
	else if (ch->race == race_werewolf)
		send_to_char("You cant control your lycanthropy.\n\r", ch);
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
    int dam, dam_type;

    if ( can_behead && number_bits(4) == 0 )
    {
	act( "Your head is chopped right off!", ch, NULL, NULL, TO_CHAR );
	act( "$n's head is chopped right off!", ch, NULL, NULL, TO_ROOM );
	behead( ch, ch );
	return;
    }

    /* damage */
    dam_type = number_range(DAM_BASH, DAM_POISON);
    dam = 100 + dice( ch->level, ch->level );
    damage( ch, ch, dam, 0, dam_type, FALSE);
    
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

void do_disarm_trap( CHAR_DATA *ch, char *argument )
{
    int door, skill;
    EXIT_DATA *exit, *rev_exit;
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


void do_root( CHAR_DATA *ch, char *argument )
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

/* checks if a character looses blood */


void check_bleed( CHAR_DATA *ch, int dir )
{
    OBJ_DATA *blood;
    char buf[MSL];
    AFFECT_DATA *af;

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


    /* This was commented out, and the below section added so that players
       affected by rupture will leave a blood trail regardless of their HP
       - Astark Nov 2012
    if ( save_body_affect(ch, (ch->max_hit/4 - ch->hit)/10) )
	return; */

    if ( !is_affected(ch,gsn_rupture) && save_body_affect(ch, (ch->max_hit/4 - ch->hit)/10) )
	return;

    /* create blood object */
    if ( (blood = create_object(get_obj_index(OBJ_VNUM_BLOOD), 0)) == NULL )
	return;



    if ( ch->hit <= ch->max_hit/4 || is_affected(ch, gsn_rupture) )
    {
    /* add direction hint */
    sprintf( buf, "{rA trail of blood leads %s.{x", dir_name[dir] );
    free_string( blood->description );
    blood->description = str_dup( buf );
    /* shouldn't lie around for all time */
    blood->timer = 10;
    obj_to_room( blood, ch->in_room );
    send_to_char( "{rYou leave a trail of blood.{x\n\r", ch );
    }
}

/*
void load_storage_boxes( CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	int i;
	if (ch->pcdata->storage_boxes<1)
	  return;

	send_to_char("As you enter, an employee brings in your boxes and sets them before you.\n\r",ch);
	for (i=1;i<=ch->pcdata->storage_boxes;i++)
	{
	    ch->pcdata->box_data[i-1] = create_object(get_obj_index(OBJ_VNUM_STORAGE_BOX), 0);
	    obj_to_room( ch->pcdata->box_data[i-1], ch->in_room);   
	}
	load_storage_box(ch);
}
*/
/*void unload_storage_boxes( CHAR_DATA *ch)
{
	sh_int i;

        for (i=0;i<ch->pcdata->storage_boxes;i++)
        {
            extract_obj(ch->pcdata->box_data[i]);
            ch->pcdata->box_data[i]=NULL;
        }
}
	
*/



bool explored_vnum(CHAR_DATA *ch, int vnum)
{	int mask = vnum / 32; //Get which bucket the bit is in
	unsigned int bit = vnum % 32; //Get which bit in the bucket we're playing with
	EXPLORE_HOLDER *pExp; //The buckets bucket.

	if(bit == 0 ) // % 32 will return 0 if vnum == 32, instead make it the last bit of the previous mask
	{	mask--;
		bit = 32;
	}

	for(pExp = ch->pcdata->explored->bits ; pExp ; pExp = pExp->next ) //Iterate through the buckets
	{	if(pExp->mask != mask)
			continue;
		//Found the right bucket, might be explored.
		if(IS_SET_EXPLORE(pExp->bits, ( 1 << bit ) ) ) //Convert bit to 2^(bit-1) and see if it's set.
			return TRUE;
		return FALSE; //Return immediately. This value wont be in any other bucket.
	}
	return FALSE;
}
//Explore a vnum. Assume it's not explored and just set it.
void explore_vnum(CHAR_DATA *ch, int vnum )
{	int mask = vnum / 32; //Get which bucket it will be in
	unsigned int bit = vnum % 32; // Get which bit to set
	EXPLORE_HOLDER *pExp; //The buckets bucket.

	if(bit == 0 ) // % 32 will return 0 if vnum is a multiple 32, instead make it the last bit of the previous mask
	{	mask--;
		bit = 32;
	}

	//Find the bucket.
	for(pExp = ch->pcdata->explored->bits ; pExp ; pExp = pExp->next )
		if(pExp->mask == mask)
			break;

	if(!pExp) //If it's null, bucket not found, we'll whip one up.
	{	pExp = (EXPLORE_HOLDER *)calloc(sizeof(*pExp), 1); //Alloc and zero
		pExp->mask = mask;
		pExp->next = ch->pcdata->explored->bits; //Add to
		ch->pcdata->explored->bits = pExp;       //the list
	}

	SET_BIT_EXPLORE(pExp->bits, ( 1 << bit ) ); //Convert bit to 2^(bit-1) and set
	ch->pcdata->explored->set++; //Tell how many rooms we've explored
}


//Explore a vnum.
void check_explore( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoom )
{	if(IS_NPC(ch) ) return;

	
	if(explored_vnum(ch, pRoom->vnum) )
		return;

	explore_vnum(ch, pRoom->vnum);
}

void do_explored(CHAR_DATA *ch, char *argument )
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
