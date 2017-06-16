/**************************************************************************
*  File: olc_save.c                                                       *
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
/* OLC_SAVE.C
* This takes care of saving all the .are information.
* Notes:
* -If a good syntax checker is used for setting vnum ranges of areas
*  then it would become possible to just cycle through vnums instead
*  of using the iHash stuff and checking that the room or reset or
*  mob etc is part of that area.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"

//#define DIF(a,b) (~((~a)|(b)))
#define DIF(a,b,c) flag_copy(a,b); flag_remove_field(a,c)

void save_helps( FILE *fp, HELP_AREA *ha );
void save_other_helps( void );
void save_skills( void );
void reverse_mprog_order args( (MOB_INDEX_DATA *pMobIndex) );
void reverse_affect_order args( (OBJ_INDEX_DATA *pObjIndex) );


/*
*  Verbose writes reset data in plain english into the comments
*  section of the resets.  It makes areas considerably larger but
*  may aid in debugging.
*/

/* #define VERBOSE */

/*****************************************************************************
Name:		fix_string
Purpose:	Returns a string without \r and ~.
****************************************************************************/
char *fix_string( const char *str )
{
    static char strfix[MAX_STRING_LENGTH * 3];
    int i;
    int o;
    
    if ( str == NULL )
        return '\0';
    
    for ( o = i = 0; str[i+o] != '\0'; i++ )
    {
        if (i == MAX_STRING_LENGTH * 3)
        {
            bugf( "Fix_string: String too long!" );
            log_string(str);
            i--;
            break;
        }
        
        while (str[i+o] == '\r' || str[i+o] == '~')
            o++;

        if (str[i+o]== '\0')
            break;

        strfix[i] = str[i+o];
    }
    strfix[i] = '\0';
    return strfix;
}

bool must_save_area( AREA_DATA *area )
{
    return area->save && !IS_SET( area->area_flags, AREA_CLONE );
}

/*****************************************************************************
Name:		save_area_list
Purpose:	Saves the listing of files to be loaded at startup.
Called by:	do_asave(olc_save.c).
****************************************************************************/
void save_area_list( void )
{
    FILE *fp;
    AREA_DATA *pArea;
    extern HELP_AREA * had_list;
    HELP_AREA * ha;


    if ( ( fp = fopen( "area.lst", "w" ) ) == NULL )
    {
        bug( "Save_area_list: fopen", 0 );
        log_error( "area.lst" );
    }
    else
    {
        for ( ha = had_list; ha; ha = ha->next )
            if ( ha->area == NULL )
                fprintf( fp, "%s\n", ha->filename );


        for( pArea = area_first; pArea; pArea = pArea->next )
        {
            if ( must_save_area(pArea) )
                fprintf( fp, "%s\n", pArea->file_name );
        }

        fprintf( fp, "$\n" );
        fclose( fp );
    }

    return;
}


/*
* ROM OLC
* Used in save_object below.  Writes
* flags on the form fread_flag reads.
* 
* buf[] must hold at least 32+1 characters.
*
* -- Hugin
*/
char *fwrite_flag( long flags, char buf[] )
{
    char offset;
    char *cp;

    buf[0] = '\0';

    if ( flags == 0 )
    {
        strcpy( buf, "0" );
        return buf;
    }

    /* 32 -- number of bits in a long */

    for ( offset = 0, cp = buf; offset < 32; offset++ )
        if ( flags & ( (long)1 << offset ) )
        {
            if ( offset <= 'Z' - 'A' )
                *(cp++) = 'A' + offset;
            else
                *(cp++) = 'a' + offset - ( 'Z' - 'A' + 1 );
        }

    *cp = '\0';

    return buf;
}

/* reverses the order of the affect list of pObjIndex
 */
void reverse_affect_order(OBJ_INDEX_DATA *pObjIndex)
{
    AFFECT_DATA 
        *new_affect_list = NULL,
        *next_affect;
    
    if (pObjIndex->affected == NULL || pObjIndex->affected->next == NULL)
        return;
    
    next_affect = pObjIndex->affected->next;
    while (next_affect)
    {
        pObjIndex->affected->next = new_affect_list;
        new_affect_list = pObjIndex->affected;
        pObjIndex->affected = next_affect;
        next_affect = next_affect->next;
    }
    pObjIndex->affected->next = new_affect_list;
}


/*****************************************************************
 * Name: reverse_mprog_order
 * Purpose: fix bug that reverses order of mob mprogs as they save
 * Called by:  save_mobble
 *****************************************************************/
void reverse_mprog_order(MOB_INDEX_DATA *pMobIndex)
{
    PROG_LIST
        *new_mprog_list = NULL,
        *next_mprog;
    
    if (pMobIndex->mprogs == NULL || pMobIndex->mprogs->next == NULL)
        return;
    
    next_mprog = pMobIndex->mprogs->next;
    while (next_mprog)
    {
        pMobIndex->mprogs->next = new_mprog_list;
        new_mprog_list = pMobIndex->mprogs;
        pMobIndex->mprogs = next_mprog;
        next_mprog = next_mprog->next;
    }
    pMobIndex->mprogs->next = new_mprog_list;
}

void reverse_oprog_order(OBJ_INDEX_DATA *pObjIndex)
{
    PROG_LIST
        *new_oprog_list = NULL,
        *next_oprog;

    if (pObjIndex->oprogs == NULL || pObjIndex->oprogs->next == NULL)
        return;

    next_oprog = pObjIndex->oprogs->next;
    while (next_oprog)
    {
        pObjIndex->oprogs->next = new_oprog_list;
        new_oprog_list = pObjIndex->oprogs;
        pObjIndex->oprogs = next_oprog;
        next_oprog = next_oprog->next;
    }
    pObjIndex->oprogs->next = new_oprog_list;
}

void reverse_aprog_order(AREA_DATA *pArea)
{
    PROG_LIST
        *new_aprog_list = NULL,
        *next_aprog;

    if (pArea->aprogs == NULL || pArea->aprogs->next == NULL)
        return;

    next_aprog = pArea->aprogs->next;
    while (next_aprog)
    {
        pArea->aprogs->next = new_aprog_list;
        new_aprog_list = pArea->aprogs;
        pArea->aprogs = next_aprog;
        next_aprog = next_aprog->next;
    }
    pArea->aprogs->next = new_aprog_list;
}

