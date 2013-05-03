/***************************************************************************
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
 *  around, comes around.    *
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

#include <lua.h>

/* change this value to 0 for running on darkhorse */
#define SOCR 1

#if defined( macintosh ) || defined( WIN32 )
#define NOCRYPT
#if defined( unix )
#undef  unix
#endif
#endif


#ifdef TESTER
#define FSTAT
#endif
/* debugging macros */
/* #define SIM_DEBUG */
/* #define FLUSH_DEBUG */
/* #define FLAG_DEBUG */
/* #define ASSERT_DEBUG */
/* #define MPROG_DEBUG */
#define BOX_LOG
#define SMITH_LOG

#ifdef ASSERT_DEBUG
#include <assert.h>
#endif

/*
 * Accommodate old non-Ansi compilers.
 */
#if defined(TRADITIONAL)
#define const
#define args( list )            ( )
#define DECLARE_DO_FUN( fun )       void fun( )
#define DECLARE_SPEC_FUN( fun )     bool fun( )
#define DECLARE_SPELL_FUN( fun )    void fun( )
#define DECLARE_SONG_FUN( fun)  bool fun( )
#else
#define args( list )            list
#define DECLARE_DO_FUN( fun )       DO_FUN    fun
#define DECLARE_SPEC_FUN( fun )     SPEC_FUN  fun
#define DECLARE_SPELL_FUN( fun )    SPELL_FUN fun
#define DECLARE_SONG_FUN( fun ) SONG_FUN fun
#endif

/* system calls */
int unlink();
int system();



/*
 * Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
#if !defined(FALSE)
#define FALSE    0
#endif

#if !defined(TRUE)
#define TRUE     1
#endif

#if defined(_AIX)
#if !defined(const)
#define const
#endif
typedef int             sh_int;
typedef int             bool;
#define unix
#else
typedef short   int         sh_int;
typedef unsigned char           bool;
#endif



/*
 * Structure types.
 */
typedef struct  affect_data      AFFECT_DATA;
typedef struct  area_data        AREA_DATA;
typedef struct  ban_data         BAN_DATA;
typedef struct  buf_type         BUFFER;
typedef struct  string_ring_buf  SR_BUF;
typedef struct  char_data        CHAR_DATA;
typedef struct  descriptor_data  DESCRIPTOR_DATA;
typedef struct  exit_data        EXIT_DATA;
typedef struct  extra_descr_data EXTRA_DESCR_DATA;
typedef struct  help_data        HELP_DATA;
typedef struct  help_area_data   HELP_AREA;
typedef struct  kill_data        KILL_DATA;
typedef struct  mem_data         MEM_DATA;
typedef struct  mob_index_data_old MOB_INDEX_DATA_OLD;
typedef struct  mob_index_data   MOB_INDEX_DATA;
typedef struct  note_data        NOTE_DATA;
typedef struct  obj_data         OBJ_DATA;
typedef struct  obj_index_data   OBJ_INDEX_DATA;
typedef struct  pc_data          PC_DATA;
typedef struct  gen_data         GEN_DATA;
typedef struct  reset_data       RESET_DATA;
typedef struct  room_index_data  ROOM_INDEX_DATA;
typedef struct  shop_data        SHOP_DATA;
typedef struct  time_info_data   TIME_INFO_DATA;
typedef struct  weather_data     WEATHER_DATA;
typedef struct  mprog_list       MPROG_LIST;
typedef struct  mprog_code       MPROG_CODE;
typedef struct  sort_table       SORT_TABLE;
typedef struct  disabled_data    DISABLED_DATA;
typedef struct  clanwar_data     CLANWAR_DATA;
typedef struct  board_data       BOARD_DATA;
typedef struct  colour_data      COLOUR_DATA;
typedef struct  penalty_data     PENALTY_DATA;
typedef struct  crime_data       CRIME_DATA;
typedef struct  reserved_data    RESERVED_DATA;
typedef struct  auth_list        AUTH_LIST;
typedef struct  grant_data       GRANT_DATA;
typedef struct	wiz_data         WIZ_DATA;
typedef struct  clan_rank_data   CLAN_RANK_DATA;
typedef struct  clan_data        CLAN_DATA;
typedef struct  quest_data       QUEST_DATA;
typedef struct  portal_data      PORTAL_DATA;
typedef struct  achievement_entry ACHIEVEMENT;
/* religion */
typedef struct religion_data RELIGION_DATA;
/* typedef struct religion_war_data RELIGION_WAR_DATA; */
typedef struct follower_data FOLLOWER_DATA;
typedef struct religion_rank_data RELIGION_RANK_DATA;
typedef struct prayer_data PRAYER_DATA;
/* from buffer_util.h, moved here: */
typedef struct mem_file_type MEMFILE;

/*
 * Function types.
 */
typedef void DO_FUN args( ( CHAR_DATA *ch, char *argument ) );
typedef bool SPEC_FUN   args( ( CHAR_DATA *ch ) );
typedef void SPELL_FUN  args( ( int sn, int level, CHAR_DATA *ch, void *vo,
				int target ) );
/* for object extracting in handler.c */
typedef bool OBJ_CHECK_FUN( OBJ_DATA *obj );

typedef struct comm_history_entry COMM_ENTRY;
typedef struct comm_history_type COMM_HISTORY;
typedef struct pers_comm_entry PERS_ENTRY;
typedef struct pers_comm_history PERS_HISTORY;

struct pers_comm_entry
{
	PERS_ENTRY *next;
	PERS_ENTRY *prev;
	char *text;
};

struct pers_comm_history
{
	sh_int size;
	PERS_ENTRY *head;
	PERS_ENTRY *tail;
};
	
struct comm_history_entry
{
    COMM_ENTRY *next;
    COMM_ENTRY *prev;

    char *timestamp;
    sh_int channel;
    char *text;
    bool invis;
    char *mimic_name;
    char *name;
};

struct comm_history_type
{
    sh_int size;
    COMM_ENTRY *head; /* most recent */
    COMM_ENTRY *tail; /* oldest */
};


typedef bool CHAN_CHECK args( ( CHAR_DATA *ch) );
typedef struct channel_type
{
	sh_int *psn;
	char *name;
	char *first_pers;
	char *third_pers;
	char prime_color;
	char second_color;
	sh_int offbit;
	sh_int min_level;
	CHAN_CHECK *check; /*pointer to special check func*/
} CHANNEL;
extern const CHANNEL public_channel_table[];
bool check_savant( CHAR_DATA *ch );
bool check_immtalk( CHAR_DATA *ch );

bool is_remort_obj( OBJ_DATA *obj );
bool is_sticky_obj( OBJ_DATA *obj );
bool is_drop_obj( OBJ_DATA *obj );



/*
 * String and memory management parameters.
 */

/* 
 * Increased MSL to 10000 from 4096.. allows us to see long pages of
 * information like vlist obj in bastion, and grep obj world extra 
 * remort. - Astark 1-3-13 
 */

#define MAX_KEY_HASH         1024
#define MAX_STRING_LENGTH    10000
#define MAX_INPUT_LENGTH      300
#define PAGELEN                33
#define MAX_MEM_LIST           16


/* I am lazy :) */
#define MSL MAX_STRING_LENGTH
#define MIL MAX_INPUT_LENGTH

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SKILL         418
#define MAX_GROUP          76 /* accurate jan 2013 */
#define MAX_IN_GROUP       15
#define MAX_ALIAS          35
#define MAX_CLASS          15
#define MAX_PC_RACE        65 /*accurate jan 2013 */
#define MAX_BOARD          12
#define MAX_CLAN           12
#define MAX_CLAN_RANK      13
#define MAX_PENALTY         8
#define MAX_PENALTY_SEVERITY 5
#define MAX_JAIL_ROOM      50 /* Max count of total jail rooms in MUD */
#define MAX_FORGET          5
#define MAX_DAMAGE_MESSAGE  102
#define MAX_AREA_CLONE     10
#define MAX_LEVEL          110
#define MAX_STORAGE_BOX	   5
#define MAX_QUOTES         22 /* This must equal the # of quotes you have */
#define MAX_CP						 60
#define LEVEL_IMMORTAL     (MAX_LEVEL - 9)
#define LEVEL_HERO         (MAX_LEVEL - 10)
#define LEVEL_MIN_HERO     (MAX_LEVEL - 20)
#define LEVEL_UNAUTHED      5 /* Max level an unauthed newbie can gain */

#define PULSE_PER_SECOND        4
#define PULSE_VIOLENCE        ( 3 * PULSE_PER_SECOND)
#define PULSE_MOBILE          ( 4 * PULSE_PER_SECOND)
#define PULSE_MOBILE_SPECIAL  ( 1 * PULSE_PER_SECOND)
#define PULSE_MUSIC       ( 6 * PULSE_PER_SECOND)
#define PULSE_TICK        (30 * PULSE_PER_SECOND)
#define PULSE_AREA        (120 * PULSE_PER_SECOND)
#define PULSE_SAVE            ( 2 * PULSE_PER_SECOND )
#define PULSE_HERB            ( 15 * 60 * PULSE_PER_SECOND )
#define PULSE_PER_MINUTE	( 60 * PULSE_PER_SECOND )
/* #define PULSE_HERB            ( 15 * PULSE_PER_SECOND ) */

/* times */
#define MINUTE 60
#define HOUR 3600
#define DAY (24*HOUR)
#define WEEK (7*DAY)
#define MONTH (30*DAY)
#define YEAR (365*DAY)

#define PK_EXPIRE_DAYS 90

/* maximum current remort level - update when adding new remorts */
#ifdef TESTER
#define MAX_REMORT 8
#else
#define MAX_REMORT 7 
#endif
/* version numbers for downward compatibility
 */
#define CURR_AREA_VERSION 1

/*#define CREATOR         (MAX_LEVEL - 1)
#define SUPREME         (MAX_LEVEL - 2)
#define DEITY           (MAX_LEVEL - 3)
#define GOD         (MAX_LEVEL - 4)
#define IMMORTAL        (MAX_LEVEL - 5)
#define DEMI            (MAX_LEVEL - 6)
#define ANGEL           (MAX_LEVEL - 7)
#define AVATAR          (MAX_LEVEL - 8)
#define BUILDER     (MAX_LEVEL - 9)*/

#define IMPLEMENTOR  MAX_LEVEL
#define ARCHON       MAX_LEVEL - 2
#define VICEARCHON   MAX_LEVEL - 4
#define GOD          MAX_LEVEL - 6
#define DEMIGOD      MAX_LEVEL - 8
#define SAVANT       MAX_LEVEL - 9

#define HERO         LEVEL_HERO

/* for command types */
#define ML  MAX_LEVEL   /* implementor */
#define L1  MAX_LEVEL - 1   /* creator */
#define L2  MAX_LEVEL - 2   /* supreme being */
#define L3  MAX_LEVEL - 3   /* deity */
#define L4  MAX_LEVEL - 4   /* god */
#define L5  MAX_LEVEL - 5   /* immortal */
#define L6  MAX_LEVEL - 6   /* demigod */
#define L7  MAX_LEVEL - 7   /* angel */
#define L8  MAX_LEVEL - 8   /* avatar */
#define L9  MAX_LEVEL - 9   /* builder */
#define IM  LEVEL_IMMORTAL  /* builder */
#define HE  LEVEL_HERO  /* hero */

/*
 * Colour stuff by Lope of Loping Through The MUD
 */
#define CLEAR      "[0m"      /* Resets Colour    */
#define C_RED      "[0;31m"   /* Normal Colours   */
#define C_GREEN        "[0;32m"
#define C_YELLOW   "[0;33m"
#define C_BLUE     "[0;34m"
#define C_MAGENTA  "[0;35m"
#define C_CYAN     "[0;36m"
#define C_WHITE        "[0;37m"
#define C_D_GREY   "[1;30m"   /* Light Colors     */
#define C_B_RED        "[1;31m"
#define C_B_GREEN  "[1;32m"
#define C_B_YELLOW "[1;33m"
#define C_B_BLUE   "[1;34m"
#define C_B_MAGENTA    "[1;35m"
#define C_B_CYAN   "[1;36m"
#define C_B_WHITE  "[1;37m"
#define BOLD       "[1m"
#define BLINK      "[5m"
#define REVERSE    "[7m"

#define COLOUR_NONE 8
#define RED         1
#define GREEN       2
#define YELLOW      3
#define BLUE        4
#define MAGENTA     5
#define CYAN        6
#define WHITE       7
#define BLACK       0

#define NORMAL      0
#define BRIGHT      1


/*
 * Site ban structure.
 */

#define BAN_SUFFIX      A
#define BAN_PREFIX      B
#define BAN_NEWBIES     C
#define BAN_ALL         D   
#define BAN_PERMIT      E
#define BAN_PERMANENT   F

/*
 * structure and methods for flag handling
 * implemented in tflag.c
 */

/* FLAG_MAX_BYTE must be bigger than highest number of racial flags */
/* FLAG_MAX_BIT must be bigger than highest affect number */
#define FLAG_MAX_BYTE          32
#define FLAG_MAX_BIT           (8 * FLAG_MAX_BYTE) 
typedef char tflag[FLAG_MAX_BYTE];
typedef char msl_string[MSL];

typedef struct smith_data 
{
    OBJ_DATA *old_obj;
    OBJ_DATA *new_obj;
} SMITH_DATA;

typedef struct explore_holder
{	
	struct explore_holder *next;
	unsigned int bits;
	int mask;
} EXPLORE_HOLDER;

typedef struct exploration_data
{	
	EXPLORE_HOLDER *buckets;
	int set;
} EXPLORE_DATA;



struct  ban_data
{
	BAN_DATA *  next;
	bool    valid;
	tflag   ban_flags;
	sh_int  level;
	char *  name;
};

struct	wiz_data
{
    WIZ_DATA *	next;
    bool	valid;
    sh_int	level;
    char *	name;
};

struct struckdrunk
{
		int     min_drunk_level;
		int     number_of_rep;
		char    *replacement[11];
};

struct buf_type
{
	BUFFER *    next;
	bool        valid;
	sh_int      state;  /* error state of the buffer */
	sh_int      size;   /* size in k */
	char *      string; /* buffer's string */
};

/* Erwin's dynamic buffer system. */
typedef struct buffer_type DBUFFER;

struct buffer_type
{
	char *data; /* The data */
	
	int len;	/* The current len of the buffer */
	int size;	/* The allocated size of data */
	
	bool overflowed; /* Is the buffer overflowed? */
};


struct sort_table
{
	SORT_TABLE *    prev;
	SORT_TABLE *    next;
	CHAR_DATA  *    owner;
	int         score;
	bool            valid;
};

/* ring-buffer for methods returning pointers to local static strings
 * which must not overlap for several different calls
 */
#define MAX_STRING_RING_BUF 5
struct string_ring_buf
{
    int last_index;
    char buf[MAX_STRING_RING_BUF][MSL];
};

/*
 * Time and weather stuff.
 */
#define SUN_DARK            0
#define SUN_RISE            1
#define SUN_LIGHT           2
#define SUN_SET             3

#define SKY_CLOUDLESS       0
#define SKY_CLOUDY          1
#define SKY_RAINING         2
#define SKY_LIGHTNING       3

struct  time_info_data
{
	int     hour;
	int     day;
	int     month;
	int     year;
};

struct  weather_data
{
	int     mmhg;
	int     change;
	int     sky;
	int     sunlight;
};


/*
 * Reserved names structure.
 */
struct	reserved_data
{
    RESERVED_DATA *next;
    RESERVED_DATA *prev;
    char *name;
};


/* Name authorization */
#define AUTH_STATUS_DISABLED 0
#define AUTH_STATUS_ENABLED  1
#define AUTH_STATUS_IMM_ON   2

typedef enum 
{ 
   AUTH_ONLINE = 0, 
   AUTH_OFFLINE, 
   AUTH_LINK_DEAD, 
   AUTH_CHANGE_NAME,
   AUTH_DENIED, 
   AUTH_AUTHED 
} auth_types;

struct auth_list
{
   char  * name;           /* Name of character awaiting authorization */
   int   state;            /* Current state of authed */
   char  * authed_by;	   /* Name of immortal who authorized the name */
   char  * change_by;	   /* Name of immortal requesting name change */
   char  * denied_by;      /* Name of immortal who denied the name */
   AUTH_LIST *next;
   AUTH_LIST *prev;
};

/* Grant */
struct grant_data
{
    GRANT_DATA *        next;
    DO_FUN *            do_fun;
    char *              name;
    int                 duration;
    int                 level;
};



/* Clanwars.  -Rimbol */
#define CLANWAR_PEACE  0
#define CLANWAR_WAR    1
#define CLANWAR_TRUCE  2
#define CLANWAR_TREATY 3

struct clanwar_data
{
   CLANWAR_DATA *next;
   sh_int clan_one;       /* Clan declaring war                        */
   sh_int clan_two;       /* Opposing clan                             */
   char *initiator_name;  /* Person initiating war                     */
   sh_int initiator_rank; /* Rank of person initiating war             */
   char *truce_name;      /* Person declaring a truce                  */
   int truce_timer;       /* Number of ticks left in truce             */
   sh_int status;         /* Status of war                             */
   int pkills;            /* Number of pkills by this clan during war  */
};


/* Clan data.  -Rim 1/2000 */

#define CLAN_STATUS_INACTIVE 0
#define CLAN_STATUS_ACTIVE   1

struct clan_rank_data
{
    char *   name;
    char *   who_name;
    sh_int   min_level;         /* Minimum level required to be promoted to this rank. */
    sh_int   max_promote_rank;  /* Maximum rank players of this rank may promote others to. */
    int      available_slots;   /* Number of players that may currently be promoted to this rank. */
    bool     clanwar_pkill;     /* Participation in clanwar pkill.   */
    bool     can_use_clantalk;  /* Ability to see clan channel.      */
    bool     can_note;          /* Ability to read/write clan notes. */
    bool     can_marry;         /* Ability to marry clan members.    */
    bool     can_warfare;       /* Ability to participate in warfare clan wars. */
    bool     can_declare_war;   /* Ability to declare clan wars      */
    bool     can_declare_truce; /* Ability to declare truces in clan wars */
    bool     can_declare_treaty;/* Ability to declare clan treaties  */
    bool     can_invite;        /* Ability to invite others to join the clan */
    bool     can_set_motd;      /* Ability to set the clan motd */
};

struct clan_data
{
    char *   name;
    char *   filename;
    bool     active;           
    bool     allow_recruits;    /* Whether or not players may currently recruit to this clan. */
    bool     invitation_only;   /* Is the clan invitation only? */
    char *   who_name;
    char *   who_color;
    char *   patron;            /* Patron name referenced in code. */
    char *   motd;		/* Special motd displayed for clan members. */
    sh_int   hall;              /* Recall room */
    sh_int   donation;          /* Clan donation room */
    time_t   creation_date;     /* Date clan created */
    sh_int   rank_count;        /* Number of ranks found at boot-time */
    CLAN_RANK_DATA  rank_list[MAX_CLAN_RANK]; /* Linked list of rank records specific to this clan */
    long     pkills;            /* Number of players pkilled by clan members */
    long     pdeaths;           /* Number of pkill deaths by clan members */
    long     mobkills;          /* Number of mobs killed by clan members */
    long     mobdeaths;         /* Number of mob deaths by clan members */
    sh_int   min_align;         /* Alignment threshold for membership */
    sh_int   max_align;         /* Alignment threshold for membership */
    bool     changed;           /* Flag to indicate whether a clan has changed and needs to be saved */
};




/* Linked list of disabled commands.  Each node describes one command. */
struct disabled_data
{
   DISABLED_DATA *next;            /* pointer to next node */
   sh_int spell;                   /* will be 1 if spell, 0 otherwise */
   char *command_name;             /* name of disabled command/spell */
   DO_FUN *do_fun;                 /* NULL if command is a spell */
   SPELL_FUN *spell_fun;           /* NULL if command is not a spell */
   char *disabled_by;              /* name of disabler */
   sh_int level;                   /* level of disabler */
};


