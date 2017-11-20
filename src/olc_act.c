/***************************************************************************
*  File: olc_act.c                                                        *
*                                                                         *
*  Much time and thought has gone into this software and you are          *
*  benefitting.  We hope that you share your changes too.  What goes      *
*  around, comes around.                                                  *
*                                                                         *
*  This code was freely distributed with the The Isles 1.1 source code,   *
*  and has been used here for OLC - OLC would not be what it is without   *
*  all the previous coders who released their source code.                *
*                                                                         *
***************************************************************************/



#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"
#include "mob_stats.h"

char * mprog_type_to_name ( int type );

/*
#define ALT_FLAGVALUE_SET( _blargh, _table, _arg )		\
{							\
    int blah = flag_lookup(_arg, _table);		\
    _blargh = (blah == NO_FLAG) ? 0 : blah;		\
}

#define ALT_FLAGVALUE_TOGGLE( _blargh, _table, _arg )		\
{							\
    int blah = flag_lookup(_arg, _table);		\
    _blargh ^= (blah == NO_FLAG) ? 0 : blah;	\
}
*/

#define ALT_FLAGVALUE_SET( _blargh, _table, _arg ) \
    _blargh = alt_flagvalue( _table, _arg )
#define ALT_FLAGVALUE_TOGGLE( _blargh, _table, _arg ) \
    _blargh = alt_flagvalue_toggle( _blargh, _table, _arg )

long alt_flagvalue( const struct flag_type *flag_table, const char *argument )
{
    long buf = 0;
    int flag;
#ifdef FLAG_DEBUG
    log_string( "alt_flagvalue: start" );
#endif
    flag = flag_lookup(argument, flag_table);
    if ( flag != NO_FLAG )
	I_SET_BIT( buf, flag );
#ifdef FLAG_DEBUG
    log_string( "alt_flagvalue: done" );
#endif
    return buf;
}

long alt_flagvalue_toggle( long old_flag, const struct flag_type *flag_table, const char *argument )
{
    long buf = old_flag;
    int flag;
#ifdef FLAG_DEBUG
    log_string( "alt_flagvalue_toggle: start" );
#endif
    flag = flag_lookup(argument, flag_table );
    if ( flag != NO_FLAG )
	I_TOGGLE_BIT( buf, flag );
#ifdef FLAG_DEBUG
    log_string( "alt_flagvalue_toggle: done" );
#endif
    return buf;
}

/* Return TRUE if area changed, FALSE if not. */
#define REDIT( fun ) bool fun( CHAR_DATA *ch, const char *argument )
#define OEDIT( fun ) bool fun( CHAR_DATA *ch, const char *argument )
#define MEDIT( fun ) bool fun( CHAR_DATA *ch, const char *argument )
#define AEDIT( fun ) bool fun( CHAR_DATA *ch, const char *argument )
#define HEDIT( fun ) bool fun( CHAR_DATA *ch, const char *argument )

#define RACEEDIT( fun )    bool fun( CHAR_DATA *ch, const char *argument )
#define SKILLEDIT( fun )   bool fun( CHAR_DATA *ch, const char *argument )
#define REMORTEDIT( fun )  bool fun( CHAR_DATA *ch, const char *argument )



extern HELP_AREA * had_list;

struct olc_help_type
{
    const char *command;
    const void *structure;
    const char *desc;
};



bool show_version( CHAR_DATA *ch, const char *argument )
{
    send_to_char( VERSION, ch );
    send_to_char( "\n\r", ch );
    send_to_char( AUTHOR, ch );
    send_to_char( "\n\r", ch );
    send_to_char( DATE, ch );
    send_to_char( "\n\r", ch );
    send_to_char( CREDITS, ch );
    send_to_char( "\n\r", ch );
    
    return FALSE;
}    

/*
* This table contains help commands and a brief description of each.
* ------------------------------------------------------------------
*/
const struct olc_help_type help_table[] =
{
    {	"area",		area_flags,	 "Area attributes."		 },
    {	"room",		room_flags,	 "Room attributes."		 },
    {	"sector",	sector_flags,	 "Sector types, terrain."	 },
    {	"exit",		exit_flags,	 "Exit types."			 },
    {	"type",		type_flags,	 "Types of objects."		 },
    {	"extra",	extra_flags,	 "Object attributes."		 },
    {	"wear",		wear_types,	 "Where to wear object."	 },
    {	"spec",		spec_table,	 "Available special programs." 	 },
    {	"sex",		sex_flags,	 "Sexes."			 },
    {	"act",		act_flags,	 "Mobile attributes."		 },
    {	"affect",	affect_flags,	 "Mobile affects."		 },
    {	"wear-loc",	wear_loc_flags,	 "Where mobile wears object."	 },
    {	"spells",	skill_table,	 "Names of current spells." 	 },
    {	"container",	container_flags, "Container status."		 },
    
    /* ROM specific bits: */
    
    {   "apply",	apply_flags,	 "Apply flags"			 },
    {	"form",		form_flags,	 "Mobile body form."	         },
    {	"part",		part_flags,	 "Mobile body parts."		 },
    {	"imm",		imm_flags,	 "Mobile immunity."		 },
    {	"res",		res_flags,	 "Mobile resistance."	         },
    {	"vuln",		vuln_flags,	 "Mobile vulnerability."	 },
    {	"off",		off_flags,	 "Mobile offensive behaviour."	 },
    {	"size",		size_flags,	 "Mobile size."			 },
    {   "position",     position_flags,  "Mobile positions."             },
    {   "wclass",       weapon_class,    "Weapon class."                 }, 
    {   "wtype",        weapon_type2,    "Special weapon type."          },
    {	"portal",	portal_flags,	 "Portal types."		 },
    {	"furniture",	furniture_flags, "Furniture types."		 },
    {   "liquid",	liq_table,	 "Liquid types."		 },
    {	"apptype",	apply_types,	 "Apply types."			 },
    {	"weapon",	attack_table,	 "Weapon types."		 },
    {	"mprog",	mprog_flags,	 "MobProgram flags."		 },
    {	NULL,		NULL,		 NULL				 }
};



/*****************************************************************************
Name:		show_flag_cmds
Purpose:	Displays settable flags and stats.
Called by:	show_help(olc_act.c).
****************************************************************************/
void show_flag_cmds( CHAR_DATA *ch, const struct flag_type *flag_table )
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH ];
    int  flag;
    int  col;
    
    buf1[0] = '\0';
    col = 0;
    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
        if ( flag_table[flag].settable )
        {
            sprintf( buf, "%-19.18s", flag_table[flag].name );
            strcat( buf1, buf );
            if ( ++col % 4 == 0 )
                strcat( buf1, "\n\r" );
        }
    }
    
    if ( col % 4 != 0 )
        strcat( buf1, "\n\r" );
    
    send_to_char( buf1, ch );
    return;
}


/*****************************************************************************
Name:		show_skill_cmds
Purpose:	Displays all skill functions.
Does remove those damn immortal commands from the list.
Could be improved by:
(1) Adding a check for a particular class.
(2) Adding a check for a level range.
Called by:	show_help(olc_act.c).
****************************************************************************/
void show_skill_cmds( CHAR_DATA *ch, int tar )
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH*2 ];
    int  sn;
    int  col;
    
    buf1[0] = '\0';
    col = 0;
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if ( !skill_table[sn].name )
            break;
        
        if ( !str_cmp( skill_table[sn].name, "reserved" )
            || skill_table[sn].spell_fun == spell_null )
            continue;
        
        if ( tar == -1 || skill_table[sn].target == tar )
        {
            sprintf( buf, "%-19.18s", skill_table[sn].name );
            strcat( buf1, buf );
            if ( ++col % 4 == 0 )
                strcat( buf1, "\n\r" );
        }
    }
    
    if ( col % 4 != 0 )
        strcat( buf1, "\n\r" );
    
    send_to_char( buf1, ch );
    return;
}



/*****************************************************************************
Name:		show_spec_cmds
Purpose:	Displays settable special functions.
Called by:	show_help(olc_act.c).
****************************************************************************/
void show_spec_cmds( CHAR_DATA *ch )
{
    char buf  [ MAX_STRING_LENGTH ];
    char buf1 [ MAX_STRING_LENGTH ];
    int  spec;
    int  col;
    
    buf1[0] = '\0';
    col = 0;
    send_to_char( "Preceed special functions with 'spec_'\n\r\n\r", ch );
    for (spec = 0; spec_table[spec].function != NULL; spec++)
    {
        sprintf( buf, "%-19.18s", &spec_table[spec].name[5] );
        strcat( buf1, buf );
        if ( ++col % 4 == 0 )
            strcat( buf1, "\n\r" );
    }
    
    if ( col % 4 != 0 )
        strcat( buf1, "\n\r" );
    
    send_to_char( buf1, ch );
    return;
}



/*****************************************************************************
Name:		show_help
Purpose:	Displays help for many tables used in OLC.
Called by:	olc interpreters.
****************************************************************************/
bool show_help( CHAR_DATA *ch, const char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char spell[MAX_INPUT_LENGTH];
    int cnt;
    
    argument = one_argument( argument, arg );
    one_argument( argument, spell );
    
    /*
    * Display syntax.
    */
    if ( arg[0] == '\0' )
    {
        send_to_char( "Syntax:  ? [command]\n\r\n\r", ch );
        send_to_char( "[command]  [description]\n\r", ch );
        for (cnt = 0; help_table[cnt].command != NULL; cnt++)
        {
            sprintf( buf, "%-10.10s -%s\n\r",
                capitalize( help_table[cnt].command ),
                help_table[cnt].desc );
            send_to_char( buf, ch );
        }
        return FALSE;
    }
    
    /*
    * Find the command, show changeable data.
    * ---------------------------------------
    */
    for (cnt = 0; help_table[cnt].command != NULL; cnt++)
    {
        if (  arg[0] == help_table[cnt].command[0]
            && !str_prefix( arg, help_table[cnt].command ) )
        {
            if ( help_table[cnt].structure == spec_table )
            {
                show_spec_cmds( ch );
                return FALSE;
            }
            else
                if ( help_table[cnt].structure == liq_table )
                {
                    show_liqlist( ch );
                    return FALSE;
                }
                else
                    if ( help_table[cnt].structure == attack_table )
                    {
                        show_damlist( ch );
                        return FALSE;
                    }
                    else
                        if ( help_table[cnt].structure == skill_table )
                        {
                            
                            if ( spell[0] == '\0' )
                            {
                                send_to_char( "Syntax:  ? spells "
                                    "[ignore/attack/defend/self/object/all]\n\r", ch );
                                return FALSE;
                            }
                            
                            if ( !str_prefix( spell, "all" ) )
                                show_skill_cmds( ch, -1 );
                            else if ( !str_prefix( spell, "ignore" ) )
                            {
                                show_skill_cmds( ch, TAR_IGNORE );
                                show_skill_cmds( ch, TAR_IGNORE_OFF );
                                show_skill_cmds( ch, TAR_IGNORE_OBJ );
                                show_skill_cmds( ch, TAR_IGNORE_DEF );
                            }
                            else if ( !str_prefix( spell, "attack" ) )
                            {
                                send_to_char("   Non-targetted attack spells:\n\r",ch);
                                show_skill_cmds( ch, TAR_CHAR_OFFENSIVE );
                                send_to_char( "   Targetted attack spells:\n\r",ch);
                                show_skill_cmds( ch, TAR_VIS_CHAR_OFF );
                            }
                            else if ( !str_prefix( spell, "defend" ) )
                                show_skill_cmds( ch, TAR_CHAR_DEFENSIVE );
                            else if ( !str_prefix( spell, "self" ) )
                                show_skill_cmds( ch, TAR_CHAR_SELF );
                            else if ( !str_prefix( spell, "object" ) )
                                show_skill_cmds( ch, TAR_OBJ_INV );
                            else
                                send_to_char( "Syntax:  ? spell "
                                "[ignore/attack/defend/self/object/all]\n\r", ch );
                            
                            return FALSE;
                        }
                        else
                        {
                            show_flag_cmds( ch, help_table[cnt].structure );
                            return FALSE;
                        }
        }
    }
    
    show_help( ch, "" );
    return FALSE;
}

REDIT( redit_rlist )
{
    ROOM_INDEX_DATA	*pRoomIndex;
    AREA_DATA		*pArea;
    char		buf  [ MAX_STRING_LENGTH   ];
    BUFFER		*buf1;
    char		arg  [ MAX_INPUT_LENGTH    ];
    bool found;
    int vnum;
    int  col = 0;
    
    one_argument( argument, arg );
    
    pArea = ch->in_room->area;
    buf1=new_buf();
    /*    buf1[0] = '\0'; */
    found   = FALSE;
    
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pRoomIndex = get_room_index( vnum ) ) )
        {
            found = TRUE;
            sprintf( buf, "[%5d] %-17.16s",
                vnum, capitalize( pRoomIndex->name ) );
            add_buf( buf1, buf );
            if ( ++col % 3 == 0 )
                add_buf( buf1, "\n\r" );
        }
    }
    
    if ( !found )
    {
        send_to_char( "Room(s) not found in this area.\n\r", ch);
    }
    else
    {
        if ( col % 3 != 0 )
            add_buf( buf1, "\n\r" );
        
        page_to_char_bw( buf_string(buf1), ch );
    }

    free_buf(buf1);
    return FALSE;
}

REDIT( redit_mlist )
{
    MOB_INDEX_DATA	*pMobIndex;
    AREA_DATA		*pArea;
    char		buf  [ MAX_STRING_LENGTH   ];
    BUFFER		*buf1;
    char		arg  [ MAX_INPUT_LENGTH    ];
    bool fAll, found;
    int vnum;
    int  col = 0;
    
    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Syntax:  mlist <all/name>\n\r", ch );
        return FALSE;
    }
    
    buf1=new_buf();
    pArea = ch->in_room->area;
    /*    buf1[0] = '\0'; */
    fAll    = !str_cmp( arg, "all" );
    found   = FALSE;
    
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
        {
            if ( fAll || is_name( arg, pMobIndex->player_name ) )
            {
                found = TRUE;
                sprintf( buf, "[%5d] %-17.16s",
                    pMobIndex->vnum, capitalize( pMobIndex->short_descr ) );
                add_buf( buf1, buf );
                if ( ++col % 3 == 0 )
                    add_buf( buf1, "\n\r" );
            }
        }
    }
    
    if ( !found )
    {
        send_to_char( "Mobile(s) not found in this area.\n\r", ch);
    }
    else
    {
        if ( col % 3 != 0 )
            add_buf( buf1, "\n\r" );
        
        page_to_char_bw( buf_string(buf1), ch );
    }
    free_buf(buf1);
    return FALSE;
}



REDIT( redit_olist )
{
    OBJ_INDEX_DATA	*pObjIndex;
    AREA_DATA		*pArea;
    char		buf  [ MAX_STRING_LENGTH   ];
    BUFFER		*buf1;
    char		arg  [ MAX_INPUT_LENGTH    ];
    bool fAll, found;
    int vnum;
    int  col = 0;
    
    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Syntax:  olist <all/name/item_type>\n\r", ch );
        return FALSE;
    }
    
    pArea = ch->in_room->area;
    buf1=new_buf();
    /*    buf1[0] = '\0'; */
    fAll    = !str_cmp( arg, "all" );
    found   = FALSE;
    
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pObjIndex = get_obj_index( vnum ) ) )
        {
            if ( fAll || is_name( arg, pObjIndex->name )
                || flag_lookup(arg, type_flags) == pObjIndex->item_type )
            {
                found = TRUE;
                sprintf( buf, "[%5d] %-17.16s",
                    pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
                add_buf( buf1, buf );
                if ( ++col % 3 == 0 )
                    add_buf( buf1, "\n\r" );
            }
        }
    }
    
    if ( !found )
    {
        send_to_char( "Object(s) not found in this area.\n\r", ch);
    }
    else
    {
        if ( col % 3 != 0 )
            add_buf( buf1, "\n\r" );
        
        page_to_char_bw( buf_string(buf1), ch );
    }
    free_buf(buf1);
    return FALSE;
}



REDIT( redit_mshow )
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  mshow <vnum>\n\r", ch );
        return FALSE;
    }
    
    if ( !is_number( argument ) )
    {
        send_to_char( "REdit:  argument is the mob vnum.\n\r", ch);
        return FALSE;
    }
    
    if ( is_number( argument ) )
    {
        value = atoi( argument );
        if ( !( pMob = get_mob_index( value ) ))
        {
            send_to_char( "REdit:  That mobile does not exist.\n\r", ch );
            return FALSE;
        }
        
        ch->desc->pEdit = (void *)pMob;
    }
    
    medit_show( ch, argument );
    ch->desc->pEdit = (void *)ch->in_room;
    return FALSE; 
}



REDIT( redit_oshow )
{
    OBJ_INDEX_DATA *pObj;
    int value;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  oshow <vnum>\n\r", ch );
        return FALSE;
    }
    
    if ( !is_number( argument ) )
    {
        send_to_char( "REdit:  argument is the obj vnum.\n\r", ch);
        return FALSE;
    }
    
    if ( is_number( argument ) )
    {
        value = atoi( argument );
        if ( !( pObj = get_obj_index( value ) ))
        {
            send_to_char( "REdit:  That object does not exist.\n\r", ch );
            return FALSE;
        }
        
        ch->desc->pEdit = (void *)pObj;
    }
    
    oedit_show( ch, argument );
    ch->desc->pEdit = (void *)ch->in_room;
    return FALSE; 
}



/*****************************************************************************
Name:		check_range( lower vnum, upper vnum )
Purpose:	Ensures the range spans only one area.
Called by:	aedit_vnum(olc_act.c).
****************************************************************************/
bool check_range( int lower, int upper )
{
    AREA_DATA *pArea;
    int cnt = 0;
    
    for ( pArea = area_first; pArea; pArea = pArea->next )
    {
    /*
    * lower < area < upper
        */
        if ( ( lower <= pArea->min_vnum && pArea->min_vnum <= upper )
            ||   ( lower <= pArea->max_vnum && pArea->max_vnum <= upper ) )
            ++cnt;
        
        if ( cnt > 1 )
            return FALSE;
    }
    return TRUE;
}



AREA_DATA *get_vnum_area( int vnum )
{
    AREA_DATA *pArea;
    
    for ( pArea = area_first; pArea; pArea = pArea->next )
    {
        if ( vnum >= pArea->min_vnum
            && vnum <= pArea->max_vnum )
            return pArea;
    }
    
    return NULL;
}


/*****************************************************************
 * Name: update_aprog_flags
 * Purpose: fix bug that removes valid aprog flags
 * Called by: oedit_delaprog
 *****************************************************************/
void update_aprog_flags( AREA_DATA *pArea )
{
    PROG_LIST *list;

    /* clear flags */
    flag_clear( pArea->aprog_flags );

    /* re-add all flags needed */
    for (list = pArea->aprogs; list != NULL; list = list->next)
        SET_BIT(pArea->aprog_flags, list->trig_type);
}

AEDIT ( aedit_delaprog )
{
    AREA_DATA *pArea;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char aprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_AREA(ch, pArea);

    one_argument( argument, aprog );
    if (!is_number( aprog ) || aprog[0] == '\0' )
    {
        send_to_char("Syntax:  delaprog [#aprog]\n\r",ch);
        return FALSE;
    }

    value = atoi ( aprog );

    if ( value < 0 )
    {
        send_to_char("Only non-negative aprog-numbers allowed.\n\r",ch);
        return FALSE;
    }

    if ( !(list= pArea->aprogs) )
    {
        send_to_char("AEdit:  Non existant mprog.\n\r",ch);
        return FALSE;
    }

    if ( value == 0 )
    {
        list = pArea->aprogs;
        pArea->aprogs = list->next;
        free_aprog( list );
    }
    else
    {
        while ( (list_next = list->next) && (++cnt < value ) )
            list = list_next;

        if ( list_next )
        {
            list->next = list_next->next;
            free_aprog(list_next);
        }
        else
        {
            send_to_char("No such aprog.\n\r",ch);
            return FALSE;
        }
    }

    update_aprog_flags(pArea);

    send_to_char("Aprog removed.\n\r", ch);
    return TRUE;
}

AEDIT ( aedit_addaprog )
{
    int value;
    AREA_DATA *pArea;
    PROG_LIST *list;
    PROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char phrase[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);
    argument=one_argument(argument, num);
    argument=one_argument(argument, trigger);
    argument=one_argument(argument, phrase);

    if (!is_number(num) || trigger[0] =='\0' || phrase[0] =='\0' )
    {
        send_to_char("Syntax:   addaprog [vnum] [trigger] [phrase]\n\r",ch);
        return FALSE;
    }

    if ( (value = flag_lookup(trigger, aprog_flags)) == NO_FLAG )
    {
        send_to_char("Valid flags are:\n\r",ch);
        show_help( ch, "aprog");
        return FALSE;
    }

    if ( ( code =get_aprog_index (atoi(num) ) ) == NULL)
    {
        send_to_char("No such AREAProgram.\n\r",ch);
        return FALSE;
    }

    if ( value==ATRIG_TIMER && IS_SET( pArea->aprog_flags, value ) )
    {
        send_to_char("Can only have one timer trigger.\n\r", ch );
        return FALSE;
    }

    list                  = new_aprog();
    list->vnum            = atoi(num);
    list->trig_type       = value;
    list->trig_phrase     = str_dup(phrase);
    list->script          = code;
    SET_BIT(pArea->aprog_flags,value);
    list->next            = pArea->aprogs;
    pArea->aprogs          = list;
    
    aprog_setup( pArea );

    send_to_char( "Aprog Added.\n\r",ch);
    return TRUE;
}



/*
* Area Editor Functions.
*/
AEDIT( aedit_show )
{
    AREA_DATA *pArea;
    PROG_LIST *list;
    char buf  [MAX_STRING_LENGTH];
    int i;
    
    EDIT_AREA(ch, pArea);
    
    sprintf( buf, "Name:       [%5d] %s\n\r", pArea->vnum, pArea->name );
    send_to_char( buf, ch );
    
#if 0  /* ROM OLC */
    sprintf( buf, "Recall:     [%5d] %s\n\r", pArea->recall,
        get_room_index( pArea->recall )
        ? get_room_index( pArea->recall )->name : "none" );
    send_to_char( buf, ch );
#endif /* ROM */
    
    sprintf( buf, "File:       %s\n\r", pArea->file_name );
    send_to_char( buf, ch );
    
    sprintf( buf, "Vnums:      [%d-%d]\n\r", pArea->min_vnum, pArea->max_vnum );
    send_to_char( buf, ch );
    
    sprintf( buf, "Age:        [%d]\n\r",	pArea->age );
    send_to_char( buf, ch );
    
    sprintf( buf, "Reset Time: [%d]\n\r", pArea->reset_time );
    send_to_char(buf,ch);
    
    sprintf( buf, "Players:    [%d]\n\r", pArea->nplayer );
    send_to_char( buf, ch );
    
    sprintf( buf, "Security:   [%d]\n\r", pArea->security );
    send_to_char( buf, ch );

   /* Added minimum and maximum level fields to make a new
      and easier to read areas list - Astark Dec 2012 */ 
    sprintf( buf, "Min Level:  [%d]\n\r", pArea->minlevel );
    send_to_char( buf, ch);

    sprintf( buf, "Max Level:  [%d]\n\r", pArea->maxlevel );
    send_to_char( buf, ch);

    sprintf( buf, "Miniquests: [%d]\n\r", pArea->miniquests );
    send_to_char( buf, ch);
    
    sprintf( buf, "Builders:   [%s]\n\r", pArea->builders );
    send_to_char( buf, ch );
    
    sprintf( buf, "Credits:    [%s]\n\r", pArea->credits );
    send_to_char_bw( buf, ch );
    
    sprintf( buf, "Flags:      [%s]\n\r", flag_bits_name(area_flags, pArea->area_flags) );
    send_to_char( buf, ch );

    sprintf( buf, "Comments:\n\r%s", pArea->comments );
    send_to_char( buf, ch );
    
    for ( i = 0; i < MAX_AREA_CLONE; i++ )
        if ( pArea->clones[i] > 0 )
        {
            sprintf( buf, "[%d] Clone: %5d\n\r", i, pArea->clones[i] );
            send_to_char( buf, ch );
        }

    if ( pArea->aprogs )
    {
        int cnt;

        sprintf(buf, "\n\rAREAPrograms for [%5d]:\n\r", pArea->vnum);
        send_to_char( buf, ch );

        for (cnt=0, list=pArea->aprogs; list; list=list->next)
        {
            if (cnt ==0)
            {
                send_to_char ( " Number Vnum Trigger Phrase\n\r", ch );
                send_to_char ( " ------ ---- ------- ------\n\r", ch );
            }

            sprintf(buf, "[%5d] %4d %7s %s\n\r", cnt,
                    list->vnum,name_lookup(list->trig_type, aprog_flags),
                    list->trig_phrase);
            send_to_char( buf, ch );
            cnt++;
        }
    }

    return FALSE;
}

