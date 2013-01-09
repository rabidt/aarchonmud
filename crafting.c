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
#define CRFT 10553
#define MAX_MATERIALS 4


void check_craft_obj( OBJ_DATA *obj, int type );
void craft_obj_max_caster( OBJ_DATA *obj, int ops );
void craft_obj_stat_caster( OBJ_DATA *obj, int ops );
void craft_obj_roll_caster( OBJ_DATA *obj, int ops );
void craft_obj_max_physical( OBJ_DATA *obj, int ops );
void craft_obj_roll_physical( OBJ_DATA *obj, int ops );
//void craft_obj_stat( OBJ_DATA *obj, int ops );
//void craft_obj_roll( OBJ_DATA *obj, int ops );
//void craft_obj_max( OBJ_DATA *obj, int ops );
void add_craft_affect( OBJ_DATA *obj, AFFECT_DATA *aff );
void craft_obj_physical( OBJ_DATA *obj, int ops );
void craft_obj_stat_physical( OBJ_DATA *obj, int ops );

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
    { CRFT+10, 3 }, // Mephiston's Inspiration
    { CRFT+11, 3 }, // Rynor's Creativity
    { CRFT+12, 4 }, // Bobble's Brilliance
    { CRFT+13, 4 }, // Rimbol's Strength

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
    
    { "champion_gauntlets",  CRFT+42, { CRFT+12, CRFT+13 },              97 }, // Bobble's Brilliance, Rimbol's Strength
    { "champion_ring",       CRFT+41, { CRFT+8,  CRFT+10,  CRFT+13 },    97 }, // Metal Scraps, Mephiston's Inspiration, Rimbol's Strength
    { "champion_necklace",   CRFT+40, { CRFT+2,  CRFT+6,   CRFT+11 },    97 }, // Blissful Essence, Adamantium Smelt, Rynor's Creativity
    { "champion_whip",       CRFT+39, { CRFT+1,  CRFT+5,   CRFT+12 },    97 }, // Sparkling Essence, Gold Smelt, Bobble's Brilliance
    { "champion_mace",       CRFT+38, { CRFT+5,  CRFT+6,   CRFT+9  },    97 }, // Gold Smelt, Adamantium Smelt, Fastening Bolts
    { "ornate_bracer",       CRFT+37, { CRFT+5,  CRFT+15 },              94 }, // Gold Smelt, Wire Band
    { "ornate_sword",        CRFT+36, { CRFT+1,  CRFT+4,   CRFT+14 },    94 }, // Sparkling Essence, Iron Smelt, Steel Rivet
    { "ornate_visor",        CRFT+35, { CRFT+5,  CRFT+9 },               94 }, // Gold Smelt, Fastening Bolts
    { "glorious_bracers",    CRFT+34, { CRFT+2,  CRFT+5 },               92 }, // Blissful Essence, Gold Smelt
    { "glorious_robe",       CRFT+33, { CRFT+4,  CRFT+9 },               92 }, // Iron Smelt, Fastening Bolts
    { "glorious_aura",       CRFT+32, { CRFT+0,  CRFT+1,   CRFT+2  },    92 }, // Foul Essence, Sparkling Essence, Blissful Essence
    { "glorious_boots",      CRFT+31, { CRFT+8,  CRFT+9,   CRFT+15 },    92 }, // Metal Scraps, Fastening Bolts, Wire Band
    { "master_ring",         CRFT+30, { CRFT+0,  CRFT+4,   CRFT+9  },    91 }, // Foul Essence, Iron Smelt, Fastening Bolts
    { "master_leggings",     CRFT+29, { CRFT+1,  CRFT+7,   CRFT+8  },    91 }, // Sparkling Essence, Heavy Strap, Metal Scraps
    { "master_polearm",      CRFT+28, { CRFT+3,  CRFT+4,   CRFT+14 },    91 }, // Bronze Smelt, Iron Smelt, Steel Rivet
    { "master_boots",        CRFT+27, { CRFT+0,  CRFT+7,   CRFT+9  },    91 }, // Foul Essence, Heavy Straps, Fastening Bolts
    { "mighty_polearm",      CRFT+26, { CRFT+0,  CRFT+4 },               50 }, // Foul Essence, Iron Smelt
    { "mighty_breastplate",  CRFT+25, { CRFT+1,  CRFT+8 },               50 }, // Sparkling Essence, Metal Scraps
    { "mighty_vambrace",     CRFT+24, { CRFT+3,  CRFT+7 },               50 }, // Bronze Smelt, Heavy Straps
    { "splendid_shield",     CRFT+23, { CRFT+3,  CRFT+7 },               30 }, // Bronze Smelt, Heavy Straps
    { "splendid_orb",        CRFT+22, { CRFT+0,  CRFT+1 },               30 }, // Foul Essence, Sparkling Essence
    { "splendid_belt",       CRFT+21, { CRFT+7,  CRFT+8 },               30 }, // Heavy Straps, Metal Scraps
    { "fine_amulet",         CRFT+20, { CRFT+7 },                        12 }, // Heavy Straps
    { "fine_gloves",         CRFT+19, { CRFT+3 },                        12 }, // Bronze Smelt 
    { NULL, 0, {}, 0 }
};



