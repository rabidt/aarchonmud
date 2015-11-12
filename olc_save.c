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
#include "lua_file.h"

//#define DIF(a,b) (~((~a)|(b)))
#define DIF(a,b,c) flag_copy(a,b); flag_remove_field(a,c)

void save_helps( LSF *parent, HELP_AREA *ha );
void save_other_helps( void );
void save_skills();
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
void save_area_list()
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

void save_mobprogs( LSF *lsfp, AREA_DATA *pArea )
{
    PROG_CODE *pMprog;
    int i;
    
    LSF_kv_tbl(lsfp, "MobProgs");
    
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pMprog = get_mprog_index(i) ) != NULL)
        {
            LSF_add_tbl(lsfp);

            LSF_kv_int(lsfp, "Vnum", i);
            LSF_kv_bool(lsfp, "Lua", pMprog->is_lua);
            LSF_kv_int(lsfp, "Security", pMprog->security);
            LSF_kv_str(lsfp, "Code", fix_string(pMprog->code));

            LSF_end_tbl(lsfp);
        }
    }
    
    LSF_end_tbl(lsfp);
    return;
}

void save_objprogs( LSF *lsfp, AREA_DATA *pArea )
{
    PROG_CODE *pOprog;
    int i;

    LSF_kv_tbl(lsfp, "ObjProgs");

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pOprog = get_oprog_index(i) ) != NULL)
        {
            LSF_add_tbl(lsfp);

            LSF_kv_int(lsfp, "Vnum", i);
            LSF_kv_int(lsfp, "Security", pOprog->security);
            LSF_kv_str(lsfp, "Code", fix_string(pOprog->code));

            LSF_end_tbl(lsfp);
        }
    }

    LSF_end_tbl(lsfp);
    return;
}

void save_areaprogs( LSF *lsfp, AREA_DATA *pArea )
{
    PROG_CODE *pAprog;
    int i;

    LSF_kv_tbl(lsfp, "AreaProgs");

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pAprog = get_aprog_index(i) ) != NULL)
        {
            LSF_add_tbl(lsfp);

            LSF_kv_int(lsfp, "Vnum", i);
            LSF_kv_int(lsfp, "Security", pAprog->security);
            LSF_kv_str(lsfp, "Code", fix_string(pAprog->code));

            LSF_end_tbl(lsfp);
        }
    }

    LSF_end_tbl(lsfp);
    return;
}

