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
*   ROM 2.4 is copyright 1993-1996 Russ Taylor             *
*   ROM has been brought to you by the ROM consortium          *
*       Russ Taylor (rtaylor@efn.org)                  *
*       Gabrielle Taylor                           *
*       Brian Moore (zump@rom.org)                     *
*   By using this code, you have agreed to follow the terms of the     *
*   ROM license, in the file Rom24/doc/rom.license             *
***************************************************************************/

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "simsave.h"
#include "buffer_util.h"

int     execl           args( ( const char *path, const char *arg, ... ) );
int close       args( ( int fd ) );



/* command procedures needed */
DECLARE_DO_FUN(do_mload     );
DECLARE_DO_FUN(do_oload     );
DECLARE_DO_FUN(do_quit      );
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_stand     );
DECLARE_DO_FUN(do_help      );
DECLARE_DO_FUN(do_grant     );
DECLARE_DO_FUN(do_revoke    );
DECLARE_DO_FUN( do_pipe     );


/*
* Local functions.
*/
void  sort_reserved  args( ( RESERVED_DATA *pRes ) );

DEF_DO_FUN(do_wiznet)
{
    int flag, col = 0;
    char buf[MAX_STRING_LENGTH];
    
    if ( argument[0] == '\0' )
        /* Show wiznet options - just like channel command */
    {
        send_to_char("{V{+WELCOME TO WIZNET!{x\n\r", ch);
        send_to_char("{VOption        Level Status{x\n\r",ch);
        send_to_char("{V--------------------------{x\n\r",ch);
        /* list of all wiznet options */
        buf[0] = '\0';
        
        for (flag = 0; wiznet_table[flag].name != NULL; flag++)
        {
            if (wiznet_table[flag].level <= get_trust(ch))
            {
                sprintf( buf, "{V%-14s%d{x   %-6s   ", 
                    capitalize(wiznet_table[flag].name),
                    wiznet_table[flag].level,
                    IS_SET(ch->wiznet,wiznet_table[flag].flag) ? "{YOn" : "{GOff" );
                send_to_char(buf, ch);   
                col++;
                
                if (col == 3)
                {
                    send_to_char("\n\r",ch);
                    col = 0;
                }
            }
        }
        /* To avoid color bleeding */
        send_to_char("{x\n\r",ch);
        return;
    }    
    
    if (!str_prefix(argument,"on"))
    {     
        send_to_char("{VWelcome to Wiznet!{x\n\r",ch);
        SET_BIT(ch->wiznet,WIZ_ON);
        return;
    }
    
    if (!str_prefix(argument,"off"))
    {
        send_to_char("{VSigning off of Wiznet.{x\n\r",ch);
        REMOVE_BIT(ch->wiznet,WIZ_ON);
        return;
    }
    
    flag = wiznet_lookup(argument);
    
    if (flag == -1 || get_trust(ch) < wiznet_table[flag].level) 
    {
        send_to_char("{VNo such option.{x\n\r",ch);
        return;
    }
    
    if (IS_SET(ch->wiznet,wiznet_table[flag].flag))
    {
        sprintf(buf,"{VYou will no longer see %s on wiznet.{x\n\r",
            wiznet_table[flag].name);
        send_to_char(buf,ch);
        REMOVE_BIT(ch->wiznet,wiznet_table[flag].flag);
        return;
    }
    else  
    {
        sprintf(buf,"{VYou will now see %s on wiznet.{x\n\r",
            wiznet_table[flag].name);
        send_to_char(buf,ch);
        SET_BIT(ch->wiznet,wiznet_table[flag].flag);
        return;
    }
}



void wiznet( const char *string, CHAR_DATA *ch, const void *arg1, long flag, long flag_skip, int min_level )
{
    DESCRIPTOR_DATA *d;
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        bool playing = IS_PLAYING(d->connected);
        if ( !playing || !d->character || d->character == ch )
            continue;
        bool auth_match = flag == WIZ_AUTH && CAN_AUTH(d->character);
        bool imm_match = IS_IMMORTAL(d->character) && get_trust(d->character) >= min_level && IS_SET(d->character->wiznet,WIZ_ON) && (!flag || IS_SET(d->character->wiznet,flag));
        bool skip_match = flag_skip && IS_SET(d->character->wiznet,flag_skip);
        if ( auth_match || (imm_match && !skip_match) )
        {
            if (IS_SET(d->character->wiznet,WIZ_PREFIX))
                send_to_char("{V--> ",d->character);
            else
                send_to_char("{V", d->character);
            act_new(string, d->character, arg1, ch, TO_CHAR, POS_DEAD);
            send_to_char("{x", d->character );
        }
    }
    
    return;
}



/* equips a character */
DEF_DO_FUN(do_outfit)
{
    OBJ_DATA *obj;
    int i,sn,vnum;
    
    if ( (!IS_IMMORTAL(ch) && ch->level > 5) || IS_NPC(ch) )
    {
        send_to_char("Find it yourself!\n\r",ch);
        return;
    }
    
    if (ch->carry_number+4 > can_carry_n(ch))
    {
        send_to_char("Drop something first.\n\r",ch);
        return;
    }
    
    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_LIGHT );
    }
    
    if ( ( obj = get_eq_char( ch, WEAR_TORSO ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_TORSO );
    }


    if ( ch->pcdata->remorts < 1 )
    {
    	if ( ( obj = get_obj_carry( ch, "guide", ch ) ) == NULL )
    	{
        	obj = create_object( get_obj_index(OBJ_VNUM_NEWBIE_GUIDE), 0 );
        	obj->cost = 0;
       		obj_to_char( obj, ch );
    	}
	if ( ( obj = get_obj_carry( ch, "map", ch ) ) == NULL )
        {
                obj = create_object( get_obj_index(OBJ_VNUM_MAP), 0 );
                obj->cost = 0;
                obj_to_char( obj, ch );
        }
    }

    
    /* do the weapon thing */
    if ((obj = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
        sn = 0; 
        vnum = OBJ_VNUM_SCHOOL_SWORD; /* just in case! */
        
        for (i = 0; weapon_table[i].name != NULL; i++)
        {
            if ( get_skill(ch, sn) < 
                get_skill(ch, *weapon_table[i].gsn) )
            {
                sn = *weapon_table[i].gsn;
                vnum = weapon_table[i].vnum;
            }
        }
        if (get_skill(ch, sn) > get_skill( ch, gsn_hand_to_hand) )
        {
            obj = create_object(get_obj_index(vnum),0);
            obj_to_char(obj,ch);
            equip_char(ch,obj,WEAR_WIELD);
        }
    }
    
    if (((obj = get_eq_char(ch,WEAR_WIELD)) == NULL 
        ||   !IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS)) 
        &&  (obj = get_eq_char( ch, WEAR_SHIELD ) ) == NULL
        && (ch->pcdata->learned[gsn_shield_block]>1) )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
        /* equip_char( ch, obj, WEAR_SHIELD ); */
    }
    
    send_to_char("You have been equipped by Rimbol.\n\r",ch);
}


DEF_DO_FUN(do_smote)
{
    CHAR_DATA *vch;
    const char *letter, *name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    size_t matches = 0;
    
    if ( !IS_NPC(ch) && IS_SET(ch->penalty, PENALTY_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }
    
    if (strstr(argument,ch->name) == NULL)
    {
        send_to_char("You must include your name in an smote.\n\r",ch);
        return;
    }
    
    send_to_char(argument,ch);
    send_to_char("\n\r",ch);
    
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;
        
        if ((letter = strstr(argument,vch->name)) == NULL)
        {
            send_to_char(argument,vch);
            send_to_char("\n\r",vch);
            continue;
        }
        
        strcpy(temp,argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        name = vch->name;
        
        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == strlen(vch->name))
            {
                strcat(temp,"r");
                continue;
            }
            
            if (*letter == 's' && matches == strlen(vch->name))
            {
                matches = 0;
                continue;
            }
            
            if (matches == strlen(vch->name))
            {
                matches = 0;
            }
            
            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == strlen(vch->name))
                {
                    strcat(temp,"you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat(last,letter,1);
                continue;
            }
            
            matches = 0;
            strcat(temp,last);
            strncat(temp,letter,1);
            last[0] = '\0';
            name = vch->name;
        }
        
        send_to_char(temp,vch);
        send_to_char("\n\r",vch);
    }
    
    return;
}

DEF_DO_FUN(do_bamfin)
{
    char buf[MAX_STRING_LENGTH];
    
    if ( !IS_NPC(ch) )
    {
        argument = smash_tilde_cc(argument);
        
        if (argument[0] == '\0')
        {
            sprintf(buf,"Your poofin is %s\n\r",ch->pcdata->bamfin);
            send_to_char(buf,ch);
            return;
        }
        
        if ( strstr(argument,ch->name) == NULL)
        {
            send_to_char("You must include your name.\n\r",ch);
            return;
        }
        
        free_string( ch->pcdata->bamfin );
        ch->pcdata->bamfin = str_dup( argument );
        
        sprintf(buf,"Your poofin is now %s\n\r",ch->pcdata->bamfin);
        send_to_char(buf,ch);
    }
    return;
}



DEF_DO_FUN(do_bamfout)
{
    char buf[MAX_STRING_LENGTH];
    
    if ( !IS_NPC(ch) )
    {
        argument = smash_tilde_cc(argument);
        
        if (argument[0] == '\0')
        {
            sprintf(buf,"Your poofout is %s\n\r",ch->pcdata->bamfout);
            send_to_char(buf,ch);
            return;
        }
        
        if ( strstr(argument,ch->name) == NULL)
        {
            send_to_char("You must include your name.\n\r",ch);
            return;
        }
        
        free_string( ch->pcdata->bamfout );
        ch->pcdata->bamfout = str_dup( argument );
        
        sprintf(buf,"Your poofout is now %s\n\r",ch->pcdata->bamfout);
        send_to_char(buf,ch);
    }
    return;
}