void do_supplies( CHAR_DATA *ch, char *argument )
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

void do_craft( CHAR_DATA *ch, char *argument )
{
    int i, j, skill, craft, type;
    char buf[MSL];
    OBJ_INDEX_DATA *materials;
    OBJ_DATA *crafting;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MIL];

    argument=one_argument(argument, arg1);
    argument=one_argument(argument, arg2);

    if ( (skill = get_skill(ch, gsn_craft)) == 0 )
    {
	send_to_char( "You should learn to craft before trying this.\n\r", ch );
	return;
    }

    /* Prints list of possible materials when no argument is used */
    if ( arg1[0] == '\0' || (arg2[0]==0) )
    {
        send_to_char("Syntax: craft <item_name> <physical | mental> \n\r", ch);
	send_to_char( "Craft what? Please specify what you will be crafting.\n\r", ch );
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
        /* This passes the parameter */
	if (!strcmp(arg2, "physical"))
		type=1;
	else if (!strcmp(arg2, "mental"))
		type=2;
	else
	{
		send_to_char("Please specify if you want 'physical' or 'mental' preference for the stats\n\r", ch);
		send_to_char("Syntax: craft <item_name> <physical | mental> \n\r", ch);
		return;
	}
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




void do_extract( CHAR_DATA *ch, char *argument)
{
    int mtable, chance2, material, skill;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
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

    obj = get_obj_carry(ch, arg, ch);

 /* We should allow sticky items to be extracted, as many of them are worthless
    depending on the players' class. No_extract now working - Astark 12-23-12 
    if (is_sticky_obj(obj)) */
    if (IS_SET(obj->extra_flags, ITEM_NO_EXTRACT))
     {
         send_to_char("You can't extract materials from that item.\n\r",ch);
         return;
     }
   
    /* Only weapons and armor can be extracted */
    if ( obj->item_type != ITEM_ARMOR && obj->item_type != ITEM_WEAPON )
    {
        send_to_char( "You can only extract materials from armor and weapons.\n\r", ch );
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
 
    chance2 = rand() %100;
 
/* Used for verifying that the integers are working properly. Commented out on purpose
    sprintf( buf, "mtable = %d , chance2= %d.\n\r", mtable, chance2 );
	send_to_char( buf, ch ); 

*/


/* Items level 90 and below can only make items rarity 0-1 */
    if (mtable <= 90)
    {
        if (chance2 <= 18)
        {
            material = create_object(get_obj_index(10553), 0);
        }
        else if (chance2 > 18 && chance2 <= 36)
        {
            material = create_object(get_obj_index(10556), 0);
        }
        else if (chance2 > 36 && chance2 <= 54)
        {
            material = create_object(get_obj_index(10560), 0);
        }
        else if (chance2 > 54 && chance2 <= 65)
        {
            material = create_object(get_obj_index(10554), 0);
        }
        else if (chance2 > 65 && chance2 <= 75)
        {
            material = create_object(get_obj_index(10557), 0);
        }
        else if (chance2 > 75 && chance2 <= 88)
        {
            material = create_object(get_obj_index(10561), 0);
        }
        else if (chance2 > 88 && chance2 <= 100)
        {
            material = create_object(get_obj_index(10567), 0);
        }
    }


/* level 90 - 93 items (up to rating 2) can make items from 1-2 */
    if (mtable > 90 && mtable < 107)
    {
        if (chance2 <= 15)
        {
            material = create_object(get_obj_index(10554), 0);
        }
        else if (chance2 > 15 && chance2 <= 30)
        {
            material = create_object(get_obj_index(10557), 0);
        }
        else if (chance2 > 30 && chance2 <= 45)
        {
            material = create_object(get_obj_index(10561), 0);
        }
        else if (chance2 > 45 && chance2 <= 60)
        {
            material = create_object(get_obj_index(10567), 0);
        }
        else if (chance2 > 60 && chance2 <= 70)
        {
            material = create_object(get_obj_index(10555), 0);
        }
        else if (chance2 > 70 && chance2 <= 80)
        {
            material = create_object(get_obj_index(10558), 0);
        }
        else if (chance2 > 80 && chance2 <= 90)
        {
            material = create_object(get_obj_index(10562), 0);
        }
        else if (chance2 > 90 && chance2 <= 100)
        {
            material = create_object(get_obj_index(10568), 0);
        }
    }


/* level 93 (rating 3) - 95 (rating 2) items can make items from 2-3 */
    if (mtable >= 107 && mtable <= 115)
    {
        if (chance2 <= 13)
        {
            material = create_object(get_obj_index(10555), 0);
        }
        else if (chance2 > 13 && chance2 <= 26)
        {
            material = create_object(get_obj_index(10558), 0);
        }
        else if (chance2 > 26 && chance2 <= 39)
        {
            material = create_object(get_obj_index(10562), 0);
        }
        else if (chance2 > 39 && chance2 <= 54)
        {
            material = create_object(get_obj_index(10568), 0);
        }
        else if (chance2 > 54 && chance2 <= 70)
        {
            material = create_object(get_obj_index(10559), 0);
        }
        else if (chance2 > 70 && chance2 <= 85)
        {
            material = create_object(get_obj_index(10563), 0);
        }
        else if (chance2 > 85 && chance2 <= 100)
        {
            material = create_object(get_obj_index(10564), 0);
        }
    }


/* level 95 (rating 3) - 97 (rating 5) items can make items from 3-4 */
    if (mtable >= 116)
    {
        if (chance2 <= 15)
        {
            material = create_object(get_obj_index(10559), 0);
        }
        else if (chance2 > 15 && chance2 <= 30)
        {
            material = create_object(get_obj_index(10563), 0);
        }
        else if (chance2 > 30 && chance2 <= 50)
        {
            material = create_object(get_obj_index(10564), 0);
        }
        else if (chance2 > 50 && chance2 <= 75)
        {
            material = create_object(get_obj_index(10565), 0);
        }
        else if (chance2 > 75 && chance2 <= 100)
        {
            material = create_object(get_obj_index(10566), 0);
        }
    }

    extracted = material;
    extract_obj( obj );

    WAIT_STATE( ch, skill_table[gsn_craft].beats );
    if ( chance(skill-5) )
    {
        sprintf(buf, "%s vanishes as you extract %s from it.\n\r", obj->short_descr, extracted->short_descr);
        send_to_char(buf,ch);
        act( "$n extracts $p from $P.", ch, extracted, obj, TO_ROOM );
        obj_to_char(material, ch);
	check_improve( ch, gsn_craft, TRUE, 1 );
    }
    else
    {
	send_to_char( "Hmmm.. That didn't go as planned..\n\r", ch );
	check_improve( ch, gsn_craft, FALSE, 1 );
	return;
    }

    return;
}


/*
 * Code from enchant.c rewritten for crafting only so that
 * enchant armor/weapon don't break. Thanks Maed!
 */
int get_craft_ops( OBJ_DATA *obj, int level )
{
    AFFECT_DATA *aff;
    int ops_left, fail, vnum;

    /* no enchanting of quest eq etc. */
    if ( obj == NULL
	 || IS_OBJ_STAT(obj, ITEM_STICKY)
	 || obj->pIndexData->vnum == OBJ_VNUM_BLACKSMITH
         || (IS_SET(obj->extra_flags, ITEM_NO_EXTRACT)))
	return 0;

    /* no enchanting of objects which add special effects */
    for ( aff = obj->pIndexData->affected; aff != NULL; aff = aff->next )
	if ( aff->bitvector != 0 )
	    return 0;

    /* ops below spec */
    ops_left = get_obj_spec( obj ) - get_obj_ops( obj );
    if ( IS_OBJ_STAT(obj, ITEM_BLESS) )
	ops_left += 1;
    /* check for artificially created items => harder to enchant */
    if ( obj->pIndexData->vnum == OBJ_VNUM_SIVA_WEAPON )
	ops_left -= 5;

    /* check for failure */
    fail = 50 + UMAX(0, obj->level - level);
    fail -= 5 * ops_left;
    fail = URANGE(1, fail, 99);
    if ( chance(fail) )
    {
	fail = 0;
	while ( number_bits(2) == 0 )
	    fail--;
	return fail;
    }

    /* ok, successful enchant */
    ops_left = UMAX( 0, ops_left );
    return ops_left/2 + 1;
}


/* enchants crafting objects. Effective Nov 2012, this now 
checks for a physical or mental parameter to make the items
more useable - Astark  */

void check_craft_obj( OBJ_DATA *obj, int type )
{
    if ( obj == NULL || !IS_OBJ_STAT(obj, ITEM_RANDOM) )
	return;

    REMOVE_BIT( obj->extra_flags, ITEM_RANDOM );
  
    if (type == 1)
        craft_obj_physical( obj, get_obj_spec(obj) - get_obj_ops(obj) );
    else
        craft_obj_caster( obj, get_obj_spec(obj) - get_obj_ops(obj) );
}



void add_craft_affect( OBJ_DATA *obj, AFFECT_DATA *aff )
{
    AFFECT_DATA *obj_aff;

    if ( obj == NULL || aff == NULL )
	return;

    /* search for matching affect already on object */
    for ( obj_aff = obj->affected; obj_aff != NULL; obj_aff = obj_aff->next )
	if ( obj_aff->location == aff->location
	     && obj_aff->duration == aff->duration )
	    break;

    /* found matching affect on object? */
    if ( obj_aff != NULL )
	obj_aff->modifier += aff->modifier;
    else
	affect_to_obj( obj, aff );
}





/* Section added for physical paramter */
void craft_obj_physical( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA aff;
    int add;

    if ( obj == NULL || ops <= 0 )
	return;

    /* stats */
    add = number_range( 5, 20 );          /* min of 4, max of 16 ops for stats */
    craft_obj_stat_physical( obj, add );  
    ops -= add;                           /* lower available ops to keep item in spec */
    add = number_range( ops/7, ops/2 );     /* will determine how many ops to spent on HR/DR/AC/Saves */
    craft_obj_roll_physical( obj, add );  
    ops -= add;                     
    craft_obj_max_physical( obj, ops );   /* uses the rest on HP/Mana/Move */
}

/* add str, con, .. */
void craft_obj_stat_physical( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    AFFECT_DATA *aff;
    int add, total, max;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops * 4;
    
    while ( total > 0 )
    {
	/* modifier */
	if ( total < 4 )
	    add = total;
	else
	{
	    max = UMIN( 10, total );
	    add = number_range( 4, max );
	}
	af.modifier = add;

	/* location */
        if ( !number_bits(1) )
        {
    	    switch ( number_range(0, 7) )
	    {
	    case 0: af.location = APPLY_STR; break;
	    case 1: af.location = APPLY_CON; break;
	    case 2: af.location = APPLY_VIT; break;
	    case 3: af.location = APPLY_AGI; break;
	    case 4: af.location = APPLY_DEX; break;
	    case 5: af.location = APPLY_DIS; break;
	    case 6: af.location = APPLY_CHA; break;
	    case 7: af.location = APPLY_LUC; break;
	    }
        }
        else
        {
    	    switch ( number_range(0, 5) )
	    {
	    case 0: af.location = APPLY_STR; break;
	    case 1: af.location = APPLY_CON; break;
	    case 2: af.location = APPLY_VIT; break;
	    case 3: af.location = APPLY_AGI; break;
	    case 4: af.location = APPLY_DEX; break;
	    case 5: af.location = APPLY_LUC; break;
	    }
        }

	add_craft_affect( obj, &af );
	
	total -= add;
    }
}

/* add hit, dam, ac, saves */
void craft_obj_roll_physical( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    int add, total, max, choice;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops;
    
    while ( total > 0 )
    {
	/* modifier */
	max = UMIN( 5, total );
	add = number_range( 1, max );
	af.modifier = add;
	
	/* location */
	switch ( obj->item_type )
	{
	default:
	    choice = number_range(0,3); break;
	case ITEM_WEAPON:
	    if ( !number_bits(2) )             /* You have a better chance of getting HR/DR than saves & AC */
		choice = number_range(0,3);
	    else if ( !number_bits(1) )
		choice = number_range(0,2);   /* Same as above.. little better chance of AC than saves on weapons */
            else
                choice = number_range(0,1);
	case ITEM_ARMOR:
	    if ( !number_bits(2) )             /* You have a better chance of getting HR/DR than saves & AC */
		choice = number_range(0,3);
	    else
		choice = number_range(0,1);
	}
	/* translate location & adjust modifier */
	switch( choice )
	{
	default: return;
	case 0: af.location = APPLY_HITROLL; break;
	case 1: af.location = APPLY_DAMROLL; break;
	case 2:
	    af.location = APPLY_AC; 
	    af.modifier *= -10;
	    break;
	case 3:
	    af.location = APPLY_SAVES;
	    af.modifier *= -1;
	}
		
	add_craft_affect( obj, &af );
	
	total -= add;
    }
}

/* add hp, mana, move */
void craft_obj_max_physical( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    int add, total, max, choice;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops * 10;
    
    while ( total > 0 )
    {
	/* modifier */
	if ( total < 20 )
	    add = total;
	else
	{
	    max = UMIN( 50, total );
	    add = number_range( 10, max );
	}
	af.modifier = add;

	/* location */
        if (!number_bits(2))
            choice = number_range(0,1); /* Better chance of getting HP, than Move */
        else
            choice = number_range(0,0);
	switch ( choice )
	{
	case 0: af.location = APPLY_HIT; break;
	case 1: af.location = APPLY_MOVE; break;
	}

	add_craft_affect( obj, &af );
	
	total -= add;
    }
}

void add_craft_affect_physical( OBJ_DATA *obj, AFFECT_DATA *aff )
{
    AFFECT_DATA *obj_aff;

    if ( obj == NULL || aff == NULL )
	return;

    /* search for matching affect already on object */
    for ( obj_aff = obj->affected; obj_aff != NULL; obj_aff = obj_aff->next )
	if ( obj_aff->location == aff->location
	     && obj_aff->duration == aff->duration )
	    break;

    /* found matching affect on object? */
    if ( obj_aff != NULL )
	obj_aff->modifier += aff->modifier;
    else
	affect_to_obj( obj, aff );
}





/* Section added for caster parameter */
void craft_obj_caster( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA aff;
    int add;

    if ( obj == NULL || ops <= 0 )
	return;

    /* stats */
    add = number_range( 4, 18 );         /* min of 4, max of 18 ops for stats */
    craft_obj_stat_caster( obj, add );   
    ops -= add;                          /* lower available ops to keep item in spec */
    add = number_range( ops/8, ops/2 );  /* will determine how many ops to spent on HR/DR/AC/Saves */
    craft_obj_roll_caster( obj, add );   
    ops -= add;                          
    craft_obj_max_caster( obj, ops );    /* uses the rest on HP/Mana/Move */
}

/* add str, con, .. */
void craft_obj_stat_caster( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    AFFECT_DATA *aff;
    int add, total, max;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops * 4;
    
    while ( total > 0 )
    {
	/* modifier */
	if ( total < 4 )
	    add = total;
	else
	{
	    max = UMIN( 10, total );
	    add = number_range( 4, max );
	}
	af.modifier = add;

	/* location */
	switch ( number_range(0, 7) )
	{
	case 0: af.location = APPLY_CON; break;
	case 1: af.location = APPLY_VIT; break;
	case 2: af.location = APPLY_AGI; break;
	case 3: af.location = APPLY_INT; break;
	case 4: af.location = APPLY_WIS; break;
	case 5: af.location = APPLY_DIS; break;
	case 6: af.location = APPLY_CHA; break;
	case 7: af.location = APPLY_LUC; break;
	}

	add_craft_affect( obj, &af );
	
	total -= add;
    }
}

/* add hit, dam, ac, saves */
void craft_obj_roll_caster( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    int add, total, max, choice;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops;
    
    while ( total > 0 )
    {
	/* modifier */
	max = UMIN( 5, total );
	add = number_range( 1, max );
	af.modifier = add;
	
	/* location */
	switch ( obj->item_type )
	{
	default:
	    choice = number_range(0,3); break;
	case ITEM_WEAPON:
	    if ( !number_bits(2) )             // You have a better chance of getting saves & AC than HR/DR
		choice = number_range(0,3);
	    else
		choice = number_range(2,3);
	case ITEM_ARMOR:
	    if ( !number_bits(2) )             // You have a better chance of getting saves & AC than HR/DR
		choice = number_range(0,3);
	    else
		choice = number_range(2,3);
	}
	/* translate location & adjust modifier */
	switch( choice )
	{
	default: return;
	case 0: af.location = APPLY_HITROLL; break;
	case 1: af.location = APPLY_DAMROLL; break;
	case 2:
	    af.location = APPLY_SAVES;
	    af.modifier *= -1;
	    break;
	case 3:
	    af.location = APPLY_AC; 
	    af.modifier *= -10;
	}
		
	add_craft_affect( obj, &af );
	
	total -= add;
    }
}

/* add hp, mana, move */
void craft_obj_max_caster( OBJ_DATA *obj, int ops )
{
    AFFECT_DATA af;
    int add, total, max, choice;
    
    af.where        = TO_OBJECT;
    af.type         = 0;
    af.level        = 0;
    af.duration     = -1;
    af.bitvector    = 0;
    af.detect_level = 0;

    total = ops * 10;
    
    while ( total > 0 )
    {
	/* modifier */
	if ( total < 20 )
	    add = total;
	else
	{
	    max = UMIN( 50, total );
	    add = number_range( 10, max );
	}
	af.modifier = add;

	/* location */
        if ( !number_bits(3) )
            choice = number_range(0,2); /* Better chance of getting Mana than HP or Move */
        else if ( !number_bits(1) )
            choice = number_range(0,1);
        else
            choice = 1;
	switch ( choice )
	{
	case 0: af.location = APPLY_HIT; break;
	case 1: af.location = APPLY_MANA; break;
        case 2: af.location = APPLY_MOVE; break;
	}

	add_craft_affect( obj, &af );
	
	total -= add;
    }
}