// quick command for testing
DEF_DO_FUN( do_areset )
{
    reset_area(ch->in_room->area);
    send_to_char("Area reset.\n\r", ch);
}

AEDIT( aedit_reset )
{
    AREA_DATA *pArea;
    
    EDIT_AREA(ch, pArea);
    
    reset_area( pArea );
    send_to_char( "Area reset.\n\r", ch );
    
    return FALSE;
}


AEDIT( aedit_purge )
{
    AREA_DATA *pArea;
    
    EDIT_AREA(ch, pArea);
    
    purge_area( pArea );
    send_to_char( "Area purged.\n\r", ch );
    
    return FALSE;
}


AEDIT( aedit_create )
{
    AREA_DATA *pArea;
    
    pArea               =   new_area();
    area_last->next     =   pArea;
    area_last		=   pArea;	/* Thanks, Walker. */
    ch->desc->pEdit     =   (void *)pArea;
    
    /* SET_BIT( pArea->area_flags, AREA_ADDED ); */
    send_to_char( "Area Created.\n\r", ch );
    return FALSE;
}



AEDIT( aedit_scrap )
{
    AREA_DATA *pArea;
    
    EDIT_AREA(ch, pArea);
    
    if ( get_trust(ch) < MAX_LEVEL)
    {
        send_to_char( "The area list may only be manipulated by Implementors.\n\r", ch );
        return FALSE;
    }
    
    pArea->save = !(pArea->save);
    
    if (pArea->save)
        send_to_char( "Area will NOT be scrapped.\n\r", ch );
    else
        send_to_char( "Area will be scrapped!\n\r", ch );
    
    return FALSE;
}



AEDIT( aedit_move )
{
    AREA_DATA *from;
    AREA_DATA *from_prev;
    AREA_DATA *to_prev;
    AREA_DATA *to;
    char arg[MAX_STRING_LENGTH];
    int i, i2;
    
    if ( get_trust(ch) < MAX_LEVEL)
    {
        send_to_char( "The area list may only be manipulated by Implementors.\n\r", ch );
        return FALSE;
    }
    
    EDIT_AREA(ch, from);
    
    one_argument( argument, arg );
    
    if ( arg[0] == '\0' || !is_number(arg))
    {
        send_to_char( "Swap this area with which area?\n\r", ch );
        return FALSE;
    }
    
    from_prev = area_first;
    
    for ( to=area_first; to!=from; to=to->next )
        from_prev=to;
    
    i2=atoi(arg);
    to_prev= area_first;
    
    for ( i=1; i<i2; i++ )
    {
        if (to_prev->next->next == NULL)
        {
            send_to_char( "There arent that many areas.\n\r", ch );
            return FALSE;
        }
        
        to_prev = to_prev->next;
    }
    
    if (i2>0) to=to_prev->next;
    else to=area_first;
    
    if (to==from)
    {
        send_to_char( "You cant move an area onto itself.\n\r", ch );
        return FALSE;
    }
    
    if (from==area_last) area_last=from_prev;
    else if (to==area_last) area_last=from; 
    
    if (from==area_first) area_first=from->next;
    else from_prev->next=from->next;
    from->next=to;
    
    if (i2>0) to_prev->next = from;
    else area_first=from;
    
    i=0;
    for (to=area_first; to!=NULL; to=to->next)
    {
        to->vnum=i;
        i++;
    }
    
    send_to_char( "Area Moved.\n\r", ch );
    return FALSE;
}



AEDIT( aedit_name )
{
    AREA_DATA *pArea;
    
    EDIT_AREA(ch, pArea);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:   name [$name]\n\r", ch );
        return FALSE;
    }
    
    free_string( pArea->name );
    pArea->name = str_dup( argument );
    
    send_to_char( "Name set.\n\r", ch );
    return TRUE;
}


AEDIT( aedit_credits )
{
    AREA_DATA *pArea;
    
    EDIT_AREA(ch, pArea);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:   credits [$credits]\n\r", ch );
        return FALSE;
    }
    
    free_string( pArea->credits );
    pArea->credits = str_dup( argument );
    
    send_to_char( "Credits set.\n\r", ch );
    return TRUE;
}

AEDIT( aedit_remort )
{
    AREA_DATA *pArea;
    
    EDIT_AREA(ch, pArea);
    
    TOGGLE_BIT(pArea->area_flags, AREA_REMORT);
    
    send_to_char( "Remort flag toggled.\n\r", ch );
    return TRUE;
}

AEDIT( aedit_clone )
{
    AREA_DATA *pArea;
    char arg1[MIL];
    char arg2[MIL];
    int nr, min_vnum, max_vnum;
    char buf[MIL];
    
    EDIT_AREA(ch, pArea);
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0'
	 || !is_number(arg1) || !is_number(arg2) )
    {
        send_to_char( "Syntax: clone <nr> <min_vnum>\n\r", ch );
        return FALSE;
    }

    nr = atoi(arg1);
    min_vnum = atoi(arg2);
    max_vnum = min_vnum + pArea->max_vnum - pArea->min_vnum;

    if ( nr < 0 || nr >= MAX_AREA_CLONE )
    {
	sprintf( buf, "Nr must be 0 - %d.\n\r", MAX_AREA_CLONE - 1 );
        send_to_char( buf, ch );
        return FALSE;
    }

    if ( min_vnum != 0 && !range_is_free(min_vnum, max_vnum) )
    {
	send_to_char( "That vnum range is already in use.\n\r", ch );
        return FALSE;
    }

    pArea->clones[nr] = min_vnum;
    send_to_char( "Ok. Clone will be created upon reboot.\n\r", ch );

    return TRUE;
}

AEDIT( aedit_file )
{
    AREA_DATA *pArea;
    char file[MAX_STRING_LENGTH];
    int i, length;
    
    EDIT_AREA(ch, pArea);
    
    one_argument( argument, file );	/* Forces Lowercase */
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  filename [$file]\n\r", ch );
        return FALSE;
    }
    
    /*
    * Simple Syntax Check.
    */
    length = strlen( argument );
    if ( length > 8 )
    {
        send_to_char( "No more than eight characters allowed.\n\r", ch );
        return FALSE;
    }
    
    /*
    * Allow only letters and numbers.
    */
    for ( i = 0; i < length; i++ )
    {
        if ( !isalnum( file[i] ) )
        {
            send_to_char( "Only letters and numbers are valid.\n\r", ch );
            return FALSE;
        }
    }    
    
    free_string( pArea->file_name );
    strcat( file, ".are" );
    pArea->file_name = str_dup( file );
    
    send_to_char( "Filename set.\n\r", ch );
    return TRUE;
}



AEDIT( aedit_age )
{
    AREA_DATA *pArea;
    char age[MAX_STRING_LENGTH];
    
    EDIT_AREA(ch, pArea);
    
    one_argument( argument, age );
    
    if ( !is_number( age ) || age[0] == '\0' )
    {
        send_to_char( "Syntax:  age [#xage]\n\r", ch );
        return FALSE;
    }
    
    pArea->age = atoi( age );
    
    send_to_char( "Age set.\n\r", ch );
    return TRUE;
}

AEDIT( aedit_reset_time )
{
    AREA_DATA *pArea;
    char age[MAX_STRING_LENGTH];
    
    EDIT_AREA(ch, pArea);
    
    one_argument( argument, age );
    
    if ( !is_number( age ) || age[0] == '\0' )
    {
        send_to_char( "Syntax:  time [#xtime]\n\r", ch );
        return FALSE;
    }
    
    pArea->reset_time = atoi( age );
    
    send_to_char( "Reset time set.\n\r", ch );
    return TRUE;
}



#if 0 /* ROM OLC */
AEDIT( aedit_recall )
{
    AREA_DATA *pArea;
    char room[MAX_STRING_LENGTH];
    int  value;
    
    EDIT_AREA(ch, pArea);
    
    one_argument( argument, room );
    
    if ( !is_number( argument ) || argument[0] == '\0' )
    {
        send_to_char( "Syntax:  recall [#xrvnum]\n\r", ch );
        return FALSE;
    }
    
    value = atoi( room );
    
    if ( !get_room_index( value ) )
    {
        send_to_char( "AEdit:  Room vnum does not exist.\n\r", ch );
        return FALSE;
    }
    
    pArea->recall = value;
    
    send_to_char( "Recall set.\n\r", ch );
    return TRUE;
}
#endif /* ROM OLC */


AEDIT( aedit_minlevel )
{
    AREA_DATA *pArea;
    char min[MAX_STRING_LENGTH];
    int  value;
    
    EDIT_AREA(ch, pArea);
    one_argument( argument, min );
    if ( !is_number( min ) || min[0] == '\0' )
    {
        send_to_char( "Syntax:  minlevel [#xlevel]\n\r", ch );
        return FALSE;
    }
    value = atoi( min );
    pArea->minlevel = value;
    send_to_char( "Min level set.\n\r", ch );
    return TRUE;
}


/* Sets the max level of the zone. Used in the new areas list
   for simplicity - Astark Dec 2012 */
AEDIT( aedit_maxlevel )
{
    AREA_DATA *pArea;
    char max[MAX_STRING_LENGTH];
    int  value;
    
    EDIT_AREA(ch, pArea);
    one_argument( argument, max );
    if ( !is_number( max ) || max[0] == '\0' )
    {
        send_to_char( "Syntax:  maxlevel [#xlevel]\n\r", ch );
        return FALSE;
    }
    value = atoi( max );
    pArea->maxlevel = value;
    send_to_char( "Max level set.\n\r", ch );
    return TRUE;
}


/* Sets the number of mini-quests that players should find in the zone
   - Astark Dec 2012 */
AEDIT( aedit_miniquests )
{
    AREA_DATA *pArea;
    char quests[MAX_STRING_LENGTH];
    int  value;
    
    EDIT_AREA(ch, pArea);
    one_argument( argument, quests);
    if ( !is_number( quests ) || quests[0] == '\0' )
    {
        send_to_char( "Syntax:  miniquests [#of mini quests]\n\r", ch );
        return FALSE;
    }
    value = atoi( quests );
    pArea->miniquests  = value;
    send_to_char( "Number of mini-quests set.\n\r", ch );
    return TRUE;
}

AEDIT( aedit_security )
{
    AREA_DATA *pArea;
    char sec[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int  value;
    
    EDIT_AREA(ch, pArea);
    
    one_argument( argument, sec );
    
    if ( !is_number( sec ) || sec[0] == '\0' )
    {
        send_to_char( "Syntax:  security [#xlevel]\n\r", ch );
        return FALSE;
    }
    
    value = atoi( sec );
    
    if ( value > ch->pcdata->security || value < 0 )
    {
        if ( ch->pcdata->security != 0 )
        {
            sprintf( buf, "Security is 0-%d.\n\r", ch->pcdata->security );
            send_to_char( buf, ch );
        }
        else
            send_to_char( "Security is 0 only.\n\r", ch );
        return FALSE;
    }
    
    pArea->security = value;
    
    send_to_char( "Security set.\n\r", ch );
    return TRUE;
}

AEDIT( aedit_comments)
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    if ( argument[0] == '\0' )
    {
        string_append( ch, &pArea->comments );
        return TRUE;
    }

    send_to_char( "Syntax:  comments   - line edit\n\r", ch );
    return FALSE;
}

AEDIT( aedit_builder )
{
    AREA_DATA *pArea;
    char name[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    
    EDIT_AREA(ch, pArea);
    
    one_argument( argument, name );
    
    if ( name[0] == '\0' )
    {
        send_to_char( "Syntax:  builder [$name]  -toggles builder\n\r", ch );
        send_to_char( "Syntax:  builder All      -allows everyone\n\r", ch );
        return FALSE;
    }
    
    name[0] = UPPER( name[0] );
    
    if ( strstr( pArea->builders, name ) != '\0' )
    {
        pArea->builders = string_replace( pArea->builders, name, "\0" );
        pArea->builders = trim_realloc( pArea->builders );
        
        if ( pArea->builders[0] == '\0' )
        {
            free_string( pArea->builders );
            pArea->builders = str_dup( "None" );
        }
        send_to_char( "Builder removed.\n\r", ch );
        return TRUE;
    }
    else
    {
        buf[0] = '\0';
        if ( strstr( pArea->builders, "None" ) != '\0' )
        {
            pArea->builders = string_replace( pArea->builders, "None", "\0" );
            pArea->builders = trim_realloc( pArea->builders );
        }
        
        if (pArea->builders[0] != '\0' )
        {
            strcat( buf, pArea->builders );
            strcat( buf, " " );
        }
        strcat( buf, name );
        free_string( pArea->builders );
        pArea->builders = str_dup(string_proper(buf));
        
        send_to_char( "Builder added.\n\r", ch );
        send_to_char( pArea->builders,ch);
        return TRUE;
    }
    
    return FALSE;
}



AEDIT( aedit_vnum )
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;
    
    EDIT_AREA(ch, pArea);
    
    argument = one_argument( argument, lower );
    one_argument( argument, upper );
    
    if ( !is_number( lower ) || lower[0] == '\0'
        || !is_number( upper ) || upper[0] == '\0' )
    {
        send_to_char( "Syntax:  vnum [#xlower] [#xupper]\n\r", ch );
        return FALSE;
    }
    
    if ( ( ilower = atoi( lower ) ) > ( iupper = atoi( upper ) ) )
    {
        send_to_char( "AEdit:  Upper must be larger then lower.\n\r", ch );
        return FALSE;
    }
    
    if ( !check_range( atoi( lower ), atoi( upper ) ) )
    {
        send_to_char( "AEdit:  Range must include only this area.\n\r", ch );
        return FALSE;
    }
    
    if ( get_vnum_area( ilower )
        && get_vnum_area( ilower ) != pArea )
    {
        send_to_char( "AEdit:  Lower vnum already assigned.\n\r", ch );
        return FALSE;
    }
    
    pArea->min_vnum = ilower;
    send_to_char( "Lower vnum set.\n\r", ch );
    
    if ( get_vnum_area( iupper )
        && get_vnum_area( iupper ) != pArea )
    {
        send_to_char( "AEdit:  Upper vnum already assigned.\n\r", ch );
        return TRUE;	/* The lower value has been set. */
    }
    
    pArea->max_vnum = iupper;
    send_to_char( "Upper vnum set.\n\r", ch );
    
    return TRUE;
}



AEDIT( aedit_lvnum )
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;
    
    EDIT_AREA(ch, pArea);
    
    one_argument( argument, lower );
    
    if ( !is_number( lower ) || lower[0] == '\0' )
    {
        send_to_char( "Syntax:  min_vnum [#xlower]\n\r", ch );
        return FALSE;
    }
    
    if ( ( ilower = atoi( lower ) ) > ( iupper = pArea->max_vnum ) )
    {
        send_to_char( "AEdit:  Value must be less than the max_vnum.\n\r", ch );
        return FALSE;
    }
    
    if ( !check_range( ilower, iupper ) )
    {
        send_to_char( "AEdit:  Range must include only this area.\n\r", ch );
        return FALSE;
    }
    
    if ( get_vnum_area( ilower )
        && get_vnum_area( ilower ) != pArea )
    {
        send_to_char( "AEdit:  Lower vnum already assigned.\n\r", ch );
        return FALSE;
    }
    
    pArea->min_vnum = ilower;
    send_to_char( "Lower vnum set.\n\r", ch );
    return TRUE;
}



AEDIT( aedit_uvnum )
{
    AREA_DATA *pArea;
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;
    
    EDIT_AREA(ch, pArea);
    
    one_argument( argument, upper );
    
    if ( !is_number( upper ) || upper[0] == '\0' )
    {
        send_to_char( "Syntax:  max_vnum [#xupper]\n\r", ch );
        return FALSE;
    }
    
    if ( ( ilower = pArea->min_vnum ) > ( iupper = atoi( upper ) ) )
    {
        send_to_char( "AEdit:  Upper must be larger then lower.\n\r", ch );
        return FALSE;
    }
    
    if ( !check_range( ilower, iupper ) )
    {
        send_to_char( "AEdit:  Range must include only this area.\n\r", ch );
        return FALSE;
    }
    
    if ( get_vnum_area( iupper )
        && get_vnum_area( iupper ) != pArea )
    {
        send_to_char( "AEdit:  Upper vnum already assigned.\n\r", ch );
        return FALSE;
    }
    
    pArea->max_vnum = iupper;
    send_to_char( "Upper vnum set.\n\r", ch );
    
    return TRUE;
}



/*
* Room Editor Functions.
*/

/*****************************************************************
 * Name: update_aprog_flags
 * Purpose: fix bug that removes valid aprog flags
 * Called by: oedit_delaprog
 *****************************************************************/
void update_rprog_flags( ROOM_INDEX_DATA *pRoom )
{
    PROG_LIST *list;

    /* clear flags */
    flag_clear( pRoom->rprog_flags );

    /* re-add all flags needed */
    for (list = pRoom->rprogs; list != NULL; list = list->next)
        SET_BIT(pRoom->rprog_flags, list->trig_type);
}

REDIT ( redit_delrprog )
{
    ROOM_INDEX_DATA *pRoom;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char rprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_ROOM(ch, pRoom);

    one_argument( argument, rprog );
    if (!is_number( rprog ) || rprog[0] == '\0' )
    {
        send_to_char("Syntax:  delrprog [#rprog]\n\r",ch);
        return FALSE;
    }

    value = atoi ( rprog );

    if ( value < 0 )
    {
        send_to_char("Only non-negative rprog-numbers allowed.\n\r",ch);
        return FALSE;
    }

    if ( !(list= pRoom->rprogs) )
    {
        send_to_char("REdit:  Non existent rprog.\n\r",ch);
        return FALSE;
    }

    if ( value == 0 )
    {
        list = pRoom->rprogs;
        pRoom->rprogs = list->next;
        free_rprog( list );
    }
    else
    {
        while ( (list_next = list->next) && (++cnt < value ) )
            list = list_next;

        if ( list_next )
        {
            list->next = list_next->next;
            free_rprog(list_next);
        }
        else
        {
            send_to_char("No such rprog.\n\r",ch);
            return FALSE;
        }
    }

    update_rprog_flags(pRoom);

    send_to_char("Rprog removed.\n\r", ch);
    return TRUE;
}

REDIT ( redit_addrprog )
{
    int value;
    ROOM_INDEX_DATA *pRoom;
    PROG_LIST *list;
    PROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char phrase[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];

    EDIT_ROOM(ch, pRoom);
    argument=one_argument(argument, num);
    argument=one_argument(argument, trigger);
    argument=one_argument(argument, phrase);

    if (!is_number(num) || trigger[0] =='\0' || phrase[0] =='\0' )
    {
        send_to_char("Syntax:   addrprog [vnum] [trigger] [phrase]\n\r",ch);
        return FALSE;
    }

    if ( (value = flag_lookup(trigger, rprog_flags)) == NO_FLAG )
    {
        send_to_char("Valid flags are:\n\r",ch);
        show_help( ch, "rprog");
        return FALSE;
    }

    if ( ( code =get_rprog_index (atoi(num) ) ) == NULL)
    {
        send_to_char("No such ROOMProgram.\n\r",ch);
        return FALSE;
    }

    if ( value==RTRIG_TIMER && IS_SET( pRoom->rprog_flags, value ) )
    {
        send_to_char("Can only have one timer trigger.\n\r", ch );
        return FALSE;
    }

    list                  = new_rprog();
    list->vnum            = atoi(num);
    list->trig_type       = value;
    list->trig_phrase     = str_dup(phrase);
    list->script          = code;
    SET_BIT(pRoom->rprog_flags,value);
    list->next            = pRoom->rprogs;
    pRoom->rprogs          = list;
    
    rprog_setup( pRoom );

    send_to_char( "Rprog Added.\n\r",ch);
    return TRUE;
}

REDIT( redit_show )
{
    ROOM_INDEX_DATA	*pRoom;
    char		buf  [MAX_STRING_LENGTH];
    char		buf1 [2*MAX_STRING_LENGTH];
    OBJ_DATA		*obj;
    CHAR_DATA		*rch;
    int			door;
    bool		fcnt;
    
    EDIT_ROOM(ch, pRoom);
    
    buf1[0] = '\0';
    
    sprintf( buf, "Description:\n\r%s", pRoom->description );
    strcat( buf1, buf );
    
    sprintf( buf, "Name:       [%s]\n\rArea:       [%5d] %s\n\r",
        pRoom->name, pRoom->area->vnum, pRoom->area->name );
    strcat( buf1, buf );
    
    sprintf( buf, "Vnum:       [%5d]\n\rSector:     [%s]\n\r",
        pRoom->vnum, flag_bit_name(sector_flags, pRoom->sector_type) );
    strcat( buf1, buf );
    
    sprintf( buf, "Room flags: [%s]\n\r",
        flag_bits_name(room_flags, pRoom->room_flags) );
    strcat( buf1, buf );
    
    if ( pRoom->heal_rate != 100 || pRoom->mana_rate != 100 )
    {
        sprintf( buf, "Health rec: [%d]\n\rMana rec  : [%d]\n\r",
            pRoom->heal_rate , pRoom->mana_rate );
        strcat( buf1, buf );
    }
    
    if ( pRoom->clan > 0 )
    {
        sprintf( buf, "Clan      : [%d] %s\n\r",
            pRoom->clan,
            clan_table[pRoom->clan].name );
        strcat( buf1, buf );
    }
    
    if ( pRoom->clan_rank > 0 )
    {
        sprintf( buf, "Clan Rank : [%d] %s\n\r",
            pRoom->clan_rank,
            clan_table[pRoom->clan].rank_list[pRoom->clan_rank].name );
        strcat( buf1, buf );
    }
    
    if ( !IS_NULLSTR(pRoom->owner) )
    {
        sprintf( buf, "Owner     : [%s]\n\r", pRoom->owner );
        strcat( buf1, buf );
    }
    
    if ( pRoom->extra_descr )
    {
        EXTRA_DESCR_DATA *ed;
        
        strcat( buf1, "Desc Kwds:  [" );
        for ( ed = pRoom->extra_descr; ed; ed = ed->next )
        {
            strcat( buf1, ed->keyword );
            if ( ed->next )
                strcat( buf1, " " );
        }
        strcat( buf1, "]\n\r" );
    }
    
    strcat( buf1, "Characters: [" );
    fcnt = FALSE;
    for ( rch = pRoom->people; rch; rch = rch->next_in_room )
    {
        if (can_see( ch, rch ) )
        {
            one_argument( rch->name, buf );
            strcat( buf1, buf );
            strcat( buf1, " " );
            fcnt = TRUE;
        }
    }
    
    if ( fcnt )
    {
        int end;
        
        end = strlen(buf1) - 1;
        buf1[end] = ']';
        strcat( buf1, "\n\r" );
    }
    else
        strcat( buf1, "none]\n\r" );
    
    strcat( buf1, "Objects:    [" );
    fcnt = FALSE;
    for ( obj = pRoom->contents; obj; obj = obj->next_content )
    {
        one_argument( obj->name, buf );
        strcat( buf1, buf );
        strcat( buf1, " " );
        fcnt = TRUE;
    }
    
    if ( fcnt )
    {
        int end;
        
        end = strlen(buf1) - 1;
        buf1[end] = ']';
        strcat( buf1, "\n\r" );
    }
    else
        strcat( buf1, "none]\n\r" );
    
    for ( door = 0; door < MAX_DIR; door++ )
    {
        EXIT_DATA *pexit;
        
        if ( ( pexit = pRoom->exit[door] ) )
        {
            //char word[MAX_INPUT_LENGTH];
            char reset_state[MAX_STRING_LENGTH];
            //char *state;
            //int i, length;
            
            sprintf( buf, "-%-5s to [%5d] Key: [%5d] ",
                capitalize(dir_name[door]),
                pexit->u1.to_room ? pexit->u1.to_room->vnum : 0,      /* ROM OLC */
                pexit->key );
            strcat( buf1, buf );
            
            /*
            * Format up the exit info.
            * Capitalize all flags that are not part of the reset info.
            */
	    sprintf( reset_state, " Exit flags: [%s] [%s]\n\r", 
            flag_bits_name(exit_flags, pexit->rs_flags),
            flag_bits_name(exit_flags, pexit->exit_info));
	    strcat( buf1, reset_state );
            
            if ( pexit->keyword && pexit->keyword[0] != '\0' )
            {
                sprintf( buf, "Kwds: [%s]\n\r", pexit->keyword );
                strcat( buf1, buf );
            }
            if ( pexit->description && pexit->description[0] != '\0' )
            {
                sprintf( buf, "%s", pexit->description );
                strcat( buf1, buf );
            }
        }
    }
    
    send_to_char( buf1, ch );
    
    sprintf( buf, "Comments:\n\r%s", pRoom->comments );
    send_to_char( buf, ch );

    if ( pRoom->rprogs )
    {
        PROG_LIST *list;
        int cnt;

        sprintf(buf, "\n\rROOMPrograms for [%5d]:\n\r", pRoom->vnum);
        send_to_char( buf, ch );

        for (cnt=0, list=pRoom->rprogs; list; list=list->next)
        {
            if (cnt ==0)
            {
                send_to_char ( " Number Vnum Trigger Phrase\n\r", ch );
                send_to_char ( " ------ ---- ------- ------\n\r", ch );
            }

            sprintf(buf, "[%5d] %4d %7s %s\n\r", cnt,
                list->vnum,name_lookup(list->trig_type, rprog_flags),
                list->trig_phrase);
            send_to_char( buf, ch );
            cnt++;
        }
    }
    return FALSE;
}