DEF_DO_FUN(do_echo)
{
    DESCRIPTOR_DATA *d;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Global echo what?\n\r", ch );
        return;
    }
    
    for ( d = descriptor_list; d; d = d->next )
    {
        if ( IS_PLAYING(d->connected) ) 
        {
            if (get_trust(d->character) >= get_trust(ch))
                send_to_char( "global> ",d->character);
            send_to_char( argument, d->character );
            send_to_char( "\n\r",   d->character );
        }
    }
    
    return;
}



DEF_DO_FUN(do_recho)
{
    DESCRIPTOR_DATA *d;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Local echo what?\n\r", ch );
        
        return;
    }
    
    for ( d = descriptor_list; d; d = d->next )
    {
        if ( (IS_PLAYING(d->connected) ) 
            &&   d->character->in_room == ch->in_room )
        {
            if (get_trust(d->character) >= get_trust(ch))
                send_to_char( "local> ",d->character);
            send_to_char( argument, d->character );
            send_to_char( "\n\r",   d->character );
        }
    }
    
    return;
}

DEF_DO_FUN(do_zecho)
{
    DESCRIPTOR_DATA *d;
    
    if (argument[0] == '\0')
    {
        send_to_char("Zone echo what?\n\r",ch);
        return;
    }
    
    for (d = descriptor_list; d; d = d->next)
    {
        if ( (IS_PLAYING(d->connected) ) 
            &&  d->character->in_room != NULL && ch->in_room != NULL
            &&  d->character->in_room->area == ch->in_room->area)
        {
            if (get_trust(d->character) >= get_trust(ch))
                send_to_char("zone> ",d->character);
            send_to_char(argument,d->character);
            send_to_char("\n\r",d->character);
        }
    }
}

DEF_DO_FUN(do_pecho)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    
    argument = one_argument(argument, arg);
    
    if ( argument[0] == '\0' || arg[0] == '\0' )
    {
        send_to_char("Personal echo what?\n\r", ch); 
        return;
    }
    
    if  ( (victim = get_char_world(ch, arg) ) == NULL )
    {
        send_to_char("Target not found.\n\r",ch);
        return;
    }
    
    if (get_trust(victim) >= get_trust(ch) && get_trust(ch) != MAX_LEVEL)
        send_to_char( "personal> ",victim);
    
    send_to_char(argument,victim);
    send_to_char("\n\r",victim);
    send_to_char( "personal> ",ch);
    send_to_char(argument,ch);
    send_to_char("\n\r",ch);
}


DEF_DO_FUN(do_transfer)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' )
    {
        send_to_char( "Transfer whom (and where)?\n\r", ch );
        return;
    }
    
    if ( !str_cmp( arg1, "all" ) )
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( (IS_PLAYING(d->connected)) 
                &&   d->character != ch
                &&   d->character->in_room != NULL
                &&   can_see( ch, d->character ) )
            {
                char buf[MAX_STRING_LENGTH];
                sprintf( buf, "%s %s", d->character->name, arg2 );
                do_transfer( ch, buf );
            }
        }
        return;
    }
    
    /*
    * Thanks to Grodyn for the optional location parameter.
    */
    if ( arg2[0] == '\0' )
    {
        location = ch->in_room;
    }
    else
    {
        if ( ( location = find_location( ch, arg2 ) ) == NULL )
        {
            send_to_char( "No such location.\n\r", ch );
            return;
        }
        
        if ( !is_room_owner(ch,location) && room_is_private( location ) 
            &&  get_trust(ch) < MAX_LEVEL)
        {
            send_to_char( "That room is private right now.\n\r", ch );
            return;
        }
    }
    
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( victim->in_room == NULL )
    {
        send_to_char( "They are in limbo.\n\r", ch );
        return;
    }
    
    if ( victim->fighting != NULL )
        stop_fighting( victim, TRUE );
    act( "$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, location );
    act( "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
    if ( ch != victim )
        act( "$n has transferred you.", ch, NULL, victim, TO_VICT );
    do_look( victim, "auto" );
    send_to_char( "Ok.\n\r", ch );
}



DEF_DO_FUN(do_at)
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    OBJ_DATA *on;
    CHAR_DATA *wch;
    
    argument = one_argument( argument, arg );
    
    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
        send_to_char( "At where what?\n\r", ch );
        return;
    }
    
    if ( ( location = find_location( ch, arg ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }
    
    if (!is_room_owner(ch,location) && room_is_private( location ) 
        &&  get_trust(ch) < MAX_LEVEL)
    {
        send_to_char( "That room is private right now.\n\r", ch );
        return;
    }
    
    original = ch->in_room;
    on = ch->on;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );
    
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
    
    return;
}



DEF_DO_FUN(do_goto)
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    int count = 0;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Goto where?\n\r", ch );
        return;
    }
    
    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }
    
    count = 0;
    for ( rch = location->people; rch != NULL; rch = rch->next_in_room )
        count++;
    
    if (ch->level < L2)
    {
        if (!is_room_owner(ch,location) && room_is_private(location) 
            &&  (count > 1 || get_trust(ch) < MAX_LEVEL))
        {
            send_to_char( "That room is private right now.\n\r", ch );
            return;
        }
    }
    
    if ( ch->fighting != NULL )
        stop_fighting( ch, TRUE );
    
    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
            else
                act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }
    
    char_from_room( ch );
    char_to_room( ch, location );
    
    
    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
            else
                act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }
    
    do_look( ch, "auto" );
    return;
}

DEF_DO_FUN(do_copyove)
{
    send_to_char( "If you want to COPYOVER, spell it out.\n\r", ch );
    return;
}

DEF_DO_FUN(do_reboo)
{
    send_to_char( "If you want to REBOOT, spell it out.\n\r", ch );
    return;
}

DEF_DO_FUN(do_reboot)
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    
    if (ch->invis_level < LEVEL_HERO)
    {
        sprintf( buf, "Reboot by %s.", ch->name );
        do_echo( ch, buf );
    }
    
    merc_down = TRUE;
    final_player_save();
    for ( d = descriptor_list; d != NULL; d = d_next )
    {
        d_next = d->next;
        close_socket(d);
    }
    
    return;
}



DEF_DO_FUN(do_shutdow)
{
    send_to_char( "If you want to SHUTDOWN, spell it out.\n\r", ch );
    return;
}



DEF_DO_FUN(do_shutdown)
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    
    if (ch->invis_level < LEVEL_HERO)
        sprintf( buf, "Shutdown by %s.", ch->name );
    append_file( ch, SHUTDOWN_FILE, buf );
    strcat( buf, "\n\r" );
    if (ch->invis_level < LEVEL_HERO)
        do_echo( ch, buf );
    merc_down = TRUE;
    final_player_save();
    for ( d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        close_socket(d);
    }
    return;
}

DEF_DO_FUN(do_snoop)
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Snoop whom?\n\r", ch );
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( victim->desc == NULL )
    {
        send_to_char( "No descriptor to snoop.\n\r", ch );
        return;
    }
    
    if ( victim == ch )
    {
        send_to_char( "Cancelling all snoops.\n\r", ch );
        wiznet("$N stops being such a snoop.",
            ch,NULL,WIZ_SNOOPS,WIZ_SECURE,get_trust(ch));
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->snoop_by == ch->desc )
                d->snoop_by = NULL;
        }
        return;
    }
    
    if ( victim->desc->snoop_by != NULL )
    {
        send_to_char( "Busy already.\n\r", ch );
        return;
    }
    
    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room 
        &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r",ch);
        return;
    }
    
    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }
    
    if ( ch->desc != NULL )
    {
        for ( d = ch->desc->snoop_by; d != NULL; d = d->snoop_by )
        {
            if ( d->character == victim || d->original == victim )
            {
                send_to_char( "No snoop loops.\n\r", ch );
                return;
            }
        }
    }
    
    victim->desc->snoop_by = ch->desc;
    /*
    if (get_trust(ch) < MAX_LEVEL)
    {
    sprintf(buf,"%s is watching you.\n\r", ch->name);
    send_to_char(buf, victim);
    }
    */
    sprintf(buf,"$N starts snooping on %s",
        (IS_NPC(ch) ? victim->short_descr : victim->name));
    wiznet(buf,ch,NULL,WIZ_SNOOPS,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}



DEF_DO_FUN(do_switch)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Switch into whom?\n\r", ch );
        return;
    }
    
    if ( ch->desc == NULL )
        return;
    
    if ( ch->desc->original != NULL )
    {
        send_to_char( "You are already switched.\n\r", ch );
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( victim == ch )
    {
        send_to_char( "Ok.\n\r", ch );
        return;
    }
    
    if (!IS_NPC(victim))
    {
        send_to_char("You can only switch into mobiles.\n\r",ch);
        return;
    }
    
    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room 
        &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r",ch);
        return;
    }
    
    if ( victim->desc != NULL )
    {
        send_to_char( "Character in use.\n\r", ch );
        return;
    }
    
    if (IS_SET(victim->penalty, PENALTY_FREEZE))
    {
        send_to_char("Character is frozen. Switch aborted to avoid trapping you.\n\r",ch);
        return;
    }
    
    sprintf(buf,"$N switches into %s",victim->short_descr);
    wiznet(buf,ch,NULL,WIZ_SWITCHES,WIZ_SECURE,get_trust(ch));
    
	if (!IS_NPC(ch))
		REMOVE_BIT(ch->pcdata->tag_flags, TAG_PLAYING);
    ch->desc->character = victim;
    ch->desc->original  = ch;
    victim->desc        = ch->desc;
    ch->desc            = NULL;
    /* change communications to match */
    if (ch->prompt != NULL)
        victim->prompt = str_dup(ch->prompt);
    flag_copy( victim->comm, ch->comm );
    victim->lines = ch->lines;
    send_to_char( "Ok.\n\r", victim );
    return;
}



