/* The following code is based on ILAB OLC by Jason Dinkel */
/* Mobprogram code by Lordrom for Nevermore Mud */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lua_scripting.h"

#define RPEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

const struct olc_cmd_type rpedit_table[] =
{
/*	{	command		function	      }, */
   
   {  "commands", show_commands  },
   {  "create",   rpedit_create  },
   {  "code",     rpedit_code    },
   {  "show",     rpedit_show    },
   {  "security", rpedit_security},
   {  "?",        show_help      },
  
   {  NULL,       0              }
};

void rpedit( CHAR_DATA *ch, char *argument)
{
    PROG_CODE *pRcode;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;
    AREA_DATA *ad;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument( argument, command);

    EDIT_RPCODE(ch, pRcode);
    if (!pRcode)
    {
        bugf("rpedit called by %s with wrong edit mode: %d.",
                ch->name, ch->desc->editor );
        return;
    }

    if (pRcode)
    {
       ad = get_vnum_area( pRcode->vnum );
       
       if ( ad == NULL ) /* ??? */
       {
          edit_done(ch);
          return;
       }
       
       if ( !IS_BUILDER(ch, ad) )
       {
          send_to_char("RPEdit: Insufficient security to modify code.\n\r", ch);
          edit_done(ch);
          return;
       }
       
    }

    if (command[0] == '\0')
    {
        rpedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done") )
    {
        edit_done(ch);
        return;
    }

    for (cmd = 0; rpedit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix(command, rpedit_table[cmd].name) )
        {
           if ((*rpedit_table[cmd].olc_fun) (ch, argument) && pRcode)
              if ((ad = get_vnum_area(pRcode->vnum)) != NULL)
                 SET_BIT(ad->area_flags, AREA_CHANGED);
              return;
        }
    }

    interpret(ch, arg);

    return;
}

void do_rprun( CHAR_DATA *ch, char *argument)
{
    if ( IS_NPC(ch) )
        return;

    ROOM_INDEX_DATA *room;
    int vnum=0;
    char arg[MSL];
    char arg2[MSL];
    PROG_CODE *pRcode;
    bool result=FALSE;

    if ( argument[0]=='\0' )
    {
        ptc(ch, "rprun [vnum]\n\r");
        return;
    }

    argument=one_argument( argument, arg );

    if (!is_number(arg))
    {
        ptc( ch, "Bad argument: %s. Should be a number.\n\r", arg);
        return;
    }
    
    vnum=atoi(arg);

    if ( ( pRcode=get_rprog_index(vnum) ) == NULL )
    {
        ptc( ch, "Rprog %d doesn't exist.\n\r", vnum );
        return;
    }

    if ( ch->in_room )
    {
        room=ch->in_room;
    }
    else
    {
        ptc( ch, "Couldn't find area!\n\r");
        if ( !ch->in_room )
        {
            bugf("do_rprun: %s has no in_room", ch->name);
        }
        return;
    }


    ptc( ch, "Running rprog %d in room %d with %s as ch1\n\r",
            vnum,
            room->vnum,
            ch->name);

    result=lua_room_program( NULL, vnum, pRcode->code, room,
           ch, NULL, NULL, NULL, NULL,
           RTRIG_CALL, pRcode->security );

    ptc( ch, "Rprog completed. Result: %s\n\r", result ? "TRUE" : "FALSE" );

}

void do_rpedit(CHAR_DATA *ch, char *argument)
{
    PROG_CODE *pRcode;
    char command[MAX_INPUT_LENGTH];

    argument = one_argument(argument, command);

    if( is_number(command) )
    {
       int vnum = atoi(command);
       AREA_DATA *ad;
       
       if ( (pRcode = get_rprog_index(vnum)) == NULL )
       {
          send_to_char("RPEdit : That vnum does not exist.\n\r",ch);
          return;
       }
       
       ad = get_vnum_area(vnum);
       
       if ( ad == NULL )
       {
          send_to_char( "RPEdit : Vnum not assigned to this area.\n\r", ch );
          return;
       }
       
       if ( !IS_BUILDER(ch, ad) )
       {
          send_to_char("RPEdit : Insufficient security to edit area.\n\r", ch );
          return;
       }
       
       if ( ch->in_room->area != ad )
       {
	   send_to_char( "RPEdit: Warning: prog lies outside current area.\n\r", ch );
       }

       clone_warning( ch, ad );
       ch->desc->pEdit  = (void *)pRcode;
       ch->desc->editor	= ED_RPCODE;
       
       return;
    }

    if ( !str_cmp(command, "create") )
    {
       if (argument[0] == '\0')
       {
          send_to_char( "Syntax : rpedit create [vnum]\n\r", ch );
          return;
       }
       
       rpedit_create(ch, argument);
       return;
    }

    send_to_char( "Syntax : rpedit [vnum]\n\r", ch );
    send_to_char( "         rpedit create [vnum]\n\r", ch );

    return;
}

