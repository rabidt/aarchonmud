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

/***************************************************************************
*	ROM 2.4 is copyright 1993-1995 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@pacinfo.com)				   *
*	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
*	    Brian Moore (rom@rom.efn.org)				   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  Based on MERC 2.2 MOBprograms by N'Atas-ha.                            *
 *  Written and adapted to ROM 2.4 by                                      *
 *          Markku Nylander (markku.nylander@uta.fi)                       *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "mob_cmds.h"
#include "tables.h"
#include "lua_main.h"

DECLARE_DO_FUN( do_look );
DECLARE_DO_FUN( do_peace );

/*
 * Command table.
 */
const	struct	mob_cmd_type	mob_cmd_table	[] =
{
  { "asound",     do_mpasound,     "[string] : echo to all the rooms around the mobile"     },
  { "gecho",      do_mpgecho,      "[string] : echo to all active players in the game"      },
  { "zecho",      do_mpzecho,      "[string] : echo to all players in the same area"        },
  { "kill",	  do_mpkill,       "[victim] : start combat with a player"                  },
  { "assist",     do_mpassist,     "[victim] : assist another mob or player"                },
  { "junk",	  do_mpjunk,       "[item|all|all.xxx] : destroy item(s) in inventory"      },
  { "echo",	  do_mpecho,       "[string] : echo to room"                                },
  { "echoaround", do_mpechoaround, "[victim] [string] : echo to all in room save victim"    },
  { "echoat",     do_mpechoat,     "[victim] (string) : echo to victim"                     },
  { "mload",      do_mpmload,      "[vnum] : load a mobile"                                 },
  { "oload",      do_mpoload,      "[vnum] (0 R|W) : load an object to inv, room or worn"   },
  { "purge",      do_mppurge,      "(target) : purge all objs and mobs in room or target"   },
  { "goto",	  do_mpgoto,       "[location] : go to location"                            },
  { "at",	  do_mpat,         "[location] [command] : execute command at location"     },
  { "transfer",	  do_mptransfer,   "[victim|'all'] [location] : move target or all in room" },
  { "gtransfer",  do_mpgtransfer,  "[victim] [location] : move group of victim to location"            },
  { "otransfer",  do_mpotransfer,  "[object] [location] : move object from room"            },
  { "force",      do_mpforce,      "[victim] [command] : victim executes command"           },
  { "gforce",     do_mpgforce,     "[victim] [command] : victim's group executes command"   },
  { "vforce",     do_mpvforce,     "[vnum] [command] : all mobs of vnum execute command"    },
  { "cast",	  do_mpcast,       "[spell] (target) : cast spell without failure and cost" },
  { "damage",     do_mpdamage,     "[victim|'all'] [min] [max] (kill) : deal (lethal) damage"     },
  { "remember",   do_mpremember,   "[victim] : remember victim, later referred to with $q"  },
  { "forget",     do_mpforget,     ": forget victim previously remembered"                  },
  { "delay",      do_mpdelay,      "[pulses] : delay mob for delay triggers (pulse=4sec)"   },
  { "cancel",     do_mpcancel,     ": remove delay"                                         },
  { "call",	  do_mpcall,       "[vnum] (victim) (obj1) (obj2) : execute another mprog"  },
  { "remove",     do_mpremove,     "[victim] [vnum|all] (inv|get|room) : extract object"    },
  { "remort",     do_mpremort,     "[victim] : remort a player"                             },
  { "qset",       do_mpqset,       "[victim] [id] [value] [time limit] : set quest-state for player"     },
  { "qadvance",   do_mpqadvance,   "[victim] [id] (increment) : increase quest-state"       },
  { "reward",     do_mpreward,     "[victim] [exp|qp|gold] [ammount] : give exp or qp reward"    },
  { "peace",      do_mppeace,      "(victim) : stop combat and make mobs non-aggro"         },
  { "restore",    do_mprestore,    "[victim] : restore victim"                              },
  { "act",        do_mpact,        "(not) [flag] : set or remove an act-flag"               },
  { "hit",        do_mphit,        "(victim) : do one attack"      },
  { "",	          0,               ""       }
};

DEF_DO_FUN(do_mob)
{
    /*
     * Security check!
     */
    if ( ch->desc != NULL && get_trust(ch) < MAX_LEVEL )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }
    mob_interpret( ch, argument );
}
/*
 * Mob command interpreter. Implemented separately for security and speed
 * reasons. A trivial hack of interpret()
 */
void mob_interpret( CHAR_DATA *ch, const char *argument )
{
    char buf[MAX_STRING_LENGTH], command[MAX_INPUT_LENGTH];
    int cmd;
    //const char *org_arg = argument;

    argument = one_argument( argument, command );

    /*
     * Look for command in command table.
     */
    for ( cmd = 0; mob_cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
	if ( command[0] == mob_cmd_table[cmd].name[0]
	&&   !str_prefix( command, mob_cmd_table[cmd].name ) )
	{
	    /* Record the command */
	    /*
	    sprintf (last_command, "[%5d] %s in [%5d]: mob %s",
		     IS_NPC(ch) ? ch->pIndexData->vnum : 0,
		     IS_NPC(ch) ? ch->short_descr : ch->name,
		     ch->in_room ? ch->in_room->vnum : 0,
		     org_arg);
	    */

	    (*mob_cmd_table[cmd].do_fun) ( ch, argument );

	    /* Record that the command was the last done, but it is finished */
	    /*
	    sprintf (last_command, "(Finished) [%5d] %s in [%5d]: mob %s",
		     IS_NPC(ch) ? ch->pIndexData->vnum : 0,
		     IS_NPC(ch) ? ch->short_descr : ch->name,
		     ch->in_room ? ch->in_room->vnum : 0,
		     org_arg);
	    */

	    tail_chain( );
	    return;
	}
    }
    sprintf( buf, "Mob_interpret: invalid cmd from mob %d: '%s'",
	IS_NPC(ch) ? ch->pIndexData->vnum : 0, command );
    bug( buf, 0 );
}

char *mprog_type_to_name( int type )
{
    switch ( type )
    {
    case TRIG_ACT:             	return "ACT";
    case TRIG_SPEECH:          	return "SPEECH";
    case TRIG_RANDOM:          	return "RANDOM";
    case TRIG_FIGHT:           	return "FIGHT";
    case TRIG_HPCNT:           	return "HPCNT";
    case TRIG_DEATH:           	return "DEATH";
    case TRIG_ENTRY:           	return "ENTRY";
    case TRIG_GREET:           	return "GREET";
    case TRIG_GRALL:        	return "GRALL";
    case TRIG_GIVE:            	return "GIVE";
    case TRIG_BRIBE:           	return "BRIBE";
    case TRIG_KILL:	      	return "KILL";
    case TRIG_DELAY:           	return "DELAY";
    case TRIG_SURR:	      	return "SURRENDER";
    case TRIG_EXIT:	      	return "EXIT";
    case TRIG_EXALL:	      	return "EXALL";
    case TRIG_EXBOMB:		return "EXBOMB";
    case TRIG_DRBOMB:		return "DRBOMB";
    case TRIG_DEFEAT:		return "DEFEAT";
    case TRIG_SOCIAL:		return "SOCIAL";
    case TRIG_TRY:              return "TRY";
    case TRIG_RESET:            return "RESET";
    case TRIG_MPCNT:           	return "MPCNT";
    case TRIG_SPELL:            return "SPELL";
    case TRIG_TIMER:            return "TIMER";
    case TRIG_COMMAND:          return "COMMAND";
    default:                  	return "ERROR";
    }
}