/* returns the revers exit if there exists one that leads back to 
 * the given room in the revers direction */
EXIT_DATA* get_revers_exit( ROOM_INDEX_DATA *pRoom, int door, bool changed )
{
    ROOM_INDEX_DATA *pToRoom;
    int rev;

    if ( pRoom == NULL || pRoom->exit[door] == NULL )
	return NULL;

    pToRoom = pRoom->exit[door]->u1.to_room;     /* ROM OLC */
    rev = rev_dir[door];

    if ( pToRoom == NULL
	 || pToRoom->exit[rev] == NULL
	 || pToRoom->exit[rev]->u1.to_room != pRoom )
	return NULL;

    if ( changed )
	SET_BIT( pToRoom->area->area_flags, AREA_CHANGED );

    return pToRoom->exit[rev];
}

/* Local function. */
bool change_exit( CHAR_DATA *ch, const char *argument, int door )
{
    ROOM_INDEX_DATA *pRoom;
    EXIT_DATA *rev_exit;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH], arg2[MIL];
    int  value;
    
    EDIT_ROOM(ch, pRoom);
    
    /*
    * Set the exit flags, needs full argument.
    * ----------------------------------------
    */
    if ( (value = flag_lookup(argument, exit_flags)) != NO_FLAG )
    {
        if ( !pRoom->exit[door] )
        {
            send_to_char("Exit does not exist.\n\r",ch);
            return FALSE;
        }
        
        /*
        * This room.
        */
        
        /* currently all other flags depend on door flag being set */
        if ( !IS_SET(pRoom->exit[door]->rs_flags, EX_ISDOOR) &&
                value != EX_ISDOOR)
        {
            ptc(ch, "'door' flag must be set before any other flag.\n\r");
            return FALSE;
        }

        TOGGLE_BIT(pRoom->exit[door]->rs_flags,  value);
        
        /* Suggested bug fix by: Nebseni [nebseni@clandestine.bcn.net] */
        if ( !IS_SET(pRoom->exit[door]->rs_flags, EX_ISDOOR) )
        {
            flag_clear( pRoom->exit[door]->rs_flags );
            ptc(ch, "'door' flag removed, all other flags cleared.\n\r");
        }

        /* Don't toggle exit_info because it can be changed by players. */
        flag_copy( pRoom->exit[door]->exit_info, pRoom->exit[door]->rs_flags );
        
        /*
        * Connected room.
        */
        rev_exit = get_revers_exit( pRoom, door, TRUE );
        if ( rev_exit != NULL )
        {
            flag_copy( rev_exit->rs_flags, pRoom->exit[door]->rs_flags );
            flag_copy( rev_exit->exit_info, pRoom->exit[door]->exit_info );
        }
        
        send_to_char( "Exit flag toggled.\n\r", ch );
        return TRUE;
    }
    
    /*
    * Now parse the arguments.
    */
    argument = one_argument( argument, command );
    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );
    
    if ( command[0] == '\0' )	/* Move command. */
    {
        move_char( ch, door, TRUE );                    /* ROM OLC */
        return FALSE;
    }
    
    if ( command[0] == '?' )
    {
        do_help( ch, "EXIT" );
        return FALSE;
    }
    
    if ( !str_cmp( command, "delete" ) )
    {
        ROOM_INDEX_DATA *pToRoom;
        sh_int rev;                                     /* ROM OLC */
        
        if ( !pRoom->exit[door] )
        {
            send_to_char( "REdit:  Cannot delete a null exit.\n\r", ch );
            return FALSE;
        }
        
        /*
        * Remove ToRoom Exit.
        */
	rev_exit = get_revers_exit( pRoom, door, TRUE );
	/* still need rev and pToRoom here --Bobble */
        rev = rev_dir[door];
        pToRoom = pRoom->exit[door]->u1.to_room;       /* ROM OLC */
        
        if ( rev_exit != NULL )
        {
            free_exit( rev_exit );
            pToRoom->exit[rev] = NULL;
        }
        
        /*
        * Remove this exit.
        */
        free_exit( pRoom->exit[door] );
        pRoom->exit[door] = NULL;
        
        send_to_char( "Exit unlinked.\n\r", ch );
        return TRUE;
    }
    
    if ( !str_cmp( command, "link" ) )
    {
        EXIT_DATA *pExit;
        ROOM_INDEX_DATA *toRoom;
        
        if ( arg[0] == '\0' || !is_number( arg ) )
        {
            send_to_char( "Syntax:  [direction] link [vnum]\n\r", ch );
            return FALSE;
        }
        
        value = atoi( arg );
        
        if ( ! (toRoom = get_room_index( value )) )
        {
            send_to_char( "REdit:  Cannot link to non-existant room.\n\r", ch );
            return FALSE;
        }
        
        if ( !IS_BUILDER( ch, toRoom->area ) )
        {
            send_to_char( "REdit:  Cannot link to that area.\n\r", ch );
            return FALSE;
        }
        
        if ( toRoom->exit[rev_dir[door]] )
        {
            send_to_char( "REdit:  Remote side's exit already exists.\n\r", ch );
            return FALSE;
        }
        
        if ( !pRoom->exit[door] )
            pRoom->exit[door] = new_exit();
        
        
        pRoom->exit[door]->u1.to_room = toRoom;
        pRoom->exit[door]->orig_door = door;
        
        door                    = rev_dir[door];
        pExit                   = new_exit();
        pExit->u1.to_room       = pRoom;
        pExit->orig_door	= door;
        toRoom->exit[door]      = pExit;
	SET_BIT( toRoom->area->area_flags, AREA_CHANGED );

        send_to_char( "Two-way link established.\n\r", ch );
        return TRUE;
    }
    
    if ( !str_cmp( command, "dig" ) )
    {
        char buf[MAX_STRING_LENGTH];
        
        if ( arg[0] == '\0' || !is_number( arg ) )
        {
            send_to_char( "Syntax: [direction] dig <vnum>\n\r", ch );
            return FALSE;
        }
        
        redit_create( ch, arg );
        sprintf( buf, "link %s", arg );
        change_exit( ch, buf, door);
        return TRUE;
    }
    
    if ( !str_cmp( command, "room" ) )
    {
        ROOM_INDEX_DATA *toRoom;
        
        if ( arg[0] == '\0' || !is_number( arg ) )
        {
            send_to_char( "Syntax:  [direction] room [vnum]\n\r", ch );
            return FALSE;
        }
        
        value = atoi( arg );
        
        if ( !(toRoom = get_room_index( value )) )
        {
            send_to_char( "REdit:  Cannot link to non-existant room.\n\r", ch );
            return FALSE;
        }
        
        if ( !pRoom->exit[door] )
            pRoom->exit[door] = new_exit();
        
        pRoom->exit[door]->u1.to_room = toRoom;    /* ROM OLC */
        pRoom->exit[door]->orig_door = door;
        
        send_to_char( "One-way link established.\n\r", ch );
        return TRUE;
    }
    
    if ( !str_cmp( command, "key" ) )
    {
        OBJ_INDEX_DATA *key;
        
        if ( arg[0] == '\0'
	     || !is_number( arg )
	     || (strcmp(arg2, "") && strcmp(arg2, "oneway")) )
        {
            send_to_char( "Syntax:  [direction] key [vnum]\n\r", ch );
            send_to_char( "         [direction] key [vnum] oneway\n\r", ch );
            return FALSE;
        }
        
        if ( !pRoom->exit[door] )
        {
            send_to_char("Exit does not exist.\n\r",ch);
            return FALSE;
        }
        
        value = atoi( arg );
        
	if ( value != 0 )
	{
	    if ( !(key = get_obj_index( value )) )
	    {
		send_to_char( "REdit:  Key doesn't exist.\n\r", ch );
		return FALSE;
	    }
        
	    if ( key->item_type != ITEM_KEY && key->item_type != ITEM_ROOM_KEY )
	    {
		send_to_char( "REdit:  Object is not a key.\n\r", ch );
		return FALSE;
	    }
	}
        
        pRoom->exit[door]->key = value;

	if ( strcmp(arg2, "oneway") )
	{
	    /* revers exit too */
	    rev_exit = get_revers_exit( pRoom, door, TRUE );
	    if ( rev_exit != NULL )
	    {
		rev_exit->key = value;
	    }
	}
        
        send_to_char( "Exit key set.\n\r", ch );
        return TRUE;
    }
    
    if ( !str_cmp( command, "name" ) )
    {
        if ( arg[0] == '\0' )
        {
            send_to_char( "Syntax:  [direction] name [string]\n\r", ch );
            send_to_char( "         [direction] name none\n\r", ch );
            return FALSE;
        }
        
        if ( !pRoom->exit[door] )
        {
            send_to_char("Exit does not exist.\n\r",ch);
            return FALSE;
        }
        
        free_string( pRoom->exit[door]->keyword );
        
        if (str_cmp(arg,"none"))
            pRoom->exit[door]->keyword = str_dup( arg );
        else
            pRoom->exit[door]->keyword = str_dup( "" );
        
        send_to_char( "Exit name set.\n\r", ch );
        return TRUE;
    }
    
    if ( !str_prefix( command, "description" ) )
    {
        if ( arg[0] == '\0' )
        {
            if ( !pRoom->exit[door] )
            {
                send_to_char("Exit does not exist.\n\r",ch);
                return FALSE;
            }
            
            string_append( ch, &pRoom->exit[door]->description );
            return TRUE;
        }
        
        send_to_char( "Syntax:  [direction] desc\n\r", ch );
        return FALSE;
    }
    
    return FALSE;
}



REDIT( redit_north )
{
    if ( change_exit( ch, argument, DIR_NORTH ) )
        return TRUE;
    
    return FALSE;
}



REDIT( redit_south )
{
    if ( change_exit( ch, argument, DIR_SOUTH ) )
        return TRUE;
    
    return FALSE;
}



REDIT( redit_east )
{
    if ( change_exit( ch, argument, DIR_EAST ) )
        return TRUE;
    
    return FALSE;
}



REDIT( redit_west )
{
    if ( change_exit( ch, argument, DIR_WEST ) )
        return TRUE;
    
    return FALSE;
}



REDIT( redit_up )
{
    if ( change_exit( ch, argument, DIR_UP ) )
        return TRUE;
    
    return FALSE;
}



REDIT( redit_down )
{
    if ( change_exit( ch, argument, DIR_DOWN ) )
        return TRUE;
    
    return FALSE;
}

REDIT( redit_northeast )
{
    if ( change_exit( ch, argument, DIR_NORTHEAST ) )
        return TRUE;
    
    return FALSE;
}


REDIT( redit_southeast )
{
    if ( change_exit( ch, argument, DIR_SOUTHEAST ) )
        return TRUE;
    
    return FALSE;
}


REDIT( redit_southwest )
{
    if ( change_exit( ch, argument, DIR_SOUTHWEST ) )
        return TRUE;
    
    return FALSE;
}


REDIT( redit_northwest )
{
    if ( change_exit( ch, argument, DIR_NORTHWEST ) )
        return TRUE;
    
    return FALSE;
}




REDIT( redit_ed )
{
    ROOM_INDEX_DATA *pRoom;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];
    
    EDIT_ROOM(ch, pRoom);
    
    argument = one_argument( argument, command );
    one_argument( argument, keyword );
    
    if ( command[0] == '\0' || keyword[0] == '\0' )
    {
        send_to_char( "Syntax:  ed add [keyword]\n\r", ch );
        send_to_char( "         ed edit [keyword]\n\r", ch );
        send_to_char( "         ed delete [keyword]\n\r", ch );
        send_to_char( "         ed format [keyword]\n\r", ch );
        return FALSE;
    }
    
    if ( !str_cmp( command, "add" ) )
    {
        if ( keyword[0] == '\0' )
        {
            send_to_char( "Syntax:  ed add [keyword]\n\r", ch );
            return FALSE;
        }
        
        ed			=   new_extra_descr();
        ed->keyword		=   str_dup( keyword );
        ed->description		=   str_dup( "" );
        ed->next		=   pRoom->extra_descr;
        pRoom->extra_descr	=   ed;
        
        string_append( ch, &ed->description );
        
        return TRUE;
    }
    
    
    if ( !str_cmp( command, "edit" ) )
    {
        if ( keyword[0] == '\0' )
        {
            send_to_char( "Syntax:  ed edit [keyword]\n\r", ch );
            return FALSE;
        }
        
        for ( ed = pRoom->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
        }
        
        if ( !ed )
        {
            send_to_char( "REdit:  Extra description keyword not found.\n\r", ch );
            return FALSE;
        }
        
        string_append( ch, &ed->description );
        
        return TRUE;
    }
    
    
    if ( !str_cmp( command, "delete" ) )
    {
        EXTRA_DESCR_DATA *ped = NULL;
        
        if ( keyword[0] == '\0' )
        {
            send_to_char( "Syntax:  ed delete [keyword]\n\r", ch );
            return FALSE;
        }
        
        for ( ed = pRoom->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
            ped = ed;
        }
        
        if ( !ed )
        {
            send_to_char( "REdit:  Extra description keyword not found.\n\r", ch );
            return FALSE;
        }
        
        if ( !ped )
            pRoom->extra_descr = ed->next;
        else
            ped->next = ed->next;
        
        free_extra_descr( ed );
        
        send_to_char( "Extra description deleted.\n\r", ch );
        return TRUE;
    }
    
    
    if ( !str_cmp( command, "format" ) )
    {
        if ( keyword[0] == '\0' )
        {
            send_to_char( "Syntax:  ed format [keyword]\n\r", ch );
            return FALSE;
        }
        
        for ( ed = pRoom->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
        }
        
        if ( !ed )
        {
            send_to_char( "REdit:  Extra description keyword not found.\n\r", ch );
            return FALSE;
        }
        
        ed->description = format_string( ed->description );
        
        send_to_char( "Extra description formatted.\n\r", ch );
        return TRUE;
    }
    
    redit_ed( ch, "" );
    return FALSE;
}

REDIT( redit_delete )
{
    ROOM_INDEX_DATA *pRoom=NULL;
    char command[MIL];

    argument = one_argument(argument, command);

    if (!is_number(command))
    {
        send_to_char( "Syntax : redit delete [vnum]\n\r", ch);
        return FALSE;
    }

    int vnum1=atoi(command);

    if ( (pRoom = get_room_index(vnum1)) == NULL )
    {
        send_to_char("Room does not exist.\n\r", ch );
        return FALSE;
    }

    AREA_DATA *ad = get_vnum_area( vnum1 );

    if ( ad == NULL )
    {
        send_to_char("Vnum not assigned to an area.\n\r", ch );
        return FALSE;
    }

    if ( !IS_BUILDER(ch,ad) )
    {
        send_to_char( "Insufficient security to delete room.\n\r", ch );
        return FALSE;
    }

    /* should be empty first */
    if ( pRoom->contents || pRoom->people )
    {
        send_to_char( "Room is not empty.\n\r", ch );
        return FALSE;
    }

    /* check for resets */
    if ( pRoom->reset_first )
    {
        send_to_char( "Please remove all resets first.\n\r", ch );
        return FALSE;
    }

    /* check for progs */
    if ( pRoom->rprogs )
    {
        send_to_char( "Please remove all rprogs first.\n\r", ch );
        return FALSE;
    }

    /* check for links */
    AREA_DATA *a;
    for ( a=area_first ; a ; a=a->next )
    {
        ROOM_INDEX_DATA *r;
        int vnum;
        for ( vnum=a->min_vnum ; vnum <= a->max_vnum ; vnum++ )
        {
            r=get_room_index(vnum);
            if (!r)
                continue;

            int i;
            EXIT_DATA *ex;
            for ( i=0 ; i<10 ; i++ )
            {
                ex=r->exit[i];
                if (!ex)
                    continue;
                if (ex->u1.to_room == pRoom)
                {
                    ptc( ch, "Can't delete room %d, room %d still links to it.\n\r", pRoom->vnum, r->vnum );
                    return FALSE;
                }
            }
        }
    }

    /* and portals for good measure */
    for ( a=area_first ; a ; a=a->next )
    {
        OBJ_INDEX_DATA *oid;
        int vnum;
        for ( vnum=a->min_vnum ; vnum <= a->max_vnum ; vnum++ )
        {
            oid=get_obj_index(vnum);
            if (!oid)
                continue;

            if (oid->item_type != ITEM_PORTAL )
                continue;

            if (oid->value[3] == pRoom->vnum)
            {
                ptc( ch, "Can't delete room %d, object %d links to it.\n\r", pRoom->vnum, oid->vnum );
                return FALSE;
            }
        }
    }

    if (is_being_edited(pRoom))
    {
        send_to_char( "Can't delete room, it is being edited.\n\r", ch );
        return FALSE;
    }

    /* if we're here, it's ok to delete */
    int iHash=vnum1 % MAX_KEY_HASH;

    ROOM_INDEX_DATA *curr, *last=NULL;

    for ( curr=room_index_hash[iHash]; curr; curr=curr->next )
    {
        if ( curr == pRoom )
        {
            if ( !last )
            {
                room_index_hash[iHash]=curr->next;
            }
            else
            {
                last->next=curr->next;
            }

            SET_BIT( ad->area_flags, AREA_CHANGED );
            clone_warning( ch, ad );
            free_room_index( pRoom );
            send_to_char( "Room deleted.\n\r", ch );
            return TRUE;
        }

        last=curr;
    }
    // should not be possible to reach this
    return FALSE;    
}


REDIT( redit_create )
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom=NULL;
    int value;
    int iHash;
    
    //EDIT_ROOM(ch, pRoom);
    
    value = atoi( argument );
    
    if ( argument[0] == '\0' || value <= 0 )
    {
        send_to_char( "Syntax:  create [vnum > 0]\n\r", ch );
        return FALSE;
    }
    
    pArea = get_vnum_area( value );
    if ( !pArea )
    {
        send_to_char( "REdit:  That vnum is not assigned an area.\n\r", ch );
        return FALSE;
    }
    
    if ( !IS_BUILDER( ch, pArea ) )
    {
        send_to_char( "REdit:  Vnum in an area you cannot build in.\n\r", ch );
        return FALSE;
    }
    
    if ( get_room_index( value ) )
    {
        send_to_char( "REdit:  Room vnum already exists.\n\r", ch );
        return FALSE;
    }
    
    pRoom			= new_room_index();
    pRoom->area			= pArea;
    pRoom->vnum			= value;
    
    if ( value > top_vnum_room )
        top_vnum_room = value;
    
    iHash			= value % MAX_KEY_HASH;
    pRoom->next			= room_index_hash[iHash];
    room_index_hash[iHash]	= pRoom;
    ch->desc->pEdit		= (void *)pRoom;
    
    send_to_char( "Room created.\n\r", ch );
    return TRUE;
}



REDIT( redit_name )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  name [name]\n\r", ch );
        return FALSE;
    }
    
    free_string( pRoom->name );
    pRoom->name = str_dup( argument );
    
    send_to_char( "Name set.\n\r", ch );
    return TRUE;
}

REDIT( redit_comments )
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if ( argument[0] == '\0' )
    {
        string_append( ch, &pRoom->comments );
        return TRUE;
    }

    send_to_char( "Syntax:  comments\n\r", ch );
    return FALSE;
}

REDIT( redit_desc )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    if ( argument[0] == '\0' )
    {
        string_append( ch, &pRoom->description );
        return TRUE;
    }
    
    send_to_char( "Syntax:  desc\n\r", ch );
    return FALSE;
}

REDIT( redit_heal )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    if (is_number(argument))
    {
        pRoom->heal_rate = atoi ( argument );
        send_to_char ( "Heal rate set.\n\r", ch);
        return TRUE;
    }
    
    send_to_char ( "Syntax : heal <#xnumber>\n\r", ch);
    return FALSE;
}       

REDIT( redit_mana )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    if (is_number(argument))
    {
        pRoom->mana_rate = atoi ( argument );
        send_to_char ( "Mana rate set.\n\r", ch);
        return TRUE;
    }
    
    send_to_char ( "Syntax : mana <#xnumber>\n\r", ch);
    return FALSE;
}       

REDIT( redit_clan )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    pRoom->clan = clan_lookup(argument);
    
    ptc( ch, "Clan set to %s.\n\r", clan_table[pRoom->clan].name );
    return TRUE;
}

REDIT( redit_clan_rank )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    pRoom->clan_rank = clan_rank_lookup(pRoom->clan, argument);
    
    if ( pRoom->clan_rank == 0 )
        ptc( ch, "Clan Rank set to none.\n\r" );
    else
        ptc( ch, "Clan Rank set to %s.\n\r", clan_table[pRoom->clan].rank_list[pRoom->clan_rank] );  
    return TRUE;
}

REDIT( redit_format )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    pRoom->description = format_string( pRoom->description );
    
    send_to_char( "String formatted.\n\r", ch );
    return TRUE;
}



REDIT( redit_mreset )
{
    ROOM_INDEX_DATA	*pRoom;
    MOB_INDEX_DATA	*pMobIndex;
    CHAR_DATA		*newmob;
    char		arg [ MAX_INPUT_LENGTH ];
    char		arg2 [ MAX_INPUT_LENGTH ];
    
    RESET_DATA		*pReset;
    char		output [ MAX_STRING_LENGTH ];
    
    EDIT_ROOM(ch, pRoom);
    
    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    
    if ( arg[0] == '\0' || !is_number( arg ) )
    {
        send_to_char ( "Syntax:  mreset <vnum> <max #x> <mix #x>\n\r", ch );
        return FALSE;
    }
    
    if ( !( pMobIndex = get_mob_index( atoi( arg ) ) ) )
    {
        send_to_char( "REdit: No mobile has that vnum.\n\r", ch );
        return FALSE;
    }
    
    if ( pMobIndex->area != pRoom->area )
    {
        send_to_char( "REdit: No such mobile in this area.\n\r", ch );
        return FALSE;
    }
    
    /*
    * Create the mobile reset.
    */
    pReset              = new_reset_data();
    pReset->command	= 'M';
    pReset->arg1	= pMobIndex->vnum;
    pReset->arg2	= is_number( arg2 ) ? atoi( arg2 ) : MAX_MOB;
    pReset->arg3	= pRoom->vnum;
    pReset->arg4	= is_number( argument ) ? atoi (argument) : 1;
    add_reset( pRoom, pReset, 0/* Last slot*/ );
    
    /*
    * Create the mobile.
    */
    newmob = create_mobile( pMobIndex );
    char_to_room( newmob, pRoom );
    
    sprintf( output, "%s (%d) has been loaded and added to resets.\n\r"
        "There will be a maximum of %d loaded to this room.\n\r",
        capitalize( pMobIndex->short_descr ),
        pMobIndex->vnum,
        pReset->arg2 );
    send_to_char( output, ch );
    act( "$n has created $N!", ch, NULL, newmob, TO_ROOM );
    return TRUE;
}



struct wear_type
{
    int	wear_loc;
    int	wear_bit;
};



const struct wear_type wear_table[] =
{
    {   WEAR_NONE,  ITEM_CARRY      },
    {	WEAR_LIGHT,	ITEM_LIGHT		},
    {	WEAR_FINGER_L,	ITEM_WEAR_FINGER	},
    {	WEAR_FINGER_R,	ITEM_WEAR_FINGER	},
    {	WEAR_NECK_1,	ITEM_WEAR_NECK		},
    {	WEAR_NECK_2,	ITEM_WEAR_NECK		},
    {	WEAR_TORSO,	ITEM_WEAR_TORSO		},
    {	WEAR_HEAD,	ITEM_WEAR_HEAD		},
    {	WEAR_LEGS,	ITEM_WEAR_LEGS		},
    {	WEAR_FEET,	ITEM_WEAR_FEET		},
    {	WEAR_HANDS,	ITEM_WEAR_HANDS		},
    {	WEAR_ARMS,	ITEM_WEAR_ARMS		},
    {	WEAR_SHIELD,	ITEM_WEAR_SHIELD	},
    {	WEAR_ABOUT,	ITEM_WEAR_ABOUT		},
    {	WEAR_WAIST,	ITEM_WEAR_WAIST		},
    {	WEAR_WRIST_L,	ITEM_WEAR_WRIST		},
    {	WEAR_WRIST_R,	ITEM_WEAR_WRIST		},
    {	WEAR_WIELD,	ITEM_WIELD		},
    {	WEAR_HOLD,	ITEM_HOLD		},
    {	NO_FLAG,	NO_FLAG			}
};



