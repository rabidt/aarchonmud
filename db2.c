/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif
#endif

#include "merc.h"
#include "db.h"
#include "tables.h"
#include "lookup.h"
#include "mob_stats.h"


int flag_lookup args( ( const char *name, const struct flag_type *flag_table) );
char *flag_string( const struct flag_type *flag_table, tflag bits );
char *flag_stat_string( const struct flag_type *flag_table, int bit );

#define FLAG_READ_SET(fp,flag,set_flag) fread_tflag(fp,flag); flag_set_field(flag,set_flag)

/*
 * Snarf a mob section.  new style
 */
void load_mobiles( FILE *fp )
{
    MOB_INDEX_DATA_OLD *pMobIndex;
    
    if ( !area_last )   /* OLC */
    {
        bug( "Load_mobiles: no #AREA seen yet.", 0 );
        exit( 1 );
    }
    
    for ( ; ; )
    {
        int vnum;
        char letter;
        int iHash;
        
        letter                          = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_mobiles: # not found.", 0 );
            exit( 1 );
        }
        
        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;
        
        fBootDb = FALSE;
        if ( get_mob_index( vnum ) != NULL )
        {
            bug( "Load_mobiles: vnum %d duplicated.", vnum );
            exit( 1 );
        }
        fBootDb = TRUE;
        
        pMobIndex                       = alloc_mem( sizeof(*pMobIndex) );
        pMobIndex->vnum                 = vnum;
        pMobIndex->area                 = area_last;               /* OLC */
        pMobIndex->new_format		= TRUE;
        newmobs++;
        pMobIndex->player_name          = fread_string( fp );
        pMobIndex->short_descr          = fread_string( fp );
        pMobIndex->long_descr           = fread_string( fp );
        pMobIndex->description          = fread_string( fp );
        pMobIndex->race		 	= race_lookup(fread_string( fp ));
        
        pMobIndex->long_descr[0]        = UPPER(pMobIndex->long_descr[0]);
        pMobIndex->description[0]       = UPPER(pMobIndex->description[0]);
        
        FLAG_READ_SET( fp, pMobIndex->act, race_table[pMobIndex->race].act );
	SET_BIT( pMobIndex->act, ACT_IS_NPC );

        FLAG_READ_SET( fp, pMobIndex->affect_field, 
		       race_table[pMobIndex->race].affect_field );

        pMobIndex->pShop                = NULL;
        pMobIndex->alignment            = fread_number( fp );

        pMobIndex->group                = fread_number( fp );
        
        pMobIndex->level                = fread_number( fp );
        pMobIndex->hitroll              = fread_number( fp );  
        
        /* read hit dice */
        pMobIndex->hit[DICE_NUMBER]     = fread_number( fp );  
        /* 'd'          */                fread_letter( fp ); 
        pMobIndex->hit[DICE_TYPE]   	= fread_number( fp );
        /* '+'          */                fread_letter( fp );   
        pMobIndex->hit[DICE_BONUS]      = fread_number( fp ); 
        
        /* read mana dice */
        pMobIndex->mana[DICE_NUMBER]	= fread_number( fp );
        fread_letter( fp );
        pMobIndex->mana[DICE_TYPE]	= fread_number( fp );
        fread_letter( fp );
        pMobIndex->mana[DICE_BONUS]	= fread_number( fp );
        
        /* read damage dice */
        pMobIndex->damage[DICE_NUMBER]	= fread_number( fp );
        fread_letter( fp );
        pMobIndex->damage[DICE_TYPE]	= fread_number( fp );
        fread_letter( fp );
        pMobIndex->damage[DICE_BONUS]	= fread_number( fp );
        pMobIndex->dam_type		= attack_lookup(fread_word(fp));
        
        /* read armor class */
        pMobIndex->ac[AC_PIERCE]	= fread_number( fp ) * 10;
        pMobIndex->ac[AC_BASH]		= fread_number( fp ) * 10;
        pMobIndex->ac[AC_SLASH]		= fread_number( fp ) * 10;
        pMobIndex->ac[AC_EXOTIC]	= fread_number( fp ) * 10;
        
        /* read flags and add in data from the race table */
        FLAG_READ_SET( fp, pMobIndex->off_flags, race_table[pMobIndex->race].off );
        FLAG_READ_SET( fp, pMobIndex->imm_flags, race_table[pMobIndex->race].imm );
        FLAG_READ_SET( fp, pMobIndex->res_flags, race_table[pMobIndex->race].res );
        FLAG_READ_SET( fp, pMobIndex->vuln_flags, race_table[pMobIndex->race].vuln );
        
        /* vital statistics */
        pMobIndex->start_pos		= position_lookup(fread_word(fp));
        pMobIndex->default_pos		= position_lookup(fread_word(fp));
        pMobIndex->sex			= sex_lookup(fread_word(fp));
        
        pMobIndex->wealth		= fread_number( fp );
        
        FLAG_READ_SET( fp, pMobIndex->form, race_table[pMobIndex->race].form );
        FLAG_READ_SET( fp, pMobIndex->parts, race_table[pMobIndex->race].parts );

        /* size */
        CHECK_POS( pMobIndex->size, size_lookup(fread_word(fp)), "size" );
        /*	pMobIndex->size			= size_lookup(fread_word(fp)); */
        pMobIndex->material		= str_dup(fread_word( fp ));
        pMobIndex->stance = STANCE_DEFAULT;
        
        for ( ; ; )
        {
            letter = fread_letter( fp );
            
            if (letter == 'F')
            {
                char *word;
                tflag vector;
                
                word                    = fread_word(fp);
                fread_tflag( fp, vector );
                
                if (!str_prefix(word,"act"))
                    REMOVE_BITS(pMobIndex->act,vector);
                else if (!str_prefix(word,"aff"))
                    REMOVE_BITS(pMobIndex->affect_field,vector);
                else if (!str_prefix(word,"off"))
                    REMOVE_BITS(pMobIndex->off_flags,vector);
                else if (!str_prefix(word,"imm"))
                    REMOVE_BITS(pMobIndex->imm_flags,vector);
                else if (!str_prefix(word,"res"))
                    REMOVE_BITS(pMobIndex->res_flags,vector);
                else if (!str_prefix(word,"vul"))
                    REMOVE_BITS(pMobIndex->vuln_flags,vector);
                else if (!str_prefix(word,"for"))
                    REMOVE_BITS(pMobIndex->form,vector);
                else if (!str_prefix(word,"par"))
                    REMOVE_BITS(pMobIndex->parts,vector);
                else
                {
                    bug("Flag remove: flag not found.",0);
                    exit(1);
                }
            }
            else if ( letter == 'M' )
            {
                MPROG_LIST *pMprog;
                char *word;
                int trigger = 0;
                
                pMprog              = alloc_perm(sizeof(*pMprog));
                word   		    = fread_word( fp );
                if ( (trigger = flag_lookup( word, mprog_flags )) == NO_FLAG )
                {
                    bug("MOBprogs: invalid trigger.",0);
                    exit(1);
                }
                SET_BIT( pMobIndex->mprog_flags, trigger );
                pMprog->trig_type   = trigger;
                pMprog->vnum        = fread_number( fp );
                pMprog->trig_phrase = fread_string( fp );
                pMprog->next        = pMobIndex->mprogs;
                pMobIndex->mprogs   = pMprog;
            }
            else if ( letter == 'S' )
            {
                char *word = fread_word(fp);
                if (!str_prefix(word, "stance"))
                {
                    pMobIndex->stance = fread_number(fp);
                }
                else
                {
                    bug("Special: not found on vnum %d.", vnum);
                    exit(1);
                }
            }
            else
            {
                ungetc(letter,fp);
                break;
            }
        }
        
        // convert to MOB_INDEX_DATA
        MOB_INDEX_DATA *pMobbleIndex = convert_to_mobble( pMobIndex );
        free_mem( pMobIndex, sizeof(*pMobIndex) );
        
        index_mobile ( pMobbleIndex );
    }
    
    return;
}

