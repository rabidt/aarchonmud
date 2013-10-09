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

#define OPEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

const struct olc_cmd_type opedit_table[] =
{
/*	{	command		function	      }, */
   
   {  "commands", show_commands  },
   {  "create",   opedit_create  },
   {  "code",     opedit_code    },
   {  "show",     opedit_show    },
   //{  "list",     opedit_list    },
   //{  "if",       opedit_if      },
   //{  "mob",      opedit_mob     },
   {  "?",        show_help      },
   //{  "lua",      opedit_lua     },
   
   {  NULL,       0              }
};

void opedit( CHAR_DATA *ch, char *argument)
{
    OPROG_CODE *pOcode;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;
    AREA_DATA *ad;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument( argument, command);

    EDIT_OPCODE(ch, pOcode);

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
           if ((*opedit_table[cmd].olc_fun) (ch, argument) && pOcode)
              if ((ad = get_vnum_area(pOcode->vnum)) != NULL)
                 SET_BIT(ad->area_flags, AREA_CHANGED);
              return;
        }
    }

    interpret(ch, arg);

    return;
}

void do_opedit(CHAR_DATA *ch, char *argument)
{
    OPROG_CODE *pOcode;
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
	   send_to_char( "OPEdit: Warning: MobProg lies outside current area.\n\r", ch );
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

    send_to_char( "Syntax : opedit [vnum]\n\r", ch );
    send_to_char( "         opedit create [vnum]\n\r", ch );

    return;
}

OPEDIT (opedit_create)
{
    OPROG_CODE *pOcode;
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
       send_to_char("OPEdit : Insufficient security to create MobProgs.\n\r", ch);
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
    pOcode->next		= oprog_list;
    oprog_list			= pOcode;
    ch->desc->pEdit		= (void *)pOcode;
    ch->desc->editor		= ED_OPCODE;

    send_to_char("ObjProgram Code Created.\n\r",ch);

    return TRUE;
}

OPEDIT(opedit_show)
{
    OPROG_CODE *pOcode;
    char buf[MAX_STRING_LENGTH];
    EDIT_OPCODE(ch,pOcode);

    sprintf(buf,
           "Vnum:       [%d]\n\r"
           "Code:\n\r%s\n\r",
           pOcode->vnum,
           pOcode->code  );
    page_to_char_new(buf, ch, TRUE);

    return FALSE;
}

void fix_oprog_objs( CHAR_DATA *ch, OPROG_CODE *pOcode )
{
    OPROG_LIST *mpl;
    int hash;
    char buf[MSL];
    OBJ_INDEX_DATA *obj;

    if ( pOcode != NULL )
        for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
            for ( obj = obj_index_hash[hash]; obj; obj = obj->next )
                for ( mpl = obj->oprogs; mpl; mpl = mpl->next )
                    if ( mpl->vnum == pOcode->vnum )
                    {
                        sprintf( buf, "Fixing obj %d.\n\r", obj->vnum );
                        send_to_char( buf, ch );
                        mpl->code = pOcode->code;
                   
                        lua_load_oprog( mud_LS, pOcode->vnum, pOcode->code);
                        ptc(ch, "Fixed lua script for %d.\n\r", pOcode->vnum);
                        
                    } 
}
#if 0
OPEDIT(opedit_lua)
{
    OPROG_CODE *pOcode;
    OPROG_LIST *mpl;
    EDIT_OPCODE(ch, pOcode);
    OBJ_INDEX_DATA *mob;
    int hash;
    char buf[MSL];

    pOcode->is_lua = !pOcode->is_lua;
    ptc( ch, "LUA set to %s\n\r", pOcode->is_lua ? "TRUE" : "FALSE" );
    if ( pOcode->is_lua )
        lua_mprogs++;
    else
        lua_mprogs--;

    fix_mprog_mobs( ch, pOcode);
}
#endif
/* Procedure to run when MPROG is changed and needs to be updated
   on mobs using it */

OPEDIT(opedit_code)
{
    OPROG_CODE *pOcode;
    EDIT_OPCODE(ch, pOcode);

    if (argument[0] =='\0')
    {
       string_append(ch, &pOcode->code);
       return TRUE;
    }

    send_to_char("Syntax: code\n\r",ch);
    return FALSE;
}
#if 0
OPEDIT( opedit_list )
{
   int count = 1;
   OPROG_CODE *mprg;
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
#endif
#if 0
/* define in mob_prog.c and mob_cmd.c */
typedef char* keyword_list[][2];
extern const keyword_list fn_keyword;
/* display valid if-checks */
OPEDIT( opedit_if )
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
OPEDIT( opedit_mob )
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
#endif
