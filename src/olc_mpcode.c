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
#include "mob_cmds.h"
#include "lua_scripting.h"
#include "lua_main.h"

#define MPEDIT( fun ) bool fun( CHAR_DATA *ch, const char *argument )

const struct olc_cmd_type mpedit_table[] =
{
/*	{	command		function	      }, */
   
   {  "commands", show_commands  },
   {  "create",   mpedit_create  },
   {  "code",     mpedit_code    },
   {  "show",     mpedit_show    },
   {  "if",       mpedit_if      },
   {  "mob",      mpedit_mob     },
   {  "?",        show_help      },
   {  "lua",      mpedit_lua     },
   {  "security", mpedit_security },
   
   {  NULL,       0              }
};

void mpedit( CHAR_DATA *ch, const char *argument)
{
    PROG_CODE *pMcode;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;
    AREA_DATA *ad;

    smash_tilde_cpy(arg, argument);
    argument = one_argument(arg, command);

    EDIT_MPCODE(ch, pMcode);
    if (!pMcode)
    {
        bugf("mpedit called by %s with wrong edit mode: %d.",
                ch->name, ch->desc->editor );
        return;
    }


    if (pMcode)
    {
       ad = get_vnum_area( pMcode->vnum );
       
       if ( ad == NULL ) /* ??? */
       {
          edit_done(ch);
          return;
       }
       
       if ( !IS_BUILDER(ch, ad) )
       {
          send_to_char("MPEdit: Insufficient security to modify code.\n\r", ch);
          edit_done(ch);
          return;
       }
       
    }

    if (command[0] == '\0')
    {
        mpedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done") )
    {
        edit_done(ch);
        return;
    }

    for (cmd = 0; mpedit_table[cmd].name != NULL; cmd++)
    {

        if (!str_prefix(command, mpedit_table[cmd].name) )
        {
            if ( strlen(mpedit_table[cmd].name) >= 3
                    && strlen(command) < 3 )
                break;

            if ((*mpedit_table[cmd].olc_fun) (ch, argument) && pMcode)
                if ((ad = get_vnum_area(pMcode->vnum)) != NULL)
                    SET_BIT(ad->area_flags, AREA_CHANGED);
            return;
        }
    }

    interpret(ch, arg);

    return;
}

DEF_DO_FUN(do_mprun)
{

    if (IS_NPC(ch))
        return;

    CHAR_DATA *mob;
    CHAR_DATA *ch1=NULL;
    int vnum=0;
    char arg[MSL];
    char arg2[MSL];
    PROG_CODE *pMcode;

    if ( argument[0]=='\0' )
    {
        ptc(ch, "mprun [vnum] (runs on self)\n\r");
        ptc(ch, "mprun [mobname] [vnum] (runs on mob, same room only)\n\r");
        return;
    }

    argument=one_argument( argument, arg );
    if (is_number(arg))
    {
        /* 1st arg is vnum so just grab vnum and run prog */
        vnum=atoi(arg);
        mob=ch;
    }
    else
    {
        /* look in room only to prevent any keyword mishaps */
        if ( ( mob = get_char_room( ch, arg ) ) == NULL )
        {
            ptc( ch, "Couldn't find %s in the room.\n\r", arg );
            return;
        }
        ch1=ch;
        
        argument=one_argument( argument, arg2 );

        if (!is_number(arg2))
        {
            ptc( ch, "Bad argument #2: %s. Should be a number.\n\r", arg2);
            return;
        }
        
        vnum=atoi(arg2);
    }

    /* we have args, run the prog */
    if ( ( pMcode=get_mprog_index(vnum) ) == NULL )
    {
        ptc( ch, "Mprog %d doesn't exist.\n\r", vnum );
        return;
    }
    
    if ( !pMcode->is_lua )
    {
        ptc( ch, "mprun only supports lua mprogs.\n\r" );
        return;
    }

    ptc( ch, "Running mprog %d on %s(%d) in room %s(%d)",
            vnum,
            mob->name,
            IS_NPC(mob) ? mob->pIndexData->vnum : 0,
            mob->in_room ? mob->in_room->name : "NO ROOM",
            mob->in_room ? mob->in_room->vnum : 0);

    if (ch1)
        ptc( ch, " with %s as ch1", ch1->name);
    ptc(ch, "\n\r");

    lua_mob_program( NULL, vnum, pMcode->code, mob, (mob==ch)?NULL:ch, NULL, 0, NULL, 0, TRIG_CALL, pMcode->security );

    ptc( ch, "Mprog completed.\n\r");


}

DEF_DO_FUN(do_mpedit)
{
    PROG_CODE *pMcode;
    char command[MAX_INPUT_LENGTH];

    argument = one_argument(argument, command);

    if( is_number(command) )
    {
       int vnum = atoi(command);
       AREA_DATA *ad;
       
       if ( (pMcode = get_mprog_index(vnum)) == NULL )
       {
          send_to_char("MPEdit : That vnum does not exist.\n\r",ch);
          return;
       }
       
       ad = get_vnum_area(vnum);
       
       if ( ad == NULL )
       {
          send_to_char( "MPEdit : Vnum not assigned to this area.\n\r", ch );
          return;
       }
       
       if ( !IS_BUILDER(ch, ad) )
       {
          send_to_char("MPEdit : Insufficient security to edit area.\n\r", ch );
          return;
       }
       
       if ( ch->in_room->area != ad )
       {
	   send_to_char( "MPEdit: Warning: MobProg lies outside current area.\n\r", ch );
       }

       clone_warning( ch, ad );
       ch->desc->pEdit  = (void *)pMcode;
       ch->desc->editor	= ED_MPCODE;
       
       return;
    }

    if ( !str_cmp(command, "create") )
    {
       if (argument[0] == '\0')
       {
          send_to_char( "Syntax : mpedit create [vnum]\n\r", ch );
          return;
       }
       
       mpedit_create(ch, argument);
       return;
    }

    if ( !str_cmp(command, "delete") )
    {
        if (argument[0] == '\0')
        {
            send_to_char( "Syntax : mpedit delete [vnum]\n\r", ch );
            return;
        }

        mpedit_delete(ch, argument);
        return;
    }

    send_to_char( "Syntax : mpedit [vnum]\n\r", ch );
    send_to_char( "         mpedit create [vnum]\n\r", ch );

    return;
}

MPEDIT (mpedit_delete)
{
    PROG_CODE *pMcode;
    char command[MIL];

    argument = one_argument(argument, command);

    if (!is_number(command))
    {
        send_to_char( "Syntax : mpedit create [vnum]\n\r", ch );
        return FALSE;
    }

    int vnum=atoi(command);

    if ( (pMcode = get_mprog_index(vnum)) == NULL )
    {
        send_to_char("Mprog does not exist.\n\r", ch );
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
        send_to_char( "Insufficient security to delete mprog.\n\r", ch );
        return FALSE;
    }
    
    MOB_INDEX_DATA *mob;
    PROG_LIST *lst;
    int mvnum;
    for ( mvnum=0 ; mvnum <= top_vnum_mob ; mvnum++ )
    {
        mob=get_mob_index( mvnum );
        if (!mob)
            continue;

        for ( lst=mob->mprogs ; lst ; lst=lst->next )
        {
            if ( lst->script == pMcode )
            {
                send_to_char( "Can't delete mprog, it is used.\n\r", ch);
                return FALSE;
            }
        }
    }

    if (is_being_edited(pMcode))
    {
        send_to_char( "Can't delete mprog, it is being edited.\n\r", ch);
        return FALSE;
    }

    /* if we got here, we're good to delete */
    PROG_CODE *curr, *last=NULL;
    for ( curr=mprog_list ; curr ; curr=curr->next )
    {
        if ( curr==pMcode )
        {
            if (!last)
            {
                mprog_list=curr->next;
            }
            else
            {
                last->next=curr->next;
            }

            free_mpcode( pMcode );
            SET_BIT( ad->area_flags, AREA_CHANGED);
            send_to_char( "Mprog deleted.\n\r", ch );
            return TRUE;
        }
        last=curr;
    }
    return FALSE;
}



MPEDIT (mpedit_create)
{
    PROG_CODE *pMcode;
    int value = atoi(argument);
    AREA_DATA *ad;

    if (IS_NULLSTR(argument) || value < 1)
    {
        send_to_char("Syntax: mpedit create [vnum]\n\r",ch);
        return FALSE;
    }

    ad = get_vnum_area(value);

    if ( ad == NULL )
    {
       send_to_char( "MPEdit : Vnum not assigned to this area.\n\r", ch );
       return FALSE;
    }

    if ( !IS_BUILDER(ch, ad) )
    {
       send_to_char("MPEdit : Insufficient security to create MobProgs.\n\r", ch);
       return FALSE;
    }

    if ( ch->in_room->area != ad )
    {
	send_to_char( "MPEdit: MobProg lies outside current area.\n\r", ch );
	return FALSE;
    }

    if ( get_mprog_index(value) )
    {
       send_to_char("MPEdit: MobProg vnum already exists.\n\r",ch);
       return FALSE;
    }

    clone_warning( ch, ad );

    pMcode			= new_mpcode();
    pMcode->vnum		= value;
    pMcode->security    = ch->pcdata->security;
    pMcode->next		= mprog_list;
    mprog_list			= pMcode;
    ch->desc->pEdit		= (void *)pMcode;
    ch->desc->editor		= ED_MPCODE;

    send_to_char("MobProgram Code Created.\n\r",ch);

    return TRUE;
}

MPEDIT(mpedit_show)
{
    PROG_CODE *pMcode;
    EDIT_MPCODE(ch,pMcode);

    ptc(ch,
           "Vnum:       [%d]\n\r"
           "Lua:        %s\n\r"
           "Security:   %d\n\r"
           "Code:\n\r",
           pMcode->vnum,
           pMcode->is_lua ? "True" : "False",
           pMcode->security);
    if (pMcode->is_lua)
        dump_prog(ch, pMcode->code, TRUE);
    else
        page_to_char_new( pMcode->code, ch, TRUE);

    return FALSE;
}

void fix_mprog_mobs( CHAR_DATA *ch, PROG_CODE *pMcode )
{
    if ( pMcode->is_lua )
    {
        check_mprog( g_mud_LS, pMcode->vnum, pMcode->code );
        ptc(ch, "Fixed lua script for %d.\n\r", pMcode->vnum );
    }
}

MPEDIT(mpedit_security)
{
    PROG_CODE *pMcode;
    EDIT_MPCODE(ch, pMcode);
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

    if (newsec == pMcode->security)
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
    
    pMcode->security=newsec;
    ptc(ch, "Security for %d updated to %d.\n\r",
            pMcode->vnum, pMcode->security);
    return TRUE;
}


MPEDIT(mpedit_lua)
{
    PROG_CODE *pMcode;
    EDIT_MPCODE(ch, pMcode);
    
    pMcode->is_lua = !pMcode->is_lua;
    ptc( ch, "LUA set to %s\n\r", pMcode->is_lua ? "TRUE" : "FALSE" );
    if ( pMcode->is_lua )
        lua_mprogs++;
    else
        lua_mprogs--;

    fix_mprog_mobs( ch, pMcode);
    return TRUE;
}

/* Procedure to run when MPROG is changed and needs to be updated
   on mobs using it */
MPEDIT(mpedit_code)
{
    PROG_CODE *pMcode;
    EDIT_MPCODE(ch, pMcode);

    if (argument[0] =='\0')
    {
       string_append(ch, &pMcode->code);
       return TRUE;
    }

    send_to_char("Syntax: code\n\r",ch);
    return FALSE;
}

/* define in mob_prog.c and mob_cmd.c */
typedef char* keyword_list[][2];
extern const keyword_list fn_keyword;
/* display valid if-checks */
MPEDIT( mpedit_if )
{
    BUFFER *buffer;
    char buf[MSL];
    int i;

    buffer = new_buf();

    add_buf( buffer, "==================== Valid if-checks ====================\n\r" );
    for( i = 0; fn_keyword[i][0][0] != '\n'; i++ )
    {
	sprintf( buf, "%-14s: %s\n\r", fn_keyword[i][0], fn_keyword[i][1] );
	add_buf( buffer, buf );
    }
    
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return FALSE;
}

/* display valid mob commands */
MPEDIT( mpedit_mob )
{
    BUFFER *buffer;
    char buf[MSL];
    int i;

    buffer = new_buf();

    add_buf( buffer, "==================== Valid mob-commands =================\n\r" );
    for( i = 0; mob_cmd_table[i].name[0] != '\0'; i++ )
    {
	sprintf( buf, "mob %s %s\n\r", mob_cmd_table[i].name, mob_cmd_table[i].help );
	add_buf( buffer, buf );
    }
    
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return FALSE;
}
