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
#include "lua_main.h"

#define OPEDIT( fun ) bool fun( CHAR_DATA *ch, const char *argument )

const struct olc_cmd_type opedit_table[] =
{
/*	{	command		function	      }, */
   
   {  "commands", show_commands  },
   {  "create",   opedit_create  },
   {  "code",     opedit_code    },
   {  "show",     opedit_show    },
   {  "security", opedit_security},
   {  "?",        show_help      },
   
   {  NULL,       0              }
};

void opedit( CHAR_DATA *ch, const char *argument )
{
    PROG_CODE *pOcode;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;
    AREA_DATA *ad;

    smash_tilde_cpy(arg, argument);
    argument = one_argument(arg, command);

    EDIT_OPCODE(ch, pOcode);
    if (!pOcode)
    {
        bugf("mpedit called by %s with wrong edit mode: %d.",
                ch->name, ch->desc->editor );
        return;
    }

    if (pOcode)
    {
       ad = get_vnum_area( pOcode->vnum );
       
       if ( ad == NULL ) /* ??? */
       {
          edit_done(ch);
          return;
       }
       
       if ( !IS_BUILDER(ch, ad) )
       {
          send_to_char("OPEdit: Insufficient security to modify code.\n\r", ch);
          edit_done(ch);
          return;
       }
       
    }

    if (command[0] == '\0')
    {
        opedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done") )
    {
        edit_done(ch);
        return;
    }

    for (cmd = 0; opedit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix(command, opedit_table[cmd].name) )
        {
            if ( strlen(opedit_table[cmd].name) >= 3
                    && strlen(command) < 3 )
                break;

            if ((*opedit_table[cmd].olc_fun) (ch, argument) && pOcode)
                if ((ad = get_vnum_area(pOcode->vnum)) != NULL)
                    SET_BIT(ad->area_flags, AREA_CHANGED);
            return;
        }
    }

    interpret(ch, arg);

    return;
}

DEF_DO_FUN(do_oprun)
{
    if (IS_NPC(ch))
        return;

    OBJ_DATA *obj;
    int vnum=0;
    char arg[MSL];
    char arg2[MSL];
    PROG_CODE *pOcode;
    bool result;

    if ( argument[0]=='\0' )
    {
        ptc(ch, "oprun [objname] [vnum]\n\r");
        return;
    }

    argument=one_argument( argument, arg );

    if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
    {
        ptc( ch, "Couldn't find %s in room or inventory.\n\r", arg );
        return;
    }

    argument=one_argument( argument, arg2 );

    if (!is_number(arg2))
    {
        ptc( ch, "Bad argument #2: %s. Should be a number.\n\r", arg2);
        return;
    }

    vnum=atoi(arg2);

    if ( ( pOcode=get_oprog_index(vnum) ) == NULL )
    {
        ptc( ch, "Oprog %d doesn't exist.\n\r", vnum );
        return;
    }

    ptc( ch, "Running oprog %d on %s(%d)",
            vnum,
            obj->name,
            obj->pIndexData->vnum);
    if ( obj->in_room )
        ptc( ch, " in room %s(%d)", obj->in_room->name, obj->in_room->vnum );
    else if ( obj->carried_by )
        ptc( ch, " in %s's inventory", obj->carried_by->name );
    else if ( obj->in_obj )
        ptc( ch, " in %s(%d)", obj->in_obj, obj->in_obj->pIndexData->vnum );

    ptc( ch, " with %s as ch1\n\r", ch->name);

   result=lua_obj_program( g_mud_LS, NULL, vnum, pOcode->code, obj, NULL, ch, NULL, OTRIG_CALL, pOcode->security);

   ptc( ch, "Oprog completed. Result is: %s\n\r", result ? "TRUE" : "FALSE" );

}

    

DEF_DO_FUN(do_opedit)
{
    PROG_CODE *pOcode;
    char command[MAX_INPUT_LENGTH];

    argument = one_argument(argument, command);

    if( is_number(command) )
    {
       int vnum = atoi(command);
       AREA_DATA *ad;
       
       if ( (pOcode = get_oprog_index(vnum)) == NULL )
       {
          send_to_char("OPEdit : That vnum does not exist.\n\r",ch);
          return;
       }
       
       ad = get_vnum_area(vnum);
       
       if ( ad == NULL )
       {
          send_to_char( "OPEdit : Vnum not assigned to this area.\n\r", ch );
          return;
       }
       
       if ( !IS_BUILDER(ch, ad) )
       {
          send_to_char("OPEdit : Insufficient security to edit area.\n\r", ch );
          return;
       }
       
       if ( ch->in_room->area != ad )
       {
	   send_to_char( "OPEdit: Warning: ObjProg lies outside current area.\n\r", ch );
       }

       clone_warning( ch, ad );
       ch->desc->pEdit  = (void *)pOcode;
       ch->desc->editor	= ED_OPCODE;
       
       return;
    }

    if ( !str_cmp(command, "create") )
    {
       if (argument[0] == '\0')
       {
          send_to_char( "Syntax : opedit create [vnum]\n\r", ch );
          return;
       }
       
       opedit_create(ch, argument);
       return;
    }

    if ( !str_cmp(command, "delete") )
    {
        if (argument[0] == '\0')
        {
            send_to_char( "Syntax : opedit delete [vnum]\n\r", ch );
            return;
        }

        opedit_delete(ch, argument);
        return;
    }

    send_to_char( "Syntax : opedit [vnum]\n\r", ch );
    send_to_char( "         opedit create [vnum]\n\r", ch );

    return;
}

