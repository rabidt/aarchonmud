/* The following code is based on ILAB OLC by Jason Dinkel */
/* Mobprogram code by Lordrom for Nevermore Mud */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lua_scripting.h"

#define APEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

const struct olc_cmd_type apedit_table[] =
{
/*	{	command		function	      }, */
   
   {  "commands", show_commands  },
   {  "create",   apedit_create  },
   {  "code",     apedit_code    },
   {  "show",     apedit_show    },
   {  "security", apedit_security},
   {  "?",        show_help      },
   {  NULL,       0              }
};

void apedit( CHAR_DATA *ch, char *argument)
{
    PROG_CODE *pAcode;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;
    AREA_DATA *ad;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument( argument, command);

    EDIT_APCODE(ch, pAcode);
    if (!pAcode)
    {
        bugf("mpedit called by %s with wrong edit mode: %d.",
                ch->name, ch->desc->editor );
        return;
    }

    if (pAcode)
    {
       ad = get_vnum_area( pAcode->vnum );
       
       if ( ad == NULL ) /* ??? */
       {
          edit_done(ch);
          return;
       }
       
       if ( !IS_BUILDER(ch, ad) )
       {
          send_to_char("APEdit: Insufficient security to modify code.\n\r", ch);
          edit_done(ch);
          return;
       }
       
    }

    if (command[0] == '\0')
    {
        apedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done") )
    {
        edit_done(ch);
        return;
    }

    for (cmd = 0; apedit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix(command, apedit_table[cmd].name) )
        {
            if ( strlen(apedit_table[cmd].name) >= 3
                    && strlen(command) < 3 )
                break;

            if ((*apedit_table[cmd].olc_fun) (ch, argument) && pAcode)
                if ((ad = get_vnum_area(pAcode->vnum)) != NULL)
                    SET_BIT(ad->area_flags, AREA_CHANGED);
            return;
        }
    }

    interpret(ch, arg);

    return;
}

void do_aprun( CHAR_DATA *ch, char *argument)
{
    if ( IS_NPC(ch) )
        return;

    AREA_DATA *area;
    int vnum=0;
    char arg[MSL];
    char arg2[MSL];
    PROG_CODE *pAcode;
    bool result=FALSE;

    if ( argument[0]=='\0' )
    {
        ptc(ch, "aprun [vnum]\n\r");
        return;
    }

    argument=one_argument( argument, arg );

    if (!is_number(arg))
    {
        ptc( ch, "Bad argument: %s. Should be a number.\n\r", arg);
        return;
    }
    
    vnum=atoi(arg);

    if ( ( pAcode=get_aprog_index(vnum) ) == NULL )
    {
        ptc( ch, "Aprog %d doesn't exist.\n\r", vnum );
        return;
    }

    if ( ch->in_room && ch->in_room->area )
    {
        area=ch->in_room->area;
    }
    else
    {
        ptc( ch, "Couldn't find area!\n\r");
        if ( !ch->in_room )
        {
            bugf("do_aprun: %s has no in_room", ch->name);
        }
        else if ( !ch->in_room->area )
        {
            bugf("do_aprun: %s(%d) has no area", ch->in_room->name, ch->in_room->vnum);
        }
        return;
    }


    ptc( ch, "Running aprog %d in area %s with %s as ch1\n\r",
            vnum,
            area->name,
            ch->name);

    result=lua_area_program( NULL, vnum, pAcode->code, area, ch, ATRIG_CALL, pAcode->security );

    ptc( ch, "Aprog completed. Result: %s\n\r", result ? "TRUE" : "FALSE" );

}