/*
 * convert new-style mobile to xtra-new style mobile ("mobble")
 */
#define MCOPY(field) pMobIndex->field = pMobIndexOld->field
#define MCOPY_FLAGS(field) flag_copy(pMobIndex->field, pMobIndexOld->field)
MOB_INDEX_DATA* convert_to_mobble ( MOB_INDEX_DATA_OLD *pMobIndexOld )
{
    MOB_INDEX_DATA *pMobIndex;
    long actual;
    long spec;
    
    pMobIndex = alloc_perm( sizeof(*pMobIndex) );
    
    // identical fields, just copy
    MCOPY(vnum);
    MCOPY(area);
    MCOPY(spec_fun);
    MCOPY(pShop);
    MCOPY(mprogs);
    MCOPY_FLAGS(mprog_flags);

    MCOPY(player_name);
    MCOPY(short_descr);
    MCOPY(long_descr);
    MCOPY(description);
    MCOPY(race);
    MCOPY(sex);
    
    MCOPY_FLAGS(act);
    MCOPY_FLAGS(affect_field);
    MCOPY_FLAGS(off_flags);
    MCOPY_FLAGS(imm_flags);
    MCOPY_FLAGS(res_flags);
    MCOPY_FLAGS(vuln_flags);
    MCOPY_FLAGS(form);
    MCOPY_FLAGS(parts);
    
    MCOPY(level);
    MCOPY(dam_type);
    MCOPY(stance);

    MCOPY(size);
    MCOPY(alignment);
    MCOPY(group);
    MCOPY(start_pos);
    MCOPY(default_pos);

    // new fields
    
    // hitpoints
    actual = average_roll(pMobIndexOld->hit[DICE_NUMBER], pMobIndexOld->hit[DICE_TYPE], pMobIndexOld->hit[DICE_BONUS]);
    spec = average_mob_hp(pMobIndexOld->level);
    pMobIndex->hitpoint_percent = URANGE(50, 100 * actual / UMAX(1,spec), 200);

    pMobIndex->mana_percent     = 100;
    pMobIndex->move_percent     = 100;
    
    // hitroll
    actual = pMobIndexOld->hitroll;
    spec = pMobIndex->level;
    pMobIndex->hitroll_percent  = URANGE(50, 100 + 100 * actual / UMAX(1,spec), 200);
    
    // damage
    actual = average_roll(pMobIndexOld->damage[DICE_NUMBER], pMobIndexOld->damage[DICE_TYPE], 0) + pMobIndexOld->damage[DICE_BONUS] / 4;
    spec = average_mob_damage(pMobIndexOld->level);
    pMobIndex->damage_percent   = URANGE(50, 100 * actual / UMAX(1,spec), 200);
    
    pMobIndex->ac_percent       = 100;
    pMobIndex->saves_percent    = 100;
    
    // wealth - as we don't have shops loaded yet, this will result in excessive wealth percent for shopkeepers
    // however, we cap at 200%, so no big deal
    actual = pMobIndexOld->wealth;
    spec = level_base_wealth(pMobIndex->level);
    pMobIndex->wealth_percent   = URANGE(0, 100 * actual / UMAX(1,spec), 200);
    
    return pMobIndex;
}
#undef MCOPY
#undef MCOPY_FLAGS

/*
 * Snarf a mob section. xtra-new style -- Bobble
 */