void save_roomprogs( LSF *lsfp, AREA_DATA *pArea )
{
    PROG_CODE *pRprog;
    int i;

    LSF_kv_tbl(lsfp, "RoomProgs");

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pRprog = get_rprog_index(i) ) != NULL)
        {
            LSF_add_tbl(lsfp);

            LSF_kv_int(lsfp, "Vnum", i);
            LSF_kv_int(lsfp, "Security", pRprog->security);
            LSF_kv_str(lsfp, "Code", fix_string(pRprog->code));

            LSF_end_tbl(lsfp);
        }
    }

    LSF_end_tbl(lsfp);
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
void save_mobble( LSF *lsfp, MOB_INDEX_DATA *pMobIndex )
{
    LSF_add_tbl( lsfp);
    sh_int race = pMobIndex->race;
    PROG_LIST *pMprog;
    
    LSF_kv_int( lsfp, "Vnum", pMobIndex->vnum );
    LSF_kv_str( lsfp, "Name", pMobIndex->player_name );
    LSF_kv_str( lsfp, "ShortDescr", pMobIndex->short_descr);
    LSF_kv_str( lsfp, "LongDescr", pMobIndex->long_descr);
    LSF_kv_str( lsfp, "Description", pMobIndex->description);
    LSF_kv_str( lsfp, "Comments", pMobIndex->comments);
    LSF_kv_str( lsfp, "Race", race_table[race].name);
    LSF_kv_str( lsfp, "Sex", sex_table[pMobIndex->sex].name);
    

    if (!flag_equal(pMobIndex->act, race_table[race].act))
        LSF_kv_flags(lsfp, "Act", act_flags, pMobIndex->act);
    if (!flag_equal(pMobIndex->affect_field, race_table[race].affect_field))
        LSF_kv_flags(lsfp, "Affects", affect_flags, pMobIndex->affect_field);
    if (!flag_equal(pMobIndex->off_flags, race_table[race].off))
        LSF_kv_flags(lsfp, "Offensive", off_flags, pMobIndex->off_flags);
    if (!flag_equal(pMobIndex->imm_flags, race_table[race].imm))
        LSF_kv_flags(lsfp, "Immune", imm_flags, pMobIndex->imm_flags);
    if (!flag_equal(pMobIndex->res_flags, race_table[race].res))
        LSF_kv_flags(lsfp, "Resist", res_flags, pMobIndex->res_flags);
    if (!flag_equal(pMobIndex->vuln_flags, race_table[race].vuln))
        LSF_kv_flags(lsfp, "Vuln", vuln_flags, pMobIndex->vuln_flags);

    if (!flag_equal(pMobIndex->form, race_table[race].form))
        LSF_kv_flags(lsfp, "Form", form_flags, pMobIndex->form);
    if (!flag_equal(pMobIndex->parts, race_table[race].parts))
       LSF_kv_flags(lsfp, "Parts", part_flags, pMobIndex->parts);

    LSF_kv_int(lsfp, "Level", pMobIndex->level);

    if (pMobIndex->hitpoint_percent != 100)
        LSF_kv_int(lsfp, "HpPcnt", pMobIndex->hitpoint_percent);
    if (pMobIndex->mana_percent != 100)
        LSF_kv_int(lsfp, "ManaPcnt", pMobIndex->mana_percent);
    if (pMobIndex->move_percent != 100)
        LSF_kv_int(lsfp, "MovePcnt", pMobIndex->move_percent);
    if (pMobIndex->hitroll_percent != 100)
        LSF_kv_int(lsfp, "HitrollPcnt", pMobIndex->hitpoint_percent);
    if (pMobIndex->damage_percent != 100)
        LSF_kv_int(lsfp, "DamrollPcnt", pMobIndex->damage_percent);
    if (pMobIndex->ac_percent != 100)
        LSF_kv_int(lsfp, "AcPcnt", pMobIndex->ac_percent);
    if (pMobIndex->saves_percent != 100)
        LSF_kv_int(lsfp, "SavesPcnt", pMobIndex->saves_percent);
    if (pMobIndex->wealth_percent != 100)
        LSF_kv_int(lsfp, "WealthPcnt", pMobIndex->wealth_percent);
    
    LSF_kv_str(lsfp, "DamtypeName", attack_table[pMobIndex->dam_type].name);

    if (pMobIndex->stance != STANCE_DEFAULT)
        LSF_kv_str(lsfp, "Stance", stances[pMobIndex->stance].name); 
     
    if (pMobIndex->size != SIZE_MEDIUM)
        LSF_kv_str(lsfp, "Size", size_table[pMobIndex->size].name);

    if (pMobIndex->alignment != 0)
        LSF_kv_int(lsfp, "Align", pMobIndex->alignment);
    if (pMobIndex->group != 0)
        LSF_kv_int(lsfp, "Group", pMobIndex->group);
    

    if (pMobIndex->start_pos != POS_STANDING)
        LSF_kv_str(lsfp, "StartPosition", position_table[pMobIndex->start_pos].short_name);
    if (pMobIndex->default_pos != POS_STANDING)
        LSF_kv_str(lsfp, "DefaultPosition", position_table[pMobIndex->default_pos].short_name);
       
    LSF_kv_tbl(lsfp, "MTrigs");
    reverse_mprog_order(pMobIndex);    
    for (pMprog = pMobIndex->mprogs; pMprog; pMprog = pMprog->next)
    {
        LSF_add_tbl(lsfp);
        LSF_kv_str(lsfp, "Type", name_lookup(pMprog->trig_type, mprog_flags));
        LSF_kv_int(lsfp, "Vnum", pMprog->vnum);
        LSF_kv_str(lsfp, "Phrase", pMprog->trig_phrase);
        LSF_end_tbl(lsfp);
    }
    reverse_mprog_order(pMobIndex);
    LSF_end_tbl(lsfp); //MTrigs
    

    LSF_end_tbl(lsfp); //mobble
    return;
}


/*****************************************************************************
Name:           save_mobbles
Purpose:        Save #MOBBLES secion of an area file.
Called by:      save_area(olc_save.c).
****************************************************************************/
void save_mobbles( LSF *lsfp, AREA_DATA *pArea )
{
    int i;
    MOB_INDEX_DATA *pMob;
    
    LSF_kv_tbl( lsfp, "Mobbles");
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pMob = get_mob_index( i )) )
            save_mobble( lsfp, pMob );
    }
    LSF_end_tbl(lsfp);
    
    return;
}