/*****************************************************************************
Name:		wear_loc
Purpose:	Returns the location of the bit that matches the count.
1 = first match, 2 = second match etc.
Called by:	oedit_reset(olc_act.c).
****************************************************************************/
/*
int wear_loc(int bits, int count)
{
    int flag;
    
    for (flag = 0; wear_table[flag].wear_bit != NO_FLAG; flag++)
    {
        if ( IS_SET(bits, wear_table[flag].wear_bit) && --count < 1)
            return wear_table[flag].wear_loc;
    }
    
    return NO_FLAG;
}
*/


/*****************************************************************************
Name:		wear_bit
Purpose:	Converts a wear_loc into a bit.
Called by:	redit_oreset(olc_act.c).
****************************************************************************/
int wear_bit(int loc)
{
    int flag;
    
    for (flag = 0; wear_table[flag].wear_loc != NO_FLAG; flag++)
    {
        if ( loc == wear_table[flag].wear_loc )
            return wear_table[flag].wear_bit;
    }
    
    return 0;
}



REDIT( redit_oreset )
{
    ROOM_INDEX_DATA	*pRoom;
    OBJ_INDEX_DATA	*pObjIndex;
    OBJ_DATA		*newobj;
    OBJ_DATA		*to_obj;
    CHAR_DATA		*to_mob;
    char		arg1 [ MAX_INPUT_LENGTH ];
    char		arg2 [ MAX_INPUT_LENGTH ];
    int			olevel = 0;
    
    RESET_DATA		*pReset;
    char		output [ MAX_STRING_LENGTH ];
    
    EDIT_ROOM(ch, pRoom);
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' || !is_number( arg1 ) )
    {
        send_to_char ( "Syntax:  oreset <vnum> <args>\n\r", ch );
        send_to_char ( "        -no_args               = into room\n\r", ch );
        send_to_char ( "        -<obj_name>            = into obj\n\r", ch );
        send_to_char ( "        -<mob_name> <wear_loc> = into mob\n\r", ch );
        return FALSE;
    }
    
    if ( !( pObjIndex = get_obj_index( atoi( arg1 ) ) ) )
    {
        send_to_char( "REdit: No object has that vnum.\n\r", ch );
        return FALSE;
    }
    
    if ( pObjIndex->area != pRoom->area )
    {
        send_to_char( "REdit: No such object in this area.\n\r", ch );
        return FALSE;
    }
    
    /*
    * Load into room.
    */
    if ( arg2[0] == '\0' )
    {
        pReset		= new_reset_data();
        pReset->command	= 'O';
        pReset->arg1	= pObjIndex->vnum;
        pReset->arg2	= 0;
        pReset->arg3	= pRoom->vnum;
        pReset->arg4	= 0;
        add_reset( pRoom, pReset, 0/* Last slot*/ );
        
        newobj = create_object(pObjIndex);
        obj_to_room( newobj, pRoom );
        
        sprintf( output, "%s (%d) has been loaded and added to resets.\n\r",
            capitalize( pObjIndex->short_descr ),
            pObjIndex->vnum );
        send_to_char( output, ch );
    }
    else
    /*
    * Load into object's inventory.
    */
    if ( argument[0] == '\0'
        && ( ( to_obj = get_obj_list( ch, arg2, pRoom->contents ) ) != NULL ) )
    {
        pReset		= new_reset_data();
        pReset->command	= 'P';
        pReset->arg1	= pObjIndex->vnum;
        pReset->arg2	= 0;
        pReset->arg3	= to_obj->pIndexData->vnum;
        pReset->arg4	= 1;
        add_reset( pRoom, pReset, 0/* Last slot*/ );
        
        newobj = create_object(pObjIndex);
        newobj->cost = 0;
        obj_to_obj( newobj, to_obj );
        
        sprintf( output, "%s (%d) has been loaded into "
            "%s (%d) and added to resets.\n\r",
            capitalize( newobj->short_descr ),
            newobj->pIndexData->vnum,
            to_obj->short_descr,
            to_obj->pIndexData->vnum );
        send_to_char( output, ch );
    }
    else
    /*
    * Load into mobile's inventory.
    */
    if ( ( to_mob = get_char_room( ch, arg2 ) ) != NULL )
    {
        int	wear_loc;
        
        /*
        * Make sure the location on mobile is valid.
        */
        if ( (wear_loc = flag_lookup(argument, wear_loc_flags)) == NO_FLAG )
        {
            send_to_char( "REdit: Invalid wear_loc.  '? wear-loc'\n\r", ch );
            return FALSE;
        }
        
        /*
        * Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
        */
        if ( pObjIndex->wear_type != wear_bit(wear_loc) )
        {
            sprintf( output,
                "%s (%d) has wear type: [%s]\n\r",
                capitalize( pObjIndex->short_descr ),
                pObjIndex->vnum,
                wear_bit_name(pObjIndex->wear_type) );
            send_to_char( output, ch );
            return FALSE;
        }
        
        /*
        * Can't load into same position.
        */
        if ( get_eq_char( to_mob, wear_loc ) )
        {
            send_to_char( "REdit:  Object already equipped.\n\r", ch );
            return FALSE;
        }
        
        pReset		= new_reset_data();
        pReset->arg1	= pObjIndex->vnum;
        pReset->arg2	= wear_loc;
        if ( pReset->arg2 == WEAR_NONE )
            pReset->command = 'G';
        else
            pReset->command = 'E';
        pReset->arg3	= wear_loc;
        
        add_reset( pRoom, pReset, 0/* Last slot*/ );
        
        olevel  = URANGE( 0, to_mob->level - 2, LEVEL_HERO );
        
        if ( to_mob->pIndexData->pShop )	/* Shop-keeper? */
        {
            switch ( pObjIndex->item_type )
            {
            default:		olevel = 0;				break;
            case ITEM_PILL:	olevel = number_range(  0, 10 );	break;
            case ITEM_POTION:	olevel = number_range(  0, 10 );	break;
            case ITEM_SCROLL:	olevel = number_range(  5, 15 );	break;
            case ITEM_WAND:	olevel = number_range( 10, 20 );	break;
            case ITEM_STAFF:	olevel = number_range( 15, 25 );	break;
            case ITEM_ARMOR:	olevel = number_range(  5, 15 );	break;
            case ITEM_WEAPON:	if ( pReset->command == 'G' )
                                    olevel = number_range( 5, 15 );
                else
                    olevel = number_fuzzy( olevel );
                break;
            }
            
            newobj = create_object(pObjIndex);
            if ( pReset->arg2 == WEAR_NONE )
                SET_BIT( newobj->extra_flags, ITEM_INVENTORY );
        }
        else
            newobj = create_object(pObjIndex);
        
        obj_to_char( newobj, to_mob );
        if ( pReset->command == 'E' )
            equip_char( to_mob, newobj, pReset->arg3 );
        
        sprintf( output, "%s (%d) has been loaded "
            "%s of %s (%d) and added to resets.\n\r",
            capitalize( pObjIndex->short_descr ),
            pObjIndex->vnum,
            flag_bit_name(wear_loc_strings, pReset->arg3),
            to_mob->short_descr,
            to_mob->pIndexData->vnum );
        send_to_char( output, ch );
    }
    else	/* Display Syntax */
    {
        send_to_char( "REdit:  That mobile isn't here.\n\r", ch );
        return FALSE;
    }
    
    act( "$n has created $p!", ch, newobj, NULL, TO_ROOM );
    return TRUE;
}



/*
* Object Editor Functions.
*/
void show_obj_values( CHAR_DATA *ch, OBJ_INDEX_DATA *obj )
{
    char buf[MAX_STRING_LENGTH];
    
    switch( obj->item_type )
    {
    default:	/* No values. */
        break;
        
    case ITEM_LIGHT:
        if ( obj->value[2] == -1 || obj->value[2] == 999 ) /* ROM OLC */
            sprintf( buf, "[v2] Light Hours:  Infinite[-1]\n\r" );
        else
            sprintf( buf, "[v2] Light Hours:  [%d]\n\r", obj->value[2] );
        send_to_char( buf, ch );
        break;

    case ITEM_ARROWS:
        sprintf( buf,
		 "[v0] Amount:         [%d]\n\r"
		 "[v1] Damage:         [%d]\n\r"
		 "[v2] Damage Type:    %s\n\r",
		 obj->value[0],
		 obj->value[1],
		 flag_bit_name(damage_type, obj->value[2]) );
        send_to_char( buf, ch );
        break;
        
    case ITEM_WAND:
    case ITEM_STAFF:
        sprintf( buf,
            "[v0] Spell Level:    [%d]\n\r"
            "[v1] Charges Total:  [%d]\n\r"
            "[v2] Charges Left:   [%d]\n\r"
            "[v3] Spell:          %s\n\r",
            obj->value[0],
            obj->value[1],
            obj->value[2],
            obj->value[3] != -1 ? skill_table[obj->value[3]].name
            : "reserved" );
        send_to_char( buf, ch );
        break;
        
    case ITEM_PORTAL:
        sprintf( buf,
            "[v0] Charges:        [%d]\n\r"
            "[v1] Exit Flags:     %s\n\r"
            "[v2] Portal Flags:   %s\n\r"
            "[v3] Goes to (vnum): [%d]\n\r",
            obj->value[0],
            i_flag_bits_name(exit_flags, obj->value[1]),
            i_flag_bits_name(portal_flags, obj->value[2]),
            obj->value[3] );
        send_to_char( buf, ch);
        break;
        
    case ITEM_FURNITURE:          
        sprintf( buf,
            "[v0] Max people:      [%d]\n\r"
            "[v1] Max weight:      [%d] (unused in game)\n\r"
            "[v2] Furniture Flags: %s\n\r"
            "[v3] Heal bonus:      [%d]\n\r"
            "[v4] Mana bonus:      [%d]\n\r",
            obj->value[0],
            obj->value[1],
            i_flag_bits_name(furniture_flags, obj->value[2]),
            obj->value[3],
            obj->value[4] );
        send_to_char( buf, ch );
        break;
        
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
        sprintf( buf,
            "[v0] Spell Level:  [%d]\n\r"
            "[v1] Spell:     %s\n\r"
            "[v2] Spell:     %s\n\r"
            "[v3] Spell:     %s\n\r"
            "[v4] Spell:     %s\n\r",
            obj->value[0],
            obj->value[1] != -1 ? skill_table[obj->value[1]].name
            : "reserved",
            obj->value[2] != -1 ? skill_table[obj->value[2]].name
            : "reserved",
            obj->value[3] != -1 ? skill_table[obj->value[3]].name
            : "reserved",
            obj->value[4] != -1 ? skill_table[obj->value[4]].name
            : "reserved" );
        send_to_char( buf, ch );
        break;
        
    /* ARMOR for ROM */
    case ITEM_ARMOR:
        sprintf( buf,
            "[v0] Ac              [%d]\n\r",
            obj->value[0] );
        send_to_char( buf, ch );
        break;
        
        /* WEAPON changed in ROM: */
        /* I had to split the output here, I have no idea why, but it helped -- Hugin */
        /* It somehow fixed a bug in showing scroll/pill/potions too ?! */
    case ITEM_WEAPON:
        sprintf( buf, "[v0] Weapon class:   %s\n\r",
	    flag_bit_name(weapon_class, obj->value[0]) );
        send_to_char( buf, ch );
        sprintf( buf, "[v1] Number of dice: [%d]\n\r", obj->value[1] );
        send_to_char( buf, ch );
        sprintf( buf, "[v2] Type of dice:   [%d]\n\r", obj->value[2] );
        send_to_char( buf, ch );
        sprintf( buf, "[v3] Damage Type:    %s\n\r",
            attack_table[obj->value[3]].name );
        send_to_char( buf, ch );
        sprintf( buf, "[v4] Special type:   %s\n\r",
            i_flag_bits_name(weapon_type2, obj->value[4]) );
        send_to_char( buf, ch );
        break;
        
    case ITEM_CONTAINER:
        sprintf( buf,
            "[v0] Weight Capacity: [%d lbs]\n\r"
            "[v1] Flags:           [%s]\n\r"
            "[v2] Key:        %s [%d]\n\r"
            "[v3] Item Capacity    [%d]\n\r"
            "[v4] Weight Mult(%%)   [%d]\n\r",
            obj->value[0],
            i_flag_bits_name(container_flags, obj->value[1]),
            get_obj_index(obj->value[2])
            ? get_obj_index(obj->value[2])->short_descr
            : "none",
            obj->value[2],
            obj->value[3],
            obj->value[4] );
        send_to_char( buf, ch );
        break;
        
    case ITEM_DRINK_CON:
        sprintf( buf,
            "[v0] Liquid Total: [%d]\n\r"
            "[v1] Liquid Left:  [%d]\n\r"
            "[v2] Liquid Type:  %s\n\r"
            "[v3] Poisoned:     %s\n\r",
            obj->value[0],
            obj->value[1],
            liq_table[obj->value[2]].liq_name,
            obj->value[3] != 0 ? "Yes" : "No" );
        send_to_char( buf, ch );
        break;
        
    case ITEM_FOUNTAIN:
        sprintf( buf,
            "[v0] Liquid Total: [%d]\n\r"
            "[v1] Liquid Left:  [%d]\n\r"
            "[v2] Liquid Type:  %s\n\r",
            obj->value[0],
            obj->value[1],
            liq_table[obj->value[2]].liq_name );
        send_to_char( buf,ch );
        break;
        
    case ITEM_FOOD:
        sprintf( buf,
            "[v0] Food hours: [%d]\n\r"
            "[v1] Full hours: [%d]\n\r"
            "[v3] Poisoned:   %s\n\r",
            obj->value[0],
            obj->value[1],
            obj->value[3] != 0 ? "Yes" : "No" );
        send_to_char( buf, ch );
        break;
        
    case ITEM_MONEY:
        sprintf( buf, "[v0] Silver:   [%d]\n\r"
            "[v1] Gold:     [%d]\n\r",
            obj->value[0], obj->value[1]);
        send_to_char( buf, ch );
        break;

    case ITEM_EXPLOSIVE:
        sprintf( buf,
            "[v0] Number of Dice: [%d]\n\r"
            "[v1] Type of Dice: [%d]\n\r",
            obj->value[0],
            obj->value[1]);
        send_to_char( buf, ch );
        break;
    }
    
    return;
}



