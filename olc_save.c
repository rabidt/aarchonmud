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

void save_helps( LStbl *parent, HELP_AREA *ha );
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

void save_mobprogs( LStbl *parent, AREA_DATA *pArea )
{
    PROG_CODE *pMprog;
    int i;
    
    LSarr progs;
    LSarr_create(&progs);
    LStbl_kv_arr(parent, "MobProgs", &progs);
    
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pMprog = get_mprog_index(i) ) != NULL)
        {
            LStbl prog;
            LStbl_create(&prog);
            LSarr_add_tbl(&progs, &prog);

            LStbl_kv_int(&prog, "Vnum", i);
            LStbl_kv_bool(&prog, "Lua", pMprog->is_lua);
            LStbl_kv_int(&prog, "Security", pMprog->security);
            LStbl_kv_str(&prog, "Code", fix_string(pMprog->code));

            LStbl_release(&prog);
        }
    }
    
    LSarr_release(&progs);
    return;
}

void save_objprogs( LStbl *parent, AREA_DATA *pArea )
{
    PROG_CODE *pOprog;
    int i;

    LSarr progs;
    LSarr_create(&progs);
    LStbl_kv_arr(parent, "ObjProgs", &progs);

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pOprog = get_oprog_index(i) ) != NULL)
        {
            LStbl prog;
            LStbl_create(&prog);
            LSarr_add_tbl(&progs, &prog);

            LStbl_kv_int(&prog, "Vnum", i);
            LStbl_kv_int(&prog, "Security", pOprog->security);
            LStbl_kv_str(&prog, "Code", fix_string(pOprog->code));

            LStbl_release(&prog);
        }
    }

    LSarr_release(&progs);
    return;
}

void save_areaprogs( LStbl *parent, AREA_DATA *pArea )
{
    PROG_CODE *pAprog;
    int i;

    LSarr progs;
    LSarr_create(&progs);
    LStbl_kv_arr(parent, "AreaProgs", &progs);

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pAprog = get_aprog_index(i) ) != NULL)
        {
            LStbl prog;
            LStbl_create(&prog);
            LSarr_add_tbl(&progs, &prog);

            LStbl_kv_int(&prog, "Vnum", i);
            LStbl_kv_int(&prog, "Security", pAprog->security);
            LStbl_kv_str(&prog, "Code", fix_string(pAprog->code));

            LStbl_release(&prog);
        }
    }

    LSarr_release(&progs);
    return;
}

