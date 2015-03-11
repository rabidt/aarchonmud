/**************************************************************************
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
*   ROM 2.4 is copyright 1993-1996 Russ Taylor             *
*   ROM has been brought to you by the ROM consortium          *
*       Russ Taylor (rtaylor@efn.org)                  *
*       Gabrielle Taylor                           *
*       Brian Moore (zump@rom.org)                     *
*   By using this code, you have agreed to follow the terms of the     *
*   ROM license, in the file Rom24/doc/rom.license             *
***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lua_scripting.h"
#include "lookup.h"
#include "simsave.h"

/* command procedures needed */
DECLARE_DO_FUN(do_return    );

/*
 * Local functions.
 */
void    affect_modify   args( ( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd ) );
ROOM_INDEX_DATA* get_room_in_range( int min_vnum, int max_vnum, const char *argument, bool exact );
bool check_see_target( CHAR_DATA *ch, CHAR_DATA *victim );
bool check_see_new( CHAR_DATA *ch, CHAR_DATA *victim, bool combat );
OBJ_DATA *get_obj_new( CHAR_DATA *ch, const char *argument, bool area, bool exact );
OBJ_DATA *get_obj_here_new( CHAR_DATA *ch, const char *argument, bool exact );
OBJ_DATA *get_obj_wear_new( CHAR_DATA *ch, const char *arg, int *number, bool exact );
OBJ_DATA *get_obj_carry_new( CHAR_DATA *ch, const char *arg, CHAR_DATA *viewer, int *number, bool exact );
OBJ_DATA *get_obj_list_new( CHAR_DATA *ch, const char *arg, OBJ_DATA *list, int *number, bool exact );
CHAR_DATA *get_char_new( CHAR_DATA *ch, const char *argument, bool area, bool exact );
CHAR_DATA *get_char_room_new( CHAR_DATA *ch, const char *argument, bool exact );
OBJ_DATA *get_obj_list_new( CHAR_DATA *ch, const char *arg, OBJ_DATA *list, int *number, bool exact );
CHAR_DATA *get_char_group_new( CHAR_DATA *ch, const char *argument, bool exact );

/* friend stuff -- for NPC's mostly */
bool is_friend(CHAR_DATA *ch,CHAR_DATA *victim)
{
    if (is_same_group(ch,victim))
        return TRUE;
    
    if (!IS_NPC(ch))
        return FALSE;
    
    if (!IS_NPC(victim))
    {
        if (IS_SET(ch->off_flags,ASSIST_PLAYERS))
            return TRUE;
        else
            return FALSE;
    }
    
    if (IS_AFFECTED(ch,AFF_CHARM))
        return FALSE;
    
    if (IS_SET(ch->off_flags,ASSIST_ALL))
        return TRUE;
    
    if (ch->group && ch->group == victim->group)
        return TRUE;
    
    if (IS_SET(ch->off_flags,ASSIST_VNUM)
        &&  ch->pIndexData == victim->pIndexData)
        return TRUE;
    
    if (IS_SET(ch->off_flags,ASSIST_RACE) && ch->race == victim->race)
        return TRUE;
    
    if (IS_SET(ch->off_flags,ASSIST_ALIGN)
        &&  !IS_SET(ch->act,ACT_NOALIGN) && !IS_SET(victim->act,ACT_NOALIGN)
        &&  ((IS_GOOD(ch) && IS_GOOD(victim))
        ||   (IS_EVIL(ch) && IS_EVIL(victim))
        ||   (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
        return TRUE;
    
    return FALSE;
}

/* returns number of people on an object */
int count_users(OBJ_DATA *obj)
{
    CHAR_DATA *fch;
    int count = 0;
    
    if (obj->in_room == NULL)
        return 0;
    
    for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
        if (fch->on == obj)
            count++;
        
        return count;
}
	
/* returns material number */
int material_lookup (const char *name)
{
	return 0;
}


int weapon_lookup (const char *name)
{
    int type;
    
    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
            &&  !str_prefix(name,weapon_table[type].name))
            return type;
    }
    
    return -1;
}

int weapon_type (const char *name)
{
    int type;
    
    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
            &&  !str_prefix(name,weapon_table[type].name))
            return weapon_table[type].type;
    }
    
    return WEAPON_EXOTIC;
}

const char *item_name(int item_type)
{
    int type;
    
    for (type = 0; item_table[type].name != NULL; type++)
        if (item_type == item_table[type].type)
            return item_table[type].name;
        return "none";
}

const char *weapon_name( int weapon_type)
{
    int type;
    
    for (type = 0; weapon_table[type].name != NULL; type++)
        if (weapon_type == weapon_table[type].type)
            return weapon_table[type].name;
        return "exotic";
}

int attack_exact_lookup  (const char *name)
{
    int att;
    
    for ( att = 0; attack_table[att].name != NULL; att++)
    {
        if (!strcmp(name, attack_table[att].name))
            return att;
    }
    
    return 0;
}

int attack_lookup  (const char *name)
{
    int att;
    
    for ( att = 0; attack_table[att].name != NULL; att++)
    {
        if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
            &&  !str_prefix(name,attack_table[att].name))
            return att;
    }
    
    return 0;
}

/* returns a flag for wiznet */
long wiznet_lookup (const char *name)
{
    int flag;
    
    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
    {
        if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
            && !str_prefix(name,wiznet_table[flag].name))
            return flag;
    }
    
    return -1;
}

/* returns class number */
int class_lookup (const char *name)
{
    int class;
    
    for ( class = 0; class < MAX_CLASS; class++)
    {
        if (LOWER(name[0]) == LOWER(class_table[class].name[0])
            &&  !str_prefix( name,class_table[class].name))
            return class;
    }
    
    return -1;
}

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */

int check_immune(CHAR_DATA *ch, int dam_type)
{
    int immune, def, bit, found;
    
    def = IS_NORMAL;
    
    if (dam_type == DAM_NONE)
        return IS_NORMAL;
    
    if (dam_type <= 3)
    {
        if (IS_SET(ch->imm_flags,IMM_WEAPON))
            def = IS_IMMUNE;
        else if (IS_SET(ch->res_flags,RES_WEAPON))
            def = IS_RESISTANT;
        else if (IS_SET(ch->vuln_flags,VULN_WEAPON))
            def = IS_VULNERABLE;
    }
    else /* magical attack */
    {
        if (IS_SET(ch->imm_flags,IMM_MAGIC))
            def = IS_IMMUNE;
        else if (IS_SET(ch->res_flags,RES_MAGIC))
            def = IS_RESISTANT;
        else if (IS_SET(ch->vuln_flags,VULN_MAGIC))
            def = IS_VULNERABLE;
    }
    
    /* set bits to check -- VULN etc. must ALL be the same or this will fail */
    switch (dam_type)
    {
    case(DAM_BASH):     bit = IMM_BASH;     break;
    case(DAM_PIERCE):   bit = IMM_PIERCE;   break;
    case(DAM_SLASH):    bit = IMM_SLASH;    break;
    case(DAM_FIRE):     bit = IMM_FIRE;     break;
    case(DAM_COLD):     bit = IMM_COLD;     break;
    case(DAM_LIGHTNING):    bit = IMM_LIGHTNING;    break;
    case(DAM_ACID):     bit = IMM_ACID;     break;
    case(DAM_POISON):   bit = IMM_POISON;   break;
    case(DAM_NEGATIVE): bit = IMM_NEGATIVE; break;
    case(DAM_HOLY):     bit = IMM_HOLY;     break;
    case(DAM_ENERGY):   bit = IMM_ENERGY;   break;
    case(DAM_MENTAL):   bit = IMM_MENTAL;   break;
    case(DAM_DISEASE):  bit = IMM_DISEASE;  break;
    case(DAM_DROWNING): bit = IMM_DROWNING; break;
    case(DAM_LIGHT):    bit = IMM_LIGHT;    break;
    case(DAM_CHARM):    bit = IMM_CHARM;    break;
    case(DAM_SOUND):    bit = IMM_SOUND;    break;
    default:        return def;
    }
    
    immune = IS_NORMAL;
    found = 0;

    if (IS_SET(ch->imm_flags,bit))
	{
        immune += 2;
	found = 1;
	}

    if (IS_SET(ch->res_flags,bit))
	{
        immune += 1;
	found = 1;
	}

    if (IS_SET(ch->vuln_flags,bit))
	{
	    immune -= 1;
	    found = 1;
	}

    if (!found)
	immune = def;
    if (immune > IS_IMMUNE)
	immune = IS_IMMUNE;
    
    return immune;
}

bool is_clan(CHAR_DATA *ch)
{
    return (bool)(ch->clan);
}


bool is_same_clan(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (clan_table[ch->clan].active)
        return (ch->clan == victim->clan);
    else
        return FALSE;
}


/* for returning weapon information of the primary weapon */
int get_weapon_sn(CHAR_DATA *ch)
{
  return get_weapon_sn_new(ch, FALSE);
}


/* for returning weapon information */
int get_weapon_sn_new(CHAR_DATA *ch, bool secondary)
{
    OBJ_DATA *wield;
    int sn;
    
    if (secondary)
      wield = get_eq_char( ch, WEAR_SECONDARY );
    else
      wield = get_eq_char( ch, WEAR_WIELD );

    if (wield == NULL || wield->item_type != ITEM_WEAPON)
        sn = gsn_hand_to_hand;
    else switch (wield->value[0])
    {
        default :               sn = -1;                break;
        case(WEAPON_SWORD):     sn = gsn_sword;         break;
        case(WEAPON_DAGGER):    sn = gsn_dagger;        break;
        case(WEAPON_SPEAR):     sn = gsn_spear;         break;
        case(WEAPON_MACE):      sn = gsn_mace;          break;
        case(WEAPON_AXE):       sn = gsn_axe;           break;
        case(WEAPON_FLAIL):     sn = gsn_flail;         break;
        case(WEAPON_WHIP):      sn = gsn_whip;          break;
        case(WEAPON_POLEARM):   sn = gsn_polearm;       break;
        case(WEAPON_GUN):       sn = gsn_gun;           break;
        case(WEAPON_BOW):       sn = gsn_bow;           break;
    }
    return sn;
}

int get_base_sex(CHAR_DATA *ch)
{
    if (IS_NPC(ch))
        return SEX_NEUTRAL;
    
    // male/female only races force base gender
    if (pc_race_table[ch->race].gender != SEX_BOTH)
        return pc_race_table[ch->race].gender;
    
    return ch->pcdata->true_sex;
}

void add_apply(CHAR_DATA *ch, int mod, int location)
{
    int i;

    switch (location)
    {
        case APPLY_STR:     ch->mod_stat[STAT_STR]  += mod; break;
        case APPLY_DEX:     ch->mod_stat[STAT_DEX]  += mod; break;
        case APPLY_INT:     ch->mod_stat[STAT_INT]  += mod; break;
        case APPLY_WIS:     ch->mod_stat[STAT_WIS]  += mod; break;
        case APPLY_CON:     ch->mod_stat[STAT_CON]  += mod; break;
        case APPLY_VIT:     ch->mod_stat[STAT_VIT]  += mod; break;
        case APPLY_AGI:     ch->mod_stat[STAT_AGI]  += mod; break;
        case APPLY_DIS:     ch->mod_stat[STAT_DIS]  += mod; break;
        case APPLY_CHA:     ch->mod_stat[STAT_CHA]  += mod; break;
        case APPLY_LUC:     ch->mod_stat[STAT_LUC]  += mod; break;
        case APPLY_STATS:
            for ( i = 0; i < MAX_STATS; i++)
                ch->mod_stat[i] += mod;
            break;
        case APPLY_SKILLS:  ch->mod_skills  += mod; break;
        case APPLY_LEVEL:   ch->mod_level   += mod; break;
            
        case APPLY_SEX:     ch->sex         += mod; break;
        case APPLY_MANA:    ch->max_mana    += mod; break;
        case APPLY_HIT:     ch->max_hit     += mod; break;
        case APPLY_MOVE:    ch->max_move    += mod; break;
            
        case APPLY_AC:
            ch->armor += mod;
            break;
        case APPLY_HITROLL: ch->hitroll     += mod; break;
        case APPLY_DAMROLL: ch->damroll     += mod; break;
            
        case APPLY_SAVES:           ch->saving_throw += mod; break;
        case APPLY_SAVING_ROD:      ch->saving_throw += mod; break;
        case APPLY_SAVING_PETRI:    ch->saving_throw += mod; break;
        case APPLY_SAVING_BREATH:   ch->saving_throw += mod; break;
        case APPLY_SAVING_SPELL:    ch->saving_throw += mod; break;
        
        default: break;
    }
}

/* used to de-screw characters */
void reset_char(CHAR_DATA *ch)
{
    int loc, stat;
    OBJ_DATA *obj;
    AFFECT_DATA *af;
    
    if (IS_NPC(ch))
        return;
    
    // reset sex
    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
        ch->pcdata->true_sex = 0;
    ch->sex = get_base_sex(ch);

    // reset stats
    for (stat = 0; stat < MAX_STATS; stat++)
        ch->mod_stat[stat] = 0;
    
    ch->mod_skills = 0;
    ch->mod_level = 0;
    ch->max_hit = ch->pcdata->perm_hit = ch->pcdata->trained_hit_bonus = 0;
    ch->max_mana = ch->pcdata->perm_mana = ch->pcdata->trained_mana_bonus = 0;
    ch->max_move = ch->pcdata->perm_move = ch->pcdata->trained_move_bonus = 0;
    
    ch->armor       = 100;
    ch->heavy_armor = 0;
    
    ch->hitroll     = 0;
    ch->damroll     = 0;
    ch->saving_throw    = 0;
    
    /* now start adding back the effects */
    tattoo_modify_reset(ch);
    for (loc = 0; loc < MAX_WEAR; loc++)
    {
        obj = get_eq_char(ch,loc);
        if (obj == NULL)
            continue;
        ch->armor -= apply_ac( obj, loc );
        ch->heavy_armor += apply_heavy_armor(obj, loc);
        
            for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
                add_apply(ch, af->modifier, af->location);
            
            for ( af = obj->affected; af != NULL; af = af->next )
                add_apply(ch, af->modifier, af->location);
    }
    
    /* now add back spell effects */
    for (af = ch->affected; af != NULL; af = af->next)
        add_apply(ch, af->modifier, af->location);
    
    /* make sure sex is RIGHT!!!! */
    if (ch->sex < 0 || ch->sex > 2)
        ch->sex = get_base_sex(ch);
    
    update_perm_hp_mana_move(ch);
    
    // adjust XP to fit within current level range (needed e.g. when racial ETL is adjusted)
    int epl = exp_per_level(ch);
    int min_exp = epl * (ch->level);
    int max_exp = epl * (ch->level + 1) - 1;
    if (ch->exp < min_exp || ch->exp > max_exp)
    {
        int new_exp = URANGE(min_exp, ch->exp, max_exp);
        logpf("Resetting %s's experience from %d to %d (level %d).", ch->name, ch->exp, new_exp, ch->level);
        ch->exp = new_exp;
    }
    
    // wimpy & calm percentages
    ch->wimpy = URANGE(0, ch->wimpy, 100);
    ch->calm = URANGE(0, ch->calm, 100);
    
    // we have heroes turning up with positive hunger/thirst, somehow
    if ( IS_HERO(ch) )
    {
        ch->pcdata->condition[COND_THIRST] = -1;
        ch->pcdata->condition[COND_HUNGER] = -1;
    }
    
    // ensure dragonborn have a bloodline
    if ( ch->race == race_dragonborn && ch->pcdata->morph_race == 0 )
    {
        int morph_race = number_range(MORPH_DRAGON_RED, MORPH_DRAGON_WHITE);
        logpf("resetting morph race for %s to %s", ch->name, morph_race_table[morph_race].name);
        ch->pcdata->morph_race = morph_race;
        ch->pcdata->morph_time = -1;
        morph_update(ch);
    }
}