void do_apedit(CHAR_DATA *ch, char *argument)
{
    PROG_CODE *pAcode;
    char command[MAX_INPUT_LENGTH];

    argument = one_argument(argument, command);

    if( is_number(command) )
    {
       int vnum = atoi(command);
       AREA_DATA *ad;
       
       if ( (pAcode = get_aprog_index(vnum)) == NULL )
       {
          send_to_char("APEdit : That vnum does not exist.\n\r",ch);
          return;
       }
       
       ad = get_vnum_area(vnum);
       
       if ( ad == NULL )
       {
          send_to_char( "APEdit : Vnum not assigned to this area.\n\r", ch );
          return;
       }
       
       if ( !IS_BUILDER(ch, ad) )
       {
          send_to_char("APEdit : Insufficient security to edit area.\n\r", ch );
          return;
       }
       
       if ( ch->in_room->area != ad )
       {
	   send_to_char( "APEdit: Warning: prog lies outside current area.\n\r", ch );
       }

       clone_warning( ch, ad );
       ch->desc->pEdit  = (void *)pAcode;
       ch->desc->editor	= ED_APCODE;
       
       return;
    }

    if ( !str_cmp(command, "create") )
    {
       if (argument[0] == '\0')
       {
          send_to_char( "Syntax : apedit create [vnum]\n\r", ch );
          return;
       }
       
       apedit_create(ch, argument);
       return;
    }

    send_to_char( "Syntax : apedit [vnum]\n\r", ch );
    send_to_char( "         apedit create [vnum]\n\r", ch );

    return;
}

APEDIT (apedit_create)
{
    PROG_CODE *pAcode;
    int value = atoi(argument);
    AREA_DATA *ad;

    if (IS_NULLSTR(argument) || value < 1)
    {
        send_to_char("Syntax: apedit create [vnum]\n\r",ch);
        return FALSE;
    }

    ad = get_vnum_area(value);

    if ( ad == NULL )
    {
       send_to_char( "APEdit : Vnum not assigned to this area.\n\r", ch );
       return FALSE;
    }

    if ( !IS_BUILDER(ch, ad) )
    {
       send_to_char("APEdit : Insufficient security to create AreaProgs.\n\r", ch);
       return FALSE;
    }

    if ( ch->in_room->area != ad )
    {
	send_to_char( "APEdit: prog lies outside current area.\n\r", ch );
	return FALSE;
    }

    if ( get_aprog_index(value) )
    {
       send_to_char("APEdit: prog vnum already exists.\n\r",ch);
       return FALSE;
    }

    clone_warning( ch, ad );

    pAcode			= new_apcode();
    pAcode->vnum		= value;
    pAcode->security    = ch->pcdata->security;
    pAcode->next		= aprog_list;
    aprog_list			= pAcode;
    ch->desc->pEdit		= (void *)pAcode;
    ch->desc->editor		= ED_APCODE;

    send_to_char("AREAprog Code Created.\n\r",ch);

    return TRUE;
}

APEDIT(apedit_show)
{
    PROG_CODE *pAcode;
    EDIT_APCODE(ch,pAcode);

    ptc(ch,
           "Vnum:       [%d]\n\r"
           "Security:   %d\n\r"
           "Code:\n\r",
           pAcode->vnum,
           pAcode->security);
    dump_prog( ch, pAcode->code, TRUE);

    return FALSE;
}

APEDIT(apedit_security)
{
    PROG_CODE *pAcode;
    EDIT_APCODE(ch, pAcode);
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

    if (newsec == pAcode->security)
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

    pAcode->security=newsec;
    ptc(ch, "Security for %d updated to %d.\n\r",
            pAcode->vnum, pAcode->security);

}

void fix_aprog_areas( CHAR_DATA *ch, PROG_CODE *pAcode )
{
    PROG_LIST *apl;
    int hash;
    char buf[MSL];
    AREA_DATA *area;

    check_aprog( g_mud_LS, pAcode->vnum, pAcode->code);
    ptc(ch, "Fixed lua script for %d.\n\r", pAcode->vnum);

}

/* Procedure to run when APROG is changed and needs to be updated
   on mobs using it */

APEDIT(apedit_code)
{
    PROG_CODE *pAcode;
    EDIT_APCODE(ch, pAcode);

    if (argument[0] =='\0')
    {
       string_append(ch, &pAcode->code);
       return TRUE;
    }

    send_to_char("Syntax: code\n\r",ch);
    return FALSE;
}