/* List of crime categories. */
struct crime_data
{
   CRIME_DATA *next;
   char   *name;     /* Keyword */
   char   *desc;     /* Description of crime (used primarily for crime type list) */
   char   *imm_name; /* Immortal that convicted or forgave the player */
   time_t timestamp; /* Date/time player convicted or forgiven */
   bool   forgive;   /* 0 = conviction, 1 = forgiven.  Tracked for audit purposes. */
   bool   valid;
};


#define PENALTY_STATUS_NONE           0
#define PENALTY_STATUS_PENDING        1
#define PENALTY_STATUS_SERVING        2
#define PENALTY_STATUS_PAROLE_PENDING 3
#define PENALTY_STATUS_PARDON_PENDING 4
#define PENALTY_STATUS_PAROLED        5
#define PENALTY_STATUS_PARDONED       6
#define PENALTY_STATUS_COMPLETE       7


/* Immortal-imposed penalty data. */
struct penalty_data
{
    PENALTY_DATA *next;           /* Pointer to next node */

    sh_int status;                /* Pending, serving, paroled, pardoned, complete */

    char *victim_name;            /* Name of victim */
    sh_int victim_level;          /* Level of victim */

    char *imm_name;               /* Name of imm applying penalty */
    sh_int imm_level;             /* Level of imm applying penalty */
    long imposed_time;            /* Actual date/time penalty was imposed */

    char *penalty_type;           /* Type of penalty */
    sh_int severity;              /* Severity level */
    long start_time;              /* Start time for character to begin penalty (ch->played) */
    long duration;                /* End time for penalty (ch->played) */
    sh_int points;                /* # of penalty (demerit) points this imposes on player */

    int jail_room;                /* If this is a jail penalty, the jail room vnum */

    char *changed_by;             /* Name of character changing this record last */
    long changed_time;            /* Actual date/time of last change to this record */

    char *text;                   /* Used when building reason into note */
};


/*
 * Connected state for a channel.
 */
#define CON_PLAYING              0
#define CON_GET_NAME             1
#define CON_GET_OLD_PASSWORD     2
#define CON_CONFIRM_NEW_NAME     3
#define CON_GET_NEW_PASSWORD     4
#define CON_CONFIRM_NEW_PASSWORD 5
#define CON_GET_NEW_RACE         6
#define CON_GET_NEW_SEX          7
#define CON_GET_NEW_CLASS        8
#define CON_GET_ALIGNMENT        9
#define CON_DEFAULT_CHOICE      10 
#define CON_GEN_GROUPS          11 
#define CON_PICK_WEAPON         12
#define CON_READ_IMOTD          13
#define CON_READ_MOTD           14
#define CON_BREAK_CONNECT       15
#define CON_FTP_COMMAND         16
#define CON_FTP_DATA            17
#define CON_FTP_AUTH            18
#define CON_GET_CREATION_MODE   19
#define CON_ROLL_STATS          20
#define CON_GET_STAT_PRIORITY   21
#define CON_COPYOVER_RECOVER    22
#define CON_NOTE_TO             23
#define CON_NOTE_SUBJECT        24
#define CON_NOTE_EXPIRE         25
#define CON_NOTE_TEXT           26
#define CON_NOTE_FINISH         27
#define CON_PENALTY_SEVERITY    28
#define CON_PENALTY_CONFIRM     29
#define CON_PENALTY_HOURS       30
#define CON_PENALTY_POINTS      31
#define CON_PENALTY_PENLIST     32
#define CON_PENALTY_FINISH      33
#define CON_GET_COLOUR          34
#define CON_CLOSED              35
#define MAX_CON_STATE           36

#define CREATION_UNKNOWN         0
#define CREATION_INSTANT         1
#define CREATION_QUICK           2
#define CREATION_NORMAL          3
#define CREATION_REMORT          4


typedef enum { FTP_NORMAL, FTP_PUSH, FTP_PUSH_WAIT } ftp_mode;

/*
 * Descriptor (channel) structure.
 */
struct  descriptor_data
{
	DESCRIPTOR_DATA *   next;
	DESCRIPTOR_DATA *   snoop_by;
	CHAR_DATA *     character;
	CHAR_DATA *     original;
	bool        valid;
	char *      host;
	sh_int      descriptor;
	sh_int      connected;
	bool        fcommand;
	char        inbuf       [4 * MAX_INPUT_LENGTH];
	char        incomm      [MAX_INPUT_LENGTH];
	char        inlast      [MAX_INPUT_LENGTH];
        bool        last_msg_was_prompt;
	int         repeat;
	char *      outbuf;
	int         outsize;
	int         outtop;
	char *      showstr_head;
	char *      showstr_point;
	void *              pEdit;      /* OLC */
	char **             pString;    /* OLC */
	int         editor;     /* OLC */
	int     inactive;
	char *      username;
	struct
	{
		char *      filename;   /* Filename being written to */
		char *      data;       /* Data being written    */
		short int   lines_left; /* Lines left        */
		short int   lines_sent; /* Lines sent so far     */
		ftp_mode    mode;       /* FTP_xxx           */
	} ftp;
};


/*
 * TO types for act.
 */
#define TO_ROOM          0
#define TO_NOTVICT       1
#define TO_VICT          2
#define TO_CHAR          3
#define TO_ALL           4
#define TO_ROOM_UNAUTHED 5



/*
 * Help table types.
 */
struct  help_data
{
   HELP_DATA * next;
   HELP_DATA * next_area;
   sh_int  level;
   char *  keyword;
   char *  text;
   bool delete;    
};


struct help_area_data
{
   HELP_AREA *  next;
   HELP_DATA *  first;
   HELP_DATA *  last;
   AREA_DATA *  area;
   char * filename;
};


/*
 * Shop types.
 */
#define MAX_TRADE    5

struct  shop_data
{
	SHOP_DATA * next;           /* Next shop in list        */
	sh_int  keeper;         /* Vnum of shop keeper mob  */
	sh_int  buy_type [MAX_TRADE];   /* Item types shop will buy */
	sh_int  profit_buy;     /* Cost multiplier for buying   */
	sh_int  profit_sell;        /* Cost multiplier for selling  */
	sh_int  open_hour;      /* First opening hour       */
	sh_int  close_hour;     /* First closing hour       */
};



/*
 * Per-class stuff.
 */

#define MAX_GUILD   2
#define MAX_STATS   10
#define MAX_CURRSTAT 200
#define STAT_STR        0
#define STAT_CON        1
#define STAT_VIT        2
#define STAT_AGI        3
#define STAT_DEX        4
#define STAT_INT        5
#define STAT_WIS        6
#define STAT_DIS        7
#define STAT_CHA        8
#define STAT_LUC        9
#define STAT_NONE		MAX_STATS
  

struct  class_type
{
    char *  name;           /* the full name of the class */
    char    who_name    [4];    /* Three-letter name for 'who'  */
    sh_int  attr_prime;     /* Prime attribute      */
    sh_int  attr_second[2]; /* Secondary attributes  */
    sh_int  stat_priority[MAX_STATS-3];
    sh_int  weapon;         /* First weapon         */
    sh_int  guild[MAX_GUILD];   /* Vnum of guild rooms      */
    sh_int  skill_adept;        /* Maximum skill level      */
    sh_int  attack_factor;      /* replace the old thac0 values --Bobble */
    sh_int  defense_factor;
    sh_int  hp_gain;
    sh_int  mana_gain;          /* Class gains mana on level    */
    sh_int  move_gain;
    char *  base_group;     /* base skills gained       */
	char *  default_group;      /* default skills gained    */
};

struct item_type
{
	int     type;
	char *  name;
};

struct weapon_type
{
	char *  name;
	int  vnum;
	sh_int  type;
	sh_int  *gsn;
};

struct wiznet_type
{
	char *  name;
	long    flag;
	int     level;
};

struct attack_type
{
	char *  name;           /* name */
	char *  noun;           /* message */
	int     damage;         /* damage class */
};

struct race_type
{
	char *  name;           /* call name of the race */
	bool    pc_race;        /* can be chosen by pcs */
	tflag   act;            /* act bits for the race */
        tflag   affect_field;   /* aff bits for the race */
	tflag    off;            /* off bits for the race */
	tflag    imm;            /* imm bits for the race */
	tflag    res;            /* res bits for the race */
	tflag    vuln;           /* vuln bits for the race */
	tflag    form;           /* default form flag for the race */
	tflag    parts;          /* default parts for the race */
};

struct align_type
{
    char * name;
    int align;
};

struct pc_race_type  /* additional data for pc races */
{
	char *  name;           /* MUST be in race_type */
	char    who_name[9];
	sh_int  class_mult[MAX_CLASS];  /* exp multiplier for class, * 100 */
	sh_int  num_skills;
	char *  skills[5];
	sh_int  skill_level[5];
	sh_int  skill_percent[5];
	sh_int  min_stats[MAX_STATS];   /* minimum stats */
	sh_int  max_stats[MAX_STATS];   /* maximum stats */
        sh_int  remort_bonus[MAX_STATS]; /* bonus for chosing a lower remort race */
	sh_int  size;
	sh_int  gender;
	sh_int  remorts;
	sh_int  skill_gsns[5];      /* bonus skills for the race */
};


struct spec_type
{
	char *  name;           /* special function name */
	SPEC_FUN *  function;       /* the function */
};



/*
 * Data structure for notes.
 */

struct  note_data
{
	NOTE_DATA * next;
	bool    valid;
	sh_int  type;
	char *  sender;
	char *  date;
	char *  to_list;
	char *  subject;
	char *  text;
	time_t  date_stamp;
	  time_t  expire;
};

/* Values and data structure for Erwin's board system */
#define DEF_NORMAL  0 /* No forced change, but default (any string)   */
#define DEF_INCLUDE 1 /* 'names' MUST be included (only ONE name!)    */
#define DEF_EXCLUDE 2 /* 'names' must NOT be included (one name only) */

#define DEFAULT_BOARD 0 /* default board is board #0 in the boards      */
						/* It should be readable by everyone!           */
#define MAX_LINE_LENGTH 80 /* enforce a max length of 80 on text lines, reject longer lines */
						   /* This only applies in the Body of the note */                        
#define MAX_NOTE_TEXT (4*MAX_STRING_LENGTH - 1000)
#define BOARD_NOTFOUND -1 /* Error code from board_lookup() and board_number */

/* Data about a board */
struct board_data
{
	char *short_name;      /* Max 8 chars */
	char *long_name;       /* Explanatory text, should be no more than 40 ? chars */
	
	int read_level;        /* minimum level to see board */
	int write_level;       /* minimum level to post notes */
    int special_access;    /* Special access restrictions */

	char *names;           /* Default recipient */
	int force_type;        /* Default action (DEF_XXX) */
	
	int purge_days;        /* Default expiration */
    int purge_count;       /* Max messages before purge */

	/* Non-constant data */
	NOTE_DATA *note_first; /* pointer to board's first note */
	bool changed;
		
};


/*
 * An affect.
 */
struct  affect_data
{
    AFFECT_DATA *   next;
    bool        valid;
    sh_int      where;
    sh_int      type;
    sh_int      level;
    sh_int      duration;
    sh_int      location;
    sh_int      modifier;
    int         bitvector;
    sh_int      detect_level;
};

/* where definitions */
#define TO_AFFECTS  0
#define TO_OBJECT   1
#define TO_IMMUNE   2
#define TO_RESIST   3
#define TO_VULN     4
#define TO_WEAPON   5
/* bitvector used as memory special to affect */
#define TO_SPECIAL  6


/*
 * A kill structure (indexed by level).
 */
struct  kill_data
{
	sh_int      number;
	sh_int      killed;
};



/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO          3090
#define MOB_VNUM_CITYGUARD     3060
#define MOB_VNUM_PATROLMAN     2106
#define MOB_VNUM_ZOMBIE         1
#define GROUP_VNUM_TROLLS      2100
#define GROUP_VNUM_OGRES       2101
#define MOB_VNUM_SIDEKICK       4      
#define MOB_VNUM_TREEGOLEM      5
#define MOB_VNUM_MEPHFISHTON    6
#define MOB_VNUM_SNAKE          7
#define MOB_VNUM_VAMPIRE        8
#define MOB_VNUM_GHOST          9
#define MOB_VNUM_BASIC_APPARITION 11
#define MOB_VNUM_HOLY_APPARITION 12
#define MOB_VNUM_WATER_ELEMENTAL 13
#define MOB_VNUM_BEAST           14

/* RT ASCII conversions -- used so we can have letters in this file */

/* this is used for flags - but flags have changed --Bobble
#define A           1
#define B           2
#define C           4
#define D           8
#define E           16
#define F           32
#define G           64
#define H           128

#define I           256
#define J           512
#define K               1024
#define L           2048
#define M           4096
#define N           8192
#define O           16384
#define P           32768

#define Q           65536
#define R           131072
#define S           262144
#define T           524288
#define U           1048576
#define V           2097152
#define W           4194304
#define X           8388608

#define Y           16777216
#define Z           33554432
#define aa          67108864    // doubled due to conflicts
#define bb          134217728
#define cc          268435456    
#define dd          536870912
#define ee          1073741824
#define ff          -2147483647
#define gg          -2147483646
#define hh          -2147483644
#define ii          -2147483640
#define jj          -2147483632
#define kk          -2147483616
#define ll          -2147483584
#define mm          -2147483520
#define nn          -2147483392
#define oo          -2147483136
#define pp          -2147482624
#define qq          -2147481600
#define rr          -2147479552
#define ss          -2147475456
#define tt          -2147467264
#define uu          -2147450880
#define vv          -2147418112
#define ww          -2147352576
#define xx          -2147221504
#define yy          -2146959360
#define zz          -2146435072
*/
#define A           1
#define B           2
#define C           3
#define D           4
#define E           5
#define F           6 
#define G           7 
#define H           8  
#define I           9
#define J           10
#define K           11
#define L           12
#define M           13
#define N           14
#define O           15
#define P           16
#define Q           17
#define R           18
#define S           19
#define T           20
#define U           21
#define V           22
#define W           23
#define X           24
#define Y           25
#define Z           26

#define aa          27    // doubled due to conflicts
#define bb          28
#define cc          29    
#define dd          30
#define ee          31
#define ff          33    // bit 32 wasn't used in old-style
#define gg          34
#define hh          35
#define ii          36
#define jj          37
#define kk          38
#define ll          39
#define mm          40
#define nn          41
#define oo          42
#define pp          43
#define qq          44
#define rr          45
#define ss          46
#define tt          47
#define uu          48
#define vv          49
#define ww          50
#define xx          51
#define yy          52
#define zz          53
/*
#define aaa         54
#define bbb         55
#define ccc         56
#define ddd         57
#define eee         58
#define fff         59
#define ggg         60
#define hhh         61
#define iii         62
#define jjj         63
#define kkk         64
#define lll         65
#define mmm         66
#define nnn         67
#define ooo         68
#define ppp         69
#define qqq         70
#define rrr         71
#define sss         72
#define ttt         73
#define uuu         74
#define vvv         75
#define www         76
#define xxx         77
#define yyy         78
#define zzz         79
#define aabbb          80
#define aabbc          81
#define aabbd          82
#define aabbe          83
#define aabbf          84
#define aabbg          85
#define aabbh          86
#define aabbi          87
#define aabbj          88
#define aabbk          89
#define aabbl          90
#define aabbm          91
#define aabbn          92
#define aabbo          93
#define aabbp          94
#define aabbq          95
#define aabbr          96
#define aabbs          97
#define aabbt          98
#define aabbu          99
#define aabbv          100 */

/* damn pseudo flags are still needed for objects in value field */
/*
#define AA           1
#define BB           2
#define CC           4
#define DD           8
#define EE           16
#define FF           32
#define GG           64
#define HH           128
#define II           256
#define JJ           512
#define KK           1024
#define LL           2048
#define MM           4096
#define NN           8192
#define OO           16384
#define PP           32768
#define QQ           65536
#define RR           131072
#define SS           262144
#define TT           524288
#define UU           1048576
#define VV           2097152
#define WW           4194304
#define XX           8388608
#define YY           16777216
#define ZZ           33554432
*/

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC      (A)     /* Auto set for mobs    */
#define ACT_SENTINEL    (B)     /* Stays in one room    */
#define ACT_SCAVENGER   (C)     /* Picks up objects */
#define ACT_WIZI        (D)     /* invisible to mortals */
#define ACT_OBJ         (E)     /* Mobs that simulate objects */
#define ACT_AGGRESSIVE  (F)     /* Attacks PC's     */
#define ACT_STAY_AREA   (G)     /* Won't leave area */
#define ACT_WIMPY       (H)
#define ACT_PET         (I)     /* Auto set for pets    */
#define ACT_TRAIN       (J)     /* Can train PC's   */
#define ACT_PRACTICE    (K)     /* Can practice PC's    */
#define ACT_NO_TRACK    (L)  
#define ACT_SEE_ALL     (M)
#define ACT_TRIGGER_ALWAYS (N)
#define ACT_UNDEAD      (O) 
#define ACT_SPELLUP     (P) 
#define ACT_CLERIC      (Q)
#define ACT_MAGE        (R)
#define ACT_THIEF       (S)
#define ACT_WARRIOR     (T)
#define ACT_NOALIGN     (U)
#define ACT_NOPURGE     (V)
#define ACT_OUTDOORS    (W)
#define ACT_BANKER      (X)
#define ACT_INDOORS     (Y)
#define ACT_GUN         (Z)
#define ACT_IS_HEALER   (aa)
#define ACT_GAIN        (bb)
#define ACT_UPDATE_ALWAYS (cc)
#define ACT_IS_CHANGER  (dd)
#define ACT_NO_QUEST    (ee)
#define ACT_SAFE        (ff)
#define ACT_IGNORE_SAFE (gg)
#define ACT_JUDGE       (hh)    /* killer/thief flags removal */
#define ACT_NOEXP       (ii)    /* no experience from killing this mob */
#define ACT_NOMIMIC     (jj)    /* cannot mimic this mob */
#define ACT_HARD_QUEST  (kk)
#define ACT_STAGGERED   (ll)    /* no bonus attacks for being high-level */

/* damage classes */
#define DAM_NONE                0
#define DAM_BASH                1
#define DAM_PIERCE              2
#define DAM_SLASH               3
#define DAM_FIRE                4
#define DAM_COLD                5
#define DAM_LIGHTNING           6
#define DAM_ACID                7
#define DAM_POISON              8
#define DAM_NEGATIVE            9
#define DAM_HOLY                10
#define DAM_ENERGY              11
#define DAM_MENTAL              12
#define DAM_DISEASE             13
#define DAM_DROWNING            14
#define DAM_LIGHT               15
#define DAM_OTHER               16
#define DAM_HARM                17
#define DAM_CHARM               18
#define DAM_SOUND               19

/* DAM_MIX_MOD is modula for storing two damage types in one int */
#define DAM_MIX_MOD        20
#define MIX_DAMAGE(x,y)    (x == y || x == DAM_NONE ? y : y == DAM_NONE ? x : \
                            x + y * DAM_MIX_MOD)
#define IS_MIXED_DAMAGE(x) (x >= DAM_MIX_MOD)
#define FIRST_DAMAGE(x)    (x % DAM_MIX_MOD)
#define SECOND_DAMAGE(x)   (x / DAM_MIX_MOD)

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         (A)
#define OFF_BACKSTAB            (B)
#define OFF_BASH                (C)
#define OFF_BERSERK             (D)
#define OFF_DISARM              (E)
#define OFF_DODGE               (F)
#define OFF_FADE                (G)
#define OFF_FAST                (H)
#define OFF_KICK                (I)
#define OFF_KICK_DIRT           (J)
#define OFF_PARRY               (K)
#define OFF_RESCUE              (L)
#define OFF_TAIL                (M)
#define OFF_TRIP                (N)
#define OFF_CRUSH               (O)
#define ASSIST_ALL              (P)
#define ASSIST_ALIGN            (Q)
#define ASSIST_RACE             (R)
#define ASSIST_PLAYERS          (S)
#define ASSIST_GUARD            (T)
#define ASSIST_VNUM             (U)
#define OFF_DISTRACT	        (V)
#define OFF_HUNT		(X)
#define OFF_ARMED               (aa)
#define OFF_CIRCLE              (bb)