/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust( CHAR_DATA *ch )
{
    if ( ch->desc != NULL && ch->desc->original != NULL )
        ch = ch->desc->original;
    
    if (ch->trust)
        return ch->trust;
    
    if ( IS_NPC(ch) && ch->level >= LEVEL_HERO )
        return LEVEL_HERO - 1;

    // remorted characters have at least trust equal to highest level they reached previously
    if ( !IS_NPC(ch) && ch->pcdata->remorts > 0 )
        return UMAX(ch->level, LEVEL_HERO - 11 + ch->pcdata->remorts);
    
    return ch->level;
}


/*
 * Retrieve a character's age.
 */
int get_age( CHAR_DATA *ch )
{
    return 17 + ( ch->played + (int) (current_time - ch->logon) ) / 72000;
}

/* command for retrieving stats */
	
/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n( CHAR_DATA *ch )
{
	if ( IS_IMMORTAL(ch) )
	    return 1000;

    /* Added a base value of 5 to the number of items that can be carried - Astark 12-27-12 */
	return MAX_WEAR + ch->level + 5;
}



/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w( CHAR_DATA *ch )
{
    if ( !IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL )
        return 10000000;
    
    /* Added a base value of 100 to the maximum weight that can be carried. Currently 
       low strength characters are at a severe disadvantage - Astark 12-27-12  */

    return ch_str_carry(ch) * 10 + ch->level * 25 + 100;
}



/*
 * See if a string is one of the names of an object.
 */

bool is_name ( const char *str, const char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    const char *list, *string;
    
    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
        return FALSE;
    
    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
	return TRUE;
    /* Changed from FALSE 1/10/98 by Rimbol
       to support "1.", "2." etc. addressing */
    
    string = str;
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
        str = one_argument(str,part);
        
        if (part[0] == '\0' )
            return TRUE;
        
        /* check to see if this is part of namelist */
        list = namelist;
        for ( ; ; )  /* start parsing namelist */
        {
            list = one_argument(list,name);
            if (name[0] == '\0')  /* this name was not found */
                return FALSE;
            
            if (!str_prefix(string,name))
                return TRUE; /* full pattern match */
            
            if (!str_prefix(part,name))
                break;
        }
    }
}

bool is_exact_name( const char *str, const char *namelist )
{
    char name[MAX_INPUT_LENGTH];
    
    if (namelist == NULL)
        return FALSE;
    
    for ( ; ; )
    {
        namelist = one_argument( namelist, name );
        if ( name[0] == '\0' )
            return FALSE;
        if ( !str_cmp( str, name ) )
            return TRUE;
    }
}

bool is_either_name( const char *str, const char *namelist, bool exact )
{
    if ( exact )
	return is_exact_name( str, namelist );
    else
	return is_name( str, namelist );
}

bool is_mimic( CHAR_DATA *ch )
{
    return is_affected( ch, gsn_mimic )
	|| is_affected( ch, gsn_disguise );
}

MOB_INDEX_DATA* get_mimic( CHAR_DATA *ch )
{
    AFFECT_DATA *aff = affect_find( ch->affected, gsn_mimic );
    
    if ( aff == NULL )
	aff = affect_find( ch->affected, gsn_disguise );

    if ( aff == NULL )
    {
	bugf( "get_mimic: aff NULL" );
	return NULL;
    }

    return get_mob_index(aff->bitvector);
}

bool is_ch_name( char *str, CHAR_DATA *ch, bool exact, CHAR_DATA *viewer )
{
    MOB_INDEX_DATA *mimic;

    if ( is_either_name(str, ch->name, exact) )
	return TRUE;

    if ( !IS_NPC(viewer) && ch != viewer && is_mimic(ch) )
    {
	if ( (mimic = get_mimic(ch)) != NULL )
	     return is_either_name( str, mimic->player_name, exact );
    }
    return FALSE;
}

const char* get_mimic_PERS( CHAR_DATA *ch, CHAR_DATA *looker )
{
    return get_mimic_PERS_new( ch, looker, 0);
}

const char* get_mimic_PERS_new( CHAR_DATA *ch, CHAR_DATA *looker, long gagtype)
{
    if ( !can_see(looker, ch) )
	return "someone";
    
    if ( IS_NPC(ch) )
	return ch->short_descr;

    if ( is_mimic(ch) )
    {
	MOB_INDEX_DATA *mimic = get_mimic( ch );

	if ( mimic != NULL )
	{
	    if( PLR_ACT(looker, PLR_HOLYLIGHT) )
	    {
		/* static since we're returning it*/
	        static char buf[MAX_STRING_LENGTH];

	        sprintf( buf, "(%s) %s", ch->name, mimic->short_descr );
		return buf;
	    }
	    else
	        return mimic->short_descr;
	}
    }
    /* static since we're returning it*/
    static char buf[MAX_STRING_LENGTH];
    //sprintf( buf, "%s%s%s{x", ch->pcdata->name_color, ch->pcdata->pre_title, ch->name);
    sprintf (buf, "%s%s%s{x", (gagtype==GAG_NCOL_CHAN)?"":ch->pcdata->name_color, ch->pcdata->pre_title, ch->name);
    return buf;
}


/* enchanted stuff for eq */
/*
void affect_enchant(OBJ_DATA *obj)
{
    // okay, move all the old flags into new vectors if we have to 
    if (!obj->enchanted)
    {
        AFFECT_DATA *paf, *af_new;
        obj->enchanted = TRUE;
        
        for (paf = obj->pIndexData->affected;
        paf != NULL; paf = paf->next)
        {
            af_new = new_affect();
            
            af_new->next = obj->affected;
            obj->affected = af_new;
            
            af_new->where   = paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
        }
    }
}
*/		

/*
 * Apply or remove an affect to a character.
 */
void affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd )
{
    affect_modify_new( ch, paf, fAdd, TRUE );
}

void affect_modify_new( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd, bool drop )
{
    int mod;
    
    mod = paf->modifier;

    if ( paf->bitvector < 0 )
    {
	bug( "affect_modify: invalid bitvector (%d)", paf->bitvector );
	return;
    }

    if ( fAdd )
    {
	if ( paf->bitvector > 0 )
        switch (paf->where)
        {
        case TO_AFFECTS:
            SET_BIT(ch->affect_field, paf->bitvector);
            break;
        case TO_IMMUNE:
            SET_BIT(ch->imm_flags,paf->bitvector);
            break;
        case TO_RESIST:
            SET_BIT(ch->res_flags,paf->bitvector);
            break;
        case TO_VULN:
            SET_BIT(ch->vuln_flags,paf->bitvector);
            break;
        }
    }
    else
    {
	if ( paf->bitvector > 0 )
        switch (paf->where)
        {
        case TO_AFFECTS:
            REMOVE_BIT(ch->affect_field, paf->bitvector);
            break;
        case TO_IMMUNE:
            REMOVE_BIT(ch->imm_flags,paf->bitvector);
            break;
        case TO_RESIST:
            REMOVE_BIT(ch->res_flags,paf->bitvector);
            break;
        case TO_VULN:
            REMOVE_BIT(ch->vuln_flags,paf->bitvector);
            break;
        }
        mod = 0 - mod;
    }
    
    add_apply(ch, mod, paf->location);

    if ( drop )
	check_drop_weapon( ch );

    /* permanent hp/mana/move might need an update */
    update_perm_hp_mana_move(ch);    

    return;
}

void check_drop_weapon( CHAR_DATA *ch )
{
    static int depth = 0;
    OBJ_DATA *wield;

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if ( !IS_NPC(ch) 
	 && ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL
	 && get_obj_weight(wield) > ch_str_wield(ch)
	 && !IS_OBJ_STAT(wield,ITEM_NOREMOVE) )
    {
        
        if ( depth == 0 )
        {
            depth++;
            act( "You drop $p.", ch, wield, NULL, TO_CHAR );
            act( "$n drops $p.", ch, wield, NULL, TO_ROOM );

	    obj_from_char( wield );

	    /* always give to char - to room can allow duping by quitting
	    if (IS_OBJ_STAT(wield, ITEM_NODROP) || IS_OBJ_STAT(wield, ITEM_STICKY))
		obj_to_char(wield, ch);
	    else
		obj_to_room(wield, ch->in_room);
	    */
	    obj_to_char(wield, ch);

            depth--;
        }
    }
}

/* find an affect in an affect list */
AFFECT_DATA *affect_find(AFFECT_DATA *paf, int sn)
{
    AFFECT_DATA *paf_find;
    
    for ( paf_find = paf; paf_find != NULL; paf_find = paf_find->next )
    {
        if ( paf_find->type == sn )
            return paf_find;
    }
    
    return NULL;
}

/* find an affect with flag in an affect list */
AFFECT_DATA *affect_find_flag(AFFECT_DATA *paf, int flag)
{
    AFFECT_DATA *paf_find;
    
    for ( paf_find = paf; paf_find != NULL; paf_find = paf_find->next )
    {
        if ( paf_find->where == TO_AFFECTS
	     && paf_find->bitvector == flag )
            return paf_find;
    }
    
    return NULL;
}

/* find an affect with fixed location and duration in an affect list */
AFFECT_DATA* affect_find_location(AFFECT_DATA *paf, int type, int location, int duration)
{
    AFFECT_DATA *paf_find;
    
    for ( paf_find = paf; paf_find != NULL; paf_find = paf_find->next )
    {
        if ( paf_find->type == type && paf_find->location == location
            && (paf_find->duration == duration || UMIN(paf_find->duration, duration) >= 0) )
            return paf_find;
    }
    
    return NULL;
}

/* return level of affect sn on ch or -1 if none found */
int affect_level( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *aff;

    if ( ch == NULL )
	return -1;

    aff = affect_find( ch->affected, sn );
    
    if ( aff == NULL )
	return -1;
    else
	return aff->level;
}

/* fix object affects when removing one */
void affect_check(CHAR_DATA *ch,int where,int vector)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    
    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0)
        return;

	if (vector>0)
	{
	    struct race_type *race = get_morph_race_type( ch );

	    switch (where)
	    {
	    case TO_AFFECTS:
		if ( IS_SET(race->affect_field, vector) )
		{
		    SET_BIT(ch->affect_field, vector);
		    return;
		}
		break;
	    case TO_IMMUNE:
		if ( IS_SET(race->imm, vector) )
		{
		    SET_BIT(ch->imm_flags, vector);
		    return;
		}
		break;
	    case TO_RESIST:
		if ( IS_SET(race->res, vector) )
		{
		    SET_BIT(ch->res_flags, vector);
		    return;
		}
		break;
	    case TO_VULN:
		if ( IS_SET(race->vuln, vector) )
		{
		    SET_BIT(ch->vuln_flags, vector);
		    return;
		}
		break;
	    }
	}
    
    for (paf = ch->affected; paf != NULL; paf = paf->next)
        if (paf->where == where && paf->bitvector == vector)
        {
	    set_affect_flag( ch, paf );
            return;
        }
        
        for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
        {
            if (obj->wear_loc == -1)
                continue;
            
            for (paf = obj->affected; paf != NULL; paf = paf->next)
                if (paf->where == where && paf->bitvector == vector)
                {
		    set_affect_flag( ch, paf );
                    return;
                }
                
	    for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		if (paf->where == where && paf->bitvector == vector)
                {
		    set_affect_flag( ch, paf );
		    return;
		}
        }
}

/*
 * Give an affect to a char.
 */

void affect_to_char_tagsafe( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_new = new_affect();
    *paf_new = *paf;

    ch->affected = affect_insert(ch->affected, paf_new);

    affect_modify( ch, paf_new, TRUE );
    return;
}

void affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    paf->tag=NULL;
    affect_to_char_tagsafe( ch, paf );
    return;
}

/* give an affect to an object */
void affect_to_obj_tagsafe(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;
    
    paf_new = new_affect();
    
    *paf_new        = *paf;
    obj->affected   = affect_insert(obj->affected, paf_new);
    
    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector)
        switch (paf->where)
    {
        case TO_OBJECT:
            SET_BIT(obj->extra_flags,paf->bitvector);
            break;
        case TO_WEAPON:
            if (obj->item_type == ITEM_WEAPON)
                I_SET_BIT(obj->value[4], paf->bitvector);
            break;
    }
        
    return;
}

void affect_to_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    paf->tag=NULL;
    affect_to_obj_tagsafe( obj, paf );
    return;
}

/*
 * Remove an affect from a char.
 */
void affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    ch->affected = affect_remove_list(ch->affected, paf);
    
    affect_modify(ch, paf, FALSE);
    affect_check(ch, paf->where, paf->bitvector);
    
    free_affect(paf);
}

// temporarily disable an affect on ch
void affect_freeze( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    // remove from ch->affected
    ch->affected = affect_remove_list(ch->affected, paf);
    affect_modify(ch, paf, FALSE);
    affect_check(ch, paf->where, paf->bitvector);

    // move to ch->aff_stasis
    ch->aff_stasis = affect_insert(ch->aff_stasis, paf);
}

// reenable a previously frozen affect
void affect_unfreeze( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    // remove from ch->aff_stasis
    ch->aff_stasis = affect_remove_list(ch->aff_stasis, paf);
    
    // add to ch->affected
    ch->affected = affect_insert(ch->affected, paf);
    affect_modify(ch, paf, TRUE);
}

void affect_remove_obj( OBJ_DATA *obj, AFFECT_DATA *paf)
{
    int where, vector;
    if ( obj->affected == NULL )
    {
        bug( "Affect_remove_object: no affect.", 0 );
        return;
    }
    
    if (obj->carried_by != NULL && obj->wear_loc != -1)
        affect_modify( obj->carried_by, paf, FALSE );
    
    where = paf->where;
    vector = paf->bitvector;
    
    /* remove flags from the object if needed */
    if (paf->bitvector)
        switch( paf->where)
    {
        case TO_OBJECT:
            REMOVE_BIT(obj->extra_flags,paf->bitvector);
            break;
        case TO_WEAPON:
            if (obj->item_type == ITEM_WEAPON)
                I_REMOVE_BIT(obj->value[4],paf->bitvector);
            break;
    }
    
    if ( paf == obj->affected )
    {
        obj->affected    = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;
        
        for ( prev = obj->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
                prev->next = paf->next;
                break;
            }
        }
        
        if ( prev == NULL )
        {
            bug( "Affect_remove_object: cannot find paf.", 0 );
            return;
        }
    }
    
    free_affect(paf);
    
    if (obj->carried_by != NULL && obj->wear_loc != -1)
        affect_check(obj->carried_by,where,vector);
    return;
}