OPEDIT (opedit_delete)
{
    PROG_CODE *pOcode;
    char command[MIL];

    argument = one_argument(argument, command);

    if (!is_number(command))
    {
        send_to_char( "Syntax : opedit create [vnum]\n\r", ch );
        return FALSE;
    }

    int vnum=atoi(command);

    if ( (pOcode = get_oprog_index(vnum)) == NULL )
    {
        send_to_char("Oprog does not exist.\n\r", ch );
        return FALSE;
    }

    AREA_DATA *ad = get_vnum_area( vnum );

    if ( ad == NULL )
    {
        send_to_char("Vnum not assigned to an area.\n\r", ch );
        return FALSE;
    }

    if ( !IS_BUILDER(ch,ad) )
    {
        send_to_char( "Insufficient security to delete oprog.\n\r", ch );
        return FALSE;
    }

    OBJ_INDEX_DATA *obj;
    PROG_LIST *lst;
    int ovnum;
    for ( ovnum=0 ; ovnum <= top_vnum_obj ; ovnum++ )
    {
        obj=get_obj_index( ovnum );
        if (!obj)
            continue;

        for ( lst=obj->oprogs ; lst ; lst=lst->next )
        {
            if ( lst->script == pOcode )
            {
                send_to_char( "Can't delete oprog, it is used.\n\r", ch);
                return FALSE;
            }
        }
    }

    if (is_being_edited(pOcode))
    {
        send_to_char( "Can't delete oprog, it is being edited.\n\r", ch );
        return FALSE;
    }

    /* if we got here, we're good to delete */
    PROG_CODE *curr, *last=NULL;
    for ( curr=oprog_list ; curr ; curr=curr->next )
    {
        if ( curr==pOcode )
        {
            if (!last)
            {
                oprog_list=curr->next;
            }
            else
            {
                last->next=curr->next;
            }

            free_opcode( pOcode );
            SET_BIT( ad->area_flags, AREA_CHANGED);
            send_to_char( "Oprog deleted.\n\r", ch );
            return TRUE;
        }
        last=curr;
    }
    return FALSE;
}

OPEDIT (opedit_create)
{
    PROG_CODE *pOcode;
    int value = atoi(argument);
    AREA_DATA *ad;

    if (IS_NULLSTR(argument) || value < 1)
    {
        send_to_char("Syntax: opedit create [vnum]\n\r",ch);
        return FALSE;
    }

    ad = get_vnum_area(value);

    if ( ad == NULL )
    {
       send_to_char( "OPEdit : Vnum not assigned to this area.\n\r", ch );
       return FALSE;
    }

    if ( !IS_BUILDER(ch, ad) )
    {
       send_to_char("OPEdit : Insufficient security to create ObjProgs.\n\r", ch);
       return FALSE;
    }

    if ( ch->in_room->area != ad )
    {
	send_to_char( "OPEdit: ObjProg lies outside current area.\n\r", ch );
	return FALSE;
    }

    if ( get_oprog_index(value) )
    {
       send_to_char("OPEdit: ObjProg vnum already exists.\n\r",ch);
       return FALSE;
    }

    clone_warning( ch, ad );

    pOcode			= new_opcode();
    pOcode->vnum		= value;
    pOcode->security    = ch->pcdata->security;
    pOcode->next		= oprog_list;
    oprog_list			= pOcode;
    ch->desc->pEdit		= (void *)pOcode;
    ch->desc->editor		= ED_OPCODE;

    send_to_char("ObjProgram Code Created.\n\r",ch);

    return TRUE;
}

OPEDIT(opedit_show)
{
    PROG_CODE *pOcode;
    EDIT_OPCODE(ch,pOcode);

    ptc(ch,
           "Vnum:       [%d]\n\r"
           "Security:   %d\n\r"
           "Code:\n\r",
           pOcode->vnum,
           pOcode->security);
    dump_prog( ch, pOcode->code, TRUE);

    return FALSE;
}

OPEDIT(opedit_security)
{
    PROG_CODE *pOcode;
    EDIT_OPCODE(ch, pOcode);
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
            return FALSE;
        }
    }

    if (newsec == pOcode->security)
    {
        ptc(ch, "Security is already at %d.\n\r", newsec );
        return FALSE;
    }
    else if (newsec > ch->pcdata->security )
    {
        ptc(ch, "Your security %d doesn't allow you to set security %d.\n\r",
                ch->pcdata->security, newsec);
        return FALSE;
    }

    pOcode->security=newsec;
    ptc(ch, "Security for %d updated to %d.\n\r",
            pOcode->vnum, pOcode->security);
    return TRUE;
}

void fix_oprog_objs( CHAR_DATA *ch, PROG_CODE *pOcode )
{
    check_oprog( g_mud_LS, pOcode->vnum, pOcode->code);
    ptc(ch, "Fixed lua script for %d.\n\r", pOcode->vnum);

}

/* Procedure to run when OPROG is changed and needs to be updated
   on mobs using it */

OPEDIT(opedit_code)
{
    PROG_CODE *pOcode;
    EDIT_OPCODE(ch, pOcode);

    if (argument[0] =='\0')
    {
       string_append(ch, &pOcode->code);
       return TRUE;
    }

    send_to_char("Syntax: code\n\r",ch);
    return FALSE;
}
