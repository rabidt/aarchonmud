#ifndef LEADERBOARD_H
#define LEADERBOARD_H

//#define LBOARD_DEBUG

#define MAX_DISPLAY_ENTRY 20
#define MAX_LBOARD_RESULT 10
#define RESULT_NUM_RANK		3

typedef struct lboard_entry LBOARD_ENTRY;
typedef struct lboard LBOARD;
typedef struct lboard_result LBOARD_RESULT;
typedef struct lboard_table_entry LBOARD_TABLE_ENTRY;

extern time_t daily_reset;
extern time_t weekly_reset;
extern time_t monthly_reset;

extern LBOARD *daily_results;
extern LBOARD *weekly_results;
extern LBOARD *monthly_results;

struct lboard_table_entry
{
	bool enabled;
	LBOARD *board;
};

struct lboard
{
    char *board_name;
	LBOARD_ENTRY *head;
    LBOARD_ENTRY *tail;
};

struct lboard_entry
{
    LBOARD_ENTRY *previous;
    LBOARD_ENTRY *next;
    char *name;
    int value;
	int rank;
};

struct lboard_result
{
	LBOARD_RESULT *next;
	time_t end_time;
	char *text;
};


LBOARD_ENTRY *find_in_lboard( LBOARD **board, char *name );
LBOARD *add_to_lboard( LBOARD **board, char *name, int increment );
void update_lboard( int lboard_type, CHAR_DATA *ch, int current, int increment );
LBOARD_ENTRY *lboard_entry_new( char *name, int value );
void do_lboard( CHAR_DATA *ch, char *argument);
LBOARD* lboard_load_from_file( FILE *fp, int type );
void reset_daily_lboards();
void reset_weekly_lboards();
void reset_monthly_lboards();
void reset_lboard( LBOARD **board);

#endif