#define KEY(keystring) (strcmp(key,keystring) == 0)
void load_mobbles( FILE *fp )
{
    MOB_INDEX_DATA *pMobIndex;
    
    if ( !area_last )
    {
        bug( "Load_mobbles: no #AREA seen yet.", 0 );
        exit( 1 );
    }
    
    for ( ; ; )
    {
        int vnum;
        char letter;
        int iHash;
        
        letter                          = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_mobbles: # not found.", 0 );
            exit( 1 );
        }
        
        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;
        
        fBootDb = FALSE;
        if ( get_mob_index( vnum ) != NULL )
        {
            bug( "Load_mobbles: vnum %d duplicated.", vnum );
            exit( 1 );
        }
        fBootDb = TRUE;
        
        pMobIndex                       = alloc_perm( sizeof(*pMobIndex) );
        pMobIndex->vnum                 = vnum;
        pMobIndex->area                 = area_last;
        pMobIndex->pShop                = NULL;
        // set default values
        pMobIndex->level                = 0;
        pMobIndex->hitpoint_percent     = 100;
        pMobIndex->mana_percent         = 100;
        pMobIndex->move_percent         = 100;
        pMobIndex->hitroll_percent      = 100;
        pMobIndex->damage_percent       = 100;
        pMobIndex->ac_percent           = 100;
        pMobIndex->saves_percent        = 100;
        pMobIndex->wealth_percent       = 100;
        pMobIndex->size                 = SIZE_MEDIUM;
        pMobIndex->alignment            = 0;
        pMobIndex->group                = 0;
        pMobIndex->start_pos            = POS_STANDING;
        pMobIndex->default_pos          = POS_STANDING;
        pMobIndex->stance               = STANCE_DEFAULT;
        
        // now read required and optional fields until END is encountered
        char* key;
        while (TRUE) {
            key = fread_word(fp);

            if KEY("END")
                break;
            else if KEY("NAME")
                pMobIndex->player_name = fread_string( fp );
            else if KEY("SDESC")
                pMobIndex->short_descr = fread_string( fp );
            else if KEY("LDESC")
            {
                pMobIndex->long_descr = fread_string( fp );
                pMobIndex->long_descr[0] = UPPER(pMobIndex->long_descr[0]);
            }
            else if KEY("DESC")
            {
                pMobIndex->description = fread_string( fp );
                pMobIndex->description[0] = UPPER(pMobIndex->description[0]);
            }
            else if KEY("RACE")
            {
                int race = pMobIndex->race = race_lookup(fread_string( fp ));
                // init race-specific defaults
                flag_copy(pMobIndex->act, race_table[race].act);
                flag_copy(pMobIndex->affect_field, race_table[race].affect_field);
                flag_copy(pMobIndex->off_flags, race_table[race].off);
                flag_copy(pMobIndex->imm_flags, race_table[race].imm);
                flag_copy(pMobIndex->res_flags, race_table[race].res);
                flag_copy(pMobIndex->vuln_flags, race_table[race].vuln);
                flag_copy(pMobIndex->form, race_table[race].form);
                flag_copy(pMobIndex->parts, race_table[race].parts);
            }
            else if KEY("SEX")
                pMobIndex->sex = sex_lookup(fread_word(fp));
            else if KEY("ACT")
                fread_tflag(fp, pMobIndex->act);
            else if KEY("AFF")
                fread_tflag(fp, pMobIndex->affect_field);
            else if KEY("OFF")
                fread_tflag(fp, pMobIndex->off_flags);
            else if KEY("IMM")
                fread_tflag(fp, pMobIndex->imm_flags);
            else if KEY("RES")
                fread_tflag(fp, pMobIndex->res_flags);
            else if KEY("VULN")
                fread_tflag(fp, pMobIndex->vuln_flags);
            else if KEY("FORM")
                fread_tflag(fp, pMobIndex->form);
            else if KEY("PARTS")
                fread_tflag(fp, pMobIndex->parts);
            else if KEY("LVL")
                pMobIndex->level = fread_number( fp );
            else if KEY("HP")
                pMobIndex->hitpoint_percent = fread_number( fp );
            else if KEY("MANA")
                pMobIndex->mana_percent = fread_number( fp );
            else if KEY("MOVE")
                pMobIndex->move_percent = fread_number( fp );
            else if KEY("HIT")
                pMobIndex->hitroll_percent = fread_number( fp );
            else if KEY("DAM")
                pMobIndex->damage_percent = fread_number( fp );
            else if KEY("AC")
                pMobIndex->ac_percent = fread_number( fp );
            else if KEY("SAVES")
                pMobIndex->saves_percent = fread_number( fp );
            else if KEY("WEALTH")
                pMobIndex->wealth_percent = fread_number( fp );
            else if KEY("DAMTYPE")
                pMobIndex->dam_type = attack_lookup(fread_word(fp));
            else if KEY("STANCE")
                pMobIndex->stance = fread_number( fp );
            else if KEY("SIZE")
                pMobIndex->size = size_lookup(fread_word(fp));
            else if KEY("ALIGN")
                pMobIndex->alignment = fread_number( fp );
            else if KEY("GROUP")
                pMobIndex->group = fread_number( fp );
            else if KEY("SPOS")
                pMobIndex->start_pos = position_lookup(fread_word(fp));
            else if KEY("DPOS")
                pMobIndex->default_pos = position_lookup(fread_word(fp));
            else if KEY("MPROG")
            {
                MPROG_LIST *pMprog;
                char *word;
                int trigger = 0;
                
                pMprog              = alloc_perm(sizeof(*pMprog));
                word                = fread_word( fp );
                if ( (trigger = flag_lookup( word, mprog_flags )) == NO_FLAG )
                {
                    bugf("load_mobbles.MPROG: invalid trigger '%s' for mobile %d.", word, vnum);
                    exit(1);
                }
                SET_BIT( pMobIndex->mprog_flags, trigger );
                pMprog->trig_type   = trigger;
                pMprog->vnum        = fread_number( fp );
                pMprog->trig_phrase = fread_string( fp );
                pMprog->next        = pMobIndex->mprogs;
                pMobIndex->mprogs   = pMprog;
            }
            else
            {
                bugf("load_mobbles: unknown key '%s' for mobile %d.", key, vnum);
                exit(1);                
            }
        } // end of single mob 
                
        SET_BIT( pMobIndex->act, ACT_IS_NPC );

        index_mobile ( pMobIndex );
    }
    
    return;
}
#undef KEY

/*
 * Snarf an obj section. new style
 */