void save_roomprogs( LStbl *parent, AREA_DATA *pArea )
{
    PROG_CODE *pRprog;
    int i;

    LSarr progs;
    LSarr_create(&progs);
    LStbl_kv_arr(parent, "RoomProgs", &progs);

    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pRprog = get_rprog_index(i) ) != NULL)
        {
            LStbl prog;
            LStbl_create(&prog);
            LSarr_add_tbl(&progs, &prog);

            LStbl_kv_int(&prog, "Vnum", i);
            LStbl_kv_int(&prog, "Security", pRprog->security);
            LStbl_kv_str(&prog, "Code", fix_string(pRprog->code));

            LStbl_release(&prog);
        }
    }

    LSarr_release(&progs);
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
void save_mobble( LSarr *parent, MOB_INDEX_DATA *pMobIndex )
{
    LStbl mobble;
    LStbl_create( &mobble );
    LSarr_add_tbl( parent, &mobble );
    sh_int race = pMobIndex->race;
    PROG_LIST *pMprog;
    
    LStbl_kv_int( &mobble, "Vnum", pMobIndex->vnum );
    LStbl_kv_str( &mobble, "Name", pMobIndex->player_name );
    LStbl_kv_str( &mobble, "ShortDescr", pMobIndex->short_descr);
    LStbl_kv_str( &mobble, "LongDescr", pMobIndex->long_descr);
    LStbl_kv_str( &mobble, "Description", pMobIndex->description);
    LStbl_kv_str( &mobble, "Comments", pMobIndex->comments);
    LStbl_kv_str( &mobble, "Race", race_table[race].name);
    LStbl_kv_str( &mobble, "Sex", sex_table[pMobIndex->sex].name);
    

    if (!flag_equal(pMobIndex->act, race_table[race].act))
        LStbl_kv_flags(&mobble, "Act", act_flags, pMobIndex->act);
    if (!flag_equal(pMobIndex->affect_field, race_table[race].affect_field))
        LStbl_kv_flags(&mobble, "Affects", affect_flags, pMobIndex->affect_field);
    if (!flag_equal(pMobIndex->off_flags, race_table[race].off))
        LStbl_kv_flags(&mobble, "Offensive", off_flags, pMobIndex->off_flags);
    if (!flag_equal(pMobIndex->imm_flags, race_table[race].imm))
        LStbl_kv_flags(&mobble, "Immune", imm_flags, pMobIndex->imm_flags);
    if (!flag_equal(pMobIndex->res_flags, race_table[race].res))
        LStbl_kv_flags(&mobble, "Resist", res_flags, pMobIndex->res_flags);
    if (!flag_equal(pMobIndex->vuln_flags, race_table[race].vuln))
        LStbl_kv_flags(&mobble, "Vuln", vuln_flags, pMobIndex->vuln_flags);

    if (!flag_equal(pMobIndex->form, race_table[race].form))
        LStbl_kv_flags(&mobble, "Form", form_flags, pMobIndex->form);
    if (!flag_equal(pMobIndex->parts, race_table[race].parts))
       LStbl_kv_flags(&mobble, "Parts", part_flags, pMobIndex->parts);

    LStbl_kv_int(&mobble, "Level", pMobIndex->level);

    if (pMobIndex->hitpoint_percent != 100)
        LStbl_kv_int(&mobble, "HpPcnt", pMobIndex->hitpoint_percent);
    if (pMobIndex->mana_percent != 100)
        LStbl_kv_int(&mobble, "ManaPcnt", pMobIndex->mana_percent);
    if (pMobIndex->move_percent != 100)
        LStbl_kv_int(&mobble, "MovePcnt", pMobIndex->move_percent);
    if (pMobIndex->hitroll_percent != 100)
        LStbl_kv_int(&mobble, "HitrollPcnt", pMobIndex->hitpoint_percent);
    if (pMobIndex->damage_percent != 100)
        LStbl_kv_int(&mobble, "DamrollPcnt", pMobIndex->damage_percent);
    if (pMobIndex->ac_percent != 100)
        LStbl_kv_int(&mobble, "AcPcnt", pMobIndex->ac_percent);
    if (pMobIndex->saves_percent != 100)
        LStbl_kv_int(&mobble, "SavesPcnt", pMobIndex->saves_percent);
    if (pMobIndex->wealth_percent != 100)
        LStbl_kv_int(&mobble, "WealthPcnt", pMobIndex->wealth_percent);
    
    LStbl_kv_str(&mobble, "DamtypeName", attack_table[pMobIndex->dam_type].name);

    if (pMobIndex->stance != STANCE_DEFAULT)
        LStbl_kv_str(&mobble, "Stance", stances[pMobIndex->stance].name); 
     
    if (pMobIndex->size != SIZE_MEDIUM)
        LStbl_kv_str(&mobble, "Size", size_table[pMobIndex->size].name);

    if (pMobIndex->alignment != 0)
        LStbl_kv_int(&mobble, "Align", pMobIndex->alignment);
    if (pMobIndex->group != 0)
        LStbl_kv_int(&mobble, "Group", pMobIndex->group);
    

    if (pMobIndex->start_pos != POS_STANDING)
        LStbl_kv_str(&mobble, "StartPosition", position_table[pMobIndex->start_pos].short_name);
    if (pMobIndex->default_pos != POS_STANDING)
        LStbl_kv_str(&mobble, "DefaultPosition", position_table[pMobIndex->default_pos].short_name);
       
    LSarr mtrigs;
    LSarr_create(&mtrigs);
    LStbl_kv_arr(&mobble, "MTrigs", &mtrigs);
    reverse_mprog_order(pMobIndex);    
    for (pMprog = pMobIndex->mprogs; pMprog; pMprog = pMprog->next)
    {
        LStbl mtrig;
        LStbl_create(&mtrig);
        LSarr_add_tbl(&mtrigs, &mtrig);
        LStbl_kv_str(&mtrig, "Type", name_lookup(pMprog->trig_type, mprog_flags));
        LStbl_kv_int(&mtrig, "Vnum", pMprog->vnum);
        LStbl_kv_str(&mtrig, "Phrase", pMprog->trig_phrase);
        LStbl_release(&mtrig);
    }
    reverse_mprog_order(pMobIndex);
    LSarr_release(&mtrigs);
    

    LStbl_release(&mobble); 
    return;
}