void reverse_rprog_order(ROOM_INDEX_DATA *pRoom)
{
    PROG_LIST
        *new_rprog_list = NULL,
        *next_rprog;

    if (pRoom->rprogs == NULL || pRoom->rprogs->next == NULL)
        return;

    next_rprog = pRoom->rprogs->next;
    while (next_rprog)
    {
        pRoom->rprogs->next = new_rprog_list;
        new_rprog_list = pRoom->rprogs;
        pRoom->rprogs = next_rprog;
        next_rprog = next_rprog->next;
    }
    pRoom->rprogs->next = new_rprog_list;
}

void save_mobprogs( FILE *fp, AREA_DATA *pArea )
{
    PROG_CODE *pMprog;
    int i;
    
    fprintf(fp, "#MOBPROGS\n");
    
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pMprog = get_mprog_index(i) ) != NULL)
        {
		          fprintf(fp, "#%d\n", i);
                  fprintf(fp, "LUA %d\n", pMprog->is_lua);
                  fprintf(fp, "SEC %d\n", pMprog->security);
                  rfprintf(fp, "CODE %s~\n", fix_string(pMprog->code));
                  fprintf(fp, "End\n");
        }
    }
    
    fprintf(fp,"#0\n\n");
    return;
}

void save_objprogs( FILE *fp, AREA_DATA *pArea )
{
    PROG_CODE *pOprog;
    int i;

    fprintf(fp, "#OBJPROGS\n");

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pOprog = get_oprog_index(i) ) != NULL)
        {
                  fprintf(fp, "#%d\n", i);
                  fprintf(fp, "SEC %d\n", pOprog->security);
                  rfprintf(fp, "CODE %s~\n", fix_string(pOprog->code));
                  fprintf(fp, "End\n");
        }
    }

    fprintf(fp,"#0\n\n");
    return;
}

void save_areaprogs( FILE *fp, AREA_DATA *pArea )
{
    PROG_CODE *pAprog;
    int i;

    fprintf(fp, "#AREAPROGS\n");

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pAprog = get_aprog_index(i) ) != NULL)
        {
                  fprintf(fp, "#%d\n", i);
                  fprintf(fp, "SEC %d\n", pAprog->security);
                  rfprintf(fp, "CODE %s~\n", fix_string(pAprog->code));
                  fprintf(fp, "End\n");
        }
    }

    fprintf(fp,"#0\n\n");
    return;
}

void save_roomprogs( FILE *fp, AREA_DATA *pArea )
{
    PROG_CODE *pRprog;
    int i;

    fprintf(fp, "#ROOMPROGS\n");

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pRprog = get_rprog_index(i) ) != NULL)
        {
                  fprintf(fp, "#%d\n", i);
                  fprintf(fp, "SEC %d\n", pRprog->security);
                  rfprintf(fp, "CODE %s~\n", fix_string(pRprog->code));
                  fprintf(fp, "End\n");
        }
    }

    fprintf(fp,"#0\n\n");
    return;
}

#define FPRINT_FIELD_INT(field, value, default) \
    if (value != default) fprintf( fp, "%s %d\n", field, value )
#define FPRINT_FIELD_NAMED(field, value, default, name) \
    if (value != default) fprintf( fp, "%s %s\n", field, name )
#define FPRINT_FIELD_FLAGS(field, value, default) \
    if (!flag_equal(value,default)) fprintf( fp, "%s %s\n", field, print_tflag(value) )

/*****************************************************************************
Name:           save_mobble
Purpose:        Save one mobile to file, extra-new format -- Bobble
Called by:      save_mobbles (below).
****************************************************************************/
void save_mobble( FILE *fp, MOB_INDEX_DATA *pMobIndex )
{
    sh_int race = pMobIndex->race;
    PROG_LIST *pMprog;
    int sn;
    
    fprintf( fp, "#%d\n",       pMobIndex->vnum );
    rfprintf( fp, "NAME %s~\n",  pMobIndex->player_name );
    rfprintf( fp, "SDESC %s~\n", pMobIndex->short_descr );
    rfprintf( fp, "LDESC %s~\n", fix_string(pMobIndex->long_descr) );
    rfprintf( fp, "DESC %s~\n",  fix_string(pMobIndex->description) );
    rfprintf( fp, "COMMENTS %s~\n", fix_string(pMobIndex->comments) );
    rfprintf( fp, "RACE %s~\n",  race_table[race].name );
    fprintf( fp, "SEX %s\n",    sex_table[pMobIndex->sex].name );
    
    for ( sn = 1; sn < MAX_SKILL; sn++ )
        if ( pMobIndex->skills[sn] )
            rfprintf(fp, "SKILL %s~\n", skill_table[sn].name);
    
    // flags must come after race, apart from that order does not matter
    FPRINT_FIELD_FLAGS("ACT",   pMobIndex->act, race_table[race].act );
    FPRINT_FIELD_FLAGS("AFF",   pMobIndex->affect_field, race_table[race].affect_field );
    FPRINT_FIELD_FLAGS("OFF",   pMobIndex->off_flags, race_table[race].off );
    FPRINT_FIELD_FLAGS("IMM",   pMobIndex->imm_flags, race_table[race].imm );
    FPRINT_FIELD_FLAGS("RES",   pMobIndex->res_flags, race_table[race].res );
    FPRINT_FIELD_FLAGS("VULN",  pMobIndex->vuln_flags, race_table[race].vuln );
    FPRINT_FIELD_FLAGS("FORM",  pMobIndex->form, race_table[race].form );
    FPRINT_FIELD_FLAGS("PARTS", pMobIndex->parts, race_table[race].parts );

    fprintf( fp, "LVL %d\n",    pMobIndex->level );
    FPRINT_FIELD_INT("HP",      pMobIndex->hitpoint_percent, 100);
    FPRINT_FIELD_INT("MANA",    pMobIndex->mana_percent, 100);
    FPRINT_FIELD_INT("MOVE",    pMobIndex->move_percent, 100);
    FPRINT_FIELD_INT("HIT",     pMobIndex->hitroll_percent, 100);
    FPRINT_FIELD_INT("DAM",     pMobIndex->damage_percent, 100);
    FPRINT_FIELD_INT("AC",      pMobIndex->ac_percent, 100);
    FPRINT_FIELD_INT("SAVES",   pMobIndex->saves_percent, 100);
    FPRINT_FIELD_INT("WEALTH",  pMobIndex->wealth_percent, 100);
    
    fprintf( fp, "DAMTYPE %s\n", attack_table[pMobIndex->dam_type].name );
    FPRINT_FIELD_INT("STANCE",  pMobIndex->stance, STANCE_DEFAULT);
    
    FPRINT_FIELD_NAMED("SIZE",  pMobIndex->size, SIZE_MEDIUM, size_table[pMobIndex->size].name);
    FPRINT_FIELD_INT("ALIGN",   pMobIndex->alignment, 0);
    FPRINT_FIELD_INT("GROUP",   pMobIndex->group, 0);

    FPRINT_FIELD_NAMED("SPOS",  pMobIndex->start_pos, POS_STANDING, position_table[pMobIndex->start_pos].short_name);
    FPRINT_FIELD_NAMED("DPOS",  pMobIndex->default_pos, POS_STANDING, position_table[pMobIndex->default_pos].short_name);
       
    reverse_mprog_order(pMobIndex);    
    for (pMprog = pMobIndex->mprogs; pMprog; pMprog = pMprog->next)
    {
        rfprintf(fp, "MPROG %s %d %s~\n", mprog_type_to_name(pMprog->trig_type), pMprog->vnum, pMprog->trig_phrase);
    }
    reverse_mprog_order(pMobIndex);

    fprintf( fp, "END\n\n" );
    
    return;
}