/* return values for check_imm */
#define IS_NORMAL           0
#define IS_IMMUNE           2
#define IS_RESISTANT        1
#define IS_VULNERABLE       -1

/* IMM bits for mobs */
#define IMM_SUMMON              (A)
#define IMM_CHARM               (B)
#define IMM_MAGIC               (C)
#define IMM_WEAPON              (D)
#define IMM_BASH                (E)
#define IMM_PIERCE              (F)
#define IMM_SLASH               (G)
#define IMM_FIRE                (H)
#define IMM_COLD                (I)
#define IMM_LIGHTNING           (J)
#define IMM_ACID                (K)
#define IMM_POISON              (L)
#define IMM_NEGATIVE            (M)
#define IMM_HOLY                (N)
#define IMM_ENERGY              (O)
#define IMM_MENTAL              (P)
#define IMM_DISEASE             (Q)
#define IMM_DROWNING            (R)
#define IMM_LIGHT               (S)
#define IMM_SOUND               (T)
#define IMM_WOOD                (X)
#define IMM_SILVER              (Y)
#define IMM_IRON                (Z)
 
/* RES bits for mobs */
#define RES_SUMMON              (A)
#define RES_CHARM               (B)
#define RES_MAGIC               (C)
#define RES_WEAPON              (D)
#define RES_BASH                (E)
#define RES_PIERCE              (F)
#define RES_SLASH               (G)
#define RES_FIRE                (H)
#define RES_COLD                (I)
#define RES_LIGHTNING           (J)
#define RES_ACID                (K)
#define RES_POISON              (L)
#define RES_NEGATIVE            (M)
#define RES_HOLY                (N)
#define RES_ENERGY              (O)
#define RES_MENTAL              (P)
#define RES_DISEASE             (Q)
#define RES_DROWNING            (R)
#define RES_LIGHT               (S)
#define RES_SOUND               (T)
#define RES_WOOD                (X)
#define RES_SILVER              (Y)
#define RES_IRON                (Z)
 
/* VULN bits for mobs */
#define VULN_SUMMON             (A)
#define VULN_CHARM              (B)
#define VULN_MAGIC              (C)
#define VULN_WEAPON             (D)
#define VULN_BASH               (E)
#define VULN_PIERCE             (F)
#define VULN_SLASH              (G)
#define VULN_FIRE               (H)
#define VULN_COLD               (I)
#define VULN_LIGHTNING          (J)
#define VULN_ACID               (K)
#define VULN_POISON             (L)
#define VULN_NEGATIVE           (M)
#define VULN_HOLY               (N)
#define VULN_ENERGY             (O)
#define VULN_MENTAL             (P)
#define VULN_DISEASE            (Q)
#define VULN_DROWNING           (R)
#define VULN_LIGHT              (S)
#define VULN_SOUND              (T)
#define VULN_WOOD               (X)
#define VULN_SILVER             (Y)
#define VULN_IRON               (Z)
 
/* body form */
#define FORM_EDIBLE             (A)
#define FORM_POISON             (B)
#define FORM_MAGICAL            (C)
#define FORM_INSTANT_DECAY      (D)
#define FORM_OTHER              (E)  /* defined by material bit */
 
/* actual form */
#define FORM_ANIMAL             (G)
#define FORM_SENTIENT           (H)
#define FORM_UNDEAD             (I) //vuln-immune to some spells
#define FORM_CONSTRUCT          (J) //train stats to max for gold
#define FORM_MIST               (K)
#define FORM_INTANGIBLE         (L)

#define FORM_BIPED              (M)
#define FORM_AGILE              (N) //10 move per level
#define FORM_INSECT             (O)
#define FORM_SPIDER             (P)
#define FORM_CRUSTACEAN         (Q)
#define FORM_WORM               (R)
#define FORM_BLOB               (S)
 
#define FORM_PLANT              (U)  //grow roots
#define FORM_MAMMAL             (V)
#define FORM_BIRD               (W)
#define FORM_REPTILE            (X)
#define FORM_SNAKE              (Y)
#define FORM_DRAGON             (Z)
#define FORM_AMPHIBIAN          (aa)
#define FORM_FISH               (bb)
#define FORM_COLD_BLOOD         (cc)    

#define FORM_BRIGHT             (dd) //self light source
#define FORM_TOUGH              (ee) //10 hp per level
#define FORM_SUNBURN            (ff) //damage from sun when outdoors
#define FORM_DOUBLE_JOINTED     (gg) //better dodging
#define FORM_FROST              (hh) //frost aura
#define FORM_BURN               (ii) //burning aura
#define FORM_WISE               (jj) //10 mana per level
#define FORM_CONDUCTIVE         (kk) //electric aura

/* body parts */
#define PART_HEAD               (A)
#define PART_ARMS               (B)
#define PART_LEGS               (C)
#define PART_HEART              (D)
#define PART_BRAINS             (E)
#define PART_GUTS               (F)
#define PART_HANDS              (G)
#define PART_FEET               (H)
#define PART_FINGERS            (I)
#define PART_EAR                (J)
#define PART_EYE                (K)
#define PART_LONG_TONGUE        (L)
#define PART_EYESTALKS          (M)
#define PART_TENTACLES          (N)
#define PART_FINS               (O)
#define PART_WINGS              (P)
#define PART_TAIL               (Q)
/* for combat */
#define PART_CLAWS              (U)
#define PART_FANGS              (V)
#define PART_HORNS              (W)
#define PART_SCALES             (X)
#define PART_TUSKS              (Y)

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_NONE               0 /* NOT used */
#define AFF_BLIND              1
#define AFF_INVISIBLE          2
#define AFF_DETECT_EVIL        3
#define AFF_DETECT_INVIS       4
#define AFF_DETECT_MAGIC       5
#define AFF_DETECT_HIDDEN      6
#define AFF_DETECT_GOOD        7
#define AFF_SANCTUARY          8
#define AFF_FADE	       9
#define AFF_INFRARED          10
#define AFF_CURSE             11
#define AFF_ASTRAL            12
#define AFF_POISON            13
#define AFF_PROTECT_EVIL      14
#define AFF_PROTECT_GOOD      15
#define AFF_SNEAK             16
#define AFF_HIDE              17
#define AFF_SLEEP             18
#define AFF_CHARM             19
#define AFF_FLYING            20
#define AFF_PASS_DOOR         21
#define AFF_HASTE             22
#define AFF_CALM              23
#define AFF_PLAGUE            24
#define AFF_WEAKEN            25
#define AFF_DARK_VISION       26
#define AFF_BERSERK           27
#define AFF_BREATHE_WATER     28
#define AFF_REGENERATION      29
#define AFF_SLOW              30
#define AFF_BATTLE_METER      31
#define AFF_FEAR              32
#define AFF_DETECT_ASTRAL     33
#define AFF_SHELTER           34
#define AFF_CHAOS_FADE        35
#define AFF_FEEBLEMIND        36
#define AFF_SPLIT             37
#define AFF_GUARD             38
#define AFF_RITUAL            39
#define AFF_NECROSIS          40
#define AFF_ANIMATE_DEAD      41
#define AFF_ELEMENTAL_SHIELD  42
#define AFF_PROTECT_MAGIC     43
#define AFF_FAERIE_FIRE       44
#define AFF_NO_TRACE          45        
#define AFF_ENTANGLE          46
#define AFF_INSANE            47
#define AFF_LAUGH             48
#define AFF_SORE              49
#define AFF_DARKNESS          50
#define AFF_TOMB_ROT          51
#define AFF_DEATHS_DOOR       52
#define AFF_HEROISM           53
#define AFF_REFLECTION        54
#define AFF_ROOTS             55
#define AFF_MANTRA            56
#define AFF_LEARN             57
#define AFF_HAUNTED           58
#define AFF_MANA_BURN         59
#define AFF_IRON_MAIDEN       60
#define AFF_FLEE              61
#define AFF_HEAL              62
#define AFF_OVERCHARGE        63
#define AFF_GIANT_STRENGTH    64
#define AFF_PHASE             65
#define AFF_SHROUD            66
#define AFF_PARALYSIS         67
#define AFF_INFECTIOUS_ARROW  68
#define AFF_FERVENT_RAGE      69
#define AFF_FERVENT_RAGE_COOLDOWN  70
#define AFF_PAROXYSM          71
#define AFF_PAROXYSM_COOLDOWN 72
#define AFF_RUPTURE           73
#define AFF_HALLOW            74
#define AFF_MINOR_FADE        75
#define AFF_REPLENISH         76
#define AFF_FORTUNE           77


/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL         0
#define SEX_MALE            1
#define SEX_FEMALE          2
#define SEX_BOTH            3

/* AC types */
#define AC_PIERCE           0
#define AC_BASH             1
#define AC_SLASH            2
#define AC_EXOTIC           3

/* dice */
#define DICE_NUMBER         0
#define DICE_TYPE           1
#define DICE_BONUS          2

/* size */
#define SIZE_TINY           0
#define SIZE_SMALL          1
#define SIZE_MEDIUM         2
#define SIZE_LARGE          3
#define SIZE_HUGE           4
#define SIZE_GIANT          5

/* toggle for old score and finger */
#define TOGG_OLDSCORE       1
#define TOGG_OLDFINGER      2
#define TOGG_STATBARS       3

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SILVER_ONE       1
#define OBJ_VNUM_GOLD_ONE         2
#define OBJ_VNUM_GOLD_SOME        3
#define OBJ_VNUM_SILVER_SOME      4
#define OBJ_VNUM_COINS            5

#define OBJ_VNUM_ARROWS           6
#define OBJ_VNUM_SCROLL           7

#define OBJ_VNUM_CORPSE_NPC      10
#define OBJ_VNUM_CORPSE_PC       11
#define OBJ_VNUM_SEVERED_HEAD    12
#define OBJ_VNUM_TORN_HEART      13
#define OBJ_VNUM_SLICED_ARM      14
#define OBJ_VNUM_SLICED_LEG      15
#define OBJ_VNUM_GUTS            16
#define OBJ_VNUM_BRAINS          17

#define OBJ_VNUM_MUSHROOM        20
#define OBJ_VNUM_LIGHT_BALL      21
#define OBJ_VNUM_SPRING          22
#define OBJ_VNUM_DISC            23
#define OBJ_VNUM_PORTAL          25
#define OBJ_VNUM_GRUB            26
#define OBJ_VNUM_DIVINE_BALL     27
#define OBJ_VNUM_BOMB            28
#define OBJ_VNUM_TRAILMIX        30
#define OBJ_VNUM_TORCH           31
#define OBJ_VNUM_DUMMY          32    
#define OBJ_VNUM_FIRE           37       
#define OBJ_VNUM_BIG_FIRE       38       
#define OBJ_VNUM_HUGE_FIRE      39       
#define OBJ_VNUM_FISH           40
#define OBJ_VNUM_BIG_FISH       41
#define OBJ_VNUM_HUGE_FISH      42
#define OBJ_VNUM_BOOT           43
#define OBJ_VNUM_RAFT           44    
#define OBJ_VNUM_ROSE           45
#define OBJ_VNUM_MOB_WEAPON     46
#define OBJ_VNUM_BLOOD          47
#define OBJ_VNUM_PIT           3010

#define OBJ_VNUM_SCHOOL_MACE       18400
#define OBJ_VNUM_SCHOOL_DAGGER     18401
#define OBJ_VNUM_SCHOOL_SWORD      18402
#define OBJ_VNUM_SCHOOL_SPEAR      18417
#define OBJ_VNUM_SCHOOL_STAFF      18418
#define OBJ_VNUM_SCHOOL_AXE        18419
#define OBJ_VNUM_SCHOOL_FLAIL      18420
#define OBJ_VNUM_SCHOOL_WHIP       18421
#define OBJ_VNUM_SCHOOL_POLEARM    18422
#define OBJ_VNUM_SCHOOL_GUN        18423
#define OBJ_VNUM_SCHOOL_BOW        18432

#define OBJ_VNUM_SCHOOL_VEST       18403
#define OBJ_VNUM_SCHOOL_SHIELD     18404
#define OBJ_VNUM_SCHOOL_BANNER     18416
#define OBJ_VNUM_NEWBIE_GUIDE      18431
/*#define OBJ_VNUM_MAP           3162*/
/* For Bastion */
#define OBJ_VNUM_MAP           10317

#define OBJ_VNUM_WHISTLE       2116
#define OBJ_VNUM_SIVA_WEAPON    34
#define OBJ_VNUM_GOODBERRY      35
#define OBJ_VNUM_STORAGE_BOX    48	

/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT            1
#define ITEM_SCROLL           2
#define ITEM_WAND             3
#define ITEM_STAFF            4
#define ITEM_WEAPON           5
#define ITEM_TREASURE         8
#define ITEM_ARMOR            9
#define ITEM_POTION          10
#define ITEM_CLOTHING        11
#define ITEM_FURNITURE       12
#define ITEM_TRASH           13
#define ITEM_CONTAINER       15
#define ITEM_DRINK_CON       17
#define ITEM_KEY             18
#define ITEM_FOOD            19
#define ITEM_MONEY           20
#define ITEM_BOAT            22
#define ITEM_CORPSE_NPC      23
#define ITEM_CORPSE_PC       24
#define ITEM_FOUNTAIN        25
#define ITEM_PILL            26
#define ITEM_PROTECT         27
#define ITEM_MAP             28
#define ITEM_PORTAL          29
#define ITEM_WARP_STONE      30
#define ITEM_ROOM_KEY        31
#define ITEM_GEM             32
#define ITEM_JEWELRY         33
#define ITEM_JUKEBOX         34
#define ITEM_EXPLOSIVE       35
#define ITEM_HOGTIE          36
#define ITEM_DOWSING_STICK   37
#define ITEM_SILVER_HERB     38 
#define ITEM_BLACK_HERB      39
#define ITEM_RED_HERB        40
#define ITEM_MOTTLED_HERB    50
#define ITEM_CIGARETTE	     51
#define ITEM_ARROWS          52

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW       (A)
#define ITEM_HUM        (B)
#define ITEM_DARK       (C)
#define ITEM_LOCK       (D)
#define ITEM_EVIL       (E)
#define ITEM_INVIS      (F)
#define ITEM_MAGIC      (G)
#define ITEM_NODROP     (H)
#define ITEM_BLESS      (I)
#define ITEM_ANTI_GOOD      (J)
#define ITEM_ANTI_EVIL      (K)
#define ITEM_ANTI_NEUTRAL   (L)
#define ITEM_NOREMOVE       (M)
#define ITEM_INVENTORY      (N)
#define ITEM_NOPURGE        (O)
#define ITEM_ROT_DEATH      (P)
#define ITEM_VIS_DEATH      (Q)
#define ITEM_NO_LORE        (R)
#define ITEM_NONMETAL       (S)
#define ITEM_NOLOCATE       (T)
#define ITEM_MELT_DROP      (U)
#define ITEM_HAD_TIMER      (V)
#define ITEM_SELL_EXTRACT   (W)
#define ITEM_RANDOM         (X)
#define ITEM_BURN_PROOF     (Y)
#define ITEM_NOUNCURSE      (Z)
#define ITEM_STICKY         (aa)
#define ITEM_JAMMED         (bb)        
#define ITEM_ONE_USE        (cc)
#define ITEM_REMORT	    (dd)
#define ITEM_TRAPPED        (ee)
#define ITEM_EASY_DROP      (ff)
#define ITEM_NO_EXTRACT     (gg)
#define ITEM_QUESTEQ        (hh)

/* class restriction flags */
#define ITEM_ALLOW_WARRIOR        100
#define ITEM_ALLOW_THIEF          101
#define ITEM_ALLOW_CLERIC         102
#define ITEM_ALLOW_MAGE           103

#define ITEM_ANTI_WARRIOR         104
#define ITEM_ANTI_THIEF           105
#define ITEM_ANTI_CLERIC          106
#define ITEM_ANTI_MAGE            107

#define ITEM_CLASS_WARRIOR        108
#define ITEM_CLASS_THIEF          109
#define ITEM_CLASS_CLERIC         110
#define ITEM_CLASS_MAGE           111
#define ITEM_CLASS_GLADIATOR      112
#define ITEM_CLASS_SAMURAI        113
#define ITEM_CLASS_PALADIN        114
#define ITEM_CLASS_ASSASSIN       115
#define ITEM_CLASS_NINJA          116
#define ITEM_CLASS_MONK           117
#define ITEM_CLASS_TEMPLAR        118
#define ITEM_CLASS_ILLUSIONIST    119
#define ITEM_CLASS_GUNSLINGER     120
#define ITEM_CLASS_RANGER         121
#define ITEM_CLASS_NECROMANCER    122

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE           (A)
#define ITEM_WEAR_FINGER    (B)
#define ITEM_WEAR_NECK      (C)
#define ITEM_WEAR_TORSO     (D)
#define ITEM_WEAR_HEAD      (E)
#define ITEM_WEAR_LEGS      (F)
#define ITEM_WEAR_FEET      (G)
#define ITEM_WEAR_HANDS     (H)
#define ITEM_WEAR_ARMS      (I)
#define ITEM_WEAR_SHIELD    (J)
#define ITEM_WEAR_ABOUT     (K)
#define ITEM_WEAR_WAIST     (L)
#define ITEM_WEAR_WRIST     (M)
#define ITEM_WIELD          (N)
#define ITEM_HOLD           (O)
#define ITEM_NO_SAC         (P)
#define ITEM_WEAR_FLOAT     (Q)
#define ITEM_TRANSLUCENT    (R)

/* weapon class */
#define WEAPON_EXOTIC       0
#define WEAPON_SWORD        1
#define WEAPON_DAGGER       2
#define WEAPON_SPEAR        3
#define WEAPON_MACE         4
#define WEAPON_AXE          5
#define WEAPON_FLAIL        6
#define WEAPON_WHIP         7   
#define WEAPON_POLEARM      8
#define WEAPON_GUN          9
#define WEAPON_BOW         10

/* weapon types */
#define WEAPON_FLAMING      (A)
#define WEAPON_FROST        (B)
#define WEAPON_VAMPIRIC     (C)
#define WEAPON_SHARP        (D)
#define WEAPON_VORPAL       (E)
#define WEAPON_TWO_HANDS    (F)
#define WEAPON_SHOCKING     (G)
#define WEAPON_POISON       (H)
#define WEAPON_MANASUCK     (I)
#define WEAPON_MOVESUCK     (J)
#define WEAPON_DUMB         (K)
#define WEAPON_PUNCTURE     (L)  
#define WEAPON_PARALYSIS_POISON (M)
#define WEAPON_STORMING     (N)

/* gate flags */
#define GATE_NORMAL_EXIT    (A)
#define GATE_NOCURSE        (B)
#define GATE_GOWITH         (C)
#define GATE_BUGGY          (D)
#define GATE_RANDOM         (E)
#define GATE_IGNORE_NO_RECALL (F)
#define GATE_WARFARE        (G)
#define GATE_ASTRAL         (H)
#define GATE_NOQUEST        (I)
#define GATE_STAY_AREA      (J)

/* furniture flags */
#define STAND_AT        (A)
#define STAND_ON        (B)
#define STAND_IN        (C)
#define SIT_AT          (D)
#define SIT_ON          (E)
#define SIT_IN          (F)
#define REST_AT         (G)
#define REST_ON         (H)
#define REST_IN         (I)
#define SLEEP_AT        (J)
#define SLEEP_ON        (K)
#define SLEEP_IN        (L)
#define PUT_AT          (M)
#define PUT_ON          (N)
#define PUT_IN          (O)
#define PUT_INSIDE      (P)



