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
#include "lua_scripting.h"

#define APEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

const struct olc_cmd_type apedit_table[] =
{
/*	{	command		function	      }, */
   
   {  "commands", show_commands  },
   {  "create",   apedit_create  },
   {  "code",     apedit_code    },
   {  "show",     apedit_show    },
   //{  "list",     apedit_list    },
   //{  "if",       apedit_if      },
   //{  "mob",      apedit_mob     },
   {  "?",        show_help      },
   //{  "lua",      apedit_lua     },
   
   {  NULL,       0              }
};

void apedit( CHAR_DATA *ch, char *argument)
{
    APROG_CODE *pAcode;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;
    AREA_DATA *ad;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument( argument, command);

    EDIT_APCODE(ch, pAcode);

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
    APROG_CODE *pAcode;
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

    result=lua_area_program( NULL, vnum, pAcode->code, area, ch, ATRIG_CALL );

    ptc( ch, "Aprog completed. Result: %s\n\r", result ? "TRUE" : "FALSE" );

}

void do_apedit(CHAR_DATA *ch, char *argument)
{
    APROG_CODE *pAcode;
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
    APROG_CODE *pAcode;
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
    pAcode->next		= aprog_list;
    aprog_list			= pAcode;
    ch->desc->pEdit		= (void *)pAcode;
    ch->desc->editor		= ED_APCODE;

    send_to_char("AREAprog Code Created.\n\r",ch);

    return TRUE;
}

APEDIT(apedit_show)
{
    APROG_CODE *pAcode;
    char buf[MAX_STRING_LENGTH];
    EDIT_APCODE(ch,pAcode);

    sprintf(buf,
           "Vnum:       [%d]\n\r"
           "Code:\n\r%s\n\r",
           pAcode->vnum,
           pAcode->code  );
    page_to_char_new(buf, ch, TRUE);

    return FALSE;
}

void fix_aprog_areas( CHAR_DATA *ch, APROG_CODE *pAcode )
{
    APROG_LIST *apl;
    int hash;
    char buf[MSL];
    AREA_DATA *area;

    if ( pAcode != NULL )
		for (area = area_first ; area ; area = area->next)
			for ( apl = area->aprogs; apl; apl=apl->next )
				if (apl->vnum == pAcode->vnum)
				{
					sprintf( buf, "Fixing area %s.\n\r", area->name );
					send_to_char( buf, ch);
					apl->code = pAcode->code;
					
					lua_load_aprog( g_mud_LS, pAcode->vnum, pAcode->code);
					ptc(ch, "Fixed lua script for %d.\n\r", pAcode->vnum);
				}
				
}

// void fix_aprog_objs( CHAR_DATA *ch, APROG_CODE *pAcode )
// {
    // APROG_LIST *mpl;
    // int hash;
    // char buf[MSL];
    // OBJ_INDEX_DATA *obj;

    // if ( pAcode != NULL )
        // for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
            // for ( obj = obj_index_hash[hash]; obj; obj = obj->next )
                // for ( mpl = obj->aprogs; mpl; mpl = mpl->next )
                    // if ( mpl->vnum == pAcode->vnum )
                    // {
                        // sprintf( buf, "Fixing obj %d.\n\r", obj->vnum );
                        // send_to_char( buf, ch );
                        // mpl->code = pAcode->code;
                   
                        // lua_load_aprog( mud_LS, pAcode->vnum, pAcode->code);
                        // ptc(ch, "Fixed lua script for %d.\n\r", pAcode->vnum);
                        
                    // } 
// }
// #if 0
// APEDIT(apedit_lua)
// {
    // APROG_CODE *pAcode;
    // APROG_LIST *mpl;
    // EDIT_APCODE(ch, pAcode);
    // OBJ_INDEX_DATA *mob;
    // int hash;
    // char buf[MSL];

    // pAcode->is_lua = !pAcode->is_lua;
    // ptc( ch, "LUA set to %s\n\r", pAcode->is_lua ? "TRUE" : "FALSE" );
    // if ( pAcode->is_lua )
        // lua_mprogs++;
    // else
        // lua_mprogs--;

    // fix_mprog_mobs( ch, pAcode);
// }
// #endif
/* Procedure to run when MPROG is changed and needs to be updated
   on mobs using it */

APEDIT(apedit_code)
{
    APROG_CODE *pAcode;
    EDIT_APCODE(ch, pAcode);

    if (argument[0] =='\0')
    {
       string_append(ch, &pAcode->code);
       return TRUE;
    }

    send_to_char("Syntax: code\n\r",ch);
    return FALSE;
}
// #if 0
// APEDIT( apedit_list )
// {
   // int count = 1;
   // APROG_CODE *mprg;
   // char buf[MAX_STRING_LENGTH];
   // BUFFER *buffer;
   // bool fAll = !str_cmp(argument, "all");
   // char blah;
   // AREA_DATA *ad;
   
   // buffer = new_buf();
   
   // for (mprg = mprog_list; mprg !=NULL; mprg = mprg->next)
      // if ( fAll || IS_BETWEEN(ch->in_room->area->min_vnum, mprg->vnum, ch->in_room->area->max_vnum) )
      // {
         // ad = get_vnum_area(mprg->vnum);
         
         // if ( ad == NULL )
            // blah = '?';
         // else
            // if ( IS_BUILDER(ch, ad) )
               // blah = '*';
            // else
               // blah = ' ';
            
            // sprintf(buf, "[%3d] (%c) %5d\n\r", count, blah, mprg->vnum );
            // add_buf(buffer, buf);
            
            // count++;
      // }
      
      // if ( count == 1 )
      // {
         // if ( fAll )
            // add_buf( buffer, "No existen progs.\n\r" );
         // else
            // add_buf( buffer, "No existen progs en esta area.\n\r" );
         
      // }
      
      // page_to_char(buf_string(buffer), ch);
      // free_buf(buffer);
      
      // return FALSE;
      
// }
// #endif
// #if 0
// /* define in mob_prog.c and mob_cmd.c */
// typedef char* keyword_list[][2];
// extern const keyword_list fn_keyword;
// /* display valid if-checks */
// APEDIT( apedit_if )
// {
    // BUFFER *buffer;
    // char buf[MSL];
    // int i;

    // buffer = new_buf();

    // add_buf( buffer, "==================== Valid if-checks ====================\n\r" );
    // for( i = 0; fn_keyword[i][0][0] != '\n'; i++ )
    // {
	// sprintf( buf, "%-14s: %s\n\r", fn_keyword[i][0], fn_keyword[i][1] );
	// add_buf( buffer, buf );
    // }
    
    // page_to_char(buf_string(buffer), ch);
    // free_buf(buffer);
    // return FALSE;
// }

// /* display valid mob commands */
// APEDIT( apedit_mob )
// {
    // BUFFER *buffer;
    // char buf[MSL];
    // int i;

    // buffer = new_buf();

    // add_buf( buffer, "==================== Valid mob-commands =================\n\r" );
    // for( i = 0; mob_cmd_table[i].name[0] != '\0'; i++ )
    // {
	// sprintf( buf, "mob %s %s\n\r", mob_cmd_table[i].name, mob_cmd_table[i].help );
	// add_buf( buffer, buf );
    // }
    
    // page_to_char(buf_string(buffer), ch);
    // free_buf(buffer);
    // return FALSE;
// }
// #endif
