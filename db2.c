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
#include <sys/types.h>
#include <sys/time.h>

#include "merc.h"
#include "db.h"
#include "tables.h"
#include "lookup.h"
#include "mob_stats.h"


#define VER_ONE_AC_VAL    4
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

        letter                          = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_mobiles: # not found.", 0 );
            exit( 1 );
        }

        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;

        if ( get_mob_index( vnum ) != NULL )
        {
            bug( "Load_mobiles: vnum %d duplicated.", vnum );
            exit( 1 );
        }

        pMobIndex                       = alloc_mem( sizeof(*pMobIndex) );
        pMobIndex->vnum                 = vnum;
        pMobIndex->area                 = area_last;               /* OLC */
        pMobIndex->player_name          = fread_string( fp );
        pMobIndex->short_descr          = fread_string( fp );
        pMobIndex->long_descr           = upper_realloc(fread_string(fp));
        extern int area_version;
        if ( area_version < VER_NEW_MOB_LDESC )
        {
            /* old format always had \n\r appended to the
               actual long_descr, let's kill that */
            pMobIndex->long_descr = trim_realloc(pMobIndex->long_descr);
        }
        pMobIndex->description          = upper_realloc(fread_string(fp));
        pMobIndex->race		 	= race_lookup(fread_string( fp ));

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
        pMobIndex->ac           = fread_number( fp ) * 10;
        if (area_version < VER_ONE_AC_VAL)
        {
            /* read the first value, burn the other 3 */
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
        }

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
                const char *word;
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
                PROG_LIST *pMprog;
                const char *word;
                int trigger = 0;

                pMprog              = alloc_MTRIG();
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
                const char *word = fread_word(fp);
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
    long actual, spec, base;

    pMobIndex = alloc_MOBPROTO();

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

    pMobIndex->hitpoint_percent = 100;
    pMobIndex->mana_percent     = 100;
    pMobIndex->move_percent     = 100;
    pMobIndex->hitroll_percent  = 100;
    pMobIndex->damage_percent   = 100;
    pMobIndex->ac_percent       = 100;
    pMobIndex->saves_percent    = 100;
    pMobIndex->wealth_percent   = 0;

    if (pMobIndex->level > 0)
    {
        // boss mobs should keep their main stats (hp/damage/nr_attacks) unchanged during migration, to adjust later one by one
        // approximation: boss mob = lvl 120+ with sanc or level 80+ in remort
        if ((pMobIndex->level >= 120 && IS_SET(pMobIndex->affect_field, AFF_SANCTUARY))
                || (pMobIndex->level >= 80 && IS_SET(pMobIndex->area->area_flags, AREA_REMORT))
           )
        {
            // hitpoints
            actual = average_roll(pMobIndexOld->hit[DICE_NUMBER], pMobIndexOld->hit[DICE_TYPE], pMobIndexOld->hit[DICE_BONUS]);
            base = level_base_hp(pMobIndex->level);
            pMobIndex->hitpoint_percent = 100 * actual / base;

            // damage
            actual = average_roll(pMobIndexOld->damage[DICE_NUMBER], pMobIndexOld->damage[DICE_TYPE], 0) + pMobIndexOld->damage[DICE_BONUS] / 4;
            base = level_base_damage(pMobIndex->level) + level_base_damroll(pMobIndex->level) / 4;
            pMobIndex->damage_percent = 100 * actual / base;

            // number of attacks
            SET_BIT(pMobIndex->act, ACT_STAGGERED);
        }

        // wealth - as we don't have shops loaded yet, this will result in excessive wealth percent for shopkeepers
        // however, we cap at 200%, so no big deal
        actual = pMobIndexOld->wealth;
        spec = level_base_wealth(pMobIndex->level);
        pMobIndex->wealth_percent   = URANGE(0, 100 * actual / UMAX(1,spec), 200);
    }

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

        letter                          = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_mobbles: # not found.", 0 );
            exit( 1 );
        }

        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;

        if ( get_mob_index( vnum ) != NULL )
        {
            bug( "Load_mobbles: vnum %d duplicated.", vnum );
            exit( 1 );
        }

        pMobIndex                       = alloc_MOBPROTO();
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
        const char *key;
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
                pMobIndex->long_descr = upper_realloc(fread_string(fp));

                extern int area_version;
                if ( area_version < VER_NEW_MOB_LDESC )
                {
                    /* old format always had \n\r appended to the
                       actual long_descr, let's kill that */
                    pMobIndex->long_descr = trim_realloc(pMobIndex->long_descr);
                }
            }
            else if KEY("DESC")
            {
                pMobIndex->description = upper_realloc(fread_string(fp));
            }
            else if KEY("NOTES")
            {
                pMobIndex->comments = fread_string( fp );
            }
            else if KEY("COMMENTS")
            {
                pMobIndex->comments = fread_string( fp );
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
                PROG_LIST *pMprog;
                const char *word;
                int trigger = 0;

                pMprog              = alloc_MTRIG();
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

        if ( !pMobIndex->comments )
            pMobIndex->comments=str_dup("");

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
    extern int area_version;
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
        AFFECT_DATA *paf = NULL; // last affect read

        letter                          = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_objects: # not found.", 0 );
            exit( 1 );
        }

        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;

        if ( get_obj_index( vnum ) != NULL )
        {
            bug( "Load_objects: vnum %d duplicated.", vnum );
            exit( 1 );
        }

        pObjIndex                       = alloc_OBJPROTO();
        pObjIndex->vnum                 = vnum;
        pObjIndex->area                 = area_last;            /* OLC */
        pObjIndex->reset_num		= 0;
        pObjIndex->combine_vnum         = 0;
        pObjIndex->diff_rating          = 0;

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
        /*pObjIndex->durability		= fread_number( fp );*/
        if ( area_version < VER_UPDATE_OBJ_FMT )
        {
            fread_number( fp ); /* read durability */

        /* condition */
            fread_letter( fp ); /* read condition */
        /*
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
        }*/

        }

        for ( ; ; )
        {
            char letter;

            letter = fread_letter( fp );

            if ( letter == 'A' )
            {
                paf                     = new_affect();
                paf->where              = TO_OBJECT;
                paf->type               = -1;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = fread_number( fp );
                paf->modifier           = fread_number( fp );
                paf->bitvector          = 0;
                pObjIndex->affected     = affect_insert( pObjIndex->affected, paf );
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
                if ( paf == NULL )
                {
                    bug( "Load_Objects: No affect for detect_level (#%d)", vnum );
                    fread_number( fp );
                }
                paf->detect_level = fread_number( fp );
            }

            else if (letter == 'R')
            {
                pObjIndex->rank = clan_rank_lookup(pObjIndex->clan, fread_string( fp ));	
            }

            else if (letter == 'F')
            {
                paf             = new_affect();
                letter          = fread_letter(fp);
                switch (letter)
                {
                    case 'A':
                        paf->where = TO_AFFECTS;
                        break;
                    case 'I':
                        paf->where = TO_IMMUNE;
                        break;
                    case 'R':
                        paf->where = TO_RESIST;
                        break;
                    case 'V':
                        paf->where = TO_VULN;
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

                pObjIndex->affected     = affect_insert( pObjIndex->affected, paf );
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
        /* load obj progs if any */
        while (TRUE)
        {
            letter = fread_letter( fp );
            if ( letter == 'O' ) /* we have oprogs */
            {
                PROG_LIST *pOprog;
                const char *word;
                int trigger = 0;

                pOprog              = alloc_OTRIG();
                word                = fread_word( fp );
                if ( (trigger = flag_lookup( word, oprog_flags )) == NO_FLAG )
                {
                    bugf("load_obj.O: invalid trigger '%s' for object %d.", word, vnum);
                    exit(1);
                }
                SET_BIT( pObjIndex->oprog_flags, trigger );
                pOprog->trig_type   = trigger;
                pOprog->vnum        = fread_number( fp );
                pOprog->trig_phrase = fread_string( fp );
                pOprog->next        = pObjIndex->oprogs;
                pObjIndex->oprogs   = pOprog;
            }
            else if ( letter == 'N' )
            {
                pObjIndex->comments = fread_string( fp );
            }
            else
            {
                ungetc( letter, fp );
                break;
            }
        }
        
        if ( !pObjIndex->comments )
            pObjIndex->comments = str_dup( "" );

        iHash                   = vnum % MAX_KEY_HASH;
        pObjIndex->next         = obj_index_hash[iHash];
        obj_index_hash[iHash]   = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   /* OLC */
        assign_area_vnum( vnum );                                   /* OLC */
    }

    return;
}

/* 
 * new_dump written by Rahl (Daniel Anderson) of Broken Shadows
 */
DEF_DO_FUN(do_new_dump)
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
            fprintf( fp, "Name: %s.\n", mob->name );

            fprintf( fp, "Area: %s\n", mob->pIndexData->area->name );

            fprintf( fp, "Vnum: %d  Race: %s  Sex: %s  Room: %d  Count %d\n", 
                    IS_NPC(mob) ? mob->pIndexData->vnum : 0,
                    race_table[mob->race].name,
                    mob->sex == SEX_MALE    ? "male"   :
                    mob->sex == SEX_FEMALE  ? "female" : "neutral",
                    mob->in_room == NULL    ?        0 : mob->in_room->vnum, 
                    mob->pIndexData->count );

            fprintf( fp, 
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

            fprintf( fp,
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

            fprintf( fp, "Hp: %d  Mana: %d  Move: %d  Hit: %d  Dam: %d\n",
                    mob->max_hit,
                    mob->max_mana,
                    mob->max_move,
                    GET_HITROLL(mob), GET_DAMROLL(mob) );

            fprintf( fp,
                    "Lv: %d  Align: %d  Gold: %ld  Damage: %dd%d  Message: %s\n",
                    mob->level,                   
                    mob->alignment,
                    mob->gold,
                    mob->damage[DICE_NUMBER],mob->damage[DICE_TYPE],
                    attack_table[mob->dam_type].name);

            fprintf(fp,"Armor: %d\n", GET_AC(mob));

            fprintf(fp, "Act: %s\n", act_bits_name(mob->act));

            if (IS_NPC(mob) && mob->off_flags)
            {
                fprintf(fp, "Offense: %s\n",off_bits_name(mob->off_flags));
            }

            if (mob->imm_flags)
            {
                fprintf(fp, "Immune: %s\n",imm_bits_name(mob->imm_flags));
            }

            if (mob->res_flags)
            {
                fprintf(fp, "Resist: %s\n", imm_bits_name(mob->res_flags));
            }

            if (mob->vuln_flags)
            {
                fprintf(fp, "Vulnerable: %s\n", imm_bits_name(mob->vuln_flags));
            }

            fprintf(fp, "Form: %s\nParts: %s\n", 
                    form_bits_name(mob->form), part_bits_name(mob->parts));

            if (mob->affect_field)
            {
                fprintf(fp, "Affected by %s\n", affect_bits_name(mob->affect_field));
            }

            fprintf( fp, "Short description: %s\nLong description: %s",
                    mob->short_descr,
                    mob->long_descr[0] != '\0' ? mob->long_descr : "(none)\n" );

            if ( IS_NPC(mob) && mob->spec_fun != 0 )
            {
                fprintf( fp, "Special procedure: %s\n", spec_name_lookup( mob->spec_fun ) );
            }

            for ( paf = mob->affected; paf != NULL; paf = paf->next )
            {
                fprintf( fp,
                        "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n",
                        skill_table[(int) paf->type].name,
                        affect_loc_name( paf->location ),
                        paf->modifier,
                        paf->duration,
                        affect_bit_name( paf->bitvector ),
                        paf->level
                       );
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

            fprintf( fp, "Name(s): %s\n", obj->name );

            fprintf( fp, "Area: %s\n", obj->pIndexData->area->name );

            fprintf( fp, "Vnum: %d  Type: %s  Number: %d/%d  Weight: %d/%d\n",
                    obj->pIndexData->vnum,
                    flag_bit_name(type_flags, obj->item_type), 1, get_obj_number( obj ),
                    obj->weight, get_obj_weight( obj ) );

            fprintf( fp, "Short description: %s\nLong description: %s\n",
                    obj->short_descr, obj->description );

            fprintf( fp, "Wear bits: %s\t\tExtra bits: %s\n",
                    wear_bits_name(obj->wear_flags), extra_bits_name( obj->extra_flags ) );

            fprintf( fp, "Level: %d  Cost: %d  Timer: %d\n",
                    obj->level, obj->cost, obj->timer );

            fprintf( fp,
                    "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n",
                    obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
                    obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
                    obj->carried_by == NULL    ? "(none)" : obj->carried_by->name,
                    obj->wear_loc );

            fprintf( fp, "Values: %d %d %d %d %d\n",
                    obj->value[0], obj->value[1], obj->value[2], obj->value[3],
                    obj->value[4] );

            /* now give out vital statistics as per identify */

            switch ( obj->item_type )
            {
                case ITEM_SCROLL: 
                case ITEM_POTION:
                case ITEM_PILL:
                    fprintf( fp, "Level %d spells of:", obj->value[0] );

                    if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
                    {
                        fprintf( fp, " '%s'", skill_table[obj->value[1]].name );
                    }

                    if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
                    {
                        fprintf( fp, " '%s'", skill_table[obj->value[2]].name );
                    }

                    if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
                    {
                        fprintf( fp, " '%s'", skill_table[obj->value[3]].name );
                    }

                    fprintf( fp, ".\n" );
                    break;

                case ITEM_WAND: 
                case ITEM_STAFF: 
                    fprintf( fp, "Has %d(%d) charges of level %d",
                            obj->value[1], obj->value[2], obj->value[0] );

                    if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
                    {
                        fprintf( fp, " '%s'", skill_table[obj->value[3]].name );
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
                    fprintf(fp,"Damage is %dd%d (average %d)\n",
                            obj->value[1],obj->value[2],
                            (1 + obj->value[2]) * obj->value[1] / 2);

                    if (obj->value[4])  /* weapon flags */
                    {
                        fprintf(fp,"Weapons flags: %s\n",weapon_bits_name(obj->value[4]));
                    }
                    break;

                case ITEM_ARMOR:
                    fprintf( fp,
                            "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n", 
                            obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
                    break;
            }  /* switch */

            for ( paf = obj->affected; paf != NULL; paf = paf->next )
            {
                fprintf( fp, "Affects %s by %d, level %d",
                        affect_loc_name( paf->location ), paf->modifier,paf->level );
                /* added by Rahl */
                if ( paf->duration > -1 )
                    fprintf( fp, ", %d hours.\n", paf->duration );
                else
                    fprintf( fp, ".\n" );
                if ( paf->bitvector )
                {
                    switch ( paf->where )
                    {
                        case TO_AFFECTS:
                            fprintf( fp, "Adds %s affect.\n", affect_bit_name( paf->bitvector ) );
                            break;
                        case TO_WEAPON:
                            fprintf( fp, "Adds %s weapon flags.\n", weapon_bits_name( paf->bitvector ) );
                            break;
                        case TO_OBJECT:
                            fprintf( fp, "Adds %s object flag.\n", extra_bit_name( paf->bitvector ) );
                            break;
                        case TO_IMMUNE:
                            fprintf( fp, "Adds immunity to %s.\n", imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_RESIST:
                            fprintf( fp, "Adds resistance to %s.\n", imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_VULN:
                            fprintf( fp, "Adds vulnerability to %s.\n", imm_bit_name( paf->bitvector ) );
                            break;
                        default:
                            fprintf( fp, "Unknown bit %d %d\n", paf->where, paf->bitvector );
                            break;
                    }
                }  /* if */
            }  /* for */

            for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
            {
                fprintf( fp, "Affects %s by %d, level %d.\n",
                        affect_loc_name( paf->location ), paf->modifier,paf->level );
                if ( paf->bitvector )
                {
                    switch ( paf->where )
                    {
                        case TO_AFFECTS:
                            fprintf( fp, "Adds %s affect.\n", affect_bit_name( paf->bitvector ) );
                            break;
                        case TO_WEAPON:
                            fprintf( fp, "Adds %s weapon flags.\n", weapon_bits_name( paf->bitvector ) );
                            break;
                        case TO_OBJECT:
                            fprintf( fp, "Adds %s object flag.\n", extra_bit_name( paf->bitvector ) );
                            break;
                        case TO_IMMUNE:
                            fprintf( fp, "Adds immunity to %s.\n", imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_RESIST:
                            fprintf( fp, "Adds resistance to %s.\n", imm_bit_name( paf->bitvector ) );
                            break;
                        case TO_VULN:
                            fprintf( fp, "Adds vulnerability to %s.\n", imm_bit_name( paf->bitvector ) );
                            break;
                        default:
                            fprintf( fp, "Unknown bit %d %d\n", paf->where, paf->bitvector );
                            break;
                    }      /* switch */
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
            fprintf( fp, "\nName: '%s.'\nArea: '%s'.\n",
                    pRoomIndex->name,
                    pRoomIndex->area->name );

            fprintf( fp,
                    "Vnum: %d.  Sector: %d.  Light: %d.\n",
                    pRoomIndex->vnum,
                    pRoomIndex->sector_type,
                    pRoomIndex->light );

            fprintf( fp, "Room flags: %s.\n", print_tflag(pRoomIndex->room_flags));

            fprintf( fp, "Description:\n%s\n",
                    pRoomIndex->description );

            if ( pRoomIndex->extra_descr != NULL )
            {
                EXTRA_DESCR_DATA *ed;

                fprintf( fp, "Extra description keywords: '" );
                for ( ed = pRoomIndex->extra_descr; ed; ed = ed->next )
                {
                    fprintf( fp, "%s", ed->keyword );
                    if ( ed->next != NULL )
                        fprintf( fp, " " );
                }
                fprintf( fp, "'.\n" );
            }

            fprintf( fp, "Characters:" );
            for ( rch = pRoomIndex->people; rch; rch = rch->next_in_room )
            {
                one_argument( rch->name, buf );
                fprintf( fp, " %s", buf );
            }

            fprintf( fp, ".\nObjects:   " );
            for ( obj = pRoomIndex->contents; obj; obj = obj->next_content )
            {
                one_argument( obj->name, buf );
                fprintf( fp, " %s", buf );
            }
            fprintf( fp, ".\n" );

            for ( door = 0; door <= 5; door++ )
            {
                EXIT_DATA *pexit;

                if ( ( pexit = pRoomIndex->exit[door] ) != NULL )
                {
                    fprintf( fp,
                            "Door: %d.  To: %d.  Key: %d.  Exit flags: %s.\nKeyword: '%s'.  Description: %s",
                            door,
                            (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
                            pexit->key,
                            print_tflag( pexit->exit_info ),
                            pexit->keyword,
                            pexit->description[0] != '\0' ? pexit->description : "(none).\n" );
                }
            }

        }

    /* close file */
    fclose(fp);

    send_to_char( "Done writing files...\n\r", ch );
}