/*****************************************************************************
Name:           save_mobbles
Purpose:        Save #MOBBLES secion of an area file.
Called by:      save_area(olc_save.c).
****************************************************************************/
void save_mobbles( FILE *fp, AREA_DATA *pArea )
{
    int i;
    MOB_INDEX_DATA *pMob;
    
    fprintf( fp, "#MOBBLES\n" );
    
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pMob = get_mob_index( i )) )
            save_mobble( fp, pMob );
    }
    
    fprintf( fp, "#0\n\n\n\n" );
    return;
}


/*****************************************************************************
Name:		save_object
Purpose:	Save one object to file.
new ROM format saving -- Hugin
Called by:	save_objects (below).
****************************************************************************/
void save_object( FILE *fp, OBJ_INDEX_DATA *pObjIndex )
{
    AFFECT_DATA *pAf;
    EXTRA_DESCR_DATA *pEd;
    char buf[MAX_STRING_LENGTH];
    
    fprintf( fp, "#%d\n",    pObjIndex->vnum );
    rfprintf( fp, "%s~\n",    pObjIndex->name );
    rfprintf( fp, "%s~\n",    pObjIndex->short_descr );
    rfprintf( fp, "%s~\n",    fix_string( pObjIndex->description ) );
    rfprintf( fp, "%s~\n",    pObjIndex->material );
    
    fprintf( fp, "%s ",      item_name(pObjIndex->item_type));
    fprintf( fp, "%s ",      print_tflag( pObjIndex->extra_flags ) );
    fprintf( fp, "%s\n", flag_bit_name( wear_types, pObjIndex->wear_type));
    
    
    
    /*
    *  Using fwrite_flag to write most values gives a strange
    *  looking area file, consider making a case for each
    *  item type later.
    */
    
    switch ( pObjIndex->item_type )
    {
    default:
        fprintf( fp, "%s ",  fwrite_flag( pObjIndex->value[0], buf ) );
        fprintf( fp, "%s ",  fwrite_flag( pObjIndex->value[1], buf ) );
        fprintf( fp, "%s ",  fwrite_flag( pObjIndex->value[2], buf ) );
        fprintf( fp, "%s ",  fwrite_flag( pObjIndex->value[3], buf ) );
        fprintf( fp, "%s\n", fwrite_flag( pObjIndex->value[4], buf ) );
        break;
        
        
    case ITEM_EXPLOSIVE:
        fprintf( fp, "%d %d %d %d %s\n",
            pObjIndex->value[0],
            pObjIndex->value[1],
            pObjIndex->value[2],
            pObjIndex->value[3],
            fwrite_flag(pObjIndex->value[4], buf) );
        break;			
        
    case ITEM_DRINK_CON:
    case ITEM_FOUNTAIN:
        fprintf( fp, "%d %d '%s' %d %d\n",
            pObjIndex->value[0],
            pObjIndex->value[1],
            liq_table[pObjIndex->value[2]].liq_name,
            pObjIndex->value[3],
            pObjIndex->value[4]);
        break;
        
    case ITEM_CONTAINER:
        fprintf( fp, "%d %s %d %d %d\n",
            pObjIndex->value[0],
            fwrite_flag( pObjIndex->value[1], buf ),
            pObjIndex->value[2],
            pObjIndex->value[3],
            pObjIndex->value[4]);
        break;
        
    case ITEM_WEAPON:
        fprintf( fp, "%s %d %d %s %s\n",
            weapon_name(pObjIndex->value[0]),
            pObjIndex->value[1],
            pObjIndex->value[2],
            attack_table[pObjIndex->value[3]].name,
            fwrite_flag( pObjIndex->value[4], buf ) );
        break;
        
    case ITEM_PILL:
    case ITEM_POTION:
    case ITEM_SCROLL:
        fprintf( fp, "%d '%s' '%s' '%s' '%s'\n",
            pObjIndex->value[0] > 0 ? /* no negative numbers */
            pObjIndex->value[0]
            : 0,
            pObjIndex->value[1] != -1 ?
            skill_table[pObjIndex->value[1]].name
            : "",
            pObjIndex->value[2] != -1 ?
            skill_table[pObjIndex->value[2]].name
            : "",
            pObjIndex->value[3] != -1 ?
            skill_table[pObjIndex->value[3]].name
            : "",
            pObjIndex->value[4] != -1 ?
            skill_table[pObjIndex->value[4]].name
            : "");
        break;
        
    case ITEM_STAFF:
    case ITEM_WAND:
        fprintf( fp, "%d %d %d '%s' %d\n",
            pObjIndex->value[0],
            pObjIndex->value[1],
            pObjIndex->value[2],
            pObjIndex->value[3] != -1 ? 
            skill_table[pObjIndex->value[3]].name : "",
            pObjIndex->value[4] );
        break;
    }
    
    fprintf( fp, "%d ", pObjIndex->level );
    fprintf( fp, "%d ", pObjIndex->weight );
    fprintf( fp, "%d ", pObjIndex->cost );
    
    if (pObjIndex->clan > 0)
        rfprintf ( fp, "C %s~\n" , clan_table[pObjIndex->clan].name );
    
    if (pObjIndex->rank > 0 && pObjIndex->clan > 0)
        rfprintf ( fp, "R %s~\n" , clan_table[pObjIndex->clan].rank_list[pObjIndex->rank].name );      
    if (pObjIndex->combine_vnum > 0)
	fprintf ( fp, "B %d\n", pObjIndex->combine_vnum );

    if (pObjIndex->diff_rating > 0)
	fprintf ( fp, "G %d\n", pObjIndex->diff_rating );
    
    reverse_affect_order(pObjIndex);    
    for( pAf = pObjIndex->affected; pAf; pAf = pAf->next )
    {
        if (pAf->where == TO_OBJECT)
            fprintf( fp, "A\n%d %d\n",  pAf->location, pAf->modifier );
        else
        {
            fprintf( fp, "F\n" );
            
            switch(pAf->where)
            {
            case TO_AFFECTS:
                fprintf( fp, "A " );
                break;
            case TO_IMMUNE:
                fprintf( fp, "I " );
                break;
            case TO_RESIST:
                fprintf( fp, "R " );
                break;
            case TO_VULN:
                fprintf( fp, "V " );
                break;
            default:
                bug( "olc_save: Invalid Affect->where (%d)", pAf->where);
                break;
            }
            
            if (pAf->bitvector == 0)
                bug( "olc_save: bitvector == 0 for object %d", pObjIndex->vnum ); 
            fprintf( fp, "%d %d %d\n", pAf->location, pAf->modifier, pAf->bitvector );
        }
        if (pAf->detect_level != 0)
            fprintf( fp, "D %d\n", pAf->detect_level ); 
    }
    reverse_affect_order(pObjIndex);
    
    for( pEd = pObjIndex->extra_descr; pEd; pEd = pEd->next )
    {
        rfprintf( fp, "E\n%s~\n%s~\n", pEd->keyword,
            fix_string( pEd->description ) );
    }

    /* save oprogs if any */
    if (pObjIndex->oprogs != NULL)
    {
        PROG_LIST *pOprog;
        reverse_oprog_order(pObjIndex);
        for (pOprog = pObjIndex->oprogs; pOprog; pOprog = pOprog->next)
        {
            rfprintf(fp, "O %s %d %s~\n", name_lookup(pOprog->trig_type, oprog_flags), pOprog->vnum, pOprog->trig_phrase);
        }
        reverse_oprog_order(pObjIndex); 
    }

    rfprintf( fp, "N %s~\n", fix_string( pObjIndex->comments ) );
    
    return;
}