RPEDIT (rpedit_create)
{
    PROG_CODE *pRcode;
    int value = atoi(argument);
    AREA_DATA *ad;

    if (IS_NULLSTR(argument) || value < 1)
    {
        send_to_char("Syntax: rpedit create [vnum]\n\r",ch);
        return FALSE;
    }

    ad = get_vnum_area(value);

    if ( ad == NULL )
    {
       send_to_char( "RPEdit : Vnum not assigned to this area.\n\r", ch );
       return FALSE;
    }

    if ( !IS_BUILDER(ch, ad) )
    {
       send_to_char("RPEdit : Insufficient security to create AreaProgs.\n\r", ch);
       return FALSE;
    }

    if ( ch->in_room->area != ad )
    {
	send_to_char( "RPEdit: prog lies outside current area.\n\r", ch );
	return FALSE;
    }

    if ( get_rprog_index(value) )
    {
       send_to_char("RPEdit: prog vnum already exists.\n\r",ch);
       return FALSE;
    }

    clone_warning( ch, ad );

    pRcode			= new_rpcode();
    pRcode->vnum		= value;
    pRcode->security    = ch->pcdata->security;
    pRcode->next		= rprog_list;
    rprog_list			= pRcode;
    ch->desc->pEdit		= (void *)pRcode;
    ch->desc->editor		= ED_RPCODE;

    send_to_char("ROOMprog Code Created.\n\r",ch);

    return TRUE;
}

RPEDIT(rpedit_show)
{
    PROG_CODE *pRcode;
    EDIT_RPCODE(ch,pRcode);

    ptc( ch,
           "Vnum:       [%d]\n\r"
           "Security:   %d\n\r"
           "Code:\n\r",
           pRcode->vnum,
           pRcode->security);

    dump_prog( ch, pRcode->code, TRUE);
    
    return FALSE;
}

RPEDIT(rpedit_security)
{
    PROG_CODE *pRcode;
    EDIT_RPCODE(ch, pRcode);
    int newsec;

    if ( argument[0] == '\0' )
    {
        newsec=ch->pcdata->security;
    }
    else
    {
        if (is_number(argument))
        {
            newsec=atoi(argument);
        }
        else
        {
            ptc(ch, "Bad argument: . Must be a number.\n\r", argument);
            return;
        }
    }

    if (newsec == pRcode->security)
    {
        ptc(ch, "Security is already at %d.\n\r", newsec );
        return;
    }
    else if (newsec > ch->pcdata->security )
    {
        ptc(ch, "Your security %d doesn't allow you to set security %d.\n\r",
                ch->pcdata->security, newsec);
        return;
    }

    pRcode->security=newsec;
    ptc(ch, "Security for %d updated to %d.\n\r",
            pRcode->vnum, pRcode->security);

}

void fix_rprog_rooms( CHAR_DATA *ch, PROG_CODE *pRcode )
{
    PROG_LIST *rpl;
    int hash;
    char buf[MSL];
    ROOM_INDEX_DATA *room;

    check_rprog( g_mud_LS, pRcode->vnum, pRcode->code);
    ptc(ch, "Fixed lua script for %d.\n\r", pRcode->vnum);

}


RPEDIT(rpedit_code)
{
    PROG_CODE *pRcode;
    EDIT_RPCODE(ch, pRcode);

    if (argument[0] =='\0')
    {
       string_append(ch, &pRcode->code);
       return TRUE;
    }

    send_to_char("Syntax: code\n\r",ch);
    return FALSE;
}