/*
 * Strip all affects of a given sn.
 */
void affect_strip_obj( OBJ_DATA *obj, int sn )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;
    
    for ( paf = obj->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        if ( paf->type == sn )
            affect_remove_obj( obj, paf );
    }
    
    return;
}

/*
 * Strip all affects of a given sn, or all if sn = 0
 */
void affect_strip( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;
    
    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        if ( !sn || paf->type == sn )
            affect_remove( ch, paf );
    }
    
    return;
}

/*
 * Freeze all affects of a given sn, or all if sn = 0
 */
void affect_freeze_sn( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;
    
    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        if ( !sn || paf->type == sn )
            affect_freeze(ch, paf);
    }
}

/*
 * Unfreeze all affects of a given sn, or all if sn = 0
 */
void affect_unfreeze_sn( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;
    
    for ( paf = ch->aff_stasis; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        if ( !sn || paf->type == sn )
            affect_unfreeze(ch, paf);
    }
}


/*
 * Strip all custom_affects of a given tag.
 */
void custom_affect_strip( CHAR_DATA *ch, const char *tag )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        if ( paf->type == gsn_custom_affect 
                && !str_cmp( tag, paf->tag ) )
        {
            affect_remove( ch, paf );
        }
    }
    
    return;
}

/*
 * Strip all offensive affects
 */
void affect_strip_offensive( CHAR_DATA *ch )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
        if ( is_offensive(paf->type) )
            affect_remove( ch, paf );
    }
}

/* strip all affects which add given affect flag
 */
void affect_strip_flag( CHAR_DATA *ch, int flag )
{
    AFFECT_DATA *paf = ch->affected;

    while ( paf != NULL )
	if ( paf->where == TO_AFFECTS && paf->bitvector == flag )
	{
	    affect_strip( ch, paf->type );
	    paf = ch->affected;
	}
	else
	    paf = paf->next;

    if ( IS_NPC(ch) )
	REMOVE_AFFECT( ch, flag );
}

/*
 * Return true if a char is affected by a spell.
 */
bool is_affected( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;
    
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->type == sn )
            return TRUE;
    }
    
    return FALSE;
}

/*
 * Add or enhance an affect.
 */
void affect_join( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_old;
    
    for ( paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next )
    {
	/* check location as well --Bobble */
        if ( paf_old->type == paf->type && paf_old->location == paf->location )
        {
            paf->level = UMAX( paf->level, paf_old->level );
            paf->duration = UMAX( paf->duration, paf_old->duration );
            paf->modifier += paf_old->modifier;
            affect_remove( ch, paf_old );
            break;
        }
    }
    
    affect_to_char( ch, paf );
    return;
}

/*
 * Add or enhance an affect.
 */
void affect_join_capped( CHAR_DATA *ch, AFFECT_DATA *paf, int cap )
{
    AFFECT_DATA *paf_old;
    
    for ( paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next )
    {
        if ( paf_old->type == paf->type && paf_old->location == paf->location )
        {
            paf->level = UMAX( paf->level, paf_old->level );
            paf->duration = UMAX( paf->duration, paf_old->duration );
            // negative cap indicates a lower bound
            if ( cap < 0 )
                paf->modifier = UMAX(UMIN(cap, paf_old->modifier), paf->modifier + paf_old->modifier);
            else
                paf->modifier = UMIN(UMAX(cap, paf_old->modifier), paf->modifier + paf_old->modifier);
            affect_remove( ch, paf_old );
            break;
        }
    }
    
    affect_to_char( ch, paf );
    return;
}

void affect_renew( CHAR_DATA *ch, int sn, int level, int duration )
{
    AFFECT_DATA *paf;
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
        if ( paf->type == sn )
        {
            paf->level = UMAX(paf->level, level);
            paf->duration = UMAX(paf->duration, duration);
        }
}

const char* affect_name( AFFECT_DATA *paf )
{
    if ( paf->type == gsn_custom_affect )
        return paf->tag;
    else
        return skill_table[paf->type].name;
}

/*
 * Return -1, 0 or 1 depending on ordering of af1, af2
 */
int aff_cmp( AFFECT_DATA *af1, AFFECT_DATA *af2 )
{
    // only use name for skill-based or custom affects
    // but not, e.g. for object affects (type = -1)
    if ( af1->type > 0 && af2->type > 0 )
    {
        int name_cmp = strcmp(affect_name(af1), affect_name(af2));
        if ( name_cmp != 0 )
            return name_cmp;
    }
#define affcmp(X) if (af1->X != af2->X) return (af1->X < af2->X) ? -1 : 1
    affcmp(type);
    affcmp(where);
    int loc1 = index_lookup( af1->location, apply_flags );
    int loc2 = index_lookup( af2->location, apply_flags );
    if (loc1 != loc2)
        return (loc1 < loc2) ? -1 : 1;
    affcmp(bitvector);
#undef affcmp
    return 0;
}

/*
 * inserts an affect into an existing affect list in fixed order
 */
AFFECT_DATA* affect_insert( AFFECT_DATA *affect_list, AFFECT_DATA *paf )
{
    if ( affect_list == NULL || aff_cmp(paf, affect_list) <= 0 )
    {
        paf->next = affect_list;
        return paf;
    }

    AFFECT_DATA *prev = affect_list;
    while ( prev->next && aff_cmp(paf, prev->next) > 0 )
        prev = prev->next;
    
    paf->next = prev->next;
    prev->next = paf;
    
    return affect_list;
}

/*
 * removes an affect from a given list, returning the new list
 */
AFFECT_DATA* affect_remove_list( AFFECT_DATA *affect_list, AFFECT_DATA *paf )
{
    
    if ( affect_list == NULL || paf == NULL )
    {
        bugf("affect_remove_list: NULL parameter");
        return affect_list;
    }
    
    if ( affect_list == paf )
        return affect_list->next;
    
    AFFECT_DATA *prev = affect_list;
    while ( prev->next && prev->next != paf )
        prev = prev->next;
    
    if ( !prev->next )
        bugf("affect_remove_list: affect not found");
    else
        prev->next = paf->next;
    
    return affect_list;
}

// Check if ch is really in some room - may not be the case right after login
bool is_in_room( CHAR_DATA *ch )
{
    CHAR_DATA *ch_in_room;

    if ( ch == NULL || ch->in_room == NULL )
        return FALSE;

    for ( ch_in_room = ch->in_room->people; ch_in_room != NULL; ch_in_room = ch_in_room->next_in_room )
        if ( ch_in_room == ch )
            return TRUE;

    return FALSE;
}

/*
 * Move a char out of a room.
 */
void char_from_room( CHAR_DATA *ch )
{
    OBJ_DATA *obj;
    
    if ( ch == NULL || ch->in_room == NULL )
    {
        bug( "Char_from_room: NULL.", 0 );
        return;
    }
    
    if ( !IS_NPC(ch) )
    {
        if( --ch->in_room->area->nplayer < 0 )
	    {
	        bug( "Area->nplayer reduced below zero by char_from_room.  Reset to zero.", 0 );
	        ch->in_room->area->nplayer = 0;
	    }
	    /*only make this check for players or we get a crash*/
    	if ( IS_SET(ch->in_room->room_flags, ROOM_BOX_ROOM) && ch->pcdata->storage_boxes>0)
        {
	    /* quit_save_char_obj will put player mf on player_quit_list and box mf on
               box_mf_list. We need to remove from player_save_list so this box mf
               and player mf are saved together. If there's an existing mf on
               player_quit_list and we put mf on box_mf_list then box will save with
               existing player mf, which is not likely to cause problems, but we
               should protect against it anyway.
               We don't want to do this in all quit cases, only when leaving box_room.*/
	        remove_from_save_list(capitalize(ch->name));
            quit_save_char_obj(ch);
	        unload_storage_boxes(ch);
            send_to_char( "As you leave the room, an employee takes your boxes back down to the basement.\n\r",ch);
        }
        if ( IS_SET(ch->in_room->room_flags, ROOM_BLACKSMITH) ) 
        {
            /* leaving a smithy, might need to cancel transaction */
            cancel_smith(ch);
        }
    }
    
    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
        &&   obj->item_type == ITEM_LIGHT
        &&   obj->value[2] != 0
        &&   ch->in_room->light > 0 )
        --ch->in_room->light;
    
    if ( IS_SET(ch->form, FORM_BRIGHT) )
	--ch->in_room->light;
    
    if ( ch == ch->in_room->people )
    {
        ch->in_room->people = ch->next_in_room;
    }
    else
    {
        CHAR_DATA *prev;
        
        for ( prev = ch->in_room->people; prev; prev = prev->next_in_room )
        {
            if ( prev->next_in_room == ch )
            {
                prev->next_in_room = ch->next_in_room;
                break;
            }
        }
        
        if ( prev == NULL )
            bugf("Char_from_room: %s not found in room %d", ch->name, ch->in_room->vnum);
    }
    
    ch->in_room      = NULL;
    ch->next_in_room = NULL;
    ch->on       = NULL;  /* sanity check! */
    return;
}



/*
 * Move a char into a room.
 */
void char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    OBJ_DATA *obj;
    
    if ( ch == NULL )
	return;

    if ( pRoomIndex == NULL )
    {
        ROOM_INDEX_DATA *room;
        
        bug( "Char_to_room: NULL.", 0 );
        
        if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
            char_to_room(ch,room);
        
        return;
    }
    
    ch->in_room     = pRoomIndex;
    ch->next_in_room    = pRoomIndex->people;
    check_explore(ch, pRoomIndex); //Explore the room
    pRoomIndex->people  = ch;

    
    if ( !IS_NPC(ch) )
    {
        if (ch->in_room->area->empty)
        {
            ch->in_room->area->empty = FALSE;
            ch->in_room->area->age = 0;
        }
        ++ch->in_room->area->nplayer;
	if ( IS_SET(ch->in_room->room_flags, ROOM_BOX_ROOM))
	  load_storage_boxes(ch);
    }
    
    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
        &&   obj->item_type == ITEM_LIGHT
        &&   obj->value[2] != 0 )
        ++ch->in_room->light;

    if ( IS_SET(ch->form, FORM_BRIGHT) )
	++ch->in_room->light;
    
    
    if (IS_AFFECTED(ch,AFF_PLAGUE))
    {
        AFFECT_DATA *af, plague;
        CHAR_DATA *vch;
        
        for ( af = ch->affected; af != NULL; af = af->next )
        {
            if (af->type == gsn_plague || (af->type == gsn_god_curse && af->bitvector == AFF_PLAGUE))
                break;
        }
        
        if (af == NULL)
        {
            REMOVE_BIT(ch->affect_field,AFF_PLAGUE);
            return;
        }
        
        if (af->level == 1)
            return;
        
        plague.where        = TO_AFFECTS;
        plague.type         = gsn_plague;
        plague.level        = af->level - 1;
        plague.duration     = number_range(1,2 * plague.level);
        plague.location     = APPLY_STR;
        plague.modifier     = -5;
        plague.bitvector    = AFF_PLAGUE;
        
        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell(vch, NULL, plague.level - 2, DAM_DISEASE)
                &&  !IS_IMMORTAL(vch) &&
                !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(6) == 0)
            {
                send_to_char("You feel hot and feverish.\n\r",vch);
                act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM);
                affect_join(vch,&plague);
            }
        }
    }
    else if (IS_AFFECTED(ch,AFF_LAUGH))
    {
        AFFECT_DATA *af, laugh;
        
        CHAR_DATA *vch = NULL;
        
        for ( af = ch->affected; af != NULL; af = af->next )
        {
            if (af->type == gsn_laughing_fit)
                break;
        }  
        
        if (af == NULL)
        { 
            REMOVE_BIT(ch->affect_field,AFF_LAUGH);
            return;
        }
        
        if (af->level <= 1)
            return;
        
        affect_strip(ch, gsn_laughing_fit);
        laugh.where        = TO_AFFECTS;
        laugh.type         = gsn_laughing_fit;
        laugh.level        = af->level;
        laugh.duration     = af->duration;
        laugh.modifier     = af->modifier;
        laugh.bitvector    = AFF_LAUGH;
        
        laugh.location     = APPLY_STR;
        affect_to_char(ch, &laugh);
        
        laugh.location     = APPLY_HITROLL;
        affect_to_char(ch, &laugh);
        
        laugh.location     = APPLY_INT;
        affect_to_char(ch, &laugh);
        
        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell(vch, NULL, laugh.level - 2, DAM_MENTAL)
                &&  !IS_IMMORTAL(vch) &&
                !IS_AFFECTED(vch,AFF_LAUGH) && number_bits(6) == 0)
            {
                send_to_char("You feel light headed and giddy.\n\r",vch);
                act("$n gets a dumb grin on $s face and starts giggling.",vch,NULL,NULL, TO_ROOM);
                
                laugh.where     = TO_AFFECTS;
                laugh.type      = gsn_laughing_fit;
                laugh.level     = af->level - 1;
                laugh.duration  = number_range(1,laugh.level);
                laugh.modifier  = -2;
                laugh.bitvector = AFF_LAUGH;
                
                laugh.location  = APPLY_STR;
                affect_to_char(vch, &laugh);
                laugh.location  = APPLY_HITROLL;
                affect_to_char(vch, &laugh);
                laugh.location  = APPLY_INT;
                affect_to_char(vch, &laugh);
            }
        }
    }
    return;
}


/*
 * Give an obj to a char.
 */
void obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch )
{
    if (obj == NULL || ch == NULL) {
	bugf("obj_to_char: NULL pointer");
	return;
    }
	
    obj->next_content = ch->carrying;
    ch->carrying     = obj;
    obj->carried_by  = ch;
    obj->in_room     = NULL;
    obj->in_obj      = NULL;
    ch->carry_number += get_obj_number( obj );
    ch->carry_weight += get_obj_weight( obj );

    /*
    if ( is_relic_obj(obj) )
    {
	act( "The burden of $p slows you down.", ch, obj, NULL, TO_CHAR );
	WAIT_STATE(ch, 6);
    }
    */
}
 
 

/*
 * Take an obj from its character.
 */
void obj_from_char( OBJ_DATA *obj )
{
    CHAR_DATA *ch;
    
    if ( ( ch = obj->carried_by ) == NULL )
    {
        bug( "Obj_from_char: null ch.", 0 );
        return;
    }
    
    if ( obj->wear_loc != WEAR_NONE )
        unequip_char( ch, obj );
    
    if ( ch->carrying == obj )
    {
        ch->carrying = obj->next_content;
    }
    else
    {
        OBJ_DATA *prev;
        
        for ( prev = ch->carrying; prev != NULL; prev = prev->next_content )
        {
            if ( prev->next_content == obj )
            {
                prev->next_content = obj->next_content;
                break;
            }
        }
        
        if ( prev == NULL )
            bug( "Obj_from_char: obj not in list.", 0 );
    }
    
    obj->carried_by  = NULL;
    obj->next_content    = NULL;
    ch->carry_number    -= get_obj_number( obj );
    ch->carry_weight    -= get_obj_weight( obj );
    return;
}