bool set_obj_values( CHAR_DATA *ch, OBJ_INDEX_DATA *pObj, int value_num, const char *argument)
{
    int value1;

    switch( pObj->item_type )
    {
    default:
        break;
        
    case ITEM_LIGHT:
        switch ( value_num )
        {
        default:
            do_help( ch, "ITEM_LIGHT" );
            return FALSE;
        case 2:
            send_to_char( "HOURS OF LIGHT SET.\n\r\n\r", ch );
            pObj->value[2] = atoi( argument );
            break;
        }
        break;
        
    case ITEM_ARROWS:
	switch ( value_num )
	{
	default:
	    do_help( ch, "ITEM_ARROWS" );
	    return FALSE;
	case 0:
	    send_to_char( "NUMBER OF ARROWS SET.\n\r\n\r", ch );
	    pObj->value[0] = atoi( argument );
	    break;
	case 1:
	    send_to_char( "DAMAGE SET.\n\r\n\r", ch );
	    pObj->value[1] = atoi( argument );
	    break;
	case 2:
	    if ( (value1 = flag_lookup(argument, damage_type)) != NO_FLAG )
	    {
		send_to_char( "DAMAGE TYPE SET.\n\r\n\r", ch );
		pObj->value[2] = value1;
	    }
	    break;
	}
	break;

    case ITEM_WAND:
    case ITEM_STAFF:
	switch ( value_num )
        {
	default:
	    do_help( ch, "ITEM_STAFF_WAND" );
	    return FALSE;
	case 0:
	    send_to_char( "SPELL LEVEL SET.\n\r\n\r", ch );
	    pObj->value[0] = atoi( argument );
	    break;
	case 1:
	    send_to_char( "TOTAL NUMBER OF CHARGES SET.\n\r\n\r", ch );
	    pObj->value[1] = atoi( argument );
	    break;
	case 2:
	    send_to_char( "CURRENT NUMBER OF CHARGES SET.\n\r\n\r", ch );
	    pObj->value[2] = atoi( argument );
	    break;
	case 3:
	    send_to_char( "SPELL TYPE SET.\n\r", ch );
	    pObj->value[3] = spell_lookup( argument );
	    break;
	}
	break;
            
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
	switch ( value_num )
	{
	default:
	    do_help( ch, "ITEM_SCROLL_POTION_PILL" );
	    return FALSE;
	case 0:
	    send_to_char( "SPELL LEVEL SET.\n\r\n\r", ch );
	    pObj->value[0] = atoi( argument );
	    break;
	case 1:
	    send_to_char( "SPELL TYPE 1 SET.\n\r\n\r", ch );
	    pObj->value[1] = spell_lookup( argument );
	    break;
	case 2:
	    send_to_char( "SPELL TYPE 2 SET.\n\r\n\r", ch );
	    pObj->value[2] = spell_lookup( argument );
	    break;
	case 3:
	    send_to_char( "SPELL TYPE 3 SET.\n\r\n\r", ch );
	    pObj->value[3] = spell_lookup( argument );
	    break;
	case 4:
	    send_to_char( "SPELL TYPE 4 SET.\n\r\n\r", ch );
	    pObj->value[4] = spell_lookup( argument );
	    break;
	}
	break;
	
	/* ARMOR for ROM: */
    case ITEM_ARMOR:
	switch ( value_num )
	    {
	    default:
		do_help( ch, "ITEM_ARMOR" );
		return FALSE;
	    case 0:
		send_to_char( "AC SET.\n\r\n\r", ch );
		pObj->value[0] = atoi( argument );
		break;
	    }
	break;
	
	/* WEAPONS changed in ROM */
	
    case ITEM_WEAPON:
	switch ( value_num )
	{
	default:
	    do_help( ch, "ITEM_WEAPON" );
	    return FALSE;
	case 0:
	    if ( (value1 = flag_lookup(argument, weapon_class)) != NO_FLAG )
		{
		    send_to_char( "WEAPON CLASS SET.\n\r\n\r", ch );
		    pObj->value[0] = value1;
		}
	    break;
	case 1:
	    send_to_char( "NUMBER OF DICE SET.\n\r\n\r", ch );
	    pObj->value[1] = atoi( argument );
	    break;
	case 2:
	    send_to_char( "TYPE OF DICE SET.\n\r\n\r", ch );
	    pObj->value[2] = atoi( argument );
	    break;
	case 3:
	    send_to_char( "WEAPON TYPE SET.\n\r\n\r", ch );
	    pObj->value[3] = attack_lookup( argument );
	    break;
	case 4:
	    send_to_char( "SPECIAL WEAPON TYPE TOGGLED.\n\r\n\r", ch );
	    ALT_FLAGVALUE_TOGGLE( pObj->value[4], weapon_type2, argument );
	    break;
	}
	break;
	
    case ITEM_PORTAL:
	switch ( value_num )
	    {
	    default:
		do_help(ch, "ITEM_PORTAL" );
		return FALSE;
		
	    case 0:
		send_to_char( "CHARGES SET.\n\r\n\r", ch);
		pObj->value[0] = atoi ( argument );
		break;
	    case 1:
		send_to_char( "EXIT FLAGS SET.\n\r\n\r", ch);
		ALT_FLAGVALUE_TOGGLE( pObj->value[1], exit_flags, argument );
		break;
	    case 2:
		send_to_char( "PORTAL FLAGS SET.\n\r\n\r", ch);
		ALT_FLAGVALUE_TOGGLE( pObj->value[2], portal_flags, argument );
		break;
	    case 3:
		send_to_char( "EXIT VNUM SET.\n\r\n\r", ch);
		pObj->value[3] = atoi ( argument );
		break;
	    }
	break;
                                
    case ITEM_FURNITURE:
	switch ( value_num )
	    {
	    default:
		do_help( ch, "ITEM_FURNITURE" );
		return FALSE;
		
	    case 0:
		send_to_char( "NUMBER OF PEOPLE SET.\n\r\n\r", ch);
		pObj->value[0] = atoi ( argument );
		break;
	    case 1:
		send_to_char( "MAX WEIGHT SET.\n\r\n\r", ch);
		pObj->value[1] = atoi ( argument );
		break;
	    case 2:
		send_to_char( "FURNITURE FLAGS TOGGLED.\n\r\n\r", ch);
		ALT_FLAGVALUE_TOGGLE( pObj->value[2], furniture_flags, argument );
		break;
	    case 3:
		send_to_char( "HEAL BONUS SET.\n\r\n\r", ch);
		pObj->value[3] = atoi ( argument );
		break;
	    case 4:
		send_to_char( "MANA BONUS SET.\n\r\n\r", ch);
		pObj->value[4] = atoi ( argument );
		break;
	    }
	break;
	
    case ITEM_CONTAINER:
	switch ( value_num )
	    {
		int value;
		
	    default:
		do_help( ch, "ITEM_CONTAINER" );
		return FALSE;
	    case 0:
		send_to_char( "WEIGHT CAPACITY SET.\n\r\n\r", ch );
		pObj->value[0] = atoi( argument );
		break;
	    case 1:
		if ( (value = flag_lookup(argument, container_flags)) != NO_FLAG )
		    I_TOGGLE_BIT(pObj->value[1], value);
		else
		    {
			do_help ( ch, "ITEM_CONTAINER" );
			return FALSE;
		    }
		send_to_char( "CONTAINER TYPE SET.\n\r\n\r", ch );
		break;
	    case 2:
		if ( atoi(argument) != 0 )
		    {
			if ( !get_obj_index( atoi( argument ) ) )
			    {
				send_to_char( "THERE IS NO SUCH ITEM.\n\r\n\r", ch );
				return FALSE;
			    }
			
			if ( get_obj_index( atoi( argument ) )->item_type != ITEM_KEY )
			    {
				send_to_char( "THAT ITEM IS NOT A KEY.\n\r\n\r", ch );
				return FALSE;
			    }
		    }
		send_to_char( "CONTAINER KEY SET.\n\r\n\r", ch );
		pObj->value[2] = atoi( argument );
		break;
	    case 3:
		send_to_char( "CONTAINER MAX WEIGHT SET.\n\r", ch);
		pObj->value[3] = atoi( argument );
		break;
	    case 4:
		send_to_char( "WEIGHT MULTIPLIER SET.\n\r\n\r", ch );
		pObj->value[4] = atoi ( argument );
		break;
	    }
	break;
	
    case ITEM_DRINK_CON:
	switch ( value_num )
	    {
	    default:
		do_help( ch, "ITEM_DRINK" );
		/* OLC		    do_help( ch, "liquids" );    */
		return FALSE;
	    case 0:
		send_to_char( "MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch );
		pObj->value[0] = atoi( argument );
		break;
	    case 1:
		send_to_char( "CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch );
		pObj->value[1] = atoi( argument );
		break;
	    case 2:
		send_to_char( "LIQUID TYPE SET.\n\r\n\r", ch );
		pObj->value[2] = ( liq_lookup(argument) != -1 ?
				   liq_lookup(argument) : 0 );
		break;
	    case 3:
		send_to_char( "POISON VALUE TOGGLED.\n\r\n\r", ch );
		pObj->value[3] = ( pObj->value[3] == 0 ) ? 1 : 0;
		break;
	    }
	break;
	
    case ITEM_FOUNTAIN:
	switch (value_num)
	    {
	    default:
		do_help( ch, "ITEM_FOUNTAIN" );
		/* OLC		    do_help( ch, "liquids" );    */
		return FALSE;
	    case 0:
		send_to_char( "MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch );
		pObj->value[0] = atoi( argument );
		break;
	    case 1:
		send_to_char( "CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch );
		pObj->value[1] = atoi( argument );
		break;
	    case 2:
		send_to_char( "LIQUID TYPE SET.\n\r\n\r", ch );
		pObj->value[2] = ( liq_lookup( argument ) != -1 ?
				   liq_lookup( argument ) : 0 );
		break;
	    }
	break;
	
    case ITEM_FOOD:
	switch ( value_num )
	    {
	    default:
		do_help( ch, "ITEM_FOOD" );
		return FALSE;
	    case 0:
		send_to_char( "HOURS OF FOOD SET.\n\r\n\r", ch );
		pObj->value[0] = atoi( argument );
		break;
	    case 1:
		send_to_char( "HOURS OF FULL SET.\n\r\n\r", ch );
		pObj->value[1] = atoi( argument );
		break;
	    case 3:
		send_to_char( "POISON VALUE TOGGLED.\n\r\n\r", ch );
		pObj->value[3] = ( pObj->value[3] == 0 ) ? 1 : 0;
		break;
	    }
	break;
	
    case ITEM_MONEY:
	switch ( value_num )
	    {
	    default:
		do_help( ch, "ITEM_MONEY" );
		return FALSE;
	    case 1:
		send_to_char( "GOLD AMOUNT SET.\n\r\n\r", ch );
		pObj->value[1] = atoi( argument );
		break;
	    case 0:
		send_to_char( "SILVER AMOUNT SET.\n\r\n\r", ch );
		pObj->value[0] = atoi( argument );
		break;
	    }
	break;
    
    case ITEM_EXPLOSIVE:
    switch ( value_num )
        {
        default:
        do_help( ch, "ITEM_EXPLOSIVE" );
        return FALSE;
        case 0:
        send_to_char( "NUMBER OF DICE SET.\n\r\n\r", ch );
        pObj->value[0] = atoi( argument );
        break;
        case 1:
        send_to_char( "TYPE OF DICE SET.\n\r\n\r", ch );
        pObj->value[1] = atoi( argument );
        break;
        }
    break;
    }
    
    show_obj_values( ch, pObj );
    
    return TRUE;
}

#define MAX_RATING 6
char* get_rating_name( int rating )
{
    static char rating_name[MAX_RATING][100] =
    {
	"easy",
	"normal",
	"hard",
	"very hard",
	"extreme",
	"..."
    };

    if ( rating < 0 || rating >= MAX_RATING )
	return "?";
    else
	return rating_name[rating];
}

/*****************************************************************
 * Name: update_oprog_flags
 * Purpose: fix bug that removes valid oprog flags
 * Called by: oedit_deloprog
 *****************************************************************/
void update_oprog_flags( OBJ_INDEX_DATA *pObj )
{
    PROG_LIST *list;

    /* clear flags */
    flag_clear( pObj->oprog_flags );

    /* re-add all flags needed */
    for (list = pObj->oprogs; list != NULL; list = list->next)
        SET_BIT(pObj->oprog_flags, list->trig_type);
}

OEDIT ( oedit_deloprog )
{
    OBJ_INDEX_DATA *pObj;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char oprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_OBJ(ch, pObj);

    one_argument( argument, oprog );
    if (!is_number( oprog ) || oprog[0] == '\0' )
    {
        send_to_char("Syntax:  deloprog [#oprog]\n\r",ch);
        return FALSE;
    }

    value = atoi ( oprog );

    if ( value < 0 )
    {
        send_to_char("Only non-negative mprog-numbers allowed.\n\r",ch);
        return FALSE;
    }

    if ( !(list= pObj->oprogs) )
    {
        send_to_char("OEdit:  Non existant mprog.\n\r",ch);
        return FALSE;
    }

    if ( value == 0 )
    {
        list = pObj->oprogs;
        pObj->oprogs = list->next;
        free_oprog( list );
    }
    else
    {
        while ( (list_next = list->next) && (++cnt < value ) )
            list = list_next;

        if ( list_next )
        {
            list->next = list_next->next;
            free_oprog(list_next);
        }
        else
        {
            send_to_char("No such oprog.\n\r",ch);
            return FALSE;
        }
    }

    update_oprog_flags(pObj);

    send_to_char("Oprog removed.\n\r", ch);
    return TRUE;
}

OEDIT ( oedit_addoprog )
{
    int value;
    OBJ_INDEX_DATA *pObj;
    PROG_LIST *list;
    PROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char phrase[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];

    EDIT_OBJ(ch, pObj);
    argument=one_argument(argument, num);
    argument=one_argument(argument, trigger);
    argument=one_argument(argument, phrase);

    if (!is_number(num) || trigger[0] =='\0' || phrase[0] =='\0' )
    {
        send_to_char("Syntax:   addoprog [vnum] [trigger] [phrase]\n\r",ch);
        return FALSE;
    }

    if ( (value = flag_lookup(trigger, oprog_flags)) == NO_FLAG )
    {
        send_to_char("Valid flags are:\n\r",ch);
        show_help( ch, "oprog");
        return FALSE;
    }

    if ( ( code =get_oprog_index (atoi(num) ) ) == NULL)
    {
        send_to_char("No such OBJProgram.\n\r",ch);
        return FALSE;
    }

    if ( value == OTRIG_TIMER && IS_SET( pObj->oprog_flags, value ) )
    {
        send_to_char("Can only have one timer trigger.\n\r", ch);
        return FALSE;
    }

    list                  = new_oprog();
    list->vnum            = atoi(num);
    list->trig_type       = value;
    list->trig_phrase     = str_dup(phrase);
    list->script          = code;
    SET_BIT(pObj->oprog_flags,value);
    list->next            = pObj->oprogs;
    pObj->oprogs          = list;

    send_to_char( "Oprog Added.\n\r",ch);
    return TRUE;
}

OEDIT( oedit_show )
{
    OBJ_INDEX_DATA *pObj;
    PROG_LIST *list;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
    int cnt1;
    
    EDIT_OBJ(ch, pObj);
    
    sprintf( buf, "Name:        [%s]\n\rArea:        [%5d] %s\n\r",
        pObj->name,
        !pObj->area ? -1        : pObj->area->vnum,
        !pObj->area ? "No Area" : pObj->area->name );
    send_to_char( buf, ch );
    
    
    sprintf( buf, "Vnum:        [%5d]\n\rType:        [%s]\n\r",
        pObj->vnum,
        flag_bit_name(type_flags, pObj->item_type) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Level:       [%5d]\n\r", pObj->level );
    send_to_char( buf, ch );
    
    sprintf( buf, "Wear type:   [%s]\n\r",
        wear_bit_name(pObj->wear_type) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Extra flags: [%s]\n\r",
        extra_bits_name(pObj->extra_flags) );
    send_to_char( buf, ch );
    
    if (pObj->clan>0 || pObj->rank>0)
    {
        sprintf( buf, "Clan:        [%s]\n\r"
            "Clan Rank:   [%s]\n\r",
            clan_table[pObj->clan].name,
            clan_table[pObj->clan].rank_list[pObj->rank].name
            );
        send_to_char( buf, ch );
    }
    
    sprintf( buf, "Material:    [%s]\n\r",                /* ROM */
        pObj->material );
    send_to_char( buf, ch );
    
    sprintf( buf, "Weight:      [%5d]\n\rCost:        [%5d]\n\r",
        pObj->weight, pObj->cost );
    send_to_char( buf, ch );
    
    if (pObj->combine_vnum > 0)
    {
	sprintf( buf, "Combine:     [%5d]\n\r", pObj->combine_vnum );
	send_to_char( buf, ch );
    }

    if ( pObj->diff_rating != 0 || pObj->level >= 90 )
    {
	sprintf( buf, "Rating:      [%5d] (%s)\n\r", 
		 pObj->diff_rating, get_rating_name(pObj->diff_rating) );
	send_to_char( buf, ch );
    }

    /* Info about OPs to spend: */
    sprintf( buf, "OPs:         [%2d/%2d]\n\r",
        get_obj_index_ops(pObj), get_obj_index_spec(pObj, pObj->level) );
    send_to_char( buf, ch );

    if ( pObj->extra_descr )
    {
        EXTRA_DESCR_DATA *ed;
        
        send_to_char( "Ex desc kwd: ", ch );
        
        for ( ed = pObj->extra_descr; ed; ed = ed->next )
        {
            send_to_char( "[", ch );
            send_to_char( ed->keyword, ch );
            send_to_char( "]", ch );
        }
        
        send_to_char( "\n\r", ch );
    }
    
    sprintf( buf, "Short desc:  %s\n\rLong desc:\n\r     %s\n\r",
        pObj->short_descr, pObj->description );
    send_to_char( buf, ch );

    sprintf( buf, "Comments:\n\r%s", pObj->comments );
    send_to_char( buf, ch );
    
    for ( cnt1 = 0, paf = pObj->affected; paf; paf = paf->next )
    {
        if ( cnt1 == 0 )
        {
            send_to_char( "Number Modifier Affects\n\r", ch );
            send_to_char( "------ -------- -------\n\r", ch );
        }

        sprintf( buf, "[%4d] %-8d %s", cnt1,
            paf->modifier,
            flag_bit_name(apply_flags, paf->location) );
        send_to_char( buf, ch );

	if (paf->bitvector)
	  {
	    send_to_char( " & ", ch );
	    buf[0] = '\0';
	    switch(paf->where)
	      {
	      case TO_AFFECTS:
		sprintf(buf,"%s affect", affect_bit_name(paf->bitvector));
		break;
	      case TO_OBJECT:
		sprintf(buf,"%s object flag", extra_bit_name(paf->bitvector));
		break;
	      case TO_WEAPON:
		sprintf(buf,"%s weapon flag", weapon_bits_name(paf->bitvector));
		break;
	      case TO_IMMUNE:
		sprintf(buf,"immunity to %s", imm_bit_name(paf->bitvector));
		break;
	      case TO_RESIST:
		sprintf(buf,"resistance to %s", imm_bit_name(paf->bitvector));
		break;
	      case TO_VULN:
		sprintf(buf,"vulnerability to %s", imm_bit_name(paf->bitvector));
		break;
	      case TO_SPECIAL:
		  sprintf( buf, "special %d", paf->bitvector );
	      default:
		sprintf(buf,"bug: invalid where (%d)", paf->where);
		break;
	      }
	    send_to_char( buf, ch );
	  }

	if ( paf->detect_level != 0 )
	{
	    sprintf( buf, " (detect = %d)", paf->detect_level );
	    send_to_char( buf, ch );
	}

	send_to_char( "\n\r", ch );
        cnt1++;
    }
    
    show_obj_values( ch, pObj );


    if ( pObj->oprogs )
    {
        int cnt;

        sprintf(buf, "\n\rOBJPrograms for [%5d]:\n\r", pObj->vnum);
        send_to_char( buf, ch );

        for (cnt=0, list=pObj->oprogs; list; list=list->next)
        {
            if (cnt ==0)
            {
                send_to_char ( " Number Vnum Trigger Phrase\n\r", ch );
                send_to_char ( " ------ ---- ------- ------\n\r", ch );
            }

            sprintf(buf, "[%5d] %4d %7s %s\n\r", cnt,
                list->vnum,name_lookup(list->trig_type, oprog_flags),
                list->trig_phrase);
            send_to_char( buf, ch );
            cnt++;
        }
    }
    
    return FALSE;
}


/*
* Need to issue warning if flag isn't valid. -- does so now -- Hugin.
*/
OEDIT( oedit_addaffect )
{
    int value;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    char det[MSL];
    int detect_level = 0;
    
    EDIT_OBJ(ch, pObj);
    
    argument = one_argument( argument, loc );
    argument = one_argument( argument, mod );
    one_argument( argument, det );
    
    if ( loc[0] == '\0' || mod[0] == '\0' || !is_number( mod ) )
    {
        send_to_char( "Syntax:  addaffect [location] [#xmod] (#detect)\n\r", ch );
        return FALSE;
    }

    if ( (value = flag_lookup(loc, apply_flags)) == NO_FLAG ) /* Hugin */
    {
        send_to_char( "Valid affects are:\n\r", ch );
        show_help( ch, "apply" );
        return FALSE;
    }
    else
    {
        int idx = index_lookup(value, apply_flags);
        if ( !apply_flags[idx].settable )
        {
            ptc(ch, "Cannot set %s flag.\n\r", apply_flags[idx].name );
            return FALSE;
        }
    }
    
    if ( det[0] != '\0' )
    {
        if ( is_number( det ) )
            detect_level = atoi( det );
        else
        {
            send_to_char( "Detect level must be an integer.\n\r", ch );
            return FALSE;
        }
    }
    
    pAf             =   new_affect();
    pAf->location   =   value;
    pAf->modifier   =   atoi( mod );
    pAf->where	    =   TO_OBJECT;
    pAf->type       =   -1;
    pAf->duration   =   -1;
    pAf->bitvector  =   0;
    pAf->level      =	pObj->level;
    pAf->detect_level = detect_level;
    pObj->affected  =   affect_insert( pObj->affected, pAf );
    
    send_to_char( "Affect added.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_addapply )
{
    int bv,typ;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char type[MAX_STRING_LENGTH];
    char bvector[MAX_STRING_LENGTH];
    char buf[MSL];
    char det[MSL];
    int detect_level = 0;
    
    EDIT_OBJ(ch, pObj);
    
    argument = one_argument( argument, type );
    argument = one_argument( argument, bvector );
    one_argument( argument, det );

    if ( get_trust(ch) < L2 )
    {
      sprintf( buf, "You must be level %d to use addapply.\n\r", L2 );
      send_to_char( buf, ch );
      return FALSE;
    }

    if ( type[0] == '\0' )
    {
      send_to_char( "Syntax:  addapply [type] [bitvector] (#detect)\n\r", ch );
      return FALSE;
    }
    
    if ( (typ = flag_lookup(type, apply_types)) == NO_FLAG )
    {
        send_to_char( "Invalid apply type. Valid apply types are:\n\r", ch);
        show_help( ch, "apptype" );
        return FALSE;
    }
    else if ( !apply_types[typ].settable )
    {
        send_to_char( "Cannot set type.\n\r", ch );
        return FALSE;
    }
    
    if ( bvector[0] == '\0' || 
	 (bv = flag_lookup(bvector, bitvector_type[typ].table)) == NO_FLAG )
    {
        send_to_char( "Invalid bitvector type.\n\r", ch );
        send_to_char( "Valid bitvector types are:\n\r", ch );
        show_help( ch, bitvector_type[typ].help );
        return FALSE;
    }
    
    if ( det[0] != '\0' )
    {
        if ( is_number( det ) )
            detect_level = atoi( det );
        else
        {
            send_to_char( "Detect level must be an integer.\n\r", ch );
            return FALSE;
        }
    }

    pAf             =   new_affect();
    pAf->location   =   APPLY_NONE;
    pAf->modifier   =   0;
    pAf->where	    =   typ;
    pAf->type	    =	-1;
    pAf->duration   =   -1;
    pAf->bitvector  =   bv;
    pAf->level      =	pObj->level;
    pAf->detect_level = detect_level;
    pObj->affected  =   affect_insert(pObj->affected, pAf);
    
    send_to_char( "Apply added.\n\r", ch);
    return TRUE;
}

/*
* My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
* for really teaching me how to manipulate pointers.
*/
OEDIT( oedit_delaffect )
{
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    AFFECT_DATA *pAf_next;
    char affect[MAX_STRING_LENGTH];
    int  value;
    int  cnt = 0;
    
    EDIT_OBJ(ch, pObj);
    
    one_argument( argument, affect );
    
    if ( !is_number( affect ) || affect[0] == '\0' )
    {
        send_to_char( "Syntax:  delaffect [#xaffect]\n\r", ch );
        return FALSE;
    }
    
    value = atoi( affect );
    
    if ( value < 0 )
    {
        send_to_char( "Only non-negative affect-numbers allowed.\n\r", ch );
        return FALSE;
    }
    
    if ( !( pAf = pObj->affected ) )
    {
        send_to_char( "OEdit:  Non-existant affect.\n\r", ch );
        return FALSE;
    }
    
    if( value == 0 )	/* First case: Remove first affect */
    {
        pAf = pObj->affected;
        pObj->affected = pAf->next;
        free_affect( pAf );
    }
    else		/* Affect to remove is not the first */
    {
        while ( ( pAf_next = pAf->next ) && ( ++cnt < value ) )
            pAf = pAf_next;
        
        if( pAf_next )		/* See if it's the next affect */
        {
            pAf->next = pAf_next->next;
            free_affect( pAf_next );
        }
        else                                 /* Doesn't exist */
        {
            send_to_char( "No such affect.\n\r", ch );
            return FALSE;
        }
    }
    
    send_to_char( "Affect removed.\n\r", ch);
    return TRUE;
}



OEDIT( oedit_name )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  name [string]\n\r", ch );
        return FALSE;
    }
    
    free_string( pObj->name );
    pObj->name = str_dup( argument );
    
    send_to_char( "Name set.\n\r", ch);
    return TRUE;
}



OEDIT( oedit_short )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  short [string]\n\r", ch );
        return FALSE;
    }
    
    free_string( pObj->short_descr );
    pObj->short_descr = str_dup( argument );
    /*
    pObj->short_descr[0] = LOWER( pObj->short_descr[0] );
    */
    
    send_to_char( "Short description set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_clan )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    pObj->clan = clan_lookup(argument);
    
    ptc( ch, "Clan set to %s.\n\r", clan_table[pObj->clan].name );
    return TRUE;
}

OEDIT( oedit_rank )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    pObj->rank = clan_rank_lookup(pObj->clan, argument);
    
    if ( pObj->rank == 0 )
        ptc( ch, "Clan Rank set to none.\n\r" );
    else
        ptc( ch, "Clan Rank set to %s.\n\r", clan_table[pObj->clan].rank_list[pObj->rank] );
    return TRUE;
}



OEDIT( oedit_long )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  long [string]\n\r", ch );
        return FALSE;
    }
    
    free_string( pObj->description );
    pObj->description = upper_realloc(str_dup(argument));
    
    send_to_char( "Long description set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_comments)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if ( argument[0] == '\0' )
    {
        string_append( ch, &pObj->comments );
        return TRUE;
    }

    send_to_char( "Syntax:  comments   - line edit\n\r", ch );
    return FALSE;
}

bool set_value( CHAR_DATA *ch, OBJ_INDEX_DATA *pObj, const char *argument, int value )
{
    if ( argument[0] == '\0' )
    {
        set_obj_values( ch, pObj, -1, "" );     /* '\0' changed to "" -- Hugin */
        return FALSE;
    }
    
    if ( set_obj_values( ch, pObj, value, argument ) )
        return TRUE;
    
    return FALSE;
}



/*****************************************************************************
Name:		oedit_values
Purpose:	Finds the object and sets its value.
Called by:	The four valueX functions below. (now five -- Hugin )
****************************************************************************/
bool oedit_values( CHAR_DATA *ch, const char *argument, int value )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    if ( set_value( ch, pObj, argument, value ) )
        return TRUE;
    
    return FALSE;
}


OEDIT( oedit_value0 )
{
    if ( oedit_values( ch, argument, 0 ) )
        return TRUE;
    
    return FALSE;
}



OEDIT( oedit_value1 )
{
    if ( oedit_values( ch, argument, 1 ) )
        return TRUE;
    
    return FALSE;
}



OEDIT( oedit_value2 )
{
    if ( oedit_values( ch, argument, 2 ) )
        return TRUE;
    
    return FALSE;
}



OEDIT( oedit_value3 )
{
    if ( oedit_values( ch, argument, 3 ) )
        return TRUE;
    
    return FALSE;
}



OEDIT( oedit_value4 )
{
    if ( oedit_values( ch, argument, 4 ) )
        return TRUE;
    
    return FALSE;
}



OEDIT( oedit_weight )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        send_to_char( "Syntax:  weight [number]\n\r", ch );
        return FALSE;
    }
    
    pObj->weight = atoi( argument );
    
    send_to_char( "Weight set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_cost )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        send_to_char( "Syntax:  cost [number]\n\r", ch );
        return FALSE;
    }
    
    pObj->cost = atoi( argument );
    
    send_to_char( "Cost set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_combine )
{
    OBJ_INDEX_DATA *pObj;
    char buf[MIL];
    int vnum;

    EDIT_OBJ(ch, pObj);

    one_argument( argument, buf );
    if (buf[0] == '\0' || !is_number( buf ))
    {
      send_to_char( "Syntax: combine [#vnum of resulting object]\n\r", ch );
      send_to_char( "        combine 0\n\r", ch );
      return FALSE;
    }
    
    vnum = atoi( buf );
    
    if (vnum != 0 && !get_obj_index(vnum))
    {
      send_to_char( "That vnum doesn't exist.\n\r", ch );
      return FALSE;
    }

    pObj->combine_vnum = vnum;
    if (vnum == 0)
      send_to_char( "Combine removed.\n\r", ch );
    else
      send_to_char( "Combine set.\n\r", ch );
    
    return TRUE;
}

void show_ratings( CHAR_DATA *ch )
{
    char buf[MSL];
    int i;

    send_to_char( "The following ratings can be set:\n\r", ch );
    for ( i = 0; i < MAX_RATING; i++ )
    {
	sprintf( buf, "%d (%s)\n\r", i, get_rating_name(i) );
	send_to_char( buf, ch );
    }
}

OEDIT( oedit_rating )
{
    OBJ_INDEX_DATA *pObj;
    char buf[MIL];
    int value;

    EDIT_OBJ(ch, pObj);

    one_argument( argument, buf );

    if ( get_trust(ch) < L2 )
    {
	send_to_char( "You must be level 108 to rate the object's difficulty.\n\r", ch );
	return FALSE;
    }

    if (buf[0] == '\0' || !is_number( buf ))
    {
	send_to_char( "Syntax: rating [difficulty rating]\n\r", ch );
	show_ratings( ch );
	return FALSE;
    }
    
    value = atoi( buf );
    if ( value < 0 || value >= MAX_RATING )
    {
	show_ratings( ch );
	return FALSE;
    }

    /* set the rating */
    pObj->diff_rating = value;
    send_to_char( "Difficulty rating set.\n\r", ch );
    return TRUE;
}

OEDIT( oedit_delete )
{
    OBJ_INDEX_DATA *pObj;
    AREA_DATA *pArea;
    int value;
    int iHash;

    value = atoi( argument );
    if ( argument[0] == '\0' || value == 0 )
    {
        send_to_char( "Syntax:  oedit delete [vnum]\n\r", ch );
        return FALSE;
    }

    pArea = get_vnum_area( value );
    if ( !pArea )
    {
        send_to_char( "OEdit:  That vnum is not assigned an area.\n\r", ch );
        return FALSE;
    }

    if ( !IS_BUILDER( ch, pArea ) )
    {
        send_to_char( "OEdit:  Vnum in an area you cannot build in.\n\r", ch );
        return FALSE;
    }

    if ( (pObj = get_obj_index( value ) ) == NULL )
    {
        send_to_char( "OEdit:  No such object.\n\r", ch );
        return FALSE;
    }
   
    /* check for instances */
    OBJ_DATA *obj;
    for ( obj=object_list ; obj ; obj=obj->next )
    {
        if ( obj->pIndexData == pObj )
        {
            send_to_char( "Can't delete, instances exist.\n\r", ch );
            return FALSE;
        }
    }

    /* check for resets */
    ROOM_INDEX_DATA *room;
    RESET_DATA *rst;
    int rvnum;

    for ( rvnum=0 ; rvnum <= top_vnum_room ; rvnum++ )
    {
        if ( (room=get_room_index(rvnum) ) == NULL )
            continue;

        for ( rst=room->reset_first ; rst ; rst=rst->next )
        {
            switch (rst->command)
            {
                case 'O':
                case 'P':
                case 'G':
                case 'E':
                    break;
                default:
                    continue;
            }

            if ( rst->arg1 == value )
            {
                send_to_char( "OEdit:  Can't delete, resets exist.\n\r", ch );
                return FALSE;
            } 
        }
    }

    if (is_being_edited(pObj))
    {
        send_to_char( "Can't delete obj, it is being edited.\n\r", ch );
        return FALSE;
    }

    /* got here means we're good to delete */
    iHash=value % MAX_KEY_HASH;

    OBJ_INDEX_DATA *curr, *last=NULL;

    for ( curr=obj_index_hash[iHash]; curr; curr=curr->next )
    {
        if ( curr == pObj )
        {
            if ( !last )
            {
                obj_index_hash[iHash]=curr->next;
            }
            else
            {
                last->next=curr->next;
            }

            free_obj_index( pObj );
            send_to_char( "Object deleted.\n\r", ch );
            return TRUE;
        }
        
        last=curr;
    }
    // should not reach this point
    return FALSE;
}

OEDIT( oedit_create )
{
    OBJ_INDEX_DATA *pObj;
    AREA_DATA *pArea;
    int  value;
    int  iHash;
    
    value = atoi( argument );
    if ( argument[0] == '\0' || value == 0 )
    {
        send_to_char( "Syntax:  oedit create [vnum]\n\r", ch );
        return FALSE;
    }
    
    pArea = get_vnum_area( value );
    if ( !pArea )
    {
        send_to_char( "OEdit:  That vnum is not assigned an area.\n\r", ch );
        return FALSE;
    }
    
    if ( !IS_BUILDER( ch, pArea ) )
    {
        send_to_char( "OEdit:  Vnum in an area you cannot build in.\n\r", ch );
        return FALSE;
    }
    
    if ( get_obj_index( value ) )
    {
        send_to_char( "OEdit:  Object vnum already exists.\n\r", ch );
        return FALSE;
    }
    
    pObj			= new_obj_index();
    pObj->vnum			= value;
    pObj->area			= pArea;
    
    if ( value > top_vnum_obj )
        top_vnum_obj = value;
    
    iHash			= value % MAX_KEY_HASH;
    pObj->next			= obj_index_hash[iHash];
    obj_index_hash[iHash]	= pObj;
    ch->desc->pEdit		= (void *)pObj;
    
    send_to_char( "Object Created.\n\r", ch );
    return TRUE;
}



OEDIT( oedit_ed )
{
    OBJ_INDEX_DATA *pObj;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];
    
    EDIT_OBJ(ch, pObj);
    
    argument = one_argument( argument, command );
    one_argument( argument, keyword );
    
    if ( command[0] == '\0' )
    {
        send_to_char( "Syntax:  ed add [keyword]\n\r", ch );
        send_to_char( "         ed delete [keyword]\n\r", ch );
        send_to_char( "         ed edit [keyword]\n\r", ch );
        send_to_char( "         ed format [keyword]\n\r", ch );
        return FALSE;
    }
    
    if ( !str_cmp( command, "add" ) )
    {
        if ( keyword[0] == '\0' )
        {
            send_to_char( "Syntax:  ed add [keyword]\n\r", ch );
            return FALSE;
        }
        
        ed                  =   new_extra_descr();
        ed->keyword         =   str_dup( keyword );
        ed->next            =   pObj->extra_descr;
        pObj->extra_descr   =   ed;
        
        string_append( ch, &ed->description );
        
        return TRUE;
    }
    
    if ( !str_cmp( command, "edit" ) )
    {
        if ( keyword[0] == '\0' )
        {
            send_to_char( "Syntax:  ed edit [keyword]\n\r", ch );
            return FALSE;
        }
        
        for ( ed = pObj->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
        }
        
        if ( !ed )
        {
            send_to_char( "OEdit:  Extra description keyword not found.\n\r", ch );
            return FALSE;
        }
        
        string_append( ch, &ed->description );
        
        return TRUE;
    }
    
    if ( !str_cmp( command, "delete" ) )
    {
        EXTRA_DESCR_DATA *ped = NULL;
        
        if ( keyword[0] == '\0' )
        {
            send_to_char( "Syntax:  ed delete [keyword]\n\r", ch );
            return FALSE;
        }
        
        for ( ed = pObj->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
            ped = ed;
        }
        
        if ( !ed )
        {
            send_to_char( "OEdit:  Extra description keyword not found.\n\r", ch );
            return FALSE;
        }
        
        if ( !ped )
            pObj->extra_descr = ed->next;
        else
            ped->next = ed->next;
        
        free_extra_descr( ed );
        
        send_to_char( "Extra description deleted.\n\r", ch );
        return TRUE;
    }
    
    
    if ( !str_cmp( command, "format" ) )
    {
        if ( keyword[0] == '\0' )
        {
            send_to_char( "Syntax:  ed format [keyword]\n\r", ch );
            return FALSE;
        }
        
        for ( ed = pObj->extra_descr; ed; ed = ed->next )
        {
            if ( is_name( keyword, ed->keyword ) )
                break;
        }
        
        if ( !ed )
        {
            send_to_char( "OEdit:  Extra description keyword not found.\n\r", ch );
            return FALSE;
        }
        
        ed->description = format_string( ed->description );
        
        send_to_char( "Extra description formatted.\n\r", ch );
        return TRUE;
    }
    
    oedit_ed( ch, "" );
    return FALSE;
}