/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE            0
#define APPLY_STR             1
#define APPLY_DEX             2
#define APPLY_INT             3
#define APPLY_WIS             4
#define APPLY_CON             5
#define APPLY_SEX             6
#define APPLY_CLASS           7
#define APPLY_LEVEL           8
#define APPLY_AGE             9
#define APPLY_HEIGHT             10
#define APPLY_WEIGHT             11
#define APPLY_MANA           12
#define APPLY_HIT            13
#define APPLY_MOVE           14
#define APPLY_GOLD           15
#define APPLY_EXP            16
#define APPLY_AC             17
#define APPLY_HITROLL            18
#define APPLY_DAMROLL            19
#define APPLY_SAVES          20
#define APPLY_SAVING_PARA        20
#define APPLY_SAVING_ROD         21
#define APPLY_SAVING_PETRI       22
#define APPLY_SAVING_BREATH      23
#define APPLY_SAVING_SPELL       24
#define APPLY_SPELL_AFFECT       25
#define APPLY_VIT              26
#define APPLY_AGI              27
#define APPLY_DIS              28
#define APPLY_CHA              29
#define APPLY_LUC              30
// #define APPLY_COMBO              31


/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE        (A)
#define CONT_PICKPROOF        (B)
#define CONT_CLOSED           (C)
#define CONT_LOCKED           (D)
#define CONT_PUT_ON           (E)
#define CONT_EASY             (F)
#define CONT_HARD             (G)
#define CONT_INFURIATING      (H)



/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */

#define ROOM_VNUM_LIMBO           2
#define ROOM_VNUM_CHAT         1200
#define ROOM_VNUM_TEMPLE       10280
#define ROOM_VNUM_ALTAR        10280
#define ROOM_VNUM_SCHOOL       18400
#define ROOM_VNUM_BALANCE      4500
#define ROOM_VNUM_CIRCLE       4400
#define ROOM_VNUM_DEMISE       4201
#define ROOM_VNUM_HONOR        4300
#define ROOM_VNUM_MORGUE       10281
#define ROOM_VNUM_RECALL       10204
#define ROOM_VNUM_BANK         10286
#define ROOM_VNUM_DONATION     10414
#define ROOM_VNUM_EAGLE        26188
#define ROOM_VNUM_AUTH_START   18400
#define ROOM_VNUM_AUTH_END     18499
           


/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK           (A)
#define ROOM_NO_MOB         (C)
#define ROOM_INDOORS        (D)
#define ROOM_NO_SCOUT       (E)
#define ROOM_TATTOO_SHOP    (F)
#define ROOM_NO_RANDOM      (G)
#define ROOM_PRIVATE        (J)
#define ROOM_SAFE           (K)
#define ROOM_SOLITARY       (L)
#define ROOM_PET_SHOP       (M)
#define ROOM_NO_RECALL      (N)
#define ROOM_IMP_ONLY       (O)
#define ROOM_GODS_ONLY      (P)
#define ROOM_HEROES_ONLY    (Q)
#define ROOM_NEWBIES_ONLY   (R)
#define ROOM_LAW            (S)
#define ROOM_NOWHERE        (T)
#define ROOM_DONATION       (U)
#define ROOM_SNARE          (V)
#define ROOM_BLACKSMITH     (W)
#define ROOM_PEEL           (X)
#define ROOM_JAIL           (Y)
#define ROOM_NO_QUEST       (Z)
#define ROOM_ARENA          (aa)
#define ROOM_BARREN         (bb)
#define ROOM_BOX_SHOP       (cc)
#define ROOM_BOX_ROOM       (dd)
#define ROOM_HARD_QUEST       (ee)

/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH             0
#define DIR_EAST              1
#define DIR_SOUTH             2
#define DIR_WEST              3
#define DIR_UP                4
#define DIR_DOWN              5
#define DIR_NORTHEAST       6
#define DIR_SOUTHEAST       7
#define DIR_SOUTHWEST       8
#define DIR_NORTHWEST       9



/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR             (A)
#define EX_CLOSED             (B)
#define EX_LOCKED             (C)
#define EX_PICKPROOF          (F)
#define EX_NOPASS             (G)
#define EX_EASY               (H)
#define EX_HARD               (I)
#define EX_INFURIATING        (J)
#define EX_NOCLOSE            (K)
#define EX_NOLOCK             (L)
#define EX_TRAPPED            (M)
#define EX_HIDDEN             (N)

/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE           0
#define SECT_CITY             1
#define SECT_FIELD            2
#define SECT_FOREST           3
#define SECT_HILLS            4
#define SECT_MOUNTAIN         5
#define SECT_WATER_SHALLOW    6
#define SECT_WATER_DEEP       7
#define SECT_UNDERWATER       8
#define SECT_AIR              9
#define SECT_DESERT          10
#define SECT_UNDERGROUND     11
#define SECT_MAX             12



/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE            -1
#define WEAR_LIGHT            0
#define WEAR_FINGER_L         1
#define WEAR_FINGER_R         2
#define WEAR_NECK_1           3
#define WEAR_NECK_2           4
#define WEAR_TORSO            5
#define WEAR_HEAD             6
#define WEAR_LEGS             7
#define WEAR_FEET             8
#define WEAR_HANDS            9
#define WEAR_ARMS            10
#define WEAR_SHIELD          11
#define WEAR_ABOUT           12
#define WEAR_WAIST           13
#define WEAR_WRIST_L         14
#define WEAR_WRIST_R         15
#define WEAR_WIELD           16
#define WEAR_HOLD            17
#define WEAR_FLOAT           18
#define WEAR_SECONDARY       19
#define MAX_WEAR             20

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/* tattoo stuff */
#define TATTOO_NONE -1
typedef int tattoo_list[MAX_WEAR];

/*
 * Conditions.
 */
#define COND_DRUNK            0
#define COND_FULL             1
#define COND_THIRST           2
#define COND_HUNGER           3
#define COND_SMOKE	      4
#define COND_TOLERANCE        5
#define COND_DEEP_SLEEP       6

/*
 * Positions.
 */
#define POS_DEAD              0
#define POS_MORTAL            1
#define POS_INCAP             2
#define POS_STUNNED           3
#define POS_SLEEPING              4
#define POS_RESTING           5
#define POS_SITTING           6
#define POS_FIGHTING              7
#define POS_STANDING              8



/*
 * ACT bits for players.
 */
#define PLR_IS_NPC      (A)     /* Don't EVER set.  */
#define PLR_MUDFTP      (B)

/* RT auto flags */
#define PLR_AUTOASSIST      (C)
#define PLR_AUTOEXIT        (D)
#define PLR_AUTOLOOT        (E)
#define PLR_AUTOSAC         (F)
#define PLR_AUTOGOLD        (G)
#define PLR_AUTOSPLIT       (H)

/* Auth */
#define PLR_UNAUTHED        (I)
#define PLR_CONSENT         (J)

#define PLR_AUTORESCUE      (K)
#define PLR_LAW             (L) /* player is law immortal */
#define PLR_HELPER          (M) /* newbie helper */

/* RT personal flags */
#define PLR_HOLYLIGHT       (N)
#define PLR_CANLOOT         (P)
#define PLR_NOSUMMON        (Q)
#define PLR_NOFOLLOW        (R)
#define PLR_QUESTOR         (S)
#define PLR_QUESTORHARD     (V)

/*
 * Colour stuff by Lope of Loping Through The MUD
 */
#define PLR_COLOUR     (T)    /* Colour Flag By Lope */

/* penalty flags */
#define PLR_PERMIT      (U)
#define PLR_LOG         (W)
#define PLR_DENY        (X)
#define PLR_THIEF       (Z)
#define PLR_KILLER      (aa)

/* etc */
#define PLR_TITLE       (bb)
#define PLR_PERM_PKILL  (cc)
#define PLR_NOCANCEL    (dd)
#define PLR_WAR         (ee) 
#define PLR_IMMQUEST    (ff)
#define PLR_HARDCORE    (gg)
#define PLR_NOLOCATE    (hh)
#define PLR_NOACCEPT    (ii)
#define PLR_NOSURR      (jj)
#define PLR_RP          (kk)
#define PLR_TRIG_SAFE   (ll)
#define PLR_INACTIVE_HELPER (mm)
#define PLR_ANTI_HELEPR (nn)
#define PLR_NOEXP       (oo)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET              (A)
#define COMM_DEAF               (B)
#define COMM_NOWIZ              (C)
#define COMM_NOAUCTION          (D)
#define COMM_NOGOSSIP           (E)
#define COMM_NOQUESTION         (F)
#define COMM_NOMUSIC            (G)
#define COMM_NOCLAN             (H)
#define COMM_NOQUOTE            (I)
#define COMM_SHOUTSOFF          (J)
#define COMM_NOINFO             (K)  
#define COMM_NOREL              (U)

/* display flags */
#define COMM_COMPACT        (L)
#define COMM_BRIEF          (M)
#define COMM_PROMPT         (N)
#define COMM_COMBINE        (O)
#define COMM_TELNET_GA      (P)
#define COMM_SHOW_AFFECTS   (Q)
#define COMM_NOGRATZ        (R)
#define COMM_NOWAR          (S) 
#define COMM_NOSAVANT       (T)

/* other */
#define COMM_SNOOP_PROOF    (Y)
#define COMM_AFK        (Z)
#define COMM_NOGAME     (aa)
#define COMM_NOBITCH    (bb)
#define COMM_SHOW_WORTH (cc)
#define COMM_SHOW_ATTRIB (dd)
#define COMM_NONEWBIE    (ee)
#define COMM_BUSY        (ff)


/* WIZnet flags */
#define WIZ_ON          (A)
#define WIZ_TICKS       (B)
#define WIZ_LOGINS      (C)
#define WIZ_SITES       (D)
#define WIZ_LINKS       (E)
#define WIZ_DEATHS      (F)
#define WIZ_RESETS      (G)
#define WIZ_MOBDEATHS   (H)
#define WIZ_FLAGS       (I)
#define WIZ_PENALTIES   (J)
#define WIZ_SACCING     (K)
#define WIZ_LEVELS      (L)
#define WIZ_SECURE      (M)
#define WIZ_SWITCHES    (N)
#define WIZ_SNOOPS      (O)
#define WIZ_RESTORE     (P)
#define WIZ_LOAD        (Q)
#define WIZ_NEWBIE      (R)
#define WIZ_PREFIX      (S)
#define WIZ_SPAM        (T)
#define WIZ_ASAVE       (U)
#define WIZ_FTAG        (V)
#define WIZ_AUTH        (W)
#define WIZ_CHEAT       (X)
#define WIZ_RELIGION	(Y)
#define WIZ_MEMCHECK	(Z)
#define WIZ_BUGS        (aa)

/* Freeze Tag flags */
#define TAG_PLAYING     (A)
#define TAG_FROZEN      (B)
#define TAG_RED         (C)
#define TAG_BLUE        (D)

/* penalties */
#define PENALTY_NOEMOTE    (A)
#define PENALTY_NOSHOUT    (B)
#define PENALTY_NOTELL     (C)
#define PENALTY_NOCHANNEL  (D) 
#define PENALTY_FREEZE     (E)
#define PENALTY_JAIL       (F)
#define PENALTY_NONOTE     (G)

/* fighting display gags */
#define GAG_MISS       (A)
#define GAG_WFLAG      (B)
#define GAG_FADE       (C)
#define GAG_BLEED      (D)
#define GAG_IMMUNE     (E)
#define GAG_EQUIP      (F)
#define GAG_AURA       (G)
#define GAG_SUNBURN    (H)
#define GAG_NCOL_CHAN  (I)

/* channel definitions for log_chan/playback */
extern sh_int sn_gossip;
extern sh_int sn_auction;
extern sh_int sn_music;
extern sh_int sn_question;
extern sh_int sn_answer;
extern sh_int sn_quote;
extern sh_int sn_gratz;
extern sh_int sn_gametalk;
extern sh_int sn_bitch;
extern sh_int sn_immtalk;
extern sh_int sn_savantalk;
extern sh_int sn_newbie;

/* Why not replace all the ones below with just this?
   We might want to have daily/weekly/monthly/overall boards differ
   in what is tracked and order in the list so
   let's keep it all separate for now.
   Yes, it's kind of messy, but it's functional and flexible. */

/* defines for lboard type 
 used for args to functions
 such as update_lboard 	*/

#define LBOARD_MKILL	0
#define LBOARD_QCOMP	1
#define LBOARD_BHD		2
#define LBOARD_QPNT		3
#define LBOARD_WKILL	4
#define LBOARD_EXPL		5
#define LBOARD_QFAIL	6
#define LBOARD_LEVEL	7
#define LBOARD_PKILL	8
#define MAX_LBOARD 		9

extern sh_int sn_daily;
extern sh_int sn_weekly;
extern sh_int sn_monthly;
extern sh_int sn_overall;

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct  mob_index_data_old
{
	MOB_INDEX_DATA_OLD *    next;
	SPEC_FUN *      spec_fun;
	SHOP_DATA *     pShop;
	MPROG_LIST *        mprogs;
	AREA_DATA *     area;       /* OLC */
	int      vnum;
	sh_int      group;
	bool        new_format;
	sh_int      count;
	sh_int      killed;
	char *      player_name;
	char *      short_descr;
	char *      long_descr;
	char *      description;
	tflag        act;
        tflag       affect_field;
	sh_int      alignment;
	sh_int      level;
	sh_int      hitroll;
	int         hit[3];
	int         mana[3];
	sh_int      damage[3];
	sh_int      ac[4];
	sh_int      dam_type;
	tflag        off_flags;
	tflag        imm_flags;
	tflag        res_flags;
	tflag        vuln_flags;
	sh_int      start_pos;
	sh_int      default_pos;
	sh_int      sex;
	sh_int      race;
	long        wealth;
	tflag        form;
	tflag        parts;
	sh_int      size;
	char *      material;
	tflag        mprog_flags;
    sh_int      stance;
};

/*
 * Prototype for a mob (new-style).
 * This is the in-memory version of #MOBBLES.
 */
struct  mob_index_data
{
    MOB_INDEX_DATA* next;
    SPEC_FUN*   spec_fun;
    SHOP_DATA*  pShop;
    MPROG_LIST* mprogs;
    AREA_DATA*  area;
    int         vnum;
    sh_int      group;
    sh_int      count;
    sh_int      killed;
    char*       player_name;
    char*       short_descr;
    char*       long_descr;
    char*       description;
    tflag       act;
    tflag       affect_field;
    sh_int      alignment;
    sh_int      level;
    sh_int      hitpoint_percent;
    sh_int      mana_percent;
    sh_int      move_percent;
    sh_int      hitroll_percent;
    sh_int      damage_percent;
    sh_int      ac_percent;
    sh_int      saves_percent;
    sh_int      dam_type;
    tflag       off_flags;
    tflag       imm_flags;
    tflag       res_flags;
    tflag       vuln_flags;
    sh_int      start_pos;
    sh_int      default_pos;
    sh_int      sex;
    sh_int      race;
    sh_int      wealth_percent;
    tflag       form;
    tflag       parts;
    sh_int      size;
    tflag       mprog_flags;
    sh_int      stance;
};

/* memory settings */
#define MEM_CUSTOMER    A   
#define MEM_SELLER  B
#define MEM_HOSTILE C
#define MEM_AFRAID  D

/* memory for mobs */
struct mem_data
{
    MEM_DATA    *next;
    bool    valid;
    int     id;     
    int     reaction;
    time_t  when;
};


/*
 * One character (PC or NPC).
 * char data
 */
struct  char_data
{
	CHAR_DATA *     next;
	CHAR_DATA *     next_in_room;
	CHAR_DATA *     master;
	CHAR_DATA *     leader;
	CHAR_DATA *     fighting;
	CHAR_DATA *     reply;
	CHAR_DATA *     pet;
	CHAR_DATA *     mprog_target;
	MEM_DATA *      memory;
	MEM_DATA *		aggressors;
	SPEC_FUN *      spec_fun;
	MOB_INDEX_DATA *    pIndexData;
	DESCRIPTOR_DATA *   desc;
	AFFECT_DATA *   affected;
	OBJ_DATA *      carrying;
	OBJ_DATA *      on;
	ROOM_INDEX_DATA *   in_room;
	ROOM_INDEX_DATA *   was_in_room;
	AREA_DATA *     zone;
	PC_DATA *       pcdata;
	GEN_DATA *      gen_data;
	bool        valid;
	char *      name;
	long        id;
	sh_int      version;
	char *      short_descr;
	char *      long_descr;
	char *      description;
	char *      prompt;
	char *      prefix;
	sh_int      group;
	sh_int      clan;
	sh_int      sex;
	sh_int      class;
	sh_int      race;
	sh_int      level;
	sh_int      trust;
	int         played;
	int         lines;  /* for the pager */
	time_t      logon;
	sh_int      timer;
	sh_int      wait;
	sh_int      daze;
	sh_int	    stop;
	int      hit;
	int      max_hit;
	int      mana;
	int      max_mana;
	int      move;
	int      max_move;
	long        gold;
	long        silver;
	int         exp;
	tflag        act;
	tflag        comm;   /* RT added to pad the vector */
	tflag        wiznet; /* wiz stuff */
	tflag        imm_flags;
	tflag        res_flags;
	tflag        vuln_flags;
	sh_int      invis_level;
	sh_int      incog_level;
        tflag       affect_field;
	sh_int      position;
	sh_int      practice;
	sh_int      train;
	int         carry_weight;
	sh_int      carry_number;
	sh_int      saving_throw;
	sh_int      alignment;
	sh_int      hitroll;
	sh_int      damroll;
	sh_int      armor[4];
	sh_int      wimpy;
        tflag       penalty;
        tflag       gag;  
        tflag       togg;
//        int         combo_points;   
	/* stats */
	sh_int      perm_stat[MAX_STATS];
	sh_int      mod_stat[MAX_STATS];
	/* parts stuff */
	tflag        form;
	tflag        parts;
	sh_int      size;
	/* mobile stuff */
	tflag        off_flags;
	sh_int      damage[3];
	sh_int      dam_type;
	sh_int      start_pos;
	sh_int      default_pos;
	  sh_int        mprog_delay;
	char *hunting;
	sh_int  stance;
	sh_int      slow_move;
        bool        just_killed; /* for checking if char was just killed */
        bool        must_extract; /* for delayed char purging */
    lua_State *LS;
	#ifdef FSTAT
	/* Stuff for fight statistics*/
	int	attacks_attempts;
	int	attacks_success;
	int	attacks_misses;
	int	damage_dealt;
	int	fight_rounds;
	int	damage_taken;
	int	mana_used;
	int	moves_used;
	#endif
};



/*
 * Data which only PC's have. 
 * Player character data, pc data
 */
struct  pc_data
{
    PC_DATA *       next;
    /*BUFFER *        buffer; old buffer for replay */
	bool	new_tells; /* whether there are unread tells */
    COLOUR_DATA *   code;
    SORT_TABLE *    bounty_sort;
    bool        valid;
    char *      pwd;
    char *      bamfin;
    char *      bamfout;
    char *      title;
    char *	name_color;
    char *	pre_title;
    char *  last_host;
    BOARD_DATA *  board;                /* The current board */
    time_t        last_note[MAX_BOARD]; /* last note for the boards */
    NOTE_DATA *   in_progress;
    int      perm_hit;
    int      perm_mana;
    int      perm_move;

	PERS_HISTORY *gtell_history;
	PERS_HISTORY *tell_history;
	PERS_HISTORY *clan_history;
	
    int             achpoints; /* Astark September 2012*/
    sh_int          behead_cnt;
    sh_int	    storage_boxes; /*Number of storage boxes the player has*/
    OBJ_DATA *	box_data[MAX_STORAGE_BOX];/*So we know if boxes are loaded and have easy access to them for saving purposes*/

