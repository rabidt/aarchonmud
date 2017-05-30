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
#ifndef RECYCLE_H
#define RECYCLE_H

/* externs */
extern char str_empty[1];
extern int mobile_count;
extern int object_count;

/* stuff for providing a crash-proof buffer */

/* Changed to reflect the max value of a short integer. Not sure what affects
   this will have our on memory usage, but it allows paging of longer files like
   the output of alist - Astark 12-21-12 */

/* Changed this from 10 to 11 to reflect the addition of 32767 in
   recycle.c - Astark 12-21-12 */

#define MAX_BUF		32767
#define MAX_BUF_LIST 	11
#define BASE_BUF 	1024

/* valid states */
#define BUFFER_SAFE	0
#define BUFFER_OVERFLOW	1
#define BUFFER_FREED 	2

/* note recycling */
#define ND NOTE_DATA
ND	*new_note args( (void) );
void	free_note args( (NOTE_DATA *note) );
#undef ND

/* ban data recycling */
#define BD BAN_DATA
BD	*new_ban args( (void) );
void	free_ban args( (BAN_DATA *ban) );
#undef BD

/* wizlist data recycling */
#define WD WIZ_DATA
WD  *new_wiz args( (void) );
void    free_wiz args( (WIZ_DATA *ban) );
#undef WD

/* crime list data recycling */
#define CRD CRIME_DATA
CRD  *new_crime args( (void) );
void    free_crime args( (CRIME_DATA *crime) );
#undef CRD

/* descriptor recycling */
//#define DD DESCRIPTOR_DATA
//DD	*new_descriptor args( (void) );
DESCRIPTOR_DATA	*new_descriptor args( (void) );
void	free_descriptor args( (DESCRIPTOR_DATA *d) );
//#undef DD

/* char gen data recycling */
#define GD GEN_DATA
GD 	*new_gen_data args( (void) );
void	free_gen_data args( (GEN_DATA * gen) );
#undef GD

/* extra descr recycling */
#define ED EXTRA_DESCR_DATA
ED	*new_extra_descr args( (void) );
void	free_extra_descr args( (EXTRA_DESCR_DATA *ed) );
#undef ED

/* affect recycling */
#define AD AFFECT_DATA
AD	*new_affect args( (void) );
void	free_affect args( (AFFECT_DATA *af) );
#undef AD

/* object recycling */
#define OD OBJ_DATA
OD	*new_obj args( (void) );
void	free_obj args( (OBJ_DATA *obj) );
#undef OD

/* character recyling */
#define CD CHAR_DATA
#define PD PC_DATA
CD	*new_char args( (void) );
void	free_char args( (CHAR_DATA *ch) );
PD	*new_pcdata args( (void) );
void	free_pcdata args( (PC_DATA *pcdata) );
#undef PD
#undef CD

/* portal recycling */
PORTAL_DATA *new_portal( void );
void free_portal( PORTAL_DATA *portal );

/* quest recycling */
QUEST_DATA *new_quest( void );
void free_quest( QUEST_DATA *quest );

/* mob id and memory procedures */
#define MD MEM_DATA
long 	get_pc_id args( (void) );
long	get_mob_id args( (void) );
MD	*new_mem_data args( (void) );
void	free_mem_data args( ( MEM_DATA *memory) );
MD	*find_memory args( (MEM_DATA *memory, long id) );
#undef MD

/* buffer procedures */
HELP_AREA  * new_had		  args( (void) );
HELP_DATA  * new_help	  args( (void) );
SORT_TABLE * new_sort     args( (void) );
void		    free_sort    args( (SORT_TABLE * sort) );

#endif // RECYCLE_H