/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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

#ifndef TABLES_H
#define TABLES_H

/* other tables */
extern const char* spell_target_names[];
extern const char* spell_duration_names[];

struct flag_type
{
    char *name;
    int bit;
    bool settable;
};


struct penalty_type
{
    char *name;                                /* "nochannel", "noemote", "freeze", etc. */
    long bit;                                  /* flag bit to assign to player */
    char *apply_string;                        /* "no-channelled", "no-emoted", "frozen", etc. */
    long sev_duration[MAX_PENALTY_SEVERITY];   /* duration of penalty at each severity (-1 = specify, -2 = infinite)*/
    sh_int sev_points[MAX_PENALTY_SEVERITY];   /* demerit points assigned for various severities (-1 = specify) */
    sh_int sev_level[MAX_PENALTY_SEVERITY];    /* minimum level to impose this severity */
};

struct stat_type
{
	char *name;
	char *abbreviation;
	int stat;
	int dice[MAX_STATS];
};

struct position_type
{
    char *name;
    char *short_name;
};

struct sex_type
{
    char *name;
};

struct size_type
{
    char *name;
};

struct bit_type
{
	const	struct flag_type * table;
	char * help;
};


struct pkgrade_type
{
    char *grade;
    int pkpoints;
    int earned;
    int lost;
    int lost_in_warfare;
};

/* flag tables */
extern  const   struct  flag_type       act_flags[];
extern  const   struct  flag_type       plr_flags[];
extern  const   struct  flag_type       affect_flags[];
extern  const   struct  flag_type       off_flags[];
extern  const   struct  flag_type       imm_flags[];
extern  const   struct  flag_type       form_flags[];
extern  const   struct  flag_type       part_flags[];
extern  const   struct  flag_type       comm_flags[];
extern  const   struct  flag_type       extra_flags[];
extern  const   struct  flag_type       wear_types[];
extern  const   struct  flag_type       weapon_flags[];
extern  const   struct  flag_type       container_flags[];
extern  const   struct  flag_type       portal_flags[];
extern  const   struct  flag_type       room_flags[];
extern  const   struct  flag_type       exit_flags[];
extern  const   struct  flag_type       mprog_flags[];
extern  const   struct  flag_type       oprog_flags[];
extern  const   struct  flag_type       aprog_flags[];
extern  const   struct  flag_type       rprog_flags[];
extern  const   struct  flag_type       area_flags[];
extern  const   struct  flag_type       sector_flags[];
extern  const   struct  flag_type       door_resets[];
extern  const   struct  flag_type       wear_loc_strings[];
extern  const   struct  flag_type       wear_loc_flags[];
extern  const   struct  flag_type       res_flags[];
extern  const   struct  flag_type       imm_flags[];
extern  const   struct  flag_type       vuln_flags[];
extern  const   struct  flag_type       type_flags[];
extern  const   struct  flag_type       apply_flags[];
extern  const   struct  flag_type       sex_flags[];
extern  const   struct  flag_type       furniture_flags[];
extern  const   struct  flag_type       weapon_class[];
extern  const   struct  flag_type       apply_types[];
extern  const   struct  flag_type       weapon_type2[];
extern  const   struct  flag_type       damage_type[];
extern  const   struct  flag_type       apply_types[];
extern  const   struct  flag_type       size_flags[];
extern  const   struct  flag_type       position_flags[];
extern  const   struct  flag_type       ac_type[];
extern  const   struct  bit_type        bitvector_type[];
extern  const   struct  flag_type       con_states[];

/* game tables */

extern  const   struct  stat_type       stat_table[];
extern  const   struct  position_type   position_table[];
extern  const   struct  sex_type        sex_table[];
extern  const   struct  size_type       size_table[];
extern  const   struct  penalty_type penalty_table[];
extern  const   struct  pkgrade_type    pkgrade_table[];

extern const ACHIEVEMENT achievement_table [];

/* functions */
bool is_settable( int flag, const struct flag_type *flag_table );

#endif
