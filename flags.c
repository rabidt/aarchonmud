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
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@efn.org)				   *
 *	    Gabrielle Taylor						   *
 *	    Brian Moore (zump@rom.org)					   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include <time.h>

int flag_lookup args( ( const char *name, const struct flag_type *flag_table) );
void print_pkill_expire( CHAR_DATA *ch );

DEF_DO_FUN(do_flag)
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH],arg3[MAX_INPUT_LENGTH];
    char word[MAX_INPUT_LENGTH], buf[MSL];
    CHAR_DATA *victim;
    tflag *flag, old, new, marked;
    int pos;
    char type;
    const struct flag_type *flag_table;
    bool old_set, new_set;
    int bit;

    argument = one_argument(argument,arg1);
    argument = one_argument(argument,arg2);
    argument = one_argument(argument,arg3);

    type = argument[0];

    if (type == '=' || type == '-' || type == '+')
        argument = one_argument(argument,word);

    if (arg1[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  flag mob  <name> <field> <flags>\n\r",ch);
        send_to_char("  flag char <name> <field> <flags>\n\r",ch);
        send_to_char("  mob  flags: act,aff,off,imm,res,vuln,form,part\n\r",ch);
        send_to_char("  char flags: plr,comm,aff,imm,res,vuln,\n\r",ch);
        send_to_char("  +: add flag, -: remove flag, = set equal to\n\r",ch);
        send_to_char("  otherwise flag toggles the flags listed.\n\r",ch);
        return;
    }

    if (arg2[0] == '\0')
    {
        send_to_char("What do you wish to set flags on?\n\r",ch);
        return;
    }

    if (arg3[0] == '\0')
    {
        send_to_char("You need to specify a flag to set.\n\r",ch);
        return;
    }

    if (argument[0] == '\0' && type != '=')
    {
        send_to_char("Which flags do you wish to change?\n\r",ch);
        return;
    }

    if (!str_prefix(arg1,"mob") || !str_prefix(arg1,"char"))
    {
        victim = get_char_world(ch,arg2);
        if (victim == NULL)
        {
            send_to_char("You can't find them.\n\r",ch);
            return;
        }

        /* select a flag to set */
        if (!str_prefix(arg3,"act"))
        {
            if (!IS_NPC(victim))
            {
                send_to_char("Use plr for PCs.\n\r",ch);
                return;
            }

            flag = &victim->act;
            flag_table = act_flags;
        }

        else if (!str_prefix(arg3,"plr"))
        {
            if (IS_NPC(victim))
            {
                send_to_char("Use act for NPCs.\n\r",ch);
                return;
            }

            flag = &victim->act;
            flag_table = plr_flags;
        }

        else if (!str_prefix(arg3,"aff"))
        {
            flag = &victim->affect_field;
            flag_table = affect_flags;
        }

        else if (!str_prefix(arg3,"immunity"))
        {
            flag = &victim->imm_flags;
            flag_table = imm_flags;
        }

        else if (!str_prefix(arg3,"resist"))
        {
            flag = &victim->res_flags;
            flag_table = imm_flags;
        }

        else if (!str_prefix(arg3,"vuln"))
        {
            flag = &victim->vuln_flags;
            flag_table = imm_flags;
        }

        else if (!str_prefix(arg3,"form"))
        {
            if (!IS_NPC(victim))
            {
                send_to_char("Form can't be set on PCs.\n\r",ch);
                return;
            }

            flag = &victim->form;
            flag_table = form_flags;
        }

        else if (!str_prefix(arg3,"parts"))
        {
            if (!IS_NPC(victim))
            {
                send_to_char("Parts can't be set on PCs.\n\r",ch);
                return;
            }

            flag = &victim->parts;
            flag_table = part_flags;
        }

        else if (!str_prefix(arg3,"comm"))
        {
            if (IS_NPC(victim))
            {
                send_to_char("Comm can't be set on NPCs.\n\r",ch);
                return;
            }

            flag = &victim->comm;
            flag_table = comm_flags;
        }

        else 
        {
            send_to_char("That's not an acceptable flag.\n\r",ch);
            return;
        }

        flag_copy( old, *flag );
        victim->zone = NULL;

        if (type == '=')
            flag_clear( new );
        else
            flag_copy( new, old );

        flag_clear( marked );

        /* mark the words */
        for (; ;)
        {
            argument = one_argument(argument,word);

            if (word[0] == '\0')
                break;

            pos = flag_lookup(word,flag_table);

            if (pos == NO_FLAG)
            {
                send_to_char("That flag doesn't exist!\n\r",ch);
                return;
            }
            else
                SET_BIT(marked,pos);
        }

        for (pos = 0; flag_table[pos].name != NULL; pos++)
        {
            /*
               if (!flag_table[pos].settable && IS_SET(old,flag_table[pos].bit))
               {
               SET_BIT(new,flag_table[pos].bit);
               continue;
               }
             */
            bit = flag_table[pos].bit;
            old_set = IS_SET( old, bit );

            if (IS_SET(marked, bit))
            {
                switch(type)
                {
                    case '=':
                    case '+':
                        SET_BIT(new, bit);
                        break;
                    case '-':
                        REMOVE_BIT(new, bit);
                        break;
                    default:
                        if (IS_SET(new, bit))
                            REMOVE_BIT(new, bit);
                        else
                            SET_BIT(new, bit);
                }
            }

            /* correct if not settable & give failure message */
            new_set = IS_SET( new, bit );
            if ( !flag_table[pos].settable && new_set != old_set )
            {
                if ( old_set )
                    SET_BIT( new, bit );
                else
                    REMOVE_BIT( new, bit );
                sprintf( buf, "The %s flag isn't settable.\n\r", flag_table[pos].name );
                send_to_char( buf, ch );
            }

        }
        flag_copy( *flag, new );
        return;
    }
}