/* ROM object functions : */

OEDIT( oedit_extra )      /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_OBJ(ch, pObj);
        
        if ( (value = flag_lookup(argument, extra_flags)) != NO_FLAG )
        {
            TOGGLE_BIT(pObj->extra_flags, value);
            
            send_to_char( "Extra flag toggled.\n\r", ch);
            return TRUE;
        }
    }
    
    send_to_char( "Syntax:  extra [flag]\n\r"
        "Type '? extra' for a list of flags.\n\r", ch );
    return FALSE;
}


OEDIT( oedit_wear )      /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_OBJ(ch, pObj);
        
        if ( (value = flag_lookup(argument, wear_types)) != NO_FLAG )
        {
            pObj->wear_type = value;
            
            send_to_char( "Wear type set.\n\r", ch);
            return TRUE;
        }
    }
    
    send_to_char( "Syntax:  wear [type]\n\r"
        "Type '? wear' for a list of flags.\n\r", ch );
    return FALSE;
}


OEDIT( oedit_type )      /* Moved out of oedit() due to naming conflicts -- Hugin */
{
    OBJ_INDEX_DATA *pObj;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_OBJ(ch, pObj);
        
        if ( (value = flag_lookup(argument, type_flags)) != NO_FLAG )
        {
            send_to_char( "Type set.\n\r", ch);

            if ( pObj->item_type != value )
            {
                pObj->item_type = value;
                pObj->value[0] = 0;
                pObj->value[1] = 0;
                pObj->value[2] = 0;
                pObj->value[3] = 0;
                pObj->value[4] = 0;     /* ROM */
            }
            if ( value == ITEM_WAND || value == ITEM_STAFF )
            {
                // default of 50 charges for wands and staffs
                pObj->value[1] = 50;
                pObj->value[2] = 50;
            }
            return TRUE;
        }
    }
    
    send_to_char( "Syntax:  type [flag]\n\r"
        "Type '? type' for a list of flags.\n\r", ch );
    return FALSE;
}

OEDIT( oedit_material )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  material [string]\n\r", ch );
        return FALSE;
    }
    
    free_string( pObj->material );
    pObj->material = str_dup( argument );
    
    send_to_char( "Material set.\n\r", ch);
    return TRUE;
}

OEDIT( oedit_level )
{
    OBJ_INDEX_DATA *pObj;
    
    EDIT_OBJ(ch, pObj);
    
    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        send_to_char( "Syntax:  level [number]\n\r", ch );
        return FALSE;
    }
    
    pObj->level = atoi( argument );
    
    send_to_char( "Level set.\n\r", ch);
    return TRUE;
}

#define OBJ_STAT_AC           0 
#define OBJ_STAT_SHOP_COST    1
#define OBJ_STAT_DROP_COST    2
#define OBJ_STAT_NR           3 


/* values in above order, obj_stats[i] = stats for level i+1 */
static const int obj_ovalue[][OBJ_STAT_NR] = {
    {1,500,150          },// 1
    {1,1000,300         },
    {1,1500,450         },
    {1,2000,600         },
    {1,3000,900         },//5
    {1,3600,1080        },
    {1,4200,1260        },
    {1,4800,1440        },
    {2,5400,1620        },
    {2,7000,2100        },//10
    {2,7700,2310        },
    {3,8400,2520        },
    {3,9100,2730        },
    {3,9800,2940        },
    {3,12000,3600       },//15
    {4,12800,3840       },
    {4,13600,4080       },
    {4,14400,4320       },
    {4,17100,5130       },
    {4,18000,5400       },//20
    {5,18900,5670       },
    {5,19800,5940       },
    {5,20700,6210       },
    {5,24000,7200       },
    {6,25000,7500       },//25
    {6,26000,7800       },
    {6,27000,8100       },
    {6,28000,8400       },
    {6,31900,9570       },
    {7,33000,9900       },//30
    {7,34100,10230      },
    {7,35200,10560      },
    {7,36300,10890      },
    {8,40800,12240      },
    {8,42000,12600      },//35
    {8,43200,12960      },
    {8,44400,13320      },
    {8,49400,14820      },
    {9,50700,15210      },
    {9,52000,15600      },//40
    {9,53300,15990      },
    {9,54600,16380      },
    {10,60200,18060    },
    {10,61600,18480    },
    {10,63000,18900    },//45
    {10,64400,19320    },
    {10,65800,19740    },
    {11,72000,21600    },
    {11,73500,22050    },
    {11,75000,22500    },//50
    {11,76500,22950    },
    {12,78000,23400    },
    {12,84800,25440    },
    {12,86400,25920    },
    {12,88000,26400    },//55
    {12,89600,26880    },
    {13,96900,29070    },
    {13,98600,29580    },
    {13,100300,30090   },
    {13,102000,30600   },//60
    {14,103700,31110   },
    {14,111600,33480   },
    {14,113400,34020   },
    {14,115200,34560   },
    {14,117000,35100   },//65
    {15,118800,35640   },
    {15,127300,38190   },
    {15,129200,38760   },
    {15,131100,39330   },
    {16,133000,39900   },//70
    {16,134900,40470   },
    {16,144000,43200   },
    {16,146000,43800   },
    {16,148000,44400   },
    {17,150000,45000   },//75
    {17,159600,47880   },
    {17,161700,48510   },
    {17,163800,49140   },
    {18,165900,49770   },
    {18,168000,50400   },//80
    {18,178200,53460   },
    {18,180400,54120   },
    {18,182600,54780   },
    {19,184800,55440   },
    {19,187000,56100   },//85
    {19,197800,59340   },
    {19,200100,60030   },
    {20,202400,60720   },
    {20,204700,61410   },
    {20,216000,64800   },//90
    {20,449600,134880  },
    {20,464400,139320  },
    {21,479400,143820  },
    {21,494600,148380  },
    {21,510000,153000  },//95
    {21,532200,159660  },
    {22,551100,165330  },
    {22,572000,171600  },
    {22,594000,178200  },
    {22,600000,180000  }//100
};


const int* get_obj_ovalue( int level )
{
    static int ovalue[OBJ_STAT_NR] = { };
    level = UMAX( 1, level );

    if ( level <= 100 )
        return obj_ovalue[level-1];
    
    /* extrapolate */
    ovalue[OBJ_STAT_AC] = 15 + (level-5);
    ovalue[OBJ_STAT_DROP_COST] = 15 + (level-5);
    ovalue[OBJ_STAT_SHOP_COST] = 15 + (level-5);

    return ovalue;
} 

int armor_class_by_level( int level )
{
    return get_obj_ovalue(level)[OBJ_STAT_AC];
}

bool apply_obj_hardcaps( OBJ_INDEX_DATA *obj )
{
    AFFECT_DATA *aff;
    bool found = FALSE;

    if ( obj->level >= LEVEL_IMMORTAL )
        return FALSE;

    for ( aff = obj->affected; aff != NULL; aff = aff->next )
    {
        int spec = get_affect_cap( aff->location, obj->level );
        int value = aff->modifier;
        int factor = spec < 0 ? -1 : 1; // saves & AC

        // exceeding hard spec
        if ( value*factor > spec*factor && is_affect_cap_hard(aff->location) )
        {
            aff->modifier = spec;
            found = TRUE;
        }
        // below negative spec
        if ( value*factor < -spec*factor )
        {
            aff->modifier = -spec;
            found = TRUE;
        }
    }
    return found;
}

bool adjust_obj_weight( OBJ_INDEX_DATA *obj )
{
    int weight, min_weight, max_weight;

    if ( obj->item_type == ITEM_ARMOR )
    {
        weight = 50;
        if ( CAN_WEAR(obj,ITEM_WEAR_FLOAT) )
            weight = 0;
        else if ( CAN_WEAR(obj,ITEM_WEAR_FINGER) )
            weight = 10;
        else if ( CAN_WEAR(obj,ITEM_WEAR_TORSO) )
            weight = 100;
        if ( IS_OBJ_STAT(obj,ITEM_TRANSLUCENT_EX) )
            weight /= 2;
    }
    else if ( obj->item_type == ITEM_WEAPON )
    {
        switch (obj->value[0])
        {
            case WEAPON_SWORD:  weight = 60; break;
            case WEAPON_DAGGER: weight = 20; break;
            case WEAPON_SPEAR:  weight = 50; break;
            case WEAPON_MACE:   weight = 100; break;
            case WEAPON_AXE:    weight = 80; break;
            case WEAPON_FLAIL:  weight = 90; break;
            case WEAPON_WHIP:   weight = 30; break;
            case WEAPON_POLEARM:weight = 70; break;
            case WEAPON_GUN:    weight = 40; break;
            case WEAPON_BOW:    weight = 50; break;
            default:            weight = 60; break;
        }
        if ( IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) )
            weight += weight / 2;
    }
    else
        return FALSE;
    
    min_weight = weight / 2;
    max_weight = weight * 2;

    if ( min_weight <= obj->weight && obj->weight <= max_weight )
        return FALSE;
    
    if ( obj->weight == 0 )
        obj->weight = weight;
    else
        obj->weight = URANGE(min_weight, obj->weight, max_weight);
    
    return TRUE;
}

// find a nice damage die to use to get to given average damage
int nice_dam_die( int dam )
{
    int i;
    // special case
    if ( dam <= 1 )
        return 1;
    // try to find a nice die
#define MAX_DIE 8
    int dice[MAX_DIE] = {20,12,10,8,6,4,3,2};
    for ( i = 0; i < MAX_DIE; i++ )
        if ( (2*dam) % (dice[i]+1) == 0 )
            return dice[i];
#undef MAX_DIE
    // no nice solution
    return dam - 1;
}

void set_weapon_dam( OBJ_DATA *pObj, int dam )
{
    int die = nice_dam_die(dam);
    pObj->value[1] = (2*dam) / (die+1);
    pObj->value[2] = die;
}

void set_weapon_index_dam( OBJ_INDEX_DATA *pObj, int dam )
{
    int die = nice_dam_die(dam);
    pObj->value[1] = (2*dam) / (die+1);
    pObj->value[2] = die;
}

bool adjust_weapon_dam( OBJ_INDEX_DATA *pObj )
{
    if ( !pObj || pObj->item_type != ITEM_WEAPON )
        return FALSE;
    
    int dam = weapon_index_dam_spec(pObj);
    if ( average_weapon_index_dam(pObj) != dam )
    {
        set_weapon_index_dam(pObj, dam);
        // ensure area change is marked if called from lua
        SET_BIT(pObj->area->area_flags, AREA_CHANGED);
        return TRUE;
    }
    else
        return FALSE;
}

bool adjust_bomb_dam( OBJ_INDEX_DATA *pObj )
{
    // #dice
    pObj->value[0] = 2 * (25 + pObj->level * 3/4);
    // type of dice
    pObj->value[1] = 20;
    return TRUE;
}

/* Sets values for Armor Class based on level, and also
 * cost by using adjust drop or adjust shop.
 * Check the table above for values in case they need to
 * be changed. They're based on building guidelines from 3/18/12
 * - Astark
 */
OEDIT( oedit_adjust )
{
    OBJ_INDEX_DATA *pObj;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    const int *ovalue;
    bool set_drop = FALSE;
    bool set_shop = FALSE;
    
    EDIT_OBJ(ch, pObj);
    
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] != '\0')
    {
        if (strcmp(arg1, "drop") == 0)
            set_drop = TRUE;
        else if (strcmp(arg1, "shop") == 0)
            set_shop = TRUE;
        else
        {
            send_to_char("Syntax: adjust\n\r", ch);
            send_to_char("        adjust drop (items from killing a mob)\n\r", ch);
            send_to_char("        adjust shop (items sold in a shop)\n\r", ch);
            return FALSE;
        }
    }
    
    if (pObj->level < 1 || pObj->level > 100)
    {
        send_to_char("You must set the object's level first (1-100).\n\r", ch);
        return FALSE;
    }

    ovalue = get_obj_ovalue( pObj->level );

    // AC
    if (pObj->item_type == ITEM_ARMOR)
    {
        int ac_sum = pObj->value[0];
        int spec_sum = ovalue[OBJ_STAT_AC];
        if ( ac_sum != spec_sum )
        {
            pObj->value[0] = ovalue[OBJ_STAT_AC];
            send_to_char("AC has been adjusted.\n\r", ch);
        }
    }
    // damage
    else if ( pObj->item_type == ITEM_WEAPON )
    {
        if ( adjust_weapon_dam(pObj) )
            send_to_char("Damage has been adjusted.\n\r", ch);
    }
    else if ( pObj->item_type == ITEM_EXPLOSIVE )
    {
        if ( adjust_bomb_dam(pObj) )
            send_to_char("Damage has been adjusted.\n\r", ch);
    }
    
    // cost
    if ( set_drop || set_shop )
    {
        int spec_cost = set_shop ? ovalue[OBJ_STAT_SHOP_COST] : ovalue[OBJ_STAT_DROP_COST];
        if ( pObj->cost != spec_cost )
        {
            pObj->cost = spec_cost;
            send_to_char("Cost has been adjusted.\n\r", ch);
        }
    }
    
    // weight
    if ( adjust_obj_weight(pObj) )
        send_to_char("Weight has been adjusted.\n\r", ch);

    if ( apply_obj_hardcaps(pObj) )
        send_to_char("Magic bonuses/penalties have been adjusted.\n\r", ch);
    
    send_to_char("Stats set according to level.\n\r", ch);
    return TRUE;
}


/*
* Mobile Editor Functions.
*/
MEDIT( medit_show )
{
    MOB_INDEX_DATA *pMob;
    char buf[MAX_STRING_LENGTH];
    PROG_LIST *list;
    int sn;
    
    EDIT_MOB(ch, pMob);
    
    sprintf( buf, "Name:        [%s]\n\rArea:        [%5d] %s\n\r",
        pMob->player_name,
        !pMob->area ? -1        : pMob->area->vnum,
        !pMob->area ? "No Area" : pMob->area->name );
    send_to_char( buf, ch );
    
    sprintf( buf, "Act:         [%s]\n\r",
        act_bits_name(pMob->act) );
    send_to_char( buf, ch );
    
    sprintf( buf,
        "Vnum:        [%5d]         Sex: [%s]     Race: [%s]\n\r",
        pMob->vnum,
        pMob->sex == SEX_MALE    ? "male   " :
        pMob->sex == SEX_FEMALE  ? "female " : 
        pMob->sex == SEX_BOTH    ? "random " : "neutral",
        race_table[pMob->race].name );
    send_to_char( buf, ch );
    
    sprintf( buf,
        "Level:       [%3d]         Align: [%4d]\n\r",
        pMob->level,
        pMob->alignment);
    send_to_char( buf, ch );
    
    sprintf( buf,
        "Hitroll:     [%3d%%=%5d] Damage: [%3d%%=%5d] Dam Type: [%s]\n\r",
        pMob->hitroll_percent,
        mob_base_hitroll(pMob, pMob->level),
        pMob->damage_percent,
        mob_base_damage(pMob, pMob->level),
        attack_table[pMob->dam_type].name );         
    send_to_char( buf, ch );
    
    sprintf( buf,
        "Hitpoints:   [%3d%%=%5d]   Mana: [%3d%%=%5d]     Move: [%3d%%=%5d]\n\r",
        pMob->hitpoint_percent,
        mob_base_hp(pMob, pMob->level),
        pMob->mana_percent,
        mob_base_mana(pMob, pMob->level),
        pMob->move_percent,
        mob_base_move(pMob, pMob->level)
    );
    send_to_char( buf, ch );

    sprintf( buf, "Armor:       [%3d%%=%5d]  Saves: [%3d%%=%5d]\n\r",
        pMob->ac_percent,
        mob_base_ac(pMob, pMob->level),
        pMob->saves_percent,
        mob_base_saves(pMob, pMob->level)
    );
    send_to_char( buf, ch );
    
    if ( pMob->group )
    {
        sprintf( buf, "Group:       [%5d]\n\r", pMob->group );
        send_to_char( buf, ch );
    }    
    
    /* ROM values end */
    
    sprintf( buf, "Affected by: [%s]\n\r",
        affect_bits_name( pMob->affect_field ));
    send_to_char( buf, ch );
    
    /* ROM values: */
    
    sprintf( buf, "Form:        [%s]\n\r",
        form_bits_name(pMob->form) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Parts:       [%s]\n\r",
        part_bits_name(pMob->parts) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Imm:         [%s]\n\r",
        imm_bits_name(pMob->imm_flags) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Res:         [%s]\n\r",
        imm_bits_name(pMob->res_flags) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Vuln:        [%s]\n\r",
        imm_bits_name(pMob->vuln_flags) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Off:         [%s]\n\r",
        off_bits_name(pMob->off_flags) );
    send_to_char( buf, ch );
    
    buf[0] = '\0';
    for ( sn = 1; sn < MAX_SKILL; sn++ )
        if ( pMob->skills[sn] )
            sprintf(buf + strlen(buf), " %s", skill_table[sn].name);
    if ( buf[0] != '\0' )
        ptc(ch, "Skills:      [%s]\n\r", buf + 1);
    
    sprintf( buf, "Size:        [%s]\n\r",
        flag_bit_name(size_flags, pMob->size) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Start pos.   [%s]\n\r",
        flag_bit_name(position_flags, pMob->start_pos) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Default pos  [%s]\n\r",
        flag_bit_name(position_flags, pMob->default_pos) );
    send_to_char( buf, ch );
    
    sprintf( buf, "Wealth:      [%d%%=%ld]\n\r",
        pMob->wealth_percent,
        mob_base_wealth(pMob)
    );
    send_to_char( buf, ch );
    
    /* ROM values end */

    sprintf( buf, "Stance:      [%s]\n\r", stances[pMob->stance].name);
    send_to_char( buf, ch );    

    if ( pMob->spec_fun )
    {
        sprintf( buf, "Spec fun:    [%s]\n\r",  spec_name_lookup( pMob->spec_fun ) );
        send_to_char( buf, ch );
    }
    
    sprintf( buf, "Short descr: %s\n\rLong descr:\n\r%s\n\r",
        pMob->short_descr,
        pMob->long_descr );
    send_to_char( buf, ch );
    
    sprintf( buf, "Description:\n\r%s", pMob->description );
    send_to_char( buf, ch );

    sprintf( buf, "Comments:\n\r%s", pMob->comments );
    send_to_char( buf, ch );
    
    if ( pMob->pShop )
    {
        SHOP_DATA *pShop;
        int iTrade;
        
        pShop = pMob->pShop;
        
        sprintf( buf,
            "Shop data for [%5d]:\n\r"
            "  Markup for purchaser: %d%%\n\r"
            "  Markdown for seller:  %d%%\n\r",
            pShop->keeper, pShop->profit_buy, pShop->profit_sell );
        send_to_char( buf, ch );
        sprintf( buf, "  Hours: %d to %d.\n\r",
            pShop->open_hour, pShop->close_hour );
        send_to_char( buf, ch );
        
        for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
        {
            if ( pShop->buy_type[iTrade] != 0 )
            {
                if ( iTrade == 0 ) {
                    send_to_char( "  Number Trades Type\n\r", ch );
                    send_to_char( "  ------ -----------\n\r", ch );
                }
                sprintf( buf, "  [%4d] %s\n\r", iTrade,
                    flag_bit_name(type_flags, pShop->buy_type[iTrade]) );
                send_to_char( buf, ch );
            }
        }
    }

    if ( pMob->boss_achieve )
    {
        send_to_char( "\n\rBOSS ACHIEVEMENT\n\r", ch );
        ptc( ch, "QP reward:   %d\n\r", pMob->boss_achieve->quest_reward);
        ptc( ch, "EXP reward:  %d\n\r", pMob->boss_achieve->exp_reward);
        ptc( ch, "Gold reward: %d\n\r", pMob->boss_achieve->gold_reward);
        ptc( ch, "AchP reward: %d\n\r", pMob->boss_achieve->ach_reward);
        
    }
    
    if ( pMob->mprogs )
    {
        int cnt;
        
        sprintf(buf, "\n\rMOBPrograms for [%5d]:\n\r", pMob->vnum);
        send_to_char( buf, ch );
        
        for (cnt=0, list=pMob->mprogs; list; list=list->next)
        {
            if (cnt ==0)
            {
                send_to_char ( " Number Vnum Trigger Phrase\n\r", ch );
                send_to_char ( " ------ ---- ------- ------\n\r", ch );
            }
            
            sprintf(buf, "[%5d] %4d %7s %s\n\r", cnt,
                list->vnum,mprog_type_to_name(list->trig_type),
                list->trig_phrase);
            send_to_char( buf, ch );
            cnt++;
        }
    }
    
    return FALSE;
}

