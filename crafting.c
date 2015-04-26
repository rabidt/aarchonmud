/* 
 * Crafting by Astark - crimsonsage@gmail.com - June 2012
 * Loosely based on Alchemy by Bobble
 */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"

/* crafting area starting vnum */
#define CRFT 8600
#define MAX_MATERIALS 4


void check_craft_obj( OBJ_DATA *obj, int type );

struct materials_type
{
    int vnum;
    int rarity;
};


struct materials_type materials_table[] =
{
/*    Vnum  , Rarity */
    { CRFT+0,  0 }, // Foul Essence
    { CRFT+1,  1 }, // Sparkling Essence
    { CRFT+2,  2 }, // Blissful Essence
    { CRFT+3,  0 }, // Bronze Smelt
    { CRFT+4,  1 }, // Iron Smelt
    { CRFT+5,  2 }, // Gold Smelt
    { CRFT+6,  3 }, // Adamantium Smelt
    { CRFT+7,  0 }, // Heavy Straps
    { CRFT+8,  1 }, // Metal Scraps
    { CRFT+14, 1 }, // Steel Rivet
    { CRFT+9,  2 }, // Fastening Bolts
    { CRFT+15, 2 }, // Wire Band
    { CRFT+53, 2 }, // Titanium Thread
    { CRFT+10, 3 }, // Mephiston's Inspiration
    { CRFT+11, 3 }, // Rynor's Creativity
    { CRFT+12, 4 }, // Bobble's Brilliance
    { CRFT+13, 4 }, // Rimbol's Strength
    { CRFT+51, 4 }, // Vodur's Mischief
    { CRFT+52, 4 }, // Astark's Ambition

    { 0, 0 }
};



struct crafting_type
{
    char *name;
    int crafting_vnum;
    int materials_vnum[MAX_MATERIALS];
    int level;
};

struct crafting_type crafting_table[] =
{
//fine, splendid, mighty, master, glorious, champion
//12  , 30      , 50    , 91    , 92      , 97
//other possible affixes for future use: heroic, magnificent, illustrious, radiant, imperial, marvelous
    