/*****************************************************************************
Name:		save_object
Purpose:	Save one object to file.
new ROM format saving -- Hugin
Called by:	save_objects (below).
****************************************************************************/
void save_object( LSF *lsfp, OBJ_INDEX_DATA *pObjIndex )
{
    AFFECT_DATA *pAf;
    EXTRA_DESCR_DATA *pEd;
    
    LSF_add_tbl(lsfp);

    LSF_kv_int(lsfp, "Vnum", pObjIndex->vnum);
    LSF_kv_str(lsfp, "Name", pObjIndex->name);
    LSF_kv_str(lsfp, "ShortDescr", pObjIndex->short_descr);
    LSF_kv_str(lsfp, "Description", fix_string(pObjIndex->description));
    LSF_kv_str(lsfp, "Material", pObjIndex->material);
    
    LSF_kv_str(lsfp, "ItemType", item_name(pObjIndex->item_type));
    LSF_kv_flags(lsfp, "Extra", extra_flags, pObjIndex->extra_flags);
    LSF_kv_str(lsfp, "WearType", flag_bit_name( wear_types, pObjIndex->wear_type));
    
    
    /*
    *  Using fwrite_flag to write most values gives a strange
    *  looking area file, consider making a case for each
    *  item type later.
    */
    
    switch ( pObjIndex->item_type )
    {
    default:
        {
        LSF_kv_tbl(lsfp, "Values");
        int i;
        for (i=0; i<=4; i++)
        {
            LSF_iv_int(lsfp, i, pObjIndex->value[i]);
        }
        LSF_end_tbl(lsfp);
        break;
        }
        
#if 0 
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
#endif
    }
    
    LSF_kv_int( lsfp, "Level", pObjIndex->level);
    LSF_kv_int( lsfp, "Weight", pObjIndex->weight);
    LSF_kv_int( lsfp, "Cost", pObjIndex->cost);
    
    if (pObjIndex->clan > 0)
        LSF_kv_str(lsfp, "Clan", clan_table[pObjIndex->clan].name );

    if (pObjIndex->rank > 0 && pObjIndex->clan > 0)
        LSF_kv_str(lsfp, "ClanRank", clan_table[pObjIndex->clan].rank_list[pObjIndex->rank].name );

    if (pObjIndex->combine_vnum > 0)
        LSF_kv_int(lsfp, "CombineVnum", pObjIndex->combine_vnum);

    if (pObjIndex->diff_rating > 0)
        LSF_kv_int(lsfp, "Rating", pObjIndex->diff_rating);
    
    LSF_kv_tbl(lsfp, "Affects");
    reverse_affect_order(pObjIndex);    
    for( pAf = pObjIndex->affected; pAf; pAf = pAf->next )
    {
        LSF_add_tbl(lsfp);
        if (pAf->where == TO_OBJECT)
        {
            LSF_kv_str(lsfp, "Where", flag_bit_name(apply_types, pAf->where));
            LSF_kv_str(lsfp, "Location", flag_bit_name(apply_flags, pAf->location));
            LSF_kv_int(lsfp, "Modifier", pAf->modifier);
        }
        else
        {
            switch(pAf->where)
            {
            case TO_AFFECTS:
            case TO_IMMUNE:
            case TO_RESIST:
            case TO_VULN:
                LSF_kv_str(lsfp, "Where", flag_bit_name(apply_types, pAf->where));
                break;
            default:
                bug( "olc_save: Invalid Affect->where (%d)", pAf->where);
                break;
            }
            
            if (pAf->bitvector == 0)
                bug( "olc_save: bitvector == 0 for object %d", pObjIndex->vnum ); 

            LSF_kv_str(lsfp, "Location", flag_bit_name(apply_flags, pAf->location));
            LSF_kv_int(lsfp, "Modifier", pAf->modifier);
            LSF_kv_int(lsfp, "Bitvector", pAf->bitvector);
        }
        if (pAf->detect_level != 0)
            LSF_kv_int(lsfp, "DetectLevel", pAf->detect_level);

        LSF_end_tbl(lsfp);
    }
    reverse_affect_order(pObjIndex);
    LSF_end_tbl(lsfp);
    
    LSF_kv_tbl(lsfp, "ExtraDesc");
    for( pEd = pObjIndex->extra_descr; pEd; pEd = pEd->next )
    {
        LSF_add_tbl(lsfp);
        LSF_kv_str(lsfp, "Keyword", pEd->keyword);
        LSF_kv_str(lsfp, "Description", fix_string( pEd->description ));
        LSF_end_tbl(lsfp);
    }
    LSF_end_tbl(lsfp);

    /* save oprogs if any */
    LSF_kv_tbl(lsfp, "OTrigs");
    PROG_LIST *pOprog;
    reverse_oprog_order(pObjIndex);
    for (pOprog = pObjIndex->oprogs; pOprog; pOprog = pOprog->next)
    {
        LSF_add_tbl(lsfp);
        LSF_kv_str(lsfp, "Type", name_lookup(pOprog->trig_type, oprog_flags));
        LSF_kv_int(lsfp, "Vnum", pOprog->vnum);
        LSF_kv_str(lsfp, "Phrase", pOprog->trig_phrase);
        LSF_end_tbl(lsfp);
    }
    reverse_oprog_order(pObjIndex); 
    LSF_end_tbl(lsfp);

 
    LSF_end_tbl(lsfp);
    return;
}