/*****************************************************************************
Name:		save_objects
Purpose:	Save #OBJECTS section of an area file.
Called by:	save_area(olc_save.c).
Notes:         Changed for ROM OLC.
****************************************************************************/
void save_objects( FILE *fp, AREA_DATA *pArea )
{
    int i;
    OBJ_INDEX_DATA *pObj;
    
    fprintf( fp, "#OBJECTS\n" );
    
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pObj = get_obj_index( i )) )
            save_object( fp, pObj );
    }
    
    fprintf( fp, "#0\n\n\n\n" );
    return;
}





/*****************************************************************************
Name:		save_rooms
Purpose:	Save #ROOMS section of an area file.
Called by:	save_area(olc_save.c).
****************************************************************************/
void save_rooms( FILE *fp, AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoomIndex;
    EXTRA_DESCR_DATA *pEd;
    EXIT_DATA *pExit;
    int iHash;
    int door;
    
    fprintf( fp, "#ROOMS\n" );
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
        {
            if ( pRoomIndex->area == pArea )
            {
                fprintf( fp, "#%d\n",		pRoomIndex->vnum );
                rfprintf( fp, "%s~\n",		pRoomIndex->name );
                rfprintf( fp, "%s~\n",		fix_string( pRoomIndex->description ) );
                fprintf( fp, "0 " );
                fprintf( fp, "%s ",		print_tflag(pRoomIndex->room_flags) );
                fprintf( fp, "%d\n",		pRoomIndex->sector_type );
                
                for ( pEd = pRoomIndex->extra_descr; pEd;
                pEd = pEd->next )
                {
                    rfprintf( fp, "E\n%s~\n%s~\n", pEd->keyword,
                        fix_string( pEd->description ) );
                }
                for( door = 0; door < MAX_DIR; door++ )	/* I hate this! */
                {
                    if ( ( pExit = pRoomIndex->exit[door] )
                        && pExit->u1.to_room )
                    {
                        //int locks = 0;
                        
                        /* HACK : TO PREVENT EX_LOCKED etc without EX_ISDOOR
                        to stop booting the mud */
                        if ( IS_SET(pExit->rs_flags, EX_CLOSED)
                            ||   IS_SET(pExit->rs_flags, EX_LOCKED)
                            ||   IS_SET(pExit->rs_flags, EX_PICKPROOF)
                            ||   IS_SET(pExit->rs_flags, EX_NOPASS)
                            ||   IS_SET(pExit->rs_flags, EX_EASY)
                            ||   IS_SET(pExit->rs_flags, EX_HARD)
                            ||   IS_SET(pExit->rs_flags, EX_INFURIATING)
                            ||   IS_SET(pExit->rs_flags, EX_NOCLOSE)
                            ||   IS_SET(pExit->rs_flags, EX_HIDDEN)
                            ||   IS_SET(pExit->rs_flags, EX_NOLOCK) )
                            SET_BIT(pExit->rs_flags, EX_ISDOOR);
			/* I think this was a bug --Bobble
                        else
                            REMOVE_BIT(pExit->rs_flags, EX_ISDOOR);
			*/
                        
                        /* THIS SUCKS but it's backwards compatible */
                        /* NOTE THAT EX_NOCLOSE NOLOCK etc aren't being saved */
                        
			/* we don't don't need that crap anymore --Bobble
                        if ( IS_SET( pExit->rs_flags, EX_ISDOOR ) 
                            && ( !IS_SET( pExit->rs_flags, EX_PICKPROOF ) ) 
                            && ( !IS_SET( pExit->rs_flags, EX_NOPASS ) ) )
                            locks = 1;
                        if ( IS_SET( pExit->rs_flags, EX_ISDOOR )
                            && ( IS_SET( pExit->rs_flags, EX_PICKPROOF ) )
                            && ( !IS_SET( pExit->rs_flags, EX_NOPASS ) ) )
                            locks = 2;
                        if ( IS_SET( pExit->rs_flags, EX_ISDOOR )
                            && ( !IS_SET( pExit->rs_flags, EX_PICKPROOF ) )
                            && ( IS_SET( pExit->rs_flags, EX_NOPASS ) ) )
                            locks = 3;
                        if ( IS_SET( pExit->rs_flags, EX_ISDOOR )
                            && ( IS_SET( pExit->rs_flags, EX_PICKPROOF ) )
                            && ( IS_SET( pExit->rs_flags, EX_NOPASS ) ) )
                            locks = 4;
			*/
                        
                        fprintf( fp, "D%d\n",      pExit->orig_door );
                        rfprintf( fp, "%s~\n",      fix_string( pExit->description ) );
                        rfprintf( fp, "%s~\n",      pExit->keyword );
                        fprintf( fp, "%s %d %d\n",
                        print_tflag(pExit->rs_flags),
                        pExit->key,
                        pExit->u1.to_room->vnum );
			/*
                        fprintf( fp, "%d %d %d\n", locks,
                            pExit->key,
                            pExit->u1.to_room->vnum );
			*/
                    }
                }
                if (pRoomIndex->mana_rate != 100 || pRoomIndex->heal_rate != 100)
                    fprintf ( fp, "M %d H %d\n",pRoomIndex->mana_rate,
                    pRoomIndex->heal_rate);
                if (pRoomIndex->clan > 0)
                    rfprintf ( fp, "C %s~\n" , clan_table[pRoomIndex->clan].name );
                
                if (pRoomIndex->clan_rank > 0 && pRoomIndex->clan > 0)
                    rfprintf ( fp, "R %s~\n" , clan_table[pRoomIndex->clan].rank_list[pRoomIndex->clan_rank].name );      
                
                if (!IS_NULLSTR(pRoomIndex->owner))
                    rfprintf ( fp, "O %s~\n" , pRoomIndex->owner );
                
                /* save rprogs if any */
                if (pRoomIndex->rprogs != NULL)
                {
                    PROG_LIST *pRprog;
                    reverse_rprog_order(pRoomIndex);
                    for (pRprog = pRoomIndex->rprogs; pRprog; pRprog = pRprog->next)
                    {
                        rfprintf(fp, "P %s %d %s~\n", name_lookup(pRprog->trig_type, rprog_flags), pRprog->vnum, pRprog->trig_phrase);
                    }
                    reverse_rprog_order(pRoomIndex);
                }

                rfprintf ( fp, "N %s~\n", pRoomIndex->comments );
                
                fprintf( fp, "S\n" );
            }
        }
    }
    fprintf( fp, "#0\n\n\n\n" );
    return;
}



