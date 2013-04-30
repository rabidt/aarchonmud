/* The following code is based on ILAB OLC by Jason Dinkel */
/* Mobprogram code by Lordrom for Nevermore Mud */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "mob_cmds.h"

#define MPEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

const struct olc_cmd_type mpedit_table[] =
{
/*	{	command		function	      }, */
   
   {  "commands", show_commands  },
   {  "create",   mpedit_create  },
   {  "code",     mpedit_code    },
   {  "show",     mpedit_show    },
   {  "list",     mpedit_list    },
   {  "if",       mpedit_if      },
   {  "mob",      mpedit_mob     },
   {  "?",        show_help      },
   {  "lua",      mpedit_lua     },
   
   {  NULL,       0              }
};

void mpedit( CHAR_DATA *ch, char *argument)
{
    MPROG_CODE *pMcode;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;
    AREA_DATA *ad;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument( argument, command);

    EDIT_MPCODE(ch, pMcode);

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
           if ((*mpedit_table[cmd].olc_fun) (ch, argument) && pMcode)
              if ((ad = get_vnum_area(pMcode->vnum)) != NULL)
                 SET_BIT(ad->area_flags, AREA_CHANGED);
              return;
        }
    }

    interpret(ch, arg);

    return;
}

void do_mpedit(CHAR_DATA *ch, char *argument)
{
    MPROG_CODE *pMcode;
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

    send_to_char( "Syntax : mpedit [vnum]\n\r", ch );
    send_to_char( "         mpedit create [vnum]\n\r", ch );

    return;
}

MPEDIT (mpedit_create)
{
    MPROG_CODE *pMcode;
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
    pMcode->next		= mprog_list;
    mprog_list			= pMcode;
    ch->desc->pEdit		= (void *)pMcode;
    ch->desc->editor		= ED_MPCODE;

    send_to_char("MobProgram Code Created.\n\r",ch);

    return TRUE;
}

MPEDIT(mpedit_show)
{
    MPROG_CODE *pMcode;
    char buf[MAX_STRING_LENGTH];

    EDIT_MPCODE(ch,pMcode);

    sprintf(buf,
           "Vnum:       [%d]\n\r"
           "Lua:        %s\n\r"
           "Code:\n\r%s\n\r",
           pMcode->vnum,
           pMcode->is_lua ? "True" : "False",
           pMcode->code);
    send_to_char(buf, ch);

    return FALSE;
}

MPEDIT(mpedit_lua)
{
    MPROG_CODE *pMcode;
    MPROG_LIST *mpl;
    EDIT_MPCODE(ch, pMcode);
    MOB_INDEX_DATA *mob;
    int hash;
    char buf[MSL];

    pMcode->is_lua = !pMcode->is_lua;
    ptc( ch, "LUA set to %s\n\r", pMcode->is_lua ? "TRUE" : "FALSE" );

            for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
               for ( mob = mob_index_hash[hash]; mob; mob = mob->next )
                  for ( mpl = mob->mprogs; mpl; mpl = mpl->next )
                     if ( mpl->vnum == pMcode->vnum )
                     {
                        sprintf( buf, "Fixing mob %d.\n\r", mob->vnum );
                        send_to_char( buf, ch );
                        mpl->code = pMcode->code;
                        mpl->is_lua = pMcode->is_lua;
                     }
}

MPEDIT(mpedit_code)
{
    MPROG_CODE *pMcode;
    EDIT_MPCODE(ch, pMcode);

    if (argument[0] =='\0')
    {
       string_append(ch, &pMcode->code);
       return TRUE;
    }

    send_to_char("Syntax: code\n\r",ch);
    return FALSE;
}

MPEDIT( mpedit_list )
{
   int count = 1;
   MPROG_CODE *mprg;
   char buf[MAX_STRING_LENGTH];
   BUFFER *buffer;
   bool fAll = !str_cmp(argument, "all");
   char blah;
   AREA_DATA *ad;
   
   buffer = new_buf();
   
   for (mprg = mprog_list; mprg !=NULL; mprg = mprg->next)
      if ( fAll || IS_BETWEEN(ch->in_room->area->min_vnum, mprg->vnum, ch->in_room->area->max_vnum) )
      {
         ad = get_vnum_area(mprg->vnum);
         
         if ( ad == NULL )
            blah = '?';
         else
            if ( IS_BUILDER(ch, ad) )
               blah = '*';
            else
               blah = ' ';
            
            sprintf(buf, "[%3d] (%c) %5d\n\r", count, blah, mprg->vnum );
            add_buf(buffer, buf);
            
            count++;
      }
      
      if ( count == 1 )
      {
         if ( fAll )
            add_buf( buffer, "No existen MobPrograms.\n\r" );
         else
            add_buf( buffer, "No existen MobPrograms en esta area.\n\r" );
         
      }
      
      page_to_char(buf_string(buffer), ch);
      free_buf(buffer);
      
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