/*****************************************************************************
Name:		save_objects
Purpose:	Save #OBJECTS section of an area file.
Called by:	save_area(olc_save.c).
Notes:         Changed for ROM OLC.
****************************************************************************/
void save_objects( LSF *lsfp, AREA_DATA *pArea )
{
    int i;
    OBJ_INDEX_DATA *pObj;
    
    LSF_kv_tbl(lsfp, "Objects"); 
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pObj = get_obj_index( i )) )
            save_object( lsfp, pObj );
    }
    LSF_end_tbl(lsfp);
    return;
}





/*****************************************************************************
Name:		save_rooms
Purpose:	Save #ROOMS section of an area file.
Called by:	save_area(olc_save.c).
****************************************************************************/
void save_rooms( LSF *lsfp, AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoomIndex;
    EXTRA_DESCR_DATA *pEd;
    EXIT_DATA *pExit;
    int iHash;
    int door;
    
    LSF_kv_tbl( lsfp, "Rooms");

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
        {
            if ( pRoomIndex->area == pArea )
            {
                LSF_add_tbl(lsfp);

                LSF_kv_int(lsfp, "Vnum", pRoomIndex->vnum);
                LSF_kv_str(lsfp, "Name", pRoomIndex->name);
                LSF_kv_str(lsfp, "Description", 
                        fix_string(pRoomIndex->description));

                LSF_kv_flags(lsfp, "RoomFlags", room_flags, 
                        pRoomIndex->room_flags);
                LSF_kv_str(lsfp, "SectorType", 
                        flag_bit_name(sector_flags, pRoomIndex->sector_type));

                
                LSF_kv_tbl(lsfp, "ExtraDescr");
                for ( pEd = pRoomIndex->extra_descr; pEd;
                pEd = pEd->next )
                {
                    LSF_add_tbl(lsfp);

                    LSF_kv_str(lsfp, "Keyword", pEd->keyword);
                    LSF_kv_str(lsfp, "Description",
                            fix_string(pEd->description));
                    LSF_end_tbl(lsfp);
                }
                LSF_end_tbl(lsfp);

                LSF_kv_tbl(lsfp, "Exits");
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
                        LSF_add_tbl(lsfp);

                        LSF_kv_str(lsfp, "Direction", 
                                dir_name[pExit->orig_door]);
                        LSF_kv_str(lsfp, "Description", 
                                fix_string(pExit->description));
                        LSF_kv_str(lsfp, "Keyword", pExit->keyword);
                        LSF_kv_flags(lsfp, "Flags", exit_flags, 
                                pExit->rs_flags);
                        LSF_kv_int(lsfp, "Key", pExit->key);
                        LSF_kv_int(lsfp, "Destination", pExit->u1.to_room->vnum);
			/*
                        fprintf( fp, "%d %d %d\n", locks,
                            pExit->key,
                            pExit->u1.to_room->vnum );
			*/
                        LSF_end_tbl(lsfp);
                    }
                }
                LSF_end_tbl(lsfp);

                if (pRoomIndex->mana_rate != 100 || pRoomIndex->heal_rate != 100)
                {
                    LSF_kv_int(lsfp, "ManaRate", pRoomIndex->mana_rate);
                    LSF_kv_int(lsfp, "HealRate", pRoomIndex->heal_rate);
                }

                if (pRoomIndex->clan > 0)
                {
                    LSF_kv_str(lsfp, "Clan", clan_table[pRoomIndex->clan].name);
                }
                
                if (pRoomIndex->clan_rank > 0 && pRoomIndex->clan > 0)
                {
                    LSF_kv_str(lsfp, "ClanRank", clan_table[pRoomIndex->clan].rank_list[pRoomIndex->clan_rank].name);

                }
                
                if (!IS_NULLSTR(pRoomIndex->owner))
                {
                    LSF_kv_str(lsfp, "Owner", pRoomIndex->owner);
                }
                
                /* save rprogs if any */
                if (pRoomIndex->rprogs != NULL)
                {
                    LSF_kv_tbl(lsfp, "RProgs");

                    PROG_LIST *pRprog;
                    reverse_rprog_order(pRoomIndex);
                    for (pRprog = pRoomIndex->rprogs; pRprog; pRprog = pRprog->next)
                    {
                        LSF_add_tbl(lsfp);
                        
                        LSF_kv_str(lsfp, "TrigType", name_lookup(pRprog->trig_type, rprog_flags));
                        LSF_kv_int(lsfp, "Vnum", pRprog->vnum);
                        LSF_kv_str(lsfp, "Phrase", pRprog->trig_phrase);

                        LSF_end_tbl(lsfp);
                    }
                    reverse_rprog_order(pRoomIndex);

                    LSF_end_tbl(lsfp);
                }

                LSF_kv_str(lsfp, "Comments", pRoomIndex->comments);
                
                LSF_end_tbl(lsfp);
            }
        }
    }
    LSF_end_tbl(lsfp);
    return;
}