/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac( OBJ_DATA *obj, int iWear )
{
    if ( obj->item_type != ITEM_ARMOR )
        return 0;
    
    switch ( iWear )
    {
    case WEAR_TORSO:   return 3 * obj->value[0];
    case WEAR_HEAD:    return 2 * obj->value[0];
    case WEAR_LEGS:    return 2 * obj->value[0];
    case WEAR_FEET:    return     obj->value[0];
    case WEAR_HANDS:   return     obj->value[0];
    case WEAR_ARMS:    return     obj->value[0];
    case WEAR_SHIELD:  return     obj->value[0];
    case WEAR_NECK_1:  return     obj->value[0];
    case WEAR_NECK_2:  return     obj->value[0];
    case WEAR_ABOUT:   return 2 * obj->value[0];
    case WEAR_WAIST:   return     obj->value[0];
    case WEAR_WRIST_L: return     obj->value[0];
    case WEAR_WRIST_R: return     obj->value[0];
    case WEAR_HOLD:    return     obj->value[0];
    case WEAR_FINGER_L: return    obj->value[0];
    case WEAR_FINGER_R: return    obj->value[0];
    case WEAR_FLOAT:   return     obj->value[0];
    }
    
    return 0;
}

int apply_heavy_armor( OBJ_DATA *obj, int iWear )
{
    if ( !IS_OBJ_STAT(obj, ITEM_HEAVY_ARMOR) )
        return 0;
    
    switch ( iWear )
    {
        default:            return 0;
        case WEAR_TORSO:    return 3;
        case WEAR_HEAD:     return 2;
        case WEAR_LEGS:     return 2;
        case WEAR_FEET:     return 2;
        case WEAR_HANDS:    return 2;
        case WEAR_ARMS:     return 2;
        case WEAR_NECK_1:   return 1;
        case WEAR_NECK_2:   return 1;
        case WEAR_ABOUT:    return 2;
        case WEAR_WAIST:    return 2;
        case WEAR_WRIST_L:  return 2;
        case WEAR_WRIST_R:  return 2;
        case WEAR_FINGER_L: return 1;
        case WEAR_FINGER_R: return 1;
    }
}

// returns heavy armor bonus as percentage of max
int get_heavy_armor_bonus( CHAR_DATA *ch )
{
    return ch->heavy_armor * 4;
}

// returns heavy armor penalty as percentage of max
int get_heavy_armor_penalty( CHAR_DATA *ch )
{
    int skill = get_skill(ch, gsn_heavy_armor) + mastery_bonus(ch, gsn_heavy_armor, 30, 50);
    if ( IS_SET(ch->form, FORM_ARMORED) )
        skill += 30;
    return get_heavy_armor_bonus(ch) * (300 - skill) / 300;
}

/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_eq_char( CHAR_DATA *ch, int iWear )
{
    OBJ_DATA *obj;
    
    if (ch == NULL)
        return NULL;
    
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc == iWear )
            return obj;
    }
    
    return NULL;
}

bool class_can_use_obj( int class, OBJ_DATA *obj );

/*
 * Equip a char with an obj.
 */
void equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear )
{
    AFFECT_DATA *paf;
    
    if (ch == NULL || obj == NULL) {
	bugf("Equip_char: NULL pointer");
	return;
    }

	
    if ( get_eq_char( ch, iWear ) != NULL )
    {
        bugf( "Equip_char: %d wears %d at %d: already equipped",
	      IS_NPC(ch) ? ch->pIndexData->vnum : 0, obj->pIndexData->vnum, iWear );
        return;
    }
    
    if ( !IS_NPC(ch) && !class_can_use_obj(ch->class, obj) )
    {
        act( "Realizing you can't use $p, you remove it again.",
	     ch, obj, NULL, TO_CHAR );
        act( "Realizing $e can't use $p, $n removes it again.",
	     ch, obj, NULL, TO_ROOM );
	return;
    }

    if ( !IS_NPC(ch) )
    if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)        && IS_EVIL(ch)    )
        ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(ch)    )
        ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch) ) )
    {
	/*
	 * Thanks to Morgenes for the bug fix here!
	 */
        act( "You are zapped by $p and remove it.", ch, obj, NULL, TO_CHAR );
        act( "$n is zapped by $p and removes it.",  ch, obj, NULL, TO_ROOM );
	/* let's not loose items this way.. --Bobble
	obj_from_char( obj );
	obj_to_room( obj, ch->in_room );
	*/
        return;
    }

    // remove current tattoo effect
    tattoo_modify_equip( ch, iWear, FALSE, FALSE, FALSE );
    // wear item - this is picked up by tattoo_modify_equip
    obj->wear_loc = iWear;
    // add new tattoo effect
    tattoo_modify_equip( ch, iWear, TRUE, FALSE, FALSE );
    
    // add item armor / affects
    ch->armor -= apply_ac( obj, iWear );
    ch->heavy_armor += apply_heavy_armor(obj, iWear);
    
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        if ( paf->location != APPLY_SPELL_AFFECT )
            affect_modify_new( ch, paf, TRUE, FALSE );

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
        if ( paf->location == APPLY_SPELL_AFFECT )
            affect_to_char ( ch, paf );
        else
            affect_modify_new( ch, paf, TRUE, FALSE );
    
    if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room != NULL )
        ++ch->in_room->light;

    // we delayed weapon-drop check until all affects (equipment & tattoo) have been applied
    check_drop_weapon( ch );

    check_item_trap_hit(ch, obj);

    return;
}



/*
 * Unequip a char with an obj.
 */
void unequip_char( CHAR_DATA *ch, OBJ_DATA *obj )
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *lpaf = NULL;
    AFFECT_DATA *lpaf_next = NULL;
    int iWear;
    OBJ_DATA *secondary;
    
    if ((obj->wear_loc == WEAR_WIELD) &&
        ((secondary = get_eq_char(ch, WEAR_SECONDARY)) != NULL))
        secondary->wear_loc = WEAR_WIELD;
    
    if ( (iWear=obj->wear_loc) == WEAR_NONE )
    {
        bug( "Unequip_char: already unequipped.", 0 );
        return;
    }
    
    // remove current tattoo effect
    tattoo_modify_equip( ch, iWear, FALSE, FALSE, FALSE );
    // remove item - this is picked up by tattoo_modify_equip
    obj->wear_loc = WEAR_NONE;
    // add new tattoo effect
    tattoo_modify_equip( ch, iWear, TRUE, FALSE, FALSE );

    // add item armor / affects
    ch->armor += apply_ac( obj, iWear );
    ch->heavy_armor -= apply_heavy_armor(obj, iWear);
    
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        if ( paf->location == APPLY_SPELL_AFFECT )
        {
            for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
            {
                lpaf_next = lpaf->next;
                if ( (lpaf->type == paf->type) && (lpaf->level == paf->level) && (lpaf->location == APPLY_SPELL_AFFECT) )
                {
                    affect_remove( ch, lpaf );
                    lpaf_next = NULL;
                }
            }
        }
        else
        {
            affect_modify_new( ch, paf, FALSE, FALSE );
            affect_check(ch,paf->where,paf->bitvector);
        }
            
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
        if ( paf->location == APPLY_SPELL_AFFECT )
        {
            bug ( "Norm-Apply: %d", 0 );
            for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
            {
                lpaf_next = lpaf->next;
                if ( (lpaf->type == paf->type) && (lpaf->level == paf->level) && (lpaf->location == APPLY_SPELL_AFFECT))
                {
                    bug ( "location = %d", lpaf->location );
                    bug ( "type = %d", lpaf->type );
                    affect_remove( ch, lpaf );
                    lpaf_next = NULL;
                }
            }
        }
        else
        {
            affect_modify_new( ch, paf, FALSE, FALSE );
            affect_check(ch,paf->where,paf->bitvector);
        }
    
    if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room != NULL && ch->in_room->light > 0 )
        --ch->in_room->light;
    
    // we delayed weapon-drop check until all affects (equipment & tattoo) have been applied
    check_drop_weapon( ch );    
    
    return;
}



/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list( OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list )
{
    OBJ_DATA *obj;
    int nMatch;
    
    nMatch = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
        if ( obj->pIndexData == pObjIndex )
            nMatch++;
    }
    
    return nMatch;
}



/*
 * Move an obj out of a room.
 */
void obj_from_room( OBJ_DATA *obj )
{
    ROOM_INDEX_DATA *in_room;
    CHAR_DATA *ch;
    
    if ( ( in_room = obj->in_room ) == NULL )
    {
        bug( "obj_from_room: NULL.", 0 );
        return;
    }
    
    for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
        if (ch->on == obj)
            ch->on = NULL;
        
        if ( obj == in_room->contents )
        {
            in_room->contents = obj->next_content;
        }
        else
        {
            OBJ_DATA *prev;
            
            for ( prev = in_room->contents; prev; prev = prev->next_content )
            {
                if ( prev->next_content == obj )
                {
                    prev->next_content = obj->next_content;
                    break;
                }
            }
            
            if ( prev == NULL )
            {
                bug( "Obj_from_room: obj not found.", 0 );
                return;
            }
        }
        
        obj->in_room      = NULL;
        obj->next_content = NULL;
        return;
}



/*
 * Move an obj into a room.
 */
void obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex )
{
    if (pRoomIndex == NULL)
    {
        bug("OBJ_TO_ROOM: NULL pRoomIndex", 0);
        return;
    }
    
    obj->next_content       = pRoomIndex->contents;
    pRoomIndex->contents    = obj;
    obj->in_room        = pRoomIndex;
    obj->carried_by     = NULL;
    obj->in_obj         = NULL;
    if (IS_SET(pRoomIndex->room_flags, ROOM_DONATION))
        obj->cost = 0;
    
    return;
}



/*
 * Move an object into an object.
 */
void obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to )
{
    obj->next_content       = obj_to->contains;
    obj_to->contains        = obj;
    obj->in_obj         = obj_to;
    obj->in_room        = NULL;
    obj->carried_by     = NULL;
    
    for ( ; obj_to != NULL; obj_to = obj_to->in_obj )
    {
        if ( obj_to->carried_by != NULL )
        {
            obj_to->carried_by->carry_number += get_obj_number( obj );
            obj_to->carried_by->carry_weight += get_obj_weight( obj )
                * WEIGHT_MULT(obj_to) / 100;
        }
    }
    
    return;
}



/*
 * Move an object out of an object.
 */
void obj_from_obj( OBJ_DATA *obj )
{
    OBJ_DATA *obj_from;
    
    if ( ( obj_from = obj->in_obj ) == NULL )
    {
        bug( "Obj_from_obj: null obj_from.", 0 );
        return;
    }
    
    if ( obj == obj_from->contains )
    {
        obj_from->contains = obj->next_content;
    }
    else
    {
        OBJ_DATA *prev;
        
        for ( prev = obj_from->contains; prev; prev = prev->next_content )
        {
            if ( prev->next_content == obj )
            {
                prev->next_content = obj->next_content;
                break;
            }
        }
        
        if ( prev == NULL )
        {
            bug( "Obj_from_obj: obj not found.", 0 );
            return;
        }
    }
    
    obj->next_content = NULL;
    obj->in_obj       = NULL;
    
    for ( ; obj_from != NULL; obj_from = obj_from->in_obj )
    {
        if ( obj_from->carried_by != NULL )
        {
            obj_from->carried_by->carry_number -= get_obj_number( obj );
            obj_from->carried_by->carry_weight -= get_obj_weight( obj )
                * WEIGHT_MULT(obj_from) / 100;
        }
    }
    
    return;
}

void obj_from_world( OBJ_DATA *obj )
{
    if ( obj == NULL )
	return;

    if ( obj->in_room != NULL )
        obj_from_room( obj );
    else if ( obj->carried_by != NULL )
        obj_from_char( obj );
    else if ( obj->in_obj != NULL )
        obj_from_obj( obj );

}

void obj_from_object_list( OBJ_DATA *obj )
{
    if ( object_list == obj )
    {
        object_list = obj->next;
    }
    else
    {
        OBJ_DATA *prev;
        
        for ( prev = object_list; prev != NULL; prev = prev->next )
        {
            if ( prev->next == obj )
            {
                prev->next = obj->next;
                break;
            }
        }
        
        if ( prev == NULL )
        {
            bug( "obj_from_object_list: obj %d not found.", obj->pIndexData->vnum );
            return;
        }
    }
}

/*
 * Extract an obj from the world.
 */
void extract_obj( OBJ_DATA *obj )
{

    if (obj == NULL)
    {
        bug("BUG: extract_obj, obj == NULL",0);
        return;
    }

    if (g_LuaScriptInProgress || is_mprog_running())
    {
        obj->must_extract=TRUE;
        return;
    }

    /* safety-net against infinite extracting */
    obj->must_extract = FALSE;

    OBJ_DATA *obj_content;
    OBJ_DATA *obj_next;
    
    unregister_obj_timer( obj );
    obj_from_world( obj );
    free_relic( obj );
    
    for ( obj_content = obj->contains; obj_content; obj_content = obj_next )
    {
        obj_next = obj_content->next_content;
        extract_obj( obj_content );
    }

    obj_from_object_list(obj);

    --obj->pIndexData->count;
    free_obj(obj);
    return;
}

/********** extract handling **********/
/* made flexible by Bobble */

bool is_remort_obj( OBJ_DATA *obj )
{
    return IS_OBJ_STAT( obj, ITEM_REMORT )
	|| IS_SET( obj->pIndexData->area->area_flags, AREA_REMORT );
}

bool is_sticky_obj( OBJ_DATA *obj )
{
    return IS_OBJ_STAT( obj, ITEM_STICKY ) && TRUE;
}

bool is_drop_obj( OBJ_DATA *obj )
{
    return IS_OBJ_STAT( obj, ITEM_EASY_DROP )
	|| is_relic_obj( obj );
}

bool is_questeq( OBJ_DATA *obj )
{
    return IS_OBJ_STAT(obj->pIndexData, ITEM_QUESTEQ) && TRUE;
}

bool contains_obj_recursive( OBJ_DATA *obj, OBJ_CHECK_FUN *obj_check )
{
    if ( obj == NULL || obj_check == NULL )
    {
        bugf("contains_obj_recursive: NULL pointer given");
        return FALSE;
    }
    if ( obj_check(obj) )
        return TRUE;

    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        if ( contains_obj_recursive(obj, obj_check) )
            return TRUE;

    return FALSE;
}

void extract_char_eq( CHAR_DATA *ch, OBJ_CHECK_FUN *extract_it, int to_loc )
{
    OBJ_DATA *obj, *obj_next;

    if ( ch == NULL || extract_it == NULL )
    {
	bugf( "extract_char_eq: NULL pointer given" );
	return;
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next_content;
	extract_char_obj( ch, extract_it, to_loc, obj );
    }
}

void extract_char_obj( CHAR_DATA *ch, OBJ_CHECK_FUN *extract_it, int to_loc,
		       OBJ_DATA *obj )
{
    OBJ_DATA *in, *in_next;

    if ( extract_it(obj) )
    {
	obj_from_world( obj );
	switch( to_loc )
	{
	case TO_ROOM: obj_to_room( obj, ch->in_room ); break;
	case TO_CHAR: obj_to_char( obj, ch ); break;
	default: extract_obj( obj );
	}
	return;
    }

    for (in = obj->contains; in != NULL; in = in_next)
    {
	in_next = in->next_content;
	extract_char_obj( ch, extract_it, to_loc, in );
    }
} 