    sh_int      trained_hit;
    sh_int      trained_mana;
    sh_int      trained_move;
    int         trained_hit_bonus;
    int         trained_mana_bonus;
    int         trained_move_bonus;
    sh_int      true_sex;
    int         last_level;
    sh_int      highest_level; /* highest level reached during current remort */
    sh_int      condition   [7];
    sh_int      learned     [MAX_SKILL];
    bool        group_known [MAX_GROUP];
    sh_int      points;
    bool        confirm_delete;
    char *      alias[MAX_ALIAS];
    char *      alias_sub[MAX_ALIAS];
    char *      combat_action; /* default action to perform during combat */
    int         security;
    int         bounty;
    sh_int	pkpoints;      /* Initialy based on pkill_count, pkpoints determine pk grade */
    sh_int      pkill_count;
    sh_int      pkill_deaths;
    bool        confirm_pkill;
    sh_int      pkill_timer;
    sh_int      customduration;
    char *      customflag;
    sh_int      remorts;
    sh_int      original_stats[MAX_STATS];
    sh_int      history_stats[MAX_STATS];
    long        field;
    SMITH_DATA  *smith;
    PENALTY_DATA *new_penalty;
    int         demerit_points;
    sh_int      auth_state;
    char *      authed_by;
    char *      spouse;
    GRANT_DATA * granted;
    CRIME_DATA * crimes;
    char *	forget[MAX_FORGET];
    char *      invitation[MAX_CLAN];
    tflag       achievements;
    EXPLORE_DATA *explored;
    
	int warfare_hp;
	int warfare_mana;
	int warfare_move;

	sh_int morph_time;
	sh_int morph_race;

	/*
	 * Colour data stuff for config.
	 */
	
	int gossip[3];          /* {p */
	int gossip_text[3];     /* {P */
	int auction[3];         /* {a */
	int auction_text[3];    /* {A */
	int music[3];           /* {e */
	int music_text[3];      /* {E */
	int question[3];        /* {q */
	int question_text[3];   /* {Q */
	int answer[3];          /* {j */
	int answer_text[3];     /* {J */
	int quote[3];           /* {h */
	int quote_text[3];      /* {H */
	int gratz[3];           /* {z */
	int gratz_text[3];      /* {Z */
	int immtalk[3];         /* {i */
	int immtalk_text[3];    /* {I */
	int shouts[3];          /* {u */
	int shouts_text[3];     /* {U */
	int tells[3];           /* {t */
	int tell_text[3];       /* {T */
	int info[3];            /* {1 */
	int info_text[3];       /* {2 */
	int gametalk[3];        /* {k */
	int gametalk_text[3];   /* {K */
	int bitch[3];           /* {f */
	int bitch_text[3];      /* {F */
	int newbie[3];          /* {n */
	int newbie_text[3];     /* {N */
	int clan[3];            /* {l */
	int clan_text[3];       /* {L */
	int say[3];             /* {s */
	int say_text[3];        /* {S */
	int gtell[3];           /* {3 */
	int gtell_text[3];      /* {4 */
	int room_title[3];      /* {o */
	int room_exits[3];      /* {O */
	int wiznet[3];          /* {V */
	int warfare[3];         /* {5 */
	int warfare_text[3];    /* {6 */
	int savantalk[3];       /* {7 */
	int savantalk_text[3];  /* {8 */
	int proclaim[3];        /* {9 */
	int proclaim_text[3];   /* {0 */

#if 1
	/* quest stuff */
	CHAR_DATA *         questgiver; /* Vassago */
	int                 questpoints;  /* Vassago */
	sh_int              nextquest; /* Vassago */
	sh_int              countdown; /* Vassago */
	sh_int              questobj; /* Vassago */
	sh_int              questmob; /* Vassago */
	sh_int		    questroom;  /* Quirky */
	sh_int		    questarea;  /* Quirky */
	long		bank;
	sh_int      clan_rank; /* 0 for unguilded and loner, > 0 otherwise */
	tflag        tag_flags;
	int                 total_wars;
	int		    warpoints;
	int                 war_kills;
	int                 armageddon_won;
	int                 armageddon_lost;
	int                 armageddon_kills;
	int                 class_won;
	int                 class_lost;
	int                 class_kills;
	int                 race_won;
	int                 race_lost;
	int                 race_kills;
	int                 clan_won;
	int                 clan_lost;
	int                 clan_kills;
	int                 mob_kills;
	int                 mob_deaths;
	int                 quest_failed;
	int                 quest_success;
	int                 gender_won;
	int                 gender_lost;
	int                 gender_kills;
	int                 religion_won;
	int                 religion_lost;
	int                 religion_kills;
#endif
    QUEST_DATA *qdata;
    long house;            /* Settable by imms, shows only in do_worth, lets people feel their house is part of their assets */
    tattoo_list tattoos;
    FOLLOWER_DATA *ch_rel;
    time_t prayed_at;
    PRAYER_DATA *prayer_request;

    time_t pkill_expire; /* timestamp when you can turn it off */

};

/* Data for special quests */
struct quest_data
{
    QUEST_DATA *next;
    int id;
    int status;
    bool valid;
    int timer; /* Used for repeatable Mini Quests -Astark 10-9-12 */
    time_t limit;
};

/* Data for portal locations */
struct portal_data
{
    PORTAL_DATA *next;
    int vnum;
    char *name;
    bool valid;
};

/* Data for generating characters -- only used during generation */
struct gen_data
{
	GEN_DATA    *next;
	bool    valid;
	bool    skill_chosen[MAX_SKILL];
	bool    group_chosen[MAX_GROUP];
	int unused_die[MAX_STATS+5];
	int assigned_die[MAX_STATS+5];
	int stat_priority[MAX_STATS+5];
	int     points_chosen;
};



/*
 * Liquids.
 */
#define LIQ_WATER        0

struct  liq_type
{
	char *  liq_name;
	char *  liq_color;
	sh_int  liq_affect[5];
};



/*
 * Extra description data for a room or object.
 */
struct  extra_descr_data
{
	EXTRA_DESCR_DATA *next; /* Next in list                     */
	bool valid;
	char *keyword;              /* Keyword in look/examine          */
	char *description;          /* What to see                      */
};



/*
 * Prototype for an object.
 */
struct  obj_index_data
{
	OBJ_INDEX_DATA *    next;
	EXTRA_DESCR_DATA *  extra_descr;
	AFFECT_DATA *   affected;
	AREA_DATA *        area;       /* OLC */
	bool        new_format;
	char *      name;
	char *      short_descr;
	char *      description;
	int         vnum;
	sh_int      reset_num;
	char *      material;
	sh_int      item_type;
	tflag       extra_flags;
	tflag       wear_flags;
	sh_int      level;
	sh_int      condition;
	sh_int      count;
	sh_int      weight;
	int         cost;
	int         value[5];
	sh_int      durability;
	sh_int	    clan;
	sh_int	    rank;
        int         combine_vnum;
        sh_int      diff_rating; /* difficulty to get object */
};



/*
 * One object.
 */
struct  obj_data
{
	OBJ_DATA *      next;
	OBJ_DATA *      next_content;
	OBJ_DATA *      contains;
	OBJ_DATA *      in_obj;
	OBJ_DATA *      on;
	CHAR_DATA *     carried_by;
	EXTRA_DESCR_DATA *  extra_descr;
	AFFECT_DATA *   affected;
	OBJ_INDEX_DATA *    pIndexData;
	ROOM_INDEX_DATA *   in_room;
	bool        valid;
	char *          owner;
	char *      name;
	char *      short_descr;
	char *      description;
	sh_int      item_type;
	tflag         extra_flags;
	tflag         wear_flags;
	sh_int      wear_loc;
	sh_int      weight;
	int         cost;
	sh_int      level;
	sh_int      condition;
	char *      material;
	sh_int      timer;
	int         value   [5];
	sh_int  durability;
	sh_int	clan;
	sh_int	rank;
};



/*
 * Exit data.
 */
struct  exit_data
{
	union
	{
	    ROOM_INDEX_DATA * to_room;
	    int          vnum;
	} u1;
	tflag       exit_info;
	sh_int      key;
	char *      keyword;
	char *      description;
        EXIT_DATA * next;       /* OLC */
	tflag       rs_flags;   /* OLC */
	int         orig_door;  /* OLC */
};



/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile 
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct  reset_data
{
	RESET_DATA *    next;
	char        command;
	sh_int      arg1;
	sh_int      arg2;
	sh_int      arg3;
	sh_int      arg4;
};



/*
 * Area definition.
 */
struct  area_data
{
	AREA_DATA * next;
        HELP_AREA * helps;
	char *      file_name;
	char *      name;
	char *      credits;
	sh_int      age;
	sh_int      nplayer;
	sh_int      reset_time;
	int      min_vnum;
	int      max_vnum;
	bool        save;
	bool        empty;
        char *      builders;   /* OLC */ /* Listing of */
	int         vnum;       /* OLC */ /* Area vnum  */
	tflag       area_flags; /* OLC */
	int         security;   /* OLC */ /* Value 1-9  */
        int         clones[MAX_AREA_CLONE]; /* area cloning */
      /* Added minlevel, maxlevel, and mini-quests for new areas command 
         - Astark Dec 2012 */
        int      minlevel;
        int      maxlevel;
        int      miniquests;
};



/*
 * Room type.
 */
struct  room_index_data
{
    ROOM_INDEX_DATA *   next;
    CHAR_DATA *     people;
    OBJ_DATA *      contents;
    EXTRA_DESCR_DATA *  extra_descr;
    AREA_DATA *     area;
    EXIT_DATA *     exit    [10];
    RESET_DATA *   reset_first;    /* OLC */
    char *      name;
    char *      description;
    char *      owner;
    int      vnum;
    tflag       room_flags;
    sh_int      light;
    sh_int      sector_type;
    sh_int      heal_rate;
    sh_int      mana_rate;
    sh_int      clan;
    sh_int      clan_rank;
};



/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED               -1
#define TYPE_HIT                     1000



/*
 *  Target types.
 */
#define TAR_IGNORE              0
#define TAR_CHAR_OFFENSIVE      1
#define TAR_CHAR_DEFENSIVE      2
#define TAR_CHAR_SELF           3
#define TAR_OBJ_INV             4
#define TAR_OBJ_CHAR_DEF        5
#define TAR_OBJ_CHAR_OFF        6
#define TAR_VIS_CHAR_OFF        7
#define TAR_CHAR_NEUTRAL        8

#define TARGET_CHAR         0
#define TARGET_OBJ          1
#define TARGET_ROOM         2
#define TARGET_NONE         3

#define TAR_NONE        0
#define TAR_SELF        1
#define TAR_GROUP       2
#define TAR_NEUTRAL 4
#define TAR_ENEMY       8
#define TAR_OFFENSIVE   16

#define TAR_ALL     15
#define TAR_FRIEND  3
#define TAR_NOT_FRIEND  12
#define TAR_NOT_SELF    14
#define TAR_NOT_NEUTRAL 11
#define TAR_NOT_ENEMY   7
#define TAR_NOT_GROUP   13



/*
 * Skills include spells as a particular case.
 */
struct  skill_type
{
	char *  name;           /* Name of skill        */
	sh_int  skill_level[MAX_CLASS]; /* Level needed by class    */
	sh_int  rating[MAX_CLASS];  /* How hard it is to learn  */
	sh_int	cap[MAX_CLASS];		/* Maximum learnable percentage */
	sh_int	stat_prime, stat_second, stat_third;
	SPELL_FUN * spell_fun;      /* Spell pointer (for spells)   */
	sh_int  target;         /* Legal targets        */
	sh_int  minimum_position;   /* Position for caster / user   */
	sh_int *pgsn;           /* Pointer to associated gsn    */
	sh_int  slot;           /* Slot for #OBJECT loading */
	sh_int  min_mana;       /* Minimum mana used        */
	sh_int  beats;          /* Waiting time after use   */
	char *  noun_damage;        /* Damage message       */
	char *  msg_off;        /* Wear off message     */
	char *  msg_obj;        /* Wear off message for obects  */
};


struct  group_type
{
	char *  name;
	sh_int  rating[MAX_CLASS];
	char *  spells[MAX_IN_GROUP];
};

/*
 * MOBprog definitions
 */                   
#define TRIG_ACT    (A)
#define TRIG_BRIBE  (B)
#define TRIG_DEATH  (C)
#define TRIG_ENTRY  (D)
#define TRIG_FIGHT  (E)
#define TRIG_GIVE   (F)
#define TRIG_GREET  (G)
#define TRIG_GRALL  (H)
#define TRIG_KILL   (I)
#define TRIG_HPCNT  (J)
#define TRIG_RANDOM (K)
#define TRIG_SPEECH (L)
#define TRIG_EXIT   (M)
#define TRIG_EXALL  (N)
#define TRIG_DELAY  (O)
#define TRIG_SURR   (P)
#define TRIG_DRBOMB (Q)
#define TRIG_EXBOMB (R)
#define TRIG_DEFEAT (S)
#define TRIG_SOCIAL (T)
#define TRIG_TRY    (U)
#define TRIG_RESET  (V)
#define TRIG_MPCNT  (W)
#define TRIG_SPELL  (X)

struct mprog_list
{
	int         trig_type;
	char *      trig_phrase;
	int         vnum;
	char *          code;
	MPROG_LIST *    next;
	bool        valid;
    bool is_lua;
};

struct mprog_code
{
    bool        is_lua;
	int         vnum;
	char *      code;
	MPROG_CODE *    next;
};

extern sh_int race_werewolf;
extern sh_int race_naga;
extern sh_int race_doppelganger;
extern sh_int race_vampire;

/*
 * These are skill_lookup return values for common skills and spells.
 */
extern  sh_int  gsn_mindflay;
extern  sh_int  gsn_backstab;
extern  sh_int  gsn_blackjack;
extern  sh_int  gsn_beheading;
extern  sh_int  gsn_circle;
extern  sh_int  gsn_dodge;
extern  sh_int  gsn_envenom;
extern  sh_int  gsn_hide;
extern  sh_int  gsn_peek;
extern  sh_int  gsn_pick_lock;
extern  sh_int  gsn_sneak;
extern  sh_int  gsn_steal;
extern  sh_int  gsn_disguise;
extern  sh_int  gsn_disarm_trap;

extern  sh_int  gsn_disarm;
extern  sh_int  gsn_enhanced_damage;
extern  sh_int  gsn_kick;
extern sh_int  gsn_gouge;
extern sh_int  gsn_chop;
extern sh_int  gsn_bite;
extern sh_int  gsn_melee;
extern sh_int  gsn_brawl;
extern sh_int  gsn_guard;
extern sh_int  gsn_kung_fu;
extern sh_int  gsn_leg_sweep;
extern sh_int  gsn_endurance;
extern sh_int  gsn_uppercut;
extern sh_int  gsn_war_cry;
extern sh_int  gsn_dual_wield;
extern sh_int  gsn_dual_dagger;
extern sh_int  gsn_dual_sword;
extern sh_int  gsn_dual_axe;
extern sh_int  gsn_dual_gun;
extern sh_int  gsn_tumbling;
extern sh_int  gsn_feint;
extern sh_int  gsn_distract;
extern sh_int  gsn_avoidance;
extern sh_int  gsn_parry;
extern sh_int  gsn_rescue;
extern sh_int  gsn_second_attack;
extern sh_int  gsn_third_attack;
extern sh_int  gsn_sustenance;   
extern sh_int  gsn_hunt;
extern sh_int  gsn_pathfind;
extern sh_int  gsn_streetwise;
extern sh_int  gsn_ignite;
extern sh_int  gsn_assassination;
extern sh_int  gsn_brutal_damage;
extern sh_int  gsn_razor_claws;

extern sh_int  gsn_blindness;
extern sh_int  gsn_charm_person;
extern sh_int  gsn_curse;
extern sh_int  gsn_invis;
extern sh_int  gsn_astral;
extern sh_int  gsn_mass_invis;
extern sh_int  gsn_plague;
extern sh_int  gsn_poison;
extern sh_int  gsn_sleep;
extern sh_int  gsn_fly;
extern sh_int  gsn_sanctuary;
extern sh_int  gsn_necrosis;
extern sh_int  gsn_ritual;
extern sh_int  gsn_word_of_recall;
extern sh_int  gsn_fear;
extern sh_int  gsn_confusion;
extern sh_int  gsn_mass_confusion;
extern sh_int  gsn_haste;
extern sh_int  gsn_giant_strength;
extern sh_int  gsn_slow;

/* new gsns */
extern sh_int  gsn_axe;
extern sh_int  gsn_dagger;
extern sh_int  gsn_flail;
extern sh_int  gsn_mace;
extern sh_int  gsn_polearm;
extern sh_int  gsn_gun;
extern sh_int  gsn_bow;
extern sh_int  gsn_shield_block;
extern sh_int  gsn_wrist_shield;
extern sh_int  gsn_spear;
extern sh_int  gsn_sword;
extern sh_int  gsn_whip;
extern sh_int  gsn_fire_breath;
 
extern sh_int  gsn_craft;
extern sh_int  gsn_crush;
extern sh_int  gsn_bash;
extern sh_int  gsn_berserk;
extern sh_int  gsn_dirt;
extern sh_int  gsn_hand_to_hand;
extern sh_int  gsn_trip;
extern sh_int  gsn_fervent_rage;
extern sh_int  gsn_fervent_rage_cooldown;
extern sh_int  gsn_paroxysm;
extern sh_int  gsn_paroxysm_cooldown;
extern sh_int  gsn_rupture;
extern sh_int  gsn_phase;
extern sh_int  gsn_replenish;
extern sh_int  gsn_replenish_cooldown;
 
extern sh_int  gsn_fast_healing;
extern sh_int  gsn_haggle;
extern sh_int  gsn_arcane_lore;
extern sh_int  gsn_lore;
extern sh_int  gsn_meditation;
extern sh_int  gsn_appraise;
extern sh_int  gsn_weapons_lore;

extern sh_int  gsn_alertness;
extern sh_int  gsn_evasive;
extern sh_int  gsn_fatal_blow;
extern sh_int  gsn_two_handed;
 
extern sh_int  gsn_scrolls;
extern sh_int  gsn_staves;
extern sh_int  gsn_wands;
extern sh_int  gsn_recall;
extern sh_int  gsn_flee;
extern sh_int  gsn_retreat;
extern sh_int  gsn_entrapment;

extern sh_int  gsn_mug;
extern sh_int  gsn_headbutt;
extern sh_int  gsn_net;
extern sh_int  gsn_regeneration; 
extern sh_int  gsn_drain_life; 
extern sh_int  gsn_snipe;
extern sh_int  gsn_unjam;
extern sh_int  gsn_burst;
extern sh_int  gsn_tight_grouping;
extern sh_int  gsn_pistol_whip;
extern sh_int  gsn_duck;
extern sh_int  gsn_quick_draw;
extern sh_int  gsn_shoot_lock;
extern sh_int  gsn_drunken_fury;
extern sh_int  gsn_thousand_yard_stare;
extern sh_int  gsn_set_snare;
extern sh_int  gsn_peel;
extern sh_int  gsn_aim;
extern sh_int  gsn_semiauto;
extern sh_int  gsn_fullauto;
extern sh_int  gsn_hogtie;
extern sh_int  gsn_elude;
extern sh_int  gsn_estimate;

extern sh_int  gsn_forage;
extern sh_int  gsn_torch;
extern sh_int  gsn_shelter;
extern sh_int  gsn_firstaid;
extern sh_int  gsn_detoxify;
extern sh_int  gsn_tame;

extern sh_int gsn_bear;
extern sh_int gsn_boa;
extern sh_int gsn_bunny;
extern sh_int gsn_dragon;
extern sh_int gsn_eagle;
extern sh_int gsn_eel;
extern sh_int gsn_lion;
extern sh_int gsn_phoenix;
extern sh_int gsn_porcupine;
extern sh_int gsn_rhino;
extern sh_int gsn_scorpion;
extern sh_int gsn_tiger;
extern sh_int gsn_toad;
extern sh_int gsn_tortoise;
extern sh_int gsn_unicorn;
extern sh_int gsn_finesse;
extern sh_int gsn_rage;
extern sh_int gsn_retribution;
extern sh_int gsn_serpent;
extern sh_int gsn_blade_dance;
extern sh_int gsn_shadowclaw;
extern sh_int gsn_shadowessence;
extern sh_int gsn_shadowsoul;
extern sh_int gsn_shadowwalk;
extern sh_int gsn_ambush;
extern sh_int gsn_anklebiter;
extern sh_int gsn_arcana;
extern sh_int gsn_bloodbath;
extern sh_int gsn_kamikaze;
extern sh_int gsn_showdown;
extern sh_int gsn_target_practice;
extern sh_int gsn_stalk;
extern sh_int gsn_korinns_inspiration;
extern sh_int gsn_parademias_bile;
extern sh_int gsn_firewitchs_seance;
extern sh_int gsn_swaydes_mercy;
extern sh_int gsn_quirkys_insanity;
extern sh_int gsn_cone_of_exhaustion;
extern sh_int gsn_zombie_breath;
extern sh_int gsn_zone_of_damnation;
extern sh_int gsn_decompose;
extern sh_int gsn_overcharge;