    { "sacred_sabatons",     CRFT+50, { CRFT+51, CRFT+52 },              98 }, // Astark's Ambition, Vodur's Mischief
    { "sacred_hauberk",      CRFT+49, { CRFT+52, CRFT+13 },              98 }, // Astark's Ambition, Rimbol's Strength
    { "sacred_greaves",      CRFT+48, { CRFT+51, CRFT+12 },              98 }, // Vodur's Mischief, Bobble's Brilliance
    { "sacred_saber",        CRFT+54, { CRFT+14, CRFT+10, CRFT+11, },    98 }, // Steel Rivet, Mephiston's Inspiration, Rynor's Creativity
    { "sacred_blaster",      CRFT+55, { CRFT+10, CRFT+11, CRFT+6,  },    98 }, // Adamantium Smelt, Mephiston's, Rynor's
    { "champion_gauntlets",  CRFT+39, { CRFT+12, CRFT+13 },              97 }, // Bobble's Brilliance, Rimbol's Strength
    { "champion_ring",       CRFT+38, { CRFT+8,  CRFT+10,  CRFT+13 },    97 }, // Metal Scraps, Mephiston's Inspiration, Rimbol's Strength
    { "champion_necklace",   CRFT+37, { CRFT+2,  CRFT+6,   CRFT+11 },    97 }, // Blissful Essence, Adamantium Smelt, Rynor's Creativity
    { "champion_whip",       CRFT+36, { CRFT+1,  CRFT+5,   CRFT+12 },    97 }, // Sparkling Essence, Gold Smelt, Bobble's Brilliance
    { "champion_mace",       CRFT+35, { CRFT+5,  CRFT+6,   CRFT+9  },    97 }, // Gold Smelt, Adamantium Smelt, Fastening Bolts
    { "ornate_legplates",    CRFT+47, { CRFT+6,  CRFT+15 },              94 }, // Adamantium Smelt, Wire Band
    { "ornate_bracer",       CRFT+34, { CRFT+5,  CRFT+15 },              94 }, // Gold Smelt, Wire Band
    { "ornate_sword",        CRFT+33, { CRFT+1,  CRFT+4,   CRFT+14 },    94 }, // Sparkling Essence, Iron Smelt, Steel Rivet
    { "ornate_visor",        CRFT+32, { CRFT+5,  CRFT+9 },               94 }, // Gold Smelt, Fastening Bolts
    { "glorious_pendant",    CRFT+46, { CRFT+15, CRFT+2 },               92 }, // Wire Band, Blissful Essence
    { "glorious_sleeves",    CRFT+45, { CRFT+14, CRFT+53},               92 }, // Steel Rivet, Titanium Thread
    { "glorious_bracers",    CRFT+31, { CRFT+2,  CRFT+5 },               92 }, // Blissful Essence, Gold Smelt
    { "glorious_robe",       CRFT+30, { CRFT+4,  CRFT+9 },               92 }, // Iron Smelt, Fastening Bolts
    { "glorious_aura",       CRFT+29, { CRFT+0,  CRFT+1,   CRFT+2  },    92 }, // Foul Essence, Sparkling Essence, Blissful Essence
    { "glorious_boots",      CRFT+28, { CRFT+8,  CRFT+9,   CRFT+15 },    92 }, // Metal Scraps, Fastening Bolts, Wire Band
    { "master_cuirass",      CRFT+44, { CRFT+53, CRFT+1},                91 }, // Titanium Thread, Sparkling Essence
    { "master_girdle",       CRFT+43, { CRFT+53, CRFT+3, CRFT+0 },       91 }, // Titanium Thread, Bronze Smelt, Foul Essence
    { "master_ring",         CRFT+27, { CRFT+0,  CRFT+4,   CRFT+9  },    91 }, // Foul Essence, Iron Smelt, Fastening Bolts
    { "master_leggings",     CRFT+26, { CRFT+1,  CRFT+7,   CRFT+8  },    91 }, // Sparkling Essence, Heavy Strap, Metal Scraps
    { "master_polearm",      CRFT+25, { CRFT+3,  CRFT+4,   CRFT+14 },    91 }, // Bronze Smelt, Iron Smelt, Steel Rivet
    { "master_boots",        CRFT+24, { CRFT+0,  CRFT+7,   CRFT+9  },    91 }, // Foul Essence, Heavy Straps, Fastening Bolts
    { "mighty_flail",        CRFT+42, { CRFT+14, CRFT+3},                50 }, // Steel Rivet, Bronze Smelt
    { "mighty_spear",        CRFT+23, { CRFT+0,  CRFT+4 },               50 }, // Foul Essence, Iron Smelt
    { "mighty_breastplate",  CRFT+22, { CRFT+1,  CRFT+8 },               50 }, // Sparkling Essence, Metal Scraps
    { "mighty_vambrace",     CRFT+21, { CRFT+3,  CRFT+7 },               50 }, // Bronze Smelt, Heavy Straps
    { "splendid_rifle",      CRFT+41, { CRFT+4,  CRFT+8 },               30 }, // Iron Smelt, Metal Scraps
    { "splendid_shield",     CRFT+20, { CRFT+3,  CRFT+7 },               30 }, // Bronze Smelt, Heavy Straps
    { "splendid_orb",        CRFT+19, { CRFT+0,  CRFT+1 },               30 }, // Foul Essence, Sparkling Essence
    { "splendid_belt",       CRFT+18, { CRFT+7,  CRFT+8 },               30 }, // Heavy Straps, Metal Scraps
    { "fine_dagger",         CRFT+40, { CRFT+0 },                        12 }, // Foul Essence
    { "fine_amulet",         CRFT+17, { CRFT+7 },                        12 }, // Heavy Straps
    { "fine_gloves",         CRFT+16, { CRFT+3 },                        12 }, // Bronze Smelt 
    { NULL, 0, {}, 0 }
};



DEF_DO_FUN(do_supplies)
{
    int i;
    char buf[MSL], *rarity;
    OBJ_INDEX_DATA *materials;

    send_to_char( "The following crafting materials exist:\n\r", ch );

    for ( i = 0; materials_table[i].vnum != 0; i++ )
    {
	if ( (materials = get_obj_index(materials_table[i].vnum)) == NULL )
	    continue;
	switch ( materials_table[i].rarity )
	{
	case 0: rarity = "very easy to extract"; break;
	case 1: rarity = "easy to extract"; break;
	case 2: rarity = "difficult to extract"; break;
	case 3: rarity = "very difficult to extract"; break;
	default: rarity = "extremely difficult to extract"; break;
	}
	sprintf( buf, "%29s is %s.\n\r", materials->short_descr, rarity);
	send_to_char( buf, ch );
    }
}

