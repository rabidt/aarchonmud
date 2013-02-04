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

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
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
#include "leaderboard.h"
#if defined(linux)
int     execl           args( ( const char *path, const char *arg, ... ) );
int close       args( ( int fd ) );
bool    write_to_descriptor args( ( int desc, char *txt, int length ) );
#endif



/* command procedures needed */
DECLARE_DO_FUN(do_mload     );
DECLARE_DO_FUN(do_oload     );
DECLARE_DO_FUN(do_quit      );
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_stand     );
DECLARE_DO_FUN(do_help      );
DECLARE_DO_FUN(do_grant     );
DECLARE_DO_FUN(do_revoke    );


/*
* Local functions.
*/
int   flag_value     args( ( const struct flag_type *flag_table, char *argument) );
void  sort_reserved  args( ( RESERVED_DATA *pRes ) );
void  raw_kill       args( ( CHAR_DATA *victim, CHAR_DATA *killer, bool to_morgue ) );
void do_qset(CHAR_DATA *ch, char *argument);

void do_wiznet( CHAR_DATA *ch, char *argument )
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



void wiznet(char *string, CHAR_DATA *ch, OBJ_DATA *obj,
            long flag, long flag_skip, int min_level) 
{
    DESCRIPTOR_DATA *d;
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ((d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected))
	    &&  d->character != ch
	    && (flag == WIZ_AUTH && CAN_AUTH(d->character)
		|| (IS_IMMORTAL(d->character) 
		    && IS_SET(d->character->wiznet,WIZ_ON) 
		    && (!flag || IS_SET(d->character->wiznet,flag))
		    && (!flag_skip || !IS_SET(d->character->wiznet,flag_skip))
		    && get_trust(d->character) >= min_level)))
        {
            if (IS_SET(d->character->wiznet,WIZ_PREFIX))
                send_to_char("{V--> ",d->character);
            else
                send_to_char("{V", d->character);
            act_new(string,d->character,obj,ch,TO_CHAR,POS_DEAD);
            send_to_char("{x", d->character );
        }
    }
    
    return;
}