extern sh_int gsn_immolation;
extern sh_int gsn_epidemic;
extern sh_int gsn_absolute_zero;
extern sh_int gsn_electrocution;
extern sh_int gsn_shadow_shroud;
extern sh_int gsn_jihad;
extern sh_int gsn_vampire_hunting;
extern sh_int gsn_witch_hunting;
extern sh_int gsn_werewolf_hunting;
extern sh_int gsn_inquisition;
extern sh_int  gsn_raft; 
extern sh_int  gsn_giantfeller;        
extern sh_int  gsn_woodland_combat;        
extern sh_int  gsn_taxidermy;
extern sh_int  gsn_introspection;
extern sh_int  gsn_climbing;
extern sh_int  gsn_blindfighting;
extern sh_int  gsn_beast_mastery;
extern sh_int  gsn_camp_fire;
extern sh_int  gsn_treat_weapon;
extern sh_int  gsn_soothe;
extern sh_int  gsn_fishing;
extern sh_int  gsn_goblincleaver;
extern sh_int  gsn_tempest;
extern sh_int  gsn_wendigo;
extern sh_int  gsn_venom_bite;
extern sh_int  gsn_vampiric_bite;
extern sh_int  gsn_maul;
extern sh_int  gsn_extra_attack;
extern sh_int  gsn_swimming;

extern sh_int gsn_fade;
extern sh_int gsn_minor_fade;
extern sh_int  gsn_shield_bash;
extern sh_int  gsn_choke_hold;
extern sh_int  gsn_roundhouse;
extern sh_int  gsn_hurl;
extern sh_int  gsn_spit;
extern sh_int  gsn_laughing_fit;
extern sh_int  gsn_deaths_door;
extern sh_int  gsn_blessed_darkness;
extern sh_int  gsn_tomb_rot;
extern sh_int  gsn_bless;
extern sh_int  gsn_prayer;
extern sh_int  gsn_bodyguard;
extern sh_int  gsn_back_leap;
extern sh_int  gsn_mana_shield;
extern sh_int  gsn_leadership;
extern sh_int  gsn_reflection;
extern sh_int  gsn_prot_magic;
extern sh_int  gsn_focus;
extern sh_int  gsn_anatomy;
extern sh_int  gsn_mimic;
extern sh_int  gsn_mirror_image;
extern sh_int  gsn_intimidation;
extern sh_int  gsn_dowsing;
extern sh_int  gsn_rustle_grub;
extern sh_int  gsn_slash_throat;
extern sh_int  gsn_puppetry;
extern sh_int  gsn_jump_up;
extern sh_int  gsn_survey;
extern sh_int  gsn_charge;
extern sh_int  gsn_enchant_arrow;
extern sh_int  gsn_fledging;
extern sh_int  gsn_mass_combat;
extern sh_int  gsn_double_strike;
extern sh_int  gsn_round_swing;
extern sh_int  gsn_sharp_shooting;
extern sh_int  gsn_puncture;
extern sh_int  gsn_scribe;
extern sh_int  gsn_alchemy;
extern sh_int  gsn_dimensional_blade;
extern sh_int  gsn_elemental_blade;
extern sh_int  gsn_ashura;
extern sh_int  gsn_shan_ya;

/* astark stuff */

extern sh_int  gsn_aversion;
extern sh_int  gsn_strafe;
extern sh_int  gsn_critical;
extern sh_int  gsn_infectious_arrow;
extern sh_int  gsn_unearth;
//extern sh_int  gsn_combo_attack;
extern sh_int  gsn_magic_missile;
extern sh_int  gsn_acid_blast;
extern sh_int  gsn_armor;
extern sh_int  gsn_power_thrust;
extern sh_int  gsn_natural_resistance;
extern sh_int  gsn_quivering_palm;
extern sh_int  gsn_bless;
extern sh_int  gsn_blindness;
extern sh_int  gsn_burning_hands;
extern sh_int  gsn_call_lightning;
extern sh_int  gsn_calm;
extern sh_int  gsn_basic_apparition;
extern sh_int  gsn_holy_apparition;
extern sh_int  gsn_phantasmal_image;
extern sh_int  gsn_shroud_of_darkness;
extern sh_int  gsn_paralysis_poison;
extern sh_int  gsn_hailstorm;
extern sh_int  gsn_control_weather;
extern sh_int  gsn_call_lightning;
extern sh_int  gsn_lightning_bolt;
extern sh_int  gsn_monsoon;
extern sh_int  gsn_meteor_swarm;
extern sh_int  gsn_smotes_anachronism;
extern sh_int  gsn_enchant_armor;
extern sh_int  gsn_enchant_weapon;
extern sh_int  gsn_enchant_arrow;
extern sh_int  gsn_solar_flare;
extern sh_int  gsn_iron_hide;
extern sh_int  gsn_feeblemind;

extern sh_int  gsn_god_bless;
extern sh_int  gsn_god_curse;



/*
 * Struct information for achievements_entry
 */

struct achievement_entry
{
   //char type[MSL];
   int type;
   int limit;      
   int quest_reward; 
   int exp_reward;   
   int gold_reward; 
   int ach_reward; 
   int obj_reward;   
   int bit_vector;
};

/*Achievement types*/
/*if you change these, you need to update
  achievement_display*/
#define ACHV_NONE	0
#define ACHV_LEVEL 	1
#define ACHV_MKILL	2
#define ACHV_REMORT	3
#define ACHV_QCOMP	4
#define ACHV_WKILL	5
#define ACHV_WWIN	6
#define ACHV_BEHEAD	7
#define ACHV_PKILL	8
#define ACHV_AGE	9
#define ACHV_MAXHP	10
#define ACHV_MAXMN	11
#define	ACHV_MAXMV	12
#define ACHV_EXPLORED   13
//#define ACHV_TATT	14

/*bitvector for achievement tflag*/
#define    ACHIEVE_LEVEL_1 1
#define    ACHIEVE_LEVEL_2 2
#define    ACHIEVE_LEVEL_3 3
#define    ACHIEVE_LEVEL_4 4
#define    ACHIEVE_LEVEL_5 5
#define    ACHIEVE_LEVEL_6 6
#define    ACHIEVE_LEVEL_7 7
#define    ACHIEVE_LEVEL_8 8
#define    ACHIEVE_LEVEL_9 9
#define   ACHIEVE_LEVEL_10 10
#define   ACHIEVE_LEVEL_11 11
#define   ACHIEVE_LEVEL_12 12
#define   ACHIEVE_LEVEL_13 13
#define   ACHIEVE_LEVEL_14 14
#define   ACHIEVE_LEVEL_15 15
#define    ACHIEVE_MKILL_1 16
#define    ACHIEVE_MKILL_2 17
#define    ACHIEVE_MKILL_3 18
#define    ACHIEVE_MKILL_4 19
#define    ACHIEVE_MKILL_5 20
#define    ACHIEVE_MKILL_6 21
#define    ACHIEVE_MKILL_7 22
#define    ACHIEVE_MKILL_8 23
#define    ACHIEVE_MKILL_9 24
#define   ACHIEVE_MKILL_10 25
#define   ACHIEVE_MKILL_11 26
#define   ACHIEVE_REMORT_1 27
#define   ACHIEVE_REMORT_2 28
#define   ACHIEVE_REMORT_3 29
#define   ACHIEVE_REMORT_4 30
#define   ACHIEVE_REMORT_5 31
#define   ACHIEVE_REMORT_6 32
#define   ACHIEVE_REMORT_7 33
#define   ACHIEVE_REMORT_8 34
#define   ACHIEVE_REMORT_9 35
#define  ACHIEVE_REMORT_10 36
#define    ACHIEVE_QCOMP_1 37
#define    ACHIEVE_QCOMP_2 38
#define    ACHIEVE_QCOMP_3 39
#define    ACHIEVE_QCOMP_4 40
#define    ACHIEVE_QCOMP_5 41
#define    ACHIEVE_QCOMP_6 42
#define    ACHIEVE_QCOMP_7 43
#define    ACHIEVE_QCOMP_8 44
#define    ACHIEVE_QCOMP_9 45
#define   ACHIEVE_QCOMP_10 46
#define ACHIEVE_WARKILLS_1 47
#define ACHIEVE_WARKILLS_2 48
#define ACHIEVE_WARKILLS_3 49
#define ACHIEVE_WARKILLS_4 50
#define ACHIEVE_WARKILLS_5 51
#define ACHIEVE_WARKILLS_6 52
#define ACHIEVE_WARKILLS_7 53
#define ACHIEVE_WARKILLS_8 54
#define  ACHIEVE_WARWINS_1 55
#define  ACHIEVE_WARWINS_2 56
#define  ACHIEVE_WARWINS_3 57
#define  ACHIEVE_WARWINS_4 58
#define  ACHIEVE_WARWINS_5 59
#define  ACHIEVE_WARWINS_6 60
#define  ACHIEVE_WARWINS_7 61
#define  ACHIEVE_WARWINS_8 62
#define  ACHIEVE_BEHEADS_1 63
#define  ACHIEVE_BEHEADS_2 64
#define  ACHIEVE_BEHEADS_3 65
#define  ACHIEVE_BEHEADS_4 66
#define  ACHIEVE_BEHEADS_5 67
#define  ACHIEVE_BEHEADS_6 68
#define  ACHIEVE_BEHEADS_7 69
#define  ACHIEVE_BEHEADS_8 70
#define   ACHIEVE_PKILLS_1 71
#define   ACHIEVE_PKILLS_2 72
#define   ACHIEVE_PKILLS_3 73
#define   ACHIEVE_PKILLS_4 74
#define   ACHIEVE_PKILLS_5 75
#define      ACHIEVE_AGE_1 76
#define      ACHIEVE_AGE_2 77
#define      ACHIEVE_AGE_3 78
#define      ACHIEVE_AGE_4 79
#define      ACHIEVE_AGE_5 80
#define     ACHIEVE_TATT_1 81
#define     ACHIEVE_TATT_2 82
#define     ACHIEVE_TATT_3 83
#define    ACHIEVE_MAXHP_1 84
#define    ACHIEVE_MAXHP_2 85
#define    ACHIEVE_MAXHP_3 86
#define    ACHIEVE_MAXHP_5 87
#define    ACHIEVE_MAXHP_4 88 /* MAXHP_4 and MAXHP_5 have been inversed due to the numbers
                                 being set to the same value during the initial relase of
                                 the code. This gave anyone with 10k hp, the 15k hp achievement
                                 as well. - Astark */
#define    ACHIEVE_MAXMN_1 89
#define    ACHIEVE_MAXMN_2 90
#define    ACHIEVE_MAXMN_3 91
#define    ACHIEVE_MAXMN_4 92
#define    ACHIEVE_MAXMN_5 93
#define    ACHIEVE_MAXMV_1 94
#define    ACHIEVE_MAXMV_2 95
#define    ACHIEVE_MAXMV_3 96
#define    ACHIEVE_MAXMV_4 97
#define    ACHIEVE_EXPLORED_1 98
#define    ACHIEVE_EXPLORED_2 99
#define    ACHIEVE_EXPLORED_3 100
#define    ACHIEVE_EXPLORED_4 101
#define    ACHIEVE_EXPLORED_5 102
#define    ACHIEVE_EXPLORED_6 103
#define    ACHIEVE_EXPLORED_7 104
#define    ACHIEVE_EXPLORED_8 105

/*
#define ACHIEVE_LEVEL_1    A
#define ACHIEVE_LEVEL_2    B
#define ACHIEVE_LEVEL_3    D
#define ACHIEVE_LEVEL_4    E
#define ACHIEVE_LEVEL_5    F
#define ACHIEVE_LEVEL_6    G
#define ACHIEVE_LEVEL_7    H
#define ACHIEVE_LEVEL_8    I
#define ACHIEVE_LEVEL_9    J
#define ACHIEVE_LEVEL_10   K
#define ACHIEVE_MKILL_1    L
#define ACHIEVE_MKILL_2    M
#define ACHIEVE_MKILL_3    N
#define ACHIEVE_MKILL_4    O
*/



/*
 * Utility macros.
 */
#define IS_VALID(data)      ((data) != NULL && (data)->valid)
#define VALIDATE(data)      ((data)->valid = TRUE)
#define INVALIDATE(data)    ((data)->valid = FALSE)
#define UMIN(a, b)      ((a) < (b) ? (a) : (b))
#define UMAX(a, b)      ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)     ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)        ((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)        ((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define ABS(a)          ((a) >= 0 ? (a) : -(a))
/*
#define SET_BIT(var, bit)  ((bit) > 0 ? ((var) |= (bit)) : (*((&((var))) + 1) |= (bit))) 
#define REMOVE_BIT(var, bit)  ((bit) > 0 ? ((var) &= ~(bit)) : (*((&((var))) + 1) &= ~(bit))) 
#define IS_SET(var, bit)  ((bit) > 0 ? ((var) & (bit)) : (*((&((var))) + 1) & (bit) & 2147483647)) 
*/
/* integer flags - used on objects in the value field */
#define I_BIT(bit)             (1L << ((bit)-1))
#ifdef ASSERT_DEBUG
#define I_SET_BIT(var, bit)    assert(0 < (bit)), ((var) |= I_BIT(bit)) 
#define I_REMOVE_BIT(var, bit) assert(0 < (bit)), ((var) &= ~I_BIT(bit)) 
#define I_IS_SET(var, bit)     (assert(0 < (bit)), ((var) & I_BIT(bit)))
#define I_TOGGLE_BIT(var, bit) assert(0 < (bit)), ((var) ^= I_BIT(bit))
#else
#define I_SET_BIT(var, bit)    ((var) |= I_BIT(bit)) 
#define I_REMOVE_BIT(var, bit) ((var) &= ~I_BIT(bit)) 
#define I_IS_SET(var, bit)     ((var) & I_BIT(bit))
#define I_TOGGLE_BIT(var, bit) ((var) ^= I_BIT(bit))
#endif

#ifdef ASSERT_DEBUG
#define SET_BIT(flag, bit)     assert(0 < (bit)), flag_set(flag, bit)
#define REMOVE_BIT(flag, bit)  assert(0 < (bit)), flag_remove(flag, bit)
#define IS_SET(flag, bit)      (assert(0 < (bit)), flag_is_set(flag, bit))
#define TOGGLE_BIT(flag, bit)  assert(0 < (bit)), flag_toggle(flag, bit)
#else
#define SET_BIT(flag, bit)     flag_set(flag, bit)
#define REMOVE_BIT(flag, bit)  flag_remove(flag, bit)
#define IS_SET(flag, bit)      flag_is_set(flag, bit)
#define TOGGLE_BIT(flag, bit)  flag_toggle(flag, bit)
#endif

#define SET_BITS(flag, bits)     flag_set_field(flag, bits)
#define REMOVE_BITS(flag, bits)  flag_remove_field(flag, bits)
#define FLAG_CONVERT(flag)       flag = flag_convert_old( flag )

#define IS_NULLSTR(str)     ((str) == NULL || (str)[0] == '\0')
#define IS_BETWEEN(min,num,max) ( ((min) <= (num)) && ((num) <= (max)) )
#define CHECK_POS(a, b, c)  { (a) = (b); if ( (a) < 0 ) bug( "CHECK_POS : " c " == %d < 0", a ); }
#define IS_SPELL(sn) (skill_table[sn].spell_fun != spell_null)
#define IS_CHAN_OFF(ch, sn)	(IS_SET( ch->comm, public_channel_table[sn].offbit))


/*
 * Character macros.
 */
#define IS_NPC(ch)      (IS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch) (ch->desc ? is_granted_name(ch,"immflag") : get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)     ((!IS_NPC((ch))&&((ch)->level >= ((LEVEL_HERO - 10)+((ch)->pcdata->remorts)))))
#define IS_HELPER(ch)   (!IS_NPC(ch) && IS_SET((ch)->act, PLR_HELPER))
#define IS_ACTIVE_HELPER(ch)    (!IS_NPC(ch) && IS_SET((ch)->act, PLR_HELPER) && !IS_SET((ch)->act, PLR_INACTIVE_HELPER))
#define CAN_AUTH(ch)    is_granted_name(ch,"authorize")
#define IS_TRUSTED(ch,level)    (get_trust((ch)) >= (level))
#define IS_INCOG(ch) ((ch)->incog_level >= LEVEL_IMMORTAL)
#define IS_WIZI(ch) ((ch)->invis_level >= LEVEL_IMMORTAL)
#define IS_DEAD(ch) ((ch)->just_killed || (ch)->position == POS_DEAD || !IS_VALID(ch))
#define CHECK_RETURN(ch, victim) if (stop_attack(ch, victim)) return
#define IS_UNDEAD(ch) (IS_SET(ch->form, FORM_UNDEAD) || NPC_ACT(ch,ACT_UNDEAD))

#define SET_AFFECT(ch, sn)          SET_BIT((ch)->affect_field, sn)
#define REMOVE_AFFECT(ch, sn)       REMOVE_BIT((ch)->affect_field, sn)
#define IS_AFFECTED(ch, sn)         IS_SET((ch)->affect_field, sn)
#define HAS_AFFECTS(ch)             (!flag_is_empty((ch)->affect_field))
#define COPY_AFFECTS(ch1, ch2)      flag_copy((ch1)->affect_field, (ch2)->affect_field)

#define NPC_ACT(ch, flag) (IS_NPC(ch) && IS_SET((ch)->act, flag))
#define PLR_ACT(ch, flag) (!IS_NPC(ch) && IS_SET((ch)->act, flag))

#define IS_QUESTOR(ch)     (IS_SET((ch)->act, PLR_QUESTOR))         
#define IS_QUESTORHARD(ch)     (IS_SET((ch)->act, PLR_QUESTORHARD))

#define GET_AGE(ch)     ((int) (17 + ((ch)->played + current_time - (ch)->logon )/72000))
#define TIME_PLAYED(ch) ((ch)->played + current_time - (ch)->logon)

#define IS_GOOD(ch)     (ch->alignment >= 350)
#define IS_EVIL(ch)     (ch->alignment <= -350)
#define IS_NEUTRAL(ch)      (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_AWAKE(ch)        (ch->position > POS_SLEEPING)
#define GET_AC(ch,type) get_ac(ch,type)
#define GET_HITROLL(ch) get_hitroll(ch)
#define GET_DAMROLL(ch) get_damroll(ch)

#define IS_OUTSIDE(ch)      (!IS_SET((ch)->in_room->room_flags, ROOM_INDOORS))

#define WAIT_STATE(ch, npulse)  ((ch)->wait = UMAX((ch)->wait, (npulse)))
#define DAZE_STATE(ch, npulse)  ((ch)->daze = UMAX((ch)->daze, (npulse)))
#define get_carry_weight(ch)    ((ch)->carry_weight + (ch)->silver/100 + (ch)->gold/25)
#define HAS_TRIGGER(ch,trig)    (IS_SET((ch)->pIndexData->mprog_flags,(trig)))
#define IS_SWITCHED( ch )       ( ch->desc && ch->desc->original )
#define IS_BUILDER(ch, Area)    ( !IS_NPC(ch) && !IS_SWITCHED( ch ) && (ch->pcdata->security >= Area->security || strstr( Area->builders, ch->name ) || strstr( Area->builders, "All" ) ) )
#define IS_REMORT(ch)			(!IS_NPC(ch) && IS_SET(ch->in_room->area->area_flags, AREA_REMORT)) 
#define IS_NOHIDE(ch)           (!IS_NPC(ch) && IS_SET(ch->in_room->area->area_flags, AREA_NOHIDE))