void load_objects( FILE *fp )
{
    OBJ_INDEX_DATA *pObjIndex;

    if ( !area_last )   /* OLC */
    {
        bug( "Load_objects: no #AREA seen yet.", 0 );
        exit( 1 );
    }

    for ( ; ; )
    {
        int vnum;
        char letter;
        int iHash;
 
        letter                          = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_objects: # not found.", 0 );
            exit( 1 );
        }
 
        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;
 
        fBootDb = FALSE;
        if ( get_obj_index( vnum ) != NULL )
        {
            bug( "Load_objects: vnum %d duplicated.", vnum );
            exit( 1 );
        }
        fBootDb = TRUE;
 
        pObjIndex                       = alloc_perm( sizeof(*pObjIndex) );
        pObjIndex->vnum                 = vnum;
        pObjIndex->area                 = area_last;            /* OLC */
        pObjIndex->new_format           = TRUE;
	pObjIndex->reset_num		= 0;
	pObjIndex->combine_vnum         = 0;
	pObjIndex->diff_rating          = 0;

	newobjs++;
        pObjIndex->name                 = fread_string( fp );
        pObjIndex->short_descr          = fread_string( fp );
        pObjIndex->description          = fread_string( fp );
        pObjIndex->material		= fread_string( fp );
        CHECK_POS(pObjIndex->item_type, item_lookup(fread_word( fp )), "item_type" );
        fread_tflag( fp, pObjIndex->extra_flags );
        fread_tflag( fp, pObjIndex->wear_flags );
	pObjIndex->clan=0;
	pObjIndex->rank=0;

	switch(pObjIndex->item_type)
	{
	case ITEM_WEAPON:
	    pObjIndex->value[0]		= weapon_type(fread_word(fp));
	    pObjIndex->value[1]		= fread_number(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= attack_lookup(fread_word(fp));
	    pObjIndex->value[4]		= fread_flag(fp);
	    break;
	case ITEM_CONTAINER:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_flag(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= fread_number(fp);
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
   case ITEM_DRINK_CON:
	case ITEM_FOUNTAIN:
       pObjIndex->value[0]         = fread_number(fp);
       pObjIndex->value[1]         = fread_number(fp);
       CHECK_POS(pObjIndex->value[2], liq_lookup(fread_word(fp)), "liq_lookup" );

       pObjIndex->value[3]         = fread_number(fp);
       pObjIndex->value[4]         = fread_number(fp);
       break;
	case ITEM_WAND:
	case ITEM_STAFF:
	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_number(fp);
	    pObjIndex->value[2]		= fread_number(fp);
	    pObjIndex->value[3]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[4]		= fread_number(fp);
	    break;
	case ITEM_POTION:
	case ITEM_PILL:
	case ITEM_SCROLL:
 	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[2]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[3]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[4]		= skill_lookup(fread_word(fp));
	    break;
	case ITEM_CIGARETTE:
 	    pObjIndex->value[0]		= fread_number(fp);
	    pObjIndex->value[1]		= fread_number(fp);
	    pObjIndex->value[2]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[3]		= skill_lookup(fread_word(fp));
	    pObjIndex->value[4]		= skill_lookup(fread_word(fp));
	    break;
        case ITEM_EXPLOSIVE:
            pObjIndex->value[0]      = fread_number(fp);
            pObjIndex->value[1]      = fread_number(fp);
            pObjIndex->value[2]      = fread_number(fp);
            pObjIndex->value[3]      = fread_number(fp);
            pObjIndex->value[4]      = fread_number(fp);
            break;
	default:
            pObjIndex->value[0]             = fread_flag( fp );
            pObjIndex->value[1]             = fread_flag( fp );
            pObjIndex->value[2]             = fread_flag( fp );
            pObjIndex->value[3]             = fread_flag( fp );
	    pObjIndex->value[4]		    = fread_flag( fp );
	    break;
	}
	pObjIndex->level		= fread_number( fp );
        pObjIndex->weight               = fread_number( fp );
        pObjIndex->cost                 = fread_number( fp );
	pObjIndex->durability		= fread_number( fp ); 

        /* condition */
        letter 				= fread_letter( fp );
	switch (letter)
 	{
	    case ('P') :		pObjIndex->condition = 100; break;
	    case ('G') :		pObjIndex->condition =  90; break;
	    case ('A') :		pObjIndex->condition =  75; break;
	    case ('W') :		pObjIndex->condition =  50; break;
	    case ('D') :		pObjIndex->condition =  25; break;
	    case ('B') :		pObjIndex->condition =  10; break;
	    case ('R') :		pObjIndex->condition =   0; break;
	    default:			pObjIndex->condition = 100; break;
	}
 
        for ( ; ; )
        {
            char letter;
 
            letter = fread_letter( fp );
 
            if ( letter == 'A' )
            {
                AFFECT_DATA *paf;
 
                paf                     = new_affect();
		paf->where		= TO_OBJECT;
                paf->type               = -1;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = fread_number( fp );
                paf->modifier           = fread_number( fp );
                paf->bitvector          = 0;
                paf->next               = pObjIndex->affected;
                pObjIndex->affected     = paf;
                top_affect++;
            }

            else if (letter == 'B')
            {
                pObjIndex->combine_vnum = fread_number( fp );	
            }

            else if (letter == 'C')
            {
                pObjIndex->clan = clan_lookup(fread_string( fp ));	
            }
            
            else if (letter == 'D')
            {
		/* set detect_level for last affect -
		   screwy but we must keep old area files valid */
		if ( pObjIndex->affected == NULL )
		{
		    bug( "Load_Objects: No affect for detect_level (#%d)", vnum );
		    fread_number( fp );
		}
                pObjIndex->affected->detect_level = fread_number( fp );	
            }

            else if (letter == 'R')
            {
                pObjIndex->rank = clan_rank_lookup(pObjIndex->clan, fread_string( fp ));	
            }

	    else if (letter == 'F')
            {
                AFFECT_DATA *paf;
 
                paf                     = new_affect();
		letter 			= fread_letter(fp);
		switch (letter)
	 	{
		case 'A':
                    paf->where          = TO_AFFECTS;
		    break;
		case 'I':
		    paf->where		= TO_IMMUNE;
		    break;
		case 'R':
		    paf->where		= TO_RESIST;
		    break;
		case 'V':
		    paf->where		= TO_VULN;
		    break;
		default:
            	    bug( "Load_objects: Bad where on flag set.", 0 );
            	   exit( 1 );
		}
                paf->type               = -1;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = fread_number(fp);
                paf->modifier           = fread_number(fp);

		/* small hack for downward-compatibility --Bobble */
		do
		    letter = fread_letter( fp );
		while ( letter == ' ' );
		ungetc( letter, fp );
                paf->bitvector          = fread_flag(fp);
		if ( !( '0' <= letter && letter <= '9' ) )
		{
		    /* bitvector was saved old-style as a flag => convert to new format */
		    FLAG_CONVERT( paf->bitvector );
		}

                paf->next               = pObjIndex->affected;
                pObjIndex->affected     = paf;
                top_affect++;
            }
 
            else if ( letter == 'E' )
            {
                EXTRA_DESCR_DATA *ed;
 
                ed                      = alloc_perm( sizeof(*ed) );
                ed->keyword             = fread_string( fp );
                ed->description         = fread_string( fp );
                ed->next                = pObjIndex->extra_descr;
                pObjIndex->extra_descr  = ed;
                top_ed++;
            }
 
            else if (letter == 'G')
            {
                pObjIndex->diff_rating = fread_number( fp );	
            }

            else
            {
                ungetc( letter, fp );
                break;
            }
        }
 
        iHash                   = vnum % MAX_KEY_HASH;
        pObjIndex->next         = obj_index_hash[iHash];
        obj_index_hash[iHash]   = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   /* OLC */
        assign_area_vnum( vnum );                                   /* OLC */
     }
  
     return;
 }
 
/*****************************************************************************
 Name:	        convert_objects
 Purpose:	Converts all old format objects to new format
 Called by:	boot_db (db.c).
 Note:          Loops over all resets to find the level of the mob
                loaded before the object to determine the level of
                the object.
		It might be better to update the levels in load_resets().
		This function is not pretty.. Sorry about that :)
 Author:        Hugin
 ****************************************************************************/
void convert_objects( void )
{
    int vnum;
    AREA_DATA  *pArea;
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pMob = NULL;
    OBJ_INDEX_DATA *pObj;
    ROOM_INDEX_DATA *pRoom;

    if ( newobjs == top_obj_index ) return; /* all objects in new format */

    for ( pArea = area_first; pArea; pArea = pArea->next )
    {
        for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
	{
	    if ( !( pRoom = get_room_index( vnum ) ) ) continue;

	    for ( pReset = pRoom->reset_first; pReset; pReset = pReset->next )
	    {
		switch ( pReset->command )
		{
		case 'M':
		    if ( !( pMob = get_mob_index( pReset->arg1 ) ) )
			bug( "Convert_objects: 'M': bad vnum %d.", pReset->arg1 );
		    break;

		case 'O':
		    if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
		    {
			bug( "Convert_objects: 'O': bad vnum %d.", pReset->arg1 );
			break;
		    }

		    if ( pObj->new_format )
			continue;

		    if ( !pMob )
		    {
			bug( "Convert_objects: 'O': No mob reset yet.", 0 );
			break;
		    }

		    pObj->level = pObj->level < 1 ? pMob->level - 2
			: UMIN(pObj->level, pMob->level - 2);
		    break;

		case 'P':
		    {
			OBJ_INDEX_DATA *pObj, *pObjTo;

			if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
			{
			    bug( "Convert_objects: 'P': bad vnum %d.", pReset->arg1 );
			    break;
			}

			if ( pObj->new_format )
			    continue;

		if ( !( pObjTo = get_obj_index( pReset->arg3 ) ) )
			{
			    bug( "Convert_objects: 'P': bad vnum %d.", pReset->arg3 );
			    break;
			}

			pObj->level = pObj->level < 1 ? pObjTo->level
			    : UMIN(pObj->level, pObjTo->level);
		    }
		    break;

		case 'G':
		case 'E':
		    if ( !( pObj = get_obj_index( pReset->arg1 ) ) )
		    {
			bug( "Convert_objects: 'E' or 'G': bad vnum %d.", pReset->arg1 );
			break;
		    }

		    if ( !pMob )
		    {
			bug( "Convert_objects: 'E' or 'G': null mob for vnum %d.",
			     pReset->arg1 );
			break;
		    }

		    if ( pObj->new_format )
			continue;

		    if ( pMob->pShop )
		    {
			switch ( pObj->item_type )
			{
			default:
			    pObj->level = UMAX(0, pObj->level);
			    break;
		case ITEM_PILL:
			case ITEM_POTION:
			    pObj->level = UMAX(5, pObj->level);
			    break;
			case ITEM_SCROLL:
			case ITEM_ARMOR:
			case ITEM_WEAPON:
			    pObj->level = UMAX(10, pObj->level);
			    break;
			case ITEM_WAND:
			case ITEM_TREASURE:
			    pObj->level = UMAX(15, pObj->level);
			    break;
			case ITEM_STAFF:
			    pObj->level = UMAX(20, pObj->level);
			    break;
			}
		    }
		    else
		pObj->level = pObj->level < 1 ? pMob->level
			    : UMIN( pObj->level, pMob->level );
		    break;
		} /* switch ( pReset->command ) */
	    }
	}
    }

    /* do the conversion: */

    for ( pArea = area_first; pArea ; pArea = pArea->next )
	for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
	    if ( (pObj = get_obj_index( vnum )) )
 		if ( !pObj->new_format )
		    convert_object( pObj );

    return;
}



/*****************************************************************************
 Name:		convert_object
 Purpose:	Converts an old_format obj to new_format
 Called by:	convert_objects (db2.c).
 Note:          Dug out of create_obj (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_object( OBJ_INDEX_DATA *pObjIndex )
{
    int level;
    int number, type;  /* for dice-conversion */

    if ( !pObjIndex || pObjIndex->new_format ) return;

    level = pObjIndex->level;

    pObjIndex->level    = UMAX( 0, pObjIndex->level ); /* just to be sure */
    pObjIndex->cost     = 10*level;

    switch ( pObjIndex->item_type )
    {
        default:
            bug( "Obj_convert: vnum %d bad type.", pObjIndex->item_type );
            break;

        case ITEM_LIGHT:
        case ITEM_TREASURE:
        case ITEM_FURNITURE:
        case ITEM_TRASH:
        case ITEM_CONTAINER:
        case ITEM_DRINK_CON:
        case ITEM_KEY:
        case ITEM_FOOD:
        case ITEM_BOAT:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
        case ITEM_FOUNTAIN:
        case ITEM_MAP:
        case ITEM_CLOTHING:
        case ITEM_SCROLL:
        case ITEM_ARROWS:
	    break;

        case ITEM_WAND:
        case ITEM_STAFF:
            pObjIndex->value[2] = pObjIndex->value[1];
	    break;

        case ITEM_WEAPON:

	    /*
	     * The conversion below is based on the values generated
	     * in one_hit() (fight.c).  Since I don't want a lvl 50 
	     * weapon to do 15d3 damage, the min value will be below
	     * the one in one_hit, and to make up for it, I've made 
	     * the max value higher.
	     * (I don't want 15d2 because this will hardly ever roll
	     * 15 or 30, it will only roll damage close to 23.
	     * I can't do 4d8+11, because one_hit there is no dice-
	     * bounus value to set...)
	     *
	     * The conversion below gives:

	     level:   dice      min      max      mean
	       1:     1d8      1( 2)    8( 7)     5( 5)
	       2:     2d5      2( 3)   10( 8)     6( 6)
	       3:     2d5      2( 3)   10( 8)     6( 6)
	       5:     2d6      2( 3)   12(10)     7( 7)
	      10:     4d5      4( 5)   20(14)    12(10)
	      20:     5d5      5( 7)   25(21)    15(14)
	      30:     5d7      5(10)   35(29)    20(20)
	      50:     5d11     5(15)   55(44)    30(30)

	     */

	    number = UMIN(level/4 + 1, 5);
	    type   = (level + 7)/number;

            pObjIndex->value[1] = number;
            pObjIndex->value[2] = type;
    break;

        case ITEM_ARMOR:
            pObjIndex->value[0] = level / 5 + 3;
            pObjIndex->value[1] = pObjIndex->value[0];
            pObjIndex->value[2] = pObjIndex->value[0];
	    break;

        case ITEM_POTION:
        case ITEM_PILL:
            break;

        case ITEM_MONEY:
	    pObjIndex->value[0] = pObjIndex->cost;
	    break;
    }

    pObjIndex->new_format = TRUE;
    ++newobjs;

    return;
}