/* Updated November 2012 - Includes a second argument so players can specifiy
whether or not they want Physical or Mental stats - Astark */

DEF_DO_FUN(do_craft)
{
    int i, j, skill, craft, type;
    char buf[MSL];
    OBJ_INDEX_DATA *materials;
    OBJ_DATA *crafting;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MIL];

    argument=one_argument(argument, arg1);
    argument=one_argument(argument, arg2);

    /* Prints list of possible materials when no argument is used */
    if ( arg1[0] == '\0' )
    {
        send_to_char("Craft what? Please specify what you will be crafting.\n\r", ch);
        send_to_char("Syntax: craft <item_name> [physical|mental]\n\r", ch);
        send_to_char("        craft <item_name> [strengthen|lighten]\n\r", ch);
        for ( i = 0; crafting_table[i].name != NULL; i++ )
	{
	    if ( (materials = get_obj_index(crafting_table[i].materials_vnum[0])) == NULL )
		continue;

	    sprintf( buf, "lvl %2d %20s:  %s", crafting_table[i].level,
		     crafting_table[i].name, materials->short_descr );
	    send_to_char( buf, ch );

            /* Loop to print table */
	    for ( j = 1; j < MAX_MATERIALS; j++ )
		if ( crafting_table[i].materials_vnum[j] != 0 )
		{
		    if ( (materials = get_obj_index(crafting_table[i].materials_vnum[j])) == NULL )
			continue;
		    sprintf( buf, " + %s", materials->short_descr );
		    send_to_char( buf, ch );
		}

	    send_to_char("\n\r", ch );
	}
	return;
    }

    if ( (skill = get_skill(ch, gsn_craft)) == 0 )
    {
	send_to_char( "You should learn to craft before trying this.\n\r", ch );
	return;
    }

    // craft usage to toggle heavy_armor flag
    bool strengthen = !strcmp(arg2, "strengthen");
    bool weaken = !strcmp(arg2, "lighten");
    if ( strengthen || weaken )
    {
        if ( !(crafting = get_obj_carry(ch, arg1, ch)) )
        {
            send_to_char("You do not carry that item.\n\r", ch);
            return;
        }
        int ac_factor = itemwear_ac_factor(first_itemwear(crafting));
        if ( ac_factor == 0 )
        {
            ptc(ch, "%s cannot be strengthened or lightened.\n\r", crafting->short_descr);
            return;
        }
        if ( strengthen && IS_OBJ_STAT(crafting, ITEM_HEAVY_ARMOR) )
        {
            ptc(ch, "%s is as strong as you can make it.\n\r", crafting->short_descr);
            return;
        }
        if ( weaken && !IS_OBJ_STAT(crafting, ITEM_HEAVY_ARMOR) )
        {
            ptc(ch, "%s is as light as you can make it.\n\r", crafting->short_descr);
            return;
        }
        int cost_level = 10 + crafting->level + UMAX(0, crafting->level - 90) * 9;
        int cost = cost_level * cost_level * ac_factor;
        if ( ch->silver + ch->gold * 100 < cost )
        {
            ptc(ch, "It costs %.2f gold to %s %s.\n\r",
                cost * 0.01,
                strengthen ? "strengthen" : "lighten",
                crafting->short_descr
            );
            return;
        }
        WAIT_STATE( ch, skill_table[gsn_craft].beats );
        deduct_cost(ch, cost);
        if ( !per_chance(skill) )
        {
            ptc(ch, "Your attempt to %s %s fails, wasting %.2f gold worth of materials.\n\r",
                strengthen ? "strengthen" : "lighten",
                crafting->short_descr,
                cost * 0.01
            );
            check_improve(ch, gsn_craft, FALSE, 2);
        }
        else
        {
            ptc(ch, "You %s %s, using up %.2f gold worth of materials.\n\r",
                strengthen ? "strengthen" : "lighten",
                crafting->short_descr,
                cost * 0.01
            );
            if ( strengthen )
                SET_BIT(crafting->extra_flags, ITEM_HEAVY_ARMOR);
            else
                REMOVE_BIT(crafting->extra_flags, ITEM_HEAVY_ARMOR);
            check_improve(ch, gsn_craft, TRUE, 2);
        }
        return;
    }
    
    /* Check which item is crafted */
    craft = -1;
    for ( i = 0; crafting_table[i].name != NULL; i++ )
	if ( !str_cmp(crafting_table[i].name, arg1) )
	{
	    craft = i;
	    break;
	}

    if ( craft == -1 )
    {
	send_to_char( "There is no such recipe to craft.\n\r", ch );
	return;
    }

    if ( ch->level < crafting_table[craft].level )
    {
	send_to_char( "You don't know that recipe.\n\r", ch );
	return;
    }

    /* check if materials are there */
    for ( j = 0; j < MAX_MATERIALS; j++ )
	if ( crafting_table[craft].materials_vnum[j] > 0
	     && obj_on_char(ch,  crafting_table[craft].materials_vnum[j]) == NULL )
	{
	    send_to_char( "You lack the required materials.\n\r", ch );
	    return;
	}

    /* This passes the parameter */
    if ( !strcmp(arg2, "physical") )
        type = ITEM_RANDOM_PHYSICAL;
    else if ( !strcmp(arg2, "mental") )
        type = ITEM_RANDOM_CASTER;
    else if ( !strcmp(arg2, "") )
        type = ITEM_RANDOM;
    else
    {
	send_to_char("Please specify if you want 'physical' or 'mental' preference for the stats\n\r", ch);
	send_to_char("Syntax: craft <item_name> [physical|mental]\n\r", ch);
	return;
    }

    /* make sure the object exists */
    if ( get_obj_index(crafting_table[craft].crafting_vnum) == NULL )
    {
	send_to_char( "Sorry, that item doesn't exist yet.\n\r", ch );
	return;
    }

    /* extract materials */
    for ( j = 0; j < MAX_MATERIALS; j++ )
	if ( crafting_table[craft].materials_vnum[j] > 0 )
	    extract_obj( obj_on_char(ch,  crafting_table[craft].materials_vnum[j]) );
     
    /* Creates the crafted object */
    WAIT_STATE( ch, skill_table[gsn_craft].beats );
    if ( chance(skill) )
    {
	crafting = create_object( get_obj_index(crafting_table[craft].crafting_vnum), 0 ); 
        check_craft_obj( crafting, type );
        /* Hopefully not needed.. but in case */
	if ( crafting == NULL )
	    return;
	obj_to_char( crafting, ch );
	act( "You craft : $p.", ch, crafting, NULL, TO_CHAR );
	act( "$n crafts : $p.", ch, crafting, NULL, TO_ROOM );
	check_improve( ch, gsn_craft, TRUE, 1 );
    }
    else
    {
	send_to_char( "Hmmm.. That didn't go as planned.\n\r", ch );
	check_improve( ch, gsn_craft, FALSE, 1 );
	return;
    }
}