/*
 * Allow wizi mobs to send messages with act
 */
void act_non_wizi(const char *format, CHAR_DATA *ch, 
		  const void *arg1, const void *arg2,
		  int type)
{
  if (IS_NPC(ch) && IS_SET(ch->act, ACT_WIZI))
  {
    REMOVE_BIT(ch->act, ACT_WIZI);
    act(format, ch, arg1, arg2, type);
    SET_BIT(ch->act, ACT_WIZI);
  }
  else
    act(format, ch, arg1, arg2, type);
}

/*
 * Find a target char or object for an mprog - search in area first
 */
CHAR_DATA* get_mp_char( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA *victim = get_char_area( ch, argument );

    if ( ch->in_room == NULL )
	return NULL;

    if ( victim != NULL || IS_SET(ch->in_room->area->area_flags, AREA_REMORT) )
	return victim;
    else
	return get_char_world(ch, argument);
}

OBJ_DATA* get_mp_obj( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA *victim = get_obj_area( ch, argument );

    if ( ch->in_room == NULL )
	return NULL;

    if ( victim != NULL || IS_SET(ch->in_room->area->area_flags, AREA_REMORT) )
	return victim;
    else
	return get_obj_world(ch, argument);
}

ROOM_INDEX_DATA* find_mp_location( CHAR_DATA *ch, const char *arg )
{
    ROOM_INDEX_DATA *room;

    /* special check for relative vnums */
    if ( is_r_number(arg) )
	return get_room_index( r_atoi(ch, arg) );

    if ( ch->in_room == NULL )
	return NULL;

    room = find_location_new( ch, arg, TRUE );

    if ( room != NULL || IS_SET(ch->in_room->area->area_flags, AREA_REMORT) )
	return room;
    else
	return find_location_new( ch, arg, FALSE );
}

/*
 * stuff for vnums relative to area min_vnum
 */

/* returns weather arg is a normal or relative vnum */
bool is_r_number( const char *arg )
{
    char arg1[MIL];

    if ( arg == NULL || arg[0] == '\0' )
    {
        bugf( "is_r_number: invalid argument" );
    }

    if ( is_number(arg) )
	return TRUE;

    one_argument( arg, arg1 );
    return (arg1[0] == 'r') && is_number( arg1 + 1 );
}

/* converts normal or relative vnums */
int r_atoi( CHAR_DATA *ch, const char *arg )
{
    AREA_DATA *area;
    char arg1[MIL];
    int nr;

    one_argument( arg, arg1 );
    if ( arg1[0] != 'r' )
	return atoi(arg1);
    
    if ( ch == NULL || ch->in_room == NULL )
    {
	bugf( "r_atoi: NULL char or in_room" );
	return -1;
    }

    if ( (nr = atoi(arg1 + 1)) < 0 )
	return nr;

    if ( ch->pIndexData == NULL )
	area = ch->in_room->area;
    else
	area = ch->pIndexData->area;

    /* check if the 'r' might be too much */
    if ( area->min_vnum + nr > area->max_vnum )
    {
	bugf( "r_atoi: relative vnum (%s) out of area on mob %d",
	      arg, IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return nr;
    }
    return area->min_vnum + nr;
}

int r_atoi_obj( OBJ_DATA *obj, const char *arg )
{
    AREA_DATA *area;
    char arg1[MIL];
    int nr;

    one_argument( arg, arg1 );
    if ( arg1[0] != 'r' )
    return atoi(arg1);

    if ( obj == NULL )
    {
    bugf( "r_atoi_mob: NULL obj" );
    return -1;
    }

    if ( (nr = atoi(arg1 + 1)) < 0 )
    return nr;

    area = obj->pIndexData->area;

    /* check if the 'r' might be too much */
    if ( area->min_vnum + nr > area->max_vnum )
    {
    bugf( "r_atoi: relative vnum (%s) out of area on obj %d",
          arg, obj->pIndexData->vnum );
    return nr;
    }
    return area->min_vnum + nr;
}

/* 
 * Displays MOBprogram triggers of a mobile
 *
 * Syntax: mpstat [name]
 */
DEF_DO_FUN(do_mpstat)
{
    char        arg[ MAX_STRING_LENGTH  ];
    PROG_LIST  *mprg;
    CHAR_DATA   *victim;
    int i;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Mpstat whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_mp_char( ch, arg ) ) == NULL )
    {
	send_to_char( "No such creature.\n\r", ch );
	return;
    }

    if ( !IS_NPC( victim ) )
    {
	send_to_char( "That is not a mobile.\n\r", ch);
	return;
    }

    if ( !IS_BUILDER(ch, victim->pIndexData->area) )
    {
	send_to_char( "You're not a builder for this mobile's area.\n\r", ch);
	return;
    }

    sprintf( arg, "Mobile #%-6d [%s]\n\r",
	victim->pIndexData->vnum, victim->short_descr );
    send_to_char( arg, ch );

    sprintf( arg, "Delay   %-6d [%s]\n\r",
	victim->mprog_delay,
	victim->mprog_target == NULL 
		? "No target" : victim->mprog_target->name );
    send_to_char( arg, ch );

    if ( !victim->pIndexData->mprog_flags )
    {
	send_to_char( "[No programs set]\n\r", ch);
	return;
    }

    for ( i = 0, mprg = victim->pIndexData->mprogs; mprg != NULL;
	 mprg = mprg->next )

    {
	sprintf( arg, "[%2d] Trigger [%-8s] Program [%4d] Phrase [%s]\n\r",
	      ++i,
	      mprog_type_to_name( mprg->trig_type ),
	      mprg->vnum,
	      mprg->trig_phrase );
	send_to_char( arg, ch );
    }

    return;

}

/*
 * Displays the source code of a given ROOMprogram
 *
 * Syntax: rpdump [vnum]
 */
DEF_DO_FUN(do_rpdump)
{
   char buf[ MAX_INPUT_LENGTH ];
   char arg2[MIL];
   PROG_CODE *rprg;
   AREA_DATA *area;

   argument=one_argument( argument, buf );
   argument=one_argument( argument, arg2 );
   if ( ( rprg = get_rprog_index( atoi(buf) ) ) == NULL )
   {
    send_to_char( "No such ROOMprogram.\n\r", ch );
    return;
   }

   area = get_vnum_area( rprg->vnum );
   if ( area == NULL || !IS_BUILDER(ch, area) )
   {
       send_to_char( "You're not a builder for this aprog's area.\n\r", ch);
       return;
   }

   if (strcmp( arg2, "false"))
       dump_prog( ch, rprg->code, TRUE);
   else
       dump_prog( ch, rprg->code, FALSE );
}