/* equips a character */
void do_outfit ( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    int i,sn,vnum;
    
    if ( !IS_IMMORTAL(ch) && ch->level > 5 || IS_NPC(ch) )
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

    if ( ( obj = get_obj_carry( ch, "newbie", ch ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_NEWBIE_GUIDE), 0 );
        obj->cost = 0;
        obj_to_char( obj, ch );
    }

    
    /* do the weapon thing */
    if ((obj = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
        sn = 0; 
        vnum = OBJ_VNUM_SCHOOL_SWORD; /* just in case! */
        
        for (i = 0; weapon_table[i].name != NULL; i++)
        {
            if (ch->pcdata->learned[sn] < 
                ch->pcdata->learned[*weapon_table[i].gsn])
            {
                sn = *weapon_table[i].gsn;
                vnum = weapon_table[i].vnum;
            }
        }
        if (ch->pcdata->learned[sn]>ch->pcdata->learned[gsn_hand_to_hand])
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


void do_smote(CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    char *letter,*name;
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

void do_bamfin( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    
    if ( !IS_NPC(ch) )
    {
        smash_tilde( argument );
        
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



void do_bamfout( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    
    if ( !IS_NPC(ch) )
    {
        smash_tilde( argument );
        
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



void do_deny( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    
    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Deny whom?\n\r", ch );
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
    
    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }
    
    SET_BIT(victim->act, PLR_DENY);
    send_to_char( "You are denied access!\n\r", victim );
    sprintf(buf,"$N denies access to %s",victim->name);
    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    send_to_char( "OK.\n\r", ch );
    stop_fighting(victim,TRUE);
    do_quit( victim, "" );
    
    return;
}



void do_disconnect( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    
    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Disconnect whom?\n\r", ch );
        return;
    }
    
    if (is_number(arg))
    {
        int desc;
        
        desc = atoi(arg);
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->descriptor == desc )
            {
                close_socket( d );
                send_to_char( "Ok.\n\r", ch );
                return;
            }
        }
    }
    
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( victim->desc == NULL )
    {
        act( "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
        return;
    }
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d == victim->desc )
        {
            close_socket( d );
            send_to_char( "Ok.\n\r", ch );
            return;
        }
    }
    
    bug( "Do_disconnect: desc not found.", 0 );
    send_to_char( "Descriptor not found!\n\r", ch );
    return;
}



void do_clear( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: clear <character> <killer|thief|bounty|pkill|hardcore|rp>.\n\r", ch );
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
    
    if ( !str_cmp( arg2, "bounty" ) )
    {
        victim->pcdata->bounty =0;
        send_to_char("Bounty removed.\n\r", ch);
        send_to_char("Your bounty has been pardoned.\n\r", victim);
        return;
    }
    
    if ( !str_cmp( arg2, "killer" ) )
    {
        if ( IS_SET(victim->act, PLR_KILLER) )
        {
            REMOVE_BIT( victim->act, PLR_KILLER );
            send_to_char( "Killer flag removed.\n\r", ch );
            send_to_char( "You are no longer a KILLER.\n\r", victim );
        }
        return;
    }
    
    if ( !str_cmp( arg2, "thief" ) )
    {
        if ( IS_SET(victim->act, PLR_THIEF) )
        {
            REMOVE_BIT( victim->act, PLR_THIEF );
            send_to_char( "Thief flag removed.\n\r", ch );
            send_to_char( "You are no longer a THIEF.\n\r", victim );
        }
        return;
    }
    
    if ( !str_cmp( arg2, "pkill" ) )
    {
        if ( IS_SET(victim->act, PLR_PERM_PKILL) )
        {
            if (get_trust(ch) < IMPLEMENTOR)
            {
                send_to_char( "Only an implementor can turn off permanent pkill.\n\r", ch);
                return;
            }
            REMOVE_BIT( victim->act, PLR_PERM_PKILL );
            REMOVE_BIT( victim->act, PLR_HARDCORE );
            send_to_char( "Pkill turned off.\n\r", ch );
            send_to_char( "You are no longer a pkiller.\n\r", victim );
        }
        return;
    }
    
    if ( !str_cmp( arg2, "hardcore" ) )
    {
        if ( IS_SET(victim->act, PLR_HARDCORE) )
        {
            if (get_trust(ch) < IMPLEMENTOR)
            {
                send_to_char( "Only an implementor can turn off hardcore pkill.\n\r", ch);
                return;
            }
            REMOVE_BIT( victim->act, PLR_HARDCORE );
            send_to_char( "Hardcore turned off.\n\r", ch );
            send_to_char( "You are no longer a hardcore pkiller.\n\r", victim );
        }
        return;
    }

    if ( !str_cmp( arg2, "rp" ) )
    {
        if ( IS_SET(victim->act, PLR_RP) )
        {
            if (get_trust(ch) < IMPLEMENTOR)
            {
                send_to_char( "Only an implementor can turn off roleplay.\n\r", ch);
                return;
            }
            REMOVE_BIT( victim->act, PLR_RP );
            send_to_char( "Roleplay turned off.\n\r", ch );
            send_to_char( "You are no longer a roleplayer.\n\r", victim );
        }
        return;
    }

    send_to_char( "Syntax: clear <character> <killer|thief|bounty|pkill|hardcore|rp>.\n\r", ch );
    return;
}



void do_echo( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Global echo what?\n\r", ch );
        return;
    }
    
    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected) ) 
        {
            if (get_trust(d->character) >= get_trust(ch))
                send_to_char( "global> ",d->character);
            send_to_char( argument, d->character );
            send_to_char( "\n\r",   d->character );
        }
    }
    
    return;
}