/*****************************************************************************
Name:		save_specials
Purpose:	Save #SPECIALS section of area file.
Called by:	save_area(olc_save.c).
****************************************************************************/
void save_specials( FILE *fp, AREA_DATA *pArea )
{
    int iHash;
    MOB_INDEX_DATA *pMobIndex;
    
    fprintf( fp, "#SPECIALS\n" );
    
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->spec_fun )
            {
#if defined( VERBOSE )
                fprintf( fp, "M %d %s Load to: %s\n", pMobIndex->vnum,
                    spec_name_lookup( pMobIndex->spec_fun ),
                    pMobIndex->short_descr );
#else
                fprintf( fp, "M %d %s\n", pMobIndex->vnum,
                    spec_name_lookup( pMobIndex->spec_fun ) );
#endif
            }
        }
    }
    
    fprintf( fp, "S\n\n\n\n" );
    return;
}



/*
* This function is obsolete.  It it not needed but has been left here
* for historical reasons.  It is used currently for the same reason.
*
* I don't think it's obsolete in ROM -- Hugin.
*
* Yes, very obsolete. -- Vodur, Nov 2013
*/
void save_door_resets( FILE *fp, AREA_DATA *pArea )
{
    int iHash;
    ROOM_INDEX_DATA *pRoomIndex;
    EXIT_DATA *pExit;
    int door;
    
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
        {
            if ( pRoomIndex->area == pArea )
            {
                for( door = 0; door < MAX_DIR; door++ )
                {
                    if ( ( pExit = pRoomIndex->exit[door] )
                        && pExit->u1.to_room 
                        && ( IS_SET( pExit->rs_flags, EX_CLOSED )
                        || IS_SET( pExit->rs_flags, EX_LOCKED ) ) )
#if defined( VERBOSE )
                        fprintf( fp, "D 0 %d %d %d The %s door of %s is %s\n", 
                        pRoomIndex->vnum,
                        pExit->orig_door,
                        IS_SET( pExit->rs_flags, EX_LOCKED) ? 2 : 1,
                        dir_name[ pExit->orig_door ],
                        pRoomIndex->name,
                        IS_SET( pExit->rs_flags, EX_LOCKED) ? "closed and locked"
                        : "closed" );
#endif
#if !defined( VERBOSE )
                    fprintf( fp, "D 0 %d %d %d\n", 
                        pRoomIndex->vnum,
                        pExit->orig_door,
                        IS_SET (pExit->rs_flags, EX_LOCKED) ? 2 : IS_SET (pExit->rs_flags, EX_HIDDEN) ? 5 :1);
#endif
                }
            }
        }
    }
    return;
}




