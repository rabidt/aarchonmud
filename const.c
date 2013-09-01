/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1990 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 19002 by Michael          *
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
*   ROM 2.4 is copyright 19002-1996 Russ Taylor             *
*   ROM has been brought to you by the ROM consortium          *
*       Russ Taylor (rtaylor@efn.org)                  *
*       Gabrielle Taylor                           *
*       Brian Moore (zump@rom.org)                     *
*   By using this code, you have agreed to follow the terms of the     *
*   ROM license, in the file Rom24/doc/rom.license             *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"

extern int nAllocString;
extern int nAllocPerm;

/* item type list */
/* used in are files - DON'T CHANGE NAMES */
const struct item_type      item_table  []  =
{
    {   ITEM_LIGHT,         "light"        },
    {   ITEM_SCROLL,        "scroll"       },
    {   ITEM_WAND,          "wand"         },
    {   ITEM_STAFF,         "staff"        },
    {   ITEM_WEAPON,        "weapon"       },
    {   ITEM_TREASURE,      "treasure"     },
    {   ITEM_EXPLOSIVE,     "explosive"    },
    {   ITEM_ARMOR,         "armor"        },
    {   ITEM_POTION,        "potion"       },
    {   ITEM_CLOTHING,      "clothing"     },
    {   ITEM_FURNITURE,     "furniture"    },
    {   ITEM_TRASH,         "trash"        },
    {   ITEM_CONTAINER,     "container"    },
    {   ITEM_DRINK_CON,     "drink"        },
    {   ITEM_KEY,           "key"          },
    {   ITEM_FOOD,          "food"         },
    {   ITEM_MONEY,         "money"        },
    {   ITEM_BOAT,          "boat"         },
    {   ITEM_CORPSE_NPC,    "npc_corpse"   },
    {   ITEM_CORPSE_PC,     "pc_corpse"    },
    {   ITEM_FOUNTAIN,      "fountain"     },
    {   ITEM_PILL,          "pill"         },
    {   ITEM_PROTECT,       "protect"      },
    {   ITEM_MAP,           "map"          },
    {   ITEM_PORTAL,        "portal"       },
    {   ITEM_WARP_STONE,    "warp_stone"   },
    {   ITEM_ROOM_KEY,      "room_key"     },
    {   ITEM_GEM,           "gem"          },
    {   ITEM_JEWELRY,       "jewelry"      },
    {   ITEM_JUKEBOX,       "jukebox"      },
    {   ITEM_HOGTIE,        "hogtie"       },
    {   ITEM_DOWSING_STICK, "dowsingstick" },
    {   ITEM_BLACK_HERB,    "black_herb"   },
    {   ITEM_RED_HERB,      "red_herb"     },  
    {   ITEM_SILVER_HERB,   "silver_herb"  },  
    {   ITEM_MOTTLED_HERB,  "mottled_herb" },
    {   ITEM_ARROWS,        "arrows"       },
    {   0,                  NULL           }
};

/* weapon selection table */
const   struct  weapon_type weapon_table    []  =
{
   { "sword",   OBJ_VNUM_SCHOOL_SWORD,  WEAPON_SWORD,   &gsn_sword  },
   { "mace",    OBJ_VNUM_SCHOOL_MACE,   WEAPON_MACE,    &gsn_mace   },
   { "dagger",  OBJ_VNUM_SCHOOL_DAGGER, WEAPON_DAGGER,  &gsn_dagger },
   { "axe",     OBJ_VNUM_SCHOOL_AXE,    WEAPON_AXE,     &gsn_axe    },
   { "staff",   OBJ_VNUM_SCHOOL_STAFF,  WEAPON_SPEAR,   &gsn_spear  },
   { "flail",   OBJ_VNUM_SCHOOL_FLAIL,  WEAPON_FLAIL,   &gsn_flail  },
   { "whip",    OBJ_VNUM_SCHOOL_WHIP,   WEAPON_WHIP,    &gsn_whip   },
   { "polearm", OBJ_VNUM_SCHOOL_POLEARM,WEAPON_POLEARM, &gsn_polearm},
   { "gun",     OBJ_VNUM_SCHOOL_GUN,    WEAPON_GUN,     &gsn_gun    },
   { "bow",     OBJ_VNUM_SCHOOL_BOW,    WEAPON_BOW,     &gsn_bow    },
   { NULL,      0,                      0,              NULL        }
};


/* basic damage types for weapons */
const int weapon_base_damage[] =
{
    DAM_NONE,    /* exotic  */
    DAM_SLASH,   /* sword   */
    DAM_PIERCE,  /* dagger  */
    DAM_PIERCE,  /* spear   */
    DAM_BASH,    /* mace    */
    DAM_SLASH,   /* axe     */ 
    DAM_BASH,    /* flail   */
    DAM_SLASH,   /* whip    */
    DAM_SLASH,   /* polearm */
    DAM_PIERCE,  /* gun     */
    DAM_PIERCE,  /* bow     */
};

 
/* wiznet table and prototype for future flag setting */
const   struct wiznet_type      wiznet_table    []              =
{
	{    "on",           WIZ_ON,         IM },
	{    "prefix",       WIZ_PREFIX,     L8 },
	{    "ticks",        WIZ_TICKS,      L8 },
	{    "logins",       WIZ_LOGINS,     L8 },
	{    "sites",        WIZ_SITES,      L4 },
	{    "links",        WIZ_LINKS,      L8 },
	{    "newbies",      WIZ_NEWBIE,     L8 },
	{    "spam",         WIZ_SPAM,       L8 },
	{    "deaths",       WIZ_DEATHS,     L8 },
	{    "resets",       WIZ_RESETS,     L4 },
	{    "mobdeaths",    WIZ_MOBDEATHS,  L4 },
	{    "flags",        WIZ_FLAGS,      L4 },
	{    "penalties",    WIZ_PENALTIES,  L8 },
	{    "saccing",      WIZ_SACCING,    L8 },
	{    "levels",       WIZ_LEVELS,     L8 },
	{    "load",         WIZ_LOAD,       L2 },
	{    "restore",      WIZ_RESTORE,    L2 },
	{    "snoops",       WIZ_SNOOPS,     L2 },
	{    "switches",     WIZ_SWITCHES,   L2 },
	{    "secure",       WIZ_SECURE,     L2 },
	{    "asave",        WIZ_ASAVE,      L4 },
	{    "freezetag",    WIZ_FTAG,       L8 },
	{    "auth",         WIZ_AUTH,       L8 },
	{    "cheat",        WIZ_CHEAT,      L8 },
	{    "religion",     WIZ_RELIGION,   L4 },
#if defined(MEMCHECK_ENABLE)
	{    "memcheck",     WIZ_MEMCHECK,   ML },
#endif
    {    "bugs",         WIZ_BUGS,       L8 },
	{    NULL,           0,              0  }
};

/* attack table  -- not very organized :( */
const   struct attack_type  attack_table    [MAX_DAMAGE_MESSAGE]    =
{
    {   "none",         "hit",                  -1              },  /*  0 */
    {   "slice",        "slice",                DAM_SLASH       },  
    {   "stab",         "stab",                 DAM_PIERCE      },
    {   "slash",        "slash",                DAM_SLASH       },
    {   "whip",         "whip",                 DAM_SLASH       },
    {   "claw",         "claw",                 DAM_SLASH       },  /*  5 */
    {   "blast",        "blast",                DAM_BASH        },
    {   "pound",        "pound",                DAM_BASH        },
    {   "crush",        "crush",                DAM_BASH        },
    {   "grep",         "grep",                 DAM_OTHER       },
    {   "bite",         "bite",                 DAM_PIERCE      },  /* 10 */
    {   "pierce",       "pierce",               DAM_PIERCE      },
    {   "suction",      "suction",              DAM_BASH        },
    {   "beating",      "beating",              DAM_BASH        },
    {   "digestion",    "digestion",            DAM_ACID        },
    {   "charge",       "charge",               DAM_BASH        },  /* 15 */
    {   "slap",         "slap",                 DAM_BASH        },
    {   "punch",        "punch",                DAM_BASH        },
    {   "wrath",        "wrath",                DAM_ENERGY      },
    {   "magic",        "magic",                DAM_ENERGY      },
    {   "divine",       "divine power",         DAM_HOLY        },  /* 20 */
    {   "cleave",       "cleave",               DAM_SLASH       },
    {   "scratch",      "scratch",              DAM_PIERCE      },
    {   "peck",         "peck",                 DAM_PIERCE      },
    {   "peckb",        "peck",                 DAM_BASH        },
    {   "chop",         "chop",                 DAM_SLASH       },  /* 25 */
    {   "sting",        "sting",                DAM_PIERCE      },
    {   "smash",        "smash",                DAM_BASH        },
    {   "shbite",       "shocking bite",        DAM_LIGHTNING   },
    {   "flbite",       "flaming bite",         DAM_FIRE        },
    {   "frbite",       "freezing bite",        DAM_COLD        },  /* 30 */
    {   "acbite",       "acidic bite",          DAM_ACID        },
    {   "chomp",        "chomp",                DAM_PIERCE      },
    {   "drain",        "life drain",           DAM_NEGATIVE    },
    {   "thrust",       "thrust",               DAM_PIERCE      },
    {   "slime",        "slime",                DAM_ACID        },  /* 35 */
    {   "shock",        "shock",                DAM_LIGHTNING   },
    {   "thwack",       "thwack",               DAM_BASH        },
    {   "flame",        "flame",                DAM_FIRE        },
    {   "chill",        "chill",                DAM_COLD        },
    {   "pest",         "pestilence",           DAM_DISEASE     },  /* 40 */
    {   "txbite",       "toxic bite",           DAM_POISON      },
    {   "wrath",        "wrath",                DAM_HARM        },
    {   "illum",        "illumination",         DAM_LIGHT       },
    {   "anguish",      "anguish",              DAM_MENTAL      },
    {   "soaking",      "soaking",              DAM_DROWNING    },  /* 45 */
    {   "betrayal",     "betrayal",             DAM_CHARM       },
    {   "shriek",       "shriek",               DAM_SOUND       },
    {   "barrage",      "barrage",              DAM_BASH        },  
    {   "rain",         "rain of bullets",      DAM_SLASH       },
    {   "shot",         "shot",                 DAM_PIERCE      },  /* 50 */
    {   "bullet",       "bullet",               DAM_PIERCE      },
    {   "fiery",        "fiery blast",          DAM_FIRE        },
    {   "coldray",      "freeze ray",           DAM_COLD        },  
    {   "stream",       "acid stream",          DAM_ACID        },
    {   "shells",       "shell",                DAM_OTHER       },  /* 55 */
    {   "laser",        "laser ray",            DAM_ENERGY      },
    {   "sonic",        "sonic boom",           DAM_SOUND       },
    {   "spray",        "spray",                DAM_SLASH       },  
    {   "dumdums",      "dum dum",              DAM_BASH        },
    {   "frag",         "fragmenting round",    DAM_SLASH       },  /* 60 */
    {   "hollowpt",     "hollow point",         DAM_SLASH       },
    {   "concussion",   "concussion blast",     DAM_BASH        },
    {   "btalon",       "black talon",          DAM_NEGATIVE    },
    {   "blesbul",      "blessed bullet",       DAM_HOLY        },
    {   "pwhip",        "pistol whip",          DAM_BASH        },  /* 65 */
    {	"psionic",      "psionic blast",        DAM_MENTAL      },
    {   "hack",         "hack",                 DAM_MENTAL      },
    {   "acidrain",     "acid rain",            DAM_ACID        },
    {   "blizzard",     "blizzard",             DAM_COLD        },
    {   "shiver",       "shiver",               DAM_COLD        },  /* 70 */
    {   "charm",        "charm",                DAM_CHARM       },
    {   "song",         "song",                 DAM_SOUND       },
    {   "lure",         "lure",                 DAM_CHARM       },
    {   "infection",    "infection",            DAM_DISEASE     },
    {   "blaze",        "blaze",                DAM_FIRE        },  /* 75 */
    {   "exaltation",   "exaltation",           DAM_HOLY        },
    {   "glare",        "glare",                DAM_LIGHT       },
    {   "shine",        "shine",                DAM_LIGHT       },
    {   "luster",       "luster",               DAM_LIGHT       },
    {   "bolt",         "bolt",                 DAM_LIGHTNING   },  /* 80 */
    {   "static",       "static",               DAM_LIGHTNING   },
    {   "psych",        "psych",                DAM_MENTAL      },
    {   "mania",        "mania",                DAM_MENTAL      },
    {   "paralysis",    "paralysis",            DAM_MENTAL      },
    {   "souldrain",    "soul drain",           DAM_NEGATIVE    },  /* 85 */
    {   "venom",        "venom",                DAM_POISON      },
    {   "corrosion",    "corrosion",            DAM_ACID        },
    {   "screech",      "screech",              DAM_SOUND       },
    {   "virus",        "virus",                DAM_DISEASE     },
    {   "howl",         "howl",                 DAM_SOUND       },  /* 90 */
    {   "flooding",     "flooding",             DAM_DROWNING    },
    {   "drenching",    "drenching",            DAM_DROWNING    },
    {   "beam",         "beam",                 DAM_ENERGY      },
    {   "inferno",      "inferno",              DAM_FIRE        },
    {   "blessing",     "blessing",             DAM_HOLY        },  /* 95 */
    {  "contamination", "contamination",        DAM_POISON      },
    {   "demonicrage",  "demonic rage",         DAM_HOLY        },
    {"enlightenedfury", "enlightened fury",     DAM_HOLY        },
    {"darkinspiration", "dark inspiration",     DAM_HOLY        },
    {   "holyblast",    "holy blast",           DAM_HOLY        },  /* 100 */
    {   "evilblast",    "evil blast",           DAM_HOLY        },
    {   NULL,           NULL,                   0               }
};

struct align_type align_table [] =
{
/*
    {
    name, align
    },
*/
        { "satanic", -1000},
	{ "demonic",  -900},
	{ "evil",     -700},
	{ "mean",     -350},
	{ "neutral",  -100},
	{ "angelic",   900},
	{ "saintly",   700},
	{ "good",      350},
	{ "kind",      100},
	{ NULL,          0}
};