/*****************************************************************************
Name:		save_specials
Purpose:	Save #SPECIALS section of area file.
Called by:	save_area(olc_save.c).
****************************************************************************/
void save_specials( LSF *lsfp, AREA_DATA *pArea )
{
    int iHash;
    MOB_INDEX_DATA *pMobIndex;
    
    LSF_kv_tbl(lsfp, "Specials");
    
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->spec_fun )
            {
                LSF_add_tbl(lsfp);

                LSF_kv_int(lsfp, "Vnum", pMobIndex->vnum);
                LSF_kv_str(lsfp, "SpecName", spec_name_lookup(pMobIndex->spec_fun));

                LSF_end_tbl(lsfp);
            }
        }
    }
    
    LSF_end_tbl(lsfp);
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
void save_resets( LSF *lsfp,  AREA_DATA *pArea )
{
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pLastMob = NULL;
    OBJ_INDEX_DATA *pLastObj;
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    int iHash;

    LSF_kv_tbl(lsfp, "Resets");

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next )
        {
            if ( pRoom->area == pArea )
            {
                for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
                {
                    LSF_add_tbl(lsfp);

                    char cmd[2];
                    sprintf(cmd, "%c", pReset->command);
                    LSF_kv_str(lsfp, "Command", cmd);
                    switch ( pReset->command )
                    {
                        default:
                            bug( "Save_resets: bad command %c.", pReset->command );
                            break;

                        case 'M':
                            pLastMob = get_mob_index( pReset->arg1 );
                            LSF_kv_int(lsfp, "Arg1", pReset->arg1);
                            LSF_kv_int(lsfp, "Arg2", pReset->arg2);
                            LSF_kv_int(lsfp, "Arg3", pReset->arg3);
                            LSF_kv_int(lsfp, "Arg4", pReset->arg4);
                            break;

                        case 'O':
                            pLastObj = get_obj_index( pReset->arg1 );
                            pRoom = get_room_index( pReset->arg3 );
                            LSF_kv_int(lsfp, "Arg1", pReset->arg1);
                            LSF_kv_int(lsfp, "Arg3", pReset->arg3);
                            break;

                        case 'P':
                            pLastObj = get_obj_index( pReset->arg1 );
                            LSF_kv_int(lsfp, "Arg1", pReset->arg1);
                            LSF_kv_int(lsfp, "Arg2", pReset->arg2);
                            LSF_kv_int(lsfp, "Arg3", pReset->arg3);
                            LSF_kv_int(lsfp, "Arg4", pReset->arg4);
                            break;

                        case 'G':
                            LSF_kv_int(lsfp, "Arg1", pReset->arg1);
                            if ( !pLastMob )
                            {
                                sprintf( buf,
                                        "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                                bug( buf, 0 );
                            }
                            break;

                        case 'E':
                            LSF_kv_int(lsfp, "Arg1", pReset->arg1);
                            LSF_kv_int(lsfp, "Arg3", pReset->arg3);
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
                            LSF_kv_int(lsfp, "Arg1", pReset->arg1);
                            LSF_kv_int(lsfp, "Arg2", pReset->arg2);
                            break;
                    }

                    LSF_end_tbl(lsfp);
                }
            }	/* End if correct area */
        }	/* End for pRoom */
    }	/* End for iHash */
    LSF_end_tbl(lsfp);
    return;
}