/*****************************************************************************
Name:		save_resets
Purpose:	Saves the #RESETS section of an area file.
Called by:	save_area(olc_save.c)
****************************************************************************/
void save_resets( FILE *fp, AREA_DATA *pArea )
{
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pLastMob = NULL;
    OBJ_INDEX_DATA *pLastObj;
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    int iHash;
    
    fprintf( fp, "#RESETS\n" );
    
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next )
        {
            if ( pRoom->area == pArea )
            {
                for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
                {
                    switch ( pReset->command )
                    {
                    default:
                        bug( "Save_resets: bad command %c.", pReset->command );
                        break;
                        
#if defined( VERBOSE )
                    case 'M':
                        pLastMob = get_mob_index( pReset->arg1 );
                        fprintf( fp, "M 0 %d %d %d %d Load %s\n", 
                            pReset->arg1,
                            pReset->arg2,
                            pReset->arg3,
                            pReset->arg4,
                            pLastMob->short_descr );
                        break;
                        
                    case 'O':
                        pLastObj = get_obj_index( pReset->arg1 );
                        pRoom = get_room_index( pReset->arg3 );
                        fprintf( fp, "O 0 %d 0 %d %s loaded to %s\n", 
                            pReset->arg1,
                            pReset->arg3,
                            capitalize(pLastObj->short_descr),
                            pRoom->name );
                        break;
                        
                    case 'P':
                        pLastObj = get_obj_index( pReset->arg1 );
                        fprintf( fp, "P 0 %d %d %d %d %s put inside %s\n", 
                            pReset->arg1,
                            pReset->arg2,
                            pReset->arg3,
                            pReset->arg4,
                            capitalize(get_obj_index( pReset->arg1 )->short_descr),
                            pLastObj->short_descr );
                        break;
                        
                    case 'G':
                        fprintf( fp, "G 0 %d 0 %s is given to %s\n",
                            pReset->arg1,
                            capitalize(get_obj_index( pReset->arg1 )->short_descr),
                            pLastMob ? pLastMob->short_descr : "!NO_MOB!" );
                        if ( !pLastMob )
                        {
                            sprintf( buf, "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                            bug( buf, 0 );
                        }
                        break;
                        
                    case 'E':
                        fprintf( fp, "E 0 %d 0 %d %s is loaded %s of %s\n",
                            pReset->arg1,
                            pReset->arg3,
                            capitalize(get_obj_index( pReset->arg1 )->short_descr),
                            flag_bit_name(wear_loc_strings, pReset->arg3),
                            pLastMob ? pLastMob->short_descr : "!NO_MOB!" );
                        if ( !pLastMob )
                        {
                            sprintf( buf, "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                            bug( buf, 0 );
                        }
                        break;
                        
                    case 'D':
                        break;
                        
                    case 'R':
                        pRoom = get_room_index( pReset->arg1 );
                        fprintf( fp, "R 0 %d %d Randomize %s\n", 
                            pReset->arg1,
                            pReset->arg2,
                            pRoom->name );
                        break;
                    }
#endif
#if !defined( VERBOSE )
                    case 'M':
                        pLastMob = get_mob_index( pReset->arg1 );
                        fprintf( fp, "M 0 %d %d %d %d\n", 
                            pReset->arg1,
                            pReset->arg2,
                            pReset->arg3,
                            pReset->arg4 );
                        break;
                        
                    case 'O':
                        pLastObj = get_obj_index( pReset->arg1 );
                        pRoom = get_room_index( pReset->arg3 );
                        fprintf( fp, "O 0 %d 0 %d\n", 
                            pReset->arg1,
                            pReset->arg3 );
                        // to avoid 'unused' warning when VERBOSE flag is not set
                        pLastObj = pLastObj;
                        break;
                        
                    case 'P':
                        pLastObj = get_obj_index( pReset->arg1 );
                        fprintf( fp, "P 0 %d %d %d %d\n", 
                            pReset->arg1,
                            pReset->arg2,
                            pReset->arg3,
                            pReset->arg4 );
                        break;
                        
                    case 'G':
                        fprintf( fp, "G 0 %d 0\n", pReset->arg1 );
                        if ( !pLastMob )
                        {
                            sprintf( buf,
                                "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                            bug( buf, 0 );
                        }
                        break;
                        
                    case 'E':
                        fprintf( fp, "E 0 %d 0 %d\n",
                            pReset->arg1,
                            pReset->arg3 );
                        if ( !pLastMob )
                        {
                            sprintf( buf,
                                "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                            bug( buf, 0 );
                        }
                        break;
                        
                    case 'D':
                        break;
                        
                    case 'R':
                        pRoom = get_room_index( pReset->arg1 );
                        fprintf( fp, "R 0 %d %d\n", 
                            pReset->arg1,
                            pReset->arg2 );
                        break;
            }
#endif
        }
        }	/* End if correct area */
    }	/* End for pRoom */
    }	/* End for iHash */
    fprintf( fp, "S\n\n\n\n" );
    return;
}

void save_bossachievements( FILE *fp, AREA_DATA *pArea )
{
    BOSSACHV *pBoss;
    MOB_INDEX_DATA *pMobIndex;
    int iHash;
    
    fprintf( fp, "#BOSSACHV\n" );
    
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->boss_achieve )
            {
                pBoss = pMobIndex->boss_achieve;
                
                fprintf( fp, "%d ", pMobIndex->vnum);
                fprintf( fp, "%d %d %d %d\n",
                        pBoss->exp_reward,
                        pBoss->gold_reward,
                        pBoss->quest_reward,
                        pBoss->ach_reward);
            }
        }
    }
    
    fprintf( fp, "0\n\n\n\n" );
    return;
}