void print_pkill_grades( CHAR_DATA *ch )
{
    int i;
    char buf[MSL];

    send_to_char("{D         Min     Earned     Death Loss{x\n\r", ch );
    send_to_char("{DGrade    Pts    for kill     pk    wf{x\n\r", ch );

    for( i=1; pkgrade_table[i].pkpoints > 0; i++ )
    {
        sprintf( buf, "  %s    %5d    %5d     %5d  %5d\n\r",
                pkgrade_table[i].grade, pkgrade_table[i].pkpoints,
                pkgrade_table[i].earned, pkgrade_table[i].lost,
                pkgrade_table[i].lost_in_warfare );
        send_to_char( buf, ch );
    }
    sprintf( buf, "  %s    %5d    %5d     %5d  %5d\n\r",
            pkgrade_table[i].grade, pkgrade_table[i].pkpoints,
            pkgrade_table[i].earned, pkgrade_table[i].lost,
            pkgrade_table[i].lost_in_warfare );
    send_to_char( buf, ch );
}

void try_set_hardcore(CHAR_DATA *ch)
{
    if ( !IS_SET(ch->act, PLR_PERM_PKILL) )
    {
        send_to_char( "You must turn normal pkill on first!\n\r", ch );
        return;
    }

    if ( IS_SET(ch->act, PLR_HARDCORE) )
    {
        send_to_char( "You are already a hardcore pkiller.\n\r", ch );
        return;
    }

    send_to_char("Ok. Hope you know what you're up to.\n\r", ch );
    send_to_char("Type 'pkill confirm <password>' to confirm your intentions.\n\r",ch);
    send_to_char("Typing 'pkill' with no argument or with any argument other than 'confirm'\n\r", ch);
    send_to_char("will abort this action.  Please read 'HELP PKILL' before you decide.\n\r", ch);
    ch->pcdata->confirm_pkill = TRUE;
    wiznet("$N is about to turn on $S hardcore flag.",ch, NULL, WIZ_FLAGS, 0, 0);

}