OBJ_DATA* get_char_obj_vnum( CHAR_DATA *ch, int vnum )
{
    OBJ_DATA * obj;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
        if ( obj->pIndexData->vnum == vnum )
            return obj;

    return NULL;
}

/********** end extract handling **********/

/* makes a char drop all eq to room */
void drop_eq( CHAR_DATA *ch )
{
    OBJ_DATA *obj, *obj_next;

    if ( ch == NULL || ch->in_room == NULL )
	return;

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next_content;
	obj_from_char( obj );
	obj_to_room( obj, ch->in_room );
    }
}

/* get eq from an object (corpse) */
void get_eq_corpse( CHAR_DATA *ch, OBJ_DATA *corpse )
{
    OBJ_DATA *obj, *obj_next;

    if ( ch == NULL || corpse == NULL )
	return;

    for ( obj = corpse->contains; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next_content;
	obj_from_obj( obj );
	obj_to_char( obj, ch );
    }
}

void char_list_insert( CHAR_DATA *ch )
{
    // insert so that char_list remains sorted (descending) by id
    //if ( !char_list || char_list->id < ch->id )
    {
        ch->next = char_list;
        char_list = ch;
        return;
    }
    /*
    // find point to insert within sorted list
    CHAR_DATA *prev = char_list;
    while ( prev->next && prev->next->id > ch->id )
        prev = prev->next;
    // insert
    ch->next = prev->next;
    prev->next = ch;
    */
}

/*
void assert_char_list()
{
    CHAR_DATA *ch;
    for ( ch = char_list; ch && ch->next; ch = ch->next )
        if ( ch->id < ch->next->id )
            bugf("assert_char_list: %d < %d\n", ch->id, ch->next->id);
}

// returns first character in char_list with id < current_id
CHAR_DATA* char_list_next( long current_id )
{
    CHAR_DATA *ch;
    for ( ch = char_list; ch; ch = ch->next )
        if ( ch->id < current_id )
            return ch;
    return NULL;
}
*/

CHAR_DATA* char_list_next_char( CHAR_DATA *ch )
{
    // safety net
    if ( ch == NULL )
        return NULL;
    
    ch = ch->next;
    // skip characters marked for extraction and look for invalid ones
    while ( ch && (!valid_CH(ch) || ch->must_extract) )
    {
        if ( !valid_CH(ch) )
        {
            bugf("char_list_next_char: invalid character");
            return NULL;
        }
        ch = ch->next;
    }
    return ch;
}

void char_from_char_list( CHAR_DATA *ch )
{
    if ( ch == char_list )
    {
        char_list = ch->next;
    }
    else
    {
        CHAR_DATA *prev;
        
        for ( prev = char_list; prev != NULL; prev = prev->next )
        {
            if ( prev->next == ch )
            {
                prev->next = ch->next;
                break;
            }
        }
        
        if ( prev == NULL )
        {
            bug( "char_from_char_list: char not found.", 0 );
            return;
        }
    }
}

CHAR_DATA* char_list_find( char *name )
{
    CHAR_DATA *ch = char_list;
    while ( ch && !is_exact_name(name, ch->name) )
        ch = ch->next;
    return ch;
}

/*
 * Extract a char from the world. Returns true if successful.
 */
bool extract_char( CHAR_DATA *ch, bool fPull )
{
    return extract_char_new(ch, fPull, TRUE);
}

bool extract_char_new( CHAR_DATA *ch, bool fPull, bool extract_objects)
{
    CHAR_DATA *wch;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    /* safety-net against infinite extracting and double-counting */
    if ( ch->must_extract )
        return FALSE;

    unregister_ch_timer( ch );

    if ( fPull )
    {
        nuke_pets(ch);
        die_follower( ch, false );
    }

    stop_fighting( ch, TRUE );

    /* drop all easy_drop items */
    extract_char_eq( ch, &is_drop_obj, TO_ROOM );

    if (extract_objects)
        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            if (!IS_NPC(ch)
                    && !fPull
                    && IS_SET(obj->extra_flags, ITEM_STICKY))
                continue;  /* Leave item on player, not on corpse. Only for PC deaths in-game. */

            extract_obj( obj );
        }

    if (ch->in_room != NULL)
        char_from_room( ch );

    /* Death room is set in the clan table now */
    if ( !fPull )
    {
        /* make sure vampires don't get toasted over and over again */
        if ( IS_SET(ch->form, FORM_SUNBURN) )
            char_to_room(ch,get_room_index(ROOM_VNUM_TEMPLE));
        else
            char_to_room(ch,get_room_index(clan_table[ch->clan].hall));
        return TRUE;
    }
    
    // mark for full extraction later on
    ch->must_extract = TRUE;

    if ( IS_NPC(ch) )
        --ch->pIndexData->count;

    if ( ch->desc != NULL && ch->desc->original != NULL )
    {
        do_return( ch, "" );
        ch->desc = NULL;
    }

    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
        if ( wch->reply == ch )
            wch->reply = NULL;
        if ( wch->mprog_target == ch )
            wch->mprog_target = NULL;
    }

    if ( ch->desc != NULL )
    {
        ch->desc->character = NULL;
    }

    return TRUE;
}

void desc_from_descriptor_list( DESCRIPTOR_DATA *desc )
{
    if ( desc == descriptor_list )
    {
        descriptor_list = descriptor_list->next;
    }
    else
    {
        DESCRIPTOR_DATA *d;

        for ( d = descriptor_list; d && d->next != desc; d = d->next )
            ;
        if ( d != NULL )
            d->next = desc->next;
        else
            bugf("desc_from_descriptor_list: descriptor not found.");
    }
}

/*
 * Find a room by name
 */

ROOM_INDEX_DATA* get_room_area( AREA_DATA *area, const char *argument )
{
    ROOM_INDEX_DATA *room;
    room = get_room_in_range( area->min_vnum, area->max_vnum, argument, TRUE );
    if ( room != NULL )
	return room;
    return get_room_in_range( area->min_vnum, area->max_vnum, argument, FALSE );
}

ROOM_INDEX_DATA* get_room_world( const char *argument )
{
    ROOM_INDEX_DATA *room;
    room = get_room_in_range( 1, top_vnum_room, argument, TRUE );
    if ( room != NULL )
	return room;
    return get_room_in_range( 1, top_vnum_room, argument, FALSE );
}

ROOM_INDEX_DATA* get_room_in_range( int min_vnum, int max_vnum, const char *argument, bool exact )
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *room;
    int vnum, number, count = 0;

    if ( argument == NULL || argument[0] == '\0' )
	return NULL;

    number = number_argument( argument, arg );

    for ( vnum = min_vnum; vnum <= max_vnum; vnum++ )
    {
	room = get_room_index( vnum );
	if ( room == NULL || !is_either_name(arg, remove_color(room->name), exact) )
	    continue;

        if ( ++count == number )
            return room;
    }
    return NULL;
}

CHAR_DATA* get_player( const char *name )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;

    /* match exact name */
    for ( d = descriptor_list; d != NULL; d = d->next )
	if ( IS_PLAYING(d->connected) )
	{
	    ch = original_char( d->character );

	    if ( ch != NULL && !strcmp(ch->name, name) )
		return ch;
	}

    /* match partial name */
    /*
    for ( d = descriptor_list; d != NULL; d = d->next )
	if ( d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected) )
	{
	    ch = original_char( d->character );

	    if ( ch != NULL && is_name(ch->name, name) )
		return ch;
	}
    */
    return NULL;
}

/*
 * Find a char in the room.
 */
CHAR_DATA *get_char_room( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA *rch;

    rch = get_char_room_new( ch, argument, TRUE );
    if ( rch == NULL )
	rch = get_char_room_new( ch, argument, FALSE );

    return rch;
}

bool check_see_target( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( IS_NPC(ch) || victim->position < POS_STANDING )
	return can_see( ch, victim );
    else
	return check_see( ch, victim )
	    || (!number_bits(2) && can_see(ch, victim));
}

CHAR_DATA *get_char_room_new( CHAR_DATA *ch, const char *argument, bool exact )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *rch;
    int number = 0;
    int count = 0;
    
    if ( argument == NULL || argument[0] == '\0' || ch->in_room == NULL )
	return NULL;

    number = number_argument( argument, arg );
    
    if ( !str_cmp( arg, "self" ) )
        return ch;
    if ( !str_cmp( arg, "opponent" ) )
	return ch->fighting;
    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( /*!can_see( ch, rch )*/ !check_see_target( ch, rch )
	     || !is_ch_name(arg, rch, exact, ch) )
            continue;

        if ( ++count == number )
            return rch;
    }
    
    return NULL;
}

/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_world( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA *wch;

    wch = get_char_new( ch, argument, FALSE, TRUE );
    if ( wch == NULL )
	wch = get_char_new( ch, argument, FALSE, FALSE );

    return wch;    
}

/*
 * Find a char in an area.
 * (from get_char_world)
 *
 * (by Mikko Kilpikoski 09-Jun-94)
 */
CHAR_DATA *get_char_area( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA *ach;

    ach = get_char_new( ch, argument, TRUE, TRUE );
    if ( ach == NULL )
	ach = get_char_new( ch, argument, TRUE, FALSE );

    return ach;
}

// allow targeting via w.name (world), a.name (area), g.name (group)
// returns 'w', 'a', 'g' or 'x' as default
static char target_location(const char *argument, const char **nextarg)
{
    if ( argument == NULL || strlen(argument) < 2 || argument[1] != '.' )
    {
        *nextarg = argument;
        return 'x';
    }
    if ( argument[0] == 'w' || argument[0] == 'a' || argument[0] == 'g' )
    {
        *nextarg = argument + 2;
        return argument[0];
    }
    *nextarg = argument;
    return 'x';    
}

CHAR_DATA *get_char_new( CHAR_DATA *ch, const char *argument, bool area, bool exact )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *target;
    int number;
    int count;
    
    if ( ch == NULL || ch->in_room == NULL )
        return NULL;

    if ( !str_cmp(argument, "self") )
        return ch;
    if ( !str_cmp(argument, "opponent") )
        return ch->fighting;

    char location = target_location(argument, &argument);
    number = number_argument( argument, arg );
    count = 0;

    // area flag restricts locations
    if ( area && (location == 'g' || location == 'w') )
        location = 'x';
    
    if ( location == 'g' )
        return get_char_group_new(ch, argument, exact);
    
    // search in room first, keeping count
    if ( location == 'x' )
        for ( target = ch->in_room->people; target != NULL; target = target->next_in_room )
        {
            if ( !can_see( ch, target ) || !is_ch_name(arg, target, exact, ch) )
                continue;

            if ( ++count == number )
                return target;
        }
    
    // then in area, excluding room
    if ( location == 'x' || location == 'a' )
        for ( target = char_list; target != NULL ; target = target->next )
        {
            if ( target->in_room == NULL || target->in_room->area != ch->in_room->area )
                continue;
            
            if ( location == 'x' && target->in_room == ch->in_room )
                continue;
            
            if ( !can_see( ch, target ) || !is_ch_name(arg, target, exact, ch) )
                continue;

            if ( ++count == number )
                return target;
        }
    
    if ( area || location == 'a' )
        return NULL;
    
    // finally in world
    for ( target = char_list; target != NULL ; target = target->next )
    {
        if ( target->in_room == NULL )
            continue;
         
        if ( location == 'x' && target->in_room->area == ch->in_room->area )
            continue;
        
        if ( !can_see( ch, target ) || !is_ch_name(arg, target, exact, ch) )
            continue;

        if ( ++count == number )
            return target;
    }
    
    return NULL;
}

CHAR_DATA *get_char_group_new( CHAR_DATA *ch, const char *argument, bool exact )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int number;
    int count;
    
    if ( !ch || !ch->in_room )
        return NULL;

    number = number_argument( argument, arg );
    count  = 0;

    // check in room first
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( !is_ch_name(arg, gch, exact, ch) || !is_same_group(gch, ch) )
            continue;

        if ( ++count == number )
            return gch;
    }    
    // then all others
    for ( gch = char_list; gch != NULL ; gch = gch->next )
    {
        if ( gch->in_room == ch->in_room || !is_ch_name(arg, gch, exact, ch) || !is_same_group(gch, ch) )
            continue;
        
        if ( ++count == number )
            return gch;
    }
    
    return NULL;    
}

CHAR_DATA *get_char_group( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA *gch;

    gch = get_char_group_new( ch, argument, TRUE );
    if ( gch == NULL )
        gch = get_char_group_new( ch, argument, FALSE );

    return gch;    
}

CHAR_DATA* get_mob_vnum_world( int vnum )
{
    CHAR_DATA *mob;
    for ( mob = char_list; mob != NULL; mob = mob->next )
	if ( IS_NPC( mob ) && mob->pIndexData->vnum == vnum )
	    return mob;

    return NULL;
}

ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, const char *arg )
{
    return find_location_new( ch, arg, FALSE );
}

ROOM_INDEX_DATA *find_location_new( CHAR_DATA *ch, const char *arg, bool area )
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;

    /* random target */
    if ( !strcmp(arg, "random") )
    {
        if ( area )
            return get_random_room_area( ch );
        else
            return get_random_room( ch );
    }

    /* random target in the world
     * return NULL for area == TRUE to prevent remort bugs 
     */
    if ( !strcmp(arg, "wrandom") )
    {
        if ( area )
            return NULL;
        else
            return get_random_room( ch );
    }

	if ( is_number(arg) )
	    return get_room_index( atoi( arg ) );

	/* is target a char? */
	if ( area )
	    victim = get_char_area( ch, arg );
	else
	    victim = get_char_world( ch, arg );

	if ( victim != NULL )
	    return victim->in_room;

	/* is target an object? */
	if ( area )
	    obj = get_obj_area( ch, arg );
	else
	    obj = get_obj_world( ch, arg );

	if ( obj != NULL )
	    return obj->in_room;

	return NULL;
}



/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA *get_obj_type( OBJ_INDEX_DATA *pObjIndex )
{
    OBJ_DATA *obj;
    
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( obj->pIndexData == pObjIndex )
            return obj;
    }
    
    return NULL;
}

// find object of given type in content list
OBJ_DATA* get_obj_by_type( OBJ_DATA *contents, int item_type )
{
    OBJ_DATA *obj;

    for ( obj = contents; obj != NULL; obj = obj->next_content )
        if ( obj->item_type == item_type )
            return obj;

    return NULL;
}

/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list( CHAR_DATA *ch, const char *argument, OBJ_DATA *list )
{
    char arg[MAX_INPUT_LENGTH];
    int buf, number = number_argument( argument, arg );
    OBJ_DATA *obj; 

    buf = number;
    obj = get_obj_list_new( ch, arg, list, &buf, TRUE );
    if ( obj == NULL )
    {
	buf = number;
	obj = get_obj_list_new( ch, arg, list, &buf, FALSE );
    }
    
    return obj;
}

/* number is reduced for each match found
 * exact specifies wether to look for exact name matching
 */