/* race table */
/*const*/   struct  race_type   race_table  []      =
{
/*
	{
	name,       pc_race?,
	act bits,   aff_by bits,    off bits,
	imm,        res,        vuln,
	form,       parts 
	},
*/
	{ "unique",     FALSE, {}, {}, {}, {}, {}, {}, {}, {} },

	{
	"avian",            TRUE,
	{},      {AFF_FLYING}, {},
	{},      {RES_LIGHTNING}, {VULN_BASH},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},  

	{
	"drow",          TRUE,
	{},      {AFF_DARK_VISION},   {},
	{},      {RES_MAGIC},  {VULN_LIGHT,VULN_POISON},
		  {A,H,M,V,cc},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"dwarf",        TRUE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_POISON}, {VULN_DROWNING},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"elf",          TRUE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_CHARM,RES_DISEASE},  {VULN_POISON,VULN_COLD},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

        {
	"gimp",        TRUE,
	{},      {},   {},
	{},      {}, {VULN_MAGIC,VULN_WEAPON},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

        {
	"goblin",        TRUE,
	{},      {AFF_DARK_VISION},   {},
	{},      {RES_BASH,RES_LIGHTNING}, {VULN_NEGATIVE,VULN_ENERGY},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"golem",        TRUE,
	{},      {},   {},
	{},      {RES_SOUND}, {VULN_MAGIC},
	{E,H,J,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"halfelf",          TRUE,
	{},      {},             {},
	{},      {RES_DISEASE},  {VULN_COLD},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"halfogre",          TRUE,
	{},      {},             {},
	{},      {RES_COLD},  {VULN_DISEASE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"halforc",          TRUE,
	{},      {AFF_DARK_VISION}, {},
	{},      {RES_COLD,RES_ACID},  {VULN_LIGHT,VULN_PIERCE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	}, //10

	{
	"hobbit",        TRUE,
	{},      {},   {},
	{},      {RES_BASH}, {VULN_MENTAL},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},
								 
	{ 
	"human",        TRUE, 
	{},      {},      {},
	{},      {},      {},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"leprechaun",        TRUE,
	{},      {AFF_DETECT_MAGIC},   {},
	{},      {RES_ENERGY}, {VULN_NEGATIVE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"mutant",        TRUE,
	{},      {},   {},
	{},      {RES_MENTAL}, {VULN_DISEASE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"myrddraal",        TRUE,
	{},      {},   {},
	{},      {RES_COLD,RES_DISEASE}, {VULN_HOLY,VULN_DROWNING},
	{A,B,H,M,V},    {A,B,C,F,G,H,I}
	},
      
	{
	"ogre",        TRUE,
	{},      {},   {},
	{},      {RES_COLD,RES_FIRE}, {VULN_DISEASE,VULN_LIGHTNING},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"pixie",        TRUE,
	{},      {AFF_FLYING,AFF_DETECT_GOOD},   {},
	{},  {RES_LIGHT}, {VULN_COLD},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"saurin",        TRUE,
	{},      {},   {},
	{},      {RES_PIERCE}, {VULN_ACID},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"troll",        TRUE,
	{},      {AFF_DARK_VISION},   {},
	{},      {RES_COLD,RES_NEGATIVE}, {VULN_FIRE,VULN_LIGHT},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	}, //20
//1
	{
	"amazon",        TRUE,
	{},      {},   {},
	{},      {RES_DROWNING}, {},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"centaur",        TRUE,
	{},      {},   {},
	{},      {RES_DISEASE,RES_COLD,RES_POISON}, {VULN_SOUND},
	{A,H,N,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"khan",        TRUE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_ENERGY}, {VULN_DROWNING},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"nymph",        TRUE,
	{},      {AFF_DETECT_MAGIC},   {},
	{},      {RES_MAGIC}, {VULN_ACID,VULN_FIRE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

        {
	"orc",          TRUE,
	{},      {AFF_DARK_VISION}, {},
	{},      {RES_COLD,RES_ACID},  {VULN_LIGHT},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	}, 

	{
	"satyr",        TRUE,
	{},      {},   {},
	{},      {RES_MENTAL,RES_CHARM,RES_SOUND}, {VULN_FIRE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

        {
	"zombie",        TRUE,
	{},      {AFF_DARK_VISION},   {},
	{},     {RES_SLASH}, {VULN_HOLY,VULN_FIRE},
	{A,H,I,M,V,cc},    {A,B,C,D,E,F,G,H,I,J,K}
	},
//2      
	{
	"cyclops",        TRUE,
	{},      {},   {},
	{},      {RES_BASH,RES_LIGHT,RES_NEGATIVE}, {VULN_PIERCE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"dunedain",        TRUE,
	{},      {},   {},
	{},      {RES_DISEASE,RES_POISON}, {},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"highelf",        TRUE,
	{},      {AFF_DETECT_EVIL},   {},
	{},      {RES_CHARM,RES_MENTAL,RES_HOLY}, {VULN_NEGATIVE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"mantis",        TRUE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_NEGATIVE,RES_LIGHT}, {VULN_COLD,VULN_FIRE},
	{A,H,O,cc},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"martian",        TRUE,
	{},      {},   {},
	{},      {RES_ACID,RES_ENERGY}, {VULN_DROWNING},
	{A,H,M,cc},    {A,B,C,D,E,F,G,H,I,J,K}
	}, //30
     
	{
	"skaven",        TRUE,
	{},      {AFF_SNEAK},   {},
	{},      {RES_ACID,RES_POISON}, {VULN_DISEASE,VULN_DROWNING},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},
//3      
        {
	"draconian",          TRUE,
	{},      {AFF_DETECT_MAGIC}, {},
	{},      {RES_FIRE,RES_PIERCE,RES_CHARM,RES_MENTAL}, {VULN_COLD,VULN_POISON},
	{A,H,M,V,Z,cc},    {A,B,C,D,E,F,G,H,I,J,K,X}
	},

        {
	"drider",        TRUE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_DISEASE,RES_ENERGY,RES_POISON,RES_NEGATIVE}, {VULN_LIGHT,VULN_MENTAL,VULN_DROWNING},
	{A,B,H,P,V},    {A,B,C,D,E,F,G,H,J,K}
	},

        {
	"phreak",        TRUE,
	{},      {},   {},
	{},      {RES_DISEASE,RES_POISON}, {},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},
	
        {
	"sprite",        TRUE,
	{},      {AFF_DETECT_INVIS,AFF_DETECT_MAGIC},   {},
	{},      {RES_MAGIC}, {VULN_BASH,VULN_MENTAL,VULN_DISEASE,VULN_NEGATIVE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

        {
	"vampil",          TRUE,
	{},      {AFF_DARK_VISION}, {},
	{},      {RES_COLD,RES_SLASH,RES_NEGATIVE}, {VULN_HOLY,VULN_WOOD,VULN_FIRE},
	{A,H,I,M,V,cc},    {A,B,C,D,E,F,G,H,I,J,K,V}
	},
//4      
	{
	"cyborg",          TRUE,
	{},      {AFF_DETECT_HIDDEN,AFF_INFRARED,AFF_BATTLE_METER}, {},
	{},      {RES_CHARM,RES_MENTAL,RES_DISEASE,RES_POISON,RES_COLD,RES_HOLY,RES_NEGATIVE}, 
	{VULN_ACID,VULN_DROWNING},
	{H,J,M,cc},    {A,B,C,G,H,I,J,K}
	},
	
	{
	"mermaid",        TRUE,
	{},      {AFF_BREATHE_WATER},   {},
	{},      {RES_SLASH,RES_DROWNING,RES_POISON,RES_FIRE,RES_ACID}, {VULN_LIGHTNING},
	{A,H,M,aa},    {A,B,D,E,F,G,I,J,K,O,Q,X}
	},
	
	{
	"minotaur",        TRUE,
	{},      {AFF_DARK_VISION},   {},
	{},      {RES_BASH,RES_COLD}, {VULN_DISEASE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K,Q,W}
	},

	{
	"werewolf",        TRUE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_WEAPON,RES_NEGATIVE}, {VULN_LIGHT,VULN_HOLY,VULN_SILVER},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K,Q,U,V}
	}, //40
	
	{
	"wisp",        TRUE,
	{},      {AFF_DETECT_GOOD,AFF_DETECT_EVIL,AFF_FLYING,AFF_HASTE},   {},
	{},      {RES_PIERCE,RES_LIGHT,RES_SOUND,RES_ENERGY,RES_FIRE,RES_LIGHTNING}, {VULN_COLD},
	{C,H,K,M,W,dd},    {F}
	},
//5
	{ 
	"doppelganger",        TRUE, 
	{},      {},      {},
	{},      {},      {VULN_WEAPON,VULN_MAGIC},
	{A,H,S,cc},    {A,B,C,D,E,K,N}
	},

	{ 
	"harpy",        TRUE, 
	{},      {AFF_DARK_VISION,AFF_PROTECT_GOOD,AFF_FLYING},      {},
	{},      {RES_POISON,RES_NEGATIVE,RES_ACID},      {VULN_BASH},
	{A,B,H,N,W},    {A,B,C,D,E,F,G,H,I,J,K,L,P,Q,U,V}
	},

	{ 
	"naga",        TRUE, 
	{},      {AFF_INFRARED,AFF_BREATHE_WATER,AFF_DETECT_MAGIC},      {},
	{},      {RES_PIERCE,RES_DROWNING,RES_HOLY,RES_NEGATIVE,RES_COLD}, {VULN_LIGHTNING},
	{A,B,H,R,Y,cc},    {A,B,D,E,F,G,I,J,K,L,N,Q,U,V,X}
	},

	{ 
	"treant",        TRUE, 
	{},      {},      {},
	{},      {RES_LIGHT,RES_DROWNING,RES_DISEASE,RES_POISON,RES_BASH,RES_ACID,RES_MENTAL}, {VULN_FIRE,VULN_SLASH},
        {H,M,U,cc,ee},    {A,B,C,D,G,H,I,J,K,N}
	},

	{ 
	"wraith",        TRUE, 
	{},      {AFF_DETECT_MAGIC,AFF_DETECT_INVIS,AFF_FLYING,AFF_PASS_DOOR,AFF_CHAOS_FADE},      {},
	{},      {RES_WEAPON,RES_COLD,RES_NEGATIVE,RES_DISEASE,RES_POISON},{VULN_LIGHT,VULN_SILVER,VULN_ENERGY,VULN_HOLY,VULN_FIRE},
        {D,H,I,M,cc},    {D}
	},
//6
	{
	"chrysalies",        TRUE,
	{},      {AFF_FLYING,AFF_DARK_VISION},   {},
	{},      {RES_CHARM,RES_PIERCE}, {VULN_DROWNING},
	{G,H,M,O}, {A,B,C,D,E,F,G,H,I,J,K,P}
	},

	{
	"frost-giant",        TRUE,
	{},      {},   {},
	{},      {RES_COLD}, {VULN_MENTAL,VULN_CHARM},
	{A,H,M,V,hh},    {A,B,C,D,E,F,G,H,I,J,K}
	},
	{

	"gholam",        TRUE,
	{},      {AFF_DETECT_MAGIC,AFF_DETECT_INVIS,AFF_DETECT_ASTRAL,AFF_PROTECT_MAGIC},   {},
	{},      {RES_MAGIC}, {VULN_SILVER,VULN_PIERCE},
	{A,H,M,V,gg},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"illithid",        TRUE,
	{},      {AFF_DETECT_MAGIC, AFF_DARK_VISION},   {},
	{},      {RES_CHARM,RES_MENTAL}, {VULN_LIGHT,VULN_SOUND},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K,N}
	},

        {
	"vampire",          TRUE,
	{},      {AFF_DARK_VISION}, {},
	{},      {RES_COLD,RES_SLASH,RES_NEGATIVE}, {VULN_LIGHT,VULN_HOLY,VULN_WOOD,VULN_FIRE},
	{A,H,I,M,V,cc,ff},    {A,B,C,D,E,F,G,H,I,J,K,V}
	},

//7
	{
	"ahazu",        TRUE,
	{},      {},   {},
	{},      {}, {VULN_CHARM},
	{A,H,V,M},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"djinn",        TRUE,
	{},      {},   {},
	{},      {RES_FIRE}, {VULN_DROWNING,VULN_SOUND},
	{C,K,S,ii},	{A,B,C,D,E,F,G,H,I,J,K},
	},

	{
	"dryad",        TRUE,
	{},	{AFF_DETECT_MAGIC,AFF_DETECT_HIDDEN},	{},
	{},      {RES_MAGIC}, {VULN_NEGATIVE},
	{A,H,M,V,jj},	{A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"gargoyle",        TRUE,
	{},      {AFF_FLYING,AFF_DARK_VISION},   {},
	{},      {RES_BASH}, {VULN_LIGHT},
	{A,H,M,V,N,ee},	{A,B,C,D,E,F,G,H,I,J,K,U,V},
	},

	{
	"lich",        TRUE,
	{},      {AFF_DARK_VISION,AFF_DETECT_MAGIC},   {},
	{},      {RES_NEGATIVE,RES_MENTAL}, {VULN_HOLY,VULN_LIGHT},
//undead,wise
	{B,H,I,M,cc,jj},    {A,B,C,D,E,F,G,H,I,J,K}
	},

//8

	{
	"android",        TRUE,
	{},      {AFF_DETECT_HIDDEN,AFF_INFRARED,AFF_BATTLE_METER},   {},
	{},      {RES_FIRE,RES_LIGHTNING,RES_MENTAL}, {VULN_ACID},
	{H,J,M,cc,kk},   {A,B,C,G,H,I,J,K}
	},

	{
	"naiad",        TRUE,
	{},      {AFF_DETECT_MAGIC,AFF_BREATHE_WATER},   {},
	{},      {RES_DROWNING}, {},
	{A,H,M,V,jj},    {A,B,C,D,E,F,G,H,I,J,K,P}
	},

	{
	"phantom",        TRUE,
	{},      {AFF_FLYING,AFF_PASS_DOOR,AFF_MINOR_FADE,AFF_NO_TRACE},   {},
	{},      {RES_PIERCE,RES_SLASH}, {VULN_ENERGY,VULN_LIGHT},
	{D,H,K,N},    {D}
	},

        {
        "tengu",    TRUE,
        {},      {AFF_FLYING},        {},
        {},      {},    {},
	{A,H,M,V,jj},    {A,B,C,D,E,F,G,H,I,J,K}
        },

	{
	"titan",        TRUE,
	{},      {},   {},
	{},      {RES_HOLY,RES_NEGATIVE}, {VULN_LIGHTNING},
	{A,H,V,M,ee},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"voadkin",        TRUE,
	{},      {},   {},
	{},      {RES_DISEASE}, {},
	{A,H,V,M},    {A,B,C,D,E,F,G,H,I,J,K}
	},
//10
	{
	"behemoth",        TRUE,
	{},      {AFF_DARK_VISION},   {},
	{},      {RES_WEAPON,RES_COLD}, {VULN_MENTAL},
	{A,H,M,V,ee},    {A,B,C,D,E,F,G,H,I,J,K,U,V}
	},

/* NPC RACES */
{
	"bat",          FALSE,
	{},      {AFF_FLYING,AFF_DARK_VISION, OFF_DODGE,OFF_FAST},
	{},      {},      {VULN_LIGHT},
	    {A,G,V},      {A,C,D,E,F,H,J,K,P}
	},

	{
	"bear",         FALSE,
	{},      {},      {OFF_CRUSH,OFF_DISARM,OFF_BERSERK},
	{},      {RES_BASH,RES_COLD},  {},
	    {A,G,V},      {A,B,C,D,E,F,H,J,K,U,V}
	},

	{
	"cat",          FALSE,
	{},      {AFF_DARK_VISION},    {OFF_FAST,OFF_DODGE},
	{},      {},      {},
	    {A,G,V},      {A,C,D,E,F,H,J,K,Q,U,V}
	},

	{
	"centipede",        FALSE,
	{},      {AFF_DARK_VISION},    {},
	{},      {RES_PIERCE,RES_COLD},    {VULN_BASH},
	    {A,B,G,O},        {A,C,K}   
	}, //50

	{
	"dog",          FALSE,
	{},      {},      {OFF_FAST},
	{},      {},      {},
	    {A,G,V},      {A,C,D,E,F,H,J,K,U,V}
	},

	{
	"doll",         FALSE,
	{},      {},      {},
	{IMM_COLD,IMM_POISON,IMM_HOLY,IMM_NEGATIVE,IMM_MENTAL,IMM_DISEASE
	,IMM_DROWNING},  {RES_BASH,RES_LIGHT},
	{VULN_SLASH,VULN_FIRE,VULN_ACID,VULN_LIGHTNING,VULN_ENERGY},
	    {E,J,M,cc},   {A,B,C,G,H,K}
	},

	{
	"fido",         FALSE,
	{},      {},      {OFF_DODGE,ASSIST_RACE},
	{},      {},          {VULN_MAGIC},
	    {A,B,G,V},    {A,C,D,E,F,H,J,K,Q,V}
	},      
   
	{
	"fox",          FALSE,
	{},      {AFF_DARK_VISION},    {OFF_FAST,OFF_DODGE},
	{},      {},      {},
	    {A,G,V},      {A,C,D,E,F,H,J,K,Q,V}
	},

	{
	"hobgoblin",        FALSE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_DISEASE,RES_POISON}, {},
	    {A,H,M,V},        {A,B,C,D,E,F,G,H,I,J,K,Y}
	},

	{
	"kobold",       FALSE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_POISON}, {VULN_MAGIC},
	    {A,B,H,M,V},  {A,B,C,D,E,F,G,H,I,J,K,Q}
	},

	{
	"lizard",       FALSE,
	{},      {},      {},
	{},      {RES_POISON}, {VULN_COLD},
	    {A,G,X,cc},   {A,C,D,E,F,H,K,Q,V}
	},

	{
	"modron",       FALSE,
	{},      {AFF_INFRARED},       {ASSIST_RACE,ASSIST_ALIGN},
	{IMM_CHARM,IMM_DISEASE,IMM_MENTAL,IMM_HOLY,IMM_NEGATIVE},
	    {RES_FIRE,RES_COLD,RES_ACID}, {},
		{H},      {A,B,C,G,H,J,K}
	},
/*
	{
	"orc",          FALSE,
	{},      {AFF_INFRARED},   {},
	{},      {RES_DISEASE},    {VULN_LIGHT},
	    {A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},
*/	
	{
	"pig",          FALSE,
	{},      {},      {},
	{},      {},      {},
	    {A,G,V},      {A,C,D,E,F,H,J,K}
	}, //60 

	{
	"rabbit",       FALSE,
	{},      {},      {OFF_DODGE,OFF_FAST},
	{},      {},      {},
	    {A,G,V},      {A,C,D,E,F,H,J,K}
	},
	
	{
	"school monster",   FALSE,
	{ACT_NOALIGN},        {},      {},
	{IMM_CHARM,IMM_SUMMON},   {},      {VULN_MAGIC},
	    {A,M,V},      {A,B,C,D,E,F,H,J,K,Q,U}
	},  

	{
	"snake",        FALSE,
	{},      {},      {},
	{},      {RES_POISON}, {VULN_COLD},
	    {A,G,X,Y,cc}, {A,D,E,F,K,L,Q,V,X}
	},
	
	{
	 "yeti",                FALSE,
	 {},             {AFF_BERSERK}, {},
	 {},         {RES_COLD}, {VULN_FIRE},
	     {A,H,M,cc},      {A,B,C,D,E,F,G,H,I,J,K,U}
	},

	{
	 "quasit",                  FALSE,
	 {},             {AFF_SNEAK,AFF_REGENERATION}, {},
	 {},             {RES_FIRE}, {VULN_COLD},
	     {B,G,H,X,cc},    {A,B,C,D,E,F,G,H,I,J,K,L,P,Q,U,V,W,X}
	},
/* The following non-stock races are used in prrlzham.are. */
	{
	"ghost",                FALSE,
	{ACT_UNDEAD},                 {AFF_FLYING,AFF_PASS_DOOR},   {},
	{},              {RES_COLD,RES_PIERCE,RES_NEGATIVE}, {VULN_HOLY,VULN_LIGHT,VULN_ENERGY}, 
	{C,D,H,I,L,cc},     {A,B,C,G,H,J,K}
	},
 
	{
	"manx",                    FALSE,
	{},                 {AFF_DARK_VISION}, {},
	{},     {RES_COLD}, {VULN_LIGHT},
	{A,B,H,I,M},         {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	"nazgul",                  FALSE,
	{},                 {AFF_FLYING,AFF_DETECT_GOOD}, {},
	{},     {RES_FIRE,RES_POISON}, {VULN_COLD,VULN_HOLY},
	{A,B,H,I,M},         {A,B,C,D,E,F,G,H,I,J,K}
	},
 
	{
	"song bird",        FALSE,
	{},      {AFF_FLYING},     {OFF_FAST,OFF_DODGE},
	{},      {},      {},
	    {A,G,W},      {A,C,D,E,F,H,K,P}
	},

	{
	"water fowl",       FALSE,
	{},      {AFF_BREATHE_WATER,AFF_FLYING},    {},
	{},      {RES_DROWNING},       {},
	    {A,G,W},      {A,C,D,E,F,H,K,P}
	},      
  
	{
	"wolf",         FALSE,
	{},      {AFF_DARK_VISION},    {OFF_FAST,OFF_DODGE},
	{},      {},      {},  
	    {A,G,V},      {A,C,D,E,F,J,K,Q,V}
	},

	{
	"wyvern",       FALSE,
	{},      {AFF_FLYING,AFF_DETECT_INVIS,AFF_DETECT_HIDDEN},
	{OFF_BASH,OFF_FAST,OFF_DODGE},
	{IMM_POISON}, {},  {VULN_LIGHT},
	    {A,B,G,Z},        {A,C,D,E,F,H,J,K,Q,V,X}
	},
	
	{
	"unicorn",        FALSE,
	{},      {},   {},
	{},      {RES_HOLY}, {VULN_NEGATIVE},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K,W}
	},

	{
	"unique",       FALSE,
	{},      {},      {},
	{},      {},      {},      
	{},      {}
	},


	{
	  NULL, FALSE, {}, {}, {}, {}, {}, {}, {}, {}
	}
};

struct  pc_race_type    pc_race_table   [MAX_PC_RACE]  =
{
	{ 
	"null race", "",
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	0, { "" },{0},{0},
	{  20,  20,  20,  20,  20,      20,  20,  20,  20,  20 },
	{ 100, 100, 100, 100, 100,     100, 100, 100, 100, 100 },   
	{   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},

//   War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec
	{
	"avian",     "Avian ",
    {110, 110, 105, 110, 110, 110, 105, 110, 110, 105, 105, 110, 110, 105, 105},
	2, { "meditation", "lore" }, {2, 40}, {40, 50},
	{  15,  20,  35,  30,  25,      20,  30,  30,  20,  20 },
	{  95,  85, 110, 105,  95,      90, 110, 100,  95, 100 }, 
	{   4,   3,   5,   4,   4,       4,   4,   4,   4,   4 },
	SIZE_LARGE, SEX_BOTH, 0
	},

	{   
	"drow",      "Drow  ",
    {115, 110, 115, 110, 115, 115, 115, 110, 110, 115, 115, 115, 115, 115, 110},
	2, { "faerie fire", "fly" }, {8, 50}, {70, 50},
	{  25,  20,  25,  25,  25,      30,  40,  35,  25,  10 },
	{  95,  90,  95, 105, 105,     110, 100,  95,  90,  95 },   
	{   4,   3,   4,   4,   4,       5,   4,   4,   4,   4 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},
	
	{
	"dwarf",    "Dwarf ",
    {105, 110, 110, 115, 110, 110, 110, 110, 110, 105, 110, 115, 105, 110, 115},
	1, { "drunken fury" }, {25}, {70},
	{  45,  50,  25,  10,  30,      40,  30,  35,  20,  20 },
	{  95, 110, 105,  80,  95,      85, 105, 110,  80,  100 },   
	{   4,   5,   4,   3,   4,       4,   4,   5,   3,   4 },
	SIZE_SMALL, SEX_BOTH, 0
	},

	{   
	"elf",      "Elf   ",
    {110, 105, 110, 105, 115, 110, 110, 105, 105, 110, 110, 105, 110, 105, 110},
	2, { "sneak", "hide" }, {20, 10}, {80, 80},
	{  15,  20,  20,  25,  35,      30,  20,  20,  25,  20 },
	{  95,  80, 100, 105, 105,     100, 110,  90, 105, 100 },   
	{   4,   3,   4,   4,   4,       4,   5,   4,   4,   4 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},
		
	{
	"gimp",    "Gimp  ",
    { 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60},
	0, { "" }, {0}, {0},
	{ 60, 50, 40, 50, 60,      40, 50, 60, 60, 60 },
	{ 80, 80, 80, 80, 80,      80, 80, 80, 80, 80 },   
	{   2,   2,   2,   2,   2,       2,   2,   2,   2,   2 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},
// race 5 ^
	{
	"goblin",    "Goblin",
    {105, 105, 115, 115, 105, 110, 115, 105, 105, 110, 115, 115, 105, 105, 115},
	3, { "trip", "gouge", "mug" }, {10,30,50}, {80,75,70},
	{  45,  50,  40,  55,  20,      40,  50,  50,  50,  55 },
	{ 110, 105, 100, 115, 100,      75,  80,  75,  75,  80 },   
	{   5,   5,   4,   5,   4,       3,   4,   3,   3,   4 },
	SIZE_SMALL, SEX_BOTH, 0
	},

	{
	"golem",    "Golem ",
    {100, 110, 110, 110, 105, 105, 105, 110, 110, 110, 110, 110, 105, 110, 110},
	2, { "stone skin", "bash" }, {5,1}, {70, 90},
	{  35,  35,  20,   1,  10,      15,  20,  35,   5,  25 },
	{ 115, 115,  80,  80,  80,      75,  80, 115,  75,  85 },   
	{   5,   5,   4,   3,   4,       3,   4,   5,   3,   4 },
	SIZE_HUGE, SEX_BOTH, 0
	},

	{
	"halfelf",    "HlfElf",
    {110, 105, 110, 105, 110, 110, 110, 105, 105, 110, 110, 105, 110, 105, 110},
	1, { "sneak" }, {25}, {60},
	{  25,  20,  20,  15,  30,      30,  30,  30,  25,  20 },
	{  95,  90, 100, 105, 100,     100, 100,  90, 105, 100 },   
	{   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},

	{
	"halfogre",    "HlfOgr",
    { 105, 110, 110, 110, 105, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110 },
	1, { "bash" }, {30},{60},
	{  30,  35,  25,  20,  25,      25,  25,  25,  15,  20 },
	{ 110, 105, 105,  95,  95,      90,  95,  95,  95, 100 },   
	{   5,   4,   4,   4,   4,       3,   4,   4,   4,   4 },
	SIZE_LARGE, SEX_BOTH, 0
	},

	{
	"halforc",    "HlfOrc",
    { 105, 105, 110, 110, 105, 110, 110, 100, 105, 110, 110, 110, 110, 110, 110 },
	1, { "backstab" }, {12}, {40},
	{  40,  45,  30,  35,  35,      20,  15,  15,  10,  15 },
	{ 100, 105, 110,  95,  95,     100,  95, 105,  85,  90 },   
	{   4,   4,   5,   4,   4,       4,   4,   4,   3,   4 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},

	{
	"hobbit",    "Hobbit",
    { 115, 100, 110, 110, 115, 115, 115, 105, 105, 110, 110, 105, 105, 105, 110 },
	3, { "steal", "sneak", "create food" }, {10, 1, 15}, {60, 80, 50},
	{  15,  35,  30,  35,  20,      25,  25,  20,  30,  25 },
	{  85,  80,  95, 115, 115,      90, 100,  90, 100, 110 },   
	{   3,   3,   4,   5,   5,       4,   4,   3,   4,   5 },
	SIZE_SMALL, SEX_BOTH, 0
	},

	{
	"human",    "Human ",
    { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	0, { "" },{0},{0},
	{  20,  20,  20,  20,  20,      20,  20,  20,  20,  20 },
	{ 100, 100, 100, 100, 100,     100, 100, 100, 100, 100 },   
	{   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},

	{
	"leprechaun",    "Leprec",
    { 115, 105, 110, 105, 115, 115, 115, 110, 110, 110, 110, 105, 110, 110, 105 },
	2, { "charm person", "haste" }, {10, 50}, {90, 50},
	{  10,  15,  35,  25,  20,      30,  35,  30,  30,  30 },
	{  80,  90,  95, 105, 100,     115,  85,  80, 115, 115 },   
	{   3,   4,   4,   4,   4,       5,   3,   3,   5,   5 },
	SIZE_SMALL, SEX_BOTH, 0
	},

	{
	"mutant",    "Mutant",
    { 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110 },
	2, { "fear", "change sex" }, {40, 20}, {70, 70},
	{   1,   5,   1,   1,   1,       1,   1,   1,   5,   1 },
	{ 110,  90, 110, 110, 110,     110, 110, 110,  90, 110 },   
	{   4,   2,   4,   4,   4,       4,   4,   4,   2,   4 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},

	{
	"myrddraal",    "Myrdrl",
    { 105, 105, 115, 115, 110, 115, 115, 100, 105, 110, 115, 110, 110, 115, 115 },
	3, { "fear", "hide", "shadowwalk" }, {1, 20, 40}, {50, 50, 50},
	{  35,  35,  25,  10,  25,      20,  10,  40,  1,  1 },
	{ 115, 115, 105,  90 , 105,     100, 90,  120, 80, 80 },   
	{   5,   5,   4,   4,   4,       4,   3,   5,   3,   3 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},

	{
	"ogre",    "Ogre  ",
    { 105, 110, 115, 115, 105, 115, 115, 115, 115, 115, 115, 115, 110, 110, 115 },
	2, { "bash", "fast healing" }, {20, 10}, {80, 60},
	{  55,  35,  35,  15,  30,      20,  30,  25,  10,  20 },
	{ 115, 115, 105,  95,  95,      75,  90,  95,  90, 100 },   
	{   5,   5,   4,   4,   4,       2,   4,   4,   4,   4 },
	SIZE_HUGE, SEX_BOTH, 0
	},

	{
	"pixie",    "Pixie ",
    { 115, 110, 110, 105, 115, 115, 115, 110, 110, 110, 110, 105, 110, 105, 110 },
	2, { "faerie fog", "faerie fire" }, {5, 1}, {80, 90},
	{  15,  15,  40,  45,  30,      45,  30,  35,  35,  30 },
	{  75,  75, 100, 105, 100,     110, 115,  75, 105, 100 },   
	{   2,   2,   5,   5,   5,       5,   5,   2,   5,   4 },
	SIZE_TINY, SEX_BOTH, 0
	},

	{
	"saurin",    "Saurin",
    { 105, 110, 110, 110, 105, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110 },
	1, { "venom bite" }, {3},   {60},
	{  40,  40,  40,  20,  20,      25,  50,  25,  40,  20 },
	{ 100, 105, 100, 100,  95,      85,  90, 105,  80, 100 },   
	{   4,   5,   4,   4,   4,       4,   4,   4,   3,   4 },
	SIZE_MEDIUM, SEX_BOTH, 0
	},

	{
	"troll",     "Troll ",
    { 110, 115, 115, 115, 110, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115 },
	1, { "regeneration"}, {1}, {80},
	{  30,  55,  40,   5,  20,      20,  25,  10,   5,   5 },
	{ 110, 115, 120, 100, 100,     100,  90,  90,  80,  90 },   
	{   5,   5,   5,   4,   4,       4,   4,   3,   3,   3 },
	SIZE_LARGE, SEX_BOTH, 0
	},

//r1
//    War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec
	{
	"amazon",    "Amazon",
    { 120, 130, 130, 135, 125, 125, 125, 125, 125, 125, 130, 135, 125, 125, 135 },
	2, { "second attack", "war cry" },{10, 30},{50, 60},
	{  35,  35,  20,  30,  30,      35,  35,  25,  25,  30 },
	{ 100, 100, 110, 110, 110,     100, 100, 105, 105, 100 },   
	{   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
	SIZE_MEDIUM, SEX_FEMALE, 1
	},
// race 20 ^
	{
	"centaur",    "Centau",
    { 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 130, 135 },
	3, { "endurance", "kick", "lore" },{1, 5, 10},{90, 80, 60},
	{  35,  30,  35,  15,  30,      20,  25,  40,  20,  25 },
	{ 105, 110, 125,  75, 110,     100, 105, 110, 100, 105 },   
	{   4,   4,   5,   3,   4,       4,   4,   4,   4,   4 },
	SIZE_LARGE, SEX_BOTH, 1
	},

	{
	"khan",    "Khan  ",
    { 130, 125, 130, 130, 125, 125, 130, 125, 120, 130, 130, 130, 130, 130, 130 },
	3, { "hunt", "hide", "tiger" },{40, 5, 1},{60, 90, 50 },
	{  25,  20,  20,  25,  25,      20,  20,  25,  25,  20 },
	{ 105, 120, 120, 115,  95,      90, 100, 105, 105, 100 },   
	{   4,   5,   5,   4,   3,       3,   4,   4,   4,   4 },
	SIZE_SMALL, SEX_MALE, 1
	},

        {
	"nymph",    "Nymph ",
    { 135, 130, 130, 125, 135, 135, 135, 130, 130, 130, 130, 130, 130, 120, 130 },
	3, { "faerie fire", "charm person", "tree golem" }, {10,35,70}, {85,50,60},
	{  40,  60,  35,  55,  40,      25,  25,  40,  20,  60 },
	{  85,  85, 100,  90, 100,     120, 115, 110, 120,  95 },   
	{   3,   3,   4,   4,   4,       5,   5,   4,   5,   3 },
	SIZE_MEDIUM, SEX_FEMALE, 1
	},

	{
	"orc",      "Orc   ",
    { 125, 130, 130, 130, 125, 130, 125, 130, 130, 130, 130, 130, 130, 130, 130 },
	3, { "axe", "frenzy", "stone skin" }, {10,25,30}, {80,75,80},
	{  50,  50,  35,  50,  35,      25,  25,  25,  10,  20 },
	{ 115, 110, 110,  100, 110,     105, 90, 110,  90,  95 },   
	{   5,   4,   4,   4,   4,       4,   4,   4,   3,   4 },
	SIZE_MEDIUM, SEX_BOTH, 1
	},

	{
	"satyr",    "Satyr ",
    { 130, 125, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 125, 130 },
	4,{"colour spray","sleep","charm person","cure serious"},{10,5,15,20},{60,60,60,60},
	{  15,  20,  30,  20,  40,      30,  20,  20,  25,  30 },
	{  95, 100, 105,  90, 115,     110, 115,  90, 120, 110 },   
	{   3,   4,   4,   3,   5,       4,   5,   3,   5,   4 },
	SIZE_MEDIUM, SEX_BOTH, 1
	},
// race 25 ^
        {
	"zombie",    "Zombie",
    { 125, 135, 125, 125, 125, 130, 135, 135, 135, 130, 130, 130, 130, 135, 120 },
	3, { "bite", "giant strength", "animate dead" }, {10,35,70}, {75,65,50},
	{  40,  45,  35,  40,  60,      25,  35,  25,  60,  60 },
	{ 110, 120,  95,  85,  90,     120, 100, 115,  85,  95 },   
	{   4,   5,   4,   3,   4,       5,   4,   4,   3,   4 },
	SIZE_MEDIUM, SEX_BOTH, 1
	},     
//r2	
//    War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec
	{
	"cyclops",    "Cyclop",
    { 145, 150, 155, 155, 145, 145, 145, 150, 150, 145, 155, 155, 150, 145, 155 },
	4, { "bash", "enhanced damage", "curse", "lightning bolt" },{5,30,60,20},{80,50,90,60},
	{  60,  60,  55,  20,  20,      20,  25,  35,  25,  30 },
	{ 130, 120, 115, 100, 100,     100, 105, 115,  95, 110 },   
	{   5,   4,   4,   4,   4,       4,   4,   4,   3,   4 },
	SIZE_GIANT, SEX_BOTH, 2
	},

	{
	"dunedain",    "Dunedn",
    { 150, 145, 150, 150, 150, 150, 150, 145, 145, 150, 145, 150, 145, 145, 150 },
	3, { "hunt", "sustenance", "lore" },{55, 10, 1},{75, 50, 60},
	{  30,  30,  30,  30,  30,      30,  30,  30,  30,  30 },
	{ 110, 110, 110, 110, 110,     110, 110, 110, 110, 110 },   
	{   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
	SIZE_MEDIUM, SEX_BOTH, 2
	},

	{
	"highelf",    "Hi Elf",
    { 150, 150, 145, 145, 150, 150, 150, 150, 150, 145, 140, 145, 150, 140, 145 },
	4, { "pacify", "minor group heal", "rescue", "sanctuary" },{50, 40, 5, 80},{60, 60, 50, 33},
	{  30,  25,  25,  40,  40,      30,  25,  30,  50,  30 },
	{ 100, 100, 105, 110, 115,     120, 115, 110, 110, 110 },   
	{   3,   3,   3,   4,   5,       5,   5,   4,   4,   4 },
	SIZE_MEDIUM, SEX_BOTH, 2
	},

	{
	"mantis",    "Mantis",
    { 140, 145, 150, 155, 140, 140, 150, 145, 140, 140, 150, 155, 145, 145, 155 },
	5, { "venom bite", "kick", "chop", "kung fu", "dual wield" }, {1,10,15,35,50}, {60,90,88,85,75},
	{  20,  15,  20,  45,  40,      30,  15,  20,  15,  30 },
	{ 110, 115, 105, 120, 120,     100, 115, 125,  90, 110 },   
	{   4,   4,   3,   5,   5,       3,   4,   5,   3,   4 },
	SIZE_MEDIUM, SEX_NEUTRAL, 2
	},
// race 30 ^	
	{
	"martian",    "Martan",
    { 150, 150, 150, 145, 150, 150, 150, 150, 150, 145, 150, 145, 140, 150, 155 },
	5, { "gun", "unjam", "farsight", "ray of truth", "aim" }, {10,15,25,40,70}, {90,60,85,85,75},
	{  30,  20,  15,  15,  20,      50,  65,  80,  15,  15 },
	{ 110, 100, 115, 105, 100,     130, 125, 120,  75, 115 },   
	{   4,   3,   4,   4,   3,       5,   5,   5,   3,   4 },
	SIZE_MEDIUM, SEX_NEUTRAL, 2
	},          

	{
	"skaven",   "Skaven",
    { 155, 140, 150, 150, 150, 155, 155, 140, 145, 150, 150, 145, 145, 145, 155 },
	5, {"necrosis", "steal", "feint", "envenom", "sneak"},{30,25,2,10,1},{45,50,90,60,100},
	{  20,  20,  15,  65,  50,      30,  15,  15,  15,  80 },
	{ 100, 100, 115, 125, 130,     110, 115, 105,  75, 120 },   
	{   3,   3,   4,   5,   5,       4,   4,   4,   3,   5 },
	SIZE_MEDIUM, SEX_BOTH, 2
	},
//r3
//    War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec
        { 
	"draconian",   "Dracon",
    { 160, 170, 170, 165, 165, 160, 165, 170, 165, 170, 170, 165, 165, 160, 170 },
	3, {"regeneration", "fireball", "haste"},{20,35,60},{75,85,95},
	{  50,  10,  50,  50,  40,      45,  40,  20,  60,  60 },
	{ 125, 120, 120, 115, 105,     120, 120, 110, 100, 100 },   
	{   5,   4,   4,   4,   4,       4,   4,   4,   3,   4 },
	SIZE_LARGE, SEX_MALE, 3
	},
      
        {
	"drider",    "Drider",
    { 165, 165, 170, 175, 165, 170, 170, 160, 160, 165, 170, 175, 170, 165, 175 },
	5, { "venom bite", "faerie fire", "net", "second attack", "third attack" },
	   {15,30,50,65,80}, {95,90,90,55,50},
	{  20,  50,  40,  40,  45,      10,  60,  50,  60,  50 },
	{ 110, 115, 120, 110, 135,     110, 100, 115, 100, 120 },   
	{   4,   4,   4,   4,   5,       4,   3,   4,   4,   4 },
	SIZE_HUGE, SEX_BOTH, 3
	},

        {
	"phreak",    "Phreak",
    { 170, 170, 165, 165, 170, 170, 170, 170, 170, 170, 165, 165, 170, 170, 165 },
	5, { "change sex", "giant strength", "plague", "fear", "epidemic" }, {10,20,35,50,75}, {90,90,85,85,80},
	{  10,  10,  10,  10,  10,      10,  10,  10,  10,  10 },
	{ 120, 120, 120, 120, 120,     120, 120, 120, 120, 120 },   
	{   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
	SIZE_MEDIUM, SEX_BOTH, 3
	},
// race 35 ^
        {    
	"sprite",    "Sprite",
    { 175, 165, 170, 160, 175, 175, 175, 165, 165, 170, 170, 160, 170, 165, 170 },
	5, {"detect astral", "dodge", "goodberry", "teleport", "astral projection"},{5,10,25,45,65},{95,85,90,90,85},
	{  30,  10,  40,  10,  30,      30,  30,  30,  45,  45 },
	{  90, 105, 115, 120, 125,     125, 115, 110, 125, 130 },   
	{   2,   3,   4,   4,   5,       5,   4,   3,   5,   5 },
	SIZE_SMALL, SEX_BOTH, 3
	},

        { 
	"vampil",   "Vampil",
    { 170, 165, 170, 165, 170, 170, 175, 160, 165, 170, 170, 170, 165, 170, 160 },
	4, {"sneak", "vampiric bite", "charm person", "energy drain"},{20,35,50,70},{90,85,75,60},
	{  50,  10,  30,  50,  40,      20,  40,  10,  50,  50 },
        { 115, 125,  90, 115, 125,     120, 110, 125, 120, 105 },   
	{   4,   5,   2,   4,   5,       4,   4,   5,   4,   3 },
	SIZE_MEDIUM, SEX_BOTH, 3
	},
//4
//          War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec
        {
          "cyborg", "Cyborg",
          { 185, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 190, 185, 195, 190 },
          5,{"shocking grasp","electrocution","1000-yard stare","quick draw","enhanced damage"},
          {25, 85, 10, 50, 15}, {100, 50, 75, 60, 60},
          {  60,  60,  20,  35,  35,      50,  30,  60,  30,  20 },
          { 120, 120,  90, 115, 115,     110, 100, 120, 100, 110 },
          {   5,   5,   3,   4,   4,       4,   3,   5,   3,   4 },
          SIZE_MEDIUM, SEX_BOTH, 4
        },
        
        {
          "mermaid", "Mermad",
          { 190, 190, 180, 185, 190, 190, 190, 190, 190, 190, 180, 185, 190, 180, 190 },
          5, {"swimming", "monsoon", "create spring", "tempest", "major group heal"},
          {1, 40, 10, 85, 55}, {100, 80, 75, 70, 60},
          {  45,  30,  45,  40,  45,      30,  35,  35,  55,  40 },
          { 105, 110, 120, 120, 120,     125, 135, 110, 135, 120 },
          {   3,   3,   4,   4,   4,       4,   5,   4,   5,   4 },
          SIZE_MEDIUM, SEX_FEMALE, 4
        },
        
        {
          "minotaur", "Minos ",
          { 180, 190, 190, 190, 185, 190, 190, 190, 190, 190, 185, 190, 185, 185, 190 },
          5, {"axe", "beheading", "hunt", "bash", "locate object"},
          {1, 66, 20, 52, 84}, {95, 50, 75, 70, 50},
          {  65,  55,  45,  30,  45,      45,  60,  55,  30,  20 },
          { 125, 135, 120, 110, 125,     110, 120, 115, 110, 120 },
          {   5,   5,   4,   3,   4,       4,   4,   4,   3,   4 },
          SIZE_HUGE, SEX_BOTH, 4
        },
// race 40 ^        
        {
          "werewolf", "Wrwolf",
          { 185, 190, 190, 190, 185, 190, 190, 190, 185, 195, 195, 190, 190, 185, 190 },
          4, {"bite", "rage", "shadowclaw", "maul"},
          {1, 50, 50, 35}, {90, 80, 80, 50},
          {  70,  60,  50,  75,  70,      40,  45,  30,  30,  30 },
          { 130, 120, 130, 135, 130,     105, 110, 100, 110, 110 },
          {   5,   5,   5,   5,   5,       3,   3,   3,   3,   3 },
          SIZE_LARGE, SEX_BOTH, 4
        },
        
        {
          "wisp", "Wisp  ",
          { 190, 190, 185, 185, 195, 190, 195, 190, 190, 190, 185, 180, 190, 190, 185 },
          5, {"confusion", "faerie fog", "colour spray", "dodge", "immolation"},
          {80, 1, 1, 15, 60}, {65, 100, 100, 95, 80},
          {  20,  10,  55,  55,  50,      70,  50,  40,  60,  65 },
          { 100,  90, 135, 135, 130,     130, 110, 100, 120, 135 },
          {   3,   2,   5,   5,   5,       5,   3,   3,   4,   5 },
          SIZE_TINY, SEX_NEUTRAL, 4
        },
//5        
//          War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec
        {
          "doppelganger", "Doppel",
          { 210, 210, 210, 210, 210, 210, 210, 205, 210, 210, 210, 200, 210, 210, 210 },
          2, {"change sex", "mimic" },
          {1, 5}, {100, 85},
          {  20,  20,  20,  20,  20,      20,  20,  20,  20,  20 },
          { 100, 100, 100, 100, 100,     100, 100, 100, 100, 100 },
          {   0,   0,   0,   0,   0,       0,   0,   0,   0,   0 },
          SIZE_MEDIUM, SEX_NEUTRAL, 5
        },
        
        {
          "harpy", "Harpy ",
          { 215, 215, 215, 215, 215, 215, 215, 215, 215, 215, 215, 215, 215, 210, 215 },
          5, {"curse", "plague", "venom bite", "maul", "giant feller"},
          {20, 10, 1, 60, 40}, {95, 75, 90, 85, 80},
          {  85,  80,  85,  80,  70,      35,  50,  60,  20,  35 },
          { 135, 130, 135, 140, 130,     115, 115, 110, 100, 110 },
          {   5,   4,   5,   5,   4,       4,   4,   3,   3,   3 },
          SIZE_MEDIUM, SEX_FEMALE, 5
        },
        
        {
          "naga", "Naga  ",
          { 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210 },
          5, {"spit", "regeneration", "lore", "extra attack", "arcane lore"},
          {10, 1, 30, 80, 30}, {95, 90, 70, 95, 70},
          {  75,  45,  40,  50,  75,      55,  70,  45,  35,  35 },
          { 120, 125, 120, 130, 135,     120, 130, 125, 115, 115 },
          {   4,   4,   4,   4,   5,       4,   4,   4,   4,   3 },
          SIZE_LARGE, SEX_BOTH, 5
        },
// race 45 ^        
        {
          "treant", "Treant",
          { 205, 210, 210, 210, 205, 205, 205, 205, 205, 205, 210, 210, 210, 200, 210 },
          5, {"woodland combat", "entrapment", "choke hold", "soothe", "goodberry"},
          {5, 75, 35, 55, 10}, {80, 80, 85, 90, 70},
          {  70,  75,  40,  55,  60,      45,  50,  75,  60,  45 },
          { 140, 140, 120,  110, 115,     110, 130, 130, 110, 120 },
          {   5,   5,   4,   3,   3,       3,   5,   5,   3,   4 },
          SIZE_GIANT, SEX_MALE, 5
        },
        
        {
          "wraith", "Wraith",
          { 210, 210, 205, 205, 215, 210, 210, 210, 210, 210, 210, 205, 210, 215, 200 },
          5, {"invis", "energy drain", "fear", "deaths door", "intimidation"},
          {1, 90, 20, 1, 45}, {100, 80, 85, 75, 80},
          {  70,  40,  30,  75,  90,      95,  60,  55,  70,  40 },
          { 115, 120, 110, 135, 135,     135, 125, 120, 110, 110 },
          {   4,   4,   3,   5,   5,       5,   4,   4,   3,   3 },
          SIZE_MEDIUM, SEX_BOTH, 5
        },
//6       
//          War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec
        { 
          "chrysalies",   "Chrysl",
          { 235, 225, 220, 225, 235, 235, 230, 225, 230, 225, 225, 225, 230, 220, 225 },
          5, { "pass without trace", "faerie fog", "sanctuary", "sticks to snakes", "avoidance" },
          { 1, 10, 40, 52, 58}, {100, 100, 80, 70, 90},
          {  55,  25,  65,  65,  65,      30,  50,  25,  60,  60 },
          { 105, 115, 140, 140, 145,     130, 135, 125, 130, 135 },   
          {   2,   3,   5,   5,   5,       4,   4,   4,   4,   4 },
          SIZE_TINY, SEX_BOTH, 6
        },
                 
        { 
          "frost-giant",   "FrostG",
          { 225, 230, 225, 230, 225, 225, 225, 230, 230, 230, 225, 230, 230, 230, 230 },
          5, { "chill touch", "hailstorm", "frost breath", "control weather", "absolute zero" },
          {1,40,60,70,80},{100,95,90,85,80},
          {  60,  70,  45,  50,  50,      45,  50,  55,  50,  50 },
          { 140, 140, 125, 125, 125,     125, 130, 135, 120, 130 },   
          {   5,   5,   4,   4,   4,       3,   4,   4,   3,   4 },
          SIZE_GIANT, SEX_BOTH, 6
        },

        { 
          "gholam",   "Gholam",
          { 230, 225, 230, 230, 230, 230, 230, 215, 225, 235, 220, 230, 225, 220, 235 },
          4, { "witch hunting", "hunt", "stalk", "entrapment"},
          {1,5,10,20},{100,95,90,80},
          {  85,  65,  10,  40,  35,      35,  60,  35,  80,  30 },
          { 135, 115, 130, 140, 135,     135, 120, 135, 130, 130 },   
          {   4,   3,   4,   5,   4,       4,   4,   4,   4,   4 },
          SIZE_MEDIUM, SEX_BOTH, 6
        },
        
// race 50 ^
        { 
          "illithid",   "Illith",
          { 230, 225, 230, 225, 230, 230, 230, 230, 220, 230, 225, 220, 230, 230, 225 },
          4, { "feeblemind", "mindflay", "charm", "confusion" },
          { 20, 40, 60, 80}, {100, 100, 80, 75},
          {  50,  60,  60,  70,  70,      95,  80,  70,  70,  50 },
          { 120, 120, 125, 125, 125,     140, 140, 130, 120, 120 },   
          {   3,   4,   4,   4,   4,       5,   5,   4,   4,   3 },
          SIZE_MEDIUM, SEX_BOTH, 6
        },

        { 
          "vampire",   "Vampir",
          { 230, 225, 230, 230, 230, 230, 235, 220, 225, 230, 235, 230, 225, 230, 220 },
          5, { "drain life", "vampiric bite", "sneak", "charm person", "shroud of darkness"},
          {1,1,10,40,60},{100,100,95,90,100},
          {  65,  25,  45,  65,  55,      35,  55,  25,  65,  65 },
          { 130, 140, 100, 130, 140,     135, 125, 145, 135, 120 },   
          {   4,   4,   3,   4,   4,       4,   4,   5,   4,   4 },
          SIZE_MEDIUM, SEX_BOTH, 6
        },

//r7              

        //  War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec
        { 
          "ahazu",   "Ahazu ",
          { 240, 250, 255, 255, 240, 245, 250, 250, 250, 250, 255, 255, 250, 250, 255 },
          3, { "shan-ya", "dual wield", "extra attack" },
          {1, 10, 50}, {100,95,90},
          {  60,  55,  55,  65,  70,      55,  55,  50,  55,  55 },
          { 140, 135, 135, 145, 150,     125, 125, 130, 125, 135 },   
          {   4,   4,   4,   5,   5,       3,   3,   4,   4,   4 },
          SIZE_MEDIUM, SEX_BOTH, 7
        },

        { 
          "djinn",   "Djinn ",
          { 250, 250, 245, 245, 250, 250, 250, 250, 250, 250, 245, 245, 250, 250, 245 },
          5, { "burning hands", "plague", "flamestrike", "fire breath", "immolation" },
          {1, 10, 25, 55, 75}, {100, 100, 100, 90, 80},
          {  65,  75,  50,  50,  55,      60,  55,  60,  50,  55 },
          { 145, 140, 130, 130, 130,     130, 135, 140, 130, 135 },   
          {   5,   4,   4,   4,   4,       4,   4,   4,   3,   4 },
          SIZE_LARGE, SEX_BOTH, 7
        },

        { 
          "dryad",   "Dryad ",
          { 250, 255, 245, 250, 255, 250, 250, 255, 255, 245, 245, 250, 250, 240, 250 },
          5, { "cure critical", "woodland combat", "gate", "hide", "charm person" },
          {20, 35, 50, 55, 70}, {85, 90, 80, 60, 85},
          {  50,  60,  50,  60,  65,      85,  80,  70,  65,  65 },
          { 130, 140, 125, 130, 135,     135, 135, 130, 140, 130 },   
          {   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
          SIZE_MEDIUM, SEX_BOTH, 7
        },

// race 55 ^

        { 
          "gargoyle",   "Gargoy",
          { 245, 250, 250, 250, 250, 245, 250, 250, 245, 250, 250, 250, 250, 250, 250 },
          4, { "stone skin", "rage", "hurl", "bash" },
          {1, 15, 40, 65}, {100, 85, 90, 70},
          {  90,  80,  15,  45,  45,      40,  65,  40,  90,  40 },
          { 140, 125, 130, 140, 145,     140, 125, 140, 135, 130 },   
          {   4,   4,   4,   4,   5,       4,   3,   4,   4,   4 },
          SIZE_LARGE, SEX_BOTH, 7
        },

        { 
          "lich",   "Lich  ",
          { 250, 250, 245, 240, 250, 250, 250, 250, 250, 250, 255, 245, 250, 255, 235 },
          4, { "necrosis", "decompose", "mana burn", "iron maiden"},
          {5,15,25,65},{80,75,70,50},
          {  55,  40,  25,  50,  50,      90,  75,  60,  55,  50 },
          { 135, 130, 130, 130, 130,     150, 150, 140, 130, 125 },   
          {   4,   4,   3,   4,   4,       5,   5,   4,   4,   3 },
          SIZE_MEDIUM, SEX_BOTH, 7
        },        

//r8        
         /* War, Thf, Cle, Mag, Gla, Sam, Pal, Asn, Nin, Mnk, Tem, Ilu, Gun, Rng, Nec */

        { 
          "android",   "Androi",
          { 265, 270, 270, 270, 270, 265, 270, 270, 270, 270, 270, 270, 270, 270, 270 },
          4, { "shocking grasp", "lightning bolt", "chain lightning", "electrocution" },
          {10, 20, 30, 40}, {100, 95, 95, 75},
          {  70,  70,  60,  60,  70,      60,  50,  60,  50,  50 },
          { 135, 135, 130, 130, 135,     130, 125, 130, 125, 125 },   
          {   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
          SIZE_MEDIUM, SEX_BOTH, 8
        },

        { 
          "naiad",   "Naiad ",
          { 275, 270, 260, 270, 275, 270, 265, 270, 270, 265, 265, 270, 270, 260, 270 },
          5, { "swimming", "charm person", "monsoon", "water elemental", "heal" },
          {1, 20, 40, 50, 60}, {100, 100, 90, 80, 70},
          {  40,  55,  50,  60,  50,      75,  65,  65,  60,  55 },
          { 125, 135, 135, 145, 140,     145, 150, 135, 150, 145 },   
          {   2,   4,   4,   4,   4,       4,   5,   4,   5,   4 },
          SIZE_SMALL, SEX_FEMALE, 8
        },
        
        { 
          "phantom",   "Phantm",
          { 270, 265, 270, 270, 270, 275, 275, 265, 260, 270, 270, 255, 270, 270, 270 },
          5, { "invis", "shadowclaw", "shadowsoul", "basic apparition", "phantasmal image" },
          {1, 10, 20, 40, 50}, {100, 95, 95, 90, 80},
          {  45,  50,  50,  65,  65,      65,  65,  65,  45,  60 },
          { 135, 135, 135, 150, 150,     145, 140, 140, 135, 140 },   
          {   3,   4,   4,   5,   5,       4,   4,   4,   3,   4 },
          SIZE_MEDIUM, SEX_BOTH, 8
        },

        { 
          "tengu",   "Tengu ",
          { 260, 270, 270, 265, 265, 265, 265, 265, 270, 270, 270, 265, 270, 260, 265 },
          5, { "sword", "dual sword", "retribution", "mana shield", "leadership" },
          {1, 10, 20, 40, 50}, {100, 95, 90, 90, 90},
          {  45,  55,  55,  60,  50,      60,  60,  65,  65,  60 },
          { 130, 135, 135, 140, 145,     150, 150, 135, 145, 140 },   
          {   3,   3,   4,   4,   4,       5,   5,   4,   4,   4 },
          SIZE_MEDIUM, SEX_BOTH, 8
        },

        { 
          "titan",   "Titan ",
          { 260, 275, 270, 275, 260, 265, 265, 270, 275, 265, 270, 275, 265, 270, 275 },
          5, { "bash", "twohand weapons", "uppercut", "bloodbath", "brutal damage"},
          {10, 20, 30, 40, 50}, {100, 90, 90, 80, 60},
          {  75,  65,  50,  60,  60,      40,  40,  65,  60,  60 },
          { 155, 155, 140, 140, 140,     130, 130, 145, 130, 140 },   
          {   5,   5,   4,   4,   4,       3,   3,   4,   4,   4 },
          SIZE_GIANT, SEX_BOTH, 8
        },
        
        { 
          "voadkin",   "Voadki",
          { 270, 265, 270, 270, 270, 270, 270, 265, 265, 270, 265, 270, 265, 260, 275 },
          5, { "bow", "fledging", "woodlandcombat", "hunt", "beast mastery" },
          { 10, 10, 20, 30, 40 }, { 95, 95, 90, 80, 80 },
          {  55,  55,  55,  55,  55,      55,  55,  55,  55,  55 },
          { 140, 140, 140, 140, 145,     140, 145, 140, 140, 140 },   
          {   4,   4,   4,   4,   4,       4,   4,   4,   4,   4 },
          SIZE_LARGE, SEX_BOTH, 8
        },
   
        { 
          "behemoth",   "Behemo",
          { 305, 305, 315, 315, 305, 310, 310, 305, 305, 315, 310, 315, 305, 305, 315 },
          4, { "razor claws", "berserk", "brutal damage", "hunt" },
          {1,10,60,80},{100,100,100,80},
          { 120, 105,  95,  90,  90,      20,  30,  50,  50,  50 },
          { 160, 155, 155, 150, 150,     140, 140, 150, 150, 150 },  
          {   0,   0,   0,   0,   0,       0,   0,   0,   0,   0 },
          SIZE_HUGE, SEX_BOTH, 10
        }

};

/* special race_types and pc_race_types for morphing --Bobble */
struct race_type morph_race_table[] = 
{
	{ 
	"naga serpent",        TRUE, 
	{},      {AFF_INFRARED,AFF_BREATHE_WATER,AFF_DETECT_MAGIC},      {},
	{},      {RES_PIERCE,RES_DROWNING,RES_COLD}, {VULN_LIGHTNING},
	    {A,B,H,R,Y,cc},    {A,B,D,E,F,G,I,J,K,L,N,Q,U,V,X}
	},

	{ 
	"naga human",        TRUE, 
	{},      {},      {},
	{},      {RES_HOLY,RES_NEGATIVE}, {VULN_LIGHTNING},
	    {A,B,H,M},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{ 
	"wolfman",        TRUE, 
	{},      {},      {},
	{},      {},      {},
	{A,H,M,V},    {A,B,C,D,E,F,G,H,I,J,K}
	},

	{
	  NULL, TRUE, {}, {}, {}, {}, {}, {}, {}, {}
	}
};

struct pc_race_type morph_pc_race_table[] = 
{
    {
	"naga_serpent", "",
	{},
	4, {"spit", "regeneration", "lore", "arcane lore"},
	{10, 1, 30, 30}, {95, 90, 70, 70},
	{ },
	{ },
	{ },
	SIZE_LARGE, SEX_BOTH, 5
    },

    {
	"naga_human", "",
	{},
	3, {"lore", "extra attack", "arcane lore"},
	{30, 80, 30}, {70, 95, 70},
	{ },
	{ },
	{ },
	SIZE_MEDIUM, SEX_BOTH, 5
    },

    {
	"wolfman", "",
	{},
	2, {"rage", "shadowclaw"},
	{50, 50}, {80, 80},
	{ },
	{ },
	{ },
	SIZE_MEDIUM, SEX_BOTH, 4
    }
};


/*
 * Class table.
 */
const   struct  class_type  class_table [MAX_CLASS] =
{

/* Added the struct info from merc.h to make it easier to read
   the below information - Astark 12-27-12 */
//    char *  name;           /* the full name of the class */
//    char    who_name    [4];    /* Three-letter name for 'who'  */
//    sh_int  attr_prime;     /* Prime attribute      */
//    sh_int  attr_second[2]; /* Secondary attributes  */
//    sh_int  stat_weights[MAX_STATS]; /* weights for default roll assignment */
//    sh_int  weapon;         /* First weapon         */
//    sh_int  guild[MAX_GUILD];   /* Vnum of guild rooms      */
//    sh_int  skill_adept;        /* Maximum skill level      */
//    sh_int  attack_factor;      /* replace the old thac0 values --Bobble */
//    sh_int  defense_factor;
//    sh_int  hp_gain;
//    sh_int  mana_gain;          /* Class gains mana on level    */
//    sh_int  move_gain;
//    char *  base_group;     /* base skills gained       */
//    char *  default_group;      /* default skills gained    */

	{
	"warrior", "War",  STAT_STR, {STAT_CON, STAT_DIS},
        {110, 115, 115, 110, 110, 80, 80, 100, 90, 100},
	OBJ_VNUM_SCHOOL_SWORD, { 9633, 10344 },
	75,  120, 110,  115, 60, 110,
	"warrior basics", "warrior default"
	},

	{
	"thief", "Thi",  STAT_DEX, {STAT_AGI, STAT_LUC},
        {100, 110, 110, 120, 120, 85, 85, 100, 80, 100},
	OBJ_VNUM_SCHOOL_DAGGER, { 9639, 10341 },
	75, 100, 100,  110, 70, 110,
	"thief basics", "thief default"
	},

	{
	"cleric", "Cle",  STAT_WIS, {STAT_CHA, STAT_DIS},
        {80, 95, 95, 80, 100, 110, 115, 100, 110, 100},
	OBJ_VNUM_SCHOOL_MACE, { 9619, 10282 },
	75, 80, 100,  95, 115, 70,
	"cleric basics", "cleric default"
	},

	{
	"mage", "Mag",  STAT_INT, {STAT_WIS, STAT_AGI},
        {60, 90, 90, 80, 90, 120, 120, 100, 110, 100},
	OBJ_VNUM_SCHOOL_POLEARM, { 9618, 10300 },
	75, 70, 80,  90,  120, 70,
	"mage basics", "mage default"
	},

	{
	"gladiator", "Gla",  STAT_VIT, {STAT_STR, STAT_AGI},
        {110, 120, 120, 120, 110, 60, 60, 100, 80, 100},
	OBJ_VNUM_SCHOOL_AXE, { 9633, 10344 },
	75, 130, 100,  120, 40, 120,
	"gladiator basics", "gladiator default"
	},

	{
	"samurai", "Sam",  STAT_CON, {STAT_STR, STAT_DIS},
        {110, 110, 110, 110, 110, 85, 85, 100, 80, 100},
	OBJ_VNUM_SCHOOL_SWORD, { 9633, 10344 },
	75, 110, 110,  110, 70, 110,
	"samurai basics", "samurai default"
	},

	{
	"paladin", "Pal",  STAT_CHA, {STAT_WIS, STAT_STR},
        {110, 105, 105, 100, 110, 95, 95, 100, 90, 100},
	OBJ_VNUM_SCHOOL_MACE, { 9633, 10344 },
        75, 100, 110,  105, 90, 100,
	"paladin basics", "paladin default"
	},

	{
	"assassin", "Asn",  STAT_INT, {STAT_DEX, STAT_AGI},
        {100, 110, 110, 120, 120, 85, 85, 100, 80, 100},
	OBJ_VNUM_SCHOOL_DAGGER, { 9639, 10341 },
	75, 120, 100,  110, 70, 110,
	"assassin basics", "assassin default"
	},

	{
	"ninja", "Nin",  STAT_AGI, {STAT_DIS, STAT_INT},
        {100, 110, 110, 120, 120, 85, 85, 100, 80, 100},
	OBJ_VNUM_SCHOOL_SWORD, { 9639, 10341 },
	75, 110, 110,  110, 70, 110,
	"ninja basics", "ninja default"
	},

	{
	"monk", "Mon",  STAT_DIS, {STAT_WIS, STAT_AGI},
        {105, 100, 100, 105, 105, 100, 100, 100, 80, 100},
	OBJ_VNUM_SCHOOL_POLEARM, { 9619, 10282 },
	75, 100, 100,  100, 100, 100,
	"monk basics", "monk default"
	},

	{
	"templar", "Tem",  STAT_WIS, {STAT_CHA, STAT_INT},
        {80, 100, 100, 90, 100, 110, 105, 100, 110, 100},
	OBJ_VNUM_SCHOOL_FLAIL, { 9619, 10282 },
	75, 80, 90,  100, 105, 85,
	"templar basics", "templar default"
	},

	{
	"illusionist", "Ilu",  STAT_LUC, {STAT_WIS, STAT_DEX},
        {60, 95, 95, 95, 100, 110, 110, 100, 100, 110},
	OBJ_VNUM_SCHOOL_DAGGER, { 9618, 10300 },
	75, 70, 90,  95,  110, 85,
	"illusionist basics", "illusionist default"
	},

	{
	"gunslinger", "Gun",  STAT_LUC, {STAT_DEX, STAT_CON},
        {110, 115, 115, 110, 110, 80, 80, 100, 110, 110},
	OBJ_VNUM_SCHOOL_GUN, { 9633, 10328 },
	75, 130, 90,  115, 60, 110,
	"gunslinger basics", "gunslinger default"
	},
	
	{
	"ranger", "Ran",  STAT_WIS, {STAT_VIT, STAT_STR},
        {100, 105, 105, 110, 110, 90, 90, 100, 110, 100},
	OBJ_VNUM_SCHOOL_AXE, { 9633, 10289 },
	75, 100, 110,  105, 80, 110,
	"ranger basics", "ranger default"
	},

	{
	"necromancer", "Nec",  STAT_CHA, {STAT_WIS, STAT_INT},
        {60, 95, 95, 80, 90, 115, 115, 90, 120, 100},
	OBJ_VNUM_SCHOOL_POLEARM, { 9618, 10300 },
	75, 70, 90,  95,  115, 70,
	"necromancer basics", "necromancer default"
	}
};



/*
 * Titles.
 */
char *  const           title_table [MAX_CLASS][23] =
{
	{
	"Citizen",
	"Belligerent Newbie",
	"Recruit",
	"Grunt",
	"Squire",
	"Soldier",
	"Warrior",
	"Veteran",
	"Swordmaster",
	"Cavalier",
	"Knight",
	"Knight of the Sword",
	"Knight of the Rose",
	"Knight of the Round Table",
	"Dragon Slayer",
	"Demon Slayer",
	"Baron",
	"General",
	"Warlord",
	"Knight Hero",
	"Grand Knight Hero",
	"Demigod of War",
	"Omnipotent"
	},

	{
	"Beggar",
	"Sketchy Newbie",
	"Kleptomaniac",
	"Shoplifter",
	"Pick-Pocket",
	"Cut-Purse",
	"Looter",
	"Robber",
	"Fence",
	"Felon",
	"Pimp",
	"Smuggler",
	"Thief",
	"Burglar",
	"Cat Burglar",
	"Master Thief",
	"Crime Lord",
	"Underworld Kingpin",
	"Godfather",
	"Thief Hero",
	"Grand Thief Hero",
	"Demigod of Wealth",
	"Omnipresent"
	},

	{
	"Citizen",
	"Pious Newbie",
	"Believer",
	"Attendant",
	"Acolyte",
	"Adept",
	"Deacon",
	"Vicar",
	"Exorcist",
	"Priest",
	"High Priest",
	"Bishop",
	"Arch Bishop",
	"Cardinal",
	"Patriach",
	"Elder Patriarch",
	"Pontifex",
	"Saint",
	"Pope",
	"Cleric Hero",
	"Grand Cleric Hero",
	"Demigod of Knowledge",
	"Omniscient"
	},

	{
	"Citizen",
	"Studious Newbie",
	"Sorcerer's Apprentice",
	"Scholar of Magic",
	"Scribe of Magic",
	"Medium",
	"Seer",
	"Savant",
	"Abjurer",
	"Invoker",
	"Enchanter",
	"Conjuror",
	"Sorcerer",
	"Summoner",
	"Warlock",
	"Bender of Space",
	"Bender of Time",
	"Wizard",
	"Arch Wizard",
	"Mage Hero",
	"Grand Mage Hero",
	"Demigod of Magic",
	"Infinite"
	},

	{
	"Slave",
	"Rabid Newbie",
	"Tirone",
	"Bestiarius",
	"Pit Fighter",
	"Retiarius",
	"Matador",
	"Gallius",
	"Samnite",
	"Myrillone",
	"Secutore",
	"Hoplomachius",
	"Freeman",
	"Eque",
	"Charioteer",
	"Veteranus",
	"Lanista",
	"Judicial Champion",
	"Primus Palus",
	"Gladiator Hero",
	"Grand Gladiator Hero",
	"Demigod of Combat",
	"God of Combat"
	},

	{
	"Citizen",
	"Honorable Newbie",
	"Swordpupil",
	"Apprentice Samurai",
	"Foot Soldier",
	"Honor Guard",
	"Elite Guard",
	"Emporer's Guard",
	"Samurai",
	"Samurai Champion",
	"Elite Samurai",
	"Samurai of the Blade",
	"Samurai of the Eye",
	"Samurai of Honor",
	"Master Samurai",
	"Sensei",
	"Samurai Lord",
	"Samurai Overlord",
	"Shogun",
	"Samurai Hero",
	"Grand Samurai Hero",
	"Demigod of Honor",
	"God of Honor"
	},

	{
	"Noble",
	"Self-Righteous Newbie",
	"Devout",
	"Defender",
	"Protector",
	"Guardian",
	"Missionary",
	"Crusader",
	"Temple Guardian",
	"Guardian of the Outer Sanctum",
	"Guardian of the Inner Sanctum",
	"White Knight",
	"Holy Knight",
	"Paladin",
	"Grand Paladin",
	"Paladin Lord",
	"Avatar",
	"Prophet",
	"Chosen One",
	"Paladin Hero",
	"Grand Paladin Hero",
	"Demigod of Combat",
	"God of Combat"
	},

	{
	"Citizen",
	"Antisocial Newbie",
	"Punk",
	"Petty Thug",
	"Knifer",
	"Cut-Throat",
	"Sociopath",
	"Murderer",
	"Cold-Blooded Killer",
	"Stalker",
	"Mercenary",
	"Hitman",
	"Bounty Hunter",
	"Mafioso",
	"Sniper",
	"Terrorist",
	"Spy",
	"Assassin",
	"Royal Assassin",
	"Assassin Hero",
	"Grand Assassin Hero",
	"Demigod of Combat",
	"God of Combat"
	},

	{
	"Citizen",
	"Shadowy Newbie",
	"Pupil",
	"White Belt",
	"Yellow Belt",
	"Green Belt",
	"Teal Belt",
	"Striped Belt",
	"Plaid Belt",
	"Black Belt",
	"Martial Arts Master",
	"Blade Master",
	"Unseen",
	"Unheard",
	"Masked",
	"Ninja",
	"Master Ninja",
	"Rogue Ninja",
	"Shadow Ninja",
	"Ninja Hero",
	"Grand Ninja Hero",
	"Demigod of Shadows",
	"God of Shadows"
	},

	{
	"Citizen",
	"Calm Newbie",
	"Hermit",
	"Scribe",
	"Disciplined",
	"Martial Artist",
	"Master of Body",
	"Master of Mind",
	"Master of Kung Fu",
	"Master of Soul",
	"Monk",
	"Drunken Genii",
	"Master",
	"Yogi",
	"Mahatma",
	"Maharishi",
	"Enlightened",
	"Bohdisattva",
	"Buddha",
	"Monk Hero",
	"Grand Monk Hero",
	"Demigod of Enlightenment",
	"God of Enlightenment"
	},

	{
	"Noble",
	"Stern Newbie",
	"Academy Student",
	"Ensign",
	"Petty Beurocrat",
	"Lieutenant",
	"Beurocrat",
	"Corporal",
	"Detective",
	"Lawyer",
	"Public Defender",
	"District Attorney",
	"Judge",
	"Chief of Police",
	"Politican",
	"Local Official",
	"Mayor",
	"Governor",
	"Prime Minister",
	"Templar Hero",
	"Grand Templar Hero",
	"Demigod of Order",
	"God of Order"
	},

	{
	"Citizen",
	"Devious Newbie",
	"Prankster",
	"Hoaxster",
	"Gypsy",
	"Trickster",
	"Hypnotist",
	"Bunko Artist",
	"Charlatan",
	"Magician",
	"Confidence Artist",
	"Illusionist",
	"Master of Light",
	"Master of Shadow",
	"Master of Sound",
	"Master of Touch",
	"Master Illusionist",
	"Arch Illusionist",
	"Shadow Wizard",
	"Illusionist Hero",
	"Grand Illusionist Hero",
	"Demigod of Deception",
	"God of Deception"
	},

	{
	"Citizen",
	"Trigger-Happy Newbie",
	"Young Gun",
	"Outlaw",
	"Sure Shot",
	"Marksman",
	"Hired Gun",
	"Trail Blazer",
	"Professional",
	"Guerilla",
	"Bandito",
	"Gunslinger",
	"Eagle-eyed",
	"Powder-burned Killa",
	"Sniper",
	"Exterminator",
	"Master Marksman",
	"Liquidator",
	"Black Rider",
	"Gunslinger Hero",
	"Grand Gunslinger Hero",   
	"Baddest Gun there Ever Was",
	"Fastest Draw in the West"
	},
	
	{
	"Citizen",
	"All-Natural Newbie",
	"Sapling",
	"Explorer",
	"Forester",
	"Woodsman",
	"Path Finder",
	"Way Maker",
	"Favored of Diana",
	"Rover",
	"Wanderer",
	"Ranger",
	"Sure-footed",
	"Green",
	"High Protector",
	"Conservationist",
	"Master of the Trails",
	"Stalker",
	"Strider",
	"Ranger Hero",
	"Grand Ranger Hero",   
	"Master of the Forest Dark and Dreadful",
	"Friend of All Animals Great and Small"
	},

	{
	"Citizen",
	"Morbid Newbie",
	"Grave Robber",
	"Tomb Robber",
	"Mortician",
	"Necromancer's Apprentice",
	"Surgeon",
	"Lesser Animator",
	"Dark Communer",
	"Zombiemaster",
	"Animus Mundus",
	"Crafter of Souls",
	"Necrolyte",
	"High Channeler",
	"Wraithlord",
	"Lord of Death",
	"Lord of Life",
	"Necromancer",
	"Necromancer Lord",
	"Necromancer Hero",
	"Lichemaster",
	"Demigod of Creation",
	"God of Life and Death"
	}

};

/*
 * Liquid properties.
 * Used in world.obj.
 */
const   struct  liq_type    liq_table   []  =
{
/*    name          color   proof, full, thirst, food, ssize */
	{ "water",          "clear",    {   0, 1, 10, 0, 16 }   },
	{ "beer",           "amber",    {  12, 1,  8, 1, 12 }   },
	{ "red wine",       "burgundy", {  30, 1,  8, 1,  5 }   },
	{ "ale",            "brown",    {  15, 1,  8, 1, 12 }   },
	{ "dark ale",       "dark",     {  16, 1,  8, 1, 12 }   },

	{ "whisky",         "golden",   { 120, 1,  5, 0,  2 }   },
	{ "lemonade",       "pink",     {   0, 1,  9, 2, 12 }   },
	{ "firebreather",       "boiling",  { 190, 0,  4, 0,  2 }   },
	{ "local specialty",    "clear",    { 151, 1,  3, 0,  2 }   },
	{ "slime mold juice",   "green",    {   0, 2, -8, 1,  2 }   },

	{ "milk",           "white",    {   0, 2,  9, 3, 12 }   },
	{ "tea",            "tan",      {   0, 1,  8, 0,  6 }   },
	{ "coffee",         "black",    {   0, 1,  8, 0,  6 }   },
	{ "blood",          "red",      {   0, 2, -1, 2,  6 }   },
	{ "salt water",     "clear",    {   0, 1, -2, 0,  1 }   },

	{ "coke",           "brown",    {   0, 2,  9, 2, 12 }   }, 
	{ "root beer",      "brown",    {   0, 2,  9, 2, 12 }   },
        { "dr pepper",      "brown",    {   0, 2,  9, 2, 12 }   },
	{ "elvish wine",    "green",    {  35, 2,  8, 1,  5 }   },
	{ "white wine",     "golden",   {  28, 1,  8, 1,  5 }   },
	{ "champagne",      "golden",   {  32, 1,  8, 1,  5 }   },

	{ "mead",           "honey-colored",{  34, 2,  8, 2, 12 }   },
	{ "rose wine",      "pink",     {  26, 1,  8, 1,  5 }   },
	{ "benedictine wine",   "burgundy", {  40, 1,  8, 1,  5 }   },
	{ "vodka",          "clear",    { 130, 1,  5, 0,  2 }   },
	{ "cranberry juice",    "red",      {   0, 1,  9, 2, 12 }   },

	{ "orange juice",       "orange",   {   0, 2,  9, 3, 12 }   }, 
	{ "absinthe",       "green",    { 200, 1,  4, 0,  2 }   },
	{ "brandy",         "golden",   {  80, 1,  5, 0,  4 }   },
	{ "aquavit",        "clear",    { 140, 1,  5, 0,  2 }   },
	{ "schnapps",       "clear",    {  90, 1,  5, 0,  2 }   },

	{ "icewine",        "purple",   {  50, 2,  6, 1,  5 }   },
	{ "amontillado",        "burgundy", {  35, 2,  8, 1,  5 }   },
	{ "sherry",         "red",      {  38, 2,  7, 1,  5 }   },  
	{ "framboise",      "red",      {  50, 1,  7, 1,  5 }   },
	{ "rum",            "amber",    { 151, 1,  4, 0,  2 }   },

	{ "cordial",        "clear",    { 100, 1,  5, 0,  2 }   },
	{ NULL,         NULL,       {   0, 0,  0, 0,  0 }   }
};


/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
 */
#define SLOT(n) n

struct  skill_type  skill_table [MAX_SKILL] =
{

/*
 * Magic spells.
 */

/*
    {
    "skill name"
    { level of skill for each class },
    { points cost (trains) for each class },
    { percent that each class gets it at },
    STAT_1, STAT_2, STAT_3,
    0, TARGET REQUIREMENT, POSITION REQUIREMENT,
    NULL, SLOT #, MANA COST, LAG, DURATION,
    "name used when being cast", "wear off message.", "", NULL
    },
*/

	{
	"reserved",
	{ 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101 }, 
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	0,          TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT( 0),    0,  0, DUR_NONE,
	"",         "",     "", NULL
	},

	{
	"absolute zero",  
	{ 102, 102, 42, 42, 102, 102, 102, 102, 102, 102, 42, 42, 102, 102, 42 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_absolute_zero,    TAR_CHAR_SELF,     POS_FIGHTING,
	&gsn_absolute_zero,           SLOT(734),    60, 20, DUR_SHORT,
	"freezing aura",         "You are no longer freezing.",  "", NULL
	},

	{
	"acid blast",
	{ 102, 102, 102, 20, 102, 102, 102, 102, 102, 102, 60, 25, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_acid_blast,   TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(70),   20, 12, DUR_NONE,
	"acid blast",       "!Acid Blast!","", NULL
	},

	{
	"acid breath",  
	{ 102, 102, 102, 57, 102, 102, 102, 102, 102, 102, 102, 89, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_acid_breath,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(200),   18,    4, DUR_NONE,
	"blast of acid",    "!Acid Breath!",    "", NULL
	},

	{
	"angel smite",
	{ 102, 102, 35, 102, 102, 43, 60, 102, 102, 102, 40, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_angel_smite,      TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(525),     20,      12, DUR_NONE,
	"smite",                "!Angel Smite!",        "", NULL
	},

	{
	"animate dead",  
	{ 102, 102, 33, 22, 102, 102, 102, 102, 102, 102, 84, 102, 102, 102, 20 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_animate_dead, TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(667),    40, 20, DUR_LONG,
	"rage",     "!Animate Dead!","", NULL
	},

	{
	"armor",  
	{ 102, 102,  9,  7, 102, 11, 13, 102, 20, 10,  9,  8, 102, 102, 8 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_armor,        TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL,           SLOT( 1),    5, 12, DUR_NORMAL,
	"",         "You feel less armored.",   "", NULL
	},

	{
	"astarks rejuvenation",  
	{ 102, 102,  94, 102, 102, 102, 102, 102, 102, 102,  96, 102, 102, 102, 102 },
	{   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_astarks_rejuvenation, TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(508),  300,    12, DUR_NONE,
	"",         "!Astark's Rejuvenation!",   "", NULL
	},

	{
	"astral projection",   
	{ 102, 102, 102, 90, 102, 102, 102, 102, 102, 102, 102, 50, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_astral,        TAR_CHAR_SELF, POS_STANDING,
	&gsn_astral,           SLOT(3000),    100, 12, DUR_BRIEF,
	"",         "The tides of the ether wash you back into reality.","", NULL
	},

	{
	"basic apparition",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 17, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_basic_apparition,  TAR_IGNORE, POS_STANDING,
	NULL,           SLOT(2403),   120, 12, DUR_NONE,
	"basic apparition",     "!Basic Apparition!",       "", NULL
	},

	{
	"betray",  
	{ 102, 30, 102, 21, 102, 102, 102, 102, 102, 22, 102,  8, 102, 102, 21 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_betray,   TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(1003), 15, 12, DUR_NORMAL,
	"spell",        "!betray!",   "", NULL
	},

	{
	"bless",  
	{ 102, 102,  4, 102, 102, 102, 102, 102, 102,  8, 12, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_bless,        TAR_OBJ_CHAR_DEF,   POS_STANDING,
	&gsn_bless,           SLOT( 3),    5, 12, DUR_NORMAL,
	"",         "You feel less righteous.", 
	"$p's holy aura fades.", NULL
	},


	{
	"blindness",  
	{ 102, 102, 14, 24, 102, 102, 102, 102, 102, 28, 21, 15, 102, 102, 24 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_blindness,    TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_blindness,     SLOT( 4),    5, 12, DUR_BRIEF,
	"",         "You can see again.",   "", NULL
	},

	{
	"breath of god",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_breath_of_god,        TAR_CHAR_SELF,   POS_STANDING,
	NULL,           SLOT(947),    60, 18, DUR_NONE,
	"",         "",		"", NULL
	},

	{
	"burning hands",  
	{ 102, 102, 102,  4, 102,  8, 102, 102,  6, 102, 12,  4, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_burning_hands,    TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT( 5),   5, 12, DUR_NONE,
	"burning hands",    "!Burning Hands!",  "", NULL
	},

	{
	"call lightning",  
	{ 102, 102, 75, 80, 102, 102, 102, 102, 102, 102, 85, 90, 102, 74, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_call_lightning,   TAR_IGNORE,     POS_FIGHTING,
	&gsn_call_lightning,           SLOT( 6),   15, 9, DUR_NONE,
	"lightning bolt",   "!Call Lightning!", "", NULL
	},

	{
	"call sidekick",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 71, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_call_sidekick, TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(2002),    40, 20, DUR_LONG,
	"",     "!callsidekick!","", NULL
	},

	{
	"calm",  
	{ 102, 102, 12, 102, 102, 102, 102, 102, 102, 20, 22, 38, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_calm,     TAR_IGNORE,     POS_FIGHTING,
	NULL,           SLOT(509),  30, 12, DUR_BRIEF,
	"",         "You have lost your peace of mind.",    "", NULL
	},

	{
	"cancellation",  
	{ 102, 102, 24, 16, 102, 102, 102, 102, 102, 26, 22, 16, 102, 102, 16 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cancellation, TAR_CHAR_NEUTRAL, POS_FIGHTING,
	NULL,           SLOT(507),  20, 12, DUR_NONE,
	""          "!cancellation!",   "", NULL
	},

	{
	"cannibalism",   
	{ 102, 102, 102, 27, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 25 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cannibalism,  TAR_CHAR_SELF,  POS_FIGHTING,
	NULL,           SLOT(671),    5,  2, DUR_NONE,
	"",               "!Cannibalism!", "", NULL
	},

	{
	"cause critical",   
	{ 102, 102, 20, 102, 102, 102, 32, 102, 102, 40, 24, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cause_critical,   TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(63),   20, 12, DUR_NONE,
	"spell",        "!Cause Critical!", "", NULL
	},

	{
	"cause light",  
	{ 102, 102,  5, 102, 102, 102,  8, 102, 102, 10,  6, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cause_light,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(62),   15, 12, DUR_NONE,
	"spell",        "!Cause Light!",    "", NULL
	},

	{
	"cause serious",  
	{ 102, 102, 10, 102, 102, 102, 16, 102, 102, 20, 12, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cause_serious,    TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(64),   17, 12, DUR_NONE,
	"spell",        "!Cause Serious!",  "", NULL
	},

	{   
	"chain lightning", 
	{ 102, 102, 102, 30, 102, 102, 102, 102, 102, 102, 82, 40, 102, 35, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_chain_lightning,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(500),  25, 12, DUR_NONE,
	"lightning",        "!Chain Lightning!",    "", NULL
	}, 

	{
	"change sex",     
	{ 102, 102, 16, 27, 102, 102, 102, 102, 102, 32, 24, 18, 102, 102, 27 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_change_sex,   TAR_CHAR_NEUTRAL, POS_FIGHTING,
	NULL,           SLOT(82),   15, 12, DUR_NORMAL,
	"",         "Your body feels familiar again.",  "", NULL
	},

	{
	"charm person",   
	{ 102, 40, 102, 25, 102, 102, 102, 102, 32, 102, 80,  6, 102, 102, 25 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_charm_person, TAR_VIS_CHAR_OFF, POS_STANDING,
	&gsn_charm_person,  SLOT( 7),    5, 12, DUR_BRIEF,
	"",         "You feel more self-confident.",    "", NULL
	},

	{
	"chill touch",  
	{ 102, 102, 102,  6, 102, 12, 102, 102, 10, 102, 18,  8, 102, 102, 5 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_chill_touch,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT( 8),   10, 12, DUR_NONE,
	"chilling touch",   "You feel less cold.",  "", NULL
	},

	{
	"colour spray",  
	{ 102, 102, 102,  8, 102, 16, 102, 102, 14, 102, 24,  7, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_colour_spray, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(10),   12, 12, DUR_NONE,
	"colour spray",     "!Colour Spray!",   "", NULL
	},

      {
	"confusion",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 40, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_confusion, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_confusion,           SLOT(2040),   25, 18, DUR_BRIEF,
	"",     "You put the pieces of your head back together.",   "", NULL
	},
	 {
        "mass confusion",
        { 102, 102, 102,  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
 	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_mass_confusion, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
        &gsn_mass_confusion,           SLOT(2040),   25, 18, DUR_BRIEF,
        "",     "You put the pieces of your head back together.",   "", NULL
        },
	
	{
	"continual light",  
	{ 102, 102, 15,  6, 102, 41, 102, 102, 102, 24, 12, 102, 102, 102, 6 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_continual_light,  TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(57),    7, 12, DUR_SPECIAL,
	"",         "!Continual Light!",    "", NULL
	},

	{
	"control weather", 
	{ 102, 102, 29, 36, 102, 102, 102, 102, 102, 102, 26, 102, 102, 40, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_control_weather,  TAR_IGNORE,     POS_STANDING,
	&gsn_control_weather,           SLOT(11),   25, 12, DUR_NONE,
	"",         "!Control Weather!",    "", NULL
	},

	{
	"conviction", 
        { 102, 102,  102, 102, 102, 102, 96, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_conviction,   TAR_CHAR_OFFENSIVE,   POS_FIGHTING,
	NULL,           SLOT(593),   125, 18, DUR_NONE,
	"conviction",   "!conviction!", "", NULL
	},

        {
	"create bomb",   
	{ 102, 80, 102, 102, 102, 102, 102, 50, 102, 102, 102, 102, 40, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_create_bomb,    TAR_IGNORE,         POS_STANDING,
	NULL,                 SLOT(537),    30,     12, DUR_NONE,
	"",         "!Create Bomb!",    "", NULL
	},  

	{
	"create food",  
	{ 102, 102,  4,  9, 102, 27, 102, 102, 25,  4,  4, 102, 102, 102, 15 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_create_food,  TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(12),    5, 12, DUR_NONE,
	"",         "!Create Food!",    "", NULL
	},

	{
	"create rose",  
	{ 102, 102, 58, 102, 102, 102, 102, 102, 102, 72, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_create_rose,  TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(511),  30,     12, DUR_NONE,
	"",         "!Create Rose!",    "", NULL
	},  

	{
	"create spring",   
	{ 102, 102, 10, 12, 102, 36, 102, 102, 102, 16, 16, 102, 102, 102, 16 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_create_spring,    TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(80),   20, 12, DUR_NORMAL,
	"",         "!Create Spring!",  "", NULL
	},

	{
	"create water",   
	{ 102, 102,  3,  3, 102, 21, 102, 102, 20,  8,  8, 102, 102, 102, 5 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_create_water, TAR_OBJ_INV,        POS_STANDING,
	NULL,           SLOT(13),    5, 12, DUR_NONE,
	"",         "!Create Water!",   "", NULL
	},

	{
	"cure blindness", 
	{ 102, 102, 12, 102, 102, 39, 102, 102, 102, 15, 16, 102, 102, 14, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cure_blindness,   TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(14),    5, 12, DUR_NONE,
	"",         "!Cure Blindness!", "", NULL
	},

	{
	"cure critical", 
	{ 102, 102, 30, 102, 102, 102, 90, 102, 102, 50, 60, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cure_critical,    TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(15),   20, 12, DUR_NONE,
	"",         "!Cure Critical!",  "", NULL
	},

	{
	"cure disease", 
	{ 102, 102,  4, 102, 102, 102, 31, 102, 102,  5,  6, 102, 102, 32, 20 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cure_disease, TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL,           SLOT(501),  20, 12, DUR_NONE,
	"",         "!Cure Disease!",   "", NULL
	},

	{
	"cure light",  
	{ 102, 102,  1, 102, 102, 102, 20, 102, 102,  4,  5, 102, 102, 102, 5 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cure_light,   TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(16),    2, 8, DUR_NONE,
	"",         "!Cure Light!",     "", NULL
	},

	{
	"cure mortal", 
	{ 102, 102, 70, 102, 102, 102, 102, 102, 102, 102, 90, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cure_mortal,      TAR_CHAR_DEFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(526),      100,     14, DUR_NONE,
	"",                     "!Cure Mortal!",        "", NULL
	},

	{
	"cure mental", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cure_mental,  TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL,           SLOT(45),    15, 12, DUR_NONE,
	"",         "!Cure mental!",    "", NULL
	},

	{
	"cure poison", 
	{ 102, 102,  8, 102, 102, 102, 35, 102, 102, 10, 11, 102, 41, 18, 22 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cure_poison,  TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL,           SLOT(43),    5, 12, DUR_NONE,
	"",         "!Cure Poison!",    "", NULL
	},

	{
	"cure serious", 
	{ 102, 102, 15, 102, 102, 102, 50, 102, 102, 25, 30, 102, 102, 102, 21 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cure_serious, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(61),   10, 10, DUR_NONE,
	"",         "!Cure Serious!",   "", NULL
	},

	{
	"curse", 
	{ 102, 102, 12, 102, 102, 102, 102, 102, 102, 24, 16, 102, 102, 102, 35 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_curse,        TAR_OBJ_CHAR_OFF,   POS_FIGHTING,
	&gsn_curse,     SLOT(17),   20, 12, DUR_SHORT,
	"curse",        "The curse wears off.", 
	"$p is no longer impure.", NULL
	},

	{
	"damned blade",  
	{ 102, 102, 53, 102, 102, 102, 102, 102, 102, 102, 62, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_damned_blade, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(665),    30, 8, DUR_NONE,
	"",     "!Damned Blade!","", NULL
	},

	{
	"demonfire", 
	{ 102, 102, 35, 102, 102, 43, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_demonfire,    TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(505),  20, 12, DUR_NONE,
	"torments",     "!Demonfire!",      "", NULL
	},  

	{
	"detect astral", 
	{ 102, 102, 102, 60, 102, 102, 102, 102, 102, 102, 102, 32, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_detect_astral,        TAR_CHAR_SELF, POS_STANDING,
	NULL,           SLOT(3001),    50, 12, DUR_BRIEF,
	"",         "You no longer perceive the astral plane.","", NULL
	},

	{
	"detect evil",  
	{ 102, 102, 11, 24, 102, 50, 22, 102, 102, 102, 12, 23, 102, 102, 24 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_detect_evil,  TAR_CHAR_SELF,      POS_STANDING,
	NULL,           SLOT(18),    5, 12, DUR_NORMAL,
	"",         "The red in your vision disappears.",   "", NULL
	},

	{
	"detect good",  
	{ 102, 102, 11, 24, 102, 50, 22, 102, 102, 102, 12, 23, 102, 34, 24 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_detect_good,      TAR_CHAR_SELF,          POS_STANDING,
	NULL,                   SLOT(513),        5,     12, DUR_NORMAL,
	"",                     "The gold in your vision disappears.",  "", NULL
	},

	{
	"detect hidden",  
	{ 102, 102, 102, 35, 102, 102, 102, 102, 53, 102, 44,  3,  44,  38, 35 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  8,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_detect_hidden,    TAR_CHAR_SELF,      POS_STANDING,
	NULL,           SLOT(44),    5, 12, DUR_NORMAL,
	"",         "You feel less aware of your surroundings.",    
	"", NULL
	},

	{
	"detect invis",   
	{ 102, 43, 18, 15, 102, 102, 102, 102, 51, 102, 16,  2,  102, 102, 15 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_detect_invis, TAR_CHAR_SELF,      POS_STANDING,
	NULL,           SLOT(19),    5, 12, DUR_NORMAL,
	"",         "You no longer see invisible objects.",
	"", NULL
	},

	{
	"detect magic", 
	{ 102, 41, 12,  6, 102, 54, 15, 102, 49, 102, 13,  6, 102, 102, 6 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_detect_magic, TAR_CHAR_SELF,      POS_STANDING,
	NULL,           SLOT(20),    5, 12, DUR_NORMAL,
	"",         "The detect magic wears off.",  "", NULL
	},

	{
	"detect poison", 
	{ 102, 42, 10,  9, 102, 52, 20, 102, 46, 102, 10,  9, 102, 22, 9 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_detect_poison,    TAR_OBJ_INV,        POS_STANDING,
	NULL,           SLOT(21),    5, 12, DUR_NONE,
	"",         "!Detect Poison!",  "", NULL
	},

	{
	"dispel evil",  
	{ 102, 102, 10, 22, 102, 27, 16, 102, 102, 102, 21, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_dispel_evil,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(22),   15, 12, DUR_NONE,
	"dispel evil",      "!Dispel Evil!",    "", NULL
	},

	{
	"dispel good",  
	{ 102, 102, 10, 22, 102, 27, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_dispel_good,      TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(512),      15,     12, DUR_NONE,
	"dispel good",          "!Dispel Good!",    "", NULL
	},

	{
	"dispel magic",    
	{ 102, 102, 38, 12, 102, 57, 102, 102, 102, 44, 30, 14, 102, 102, 15 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_dispel_magic, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(59),   15, 12, DUR_NONE,
	"",         "!Dispel Magic!",   "", NULL
	},

	{
	"divine light", 
	{ 102, 102, 19, 102, 102, 102, 44, 102, 102, 22, 28, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_divine_light, TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(1003),  7, 12, DUR_SPECIAL,
	"",         "!Divine Light!",   "", NULL
	},

	{
	"dominate soul",   
	{ 102, 102, 48, 102, 102, 102, 102, 102, 102, 102, 55, 102, 102, 102, 65 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_dominate_soul,    TAR_CHAR_NEUTRAL, POS_FIGHTING,
	NULL,           SLOT(670),    80, 20, DUR_NONE,
	"conflict",         "!Dominate Soul!", "", NULL
	},

	{
	"earthquake",  
	{ 102, 102, 18, 102, 102, 33, 38, 102, 102, 102, 19, 102, 102, 22, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_earthquake,   TAR_IGNORE,     POS_FIGHTING,
	NULL,           SLOT(23),   15, 12, DUR_NONE,
	"earthquake",       "!Earthquake!",     "", NULL
	},

	{
	"electrocution",  
	{ 102, 102, 41, 41, 102, 102, 102, 102, 102, 102, 41, 41, 102, 102, 41 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_electrocution,    TAR_CHAR_SELF,     POS_FIGHTING,
	&gsn_electrocution,           SLOT(733),    60, 20, DUR_SHORT,
	"electric aura",         "You are no longer imbued with electrical power.",  "", NULL
	},

	{
	"enchant armor", 
	{ 102, 102, 102, 22, 102, 102, 102, 102, 102, 102, 102, 18, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_enchant_armor,    TAR_OBJ_INV,        POS_STANDING,
	&gsn_enchant_armor,           SLOT(510),  100,    24, DUR_NONE,
	"",         "!Enchant Armor!",  "", NULL
	},

	{
	"enchant arrow", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 21, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_enchant_arrow,    TAR_OBJ_INV,        POS_STANDING,
	&gsn_enchant_arrow,     SLOT(524),  30,    12, DUR_NONE,
	"enchanted arrow", "!Enchant Arrow!",  "", NULL
	},

	{
	"enchant weapon", 
	{ 102, 102, 102, 28, 102, 102, 102, 102, 102, 102, 102, 22, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_enchant_weapon,   TAR_OBJ_INV,        POS_STANDING,
	&gsn_enchant_weapon,           SLOT(24),   100,    24, DUR_NONE,
	"",         "!Enchant Weapon!", "", NULL
	},

	{
	"energy drain", 
	{ 102, 102, 102, 17, 102, 102, 102, 102, 102, 102, 24, 102, 102, 102, 17 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_energy_drain, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(25),   35, 12, DUR_NONE,
	"energy drain",     "!Energy Drain!",   "", NULL
	},

	{
	"epidemic",  
	{ 102, 102, 43, 43, 102, 102, 102, 102, 102, 102, 43, 43, 102, 102, 43 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_epidemic,    TAR_CHAR_SELF,     POS_FIGHTING,
	&gsn_epidemic,           SLOT(732),    60, 20, DUR_SHORT,
	"sickening aura",         "You are no longer disease-ridden.",  "", NULL
	},

	{
	"fade",  
	{ 102, 102, 67, 58, 102, 102, 102, 102, 102, 102, 66, 102, 102, 102, 60 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_fade,    TAR_CHAR_SELF,     POS_STANDING,
	&gsn_fade,           SLOT(735),    30, 16, DUR_BRIEF,
	"",         "You are no longer phasing in and out of existence.",  "", NULL
	},

	{
	"faerie fire", 
	{ 102, 102, 102,  4, 102, 102, 102, 102, 102, 102, 102, 2, 102, 8, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_faerie_fire,  TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(72),    5, 12, DUR_SHORT,
	"faerie fire",      "The pink aura around you fades away.",
	"", NULL
	},

	{
	"faerie fog",  
	{ 102, 102, 102,  8, 102, 102, 102, 102, 102, 102, 102, 3, 102, 12, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_faerie_fog,   TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(73),   12, 12, DUR_NONE,
	"faerie fog",       "!Faerie Fog!",     "", NULL
	},

	{
	"farsight", 
	{ 102, 102, 102, 58, 102, 102, 102, 102, 102, 102, 102, 48, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_farsight,     TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(521),  36, 20, DUR_NONE,
	"farsight",     "!Farsight!",       "", NULL
	},  

	{
	"fear", 
	{ 102, 102, 102, 35, 102, 102, 102, 102, 60, 102, 102, 4,  51, 102, 35 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  6, 102, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_fear,   TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_fear,           SLOT(1002), 15, 12, DUR_BRIEF,
	"spell",        "You feel your courage returning.",   "", NULL
	},

	{
	"feeblemind",  
	{ 102, 102, 102, 18, 102, 102, 102, 102, 102, 102, 85, 10, 102, 102, 18 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_feeblemind,   TAR_VIS_CHAR_OFF, POS_FIGHTING,
	&gsn_feeblemind,           SLOT(1001), 30, 12, DUR_SHORT,
	"spell",        "You feel smarter.",    "", NULL
	},

	{
	"fire breath", 
	{ 102, 102, 102, 51, 102, 102, 102, 102, 102, 102, 102, 86, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_fire_breath,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_fire_breath,           SLOT(201),  12,    4, DUR_NONE,
	"blast of flame",   "The smoke leaves your eyes.",  "", NULL
	},

	{
	"fireball",  
	{ 102, 102, 102, 10, 102, 20, 102, 102, 18, 102, 30, 12, 102, 16, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_fireball,     TAR_IGNORE, POS_FIGHTING,
	NULL,           SLOT(26),   15, 12, DUR_NONE,
	"fireball",     "!Fireball!",       "", NULL
	},
  
	{
	"fireproof",  
	{ 102, 102, 28, 27, 102, 102, 102, 102, 102, 102, 102, 35, 102, 102, 28 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_fireproof,    TAR_OBJ_INV,        POS_STANDING,
	NULL,           SLOT(523),  10, 12, DUR_EXTREME,
	"",         "", "$p's protective aura fades.", NULL
	},

	{
	"flamestrike", 
	{ 102, 102, 40, 102, 102, 38, 70, 102, 102, 102, 45, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_flamestrike,  TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(65),   20, 12, DUR_NONE,
	"flamestrike",      "!Flamestrike!",        "", NULL
	},

	{
	"fly", 
	{ 60, 50, 20, 25, 102, 50, 50, 60, 50, 25, 25, 30, 85, 64, 25 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_fly,      TAR_CHAR_DEFENSIVE, POS_STANDING,
	&gsn_fly,           SLOT(56),   10, 18, DUR_NORMAL,
	"",         "You slowly float to the ground.",  "", NULL
	},

	{
	"floating disc",  
	{ 102, 102, 102, 20, 102, 102, 102, 102, 36, 102, 40, 102, 102, 102, 35 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_floating_disc,    TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(522),  40, 24, DUR_EXTREME,
	"",         "!Floating disc!",  "", NULL
	},

	{
	"frenzy", 
	{ 102, 102, 28, 102, 102, 102, 102, 102, 102, 47, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_frenzy,           TAR_CHAR_DEFENSIVE,     POS_STANDING,
	NULL,                   SLOT(504),      30,     24, DUR_SHORT,
	"",                     "Your rage ebbs.",  "", NULL
	},

	{
	"frost breath",  
	{ 102, 102, 102, 53, 102, 102, 102, 102, 102, 102, 102, 87, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_frost_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(202),  14,    4, DUR_NONE,
	"blast of frost",   "!Frost Breath!",   "", NULL
	},

	{
	"gas breath",  
	{ 102, 102, 102, 59, 102, 102, 102, 102, 102, 102, 102, 90, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_gas_breath,   TAR_IGNORE,     POS_FIGHTING,
	NULL,           SLOT(203),  20,    4, DUR_NONE,
	"blast of gas",     "!Gas Breath!",     "", NULL
	},

	{
	"gate",  
	{ 50, 40, 25, 20, 102, 40, 40, 50, 40, 30, 30, 25, 86, 78, 22 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_gate,     TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(83),   80, 12, DUR_NONE,
	"",         "!Gate!",       "", NULL
	},

	{
	"giant strength",  
	{ 45, 40, 102,  8, 102, 40, 102, 50, 35, 102, 30, 10, 62, 20, 9 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  5,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_giant_strength,   TAR_CHAR_DEFENSIVE, POS_STANDING,
	&gsn_giant_strength,           SLOT(39),   20, 12, DUR_NORMAL,
	"",         "You feel weaker.", "", NULL
	},

	{
	"goodberry",  
	{ 102, 102, 7, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 36, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_goodberry,             TAR_IGNORE,             POS_FIGHTING,
	NULL,           SLOT( 738),       16,      10, DUR_NONE,
	"",     "",   "", NULL
	},

	{
	"group heal",  
	{ 102, 102, 60, 102, 102, 102, 102, 102, 102, 80, 90, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_group_heal,       TAR_IGNORE,             POS_FIGHTING,
	NULL,                   SLOT(528),      60,     14, DUR_NONE,
	"",                     "!Group Heal!",   "", NULL
	},


		{
		"hailstorm",
		{ 102, 102, 88, 88, 102, 102, 102, 102, 102, 102, 89, 102, 102, 102, 102 },
		{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_hailstorm,        TAR_VIS_CHAR_OFF,     POS_FIGHTING,
		&gsn_hailstorm,           SLOT(301),      60,   28, DUR_NONE,
		"hailstone",           "!Hailstorm!",  "",     NULL
		},

        {   
        "hallow",
        { 102, 102,  102, 102, 102, 102, 40, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
 	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_hallow,        TAR_CHAR_DEFENSIVE,   POS_STANDING,
        NULL,           SLOT(1249),   75 , 12, DUR_BRIEF,
        "",         "You are no longer hallowed.",
        "$p's looks less hallowed.", NULL
        },


	{
	"hand of siva",  
	{ 102, 102, 102, 56, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 60 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_hand_of_siva,    TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(737),    250, 40, DUR_NONE,
	"",         "",  "", NULL
	},


	{
	"harm", 
	{ 102, 102, 40, 102, 102, 102, 64, 102, 102, 80, 48, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_harm,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(27),   35, 12, DUR_NONE,
	"harm spell",       "!Harm!",       "", NULL
	},
  
	{
	"haste",  
	{ 65, 35, 102, 20, 102, 60, 102, 60, 40, 102, 102, 25, 48, 35, 20 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_haste,        TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	&gsn_haste,           SLOT(502),  30, 12, DUR_SHORT,
	"",         "You feel yourself slow down.", "", NULL
	},

	{
	"heal", 
	{ 102, 102, 35, 102, 102, 102, 80, 102, 102, 40, 45, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_heal,     TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(28),   50, 14, DUR_NONE,
	"",         "!Heal!",       "", NULL
	},
  
	{
	"heat metal", 
	{ 102, 102, 75, 65, 102, 102, 102, 102, 102, 102, 70, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_heat_metal,   TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(516),  25, 18, DUR_NONE,
	"spell",        "!Heat Metal!",     "", NULL
	},

	{
	"holy apparition",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 77, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_holy_apparition,  TAR_IGNORE, POS_STANDING,
	NULL,           SLOT(2403),   350, 12, DUR_NONE,
	"holy apparition",     "!Holy Apparition!",       "", NULL
	},

	{
	"holy binding", 
	{ 102, 102, 14, 102, 102, 102, 102, 102, 102, 17, 24, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_holy_binding, TAR_OBJ_INV,        POS_STANDING,
	NULL,           SLOT(1004), 100,    24, DUR_NONE,
	"",         "!Holy Binding!",   "", NULL
	},

	{
	"holy word",   
	{ 102, 102, 55, 102, 102, 102, 102, 102, 102, 90, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_holy_word,    TAR_IGNORE, POS_FIGHTING,
	NULL,           SLOT(506),  200,    24, DUR_NONE,
	"divine wrath",     "!Holy Word!",      "", NULL
	},

	{
	"identify",     
	{ 102, 102, 45, 18, 102, 102, 102, 102, 102, 102, 65, 24, 102, 102, 23 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_identify,     TAR_OBJ_INV,        POS_STANDING,
	NULL,           SLOT(53),   12, 24, DUR_NONE,
	"",         "!Identify!",       "", NULL
	},

	{
	"immolation",  
	{ 102, 102, 40, 40, 102, 102, 102, 102, 102, 102, 40, 40, 102, 102, 40 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_immolation,    TAR_CHAR_SELF,     POS_FIGHTING,
	&gsn_immolation,           SLOT(731),    60, 20, DUR_SHORT,
	"burning aura",         "You are no longer burning.",  "", NULL
	},

	{
	"infravision",  
	{ 20, 15, 102,  5, 102, 45, 102,  6, 20, 102, 11,  1, 22, 58, 5 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_infravision,  TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL,           SLOT(77),    5, 18, DUR_NORMAL,
	"",         "You no longer see in the dark.",   "", NULL
	},

	{
	"invisibility",   
	{ 102, 50, 102, 30, 102, 80, 102, 102, 40, 102, 102,  4, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_invis,        TAR_OBJ_CHAR_DEF,   POS_STANDING,
	&gsn_invis,     SLOT(29),    5, 12, DUR_NORMAL,
	"",         "You are no longer invisible.",     
	"$p fades into view.", NULL
	},

	{
	"iron maiden", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 86 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_iron_maiden,   TAR_VIS_CHAR_OFF,      POS_FIGHTING,
	NULL,           SLOT(2051),   60, 12, DUR_BRIEF,
	"self-torture",         "Your torture ends.",  "", NULL
	},

	{
	"know alignment", 
	{ 102, 44,  9, 13, 102, 40, 12, 102, 42, 102,  9, 12, 39, 102, 13 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_know_alignment,   TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(58),    9, 12, DUR_NONE,
	"",         "!Know Alignment!", "", NULL
	},

	{
	"lightning bolt", 
	{ 102, 102, 102, 15, 102, 30, 102, 102, 28, 102, 45, 18, 102, 20, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_lightning_bolt,   TAR_VIS_CHAR_OFF, POS_FIGHTING,
	&gsn_lightning_bolt,           SLOT(30),   15, 12, DUR_NONE,
	"lightning bolt",   "!Lightning Bolt!", "", NULL
	},

	{
	"lightning breath",
	{ 102, 102, 102, 55, 102, 102, 102, 102, 102, 102, 102, 88, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_lightning_breath, TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(204),  16,    4, DUR_NONE,
	"blast of lightning",   "!Lightning Breath!",   "", NULL
	},

	{
	"locate object",  
	{ 102, 40, 102, 28, 102, 80, 102, 102, 102, 102, 70, 31, 81, 102, 28 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_locate_object,    TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(31),   20, 18, DUR_NONE,
	"",         "!Locate Object!",  "", NULL
	},

	{
	"magic missile", 
	{ 102, 102, 102,  1, 102,  2, 102, 102,  2, 102,  3,  1, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_magic_missile,    TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(32),   1, 12, DUR_NONE,
	"magic missile",    "!Magic Missile!",  "", NULL
	},

	{
	"major group heal", 
	{ 102, 102, 80, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_major_group_heal, TAR_IGNORE,             POS_FIGHTING,
	NULL,                   SLOT(529),      100,     18, DUR_NONE,
	"",                     "!Major Group Heal!",   "", NULL
	},

	{
	"mana burn", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 46 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_mana_burn,   TAR_VIS_CHAR_OFF,      POS_FIGHTING,
	NULL,           SLOT(2050),   30, 12, DUR_SHORT,
	"burning mana",         "Your mana no longer boils.",  "", NULL
	},

	{
        "mana shield", 
        { 102, 102, 102, 30, 102, 102, 102, 102, 102, 102, 102, 50, 102, 77, 35 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_NONE, STAT_NONE, STAT_NONE,
        spell_mana_shield,   TAR_CHAR_SELF,      POS_FIGHTING,
        &gsn_mana_shield,           SLOT(2048),   30, 18, DUR_SHORT,
        "",         "Your mana no longer protects you.",  "", NULL
	},
    
	{
        "mantra", 
        { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_NONE, STAT_NONE, STAT_NONE,
        spell_mantra,   TAR_CHAR_SELF,      POS_FIGHTING,
        NULL,           SLOT(2049),   30, 12, DUR_SHORT,
        "",         "You lose touch with your mantra.",  "", NULL
        },

	{
	"mass healing",  
	{ 102, 102, 50, 102, 102, 102, 102, 102, 102, 55, 75, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_mass_healing, TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(508),  100,    32, DUR_NONE,
	"",         "!Mass Healing!",   "", NULL
	},

	{
	"mass invis",   
	{ 102, 102, 102, 60, 102, 102, 102, 102, 102, 102, 102, 22, 102, 102, 62 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_mass_invis,   TAR_IGNORE,     POS_STANDING,
	&gsn_mass_invis,    SLOT(69),   20, 24, DUR_NORMAL,
	"",         "You are no longer invisible.",     "", NULL
	},

	{
	"minor fade",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_minor_fade,    TAR_CHAR_SELF,     POS_STANDING,
	&gsn_minor_fade,           SLOT(734),    30, 16, DUR_BRIEF,
	"",         "You stop phasing in and out of existence.",  "", NULL
	},

	{
	"minor group heal",  
	{ 102, 102, 40, 102, 102, 102, 85, 102, 102, 60, 80, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_minor_group_heal, TAR_IGNORE,             POS_FIGHTING,
	NULL,                   SLOT(527),      20,     10, DUR_NONE,
	"",                     "!Minor Group Heal!",   "", NULL
	},

	{
	"mimic",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_mimic,          TAR_CHAR_DEFENSIVE,             POS_STANDING,
	&gsn_mimic,                   SLOT(666),      50,     12, DUR_NORMAL,
	"",                     "The illusion surrounding you fades.",   "", NULL
	},

	{
	"mindflay",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,    TAR_IGNORE,     POS_FIGHTING,
	&gsn_mindflay,              SLOT( 0),       0,      12, DUR_SPECIAL,
	"mindflaying",         "You mind starts working again.",  "", NULL
	},

	{
	"mirror image",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 29, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_mirror_image,          TAR_CHAR_SELF,             POS_FIGHTING,
	&gsn_mirror_image,            SLOT(667),      50,     18, DUR_NORMAL,
	"",                     "The mirror images surrounding you fade.",   "", NULL
	},

		{
		"meteor swarm",
		{ 102, 102, 72, 76, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
		{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_meteor_swarm,     TAR_VIS_CHAR_OFF,     POS_FIGHTING,
		&gsn_meteor_swarm,            SLOT(302),     50, 28, DUR_NONE,
		"swarm of meteors",     "!Meteor Swarm!", "",   NULL
		},

		{
		"monsoon",
		{ 102, 102, 78, 82, 102, 102, 102, 102, 102, 102, 87, 91, 102, 80, 102 },
		{   3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3  },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_monsoon,          TAR_IGNORE,     POS_FIGHTING,
		&gsn_monsoon,           SLOT(300),      60, 18, DUR_NONE,
		"torrential rain",      "!Monsoon!",    "",     NULL
		},


	{
	"necrosis", 
	{ 102, 102, 61, 63, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 60 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_necrosis, TAR_VIS_CHAR_OFF, POS_FIGHTING,
	&gsn_necrosis,  SLOT(669),    50, 12, DUR_SHORT,
	"necrosis",     "Your illness subsides.", "", NULL
	},

	{
	"nexus",  
	{ 102, 90, 50, 35, 102, 90, 90, 102, 90, 55, 55, 40, 88, 102, 40 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 5,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_nexus,            TAR_IGNORE,             POS_STANDING,
	NULL,                   SLOT(520),       150,   36, DUR_SPECIAL,
	"",                     "!Nexus!",      "", NULL
	},

        {   
        "overcharge",
        { 102, 102, 102,  95, 102, 102,  40, 102, 102, 102, 102,  95, 102, 102, 102 },
        {   3,   3,   3,   8,   3,   3,   3,   3,   3,   3,   3,   3,   3,   8,   3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  95, 100, 100, 100, 100 },
 	STAT_VIT, STAT_INT, STAT_CON,
        spell_overcharge,        TAR_CHAR_SELF,   POS_STANDING,
        &gsn_overcharge,           SLOT(63),   200, 18, DUR_SPECIAL,
        "",         "As your focus breaks your mana stops overcharging.",
        },



	{
	"pacify", 
	{ 102, 102, 58, 102, 102, 102, 102, 102, 102, 102, 55, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_pacify,       TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(1000), 50, 12, DUR_NONE,
	"spell",        "You feel aggressive.", "", NULL
	},

	{
	"pass door",  
	{ 90, 90, 90, 75, 102, 90, 90, 90, 85, 90, 90, 80, 102, 88, 77 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_pass_door,    TAR_CHAR_SELF,      POS_STANDING,
	NULL,           SLOT(74),   20, 12, DUR_BRIEF,
	"",         "You feel solid again.",    "", NULL
	},

	{
	"phantasmal image",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 57, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_phantasmal_image,          TAR_CHAR_SELF,             POS_STANDING,
	&gsn_phantasmal_image,            SLOT(2710),      150,     24, DUR_NORMAL,
	"phantasmal image",                     "The phantasmal images surrounding you fade.",   "", NULL
	},

	{
	"phase",  
	{ 90, 90, 90, 90, 102, 102, 102, 102, 102, 102, 66, 102, 102, 102, 60 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_phase,    TAR_CHAR_SELF,     POS_STANDING,
	&gsn_phase,           SLOT(735),    30, 8, DUR_BRIEF,
	"",         "You are no longer phasing away from offensive spells.",  "", NULL
	},

	{
	"plague", 
	{ 102, 102, 24, 30, 102, 102, 102, 102, 102, 48, 36, 102, 102, 102, 26 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_plague,       TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_plague,        SLOT(503),  20, 12, DUR_SHORT,
	"sickness",     "Your sores vanish.",   "", NULL
	},

	{
	"poison",  
	{ 102, 102, 18, 18, 102, 102, 102, 102, 102, 36, 27, 30, 102, 102, 18 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_poison,       TAR_OBJ_CHAR_OFF,   POS_FIGHTING,
	&gsn_poison,        SLOT(33),   10, 12, DUR_SHORT,
	"poison",       "You feel less sick.",  
	"The poison on $p dries up.", NULL
	},

	{
	"portal",   
	{ 80, 70, 40, 30, 102, 70, 70, 80, 70, 45, 45, 35,  89, 102, 30 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  5,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_portal,           TAR_IGNORE,             POS_STANDING,
	NULL,                   SLOT(519),       100,     24, DUR_SPECIAL,
	"",                     "!Portal!",     "", NULL
	},

	{
	"prayer",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_prayer,        TAR_CHAR_SELF,   POS_STANDING,
	&gsn_prayer,           SLOT( 948),    50, 20, DUR_LONG,
	"",         "You feel less righteous.", "", NULL
	},

	{
	"protection evil", 
	{ 102, 102, 13, 28, 102, 53, 23, 102, 102, 16, 17, 32, 102, 102, 30 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_protection_evil,  TAR_CHAR_SELF,      POS_STANDING,
	NULL,           SLOT(34),   5,  12, DUR_SHORT,
	"",         "You feel less protected.", "", NULL
	},

	{
	"protection good", 
	{ 102, 102, 13, 28, 102, 53, 102, 102, 102, 16, 17, 32, 102, 102, 30 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_protection_good,  TAR_CHAR_SELF,          POS_STANDING,
	NULL,                   SLOT(514),       5,     12, DUR_SHORT,
	"",                     "You feel less protected.", "", NULL
	},

	{
	"protection magic",  
	{ 102, 102, 71, 83, 102, 102, 102, 102, 102, 102, 79, 76, 102, 102, 83 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_protection_magic,    TAR_CHAR_DEFENSIVE,     POS_STANDING,
	NULL,           SLOT(736),    25, 8, DUR_SHORT,
	"",         "You are no longer protected from magic.",  "", NULL
	},

	{
	"ray of truth", 
	{ 102, 102, 11, 102, 102, 102, 38, 102, 102, 14, 21, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_ray_of_truth,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(518),      20,     12, DUR_NONE,
	"ray of truth",         "!Ray of Truth!",   "", NULL
	},

	{
	"recharge", 
	{ 102, 102, 102, 81, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 85 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_recharge,     TAR_OBJ_INV,        POS_STANDING,
	NULL,           SLOT(517),  60, 24, DUR_NONE,
	"",         "!Recharge!",       "", NULL
	},

	{
	"reflection", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,   3,    3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,  3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_reflection, TAR_CHAR_SELF,   POS_STANDING,
	&gsn_reflection,           SLOT(532),   80,   18, DUR_BRIEF,
	"",         "You are no longer reflecting spells.",   "", NULL
	},

	{
	"refresh", 
	{ 55, 50, 40, 30, 102, 50, 102, 102, 45,  25, 35, 40, 14, 6, 30  },
	{  3,  3,  4,  3,  3,  3,  3,  3,  3,  3,  4,  4,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_refresh,      TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(81),    15,   8, DUR_NONE,
	"refresh",      "!Refresh!",        "", NULL
	},

	{
	"remove curse", 
	{ 102, 102, 11, 102, 102, 102, 102, 102, 102, 14, 19, 102, 102, 102, 20 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_remove_curse, TAR_OBJ_CHAR_DEF,   POS_STANDING,
	NULL,           SLOT(35),    5, 12, DUR_NONE,
	"",         "!Remove Curse!",   "", NULL
	},

	{
	"replenish", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_replenish, TAR_CHAR_SELF,   POS_FIGHTING,
	NULL,           SLOT(531),   50,   12, DUR_SPECIAL,
	"",         "You are no longer being replenished.",   "", NULL
	},

	{
	"replenish cooldown",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_replenish_cooldown,           SLOT( 0),       0,      8, DUR_SPECIAL,
	"",     "You're able to replenish yourself once again.",   "", NULL
	},

	{
	"renewal", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_renewal, TAR_CHAR_SELF,   POS_STANDING,
	NULL,           SLOT(531),   50,   12, DUR_NONE,
	"",         "!Renewal!",   "", NULL
	},

	{
	"restoration",  
	{ 102, 102, 90, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_restoration,      TAR_CHAR_DEFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(530),       10,    20, DUR_NONE,
	"",                     "!Restoration!",        "", NULL
	},

	{
	"ritual sacrifice", 
	{ 102, 102, 65, 75, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 67 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_ritual_sacrifice, TAR_CHAR_SELF,  POS_STANDING,
	&gsn_ritual,        SLOT(672),    100,    16, DUR_BRIEF,
	"",               "The bloodbath has ended.", "", NULL
	},

	{
	"sanctuary",  
	{ 102, 102, 30, 45, 102, 102, 80, 102, 102, 55, 60, 50, 102, 80, 45 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_sanctuary,    TAR_CHAR_DEFENSIVE, POS_STANDING,
	&gsn_sanctuary,     SLOT(36),   75, 12, DUR_BRIEF,
	"",         "The white aura around your body fades.",
	"", NULL
	},

	{
	"shield",  
	{ 102, 102, 19, 17, 102, 31, 43, 102, 40, 20, 18, 18, 102, 102, 18 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_shield,       TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL,           SLOT(67),   12, 18, DUR_SHORT,
	"",         "Your force shield shimmers then fades away.",
	"", NULL
	},

	{
	"shocking grasp", 
	{ 102, 102, 102,  5, 102, 10, 102, 102,  8, 102, 15,  6, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_shocking_grasp,   TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(53),   8, 12, DUR_NONE,
	"shocking grasp",   "!Shocking Grasp!", "", NULL
	},

	{
	"shroud of darkness",  
	{102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_shroud_of_darkness, TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL, SLOT(2110), 50, 24, DUR_BRIEF,
	"shroud of darkness",   "You are no longer protected by darkness.", "", NULL
	},


	{
	"sleep", 
	{ 102, 102, 102,  6, 102, 102, 102, 102,  8, 102, 70,  2, 102, 102, 10 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_sleep,        TAR_VIS_CHAR_OFF, POS_FIGHTING,
	&gsn_sleep,     SLOT(38),   15, 12, DUR_SPECIAL,
	"",         "You feel less tired.", "", NULL
	},

	{
	"slow", 
	{ 102, 102, 22, 21, 102, 102, 102, 102, 102, 22, 33, 26, 102, 102, 23 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_slow,     TAR_VIS_CHAR_OFF,     POS_FIGHTING,
	&gsn_slow,           SLOT(515),      30,     12, DUR_SHORT,
	"",         "You feel yourself speed up.",  "", NULL
	},
    
	{
	"solar flare", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,   0,   3,   3,   0,   0,   0,   0,   0,   0,   3,   0,   0,   3,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_WIS, STAT_INT, STAT_CHA,
	spell_solar_flare,  TAR_VIS_CHAR_OFF, POS_FIGHTING,
	&gsn_solar_flare,		SLOT(100),			  60,		20, DUR_NONE,
	"solar flare",      "!Solar Flare!",        "", NULL
	},

	{
	"stop", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_stop,     TAR_VIS_CHAR_OFF,     POS_FIGHTING,
	NULL,           SLOT(1324),      18,     10, DUR_SPECIAL,
	"",         "!Stop!",  "", NULL
	},
       
        {
	"stone skin", 
	{ 102, 102, 29, 27, 102, 51, 73, 102, 102, 30, 27, 28, 102, 75, 27 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_stone_skin,   TAR_CHAR_SELF,      POS_STANDING,
	NULL,           SLOT(66),   12, 18, DUR_NORMAL,
	"",         "Your skin feels soft again.",  "", NULL
	},

	{
	"summon", 
	{ 90, 80, 30, 40, 102, 80, 80, 90, 80, 35, 35, 45,  87, 82, 40 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  5,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_summon,       TAR_IGNORE,     POS_STANDING,
	NULL,           SLOT(40),   50, 12, DUR_NONE,
	"",         "!Summon!",     "", NULL
	},

	{
	"teleport",  
	{ 23, 20, 18,  8, 102, 20, 20, 23, 20, 25, 25, 15, 102, 102, 12 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_teleport,     TAR_CHAR_SELF,      POS_FIGHTING,
	NULL,           SLOT( 2),   35, 12, DUR_NONE,
	"",         "!Teleport!",       "", NULL
	},

	{
	"turn undead", 
	{ 102, 102,  8, 102, 102, 102, 16, 102, 102, 11, 102, 102, 102, 39, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_turn_undead,  TAR_IGNORE, POS_FIGHTING,
	NULL,           SLOT(668),    8,  12, DUR_NONE,
	"turn",     "!Turn Undead!","", NULL
	},

	{
	"unearth", 
	{ 102, 102,  91,  90, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   3,   3,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_WIS, STAT_INT, STAT_VIT,
	spell_unearth,  TAR_VIS_CHAR_OFF, POS_FIGHTING,
	&gsn_unearth,           SLOT(653),   95, 18, DUR_NONE,
	"unearth",      "!Unearth!",        "", NULL
	},

	{
	"ventriloquate",  
	{ 102, 15, 102,  8, 102, 102, 102, 102, 102, 102, 102,  1, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_ventriloquate,    TAR_CHAR_DEFENSIVE,     POS_STANDING,
	NULL,           SLOT(41),    5, 12, DUR_NONE,
	"",         "!Ventriloquate!",  "", NULL
	},

	{
	"weaken",  
	{ 102, 102, 20, 15, 102, 102, 102, 102, 102, 20, 30, 22, 102, 102, 15 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_weaken,       TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(68),   20, 12, DUR_SHORT,
	"spell",        "You feel stronger.",   "", NULL
	},

	{
	"word of recall",  
	{  8,  4,  1,  1, 102,  4,  4,  6,  4,  2,  2,  2, 102, 102, 1 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_word_of_recall,   TAR_CHAR_SELF,      POS_RESTING,
	&gsn_word_of_recall,           SLOT(42),    50, 12, DUR_NONE,
	"",         "!Word of Recall!", "", NULL
	},

/*
 * Necromancer spells
 */

	{
	"cone of exhaustion",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 70 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_cone_of_exhaustion, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(2100),  14,    4, DUR_NONE,
	"exhausting cone",   "!cone of exhaustion!",   "", NULL
	},

	{
	"forboding ooze",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 71 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_forboding_ooze, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(2101),  14,    4, DUR_NONE,
	"ooze",   "The ooze on you drips off.",   "", NULL
	},

	{
	"tomb stench",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 72 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_tomb_stench, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(2102),  14,    4, DUR_NONE,
	"tomb stench",   "!tomb stench!",   "", NULL
	},

	{
	"zombie breath",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 73 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_zombie_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(2103),  14,    4, DUR_NONE,
	"zombie breath",   "!zombie breath!",   "", NULL
	},

	{
	"zone of damnation",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 74 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_zone_of_damnation, TAR_IGNORE, POS_FIGHTING,
	NULL,           SLOT(2104),  14,    4, DUR_NONE,
	"damnation",   "!zone of damnation!",   "", NULL
	},
	
	{
	"decompose",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_decompose, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_decompose,           SLOT(2105),  14,    12, DUR_SHORT,
	"decompose",   "Your body feels juicy again.", "", NULL
	},

	{
	"heal mind",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_heal_mind, TAR_CHAR_SELF, POS_FIGHTING,
	NULL,           SLOT(2106),  5,    18, DUR_NONE,
	"heal mind",   "!heal mind!", "", NULL
	},

	{
	"life force",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_life_force, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
	NULL,           SLOT(2107),  30,    24, DUR_NONE,
	"life force",   "!life force!", "", NULL
	},

	{
	"heal all",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_heal_all, TAR_IGNORE, POS_FIGHTING,
	NULL,           SLOT(2108),  50,    12, DUR_NONE,
	"heal all",   "!heal all!", "", NULL
	},

	{
	"extinguish",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_extinguish, TAR_OBJ_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(2109),  5,    12, DUR_NONE,
	"extinguish",   "!extinguish!", "", NULL
	},

/* 
 *  Meta-magic skills
 */
    {
    "extend spell",
    { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
    STAT_NONE, STAT_NONE, STAT_NONE,
    spell_null, TAR_IGNORE, POS_FIGHTING,
    &gsn_extend_spell, SLOT(0), 0, 0, DUR_NONE,
    "", "!extend spell!", "", NULL
    },

    {
    "empower spell",
    { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
    STAT_NONE, STAT_NONE, STAT_NONE,
    spell_null, TAR_IGNORE, POS_FIGHTING,
    &gsn_empower_spell, SLOT(0), 0, 0, DUR_NONE,
    "", "!empower spell!", "", NULL
    },

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
	{
	"general purpose",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_general_purpose,  TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(401),      0,      12, DUR_NONE,
	"general purpose ammo", "!General Purpose Ammo!",   "", NULL
	},
 
	{
	"high explosive",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_high_explosive,   TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(402),      0,      12, DUR_NONE,
	"high explosive ammo",  "!High Explosive Ammo!",    "", NULL
	},

/*  NEW RANGER STUFF by SIVA 9/28/98 */

	{
	"blind fighting", 
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 55, 102 },
	{  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_blindfighting,   SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!blindfighting!",    "", NULL
	},

        {
        "beast mastery", 
        {  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_NONE, STAT_NONE, STAT_NONE,
                spell_null,             TAR_IGNORE,             POS_FIGHTING,
                &gsn_beast_mastery,   SLOT( 0),        0,     0, DUR_NONE,
                "",                     "!beast mastery!",    "", NULL
        },

        {
	"camp fire",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 26, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 1, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_RESTING,
	&gsn_camp_fire,    SLOT(0),    0,  24, DUR_SPECIAL,
	"",         "!campfire!", ""
	},

	{
	"climbing",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 4, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 2, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_climbing,    SLOT(0),    0,  0, DUR_NONE,
	"",         "!climbing!", ""
	},
	
	{
	"entangle", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 30, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3  },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_entangle,   TAR_VIS_CHAR_OFF, POS_FIGHTING,
	NULL,           SLOT(2006),   30, 12, DUR_SHORT,
	"entanglement",   "You are no longer entangled in vines.", "", NULL
	},
	
	{
	"fishing",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 27, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 2, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_RESTING,
	&gsn_fishing,    SLOT(0),    0,  72, DUR_NONE,
	"",         "!fishing!", ""
	},
	
	{
	"giant feller", 
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 30, 102 },
	{  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_giantfeller,   SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!giantfeller!",    "", NULL
	},

	{
	"goblincleaver",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 11, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_goblincleaver,     SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"introspection",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 28, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_introspection,    SLOT(0),    0,  12, DUR_NONE,
	"",         "!introspection!", ""
	},
	
	{
	"pass without trace", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 60, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_pass_without_trace,   TAR_CHAR_SELF,      POS_STANDING,
	NULL,           SLOT(2003),   25, 18, DUR_LONG,
	"",         "Your trail is no longer obscured.",  "", NULL
	},

	{
	"raft",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 2, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 1, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_RESTING,
	&gsn_raft,    SLOT(0),    0,  12, DUR_NONE,
	"",         "!raft!", ""
	},
	
	{
	"soothe",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 80, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_soothe,    SLOT(0),    0,  0, DUR_BRIEF,
	"",         "!soothe!", ""
	},

	{
	"taxidermy",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 15, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 1, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_RESTING,
	&gsn_taxidermy,    SLOT(0),    0,  36, DUR_NONE,
	"",         "!taxidermy!", ""
	},

	{
	"tempest",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 60, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_tempest,     SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},
	
	{
	"treat weapon",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 19, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_RESTING,
	&gsn_treat_weapon,    SLOT(0),    0,  36, DUR_SPECIAL,
	"",         "The treatment you gave your weapon has worn off.", ""
	},
	
	{
	"tree golem",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 68, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_tree_golem,  TAR_IGNORE, POS_STANDING,
	NULL,           SLOT(2000),   200, 24, DUR_LONG,
	"tree golem",     "!Tree Golem!",       "", NULL
	},

        {
        "water elemental",  
        { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_NONE, STAT_NONE, STAT_NONE,
        spell_water_elemental,  TAR_IGNORE, POS_STANDING,
        NULL,           SLOT(2116),   200, 24, DUR_LONG,
        "water elemental",     "!Water Elemental!",       "", NULL
        },

	{
	"wendigo",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 70, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_wendigo,     SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"woodland combat", 
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 7, 102 },
	{  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_woodland_combat,   SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!woodlandcombat!",    "", NULL
	},

	{
	"wind war",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 76, 102 },
	{   3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_windwar,          TAR_IGNORE,     POS_FIGHTING,
	NULL,           SLOT(2001),      50, 18, DUR_NONE,
	"warring winds",      "!Wind War!",    "",     NULL
	},

/*  SKILLS  */

/* weapon skills */

	{
	"axe",
	{  1, 12, 102, 102,  1,  1, 19, 29, 102, 102, 102, 102, 102, 1, 102 },
	{  4,  8,  0,  0,  2,  4,  8, 11,  0,  0,  0,  0, 0, 3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_axe,               SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!Axe!",        "", NULL
	},

	{
	"dagger",  
	{  1,  1, 102,  1,  1,  1, 102,  1,  1, 102,  8,  1, 3, 1, 1 },
	{  2,  3,  0,  6,  2,  3,  0,  5,  6,  0, 12,  4,  5, 4, 6 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_dagger,            SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!Dagger!",     "", NULL
	},
 
	{
	"flail",  
	{  1,  8,  1, 102,  1, 102,  1, 102,  1, 86,  1, 102, 102, 102, 102 },
	{  4,  6,  6,  0,  4,  0,  4,  0,  7, 15, 10,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_flail,             SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!Flail!",      "", NULL
	},

	{
	"gun",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 1, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_gun,               SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!gun!",      "", NULL
	},

	{
	"bow",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 12, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_bow,               SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!gun!",      "", NULL
	},

	{
	"mace",  
	{  1,  1,  1, 24,  1,  1,  1,  1,  6, 102,  1, 21, 60, 102, 24 },
	{  3,  5,  5, 10,  3,  4,  4,  9,  9,  0, 11,  9,  4, 0, 10 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_mace,              SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!Mace!",       "", NULL
	},

	{
	"polearm",  
	{  1,  6,  1,  1,  1,  1,  7, 17,  1, 81,  1,  1,  102, 102, 1 },
	{  3,  5,  8,  5,  3,  4,  5, 10,  6, 15,  8,  6,  0, 0, 5 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_polearm,           SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!Polearm!",        "", NULL
	},
	
	{
	"spear",  
	{  1,  4, 102, 102,  1, 12,  4, 102,  3, 102, 13, 102, 102, 1, 102 },
	{  3,  7,  0,  0,  3,  3,  7,  0,  6,  0,  9,  0,  0,  4, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_spear,             SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!Spear!",      "", NULL
	},

	{
	"sword",  
	{  1,  2, 102, 102,  1,  1,  1,  1,  1, 102,  1, 102, 102, 1, 102 },
	{  2,  6,  0,  0,  3,  3,  8,  7,  7,  0, 12,  0,  0,  4, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_sword,             SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!sword!",      "", NULL
	},

	{
	"whip",  
	{  1,  1, 102,  5,  1,  5, 102,  1, 102, 102,  1, 102, 4, 1, 5 },
	{  3,  4,  0,  5,  3,  3,  0,  5,  0,  0,  7,  0,  5,  4, 5 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_whip,              SLOT( 0),       0,      0, DUR_NONE,
		"",                     "!Whip!",   "", NULL
	},


/* offensive combat skills */

	{
	"hand to hand",  
	{  4,  5,  6,  8,  1,  2,  5,  6,  1,  1,  6,  7,  5, 1, 8 },
	{  2,  4,  6,  8,  2,  2,  3,  4,  2,  2,  6,  6,  4, 4, 8 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_hand_to_hand,  SLOT( 0),   0,  0, DUR_NONE,
		"",         "!Hand to Hand!",   "", NULL
	},

	{
	"second attack",  
	{  2, 25, 65, 102,  2,  3,  3,  4,  3, 20, 70, 102, 35, 14, 102 },
	{  5,  6, 10,  0,  3,  5,  5,  6,  5,  5, 11,  0,  5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_second_attack,     SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!Second Attack!",  "", NULL
	},

	{
	"third attack",  
	{ 16, 102, 102, 102, 12, 20, 20, 25, 25, 45, 102, 102, 102, 102, 102 },
	{  6,  0,  0,  0,  4,  7,  7,  9,  7,  6,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_third_attack,      SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!Third Attack!",   "", NULL
	},

	{
	"enhanced damage", 
	{  2, 13, 102, 102,  2,  3,  3,  9, 10, 25, 102, 102, 21, 25, 102 },
	{  5, 10,  0,  0,  4,  6,  7,  6,  9,  8,  0,  0,  5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_enhanced_damage,   SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!Enhanced Damage!",    "", NULL
	},


	{
	"ashura", 
	{ 102, 102, 102, 102, 102, 70, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,   0,   0,   0,   0,  7,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_ashura,   SLOT( 0),        0,     0, DUR_NONE,
	"",                     "!Ashura!",    "", NULL
	},

	{
	"shan-ya", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_shan_ya,   SLOT( 0),        0,     0, DUR_SPECIAL,
	"",                     "Your battle madness subsides.",    "", NULL
	},

	/* just for playing around with behemoth --Bobble */
	{
	"brutal damage", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_brutal_damage,   SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!Brutal Damage!",    "", NULL
	},

	{
	"razor claws", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_razor_claws,   SLOT( 0),        0,     0, DUR_NONE,
		"rake",                     "!Razor Claws!",    "", NULL
	},

	{
	"dual wield",
	{  5, 102, 102, 102,  1,  5, 102, 37, 102, 102, 102, 102, 102, 10, 102 },
	{ 10,  0,  0,  0,  7, 12,  0, 12,  0,  0,  0,  0,  0,  10, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_FIGHTING,
		&gsn_dual_wield,            SLOT(0),     0, 0, DUR_NONE,
		"",         "!dual wield!", "", NULL
	},

	{
	"dual dagger",
	{ 102, 48, 102, 102, 2, 102, 102, 33, 41, 102, 102, 102, 102, 102, 102 },
	{  0,  9,  0,  0,  3,  0,  0,  4, 12,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_FIGHTING,
		&gsn_dual_dagger,           SLOT(0),     0, 0, DUR_NONE,
		"",         "!dual daggers!",   "", NULL
	},

	
	{
	"dual sword",
	{  102, 102, 102, 102, 25, 15, 102, 102, 102, 102, 102, 102, 102, 15, 102 },
	{  0,  0,  0,  0,  10,  4,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_FIGHTING,
		&gsn_dual_sword,           SLOT(0),     0, 0, DUR_NONE,
		"",         "!dual sword!",   "", NULL
	},

	{
	"dual axe",
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_FIGHTING,
		&gsn_dual_axe,           SLOT(0),     0, 0, DUR_NONE,
		"",         "!dual axe!",   "", NULL
	},

	{
	"dual gun",
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 44, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  10, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_FIGHTING,
		&gsn_dual_gun,           SLOT(0),     0, 0, DUR_NONE,
		"",         "!dual gun!",   "", NULL
	},

	{
	"backstab",
	{ 102,  2, 102, 102, 102, 102, 102,  1, 38, 102, 102, 50, 102, 102, 102 },
	{  0,  5,  0,  0,  0,  0,  0,  4,  8,  0,  0,  10,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_STANDING,
		&gsn_backstab,          SLOT( 0),        0,     24, DUR_NONE,
		"backstab",             "!Backstab!",       "", NULL
	},

	{
	"blackjack",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,      POS_FIGHTING,
		&gsn_blackjack,          SLOT( 0),        0,     24, DUR_NONE,
		"blackjack",    "The pain in your head subsides.",   "", NULL
	},

	{
	"circle",  
	{ 102, 27, 102, 102, 102, 102, 102, 33, 102, 102, 102, 102, 102, 60, 102 },
	{  0, 11,  0,  0,  0,  0,  0, 12,  0,  0,  0,  0,  0,  15, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_circle,              SLOT( 0),       0,      24, DUR_NONE,
		"circle",                 "!circle!",       "", NULL
	},

        {
	"assassination",  
	{ 102, 93, 102, 102, 102, 102, 102, 42, 102, 102, 102, 102, 63, 102, 102 },
	{  0, 14,  0,  0,  0,  0,  0, 12,  0,  0,  0,  0,  12,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_assassination,              SLOT( 0),       0,      0, DUR_NONE,
		"assassination",                 "!assassination!",       "", NULL
	},

	{
	"slash throat",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,  102, 102, 102 },
	{  0,  0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_FIGHTING,
	&gsn_slash_throat,      SLOT( 0),   0,  24, DUR_SPECIAL,
	"throat slash",      "You can speak again.",
	"", NULL
	},

	{
	"berserk",  
	{ 18, 102, 102, 102, 20, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  5,  0,  0,  0,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_berserk,           SLOT( 0),       0,      24, DUR_BRIEF,
		"",     "You feel your pulse slow down.",   "", NULL
	},

	{
	"war cry",
	{ 30, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_war_cry,           SLOT(0),     0, 24, DUR_BRIEF,
		"",         "You calm down a bit.", "", NULL
	},

	{
	"melee",
	{ 50, 102, 102, 102, 40, 55, 75, 102, 102, 102, 102, 102, 102, 102, 102 },
	{ 15,  0,  0,  0, 12, 18, 20, 0, 0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,  POS_FIGHTING,
		&gsn_melee,         SLOT(0),     0, 0, DUR_NONE,
		"", "You are no longer fighting so fiercely.",  "", NULL
	},

	{
	"mass combat",
	{ 90, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{ 10,  0,  0,  0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	 STAT_NONE, STAT_NONE, STAT_NONE,
	 spell_null,     TAR_IGNORE,  POS_FIGHTING,
	 &gsn_mass_combat,         SLOT(0),     0, 0, DUR_NONE,
	 "", "!mass combat!",  "", NULL
	},

	{
	"brawl",
	{ 35, 31, 102, 102, 20, 102, 102, 102, 53, 40, 102, 102, 36, 102, 102 },
	{  6, 10,  0,  0,  5,  0,  0,  0,  6,  5,  0,  0,  5, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,  POS_FIGHTING,
		&gsn_brawl,         SLOT(0),     0, 12, DUR_NONE,
		"",     "You are no longer kicking ass.",   "", NULL
	},

	{
	"kung fu",
	{ 45, 102, 102, 102, 40, 33, 102, 102,  6,  2, 102, 102, 102, 102, 102 },
	{  9,  0,  0,  0, 12,  8,  0,  0,  7,  6,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_kung_fu,           SLOT(0),     0, 0, DUR_NONE,
		"",         "!kung fu!",    "", NULL
	},

	{
	"jump up",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_jump_up,           SLOT(0),     0, 0, DUR_NONE,
		"",         "!jump up!",    "", NULL
	},

	{
	"double strike",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_FIGHTING,
	&gsn_double_strike,           SLOT(0),     0, 12, DUR_NONE,
	"double strike",         "!double strike!",    "", NULL
	},

	{
	"round swing",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_FIGHTING,
	&gsn_round_swing,           SLOT(0),     0, 18, DUR_NONE,
	"round swing",         "!round swing!",    "", NULL
	},

	{
	"puncture",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	&gsn_puncture,           SLOT(0),     0, 18, DUR_BRIEF,
	"puncture",         "Your armor mends again.",    "", NULL
	},

	{
	"sharp shooting",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 21, 29, 102 },
	{   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,  5,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 90, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_FIGHTING,
	&gsn_sharp_shooting,           SLOT(0),     0, 0, DUR_NONE,
	"",         "!sharp shooting!",    "", NULL
	},

	{
	"bite",
	{  8,  9, 102, 102,  5, 102, 102, 102, 102, 102, 102, 102, 102, 8, 102 },
	{  3,  3,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_bite,          SLOT(0),     0, 12, DUR_NONE,
	"bite",         "!bite!",   "", NULL
	},

	{
	"venom bite",
	{  102,  102, 102, 102,  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_venom_bite,          SLOT(0),     0, 12, DUR_NONE,
		"venomous bite",         "!venom bite!",   "", NULL
	},

	{
	"vampiric bite",
	{  102,  102, 102, 102,  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_vampiric_bite,          SLOT(0),     0, 12, DUR_NONE,
		"vampiric bite",         "!vampiric bite!",   "", NULL
	},

	{
	"chop",
	{ 10, 102, 102, 102,  4,  8, 102, 102,  6,  2, 102, 102, 102, 102, 102 },
	{  3,  0,  0,  0,  3,  2,  0,  0,  2,  1,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_chop,          SLOT(0),     0, 12, DUR_NONE,
		"chop",         "!chop!",   "", NULL
	},

	{
	"craft",
	{ 102, 102, 102,  10, 102, 102, 102, 102, 102, 102, 102, 102, 102,  20, 102 },
	{   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  90, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_craft,       SLOT( 0),   0,  36, DUR_NONE,
	"",         "!crafting!",        "", NULL
	},

	{
	"kick",  
	{  6, 102, 102, 102,  3,  5, 11, 102,  2,  1, 102, 102, 16, 3, 102 },
	{  2,  0,  0,  0,  2,  2,  3,  0,  1,  1,  0,  0, 3, 3, 0  },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
		&gsn_kick,              SLOT( 0),        0,     12, DUR_NONE,
		"kick",                 "!Kick!",       "", NULL
	},

	{
	"disarm",  
	{ 20,  3, 102, 102, 16, 18, 19, 32, 20, 102, 102, 102, 102, 102, 102 },
	{  2,  4,  0,  0,  2,  2,  2,  3,  3,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_disarm,            SLOT( 0),        0,     24, DUR_NONE,
		"",                     "!Disarm!",     "", NULL
	},
 
	{
	"uppercut",
	{ 60, 102, 102, 102, 55, 102, 102, 102, 102, 30, 102, 102, 102, 102, 102 },
	{  4,  0,  0,  0,  6,  0,  0,  0,  0,  4,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_uppercut,          SLOT(0),     0, 24, DUR_NONE,
		"uppercut",         "!uppercut!",   "", NULL
	},

	{
	"dirt kicking",  
	{ 12, 10, 102, 102, 10, 11, 102, 102, 10, 102, 102, 14,  16, 9, 102 },
	{  2,  3,  0,  0,  2,  2,  0,  0,  2,  0,  0,  6,  2,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_dirt,      SLOT( 0),   0,  24, DUR_SPECIAL,
		"kicked dirt",      "You rub the dirt out of your eyes.",
		"", NULL
	},

	{
	"gouge",
	{ 75, 70, 102, 102, 60, 102, 102, 65, 102, 102, 102, 102, 102, 102, 102 },
	{  6,  5,  0,  0,  5,  0,  0,  5,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_gouge,         SLOT(0),     0, 20, DUR_NONE,
		"gouge",            "Your eyes are restored.",  "", NULL
	},

	{
	"trip",  
	{ 16,  3, 102, 102, 13, 17, 102,  8, 12,  7, 102, 102, 102, 10, 102 },
	{  4,  2,  0,  0,  3,  4,  0,  3,  3,  2,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_trip,      SLOT( 0),   0,  24, DUR_NONE,
		"trip",         "!Trip!",       "", NULL
	},

	{
	"leg sweep",
	{ 40, 50, 102, 102, 36, 102, 102, 102, 42,  6, 102, 102, 102, 102, 102 },
	{  5,  5,  0,  0,  7,  0,  0,  0,  5,  4,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_FIGHTING,
		&gsn_leg_sweep,         SLOT(0),     0, 12, DUR_NONE,
		"leg sweep",            "!leg sweep!",  "", NULL
	},

	{
	"crush",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_crush,              SLOT( 0),       0,      24, DUR_NONE,
		"crush",                 "",       "", NULL
	},

	{
	"bash",  
	{  6, 102, 102, 102,  7,  6, 102, 50, 34, 12, 102, 102, 17, 84, 102 },
	{  5,  0,  0,  0,  4,  5,  0,  8,  8,  5,  0,  0,  7,  10, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_bash,              SLOT( 0),       0,      24, DUR_NONE,
		"bash",                 "!Bash!",       "", NULL
	},

    {
        "twohand weapons",  
        {  5, 102, 102, 102, 2,  8, 10, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  3,   0,   0,   0, 3,  3,  4,   0,   0,   0,   0,   0,   0,   0,   0 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,             TAR_IGNORE,             POS_FIGHTING,
        &gsn_two_handed,           SLOT( 0),       0,      0, DUR_NONE,
        "",                     "!Twohanded weapons!",   "", NULL
    },

    {
        "leadership",  
        {  30, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,             TAR_IGNORE,             POS_FIGHTING,
        &gsn_leadership,           SLOT( 0),       0,      0, DUR_NONE,
        "",                     "!leadership!",   "", NULL
    },


	{
	"strafe",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,  96, 102 },
	{   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    7,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	&gsn_strafe,           SLOT(0),     0, 16, DUR_NONE,
	"strafe",         "!Strafe!",    "", NULL
	},

	{
	"critical strike",
	{ 95, 102, 102, 102, 95, 95, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  5,  0,   0,   0,   5,  5,   0,   0,   0,   0,   0,   0,   0,   5,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_FIGHTING,
	&gsn_critical,           SLOT(0),     0, 0, DUR_NONE,
	"",         "!Critical Strike!",    "", NULL
	},

	{
	"infectious arrow",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 35, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	&gsn_infectious_arrow,           SLOT(0),     0, 12, DUR_SPECIAL,
	"infection",         "Your infection has been cured.",    "", NULL
	},

	{
	"fervent rage",  
	{ 102, 102, 102, 102, 92, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_fervent_rage,           SLOT( 0),       0,      8, DUR_SPECIAL,
	"",     "As your blood pressure drops you leave the state of fervent rage.",   "", NULL
	},


	{
	"fervent rage cooldown",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_fervent_rage_cooldown,           SLOT( 0),       0,      8, DUR_NONE,
	"",     "You feel like you could enter a fervent rage again.",   "", NULL
	},

	{
	"paroxysm",
	{ 102,  94, 102, 102, 102, 102, 102, 102,  94, 102, 102, 102, 102, 102, 102 },
	{   0,   7,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_FIGHTING,
	&gsn_paroxysm,      SLOT( 0),   0,  24, DUR_SPECIAL,
	"paroxysm",      "You're able to retain a combat stance again.",
	"", NULL
	},

	{
	"paroxysm cooldown",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_STANDING,
	&gsn_paroxysm_cooldown,           SLOT( 0),       0,      8, DUR_SPECIAL,
	"",     "You feel like you're able to perform another paroxysm.",   "", NULL
	},

	{
	"rupture",
	{ 102, 102, 102, 102, 102, 102, 102,  94, 102, 102, 102, 102, 102, 102, 102 },
	{    0,  0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	&gsn_rupture,      SLOT( 0),   0,  24, DUR_SPECIAL,
	"rupture",      "Your ruptured wound has healed.",
	"", NULL
	},

/*        {
        "combo attack",  
        {  30, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,             TAR_IGNORE,             POS_FIGHTING,
        &gsn_combo_attack,      SLOT( 0),       0,      0, DUR_NONE,
        "",                     "Your combat rhythm has dwindled.",   "", NULL
        },
*/

	{
	"power thrust",
	{ 60, 102, 102, 102, 55, 102, 102, 102, 102, 30, 102, 102, 102, 102, 102 },
	{  4,  0,  0,  0,  6,  0,  0,  0,  0,  4,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_power_thrust,          SLOT(0),     0, 12, DUR_NONE,
	"power thrust",         "!power thrust!",   "", NULL
	},

	{
	"quivering palm",
	{ 60, 102, 102, 102, 55, 102, 102, 102, 102, 30, 102, 102, 102, 102, 102 },
	{  4,  0,  0,  0,  6,  0,  0,  0,  0,  4,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,     TAR_IGNORE, POS_STANDING,
	&gsn_quivering_palm,          SLOT(0),     0, 18, DUR_BRIEF,
	"quivering palm",         "!quivering palm!",   "", NULL
	},



/* defensive combat skills */

	{
	"shield block",  
	{  1, 102,  1, 102,  1,  1,  1, 102, 102, 102,  1, 102, 102, 102, 102 },
	{  3,  0,  5,  0,  2,  4,  3,  0,  0,  0,  5,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_shield_block,  SLOT(0),    0,  0, DUR_NONE,
		"",         "!Shield!",     "", NULL
	},
 
	{
	"wrist shield",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_wrist_shield,  SLOT(0),    0,  0, DUR_NONE,
		"",         "!Wrist Shield!",     "", NULL
	},
 
	{
	"parry",  
	{  3, 15, 102, 102,  2,  4,  3,  8,  6, 102, 102, 102, 102, 24, 102 },
	{  4,  7,  0,  0,  4,  4,  3,  6,  5,  0,  0,  0,  0,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,            TAR_IGNORE,             POS_FIGHTING,
		&gsn_parry,             SLOT( 0),        0,     0, DUR_NONE,
		"parry",           "!Parry!",      "", NULL
	},

	{
	"dodge",  
	{  5,  4, 10,  8,  3,  4,  6,  4,  3,  2,  9,  6 , 8,  5, 8 },
	{  4,  4, 10, 12,  3,  5,  6,  4,  4,  3, 12, 10,  5,  5, 12 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_dodge,             SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!Dodge!",      "", NULL
	},
 
	{
	"guard",
	{ 24, 102, 102, 102, 25, 102, 18, 102, 102, 20, 22, 102, 102, 57, 102 },
	{  4,  0,  0,  0,  4,  0,  3,  0,  0,  5,  5,  0, 0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_guard,         SLOT(0),     0, 8, DUR_BRIEF,
		"",         "You are no longer being guarded so vigilantly.",  "", NULL
	},

	{
	"tumbling",
	{ 102, 36, 102, 102, 102, 102, 102, 102, 40, 46, 102, 102, 102, 79, 102 },
	{  0,  8,  0,  0,  0,  0,  0,  0, 10,  9,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,  POS_FIGHTING,
		&gsn_tumbling,          SLOT(0),     0, 12, DUR_SHORT,
		"tumble",           "You stop tumbling around.",    "", NULL
	},

	{
	"feint",
	{ 102, 18, 102, 102, 20, 102, 102, 22, 28, 102, 102, 102, 102, 102, 102 },
	{  0,  5,  0,  0,  7,  0,  0,  8,  9,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_feint,         SLOT(0),     0, 16, DUR_NONE,
		"",         "!feint!",  "", NULL
	},

	{
	"rescue",  
	{ 27, 102, 102, 102, 102, 28,  8, 102, 30, 20, 15, 102, 46, 63, 102 },
	{  3,  0,  0,  0,  0,  3,  2,  0,  4,  3,  2,  0,  5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_rescue,            SLOT( 0),        0,     12, DUR_NONE,
		"",                     "!Rescue!",     "", NULL
	},

	{
	"bodyguard",  
	{ 47, 102, 102, 102, 102, 48, 28, 102,  50, 40, 35, 102, 102, 102, 102 },
	{  5,  0,  0,  0,  0,  5,  4,  0,  6,  5,  4,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_bodyguard,              SLOT( 0),       0,      0, DUR_NONE,
	"bodyguard",                 "!bodyguard!",       "", NULL
	},

	{
	"back leap",  
	{ 102, 102, 102, 102, 102, 102, 102,  75, 102, 102, 102, 102, 102, 102, 102 },
	{   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_back_leap,              SLOT( 0),       0,      0, DUR_NONE,
	"back leap",                 "!back leap!",       "", NULL
	},

	{
	"distract",
	{ 102, 24, 102, 102, 102, 102, 102, 102, 34, 102, 102, 19, 102, 35, 102 },
	{  0,  6,  0,  0,  0,  0,  0,  0,  8,  0,  0,  8,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_distract,          SLOT(0),     0, 12, DUR_SPECIAL,
		"",         "!distract!",   "", NULL
	},

	{
	"avoidance",
	{ 102, 48, 102, 102, 102, 102, 102, 58, 102, 102, 102, 102, 102, 102, 102 },
	{  0, 10,  0,  0,  0,  0,  0, 12,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_RESTING,
		&gsn_avoidance,         SLOT(0),     0, 0, DUR_NONE,
		"", "!avoidance!",  "", NULL
	},

/* non-combat skills */

	{
	"hunt",
	{ 102, 55, 102, 102, 102, 102, 102, 15, 102, 102, 60, 102, 18, 10, 102 },
	{  0,  11,  0,  0,  0,  0,  0,  9,  0,  0, 15,  0, 15, 8, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_hunt,          SLOT( 0),   0,  3, DUR_NONE,
		"",         "!Hunt!",       "", NULL
	},

	{
	"path finding",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,  40, 102 },
	{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_pathfind,          SLOT( 0),   0,  3, DUR_NONE,
		"",         "!Pathfinding!",       "", NULL
	},

	{
	"street wisdom",
	{ 102,  40, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_streetwise,          SLOT( 0),   0,  3, DUR_NONE,
		"",         "!Streetwise!",       "", NULL
	},

	{
	"hide",  
	{ 102,  4, 102, 102, 102, 102, 102,  4,  4, 102, 102, 10, 19, 44, 102 },
	{  0,  2,  0,  0,  0,  0,  0,  2,  2,  0,  0,  5,  5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_hide,      SLOT( 0),    0, 12, DUR_SPECIAL,
		"",         "You come out of hiding.",       "", NULL
	},

	{
	"sneak",  
	{ 102,  5, 102, 102, 102, 102, 102,  6,  6, 102, 102, 32, 23, 45, 102 },
	{  0,  3,  0,  0,  0,  0,  0,  3,  3,  0,  0,  4,  5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_sneak,     SLOT( 0),    0, 12, DUR_SPECIAL,
		"",         "You no longer feel stealthy.", "", NULL
	},

	{
	"pick lock",  
	{ 102,  9, 102, 102, 102, 102, 102, 16, 102, 102, 102, 18, 102, 102, 102 },
	{  0,  2,  0,  0,  0,  0,  0,  3,  0,  0,  0,  3,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_pick_lock,     SLOT( 0),    0, 12, DUR_NONE,
		"",         "!Pick!",       "", NULL
	},

	{
	"disarm trap",  
	{ 102, 19, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,   2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_disarm_trap,     SLOT( 0),    0,  12, DUR_NONE,
	"",         "!Disarm Trap!",       "", NULL
	},

	{
	"peek",  
	{ 102,  6, 102, 102, 102, 102, 102, 102, 102, 102, 102, 26, 37, 102, 102 },
	{  0,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  5,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_peek,      SLOT( 0),    0,  0, DUR_NONE,
		"",         "!Peek!",       "", NULL
	},

	{
	"steal",  
	{ 102,  1, 102, 102, 102, 102, 102, 102, 102, 102, 102, 21, 84, 102, 102 },
	{  0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6, 15,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_steal,     SLOT( 0),    0, 24, DUR_NONE,
		"",         "!Steal!",      "", NULL
	},

	{
	"disguise",  
	{ 102, 11, 102, 102, 102, 102, 102,   2, 102, 102, 102, 102, 102, 102, 102 },
	{   0,  2,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 90, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_disguise,     SLOT( 0),    0, 24, DUR_NORMAL,
	"",         "Your disguise has become known.",      "", NULL
	},

	{ 
	"fast healing",  
	{  2, 10,  5, 102,  1,  4,  3,  9,  5,  4,  6, 102, 102, 21, 102 },
	{  4,  6,  4,  0,  3,  4,  4,  5,  4,  3,  4,  0,  0,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_SLEEPING,
		&gsn_fast_healing,  SLOT( 0),   0,  0, DUR_NONE,
		"",         "!Fast Healing!",   "", NULL
	},

	{
	"endurance",
	{ 13, 15, 102, 102,  6, 14, 14, 14, 14, 10, 102, 102, 9, 15, 102 },
	{  1,  2,  0,  0,  1,  1,  1,  1,  1,  1,  0,  0,  1, 1, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_SLEEPING,
		&gsn_endurance,         SLOT(0),     0, 0, DUR_NONE,
		"",         "!endurance!",  "", NULL
	},

	{
	"natural resistance",
	{ 52,  61, 102, 102, 86,  45, 54, 80, 68, 102, 102, 102, 88,  38, 102 },
	{ 100, 85,  0,   0,  70, 100, 96, 74, 91,  0,   0,   0,  66, 100,  0  },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,     TAR_IGNORE, POS_STANDING,
	&gsn_natural_resistance,         SLOT(781),     0, 0, DUR_NONE,
	"",        "!Natural Resistance!",  "", NULL
	},

	{
	"iron hide",
	{  14, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,        TAR_IGNORE,         POS_STANDING,
	&gsn_iron_hide,    SLOT(783),          0, 0, DUR_NONE,
	"",        "!Iron Hide!",  "", NULL
	},

	{
	"meditation",  
	{ 102, 102,  2,  3, 102,  6,  5, 102,  6,  1,  3,  4, 102, 35, 1 },
	{  0,  0,  4,  5,  0,  8,  7,  0,  8,  2,  5,  6,  0,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_SLEEPING,
		&gsn_meditation,    SLOT( 0),   0,  0, DUR_NONE,
		"",         "Meditation",       "", NULL
	},

	{ 
	"sustenance",
	{ 102, 102, 102, 102, 20, 102, 102, 102, 102, 20, 102, 102, 11, 13, 22 },
	{  0,  0,  0,  0,  2,  0,  0,  0,  0,  2,  0,  0,  2,  2, 4 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_SLEEPING,
		&gsn_sustenance,    SLOT( 0),   0,  0, DUR_NONE,
		"",         "!Sustenance!", "", NULL
	},

	{
	"lore",  
	{ 102, 12, 10,  8, 102, 102, 102, 102, 102,  8, 12, 12, 56, 102, 8 },
	{  0,  6,  6,  6,  0,  0,  0,  0,  0,  6,  7,  7,  10,  0, 6 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_lore,      SLOT( 0),   0,  36, DUR_NONE,
		"",         "!Lore!",       "", NULL
	},

	{
	"arcane lore",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_arcane_lore,      SLOT( 0),   0,  36, DUR_NONE,
		"",         "!Arcane Lore!",       "", NULL
	},

	{
	"weapons lore",
	{ 16, 102, 102, 102, 102, 16, 20, 20, 18, 102, 102, 102, 102, 27, 102 },
	{  3,  0,  0,  0,  0,  3,  4,  4,  3,  0,  0,  0,  0,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_OBJ_INV,    POS_RESTING,
		&gsn_weapons_lore,          SLOT(0),     0, 20, DUR_NONE,
		"",         "!weapons lore!",   "", NULL
	},

	{
	"appraise",
	{  6,  3,  6,  6, 102,  6, 102,  6,  6, 102,  6,  5,  102, 102, 6 },
	{  2,  1,  2,  2,  0,  2,  0,  2,  2,  0,  2,  2,  0,  0, 2 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_OBJ_INV,    POS_RESTING,
		&gsn_appraise,          SLOT(0),     0, 20, DUR_NONE,
		"",         "!appraise!",   "", NULL
	},

	{
	"haggle",  
	{ 14, 13, 102, 15, 102, 14, 102, 14, 15, 102, 13, 13, 102, 102, 15 },
	{  5,  4,  0,  6,  0,  5,  0,  5,  6,  0,  4,  4,  0,  0, 6 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_haggle,        SLOT( 0),   0,  0, DUR_NONE,
		"",         "!Haggle!",     "", NULL
	},

	{
	"envenom",  
	{ 102,  7, 102, 102, 102, 102, 102,  3, 102, 102, 102, 102, 102, 23, 102 },
	{  0,  3,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_envenom,       SLOT(0),    0,  12, DUR_LONG,
		"",         "!Envenom!",        "", NULL
	},

	{
	"paralysis poison",  
	{ 102,  7, 102, 102, 102, 102, 102,  3, 102, 102, 102, 102, 102, 23, 102 },
	{  0,  3,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	&gsn_paralysis_poison,       SLOT(0),    0,  12, DUR_LONG,
	"paralysis",         "Your body cleanses itself of the paralysis poison.",        "", NULL
	},


	{
	"ignite",
	{ 102, 28, 102, 102, 102, 102, 102, 11, 102, 102, 102, 102, 38, 102, 102 },
	{  0,  4,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  4,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_ignite,        SLOT(0),    0,  12, DUR_NONE,
		"",         "!Ignite!", "", NULL
	},

	{               
	"scrolls",   
	{ 102, 20,  1,  1, 102, 20, 102, 102, 16,  1,  1,  1, 102, 102, 1 },
	{  0, 10,  1,  1,  0,  9,  0,  0,  6,  1,  1,  1,  0,  0, 1 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_scrolls,       SLOT( 0),   0,  24, DUR_NONE,
		"",         "!Scrolls!",        "", NULL
	},

	{
	"scribe",
	{ 102, 102, 102,  40, 102, 102, 102, 102, 102, 102, 102,  55, 102, 102, 102 },
	{   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   8,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  80, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_scribe,       SLOT( 0),   0,  24, DUR_NONE,
	"",         "!Scribe!",        "", NULL
	},

	{
	"alchemy",
	{ 102, 102, 102,  10, 102, 102, 102, 102, 102, 102, 102, 102, 102,  20, 102 },
	{   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,  90, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_alchemy,       SLOT( 0),   0,  36, DUR_NONE,
	"",         "!Alchemy!",        "", NULL
	},

	{
	"staves",
	{ 12,  9,  1,  1, 102,  8, 10, 102, 102,  3,  4,  4, 102, 15, 1 },
	{ 10,  8,  2,  2,  0,  6,  9,  0,  0,  2,  3,  3,  0,  5, 2 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_staves,        SLOT( 0),   0,  12, DUR_NONE,
		"",         "!Staves!",     "", NULL
	},
	
	{
	"wands",  
	{  5,  2,  1,  1, 102,  2,  2,  3,  2,  1,  1,  1, 102, 30, 1 },
	{  4,  3,  1,  1,  0,  2,  3,  4,  2,  2,  1,  1,  0,  5, 1 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_wands,     SLOT( 0),   0,  12, DUR_NONE,
		"",         "!Wands!",      "", NULL
	},

	/* four new skills by mephiston 4/15/98, added by Siva */
	{
	"regeneration",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,      TAR_IGNORE,       POS_SLEEPING,
		&gsn_regeneration, SLOT( 0),   0,  0, DUR_NONE,
		"",     "!Regeneration!",  "", NULL
	},

	{
	"drain life",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,      TAR_IGNORE,       POS_SLEEPING,
		&gsn_drain_life, SLOT( 0),   0,  0, DUR_NONE,
		"",     "!DRAIN LIFE!",  "", NULL
	},

	{
	"headbutt",
	{ 102, 102, 102, 102, 75, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{ 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,         TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_headbutt,      SLOT( 0),   0,  12, DUR_NONE,
		"headbutt",         "!Headbutt!",   "", NULL
	},

	{
	"net",
	{ 102, 102, 102, 102, 65, 102, 102, 102, 102, 102, 102, 102, 102, 16, 102 },
	{ 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
		&gsn_net,      SLOT( 0),   0,  12, DUR_SPECIAL,
		"thrown net", "You break free of the net.", "",  NULL
	 },

	{
	"mug",
	{ 102, 15, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{ 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,      TAR_CHAR_OFFENSIVE,      POS_FIGHTING,
		&gsn_mug,        SLOT( 0),   0, 12, DUR_NONE,
		"mugging",       "!Mug!",    "", NULL
	},

	{
	"alertness",  
	{ 102, 5, 102, 102, 102, 102, 102, 10, 15, 102, 102, 40, 20, 30, 102 },
	{  0,  2,   0,   0,   0,   0,   0,  3,  3,   0,   0,  4,  3,  3,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_alertness,             SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!Alertness!",      "", NULL
	},

    {
        "evasive action",  
        { 50, 102, 102, 102, 40, 30, 102, 102, 20, 15, 102, 102, 102, 35, 102 },
        {  6,   0,   0,   0,  5,  4,   0,   0,  3,  3,   0,   0,   0,  5,   0 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,             TAR_IGNORE,             POS_FIGHTING,
        &gsn_evasive,             SLOT( 0),        0,     0, DUR_NONE,
        "",                     "!Evasive Action!",      "", NULL
    },

    {
        "fatal blow",
        { 80, 102, 102, 102, 70, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  8,   0,   0,   0,  7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
        { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
        STAT_VIT, STAT_STR, STAT_DEX,
        spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
        &gsn_fatal_blow,          SLOT(0),     0, 24, DUR_NONE,
        "fatal blow",         "!fatal blow!",   "", NULL
    },

	/*GUNSLINGER SKILLS BY SIVA 4/15/98*/
	{
	"set snare",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 90, 70, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_set_snare,     SLOT(0),    0,  24, DUR_NONE,
	"",         "!snare!",  ""
	},

	{
	"aim",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 50, 102, 102 },
	{  0, 0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  5,  0, 0  },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_aim,              SLOT( 0),       0,      24, DUR_NONE,
		"aimed shot",                 "!aim!",       "", NULL
	},

	{
	"semiauto",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 45, 102, 102 },
	{  0, 0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  6, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_semiauto,              SLOT( 0),       0,      15, DUR_NONE,
		"semi auto spray",                 "!semiauto!",       "", NULL
	},
	
	{
	"fullauto",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 70, 102, 102 },
	{  0, 0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  6,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_fullauto,          SLOT( 0),       0,      18, DUR_NONE,
		"full auto spray",      "!fullauto!",       "", NULL
	},
	
	{
	"hogtie",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 22, 102, 102 },
	{  0, 0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  5,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_hogtie,              SLOT( 0),       0,      8, DUR_BRIEF,
		"hogtie",  "You free yourself from the hogtie.",       "", NULL
	},

	{
	"elude",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 42, 72, 102 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,      TAR_IGNORE,       POS_STANDING,
		&gsn_elude, SLOT( 0),   0,  0, DUR_NONE,
		"",     "!elude!",  "", NULL
	},

	{
	"estimate",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 57, 66, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_estimate,     SLOT( 0),    0, 24, DUR_NONE,
		"",         "!estimate!",       "", NULL
	},

	{                                       
	"shoot lock",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 33, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_shoot_lock,     SLOT( 0),    0, 12, DUR_NONE,
		"",         "!shootlock!",       "", NULL
	},

	{
	"unjam",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,  1, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_RESTING,
		&gsn_unjam,      SLOT( 0),    0, 6, DUR_NONE,
		"",         "!unjam!",       "", NULL
	},

	{
	"quick draw",
	{ 102, 102, 102, 102, 102 , 102, 102, 102, 102, 102, 102, 102, 2, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_STANDING,
		&gsn_quick_draw,         SLOT(0),     0, 0, DUR_NONE,
		"quick draw",         "!quickdraw!",  "", NULL
	},

	{
	"1000-yard stare",
	{ 102, 102, 102, 102, 102 , 102, 102, 102, 102, 102, 102, 102, 23, 33, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_STANDING,
		&gsn_thousand_yard_stare,         SLOT(0),     0, 0, DUR_NONE,
		"",         "!1000yrdstare!",  "", NULL
	},
	
	{
	"pistol whip",
	{ 102, 102, 102, 102, 102 , 102, 102, 102, 102, 102, 102, 102, 2, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE, POS_STANDING,
		&gsn_pistol_whip,         SLOT(0),     0, 0, DUR_NONE,
		"pistol whip",         "!pistolwhip!",  "", NULL
	},

	{
	"snipe",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 30, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_STANDING,
		&gsn_snipe,          SLOT( 0),        0,     24, DUR_NONE,
		"snipe",             "!snipe!",       "", NULL
	},

	{
	"burst",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 15, 102, 102 },
	{  0, 0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0,  5,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_burst,              SLOT( 0),       0,      12, DUR_NONE,
		"burst",                 "!burst!",       "", NULL
	},

	{
	"tight grouping",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 39, 102, 102 },
	{   0,   0,   0,  0,   0,   0,   0,   0,    0,   0,  0,   0,   6,   0,  0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_tight_grouping,    SLOT( 0),              0,      12, DUR_NONE,
		"tight grouping",       "!tight grouping!",     "", NULL
	},

	{
	"duck",  
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 10, 102, 102 },
	{  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_duck,             SLOT( 0),        0,     0, DUR_NONE,
		"",                     "!Duck!",      "", NULL
	},

    {
    "true grit",
    { 102, 102, 102, 102,  90, 102, 102, 102, 102, 102, 102, 102,  75, 102, 102 },
    {   0,   0,   0,   0,   8,   0,   0,   0,   0,   0,   0,   0,   8,   0,  0  },
    { 100, 100, 100, 100,  80, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
    STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,             TAR_IGNORE,             POS_FIGHTING,
        &gsn_true_grit,         SLOT( 0),        0,     0, DUR_NONE,
        "",                     "!true grit!",      "", NULL
    },

	{
	"drunken fury",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 20, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_drunken_fury,           SLOT( 0),       0,      12, DUR_BRIEF,
		"",     "You feel less furious.",   "", NULL
	},

	{
	"beheading",  
	{ 75, 102, 102, 102, 40, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  7,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_beheading,           SLOT( 0),       0,      0, DUR_NONE,
		"{RBEHEADING{x",     "",   "", NULL
	},

	{
	"swimming",  
	{ 20, 15, 25, 30, 38, 22, 24, 27, 16, 25, 28, 32, 43,  2, 51 },
	{  4,  3,  4,  4,  5,  4,  4,  4,  4,  4,  4,  4,  5,  1,  6 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_STANDING,
		&gsn_swimming,           SLOT( 0),       0,      0, DUR_NONE,
		"drowning",     "!swim!",   "", NULL
	},

	{
	"maul",  
	{102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_maul,           SLOT( 0),       0,      0, DUR_NONE,
		"mauling",     "!maul!",   "", NULL
	},

	{
	"extra attack",  
	{102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_extra_attack,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "!extra attack!",   "", NULL
	},

	{
	"bear",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 48, 29, 102, 102, 102, 71, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_bear,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"boa",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 66, 53, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_boa,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"bunny",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_bunny,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"dragon",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 47, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_dragon,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"eagle",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 54, 35, 102, 102, 102, 67, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_eagle,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"eel",  
	{ 102, 102, 102, 102, 102, 77, 102, 102, 60, 41, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  4,  0,  0,  3,  3,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_eel,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"lion",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 90, 89, 102, 102, 102, 87, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  0,  4, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_lion,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"phoenix",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 65, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_phoenix,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"porcupine",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 84, 77, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_porcupine,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"rhino",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 78, 71, 102, 102, 102, 84, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_rhino,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"serpent",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102,  89, 102, 102, 102, 102, 102 },
	{   0,   0,   0,   0,   0,   0,   0,   0,   3,   3,   0,   0,   0,   4,  0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_serpent,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"scorpion",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 36, 17, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_scorpion,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"tiger",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 72, 59, 102, 102, 102, 77, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_tiger,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"toad",  
	{ 102, 102, 102, 102, 102, 41, 102, 102, 30, 11, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  4,  0,  0,  3,  3,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_toad,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"tortoise",  
	{ 102, 102, 102, 102, 102, 59, 102, 102, 42, 23, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  4,  0,  0,  3,  3,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_tortoise,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"unicorn",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 83, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_unicorn,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

        {
	"aversion",  
	{  92, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   6,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_aversion,  SLOT( 0),       0,      0, DUR_NONE,
	"",     "",   "", NULL
	},

	{
	"dimensional blade",  
	{ 102, 102, 102, 102, 102, 85, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_dimensional_blade,  SLOT( 0),       0,      0, DUR_NONE,
	"",     "",   "", NULL
	},

	{
	"elemental blade",  
	{ 102, 102, 102, 102, 102, 75, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_elemental_blade,  SLOT( 0),       0,      0, DUR_NONE,
	"elemental strike",     "",   "", NULL
	},

	{
	"finesse",  
	{ 32, 37, 102, 102, 52, 33, 41, 102, 26, 102, 102, 102, 102, 102, 102 },
	{  4,  6,  0,  0,  8,  4,  5,  0,  3,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_finesse,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"rage",  
	{ 41, 102, 102, 102, 44, 38, 102, 102, 54, 102, 102, 102, 102, 102, 102 },
	{  4,  0,  0,  0,  6,  4,  0,  0,  4,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_rage,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"retribution",  
	{ 102, 102, 102, 102, 79, 57, 73, 66, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  6,  3,  5,  6,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_retribution,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"blade dance",  
	{  63, 102, 102, 102, 64, 73, 102, 102, 71, 102, 102, 102, 102, 102, 102 },
	{  5,  0,  0,  0,  5,  6,  0,  0,  5,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_blade_dance,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"shadowclaw",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 26, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_shadowclaw,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"shadowessence",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 37, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_shadowessence,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"shadowsoul",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 41, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_shadowsoul,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"shadowwalk",  
	{ 102, 53, 102, 102, 102, 102, 102, 102, 21, 102, 102, 47, 102, 102, 102 },
	{  0,  5,  0,  0,  0,  0,  0,  0,  3,  0,  0,  4,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_shadowwalk,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"shadow shroud",  
	{ 102,  92, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_shadow_shroud,    TAR_CHAR_SELF,     POS_FIGHTING,
	&gsn_shadow_shroud,           SLOT(739),    60, 20, DUR_NORMAL,
	"aura of darkness",         "You are no longer surrounded by darkness.",  "", NULL
	},

	{
	"ambush",  
	{ 102, 78, 102, 102, 102, 102, 102, 57, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  8,  0,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_ambush,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"anklebiter",  
	{ 102, 30, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_anklebiter,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"arcana",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  6,  0,  0,  0,  0,  0,  0,  0,  6,  0,  0, 6 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_arcana,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"stalk",  
	{ 102, 102, 102, 102, 102, 102, 102, 33, 102, 102, 102, 102, 102, 80, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  9, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_stalk,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"bloodbath",  
	{ 102, 102, 102, 102, 65, 102, 102, 80, 102, 102, 102, 102, 82, 102, 102 },
	{  0,  0,  0,  0,  6,  0,  0,  6,  0,  0,  0,  0,  4,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_bloodbath,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"kamikaze",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_kamikaze,           SLOT( 0),       0,      0, DUR_NONE,
		"suicidal devotion",     "",   "", NULL
	},

	{
	"showdown",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 19, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_showdown,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"target practice",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 37, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_target_practice,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"jihad",  
	{ 102, 102, 102, 102, 102, 102, 90, 102, 102, 102, 29, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  4,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_jihad,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"vampire hunting",  
	{ 102, 102, 102, 102, 102, 102, 12, 102, 102, 102, 25, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  4,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_vampire_hunting,           SLOT( 0),       0,      0, DUR_NONE,
		"stake through the heart",     "",   "", NULL
	},

	{
	"witch hunting",  
	{ 102, 102, 102, 102, 102, 102, 16, 102, 102, 102, 23, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  4,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_witch_hunting,           SLOT( 0),       0,      0, DUR_NONE,
		"drowning",     "",   "", NULL
	},

	{
	"werewolf hunting",  
	{ 102, 102, 102, 102, 102, 102, 20, 102, 102, 102, 27, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  4,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_werewolf_hunting,           SLOT( 0),       0,      0, DUR_NONE,
		"silver bullet",     "",   "", NULL
	},

	{
	"inquisition",  
	{ 102, 102, 102, 102, 102, 102, 50, 102, 102, 102, 21, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  4,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_inquisition,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"forage",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 19, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 2, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_forage,    SLOT(0),    0,  12, DUR_NONE,
	"",         "!Forage!", ""
	},

	{
	"torch",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 17, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  2, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_torch,     SLOT(0),    0,  36, DUR_SPECIAL,
	"sunburn",         "!Torch!",  ""
	},

	{
	"shelter",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 37, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_shelter,   SLOT(0),    0,  36, DUR_NORMAL,
	"",         "Your shelter falls apart.",    ""
	},

	{
	"firstaid",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 31, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 5, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_firstaid,  SLOT(0),   40,  24, DUR_NONE,
	"",         "!FirstAid!",   ""
	},

	{
	"detoxify",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 7, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 1, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_detoxify,  SLOT(0),    0,  24, DUR_NONE,
	"",         "!Detoxify!",   ""
	},
 
	{
	"tame",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 33, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE,     POS_STANDING,
	&gsn_tame,      SLOT(0),    0,  12, DUR_BRIEF,
	"",         "!Tame!",   ""
	},

	/* More skills by Mephiston!  Added by Quirky, Nov 7/98 */

	{
	"shield bash",
	{  26, 102, 26, 102,  26,  26, 26, 102, 102, 102, 26, 102, 102, 0, 102  },
	{  5,  0,  5,  0,  5,  5,  5,  0,  0,  0,  5,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_shield_bash,              SLOT( 0),       0,      24, DUR_NONE,
	"shield bash",                 "!shield bash!",       "", NULL
	},

	{
	"choke hold",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 20, 25, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  3,  4,  0,  0, 0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_choke_hold,         SLOT(0),     0, 8, DUR_SPECIAL,
	"choke hold",         "You can breathe again.",  "", NULL
	},

	{
	"roundhouse",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 30,  30, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  5,  5,  0,  0, 0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_IGNORE, POS_FIGHTING,
	&gsn_roundhouse,         SLOT(0),     0, 12, DUR_NONE,
	"roundhouse",            "!roundhouse!",  "", NULL
	},

	{
	"hurl",
	{ 102, 102, 102, 102, 102, 102, 102, 102, 55, 40, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  5,  5,  0,  0, 0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_hurl,          SLOT(0),     0, 12, DUR_NONE,
	"hurl",         "!hurl!",   "", NULL
	},

	{
	"spit",
	{  102, 21, 102, 102, 28, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  4,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,             TAR_CHAR_OFFENSIVE,    POS_FIGHTING,
	&gsn_spit,              SLOT( 0),        0,     12, DUR_SPECIAL,
	"spit",                 "You rub the spit out of your eyes.",       "", NULL
	},

/* Added by Tryste */
	{
	"hand of god",
        { 102, 102, 80, 102, 102, 102, 60, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3 },
 	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
 	STAT_NONE, STAT_NONE, STAT_NONE,
      spell_hand_of_god,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
        NULL,           SLOT(500),  40, 12, DUR_NONE,
        "wrath of god",        "!HandOfGod!",    "", NULL
        },

        {   
        "heroism",
        { 102, 102,  102, 102, 102, 102, 40, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
 	STAT_NONE, STAT_NONE, STAT_NONE,
       spell_heroism,        TAR_CHAR_SELF,   POS_STANDING,
        NULL,           SLOT( 3),   75 , 12, DUR_NORMAL,
        "",         "You feel less heroic.",
        "$p's looks less heroic.", NULL
        },

        {
        "deaths door",
        { 102, 102,  35, 102, 102, 102, 80, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_deaths_door,    TAR_CHAR_SELF, POS_STANDING,
        &gsn_deaths_door,     SLOT(36),   300, 12, DUR_LONG,
        "",         "The gods lose interest in you.",
        "", NULL
        },

        { 
        "sticks to snakes",
        { 102, 102, 75, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 37, 102 },
        {  0,  0,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_sticks_to_snakes,  TAR_IGNORE, POS_STANDING,
        NULL,           SLOT(2000),   150, 24, DUR_LONG,
        "sticks to snakes",     "!SticksToSnakes!",       "", NULL
        },

	{ 
        "laughing fit",
        { 102, 80, 102, 102, 102, 102, 102, 102, 102, 102, 102, 70, 102, 102, 102 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_laughing_fit,       TAR_CHAR_OFFENSIVE, POS_FIGHTING,
        &gsn_laughing_fit,        SLOT(503),  40, 12, DUR_SHORT,
        "uncontrollable laughter",     "You become sane.",   "", NULL
        },

        {
        "peel",
        { 102, 55, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_null,     TAR_IGNORE,     POS_STANDING,
        &gsn_peel,     SLOT(0),    0,  24, DUR_NONE,
        "",         "!peel!",  ""
        },


        {
        "blessed darkness",
        { 102, 102,  102, 102, 102, 102, 102, 102, 102,  102, 102, 102, 102, 102, 10 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_blessed_darkness,        TAR_CHAR_DEFENSIVE,   POS_STANDING,
        &gsn_blessed_darkness,           SLOT( 2110),    5, 12, DUR_NORMAL,
        "",         "The darkness leaves you.",
        "$p's darkness leaves him.", NULL
        },

        {
        "glyph of evil",
        { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 55 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_glyph_of_evil,    TAR_IGNORE, POS_FIGHTING,
        NULL,           SLOT(2111),  200,    24, DUR_NONE,
        "demonic wrath",     "!Glyph of Evil!",      "", NULL
        },

        { 
        "tomb rot",
        { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 45 },
        {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_tomb_rot,       TAR_CHAR_OFFENSIVE, POS_FIGHTING,
        &gsn_tomb_rot,        SLOT(2112),  20, 12, DUR_SHORT,
        "rot",     "You feel alive again.",   "", NULL
        },

        {
        "soreness",
        { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
        {  3,  3,  3,  3,   3,   3,   3,   3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_soreness,        TAR_CHAR_OFFENSIVE, POS_FIGHTING,
        NULL,                  SLOT(2113),  30,  12, DUR_SHORT,
        "",              "You feel surprisingly better.",  "", NULL    
        },

        {
        "haunt",
        { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 70 },
        {  3,  3,  3,  3,   3,   3,   3,   3,  3,  3,  3,  3,  3,  3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_haunt,        TAR_CHAR_OFFENSIVE, POS_FIGHTING,
        NULL,                  SLOT(2114),  30,  12, DUR_SHORT,
        "",              "The spirits loose track of you.",  "", NULL    
        },

        {
        "dancing bones",
        { 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 38 },
        {  3,  3,  3,  3,   3,   3,   3,   3,  3,  3,  3,  3,  3,  3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
        spell_dancing_bones,    TAR_IGNORE, POS_FIGHTING,
        NULL,                  SLOT(2115),  10,  12, DUR_NONE,
        "",              "!DANCING BONES!",  "", NULL    
        },

/*Coded in by Korinn 1-15-99*/
	{
	"mephistons scrutiny", 
	{ 102, 102, 65, 102, 102, 102, 55, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_mephistons_scrutiny,        TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL,           SLOT(2042),    50, 12, DUR_NONE,
	"",         "Your prayers are no longer heard by Mephiston.","", NULL
	},

	{
	"rimbols invocation", 
	{ 102, 102, 93, 102, 102, 102, 94, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_rimbols_invocation,   TAR_IGNORE, POS_FIGHTING,
	NULL,           SLOT(2043),   125, 24, DUR_NONE,
	"rimbols invocation",   "!Rimbols Invocation!", "", NULL
	},
	
	{
	"korinns inspiration",  
	{ 102, 102, 43, 102, 102, 102, 30, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  4,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_korinns_inspiration,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"parademias bile",  
	{ 102, 102, 33, 102, 102, 102, 43, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  4,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_parademias_bile,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"swaydes mercy",  
	{ 102, 102, 25, 102, 102, 102, 20, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  4,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_swaydes_mercy,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"quirkys insanity",  
	{ 102, 102, 50, 102, 102, 102, 50, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 3 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_quirkys_insanity,    TAR_CHAR_SELF,     POS_FIGHTING,
	&gsn_quirkys_insanity,           SLOT(2044),    60, 24, DUR_SHORT,
	"insane aura",         "You are no longer surrounded by insanity.",  "", NULL
	},
	
	{
	"firewitchs seance",  
	{ 102, 102, 85, 102, 102, 102, 85, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  4,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,             TAR_IGNORE,             POS_FIGHTING,
		&gsn_firewitchs_seance,           SLOT( 0),       0,      0, DUR_NONE,
		"",     "",   "", NULL
	},

	{
	"sivas sacrifice", 
	{ 102, 102, 70, 102, 102, 102, 79, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 4 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_sivas_sacrifice,     TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	NULL,           SLOT(2045),   100, 36, DUR_NONE,
	"sivas sacrifice",       "!Sivas Sacrifice!",       "", NULL
	},

	{
	"smotes anachronism", 
	{ 102, 102, 35, 102, 102, 102, 35, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 4 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_smotes_anachronism,     TAR_IGNORE, POS_FIGHTING,
	&gsn_smotes_anachronism,           SLOT(2046),   60, 36, DUR_NONE,
	"smotes anachronism",       "!Smotes Anachronism!",       "", NULL
	},

	{
	"breathe water",  
	{ 102, 102, 44, 62, 102, 102, 102, 102, 102, 102, 102, 102, 102, 14, 102 },
	{  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_breathe_water,       TAR_CHAR_DEFENSIVE, POS_STANDING,
	NULL,           SLOT(2047),   16, 18, DUR_NORMAL,
	"breathe water",         "You can no longer breathe water.",
	"", NULL
	},


	/* mana recovery spell for use in objects only */
	{
	"mana heal",
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_mana_heal,        TAR_CHAR_SELF,    POS_FIGHTING,
	NULL,              SLOT(798),        50,     12, DUR_NONE,
	"",                 "!mana!",       "", NULL
	},

	{
	"flee",   
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_flee,        SLOT( 0),   0,  12, DUR_NONE,
		"",         "!Flee!",     "", NULL
	},
	
	{
	"retreat",   
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_retreat,        SLOT( 0),   0,  12, DUR_NONE,
		"",         "!Retreat!",     "", NULL
	},
	
	{
	"entrapment",   
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_entrapment,        SLOT( 0),   0,  0, DUR_NONE,
		"",         "!Entrapment!",     "", NULL
	},
	
	{ /* misuse for relic damage */
	"focus",   
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_focus,        SLOT( 0),   0,  0, DUR_NONE,
		"relic",         "!Focus!",     "", NULL
	},

	{
	"anatomy",
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_anatomy,        SLOT( 0),   0,  0, DUR_NONE,
		"",         "!Anatomy!",     "", NULL
	},

	{
	"puppetry",
	{  102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_FIGHTING,
		&gsn_puppetry,        SLOT( 0),   0,  0, DUR_NONE,
		"",         "!Puppetry!",     "", NULL
	},

	{
	"intimidation",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 55, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  0,  0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
	&gsn_intimidation,           SLOT(0),  0, 18, DUR_NONE,
	""          "intimidation",   "", NULL
	},

	{
	"dowsing",  
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 13, 42, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  3,  0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,    TAR_IGNORE,     POS_STANDING,
	&gsn_dowsing,           SLOT(0),   0, 12, DUR_NONE,
	"",         "!dowsing!",  "", NULL
	},

	{
	"rustle grub", 
	{ 102, 102, 102, 102, 102, 102, 102,  20, 102, 102, 102, 102,  12, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,    TAR_IGNORE,     POS_STANDING,
	&gsn_rustle_grub,           SLOT(0),   0, 12, DUR_NONE,
	"",         "!rustle grub!",  "", NULL
	},

	{
	"fledging", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102,  12, 102 },
	{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,    TAR_IGNORE,     POS_STANDING,
	&gsn_fledging,           SLOT(0),   0, 24, DUR_NONE,
	"",         "!fledging!",  "", NULL
	},

	{
	"survey", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,    TAR_IGNORE,     POS_STANDING,
	&gsn_survey,           SLOT(0),   0, 6, DUR_NONE,
	"",         "!survey!",  "", NULL
	},

	{
	"charge", 
	{ 38,  102, 102, 102,  35,  52,  44, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  3,    0,   0,   0,   3,   4,   4,   0,   0,   0,   0,   0,   0,   0,   0 },
	{ 100, 100, 100, 100, 100,  88,  90, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,    TAR_IGNORE,     POS_STANDING,
	&gsn_charge,           SLOT(0),   0, 24, DUR_NONE,
	"charge",         "!charge!",  "", NULL
	},

	{
	"god_bless", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,    TAR_IGNORE,     POS_STANDING,
	&gsn_god_bless,           SLOT(0),   0, 0, DUR_SPECIAL,
	"divine blessing",   "The blessing of the gods has ended.",  "", NULL
	},

	{
	"god_curse", 
	{ 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102, 102 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
	spell_null,    TAR_IGNORE,     POS_STANDING,
	&gsn_god_curse,           SLOT(0),   0, 0, DUR_SPECIAL,
	"divine curse",   "The curse of the gods has ended.",  "", NULL
	},

	{
	"recall",   
	{  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 1,  1, 1 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0 },
	{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
	STAT_NONE, STAT_NONE, STAT_NONE,
		spell_null,     TAR_IGNORE,     POS_STANDING,
		&gsn_recall,        SLOT( 0),   0,  12, DUR_NONE,
		"",         "!Recall!",     "", NULL
	},


	{NULL}

};

const   struct  group_type      group_table     [MAX_GROUP]     =
{

	{
	"rom basics",   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ "recall" }
	},

	{
	"mage basics",      { -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ "polearm", "wands", "scrolls" }
	},

	{
	"cleric basics",    { -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ "mace", "scrolls" }
	},
   
	{
	"thief basics",     { -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ "dagger", "steal", "wands" }
	},

	{
	"warrior basics",   { 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 , -1, -1, -1 },
	{ "sword", "second attack" }
	},

	{
	"gladiator basics",   { -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{
	"axe", "second attack"
	} },

	{
	"samurai basics",   { -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{
	"sword", "second attack"
	} },

	{
	"paladin basics",   { -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1 },
	{
	"mace", "second attack"
	} },

	{
	"assassin basics",   { -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1 },
	{
	"dagger", "backstab"
	} },

	{
	"ninja basics",   { -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1 },
	{
	"sword", "wands"
	} },

	{
	"monk basics",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1 },
	{
	"hand to hand", "scrolls", "meditation"
	} },

	{
	"templar basics",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1 },
	{
	"flail", "scrolls"
	} },

	{
	"illusionist basics",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1 },
	{
	"dagger", "wands", "scrolls"
	} },

	{
	"gunslinger basics",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1 },
	{
	"gun", "unjam"
	} },
	
	{
	"ranger basics",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1 },
	{
	"axe", "hunt"
	} },
	
	{
	"necromancer basics",      { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0 },
	{ "meditation", "polearm", "wands" }
	},
	

	{
	"mage default",     { -1, -1, -1, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ "lore", "combat", "detection", "enchantment", "enhancement",
	  "maledictions", "protective", "transportation", "staves" }
	},

	{
	"cleric default",   { -1, -1, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ "flail", "meditation", "shield block", "curative",  "benedictions", 
	  "creation", "healing",  "holy rites" }
	},
 
	{
	"thief default",    { -1, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ "feint", "distract", "backstab", "circle", "dodge", "second attack",
	  "trip", "hide", "peek", "pick lock", "sneak", "appraise" }
	},

	{
	"warrior default",  { 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ "weaponsmaster", "shield block", "bash", "disarm", "enhanced damage", 
	  "parry", "rescue", "third attack", "dirt kick" }
	},

	{
	"gladiator default",  { -1, -1, -1, -1, 50, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{
	  "weaponsmaster", "disarm", "dirt kick", "dodge", "enhanced damage",
	  "bite", "dual wield", "third attack", "trip", "fast healing"
	} },
	
	{
	"samurai default",  { -1, -1, -1, -1, -1, 50, -1, -1, -1, -1, -1, -1,  -1, -1, -1 },
	{
	  "martial arts", "third attack", "enhanced damage", "polearm",
	  "parry", "fast healing", "weapons lore", "disarm"
	} },
	
	{
	"paladin default",  { -1, -1, -1, -1, -1, -1, 50, -1, -1, -1, -1, -1, -1, -1, -1 },
	{
	  "shield block", "guard", "endurance", "parry", "rescue",
	  "third attack", "fast healing", "curative", "healing",
	  "protective"
	} },
	
	{
	"assassin default",  { -1, -1, -1, -1, -1, -1, -1, 50, -1, -1, -1, -1,  -1, -1, -1 },
	{
	  "circle", "dodge", "enhanced damage", "envenom", "unibomber", "hide",
	  "sneak", "second attack", "third attack"
	} },
	
	{
	"ninja default",  { -1, -1, -1, -1, -1, -1, -1, -1, 50, -1, -1, -1,  -1, -1, -1 },
	{
	  "martial arts", "hide", "sneak", "second attack", "combat"
	} },
	
	{
	"monk default",  { -1, -1, -1, -1, -1, -1, -1, -1, -1, 50, -1, -1,  -1, -1, -1 },
	{
	  "martial arts", "second attack", "protective", "healing", "harmful"
	} },
	
	{
	"templar default",  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50, -1,  -1, -1, -1 },
	{
	  "combat", "enhancement", "healing", "fast healing", "lore",
	  "shield block", "guard"
	} },
	
	{
	"illusionist default",  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50, -1, -1, -1 },
	{
	  "illusions", "beguiling", "weather", "transportation", "detection",
	  "protective", "hide", "sneak", "distract", "dirt kick", "pick lock"
	} },
		
	{
	"gunslinger default",  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50, -1, -1 },
	{
	  "warfare", "survival", "practical shooting"
	} },

	{
	"ranger default",  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50, -1 },
	{
	  "weather", "elemental", "dodge", "trip", "second attack", "introspection",
	  "kick", "woodland combat", "raft", "sustenance", "endurance", "forage"
	} },
	   
	{
	"necromancer default",     { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50 },
	{  
	    "black magics", "enhancement", "debilitations", "dark rituals",
	    "protective", "transportation", "necromancy" 
	} },

	{
	"weaponsmaster",    { 20, 40, -1, -1, 20, 20, 20, 30, 30, -1, -1, -1, -1, -1, -1 },
	{ "axe", "dagger", "flail", "mace", "polearm", "spear", "sword", "whip", "gun" }
	},

	{
	"martial arts", { 36, -1, -1, -1, 40, 20, -1, -1, 25, 20, -1, -1, -1, -1, -1 },
	{ "brawl", "chop", "dodge", "hand to hand", "kick", "kung fu", "leg sweep",
	  "trip", "uppercut", "evasive action" }
	},

	{
	"attack",       { -1, -1, 8, 7, -1, 12, 10, -1, 9, -1, 10, -1, -1, -1, -1 },
	{ "angel smite", "demonfire", "dispel evil", "dispel good", "earthquake", 
	  "flamestrike", "heat metal" }
	},

	{
	"beguiling",        { -1, 12, 8, 10, -1, -1, -1, -1, 6, -1, 10, 3, -1, -1, 9 },
	{ "betray", "calm", "charm person", "fear", "feeblemind", "pacify", "sleep",
           "laughing fit" }
	},

	{
	"benedictions",     { -1, -1, 8, -1, -1, -1, 15, -1, -1, 12, 10, -1, -1, -1, -1 },
	{ "bless", "calm", "frenzy", "holy word", "remove curse", "prayer"}
	},

	{
	"combat",       { -1, -1, -1, 9, -1, 12, -1, -1, 15, -1, 8, 15, -1, -1, -1 },
	{ "acid blast", "burning hands", "chain lightning", "chill touch",
	  "colour spray", "fireball", "lightning bolt", "magic missile",
	  "shocking grasp"  }
	},

	{
	"creation",     { -1, -1, 8, 5, -1, 9, -1, -1, 12, 6, 6, -1, -1, -1, 6 },
	{ "continual light", "create food", "create spring", "create water",
	  "create rose", "floating disc", "hand of siva", "goodberry",                       
          "sticks to snakes" }
	},

	{
	"curative",     { -1, -1, 6, -1, -1, 8, 9, -1, -1, 6, 7, -1, -1, 10, 10 },
	{ "cure blindness", "cure disease", "cure poison", "cure mental", "remove curse" }
	}, 

	{
	"detection",        { -1, 16, 8, 7, -1, 10, 15, -1, 12, -1, 9, 4, -1, 8, 7 },
	{ "detect astral", "detect evil", "detect good", "detect hidden", "detect invis", 
	  "detect magic", "detect poison", "identify", "farsight",
	  "know alignment", "locate object" } 
	},

	{
	"draconian",        { -1, -1, -1, 10, -1, -1, -1, -1, -1, -1, -1, 20, -1, -1, -1 },
	{ "acid breath", "fire breath", "frost breath", "gas breath",
	  "lightning breath"  }
	},

	{
	"black magics",     { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15 },
	{ "cone of exhaustion", "forboding ooze", "tomb stench", "zombie breath",
	  "zone of damnation" }
	},

	{
	"debilitations",     { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5 },
	{ "decompose", "soreness", "tomb rot", "necrosis", "haunt", "mana burn",
	  "iron maiden" }
	},

	{
	"dark rituals",     { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5 },
	{ "blessed darkness", "glyph of evil", "dancing bones" }
	},

	{
	"enchantment",      { -1, -1, -1, 6, -1, -1, -1, -1, -1, -1, -1, 8, -1, 9, 7 },
	{ "enchant arrow", "enchant armor", "enchant weapon", "fireproof", "recharge", "renewal" }
	},

	{ 
	"enhancement",      { 10, 10, -1, 3, -1, 5, -1, 9, 5, -1, 6, 5, 10, 10, 5 },
	{ "giant strength", "haste", "infravision", "refresh", "mantra" }
	},

	{
	"harmful",      { -1, -1, 4, -1, -1, -1, 6, -1, -1, 9, 6, -1, -1, -1, -1 },
	{ "cause critical", "cause light", "cause serious", "harm" }
	},

	{   
	"healing",      { -1, -1, 10, -1, -1, -1, 12, -1, -1, 12, 15, -1, -1, -1, -1 },
	{ "cure light", "cure serious", "cure critical", "cure mortal", 
	  "minor group heal", "group heal", "major group heal", "heal", 
	  "mass healing", "refresh", "restoration", "heal mind" }
	},
   
	{
	"holy rites",       { -1, -1, 10, -1, -1, -1, 11, -1, -1, 8, 10, -1, -1, -1, -1 },
	{ "divine light", "holy binding", "ray of truth", "turn undead", "hand of god",
          "deaths door", "heroism", "breath of god" }
	},

	{
	"illusions",     { -1, 15, -1, 5, -1, 15, -1, -1, 8, -1, -1, 1, -1, -1, -1 },
	{ "invisibility", "mass invis", "ventriloquate", "confusion", "extinguish", "mimic",
	  "mirror image" }
	},
  
	{
	"maledictions",     { -1, -1, 6, 7, -1, -1, -1, -1, 10, 8, 7, 9, -1, -1, 6 },
	{ "blindness", "change sex", "curse", "plague", 
	  "poison", "slow", "stop", "weaken" }
	},

	{
	"necromancy",   { -1, -1, 10, 12, -1, -1, -1, -1, -1, -1, 11, -1, -1, -1, 9 },
	{ "animate dead", "cannibalism", "chill touch", "damned blade", 
	  "dominate soul", "energy drain", "necrosis", "ritual sacrifice" }
	},

        /* War  Thi  Cle  Mag  Gla  Sam  Pal  Asn  Nin  Mon  Tem  Ilu  Gun  Ran  Nec */

        {
        "protective",    { -1, -1, 3, 4, -1, 8, 6, -1, 8, 4, 4, 6, -1, -1, 5 },
        { "armor", "cancellation", "dispel magic", "fireproof",
		  "protection evil", "protection good", "sanctuary", "shield", 
	      "stone skin", "mana shield" }
        },
	
	{
	"transportation",   { 15, 10, 6, 6, -1, 9, 8, 13, 8, 8, 7, 9, 15, 10, 6 },
	{ "astral projection", "fly", "gate", "nexus", "pass door", "portal", 
	  "summon", "teleport", "word of recall", "breathe water"}
	},

	{
	"high gifts",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, -1 },
	{ "turn undead", "chain lightning", "stone skin", "sanctuary", "mana shield" }
	},

	{
	"elemental",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, -1 },
	{ "fireball", "earthquake", "wind war", "monsoon" }
	},
	
	{
	"gaia magics",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, -1 },
	{ "goodberry", "entangle", "tree golem", "pass without trace", "sticks to snakes",
	  "life force" }
	},

	{
	"unibomber",        { -1, 12, -1, -1, -1, -1, -1, 6, -1, -1, -1, -1, 10, -1, -1 },
	{ "create bomb", "ignite" }
	},

	{
	"drifter",      { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, -1, -1 },
	{ "know alignment", "locate object", "cure poison", "detect hidden",
	  "fear" }
	},

	{
	"survival", { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 15, -1 },
	{ "hunt", "1000-yard stare", "elude", "set snare" }
	},

	{
	"gunsmoke", { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, -1, -1 },
	{ "call sidekick" }
	},
	
	{
	"practical shooting", { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, -1, -1 },
	{ "quick draw", "pistol whip", "duck", "shoot lock" }
	},
	
	{
	"warfare", { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, -1, -1 },
	{ "burst", "aim", "semiauto", "fullauto", "snipe" }
	},
	
	{
	"weather",      { -1, -1, 5, 7, -1, -1, -1, -1, -1, -1, 6, 1, -1, 10, -1 },
	{ "call lightning", "control weather", "faerie fire", "faerie fog",
	  "lightning bolt", "monsoon", "hailstorm", "meteor swarm", "solar flare" }
	},

	{
	"weapon styles",    { 12, -1, -1, -1, 18, 10, 9, -1, 11, -1, -1, -1, -1, -1, -1 },
	{ "finesse", "rage", "retribution", "blade dance" }
	},
	
	{
	"mystical styles",  { -1, -1, -1, -1, -1, 9, -1, -1, -1, 8, -1, -1, -1, -1, -1 },
	{ "dragon", "phoenix", "unicorn", "dimensional blade", "elemental blade" }
	},

	{
	"three seas", { -1, -1, -1, -1, -1, 10, -1, -1, 8, 8, -1, -1, -1, -1, -1 },
	{ "eel", "toad", "tortoise", "serpent"  }
	},

	{
	"vermin bite", { -1, -1, -1, -1, -1, -1, -1, -1, 8, 8, -1, -1, -1, -1, -1 },
	{ "boa", "porcupine", "scorpion" }
	},

	{
	"shadows", { -1, -1, -1, -1, -1, -1, -1, -1, 10, -1, -1, -1, -1, -1, -1 },
	{ "shadowclaw", "shadowessence", "shadowsoul", "shadowwalk" }
	},

	{
	"spirit fist", { -1, -1, -1, -1, -1, -1, -1, -1, 13, 13, -1, -1, -1, 15, -1 },
	{ "bear", "eagle", "lion", "rhino", "tiger" }
	},

	{
	"shielding", { -1, -1, 8, 10, -1, -1, -1, -1, -1, -1, 10, 9, -1, -1, 10 },
	{ "immolation", "epidemic", "electrocution", "absolute zero", "fade",
	  "protection magic", "reflection" }
	},

	{
	"holy war", { -1, -1, -1, -1, -1, -1, 17, -1, -1, -1, 17, -1, -1, -1, -1 },
	{ "jihad", "vampire hunting", "witch hunting", "werewolf hunting", "inquisition" }
	},

	{
	"nordic styles", { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, -1 },
	{ "goblincleaver", "tempest", "wendigo" }
	},

/* Sacred Invocations for Clerics and Paladins coded by Korinn 1-15-99*/
	{
	"sacred invocations", { -1, -1, 25, -1, -1, -1, 30, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ "swaydes mercy", "parademias bile", "korinns inspiration", "smotes anachronism",
 	"quirkys insanity", "mephistons scrutiny", "sivas sacrifice", "firewitchs seance",
        "rimbols invocation" }
	},

        /* War, Thf, Cle, Mag, Gla, Sam, Pal, Asn, Nin, Mnk, Tem, Ilu, Gun, Rng, Nec */
	{
        "heroic rites", { -1, 6, 9, 8, -1, -1, 6, -1, -1, -1, 8, -1, -1, -1, -1 },
        { "shadow shroud", "unearth", "hallow", "astarks rejuvenation",
          "overcharge", "conviction" }
	},

        {
        "advanced illusions", { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5, -1, -1, -1 },
        { "basic apparition", "holy apparition", "phantasmal image" }
        }

};