DEF_DO_FUN(do_return)
{
    char buf[MAX_STRING_LENGTH];
    
    if ( ch->desc == NULL )
        return;
    
    if ( ch->desc->original == NULL )
    {
        send_to_char( "You aren't switched.\n\r", ch );
        return;
    }
    
    send_to_char( "You return to your original body.\n\r", ch );
    if (ch->prompt != NULL)
    {
        free_string(ch->prompt);
        ch->prompt = NULL;
    }
    
    if ( ch->desc->original->pcdata->new_tells)
        send_to_char( "Type 'playback tell' to see missed tells.\n\r", ch );
    
    
    sprintf(buf,"$N returns from %s.",ch->short_descr);
    wiznet(buf,ch->desc->original,0,WIZ_SWITCHES,WIZ_SECURE,get_trust(ch));
    ch->desc->character       = ch->desc->original;
    ch->desc->original        = NULL;
    ch->desc->character->desc = ch->desc; 
    ch->desc                  = NULL;
    
    return;
}

/* trust levels for load and clone */
bool obj_check (CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (IS_TRUSTED(ch,L4)
        || (IS_TRUSTED(ch,L5)   && obj->level <= 20 && obj->cost <= 1000)
        || (IS_TRUSTED(ch,L6)   && obj->level <= 10 && obj->cost <= 500)
        || (IS_TRUSTED(ch,L7)   && obj->level <=  5 && obj->cost <= 250)
        || (IS_TRUSTED(ch,L8)   && obj->level ==  0 && obj->cost <= 100))
        return TRUE;
    else
        return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone)
{
    OBJ_DATA *c_obj, *t_obj;
    
    
    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
    {
        if (obj_check(ch,c_obj))
        {
            t_obj = create_object(c_obj->pIndexData,0);
            clone_object(c_obj,t_obj);
            obj_to_obj(t_obj,clone);
            recursive_clone(ch,c_obj,t_obj);
        }
    }
}

/* command that is similar to load */
DEF_DO_FUN(do_clone)
{
    char arg[MAX_INPUT_LENGTH];
    const char *rest;
    CHAR_DATA *mob;
    OBJ_DATA  *obj;
    
    rest = one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("Clone what?\n\r",ch);
        return;
    }
    
    if (!str_prefix(arg,"object"))
    {
        mob = NULL;
        obj = get_obj_here(ch,rest);
        if (obj == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            return;
        }
    }
    else if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
        obj = NULL;
        mob = get_char_room(ch,rest);
        if (mob == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            return;
        }
    }
    else /* find both */
    {
        mob = get_char_room(ch,argument);
        obj = get_obj_here(ch,argument);
        if (mob == NULL && obj == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            return;
        }
    }
    
    /* clone an object */
    if (obj != NULL)
    {
        OBJ_DATA *clone;
        
        if (!obj_check(ch,obj))
        {
            send_to_char(
                "Your powers are not great enough for such a task.\n\r",ch);
            return;
        }
        
        clone = create_object(obj->pIndexData,0); 
        clone_object(obj,clone);
        if (obj->carried_by != NULL)
            obj_to_char(clone,ch);
        else
            obj_to_room(clone,ch->in_room);
        recursive_clone(ch,obj,clone);
        
        act("$n has created $p.",ch,clone,NULL,TO_ROOM);
        act("You clone $p.",ch,clone,NULL,TO_CHAR);
        wiznet("$N clones $p.",ch,clone,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
        return;
    }
    else if (mob != NULL)
    {
        CHAR_DATA *clone;
        OBJ_DATA *new_obj;
        char buf[MAX_STRING_LENGTH];
        
        if (!IS_NPC(mob))
        {
            send_to_char("You can only clone mobiles.\n\r",ch);
            return;
        }
        
        if ((mob->level > 20 && !IS_TRUSTED(ch, L4))
            ||  (mob->level > 10 && !IS_TRUSTED(ch, L5))
            ||  (mob->level >  5 && !IS_TRUSTED(ch, L6))
            ||  (mob->level >  0 && !IS_TRUSTED(ch, L7))
            ||  !IS_TRUSTED(ch, L8))
        {
            send_to_char(
                "Your powers are not great enough for such a task.\n\r",ch);
            return;
        }
        
        clone = create_mobile(mob->pIndexData);
        clone_mobile(mob,clone); 
        
        for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
        {
            if (obj_check(ch,obj))
            {
                new_obj = create_object(obj->pIndexData,0);
                clone_object(obj,new_obj);
                recursive_clone(ch,obj,new_obj);
                obj_to_char(new_obj,clone);
                new_obj->wear_loc = obj->wear_loc;
            }
        }
        char_to_room(clone,ch->in_room);
        act("$n has created $N.",ch,NULL,clone,TO_ROOM);
        act("You clone $N.",ch,NULL,clone,TO_CHAR);
        sprintf(buf,"$N clones %s.",clone->short_descr);
        wiznet(buf,ch,NULL,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
        return;
    }
}

/* RT to replace the two load commands */

DEF_DO_FUN(do_load)
{
    char arg[MAX_INPUT_LENGTH];
    
    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  load mob [vnum] (ammount)\n\r",ch);
        send_to_char("  load obj [vnum] (ammount)\n\r",ch);
        return;
    }
    
    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    {
        do_mload(ch,argument);
        return;
    }
    
    if (!str_cmp(arg,"obj"))
    {
        do_oload(ch,argument);
        return;
    }
    /* echo syntax */
    do_load(ch,"");
}


DEF_DO_FUN(do_mload)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim = NULL;
    char buf[MAX_STRING_LENGTH];
    int i, ammount = 1;
    
    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    
    if ( arg[0] == '\0' || !is_number(arg) )
    {
        send_to_char( "Syntax: load mob [vnum] (ammount)\n\r", ch );
        return;
    }
    
    if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL )
    {
        send_to_char( "No mob has that vnum.\n\r", ch );
        return;
    }

    if ( arg2[0] != '\0')
    {
        if (!is_number(arg2))
        {
            send_to_char( "Syntax: load mob [vnum] (ammount)\n\r", ch );
            return;
        }
        ammount = atoi(arg2);
        if (ammount < 1 || ammount > 100)
        {
            send_to_char( "Ammount must be be between 1 and 100.\n\r",ch);
            return;
        }
    }
    
    for ( i = 0; i < ammount; i++ )
    {
	victim = create_mobile( pMobIndex );
	arm_npc( victim );
	char_to_room( victim, ch->in_room );
    }

    act( "You have created $N!", ch, NULL, victim, TO_CHAR );
    act( "$n has created $N!", ch, NULL, victim, TO_ROOM );
    sprintf(buf,"$N loads %s.",victim->short_descr);
    wiznet(buf,ch,NULL,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
    return;
}



DEF_DO_FUN(do_oload)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj = NULL;
    int i, ammount = 1;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' || !is_number(arg1))
    {
        send_to_char( "Syntax: load obj [vnum] (ammount)\n\r", ch );
        return;
    }
    
    if ( ( pObjIndex = get_obj_index(atoi(arg1)) ) == NULL )
    {
        send_to_char( "No object has that vnum.\n\r", ch );
        return;
    }

    if ( arg2[0] != '\0')
    {
        if (!is_number(arg2))
        {
            send_to_char( "Syntax: load obj [vnum] (ammount)\n\r", ch );
            return;
        }
        ammount = atoi(arg2);
        if (ammount < 1 || ammount > 100)
        {
            send_to_char( "Ammount must be be between 1 and 100.\n\r",ch);
            return;
        }
    }
    
    for ( i = 0; i < ammount; i++ )
    {
	obj = create_object( pObjIndex, 0 );
	check_enchant_obj( obj );
	if ( CAN_WEAR(obj, ITEM_TAKE) )
	    obj_to_char( obj, ch );
	else
	    obj_to_room( obj, ch->in_room );
    }
    act( "You have created $p!", ch, obj, NULL, TO_CHAR );
    act( "$n has created $p!", ch, obj, NULL, TO_ROOM );
    wiznet("$N loads $p.",ch,obj,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
    return;
}