void pkill_confirm(CHAR_DATA *ch, const char *arg)
{
    if (ch->pcdata->confirm_pkill)
    {
        /* extra password check to prevent pkill by trigger-abuse */
        if ( !check_password(arg, ch->pcdata->pwd) )
        {
            send_to_char( "Wrong password, try again.\n\r", ch );
            return; 
        }

        if ( !IS_SET(ch->act, PLR_PERM_PKILL) )
        {
            SET_BIT(ch->act, PLR_PERM_PKILL);
            reset_pkill_expire(ch);
            sprintf(log_buf, "%s has declared %sself to be a pkiller!",
                    ch->name, (ch->sex == SEX_FEMALE) ? "her" : "him");
            info_message(ch, log_buf, TRUE);
            log_string( log_buf );
            wiznet("$N has turned on $S pkill flag.", ch, NULL, WIZ_FLAGS, 0, 0);
        }
        else
        {
            SET_BIT(ch->act, PLR_HARDCORE);
            reset_pkill_expire(ch);
            sprintf(log_buf, "%s has declared %sself to be a hardcore pkiller!",
                    ch->name, (ch->sex == SEX_FEMALE) ? "her" : "him");
            info_message(ch, log_buf, TRUE);
            log_string( log_buf );
            wiznet("$N has turned on $S hardcore flag.", ch, NULL, WIZ_FLAGS, 0, 0);
        }
        /* Pulled from flags.c and moved over here. Stop people from turning on
           pkill just to get a cheap kill. - Astark 12-23-12 */
        if ( ch->pcdata != NULL )
            ch->pcdata->pkill_timer = -10 * PULSE_VIOLENCE;

        update_bounty(ch);

    }
    else
        send_to_char("Pkill activation was aborted.\n\r",ch);

    ch->pcdata->confirm_pkill = FALSE;
}	

void reset_pkill_expire( CHAR_DATA *ch)
{
    if ( IS_NPC(ch) )
        return;

    struct tm *timeinfo;
    timeinfo=localtime( &current_time);
    timeinfo->tm_mday += PK_EXPIRE_DAYS;

    ch->pcdata->pkill_expire=mktime(timeinfo);
}

void try_set_pkill(CHAR_DATA *ch)
{
    if ( IS_SET(ch->act, PLR_PERM_PKILL) )
    {
        send_to_char("You are already a pkiller.  Hope that's what you wanted.\n\r",ch);
        send_to_char("Typing 'pkill hardcore' followed by 'pkill confirm' will turn you\n\r", ch);
        send_to_char("into a hardcore pkiller.'\n\r", ch);
        ch->pcdata->confirm_pkill = FALSE;
        return;
    }
    else
    {
        /*send_to_char("WARNING: This command is permanent -- you can NOT EVER TURN PKILL BACK OFF!\n\r\n\r",ch);*/
        send_to_char("Type 'pkill confirm <password>' to confirm your intentions.\n\r",ch);
        send_to_char("Typing 'pkill' with no argument or with any argument other than 'confirm'\n\r", ch);
        send_to_char("will abort this action.  Please read 'HELP PKILL' before you decide.\n\r", ch);
        ch->pcdata->confirm_pkill = TRUE;
        wiznet("$N is about to turn on $S pkill flag.",ch, NULL, WIZ_FLAGS, 0, 0);
    }
}

