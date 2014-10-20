/* 
 * Religion Code
 * by Henning Koehler <koehlerh@in.tum.de>
 */

#ifndef RELIGION_H
#define RELIGION_H

/* merc.h
//typedef struct religion_data RELIGION_DATA;
//typedef struct religion_war_data RELIGION_WAR_DATA;
//typedef struct follower_data FOLLOWER_DATA;
//typedef struct religion_rank_data RELIGION_RANK_DATA;
*/

#define MAX_PRIEST 4

//#define RELIGION_WAR_PEACE 0
//#define RELIGION_WAR_WAR   1

#define RELIGION_MAX_RANK  6
#define RELIGION_RANK_NEO  0

struct religion_data
{
    RELIGION_DATA *next;
    const char *name;
    int ID;
    int min_align, max_align;
    int altar_room_vnum;
    int guard_vnum;
    OBJ_DATA *relic_obj;
    int relic_vnum;
    int relic_room_vnum; /* used to restore relic after crash */
    int relic_bonus; /* current bonus the religion gets */
    int god_power; /* favor points */
//    RELIGION_WAR_DATA *war_status;
    const char *god;
    const char *priest[MAX_PRIEST];
    FOLLOWER_DATA *follower;
    int conserve_at; /* if god_power < conserve_at, no mortal prayers are answered */
};

//struct religion_war_data
//{
//    RELIGION_WAR_DATA *next;
//    RELIGION_DATA *opp;
//    int opp_ID; /* for bootup loading only */
//    int status;
//};

struct follower_data
{
    FOLLOWER_DATA *next;
    const char *name;
    RELIGION_DATA *religion;
    time_t join_time;
    int faith;
    int favour;
};

struct religion_rank_data
{
    int min_time;
    int min_faith;
    const char *name;
};

struct prayer_data
{
    int prayer_num;
    CHAR_DATA *victim;
    int ticks;
};

typedef void RELIGION_FUN( RELIGION_DATA* religion );

/* god functions */
#define DECLARE_GOD_FUNCTION(name) bool name( CHAR_DATA *ch, CHAR_DATA *victim, const char *god_name, sh_int duration );
DECLARE_GOD_FUNCTION(god_bless)
DECLARE_GOD_FUNCTION(god_curse)
DECLARE_GOD_FUNCTION(god_heal)
DECLARE_GOD_FUNCTION(god_cleanse)
DECLARE_GOD_FUNCTION(god_defy)
DECLARE_GOD_FUNCTION(god_speed)
DECLARE_GOD_FUNCTION(god_slow)
DECLARE_GOD_FUNCTION(god_enlighten)
DECLARE_GOD_FUNCTION(god_protect)
DECLARE_GOD_FUNCTION(god_fortune)
DECLARE_GOD_FUNCTION(god_haunt)
DECLARE_GOD_FUNCTION(god_plague)
DECLARE_GOD_FUNCTION(god_confuse)
#undef DECLARE_GOD_FUNCTION

/* methods for religion_data */

RELIGION_DATA* new_religion();
void free_religion( RELIGION_DATA* religion );
//void religion_save_to_file( RELIGION_DATA *religion, FILE *fp );
void religion_save_to_buffer( RELIGION_DATA *religion, DBUFFER *fp );
RELIGION_DATA* religion_load_from_file( FILE *fp );
void religion_add_follower( RELIGION_DATA *religion, CHAR_DATA *ch );
void religion_remove_follower( CHAR_DATA *ch );
FOLLOWER_DATA* religion_get_follower( RELIGION_DATA *religion, const char *name );

void religion_check_priest_exist( RELIGION_DATA *religion );
/* RELIGION_FUN functions - to be used with all_religions */
void religion_update_priests( RELIGION_DATA *religion );
void religion_update_followers( RELIGION_DATA *religion );
void religion_restore_relic( RELIGION_DATA *religion );
void religion_create_relic( RELIGION_DATA *religion );
void religion_relic_damage( RELIGION_DATA *religion );

//int religion_get_war_status( RELIGION_DATA *religion, RELIGION_DATA *opp );
//void religion_set_war_status( RELIGION_DATA *religion, RELIGION_DATA *opp, int status );
//void religion_remove_war_status( RELIGION_DATA *religion, RELIGION_DATA *opp );

/* New by Quirky: Aug 2003 */
void grant_prayer( CHAR_DATA *ch );

/* methods for the list */

MEMFILE* save_religions();
void load_religions();
void add_religion( RELIGION_DATA *religion );
void remove_religion( RELIGION_DATA *religion );
RELIGION_DATA* get_religion_by_name( const char *name );
RELIGION_DATA* get_religion_by_ID( int ID );
void update_relic_bonus();
//void assign_religion_war_opp();
FOLLOWER_DATA* get_religion_follower_data( const char *name );
void all_religions( RELIGION_FUN *rel_fun );
/*
void update_priests();
void update_followers();
void relic_damage();
void create_relics();
*/
/* methods for religion_war_data */
//RELIGION_WAR_DATA* new_religion_war( RELIGION_DATA *opp, int status );
//void free_religion_war( RELIGION_WAR_DATA *war );
//void free_religion_war_list( RELIGION_WAR_DATA *war );
//void religion_war_save_to_file( RELIGION_WAR_DATA *war, FILE *fp );
//void religion_war_save_to_buffer( RELIGION_WAR_DATA *war, DBUFFER *fp );
//RELIGION_WAR_DATA* religion_war_load_from_file( FILE *fp );

/* methods for follower_data */
FOLLOWER_DATA* new_follower( RELIGION_DATA *religion, const char *name );
void free_follower( FOLLOWER_DATA *fol );
void free_follower_list( FOLLOWER_DATA *list );
//void follower_save_to_file( FOLLOWER_DATA *list, FILE *fp );
void follower_save_to_buffer( FOLLOWER_DATA *list, DBUFFER *fp );
FOLLOWER_DATA* follower_load_from_file( RELIGION_DATA *religion, FILE *fp );
int follower_get_rank( FOLLOWER_DATA *fol );
bool follower_is_priest( FOLLOWER_DATA *fol );

/* general methods concerning religion */

RELIGION_DATA *get_religion( CHAR_DATA *ch );
bool is_priest( CHAR_DATA *ch );
bool is_high_priest( CHAR_DATA *ch );
bool is_religion_opp( CHAR_DATA *ch, CHAR_DATA *opp );
bool carries_relic( CHAR_DATA *ch );
void check_religion_align( CHAR_DATA *ch );
void remove_priest( CHAR_DATA *ch );
RELIGION_DATA *get_religion_of_altar( ROOM_INDEX_DATA *room );
RELIGION_DATA *get_religion_of_guard( CHAR_DATA *guard );
const char* get_religion_rank_name( int rank );
const char* get_ch_rank_name( CHAR_DATA *ch );
int get_religion_bonus( CHAR_DATA *ch );
void gain_faith( CHAR_DATA *ch, int gain );
const char* get_god_name( CHAR_DATA *ch );
int get_faith( CHAR_DATA *ch );
int get_favour( CHAR_DATA *ch );
bool is_relic_obj( OBJ_DATA *obj );
void free_relic( OBJ_DATA *obj );
bool is_religion_member( CHAR_DATA *ch );
double adjust_align_change( CHAR_DATA *ch, double change );
DECLARE_DO_FUN(do_religion);

#endif