#define IS_WRITING_NOTE(con)  (( (con >= CON_NOTE_TO && con <= CON_NOTE_FINISH) \
            || (con >= CON_PENALTY_SEVERITY && con <= CON_PENALTY_FINISH) \
            ) ? TRUE : FALSE)
#define IS_PLAYING(con)         (con == CON_PLAYING || IS_WRITING_NOTE(con))
#define DESC_PC(desc)         (desc->original ? desc->original : desc->character)

#define NOT_AUTHED(ch)   (!IS_NPC(ch) && get_auth_state( ch ) != AUTH_AUTHED && IS_SET(ch->act, PLR_UNAUTHED) )

#define IS_AUTHED(ch)           (!NOT_AUTHED(ch))
/*
#define NEW_AUTH( ch )          (!IS_NPC(ch) && ch->level == 1 )
*/
	
#define IS_WAITING_FOR_AUTH(ch) (!IS_NPC(ch) && ch->desc && get_auth_state( ch ) == AUTH_ONLINE && IS_SET(ch->act, PLR_UNAUTHED) ) 
#define IS_TAG(ch) (!IS_NPC(ch) && IS_SET((ch)->pcdata->tag_flags, TAG_PLAYING))

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part) (IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)  (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj,stat)(I_IS_SET((obj)->value[4],(stat)))
#define SET_WEAPON_STAT(obj,stat) (I_SET_BIT((obj)->value[4],(stat)))
#define WEIGHT_MULT(obj)    ((obj)->item_type == ITEM_CONTAINER ? (obj)->value[4] : -1)

/*
 * Description macros.
 */
#define PERS(ch, looker) get_mimic_PERS(ch, looker)
//#define PERS(ch, looker) get_mimic_PERS_new(ch, looker, NULL)
//#define PERS(ch, looker, gagtype) get_mimic_PERS_new(ch, looker, gagtype)
/*
#define PERS(ch, looker)    ( can_see( looker, (ch) ) ? ( IS_NPC(ch) ? (ch)->short_descr : (ch)->name ) : "someone" )
*/

/*
 * Structure for a social in the socials table.
 */

struct  social_type
{
	char     name[20];
	char *    char_no_arg;
	char *    others_no_arg;
	char *    char_found;
	char *    others_found;
	char *    vict_found;
	char *    char_not_found;
	char *      char_auto;
	char *      others_auto;
};

struct stance_type
{
	char *  name;
	int     key;
	int     type;
	char *  verb;
	sh_int *        gsn;
	bool        martial;
	bool		weapon;
	int     cost;
};

/* Note: The values for these constants are used as the index of the corresponding
         entries in the stances table, in tables.c.  Thus, you *must* define them
         contiguously, and in exactly the same order as the stances entries. */
#define STANCE_DEFAULT 0
#define STANCE_BEAR 1
#define STANCE_BOA 2
#define STANCE_DRAGON 3
#define STANCE_EAGLE 4
#define STANCE_EEL 5
#define STANCE_LION 6
#define STANCE_PHOENIX 7
#define STANCE_PORCUPINE 8 
#define STANCE_RHINO 9
#define STANCE_SCORPION 10
#define STANCE_TIGER 11
#define STANCE_TOAD 12
#define STANCE_TORTOISE 13
#define STANCE_UNICORN 14
#define STANCE_FINESSE 15
#define STANCE_RAGE 16
#define STANCE_RETRIBUTION 17
#define STANCE_BLADE_DANCE 18
#define STANCE_SHADOWCLAW 19
#define STANCE_SHADOWESSENCE 20
#define STANCE_SHADOWSOUL 21
#define STANCE_SHADOWWALK 22
#define STANCE_AMBUSH 23
#define STANCE_BLOODBATH 24
#define STANCE_KAMIKAZE 25
#define STANCE_SHOWDOWN 26
#define STANCE_TARGET_PRACTICE 27
#define STANCE_JIHAD 28
#define STANCE_VAMPIRE_HUNTING 29
#define STANCE_WITCH_HUNTING 30
#define STANCE_WEREWOLF_HUNTING 31
#define STANCE_INQUISITION 32
#define STANCE_TEMPEST 33
#define STANCE_GOBLINCLEAVER 34
#define STANCE_WENDIGO 35
#define STANCE_KORINNS_INSPIRATION 36
#define STANCE_PARADEMIAS_BILE 37
#define STANCE_SWAYDES_MERCY 38
#define STANCE_FIREWITCHS_SEANCE 39
#define STANCE_BUNNY 40
#define STANCE_ANKLEBITER 41
#define STANCE_ARCANA 42
#define STANCE_DIMENSIONAL_BLADE 43
#define STANCE_ELEMENTAL_BLADE 44
#define STANCE_AVERSION 45
#define STANCE_SERPENT 46

/* morph race constants */
#define MAX_MORPH_RACE     3
#define MORPH_NAGA_SERPENT 0
#define MORPH_NAGA_HUMAN   1
#define MORPH_WOLFMAN      2

/*
 * Global constants.
 */

/* warfare.c */
extern const struct stance_type   stances[];
extern  const   struct  class_type  class_table [MAX_CLASS];
extern  const   struct  weapon_type weapon_table    [];
extern  const   struct  item_type   item_table  [];
extern  const   struct  wiznet_type wiznet_table    [];
extern  const   struct  attack_type attack_table    [];

/* race tables */
extern  struct  race_type       race_table  [];
extern  struct  pc_race_type    pc_race_table   [MAX_PC_RACE];
extern  struct  race_type       morph_race_table[];
extern  struct  pc_race_type    morph_pc_race_table[];

/* align table */
extern  struct  align_type    align_table[];

extern  const   struct  spec_type   spec_table  [];
extern  const   struct  liq_type    liq_table   [];
extern  struct  skill_type  skill_table [MAX_SKILL];
extern  const   struct  group_type      group_table [MAX_GROUP];
extern          struct  social_type *social_table;
extern  char *  const           title_table [MAX_CLASS] [23];
extern	        struct  clan_data       clan_table[MAX_CLAN];




/*
 * Global variables.
 */
extern      HELP_DATA     * help_first;
extern      SHOP_DATA     * shop_first;

extern      CHAR_DATA     * char_list;
extern      DESCRIPTOR_DATA   * descriptor_list;
extern      OBJ_DATA      * object_list;

extern      MPROG_CODE    * mprog_list;

extern      char            bug_buf     [];
extern      time_t          current_time;
extern      bool            fLogAll;
extern      FILE *          fpReserve;
extern      KILL_DATA       kill_table  [];
extern      char            log_buf     [];
extern      TIME_INFO_DATA      time_info;
extern      WEATHER_DATA        weather_info;
extern      bool            MOBtrigger;
extern  SORT_TABLE  *   bounty_table;
extern      DISABLED_DATA     *   disabled_first; /* interp.c */

extern      BOARD_DATA boards[MAX_BOARD];
extern      PENALTY_DATA   * penalty_list;
extern      CRIME_DATA     * crime_list; /* crimes/offense categories */
extern      int             jail_room_list[MAX_JAIL_ROOM];
extern      RESERVED_DATA  * first_reserved;
extern      RESERVED_DATA  * last_reserved;
extern      AUTH_LIST      * first_auth_name;
extern      AUTH_LIST      * last_auth_name;
extern      bool           wait_for_auth;
extern      CLANWAR_DATA   *  clanwar_table;
extern      const   int     weapon_base_damage[];
extern      bool           global_immediate_flush;
extern      bool           helper_visible;
extern      bool           ignore_invisible;
extern      PORTAL_DATA    *portal_list;
extern      char last_command [MSL];
extern      char last_mprog [MSL];
extern      char last_debug [MSL];

/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.
 */
#if defined(_AIX)
char *  crypt       args( ( const char *key, const char *salt ) );
#endif

#if defined(apollo)
int atoi        args( ( const char *string ) );
void *  calloc      args( ( unsigned nelem, size_t size ) );
char *  crypt       args( ( const char *key, const char *salt ) );
#endif

#if defined(hpux)
char *  crypt       args( ( const char *key, const char *salt ) );
#endif

#if defined(linux)
char *  crypt       args( ( const char *key, const char *salt ) );
#endif

#if defined(macintosh)
#define NOCRYPT
#if defined(unix)
#undef  unix
#endif
#endif

#if defined(MIPS_OS)
char *  crypt       args( ( const char *key, const char *salt ) );
#endif

#if defined(MSDOS)
#define NOCRYPT
#if defined(unix)
#undef  unix
#endif
#endif

#if defined(NeXT)
char *  crypt       args( ( const char *key, const char *salt ) );
#endif

#if defined(sequent)
char *  crypt       args( ( const char *key, const char *salt ) );
int fclose      args( ( FILE *stream ) );
int fprintf     args( ( FILE *stream, const char *format, ... ) );
int fread       args( ( void *ptr, int size, int n, FILE *stream ) );
int fseek       args( ( FILE *stream, long offset, int ptrname ) );
void    perror      args( ( const char *s ) );
int ungetc      args( ( int c, FILE *stream ) );
#endif

#if defined(sun)
char *  crypt       args( ( const char *key, const char *salt ) );
int fclose      args( ( FILE *stream ) );
int fprintf     args( ( FILE *stream, const char *format, ... ) );
#if defined(SYSV)
siz_t   fread       args( ( void *ptr, size_t size, size_t n, 
				FILE *stream) );
#else
int fread       args( ( void *ptr, int size, int n, FILE *stream ) );
#endif
int fseek       args( ( FILE *stream, long offset, int ptrname ) );
void    perror      args( ( const char *s ) );
int ungetc      args( ( int c, FILE *stream ) );
#endif

#if defined(ultrix)
char *  crypt       args( ( const char *key, const char *salt ) );
#endif



/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#if defined(NOCRYPT)
#define crypt(s1, s2)   (s1)
#endif



/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#if defined(macintosh)
#define PLAYER_DIR  ""          /* Player files */
#define TEMP_FILE   "romtmp"
#define NULL_FILE   "proto.are"     /* To reserve one stream */
#endif

#if defined(MSDOS) || defined(WIN32)
#define PLAYER_DIR      "../player/"                    /* Player files */
#define TEMP_FILE       "../player/romtmp"
#define NULL_FILE       "nul"                   /* To reserve one stream */
#define NOTE_DIR        "../notes/"
#define GOD_DIR         "../gods/"
#define CLAN_DIR	"../clans/"
#define LUA_DIR     "../src/lua/"
#endif

#if defined(unix)
#define PLAYER_DIR      "../player/"            /* Player files */
#define GOD_DIR         "../gods/"          /* list of gods */
#define TEMP_FILE   "../player/romtmp"
#define NULL_FILE   "/dev/null"     /* To reserve one stream */
#define NOTE_DIR    "../notes/"
#define CLAN_DIR	"../clans/"
#define LUA_DIR     "../src/lua/"
#endif

#define AREA_LIST       "area.lst"  /* List of areas*/
#define CLAN_LIST       "clan.lst"
#define BUG_FILE        "../log/bugs.txt" /* For 'bug' and bug()*/
#define TYPO_FILE       "../log/typos.txt" /* For 'typo'*/
#define SHUTDOWN_FILE   "shutdown.txt"/* For 'shutdown'*/
#define BAN_FILE    "ban.txt"
#define DISABLED_FILE   "disabled.txt"  /* disabled commands */
#define CLANWAR_FILE   "clanwar.txt"
#define REMORT_FILE    "remort.txt"
#define SKILL_FILE		"skill.txt"
#define STAT_FILE    "stat_count.txt"
#define PENALTY_LOG_FILE   "../log/penlog.txt"
#define PENALTY_FILE   "penalty.txt"
#define CRIME_FILE     "crimes.txt"
#define RESERVED_LIST  "reserved.txt"	/* List of reserved names	*/
#define AUTH_FILE      "auth.txt"
#define WIZ_FILE       "wizlist.txt" 
#define PLAYER_TEMP_DIR "../player/temp/"  /* for simultanious saves */
#define PORTAL_FILE    "portal.txt"
#define RELIGION_FILE  "religion.txt"
#define LBOARD_FILE    "lboard.txt"
#define LBOARD_RESULT_FILE "lboard_result.txt"
#define CHEAT_LIST     "../log/cheatlog.txt"
#define BOX_DIR	       "../box/"
#define BOX_TEMP_DIR   "../box/temp/"
#define MAX_WHO_FILE   "maxwho.txt"
#define LUA_MUD_STARTUP   LUA_DIR "startup_mud.lua"  /* script initialization - mud */
#define LUA_STARTUP    LUA_DIR "startup.lua"
#define ptc printf_to_char

/* string constants */
//#define PROMPT_DEFAULT "{g<{r%h{g/%Hhp {b%m{g/%Mmn {c%v{g/%Vmv {y%X{getl{W%z{x>{x "
#define PROMPT_DEFAULT "{x<{r%h{x/%Hhp {B%m{x/%Mmn {c%v{x/%Vmv {y%Xetl {D%z{x> "

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD  CHAR_DATA
#define MID MOB_INDEX_DATA
#define OD  OBJ_DATA
#define OID OBJ_INDEX_DATA
#define RID ROOM_INDEX_DATA
#define SF  SPEC_FUN
#define AD  AFFECT_DATA
#define MPC MPROG_CODE

/* act_comm.c */
void    check_sex   args( ( CHAR_DATA *ch) );
void    add_follower    args( ( CHAR_DATA *ch, CHAR_DATA *master ) );
void    stop_follower   args( ( CHAR_DATA *ch ) );
void    nuke_pets   args( ( CHAR_DATA *ch ) );
void    die_follower    args( ( CHAR_DATA *ch ) );
bool    is_same_group   args( ( CHAR_DATA *ach, CHAR_DATA *bch ) );
void    info_message  args( ( CHAR_DATA *ch, char *argument, bool show_to_char) );
char    *makedrunk      args( (char *string ,CHAR_DATA *ch) );
void    printf_to_char args( ( CHAR_DATA *ch, char *fmt, ...) );
void    logpf args( (char * fmt, ...) );
void    bugf args( (char * fmt, ...) );
void    mail_notify   args( ( CHAR_DATA *ch, NOTE_DATA *pnote, BOARD_DATA *board ) );
void    printf_to_wiznet args((CHAR_DATA *ch, OBJ_DATA *obj, long flag, long flag_skip, int min_level, char *fmt, ...));

/* act_enter.c */
RID  *get_random_room   args ( (CHAR_DATA *ch) );
RID  *get_random_warfare_room args ( (CHAR_DATA *ch) );

/* act_info.c */
void    set_title   args( ( CHAR_DATA *ch, char *title ) );
char    get_pkflag  args( ( CHAR_DATA *ch, CHAR_DATA *wch ) );


/* act_move.c */
int    move_char   args( ( CHAR_DATA *ch, int door, bool follow ) );
void check_explore args( ( CHAR_DATA *, ROOM_INDEX_DATA * ) );
void explore_vnum args( (CHAR_DATA *, int ) );
bool explored_vnum args( (CHAR_DATA *, int ) );



/* act_obj.c */
bool can_loot       args( (CHAR_DATA *ch, OBJ_DATA *obj, bool allow_group) );
void    wear_obj    args( (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace) );
void    get_obj         args( ( CHAR_DATA *ch, OBJ_DATA *obj,
							OBJ_DATA *container ) );
bool in_donation_room args((OBJ_DATA *obj));

/* act_wiz.c */
void wiznet     args( (char *string, CHAR_DATA *ch, OBJ_DATA *obj,
				   long flag, long flag_skip, int min_level ) );
void copyover_recover args((void));

/* alias.c */
void    substitute_alias args( (DESCRIPTOR_DATA *d, char *input) );


/* auth.c */
void load_auth_list               args( ( void ) );
void save_auth_list               args( ( void ) );
int  get_auth_state               args( ( CHAR_DATA *ch ) );
void add_to_auth                  args( ( CHAR_DATA *ch ) );
void remove_from_auth             args( ( char *name ) );
void check_auth_state             args( ( CHAR_DATA *ch ) );
AUTH_LIST *get_auth_name          args( ( char *name ) ) ;
void auth_update			      args( ( void ) );

/* ban.c */
bool    check_ban   args( ( char *site, int type) );

/* board.c */
void make_note args((const char* board_name, const char *sender,
   const char *to, const char *subject, const int expire_days, const char *text));
void finish_note args((BOARD_DATA *board, NOTE_DATA *note));
bool is_note_to args((CHAR_DATA *ch, NOTE_DATA *note));
void personal_message args((const char *sender, const char *to,
   const char *subject, const int expire_days, const char *text));
void load_boards args((void));
void save_notes args((void));
void free_note args((NOTE_DATA *note));
int board_lookup args((const char *name));

/* clan.c */
void check_clan_eq  args( ( CHAR_DATA *ch ) );
void clan_update    args( ( void ) );
void save_clan_file args( ( int clannum ) );
void save_all_clans args( ( void ) );
bool rank_available args( ( int clan, int current_rank, int new_rank ) );

/* clanwar.c */
CLANWAR_DATA * clanwar_lookup args( (sh_int clan_one, sh_int clan_two) );
void save_clanwars args( ( void ) );

/* comm.c */
void    show_string args( ( struct descriptor_data *d, char *input) );
void    close_socket    args( ( DESCRIPTOR_DATA *dclose ) );
void    write_to_buffer args( ( DESCRIPTOR_DATA *d, const char *txt,
				int length ) );
void    send_to_char    args( ( const char *txt, CHAR_DATA *ch ) );
void    page_to_char    args( ( const char *txt, CHAR_DATA *ch ) );
void    act     args( ( const char *format, CHAR_DATA *ch,
				const void *arg1, const void *arg2, int type ) );
void    act_new     args( ( const char *format, CHAR_DATA *ch, 
				const void *arg1, const void *arg2, int type,
				int min_pos) );
void    act_gag     args( ( const char *format, CHAR_DATA *ch, 
				const void *arg1, const void *arg2, int type,
				long gag_type) );
void    act_new_gag args( ( const char *format, CHAR_DATA *ch, 
				const void *arg1, const void *arg2, int type,
				int min_pos, long gag_type, bool see_only) );


/*
 * Colour stuff by Lope of Loping Through The MUD
 */
int    colour      args( ( char type, CHAR_DATA *ch, char *string ) );
void   colourconv  args( ( char *buffer, const char *txt, CHAR_DATA *ch ) );
void   send_to_char_bw args( ( const char *txt, CHAR_DATA *ch ) );
void   page_to_char_bw args( ( const char *txt, CHAR_DATA *ch ) );