/*
 * Displays the source code of a given AREAprogram
 *
 * Syntax: apdump [vnum]
 */
DEF_DO_FUN(do_apdump)
{
   char buf[ MAX_INPUT_LENGTH ];
   char arg2[MIL];
   PROG_CODE *aprg;
   AREA_DATA *area;

   argument=one_argument( argument, buf );
   argument=one_argument( argument, arg2 );
   if ( ( aprg = get_aprog_index( atoi(buf) ) ) == NULL )
   {
    send_to_char( "No such AREAprogram.\n\r", ch );
    return;
   }

   area = get_vnum_area( aprg->vnum );
   if ( area == NULL || !IS_BUILDER(ch, area) )
   {
       send_to_char( "You're not a builder for this aprog's area.\n\r", ch);
       return;
   }
   
   if (strcmp(arg2, "false"))
      dump_prog( ch, aprg->code, TRUE);
   else
      dump_prog( ch, aprg->code, FALSE); 
}

/*
 * Displays the source code of a given OBJprogram
 *
 * Syntax: opdump [vnum]
 */
DEF_DO_FUN(do_opdump)
{
   char buf[ MAX_INPUT_LENGTH ];
   char arg2[MIL];
   PROG_CODE *oprg;
   AREA_DATA *area;

   argument=one_argument( argument, buf );
   argument=one_argument( argument, arg2 );
   if ( ( oprg = get_oprog_index( atoi(buf) ) ) == NULL )
   {
    send_to_char( "No such OBJprogram.\n\r", ch );
    return;
   }

   area = get_vnum_area( oprg->vnum );
   if ( area == NULL || !IS_BUILDER(ch, area) )
   {
       send_to_char( "You're not a builder for this oprog's area.\n\r", ch);
       return;
   }

   if (strcmp(arg2, "false"))
       dump_prog( ch, oprg->code, TRUE);
   else
       dump_prog( ch, oprg->code, FALSE);
}

/*
 * Displays the source code of a given MOBprogram
 *
 * Syntax: mpdump [vnum]
 */
DEF_DO_FUN(do_mpdump)
{
   char buf[ MAX_INPUT_LENGTH ];
   char arg2[MIL];
   PROG_CODE *mprg;
   AREA_DATA *area;

   argument=one_argument( argument, buf );
   argument=one_argument( argument, arg2);
   if ( ( mprg = get_mprog_index( atoi(buf) ) ) == NULL )
   {
	send_to_char( "No such MOBprogram.\n\r", ch );
	return;
   }

   area = get_vnum_area( mprg->vnum );
   if ( area == NULL || !IS_BUILDER(ch, area) )
   {
       send_to_char( "You're not a builder for this mprog's area.\n\r", ch);
       return;
   }
   
   if (mprg->is_lua)
       if (strcmp(arg2, "false"))
           dump_prog( ch, mprg->code, TRUE );
       else
           dump_prog( ch, mprg->code, FALSE );
   else
       page_to_char_new( mprg->code, ch, TRUE);
}

/*
 * Prints the argument to all active players in the game
 *
 * Syntax: mob gecho [string]
 */