/*****************************************************************************
Name:		save_shops
Purpose:	Saves the #SHOPS section of an area file.
Called by:	save_area(olc_save.c)
****************************************************************************/
void save_shops( FILE *fp, AREA_DATA *pArea )
{
    SHOP_DATA *pShopIndex;
    MOB_INDEX_DATA *pMobIndex;
    int iTrade;
    int iHash;
    
    fprintf( fp, "#SHOPS\n" );
    
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->pShop )
            {
                pShopIndex = pMobIndex->pShop;
                
                fprintf( fp, "%d ", pShopIndex->keeper );
                for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
                {
                    if ( pShopIndex->buy_type[iTrade] != 0 )
                    {
                        fprintf( fp, "%d ", pShopIndex->buy_type[iTrade] );
                    }
                    else
                        fprintf( fp, "0 ");
                }
                fprintf( fp, "%d %d ", pShopIndex->profit_buy, pShopIndex->profit_sell );
                fprintf( fp, "%d %d\n", pShopIndex->open_hour, pShopIndex->close_hour );
            }
        }
    }
    
    fprintf( fp, "0\n\n\n\n" );
    return;
}



/*****************************************************************************
Name:		save_area
Purpose:	Save an area, note that this format is new.
Called by:	do_asave(olc_save.c).
****************************************************************************/
void save_area( AREA_DATA *pArea )
{
    struct stat st = {0};
    FILE *fp;
    int i;
    char buf[MSL];
    
    if ( pArea == NULL || IS_SET(pArea->area_flags, AREA_CLONE) )
	return;

    if ( stat(pArea->file_name, &st) == 0 )
    {
        sprintf( buf, AREA_BACKUP_DIR "%s", pArea->file_name );
        int result=rename( pArea->file_name, buf );
        if ( result != 0 )
        {
            perror("Error: ");
        }
    }

    if ( !( fp = fopen( pArea->file_name, "w" ) ) )
    {
        bug( "Open_area: fopen", 0 );
        log_error( pArea->file_name );
    }
    if ( pArea->reset_time < 1 ) 
	pArea->reset_time = 15;

    fprintf( fp, "#VER %d\n", CURR_AREA_VERSION );

    for ( i = 0; i < MAX_AREA_CLONE; i++ )
	if ( pArea->clones[i] > 0 )
	    fprintf( fp, "#CLONE %d\n", pArea->clones[i] );

    fprintf( fp, "\n#AREADATA\n" );
    rfprintf( fp, "Name %s~\n",        pArea->name );
    rfprintf( fp, "Builders %s~\n",    fix_string( pArea->builders ) );
    rfprintf( fp, "Comments %s~\n",       fix_string( pArea->comments ) );
    fprintf( fp, "VNUMs %d %d\n",     pArea->min_vnum, pArea->max_vnum );
    rfprintf( fp, "Credits %s~\n",     pArea->credits );
  /* Added minlevel, maxlevel, and miniquests for new areas command
     -Astark Dec 2012 */
    fprintf( fp, "Minlevel %d\n",     pArea->minlevel ); 
    fprintf( fp, "Maxlevel %d\n",     pArea->maxlevel );
    fprintf( fp, "Miniquests %d\n",   pArea->miniquests );
    fprintf( fp, "Security %d\n",     pArea->security );
    fprintf( fp, "Time %d\n",	      pArea->reset_time );
    if (IS_SET(pArea->area_flags,AREA_REMORT))
        fprintf( fp, "Remort\n");
    if (IS_SET(pArea->area_flags,AREA_NOQUEST))
        fprintf( fp, "NoQuest\n");
    if (IS_SET(pArea->area_flags,AREA_NOHIDE))
        fprintf( fp, "NoHide\n");
    if ( IS_SET(pArea->area_flags, AREA_SOLO) )
        fprintf(fp, "Solo\n");
    if ( IS_SET(pArea->area_flags, AREA_NOREPOP) )
        fprintf(fp, "NoRepop\n");

    /* save aprogs if any */
    if (pArea->aprogs != NULL)
    {
        PROG_LIST *pAprog;
        reverse_aprog_order(pArea);
        for (pAprog = pArea->aprogs; pAprog; pAprog = pAprog->next)
        {
            rfprintf(fp, "AProg %s %d %s~\n", name_lookup(pAprog->trig_type, aprog_flags), pAprog->vnum, pAprog->trig_phrase);
        }
        reverse_aprog_order(pArea);
    }

    
    fprintf( fp, "End\n\n\n\n" );
    
    save_mobbles( fp, pArea );
    save_objects( fp, pArea );
    save_rooms( fp, pArea );
    save_specials( fp, pArea );
    save_resets( fp, pArea );
    save_shops( fp, pArea );
    save_bossachievements( fp, pArea );
    save_mobprogs( fp, pArea );
    save_objprogs( fp, pArea );
	save_areaprogs( fp, pArea );
    save_roomprogs( fp, pArea );
    
    if ( pArea->helps && pArea->helps->first )
        save_helps( fp, pArea->helps );
    
    fprintf( fp, "#$\n" );
    
    fclose( fp );
    return;
}