DEF_DO_FUN(do_purge)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    DESCRIPTOR_DATA *d;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' || !strcmp(arg, "room") )
    {
        /* 'purge' */
        CHAR_DATA *vnext;
        OBJ_DATA  *obj_next;
        
        for ( victim = ch->in_room->people; victim != NULL; victim = vnext )
        {
            vnext = victim->next_in_room;
            if ( IS_NPC(victim) && !IS_SET(victim->act,ACT_NOPURGE) 
                && victim != ch /* safety precaution */ )
                extract_char( victim, TRUE );
        }
        
        for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            if (!IS_OBJ_STAT(obj,ITEM_NOPURGE))
                extract_obj( obj );
        }
        
        act( "$n purges the room!", ch, NULL, NULL, TO_ROOM);
        send_to_char( "Ok.\n\r", ch );
        return;
    }
    
    if ( !strcmp(arg, "area") )
    {
	purge_area( ch->in_room->area );
        act( "$n purges the area!", ch, NULL, NULL, TO_ROOM);
	send_to_char( "You purge the area.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( !IS_NPC(victim) )
    {
        
        if (ch == victim)
        {
            send_to_char("Ho ho ho.\n\r",ch);
            return;
        }
        
        if (get_trust(ch) <= get_trust(victim))
        {
            send_to_char("Maybe that wasn't a good idea...\n\r",ch);
            sprintf(buf,"%s tried to purge you!\n\r",ch->name);
            send_to_char(buf,victim);
            return;
        }
        
        act("$n disintegrates $N.",ch,0,victim,TO_NOTVICT);
        
        if (victim->level > 1)
            quit_save_char_obj( victim );
        d = victim->desc;
        extract_char( victim, TRUE );
        if ( d != NULL )
            close_socket( d );
        
        return;
    }
    
    act( "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    extract_char( victim, TRUE );
    return;
}



DEF_DO_FUN(do_advance)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;
    int iLevel;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
        send_to_char( "Syntax: advance <char> <level>.\n\r", ch );
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "That player is not here.\n\r", ch);
        return;
    }
    
    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        return;
    }
    
    if ( ( level = atoi( arg2 ) ) < 1 || level >= IMPLEMENTOR )
    {
        send_to_char( "Level must be 1 to 109.\n\r", ch );
        return;
    }
    
    if ( level > get_trust( ch ) )
    {
        send_to_char( "Limited to your trust level.\n\r", ch );
        return;
    }
    
    /*
    * Lower level:
    *   Reset to level 1.
    *   Then raise again.
    *   Currently, an imp can lower another imp.
    *   -- Swiftest
    */
    if ( level <= victim->level )
    {
        send_to_char( "Lowering a player's level!\n\r", ch );
        printf_to_char(victim, "%s touches your forehead, and you feel some of your life force slip away!\n\r", capitalize(ch->name));
        
        if (victim->level >= LEVEL_IMMORTAL)
        {
            int flag, i;
            
            update_wizlist(victim, level);
            
            for (i = IMPLEMENTOR; i > level && i >= LEVEL_IMMORTAL; i--)
            {
                sprintf(buf, "%s %d", victim->name, i);
                do_revoke(ch, buf);
            }

	    /* reset wiznet */
	    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
	    {
		if (wiznet_table[flag].level > level)
		    REMOVE_BIT(victim->wiznet, wiznet_table[flag].flag);
	    }
	    /* reset holylight, wizi and incog */
	    if ( level < LEVEL_IMMORTAL )
	    {
		REMOVE_BIT( victim->act, PLR_HOLYLIGHT );
		victim->invis_level = 0;
		victim->incog_level = 0;
	    }
	    else
	    {
		victim->invis_level = UMIN( victim->invis_level, level );
		victim->incog_level = UMIN( victim->incog_level, level );
	    }
        }    
        
	tattoo_modify_level( victim, victim->level, 0 );
	victim->level = 1;
	advance_level( victim, TRUE );
    }
    else
    {
        send_to_char( "Raising a player's level!\n\r", ch );
        printf_to_char(victim, "%s touches your forehead, and you are infused with greater life force!\n\r", capitalize(ch->name));
        
        if ((victim->level >= LEVEL_IMMORTAL) || (level >= LEVEL_IMMORTAL ))
        {
            int i;
            
            update_wizlist(victim, level);
            
            for (i = UMAX(victim->level + 1, LEVEL_IMMORTAL); i <= level; i++)
            {
                sprintf(buf, "%s %d", victim->name, i);
                do_grant(ch, buf);
            }
        }    
        
    }
    
    for ( iLevel = victim->level ; iLevel < level; iLevel++ )
    {
        victim->level += 1;
        advance_level( victim,TRUE);
    }
    sprintf(buf,"You are now level %d.\n\r",victim->level);
    send_to_char(buf,victim);
    victim->exp = exp_per_level(victim) * UMAX(1, victim->level);
    victim->trust = 0;
    return;
}



DEF_DO_FUN(do_trust)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
        send_to_char( "Syntax: trust <char> <level>.\n\r", ch );
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "That player is not here.\n\r", ch);
        return;
    }
    
    if ( ( level = atoi( arg2 ) ) < 0 || level > 109 )
    {
        send_to_char( "Level must be 0 (reset) or 1 to 109.\n\r", ch );
        return;
    }
    
    if ( level > get_trust( ch ) )
    {
        send_to_char( "Limited to your trust.\n\r", ch );
        return;
    }
    
    victim->trust = level;
    return;
}

void restore_char( CHAR_DATA *victim )
{
    affect_strip_offensive( victim );
            
    victim->hit     = victim->max_hit;
    victim->mana    = victim->max_mana;
    victim->move    = victim->max_move;
    update_pos( victim );
}

DEF_DO_FUN(do_restore)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    DESCRIPTOR_DATA *d;
    
    one_argument( argument, arg );
    if (arg[0] == '\0' || !str_cmp(arg,"room"))
    {
        /* cure room */
        
        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
	    affect_strip_offensive( vch );
            
            vch->hit    = vch->max_hit;
            vch->mana   = vch->max_mana;
            vch->move   = vch->max_move;
            update_pos( vch);
            act("$n has restored you.",ch,NULL,vch,TO_VICT);
        }
        
        sprintf(buf,"$N restored room %d.",ch->in_room->vnum);
        wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));
        
        send_to_char("Room restored.\n\r",ch);
        return;
        
    }
    
    if ( get_trust(ch) >=  MAX_LEVEL - 1 && !str_cmp(arg,"all"))
    {
        /* cure all */
        
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            victim = d->character;
            
            if (victim == NULL || IS_NPC(victim))
                continue;
            
	    affect_strip_offensive( victim );
            
            victim->hit     = victim->max_hit;
            victim->mana    = victim->max_mana;
            victim->move    = victim->max_move;
            update_pos( victim);
            if (victim->in_room != NULL)
                act("$n has restored you.",ch,NULL,victim,TO_VICT);
        }
        send_to_char("All active players restored.\n\r",ch);
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    restore_char( victim );
    act( "$n has restored you.", ch, NULL, victim, TO_VICT );
    sprintf(buf,"$N restored %s",
        IS_NPC(victim) ? victim->short_descr : victim->name);
    wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}



DEF_DO_FUN(do_log)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Log whom?\n\r", ch );
        return;
    }
    
    if ( !str_cmp( arg, "all" ) )
    {
        if ( fLogAll )
        {
            fLogAll = FALSE;
            send_to_char( "Log ALL off.\n\r", ch );
        }
        else
        {
            fLogAll = TRUE;
            send_to_char( "Log ALL on.\n\r", ch );
        }
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        return;
    }
    
    /*
    * No level check, gods can log anyone.
    */
    if ( IS_SET(victim->act, PLR_LOG) )
    {
        REMOVE_BIT(victim->act, PLR_LOG);
        send_to_char( "LOG removed.\n\r", ch );
    }
    else
    {
        SET_BIT(victim->act, PLR_LOG);
        send_to_char( "LOG set.\n\r", ch );
    }
    
    return;
}




DEF_DO_FUN(do_peace)
{
    CHAR_DATA *rch, *victim;
    char arg[MIL];

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
	victim = NULL;
    else
    {
	victim = get_char_room( ch, arg );
	if ( victim == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}
    }

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
	if ( victim != NULL && rch != victim )
	    continue;
        if ( rch->fighting != NULL )
            stop_fighting( rch, TRUE );
        if (IS_NPC(rch) && IS_SET(rch->act,ACT_AGGRESSIVE))
            REMOVE_BIT(rch->act,ACT_AGGRESSIVE);
	if (IS_NPC(rch))
	{
	    stop_hunting(rch);
	    forget_attacks(rch);
	}
    }
    
    send_to_char( "Ok.\n\r", ch );
    return;
}

DEF_DO_FUN(do_wizlock)
{
    extern bool wizlock;
    wizlock = !wizlock;
    
    if ( wizlock )
    {
        wiznet("$N has wizlocked the game.",ch,NULL,0,0,0);
        send_to_char( "Game wizlocked.\n\r", ch );
    }
    else
    {
        wiznet("$N removes wizlock.",ch,NULL,0,0,0);
        send_to_char( "Game un-wizlocked.\n\r", ch );
    }
    
    return;
}

/* RT anti-newbie code */

DEF_DO_FUN(do_newlock)
{
    extern bool newlock;
    newlock = !newlock;
    
    if ( newlock )
    {
        wiznet("$N locks out new characters.",ch,NULL,0,0,0);
        send_to_char( "New characters have been locked out.\n\r", ch );
    }
    else
    {
        wiznet("$N allows new characters back in.",ch,NULL,0,0,0);
        send_to_char( "Newlock removed.\n\r", ch );
    }
    
    return;
}

DEF_DO_FUN(do_pflag)
{
	int i;
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    sh_int duration;
    CHAR_DATA *victim;
    
    argument = smash_tilde_cc( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' || ((!is_number(arg2) || arg3[0] == '\0')
        && strcmp(arg2, "clear")) )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  pflag <name> <duration (in ticks)> <string>\n\r",ch);
        send_to_char("  pflag <name> clear\n\r",ch);
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        return;
    }
    
    if (!strcmp(arg2, "clear"))
    {
        send_to_char("Ok.\n\r",ch);
        free_string(victim->pcdata->customflag);
        victim->pcdata->customflag=str_dup("");
        victim->pcdata->customduration=0;
        return;
    }
    
    if ( (duration=atoi(arg2))<1)
    {
        send_to_char( "Duration must be positive.\n\r", ch );
        return;
    }
    
    if (duration>5760)
    {
        send_to_char("Exercise some restraint.  120 ticks = 1 rl hour.\n\r", ch);
        return;
    };
    
	for (i=0; arg3[i]; i++);
	if ((i>0) && (arg3[i-1]=='{'))
	{
		arg3[i]=' ';
		arg3[i+1]='\0';
	}

    free_string( victim->pcdata->customflag );
    victim->pcdata->customflag = str_dup(arg3);
    victim->pcdata->customduration = duration;
    
    send_to_char("Ok.\n\r", ch);
    
    return;	
}