OBJ_DATA *get_obj_list_new( CHAR_DATA *ch, const char *arg, OBJ_DATA *list, 
			    int *number, bool exact )
{
    OBJ_DATA *obj;

    if ( *number < 1 )
    {
	bug( "get_obj_list_new: invalid number (%d)", *number );
	return NULL;
    }

    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
        if ( can_see_obj(ch, obj) && is_either_name(arg, obj->name, exact) )
        {
            if ( --(*number) == 0 )
                return obj;
        }
    }
    
    return NULL;
}

/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry( CHAR_DATA *ch, const char *argument, CHAR_DATA *viewer )
{
    char arg[MAX_INPUT_LENGTH];
    int buf, number = number_argument( argument, arg );
    OBJ_DATA *obj;

    buf = number;
    obj = get_obj_carry_new( ch, arg, viewer, &buf, TRUE );
    if ( obj == NULL )
    {
	buf = number;
	obj = get_obj_carry_new( ch, arg, viewer, &buf, FALSE );
    }

    return obj;
}

OBJ_DATA *get_obj_carry_new( CHAR_DATA *ch, const char *arg, CHAR_DATA *viewer,
			     int *number, bool exact )
{
    OBJ_DATA *obj;

    if ( *number < 1 )
    {
	bug( "get_obj_carry_new: invalid number (%d)", *number );
	return NULL;
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc == WEAR_NONE
            && (can_see_obj( viewer, obj ) )
            && is_either_name( arg, obj->name, exact ) )
        {
            if ( --(*number) == 0 )
                return obj;
        }
    }
    
    return NULL;
}

/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *get_obj_wear( CHAR_DATA *ch, const char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int buf, number = number_argument( argument, arg );
    OBJ_DATA *obj;

    buf = number;
    obj = get_obj_wear_new( ch, arg, &buf, TRUE );
    if ( obj == NULL )
    {
	buf = number;
	obj = get_obj_wear_new( ch, arg, &buf, FALSE );
    }

    return obj;
}

OBJ_DATA *get_obj_wear_new( CHAR_DATA *ch, const char *arg, int *number, bool exact )
{
    OBJ_DATA *obj;
    
    if ( *number < 1 )
    {
	bug( "get_obj_wear_new: invalid number (%d)", *number );
	return NULL;
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->wear_loc != WEAR_NONE
            &&   can_see_obj( ch, obj )
            &&   is_either_name( arg, obj->name, exact ) )
        {
            if ( --(*number) == 0 )
                return obj;
        }
    }
    
    return NULL;
}

/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *get_obj_here( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA *obj = get_obj_here_new( ch, argument, TRUE );

    if ( obj == NULL )
	obj = get_obj_here_new( ch, argument, FALSE );

    return obj;
}

OBJ_DATA *get_obj_here_new( CHAR_DATA *ch, const char *argument, bool exact )
{
    OBJ_DATA *obj;
    char arg[MIL], arg1[MIL];
    int number = number_argument( argument, arg1 );

    /* allow searching of room only for obj */
    if ( str_prefix( "room.", arg1 ) )
    {
	strcpy( arg, arg1 );

	if ( ( obj = get_obj_carry_new( ch, arg, ch, &number, exact ) ) != NULL )
	    return obj;
    
	if ( ( obj = get_obj_wear_new( ch, arg, &number, exact ) ) != NULL )
	    return obj;
    }
    else
    {
	strcpy( arg, &arg1[5] );
    }

    obj = get_obj_list_new(ch, arg, ch->in_room->contents, &number, exact); 
    if ( obj != NULL )
	return obj;

    return NULL;
}

/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA *obj = get_obj_new( ch, argument, FALSE, TRUE );

    if ( obj == NULL )
	obj = get_obj_new( ch, argument, FALSE, FALSE );

    return obj;    
}

OBJ_DATA *get_obj_area( CHAR_DATA *ch, const char *argument )
{
    OBJ_DATA *obj = get_obj_new( ch, argument, TRUE, TRUE );

    if ( obj == NULL )
	obj = get_obj_new( ch, argument, TRUE, FALSE );

    return obj;    
}

OBJ_DATA *get_obj_new( CHAR_DATA *ch, const char *argument, bool area, bool exact )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;
    
    if ( ( obj = get_obj_here( ch, argument ) ) != NULL )
        return obj;
    
    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
	if ( area )
	{
	    if ( obj->carried_by != NULL )
        {
            if ( !obj->carried_by->in_room )
            {
                bugf("get_obj_new: %s carried_by not NULL but in_room is.", obj->carried_by->name);
		        continue;
		    }
		    if ( !ch->in_room )
            {
                bugf("get_obj_new: %s ch->in_room NULL.", ch->name);
                continue;
            }
		    if ( obj->carried_by->in_room->area != ch->in_room->area )
		        continue;
        }

	    if ( obj->in_room != NULL
		 && obj->in_room->area != ch->in_room->area )
		continue;
	}

        if ( can_see_obj( ch, obj ) && is_either_name( arg, obj->name, exact ) )
        {
            if ( ++count == number )
                return obj;
        }
    }
    
    return NULL;
}

void add_money_mixed( CHAR_DATA *ch, int silver, CHAR_DATA *source )
{
    add_money(ch, silver / 100, silver % 100, source);
}

void add_money( CHAR_DATA *ch, int gold, int silver, CHAR_DATA *source )
{
    int total_gold = gold + silver/100;

    if ( gold < 0 || silver < 0 )
    {
	bugf( "add_money: negative gold/silver for %s at %d",
	      ch->name, ch->in_room ? ch->in_room->vnum : 0 );
	return;
    }

    ch->gold += gold;
    ch->silver += silver;

    /* try to catch money cheaters */
    if ( total_gold > 50000 && !IS_NPC(ch) && !IS_IMMORTAL(ch) )
    {
	logpf( "add_money: %s got %d gold %d silver from %s at %d",
	       ch->name, gold, silver,
	       source ? source->name : "?",
	       ch->in_room ? ch->in_room->vnum : 0 );
    }
}

/* deduct cost from a character */
void deduct_cost(CHAR_DATA *ch, int cost)
{
    int silver = 0, gold = 0;
    char buf[MSL];
    
    silver = UMIN(ch->silver,cost);
    
    if (silver < cost)
    {
        gold = ((cost - silver + 99) / 100);
        silver = cost - 100 * gold;
    }
    
    ch->gold -= gold;
    ch->silver -= silver;
    
    if (ch->gold < 0)
    {
/*        bug("deduct costs: gold %d < 0",ch->gold); */
        sprintf(buf,"Deduct costs: gold %ld < 0, player: %s, room %d",
            ch->gold,
            ch->name != NULL ? ch->name : "Null",
            ch->in_room != NULL ? ch->in_room->vnum : 0);
        bug(buf,0);
        ch->gold = 0;
    }
    if (ch->silver < 0)
    {
/*        bug("deduct costs: silver %d < 0",ch->silver); */
        sprintf(buf,"Deduct costs: silver %ld < 0, player: %s, room %d",
            ch->silver,
            ch->name != NULL ? ch->name : "Null",
            ch->in_room != NULL ? ch->in_room->vnum : 0);
        bug(buf,0);
        ch->silver = 0;
    }
}


int money_weight( int silver, int gold )
{
    return (silver + 99) / 100 + (gold + 24) / 25;
}

/*
 * Create a 'money' obj.
 */
OBJ_DATA *create_money( int gold, int silver )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    
    if ( gold < 0 || silver < 0 || (gold == 0 && silver == 0) )
    {
        bug( "Create_money: zero or negative money.",UMIN(gold,silver));
        gold = UMAX(1,gold);
        silver = UMAX(1,silver);
    }
    
    if (gold == 0 && silver == 1)
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SILVER_ONE ), 0 );
    }
    else if (gold == 1 && silver == 0)
    {
        obj = create_object( get_obj_index( OBJ_VNUM_GOLD_ONE), 0 );
    }
    else if (silver == 0)
    {
        obj = create_object( get_obj_index( OBJ_VNUM_GOLD_SOME ), 0 );
        sprintf( buf, obj->short_descr, gold );
        free_string( obj->short_descr );
        obj->short_descr        = str_dup( buf );
        obj->value[1]           = gold;
        obj->cost               = 100 * gold;
        obj->weight     = money_weight( silver, gold );
    }
    else if (gold == 0)
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SILVER_SOME ), 0 );
        sprintf( buf, obj->short_descr, silver );
        free_string( obj->short_descr );
        obj->short_descr        = str_dup( buf );
        obj->value[0]           = silver;
        obj->cost               = silver;
        obj->weight     = money_weight( silver, gold );
    }
    
    else
    {
        obj = create_object( get_obj_index( OBJ_VNUM_COINS ), 0 );
        sprintf( buf, obj->short_descr, silver, gold );
        free_string( obj->short_descr );
        obj->short_descr    = str_dup( buf );
        obj->value[0]       = silver;
        obj->value[1]       = gold;
        obj->cost       = 100 * gold + silver;
        obj->weight     = money_weight( silver, gold );
    }
    
    return obj;
}



/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number( OBJ_DATA *obj )
{
    int number;
    
    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY
        || obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY
        || is_remort_obj(obj) )
        number = 0;
    else
        number = 1;
    
    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        number += get_obj_number( obj );
    
    return number;
}


/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight( OBJ_DATA *obj )
{
    int weight;
    OBJ_DATA *tobj;
    
    weight = obj->weight;
    for ( tobj = obj->contains; tobj != NULL; tobj = tobj->next_content )
        weight += get_obj_weight( tobj ) * WEIGHT_MULT(obj) / 100;
    
    return weight;
}

int get_true_weight(OBJ_DATA *obj)
{
    int weight;
    
    weight = obj->weight;
    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        weight += get_obj_weight( obj );
    
    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex )
{
    if (!pRoomIndex)
        return FALSE;
    
    if ( pRoomIndex->light > 0 )
        return FALSE;
    
    if ( IS_SET(pRoomIndex->room_flags, ROOM_DARK) )
        return TRUE;
    
    if ( pRoomIndex->sector_type == SECT_INSIDE
        ||   pRoomIndex->sector_type == SECT_CITY )
        return FALSE;
    
    if ( weather_info.sunlight == SUN_DARK )
        return TRUE;
    
    return FALSE;
}

bool room_is_sunlit( ROOM_INDEX_DATA *pRoomIndex )
{
    if (!pRoomIndex)
        return FALSE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_DARK) 
	 || IS_SET(pRoomIndex->room_flags, ROOM_INDOORS) )
        return FALSE;
    
    if ( pRoomIndex->sector_type == SECT_INSIDE )
        return FALSE;
    
    if ( weather_info.sunlight == SUN_DARK )
        return FALSE;
    
    return TRUE;
}

bool is_room_owner(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    if (room->owner == NULL || room->owner[0] == '\0')
        return FALSE;
    
    return is_name(ch->name,room->owner);
}

bool is_obj_owner(CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (obj->owner == NULL || obj->owner[0] == '\0')
        return FALSE;
    
    return !str_cmp(capitalize(obj->owner), capitalize(ch->name));
}

/*
 * True if room is private.
 */
bool room_is_private( ROOM_INDEX_DATA *pRoomIndex )
{
    CHAR_DATA *rch;
    int count;
    
    
    if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0')
        return TRUE;
    
    count = 0;
    for ( rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room )
        if ( !IS_NPC(rch) && !IS_IMMORTAL(rch) )
            count++;
    
    if ( IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)  && count >= 2 )
        return TRUE;
    
    if ( IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) && count >= 1 )
        return TRUE;
    
    if ( IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) )
        return TRUE;
    
    return FALSE;
}

/* visibility on a room -- for entering and exits */
bool can_see_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY)
        &&  get_trust(ch) < MAX_LEVEL)
        return FALSE;
    
    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_SEE_ALL) )
        return TRUE;
    
    if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY)
        &&  !IS_IMMORTAL(ch))
        return FALSE;
    
    if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY)
        && !IS_NPC(ch) && !IS_HERO(ch))
        return FALSE;
    
    if (IS_SET(pRoomIndex->room_flags, ROOM_NEWBIES_ONLY)
        &&  ch->level > 5 
        && !IS_IMMORTAL(ch))
        return FALSE;
    
    if (!IS_IMMORTAL(ch) 
//        && !(IS_NPC(ch) && IS_SET(ch->act,ACT_PET))
        && !IS_NPC(ch)
        && pRoomIndex->clan 
        && (ch->clan != pRoomIndex->clan
	    || ch->pcdata->clan_rank < pRoomIndex->clan_rank) )
        return FALSE;

    if (NOT_AUTHED(ch)
	//&& !IS_SET(pRoomIndex->room_flags, ROOM_NEWBIES_ONLY)
        && ( pRoomIndex->vnum < ROOM_VNUM_AUTH_START 
	     || pRoomIndex->vnum > ROOM_VNUM_AUTH_END))
        return FALSE;
    
    return TRUE;
}

/* hack to make helpers visible during a call without massive parameter passing */
bool helper_visible = FALSE;
bool ignore_invisible = FALSE; // hunt etc.

/* will not see / will see / has chance to see */
#define SEE_CANT 0
#define SEE_CAN  1
#define SEE_MAY  2

/* returns whether ch does see victim, does not see victim, or has a chance
 * to see victim (hide not checked!)
 */
int can_see_new( CHAR_DATA *ch, CHAR_DATA *victim, bool combat )
{
    if ( victim->must_extract )
        return SEE_CANT;
    
    /* RT changed so that WIZ_INVIS has levels */
    if ( ch == victim )
        return SEE_CAN;
    
    if ( helper_visible && IS_HELPER(victim) && !combat )
        return SEE_CAN;

    if ( get_trust(ch) < victim->invis_level )
        return SEE_CANT;
    
    if ( get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room )
        return SEE_CANT;
    
    if ( (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) || (IS_NPC(ch) && IS_IMMORTAL(ch)) )
        return SEE_CAN;
    
    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_SEE_ALL) && !combat )
        return SEE_CAN;
    
    if ( IS_NPC(victim) && IS_SET(victim->act, ACT_WIZI) )
	return SEE_CANT;

    if ( IS_AFFECTED(ch, AFF_BLIND) )
        return SEE_CANT;
    
    /* Freeze tag players are vis to other players. */
    if (IS_TAG(ch) && IS_TAG(victim))
        return SEE_CAN;
    
    if ( !ignore_invisible )
    {
	if ( room_is_dark( victim->in_room )
	     && (!IS_AFFECTED(ch, AFF_INFRARED) || IS_SET(victim->form, FORM_COLD_BLOOD))
	     && !IS_AFFECTED(ch, AFF_DARK_VISION) )
	    return SEE_CANT;
    
	if (IS_AFFECTED(victim, AFF_SHELTER))
	    return SEE_CANT;
	
	if ( IS_AFFECTED(victim, AFF_INVISIBLE)
	     && !IS_AFFECTED(ch, AFF_DETECT_INVIS) )
	    return SEE_CANT;
    }

    if ( IS_AFFECTED(victim, AFF_ASTRAL)
	 && !IS_AFFECTED(ch, AFF_DETECT_ASTRAL)
	 && !IS_AFFECTED(ch, AFF_ASTRAL)
	 && victim->fighting == NULL )
	return SEE_CANT;
    
    /* Other unauthed players, imms, and NPCs may see unauthed players. */
    if( NOT_AUTHED( victim ) )
    {
        if ( !NOT_AUTHED( ch ) 
	     && !IS_IMMORTAL( ch )
	     && !IS_HELPER( ch )
	     && !IS_NPC( ch ) )
	    return SEE_CANT;
    }
    
    if ( ignore_invisible )
	return SEE_CAN;
    else
	return SEE_MAY;
}

