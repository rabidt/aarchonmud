/* 
 * Alchemy system
 * by Henning Koehler <koehlerh@in.tum.de>
 */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"

void reset_herbs_area( AREA_DATA *pArea );
void reset_herbs( ROOM_INDEX_DATA *room );

DECLARE_SPELL_FUN( spell_blindness      );
DECLARE_SPELL_FUN( spell_create_rose    );
struct herb_type
{
    int vnum;
    int sector;
    int rarity;
    int reset_nr; // for counting nr of herbs reset
};

/* alchemy area starting vnum */
#define AAS 17900

struct herb_type herb_table[] =
{
    { AAS+0,  SECT_FIELD,         0, 0 }, // Cutleaf
    { AAS+1,  SECT_FOREST,        0, 0 }, // Alraune
    { AAS+2,  SECT_HILLS,         1, 0 }, // Gust Grass
    { AAS+3,  SECT_MOUNTAIN,      1, 0 }, // Fire Berry
    { AAS+4,  SECT_WATER_SHALLOW, 2, 0 }, // Devil's Root
    { AAS+5,  SECT_FIELD,         2, 0 }, // Angel Finger
    { AAS+6,  SECT_DESERT,        3, 0 }, // Blood Thorn
    { AAS+7,  SECT_FOREST,        3, 0 }, // Winterfox
    { AAS+8,  SECT_HILLS,         4, 0 }, // Faerie Wing
    { AAS+9,  SECT_WATER_SHALLOW, 4, 0 }, // Dragon's Bane
    { AAS+10, SECT_DESERT,        4, 0 }, // Desert Rose
    { AAS+11, SECT_MOUNTAIN,      4, 0 }, // Wind Wisper
    { 0, 0, 0, 0 }
};

//#define HERB_DEBUG

/* some routines to make herb reset independant of area reset */
void reset_herbs_world()
{
    AREA_DATA *pArea;
#ifdef HERB_DEBUG
    int i;
    for ( i = 0; i < 24; i++ )
#endif
    for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
        /* In game areas only -- Astark */
        if ( is_area_ingame( pArea ) )
	        reset_herbs_area( pArea );
#ifdef HERB_DEBUG
    update_herb_reset();
#endif
}

void reset_herbs_area( AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoom;
    int vnum;
    
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
        if ( !number_bits(2) && (pRoom = get_room_index(vnum)) )
            reset_herbs( pRoom );
}

/* reset herbs in a room */
void reset_herbs( ROOM_INDEX_DATA *room )
{
    int i;
    OBJ_DATA *herb;

    if ( room == NULL || IS_SET(room->room_flags, ROOM_BARREN))
	return;

    for ( i = 0; herb_table[i].vnum != 0; i++ )
    {
	if ( room->sector_type != herb_table[i].sector )
	    continue;

	if ( number_bits(herb_table[i].rarity + 5) )
	    continue;

	/* if already reset much, lower chances for new ones */
	if ( herb_table[i].reset_nr << herb_table[i].rarity >= (1<<5)
	     && number_bits(2) )
	    continue;

	/* check if a herb of this type already exists */
	for ( herb = room->contents; herb != NULL; herb = herb->next )
	    if ( herb->pIndexData->vnum == herb_table[i].vnum )
		break;
	if ( herb != NULL )
	    continue;

	if ( (herb = create_object(get_obj_index(herb_table[i].vnum), 0)) == NULL )
	    continue;

	obj_to_room( herb, room );
	herb_table[i].reset_nr++;
    }
}

/* rot herbs that haven't been picked up */
int rot_herbs( int vnum )
{
    OBJ_DATA *obj, *obj_next;
    int nr = 0;

    for ( obj = object_list; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next;

	if ( obj->pIndexData->vnum != vnum || obj->in_room == NULL )
	    continue;

	if ( number_bits(2) == 0 )
	{
	    extract_obj( obj );
	    nr++;
	}
    }
    
    return nr;
}