MEDIT( medit_delete )
{
    MOB_INDEX_DATA *pMob;
    AREA_DATA *pArea;
    int value;
    int iHash;

    value = atoi( argument );
    if ( argument[0] == '\0' || value == 0 )
    {
        send_to_char( "Syntax:  medit delete [vnum]\n\r", ch );
        return FALSE;
    }

    pArea = get_vnum_area( value );
    if ( !pArea )
    {
        send_to_char( "MEdit:  That vnum is not assigned an area.\n\r", ch );
        return FALSE;
    }

    if ( !IS_BUILDER( ch, pArea ) )
    {
        send_to_char( "MEdit:  Vnum in an area you cannot build in.\n\r", ch );
        return FALSE;
    }

    if ( (pMob = get_mob_index( value ) ) == NULL )
    {
        send_to_char( "MEdit:  No such object.\n\r", ch );
        return FALSE;
    }
    
    /* check for resets */
    ROOM_INDEX_DATA *room;
    RESET_DATA *rst;
    int rvnum;

    for ( rvnum=0 ; rvnum <= top_vnum_room ; rvnum++ )
    {
        if ( (room=get_room_index(rvnum) ) == NULL )
            continue;

        for ( rst=room->reset_first ; rst ; rst=rst->next )
        {
            switch (rst->command)
            {
                case 'M':
                    break;
                default:
                    continue;
            }

            if ( rst->arg1 == value )
            {
                send_to_char( "MEdit:  Can't delete, resets exist.\n\r", ch );
                return FALSE;
            } 
        }
    }

    /* check for mprog triggers */
    if ( pMob->mprogs )
    {
        send_to_char( "MEdit:  Can't delete, mprog triggers exist.\n\r", ch );
        return FALSE;
    }

    CHAR_DATA *m;
    for ( m=char_list ; m ; m=m->next )
    {
        if ( m->pIndexData == pMob )
        {
            send_to_char( "MEdit:  Can't delete, instances exist.\n\r", ch );
            return FALSE;
        }
    }

    if (is_being_edited(pMob))
    {
        send_to_char( "Can't delete mob, it is being edited.\n\r", ch );
        return FALSE;
    }

    /* got here means we're good to delete */
    iHash=value % MAX_KEY_HASH;

    MOB_INDEX_DATA *curr, *last=NULL;

    for ( curr=mob_index_hash[iHash]; curr; curr=curr->next )
    {
        if ( curr == pMob )
        {
            if ( !last )
            {
                mob_index_hash[iHash]=curr->next;
            }
            else
            {
                last->next=curr->next;
            }

            free_mob_index( pMob );
            send_to_char( "Mob deleted.\n\r", ch );
            return TRUE;
        }
        
        last=curr;
    }
    // should not reach this point
    return FALSE;
}


MEDIT( medit_create )
{
    MOB_INDEX_DATA *pMob;
    AREA_DATA *pArea;
    int  value;
    int  iHash;
    
    value = atoi( argument );
    if ( argument[0] == '\0' || value == 0 )
    {
        send_to_char( "Syntax:  medit create [vnum]\n\r", ch );
        return FALSE;
    }
    
    pArea = get_vnum_area( value );
    
    if ( !pArea )
    {
        send_to_char( "MEdit:  That vnum is not assigned an area.\n\r", ch );
        return FALSE;
    }
    
    if ( !IS_BUILDER( ch, pArea ) )
    {
        send_to_char( "MEdit:  Vnum in an area you cannot build in.\n\r", ch );
        return FALSE;
    }
    
    if ( get_mob_index( value ) )
    {
        send_to_char( "MEdit:  Mobile vnum already exists.\n\r", ch );
        return FALSE;
    }
    
    pMob			= new_mob_index();
    pMob->vnum			= value;
    pMob->area			= pArea;
    
    if ( value > top_vnum_mob )
        top_vnum_mob = value;        
    
    flag_clear( pMob->act ); SET_BIT( pMob->act, ACT_IS_NPC );
    iHash			= value % MAX_KEY_HASH;
    pMob->next			= mob_index_hash[iHash];
    mob_index_hash[iHash]	= pMob;
    ch->desc->pEdit		= (void *)pMob;
    
    send_to_char( "Mobile Created.\n\r", ch );
    return TRUE;
}



MEDIT( medit_spec )
{
    MOB_INDEX_DATA *pMob;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  spec [special function]\n\r", ch );
        return FALSE;
    }
    
    
    if ( !str_cmp( argument, "none" ) )
    {
        pMob->spec_fun = NULL;
        
        send_to_char( "Spec removed.\n\r", ch);
        return TRUE;
    }
    
    if ( spec_lookup( argument ) )
    {
        pMob->spec_fun = spec_lookup( argument );
        send_to_char( "Spec set.\n\r", ch);
        return TRUE;
    }
    
    send_to_char( "MEdit: No such special function.\n\r", ch );
    return FALSE;
}

#define LVL_STAT_HP_DICE_NUMBER  0
#define LVL_STAT_HP_DICE_TYPE    1
#define LVL_STAT_HP_DICE_BONUS   2
#define LVL_STAT_DAM_DICE_NUMBER 3
#define LVL_STAT_DAM_DICE_TYPE   4
#define LVL_STAT_DAM_DICE_BONUS  5
#define LVL_STAT_AC              6
#define LVL_STAT_NR              7 

/* values in above order, level_stats[i] = stats for level i+1
*/
static const int level_stats[][LVL_STAT_NR] = {
    {  2,  6,    10,  1,  4,  0,   95 },
    {  2,  7,    22,  1,  5,  0,   89 },
    {  2,  6,    35,  1,  6,  0,   83 },
    {  2,  7,    46,  1,  5,  1,   76 },
    {  2,  6,    60,  1,  6,  1,   70 },
    {  2,  7,    71,  1,  7,  1,   64 },
    {  2,  6,    85,  1,  8,  1,   58 },
    {  2,  7,    96,  1,  7,  2,   52 },
    {  2,  6,   110,  1,  8,  2,   46 },
    {  2,  7,   121,  2,  4,  2,   40 }, // 10
    {  7,  3,   134,  2,  6,  0,   33 },
    {  1, 18,   157,  4,  3,  0,   26 },
    {  1, 20,   175,  4,  3,  1,   19 },
    { 11,  3,   182,  3,  4,  2,   12 },
    {  3,  9,   208,  2,  6,  3,    5 },
    { 12,  3,   224,  2,  9,  0,   -1 },
    { 12,  3,   249,  4,  3,  3,   -7 },
    { 12,  3,   274,  1, 13,  5,  -13 },
    { 12,  3,   299,  1, 13,  6,  -19 },
    {  3,  9,   333,  2,  8,  5,  -25 }, // 20
    {  7,  5,   373,  5,  4,  2,  -31 },
    {  8,  5,   416,  5,  4,  3,  -37 },
    {  1, 38,   466,  1, 18,  7,  -43 },
    {  1, 42,   510,  1, 18,  8,  -49 },
    {  5, 10,   550,  2, 10,  7,  -55 },
    {  7,  8,   609,  9,  3,  1,  -62 },
    {  1, 54,   676,  1, 20,  9,  -69 },
    {  2, 30,   735,  1, 20, 10,  -76 },
    {  2, 32,   796, 10,  3,  1,  -83 },
    {  6, 12,   853,  4,  6,  8,  -90 }, // 30
    {  1, 72,   948, 10,  3,  3,  -96 },
    {  4, 20,  1035, 11,  3,  2, -102 },
    { 10,  9,  1120, 11,  3,  3, -108 },
    {  5, 18,  1215, 12,  3,  2, -114 },
    { 10, 10,  1300,  4,  7, 11, -120 },
    { 11, 10,  1420,  5,  6, 10, -126 },
    { 12, 10,  1520,  5,  6, 11, -132 },
    {  9, 14,  1634,  9,  4,  7, -138 },
    {  9, 15,  1745,  9,  4,  8, -144 },
    { 15, 10,  1850,  4,  8, 13, -150 }, // 40
    {  9, 18,  2088,  7,  5, 11, -156 },
    { 19, 10,  2310,  2, 14, 18, -162 },
    {  9, 22,  2552,  2, 14, 19, -168 },
    {  9, 24,  2784, 12,  3, 11, -174 },
    { 25, 10,  3000,  8,  4, 16, -180 },
    { 18, 16,  3312,  2, 14, 22, -187 },
    { 15, 22,  3620,  7,  5, 17, -194 },
    { 20, 19,  3920,  1, 32, 23, -201 },
    { 15, 28,  4230, 11,  4, 13, -208 },
    { 50, 10,  4500,  5,  8, 19, -215 }, // 50
    { 50, 10,  5000,  2, 18, 24, -221 },
    { 50, 10,  5500, 11,  4, 17, -227 },
    { 50, 10,  6000,  8,  5, 21, -233 },
    { 50, 10,  6500,  1, 32, 30, -239 },
    { 50, 10,  7000, 10,  4, 23, -245 },
    { 50, 10,  7500,  8,  5, 26, -251 },
    { 50, 10,  8000,  2, 18, 32, -257 },
    { 50, 10,  8500, 12,  4, 23, -263 },
    { 50, 10,  9000,  2, 20, 33, -269 },
    { 50, 10,  9500,  8,  6, 28, -275 }, // 60
    { 53, 10,  9700,  7,  7, 28, -281 },
    { 56, 10,  9900, 11,  5, 23, -287 },
    { 59, 10, 10100, 11,  5, 23, -293 },
    { 62, 10, 10300,  2, 24, 31, -299 },
    { 65, 10, 10500,  8,  7, 24, -305 },
    { 65, 10, 10700, 12,  5, 21, -312 },
    { 65, 10, 10900, 12,  5, 22, -319 },
    { 65, 10, 11100, 12,  5, 24, -326 },
    { 65, 10, 11300, 12,  5, 25, -333 },
    { 65, 10, 11500,  8,  7, 30, -340 }, // 70
    { 65, 10, 11700, 12,  5, 27, -346 },
    { 65, 10, 11900, 12,  5, 28, -352 },
    { 65, 10, 12100, 10,  6, 30, -358 },
    { 65, 10, 12300, 10,  6, 31, -364 },
    { 65, 10, 12500, 10,  6, 32, -370 },
    { 64, 10, 12700,  3, 18, 40, -376 },
    { 63, 10, 12900,  4, 14, 39, -382 },
    { 62, 10, 13100,  1, 54, 43, -388 },
    { 61, 10, 13300,  9,  7, 35, -394 },
    { 60, 10, 13500, 11,  6, 34, -400 }, // 80
    { 60, 11, 13500,  8,  8, 37, -406 },
    { 60, 12, 13500,  3, 20, 42, -412 },
    { 60, 13, 13500,  2, 30, 44, -418 },
    { 60, 14, 13500,  1, 60, 45, -424 },
    { 60, 15, 13500, 12,  6, 34, -430 },
    { 61, 15, 13700,  1, 60, 47, -436 },
    { 62, 15, 13900,  2, 30, 48, -442 },
    { 63, 15, 14100,  1, 60, 50, -448 },
    { 64, 15, 14300,  2, 30, 51, -454 },
    { 65, 15, 14500, 19,  4, 36, -460 }, // 90
    { 65, 15, 14750, 12,  6, 42, -466 },
    { 65, 15, 15000,  2, 32, 51, -472 },
    { 65, 15, 15250,  5, 14, 47, -478 },
    { 65, 15, 15500,  1, 68, 50, -484 },
    { 65, 15, 15750, 10,  8, 40, -490 },
    { 65, 15, 16200,  1, 72, 49, -497 },
    { 65, 15, 16650, 12,  7, 38, -504 },
    { 65, 15, 17100,  5, 16, 44, -511 },
    { 65, 15, 17550,  4, 20, 45, -518 },
    { 65, 15, 18000, 11,  8, 38, -525 }, // 100
    { 65, 15, 18450, 11,  8, 39, -530 },
    { 65, 15, 18900, 11,  8, 40, -535 },
    { 65, 15, 19350, 11,  8, 42, -540 },
    { 65, 15, 19800, 11,  8, 43, -545 },
    { 65, 15, 20250, 11,  8, 44, -550 },
    { 65, 15, 20650,  1, 80, 54, -556 },
    { 65, 15, 21050, 10,  9, 45, -562 },
    { 65, 15, 21450,  9, 10, 47, -568 },
    { 65, 15, 21850,  2, 42, 54, -574 },
    { 65, 15, 22500, 12,  8, 44, -580 }, // 110
    { 65, 15, 22950, 12,  8, 45, -608 },
    { 65, 15, 23400, 12,  8, 46, -636 },
    { 65, 15, 23850, 12,  8, 48, -664 },
    { 65, 15, 24300, 12,  8, 49, -692 },
    { 65, 15, 24750, 12,  8, 50, -720 },
    { 65, 15, 25200, 12,  8, 51, -726 },
    { 65, 15, 25650, 12,  8, 52, -732 },
    { 65, 15, 26100, 12,  8, 54, -738 },
    { 65, 15, 26550, 12,  8, 55, -744 },
    { 65, 15, 27000, 12,  8, 56, -750 } // 120
};

/* returns an array with the stats for that level */
const int* get_level_stats( int level )
{
    static int stats[LVL_STAT_NR] = { 65, 15, 27000, 12,  8, 56, -750 };

    level = UMAX( 1, level );

    if ( level <= 120 )
	return level_stats[level-1];
    
    /* extrapolate */
    stats[LVL_STAT_HP_DICE_BONUS] = 27000 + (level-120) * 500;
    stats[LVL_STAT_DAM_DICE_NUMBER] = (level+9)/10;
    stats[LVL_STAT_DAM_DICE_TYPE] = (level+9)/10 - 4;
    stats[LVL_STAT_DAM_DICE_BONUS] = 56 + 2 * (level-120);
    stats[LVL_STAT_AC] = -30 - 6 * level;

    return stats;
}

/*
void set_mob_level( CHAR_DATA *mob, int level )
{
    int *stats = get_level_stats( level );

    mob->level = level;

    mob->damage[DICE_NUMBER] = stats[LVL_STAT_DAM_DICE_NUMBER];
    mob->damage[DICE_TYPE]   = stats[LVL_STAT_DAM_DICE_TYPE];
    mob->damroll             = stats[LVL_STAT_DAM_DICE_BONUS];

    mob->max_hit = dice(stats[LVL_STAT_HP_DICE_NUMBER],
			stats[LVL_STAT_HP_DICE_TYPE])
	+ stats[LVL_STAT_HP_DICE_BONUS];
    mob->hit = mob->max_hit;

    mob->armor[AC_PIERCE] = stats[LVL_STAT_AC_WEAPON];
    mob->armor[AC_BASH]   = stats[LVL_STAT_AC_WEAPON];
    mob->armor[AC_SLASH]  = stats[LVL_STAT_AC_WEAPON];
    mob->armor[AC_EXOTIC] = stats[LVL_STAT_AC_EXOTIC];

    compute_mob_stats(mob);
}
*/

void set_mob_level( CHAR_DATA *mob, int level )
{
    MOB_INDEX_DATA *pMobIndex = mob->pIndexData;
    
    level = URANGE(1, level, 200);
    mob->level = level;

    // damage dice
    int base_damage = mob_base_damage( pMobIndex, level );
    if (base_damage <= 7) {
        mob->damage[DICE_NUMBER] = 1;
        mob->damage[DICE_TYPE]   = UMAX(2, base_damage*2 - 1);
    }
    else if (base_damage <= 21)
    {
        mob->damage[DICE_NUMBER] = 2;
        mob->damage[DICE_TYPE]   = UMAX(2, base_damage - 1);
    }
    else
    {
        mob->damage[DICE_NUMBER] = 4;
        mob->damage[DICE_TYPE]   = UMAX(2, base_damage/2 - 1);
    }

    // base stats
    mob->hit = mob->max_hit = mob_base_hp( pMobIndex, level );
    mob->mana = mob->max_mana = mob_base_mana( pMobIndex, level );
    mob->move = mob->max_move = mob_base_move( pMobIndex, level );
    mob->hitroll = mob_base_hitroll( pMobIndex, level );
    mob->damroll = mob_base_damroll( pMobIndex, level );
    mob->saving_throw = mob_base_saves( pMobIndex, level );
    mob->armor = mob_base_ac( pMobIndex, level );

    /* str ... luc */
    compute_mob_stats(mob);    
}

/* methods for spec checking - needed for grep command 
 */
int average_roll( int nr, int type, int bonus )
{
    return nr * (type + 1) / 2 + bonus;
}

int average_mob_hp( int level )
{
    const int *stats;

    stats = get_level_stats( level );
    return average_roll( stats[LVL_STAT_HP_DICE_NUMBER],
			 stats[LVL_STAT_HP_DICE_TYPE],
			 stats[LVL_STAT_HP_DICE_BONUS] );
}

int average_mob_damage( int level )
{
    const int *stats;

    stats = get_level_stats( level );
    return average_roll( stats[LVL_STAT_DAM_DICE_NUMBER], stats[LVL_STAT_DAM_DICE_TYPE], 0 ) + stats[LVL_STAT_DAM_DICE_BONUS] / 4;
}

MEDIT( medit_damtype )
{
    MOB_INDEX_DATA *pMob;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  damtype [damage message]\n\r", ch );
        send_to_char( "Para ver una lista de tipos de mensajes, pon '? weapon'.\n\r", ch );
        return FALSE;
    }
    
    pMob->dam_type = attack_lookup(argument);
    send_to_char( "Damage type set.\n\r", ch);
    return TRUE;
}


MEDIT( medit_align )
{
    MOB_INDEX_DATA *pMob;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        send_to_char( "Syntax:  alignment [number]\n\r", ch );
        return FALSE;
    }
    
    pMob->alignment = atoi( argument );
    
    send_to_char( "Alignment set.\n\r", ch);
    return TRUE;
}



MEDIT( medit_level )
{
    MOB_INDEX_DATA *pMob;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        send_to_char( "Syntax:  level [number]\n\r", ch );
        return FALSE;
    }
    
    pMob->level = atoi( argument );
    
    send_to_char( "Level set.\n\r", ch);
    return TRUE;
}



MEDIT( medit_desc )
{
    MOB_INDEX_DATA *pMob;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' )
    {
        string_append( ch, &pMob->description );
        return TRUE;
    }
    
    send_to_char( "Syntax:  desc    - line edit\n\r", ch );
    return FALSE;
}

MEDIT( medit_comments)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if ( argument[0] == '\0' )
    {
        string_append( ch, &pMob->comments );
        return TRUE;
    }

    send_to_char( "Syntax:  comments   - line edit\n\r", ch );
    return FALSE;
}

MEDIT( medit_long )
{
    MOB_INDEX_DATA *pMob;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  long [string]\n\r", ch );
        return FALSE;
    }
    
    free_string( pMob->long_descr );
    pMob->long_descr = upper_realloc(str_dup(argument));
    
    send_to_char( "Long description set.\n\r", ch);
    return TRUE;
}



MEDIT( medit_short )
{
    MOB_INDEX_DATA *pMob;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  short [string]\n\r", ch );
        return FALSE;
    }
    
    free_string( pMob->short_descr );
    pMob->short_descr = str_dup( argument );
    
    send_to_char( "Short description set.\n\r", ch);
    return TRUE;
}


/*
MEDIT( medit_name )
{
MOB_INDEX_DATA *pMob;

 EDIT_MOB(ch, pMob);
 
  if ( argument[0] == '\0' )
  {
  send_to_char( "Syntax:  name [string]\n\r", ch );
  return FALSE;
  }
  
   free_string( pMob->player_name );
   pMob->player_name = str_dup( argument );
   
    send_to_char( "Name set.\n\r", ch);
    return TRUE;
    }
*/

/* Modified medit_name suggested by Matthew Peck [x96724@exmail.usma.army.mil].
This version disallows naming mobs in OLC after existing players. */

MEDIT( medit_name )
{
    MOB_INDEX_DATA *pMob;
    char arg [MAX_INPUT_LENGTH];
    const char *s;
    char buf[MAX_INPUT_LENGTH];
    struct stat fst;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  name [string]\n\r", ch );
        return FALSE;
    }
    
    s = argument;
    
    for (s = one_argument( s, arg ); arg[0] != '\0'; 
    s = one_argument( s, arg ))
    {
        
        sprintf( buf, "%s%s", PLAYER_DIR, capitalize(arg) );
            if ( stat( buf, &fst ) != -1 )
            {
                send_to_char("You can't use a player's name for a mob.\n\r", ch);
                return FALSE;
            }
    }
    
    free_string( pMob->player_name );
    pMob->player_name = str_dup( argument );
    
    send_to_char( "Name set.\n\r", ch);
    return TRUE;
}

MEDIT( medit_bossachieve )
{
    MOB_INDEX_DATA *pMob;
    char command[MIL];
    //char arg1[MIL];

    argument = one_argument( argument, command );
    //argument = one_argument( argument, arg1 );

    EDIT_MOB(ch, pMob);
    
    if ( command[0] == '\0' )
    {
        send_to_char( "Syntax: \n\r", ch );
        send_to_char( "  bossachieve assign   -- Turn on achievement\n\r"
                      "  bossachieve remove   -- Turn off achievement\n\r"
                      "  bossachieve exp   [#value]\n\r"
                      "  bossachieve gold  [#value]\n\r"
                      "  bossachieve qp    [#value]\n\r"
                      "  bossachieve ach   [#value]\n\r",
                      ch);
        return FALSE;
    }

    if ( !str_cmp( command, "assign" ) )
    {
        if ( pMob->boss_achieve )
        {
            send_to_char( "Mob already has boss achieve assigned.\n\r", ch);
            return FALSE;
        }
        if (ch->pcdata->security < 9)
        {
            send_to_char( "Must be security 9 to add boss achievements.\n\r", ch);
            return FALSE;
        }

        pMob->boss_achieve = new_boss_achieve();
        update_bossachv_table();
        send_to_char( "Boss achievement turned on.\n\r", ch);
        return TRUE;
    }

    if (!pMob->boss_achieve)
    {
        send_to_char( "Boss achievement not turned on\n\r", ch );
        return FALSE;
    }

    if ( !str_cmp( command, "remove" ) )
    {
        free_boss_achieve( pMob->boss_achieve );
        pMob->boss_achieve = NULL;

        update_bossachv_table();
        send_to_char( "Boss achievement removed.\n\r", ch );
        return TRUE;
    }

    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        ptc( ch, "Usage: bossachieve %s [#value]\n\r", command);
        return FALSE;
    }


    int value=atoi(argument);
    if ( value < 0 )
    {
        ptc( ch, "Value must be 0 or greater.\n\r");
        return FALSE;
    }

    if (!str_cmp( command, "qp" ) )
    {
        pMob->boss_achieve->quest_reward = value;
        return TRUE;
    }
    if (!str_cmp( command, "gold" ) )
    {
        pMob->boss_achieve->gold_reward = value;
        return TRUE;
    }
    if (!str_cmp( command, "exp" ) )
    {
        pMob->boss_achieve->exp_reward = value;
        return TRUE;
    }
    if (!str_cmp( command, "ach" ) )
    {
        pMob->boss_achieve->ach_reward = value;
        return TRUE;
    }


    medit_bossachieve( ch, "" );
    return FALSE;

}