/*****************************************************************************
 Name:		convert_mobile
 Purpose:	Converts an old_format mob into new_format
 Called by:	load_old_mob (db.c).
 Note:          Dug out of create_mobile (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_mobile( MOB_INDEX_DATA_OLD *pMobIndex )
{
    int i;
    int type, number, bonus;
    int level;

    if ( !pMobIndex || pMobIndex->new_format ) return;

    level = pMobIndex->level;

    SET_BIT( pMobIndex->act, ACT_WARRIOR );

    /*
     * Calculate hit dice.  Gives close to the hitpoints
     * of old format mobs created with create_mobile()  (db.c)
     * A high number of dice makes for less variance in mobiles
     * hitpoints.
     * (might be a good idea to reduce the max number of dice)
     *
     * The conversion below gives:

       level:     dice         min         max        diff       mean
         1:       1d2+6       7(  7)     8(   8)     1(   1)     8(   8)
	 2:       1d3+15     16( 15)    18(  18)     2(   3)    17(  17)
	 3:       1d6+24     25( 24)    30(  30)     5(   6)    27(  27)
	 5:      1d17+42     43( 42)    59(  59)    16(  17)    51(  51)
	10:      3d22+96     99( 95)   162( 162)    63(  67)   131(    )
	15:     5d30+161    166(159)   311( 311)   145( 150)   239(    )
	30:    10d61+416    426(419)  1026(1026)   600( 607)   726(    )
	50:    10d169+920   930(923)  2610(2610)  1680(1688)  1770(    )

	The values in parenthesis give the values generated in create_mobile.
        Diff = max - min.  Mean is the arithmetic mean.
	(hmm.. must be some roundoff error in my calculations.. smurfette got
	 1d6+23 hp at level 3 ? -- anyway.. the values above should be
	 approximately right..)
     */
    type   = level*level*27/40;
    number = UMIN(type/40 + 1, 10); /* how do they get 11 ??? */
    type   = UMAX(2, type/number);
    bonus  = (int)UMAX(0, level*(8 + level)*.9 - number*type);

    pMobIndex->hit[DICE_NUMBER]    = number;
    pMobIndex->hit[DICE_TYPE]      = type;
    pMobIndex->hit[DICE_BONUS]     = bonus;

    pMobIndex->mana[DICE_NUMBER]   = level;
    pMobIndex->mana[DICE_TYPE]     = 10;
    pMobIndex->mana[DICE_BONUS]    = 100;

    /*
     * Calculate dam dice.  Gives close to the damage
     * of old format mobs in damage()  (fight.c)
     */
    type   = level*7/4;
    number = UMIN(type/8 + 1, 5);
    type   = UMAX(2, type/number);
    bonus  = UMAX(0, level*9/4 - number*type);

    pMobIndex->damage[DICE_NUMBER] = number;
    pMobIndex->damage[DICE_TYPE]   = type;
    pMobIndex->damage[DICE_BONUS]  = bonus;

    switch ( number_range( 1, 3 ) )
    {
        case (1): pMobIndex->dam_type =  3;       break;  /* slash  */
        case (2): pMobIndex->dam_type =  7;       break;  /* pound  */
        case (3): pMobIndex->dam_type = 11;       break;  /* pierce */
    }

    for (i = 0; i < 3; i++)
        pMobIndex->ac[i]         = interpolate( level, 100, -100);
    pMobIndex->ac[3]             = interpolate( level, 100, 0);    /* exotic */

    pMobIndex->wealth           /= 100;
    pMobIndex->size              = SIZE_MEDIUM;
    pMobIndex->material          = str_dup("none");

    pMobIndex->new_format        = TRUE;
    ++newmobs;

    return;
}


