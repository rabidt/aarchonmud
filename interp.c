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
#include "merc.h"
#include "interp.h"
#include "songs.h"

bool    check_disabled (const struct cmd_type *command);
DISABLED_DATA *disabled_first;

#define END_MARKER   "END" /* for load_disabled() and save_disabled() */

char last_command[MSL] = ""; /* Global variable to hold the last input line */

/*
 * Command logging types.
 */
#define LOG_NORMAL  0
#define LOG_ALWAYS  1
#define LOG_NEVER   2

// for commands that anyone can use on testport
#ifdef TESTER
#define LT 0
#else
#define LT L8
#endif

/*
 * Log-all switch.
 */
bool                fLogAll     = FALSE;

int nAllocString;
int nAllocPerm;


/*
 * Command table.
 */
const   struct  cmd_type    cmd_table   [] =
{
   /*
    * Common movement commands.
    */
    { "north",      do_north,   POS_STANDING,    0,  LOG_NEVER, 0, FALSE, TRUE},
    { "east",       do_east,    POS_STANDING,    0,  LOG_NEVER, 0, FALSE, TRUE},
    { "south",      do_south,   POS_STANDING,    0,  LOG_NEVER, 0, FALSE, TRUE},
    { "west",       do_west,    POS_STANDING,    0,  LOG_NEVER, 0, FALSE, TRUE},
    { "up",         do_up,      POS_STANDING,    0,  LOG_NEVER, 0, FALSE, TRUE},
    { "down",       do_down,    POS_STANDING,    0,  LOG_NEVER, 0, FALSE, TRUE},
    { "northeast",  do_northeast,   POS_STANDING,  0,  LOG_NEVER, 0, FALSE, TRUE  },
    { "northwest",  do_northwest,   POS_STANDING,  0,  LOG_NEVER, 0, FALSE, TRUE  },
    { "southeast",  do_southeast,   POS_STANDING,  0,  LOG_NEVER, 0, FALSE, TRUE  },
    { "southwest",  do_southwest,   POS_STANDING,  0,  LOG_NEVER, 0, FALSE, TRUE  },
    { "ne",         do_northeast,   POS_STANDING,  0,  LOG_NEVER, 0, FALSE, TRUE  },
    { "nw",         do_northwest,   POS_STANDING,  0,  LOG_NEVER, 0, FALSE, TRUE  },
    
{ "se",         do_southeast,   POS_STANDING,  0,  LOG_NEVER, 0, FALSE, TRUE  },
    { "sw",         do_southwest,   POS_STANDING,  0,  LOG_NEVER, 0, FALSE, TRUE  },
    
   /*
    * Common other commands.
    * Placed here so one and two letter abbreviations work.
    */
    
	/*command      function name    position     level  log level  show olc charm */
    { "as",         do_as,          POS_DEAD,       L2,  LOG_ALWAYS, 1, FALSE, FALSE },
    { "at",         do_at,          POS_DEAD,       L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "attributes", do_attributes,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "cast",       do_cast,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "auction",    do_auction,     POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "buy",        do_buy,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "balance",    do_balance,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "deposit",    do_deposit,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "withdraw",   do_withdraw,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "channel",    do_channel,     POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE   },
    { "channels",   do_channels,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "exits",      do_exits,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "explored",   do_explored,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "get",        do_get,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "goto",       do_goto,        POS_DEAD,       IM,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "group",      do_group,       POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "hit",        do_kill,    POS_FIGHTING,    0,  LOG_NORMAL, 0 , FALSE, TRUE },
    { "inventory",  do_inventory,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE },
    { "kill",       do_kill,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "look",       do_look,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "glance",     do_glance,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "clan",       do_clantalk,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "cmotd",	    do_cmotd,	POS_SLEEPING, 	0,	LOG_NORMAL,  1, FALSE, TRUE },
    { "[",          do_clantalk, POS_SLEEPING, 0, LOG_NORMAL, 0, FALSE, TRUE  },
    //{ "proclaim",   do_religion_talk, POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "music",      do_music,       POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "order",      do_order,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "practice",   do_practice,    POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "hpractice",  do_hpractice,   POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "rest",       do_rest,    POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "sit",        do_sit,     POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "stand",      do_stand,   POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "stat",       do_stat,    POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "tell",       do_tell,    POS_DEAD,            0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "unlock",     do_unlock,      POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "wield",      do_wear,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "wizhelp",    do_wizhelp, POS_DEAD,   IM,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "ptitle",     do_ptitle,  POS_DEAD,   0,   LOG_NORMAL, 1, FALSE, FALSE },
    
   /*
    * Informational commands.
    */
    { "affects",    do_affects, POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "areas",      do_areas,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "bug",        do_bug,     POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "classes",    do_classes, POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "commands",   do_commands,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "compare",    do_compare, POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "consider",   do_consider,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "count",      do_count,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "credits",    do_credits, POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "dirs",       do_dirs,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "equipment",  do_equipment,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "eqhelp",     do_eqhelp,  POS_DEAD,   0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "etls",       do_etls,    POS_DEAD,   0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "examine",    do_examine, POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "help",       do_help,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "motd",       do_motd,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "read",       do_read,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "report",     do_report,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "rules",      do_rules,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "score",      do_score,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "lboard",     do_lboard,  POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "leadership", do_leadership,  POS_RESTING, 0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "lhistory",   do_lhistory,POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "percentages", do_percentages, POS_DEAD, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "breakdown",  do_breakdown, POS_DEAD,  0,  LOG_NORMAL, 1, FALSE, TRUE   },
    { "scan",       do_scan,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "survey",     do_survey,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "skill",      do_skill,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "skills",     do_skills,  POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "social",     do_social,  POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, TRUE   },
    { "socials",    do_socials, POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "show",       do_show,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "spells",     do_spells,  POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "stats",      do_stats,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "story",      do_story,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "time",       do_time,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "typo",       do_typo,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE },
    { "weather",    do_weather, POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "who",        do_who,     POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "whois",      do_whois,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "wizlist",    do_wizlist, POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "worth",      do_worth,   POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
#ifdef FSTAT 
    { "fstat",	    do_fstat,   POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
#endif
#ifdef LAG_FREE
    { "lagfree",    do_lagfree, POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, FALSE},
#endif
//    { "combo",      do_combo,   POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    
   /*
    * Configuration commands.
    */
    { "alia",       do_alia,    POS_DEAD,    0,  LOG_NORMAL, 0, FALSE, FALSE  },
    { "alias",      do_alias,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "action",     do_action,  POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "autolist",   do_autolist,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "autoassist", do_autoassist,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "autoexit",   do_autoexit,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "autogold",   do_autogold,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "autoloot",   do_autoloot,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "autosac",    do_autosac, POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "autosplit",  do_autosplit,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "autorescue", do_autorescue,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "brief",      do_brief,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "color",      do_colour,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "colour",     do_colour,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "combine",    do_combine, POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "compact",    do_compact, POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "description",do_description, POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "delet",      do_delet,   POS_DEAD,        0,  LOG_NEVER, 0, FALSE, FALSE  },
    { "delete",     do_delete,  POS_SLEEPING,    0,  LOG_NEVER, 1, FALSE, FALSE  },
    { "itemlevel",  do_itemlevel ,  POS_DEAD,    0,  LOG_NEVER, 1, FALSE, FALSE  },
    { "nofollow",   do_nofollow,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "noloot",     do_noloot,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "nosummon",   do_nosummon,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "nocancel",   do_nocancel,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "nolocate",   do_nolocate,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "noaccept",   do_noaccept,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "nosurrender",do_nosurrender, POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "outfit",     do_outfit,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "password",   do_password,    POS_DEAD,    0,  LOG_NEVER,  1, FALSE, FALSE  },
    { "prompt",     do_prompt,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "rolldice",   do_rolldice,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE },
    { "scroll",     do_scroll,  POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "title",      do_title,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "unalias",    do_unalias, POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "wimpy",      do_wimpy,   POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, TRUE   },
    { "noexp",      do_noexp,       POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE },
    { "nohelp",     do_nohelp,      POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE },
    
   /*
    * Communication commands.
    */
    { "afk",        do_afk,     POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "answer",     do_answer,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "board",      do_board,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE },
    { "deaf",       do_deaf,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "emote",      do_emote,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "info",       do_info,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "pmote",      do_pmote,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "gossip",     do_gossip,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "\\",         do_gossip,  POS_DEAD,        0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { ",",          do_emote,   POS_RESTING,     0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { "gratz",      do_gratz,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "gtell",      do_gtell,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { ";",          do_gtell,   POS_DEAD,        0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { "quest",      do_quest,   POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "newbie",     do_newbie,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE },
    { "note",       do_note,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "pose",       do_pose,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "question",   do_question,POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "quote",      do_quote,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "quiet",      do_quiet,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "reply",      do_reply,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "replay",     do_replay,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "say",        do_say,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "'",          do_say,     POS_RESTING,     0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { "shout",      do_shout,   POS_RESTING,     3,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "unread",     do_board,   POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "yell",       do_yell,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "noreply",    do_noreply, POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "playback",   do_playback,POS_DEAD,   0,  LOG_NORMAL, 1, FALSE, FALSE},
    
   /*
    * Object manipulation commands.
    */
    { "craft",      do_craft,    POS_STANDING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "brandish",   do_brandish,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "close",      do_close,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "donate",     do_donate,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "drink",      do_drink,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "drop",       do_drop,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "eat",        do_eat,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "envenom",    do_envenom, POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "fill",       do_fill,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "give",       do_give,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "heal",       do_heal,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "hold",       do_wear,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE   },
    { "identify",   do_identify,POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "list",       do_list,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "lock",       do_lock,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "merge",      do_merge,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "open",       do_open,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "paralysispoison",    do_paralysis_poison, POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "pick",       do_pick,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "pour",       do_pour,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "put",        do_put,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "quaff",      do_quaff,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "recite",     do_recite,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "remove",     do_remove,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "sell",       do_sell,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "take",       do_get,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "sacrifice",  do_sacrifice,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "junk",       do_sacrifice,   POS_RESTING,     0,  LOG_NORMAL, 0, FALSE, FALSE  },
    { "value",      do_value,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "browse",     do_browse,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "wear",       do_wear,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "appraise",   do_appraise, POS_RESTING, 0, LOG_NORMAL, 1, FALSE, FALSE  },
    { "lore",       do_lore, POS_RESTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "zap",        do_zap,     POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "tdisarm",    do_disarm_trap, POS_STANDING, 0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "spellup",    do_spellup, POS_RESTING, 0, LOG_NORMAL, 1, FALSE, FALSE },
    { "smith",      do_smith,   POS_STANDING, 0, LOG_NORMAL, 1, FALSE, FALSE },
    
   /*
    * Combat commands.
    */
    { "mindflay",   do_mindflay, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "gaze",       do_gaze, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "gouge",      do_gouge, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "chop",       do_chop, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "bite",       do_bite, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "bitch",      do_bitch,POS_SLEEPING,  0,  LOG_NORMAL, 1, FALSE, TRUE },/* Purposely after bite */
    { "blast",      do_blast, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "guard",      do_guard, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "leg",        do_leg_sweep, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "uppercut",   do_uppercut, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "second",     do_second, POS_RESTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "war",        do_war_cry, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "tumble",     do_tumble, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "distract",   do_distract, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "feint",      do_feint, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "backstab",   do_backstab,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
        { "blackjack",  do_blackjack,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "circle",     do_circle,      POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "slash",      do_slash_throat,POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "bash",       do_bash,        POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "bs",         do_backstab,    POS_FIGHTING,    0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { "berserk",    do_berserk, POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "bomb",       do_bomb,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "dirt",       do_dirt,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "disarm",     do_disarm,  POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "flee",       do_flee,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "kick",       do_kick,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "murde",      do_murde,   POS_FIGHTING,    0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { "murder",     do_murder,  POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "rescue",     do_rescue,  POS_FIGHTING,    0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { "trip",       do_trip,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "hunt",       do_hunt,    POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "scout",      do_scout,   POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "surrender",  do_surrender,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "ignite",     do_ignite,  POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "inspire",    do_inspire,  POS_FIGHTING, 0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "headbutt",   do_headbutt, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "net",        do_net, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE},
    { "mug",        do_mug, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE},
    { "snipe",      do_snipe,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "burst",      do_burst,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "fury",       do_drunken_fury,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "shoot",      do_shoot_lock,    POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "unjam",      do_unjam,   POS_RESTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "aim",        do_aim,     POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "semiauto",   do_semiauto,POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "fullauto",   do_fullauto,POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "hogtie",     do_hogtie,  POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "shieldbash", do_shield_bash,    POS_FIGHTING,    0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "charge",     do_charge,  POS_STANDING,    0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "choke",      do_choke_hold,  POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "roundhouse", do_roundhouse, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "swing",      do_round_swing, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "double",     do_double_strike, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "hurl",       do_hurl,    POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "spit",       do_spit,    POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "fatal",      do_fatal_blow, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "intimidate", do_intimidate, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "dowse",      do_dowsing, POS_STANDING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "rustle",     do_rustle_grub, POS_STANDING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "fledge",     do_fledge,  POS_RESTING,  0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "rake",       do_rake,    POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "puncture",   do_puncture,POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "scribe",     do_scribe,  POS_RESTING,    0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "brew",       do_brew,    POS_RESTING,    0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "herbs",      do_herbs,   POS_DEAD,     0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "strafe",     do_strafe,  POS_FIGHTING,   0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "fervent",    do_fervent_rage,    POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "paroxysm",   do_paroxysm,POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "rupture",    do_rupture, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
//    { "powerthrust",do_power_thrust, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "powerattack",do_power_attack, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "extract",    do_extract, POS_RESTING,    0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "supplies",   do_supplies,POS_DEAD,       0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "quiveringpalm",   do_quivering_palm,  POS_STANDING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "smite",      do_smite,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "engage",     do_engage,  POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "layonhands", do_lay_on_hands, POS_RESTING, 0,  LOG_NORMAL, 1, FALSE, TRUE  },
    
    // Meta-magic commands.
    { "mmcast",     do_mmcast,  POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "ecast",      do_ecast,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "pcast",      do_pcast,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "qcast",      do_qcast,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "ccast",      do_ccast,   POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "permcast",   do_permcast,POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "wish",       do_wish,    POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },

   /*
    * Ranger commands.
    */
    { "forage",     do_forage,  POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "torch",      do_torch,   POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "shelter",    do_shelter, POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "firstaid",   do_firstaid, POS_FIGHTING,   0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "detoxify",   do_detoxify, POS_STANDING,   0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "tame",       do_tame,    POS_STANDING,  0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "camp",       do_camp_fire,    POS_RESTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "raft",       do_build_raft,   POS_RESTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "preserve",   do_taxidermy,    POS_RESTING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "treatment",  do_treat_weapon, POS_RESTING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "fishing",    do_fishing, POS_RESTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "infectiousarrow", do_infectious_arrow, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    
    /* bard commands */

    { "sing",       do_sing, POS_RESTING,  0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "wail",       do_wail, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE  },
    { "riff",       do_riff, POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },

    /* war commands. */
    { "startwar",   do_startwar,POS_DEAD,       L8,  LOG_ALWAYS, 1, FALSE, FALSE},
    { "nowar",      do_nowar,   POS_DEAD,        1,  LOG_NORMAL, 1, FALSE, FALSE },
    { "combat",     do_combat,  POS_STANDING,    1,  LOG_NORMAL, 1, FALSE, FALSE },
    { "warstatus",  do_warstatus,   POS_SLEEPING,    1,  LOG_NORMAL, 1, FALSE, FALSE },
    { "stopwar",    do_stopwar, POS_DEAD,       L8,  LOG_ALWAYS, 1, FALSE, FALSE },
    { "warsit",     do_warsit,  POS_SLEEPING,    1,  LOG_NORMAL, 1, FALSE, FALSE },
    
   /*
    * Mob command interpreter (placed here for faster scan...)
    */
    { "mob",        do_mob,     POS_DEAD,    0,  LOG_NORMAL,  0, FALSE, FALSE  },
    
   /*
    * Miscellaneous commands.
    */
    { "achievements",     do_achievements,  POS_DEAD,   0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "ascend",     do_ascend,  POS_STANDING,    1,  LOG_ALWAYS, 1, FALSE, FALSE },
    { "clanwar",    do_clanwar, POS_DEAD,    1,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "consent",    do_consent, POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE },
    { "marry",      do_marry,   POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "enter",      do_enter,   POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "follow",     do_follow,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "gain",       do_gain,    POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "mastery",    do_master,  POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "gametalk",   do_gametalk,POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, TRUE },
    { "go",         do_enter,   POS_STANDING,    0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { "bounty",     do_bounty,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "groups",     do_groups,  POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "hide",       do_hide,    POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "plant",      do_plant,   POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "pkill",      do_pkill,   POS_SLEEPING,    0,  LOG_NEVER,  1, FALSE, FALSE  },
    { "roleplay",   do_roleplay,POS_SLEEPING,    0,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "qui",        do_qui,     POS_DEAD,    0,  LOG_NORMAL, 0, FALSE, FALSE  },
    { "quit",       do_quit,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "recall",     do_recall,  POS_FIGHTING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "/",          do_recall,  POS_FIGHTING,    0,  LOG_NORMAL, 0, FALSE, TRUE  },
    { "rent",       do_rent,    POS_DEAD,    0,  LOG_NORMAL, 0, FALSE, FALSE  },
    { "save",       do_save,    POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "sleep",      do_sleep,   POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "sneak",      do_sneak,   POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "split",      do_split,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "steal",      do_steal,   POS_STANDING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "train",      do_train,   POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "finger",     do_finger,  POS_DEAD,   0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "bounty",     do_bounty,  POS_RESTING,     0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "visible",    do_visible, POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "wake",       do_wake,    POS_SLEEPING,    0,  LOG_NORMAL, 1, FALSE, TRUE  },
    { "walk",       do_walk,    POS_DEAD,        0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "rank",       do_rank,    POS_DEAD,    1,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "invite",     do_invite,    POS_DEAD, 0,  LOG_NORMAL,  1, FALSE, FALSE },
    { "raceskills", do_raceskills, POS_DEAD,    0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "recruit",    do_recruit, POS_DEAD,    1,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "reject",     do_reject, POS_DEAD,    1,  LOG_ALWAYS, 1, FALSE, FALSE },
    { "remort",     do_remort, POS_STANDING,    1,  LOG_ALWAYS, 1, FALSE, FALSE },
    { "songlist",   do_song_list, POS_DEAD, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "stare",      do_stare,    POS_STANDING, 0, LOG_NORMAL, 1, FALSE, FALSE }, 
    { "stance",     do_stance,    POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "stancelist", do_stance_list,  POS_DEAD, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "snare",      do_set_snare,    POS_STANDING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "estimate",   do_estimate,    POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, FALSE },
    { "stalk",      do_stalk,       POS_FIGHTING, 0, LOG_NORMAL, 1, FALSE, FALSE },
    { "fvlist",     do_fvlist,  POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "vlist",      do_vlist,   POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "openvlist",  do_openvlist, POS_DEAD,  L4,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "peel" ,      do_peel,    POS_STANDING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "name",       do_name,    POS_DEAD,    0,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "showskill",  do_showskill, POS_DEAD, 0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "showsubclass", do_showsubclass, POS_DEAD, 0,  LOG_NORMAL, 1, FALSE, FALSE },
    { "showrace",   do_showrace, POS_DEAD, 0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "racelist",   do_racelist, POS_DEAD, 0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "forget",     do_forget,    POS_DEAD, 0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "remember",   do_remember,  POS_DEAD, 0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "clanreport", do_clanreport,  POS_DEAD, 0,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "morph",      do_morph,	POS_RESTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "gag",        do_gag,     POS_DEAD,    0, LOG_NORMAL, 1, FALSE, FALSE },
    { "try",        do_try,     POS_RESTING,    0, LOG_NORMAL, 1, FALSE, TRUE },
    { "peek",       do_peek,    POS_RESTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "turnin",     do_turn_in, POS_STANDING, 0, LOG_NORMAL, 1, FALSE, FALSE },
    { "disguise",   do_disguise, POS_STANDING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "tattoo",     do_tattoo,  POS_STANDING, 0, LOG_NORMAL, 1, FALSE, FALSE },
    { "root",       do_root,    POS_RESTING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    { "sire",       do_sire,    POS_STANDING, 0, LOG_NORMAL, 1, FALSE, TRUE },
    //{ "religion",   do_religion, POS_RESTING, 0, LOG_NORMAL, 1, FALSE, FALSE },
    //{ "prayer",     do_prayer,   POS_DEAD,   0, LOG_NORMAL, 1, FALSE, FALSE },
    { "die",	    do_die,     POS_DEAD,    0, LOG_ALWAYS, 1, FALSE, FALSE },
    { "calm",       do_calm,    POS_DEAD,    0, LOG_NORMAL, 1, FALSE, TRUE },
    { "helper",     do_helper,  POS_DEAD,    0, LOG_ALWAYS, 1, FALSE, FALSE },
    { "guiconfig",  do_guiconfig, POS_DEAD,  0, LOG_NORMAL, 1, FALSE, FALSE },

    { "changelog",  do_changelog, POS_DEAD,  0, LOG_NORMAL, 1, FALSE, FALSE },

    /* Freeze Tag */
    { "ftag",       do_ftag, POS_SLEEPING,  L8, LOG_NORMAL, 1, FALSE, FALSE },
    { "tag",        do_tag, POS_RESTING,  0,  LOG_NORMAL, 1, FALSE, FALSE },
    { "red",        do_red, POS_RESTING,  0,  LOG_NORMAL, 1, FALSE, FALSE },
    { "blue",       do_blue, POS_RESTING,  0,  LOG_NORMAL, 1, FALSE, FALSE },
    
    
    /*
    * Immortal commands.
    */
    { "advance",    do_advance, POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "authorize",  do_authorize, POS_DEAD, L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "avatar",	    do_avatar,  POS_DEAD,   L9,	 LOG_ALWAYS, 1, FALSE, FALSE  },
    { "printlist",  do_printlist,POS_DEAD,  ML,  LOG_NORMAL, 1, FALSE, FALSE  },
/* Used only for some pfile testing */
#ifdef TESTER
    { "charloadtest", do_charloadtest, POS_DEAD, ML, LOG_ALWAYS, 1, FALSE, FALSE },
#endif
    { "trust",      do_trust,   POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
//    { "pipe",       do_pipe,    POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "pgrep",      do_pgrep,   POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "pload",      do_pload,   POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "punload",    do_punload, POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "allow",      do_allow,   POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "ban",        do_ban,     POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "disable",    do_disable, POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "divorce",    do_divorce, POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    /* No legitimate in game use for this -- Maedhros 12/12/2011 */
		/*    { "flag",       do_flag,    POS_DEAD,   L4,  LOG_ALWAYS, 1, FALSE, FALSE  }, */
    { "freeze",     do_freeze,  POS_DEAD,   L4,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "jail",       do_jail,    POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "penlist",    do_penlist, POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "omni",       do_omni,    POS_DEAD,   L4,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "permban",    do_permban, POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "portal",     do_portal,  POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "reboo",      do_reboo,   POS_DEAD,   ML,  LOG_NORMAL, 0, FALSE, FALSE  },
    { "reboot",     do_reboot,  POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "repeat",     do_repeat,  POS_DEAD,   LT,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "reserve",    do_reserve, POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "set",        do_set,     POS_DEAD,   L4,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "setskill",   do_setskill,POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "shutdow",    do_shutdow, POS_DEAD,   ML,  LOG_NORMAL, 0, FALSE, FALSE  },
    { "shutdown",   do_shutdown,    POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "copyove",    do_copyove,   POS_DEAD,   L2, LOG_ALWAYS, 1, FALSE, FALSE },
    { "copyover",   do_copyover,POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "wizlock",    do_wizlock, POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "force",      do_force,   POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "load",       do_load,    POS_DEAD,   L9,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "newlock",    do_newlock, POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "nochannel",  do_nochannel,  POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "noemote",    do_noemote, POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "noshout",    do_noshout, POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "notell",     do_notell,  POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "nonote",     do_nonote,  POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "pecho",      do_pecho,   POS_DEAD,   L6,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "pardon",     do_pardon,  POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "parole",     do_parole,  POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "purge",      do_purge,   POS_DEAD,   L9,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "restore",    do_restore, POS_DEAD,   LT,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "sedit",      do_sedit,   POS_DEAD,   L4,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "sla",        do_sla,     POS_DEAD,   L3,  LOG_NORMAL, 0, TRUE, FALSE },
    { "slay",       do_slay,    POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "teleport",   do_transfer,    POS_DEAD,   L9,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "transfer",   do_transfer,    POS_DEAD,   L9,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "tick",       do_tick,    POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "poofin",     do_bamfin,  POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "poofout",    do_bamfout, POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "gecho",      do_echo,    POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "holylight",  do_holylight,   POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "incognito",  do_incognito,   POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "invis",      do_invis,   POS_DEAD,   L8,  LOG_NORMAL, 0, FALSE , FALSE },
    { "log",        do_log,     POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "memory",     do_memory,  POS_DEAD,   L2,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "perfmon",    do_perfmon, POS_DEAD,   L2,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "where",      do_where,   POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "mwhere",     do_mwhere,  POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "owhere",     do_owhere,  POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "peace",      do_peace,   POS_DEAD,   LT,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "echo",       do_recho,   POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "return",     do_return,  POS_DEAD,   L6,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "snoop",      do_snoop,   POS_DEAD,   L5,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "string",     do_string,  POS_DEAD,   L4,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "switch",     do_switch,  POS_DEAD,   L6,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "wizinvis",   do_invis,   POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "vnum",       do_vnum,    POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "zecho",      do_zecho,   POS_DEAD,   L6,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "pflag",      do_pflag,   POS_DEAD,   L4,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "clone",      do_clone,   POS_DEAD,   L4,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "wiznet",     do_wiznet,  POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "savantalk",  do_savantalk, POS_DEAD, L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "immtalk",    do_immtalk, POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "imotd",      do_imotd,       POS_DEAD,       IM,  LOG_NORMAL, 1, FALSE, FALSE  },
    { ":",          do_immtalk, POS_DEAD,   L8,  LOG_NORMAL, 0, FALSE, FALSE  },
    { "smote",      do_smote,   POS_DEAD,   IM,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "prefi",      do_prefi,   POS_DEAD,   L8,  LOG_NORMAL, 0, FALSE, FALSE  },
    { "prefix",     do_prefix,  POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "grant",      do_grant,   POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "gstat",      do_gstat,   POS_DEAD,   L1,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "revoke",     do_revoke,  POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "clandump",   do_clan_dump,  POS_DEAD, ML, LOG_ALWAYS, 1, FALSE, FALSE  },
    { "crimelist",  do_crimelist,  POS_DEAD, L8, LOG_NORMAL, 1, FALSE, FALSE  },
    { "review",     do_review,  POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "punish",     do_punish,  POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },    
    { "forgive",    do_forgive, POS_DEAD,   L2,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "flush",      do_flush,   POS_DEAD,   L8,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "qlist",      do_qlist,   POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "grep",       do_grep,    POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "ashift",     do_ashift,  POS_DEAD,   ML,  LOG_ALWAYS, 0, FALSE, FALSE  },
    { "rvnum",      do_rvnum,   POS_DEAD,   ML,  LOG_ALWAYS, 0, FALSE, FALSE  },
    { "crash",      do_crash,   POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "god",        do_god,     POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "cheatlog",   do_cheatlog,POS_DEAD,   L8,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "qset",       do_qset    ,POS_DEAD,   L2,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "mortlag",    do_mortlag, POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "tables",     do_tables,  POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "luai",       do_luai,    POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "luahelp",    do_luahelp, POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "luaconfig",  do_luaconfig, POS_DEAD, L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "luaquery",   do_luaquery, POS_DEAD,  ML,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "luareset",   do_luareset, POS_DEAD,  L2,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "mudconfig",  do_mudconfig, POS_DEAD, ML,  LOG_ALWAYS, 1, FALSE, FALSE  },
    { "protocol",   do_protocol, POS_DEAD,  L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "dxport",     do_dxport,  POS_DEAD,   ML,  LOG_ALWAYS, 1, FALSE, FALSE  },

    /*
    * OLC
    */
    { "edit",       do_olc,     POS_DEAD,   L2,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "asave",      do_asave,   POS_DEAD,   L9,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "alist",      do_alist,   POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "areset",     do_areset,  POS_DEAD,   LT,  LOG_ALWAYS, 1, FALSE, FALSE },
    { "resets",     do_resets,  POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "redit",      do_redit,   POS_DEAD,   L9,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "medit",      do_medit,   POS_DEAD,   L9,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "aedit",      do_aedit,   POS_DEAD,   L2,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "oedit",      do_oedit,   POS_DEAD,   L9,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "mpedit",     do_mpedit,  POS_DEAD,   L9,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "opedit",     do_opedit,  POS_DEAD,   L9,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "apedit",     do_apedit,  POS_DEAD,   L9,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "rpedit",     do_rpedit,  POS_DEAD,   L9,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "hedit",      do_hedit,   POS_DEAD,   L8,  LOG_ALWAYS, 1, TRUE, FALSE  },
    { "mpdump",     do_mpdump,  POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "opdump",     do_opdump,  POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "apdump",     do_apdump,  POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "rpdump",     do_rpdump,  POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "scriptdump", do_scriptdump, POS_DEAD, L9, LOG_NORMAL, 1, TRUE, FALSE  },
    { "mpstat",     do_mpstat,  POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "mprun",      do_mprun,   POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "aprun",      do_aprun,   POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "oprun",      do_oprun,   POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "rprun",      do_rprun,   POS_DEAD,   L9,  LOG_NORMAL, 1, TRUE, FALSE  },
    { "mpfind",     do_progfind,  POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "progfind",   do_progfind,  POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "lfind",      do_lfind,   POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "frfind",     do_frfind,  POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "findreset",  do_findreset, POS_DEAD, L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "diagnostic",do_diagnostic, POS_DEAD, L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    { "path",       do_path,    POS_DEAD,   L9,  LOG_NORMAL, 1, FALSE, FALSE  },
    
    /*
    * Erwin's REDIT
    */
    { "rlook",      do_rlook,    POS_DEAD,  L9,  LOG_NORMAL, 1, TRUE, FALSE },
    { "rwhere",     do_rwhere,   POS_DEAD,  L9,  LOG_NORMAL, 1, TRUE, FALSE },
    { "rrandom",    do_rrandom,  POS_DEAD,  L9,  LOG_ALWAYS, 1, TRUE, FALSE },
    /*
    { "rmob",       do_rmob,     POS_DEAD,  L9,  LOG_ALWAYS, 1, TRUE, FALSE },
    { "rput",       do_rput,     POS_DEAD,  L9,  LOG_ALWAYS, 1, TRUE, FALSE },
    { "rgive",      do_rgive,    POS_DEAD,  L9,  LOG_ALWAYS, 1, TRUE, FALSE },
    { "rwear",      do_rwear,    POS_DEAD,  L9,  LOG_ALWAYS, 1, TRUE, FALSE },
    { "rdrop",      do_rdrop,    POS_DEAD,  L9,  LOG_ALWAYS, 1, TRUE, FALSE },
    { "rkill",      do_rkill,    POS_DEAD,  L9,  LOG_ALWAYS, 1, TRUE, FALSE },
    */
    { "rdoor",      do_rdoor,    POS_DEAD,  L9,  LOG_ALWAYS, 1, TRUE, FALSE },
    { "findlock",   do_findlock, POS_DEAD,  L9,  LOG_NORMAL, 1, TRUE, FALSE },
    { "rforce",     do_rforce,   POS_DEAD,  L6,  LOG_ALWAYS, 1, TRUE, FALSE },
    
    /*
    * End of list.
    */
    { "",       0,      POS_DEAD,    0,  LOG_NORMAL, 0 }
};

/* 
 * returns wether a command can be ordered to victim
 * if victim is NUll returns wether it can be ordered to some victims
 */
bool can_order( const char *command, CHAR_DATA *victim )
{
    int cmd;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
        if ( command[0] == cmd_table[cmd].name[0]
	     && !str_prefix( command, cmd_table[cmd].name ) )
	{
	    /* some special commands CAN be ordered to NPCs */
	    if ( victim == NULL || IS_NPC(victim) )
	    {
            if ( cmd_table[cmd].do_fun == do_give
                || cmd_table[cmd].do_fun == do_put
                || cmd_table[cmd].do_fun == do_drop
                || cmd_table[cmd].do_fun == do_sacrifice
                || cmd_table[cmd].do_fun == do_spellup
                || cmd_table[cmd].do_fun == do_heal
                )
		    return TRUE;
	    }

	    return (cmd_table[cmd].charm);
	}

	return TRUE;
}

bool is_either_str( const char *prefix, const char *str, bool exact )
{
    if ( exact )
        return strcmp( prefix, str ) == 0;
    else
        return !str_prefix( prefix, str );
}

int find_command( CHAR_DATA *ch, char *command, bool exact )
{
    int trust = get_trust( ch );
    int cmd;

    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
        if ( command[0] == cmd_table[cmd].name[0]
            && is_either_str(command, cmd_table[cmd].name, exact)
            && (!cmd_table[cmd].olc || (!IS_NPC(ch) && ch->pcdata->security > 0)) )
        {
            if (cmd_table[cmd].level < LEVEL_IMMORTAL)
            {
                if (cmd_table[cmd].level >= trust) 
                    continue;
            }
            else
            {
		/* auth command is special case */
		if ( !strcmp(cmd_table[cmd].name, "authorize") && CAN_AUTH(ch) )
		    return cmd;

                if (ch->desc == NULL || !is_granted(ch,cmd_table[cmd].do_fun)) 
                    continue;
            }
            
	    return cmd;
        }
    }    

    return -1;
}

void send_position_message( CHAR_DATA *ch )
{
    switch( ch->position )
    {
    case POS_DEAD:      send_to_char("Lie still; you are DEAD.\n\r", ch); break;
    case POS_MORTAL:
    case POS_INCAP:     send_to_char("You are hurt far too bad for that.\n\r", ch); break;
    case POS_STUNNED:   send_to_char("You are too stunned to do that.\n\r", ch); break;
    case POS_SLEEPING:  send_to_char("In your dreams, or what?\n\r", ch); break;
    case POS_RESTING:   send_to_char("Nah... You feel too relaxed...\n\r", ch); break;
    case POS_SITTING:   send_to_char("Better stand up first.\n\r", ch); break;
    case POS_FIGHTING:  send_to_char("No way!  You are still fighting!\n\r", ch); break;
    }
}

/*
* The main entry point for executing commands.
* Can be recursively called from 'at', 'order', 'force'.
*/
void interpret( CHAR_DATA *ch, const char *argument )
{
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH] ;
    int cmd;
    
#if defined(MEMCHECK_ENABLE)
    int string_count = nAllocString ;
    int perm_count = nAllocPerm ;
    char cmd_copy[MAX_INPUT_LENGTH] ;
    strcpy(cmd_copy, argument) ;
#endif
    
    /*
    * Strip leading spaces.
    */
    while ( ISSPACE(*argument) )
        argument++;
    if ( argument[0] == '\0' )
        return;
    
    /*
     * No hiding?  Good god what WERE they thinking?
     *
     * REMOVE_BIT( ch->affect_field, AFF_HIDE );
     * affect_strip( ch, gsn_hide);
     */
    
    /*
     * Implement freeze command.
     */
    if ( IS_SET(ch->penalty, PENALTY_FREEZE) )	
    {
        send_to_char( "You're totally frozen!\n\r", ch );
        return;
    }
    
    /*
    * Grab the command word.
    * Special parsing so ' can be a command,
    *   also no spaces needed after punctuation.
    */
    strcpy( logline, argument );
    if ( !isalpha(argument[0]) && !isdigit(argument[0]) )
    {
        command[0] = argument[0];
        command[1] = '\0';
        argument++;
        while ( ISSPACE(*argument) )
            argument++;
    }
    else
    {
        argument = one_argument( argument, command );
    }
    
    /*
    * Look for command trigger or command table.
    */
    
    if ( (cmd = find_command(ch, command, TRUE)) == -1
	 && !check_social_new(ch, command, argument, TRUE)
	 && (cmd = find_command(ch, command, FALSE)) == -1
	 && !check_social_new(ch, command, argument, FALSE) )
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }
    
    if ( cmd == -1 )
	return;

    if ( cmd_table[cmd].level <= LEVEL_HERO )
    {
        if ( !rp_command_trigger( ch, cmd, argument ) )
            return;

        if ( mp_command_trigger( ch, cmd, argument ) )
            return;

        if ( !op_command_trigger( ch, cmd, argument ) )
            return;
    }

    /*
    * Log and snoop.
    */
    if ( cmd_table[cmd].log == LOG_NEVER )
        strcpy( logline, command );
    else if ( (!IS_NPC(ch) && IS_SET(ch->act, PLR_LOG) )
	      || fLogAll
	      || cmd_table[cmd].log == LOG_ALWAYS )
    {
        sprintf( log_buf, "Log %s: %s", ch->name, logline );
        wiznet(log_buf,ch,NULL,WIZ_SECURE,0,get_trust(ch));
        log_string( log_buf );
    }
    
    if ( ch->desc != NULL && ch->desc->snoop_by != NULL && logline[0] != '\0' )
    {
        write_to_buffer( ch->desc->snoop_by, "% ",    2 );
        write_to_buffer( ch->desc->snoop_by, logline, 0 );
        write_to_buffer( ch->desc->snoop_by, "\n\r",  2 );
    }
    
    /* a normal valid command.. check if it is disabled */
        if (check_disabled (&cmd_table[cmd]))
        {
            send_to_char ("This command has been temporarily disabled.\n\r",ch);
            return;
        }
        
	/* fix against invalid positions */
	if ( ch->position == POS_STANDING && ch->fighting != NULL )
	    ch->position = POS_FIGHTING;

        /*
        * Character not in position for command?
        */
        if ( ch->position < cmd_table[cmd].position )
        {
            send_position_message(ch);
            return;
        }
    
    if ( IS_AFFECTED(ch, AFF_PETRIFIED) && cmd_table[cmd].position > POS_DEAD )
    {
        send_to_char("You cannot do that while petrified.\n\r", ch);
        return;
    }
        
	/* Record the command */
	if ( !IS_NPC(ch) )
	    sprintf (last_command, "[%5d] %s in [%5d]: %s",
		     IS_NPC(ch) ? ch->pIndexData->vnum : 0,
		     IS_NPC(ch) ? ch->short_descr : ch->name,
		     ch->in_room ? ch->in_room->vnum : 0,
		     logline);

        /*
        * Dispatch the command.
        */
        (*cmd_table[cmd].do_fun) ( ch, argument );

	/* memleak tracker additions*/
#if defined(MEMCHECK_ENABLE)
	if (string_count < nAllocString)
	{
	    sprintf(buf,
	    "Memcheck : Increase in strings :: %s : %s (from %d to %d)"
	    , ch->name, cmd_copy, string_count, nAllocString) ;
	    wiznet(buf, NULL, NULL, WIZ_MEMCHECK,0,0) ;
	}

	if (perm_count < nAllocPerm)
	{
	    sprintf(buf,
	    "Increase in perms :: %s : %s (from %d to %d)"
	    , ch->name, cmd_copy, perm_count, nAllocPerm) ;
	    wiznet(buf, NULL, NULL, WIZ_MEMCHECK, 0,0) ;
	}
#endif

	/* reset variables that are only set for one command */
	helper_visible = FALSE;
	ignore_invisible = FALSE;
        
	/* Record that the command was the last done, but it is finished */
	if ( !IS_NPC(ch) )
    {
        /* if player quit, they're char is no longer valid so we
           need to just re-use info in last_command */
        strcpy(buf, last_command);
        sprintf (last_command, "(Finished) %s", buf );
    }
	
    tail_chain( );
    return;
}


bool check_social( CHAR_DATA *ch, const char *command, const char *argument )
{
    return check_social_new( ch, command, argument, FALSE );
}

bool check_social_new( CHAR_DATA *ch, const char *command, const char *argument, bool exact )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int cmd;
    bool found;
    bool sleeping = FALSE;
    
    found  = FALSE;
    for ( cmd = 0; social_table[cmd].name[0] != '\0'; cmd++ )
    {
        if ( command[0] == social_table[cmd].name[0]
            && is_either_str(command, social_table[cmd].name, exact) )
        {
            found = TRUE;
            break;
        }
    }
    
    if ( !found )
        return FALSE;

    /* for debug, record social as last command */
    if ( !IS_NPC(ch) )
        sprintf (last_command, "[%5d] %s in [%5d]: (social) %s %s",
             IS_NPC(ch) ? ch->pIndexData->vnum : 0,
             IS_NPC(ch) ? ch->short_descr : ch->name,
             ch->in_room ? ch->in_room->vnum : 0,
             command, argument);

    if ( !IS_NPC(ch) && IS_SET(ch->penalty, PENALTY_NOEMOTE) )
    {
        send_to_char( "You are anti-social!\n\r", ch );
        return TRUE;
    }
    
    switch ( ch->position )
    {
    case POS_DEAD:
        send_to_char( "Lie still; you are DEAD.\n\r", ch );
        return TRUE;
        
    case POS_INCAP:
    case POS_MORTAL:
        send_to_char( "You are hurt far too bad for that.\n\r", ch );
        return TRUE;
        
    case POS_STUNNED:
        send_to_char( "You are too stunned to do that.\n\r", ch );
        return TRUE;
        
    case POS_SLEEPING:
    /*
     * I just know this is the path to a 12'' 'if' statement.  :(
     * But two players asked for it already!  -- Furey
     */
        if ( !str_cmp( social_table[cmd].name, "snore" ) ||
             !str_cmp( social_table[cmd].name, "ssnore" )||
	     !str_cmp( social_table[cmd].name, "scratch" )||
	     !str_cmp( social_table[cmd].name, "mumble" )||
	     !str_cmp( social_table[cmd].name, "fart" )  ||
	     !str_cmp( social_table[cmd].name, "drool" ) ||
	     !str_cmp( social_table[cmd].name, "possum" ) ||
	     !str_cmp( social_table[cmd].name, "scratch" ) ||
	     !str_cmp( social_table[cmd].name, "grunt" ) )
	{
	    sleeping = TRUE;  /* Can only perform socials on yourself while sleeping. */
            break;
	}
        send_to_char( "In your dreams, or what?\n\r", ch );
        return TRUE;
        
    }
    
    one_argument( argument, arg );
    victim = NULL;
    if ( arg[0] == '\0' )
    {
        if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
        {
            affect_strip(ch,gsn_hide);
            REMOVE_BIT(ch->affect_field, AFF_HIDE);
            send_to_char( "You come out of hiding.\n\r", ch );
        }
        act( social_table[cmd].char_no_arg,   ch, NULL, victim, TO_CHAR );
        act( social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM );
    }
    else if ( sleeping )
    {
	/* Sleeping-socials can only be done with no argument */
        send_to_char( "In your dreams, or what?\n\r", ch );
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
    }
    else if ( victim == ch )
    {
        if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
        {
            affect_strip(ch,gsn_hide);
            REMOVE_BIT(ch->affect_field, AFF_HIDE);
            send_to_char( "You come out of hiding.\n\r", ch );
        }
        act( social_table[cmd].char_auto,     ch, NULL, victim, TO_CHAR    );
        act( social_table[cmd].others_auto,   ch, NULL, victim, TO_ROOM    );
    }
    else
    {
        if ( IS_NPC(victim)
	     && MOBtrigger
	     && HAS_TRIGGER(victim, TRIG_SOCIAL)
	     && mp_act_trigger(social_table[cmd].name, victim, ch,
			       NULL,0, NULL,0, TRIG_SOCIAL) )
	    ;
	else
	{
            if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
            {
                affect_strip(ch,gsn_hide);
                REMOVE_BIT(ch->affect_field, AFF_HIDE);
                send_to_char( "You come out of hiding.\n\r", ch );
            }
            act( social_table[cmd].char_found,    ch, NULL, victim, TO_CHAR    );
            act( social_table[cmd].vict_found,    ch, NULL, victim, TO_VICT    );
            act( social_table[cmd].others_found,  ch, NULL, victim, TO_NOTVICT );
        
	    if ( !IS_NPC(ch) && IS_NPC(victim)
		 &&   !IS_AFFECTED(victim, AFF_CHARM)
		 &&   IS_AWAKE(victim)
		 &&   !IS_SET(victim->act, ACT_OBJ)
		 &&   victim->desc == NULL)
	    {
		switch ( number_bits( 4 ) )
		{
		case 0:
		    
		case 1: case 2: case 3: case 4:
		case 5: case 6: case 7: case 8:
		    act( social_table[cmd].char_found,
			 victim, NULL, ch, TO_CHAR    );
		    act( social_table[cmd].vict_found,
			 victim, NULL, ch, TO_VICT    );
		    act( social_table[cmd].others_found,
			 victim, NULL, ch, TO_NOTVICT );
		    break;
		    
		case 9: case 10: case 11: case 12:
		    if ( IS_IMMORTAL(ch) )
		    {
			act( "You nod at $N.",  victim, NULL, ch, TO_CHAR    );
			act( "$n nods at you.", victim, NULL, ch, TO_VICT    );
			act( "$n nods at $N.",  victim, NULL, ch, TO_NOTVICT );
		    }
		    else
		    {
			act( "You slap $N.",  victim, NULL, ch, TO_CHAR    );
			act( "$n slaps you.", victim, NULL, ch, TO_VICT    );
			act( "$n slaps $N.",  victim, NULL, ch, TO_NOTVICT );
		    }
		    break;
		}
	    }
	}
    }
    return TRUE;
}



/*
* Return true if an argument is completely numeric.
*/
bool is_number ( const char *arg )
{
    
    if ( *arg == '\0' )
        return FALSE;
    
    if ( *arg == '+' || *arg == '-' )
        arg++;
    
    for ( ; *arg != '\0'; arg++ )
    {
        if ( !isdigit( *arg ) )
            return FALSE;
    }
    
    return TRUE;
}

int split_argument( const char *argument, char *arg, char split_char )
{
    const char *psplit = strchr(argument, split_char);
    if ( psplit == NULL )
    {
        strcpy(arg, argument);
        return 1;
    }
    int split_idx = psplit - argument;
    
    // valid number up till split_char?
    char buf[MIL];
    strncpy(buf, argument, split_idx);
    buf[split_idx] = '\0';
    if ( !is_number(buf) )
    {
        strcpy(arg, argument);
        return 1;
    }
    
    strcpy(arg, psplit+1);
    return atoi(buf);
}

/*
* Given a string like 14.foo, return 14 and 'foo'
*/
int number_argument( const char *argument, char *arg )
{
    return split_argument(argument, arg, '.');
}

/*
* Given a string like 14*foo, return 14 and 'foo'
*/
int mult_argument( const char *argument, char *arg )
{
    return split_argument(argument, arg, '*');
}

/*
* Pick off one argument from a string and return the rest.
* Understands quotes.
*/
const char * one_argument( const char *argument, char *arg_first )
{
    char cEnd;
    
    while ( ISSPACE(*argument) )
        argument++;
    
    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;
    
    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        if ( arg_first ) // allow NULL to just skip first arg
            *(arg_first++) = LOWER(*argument);
        argument++;
    }
    if ( arg_first )
        *arg_first = '\0';
    
    while ( ISSPACE(*argument) )
        argument++;
    
    return argument;
}

/*
* Pick off one or more arguments from a string and return the rest.
* Multiple arguments (as many as possible) are returned iff
* (1) The first argument is not quoted, and
* (2) The arguments together form a valid spell name prefix
*/
const char * spell_argument( const char *argument, char *arg_spell )
{
    while ( ISSPACE(*argument) )
        argument++;
    
    // if quoted only return first argument
    if ( *argument == '\'' || *argument == '"' )
        return one_argument(argument, arg_spell);
    
    // else parse as many as we can
    argument = one_argument(argument, arg_spell);
    char *next_arg_spell = arg_spell + strlen(arg_spell);
    while ( *argument != '\0' )
    {
        // extend spell name by one argument
        *next_arg_spell = ' ';
        const char *next_arg = one_argument(argument, next_arg_spell + 1);
        // check if it's a spell
        if ( spell_lookup(arg_spell) == -1 )
        {
            // no spell, undo last read and finish
            *next_arg_spell = '\0';
            break;
        }
        // still a spell, continue parsing
        argument = next_arg;
        next_arg_spell += strlen(next_arg_spell);
    }
    return argument;
}

/*
* Pick off one argument from a string and return the rest.
* Understands quotes. Doesn't lower case.
*/
const char * one_argument_keep_case( const char *argument, char *arg_first )
{
    char cEnd;
    
    while ( ISSPACE(*argument) )
        argument++;
    
    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;
    
    while ( *argument != '\0' )
    {
        if ( *argument == cEnd )
        {
            argument++;
            break;
        }
        *arg_first = *argument;
        arg_first++;
        argument++;
    }
    *arg_first = '\0';
    
    while ( ISSPACE(*argument) )
        argument++;
    
    return argument;
}

/*
* Contributed by Alander.
*/
DEF_DO_FUN(do_commands)
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;
    
    col = 0;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
        if ( cmd_table[cmd].level <  LEVEL_HERO
            &&   cmd_table[cmd].level <= get_trust( ch )
            &&   cmd_table[cmd].show
            &&   (!cmd_table[cmd].olc || (!IS_NPC(ch)&&ch->pcdata->security>0)) )
        {
            sprintf( buf, "%-12s", cmd_table[cmd].name );
            send_to_char( buf, ch );
            if ( ++col % 6 == 0 )
                send_to_char( "\n\r", ch );
        }
    }
    
    if ( col % 6 != 0 )
        send_to_char( "\n\r", ch );
    return;
}

DEF_DO_FUN(do_disable)
{
    int i;
    DISABLED_DATA *p,*q;
    char buf[100];
    
    char arg1[MAX_INPUT_LENGTH];
    int spell = 0;  /* Set to 1 if cmd to disable is a spell */
    int command_level = MAX_LEVEL + 1;
    bool found;
    
    if (IS_NPC(ch))
    {
        send_to_char ("RETURN first.\n\r",ch);
        return;
    }
    
    if (!argument[0]) /* Nothing specified. Show disabled commands. */
    {
        if (!disabled_first) /* Any disabled at all ? */
        {
            send_to_char ("There are no commands disabled.\n\r",ch);
            return;
        }
        
        send_to_char ("Disabled commands:\n\r"
            "Command      Level   Disabled By\n\r",ch);
        
        for (p = disabled_first; p; p = p->next)
        {
            sprintf (buf, "%-12s %5d   %-12s   %s\n\r",p->command_name, p->level, p->disabled_by, p->spell ? "(spell)" : "");
            send_to_char (buf,ch);
        }
        return;
    }
    
    /* command given */
    argument = one_argument( argument, arg1 );
    
    /* Detect if user wants to disable a spell. */
    if (!strcmp(arg1, "spell"))
    {
        spell = 1;
        argument = one_argument( argument, arg1 );
        
        if (arg1[0] == '\0')
        {
            send_to_char("Disable which spell?\n\r", ch);
            return;
        }
    }
    
    /* First check if it is one of the disabled commands */
    for (p = disabled_first; p ; p = p->next)
        if (!str_cmp(arg1, p->command_name) && p->spell == spell)
            break;
        
        if (p) /* this command is disabled */
        {
        /* Optional: The level of the imm to enable the command must equal or exceed level
            of the one that disabled it */
            
            if (get_trust(ch) < p->level)
            {
                send_to_char ("This command was disabled by a higher power.\n\r",ch);
                return;
            }
            
            /* Remove */
            
            if (disabled_first == p) /* node to be removed == head ? */
                disabled_first = p->next;
            else /* Find the node before this one */
            {
                for (q = disabled_first; q->next != p; q = q->next); /* empty for */
                q->next = p->next;
            }
            
            free_string(p->command_name);
            free_string (p->disabled_by); /* free name of disabler */
            free_mem (p,sizeof(DISABLED_DATA)); /* free node */
            save_disabled(); /* save to disk */
            send_to_char ("Command enabled.\n\r",ch);
        }
        else /* not a disabled command, check if that command exists */
        {
            /* IQ test */
            if (!str_cmp(arg1,"disable"))
            {
                send_to_char ("You cannot disable the disable command.\n\r",ch);
                return;
            }
            
            /* Here's the hack to support spells. */
            
            found = FALSE;
            
            if (spell)
            {
                for ( i = 0; i < MAX_SKILL; i++ )
                {
                    if (skill_table[i].name == NULL)
                        break;
                    if (LOWER(arg1[0]) == LOWER(skill_table[i].name[0])
                        &&  !str_prefix(arg1,skill_table[i].name))
                    {
                        found = TRUE;
                        command_level = skill_table[i].skill_level[ch->class];
                        break;
                    }
                }
            }
            else
            {
                /* Search for the command */
                for (i = 0; cmd_table[i].name[0] != '\0'; i++)
                    if (!str_cmp(cmd_table[i].name, arg1))
                    {
                        found = TRUE;
                        command_level = cmd_table[i].level;
                        break;
                    }
            }
            
            /* Found? */
            if (!found)
            {
                send_to_char (spell ? "No such spell.\n\r" : "No such command.\n\r",ch);
                return;
            }
            
            /* Can the imm use this command at all ? */
            if (command_level > get_trust(ch))
            {
                send_to_char ("You do not have access to that command -- you cannot disable it.\n\r",ch);
                return;
            }
            
            /* Disable the command */
            
            p = alloc_mem (sizeof(DISABLED_DATA));
            
            p->spell = spell;
            if (spell)
            {
                p->spell_fun = skill_table[i].spell_fun;
                p->do_fun = NULL;
                p->command_name = str_dup(skill_table[i].name);
            }
            else
            {
                p->spell_fun = NULL;
                p->do_fun = cmd_table[i].do_fun;
                p->command_name = str_dup(cmd_table[i].name);
            }
            p->disabled_by = str_dup (ch->name); /* save name of disabler */
            p->level = get_trust(ch); /* save trust */
            p->next = disabled_first;
            disabled_first = p; /* add before the current first element */
            
            send_to_char ("Command disabled.\n\r",ch);
            save_disabled(); /* save to disk */
        }
}

/* Check if that command is disabled
Note that we check for equivalence of the do_fun pointers; this means
that disabling 'chat' will also disable the '.' command
*/
bool check_disabled (const struct cmd_type *command)
{
    DISABLED_DATA *p;
    
    for (p = disabled_first; p ; p = p->next)
        if (!(p->spell) && (p->do_fun == command->do_fun))
            return TRUE;
        
        return FALSE;
}


bool check_spell_disabled (const struct skill_type *command)
{
    DISABLED_DATA *p;
    
    for (p = disabled_first; p ; p = p->next)
        if (p->spell && (p->spell_fun == command->spell_fun))
            return TRUE;
        
        return FALSE;
}


/* Load disabled commands */
void load_disabled()
{
    FILE *fp;
    DISABLED_DATA *p;
    const char *name;
    int i = 0;
    int spell;
    bool found;
    
    disabled_first = NULL;
    
    fp = fopen (DISABLED_FILE, "r");
    
    if (!fp) /* No disabled file.. no disabled commands : */
        return;
    
    name = fread_word (fp);
    
    while (str_cmp(name, END_MARKER)) /* as long as name is NOT END_MARKER :) */
    {
        spell = fread_number(fp);
        
        /* Find the command in the appropriate table */
        
        found = FALSE;
        
        if (spell)
        {
            if ((i = skill_lookup(name)) > 0)
                found = TRUE;
        }
        else
            for (i = 0; cmd_table[i].name[0] ; i++)
                if (!str_cmp(cmd_table[i].name, name))
                {
                    found = TRUE;
                    break;
                }
                
                if (found) /* add new disabled command */
                {
                    p = alloc_mem(sizeof(DISABLED_DATA));
                    p->command_name = str_dup(name);
                    p->spell = spell;
                    if (spell)
                    {
                        p->spell_fun = skill_table[i].spell_fun;
                        p->do_fun = NULL;
                    }
                    else
                    {
                        p->spell_fun = NULL;
                        p->do_fun = cmd_table[i].do_fun;
                    }
                    p->disabled_by = str_dup(fread_word(fp));
                    p->level = fread_number(fp);
                    p->next = disabled_first;
                    
                    disabled_first = p;
                }
                else  /* command does not exist? */
                {
                    bugf ("Skipping uknown command %s in " DISABLED_FILE " file.", name);
                    fread_word(fp);   /* disabled_by */
                    fread_number(fp); /* level */
                }
                
                name = fread_word(fp);
    }
    
    fclose (fp);
}

/* Save disabled commands */
void save_disabled()
{
    FILE *fp;
    DISABLED_DATA *p;
    
    if (!disabled_first) /* delete file if no commands are disabled */
    {
        unlink (DISABLED_FILE);
        return;
    }

    fp = fopen (DISABLED_FILE, "w");
    
    if (!fp)
    {
        bug ("Could not open " DISABLED_FILE " for writing",0);
        return;
    }
    
    for (p = disabled_first; p ; p = p->next)
        fprintf (fp, "'%s' %d %s %d\n", p->command_name, p->spell, p->disabled_by, p->level);
    
    fprintf (fp, "%s\n",END_MARKER);
    
    fclose (fp);
}
