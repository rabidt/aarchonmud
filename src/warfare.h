/* warfare stuff by Viper, installed by Siva 9/30/98 */
#ifndef WARFARE_H
#define WARFARE_H
 
struct war_data
{
   bool on;
   bool started;
   int type;
   int min_level;
   int max_level;
   int war_time_left;
   int combatants;
   int cost;
   long owner;
   long duel_target;
   CHAR_DATA *first_combatant;
   int total_combatants;
};

typedef struct war_data WAR_DATA;

extern WAR_DATA war;

DECLARE_DO_FUN(do_startwar);
DECLARE_DO_FUN(do_stopwar);
DECLARE_DO_FUN(do_combat);
DECLARE_DO_FUN(do_warstatus);
DECLARE_DO_FUN(do_warsit);
DECLARE_DO_FUN(do_nowar);

void war_update( void );
void warfare_to_all( const char *argument );
void add_war_kills( CHAR_DATA *ch );
void check_war_win( void );
void war_remove( CHAR_DATA *ch, bool killed );
bool is_same_team( CHAR_DATA *ch1, CHAR_DATA *ch2 );
void war_end( bool success );
bool in_religion_war( CHAR_DATA *ch );
void proc_startwar( CHAR_DATA *ch, const char *argument, bool pay );
void proc_startduel( CHAR_DATA *ch, const char *argument);

/* Warfare system definitions - Viper */
#define ARMAGEDDON_WAR  0
#define RACE_WAR        1
#define CLASS_WAR       2
#define CLAN_WAR        3
#define GENDER_WAR      4
#define RELIGION_WAR    5
#define DUEL_WAR        6

/* you can change these all to free vnum */
#define WAR_ROOM_PREP   16299
#define WAR_ROOM_FIRST  16200
#define WAR_ROOM_LAST   16296
#define DUEL_ROOM_FIRST 20181
#define DUEL_ROOM_LAST  20189
#define WAR_ROOM_LOSER  16298
#define WAR_ROOM_WINNER 16297


#endif // WARFARE_H