/* db.c */
void    reset_area  args( ( AREA_DATA * pArea ) );        /* OLC */
void    purge_area  args( ( AREA_DATA * pArea ) );
void    reset_room  args( ( ROOM_INDEX_DATA *pRoom ) );  /* OLC */
char *  print_flags args( ( int flag ));
void    boot_db     args( ( void ) );
void    area_update args( ( bool all ) );
CD *    create_mobile   args( ( MOB_INDEX_DATA *pMobIndex ) );
void    clone_mobile    args( ( CHAR_DATA *parent, CHAR_DATA *clone) );
OD *    create_object   args( ( OBJ_INDEX_DATA *pObjIndex, int level ) );
void    clone_object    args( ( OBJ_DATA *parent, OBJ_DATA *clone ) );
void    clear_char  args( ( CHAR_DATA *ch ) );
char *  get_extra_descr args( ( const char *name, EXTRA_DESCR_DATA *ed ) );
MID *   get_mob_index   args( ( int vnum ) );
OID *   get_obj_index   args( ( int vnum ) );
RID *   get_room_index  args( ( int vnum ) );
MPC *   get_mprog_index args( ( int vnum ) );
char    fread_letter    args( ( FILE *fp ) );
int fread_number    args( ( FILE *fp ) );
long    fread_flag  args( ( FILE *fp ) );
char *  fread_string    args( ( FILE *fp ) );
char *  fread_string_eol args(( FILE *fp ) );
void    fread_to_eol    args( ( FILE *fp ) );
char *  fread_word  args( ( FILE *fp ) );
long    flag_convert    args( ( char letter) );
void *  alloc_mem   args( ( int sMem ) );
void *  alloc_perm  args( ( int sMem ) );
void    free_mem    args( ( void *pMem, int sMem ) );
char *  str_dup     args( ( const char *str ) );
void    free_string args( ( char *pstr ) );
int number_fuzzy    args( ( int number ) );
int number_range    args( ( int from, int to ) );
int number_percent  args( ( void ) );
int number_door args( ( void ) );
int number_bits args( ( int width ) );
long     number_mm       args( ( void ) );
int dice        args( ( int number, int size ) );
int interpolate args( ( int level, int value_00, int value_32 ) );
void    smash_tilde args( ( char *str ) );
bool    str_cmp     args( ( const char *astr, const char *bstr ) );
bool    str_prefix  args( ( const char *astr, const char *bstr ) );
bool    str_infix   args( ( const char *astr, const char *bstr ) );
bool    str_suffix  args( ( const char *astr, const char *bstr ) );
char *  capitalize  args( ( const char *str ) );
void    append_file args( ( CHAR_DATA *ch, char *file, char *str ) );
void    bug     args( ( const char *str, int param ) );
void    log_string  args( ( const char *str ) );
void    log_trace();
void    tail_chain  args( ( void ) );
char *	bin_info_string();

/* effect.c */
void    acid_effect args( (void *vo, int level, int dam, int target) );
void    cold_effect args( (void *vo, int level, int dam, int target) );
void    fire_effect args( (void *vo, int level, int dam, int target) );
void    poison_effect   args( (void *vo, int level, int dam, int target) );
void    shock_effect    args( (void *vo, int level, int dam, int target) );
void    dumb_effect     args( (void *vo, int level, int dam, int target));
void    paralysis_effect args( (void *vo,int level, int dam, int target));

/* enchant.c */
int get_enchant_ops( OBJ_DATA *obj, int level );

/* fight.c */
bool    is_safe     args( (CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    is_safe_spell   args( (CHAR_DATA *ch, CHAR_DATA *victim, bool area ) );
void    violence_update args( ( void ) );
void    multi_hit   args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
bool    damage      args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
					int dt, int class, bool show ) );
/*bool    damage_old      args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
								int dt, int class, bool show ) );*/
void    update_pos  args( ( CHAR_DATA *victim ) );
void    stop_fighting   args( ( CHAR_DATA *ch, bool fBoth ) );
void    check_killer    args( ( CHAR_DATA *ch, CHAR_DATA *victim) );


/* ftp.c */
bool    ftp_push    args( (DESCRIPTOR_DATA *d) );

/* grant.c */
bool is_granted_name    args( ( CHAR_DATA *ch, char *argument ) );
bool is_granted      args( ( CHAR_DATA *ch, DO_FUN *do_fun ) );


/* handler.c */
AD      *affect_find args( (AFFECT_DATA *paf, int sn));
void    affect_check    args( (CHAR_DATA *ch, int where, int vector) );
int count_users args( (OBJ_DATA *obj) );
void    deduct_cost args( (CHAR_DATA *ch, int cost) );
int     check_immune    args( (CHAR_DATA *ch, int dam_type) );
int     material_lookup args( ( const char *name) );
int weapon_lookup   args( ( const char *name) );
int weapon_type args( ( const char *name) );
char    *weapon_name    args( ( int weapon_Type) );
char    *item_name  args( ( int item_type) ); 
int attack_lookup   args( ( const char *name) );
long    wiznet_lookup   args( ( const char *name) );
int class_lookup    args( ( const char *name) );
bool    is_clan     args( (CHAR_DATA *ch) );
bool    is_same_clan    args( (CHAR_DATA *ch, CHAR_DATA *victim));
int     get_weapon_sn   args( ( CHAR_DATA *ch ) );
int     get_weapon_sn_new args( (CHAR_DATA *ch, bool secondary) );
int     get_age         args( ( CHAR_DATA *ch ) );
void    reset_char  args( ( CHAR_DATA *ch )  );
int get_trust   args( ( CHAR_DATA *ch ) );
int can_carry_n args( ( CHAR_DATA *ch ) );
int can_carry_w args( ( CHAR_DATA *ch ) );
bool    is_name     args( ( char *str, char *namelist ) );
bool    is_exact_name   args( ( char *str, char *namelist ) );
void    affect_to_char  args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void    affect_to_obj   args( ( OBJ_DATA *obj, AFFECT_DATA *paf ) );
void    affect_remove   args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void    affect_remove_obj args( (OBJ_DATA *obj, AFFECT_DATA *paf ) );
void    affect_strip    args( ( CHAR_DATA *ch, int sn ) );
bool    is_affected args( ( CHAR_DATA *ch, int sn ) );
void    affect_join args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void    char_from_room  args( ( CHAR_DATA *ch ) );
void    char_to_room    args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex ) );
void    obj_to_char args( ( OBJ_DATA *obj, CHAR_DATA *ch ) );
void    obj_from_char   args( ( OBJ_DATA *obj ) );
int apply_ac    args( ( OBJ_DATA *obj, int iWear, int type ) );
OD *    get_eq_char args( ( CHAR_DATA *ch, int iWear ) );
void    equip_char  args( ( CHAR_DATA *ch, OBJ_DATA *obj, int iWear ) );
void    unequip_char    args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
int count_obj_list  args( ( OBJ_INDEX_DATA *obj, OBJ_DATA *list ) );
void    obj_from_room   args( ( OBJ_DATA *obj ) );
void    obj_to_room args( ( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex ) );
void    obj_to_obj  args( ( OBJ_DATA *obj, OBJ_DATA *obj_to ) );
void    obj_from_obj    args( ( OBJ_DATA *obj ) );
void    extract_obj args( ( OBJ_DATA *obj ) );
void    extract_char    args( ( CHAR_DATA *ch, bool fPull ) );
void    extract_char_new args( ( CHAR_DATA *ch, bool fPull, bool extract_objects ) );
CHAR_DATA* get_player( char *name );
CD *    get_char_room   args( ( CHAR_DATA *ch, char *argument ) );
CD *    get_char_world  args( ( CHAR_DATA *ch, char *argument ) );
CD *    get_char_area  args( ( CHAR_DATA *ch, char *argument ) );   
OD *    get_obj_type    args( ( OBJ_INDEX_DATA *pObjIndexData ) );
OD *    get_obj_list    args( ( CHAR_DATA *ch, char *argument,
				OBJ_DATA *list ) );
OD *    get_obj_carry   args( ( CHAR_DATA *ch, char *argument, 
				CHAR_DATA *viewer ) );
OD *    get_obj_wear    args( ( CHAR_DATA *ch, char *argument ) );
OD *    get_obj_here    args( ( CHAR_DATA *ch, char *argument ) );
OD *    get_obj_world   args( ( CHAR_DATA *ch, char *argument ) );
OD *    get_obj_area    args( ( CHAR_DATA *ch, char *argument ) );
RID *   get_obj_room    args( ( OBJ_DATA *obj ) );
RID *   find_location   args( ( CHAR_DATA *ch, char *arg ) );
OD *    create_money    args( ( int gold, int silver ) );
int get_obj_number  args( ( OBJ_DATA *obj ) );
int get_obj_weight  args( ( OBJ_DATA *obj ) );
int get_true_weight args( ( OBJ_DATA *obj ) );
bool    room_is_dark    args( ( ROOM_INDEX_DATA *pRoomIndex ) );
bool    is_room_owner   args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *room) );
bool    room_is_private args( ( ROOM_INDEX_DATA *pRoomIndex ) );
bool    can_see     args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    check_see   args( (CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    can_see_obj args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
bool    can_see_room    args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex) );
bool    can_drop_obj    args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
char *  affect_loc_name args( ( int location ) );
char *  affect_bit_name args( ( int vector ) );
char *  extra_bit_name  args( ( int extra_flags ) );
char *  wear_bit_name   args( ( int wear_flags ) );
char *  act_bit_name    args( ( int act_flags ) );
char *  off_bit_name    args( ( int off_flags ) );
char *  imm_bit_name    args( ( int imm_flags ) );
char *  form_bit_name   args( ( int form_flags ) );
char *  part_bit_name   args( ( int part_flags ) );
char *  weapon_bit_name args( ( int weapon_flags ) );
char *  comm_bit_name   args( ( int comm_flags ) );
char *  togg_bit_name	args( ( int togg_flags ) );
char *  penalty_bit_name args( (int penalty_flags) );
char *  cont_bit_name   args( ( int cont_flags) );

/*
 * Colour Config
 */ 
void  default_colour  args( ( CHAR_DATA *ch ) );
void  all_colour      args( ( CHAR_DATA *ch, char *argument ) );

/* interp.c */
void    interpret   args( ( CHAR_DATA *ch, char *argument ) );
bool    is_number   args( ( char *arg ) );
int number_argument args( ( char *argument, char *arg ) );
int mult_argument   args( ( char *argument, char *arg) );
char *  one_argument    args( ( char *argument, char *arg_first ) );
char *  one_argument_keep_case( char *argument, char *arg_first );
void   load_disabled   args( ( void ) );
void   save_disabled   args( ( void ) );


/* magic.c */
int find_spell  args( ( CHAR_DATA *ch, const char *name) );
int     mana_cost   (CHAR_DATA *ch, int sn, int skill);
int skill_lookup    args( ( const char *name ) );
int slot_lookup args( ( int slot ) );
bool    saves_spell args( ( int level, CHAR_DATA *victim, int dam_type ) );
bool obj_cast_spell( int sn, int level, CHAR_DATA *ch, OBJ_DATA *obj, char *arg );

/* mob_prog.c */
void    program_flow    args( ( char *text, bool is_lua, int vnum, char *source, CHAR_DATA *mob, CHAR_DATA *ch,
				const void *arg1, sh_int arg1type,
                const void *arg2, sh_int arg2type) );
bool    mp_act_trigger  args( ( char *argument, CHAR_DATA *mob, CHAR_DATA *ch,
				const void *arg1, sh_int arg1type, 
                const void *arg2, sh_int arg2type,
                int type ) );
bool    mp_percent_trigger args( ( CHAR_DATA *mob, CHAR_DATA *ch,               
				const void *arg1, sh_int arg1type,
                const void *arg2, sh_int arg2type,
                int type ) );
void    mp_bribe_trigger  args( ( CHAR_DATA *mob, CHAR_DATA *ch, int amount ) );
bool    mp_exit_trigger   args( ( CHAR_DATA *ch, int dir ) );
void    mp_give_trigger   args( ( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj ) );
void    mp_greet_trigger  args( ( CHAR_DATA *ch ) );
void    mp_hprct_trigger  args( ( CHAR_DATA *mob, CHAR_DATA *ch ) );
void    mp_mprct_trigger  args( ( CHAR_DATA *mob, CHAR_DATA *ch ) );
bool    mp_try_trigger    args( ( char *argument, CHAR_DATA *mob ) );
bool    mp_spell_trigger  args( ( char *argument, CHAR_DATA *mob, CHAR_DATA *ch ) );

/* mob_cmds.c */
void    mob_interpret   args( ( CHAR_DATA *ch, char *argument ) );

/* olc.c */
bool    run_olc_editor    args( ( DESCRIPTOR_DATA *d ) );
char    *olc_ed_name      args( ( CHAR_DATA *ch ) );
char    *olc_ed_vnum      args( ( CHAR_DATA *ch ) );

/* penalty.c */
void save_penalties args( ( void ) );
int  show_penalties_by_player(CHAR_DATA *ch, char *victim_name, int victim_played, int format);
void delete_penalty_node args((PENALTY_DATA *node));
void penalty_update args((CHAR_DATA *ch));
void   load_crime_list args( ( void ) );
void   save_crime_list args( ( void ) );

/* lookup.c */
int race_lookup args( ( const char *name) );
int item_lookup args( ( const char *name) );
int liq_lookup  args( ( const char *name) );

/* nanny.c */
int con_state args( ( DESCRIPTOR_DATA *d ) );
int creation_mode args( ( DESCRIPTOR_DATA *d ) );
void set_con_state args((DESCRIPTOR_DATA *d, int cstate));
void set_creation_state args((DESCRIPTOR_DATA *d, int cmode));

/* remort.c */
void remort_complete args( (CHAR_DATA *ch) );
void remort_update args( ( void) );
void remort_load args( ( void) );
void remort_remove args( (CHAR_DATA *ch, bool success) );
void remort_begin args( (CHAR_DATA *ch) );

/* save.c */
void    save_char_obj   args( ( CHAR_DATA *ch ) );
bool    load_char_obj   args( ( DESCRIPTOR_DATA *d, char *name ) );

/* skills.c */
bool    parse_gen_groups args( ( CHAR_DATA *ch,char *argument ) );
void    list_group_costs args( ( CHAR_DATA *ch ) );
void    list_group_known args( ( CHAR_DATA *ch ) );
int     exp_per_level   args( ( CHAR_DATA *ch, int points ) );
void    check_improve   args( ( CHAR_DATA *ch, int sn, bool success, 
					int multiplier ) );
int     group_lookup    args( (const char *name) );
void    gn_add      args( ( CHAR_DATA *ch, int gn) );
void    gn_remove   args( ( CHAR_DATA *ch, int gn) );
void    group_add   args( ( CHAR_DATA *ch, const char *name, bool deduct) );
void    group_remove    args( ( CHAR_DATA *ch, const char *name) );
int get_skill   args( ( CHAR_DATA *ch, int sn ) );
int get_weapon_skill args(( CHAR_DATA *ch, int sn ) );

/* social-edit.c */
void load_social_table();
void save_social_table();

/* special.c */
SF *    spec_lookup args( ( const char *name ) );
char *  spec_name   args( ( SPEC_FUN *function ) );

/* stats.c */
int get_curr_stat   args( ( CHAR_DATA *ch, int stat ) );
int     get_max_train   args( ( CHAR_DATA *ch, int stat ) );
int class_bonus     args( ( int class, int stat ) );
int ch_dex_tohit        args( (CHAR_DATA *ch) );
int ch_str_todam        args( (CHAR_DATA *ch) );
int ch_str_carry        args( (CHAR_DATA *ch) );
int ch_str_wield        args( (CHAR_DATA *ch) );
int ch_int_learn        args( (CHAR_DATA *ch) );
int ch_dis_practice args( (CHAR_DATA *ch) );
int ch_prac_gains       args( (CHAR_DATA *ch, int for_level) );
int ch_agi_defensive    args( (CHAR_DATA *ch) );
int ch_dex_extrahit args( (CHAR_DATA *ch) );
int ch_con_shock        args( (CHAR_DATA *ch) );
int ch_con_hitp     args( (CHAR_DATA *ch) );
int ch_cha_aggro        args( (CHAR_DATA *ch) );
int ch_int_field        args((CHAR_DATA *ch));
int ch_wis_field        args((CHAR_DATA *ch));
int ch_dis_field        args((CHAR_DATA *ch));
int ch_luc_quest        args((CHAR_DATA *ch));
void compute_mob_stats  args( (CHAR_DATA *mob) );
int stat_gain           args( (CHAR_DATA *ch, int stat) );
void update_perm_hp_mana_move args( (CHAR_DATA *ch ) );

/* string.c */
void   string_edit    args( ( CHAR_DATA *ch, char **pString ) );
void   string_append  args( ( CHAR_DATA *ch, char **pString ) );
char * string_replace args( ( char * orig, char * old, char * new ) );
char * string_replace_ext args( ( char * orig, char * old, char * new, char *xbuf, int xbuf_length) );
void   string_add     args( ( CHAR_DATA *ch, char *argument ) );
char * format_string  args( ( char *oldstring /*, bool fSpace */ ) );
char * first_arg      args( ( char *argument, char *arg_first, bool fCase ) );
char * string_unpad   args( ( char * argument ) );
char * string_proper  args( ( char * argument ) );
char * del_last_line  args( ( char * string) );
char * del_last_line_ext args( ( char *string, char *xbuf ) );
char * force_wrap     args( ( char *oldstring ) );
int    strlen_color   args( ( char * argument ) );
char * center         args( ( char *argument, int width, char fill ) );
char * lpad           args( ( char *argument, int width, char fill ) );
char * rpad           args( ( char *argument, int width, char fill ) );

/* teleport.c */
RID *   room_by_name    args( ( char *target, int level, bool error) );

/* hunt.c */
void    hunt_victim     args( ( CHAR_DATA *ch ) );
void	set_hunting		args((CHAR_DATA *ch, CHAR_DATA *victim));
void	stop_hunting	args((CHAR_DATA *ch));
void	remember_attack	args((CHAR_DATA *ch, CHAR_DATA *victim, int dam));
void	update_memory	args((CHAR_DATA *ch));
void	forget_attacks	args((CHAR_DATA *ch));
int		check_anger		args((CHAR_DATA *ch, CHAR_DATA *victim));

/* quest.c */

bool chance(int num);
void do_quest args((CHAR_DATA *ch, char *argument)); 
void quest_update   args(( void ));   

/* update.c */
void    war_update      args( ( void ) ); 
void    advance_level   args( ( CHAR_DATA *ch, bool hide ) );
void    gain_exp    args( ( CHAR_DATA *ch, int gain ) );
void    gain_condition  args( ( CHAR_DATA *ch, int iCond, int value ) );
void    update_handler  args( ( void ) );
void    explode  args( ( OBJ_DATA *obj ) );
void      update_bounty args( ( CHAR_DATA *ch ) );
void      remove_bounty args( ( CHAR_DATA *ch ) );
void    change_align    args( (CHAR_DATA *ch, int change_by) ); 

/* wizlist.c */
void    update_wizlist  args( ( CHAR_DATA *ch, int level ) );     


#undef  CD
#undef  MID
#undef  OD
#undef  OID
#undef  RID
#undef  SF
#undef AD

/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */


/*
 * Area flags.
 */
#define         AREA_CHANGED    (A)   /* Area has been modified. */
/* #define         AREA_ADDED     (B) */  /* Area has been added to. */
/* #define         AREA_LOADING   (C) */  /* Used for counting in db.c */
#define		AREA_REMORT	(D)
#define		AREA_CLONE      (E)
#define		AREA_NOQUEST    (F)
#define         AREA_NOREPOP    (G)
#define         AREA_NOHIDE     (H)

#define MAX_DIR  10
#define NO_FLAG -99 /* Must not be used in flags or stats. */

/*
 * Global Constants
 */
extern  char *  const   dir_name        [];
extern  const   sh_int  rev_dir         [];          /* sh_int - ROM OLC */
extern  const   struct  spec_type   spec_table  [];

/*
 * Global variables
 */

extern      AREA_DATA *     area_first;
extern      AREA_DATA *     area_last;
extern      SHOP_DATA *     shop_last;

extern      int         top_affect;
extern      int         top_area;
extern      int         top_ed;
extern      int         top_exit;
extern      int         top_help;
extern      int         top_mob_index;
extern      int         top_obj_index;
extern      int         top_reset;
extern      int         top_room;
extern      int         top_shop;

extern      int         top_jail_room;

extern      int         top_vnum_mob;
extern      int         top_vnum_obj;
extern      int         top_vnum_room;
extern      int         lua_mprogs;
extern      char            str_empty       [1];

extern      MOB_INDEX_DATA *    mob_index_hash  [MAX_KEY_HASH];
extern      OBJ_INDEX_DATA *    obj_index_hash  [MAX_KEY_HASH];
extern      ROOM_INDEX_DATA *   room_index_hash [MAX_KEY_HASH];


/*
    * Lua stuff (Nick Gammon)
     */

 void open_lua  (CHAR_DATA * ch);  /* set up Lua state */
  void close_lua (CHAR_DATA * ch);  /* close down Lua state, if it exists */

#define ACT_ARG_UNDEFINED 0
#define ACT_ARG_OBJ 1
#define ACT_ARG_TEXT 2
#define ACT_ARG_CHARACTER 3