/* to check nr of herbs reset */
void update_herb_reset()
{
    int i, nr;
    char buf[MSL], outbuf[MSL];

    sprintf( outbuf, "herbs reset/rotten: " );
    for ( i = 0; herb_table[i].vnum != 0; i++ )
    {
	nr = rot_herbs( herb_table[i].vnum );

	/* print to log */
	sprintf( buf, " %2d/%2d", herb_table[i].reset_nr, nr );
	strcat( outbuf, buf );

	herb_table[i].reset_nr = 0;
    }
    log_string( outbuf );
}

DEF_DO_FUN(do_herbs)
{
    int i;
    char buf[MSL], *rarity;
    OBJ_INDEX_DATA *herb;

    send_to_char( "The following herbs exist:\n\r", ch );

    for ( i = 0; herb_table[i].vnum != 0; i++ )
    {
	if ( (herb = get_obj_index(herb_table[i].vnum)) == NULL )
	    continue;
	switch ( herb_table[i].rarity )
	{
	case 0: rarity = "very common"; break;
	case 1: rarity = "common"; break;
	case 2: rarity = "uncommon"; break;
	case 3: rarity = "very uncommon"; break;
	case 4: rarity = "rare"; break;
	default: rarity = "very rare"; break;
	}
	sprintf( buf, "%s is %s and has %s as habitat.\n\r",
		 herb->short_descr, rarity,
		 flag_bit_name(sector_flags, herb_table[i].sector) );
	send_to_char( buf, ch );
    }
}

#define MAX_HERB 3

struct recipe_type
{
    char *name;
    int potion_vnum;
    int herb_vnum[MAX_HERB];
    int level;
};

/*
    { AAS+00, SECT_FIELD,         0 }, // Cutleaf (mottled)
    { AAS+01, SECT_FOREST,        0 }, // Alraune (black)
    { AAS+02, SECT_HILLS,         1 }, // Gust Grass (silver)
    { AAS+03, SECT_MOUNTAIN,      1 }, // Fire Berry (red)
    { AAS+04, SECT_WATER_SHALLOW, 2 }, // Devil's Root (black)
    { AAS+05, SECT_FIELDS,        2 }, // Angel Finger (mottled)
    { AAS+06, SECT_DESERT,        3 }, // Blood Thorn (red)
    { AAS+07, SECT_HILLS,         3 }, // Winterfox (silver)
    { AAS+08, SECT_FOREST,        4 }, // Faerie Wing (silver, invis)
    { AAS+09, SECT_WATER_SHALLOW, 4 }, // Dragon's Bane (black)
    { AAS+10, SECT_DESERT,        5 }, // Desert Rose (red)
    { AAS+11, SECT_MOUNTAIN,      5 }, // Wind Wisper (mottled)
*/

struct recipe_type recipe_table[] =
{
    { "energizing",  AAS+19, { AAS+11, AAS+6,  AAS+3 }, 80  },
    { "miracle",  AAS+20, { AAS+11, AAS+10, AAS+8 }, 80  },
    { "hollow",   AAS+21, { AAS+8,  AAS+2,  AAS+0 }, 70  },
    { "shimmer",  AAS+22, { AAS+6,  AAS+5,  AAS+0 }, 60  },
    { "fulmin",   AAS+23, { AAS+9,  AAS+3 },         50  },
    { "spark",    AAS+24, { AAS+9,  AAS+2 },         50  },
    { "icy",      AAS+25, { AAS+7,  AAS+4,  AAS+0 }, 50  },
    { "pest",     AAS+26, { AAS+6,  AAS+4,  AAS+1 }, 50  },
    { "angel",    AAS+27, { AAS+5,  AAS+0 },         30  },
    { "devil",    AAS+28, { AAS+4,  AAS+0 },         30  },
    { "arcane",   AAS+29, { AAS+7,  AAS+2,  AAS+0 }, 20  },
    { "white",    AAS+30, { AAS+5,  AAS+1 },         20  },
    { "life",     AAS+31, { AAS+3,  AAS+1 },         10  },
    { "thought",  AAS+32, { AAS+2,  AAS+1 },         10  },
    { "clear",    AAS+33, { AAS+3,  AAS+0 },         10  },
    { "smelly",   AAS+34, { AAS+0 },                 1  },
    { "antidote", AAS+35, { AAS+1 },                 1  },
    { NULL, 0, {}, 0 }
};

