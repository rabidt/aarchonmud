#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#define LBOARD_DEBUG

#define MAX_DISPLAY_ENTRY 20
#define MAX_LBOARD_RESULT 10
#define RESULT_NUM_RANK    3

typedef struct lboard_entry LBOARD_ENTRY;
typedef struct lboard LBOARD;
typedef struct lboard_result LBOARD_RESULT;
typedef struct lboard_table_entry LBOARD_TABLE_ENTRY;
typedef struct lb_tables_entry LB_TABLES_ENTRY;


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

struct lb_tables_entry
{
    sh_int *sn;
    time_t reset;
    LBOARD_RESULT *result;
    char *keyword;
    time_t (*reset_fun)();
    LBOARD_TABLE_ENTRY table[MAX_LBOARD];
};
    


LBOARD_ENTRY *find_in_lboard( LBOARD **board, char *name );
LBOARD *add_to_lboard( LBOARD **board, char *name, int increment );
void update_lboard( int lboard_type, CHAR_DATA *ch, int current, int increment );
LBOARD_ENTRY *lboard_entry_new( char *name, int value );
void do_lboard( CHAR_DATA *ch, char *argument);
LBOARD* lboard_load_from_file( FILE *fp, int type );
void reset_lboard( LBOARD **board);
void lboard_init();

#endif