/*****************************************************************************
Name:           save_mobbles
Purpose:        Save #MOBBLES secion of an area file.
Called by:      save_area(olc_save.c).
****************************************************************************/
void save_mobbles( LStbl *parent, AREA_DATA *pArea )
{
    int i;
    MOB_INDEX_DATA *pMob;
    
    LSarr mobbles;
    LSarr_create( &mobbles );
    LStbl_kv_arr( parent, "Mobbles", &mobbles );
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pMob = get_mob_index( i )) )
            save_mobble( &mobbles, pMob );
    }
    LSarr_release( &mobbles );
    
    return;
}


/*****************************************************************************
Name:		save_object
Purpose:	Save one object to file.
new ROM format saving -- Hugin
Called by:	save_objects (below).
****************************************************************************/
void save_object( LSarr *parent, OBJ_INDEX_DATA *pObjIndex )
{
    AFFECT_DATA *pAf;
    EXTRA_DESCR_DATA *pEd;
    
    LStbl object;
    LStbl_create(&object);
    LSarr_add_tbl(parent, &object);

    LStbl_kv_int(&object, "Vnum", pObjIndex->vnum);
    LStbl_kv_str(&object, "Name", pObjIndex->name);
    LStbl_kv_str(&object, "ShortDescr", pObjIndex->short_descr);
    LStbl_kv_str(&object, "Description", fix_string(pObjIndex->description));
    LStbl_kv_str(&object, "Material", pObjIndex->material);
    
    LStbl_kv_str(&object, "ItemType", item_name(pObjIndex->item_type));
    LStbl_kv_flags(&object, "Extra", extra_flags, pObjIndex->extra_flags);
    LStbl_kv_str(&object, "WearType", flag_bit_name( wear_types, pObjIndex->wear_type));
    
    
    /*
    *  Using fwrite_flag to write most values gives a strange
    *  looking area file, consider making a case for each
    *  item type later.
    */
    
    switch ( pObjIndex->item_type )
    {
    default:
        {
        LStbl values;
        LStbl_create(&values);
        LStbl_kv_tbl(&object, "Values", &values);
        int i;
        for (i=0; i<=4; i++)
        {
            LStbl_iv_int(&values, i, pObjIndex->value[i]);
        }
        LStbl_release(&values);
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
    
    LStbl_kv_int( &object, "Level", pObjIndex->level);
    LStbl_kv_int( &object, "Weight", pObjIndex->weight);
    LStbl_kv_int( &object, "Cost", pObjIndex->cost);
    
    if (pObjIndex->clan > 0)
        LStbl_kv_str(&object, "Clan", clan_table[pObjIndex->clan].name );

    if (pObjIndex->rank > 0 && pObjIndex->clan > 0)
        LStbl_kv_str(&object, "ClanRank", clan_table[pObjIndex->clan].rank_list[pObjIndex->rank].name );

    if (pObjIndex->combine_vnum > 0)
        LStbl_kv_int(&object, "CombineVnum", pObjIndex->combine_vnum);

    if (pObjIndex->diff_rating > 0)
        LStbl_kv_int(&object, "Rating", pObjIndex->diff_rating);
    
    LSarr affects;
    LSarr_create(&affects);
    LStbl_kv_arr(&object, "Affects", &affects);
    reverse_affect_order(pObjIndex);    
    for( pAf = pObjIndex->affected; pAf; pAf = pAf->next )
    {
        LStbl affect;
        LStbl_create(&affect);
        LSarr_add_tbl(&affects, &affect);
        if (pAf->where == TO_OBJECT)
        {
            LStbl_kv_str(&affect, "Where", flag_bit_name(apply_types, pAf->where));
            LStbl_kv_str(&affect, "Location", flag_bit_name(apply_flags, pAf->location));
            LStbl_kv_int(&affect, "Modifier", pAf->modifier);
        }
        else
        {
            switch(pAf->where)
            {
            case TO_AFFECTS:
            case TO_IMMUNE:
            case TO_RESIST:
            case TO_VULN:
                LStbl_kv_str(&affect, "Where", flag_bit_name(apply_types, pAf->where));
                break;
            default:
                bug( "olc_save: Invalid Affect->where (%d)", pAf->where);
                break;
            }
            
            if (pAf->bitvector == 0)
                bug( "olc_save: bitvector == 0 for object %d", pObjIndex->vnum ); 

            LStbl_kv_str(&affect, "Location", flag_bit_name(apply_flags, pAf->location));
            LStbl_kv_int(&affect, "Modifier", pAf->modifier);
            LStbl_kv_int(&affect, "Bitvector", pAf->bitvector);
        }
        if (pAf->detect_level != 0)
            LStbl_kv_int(&affect, "DetectLevel", pAf->detect_level);

        LStbl_release(&affect);
    }
    reverse_affect_order(pObjIndex);
    LSarr_release(&affects);
    
    LSarr extradesc;
    LSarr_create(&extradesc);
    LStbl_kv_arr(&object, "ExtraDesc", &extradesc);
    for( pEd = pObjIndex->extra_descr; pEd; pEd = pEd->next )
    {
        LStbl ed;
        LStbl_create(&ed);
        LSarr_add_tbl(&extradesc, &ed);
        LStbl_kv_str(&ed, "Keyword", pEd->keyword);
        LStbl_kv_str(&ed, "Description", fix_string( pEd->description ));
        LStbl_release(&ed);
    }
    LSarr_release(&extradesc);

    /* save oprogs if any */
    LSarr otrigs;
    LSarr_create(&otrigs);
    LStbl_kv_arr(&object, "OTrigs", &otrigs);
    PROG_LIST *pOprog;
    reverse_oprog_order(pObjIndex);
    for (pOprog = pObjIndex->oprogs; pOprog; pOprog = pOprog->next)
    {
        LStbl otrig;
        LStbl_create(&otrig);
        LSarr_add_tbl(&otrigs, &otrig);
        LStbl_kv_str(&otrig, "Type", name_lookup(pOprog->trig_type, oprog_flags));
        LStbl_kv_int(&otrig, "Vnum", pOprog->vnum);
        LStbl_kv_str(&otrig, "Phrase", pOprog->trig_phrase);
        LStbl_release(&otrig);
    }
    reverse_oprog_order(pObjIndex); 
    LSarr_release(&otrigs);

 
    LStbl_release(&object);   
    return;
}




/*****************************************************************************
Name:		save_objects
Purpose:	Save #OBJECTS section of an area file.
Called by:	save_area(olc_save.c).
Notes:         Changed for ROM OLC.
****************************************************************************/
void save_objects( LStbl *parent, AREA_DATA *pArea )
{
    int i;
    OBJ_INDEX_DATA *pObj;
    
    LSarr objects;
    LSarr_create(&objects);
    LStbl_kv_arr(parent, "Objects", &objects); 
    for( i = pArea->min_vnum; i <= pArea->max_vnum; i++ )
    {
        if ( (pObj = get_obj_index( i )) )
            save_object( &objects, pObj );
    }
    LSarr_release(&objects);
    return;
}





/*****************************************************************************
Name:		save_rooms
Purpose:	Save #ROOMS section of an area file.
Called by:	save_area(olc_save.c).
****************************************************************************/
void save_rooms( LStbl *parent, AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoomIndex;
    EXTRA_DESCR_DATA *pEd;
    EXIT_DATA *pExit;
    int iHash;
    int door;
    
    LSarr rooms;
    LSarr_create( &rooms );
    LStbl_kv_arr( parent, "Rooms", &rooms);

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
        {
            if ( pRoomIndex->area == pArea )
            {
                LStbl room;
                LStbl_create(&room);
                LSarr_add_tbl(&rooms, &room);

                LStbl_kv_int(&room, "Vnum", pRoomIndex->vnum);
                LStbl_kv_str(&room, "Name", pRoomIndex->name);
                LStbl_kv_str(&room, "Description", 
                        fix_string(pRoomIndex->description));

                LStbl_kv_flags(&room, "RoomFlags", room_flags, 
                        pRoomIndex->room_flags);
                LStbl_kv_str(&room, "SectorType", 
                        flag_bit_name(sector_flags, pRoomIndex->sector_type));

                LSarr extra_descr;
                LSarr_create(&extra_descr);
                for ( pEd = pRoomIndex->extra_descr; pEd;
                pEd = pEd->next )
                {
                    LStbl ed;
                    LStbl_create(&ed);
                    LSarr_add_tbl(&extra_descr, &ed);

                    LStbl_kv_str(&ed, "Keyword", pEd->keyword);
                    LStbl_kv_str(&ed, "Description",
                            fix_string(pEd->description));
                    LStbl_release(&ed);
                }
                LSarr_release(&extra_descr);

                LSarr exits;
                LSarr_create(&exits);
                LStbl_kv_arr(&room, "Exits", &exits);
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
                        LStbl ex;
                        LStbl_create(&ex);
                        LSarr_add_tbl(&exits, &ex);

                        LStbl_kv_str(&ex, "Direction", 
                                dir_name[pExit->orig_door]);
                        LStbl_kv_str(&ex, "Description", 
                                fix_string(pExit->description));
                        LStbl_kv_str(&ex, "Keyword", pExit->keyword);
                        LStbl_kv_flags(&ex, "Flags", exit_flags, 
                                pExit->rs_flags);
                        LStbl_kv_int(&ex, "Key", pExit->key);
                        LStbl_kv_int(&ex, "Destination", pExit->u1.to_room->vnum);
			/*
                        fprintf( fp, "%d %d %d\n", locks,
                            pExit->key,
                            pExit->u1.to_room->vnum );
			*/
                        LStbl_release(&ex);
                    }
                }
                LSarr_release(&exits);

                if (pRoomIndex->mana_rate != 100 || pRoomIndex->heal_rate != 100)
                {
                    LStbl_kv_int(&room, "ManaRate", pRoomIndex->mana_rate);
                    LStbl_kv_int(&room, "HealRate", pRoomIndex->heal_rate);
                }

                if (pRoomIndex->clan > 0)
                {
                    LStbl_kv_str(&room, "Clan", clan_table[pRoomIndex->clan].name);
                }
                
                if (pRoomIndex->clan_rank > 0 && pRoomIndex->clan > 0)
                {
                    LStbl_kv_str(&room, "ClanRank", clan_table[pRoomIndex->clan].rank_list[pRoomIndex->clan_rank].name);

                }
                
                if (!IS_NULLSTR(pRoomIndex->owner))
                {
                    LStbl_kv_str(&room, "Owner", pRoomIndex->owner);
                }
                
                /* save rprogs if any */
                if (pRoomIndex->rprogs != NULL)
                {
                    LSarr rprogs;
                    LSarr_create(&rprogs);
                    LStbl_kv_arr(&room, "RProgs", &rprogs);

                    PROG_LIST *pRprog;
                    reverse_rprog_order(pRoomIndex);
                    for (pRprog = pRoomIndex->rprogs; pRprog; pRprog = pRprog->next)
                    {
                        LStbl prog;
                        LStbl_create(&prog);
                        LSarr_add_tbl(&rprogs, &prog);
                        LStbl_kv_str(&prog, "TrigType", name_lookup(pRprog->trig_type, rprog_flags));
                        LStbl_kv_int(&prog, "Vnum", pRprog->vnum);
                        LStbl_kv_str(&prog, "Phrase", pRprog->trig_phrase);

                        LStbl_release(&prog);
                    }
                    reverse_rprog_order(pRoomIndex);

                    LSarr_release(&rprogs);
                }

                LStbl_kv_str(&room, "Comments", pRoomIndex->comments);
                
                LStbl_release(&room);
            }
        }
    }
    LSarr_release(&rooms);
    return;
}