/*
 * True if char can see victim.
 */
bool can_see( CHAR_DATA *ch, CHAR_DATA *victim )
{
    return can_see_new(ch, victim, FALSE) != SEE_CANT;
}

bool can_see_combat( CHAR_DATA *ch, CHAR_DATA *victim )
{
    return can_see_new(ch, victim, TRUE) != SEE_CANT;
}

/* True if room would be dark if lights were removed */
bool room_is_dim( ROOM_INDEX_DATA *pRoomIndex )
{
    bool is_dim;
    int buf = pRoomIndex->light;
    pRoomIndex->light = 0;
    is_dim = room_is_dark(pRoomIndex);
    pRoomIndex->light = buf;
    return is_dim;
}

#define LIGHT_DARK     0
#define LIGHT_NORMAL   1
#define LIGHT_GLOW     2
#define LIGHT_BRIGHT   3

/* returns how bright a character is */
int light_status( CHAR_DATA *ch )
{
    int min_light, wear;
    OBJ_DATA *obj;

    if ( get_eq_char(ch, WEAR_LIGHT) != NULL || IS_SET(ch->form, FORM_BRIGHT) )
	return LIGHT_BRIGHT;
   
    if ( IS_AFFECTED(ch, AFF_DARKNESS) )
	min_light = LIGHT_DARK;
    else
	min_light = LIGHT_NORMAL;

    /* check for glowing or non_dark eq */
    for ( wear = WEAR_FINGER_L; wear <= WEAR_SECONDARY; wear++ )
    {
	obj = get_eq_char( ch, wear );
	if ( obj == NULL )
	    continue;
	if ( IS_OBJ_STAT(obj, ITEM_GLOW) )
	    return LIGHT_GLOW;
	if ( !IS_OBJ_STAT(obj, ITEM_DARK) )
	    min_light = LIGHT_NORMAL;
    }
    return min_light;
}

/*
 * True if char succeeds in seeing the victim.  Implements effects of hide skill.
 */
bool check_see( CHAR_DATA *ch, CHAR_DATA *victim )
{
    return check_see_new(ch, victim, FALSE);
}

bool check_see_combat( CHAR_DATA *ch, CHAR_DATA *victim )
{
    return check_see_new(ch, victim, TRUE);
}

bool check_see_new( CHAR_DATA *ch, CHAR_DATA *victim, bool combat )
{
    int see_state = can_see_new(ch, victim, combat);
    int roll_ch;
    int roll_victim;
    
    if (see_state == SEE_CAN)
        return TRUE;
    
    if (see_state == SEE_CANT)
        return FALSE;
    
    if (!IS_AFFECTED(victim, AFF_HIDE) || victim->fighting != NULL)
        return TRUE;
    
    if ( is_same_group(ch, victim) )
	return TRUE;

    // heavy armor penalty grants auto-chance to be spotted
    if ( per_chance(get_heavy_armor_penalty(victim)/2) )
        return TRUE;
    
    /* victim is hidden, check if char spots it, resisted roll */
    
    roll_ch = (ch->level + 
        get_curr_stat(ch, STAT_INT) + 
        get_curr_stat(ch, STAT_WIS)) / 5;
    roll_victim = (victim->level + 
        get_curr_stat(victim, STAT_AGI) + 
        get_curr_stat(victim, STAT_DEX)) / 5;
    
    /* consider hide skill */
    if (!IS_NPC(victim))
        roll_victim = roll_victim * get_skill(victim, gsn_hide) / 100;
    
    /* easier to hide in dark rooms */
    if (room_is_dim(victim->in_room) && !IS_AFFECTED(ch, AFF_DARK_VISION) &&
        (!IS_AFFECTED(ch, AFF_INFRARED) || IS_SET(victim->form, FORM_COLD_BLOOD)))
	switch ( light_status(victim) )
	{
	case LIGHT_DARK:
	    roll_victim *= 2;
		break;
	case LIGHT_NORMAL: 
	    roll_victim *= 2;
		break;
	case LIGHT_GLOW: 
	    roll_victim *= 2;
		break;
	case LIGHT_BRIGHT: 
		break;
	default:
		break;
	}
    
    /* small races can hide better */
    roll_victim = roll_victim * (5 + SIZE_MEDIUM - victim->size) / 5;
    
    /* detect_hidden */
    if (IS_AFFECTED(ch, AFF_DETECT_HIDDEN))
        roll_ch *= 2;

    /* alertness skill */
    if (number_percent() < get_skill(ch, gsn_alertness))
    {
        roll_ch *= 2;
        check_improve(ch,gsn_alertness,TRUE,15);
    }    

    /* now the roll */
    return number_range(0, roll_ch) > number_range(0, roll_victim * 5);
}

/*
 * True if char can see obj.
 */
bool can_see_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( obj == NULL)
        return FALSE;
    
    if ( ch == NULL)
        return FALSE;
    
    if ( obj->must_extract )
        return FALSE;
    
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT) )
        return TRUE;
    
    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_SEE_ALL) )
        return TRUE;
    
    if ( IS_SET(obj->extra_flags,ITEM_VIS_DEATH))
        return FALSE;
    
    if ( IS_AFFECTED(ch, AFF_BLIND)
	 && obj->item_type != ITEM_POTION
	 && obj->item_type != ITEM_PILL
	 && !IS_OBJ_STAT(obj,ITEM_HUM) )
        return FALSE;
    
    if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
        return TRUE;
    
    if ( IS_SET(obj->extra_flags, ITEM_INVIS)
	 && !IS_AFFECTED(ch, AFF_DETECT_INVIS) )
        return FALSE;
    
    if ( (obj->item_type == ITEM_PORTAL)
	 && I_IS_SET(obj->value[2], GATE_ASTRAL)
	 && !IS_AFFECTED(ch, AFF_DETECT_ASTRAL)
	 && !IS_AFFECTED(ch, AFF_ASTRAL) )
        return FALSE;
    
    if ( IS_OBJ_STAT(obj,ITEM_GLOW) || IS_OBJ_STAT(obj,ITEM_HUM) )
        return TRUE;
    
    if ( room_is_dark( ch->in_room ) && !IS_AFFECTED(ch, AFF_DARK_VISION) )
        return FALSE;
    
    return TRUE;
}



/*
 * True if char can drop obj.
 */
bool can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( !IS_SET(obj->extra_flags, ITEM_NODROP) )
        return TRUE;
    
    if ( !IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL )
        return TRUE;
    
    return FALSE;
}


/*
 * Return ascii name of an affect location.
 */
const char *affect_loc_name( int location )
{
    switch ( location )
    {
    case APPLY_NONE:        return "none";
    case APPLY_STR:     return "strength";
    case APPLY_DEX:     return "dexterity";
    case APPLY_INT:     return "intelligence";
    case APPLY_WIS:     return "wisdom";
    case APPLY_CON:     return "constitution";
    case APPLY_VIT:     return "vitality";
    case APPLY_AGI:     return "agility";
    case APPLY_DIS:     return "discipline";
    case APPLY_CHA:     return "charisma";
    case APPLY_LUC:     return "luck";
    case APPLY_STATS:   return "all stats";
    case APPLY_SKILLS:  return "all skills";
    case APPLY_SEX:     return "sex";
    case APPLY_CLASS:   return "class";
    case APPLY_LEVEL:   return "level";
    case APPLY_HEIGHT:  return "height";
    case APPLY_WEIGHT:  return "weight";
    case APPLY_AGE:     return "age";
    case APPLY_MANA:    return "mana";
    case APPLY_HIT:     return "hp";
    case APPLY_MOVE:    return "moves";
    case APPLY_GOLD:    return "gold";
    case APPLY_EXP:     return "experience";
    case APPLY_AC:      return "armor class";
    case APPLY_HITROLL: return "hit roll";
    case APPLY_DAMROLL: return "damage roll";
    case APPLY_SAVES:   return "saves";
    case APPLY_SAVING_ROD:      return "save vs rod";
    case APPLY_SAVING_PETRI:    return "save vs petrification";
    case APPLY_SAVING_BREATH:   return "save vs breath";
    case APPLY_SAVING_SPELL:    return "save vs spell";
    case APPLY_SPELL_AFFECT:    return "none";
//    case APPLY_COMBO:   return "combo points";
    }
    
    bug( "Affect_location_name: unknown location %d.", location );
    return "(unknown)";
}

/* methods for retrieving the ascii name(s) of flags --Bobble */

/* returns the name of a flag */
const char* flag_bit_name( const struct flag_type flag_table[], int flag )
{
    static char buf[100];
    int i;
    for ( i = 0; flag_table[i].name != NULL; i++ )
	if ( flag_table[i].bit == flag )
	    return flag_table[i].name;
    sprintf( buf, "none(%d)", flag );
    return buf;
}

/* returns a string with the flag names in a field */
const char* flag_bits_name( const struct flag_type flag_table[], tflag flag )
{
    int i;
    /* make 'sure' different calls use different buffers */
    static SR_BUF sr_buf;
    char *buf = next_sr_buf( &sr_buf );

    buf[0] = 0;
    /* add affect name one by one */
    for ( i = 1; i < FLAG_MAX_BIT; i++ )
	if ( flag_is_set( flag, i ) )
	{
	    if ( buf[0] != 0 )
		strcat( buf, " " );
	    strcat( buf, flag_bit_name(flag_table, i) );
	}
    /* no affects? */
    if ( buf[0] == 0 )
	return "none";
    else
	return buf;
}

/* returns a string with the flag names in an integer-flag */
const char* i_flag_bits_name( const struct flag_type flag_table[], long flag )
{
    int i;

    /* make 'sure' different calls use different buffers */
    static SR_BUF sr_buf;
    char *buf = next_sr_buf( &sr_buf );
    
    buf[0] = 0;
    /* add affect name one by one */
    for ( i = 0; flag_table[i].name != NULL; i++ )
	if ( I_IS_SET(flag, flag_table[i].bit) )
	{
	    if ( buf[0] != 0 )
		strcat( buf, " " );
	    strcat( buf, flag_table[i].name );
	}
    /* no affects? */
    if ( buf[0] == 0 )
	return "none";
    else
	return buf;
}

const char* affect_bit_name( int flag )
{
    return flag_bit_name( affect_flags, flag );
}

const char* affect_bits_name( tflag flag )
{
    return flag_bits_name( affect_flags, flag );
}

const char* extra_bit_name( int flag )
{
    return flag_bit_name( extra_flags, flag );
}

const char* extra_bits_name( tflag flag )
{
    return flag_bits_name( extra_flags, flag );
}

const char* act_bits_name( tflag flag )
{
    /* check for npc/player */
    if ( IS_SET( flag, ACT_IS_NPC) )
	return flag_bits_name( act_flags, flag );
    else
	return flag_bits_name( plr_flags, flag );	
}

const char* comm_bit_name( int flag )
{
    return flag_bit_name( comm_flags, flag );
}

const char* comm_bits_name( tflag flag )
{
    return flag_bits_name( comm_flags, flag );
}