DEF_DO_FUN(do_string)
{
    char type [MAX_INPUT_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    
    argument = smash_tilde_cc( argument );
    argument = one_argument( argument, type );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );
    
    if ( type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  string char <name> <field> <string>\n\r",ch);
        send_to_char("    fields: name short long desc title spec\n\r",ch);
        send_to_char("  string obj  <name> <field> <string>\n\r",ch);
        send_to_char("    fields: name short long extended\n\r",ch);
        return;
    }
    
    if (!str_prefix(type,"character") || !str_prefix(type,"mobile"))
    {
        if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
        {
            send_to_char( "They aren't here.\n\r", ch );
            return;
        }
        
        /* clear zone for mobs */
        victim->zone = NULL;
        
        /* string something */
        
        if ( !str_prefix( arg2, "name" ) )
        {
            if ( !IS_NPC(victim) )
            {
                send_to_char( "Not on PC's.\n\r", ch );
                return;
            }
            free_string( victim->name );
            victim->name = str_dup( arg3 );
            return;
        }
        
        if ( !str_prefix( arg2, "description" ) )
        {
            free_string(victim->description);
            victim->description = str_dup(arg3);
            return;
        }
        
        if ( !str_prefix( arg2, "short" ) )
        {
            free_string( victim->short_descr );
            victim->short_descr = str_dup( arg3 );
            return;
        }
        
        if ( !str_prefix( arg2, "long" ) )
        {
            free_string( victim->long_descr );
            victim->long_descr = str_dup( arg3 );
            return;
        }
        
        if ( !str_prefix( arg2, "title" ) )
        {
            if ( IS_NPC(victim) )
            {
                send_to_char( "Not on NPC's.\n\r", ch );
                return;
            }
            
            set_title( victim, arg3 );
            return;
        }
        
        if ( !str_prefix( arg2, "spec" ) )
        {
            if ( !IS_NPC(victim) )
            {
                send_to_char( "Not on PC's.\n\r", ch );
                return;
            }
            
            if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 )
            {
                send_to_char( "No such spec fun.\n\r", ch );
                return;
            }
            
            return;
        }
    }
    
    if (!str_prefix(type,"object"))
    {
        /* string an obj */
        
        if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
        {
            send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
            return;
        }
        
        if ( !str_prefix( arg2, "name" ) )
        {
            free_string( obj->name );
            obj->name = str_dup( arg3 );
            return;
        }
        
        if ( !str_prefix( arg2, "short" ) )
        {
            free_string( obj->short_descr );
            obj->short_descr = str_dup( arg3 );
            return;
        }
        
        if ( !str_prefix( arg2, "long" ) )
        {
            free_string( obj->description );
            obj->description = str_dup( arg3 );
            return;
        }
        
        if ( !str_prefix( arg2, "ed" ) || !str_prefix( arg2, "extended"))
        {
            EXTRA_DESCR_DATA *ed;
            
            argument = one_argument( argument, arg3 );
            if ( argument == NULL )
            {
                send_to_char( "Syntax: oset <object> ed <keyword> <string>\n\r",
                    ch );
                return;
            }
            
            char desc_buf[MSL];
            sprintf(desc_buf, "%s\n\r", argument);
            
            ed = new_extra_descr();
            
            ed->keyword     = str_dup( arg3     );
            ed->description = str_dup( desc_buf );
            ed->next        = obj->extra_descr;
            obj->extra_descr    = ed;
            return;
        }
    }
    
    
    /* echo bad use message */
    do_string(ch,"");
}

      
/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
DEF_DO_FUN(do_force)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
          
    argument = one_argument( argument, arg );
    
    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Force whom to do what?\n\r", ch );
	return;
    }
          
    one_argument(argument,arg2);
    
    if (!str_cmp(arg2,"delete") 
	|| !str_prefix(arg2,"mob") 
	|| !str_prefix(arg2,"force") 
	|| !str_cmp(arg2,"pkill") 
	|| !str_cmp(arg2, "gran") 
	|| !str_cmp(arg2, "rev"))
    {
	send_to_char("That will NOT be done.\n\r",ch);
	return;
    }
          
    sprintf( buf, "$n forces you to '%s'.", argument );
          
    if ( !str_cmp( arg, "all" ) )
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	
	if (get_trust(ch) < MAX_LEVEL - 3)
	{
	    send_to_char("Not at your level!\n\r",ch);
	    return;
	}
	
	for ( vch = char_list; vch != NULL; vch = vch_next )
	{
	    vch_next = vch->next;
	    
	    if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
		 && (vch->desc==NULL || vch->desc->connected==CON_PLAYING))
	    {
		act( buf, ch, NULL, vch, TO_VICT );
		interpret( vch, argument );
	    }
	}
    }
    else if (!str_cmp(arg,"players"))
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	
	if (get_trust(ch) < MAX_LEVEL - 2)
	{
	    send_to_char("Not at your level!\n\r",ch);
	    return;
	}
              
	for ( vch = char_list; vch != NULL; vch = vch_next )
	{
	    vch_next = vch->next;
	    
	    if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch ) 
		 &&   !IS_HERO(vch))
	    {
		act( buf, ch, NULL, vch, TO_VICT );
		interpret( vch, argument );
	    }
	}
    }
    else if (!str_cmp(arg,"gods"))
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	
	if (get_trust(ch) < MAX_LEVEL - 2)
	{
	    send_to_char("Not at your level!\n\r",ch);
	    return;
	}
              
	for ( vch = char_list; vch != NULL; vch = vch_next )
	{
	    vch_next = vch->next;
	    
	    if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
		 &&   IS_HERO(vch))
	    {
		act( buf, ch, NULL, vch, TO_VICT );
		interpret( vch, argument );
	    }
	}
    }
    else
    {
	CHAR_DATA *victim;
	
	if ( ( victim = get_char_world( ch, arg ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}
	
	if ( victim == ch )
	{
	    send_to_char( "Aye aye, right away!\n\r", ch );
	    return;
	}
              
	if (!is_room_owner(ch,victim->in_room) 
	    &&  ch->in_room != victim->in_room 
	    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
	{
	    send_to_char("That character is in a private room.\n\r",ch);
	    return;
	}
	
	if ( get_trust( victim ) >= get_trust( ch ) )
	{
	    send_to_char( "Do it yourself!\n\r", ch );
	    return;
	}
	
	if ( !IS_NPC(victim) && get_trust(ch) < MAX_LEVEL -3)
	{
	    send_to_char("Not at your level!\n\r",ch);
	    return;
	}
              
	act( buf, ch, NULL, victim, TO_VICT );
	interpret( victim, argument );
    }
    
    send_to_char( "Ok.\n\r", ch );
    return;
}



/*
* New routines by Dionysos.
*/
DEF_DO_FUN(do_invis)
{
    int level;
    char arg[MAX_STRING_LENGTH];
    
    /* RT code for taking a level argument */
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' ) 
        /* take the default path */
        
        if ( ch->invis_level)
        {
            ch->invis_level = 0;
            act( "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You slowly fade back into existence.\n\r", ch );
        }
        else
        {
            ch->invis_level = get_trust(ch);
            act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You slowly vanish into thin air.\n\r", ch );
        }
        else
            /* do the level thing */
        {
            level = atoi(arg);
            if (level < 2 || level > get_trust(ch))
            {
                send_to_char("Invis level must be between 2 and your level.\n\r",ch);
                return;
            }
            else
            {
                ch->reply = NULL;
                ch->invis_level = level;
                act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "You slowly vanish into thin air.\n\r", ch );
            }
        }
        
        return;
}


DEF_DO_FUN(do_incognito)
{
    int level;
    char arg[MAX_STRING_LENGTH];
    
    /* RT code for taking a level argument */
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
        /* take the default path */
        
        if ( ch->incog_level)
        {
            ch->incog_level = 0;
            act( "$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You are no longer cloaked.\n\r", ch );
        }
        else
        {
            ch->incog_level = get_trust(ch);
            act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You cloak your presence.\n\r", ch );
        }
        else
            /* do the level thing */
        {
            level = atoi(arg);
            if (level < 2 || level > get_trust(ch))
            {
                send_to_char("Incog level must be between 2 and your level.\n\r",ch);
                return;
            }
            else
            {
                ch->reply = NULL;
                ch->incog_level = level;
                act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "You cloak your presence.\n\r", ch );
            }
        }
        
        return;
}



DEF_DO_FUN(do_holylight)
{
    if ( IS_NPC(ch) )
        return;
    
    if ( IS_SET(ch->act, PLR_HOLYLIGHT) )
    {
        REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
        send_to_char( "Holy light mode off.\n\r", ch );
    }
    else
    {
        SET_BIT(ch->act, PLR_HOLYLIGHT);
        send_to_char( "Holy light mode on.\n\r", ch );
    }
    
    return;
}

/* prefix command: it will put the string typed on each line typed */

DEF_DO_FUN(do_prefi)
{
    send_to_char("You cannot abbreviate the prefix command.\r\n",ch);
    return;
}

DEF_DO_FUN(do_prefix)
{
    char buf[MAX_INPUT_LENGTH];
    
    if (argument[0] == '\0')
    {
        if (ch->prefix[0] == '\0')
        {
            send_to_char("You have no prefix to clear.\r\n",ch);
            return;
        }
        
        send_to_char("Prefix removed.\r\n",ch);
        free_string(ch->prefix);
        ch->prefix = str_dup("");
        return;
    }
    
    if (ch->prefix[0] != '\0')
    {
        sprintf(buf,"Prefix changed to %s.\r\n",argument);
        free_string(ch->prefix);
    }
    else
    {
        sprintf(buf,"Prefix set to %s.\r\n",argument);
    }
    
    ch->prefix = str_dup(argument);
}


DEF_DO_FUN(do_sla)
{
    send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
    return;
}



DEF_DO_FUN(do_slay)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    
    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Slay whom?\n\r", ch );
        return;
    }
    
    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( ch == victim )
    {
        send_to_char( "Suicide is a mortal sin.\n\r", ch );
        return;
    }
    
    if ( !IS_NPC(victim) && victim->level >= get_trust(ch) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }
    
    act( "You slay $M in cold blood!",  ch, NULL, victim, TO_CHAR    );
    act( "$n slays you in cold blood!", ch, NULL, victim, TO_VICT    );
    act( "$n slays $N in cold blood!",  ch, NULL, victim, TO_NOTVICT );
    raw_kill( victim, NULL, TRUE);

    return;
}