DEF_DO_FUN(do_mpgecho)
{
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
    {
	bug( "MpGEcho: missing argument from vnum %d",
	    IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    for ( d = descriptor_list; d; d = d->next )
    {
	if ( IS_PLAYING(d->connected) )
 	{
	    if ( IS_IMMORTAL(d->character) )
		send_to_char( "Mob echo> ", d->character );
	    send_to_char( argument, d->character );
	    send_to_char( "\n\r", d->character );
	}
    }
}

/*
 * Prints the argument to all players in the same area as the mob
 *
 * Syntax: mob zecho [string]
 */
DEF_DO_FUN(do_mpzecho)
{
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
    {
	bug( "MpZEcho: missing argument from vnum %d",
	    IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    if ( ch->in_room == NULL )
	return;

    for ( d = descriptor_list; d; d = d->next )
    {
	if ( (IS_PLAYING(d->connected))
	&&   d->character->in_room != NULL 
	&&   d->character->in_room->area == ch->in_room->area )
 	{
	    if ( IS_IMMORTAL(d->character) )
		send_to_char( "Mob echo> ", d->character );
	    send_to_char( argument, d->character );
	    send_to_char( "\n\r", d->character );
	}
    }
}

/*
 * Prints the argument to all the rooms around the mobile
 *
 * Syntax: mob asound [string]
 */
DEF_DO_FUN(do_mpasound)
{

    ROOM_INDEX_DATA *was_in_room;
    int              door;

    if ( argument[0] == '\0' )
	return;

    if ( ch->in_room == NULL )
        return;

    was_in_room = ch->in_room;
    for ( door = 0; door < 10; door++ )
    {
    	EXIT_DATA       *pexit;
      
      	if ( ( pexit = was_in_room->exit[door] ) != NULL
	  &&   pexit->u1.to_room != NULL
	  &&   pexit->u1.to_room != was_in_room )
      	{
	    ch->in_room = pexit->u1.to_room;
	    MOBtrigger  = FALSE;
	    act_non_wizi( argument, ch, NULL, NULL, TO_ROOM );
	    MOBtrigger  = TRUE;
	}
    }
    ch->in_room = was_in_room;
    return;

}

/*
 * Lets the mobile kill any player or mobile without murder
 *
 * Syntax: mob kill [victim]
 */

void mpkill( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( victim == ch || IS_NPC(victim) || ch->position == POS_FIGHTING
         || victim->in_room != ch->in_room )
    return;

    if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
    {
    bug( "MpKill - Charmed mob attacking master from vnum %d.",
        IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
    return;
    }

    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}

DEF_DO_FUN(do_mpkill)
{
    char      arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
	return;

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	return;

    mpkill( ch, victim );
}

/*
 * Lets the mobile assist another mob or player
 *
 * Syntax: mob assist [character]
 */
void mpassist( CHAR_DATA *ch, CHAR_DATA *victim)
{
    if ( victim == ch || ch->fighting != NULL || victim->fighting == NULL )
    return;

    multi_hit( ch, victim->fighting, TYPE_UNDEFINED );
    return;
} 

DEF_DO_FUN(do_mpassist)
{
    char      arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
	return;

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	return;

    mpassist( ch, victim);
}


/*
 * Lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy 
 * items using all.xxxxx or just plain all of them 
 *
 * Syntax: mob junk [item]
 */

DEF_DO_FUN(do_mpjunk)
{
    char      arg[ MAX_INPUT_LENGTH ];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    one_argument( argument, arg );

    if ( arg[0] == '\0')
	return;

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
    	if ( ( obj = get_obj_wear( ch, arg ) ) != NULL )
      	{
      	    unequip_char( ch, obj );
	    extract_obj( obj );
    	    return;
      	}
      	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	    return; 
	extract_obj( obj );
    }
    else
      	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
      	{
            obj_next = obj->next_content;
	    if ( arg[3] == '\0' || is_name( &arg[4], obj->name ) )
            {
          	if ( obj->wear_loc != WEAR_NONE)
	    	unequip_char( ch, obj );
          	extract_obj( obj );
            } 
      	}

    return;

}

/*
 * Prints the message to everyone in the room other than the mob and victim
 *
 * Syntax: mob echoaround [victim] [string]
 */

void mpechoaround( CHAR_DATA *ch, CHAR_DATA *vic, const char *txt )
{
    act_non_wizi( txt, ch, NULL, vic, TO_NOTVICT );
}

DEF_DO_FUN(do_mpechoaround)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
	return;

    if ( (victim = pget_char_room(ch, arg)) == NULL )
        return;

    mpechoaround( ch, victim, argument);
}

/*
 * Prints the message to only the victim
 *
 * Syntax: mob echoat [victim] [string]
 */
void mpechoat( CHAR_DATA *ch, CHAR_DATA *victim, const char *argument )
{
    act_non_wizi( argument, ch, NULL, victim, TO_VICT );
}

DEF_DO_FUN(do_mpechoat)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
	return;

    if ( (victim = pget_char_room(ch, arg)) == NULL )
        return;

    mpechoat( ch, victim, argument );
}

/*
 * Prints the message to the room at large
 *
 * Syntax: mpecho [string]
 */
DEF_DO_FUN(do_mpecho)
{
    if ( argument[0] == '\0' )
	return;
    act_non_wizi( argument, ch, NULL, NULL, TO_ROOM );
}

/*
 * Lets the mobile load another mobile.
 *
 * Syntax: mob mload [vnum]
 */
CHAR_DATA * mpmload( CHAR_DATA *ch, const char *argument )
{
    char            arg[ MAX_INPUT_LENGTH ];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA      *victim;
    int vnum;

    one_argument( argument, arg );

    if ( ch->in_room == NULL || arg[0] == '\0' || !is_r_number(arg) )
	return NULL;

    vnum = r_atoi( ch,arg);
    if ( ( pMobIndex = get_mob_index( vnum ) ) == NULL )
    {
	sprintf( arg, "Mpmload: bad mob index (%d) from mob %d",
	    vnum, IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	bug( arg, 0 );
	return NULL;
    }
    victim = create_mobile( pMobIndex );
    arm_npc( victim );
    char_to_room( victim, ch->in_room );
    return victim;
}

DEF_DO_FUN(do_mpmload)
{
    mpmload( ch, argument);
}


/*
 * Lets the mobile load an object
 *
 * Syntax: mob oload [vnum] [level] {R|W}
 */
DEF_DO_FUN(do_mpoload)
{
    char arg1[ MAX_INPUT_LENGTH ];
    char arg2[ MAX_INPUT_LENGTH ];
    char arg3[ MAX_INPUT_LENGTH ];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA       *obj;
    int             level;
    bool            fToroom = FALSE, fWear = FALSE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
               one_argument( argument, arg3 );
 
    if ( arg1[0] == '\0' || !is_r_number( arg1 ) )
    {
        bug( "Mpoload - Bad syntax from vnum %d.",
	    IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
        return;
    }
 
    if ( arg2[0] == '\0' )
    {
	level = get_trust( ch );
    }
    else
    {
	/*
	 * New feature from Alander.
	 */
        if ( !is_number( arg2 ) )
        {
	    bug( "Mpoload - Bad syntax from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	    return;
        }
	level = atoi( arg2 );
	if ( level < 0 )
	{
	    bug( "Mpoload - Bad level from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	    return;
	}
    }

    /*
     * Added 3rd argument
     * omitted - load to mobile's inventory
     * 'R'     - load to room
     * 'W'     - load to mobile and force wear
     */
    if ( arg3[0] == 'R' || arg3[0] == 'r' )
	fToroom = TRUE;
    else if ( arg3[0] == 'W' || arg3[0] == 'w' )
	fWear = TRUE;

    if ( ( pObjIndex = get_obj_index( r_atoi( ch, arg1 ) ) ) == NULL )
    {
	bug( "Mpoload - Bad vnum arg from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    obj = create_object(pObjIndex);
    check_enchant_obj( obj );
    if ( (fWear || !fToroom) && !CAN_WEAR(obj, ITEM_NO_CARRY) )
    {
	obj_to_char( obj, ch );
	if ( fWear )
	    wear_obj( ch, obj, TRUE );
    }
    else
    {
	obj_to_room( obj, ch->in_room );
    }

    return;
}

/*
 * Lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room. The mobile cannot
 * purge itself for safety reasons.
 *
 * syntax mob purge {target}
 */
DEF_DO_FUN(do_mppurge)
{
    char       arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;
    char buf[MSL];

    if ( ch->in_room == NULL )
        return;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        /* 'purge' */
        CHAR_DATA *vnext;
        OBJ_DATA  *obj_next;

	for ( victim = ch->in_room->people; victim != NULL; victim = vnext )
	{
	    vnext = victim->next_in_room;
	    if ( IS_NPC( victim ) && victim != ch 
		 && !IS_SET(victim->act, ACT_NOPURGE) )
	    {
		extract_char( victim, TRUE );
	    }
	}

	for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if ( !IS_SET(obj->extra_flags, ITEM_NOPURGE) )
		extract_obj( obj );
	}

	return;
    }

    if ( !strcmp(arg, "area") )
    {
	purge_area( ch->in_room->area );
	return;
    }
    

    if ( (victim = pget_char_room(ch, arg)) == NULL )
    {
	if ( ( obj = get_obj_here( ch, arg ) ) )
	{
	    extract_obj( obj );
	}
	else
	{
/*	    bug( "Mppurge - Bad argument from vnum %d.",
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 ); */
            sprintf( buf, "Mppurge - Bad argument from mob: %d, room: %d, argument: %s",
                IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
                ch->in_room != NULL ? ch->in_room->vnum : 0, 
                arg != NULL ? arg : "null");
            bug( buf, 0 );
	}
	return;
    }

    if ( !IS_NPC( victim ) )
    {
	bug( "Mppurge - Purging a PC from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    extract_char( victim, TRUE );

    return;
}


/*
 * Lets the mobile goto any location it wishes that is not private.
 *
 * Syntax: mob goto [location]
 */
DEF_DO_FUN(do_mpgoto)
{
    char             arg[ MAX_INPUT_LENGTH ];
    ROOM_INDEX_DATA *location;
    char buf[MSL];

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	bug( "Mpgoto - No argument from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    if ( ( location = find_mp_location( ch, arg ) ) == NULL )
    {
/*	bug( "Mpgoto - No such location from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 ); */
        sprintf( buf, "Mpgoto - No such location. mob: %d, target room: %s",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
            arg != NULL ? arg : "null");
        bug( buf, 0 );
	return;
    }

    if ( ch->fighting != NULL )
	stop_fighting( ch, TRUE );

    char_from_room( ch );
    char_to_room( ch, location );

    return;
}

/* 
 * Lets the mobile do a command at another location.
 *
 * Syntax: mob at [location] [commands]
 */
DEF_DO_FUN(do_mpat)
{
    char arg[ MAX_INPUT_LENGTH ];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    CHAR_DATA       *wch;
    OBJ_DATA 	    *on;
    char buf[MSL];

    argument = one_argument( argument, arg );

    /* call me paranoid.. */
    if ( ch == NULL )
	return;

    sprintf( last_debug, "mpat: start" );
    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
/*	bug( "Mpat - Bad argument from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );*/
        sprintf( buf, "Mpat - Bad argument from mob: %d, argument: %s",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
            arg != NULL ? arg : "null");
        bug( buf, 0 );
	return;
    }

    sprintf( last_debug, "mpat: find_mp_location" );
    if ( ( location = find_mp_location( ch, arg ) ) == NULL )
    {
/*	bug( "Mpat - No such location from vnum %d.",
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 ); */
        sprintf( buf, "Mpat - Bad location from mob: %d, target room: %s",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
            arg != NULL ? arg : "null");
        bug( buf, 0 );
	return;
    }

    sprintf( last_debug, "mpat: transfer to room" );
    original = ch->in_room;
    on = ch->on;
    char_from_room( ch );
    char_to_room( ch, location );
    sprintf( last_debug, "mpat: interpret" );
    interpret( ch, argument );
    sprintf( last_debug, "mpat: back to old room" );

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
	if ( wch == ch )
	{
	    char_from_room( ch );
	    char_to_room( ch, original );
	    ch->on = on;
	    break;
	}
    }
    strcpy( last_debug, "" );

    return;
}

// used by do_mptransfer and do_mpgtransfer
bool transfer_char( CHAR_DATA *victim, ROOM_INDEX_DATA *location )
{
    AREA_DATA *from_area;
    if ( !victim || !victim->in_room || !location )
        return FALSE;

    if ( room_is_private(location) )
        return FALSE;

    if ( victim->fighting != NULL )
        stop_fighting( victim, TRUE );

    if ( !rp_exit_trigger(victim) )
        return FALSE;
    if ( !ap_rexit_trigger(victim) )
        return FALSE;
    if ( !ap_exit_trigger(victim, location->area) )
        return FALSE;

    from_area=victim->in_room ? victim->in_room->area : NULL;

    char_from_room( victim );
    char_to_room( victim, location );

    do_look( victim, "auto" );
    
    if ( !IS_NPC(victim) )
    {
        ap_enter_trigger(victim, from_area);
        ap_renter_trigger(victim);
        rp_enter_trigger(victim);
        op_greet_trigger(victim);
        mp_greet_trigger(victim);
    }

    return TRUE;
}

/*
 * Lets the mobile transfer people.  The 'all' argument transfers
 *  everyone in the current room to the specified location
 *
 * Syntax: mob transfer [target|'all'] [location]
 */
DEF_DO_FUN(do_mptransfer)
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    ROOM_INDEX_DATA *location;
    CHAR_DATA       *victim;
    DESCRIPTOR_DATA *d;
    
    if ( ch->in_room == NULL )
        return;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' )
    {
        bug( "Mptransfer - Bad syntax from vnum %d.", 
            IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
        return;
    }

    if ( arg2[0] == '\0' )
    {
        location = ch->in_room;
    }
    else
    {
        if ( (location = find_mp_location(ch, arg2)) == NULL )
        {
            bugf("Mptransfer - Bad location from mob: %d, target room: %s",
                IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
                arg2 != NULL ? arg2 : "null");
            return;
        }
    }
    
    if ( !str_cmp( arg1, "all" ) )
    {
        CHAR_DATA *victim_next;
        
        for ( victim = ch->in_room->people; victim != NULL; victim = victim_next )
        {
            victim_next = victim->next_in_room;
            if (victim != ch
                && !NOT_AUTHED(victim)
                && can_see(ch, victim)
                && !IS_NPC(victim) )
            {
                transfer_char(victim, location);
            }
        }
        return;
    }
    
    if ( !str_cmp( arg1, "area" ) )
    {
        for ( d = descriptor_list; d; d = d->next )
        {
            if (   !d->character 
                || !(IS_PLAYING(d->connected))
                || !can_see(ch, d->character)
                || ch->in_room->area != d->character->in_room->area 
                || d->character->level == 1 )
                continue;
            transfer_char(d->character, location);
        }
        return;
    }

    if ( (victim = get_mp_char(ch, arg1) ) == NULL )
        return;

    transfer_char(victim, location);
    
    return;
}

/*
 * Lets the mobile transfer all chars in same group as the victim.
 *
 * Syntax: mob gtransfer [victim] [location]
 */
DEF_DO_FUN(do_mpgtransfer)
{
    char             arg1[ MAX_INPUT_LENGTH ];
    char             arg2[ MAX_INPUT_LENGTH ];
    CHAR_DATA       *who, *victim, *victim_next;
    ROOM_INDEX_DATA *location;

    if ( ch->in_room == NULL )
        return;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg2[0] == '\0' )
    {
        location = ch->in_room;
    }
    else
    {
        if ( (location = find_mp_location(ch, arg2)) == NULL )
        {
            bugf("Mpgtransfer - Bad location from mob: %d, target room: %s",
                IS_NPC(ch) ? ch->pIndexData->vnum : 0,
                arg2 != NULL ? arg2 : "null");
            return;
        }
    }

    if ( (who = pget_char_room(ch, arg1)) == NULL )
        return;

    for ( victim = ch->in_room->people; victim; victim = victim_next )
    {
    	victim_next = victim->next_in_room;
    	if( is_same_group( who,victim ) )
    	{
            transfer_char(victim, location);
    	}
    }
    return;
}

/*
 * Lets the mobile force someone to do something. Must be mortal level
 * and the all argument only affects those in the room with the mobile.
 *
 * Syntax: mob force [victim] [command]
 */
DEF_DO_FUN(do_mpforce)
{
    char arg[ MAX_INPUT_LENGTH ];
    char buf[MSL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
/*	bug( "Mpforce - Bad syntax from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 ); */
        sprintf( buf, "Mpforce - Bad syntax from mob: %d, target room: %s",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
            arg != NULL ? arg : "null");
        bug( buf, 0 );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

	for ( vch = char_list; vch != NULL; vch = vch_next )
	{
	    vch_next = vch->next;

	    if ( vch->in_room == ch->in_room
		&& get_trust( vch ) < get_trust( ch ) 
		&& can_see( ch, vch ) )
	    {
		interpret( vch, argument );
	    }
	}
    }
    else
    {
	CHAR_DATA *victim;

    if ( (victim = pget_char_room(ch, arg)) == NULL )
        return;

	if ( victim == ch )
	    return;

	interpret( victim, argument );
    }

    return;
}

/*
 * Lets the mobile force a group something. Must be mortal level.
 *
 * Syntax: mob gforce [victim] [commands]
 */
DEF_DO_FUN(do_mpgforce)
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *victim, *vch, *vch_next;
    char buf[MSL];

    if ( ch->in_room == NULL )
        return;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
/*	bug( "MpGforce - Bad syntax from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 ); */
        sprintf( buf, "MpGforce - Bad syntax from mob: %d, target room: %s",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
            arg != NULL ? arg : "null");
        bug( buf, 0 );
	return;
    }

    if ( (victim = pget_char_room(ch, arg)) == NULL )
        return;

    if ( victim == ch )
	return;

    for ( vch = victim->in_room->people; vch != NULL; vch = vch_next )
    {
	vch_next = vch->next_in_room;

	if ( is_same_group(victim,vch) )
        {
	    interpret( vch, argument );
	}
    }
    return;
}

/*
 * Forces all mobiles of certain vnum to do something (except ch)
 *
 * Syntax: mob vforce [vnum] [commands]
 */
DEF_DO_FUN(do_mpvforce)
{
    CHAR_DATA *victim, *victim_next;
    char arg[ MAX_INPUT_LENGTH ];
    int vnum;
    char buf[MSL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
/*	bug( "MpVforce - Bad syntax from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 ); */
        sprintf( buf, "MpVforce - Bad syntax from mob: %d, target room: %s",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
            arg != NULL ? arg : "null");
        bug( buf, 0 );
	return;
    }

    if ( !is_r_number( arg ) )
    {
	bug( "MpVforce - Non-number argument vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    vnum = r_atoi( ch, arg );

    for ( victim = char_list; victim; victim = victim_next )
    {
	victim_next = victim->next;
	if ( IS_NPC(victim) && victim->pIndexData->vnum == vnum
	&&   ch != victim && victim->fighting == NULL )
	    interpret( victim, argument );
    }
    return;
}

/*
 * Lets the mobile cast spells --
 * Beware: this does only crude checking on the target validity
 * and does not account for mana etc., so you should do all the
 * necessary checking in your mob program before issuing this cmd!
 *
 * Syntax: mob cast [spell] {target}
 */
DEF_DO_FUN(do_mpcast)
{
    void *victim = NULL;
    char spell[ MAX_INPUT_LENGTH ];
    int sn, target_type;
    char buf[MSL];

    target_name = one_argument( argument, spell );

    if ( spell[0] == '\0' )
    {
/*	bug( "MpCast - Bad syntax from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 ); */
        sprintf( buf, "MpCast - Bad syntax from mob: %d, spell name: %s",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
            spell != NULL ? spell : "null");
        bug( buf, 0 );
	return;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
    {
/*	bug( "MpCast - No such spell from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 ); */
        sprintf( buf, "MpCast - Bad syntax from mob: %d, spell name: %s",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0, 
            spell != NULL ? spell : "null");
        bug( buf, 0 );
	return;
    }
    
    if ( !get_spell_target( ch, target_name, sn, &target_type, &victim ) )
	return;
    victim = check_reflection( sn, ch->level, ch, victim, target_type );

    (*skill_table[sn].spell_fun)( sn, ch->level, ch, victim, target_type, FALSE );
    return;
}

/*
 * Lets mob cause unconditional damage to someone. Nasty, use with caution.
 * Also, this is silent, you must show your own damage message...
 *
 * Syntax: mob damage [victim|'all'] [min] [max] {kill}
 */
DEF_DO_FUN(do_mpdamage)
{
    CHAR_DATA *victim = NULL, *victim_next;
    char target[ MAX_INPUT_LENGTH ],
	 min[ MAX_INPUT_LENGTH ],
	 max[ MAX_INPUT_LENGTH ];
    int low, high, dam;
    bool fAll = FALSE, fKill = FALSE;

    if ( ch->in_room == NULL )
        return;

    argument = one_argument( argument, target );
    argument = one_argument( argument, min );
    argument = one_argument( argument, max );

    if ( target[0] == '\0' )
    {
	bug( "MpDamage - Bad syntax from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }
    if( !str_cmp( target, "all" ) )
	fAll = TRUE;
    else if ( (victim = pget_char_room(ch, target)) == NULL )
	return;

    if ( is_number( min ) )
	low = atoi( min );
    else
    {
	bug( "MpDamage - Bad damage min, vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }
    if ( is_number( max ) )
	high = atoi( max );
    else
    {
	bug( "MpDamage - Bad damage max, vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    /*
     * If kill parameter is omitted, this command is "safe" and will not
     * kill the victim.
     */
    one_argument( argument, target );
    if ( target[0] != '\0' )
	fKill = TRUE;

    dam = number_range(low, high);
    
    if ( fAll )
    {
        for( victim = ch->in_room->people; victim; victim = victim_next )
        {
	        victim_next = victim->next_in_room;
	        if ( victim != ch )
                deal_damage(victim, victim, dam, TYPE_UNDEFINED, DAM_NONE, FALSE, fKill);
        }
    }
    else
        deal_damage(victim, victim, dam, TYPE_UNDEFINED, DAM_NONE, FALSE, fKill);
    return;
}

/*
 * Lets the mobile to remember a target. The target can be referred to
 * with $q and $Q codes in MOBprograms. See also "mob forget".
 *
 * Syntax: mob remember [victim]
 */
DEF_DO_FUN(do_mpremember)
{
    char arg[ MAX_INPUT_LENGTH ];
    one_argument( argument, arg );
    if ( arg[0] != '\0' )
	ch->mprog_target = get_mp_char( ch, arg );
    else
	bug( "MpRemember: missing argument from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
}

/*
 * Reverse of "mob remember".
 *
 * Syntax: mob forget
 */
DEF_DO_FUN(do_mpforget)
{
    ch->mprog_target = NULL;
}

/*
 * Sets a delay for MOBprogram execution. When the delay time expires,
 * the mobile is checked for a MObprogram with DELAY trigger, and if
 * one is found, it is executed. Delay is counted in PULSE_MOBILE
 *
 * Syntax: mob delay [pulses]
 */
DEF_DO_FUN(do_mpdelay)
{
    char arg[ MAX_INPUT_LENGTH ];

    one_argument( argument, arg );
    if ( !is_number( arg ) )
    {
	bug( "MpDelay: invalid arg from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }
    ch->mprog_delay = atoi( arg );
}

/*
 * Reverse of "mob delay", deactivates the timer.
 *
 * Syntax: mob cancel
 */
DEF_DO_FUN(do_mpcancel)
{
   ch->mprog_delay = -1;
}
/*
 * Lets the mobile to call another MOBprogram withing a MOBprogram.
 * This is a crude way to implement subroutines/functions. Beware of
 * nested loops and unwanted triggerings... Stack usage might be a problem.
 * Characters and objects referred to must be in the same room with the
 * mobile.
 *
 * Syntax: mob call [vnum] [victim|'null'] [object1|'null'] [object2|'null']
 *
 */
DEF_DO_FUN(do_mpcall)
{
    char arg[ MAX_INPUT_LENGTH ];
    CHAR_DATA *vch;
    OBJ_DATA *obj1, *obj2;
    PROG_CODE *prg;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	bug( "MpCall: missing arguments from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }
    if ( ( prg = get_mprog_index( r_atoi( ch,arg) ) ) == NULL )
    {
	bug( "MpCall: invalid prog from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }
    vch = NULL;
    obj1 = obj2 = NULL;
    argument = one_argument( argument, arg );
    if ( arg[0] != '\0' )
        vch = pget_char_room(ch, arg);
    argument = one_argument( argument, arg );
    if ( arg[0] != '\0' )
    	obj1 = get_obj_here( ch, arg );
    argument = one_argument( argument, arg );
    if ( arg[0] != '\0' )
    	obj2 = get_obj_here( ch, arg );
    program_flow( argument, prg->is_lua,prg->vnum, prg->code, ch, vch, (void *)obj1, ACT_ARG_OBJ, (void *)obj2, ACT_ARG_OBJ, TRIG_CALL, prg->security );
}

/*
 * Lets the mobile to transfer an object. The object must be in the same
 * room with the mobile.
 *
 * Syntax: mob otransfer [item name] [location]
 */
DEF_DO_FUN(do_mpotransfer)
{
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *location;
    char arg[ MAX_INPUT_LENGTH ];
    char buf[ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	bug( "MpOTransfer - Missing argument from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }
    one_argument( argument, buf );
    if ( ( location = find_mp_location( ch, buf ) ) == NULL )
    {
	bug( "MpOTransfer - No such location from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }
    if ( (obj = get_obj_here( ch, arg )) == NULL )
	return;
    if ( obj->carried_by == NULL )
	obj_from_room( obj );
    else
    {
	if ( obj->wear_loc != WEAR_NONE )
	    unequip_char( ch, obj );
	obj_from_char( obj );
    }
    obj_to_room( obj, location );
}

/*
 * Lets the mobile to strip an object or all objects from the victim.
 * Useful for removing e.g. quest objects from a character.
 *
 * Syntax: mob remove [victim] [object vnum|'all'] {inv|get|room}
 */
DEF_DO_FUN(do_mpremove)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj, *obj_next;
    int vnum = 0;
    bool fAll = FALSE;
    char arg[MIL], arg2[MIL];

    argument = one_argument( argument, arg );
    if ( (victim = pget_char_room(ch, arg)) == NULL )
        return;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );
    if ( !str_cmp( arg, "money" ) )
    {
	victim->gold = 0;
	victim->silver = 0;
	return;
    }
    else if ( !str_cmp( arg, "all" ) )
	fAll = TRUE;
    else if ( !is_r_number( arg ) )
    {
	/*
	bug ( "MpRemove: Invalid object from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	*/
	int loc = flag_lookup(arg, wear_loc_flags);
	if ( loc == NO_FLAG )
	{
	    bug ( "MpRemove: Invalid wear-location from vnum %d.", 
		  IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	    return;
	}
	if ( (obj = get_eq_char(victim, loc)) == NULL )
	    return;
	obj_from_char( obj );
	if ( !strcmp(arg2, "inv") || !strcmp(arg2, "inventory"))
	    obj_to_char( obj, victim );
	else if ( !strcmp(arg2, "get") )
	    obj_to_char( obj, ch );
	else if ( !strcmp(arg2, "room") )
	    obj_to_room( obj, ch->in_room );
	else
	    extract_obj( obj );

	return;
    }
    else
	vnum = r_atoi( ch, arg );

    for ( obj = victim->carrying; obj; obj = obj_next )
    {
        obj_next = obj->next_content;
        if ( fAll || obj->pIndexData->vnum == vnum )
        {
            obj_from_char( obj );
            if ( !strcmp(arg2, "inv") || !strcmp(arg2, "inventory"))
                obj_to_char( obj, victim );
            else if ( !strcmp(arg2, "get") )
                obj_to_char( obj, ch );
            else if ( !strcmp(arg2, "room") )
                obj_to_room( obj, ch->in_room );
            else
                extract_obj( obj );
            // when removing specific object, only remove first
            if (!fAll)
                break;
        }
    }
}

void mpremort( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( IS_NPC(victim) )
    {
        bugf("mpremort: target is NPC. %s(%d)",
                victim->name, victim->pIndexData->vnum);
        return;
    }

    // usage should be logged
    if ( IS_NPC(ch) )
        logpf("do_mpremort(%d): remorting %s", ch->pIndexData->vnum, victim->name);
    else
        logpf("do_mpremort(%s): remorting %s", ch->name, victim->name);

#ifndef TESTER
    if ( !is_in_remort(victim) )
    {
        bugf("do_mpremort(%d): %s is not in remort!", (ch->pIndexData ? ch->pIndexData->vnum : 0), victim->name);
        return;
    }
#endif

    victim->pcdata->remorts++;
    remort_begin(victim);
}


DEF_DO_FUN(do_mpremort)
{
    CHAR_DATA *victim;
    char arg[ MAX_INPUT_LENGTH ];

    argument = one_argument( argument, arg );
    if ( (victim = pget_char_room(ch, arg)) == NULL || IS_NPC(victim) )
        return;
    mpremort(ch, victim);
}


/* Auth mobprog commands, ported from Smaug by Rimbol. */
DEF_DO_FUN(do_mpapply)
{
    CHAR_DATA *victim;
    
    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ))
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }
    
    if (argument[0] == '\0')
    {
        bug("Mpapply - bad syntax from vnum %d",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0 );            
        return;
    }
    
    if ( (victim = pget_char_room(ch, argument) ) == NULL )
    {
        bug("Mpapply - no such player in room from vnum %d.",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
        return;
    }
    
    if ( !victim->desc )
    {
        send_to_char( "Not on linkdeads.\n\r", ch );
        return;
    }
    
    if( !NOT_AUTHED(victim) )
        return;
    
    if( victim->pcdata->auth_state >= 1 )
        return;
    
    sprintf( log_buf, "%s@%s new %s %s applying...", 
        victim->name, victim->desc->host, 
        race_table[victim->race].name, 
        class_table[victim->class].who_name);

    wiznet(log_buf, victim, NULL, WIZ_AUTH, 0, LEVEL_IMMORTAL);
    victim->pcdata->auth_state = 1;
    return;
}

/* Auth mobprog commands, ported from Smaug by Rimbol. */
/* Based on new auth version by Samson. */
DEF_DO_FUN(do_mpapplyb)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    
    
    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ))
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }
    
    if (argument[0] == '\0')
    {
        bug("Mpapplyb - bad syntax from vnum %d",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
        return;
    }
    
    if ( (victim = pget_char_room(ch, argument)) == NULL )
    {
        bug("Mpapplyb - no such player in room from vnum %d",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
        return;
    }
    
    if ( !victim->desc )
    {
        send_to_char( "Not on linkdeads.\n\r", ch );
        return;
    }
    
    if ( victim->fighting )
        stop_fighting( victim, TRUE );

    char_from_room(victim);
    char_to_room(victim, get_room_index(ROOM_VNUM_SCHOOL));
    act_new( "$n enters this world from within a column of blinding light!",
        victim, NULL, NULL, TO_ROOM, POS_RESTING );
    do_look(victim, "auto");

    if ( NOT_AUTHED( victim ) )
    {
        sprintf( log_buf, "[%s@%s] New player entering the game.\n\r", 
                victim->name,
                victim->desc->host );
        
        wiznet(log_buf, victim, NULL, WIZ_AUTH, 0, LEVEL_IMMORTAL);        

        sprintf( buf, "\n\rYou are now entering the game...\n\r"
            "However, your character has not been authorized yet and can not\n\r"
            "advance past level 5 until approved. Your character will be saved,\n\r"
            "but not yet allowed to fully indulge in the MUD.\n\r" );
        send_to_char( buf, victim );
    }
    
    return;
}

/* stuff for special quests --Bobble */

void mpqset( CHAR_DATA *ch, CHAR_DATA *victim, const char *arg2, const char *arg3, int timer, int limit )
{
    if (IS_NPC(victim))
    {
        bugf("mpqset: target is NPC. %s(%d)",
                victim->name, victim->pIndexData->vnum);

        return;
    }

    set_quest_status( victim, r_atoi( ch,arg2), atoi(arg3), timer, limit );
}

/* Syntax: mob qset $n [id] [status] */
DEF_DO_FUN(do_mpqset)
{
    CHAR_DATA *victim;
    static char arg[MAX_INPUT_LENGTH];
    static char arg2[MIL];
    static char arg3[MIL];
    static char arg4[MIL];
    static char arg5[MIL];
    int timer;
    int limit;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );
    argument = one_argument( argument, arg5 );

    if ( arg[0] == 0 || arg2[0] == 0 || arg3[0] == 0 )
    {
	bug ( "do_mpqset: Too few arguments from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    if ( !is_r_number(arg2) || !is_number(arg3) )
    {
	bug ( "do_mpqset: invalid arguments from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }


    /* see act_wiz.c for notes - This is the timer - Astark Oct 2012 */
    
    if ( !is_number(arg4) )
        timer = 0;
    else    
        timer = atoi(arg4);

    /* see act_wiz.c for notes - This is the limit - Astark Oct 2012*/

    if ( !is_number(arg5) )
        limit = 0;
    else    
        limit = atoi(arg5);

    if ( (victim = pget_char_room(ch, arg)) == NULL )
        return;

    mpqset( ch, victim, arg2, arg3, timer, limit );
}

void mpqadvance( CHAR_DATA *ch, CHAR_DATA *victim, const char *arg2, const char *arg3 )
{
    int increment;

    if (IS_NPC(victim))
    {
        bugf("mpqadvance: target is NPC. %s(%d)",
                victim->name, victim->pIndexData->vnum);
        return;
    }

    /* do the advance */
    int id = r_atoi( ch, arg2 );
    if ( arg3[0] == 0 )
    increment = 1;
    else
    increment = atoi( arg3 );


    int old_status = quest_status( victim, id );
    int timer = qset_timer(victim, id );
    set_quest_status( victim, id, old_status + increment, timer, 0 );
}

/* Syntax: mob qadvance $n [id] {increment} */
DEF_DO_FUN(do_mpqadvance)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MIL];
    char arg3[MIL];

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg[0] == 0 || arg2[0] == 0 )
    {
	bug ( "do_mpadvance: Too few arguments from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    if ( !is_r_number(arg2) || (arg3[0] != 0 && !is_number(arg3)) )
    {
	bug ( "do_mpadvance: invalid arguments from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    if ( (victim = pget_char_room(ch, arg)) == NULL )
        return;

    mpqadvance( ch, victim, arg2, arg3 );
}

#define REWARD_EXP     0
#define REWARD_QP      1
#define REWARD_GOLD    2 
void mpreward( CHAR_DATA *ch, CHAR_DATA *victim, const char *arg2, int amount )
{
    int reward;
    char buf[MSL];

    if (IS_NPC(victim) )
    {
        bugf("mpreward: victim is NPC: %s(%d)",
                victim->name, victim->pIndexData->vnum);
        return;
    }

    /* which reward type do we have? */
    if ( !str_cmp(arg2, "exp") )
        reward = REWARD_EXP;
    else if ( !str_cmp(arg2, "qp") )
        reward = REWARD_QP;
    else if ( !str_cmp(arg2, "gold") )
        reward = REWARD_GOLD;
    else
    {
        bug( "mpreward: unknown reward type from vnum %d.",
            IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
        return;
    }

    /* log the action */
    sprintf( buf, "%s was rewarded %d %s by mob %d.",
         victim->name, amount, arg2,
         IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
    log_string( buf );
    wiznet(buf,victim,NULL,WIZ_SECURE,0,0);

    /* now the reward */
    switch ( reward )
    {
        case REWARD_EXP:
        gain_exp( victim, amount );
        break;
    case REWARD_QP:
        sprintf( buf, "You are rewarded %d quest point%s!\n\r",
                        amount, amount == 1 ? "" : "s" );
        send_to_char( buf, victim );
        victim->pcdata->questpoints += amount;
        break;
    case REWARD_GOLD:
        ptc( victim, "You are rewarded %d gold!\n\r", amount);
        victim->gold += amount;
        break;
    default:
        bugf( "mpreward: unknown reward type (%d)", reward );
    }
}

/* Syntax: mob reward $n [exp|qp|gold] [amount] */
DEF_DO_FUN(do_mpreward)
{
    CHAR_DATA *victim;
    static char arg1[MIL];
    static char arg2[MIL];
    static char arg3[MIL];
    int amount;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == 0 || arg2[0] == 0 || arg3[0] == 0 )
    {
	bug ( "do_mpreward: Too few arguments from vnum %d.", 
		IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }

    /* check amount */
    if ( !is_number(arg3) )
    {
	bug( "do_mpreward: invalid amount from vnum %d.", 
	     IS_NPC(ch) ? ch->pIndexData->vnum : 0 );
	return;
    }
    amount = atoi( arg3 );

    if ( (victim = pget_char_room(ch, arg1)) == NULL )
	return;

    if ( IS_NPC(victim) || victim->pcdata == NULL )
	return;

    mpreward( ch, victim, arg2, amount);
}

DEF_DO_FUN(do_mppeace)
{
    /* call imm command, does same thing */
    do_peace( ch, argument );
}

DEF_DO_FUN(do_mprestore)
{
    CHAR_DATA *victim = pget_char_room(ch, argument);

    if ( victim == NULL )
	return;

    restore_char( victim );
}

DEF_DO_FUN(do_mpact)
{
    int value;
    char arg1[MIL];
    bool not = FALSE;
    
    if ( !IS_NPC(ch) )
    {
	send_to_char( "You're not an NPC.\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    if ( !strcmp(arg1, "not") )
    {
	not = TRUE;
	one_argument( argument, arg1 );
    }

    if ( (value = flag_lookup(arg1, act_flags)) == NO_FLAG )
    {
	bugf( "do_mpact: unknown flag <%s> from mob %d",
	      argument, ch->pIndexData->vnum );
	return;
    }

    if ( !is_settable(value, act_flags) )
	return;

    if ( not )
	REMOVE_BIT( ch->act, value );
    else
	SET_BIT( ch->act, value );
}

DEF_DO_FUN(do_mphit)
{
    CHAR_DATA *victim;

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
    {
        bugf( "do_mphit: no victim found for %s(%d), argument: %s",
                ch->name,
                IS_NPC(ch) ? ch->pIndexData->vnum : 0,
                argument);
	    return;
    }

    one_hit( ch, victim, TYPE_UNDEFINED, FALSE );
}