void try_remove_pkill( CHAR_DATA *ch)
{
    if ( !IS_SET(ch->act, PLR_PERM_PKILL) )
    {
        send_to_char("You are not a player killer!\n\r", ch);
        return;
    }
    if (ch->class == class_lookup("assassin") )
    {
        send_to_char("Assassins cannot remove pkill status.\n\r", ch );
        return;
    }

    if ( ! IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
    {
        send_to_char("You must do this in a safe room!\n\r", ch);
        return;
    }

    if ( IS_SET(ch->act, PLR_KILLER) )
    {
        send_to_char( "You can't do this with a KILLER flag!\n\r", ch);
        return;
    }

    if ( IS_SET(ch->act, PLR_THIEF) )
    {
        send_to_char( "You can't do this with a THIEF flag!\n\r", ch);
        return;
    }

    if ( current_time < ch->pcdata->pkill_expire )
    {
        print_pkill_expire( ch );
        return;
    }

    REMOVE_BIT( ch->act, PLR_PERM_PKILL );
    REMOVE_BIT( ch->act, PLR_HARDCORE );

    send_to_char("You are no longer a player killer.\n\r", ch );

}

void print_pkill_expire( CHAR_DATA *ch)
{
    if (ch->class == class_lookup("assassin") )
    {
        send_to_char("Assassins cannot remove pkill status.\n\r", ch );
        return;
    }

    if (! IS_SET( ch->act, PLR_PERM_PKILL ) )
    {
        send_to_char("You are not a player killer.\n\r",ch);
        return;
    }

    char ts_buf[80];
    strftime(ts_buf,sizeof(ts_buf),"%x %X", localtime(&(ch->pcdata->pkill_expire)));

    printf_to_char( ch, "You can remove pkill status after %s.\n\r", 
            ts_buf );

}

/* Pkill code by Rimbol, 7/20/97 */
DEF_DO_FUN(do_pkill)
{
    char arg1[MIL], arg2[MIL];

    if (IS_NPC(ch))
        return;

    argument = one_argument_keep_case( argument, arg1 );
    argument = one_argument_keep_case( argument, arg2 );

    if( arg1[0] == '\0'||  !str_cmp(arg1, "grades") )
    {
        print_pkill_grades(ch);
        return;
    }

    if (ch->level < 10 && ch->pcdata->remorts == 0)
    {
        send_to_char("You must reach level 10 before you may alter your pkill status.\n\r",ch);
        return;
    }

    if ( !strcmp( arg1, "time") )
    {
        print_pkill_expire(ch);
        return;
    }

    if ( !strcmp( arg1, "off") )
    {
        try_remove_pkill(ch);
        return;
    }

    if ( !strcmp( arg1, "on" ) )
    {
        try_set_pkill(ch);
        return;
    }

    if ( !strcmp( arg1, "hardcore") )
    {
        try_set_hardcore(ch);
        return;
    }

    if ( !strcmp( arg1, "confirm") )
    {
        pkill_confirm(ch, arg2);
        return;
    }

    /* nothing caught... bad syntax */
    send_to_char( "Usage: pkill <grade|on|off|hardcore|confirm|time>\n\r", ch );

}

/* roleplay flag by Bobble */
DEF_DO_FUN(do_roleplay)
{
    char arg1[MIL], arg2[MIL];

    if (IS_NPC(ch))
        return;

    if ( IS_SET(ch->act, PLR_RP) )
    {
        send_to_char( "You are already a roleplayer.\n\r", ch );
        return;
    }

    if (ch->level < 10 && ch->pcdata->remorts == 0)
    {
        send_to_char( "You must reach level 10 first.\n\r", ch);
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || strcmp(arg1, "on") )
    {
        send_to_char( "Type 'roleplay on' to become a roleplayer.\n\r", ch );
        send_to_char( "Please read 'help roleplay' first.\n\r", ch );
        return;
    }

    if ( strcmp(arg2, "confirm") )
    {
        send_to_char( "This will PERMANENTLY mark you as a roleplayer!\n\r", ch );
        send_to_char( "Please read 'help roleplay' first!\n\r", ch );
        send_to_char( "Type 'roleplay on confirm' to confirm.\n\r", ch );
        return;
    }

    SET_BIT( ch->act, PLR_RP );
    sprintf(log_buf, "%s has declared %sself to be a permanent roleplayer!",
            ch->name, (ch->sex == SEX_FEMALE) ? "her" : "him");
    info_message(ch, log_buf, TRUE);
    wiznet("$N has turned on $S roleplay flag.", ch, NULL, WIZ_FLAGS, 0, 0);
    update_bounty(ch);
    send_to_char( "Ok, you're a roleplayer now. Have fun. :)\n\r", ch );
}