/* Omni wiz command by Prism <snazzy@ssnlink.net>. 
     Updated 12-8-13 by Astark to include functionality from sockets */

DEF_DO_FUN(do_omni)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int players = 0;
    const char *state;
    char            login[100];
    char            idle[10];
    buf[0]    = '\0';
    output    = new_buf();
    
    sprintf( buf, "----------------------------------------------------------------------------------------------\n\r");
    add_buf(output,buf);
    sprintf( buf, "Num  Name         Login   Idle  State    Pos    [Room ]  Qst? Host            Client\n\r");
    add_buf(output,buf);    
    sprintf( buf, "----------------------------------------------------------------------------------------------\n\r");
    add_buf(output,buf);
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA *wch = ( d->original != NULL ) ? d->original : d->character;
        
        if ( wch == NULL || !can_see(ch, wch) )
            continue;

        players++;
                   
        /* NB: You may need to edit the CON_ values */
        /* I updated to all current rom CON_ values -Silverhand */
        switch( d->connected % MAX_CON_STATE)
        {
        case CON_PLAYING:              state = "PLAYING ";    break;
        case CON_GET_NAME:             state = "Get Name";    break;
        case CON_GET_OLD_PASSWORD:     state = "Passwd  ";    break;
        case CON_CONFIRM_NEW_NAME:     state = "New Nam ";    break;
        case CON_GET_NEW_PASSWORD:     state = "New Pwd ";    break;
        case CON_CONFIRM_NEW_PASSWORD: state = "Con Pwd ";    break;
        case CON_GET_NEW_RACE:         state = "New Rac ";    break;
        case CON_GET_NEW_SEX:          state = "New Sex ";    break;
        case CON_GET_NEW_CLASS:        state = "New Cls ";    break;
        case CON_GET_ALIGNMENT:        state = "New Aln ";    break;
        case CON_DEFAULT_CHOICE:       state = "Default ";    break;
        case CON_GET_CREATION_MODE:    state = "Cre Mod ";    break;
        case CON_ROLL_STATS:           state = "Roll St ";    break;
        case CON_GET_STAT_PRIORITY:    state = "Sta Pri ";    break;
        case CON_NOTE_TO:              state = "Note To ";    break;
        case CON_NOTE_SUBJECT:         state = "Note Sub";    break;
        case CON_NOTE_EXPIRE:          state = "Note Exp";    break;
        case CON_NOTE_TEXT:            state = "Note Txt";    break;
        case CON_NOTE_FINISH:          state = "Note Fin";    break;
        case CON_GEN_GROUPS:           state = " Custom ";    break;
        case CON_PICK_WEAPON:          state = " Weapon ";    break;
        case CON_READ_IMOTD:           state = " IMOTD  ";    break;
        case CON_BREAK_CONNECT:        state = "LINKDEAD";    break;
        case CON_READ_MOTD:            state = "  MOTD  ";    break;
        case CON_GET_COLOUR:           state = " Colour?";    break;
        default:                       state = "UNKNOWN!";    break;
        }
        
        /* Format "login" value... */
        strftime( login, 100, "%I:%M%p", localtime( &wch->logon ) );
        
        if ( wch->timer > 0 )
            sprintf( idle, "%-4d", wch->timer );
        else
            sprintf( idle, "    " );
        
        /* Added an extra  %s for the questing check below - Astark Oct 2012 */
        sprintf( buf, "%-3d  	<send 'pgrep Owner %s'>%-12s	</send> %7s %5s %7.7s  %-5.5s  [%5d]   %s   	<send 'pgrep %s'>%-15s	</send> %s\n\r",
            d->descriptor,                          /* ID */
            wch->name,                              /* Send name through pgrep */
            wch->name,                              /* Name */
            login,                                  /* Login Time */
            idle,                                   /* How long idle */
            state,                                  /* State (Playing, creation, etc. */
            capitalize( position_table[wch->position].name),  /* Position */
            wch->in_room ? wch->in_room->vnum : 0,  /* Room player is in */
            IS_QUESTOR(wch) 
                || IS_QUESTORHARD(wch) ? "Y" : "N", /* Is player on a quest? */
            d->host,                                /* Send IP through pgrep */
            d->host,
            d->pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
        add_buf(output,buf);
    }
    
    /*
    * Tally the counts and send the whole list out.
    */
    sprintf( buf2, "\n\rPlayers found: %d\n\r", players );
    add_buf(output,buf2);
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}


DEF_DO_FUN(do_as)
{
    CHAR_DATA *victim ;
    DESCRIPTOR_DATA *original ;
    char arg[MAX_STRING_LENGTH] ;
    char arg2[MAX_STRING_LENGTH];
    
    argument = one_argument(argument,arg) ;
    one_argument(argument, arg2);
    
    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char("Syntax : as <victim> <action>\n\r",ch) ;
        return ;
    }
    
    if ((victim=get_char_world(ch, arg))==NULL)
    {
        send_to_char("No such person around here.\n\r", ch) ;
        return ;
    }
    
    if (get_trust(victim) >= get_trust(ch))
    {
        send_to_char("Don't get ideas above your station.\n\r", ch) ;
        return ;
    }
    
    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r",ch);
        return;
    }
    
    if (!str_cmp(arg2,"delete") 
        || !str_cmp(arg2,"quit") /* Bobble: causes bug */
        || !str_cmp(arg2,"switch") /* Bobble: causes bug */
        || !str_cmp(arg2,"return") /* Bobble: causes bug */
        || !str_cmp(arg2,"name") /* Bobble: causes bug */
        || !str_cmp(arg2,"mob") 
        || !str_cmp(arg2,"force") 
        || !str_cmp(arg2,"pkill") 
        || !str_cmp(arg2,"gran") 
        || !str_cmp(arg2,"rev"))
    {
        send_to_char("That will NOT be done.\n\r",ch);
        return;
    }
    
    original = victim->desc ;
    victim->desc = ch->desc ;
    ch->desc = NULL ;
    
    interpret(victim, argument) ;
    
    ch->desc = victim->desc ;
    victim->desc = original ;
    
    return ;
}

/** Function: do_pload
* Descr   : Loads an unconnected player onto the mud, moving them to
*         : the immortal's room so that they can be viewed/manipulated
* Returns : (void)
* Syntax  : pload (who)
* Written : v1.0 12/97 Last updated on: 5/98
* Author  : Gary McNickle <gary@dharvest.com>
* Update  : Characters returned to Original room by: Anthony Michael Tregre
*/

DEF_DO_FUN(do_pload)
{
    DESCRIPTOR_DATA d={0};
    bool isChar = FALSE;
    char name[MAX_INPUT_LENGTH];
    
    if (argument[0] == '\0') 
    {
        send_to_char("Load who?\n\r", ch);
        return;
    }
    
    argument = one_argument(argument, name);
    name[0] = UPPER(name[0]);
    
    /* Don't want to load a second copy of a player who's already online! */
    if ( char_list_find(name) != NULL )
    {
        send_to_char( "That person is already connected!\n\r", ch );
        return;
    }
    
    isChar = load_char_obj(&d, name, FALSE); /* char pfile exists? */
    
    if (!isChar) 
    {
        send_to_char("Load Who? Are you sure? I can't seem to find them.\n\r", ch);
         /* load_char_obj still loads "default" character
           even if player not found, so need to free it */
        if (d.character)
        {
            free_char(d.character);
            d.character=NULL;
        }
        return;
    }
    
    d.character->desc 	= NULL;
    char_list_insert(d.character);
    d.connected   	= CON_PLAYING;
    reset_char(d.character);
    
    /* bring player to imm */
    printf_to_char(ch,"You pull %s from the void! (was in room: %d)\n\r", 
        d.character->name,
	d.character->in_room == NULL ? 0 : d.character->in_room->vnum);
    
    act( "$n pulls $N from the void!", 
        ch, NULL, d.character, TO_ROOM );

    /* store old room, then move to imm */
    d.character->was_in_room = d.character->in_room;
    char_to_room(d.character, ch->in_room);

    if (d.character->pet != NULL)
    {
        char_to_room(d.character->pet,d.character->in_room);
        act("$n appears in the room.",d.character->pet,NULL,NULL,TO_ROOM);
    }
    
} /* end do_pload */



  /** Function: do_punload
  * Descr   : Unloads a previously "ploaded" player, returning them
  *         : back to the player directory.
  * Returns : (void)
  * Syntax  : punload (who)
  * Written : v1.0 12/97 Last updated on 5/98
  * Author  : Gary McNickle <gary@dharvest.com>
  * Update  : Characters returned to Original room by: Anthony Michael Tregre
*/
DEF_DO_FUN(do_punload)
{
    CHAR_DATA *victim;
    char who[MAX_INPUT_LENGTH];
    
    argument = one_argument(argument, who);
    
    if (who[0] == '\0')
    {
	send_to_char( "Punload who?\n\r", ch);
	return;
    }


    if ( ( victim = char_list_find(who) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Can't punload an NPC!\n\r", ch);
	return;
    }
    
    /** Person is legitimately logged on... was not ploaded.
    */
    if (victim->desc != NULL)
    { 
        send_to_char("I don't think that would be a good idea...\n\r", ch);
        return;
    }
        
    act("You release $N back into the void.", 
        ch, NULL, victim, TO_CHAR);
    act("$n releases $N back into the void.", 
        ch, NULL, victim, TO_ROOM);

    /* make sure player is released into original room */
    if ( victim->was_in_room != NULL )
    {
	char_from_room( victim );
	char_to_room( victim, victim->was_in_room );
	victim->was_in_room = NULL;
    }
    if ( victim->pet != NULL )
    {
	char_from_room( victim->pet );
	char_to_room( victim->pet, victim->in_room );
    }

    quit_save_char_obj(victim);
    extract_char(victim, TRUE);
    
} /* end do_punload */

RESERVED_DATA *first_reserved;
RESERVED_DATA *last_reserved;

/* Reserved names, ported from Smaug by Rimbol 3/99. */
void save_reserved(void)
{
    RESERVED_DATA *res;
    FILE *fp;
    
    if (!(fp = fopen(RESERVED_LIST, "w")))
    {
        bug( "Save_reserved: cannot open " RESERVED_LIST, 0 );
        log_error(RESERVED_LIST);
        return;
    }
    
    for (res = first_reserved; res; res = res->next)
        rfprintf(fp, "%s~\n", res->name);
    
    fprintf(fp, "$~\n");
    fclose(fp);
    return;
}

/* Reserved names, ported from Smaug 1.4 by Rimbol 3/99. */
DEF_DO_FUN(do_reserve)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    RESERVED_DATA *res;

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);
    
    /* too many names already --Bobble
    if (!*arg)
    {
        int wid = 0;
        BUFFER *buffer = new_buf();
        char buf[MAX_STRING_LENGTH];
    
        sprintf( buf, "\n\r-- Reserved Names --\n\r" );
	add_buf( buffer, buf );
        for (res = first_reserved; res; res = res->next)
        {
            sprintf( buf, "%c%-17s ", (*res->name == '*' ? '*' : ' '),
                (*res->name == '*' ? res->name+1 : res->name));
	    add_buf( buffer, buf );
            if (++wid % 4 == 0)
                add_buf( buffer, "\n\r" );

	    if ( wid >= 1000 )
		break;

        }
        if (wid % 4 != 0)
            add_buf( buffer, "\n\r" );

	page_to_char( buf_string(buffer), ch );
	free_buf(buffer);
        return;
    }
    */
    
    if ( !str_cmp( arg2, "remove" ) )
    {
        for (res = first_reserved; res; res = res->next)
            if (!str_cmp(arg, res->name))
            {
                if ( !res->prev )
                    first_reserved  = res->next;
                else                        
                    res->prev->next = res->next;
                
                if ( !res->next )
                    last_reserved   = res->prev;
                else                        
                    res->next->prev = res->prev;
                
                free_string(res->name);
                free_mem(res, sizeof(RESERVED_DATA));
                save_reserved();
                send_to_char("Name no longer reserved.\n\r", ch);
                return;
            }

	printf_to_char( ch, "The name %s hasn't been reserved.\n\r", arg );
	return;
    }
    
    if ( !str_cmp( arg2, "add" ) )
    {
        for (res = first_reserved; res; res = res->next)
            if ( !str_cmp(arg, res->name) )
            {
                printf_to_char( ch, "The name %s has already been reserved.\n\r", arg );
                return;
            }
            
            
            res = alloc_mem(sizeof(RESERVED_DATA));
            res->name = str_dup(arg);
            
            sort_reserved(res);
            save_reserved();
            send_to_char("Name reserved.\n\r", ch);
            return;
            
    }
    send_to_char("Syntax:\n\r"
        "To view list of reserved names:  reserve\n\r"
        "To add a name:                   reserve <name> add\n\r"
        "To remove a name:                reserve <name> remove\n\r", ch );        
}