/*****************************************************************************
Name:		do_asave
Purpose:	Entry point for saving area data.
Called by:	interpreter(interp.c)
****************************************************************************/
DEF_DO_FUN(do_asave)
{
    char arg1 [MAX_INPUT_LENGTH];
    AREA_DATA *pArea;
    int value;
    
    smash_tilde_cpy( arg1, argument );
    
    if ( arg1[0] == '\0' )
    {
        if (ch)
        {
            send_to_char( "Syntax:\n\r", ch );
            send_to_char( "  asave <vnum>   - saves a particular area\n\r",	ch );
            send_to_char( "  asave list     - saves the area.lst file\n\r",	ch );
            send_to_char( "  asave area     - saves the area being edited\n\r",	ch );
            send_to_char( "  asave changed  - saves all changed zones\n\r",	ch );
            send_to_char( "  asave world    - saves the world! (db dump)\n\r",	ch );
            send_to_char( "  asave helps    - saves non-area based help files\n\r",	ch );
			send_to_char( "  asave skills   - saves class skill information\n\r", ch);
            send_to_char( "\n\r", ch );
        }
        
        return;
    }
    
    sprintf(log_buf, "asave %s> %s", ch->name, arg1);
    wiznet(log_buf, ch, NULL, WIZ_ASAVE, 0, get_trust(ch)); 
    
    /* Snarf the value (which need not be numeric). */
    value = atoi( arg1 );
    if ( !( pArea = get_area_data( value ) ) && is_number( arg1 ) )
    {
        if (ch)
            send_to_char( "That area does not exist.\n\r", ch );
        return;
    }
    
    /* Save area of given vnum. */
    /* ------------------------ */
    
    if ( is_number( arg1 ) )
    {
        if ( ch && !IS_BUILDER( ch, pArea ) )
        {
            send_to_char( "You are not a builder for this area.\n\r", ch );
            return;
        }
        
        save_area_list();
        save_area( pArea );
        return;
    }
    
    /* Save the world, only authorized areas. */
    /* -------------------------------------- */
    
    if ( !str_cmp( "world", arg1 ) )
    {
        save_area_list();
        for( pArea = area_first; pArea; pArea = pArea->next )
        {
            /* Builder must be assigned this area. */
            if ( ch && !IS_BUILDER( ch, pArea ) )
                continue;	  
            
	    if ( IS_SET(pArea->area_flags, AREA_CLONE) )
		continue;

            save_area( pArea );
            REMOVE_BIT( pArea->area_flags, AREA_CHANGED );
        }
        
        save_other_helps( );
        
        if ( ch )
            send_to_char( "You saved the world.\n\r", ch );
        
        return;
    }
    
    /* Save changed areas, only authorized areas. */
    /* ------------------------------------------ */
    
    if ( !str_cmp( "changed", arg1 ) )
    {
        char buf[MAX_INPUT_LENGTH];
        
        save_area_list();
        
        if ( ch )
            send_to_char( "Saved zones:\n\r", ch );
        else
            log_string( "Saved zones:" );
        
        sprintf( buf, "None.\n\r" );
        
        for( pArea = area_first; pArea; pArea = pArea->next )
        {
            /* Builder must be assigned this area. */
            if ( ch && !IS_BUILDER( ch, pArea ) )
                continue;
            
            if ( IS_SET(pArea->area_flags, AREA_CLONE) )
                continue;

            /* Save changed areas. */
            if ( IS_SET(pArea->area_flags, AREA_CHANGED) )
            {
                save_area( pArea );
                sprintf( buf, "%24s - '%s'", pArea->name, pArea->file_name );
                if ( ch )
                {
                    send_to_char( buf, ch );
                    send_to_char( "\n\r", ch );
                    wiznet(buf, ch, NULL, WIZ_ASAVE, 0, get_trust(ch)); 
                }
                else
                    log_string( buf );
                REMOVE_BIT( pArea->area_flags, AREA_CHANGED );
            }
        }
        if ( !str_cmp( buf, "None.\n\r" ) )
        {
            if ( ch )
                send_to_char( buf, ch );
            else
                log_string( "None." );
        }
        return;
    }
    
    /* Save the area.lst file. */
    /* ----------------------- */
    if ( !str_cmp( arg1, "list" ) )
    {
        save_area_list();
        return;
    }
    
    /* Save area being edited, if authorized. */
    /* -------------------------------------- */
    if ( !str_cmp( arg1, "area" ) )
    {
        if ( !ch || !ch->desc )
            return;
        
        /* Is character currently editing. */
        if ( ch->desc->editor == ED_NONE )
        {
            send_to_char( "You are not editing an area, "
                "therefore an area vnum is required.\n\r", ch );
            return;
        }
        
        /* Find the area to save. */
        switch (ch->desc->editor)
        {
        case ED_AREA:
            pArea = (AREA_DATA *)ch->desc->pEdit;
            break;
        case ED_ROOM:
            pArea = ch->in_room->area;
            break;
        case ED_OBJECT:
            pArea = ( (OBJ_INDEX_DATA *)ch->desc->pEdit )->area;
            break;
        case ED_MOBILE:
            pArea = ( (MOB_INDEX_DATA *)ch->desc->pEdit )->area;
            break;
        default:
            pArea = ch->in_room->area;
            break;
        }
        
        if ( !IS_BUILDER( ch, pArea ) )
        {
            send_to_char( "You are not a builder for this area.\n\r", ch );
            return;
        }
        
        save_area_list();
        save_area( pArea );
        REMOVE_BIT( pArea->area_flags, AREA_CHANGED );
        send_to_char( "Area saved.\n\r", ch );
        return;
    }
    
    /* Save Help File */
    if(!str_cmp(arg1, "helps"))
    {
        save_other_helps();
        send_to_char( "Helps Saved.\n\r", ch);
        return;
    }

	if(!str_cmp(arg1, "skills"))
	{
		save_skills();
		send_to_char("Skills Saved.\n\r", ch);
		return;
	}
    
    /* Show correct syntax. */
    /* -------------------- */
    if (ch)
        do_asave( ch, "" );
    
    return;
}




void save_helps( FILE *fp, HELP_AREA *ha )
{
    HELP_DATA *help = ha->first;
    
    fprintf( fp, "#HELPS\n" );
    
    for ( ; help; help = help->next_area )
    {
        if(help->to_delete)
            continue;
        
        rfprintf( fp, "%d %s~\n", help->level, help->keyword );
        rfprintf( fp, "%s~\n\n", fix_string( help->text ) );
    }
    
    fprintf( fp, "-1 $~\n\n" );
    
    return;
}

void save_other_helps( void )
{
    extern HELP_AREA * had_list;
    HELP_AREA *ha;
    FILE *fp;

    for ( ha = had_list; ha; ha = ha->next )
        if ( ha->area == NULL )
        {
            fp = fopen( ha->filename, "w" );

            if ( !fp )
            {
                log_error( ha->filename );
                return;
            }

            save_helps( fp, ha );
            fprintf( fp, "#$\n" );
            fclose( fp );
        }

    return;
}