DEF_DO_FUN(do_extract)
{
    int mtable, material = 0, skill;
    OBJ_DATA *obj;
    OBJ_DATA *extracted;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg );

    if ( (skill = get_skill(ch, gsn_craft)) == 0 )
    {
        send_to_char( "You need to learn crafting before you can extract anything.\n\r", ch );
        return;
    }

    if ( arg[0] == '\0' )
    {
        send_to_char( "Extract crafting materials from what?\n\r", ch );
        return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        send_to_char("You aren't carrying that.\n\r",ch);
        return;
    }

    /* We should allow sticky items to be extracted, as many of them are worthless
       depending on the players' class. No_extract now working - Astark 12-23-12 
       if (is_sticky_obj(obj)) */
    if (IS_SET(obj->extra_flags, ITEM_NO_EXTRACT))
    {
        send_to_char("You can't extract materials from that item.\n\r",ch);
        return;
    }

    /* Only weapons and armor can be extracted */
    if ( obj->item_type != ITEM_ARMOR && obj->item_type != ITEM_WEAPON)
    {
        send_to_char( "You can only extract materials from armor and weapons.\n\r", ch );
        return;
    } 

    /* No mob equipment either */
    if (obj->level > 100)
    {
        send_to_char( "Equipment designed for mobs cannot be extracted.\n\r", ch);
        return;
    }

    logpf( "%s extracted %s", ch->name, remove_color(obj->short_descr));

    /* The materials you get are dependent on the level and rating of the object that you extract */
    if ( obj->level < 90)
    { 
        mtable = obj->level;      
    }
    else
    {
        mtable  = obj->level;
        mtable += ((obj->level - 90) * 3);
        mtable += obj->pIndexData->diff_rating * 2;
    }

    /* Items level 90 and below can only make items rarity 0-1 */
    if (mtable <= 90)
    {
        switch (number_range(0,6))
        {
            case 0: material = 8600; break; // foul essence
            case 1: material = 8601; break; // sparkling essence
            case 2: material = 8603; break; // bronze smelt
            case 3: material = 8604; break; // iron smelt
            case 4: material = 8607; break; // heavy straps
            case 5: material = 8608; break; // metal scraps
            case 6: material = 8614; break; // steel rivet
        }
    }


    /* level 90 - 93 items (up to rating 2) can make items from 1-2 */
    if (mtable > 90 && mtable < 107)
    {
        switch (number_range(0,8))
        {
            case 0: material = 8601; break; // sparkling essence
            case 1: material = 8602; break; // blissful essence
            case 2: material = 8604; break; // iron smelt
            case 3: material = 8605; break; // gold smelt
            case 4: material = 8608; break; // metal scraps
            case 5: material = 8609; break; // fastening bolts
            case 6: material = 8614; break; // steel rivet
            case 7: material = 8615; break; // wire band
            case 8: material = 8653; break; // titanium thread
        }
    }

    /* level 93 (rating 3) - 95 (rating 2) items can make items from 2-3 */
    if (mtable >= 107 && mtable <= 115)
    {
        switch (number_range(0,7))
        {
            case 0: material = 8602; break; // blissful essence
            case 1: material = 8605; break; // gold smelt
            case 2: material = 8606; break; // adamantium smelt
            case 3: material = 8609; break; // fastening bolts
            case 4: material = 8610; break; // mephiston's inspiration
            case 5: material = 8611; break; // rynor's creativity
            case 6: material = 8615; break; // wire band
            case 7: material = 8653; break; // titanium thread
        }
    }


    /* level 95 (rating 3) - 97 (rating 5) items can make items from 3-4 */
    if (mtable >= 116)
    {
        switch (number_range(0,6))
        {
            case 0: material = 8606; break; // adamantium smelt
            case 1: material = 8610; break; // mephiston's inspiration
            case 2: material = 8611; break; // rynor's creativity
            case 3: material = 8612; break; // bobble's brilliance
            case 4: material = 8613; break; // rimbols strength
            case 5: material = 8651; break; // vodurs mischief
            case 6: material = 8652; break; // astarks ambition
        }
    }


    if ( chance(skill-5) )
    {
        extracted=create_object(get_obj_index(material),0);
        sprintf(buf, "%s vanishes as you extract %s from it.\n\r", obj->short_descr, extracted->short_descr);
        send_to_char(buf,ch);
        act( "$n extracts $p from $P.", ch, extracted, obj, TO_ROOM );
        obj_to_char(extracted, ch);
        check_improve( ch, gsn_craft, TRUE, 1 );
    }
    else
    {
        send_to_char( "Hmmm.. That didn't go as planned..\n\r", ch );
        check_improve( ch, gsn_craft, FALSE, 1 );
    }
    WAIT_STATE( ch, skill_table[gsn_craft].beats );
    extract_obj( obj );

    return;
}

/* enchants crafting objects. Effective Nov 2012, this now 
checks for a physical or mental parameter to make the items
more useable - Astark  */

void check_craft_obj( OBJ_DATA *obj, int type )
{
    if ( obj == NULL || !IS_OBJ_STAT(obj, ITEM_RANDOM) )
        return;

    if ( type != ITEM_RANDOM )
    {
        REMOVE_BIT(obj->extra_flags, ITEM_RANDOM);
        SET_BIT(obj->extra_flags, type);
    }
    check_enchant_obj(obj);
}