/* if global_immediate_flush is TRUE, strings will be sent to players,
 * otherwise at end of pulse (used for debugging)
 */
#ifdef FLUSH_DEBUG
bool global_immediate_flush = TRUE;
#else
bool global_immediate_flush = FALSE;
#endif

/* toggle immediate descrictor flushing (debugging tool) */
DEF_DO_FUN(do_flush)
{
  char arg[MAX_INPUT_LENGTH];

  argument = one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    if (global_immediate_flush)
      send_to_char("Immediate descriptor flushing is currently ON.\n\r", ch);
    else
      send_to_char("Immediate descriptor flushing is currently OFF.\n\r", ch);
    send_to_char("Use 'flush on' or 'flush off' to turn it on or off.\n\r", ch);
    return;
  }

  if (!str_cmp(arg, "on"))
  {
    if (global_immediate_flush)
      send_to_char("Immediate descriptor flushing is already ON.\n\r", ch);
    else
    {
      global_immediate_flush = TRUE;
      send_to_char("Immediate descriptor flushing is now ON.\n\r", ch);
    }
    return;
  }

  if (!str_cmp(arg, "off"))
  {
    if (!global_immediate_flush)
      send_to_char("Immediate descriptor flushing is already OFF.\n\r", ch);
    else
    {
      global_immediate_flush = FALSE;
      send_to_char("Immediate descriptor flushing is now OFF.\n\r", ch);
    }
    return;
  }
  
  send_to_char("Syntax: flush [on|off]\n\r", ch);
}

/* list quests for a character */
DEF_DO_FUN(do_qlist)
{
    CHAR_DATA *victim;
    char arg[MIL];
    char buf[MSL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: qlist [name]\n\r", ch );
	return;
    }

    victim = get_char_world( ch, arg );
    if ( victim == NULL )
    {
	send_to_char( "Character not found.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "NPCs don't have a quest status.\n\r", ch );
	return;
    }

    sprintf( buf, "Quests for %s:\n\r\n\r", victim->name );
    send_to_char( buf, ch );
    show_quests( victim, ch );

    send_to_char( "\n\r", ch);
    show_luavals( victim, ch );
}

void check_sn_multiplay( CHAR_DATA *ch, CHAR_DATA *victim, int sn )
{
    char buf[MSL];
    if ( ch == victim || !is_same_player(ch, victim) )
	return;

    sprintf(buf, "Multiplay: %s %s %s on %s",
	    ch->name,
	    IS_SPELL(sn) ? "casting" : "using",
	    skill_table[sn].name,
	    victim->name );
    log_string( buf );
    cheat_log( buf );
    wiznet(buf, ch, NULL, WIZ_CHEAT, 0, LEVEL_IMMORTAL);

}

/* command that crashes the mud for testing purposes --Bobble */
DEF_DO_FUN(do_crash)
{
    int i = 0, *p = NULL;

    if ( !strcmp("null", argument) )
        *p = 0;
    else if ( !strcmp("div", argument) )
        i = 1/i;
    else if ( !strcmp("exit", argument) )
        exit(1);
    else
    {
        send_to_char( "Syntax: crash [null|div|exit]\n\r", ch );
        send_to_char( "Warning: THIS WILL CRASH THE MUD!!!\n\r", ch );
        return;
    }

    /* prevent compiler optimizations.. */
    if ( i == 0 || *p == 0 )
        send_to_char( "I smell fish.\n\r", ch );
}

DEF_DO_FUN(do_avatar)
{
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  OBJ_DATA *obj_next;
  OBJ_DATA *obj;
  int level;
  int iLevel;

  argument = one_argument ( argument, arg1 );

  /* Check Statements */

  if (arg1[0] == '\0' || !is_number(arg1))
  {
    send_to_char( "Syntax: avatar <level>.\n\r", ch);
    return;
  }

  if ( IS_NPC(ch) )
  {
    send_to_char( "Not on NPC's.\r\n",ch);
    return;
  }

  if (( level = atoi(arg1)) < 1 || level > MAX_LEVEL)
  {
    sprintf(buf, "Level must be 1 to %d.\r\n", MAX_LEVEL );
    send_to_char( buf, ch );
    return;
  }

  if ( level > get_trust(ch))
  {
    send_to_char( "Limited to your trust level.\n\r", ch);
    sprintf( buf, "Your Trust is %d.\r\n",ch->trust);
    send_to_char(buf, ch);
    return;
  }

  /* Trust stays so immortal commands still work */
  if (ch->trust == 0)
  {
    ch->trust = ch->level;
  }

  /* Level gains */

  if (level <= ch->level )
  {
    int temp_prac;

    send_to_char( "Lowering a player's level!\n\r", ch);
    send_to_char( "**** OOOOHHHHHHHH NNNNOOOO ****\n\r", ch);
    temp_prac= ch->practice;
    ch->level     = 1;
    advance_level (ch, TRUE );
    ch->practice = temp_prac;
  }
  else
  {
    send_to_char( "Raising a player's level!\n\r", ch);
    send_to_char( "**** OOOOHHHHHHHHHH  YYYYEEEESSS ****\n\r", ch);
  }

  for ( iLevel = ch->level ; iLevel < level; iLevel++)
  {
    ch->level +=1;
    advance_level(ch,TRUE);
  }
  sprintf(buf,"You are now level %d.\n\r",ch->level);
  send_to_char(buf,ch);
  ch->exp = exp_per_level(ch) * UMAX(1, ch->level);

  /* Forces the player to remove all so they need the right level eq */

  if(ch->level < 100)
  {
	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	  if (obj->wear_loc != WEAR_NONE && can_see_obj (ch, obj))
      {
        remove_obj (ch, obj->wear_loc, TRUE);
	  }
	}

/* old code has some weird display bug where it tries to remove obj in your inventory even though it's not
equipped. - Astark 6/12

    for (obj =ch->carrying; obj; obj = obj_next)
    {
      obj_next = obj->next_content;
        remove_obj (ch, obj->wear_loc, TRUE);
     
    }
*/

  }
  /* save_char_obj(ch);  save character */
  force_full_save();
  return;
}

DEF_DO_FUN(do_printlist)
{
    if (argument[0]=='\0')
    {
        ptc(ch, "Arguments: timers, save\n\r");
        return;
    }
    else if (!strcmp(argument, "timers"))
    {
        page_to_char( print_timer_list(), ch);
        return;
    }
    else if (!strcmp(argument, "save"))
    {
        char arg [MAX_INPUT_LENGTH];
        MEMFILE *mf;

        argument = one_argument(argument, arg);
        
        send_to_char("\n\r", ch);
        send_to_char("player_quit_list:\n\r", ch);
        for (mf=player_quit_list ; mf != NULL ; mf = mf->next)
        {
            send_to_char(mf->filename, ch);
            send_to_char("\n\r",ch);
        }
        send_to_char("\n\r",ch);
        send_to_char("player_save_list:\n\r", ch);
        for (mf=player_save_list ; mf != NULL ; mf = mf->next)
        {
            send_to_char(mf->filename, ch);
            send_to_char("\n\r",ch);
        }
        send_to_char("\n\r",ch);
        send_to_char("box_mf_list:\n\r", ch);
        for (mf=box_mf_list ; mf != NULL ; mf = mf->next)
        {
            send_to_char(mf->filename, ch);
            send_to_char("\n\r",ch);
        }
        send_to_char("\n\r",ch);
        return;
    } /* save */
    else
    {
        do_printlist( ch, "");
    }
}