void save_bossachievements( LSF *lsfp, AREA_DATA *pArea )
{
    BOSSACHV *pBoss;
    MOB_INDEX_DATA *pMobIndex;
    int iHash;
    
    LSF_kv_tbl(lsfp, "BossAchievements");

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->boss_achieve )
            {
                LSF_add_tbl(lsfp);

                pBoss = pMobIndex->boss_achieve;
                
                LSF_kv_int(lsfp, "Vnum", pMobIndex->vnum);
                LSF_kv_int(lsfp, "ExpReward", pBoss->exp_reward);
                LSF_kv_int(lsfp, "GoldReward", pBoss->gold_reward);
                LSF_kv_int(lsfp, "QuestReward", pBoss->quest_reward);
                LSF_kv_int(lsfp, "AchReward", pBoss->ach_reward);

                LSF_end_tbl(lsfp);
            }
        }
    }
   
    LSF_end_tbl(lsfp); 
    return;
}


/*****************************************************************************
Name:		save_shops
Purpose:	Saves the #SHOPS section of an area file.
Called by:	save_area(olc_save.c)
****************************************************************************/
void save_shops( LSF *lsfp, AREA_DATA *pArea )
{
    SHOP_DATA *pShopIndex;
    MOB_INDEX_DATA *pMobIndex;
    int iTrade;
    int iHash;
    
    LSF_kv_tbl(lsfp, "Shops");

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->pShop )
            {
                LSF_add_tbl(lsfp);

                pShopIndex = pMobIndex->pShop;
                
                LSF_kv_int(lsfp, "Keeper", pShopIndex->keeper);

                LSF_kv_tbl(lsfp, "BuyTypes"); 
                for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
                {

                    if ( pShopIndex->buy_type[iTrade] != 0 )
                    {
                        LSF_add_str(lsfp, 
                                flag_bit_name(type_flags, pShopIndex->buy_type[iTrade]));
                    }
                }
                LSF_end_tbl(lsfp);

                LSF_kv_int(lsfp, "ProfitBuy", pShopIndex->profit_buy);
                LSF_kv_int(lsfp, "ProfitSell", pShopIndex->profit_sell);
                LSF_kv_int(lsfp, "OpenHour", pShopIndex->open_hour);
                LSF_kv_int(lsfp, "CloseHour", pShopIndex->close_hour);

                LSF_end_tbl(lsfp);
            }
        }
    }
    
    LSF_end_tbl(lsfp);
    return;
}