MEDIT( medit_shop )
{
    MOB_INDEX_DATA *pMob;
    char command[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    
    argument = one_argument( argument, command );
    argument = one_argument( argument, arg1 );
    
    EDIT_MOB(ch, pMob);
    
    if ( command[0] == '\0' )
    {
        send_to_char( "Syntax:  shop hours [#xopening] [#xclosing]\n\r", ch );
        send_to_char( "         shop profit [#xbuying%] [#xselling%]\n\r", ch );
        send_to_char( "         shop type [#x0-4] [item type]\n\r", ch );
        send_to_char( "         shop assign\n\r", ch );
        send_to_char( "         shop remove\n\r", ch );
        return FALSE;
    }
    
    
    if ( !str_cmp( command, "hours" ) )
    {
        if ( arg1[0] == '\0' || !is_number( arg1 )
            || argument[0] == '\0' || !is_number( argument ) )
        {
            send_to_char( "Syntax:  shop hours [#xopening] [#xclosing]\n\r", ch );
            return FALSE;
        }
        
        if ( !pMob->pShop )
        {
            send_to_char( "MEdit:  Debes crear un shop primero (shop assign).\n\r", ch );
            return FALSE;
        }
        
        pMob->pShop->open_hour = atoi( arg1 );
        pMob->pShop->close_hour = atoi( argument );
        
        send_to_char( "Shop hours set.\n\r", ch);
        return TRUE;
    }
    
    
    if ( !str_cmp( command, "profit" ) )
    {
        if ( arg1[0] == '\0' || !is_number( arg1 )
            || argument[0] == '\0' || !is_number( argument ) )
        {
            send_to_char( "Syntax:  shop profit [#xbuying%] [#xselling%]\n\r", ch );
            send_to_char( "   buy:  100-500 (default 120)\n\r", ch );
            send_to_char( "  sell:   20-100 (default 80)\n\r", ch );
            return FALSE;
        }
        
        if ( !pMob->pShop )
        {
            send_to_char( "MEdit:  Debes crear un shop primero (shop assign).\n\r", ch );
            return FALSE;
        }
        
        pMob->pShop->profit_buy     = URANGE(100, atoi(arg1), 500);
        pMob->pShop->profit_sell    = URANGE(20, atoi(argument), 100);
        
        send_to_char( "Shop profit set.\n\r", ch);
        return TRUE;
    }
    
    
    if ( !str_cmp( command, "type" ) )
    {
        char buf[MAX_INPUT_LENGTH];
        int value;
        
        if ( arg1[0] == '\0' || !is_number( arg1 )
            || argument[0] == '\0' )
        {
            send_to_char( "Syntax:  shop type [#x0-4] [item type]\n\r", ch );
            return FALSE;
        }
        
        if ( atoi( arg1 ) >= MAX_TRADE )
        {
            sprintf( buf, "MEdit:  May sell %d items max.\n\r", MAX_TRADE );
            send_to_char( buf, ch );
            return FALSE;
        }
        
        if ( !pMob->pShop )
        {
            send_to_char( "MEdit:  Debes crear un shop primero (shop assign).\n\r", ch );
            return FALSE;
        }
        
        if ( (value = flag_lookup(argument, type_flags)) == NO_FLAG )
        {
            send_to_char( "MEdit:  That type of item is not known.\n\r", ch );
            return FALSE;
        }
        
        pMob->pShop->buy_type[atoi( arg1 )] = value;
        
        send_to_char( "Shop type set.\n\r", ch);
        return TRUE;
    }
    
    /* shop assign && shop delete by Phoenix */
    
    if ( !str_prefix(command, "assign") )
    {
        if ( pMob->pShop )
        {
            send_to_char("Mob already has a shop assigned to it.\n\r", ch);
            return FALSE;
        }
        
        pMob->pShop = new_shop();
        // set default markup/down
        pMob->pShop->profit_buy = 120;
        pMob->pShop->profit_sell = 80;
        
        if ( !shop_first )
            shop_first	= pMob->pShop;
        if ( shop_last )
            shop_last->next	= pMob->pShop;
        shop_last		= pMob->pShop;
        
        pMob->pShop->keeper	= pMob->vnum;
        
        send_to_char("New shop assigned to mobile.\n\r", ch);
        return TRUE;
    }
    
    if ( !str_prefix(command, "remove") )
    {
        SHOP_DATA *pShop;
        
        pShop		= pMob->pShop;
        pMob->pShop	= NULL;
        
        if ( pShop == shop_first )
        {
            if ( !pShop->next )
            {
                shop_first = NULL;
                shop_last = NULL;
            }
            else
                shop_first = pShop->next;
        }
        else
        {
            SHOP_DATA *ipShop;
            
            for ( ipShop = shop_first; ipShop; ipShop = ipShop->next )
            {
                if ( ipShop->next == pShop )
                {
                    if ( !pShop->next )
                    {
                        shop_last = ipShop;
                        shop_last->next = NULL;
                    }
                    else
                        ipShop->next = pShop->next;
                }
            }
        }
        
        free_shop(pShop);
        
        send_to_char("Mobile is no longer a shopkeeper.\n\r", ch);
        return TRUE;
    }
    
    medit_shop( ch, "" );
    return FALSE;
}


/* ROM medit functions: */


MEDIT( medit_sex )          /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, sex_flags) ) != NO_FLAG )
        {
            pMob->sex = value;
            
            send_to_char( "Sex set.\n\r", ch);
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: sex [sex]\n\r"
        "Type '? sex' for a list of flags.\n\r", ch );
    return FALSE;
}


MEDIT( medit_act )          /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, act_flags) ) != NO_FLAG )
        {
            TOGGLE_BIT( pMob->act, value );
            SET_BIT( pMob->act, ACT_IS_NPC );
            
            send_to_char( "Act flag toggled.\n\r", ch);
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: act [flag]\n\r"
        "Type '? act' for a list of flags.\n\r", ch );
    return FALSE;
}


MEDIT( medit_affect )      /* Moved out of medit() due to naming conflicts -- Hugin */
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, affect_flags) ) != NO_FLAG )
        {
            /*SET_BIT(pMob->affect_field, value); -Rim (for Quirky) 4/12/98*/
            
            if (!IS_SET(pMob->affect_field, value))
                SET_BIT(pMob->affect_field, value);
            else
                REMOVE_BIT(pMob->affect_field, value);
            
            send_to_char( "Affect flag toggled.\n\r", ch);
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: affect [flag]\n\r"
        "Type '? affect' for a list of flags.\n\r", ch );
    return FALSE;
}


MEDIT( medit_form )
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, form_flags) ) != NO_FLAG )
        {
            TOGGLE_BIT( pMob->form, value );
            send_to_char( "Form toggled.\n\r", ch );
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: form [flags]\n\r"
        "Type '? form' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_part )
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, part_flags) ) != NO_FLAG )
        {
            TOGGLE_BIT( pMob->parts, value );
            send_to_char( "Parts toggled.\n\r", ch );
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: part [flags]\n\r"
        "Type '? part' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_imm )
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, imm_flags) ) != NO_FLAG )
        {
            TOGGLE_BIT( pMob->imm_flags, value );
            send_to_char( "Immunity toggled.\n\r", ch );
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: imm [flags]\n\r"
        "Type '? imm' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_res )
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, res_flags) ) != NO_FLAG )
        {
            TOGGLE_BIT( pMob->res_flags, value );
            send_to_char( "Resistance toggled.\n\r", ch );
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: res [flags]\n\r"
        "Type '? res' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_vuln )
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, vuln_flags) ) != NO_FLAG )
        {
            TOGGLE_BIT( pMob->vuln_flags, value );
            send_to_char( "Vulnerability toggled.\n\r", ch );
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: vuln [flags]\n\r"
        "Type '? vuln' for a list of flags.\n\r", ch );
    return FALSE;
}

/*
MEDIT( medit_material )
{
    MOB_INDEX_DATA *pMob;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  material [string]\n\r", ch );
        return FALSE;
    }
    
    free_string( pMob->material );
    pMob->material = str_dup( argument );
    
    send_to_char( "Material set.\n\r", ch);
    return TRUE;
}
*/

MEDIT( medit_off )
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, off_flags) ) != NO_FLAG )
        {
            TOGGLE_BIT( pMob->off_flags, value );
            send_to_char( "Offensive behaviour toggled.\n\r", ch );
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: off [flags]\n\r"
        "Type '? off' for a list of flags.\n\r", ch );
    return FALSE;
}

MEDIT( medit_size )
{
    MOB_INDEX_DATA *pMob;
    int value;
    
    if ( argument[0] != '\0' )
    {
        EDIT_MOB( ch, pMob );
        
        if ( ( value = flag_lookup(argument, size_flags) ) != NO_FLAG )
        {
            pMob->size = value;
            send_to_char( "Size set.\n\r", ch );
            return TRUE;
        }
    }
    
    send_to_char( "Syntax: size [size]\n\r"
        "Type '? size' for a list of sizes.\n\r", ch );
    return FALSE;
}

MEDIT( medit_race )
{
    MOB_INDEX_DATA *pMob;
    int race;
    
    if ( argument[0] != '\0'
        && (( race = race_lookup( argument ) ) != 0
        || !strcmp(argument, "unique")) )
    {
        EDIT_MOB( ch, pMob );
        
        flag_set_field( pMob->act,          race_table[race].act );
        flag_set_field( pMob->off_flags,    race_table[race].off );
        // affect fields may be from previous race, or extra - we update racial ones which is likely what you want
        flag_remove_field( pMob->affect_field, race_table[pMob->race].affect_field );
        flag_set_field( pMob->affect_field, race_table[race].affect_field );
        flag_copy( pMob->imm_flags,         race_table[race].imm );
        flag_copy( pMob->res_flags,         race_table[race].res );
        flag_copy( pMob->vuln_flags,        race_table[race].vuln );
        flag_copy( pMob->form,              race_table[race].form );
        flag_copy( pMob->parts,             race_table[race].parts );
        if ( race < MAX_PC_RACE )
        {
            pMob->size = pc_race_table[race].size;
            if ( pc_race_table[race].gender == SEX_FEMALE || pc_race_table[race].gender == SEX_MALE )
                pMob->sex = pc_race_table[race].gender;
        }
        pMob->race = race;
        
        send_to_char( "Race set.\n\r", ch );
        return TRUE;
    }
    
    if ( argument[0] == '?' )
    {
        char buf[MAX_STRING_LENGTH];
        
        send_to_char( "Available races are:", ch );
        
        for ( race = 0; race_table[race].name != NULL; race++ )
        {
            if ( ( race % 3 ) == 0 )
                send_to_char( "\n\r", ch );
            sprintf( buf, " %-15s", race_table[race].name );
            send_to_char( buf, ch );
        }
        
        send_to_char( "\n\r", ch );
        return FALSE;
    }
    
    send_to_char( "Syntax:  race [race]\n\r"
        "Type 'race ?' for a list of races.\n\r", ch );
    return FALSE;
}

MEDIT( medit_skill )
{
    MOB_INDEX_DATA *pMob;
    int sn;
    
    if ( argument[0] != '\0' && (sn = skill_lookup(argument)) > 0 )
    {
        EDIT_MOB( ch, pMob );
        
        pMob->skills[sn] = !pMob->skills[sn];
        ptc(ch, "%s skill %s.\n\r", pMob->skills[sn] ? "Added" : "Removed", skill_table[sn].name);
        return TRUE;
    }
    
    ptc(ch, "Syntax:  skill [skill name]\n\r");
    return FALSE;
}

MEDIT( medit_position )
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int value;
    
    argument = one_argument( argument, arg );
    
    switch ( arg[0] )
    {
    default:
        break;
        
    case 'S':
    case 's':
        if ( str_prefix( arg, "start" ) )
            break;
        
        if ( ( value = flag_lookup(argument, position_flags) ) == NO_FLAG )
            break;
        
        EDIT_MOB( ch, pMob );
        
        pMob->start_pos = value;
        send_to_char( "Start position set.\n\r", ch );
        return TRUE;
        
    case 'D':
    case 'd':
        if ( str_prefix( arg, "default" ) )
            break;
        
        if ( ( value = flag_lookup(argument, position_flags) ) == NO_FLAG )
            break;
        
        EDIT_MOB( ch, pMob );
        
        pMob->default_pos = value;
        send_to_char( "Default position set.\n\r", ch );
        return TRUE;
    }
    
    send_to_char( "Syntax:  position [start/default] [position]\n\r"
        "Type '? position' for a list of positions.\n\r", ch );
    return FALSE;
}


/* sets the stance for a mob 
 */
MEDIT( medit_stance )
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int i;

    EDIT_MOB(ch, pMob);

    if (argument[0] != '\0')
    {
        argument = one_argument(argument, arg);
        for (i = 0; stances[i].name != NULL; i++)
            if (!str_prefix(arg, stances[i].name))
            {
                pMob->stance = i;
                send_to_char("Stance set.\n\r", ch);
                return TRUE;
            }
        send_to_char("Stance unknown.\n\r", ch);
        return FALSE;
    }

    send_to_char("Syntax: stance [stance]\n\r", ch);
    return FALSE;
}


void show_liqlist(CHAR_DATA *ch)
{
    int liq;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    
    buffer = new_buf();
    
    for ( liq = 0; liq_table[liq].liq_name != NULL; liq++)
    {
        if ( (liq % 21) == 0 )
            add_buf(buffer,"Name                 Color          Proof Full Thirst Food Ssize\n\r");
        
        sprintf(buf, "%-20s %-14s %5d %4d %6d %4d %5d\n\r",
            liq_table[liq].liq_name,liq_table[liq].liq_color,
            liq_table[liq].liq_affect[0],liq_table[liq].liq_affect[1],
            liq_table[liq].liq_affect[2],liq_table[liq].liq_affect[3],
            liq_table[liq].liq_affect[4] );
        add_buf(buffer,buf);
    }
    
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
    
    return;
}

/* needed for show_damlist --Bobble */
#define MAX_DAM_TYPE 20
static const char* basic_dam_names[MAX_DAM_TYPE] = 
{
  "none",
  "bash",
  "pierce",
  "slash",
  "fire",
  "cold",
  "lightning",
  "acid",
  "poison",
  "negative",
  "holy",
  "energy",
  "mental",
  "disease",
  "drowning",
  "light",
  "other",
  "harm",
  "charm",
  "sound"
};

/* returns the name for the given damage type */
const char* basic_dam_name( int dam_type )
{
  if (dam_type < 0 || dam_type >= MAX_DAM_TYPE)
    return "?";
  else
    return basic_dam_names[dam_type];
}

void show_damlist(CHAR_DATA *ch)
{
    int att;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    
    buffer = new_buf();
    
    add_buf(buffer,"Name                 Noun                 Damage\n\r");
    for ( att = 0; attack_table[att].name != NULL; att++)
    {
        sprintf(buf, "%-20s %-20s %-20s\n\r",
		attack_table[att].name, 
		attack_table[att].noun, 
		basic_dam_name(attack_table[att].damage));
        add_buf(buffer,buf);
    }
    
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
    
    return;
}


MEDIT( medit_group )
{
    MOB_INDEX_DATA *pMob;
    MOB_INDEX_DATA *pMTemp;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int temp;
    BUFFER *buffer;
    bool found = FALSE;
    
    EDIT_MOB(ch, pMob);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax: group [number]\n\r", ch);
        send_to_char( "        group show [number]\n\r", ch);
        return FALSE;
    }
    
    if (is_number(argument))
    {
        pMob->group = atoi(argument);
        send_to_char( "Group set.\n\r", ch );
        return TRUE;
    }
    
    argument = one_argument( argument, arg );
    
    if ( !strcmp( arg, "show" ) && is_number( argument ) )
    {
        if (atoi(argument) == 0)
        {
            send_to_char( "Are you crazy?\n\r", ch);
            return FALSE;
        }
        
        buffer = new_buf ();
        
        for (temp = 0; temp < 65536; temp++)
        {
            pMTemp = get_mob_index(temp);
            if ( pMTemp && ( pMTemp->group == atoi(argument) ) )
            {
                found = TRUE;
                sprintf( buf, "[%5d] %s\n\r", pMTemp->vnum, pMTemp->player_name );
                add_buf( buffer, buf );
            }
        }
        
        if (found)
            page_to_char( buf_string(buffer), ch );
        else
            send_to_char( "No mobs in that group.\n\r", ch );
        
        free_buf( buffer );
        return FALSE;
    }
    
    return FALSE;
}

REDIT( redit_owner )
{
    ROOM_INDEX_DATA *pRoom;
    
    EDIT_ROOM(ch, pRoom);
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  owner [owner]\n\r", ch );
        send_to_char( "         owner none\n\r", ch );
        return FALSE;
    }
    
    free_string( pRoom->owner );
    if (!str_cmp(argument, "none"))
        pRoom->owner = str_dup("");
    else
        pRoom->owner = str_dup( argument );
    
    send_to_char( "Owner set.\n\r", ch );
    return TRUE;
}

MEDIT ( medit_addmprog )
{
    int value;
    MOB_INDEX_DATA *pMob;
    PROG_LIST *list;
    PROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char phrase[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];
    
    EDIT_MOB(ch, pMob);
    argument=one_argument(argument, num);
    argument=one_argument(argument, trigger);
    argument=one_argument(argument, phrase);
    
    if (!is_number(num) || trigger[0] =='\0' || phrase[0] =='\0' )
    {
        send_to_char("Syntax:   addmprog [vnum] [trigger] [phrase]\n\r",ch);
        return FALSE;
    }
    
    if ( (value = flag_lookup(trigger, mprog_flags)) == NO_FLAG )
    {
        send_to_char("Valid flags are:\n\r",ch);
        show_help( ch, "mprog");
        return FALSE;
    }
    
    if ( ( code =get_mprog_index (atoi(num) ) ) == NULL)
    {
        send_to_char("No such MOBProgram.\n\r",ch);
        return FALSE;
    }

    if ( value == TRIG_TIMER  && IS_SET(pMob->mprog_flags, value) )
    {
        send_to_char("Can only have one timer trigger.\n\r", ch);
        return FALSE;
    }
    
    list                  = new_mprog();
    list->vnum            = atoi(num);
    list->trig_type       = value;
    list->trig_phrase     = str_dup(phrase);
    list->script            = code;
    SET_BIT(pMob->mprog_flags,value);
    list->next            = pMob->mprogs;
    pMob->mprogs          = list;
    
    send_to_char( "Mprog Added.\n\r",ch);
    return TRUE;
}

/*****************************************************************
 * Name: update_mprog_flags
 * Purpose: fix bug that removes valid mprog flags
 * Called by: medit_delmprog
 *****************************************************************/
void update_mprog_flags( MOB_INDEX_DATA *pMob )
{
    PROG_LIST *list;
    
    /* clear flags */
    flag_clear( pMob->mprog_flags );
    
    /* re-add all flags needed */
    for (list = pMob->mprogs; list != NULL; list = list->next)
        SET_BIT(pMob->mprog_flags, list->trig_type);
}


MEDIT ( medit_delmprog )
{
    MOB_INDEX_DATA *pMob;
    PROG_LIST *list;
    PROG_LIST *list_next;
    char mprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;
    
    EDIT_MOB(ch, pMob);
    
    one_argument( argument, mprog );
    if (!is_number( mprog ) || mprog[0] == '\0' )
    {
        send_to_char("Syntax:  delmprog [#mprog]\n\r",ch);
        return FALSE;
    }
    
    value = atoi ( mprog );
    
    if ( value < 0 )
    {
        send_to_char("Only non-negative mprog-numbers allowed.\n\r",ch);
        return FALSE;
    }
    
    if ( !(list= pMob->mprogs) )
    {
        send_to_char("MEdit:  Non existant mprog.\n\r",ch);
        return FALSE;
    }
    
    if ( value == 0 )
    {
        list = pMob->mprogs;
        pMob->mprogs = list->next;
        free_mprog( list );
    }
    else
    {
        while ( (list_next = list->next) && (++cnt < value ) )
            list = list_next;
        
        if ( list_next )
        {
            list->next = list_next->next;
            free_mprog(list_next);
        }
        else
        {
            send_to_char("No such mprog.\n\r",ch);
            return FALSE;
        }
    }

    update_mprog_flags(pMob);
    
    send_to_char("Mprog removed.\n\r", ch);
    return TRUE;
}

REDIT( redit_room )
{
    ROOM_INDEX_DATA *room;
    int value;
    
    EDIT_ROOM(ch, room);
    
    if ( (value = flag_lookup(argument, room_flags)) == NO_FLAG )
    {
        send_to_char( "Syntax: room [flags]\n\r", ch );
        return FALSE;
    }
    
    TOGGLE_BIT(room->room_flags, value);
    send_to_char( "Room flags toggled.\n\r", ch );
    return TRUE;
}

REDIT( redit_sector )
{
    ROOM_INDEX_DATA *room;
    int value;
    
    EDIT_ROOM(ch, room);
    
    if ( (value = flag_lookup(argument, sector_flags)) == NO_FLAG )
    {
        if (str_prefix(argument, "inside"))
        {
            value=SECT_INSIDE;
        }
        else
        {
            send_to_char( "Syntax: sector [type]\n\r", ch );
            return FALSE;
        }
    }
    
    room->sector_type = value;
    send_to_char( "Sector type set.\n\r", ch );
    
    return TRUE;
    
}

/* Help Editor - kermit 1/98 */
HEDIT (hedit_create)
{
    HELP_DATA *pHelp;
    HELP_AREA *pHad;
    
    if (argument[0] == '\0')
    {
        send_to_char("Syntax: hedit create [keyword(s)]\n\r",ch);
        return FALSE;
    }
    
    pHelp             = new_help();
    pHelp->keyword    = str_dup(argument);
    
    
    /* Insert new help entry at front of help.are section of help list. -Rim 8/25/98 */
    for (pHad = had_list; pHad; pHad = pHad->next)
    {
        if (!str_cmp(pHad->filename, "help.are"))
        {
            pHelp->next = pHad->first;
            pHelp->next_area = pHad->first;
            if (pHad->first == help_first)
                help_first = pHelp;
            pHad->first = pHelp;
            break;
        }
    }
    
    ch->desc->pEdit   = (void *)pHelp;
    
    send_to_char("New help entry created.\n\r",ch);
    return TRUE;
}

HEDIT( hedit_show)
{
    HELP_DATA *pHelp;
    char buf[MSL*4];
    
    EDIT_HELP(ch,pHelp);
    
    if(pHelp->to_delete)
    {
        send_to_char("\n\nThis help has been marked for deletion!\n\r",ch);
        return FALSE;
    }
    
    sprintf(buf,
        "Level:       [%d]\n\r"
        "Keywords: %s\n\r"
        "\n\r%s\n\r",
        pHelp->level, pHelp->keyword, pHelp->text);
    send_to_char_new(buf, ch, TRUE); // RAW
    
    return FALSE;
}

HEDIT( hedit_desc)
{
    HELP_DATA *pHelp;
    EDIT_HELP(ch, pHelp);
    
    if (argument[0] =='\0')
    {
        string_append(ch, &pHelp->text);
        return TRUE;
    }
    
    send_to_char(" Syntax: desc\n\r",ch);
    return FALSE;
}

HEDIT( hedit_keywords)
{
    HELP_DATA *pHelp;
    EDIT_HELP(ch, pHelp);
    
    if(argument[0] == '\0')
    {
        send_to_char(" Syntax: keywords [keywords]\n\r",ch);
        return FALSE;
    }
    
    pHelp->keyword = str_dup(argument);
    send_to_char( "Keyword(s) set.\n\r", ch);
    return TRUE;
}

HEDIT(hedit_level)
{
    HELP_DATA *pHelp;
    
    EDIT_HELP(ch, pHelp);
    
    if ( argument[0] == '\0' || !is_number( argument ) )
    {
        send_to_char( "Syntax:  level [number]\n\r", ch );
        return FALSE;
    }
    
    pHelp->level = atoi( argument );
    
    send_to_char( "Level set.\n\r", ch);
    return TRUE;
}

HEDIT( hedit_delete)
{
    HELP_DATA *pHelp;
    
    EDIT_HELP(ch,pHelp);
    
    if(!pHelp->to_delete) 
    {
        pHelp->to_delete = TRUE;
        send_to_char("You have marked this help for deletion!\n\r",ch);
        return TRUE;
    }
    
    pHelp->to_delete = FALSE;
    send_to_char("Help is no longer marked for deletion.\n\r",ch);
    return TRUE;
}

/* percentage values */

#define CMD(cmd) (strcmp(command, cmd)==0)
bool medit_percent ( CHAR_DATA *ch, const char *argument, char* command)
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int percent;
    
    if ( argument[0] == '\0' )
    {
        printf_to_char( ch, "Syntax:  %s [percentage]\n\r", command );
        return FALSE;
    }
       
    EDIT_MOB(ch, pMob);
    argument = one_argument( argument, arg );
        
    if ( !is_number( arg ) )
    {
        send_to_char( "Percentage must be a number\n\r", ch );
        return FALSE;        
    }
    
    percent = atoi( arg );
    
    if ( percent < 0 || percent > 1000 )
    {
        send_to_char( "Percentage must be a number between 0 and 1000.\n\r", ch );
        return FALSE;        
    }
    
    if CMD("hitpoints")
        pMob->hitpoint_percent = percent;
    else if CMD("mana")
        pMob->mana_percent = percent;
    else if CMD("move")
        pMob->move_percent = percent;
    else if CMD("hitroll")
        pMob->hitroll_percent = percent;
    else if CMD("damage")
        pMob->damage_percent = percent;
    else if CMD("armor")
        pMob->ac_percent = percent;
    else if CMD("saves")
        pMob->saves_percent = percent;
    else if CMD("wealth")
        pMob->wealth_percent = percent;
    else
    {
        printf_to_char( ch, "BUG: invalid command '%s'.\n\r", command );
        return FALSE;
    }
    
    printf_to_char( ch, "Set %s.\n\r", command );
    return TRUE;    
    
}
#undef CMD

MEDIT( medit_hitpoints )
{
    return medit_percent( ch, argument, "hitpoints" );
}

MEDIT( medit_mana )
{
    return medit_percent( ch, argument, "mana" );
}

MEDIT( medit_move )
{
    return medit_percent( ch, argument, "move" );
}

MEDIT( medit_hitroll )
{
    return medit_percent( ch, argument, "hitroll" );
}

MEDIT( medit_damage )
{
    return medit_percent( ch, argument, "damage" );
}

MEDIT( medit_armor )
{
    return medit_percent( ch, argument, "armor" );
}

MEDIT( medit_saves )
{
    return medit_percent( ch, argument, "saves" );
}

MEDIT( medit_wealth )
{
    return medit_percent( ch, argument, "wealth" );
}