void do_recho( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Local echo what?\n\r", ch );
        
        return;
    }
    
    for ( d = descriptor_list; d; d = d->next )
    {
        if ( (d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected)) 
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

void do_zecho(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    
    if (argument[0] == '\0')
    {
        send_to_char("Zone echo what?\n\r",ch);
        return;
    }
    
    for (d = descriptor_list; d; d = d->next)
    {
        if ( (d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected)) 
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

void do_pecho( CHAR_DATA *ch, char *argument )
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


void do_transfer( CHAR_DATA *ch, char *argument )
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
            if ( (d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected)) 
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
    else if ( !str_cmp( arg1, "questors" ) )
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( (d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected)) 
                &&   d->character != ch
                &&   d->character->in_room != NULL
                &&   can_see( ch, d->character ) 
                &&   IS_SET( d->character->act, PLR_IMMQUEST ) )
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



void do_at( CHAR_DATA *ch, char *argument )
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



void do_goto( CHAR_DATA *ch, char *argument )
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
    
    if (!is_room_owner(ch,location) && room_is_private(location) 
        &&  (count > 1 || get_trust(ch) < MAX_LEVEL))
    {
        send_to_char( "That room is private right now.\n\r", ch );
        return;
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

void do_violate( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    
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
    
    if (!room_is_private( location ))
    {
        send_to_char( "That room isn't private, use goto.\n\r", ch );
        return;
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


void do_copyove( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to COPYOVER, spell it out.\n\r", ch );
    return;
}

void do_reboo( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to REBOOT, spell it out.\n\r", ch );
    return;
}

void do_reboot( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    CHAR_DATA *vch;
    
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



void do_shutdow( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SHUTDOWN, spell it out.\n\r", ch );
    return;
}



void do_shutdown( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    CHAR_DATA *vch;
    
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

void do_protect( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    
    if (argument[0] == '\0')
    {
        send_to_char("Protect whom from snooping?\n\r",ch);
        return;
    }
    
    if ((victim = get_char_world(ch,argument)) == NULL)
    {
        send_to_char("You can't find them.\n\r",ch);
        return;
    }
    
    if (IS_SET(victim->comm,COMM_SNOOP_PROOF))
    {
        act_new("$N is no longer snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
        send_to_char("Your snoop-proofing was just removed.\n\r",victim);
        REMOVE_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
    else
    {
        act_new("$N is now snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
        send_to_char("You are now immune to snooping.\n\r",victim);
        SET_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
}



void do_snoop( CHAR_DATA *ch, char *argument )
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
    
    if ( get_trust( victim ) >= get_trust( ch ) 
        ||   IS_SET(victim->comm,COMM_SNOOP_PROOF))
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



void do_switch( CHAR_DATA *ch, char *argument )
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



void do_return( CHAR_DATA *ch, char *argument )
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
    
    if ( buf_string(ch->desc->original->pcdata->buffer)[0] != '\0')
        send_to_char( "Type 'replay' to see missed tells.\n\r", ch );
    
    
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
void do_clone(CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char *rest;
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

void do_load(CHAR_DATA *ch, char *argument )
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


void do_mload( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
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



void do_oload( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
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



void do_purge( CHAR_DATA *ch, char *argument )
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



void do_advance( CHAR_DATA *ch, char *argument )
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
    victim->exp = exp_per_level(victim,victim->pcdata->points) 
        * UMAX( 1, victim->level );
    victim->trust = 0;
    return;
}



void do_trust( CHAR_DATA *ch, char *argument )
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

void do_restore( CHAR_DATA *ch, char *argument )
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



void do_log( CHAR_DATA *ch, char *argument )
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




void do_peace( CHAR_DATA *ch, char *argument )
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

void do_wizlock( CHAR_DATA *ch, char *argument )
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

void do_newlock( CHAR_DATA *ch, char *argument )
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

void do_ptitle( CHAR_DATA *ch, char *argument)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
		send_to_char("  ptitle <name> <title>\n\r",ch);
		send_to_char("  ptitle list\n\r",ch);
        return;
    }
    if (!strcmp(arg1, "list"))
    {
        FILE * fp;
        char * word;
        char buf [MSL];
        int cost;

        fclose(fpReserve);
        strcpy(buf, "../area/pre_titles.txt");
        if (!(fp = fopen(buf, "r")))
        {
            bug("Can't open pre_titles.txt.",0);
            return FALSE;
        }

        printf_to_char(ch,"%-15s %4s\n\r", "Title", "Cost");
        for ( ; ; )
        {
            word = fread_word( fp );
            if (!strcmp(word, "End"))
             break;
            cost =fread_number(fp);
            printf_to_char(ch, "%-15s %4d\n\r",word,cost);
        }
        send_to_char("\n\r",victim);
        fclose(fp);
        fpReserve = fopen( NULL_FILE, "r" );
        return FALSE;
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
    set_pre_title(ch,argument,victim);
	return;
}

void do_namecolor( CHAR_DATA *ch, char *argument)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0'  )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  namecolor <name> <color>\n\r",ch);
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
    color_name(ch,argument,victim);

}




void do_pflag( CHAR_DATA *ch, char *argument)
{
	int i;
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    sh_int duration;
    CHAR_DATA *victim;
    
    smash_tilde( argument );
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

void do_string( CHAR_DATA *ch, char *argument )
{
    char type [MAX_INPUT_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    
    smash_tilde( argument );
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
            strcat(arg3,"\n\r");
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
            
            strcat(argument,"\n\r");
            
            ed = new_extra_descr();
            
            ed->keyword     = str_dup( arg3     );
            ed->description = str_dup( argument );
            ed->next        = obj->extra_descr;
            obj->extra_descr    = ed;
            return;
        }
    }
    
    
    /* echo bad use message */
    do_string(ch,"");
}






/* Enhanced sockets command by Stumpy, with mods by Silverhand */
void do_sockets( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA       *vch;
    DESCRIPTOR_DATA *d;
    char            buf  [ MAX_STRING_LENGTH ];
    char            buf2 [ MAX_STRING_LENGTH ];
    int             count;
    char *          st;
    char            s[100];
    char            idle[10];
    
    
    count       = 0;
    buf[0]      = '\0';
    buf2[0]     = '\0';
    
    strcat( buf2,
        "\n\r:=============================================================================:\n\r" );
    strcat( buf2, "|<><><><><><><><><><><><><><><><>  Sockets  <><><><><><><><><><><><><><><><><>|\n\r");  
    strcat( buf2, ":=============================================================================:\n\r");
    strcat( buf2, "|  [Num  State    Login  Idle ] [ Player Name ] [     Host     ]              |\n\r"); 
    strcat( buf2, ":=============================================================================:\n\r");
    
    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->character && can_see( ch, d->character ) )
        {
            /* NB: You may need to edit the CON_ values */
            /* I updated to all current rom CON_ values -Silverhand */
            switch( d->connected % MAX_CON_STATE)
            {
            case CON_PLAYING:              st = "PLAYING ";    break;
            case CON_GET_NAME:             st = "Get Name";    break;
            case CON_GET_OLD_PASSWORD:     st = "Passwd  ";    break;
            case CON_CONFIRM_NEW_NAME:     st = "New Nam ";    break;
            case CON_GET_NEW_PASSWORD:     st = "New Pwd ";    break;
            case CON_CONFIRM_NEW_PASSWORD: st = "Con Pwd ";    break;
            case CON_GET_NEW_RACE:         st = "New Rac ";    break;
            case CON_GET_NEW_SEX:          st = "New Sex ";    break;
            case CON_GET_NEW_CLASS:        st = "New Cls ";    break;
            case CON_GET_ALIGNMENT:        st = "New Aln ";	 break;
            case CON_DEFAULT_CHOICE:	     st = "Default ";	 break;
            case CON_GET_CREATION_MODE:	     st = "Cre Mod ";	 break;
            case CON_ROLL_STATS:	     st = "Roll St ";	 break;
            case CON_GET_STAT_PRIORITY:	     st = "Sta Pri ";	 break;
            case CON_NOTE_TO:              st = "Note To ";    break;
            case CON_NOTE_SUBJECT:         st = "Note Sub";    break;
            case CON_NOTE_EXPIRE:          st = "Note Exp";    break;
            case CON_NOTE_TEXT:            st = "Note Txt";    break;
            case CON_NOTE_FINISH:          st = "Note Fin";    break;
            case CON_GEN_GROUPS:	     st = " Custom ";	 break;
            case CON_PICK_WEAPON:	     st = " Weapon ";	 break;
            case CON_READ_IMOTD:  	     st = " IMOTD  "; 	 break;
            case CON_BREAK_CONNECT:	     st = "LINKDEAD";	 break;
            case CON_READ_MOTD:            st = "  MOTD  ";    break;
	    case CON_GET_COLOUR:	   st = " Colour?";    break;
            default:                       st = "UNKNOWN!";    break;
            }
            count++;
            
            /* Format "login" value... */
            vch = d->original ? d->original : d->character;
            strftime( s, 100, "%I:%M%p", localtime( &vch->logon ) );
            
            if ( vch->timer > 0 )
                sprintf( idle, "%-4d", vch->timer );
            else
                sprintf( idle, "    " );
            
            sprintf( buf, "| [%-3d %-8s %7s  %4s]  %-12s   %-30s |\n\r",
                d->descriptor,
                st,
                s,
                idle,
                ( d->original ) ? d->original->name
                : ( d->character )  ? d->character->name
                : "(None!)",
                d->host );
            
            strcat( buf2, buf );
            
        }
    }
    
    strcat( buf2, "|                                                                             |");
    sprintf( buf, "\n\r|  Users: %-2d                                                                  |\n\r", count );
    strcat( buf2, buf );
    strcat( buf2, ":=============================================================================:");
    send_to_char( buf2, ch );
    return;
}

/*
void do_sockets( CHAR_DATA *ch, char *argument )
{
char buf[2 * MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
char arg[MAX_INPUT_LENGTH];
DESCRIPTOR_DATA *d;
int count;

  count   = 0;
  buf[0]  = '\0';
  
    one_argument(argument,arg);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
    if ( d->character != NULL && can_see( ch, d->character ) 
    && (arg[0] == '\0' || is_name(arg,d->character->name)
    || (d->original && is_name(arg,d->original->name))))
    {
    count++;
    sprintf( buf + strlen(buf), "[%3d %2d] %s@%s\n\r",
    d->descriptor,
    d->connected,
    d->original  ? d->original->name  :
    d->character ? d->character->name : "(none)",
    d->host
    );
    }
    }
    if (count == 0)
    {
    send_to_char("No one by that name is connected.\n\r",ch);
    return;
    }
    
    sprintf( buf2, "%d user%s\n\r", count, count == 1 ? "" : "s" );
    strcat(buf,buf2);
    page_to_char( buf, ch );
    return;
    }
*/
      
      
/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force( CHAR_DATA *ch, char *argument )
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
void do_invis( CHAR_DATA *ch, char *argument )
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


void do_incognito( CHAR_DATA *ch, char *argument )
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



void do_holylight( CHAR_DATA *ch, char *argument )
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

void do_prefi (CHAR_DATA *ch, char *argument)
{
    send_to_char("You cannot abbreviate the prefix command.\r\n",ch);
    return;
}

void do_prefix (CHAR_DATA *ch, char *argument)
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


void do_sla( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
    return;
}



void do_slay( CHAR_DATA *ch, char *argument )
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



/* Omni wiz command by Prism <snazzy@ssnlink.net> */
void do_omni( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int immmatch;
    int mortmatch;
    int hptemp;
    
    /*
    * Initalize Variables.
    */
    
    immmatch  = 0;
    mortmatch = 0;
    buf[0]    = '\0';
    output    = new_buf();
    
    /*
    * Count and output the IMMs.
    */
    
    sprintf( buf, " ----Immortals:----\n\r");
    add_buf(output,buf);
    sprintf( buf, "Name           Level   Wiz     Incog  [ Vnum ]\n\r");
    add_buf(output,buf);
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA *wch;
        
	/*
        if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
            continue;
	*/

        wch = ( d->original != NULL ) ? d->original : d->character;
        
        if ( wch == NULL
	     || (d->connected != CON_PLAYING && !IS_WRITING_NOTE(d->connected)) )
            continue;

        if (!can_see(ch,wch) || !IS_IMMORTAL(wch))
            continue;
        
        immmatch++;
        
        sprintf( buf, "%-14s %03d     %03d     %03d    [ %05d]\n\r",
            wch->name, wch->level, wch->invis_level, wch->incog_level, wch->in_room->vnum);
        add_buf(output,buf);
    }
    
    
    /*
     * Count and output the Morts.
     */
    sprintf( buf, " \n\r ----Mortals:----\n\r");
    add_buf(output,buf);
    sprintf( buf, "Name           Race    Class   Position        Lev  %%hps   [  Vnum ]  [Quest]\n\r");
    add_buf(output,buf);
    hptemp = 0;
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA *wch;
        char const *class;
        
	/*
        if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
            continue;
	*/

        wch = ( d->original != NULL ) ? d->original : d->character;
        
        if ( wch == NULL
	     || (d->connected != CON_PLAYING && !IS_WRITING_NOTE(d->connected)) )
            continue;

        if (!can_see(ch,wch) || IS_IMMORTAL(wch))
            continue;
        
        mortmatch++;
        
        if ((wch->max_hit != wch->hit) && (wch->hit > 0))
            hptemp = (wch->hit*100)/wch->max_hit;
        else if (wch->max_hit == wch->hit)
            hptemp = 100;
        else if (wch->hit < 0)
            hptemp = 0;
        
        class = class_table[wch->class].who_name;
        /* Added an extra  %s for the questing check below - Astark Oct 2012 */
        sprintf( buf, "%-14s %6s  %3s     %-15s %-2d   %3d%%   [ %6d]    %s\n\r",
            wch->name,
            wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name : "     ",
            class,
            capitalize( position_table[wch->position].name) , 
            wch->level,
            hptemp,
            wch->in_room->vnum,
            /* Added to let IMMs see when players are questing. Allows for
               complaint-free copyovers - Astark Oct 2012 */
            IS_QUESTOR(wch) || IS_QUESTORHARD(wch) ? "Yes" : "No");
        add_buf(output,buf);
    }
    
    /*
    * Tally the counts and send the whole list out.
    */
    sprintf( buf2, "\n\rImmortals found: %d\n\r", immmatch );
    add_buf(output,buf2);
    sprintf( buf2, "  Mortals found: %d\n\r", mortmatch );
    add_buf(output,buf2);
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}


void do_as(CHAR_DATA *ch, char *argument)
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


/* Set/remove sticky flag on eq (esp. Quest eq).  For imms without "set". */
void do_sticky( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    
    obj = get_obj_world(ch,argument);
    
    if (obj != NULL)
    {
        if (IS_SET(obj->extra_flags, ITEM_STICKY))
        {
            send_to_char("Sticky flag removed.\n\r",ch);
            REMOVE_BIT(obj->extra_flags, ITEM_STICKY);
        }
        else
            send_to_char("Sticky flag set.\n\r",ch);
        SET_BIT(obj->extra_flags, ITEM_STICKY);
        return;
    }
    
    send_to_char("Object not found.\n\r",ch);
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

/* #define USE_MOUNT */
void do_pload( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA d;
    bool isChar = FALSE;
    char name[MAX_INPUT_LENGTH];
    
    if (argument[0] == '\0') 
    {
        send_to_char("Load who?\n\r", ch);
        return;
    }
    
    argument[0] = UPPER(argument[0]);
    argument = one_argument(argument, name);
    
    /* Don't want to load a second copy of a player who's already online! */
    if ( get_char_world( ch, name ) != NULL )
    {
        send_to_char( "That person is already connected!\n\r", ch );
        return;
    }
    
    isChar = load_char_obj(&d, name); /* char pfile exists? */
    
    if (!isChar) 
    {
        send_to_char("Load Who? Are you sure? I can't seem to find them.\n\r", ch);
        return;
    }
    
    d.character->desc 	= NULL;
    d.character->next	= char_list;
    char_list    		= d.character;
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

	update_lboard( LBOARD_MKILL, d.character, d.character->pcdata->mob_kills, 0);
	update_lboard( LBOARD_BHD, d.character, d.character->pcdata->behead_cnt, 0);
	update_lboard( LBOARD_QCOMP, d.character, d.character->pcdata->quest_success, 0);
	update_lboard( LBOARD_WKILL, d.character, d.character->pcdata->war_kills, 0);
	update_lboard( LBOARD_EXPL, d.character, d.character->pcdata->explored->set, 0);
	update_lboard( LBOARD_QFAIL, d.character, d.character->pcdata->quest_failed, 0);
	update_lboard( LBOARD_PKILL, d.character, d.character->pcdata->pkill_count, 0);
    
    if (d.character->pet != NULL)
    {
        char_to_room(d.character->pet,d.character->in_room);
        act("$n appears in the room.",d.character->pet,NULL,NULL,TO_ROOM);
    }
    
#if defined(USE_MOUNT)
    if (d.character->mount != NULL)
    {
        char_to_room(d.character->mount,d.character->in_room);
        act("$n has left reality behind once more!",d.character->mount,NULL,NULL,TO_ROOM);
        add_follower(d.character->mount,d.character);
        do_mount(d.character, d.character->mount->name);
    }

#endif
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
void do_punload( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char who[MAX_INPUT_LENGTH];
    
    argument = one_argument(argument, who);
    
    if ( ( victim = get_char_world( ch, who ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
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




void do_qflag( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *target;
    
    argument = one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
        send_to_char( "Who needs a quest flag set or removed?\n\r", ch );
        send_to_char( "Syntax:  qflag <character>\n\r", ch );
        return;
    }
    
    if ( ( target = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "That player isn't here.\n\r", ch );
        return;
    }
    
    if ( IS_NPC( target ) )
    {
        send_to_char( "NPC's can't play!!\n\r", ch );
        return;
    }
    
    if ( IS_SET( target->act, PLR_IMMQUEST ) )
    {
        act( "You remove $N from the quest.",ch,NULL,target,TO_CHAR );
        act( "$n removes you from the quest.",ch,NULL,target,TO_VICT );
        REMOVE_BIT( target->act, PLR_IMMQUEST );
        return;
    }
    else
    {
        act( "$N is now questing!",ch,NULL,target,TO_CHAR );
        act( "$n welcomes you to the quest!",ch,NULL,target,TO_VICT );
        SET_BIT( target->act, PLR_IMMQUEST );
        return;
    }
    
}






RESERVED_DATA *first_reserved;
RESERVED_DATA *last_reserved;

/* Reserved names, ported from Smaug by Rimbol 3/99. */
void save_reserved(void)
{
    RESERVED_DATA *res;
    FILE *fp;
    
    fclose(fpReserve);
    if (!(fp = fopen(RESERVED_LIST, "w")))
    {
        bug( "Save_reserved: cannot open " RESERVED_LIST, 0 );
        log_error(RESERVED_LIST);
        fpReserve = fopen( NULL_FILE, "r" );
        return;
    }
    
    for (res = first_reserved; res; res = res->next)
        fprintf(fp, "%s~\n", res->name);
    
    fprintf(fp, "$~\n");
    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");
    return;
}

/* Reserved names, ported from Smaug 1.4 by Rimbol 3/99. */
void do_reserve(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    RESERVED_DATA *res;

    buffer = new_buf();
    
    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);
    
    /* too many names already --Bobble
    if (!*arg)
    {
        int wid = 0;
        
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
void do_flush(CHAR_DATA *ch, char *argument)
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
void do_qlist( CHAR_DATA *ch, char *argument )
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
void do_crash( CHAR_DATA *ch, char *argument )
{
    int i = 0, *p = NULL;

    /*
    if ( get_trust(ch) < ML )
    {
	send_to_char( "Only IMPs may crash the mud!\n\r", ch );
	return;
    }
    */

    if ( !strcmp("null", argument) )
	*p = 0;
    else if ( !strcmp("div", argument) )
	i = 1/0;
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

/* New Qset command */
void do_qset( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MIL];
    char arg3[MIL];
    char arg4[MIL];
    char arg5[MIL];
    int timer;
    int limit;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );
    argument = one_argument( argument, arg5 );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: qset [name] [id] [value] [time limit]\n\r", ch );
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

    if ( !is_r_number(arg2) || !is_number(arg3) )
    {
	send_to_char("Invalid arguments.\n\r", ch) ;
	return;
    }

    /* This is a timer so that we can have expirable qsets to allow
       players to repeat quests without abusing them - Astark Oct 2012 */
    
    if ( !is_number(arg4) )
        timer = 0;
    else    
        timer = atoi(arg4);

    /* This is the counter that reduces the above timer. For example, if
       the timer is 5, and this number is 24, it will be 24 hours before
       the timer hits 0 - Astark Oct 2012 */

    if ( !is_number(arg5) )
        limit = 0;
    else    
        limit = atoi(arg5);

    if (victim->level <= 100)
    {
        send_to_char("Cheating isn't tolerated here.\n\r", ch) ;
        return ;
    }
    else
    {
        /* new function used to set timer on qstatus -Astark Oct 2012 */
        set_quest_status( victim, r_atoi( ch,arg2), atoi(arg3), timer, limit );
        act( "You have successfully changed $N's qstatus.", ch, NULL, victim, TO_CHAR );
    }
    return;

}

void do_dummy( CHAR_DATA *ch, char *argument)
{
	int value=12345;
	
	int *vptr=&value;
	
	int **vpptr=&vptr;
	
	printf_to_char(ch, "value: %d\n\r", value);
	printf_to_char(ch, "vptr: %d\n\r", vptr);
	printf_to_char(ch, "*vptr: %d\n\r", *vptr);
	printf_to_char(ch, "vpptr: %d\n\r", vpptr);
	printf_to_char(ch, "*vpptr: %d\n\r", *vpptr);
	printf_to_char(ch, "**vpptr: %d\n\r", **vpptr);
}


void do_avatar( CHAR_DATA *ch, char *argument ) /* Procedure Avatar */
{ /* Declaration */
  char buf[MAX_STRING_LENGTH];    /* buf */
  char arg1[MAX_INPUT_LENGTH];    /* arg1 */
  OBJ_DATA *obj_next;     /* obj data which is a pointer */
  OBJ_DATA *obj;      /* obj */
  int level;        /* level */
  int iLevel;       /* ilevel */

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
  ch->exp = exp_per_level(ch,ch->pcdata->points)
    *UMAX( 1, ch->level );

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


/* Added by Astark in November 2012. Heavily modified Snippet from Kadian, 
   Found on Darkoth's site */
void do_otype(CHAR_DATA *ch, char *argument)

{
    int type;
    int type2;
    int vnum=1;
    char buf[MAX_STRING_LENGTH];
    char buffer[12 * MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *item;      
    OBJ_INDEX_DATA *obj;
    bool found;

    item = one_argument(argument, arg1);
    one_argument ( item , arg2);
    found = FALSE;
    buffer [0] = '\0';

    if (arg1[0] == '\0')     
        {
        send_to_char("Type 'Help Otype' for usage\n\r",ch);
        return;
        }

    type2 = 0;

    if ((!str_cmp(arg1,"armor") || !str_cmp(arg1,"weapon"))
        && arg2[0] == NULL)
        {        
        send_to_char("syntax: otype [weapon|armor] [type|location]\n\r",ch);
        send_to_char("example: otype weapon mace OR otype armor head\n\r",ch);
        return;
        }
    /* Bug fix so that wrong types don't crash the MUD -Astark */
    else if ((str_cmp(arg1,"armor") || str_cmp(arg1,"weapon")) 
        && arg2[0] == NULL)
        {        
        send_to_char("syntax: otype [weapon|armor] [type|location]\n\r",ch);
        send_to_char("example: otype weapon mace OR otype armor head\n\r",ch);
        return;
        }        
    else if (!str_cmp(arg1,"armor"))
        {
        type = flag_value(type_flags,arg1);
        if ((type2 = flag_value(wear_flags,arg2)) == NO_FLAG)
         {
               send_to_char("No such armor type.\n\r",ch);
               return;      
         }
       }
    else if (!str_cmp(arg1,"weapon"))
        {
        type = flag_value(type_flags,arg1);
        if ((type2 = flag_value(weapon_class,arg2)) == NO_FLAG)
          {
                send_to_char("No such weapon type.\n\r",ch);
                return;
          }            
        }
    else
        {
            if((type = flag_value(type_flags,arg1)) == NO_FLAG)
                {
                send_to_char("Unknown Type.\n\r", ch);
                return;
                }
        }        

    int marker = 0;
    
    /* Different headers depending on which type is specified - Astark */
    if(str_cmp(arg1,"armor"))
        send_to_char("  #|Vnum  |Lvl |Damtype    |Weapon Flags            |Short Desc\n\r", ch);
    else
        send_to_char("  #|Vnum  |Lvl | Trans | Sticky | Short Desc\n\r", ch);
       
    for(;vnum <= top_vnum_obj; vnum++)
    {
        if((obj=get_obj_index(vnum)) != NULL && obj->level <= 100)
        {
            if((obj->item_type == type && type2 == 0
                && str_cmp(arg1,"weapon") && str_cmp(arg1,"armor"))
            || (obj->item_type == type && obj->value[0] == type2
                && str_cmp(arg1,"armor")))
            {        
                marker++;      
                sprintf(buf, "%3d|%-5d |%-3d |%-10s |%-23s |%-30s\n\r", 
                    marker, 
                    vnum, 
                    obj->level, 
                    basic_dam_name(attack_table[obj->value[3]].damage), 
                    weapon_bits_name(obj->value[4]), 
                    obj->short_descr);
                found = TRUE;
                strcat(buffer,buf);
            }
            else if((obj->item_type == type && type2 == 0
                && str_cmp(arg1,"weapon") && str_cmp(arg1,"armor"))
            || (obj->item_type == type && IS_SET(obj->wear_flags,type2)
                && str_cmp(arg1,"weapon")))
            {
                marker++;      
                sprintf(buf, "%3d|%-5d |%-3d | %-5s | %-6s | %-30s\n\r", 
                    marker, 
                    vnum, 
                    obj->level,
                    ( CAN_WEAR(obj, ITEM_TRANSLUCENT) ) ? "Trans" : "",
                    ( IS_OBJ_STAT(obj, ITEM_STICKY) )   ? "Sticky" : "",
                    obj->short_descr);
                found = TRUE;
                strcat(buffer,buf);
            }             
        }
    }
    if (!found)
        send_to_char("No objects of that type exist\n\r",ch);
    else            
        if (ch->lines)
            page_to_char(buffer,ch);
        else
            send_to_char(buffer,ch);
}   

void do_printlist(CHAR_DATA *ch, char *argument)
{
    char arg [MAX_INPUT_LENGTH];
    MEMFILE *mf;

    argument = one_argument(argument, arg);
    
    send_to_char("\n\r", ch);
    if (!strcmp(arg, "quit") || (arg[0]=='\0'))
    {
       send_to_char("player_quit_list:\n\r", ch);
       for (mf=player_quit_list ; mf != NULL ; mf = mf->next)
       {
           send_to_char(mf->filename, ch);
           send_to_char("\n\r",ch);
       }
       send_to_char("\n\r",ch);
    }
    if (!strcmp(arg, "save") || (arg[0]=='\0') )
    {
        send_to_char("player_save_list:\n\r", ch);
        for (mf=player_save_list ; mf != NULL ; mf = mf->next)
        {
            send_to_char(mf->filename, ch);
            send_to_char("\n\r",ch);
        }
       send_to_char("\n\r",ch);
    }
    if (!strcmp(arg, "box") || (arg[0]=='\0'))
    {
        send_to_char("box_mf_list:\n\r", ch);
        for (mf=box_mf_list ; mf != NULL ; mf = mf->next)
        {
            send_to_char(mf->filename, ch);
            send_to_char("\n\r",ch);
        }
       send_to_char("\n\r",ch);
    }
}

void do_charloadtest(CHAR_DATA *ch, char *argument)
{
   char buf[100000];
   char chName[MSL];
   FILE *fp;

   fp = popen( "ls -1 ../player", "r" );

   fgetf( buf, 100000, fp );




   pclose( fp );

   char * pch;
   pch = strtok (buf, "\n\r");

   while (pch != NULL)
   {
	//printf_to_char(ch,"%s\n\rbreak\n\r",pch);
	do_pload(ch, pch);
	do_punload(ch,pch);	

	pch = strtok( NULL, "\n\r");
   }






























   //page_to_char(buf,ch);
  
   return;
}

void do_lag(CHAR_DATA *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int x;
  CHAR_DATA *victim;

  argument = one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    send_to_char("Syntax : lag {M<char> {W<0-200>{x\n\r", ch);
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