const char *penalty_bits_name( tflag penalty_flags )
{
    int i;
    static char buf[512];
    buf[0] = '\0';
    
    for (i = 1; i < MAX_PENALTY; i++)
    {
        if ( IS_SET(penalty_flags, penalty_table[i].bit) )
        {
            strcat(buf, " ");
            strcat(buf, penalty_table[i].apply_string);
        }
    }
    
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

const char* imm_bit_name( int flag )
{
    return flag_bit_name( imm_flags, flag );
}

const char* imm_bits_name( tflag flag )
{
    return flag_bits_name( imm_flags, flag );
}

const char* wear_bit_name( int flag )
{
    return flag_bit_name( wear_flags, flag );
}

const char* wear_bits_name( tflag flag )
{
    return flag_bits_name( wear_flags, flag );
}

const char* form_bit_name( int flag )
{
    return flag_bit_name( form_flags, flag );
}

const char* form_bits_name( tflag flag )
{
    return flag_bits_name( form_flags, flag );
}

const char* part_bit_name( int flag )
{
    return flag_bit_name( part_flags, flag );
}

const char* part_bits_name( tflag flag )
{
    return flag_bits_name( part_flags, flag );
}

const char* weapon_bit_name( int flag )
{
    return flag_bit_name( weapon_type2, flag );
}

const char* weapon_bits_name( long flag )
{
    return i_flag_bits_name( weapon_type2, flag );
}

const char* cont_bits_name( long flag )
{
    return i_flag_bits_name( container_flags, flag );
}

const char* off_bit_name( int flag )
{
    return flag_bit_name( off_flags, flag );
}

const char* off_bits_name( tflag flag )
{
    return flag_bits_name( off_flags, flag );
}

const char* to_bit_name( int where, int flag )
{
    static char buf[MSL];
    
    switch( where )
    {
    case TO_AFFECTS:
	sprintf( buf, "%s", affect_bit_name(flag) );
	return buf;
    case TO_IMMUNE:
	sprintf( buf, "%s immunity", imm_bit_name(flag) );
	return buf;
    case TO_RESIST:
	sprintf( buf, "%s resistance", imm_bit_name(flag) );
	return buf;
    case TO_VULN:
	sprintf( buf, "%s vulnerability", imm_bit_name(flag) );
	return buf;
    default:
	return "none";
    }
}

/* Retrieve the room an object is ultimately in.  B.Castle, 4/98 */
ROOM_INDEX_DATA *get_obj_room(OBJ_DATA *obj)
{
    if (obj == NULL)                   /* Bad argument. */
        return NULL;
    
    if (obj->in_room != NULL)          /* Simple case - object on floor. */
        return obj->in_room;
    
    else if (obj->carried_by != NULL)  /* Carried by a player or mob. */
        return obj->carried_by->in_room;
    
    else if (obj->on != NULL)          /* On another object. */
        return obj->on->in_room;
    
    else if (obj->in_obj != NULL)      /* Inside a container. */
    {
        OBJ_DATA *optr;
        
        for (optr = obj; optr->in_obj != NULL; optr = optr->in_obj)
            ;                            /* Manage containers within containers. */
        
        if (optr->in_room != NULL)
            return optr->in_room;
        
        else if (optr->carried_by != NULL)   /* Container might be carried as well. */
            return optr->carried_by->in_room;
        
        else if (optr->on != NULL)      /* Container might be on an object as well. */
            return optr->on->in_room;
    }
    
    bug("Get_obj_room: Unable to find room for object %d.", obj->pIndexData->vnum);
    return NULL;                       /* Scream and run in circles. */
}

/* Retrieve the char ultimately carrying an object. */
CHAR_DATA* get_obj_char( OBJ_DATA *obj )
{
    if ( obj == NULL )
	return NULL;

    while ( obj->in_obj != NULL )
	obj = obj->in_obj;

    return obj->carried_by;
}

void default_colour( CHAR_DATA *ch )
{
    if (IS_NPC( ch ) )
        return;
    
    if ( !ch->pcdata )
        return;
    
    ch->pcdata->gossip[1] = ( MAGENTA );
    ch->pcdata->gossip_text[1] = ( MAGENTA );
    ch->pcdata->auction[1] = ( YELLOW );
    ch->pcdata->auction_text[1] = ( YELLOW );
    ch->pcdata->music[1] = ( MAGENTA );
    ch->pcdata->music_text[1] = ( MAGENTA );
    ch->pcdata->question[1] = ( GREEN );
    ch->pcdata->question_text[1] = ( YELLOW );
    ch->pcdata->answer[1] = ( YELLOW );
    ch->pcdata->answer_text[1] = ( GREEN );
    ch->pcdata->quote[1] = ( BLACK );
    ch->pcdata->quote_text[1] = ( WHITE );
    ch->pcdata->gratz[1] = ( BLUE );
    ch->pcdata->gratz_text[1] = ( BLUE );
    ch->pcdata->immtalk[1] = ( GREEN );
    ch->pcdata->immtalk_text[1] = ( GREEN );
    ch->pcdata->savantalk[1] = ( RED );
    ch->pcdata->savantalk_text[1] = ( RED );
    ch->pcdata->shouts[1] = ( YELLOW );
    ch->pcdata->shouts_text[1] = ( YELLOW );
    ch->pcdata->tells[1] = ( YELLOW );
    ch->pcdata->tell_text[1] = ( YELLOW );
    ch->pcdata->info[1] = ( WHITE );
    ch->pcdata->info_text[1] = ( WHITE );
    ch->pcdata->gametalk[1] = ( CYAN );
    ch->pcdata->gametalk_text[1] = ( CYAN );
    ch->pcdata->bitch[1] = ( RED );
    ch->pcdata->bitch_text[1] = ( RED );
    ch->pcdata->newbie[1] = ( CYAN);
    ch->pcdata->newbie_text[1] = ( CYAN );
    ch->pcdata->clan[1] = ( GREEN );
    ch->pcdata->clan_text[1] = ( GREEN );
    ch->pcdata->say[1] = ( WHITE );
    ch->pcdata->say_text[1] = ( WHITE );
    ch->pcdata->gtell[1] = ( CYAN );
    ch->pcdata->gtell_text[1] = ( CYAN );
    ch->pcdata->room_title[1] = ( WHITE );
    ch->pcdata->room_exits[1] = ( WHITE );
    ch->pcdata->wiznet[1] = ( WHITE );
    ch->pcdata->warfare[1] = ( RED );
    ch->pcdata->warfare_text[1] = ( RED );
    ch->pcdata->proclaim[1] = ( WHITE );
    ch->pcdata->proclaim_text[1] = ( BLUE );
    
    ch->pcdata->gossip[0] = ( NORMAL );
    ch->pcdata->gossip_text[0] = ( NORMAL );
    ch->pcdata->auction[0] = ( NORMAL );
    ch->pcdata->auction_text[0] = ( NORMAL );
    ch->pcdata->music[0] = ( BRIGHT );
    ch->pcdata->music_text[0] = ( BRIGHT );
    ch->pcdata->question[0] = ( NORMAL );
    ch->pcdata->question_text[0] = ( NORMAL );
    ch->pcdata->answer[0] = ( NORMAL );
    ch->pcdata->answer_text[0] = ( NORMAL );
    ch->pcdata->quote[0] = ( BRIGHT );
    ch->pcdata->quote_text[0] = ( NORMAL );
    ch->pcdata->gratz[0] = ( BRIGHT );
    ch->pcdata->gratz_text[0] = ( BRIGHT );
    ch->pcdata->immtalk[0] = ( BRIGHT );
    ch->pcdata->immtalk_text[0] = ( BRIGHT );
    ch->pcdata->savantalk[0] = ( NORMAL );
    ch->pcdata->savantalk_text[0] = ( NORMAL );
    ch->pcdata->shouts[0] = ( NORMAL );
    ch->pcdata->shouts_text[0] = ( NORMAL );
    ch->pcdata->tells[0] = ( NORMAL );
    ch->pcdata->tell_text[0] = ( BRIGHT );
    ch->pcdata->info[0] = ( BRIGHT );
    ch->pcdata->info_text[0] = ( NORMAL );
    ch->pcdata->gametalk[0] = ( NORMAL );
    ch->pcdata->gametalk_text[0] = ( NORMAL );
    ch->pcdata->bitch[0] = ( NORMAL );
    ch->pcdata->bitch_text[0] = ( BRIGHT );
    ch->pcdata->newbie[0] = ( BRIGHT );
    ch->pcdata->newbie_text[0] = ( BRIGHT );
    ch->pcdata->clan[0] = ( NORMAL );
    ch->pcdata->clan_text[0] = ( BRIGHT );
    ch->pcdata->say[0] = ( NORMAL );
    ch->pcdata->say_text[0] = ( NORMAL );
    ch->pcdata->gtell[0] = ( NORMAL );
    ch->pcdata->gtell_text[0] = ( BRIGHT );
    ch->pcdata->room_title[0] = ( BRIGHT );
    ch->pcdata->room_exits[0] = ( BRIGHT );
    ch->pcdata->wiznet[0] = ( NORMAL );
    ch->pcdata->warfare[0] = ( BRIGHT );
    ch->pcdata->warfare_text[0] = ( BRIGHT );
    ch->pcdata->proclaim[0] = ( BRIGHT );
    ch->pcdata->proclaim_text[0] = ( BRIGHT );
    
    ch->pcdata->gossip[2] = 0;
    ch->pcdata->gossip_text[2] = 0;
    ch->pcdata->auction[2] = 0;
    ch->pcdata->auction_text[2] = 0;
    ch->pcdata->music[2] = 0;
    ch->pcdata->music_text[2] = 0;
    ch->pcdata->question[2] = 0;
    ch->pcdata->question_text[2] = 0;
    ch->pcdata->answer[2] = 0;
    ch->pcdata->answer_text[2] = 0;
    ch->pcdata->quote[2] = 0;
    ch->pcdata->quote_text[2] = 0;
    ch->pcdata->gratz[2] = 0;
    ch->pcdata->gratz_text[2] = 0;
    ch->pcdata->immtalk[2] = 0;
    ch->pcdata->immtalk_text[2] = 0;
    ch->pcdata->savantalk[2] = 0;
    ch->pcdata->savantalk_text[2] = 0;
    ch->pcdata->shouts[2] = 0;
    ch->pcdata->shouts_text[2] = 0;
    ch->pcdata->tells[2] = 0;
    ch->pcdata->tell_text[2] = 0;
    ch->pcdata->info[2] = 0;
    ch->pcdata->info_text[2] = 0;
    ch->pcdata->gametalk[2] = 0;
    ch->pcdata->gametalk_text[2] = 0;
    ch->pcdata->bitch[2] = 0;
    ch->pcdata->bitch_text[2] = 0;
    ch->pcdata->newbie[2] = 0;
    ch->pcdata->newbie_text[2] = 0;
    ch->pcdata->clan[2] = 0;
    ch->pcdata->clan_text[2] = 0;
    ch->pcdata->say[2] = 0;
    ch->pcdata->say_text[2] = 0;
    ch->pcdata->gtell[2] = 0;
    ch->pcdata->gtell_text[2] = 0;
    ch->pcdata->room_title[2] = 0;
    ch->pcdata->room_exits[2] = 0;
    ch->pcdata->wiznet[2] = 0;
    ch->pcdata->warfare[2] = 0;
    ch->pcdata->warfare_text[2] = 0;
    ch->pcdata->proclaim[2] = 0;
    ch->pcdata->proclaim_text[2] = 0;
    
    return;
}

void all_colour( CHAR_DATA *ch, const char *argument )
{
    char buf[ 100 ];
    char buf2[ 100 ];
    int colour;
    int bright;
    
    if (IS_NPC(ch) || !ch->pcdata )
        return;
    
    if (!*argument)
        return;
    
    if (!str_prefix( argument, "red" ) )
    {
        colour = ( RED );
        bright = NORMAL;
        sprintf( buf2, "Red" );
    }
    else if (!str_prefix( argument, "hi-red" ) )
    {
        colour = ( RED );
        bright = BRIGHT;
        sprintf( buf2, "Red" );
    }
    else if (!str_prefix( argument, "green" ) )
    {
        colour = ( GREEN );
        bright = NORMAL;
        sprintf( buf2, "Green" );
    }
    else if (!str_prefix( argument, "hi-green" ) )
    {
        colour = ( GREEN );
        bright = BRIGHT;
        sprintf( buf2, "Green" );
    }
    else if (!str_prefix( argument, "yellow" ) )
    {
        colour = ( YELLOW );
        bright = NORMAL;
        sprintf( buf2, "Yellow" );
    }
    else if (!str_prefix( argument, "hi-yellow" ) )
    {
        colour = ( YELLOW );
        bright = BRIGHT;
        sprintf( buf2, "Yellow" );
    }
    else if (!str_prefix( argument, "blue" ) )
    {
        colour = ( BLUE );
        bright = NORMAL;
        sprintf( buf2, "Blue" );
    }
    else if (!str_prefix( argument, "hi-blue" ) )
    {
        colour = ( BLUE );
        bright = BRIGHT;
        sprintf( buf2, "Blue" );
    }
    else if (!str_prefix( argument, "magenta" ) )
    {
        colour = ( MAGENTA );
        bright = NORMAL;
        sprintf( buf2, "Magenta" );
    }
    else if (!str_prefix( argument, "hi-magenta" ) )
    {
        colour = ( MAGENTA );
        bright = BRIGHT;
        sprintf( buf2, "Magenta" );
    }
    else if (!str_prefix( argument, "cyan" ) )
    {
        colour = ( CYAN );
        bright = NORMAL;
        sprintf( buf2, "Cyan" );
    }
    else if (!str_prefix( argument, "hi-cyan" ) )
    {
        colour = ( CYAN );
        bright = BRIGHT;
        sprintf( buf2, "Cyan" );
    }
    else if (!str_prefix( argument, "white" ) )
    {
        colour = ( WHITE );
        bright = NORMAL;
        sprintf( buf2, "White" );
    }
    else if (!str_prefix( argument, "hi-white" ) )
    {
        colour = ( WHITE );
        bright = BRIGHT;
        sprintf( buf2, "White" );
    }
    else if (!str_prefix( argument, "grey" ) || !str_prefix( argument, "black" ) )
    {
        colour = ( BLACK );
        bright = NORMAL;
        sprintf( buf2, "Grey" );
    }
    else if (!str_prefix( argument, "clear" ) )
    {
        colour = ( COLOUR_NONE );
        bright = NORMAL;
        sprintf( buf2, "Clear" );
    }
    else
    {
        send_to_char( "Unrecognized colour. Unchanged.\n\r", ch );
        return;
    }
    
    ch->pcdata->gossip[1] = colour;
    ch->pcdata->gossip_text[1] = colour;
    ch->pcdata->auction[1] = colour;
    ch->pcdata->auction_text[1] = colour;
    ch->pcdata->music[1] = colour;
    ch->pcdata->music_text[1] = colour;
    ch->pcdata->question[1] = colour;
    ch->pcdata->question_text[1] = colour;
    ch->pcdata->answer[1] = colour;
    ch->pcdata->answer_text[1] = colour;
    ch->pcdata->quote[1] = colour;
    ch->pcdata->quote_text[1] = colour;
    ch->pcdata->gratz[1] = colour;
    ch->pcdata->gratz_text[1] = colour;
    ch->pcdata->immtalk[1] = colour;
    ch->pcdata->immtalk_text[1] = colour;
    ch->pcdata->savantalk[1] = colour;
    ch->pcdata->savantalk_text[1] = colour;
    ch->pcdata->shouts[1] = colour;
    ch->pcdata->shouts_text[1] = colour;
    ch->pcdata->tells[1] = colour;
    ch->pcdata->tell_text[1] = colour;
    ch->pcdata->info[1] = colour;
    ch->pcdata->info_text[1] = colour;
    ch->pcdata->gametalk[1] = colour;
    ch->pcdata->gametalk_text[1] = colour;
    ch->pcdata->bitch[1] = colour;
    ch->pcdata->bitch_text[1] = colour;
    ch->pcdata->newbie[1] = colour;
    ch->pcdata->newbie_text[1] = colour;
    ch->pcdata->clan[1] = colour;
    ch->pcdata->clan_text[1] = colour;
    ch->pcdata->say[1] = colour;
    ch->pcdata->say_text[1] = colour;
    ch->pcdata->gtell[1] = colour;
    ch->pcdata->gtell_text[1] = colour;
    ch->pcdata->room_title[1] = colour;
    ch->pcdata->room_exits[1] = colour;
    ch->pcdata->wiznet[1] = colour;
    ch->pcdata->warfare[1] = colour;
    ch->pcdata->warfare_text[1] = colour;
    ch->pcdata->proclaim[1] = colour;
    ch->pcdata->proclaim_text[1] = colour;
    
    ch->pcdata->gossip[0] = bright;
    ch->pcdata->gossip_text[0] = bright;
    ch->pcdata->auction[0] = bright;
    ch->pcdata->auction_text[0] = bright;
    ch->pcdata->music[0] = bright;
    ch->pcdata->music_text[0] = bright;
    ch->pcdata->question[0] = bright;
    ch->pcdata->question_text[0] = bright;
    ch->pcdata->answer[0] = bright;
    ch->pcdata->answer_text[0] = bright;
    ch->pcdata->quote[0] = bright;
    ch->pcdata->quote_text[0] = bright;
    ch->pcdata->gratz[0] = bright;
    ch->pcdata->gratz_text[0] = bright;
    ch->pcdata->immtalk[0] = bright;
    ch->pcdata->immtalk_text[0] = bright;
    ch->pcdata->savantalk[0] = bright;
    ch->pcdata->savantalk_text[0] = bright;
    ch->pcdata->shouts[0] = bright;
    ch->pcdata->shouts_text[0] = bright;
    ch->pcdata->tells[0] = bright;
    ch->pcdata->tell_text[0] = bright;
    ch->pcdata->info[0] = bright;
    ch->pcdata->info_text[0] = bright;
    ch->pcdata->gametalk[0] = bright;
    ch->pcdata->gametalk_text[0] = bright;
    ch->pcdata->bitch[0] = bright;
    ch->pcdata->bitch_text[0] = bright;
    ch->pcdata->newbie[0] = bright;
    ch->pcdata->newbie_text[0] = bright;
    ch->pcdata->clan[0] = bright;
    ch->pcdata->clan_text[0] = bright;
    ch->pcdata->say[0] = bright;
    ch->pcdata->say_text[0] = bright;
    ch->pcdata->gtell[0] = bright;
    ch->pcdata->gtell_text[0] = bright;
    ch->pcdata->room_title[0] = bright;
    ch->pcdata->room_exits[0] = bright;
    ch->pcdata->wiznet[0] = bright;
    ch->pcdata->warfare[0] = bright;
    ch->pcdata->warfare_text[0] = bright;
    ch->pcdata->proclaim[0] = bright;
    ch->pcdata->proclaim_text[0] = bright;
    
    sprintf( buf, "All Colour settings set to %s.\n\r", buf2 );
    send_to_char_bw( buf, ch );
    
    return;
}