/*****************************************************************************
Name:		save_specials
Purpose:	Save #SPECIALS section of area file.
Called by:	save_area(olc_save.c).
****************************************************************************/
void save_specials( LStbl *parent, AREA_DATA *pArea )
{
    int iHash;
    MOB_INDEX_DATA *pMobIndex;
    
    LSarr specials;
    LSarr_create(&specials);
    LStbl_kv_arr(parent, "Specials", &specials);
    
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->spec_fun )
            {
                LStbl special;
                LStbl_create(&special);
                LSarr_add_tbl(&specials, &special);

                LStbl_kv_int(&special, "Vnum", pMobIndex->vnum);
                LStbl_kv_str(&special, "SpecName", spec_name_lookup(pMobIndex->spec_fun));

                LStbl_release(&special);
            }
        }
    }
    
    LSarr_release(&specials);
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
void save_resets( LStbl *parent,  AREA_DATA *pArea )
{
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pLastMob = NULL;
    OBJ_INDEX_DATA *pLastObj;
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    int iHash;

    LSarr resets;
    LSarr_create(&resets);
    LStbl_kv_arr(parent, "Resets", &resets);

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next )
        {
            if ( pRoom->area == pArea )
            {
                for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
                {
                    LStbl rst;
                    LStbl_create(&rst);
                    LSarr_add_tbl(&resets, &rst);

                    char cmd[2];
                    sprintf(cmd, "%c", pReset->command);
                    LStbl_kv_str(&rst, "Command", cmd);
                    switch ( pReset->command )
                    {
                        default:
                            bug( "Save_resets: bad command %c.", pReset->command );
                            break;

                        case 'M':
                            pLastMob = get_mob_index( pReset->arg1 );
                            LStbl_kv_int(&rst, "Arg1", pReset->arg1);
                            LStbl_kv_int(&rst, "Arg2", pReset->arg2);
                            LStbl_kv_int(&rst, "Arg3", pReset->arg3);
                            LStbl_kv_int(&rst, "Arg4", pReset->arg4);
                            break;

                        case 'O':
                            pLastObj = get_obj_index( pReset->arg1 );
                            pRoom = get_room_index( pReset->arg3 );
                            LStbl_kv_int(&rst, "Arg1", pReset->arg1);
                            LStbl_kv_int(&rst, "Arg3", pReset->arg3);
                            break;

                        case 'P':
                            pLastObj = get_obj_index( pReset->arg1 );
                            LStbl_kv_int(&rst, "Arg1", pReset->arg1);
                            LStbl_kv_int(&rst, "Arg2", pReset->arg2);
                            LStbl_kv_int(&rst, "Arg3", pReset->arg3);
                            LStbl_kv_int(&rst, "Arg4", pReset->arg4);
                            break;

                        case 'G':
                            LStbl_kv_int(&rst, "Arg1", pReset->arg1);
                            if ( !pLastMob )
                            {
                                sprintf( buf,
                                        "Save_resets: !NO_MOB! in [%s]", pArea->file_name );
                                bug( buf, 0 );
                            }
                            break;

                        case 'E':
                            LStbl_kv_int(&rst, "Arg1", pReset->arg1);
                            LStbl_kv_int(&rst, "Arg3", pReset->arg3);
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
                            LStbl_kv_int(&rst, "Arg1", pReset->arg1);
                            LStbl_kv_int(&rst, "Arg2", pReset->arg2);
                            break;
                    }

                    LStbl_release(&rst);
                }
            }	/* End if correct area */
        }	/* End for pRoom */
    }	/* End for iHash */
    LSarr_release(&resets);
    return;
}