/*****************************************************************************
Name:		save_area
Purpose:	Save an area, note that this format is new.
Called by:	do_asave(olc_save.c).
****************************************************************************/
void save_area( AREA_DATA *pArea )
{
    char buf[MSL];
    struct stat st = {0};
    int i;
    
    if ( pArea == NULL || IS_SET(pArea->area_flags, AREA_CLONE) )
	return;

    /*
    if ( stat(pArea->file_name, &st) == 0 )
    {
        sprintf( buf, AREA_BACKUP_DIR "%s", pArea->file_name );
        int result=rename( pArea->file_name, buf );
        if ( result != 0 )
        {
            perror("Error: ");
        }
    }
    */
    if ( pArea->reset_time < 1 ) 
        pArea->reset_time = 15;

    sprintf(buf, "%s.lua", pArea->file_name);
    
    LSF *lArea=LSF_open(buf);
    LSF_kv_int( lArea, "Version", CURR_AREA_VERSION);

    LSF_kv_tbl( lArea, "Clones");
    for ( i = 0; i < MAX_AREA_CLONE; i++ )
        if ( pArea->clones[i] > 0 )
            LSF_add_int( lArea, pArea->clones[i]);
    LSF_end_tbl( lArea);

    LSF_kv_str( lArea, "Name", pArea->name );
    LSF_kv_str( lArea, "Builders", fix_string( pArea->builders ) );
    LSF_kv_str( lArea, "Comments", fix_string( pArea->comments ) );
    LSF_kv_int( lArea, "MinVnum", pArea->min_vnum);
    LSF_kv_int( lArea, "MaxVnum", pArea->max_vnum);
    LSF_kv_str( lArea, "Credits", pArea->credits);
  /* Added minlevel, maxlevel, and miniquests for new areas command
     -Astark Dec 2012 */
    LSF_kv_int( lArea, "MinLevel", pArea->minlevel);
    LSF_kv_int( lArea, "MaxLevel", pArea->maxlevel);
    LSF_kv_int( lArea, "Miniquests", pArea->miniquests);
    LSF_kv_int( lArea, "Security", pArea->security);
    LSF_kv_int( lArea, "Time", pArea->reset_time);

    LSF_kv_tbl( lArea, "Flags");
    if (IS_SET(pArea->area_flags,AREA_REMORT))
        LSF_add_str( lArea, 
                flag_bit_name(area_flags, AREA_REMORT));
    if (IS_SET(pArea->area_flags,AREA_NOQUEST))
        LSF_add_str( lArea, 
                flag_bit_name(area_flags, AREA_NOQUEST));
    if (IS_SET(pArea->area_flags,AREA_NOHIDE))
        LSF_add_str( lArea, 
                flag_bit_name(area_flags, AREA_NOHIDE));
    if ( IS_SET(pArea->area_flags, AREA_SOLO) )
        LSF_add_str( lArea, 
                flag_bit_name(area_flags, AREA_SOLO));
    LSF_end_tbl( lArea );
    
    LSF_kv_tbl( lArea, "ATrigs");

    if (pArea->aprogs != NULL)
    {
        PROG_LIST *pAprog;
        reverse_aprog_order(pArea);
        for (pAprog = pArea->aprogs; pAprog; pAprog = pAprog->next)
        {
            LSF_add_tbl( lArea );
            LSF_kv_str( lArea, "Type", name_lookup(pAprog->trig_type, aprog_flags));
            LSF_kv_int( lArea, "Vnum", pAprog->vnum);
            LSF_kv_str( lArea, "Phrase", pAprog->trig_phrase);
            LSF_end_tbl( lArea);
        }
        reverse_aprog_order(pArea);
    }
    LSF_end_tbl( lArea);


    
    save_mobbles( lArea, pArea );
    save_objects( lArea, pArea );
    save_rooms( lArea, pArea );
    save_specials( lArea, pArea );
    save_resets( lArea, pArea );
    save_shops( lArea, pArea );
    save_bossachievements( lArea, pArea );
    save_mobprogs( lArea, pArea );
    save_objprogs( lArea, pArea );
	save_areaprogs( lArea, pArea );
    save_roomprogs( lArea, pArea );
    
    if ( pArea->helps && pArea->helps->first )
        save_helps( lArea, pArea->helps );
    
     
    LSF_close(lArea);
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




void save_helps( LSF *lsfp, HELP_AREA *ha )
{
    HELP_DATA *help = ha->first;
    
    LSF_kv_tbl(lsfp, "Helps");
    
    for ( ; help; help = help->next_area )
    {
        if(help->delete)
            continue;
        
        LSF_add_tbl(lsfp);

        LSF_kv_int(lsfp, "Level", help->level);
        LSF_kv_str(lsfp, "Keyword", help->keyword);
        LSF_kv_str(lsfp, "Text", fix_string(help->text));

        LSF_end_tbl(lsfp);
    }
    
    LSF_end_tbl(lsfp); 
    return;
}

void save_other_helps( void )
{
    extern HELP_AREA * had_list;
    HELP_AREA *ha;
    char buf[MSL];

    for ( ha = had_list; ha; ha = ha->next )
    {
        if ( ha->area == NULL )
        {
            sprintf(buf, "%s.lua", ha->filename);
            LSF *lsfp=LSF_open(buf);
            save_helps(lsfp, ha);
            LSF_close(lsfp);

        }
    }

    return;
}