/* 
 * new_dump written by Rahl (Daniel Anderson) of Broken Shadows
 */
void do_new_dump( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA *pMobIndex;
    OBJ_INDEX_DATA *pObjIndex;
    ROOM_INDEX_DATA *pRoomIndex;
    FILE *fp;
    int vnum,nMatch = 0;
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    int door; 
    AFFECT_DATA *paf;
    CHAR_DATA *mob;
    
    /* open file */
    fclose(fpReserve);
    
    /* start printing out mobile data */
    fp = fopen("../mob.txt","w");
    
    fprintf(fp,"\nMobile Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_index; vnum++)
        if ((pMobIndex = get_mob_index(vnum)) != NULL)
        {
            nMatch++;
            mob = create_mobile( pMobIndex );
            sprintf( buf, "Name: %s.\n",
                mob->name );
            fprintf( fp, buf );

            sprintf( buf, "Area: %s\n", mob->pIndexData->area->name );
            fprintf( fp, buf );
            
            sprintf( buf, "Vnum: %d  Race: %s  Sex: %s  Room: %d  Count %d\n", 
                IS_NPC(mob) ? mob->pIndexData->vnum : 0,
                race_table[mob->race].name,
                mob->sex == SEX_MALE    ? "male"   :
            mob->sex == SEX_FEMALE  ? "female" : "neutral",
                mob->in_room == NULL    ?        0 : mob->in_room->vnum, 
                mob->pIndexData->count );
            fprintf( fp, buf );

            sprintf( buf, 
                "Str: %d(%d)  Con: %d(%d)  Vit: %d(%d)  Agi: %d(%d)  Dex: %d(%d)\n",
                mob->perm_stat[STAT_STR],
                get_curr_stat(mob,STAT_STR),
                mob->perm_stat[STAT_CON],
                get_curr_stat(mob,STAT_CON),
                mob->perm_stat[STAT_VIT],
                get_curr_stat(mob,STAT_VIT),
                mob->perm_stat[STAT_AGI],
                get_curr_stat(mob,STAT_AGI),
                mob->perm_stat[STAT_DEX],
                get_curr_stat(mob,STAT_DEX));
            fprintf( fp, buf );

            sprintf( buf, 
                "Int: %d(%d)  Wis: %d(%d)  Dis: %d(%d)  Cha: %d(%d)  Luc: %d(%d)\n",
                mob->perm_stat[STAT_INT],
                get_curr_stat(mob,STAT_INT),
                mob->perm_stat[STAT_WIS],
                get_curr_stat(mob,STAT_WIS),
                mob->perm_stat[STAT_DIS],
                get_curr_stat(mob,STAT_DIS),
                mob->perm_stat[STAT_CHA],
                get_curr_stat(mob,STAT_CHA),
                mob->perm_stat[STAT_LUC],
                get_curr_stat(mob,STAT_LUC) );
            fprintf( fp, buf );
            

            sprintf( buf, "Hp: %d  Mana: %d  Move: %d  Hit: %d  Dam: %d\n",
                mob->max_hit,
                mob->max_mana,
                mob->max_move,
                GET_HITROLL(mob), GET_DAMROLL(mob) );
            fprintf( fp, buf );
            
            sprintf( buf,
                "Lv: %d  Align: %d  Gold: %ld  Damage: %dd%d  Message: %s\n",
                mob->level,                   
                mob->alignment,
                mob->gold,
                mob->damage[DICE_NUMBER],mob->damage[DICE_TYPE],
                attack_table[mob->dam_type].name);
            fprintf( fp, buf );
            
            sprintf(buf,"Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n",
                GET_AC(mob,AC_PIERCE), GET_AC(mob,AC_BASH),
                GET_AC(mob,AC_SLASH),  GET_AC(mob,AC_EXOTIC));
            fprintf( fp, buf );
            
            sprintf(buf, "Act: %s\n",act_bits_name(mob->act));
            fprintf( fp, buf );
            
            if (IS_NPC(mob) && mob->off_flags)
            {
                sprintf(buf, "Offense: %s\n",off_bits_name(mob->off_flags));
                fprintf( fp, buf );
            }
            
            if (mob->imm_flags)
            {
                sprintf(buf, "Immune: %s\n",imm_bits_name(mob->imm_flags));
                fprintf( fp, buf );
            }
            
            if (mob->res_flags)
            {
                sprintf(buf, "Resist: %s\n", imm_bits_name(mob->res_flags));
                fprintf( fp, buf );
            }
            
            if (mob->vuln_flags)
            {
                sprintf(buf, "Vulnerable: %s\n", imm_bits_name(mob->vuln_flags));
                fprintf( fp, buf );
            }
            
            sprintf(buf, "Form: %s\nParts: %s\n", 
                form_bits_name(mob->form), part_bits_name(mob->parts));
            fprintf( fp, buf );
            
            if (mob->affect_field)
            {
                sprintf(buf, "Affected by %s\n", 
                    affect_bits_name(mob->affect_field));
                fprintf( fp, buf );
            }
            
            sprintf( buf, "Short description: %s\nLong description: %s",
                mob->short_descr,
                mob->long_descr[0] != '\0' ? mob->long_descr : "(none)\n" );
            fprintf( fp, buf );
            
            if ( IS_NPC(mob) && mob->spec_fun != 0 )
            {
                sprintf( buf, "Special procedure: %s\n", spec_name( mob->spec_fun ) );
                fprintf( fp, buf );
            }
            
            for ( paf = mob->affected; paf != NULL; paf = paf->next )
            {
                sprintf( buf,
                    "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n",
                    skill_table[(int) paf->type].name,
                    affect_loc_name( paf->location ),
                    paf->modifier,
                    paf->duration,
                    affect_bit_name( paf->bitvector ),
                    paf->level
                    );
                fprintf( fp, buf );
            }
            fprintf( fp, "\n" );
            extract_char( mob, FALSE );
    }
    fclose(fp);
    
    /* start printing out object data */
    fp = fopen("../obj.txt","w");
    
    fprintf(fp,"\nObject Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((pObjIndex = get_obj_index(vnum)) != NULL)
        {
            nMatch++;
            
            obj = create_object( pObjIndex, 0 );
            
            sprintf( buf, "Name(s): %s\n",
                obj->name );
            fprintf( fp, buf );

            sprintf( buf, "Area: %s\n", obj->pIndexData->area->name );
            fprintf( fp, buf );
            
            sprintf( buf, "Vnum: %d  Format: %s  Type: %s  Number: %d/%d  Weight: %d/%d\n",
                obj->pIndexData->vnum, obj->pIndexData->new_format ? "new" : "old",
                flag_stat_string(type_flags, obj->item_type), 1, get_obj_number( obj ),
                obj->weight, get_obj_weight( obj ) );
            fprintf( fp, buf );

            fprintf( fp, "Short description: %s\nLong description: %s\n",
                obj->short_descr, obj->description );
            /*sprintf( buf, "Short description: %s\nLong description: %s\n",
                obj->short_descr, obj->description );*/
            /*fprintf( fp, buf );*/
            
            sprintf( buf, "Wear bits: %s\tExtra bits: %s\n",
                wear_bits_name(obj->wear_flags), extra_bits_name( obj->extra_flags ) );
            fprintf( fp, buf );
            
            sprintf( buf, "Level: %d  Cost: %d  Condition: %d  Timer: %d\n",
                obj->level, obj->cost, obj->condition, obj->timer );
            fprintf( fp, buf );
            
            sprintf( buf,
                "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n",
                obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
                obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
                obj->carried_by == NULL    ? "(none)" : obj->carried_by->name,
                obj->wear_loc );
            fprintf( fp, buf );
            
            sprintf( buf, "Values: %d %d %d %d %d\n",
                obj->value[0], obj->value[1], obj->value[2], obj->value[3],
                obj->value[4] );
            fprintf( fp, buf );
            
            /* now give out vital statistics as per identify */
            
            switch ( obj->item_type )
            {
            case ITEM_SCROLL: 
            case ITEM_POTION:
            case ITEM_PILL:
                sprintf( buf, "Level %d spells of:", obj->value[0] );
                fprintf( fp, buf );
                
                if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
                {
                    fprintf( fp, " '" );
                    fprintf( fp, skill_table[obj->value[1]].name );
                    fprintf( fp, "'" );
                }
                
                if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
                {
                    fprintf( fp, " '" );
                    fprintf( fp, skill_table[obj->value[2]].name );
                    fprintf( fp, "'" );
                }
                
                if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
                {
                    fprintf( fp, " '" );
                    fprintf( fp, skill_table[obj->value[3]].name );
                    fprintf( fp, "'" );
                }
                
                fprintf( fp, ".\n" );
                break;
                
            case ITEM_WAND: 
            case ITEM_STAFF: 
                sprintf( buf, "Has %d(%d) charges of level %d",
                    obj->value[1], obj->value[2], obj->value[0] );
                fprintf( fp, buf );
                
                if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
                {
                    fprintf( fp, " '" );
                    fprintf( fp, skill_table[obj->value[3]].name );
                    fprintf( fp, "'" );
                }
                
                fprintf( fp, ".\n" );
                break;
                
            case ITEM_WEAPON:
                fprintf( fp, "Weapon type is " );
                switch (obj->value[0])
                {
                case(WEAPON_EXOTIC)     : fprintf(fp, "exotic\n");  break;
                case(WEAPON_SWORD)      : fprintf(fp, "sword\n");   break;  
                case(WEAPON_GUN)        : fprintf(fp, "gun\n");   break;  
                case(WEAPON_BOW)        : fprintf(fp, "bow\n");   break;  
                case(WEAPON_DAGGER)     : fprintf(fp, "dagger\n");  break;
                case(WEAPON_SPEAR)  : fprintf(fp, "spear/staff\n"); break;
                case(WEAPON_MACE)   : fprintf(fp, "mace/club\n");   break;
                case(WEAPON_AXE)    : fprintf(fp, "axe\n");     break;
                case(WEAPON_FLAIL)  : fprintf(fp, "flail\n");   break;
                case(WEAPON_WHIP)   : fprintf(fp, "whip\n");        break;
                case(WEAPON_POLEARM)    : fprintf(fp, "polearm\n"); break;
                default         : fprintf(fp, "unknown\n"); break;
                }
                if (obj->pIndexData->new_format)
                    sprintf(buf,"Damage is %dd%d (average %d)\n",
                    obj->value[1],obj->value[2],
                    (1 + obj->value[2]) * obj->value[1] / 2);
                else
                    sprintf( buf, "Damage is %d to %d (average %d)\n",
                    obj->value[1], obj->value[2],
                    ( obj->value[1] + obj->value[2] ) / 2 );
                fprintf( fp, buf );
                
                if (obj->value[4])  /* weapon flags */
                {
                    sprintf(buf,"Weapons flags: %s\n",weapon_bits_name(obj->value[4]));
                    fprintf(fp, buf);
                }
                break;
                
                case ITEM_ARMOR:
                    sprintf( buf, 
                        "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n", 
                        obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
                    fprintf( fp, buf );
                    break;
            }  /* switch */
            
            for ( paf = obj->affected; paf != NULL; paf = paf->next )
            {
                sprintf( buf, "Affects %s by %d, level %d",
                    affect_loc_name( paf->location ), paf->modifier,paf->level );
                fprintf( fp, buf );
                /* added by Rahl */
                if ( paf->duration > -1 )
                    sprintf( buf, ", %d hours.\n", paf->duration );
                else
                    sprintf( buf, ".\n" );
                fprintf( fp, buf );
                if ( paf->bitvector )
                {
                    switch ( paf->where )
                    {
                    case TO_AFFECTS:
                        sprintf( buf, "Adds %s affect.\n", 
                            affect_bit_name( paf->bitvector ) );
                        break;
                    case TO_WEAPON:
                        sprintf( buf, "Adds %s weapon flags.\n",
                            weapon_bits_name( paf->bitvector ) );
                        break;
                    case TO_OBJECT:
                        sprintf( buf, "Adds %s object flag.\n",
                            extra_bit_name( paf->bitvector ) );
                        break;
                    case TO_IMMUNE:
                        sprintf( buf, "Adds immunity to %s.\n",
                            imm_bit_name( paf->bitvector ) );
                        break;
                    case TO_RESIST:
                        sprintf( buf, "Adds resistance to %s.\n",
                            imm_bit_name( paf->bitvector ) );
                        break;
                    case TO_VULN:
                        sprintf( buf, "Adds vulnerability to %s.\n",
                            imm_bit_name( paf->bitvector ) );
                        break;
                    default:
                        sprintf( buf, "Unknown bit %d %d\n",
                            paf->where, paf->bitvector );
                        break;
                    }
                    fprintf( fp, buf );
                }  /* if */
            }  /* for */
            
                for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
                {
                    sprintf( buf, "Affects %s by %d, level %d.\n",
                        affect_loc_name( paf->location ), paf->modifier,paf->level );
                    fprintf( fp, buf );
                    if ( paf->bitvector )
                    {
                        switch ( paf->where )
                        {
                        case TO_AFFECTS:
                            sprintf( buf, "Adds %s affect.\n", 
                                affect_bit_name( paf->bitvector ) );
                            break;
                        case TO_WEAPON:
                            sprintf( buf, "Adds %s weapon flags.\n",
                                weapon_bits_name( paf->bitvector ) );
                            break;
                        case TO_OBJECT:
                            sprintf( buf, "Adds %s object flag.\n",
                                extra_bit_name( paf->bitvector ) );
                            break;
                        case TO_IMMUNE:
                            sprintf( buf, "Adds immunity to %s.\n",
                                imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_RESIST:
                            sprintf( buf, "Adds resistance to %s.\n",
                                imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_VULN:
                            sprintf( buf, "Adds vulnerability to %s.\n",
                                imm_bit_name( paf->bitvector ) );
                            break;
                        default:
                            sprintf( buf, "Unknown bit %d %d\n",
                                paf->where, paf->bitvector );
                            break;
                        }      /* switch */
                        fprintf( fp, buf );
                    }       /* if */
                }   /* for */
                fprintf( fp, "\n" );
                extract_obj( obj );
                
    }       /* if */
    /* close file */
    fclose(fp);
    
    
    /* start printing out room data */

    fp = fopen("../room.txt","w");
    
    fprintf(fp,"\nRoom Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;

    for (vnum = 0; vnum <= top_vnum_room; vnum++)
        if ((pRoomIndex = get_room_index(vnum)) != NULL)
        {
            nMatch++;
            sprintf( buf, "\nName: '%s.'\nArea: '%s'.\n",
                pRoomIndex->name,
                pRoomIndex->area->name );
            fprintf( fp, buf );

            sprintf( buf,
                "Vnum: %d.  Sector: %d.  Light: %d.\n",
                pRoomIndex->vnum,
                pRoomIndex->sector_type,
                pRoomIndex->light );
            fprintf( fp, buf );
            
            sprintf( buf, "Room flags: %d.\n", 
                pRoomIndex->room_flags);
            fprintf( fp, buf );

            fprintf( fp, "Description:\n%s\n",
                pRoomIndex->description );
            
            if ( pRoomIndex->extra_descr != NULL )
            {
                EXTRA_DESCR_DATA *ed;
                
                fprintf( fp, "Extra description keywords: '" );
                for ( ed = pRoomIndex->extra_descr; ed; ed = ed->next )
                {
                    fprintf( fp, ed->keyword );
                    if ( ed->next != NULL )
                        fprintf( fp, " " );
                }
                fprintf( fp, "'.\n" );
            }
            
            fprintf( fp, "Characters:" );
            for ( rch = pRoomIndex->people; rch; rch = rch->next_in_room )
            {
                fprintf( fp, " " );
                one_argument( rch->name, buf );
                fprintf( fp, buf );
            }
             
            fprintf( fp, ".\nObjects:   " );
            for ( obj = pRoomIndex->contents; obj; obj = obj->next_content )
            {
                fprintf( fp, " " );
                one_argument( obj->name, buf );
                fprintf( fp, buf );
            }
            fprintf( fp, ".\n" );
            
            for ( door = 0; door <= 5; door++ )
            {
                EXIT_DATA *pexit;
                
                if ( ( pexit = pRoomIndex->exit[door] ) != NULL )
                {
                    sprintf( buf,
                        "Door: %d.  To: %d.  Key: %d.  Exit flags: %s.\nKeyword: '%s'.  Description: %s",
                        door,
                        (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
                        pexit->key,
                        print_tflag( pexit->exit_info ),
                        pexit->keyword,
                        pexit->description[0] != '\0' ? pexit->description : "(none).\n" );
                    fprintf( fp, buf );
                }
            }
            
        }

    /* close file */
    fclose(fp);
    
    fpReserve = fopen( NULL_FILE, "r" );
    
    send_to_char( "Done writing files...\n\r", ch );
}