void save_bossachievements( LStbl *parent, AREA_DATA *pArea )
{
    BOSSACHV *pBoss;
    MOB_INDEX_DATA *pMobIndex;
    int iHash;
    
    LSarr achvs;
    LSarr_create(&achvs);
    LStbl_kv_arr(parent, "BossAchievements", &achvs);

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->boss_achieve )
            {
                LStbl achv;
                LStbl_create(&achv);
                LSarr_add_tbl(&achvs, &achv);

                pBoss = pMobIndex->boss_achieve;
                
                LStbl_kv_int(&achv, "Vnum", pMobIndex->vnum);
                LStbl_kv_int(&achv, "ExpReward", pBoss->exp_reward);
                LStbl_kv_int(&achv, "GoldReward", pBoss->gold_reward);
                LStbl_kv_int(&achv, "QuestReward", pBoss->quest_reward);
                LStbl_kv_int(&achv, "AchReward", pBoss->ach_reward);

                LStbl_release(&achv);
            }
        }
    }
   
    LSarr_release(&achvs); 
    return;
}


/*****************************************************************************
Name:		save_shops
Purpose:	Saves the #SHOPS section of an area file.
Called by:	save_area(olc_save.c)
****************************************************************************/
void save_shops( LStbl *parent, AREA_DATA *pArea )
{
    SHOP_DATA *pShopIndex;
    MOB_INDEX_DATA *pMobIndex;
    int iTrade;
    int iHash;
    
    LSarr shops;
    LSarr_create(&shops);
    LStbl_kv_arr(parent, "Shops", &shops);

    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( pMobIndex = mob_index_hash[iHash]; pMobIndex; pMobIndex = pMobIndex->next )
        {
            if ( pMobIndex && pMobIndex->area == pArea && pMobIndex->pShop )
            {
                LStbl shop;
                LStbl_create(&shop);
                LSarr_add_tbl(&shops, &shop);

                pShopIndex = pMobIndex->pShop;
                
                LStbl_kv_int(&shop, "Keeper", pShopIndex->keeper);

                LSarr buy_types;
                LSarr_create(&buy_types);
                LStbl_kv_arr(&shop, "BuyTypes", &buy_types); 
                for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
                {

                    if ( pShopIndex->buy_type[iTrade] != 0 )
                    {
                        LSarr_add_str(&buy_types, 
                                flag_bit_name(type_flags, pShopIndex->buy_type[iTrade]));
                    }
                }
                LSarr_release(&buy_types);

                LStbl_kv_int(&shop, "ProfitBuy", pShopIndex->profit_buy);
                LStbl_kv_int(&shop, "ProfitSell", pShopIndex->profit_sell);
                LStbl_kv_int(&shop, "OpenHour", pShopIndex->open_hour);
                LStbl_kv_int(&shop, "CloseHour", pShopIndex->close_hour);

                LStbl_release(&shop);
            }
        }
    }
    
    LSarr_release(&shops);
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

    LStbl area;
    LStbl_create( &area );
    LStbl_kv_int( &area, "Version", CURR_AREA_VERSION);

    LSarr clones;
    LSarr_create( &clones);
    LStbl_kv_arr( &area, "Clones", &clones);
    for ( i = 0; i < MAX_AREA_CLONE; i++ )
        if ( pArea->clones[i] > 0 )
            LSarr_add_int( &clones, pArea->clones[i]);
    LSarr_release( &clones);

    LStbl_kv_str( &area, "Name", pArea->name );
    LStbl_kv_str( &area, "Builders", fix_string( pArea->builders ) );
    LStbl_kv_str( &area, "Comments", fix_string( pArea->comments ) );
    LStbl_kv_int( &area, "MinVnum", pArea->min_vnum);
    LStbl_kv_int( &area, "MaxVnum", pArea->max_vnum);
    LStbl_kv_str( &area, "Credits", pArea->credits);
  /* Added minlevel, maxlevel, and miniquests for new areas command
     -Astark Dec 2012 */
    LStbl_kv_int( &area, "MinLevel", pArea->minlevel);
    LStbl_kv_int( &area, "MaxLevel", pArea->maxlevel);
    LStbl_kv_int( &area, "Miniquests", pArea->miniquests);
    LStbl_kv_int( &area, "Security", pArea->security);
    LStbl_kv_int( &area, "Time", pArea->reset_time);

    LSarr aflags;
    LSarr_create( &aflags);
    LStbl_kv_arr( &area, "Flags", &aflags);
    if (IS_SET(pArea->area_flags,AREA_REMORT))
        LSarr_add_str( &aflags, 
                flag_bit_name(area_flags, AREA_REMORT));
    if (IS_SET(pArea->area_flags,AREA_NOQUEST))
        LSarr_add_str( &aflags, 
                flag_bit_name(area_flags, AREA_NOQUEST));
    if (IS_SET(pArea->area_flags,AREA_NOHIDE))
        LSarr_add_str( &aflags, 
                flag_bit_name(area_flags, AREA_NOHIDE));
    if ( IS_SET(pArea->area_flags, AREA_SOLO) )
        LSarr_add_str( &aflags, 
                flag_bit_name(area_flags, AREA_SOLO));
    LSarr_release( &aflags);
    
    LSarr atrigs;
    LSarr_create( &atrigs );
    LStbl_kv_arr( &area, "ATrigs", &atrigs);

    if (pArea->aprogs != NULL)
    {
        PROG_LIST *pAprog;
        reverse_aprog_order(pArea);
        for (pAprog = pArea->aprogs; pAprog; pAprog = pAprog->next)
        {
            LStbl atrig;
            LStbl_create( &atrig);
            LSarr_add_tbl( &atrigs, &atrig);
            LStbl_kv_str( &atrig, "Type", name_lookup(pAprog->trig_type, aprog_flags));
            LStbl_kv_int( &atrig, "Vnum", pAprog->vnum);
            LStbl_kv_str( &atrig, "Phrase", pAprog->trig_phrase);
            LStbl_release( &atrig);
        }
        reverse_aprog_order(pArea);
    }
    LSarr_release( &atrigs);


    
    save_mobbles( &area, pArea );
    save_objects( &area, pArea );
    save_rooms( &area, pArea );
    save_specials( &area, pArea );
    save_resets( &area, pArea );
    save_shops( &area, pArea );
    save_bossachievements( &area, pArea );
    save_mobprogs( &area, pArea );
    save_objprogs( &area, pArea );
	save_areaprogs( &area, pArea );
    save_roomprogs( &area, pArea );
    
    if ( pArea->helps && pArea->helps->first )
        save_helps( &area, pArea->helps );
    
    
    sprintf(buf, "%s.lua", pArea->file_name);
    LStbl_save( &area, buf);
    LStbl_release( &area );

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




void save_helps( LStbl *parent, HELP_AREA *ha )
{
    HELP_DATA *help = ha->first;
    
    LSarr helps;
    LSarr_create(&helps);
    LStbl_kv_arr(parent, "Helps", &helps);
    
    for ( ; help; help = help->next_area )
    {
        if(help->delete)
            continue;
        
        LStbl h;
        LStbl_create(&h);
        LSarr_add_tbl(&helps, &h);

        LStbl_kv_int(&h, "Level", help->level);
        LStbl_kv_str(&h, "Keyword", help->keyword);
        LStbl_kv_str(&h, "Text", fix_string(help->text));

        LStbl_release(&h);
    }
    
    LSarr_release(&helps); 
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
            LStbl h;
            LStbl_create(&h);

            save_helps( &h, ha );

            sprintf(buf, "%s.lua", buf);
            LStbl_save(&h, buf);
            LStbl_release(&h);
        }
    }

    return;
}