DEF_DO_FUN(do_mortlag)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int x;
  CHAR_DATA *victim;

  argument = one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    send_to_char("Syntax : mortlag {M<char> {W<0-200>{x\n\r", ch);
    send_to_char("{R                    100 and above use sparingly!{x\n\r", ch);
    return;
  }

  if ((x = atoi(argument)) <=0)
  {
    send_to_char("{RNumerical arguments only please!{x\n\r", ch);
    return;
  }


  if ((victim = get_char_world(ch, arg)) ==NULL)
  {
    send_to_char("{RThey aren't of this world!{x\n\r", ch);
    return;
  }
  else
  {
    if (get_trust (victim) >= get_trust(ch))
    {
      send_to_char("You failed.\r\n", ch);
      return;
    }

    if (ch == victim)
    {
      send_to_char("{RDon't lag yourself! {WDoh!{x\n\r",ch);
    }
    else if (x > 200)
    {
      send_to_char("{RDon't be that mean!{x", ch);
      return;
    }
    else
    {
     /* send_to_char("{RSomeone doesn't like you!{x", victim); */
      victim->wait = victim->wait + x;
      sprintf(buf, "{RYou add lag to {W%s{x", victim->name);
      send_to_char( buf, ch );
      return;
    }
  }
}

DEF_DO_FUN(do_pgrep)
{
    if ( argument[0] == '\0' )
    {
        send_to_char(" pgrep <text> -- searches for the text in the player folder\n\r", ch );
        return;
    }

    char buf[MSL];
    sprintf( buf, "grep \"%s\" ../player/*", argument);
    do_pipe(ch, buf);

    send_to_char( "\n\r",ch);

    sprintf( buf, "grep \"%s\" ../box/*", argument);
    do_pipe(ch, buf);

    send_to_char( "\n\r",ch);
    return;
} 

/* do_tables stuff */
static void print_flag_table( CHAR_DATA *ch, const struct flag_type *tbl)
{
    char buf[MSL];
    BUFFER *buffer=new_buf();

    int i;
    sprintf(buf, "%-20s %s\n\r", "Name", "Settable");
    add_buf( buffer, buf );
    add_buf( buffer, "----------------------------------------\n\r");
    for ( i=0; tbl[i].name ; i++ )
    {
        sprintf( buf, "%-20s %s\n\r", tbl[i].name, tbl[i].settable ? "TRUE" : "FALSE" );
        if (!add_buf( buffer, buf ))
        {
            bugf("Bad stuff happened");
            free_buf(buffer);
            return;
        }
    }

    page_to_char( buf_string(buffer), ch );
    free_buf(buffer);
    return;
}

static void print_item_table( CHAR_DATA *ch, const struct item_type *tbl)
{
    ptc( ch, "Name\n\r");
    ptc( ch, "------------------------------\n\r");
    int i;
    for (i=0 ; tbl[i].name ; i++)
    {
        ptc(ch, "%s\n\r", tbl[i].name );
    }
}

static void print_attack_table( CHAR_DATA *ch, const struct attack_type *tbl)
{
    ptc( ch, "%-20s %-20s %-20s\n\r", "Name", "Noun", "Damtype");
    ptc( ch, "--------------------------------------------------------------------------------\n\r");
    int i;
    for (i=0 ; tbl[i].name ; i++)
    {
        ptc(ch, "%-20s %-20s %-20s\n\r",
                tbl[i].name,
                tbl[i].noun,
                flag_stat_string( damage_type, tbl[i].damage) );
    }
}

static void print_liq_table( CHAR_DATA *ch, const struct liq_type *tbl)
{
    ptc( ch, "%-20s %-20s %5s %5s %5s %5s %5s\n\r",
            "Name", "Color",
            "Proof", "Full", "Thrst", "Food", "Ssize");
    ptc( ch, "--------------------------------------------------------------------------------\n\r");
    int i;
    for (i=0 ; tbl[i].liq_name ; i++)
    {
        ptc( ch, "%-20s %-20s %5d %5d %5d %5d %5d\n\r",
                tbl[i].liq_name,
                tbl[i].liq_color,
                tbl[i].liq_affect[0],
                tbl[i].liq_affect[1],
                tbl[i].liq_affect[2],
                tbl[i].liq_affect[3],
                tbl[i].liq_affect[4] );
    }
}

static void print_stances( CHAR_DATA *ch, const struct stance_type *tbl)
{
    ptc( ch, "%-18s %-10s %-16s %-3s %-3s %-4s\n\r",
            "Name",
            "Damtype",
            "Verb",
            "Hth",
            "Wpn",
            "Cost");
    ptc( ch, "--------------------------------------------------------------------------------\n\r");
    int i;
    for (i=0 ; tbl[i].name ; i++)
    {
        ptc( ch, "%-18s %-10s %-16s %-3s %-3s %4d\n\r",
                tbl[i].name,
                flag_stat_string( damage_type, tbl[i].type),
                tbl[i].verb,
                tbl[i].martial ? "YES" : "no",
                tbl[i].weapon ? "YES" : "no",
                tbl[i].cost);
    }

}

static void print_skill_table (CHAR_DATA *ch, const struct skill_type *tbl)
{
    ptc( ch, "%3s %s\n\r", "SN","Name");
    int sn;
    for ( sn=0 ; tbl[sn].name ; sn++)
    {
        ptc( ch, "%3d %s\n\r",
                sn,
                tbl[sn].name);
    }
}

#define PRFLAG( flgtbl, note ) { #flgtbl , print_flag_table, flgtbl, note }

struct
{
    const char *name;
    void (*printfun)();
    const void *table;
    const char *note;
} dotable_table[]=
{
    PRFLAG( area_flags, "" ),
    PRFLAG( exit_flags, "" ),
    PRFLAG( damage_type, "" ),
    PRFLAG( type_flags, "Item type flags." ),
    PRFLAG( affect_flags, "" ),
    PRFLAG( off_flags, "Offensive flags." ),
    PRFLAG( imm_flags, "Immune flags." ),
    PRFLAG( res_flags, "Resist flags." ),
    PRFLAG( vuln_flags, ""),
    PRFLAG( extra_flags, "Item extra flags." ),
    PRFLAG( wear_flags, "" ),
    PRFLAG( room_flags, "" ),
    PRFLAG( wear_loc_flags, ""),
    PRFLAG( act_flags, ""),
    PRFLAG( plr_flags, ""),
    PRFLAG( form_flags, ""),
    PRFLAG( part_flags, ""),
    PRFLAG( comm_flags, ""),
    PRFLAG( mprog_flags, ""),
    PRFLAG( oprog_flags, ""),
    PRFLAG( aprog_flags, ""),
    PRFLAG( rprog_flags, ""),
    PRFLAG( sex_flags, ""),
    PRFLAG( door_resets, ""),
    PRFLAG( sector_flags, ""),
    PRFLAG( apply_flags, ""),
    PRFLAG( wear_loc_strings, ""),
    PRFLAG( container_flags, ""),
    PRFLAG( size_flags, ""),
    PRFLAG( weapon_class, ""),
    PRFLAG( weapon_type2, ""),
    PRFLAG( position_flags, ""),
    PRFLAG( portal_flags, ""),
    PRFLAG( furniture_flags, ""),
    PRFLAG( apply_types, ""),

    { "item_table", print_item_table, item_table, "Item types." },
    { "attack_table", print_attack_table, attack_table, "Attack types."},
    { "liq_table", print_liq_table, liq_table, "Liquid types."},
    { "stances", print_stances, stances, "Stances."},
    { "skill_table", print_skill_table, skill_table, "Skills."},
    { NULL, NULL, NULL, NULL}
};

void do_tables( CHAR_DATA *ch, const char *argument)
{
    int i;

    if ( argument[0] == '\0' )
    {
        ptc( ch, "%-20s %s\n\r", "Table", "Note");
        ptc( ch, "-----------------------------------------------------------\n\r");
        for ( i=0; dotable_table[i].name ; i++ )
        {
            ptc(ch, "%-20s %s\n\r", 
                    dotable_table[i].name,
                    dotable_table[i].note );
        }
        return;
    }

    for ( i=0; dotable_table[i].name ; i++ )
    {
        if (!str_prefix( argument, dotable_table[i].name ) )
        {
            dotable_table[i].printfun(
                    ch,
                    dotable_table[i].table);
            return;
        }
    }

    do_tables( ch, "");

}

void do_repeat( CHAR_DATA *ch, const char *argument )
{
    char arg1[MIL];
    int nr, i;
    
    if ( argument[0] == '\0' )
    {
        send_to_char("Syntax: repeat <nr> command [arguments]\n\r", ch);
        return;
    }
    
    argument = one_argument(argument, arg1);
    nr = atoi(arg1);
    if ( nr < 1 || nr > 100 )
    {
        send_to_char("First argument must be a number between 1 and 100.\n\r", ch);
        return;
    }
        
    if ( argument[0] == '\0' )
    {
        send_to_char("What command do you want to repeat?\n\r", ch);
        return;
    }
    
    for ( i = 0; i < nr; i++ )
    {
        interpret(ch, argument);
        // saveguard against repeated quitting and other bright ideas
        if ( !valid_CH(ch) || ch->must_extract )
            return;
    }
}