/* returns an obj of vnum that ch carries */
OBJ_DATA* obj_on_char( CHAR_DATA *ch, int vnum )
{
    OBJ_DATA *obj;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
	if ( obj->pIndexData->vnum == vnum && obj->wear_loc == WEAR_NONE )
	    return obj;

    return NULL;
}

DEF_DO_FUN(do_brew)
{
    int i, j, skill, recipe;
    char buf[MSL];
    OBJ_INDEX_DATA *herb;
    OBJ_DATA *potion;

    if ( argument[0] == '\0' )
    {
	send_to_char( "The following recipies exist:\n\r", ch );
	for ( i = 0; recipe_table[i].name != NULL; i++ )
	{
	    /*
	    if ( ch->level < recipe_table[i].level )
		continue;
	    */
	    if ( (herb = get_obj_index(recipe_table[i].herb_vnum[0])) == NULL )
		continue;

	    sprintf( buf, "lvl %2d %10s: %s", recipe_table[i].level,
		     recipe_table[i].name, herb->short_descr );
	    send_to_char( buf, ch );

	    for ( j = 1; j < MAX_HERB; j++ )
		if ( recipe_table[i].herb_vnum[j] != 0 )
		{
		    if ( (herb = get_obj_index(recipe_table[i].herb_vnum[j])) == NULL )
			continue;
		    sprintf( buf, " + %s", herb->short_descr );
		    send_to_char( buf, ch );
		}

	    send_to_char( "\n\r", ch );
	}
	return;
    }


    if ( (skill = get_skill(ch, gsn_alchemy)) == 0 )
    {
	send_to_char( "You should study some alchemy before trying this.\n\r", ch );
	return;
    }

    /* check which recipe is brewed */
    recipe = -1;
    for ( i = 0; recipe_table[i].name != NULL; i++ )
	if ( !str_cmp(recipe_table[i].name, argument) )
	{
	    recipe = i;
	    break;
	}

    if ( recipe == -1 )
    {
	send_to_char( "There is no such recipe.\n\r", ch );
	return;
    }

    if ( ch->level < recipe_table[recipe].level )
    {
	send_to_char( "You don't know that recipe.\n\r", ch );
	return;
    }

    /* check if herbs are there */
    for ( j = 0; j < MAX_HERB; j++ )
	if ( recipe_table[recipe].herb_vnum[j] > 0
	     && obj_on_char(ch,  recipe_table[recipe].herb_vnum[j]) == NULL )
	{
	    send_to_char( "You lack the required herbs.\n\r", ch );
	    return;
	}

    /* make sure potion exists */
    if ( get_obj_index(recipe_table[recipe].potion_vnum) == NULL )
    {
	send_to_char( "Sorry, not imped yet.\n\r", ch );
	return;
    }

    /* extract herbs */
    for ( j = 0; j < MAX_HERB; j++ )
	if ( recipe_table[recipe].herb_vnum[j] > 0 )
	    extract_obj( obj_on_char(ch,  recipe_table[recipe].herb_vnum[j]) );
     
    /* now lets see if it works */
    WAIT_STATE( ch, skill_table[gsn_alchemy].beats );
    if ( chance(skill) )
    {
	potion = create_object( get_obj_index(recipe_table[recipe].potion_vnum), 0 );
	/* better safe than sorry */
	if ( potion == NULL )
	    return;
        // adjust spell level due to mastery
        potion->value[0] += mastery_bonus(ch, gsn_alchemy, 8, 10);
	obj_to_char( potion, ch );
	act( "You brew $p.", ch, potion, NULL, TO_CHAR );
	act( "$n brews $p.", ch, potion, NULL, TO_ROOM );
	check_improve( ch, gsn_alchemy, TRUE, 1 );
    }
    else
    {
	//send_to_char( "Hmm.. seems something went wrong.\n\r", ch );
       /*Lots of stuff can go wrong with failed potions...*/

       switch (number_range(1,8))
       {
           case 1:
               act("Hair begins to grow from your eyeballs...guess the potion backfired.",ch,NULL,NULL,TO_CHAR);
               act("Hair begins to grow from $n's eyeballs...guess the potion backfired.",ch,NULL,NULL,TO_ROOM);
               break;
           case 2:
               act("{RBACKFIRE!!!!{x No, really, your back is on {RFIRE{x...guess that potion fizzled.",ch,NULL,NULL,TO_CHAR);
               act("{RBACKFIRE!!!!{x No, really, $n's back is on {RFIRE{x...guess that potion fizzled.",ch,NULL,NULL,TO_ROOM);
               break;
           case 3:
               act("You drop a potion on your foot and scream as it singes your toes!",ch,NULL,NULL,TO_CHAR);
               act("$n drops a potion on $s foot and screams as it singes $s toes.",ch,NULL,NULL,TO_ROOM);
               break;
           case 4:
               act("You blink in bewilderment as a potion explodes and envelopes your face in black smoke.",ch, NULL, NULL, TO_CHAR);
               act("$n blinks in bewilderment as a potion explodes and envelopes $s face in black smoke.",ch,NULL,NULL,TO_ROOM);
               spell_blindness(gsn_blindness, 8, ch, (void *) ch, TARGET_CHAR, FALSE);
               break;
           case 5:
               i = find_spell(ch,"change sex");
               if ( is_affected( ch, i ))
               {
                    send_to_char("You've already been changed.\n\r",ch);
                   return;
               }
               AFFECT_DATA af;
               af.where     = TO_AFFECTS;
               af.type      = i;
               af.level     = 50;
               af.duration  = 2;
               af.location  = APPLY_SEX;
               do
               {
                   af.modifier  = number_range( 0, 2 ) - ch->sex;
               }
               while ( af.modifier == 0 );
               af.bitvector = 0;
               /* affect_to_char( victim, &af ); moved */
                       //send_to_char( "You feel different.\n\r", victim );
               //act("$n doesn't look like $mself anymore...",victim,NULL,NULL,TO_ROOM);
               affect_to_char( ch, &af ); /* moved */



               act("Your plans for the evening have changed as has your sex. More alraune maybe?", ch, NULL, NULL, TO_CHAR);
//             spell_change_sex( 82, 50, ch, *(ch), TAR_CHAR_NEUTRAL);

               act("$n's plans for the night have changed along with $s sex. More alraune maybe?", ch, NULL, NULL, TO_ROOM);

               break;

           case 6:
               act("POOF! Your potion disappears in a flash of light, leaving a {Rrose{x in its place!",ch,NULL,NULL, TO_CHAR);
               act("POOF! $n's potion disappears in a flash of light, leaving a {Rrose{x in its place!", ch, NULL, NULL, TO_ROOM);
               spell_create_rose( 0, 50, ch, NULL, 0, FALSE);
               break;

           case 7:
               act("This one doesn't taste quite right...Improvising with guano wasn't your best idea.",ch,NULL,NULL, TO_CHAR);
               act("$n takes a sip of $s potion and begins to vomit...Improvising with guano wasn't $s best idea.",ch,NULL,NULL,TO_ROOM);
               break;
           case 8:
               act("As you awake you remember that \"Shaken, not stirred\" applies to Martinis only.",ch,NULL,NULL,TO_CHAR);
               act("As $n awakes next to $s failed potion mutterng something about \"shaken not stirred.\"",ch,NULL,NULL,TO_ROOM);




       }
	check_improve( ch, gsn_alchemy, FALSE, 1 );
	return;
    }
}


