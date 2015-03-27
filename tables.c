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
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "tables.h"

/*
const struct clan_type clan_table[MAX_CLAN] =
{
*/
	/* name, who entry, death-transfer room, donation room, independent */
	/* independent should be FALSE if is a real clan */
/*	{"",          "",            "Quirky",    ROOM_VNUM_RECALL, ROOM_VNUM_DONATION, TRUE  },
	{"none",      "none",        "none",      ROOM_VNUM_RECALL, ROOM_VNUM_DONATION, TRUE  },
	{"sehkma",    "{gSehkma",    "Mephiston", 10100,            10162,              FALSE }, 
	{"aessedai",  "{GAes Sedai", "Swayde",    31400,            31410,              FALSE }, 
	{"legion",    "{BLegion",    "Rimbol",    19041,            19092,              FALSE }, 
	{"q",         "{RQ",         "Quirky",    19199,            19108,              FALSE }, 
	{"noctai",    "{cNoctai",    "Lilith",    ROOM_VNUM_RECALL, ROOM_VNUM_DONATION, FALSE },
	{"coven",     "{rCoven",     "Firewitch", 1800,             1811,               FALSE },
	{"sanye",     "{CSanye",     "Kardiak",   22900,            22917,              FALSE },
    {"divergent", "{DDivergent", "Kasmer",    ROOM_VNUM_RECALL, ROOM_VNUM_DONATION, FALSE }
};*/

/* Name (used in clan_rank_lookup(), rank(), fread/fwrite_char(), etc.),
   Abbreviation in who/whois,
   Minimum level requirement to be promoted to this rank, 
   Maximum clan rank this ranking has the ability to promote to, 
   Participation in special clan war pkilling.
 */   
/*
const struct clan_rank_type clan_rank_table[] =
{   
    { "",         "",         0, "",      FALSE },
    { "recruit",  "-Rec{x",  10, "",      FALSE },
    { "initiate", "-Ini{x",  15, "",      FALSE },
    { "member",   "-Mem{x",  30, "",       TRUE },
    { "elite",    "-Elt{x",  90, "",       TRUE },
    { "elder",    "-Eld{x",  92, "member", TRUE },
    { "god",      "-God{x", 102, "elite", FALSE },
    { "patron",   "-Pat{x", 104, "god",   FALSE }
};
*/


/* Penalty table
 Name,
 Flag bit to assign to player,
 Text string to show when imposed,
 Duration of penalty at each severity (-1 = specify, -2 = infinite),
 Demerit points assigned for each severity (-1 = specify),
 Minimum imm level required to impose this severity 
*/
const struct penalty_type penalty_table[] = 
{
    {"", 0, "", 
        {0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0}},
    {"noemote",    PENALTY_NOEMOTE,    "no-emoted", 
        {3600, 18000, 36000, -1, -2}, {0, 0, 1, -1, -1}, {L8, L6, L4, L2} },
    {"noshout",    PENALTY_NOSHOUT,    "no-shouted",
        {3600, 18000, 36000, -1, -2}, {0, 0, 1, -1, -1}, {L8, L6, L4, L2}},
    {"notell",     PENALTY_NOTELL,     "no-telled",
        {3600, 18000, 36000, -1, -2}, {0, 0, 1, -1, -1}, {L8, L6, L4, L2}},
    {"nochannel",  PENALTY_NOCHANNEL, "no-channelled",
        {3600, 18000, 36000, -1, -2}, {1, 2, 2, -1, -1}, {L8, L6, L4, L2}},
    {"nonote",     PENALTY_NONOTE,    "no-noted",
        {3600, 18000, 36000, -1, -2}, {1, 2, 2, -1, -1}, {L8, L6, L4, L2}},
    {"freeze",     PENALTY_FREEZE,     "frozen",
        {3600, 18000, 36000, -1, -2}, {1, 2, 2, -1, -1}, {L4, L2, L2, ML}},
    {"jail",       PENALTY_JAIL,       "jailed",
        {3600, 18000, 36000, -1, -2}, {2, 2, 3, -1, -1}, {L8, L6, L4, L2}}
};


const struct stat_type stat_table[] =
{
	{"strength",    "Str", STAT_STR, {60, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
	{"constitution",    "Con", STAT_CON, { 0,60, 0, 0, 0, 0, 0, 0, 0, 0}},
	{"vitality",    "Vit", STAT_VIT, { 0, 0,60, 0, 0, 0, 0, 0, 0, 0}},
	{"agility",     "Agi", STAT_AGI, { 0, 0, 0,60, 0, 0, 0, 0, 0, 0}},
	{"dexterity",   "Dex", STAT_DEX, { 0, 0, 0, 0,60, 0, 0, 0, 0, 0}},
	{"intelligence",    "Int", STAT_INT, { 0, 0, 0, 0, 0,60, 0, 0, 0, 0}},
	{"wisdom",      "Wis", STAT_WIS, { 0, 0, 0, 0, 0, 0,60, 0, 0, 0}},
	{"discipline",  "Dis", STAT_DIS, { 0, 0, 0, 0, 0, 0, 0,60, 0, 0}},
	{"charisma",    "Cha", STAT_CHA, { 0, 0, 0, 0, 0, 0, 0, 0,60, 0}},
	{"luck",        "Luc", STAT_LUC, { 0, 0, 0, 0, 0, 0, 0, 0, 0,60}},
	{"body",        NULL ,   -1, {15,20,20,15,10, 0, 0, 0, 0, 0}},
	{"mind",        NULL ,   -1, { 0, 0, 0, 0, 0,25,20,15,10,10}},
	{"toughness",   NULL ,   -1, {25,20,10, 0, 0, 0, 0,25, 0, 0}},
	{"speed",       NULL ,   -1, { 0, 0,10,25,30,15, 0, 0, 0, 0}},
	{"wit",     NULL ,   -1, { 0, 0, 0, 0, 0, 0,20, 0,30,30}},
	{ NULL,     NULL ,   -1, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};
/* for position */
const struct position_type position_table[] =
{
	{   "dead",         "dead"  },
	{   "mortally wounded", "mort"  },
	{   "incapacitated",    "incap" },
	{   "stunned",      "stun"  },
	{   "sleeping",     "sleep" },
	{   "resting",      "rest"  },
	{   "sitting",      "sit"   },
	{   "fighting",     "fight" },
	{   "standing",     "stand" },
	{   NULL,           NULL    }
};

/* for sex */
const struct sex_type sex_table[] =
{
   {    "none"      },
   {    "male"      },
   {    "female"    },
   {    "either"    },
   {    NULL        }
};

/* for sizes */
const struct size_type size_table[] =
{ 
	{   "tiny"      },
	{   "small"     },
	{   "medium"    },
	{   "large"     },
	{   "huge",     },
	{   "giant"     },
	{   NULL        }
};


bool is_settable( int flag, const struct flag_type *flag_table )
{
    int i;

    for ( i = 0; flag_table[i].name != NULL; i++ )
	if ( flag_table[i].bit == flag )
	    return flag_table[i].settable;

    return FALSE;
}

/* various flag tables */
const struct flag_type act_flags[] =
{
	{   "npc",          A,  FALSE   },
	{   "sentinel",     B,  TRUE    },
	{   "scavenger",        C,  TRUE    },
	{   "wizi",         ACT_WIZI,   TRUE },
	{   "object",       ACT_OBJ, TRUE },
	{   "aggressive",       F,  TRUE    },
	{   "stay_area",        G,  TRUE    },
	{   "wimpy",        H,  TRUE    },
	{   "pet",          I,  TRUE    },
	{   "train",        J,  TRUE    },
	{   "practice",     K,  TRUE    },
	{   "no_track",     L,  TRUE    },      
	{   "see_all",      M,  TRUE    },      
	{   "trigger_always", ACT_TRIGGER_ALWAYS, TRUE },      
	{   "undead",       O,  TRUE    },
	{   "spellup",      ACT_SPELLUP,  TRUE    },
	{   "cleric",       Q,  TRUE    },
	{   "mage",         R,  TRUE    },
	{   "thief",        S,  TRUE    },
	{   "warrior",      T,  TRUE    },
	{   "noalign",      U,  TRUE    },
	{   "nopurge",      V,  TRUE    },
	{   "outdoors",     W,  TRUE    },
	{   "banker",       X,  TRUE    },
	{   "indoors",      Y,  TRUE    },
	{   "gunman",       Z,  TRUE    },
	{   "healer",       aa, TRUE    },
	{   "gain",         bb, TRUE    },
	{   "update_always",    cc, TRUE    },
	{   "changer",      dd, TRUE    },
	{   "no_quest",     ACT_NO_QUEST,    TRUE    },
	{   "safe",         ACT_SAFE,        TRUE    },
	{   "ignore_safe",  ACT_IGNORE_SAFE, TRUE    },
	{   "judge",        ACT_JUDGE,       TRUE    },
	{   "noexp",       ACT_NOEXP,      TRUE    },
	{   "nomimic",      ACT_NOMIMIC,     TRUE    },
	{   "hard_quest",   ACT_HARD_QUEST,    TRUE    },
    {   "staggered",    ACT_STAGGERED,   TRUE    },
    {   "nobehead",     ACT_NOBEHEAD,    TRUE    },
    {   "noweapon",     ACT_NOWEAPON,    TRUE    },
    {   "traveller",    ACT_TRAVELLER,   TRUE    },
    {   "achievement",  ACT_ACHIEVEMENT, TRUE    },
	{   NULL,           0,  FALSE   }
};

const struct flag_type plr_flags[] =
{
    {   "npc",          PLR_IS_NPC,     FALSE   },
    {   "autoassist",   PLR_AUTOASSIST, FALSE   },
    {   "autoexit",     PLR_AUTOEXIT,   FALSE   },
    {   "autoloot",     PLR_AUTOLOOT,   FALSE   },
    {   "autosac",      PLR_AUTOSAC,    FALSE   },
    {   "autogold",     PLR_AUTOGOLD ,  FALSE   },
    {   "autosplit",    PLR_AUTOSPLIT,  FALSE   },
    {   "unauthed",     PLR_UNAUTHED,   FALSE   },
    {   "consent",      PLR_CONSENT,    FALSE   },
    {   "autorescue",   PLR_AUTORESCUE, FALSE   },
    {   "law",          PLR_LAW,        FALSE   },
    {   "helper",       PLR_HELPER,     TRUE    },
    {   "holylight",    PLR_HOLYLIGHT,  FALSE   },
    {   "can_loot",     PLR_CANLOOT,    FALSE   },
    {   "nosummon",     PLR_NOSUMMON,   FALSE   },
    {   "nofollow",     PLR_NOFOLLOW,   FALSE   },
    {   "questing",     PLR_QUESTOR,    FALSE   },
    {   "colour",       PLR_COLOUR,     FALSE   },
    {   "permit",       PLR_PERMIT,     TRUE    },
    {   "questinghard", PLR_QUESTORHARD,FALSE   },
    {   "log",          PLR_LOG,        FALSE   },
    {   "deny",         PLR_DENY,       FALSE   },
    {   "thief",        PLR_THIEF,      FALSE   },
    {   "killer",       PLR_KILLER,     FALSE   },
    {   "plr_title",    PLR_TITLE,      FALSE   },
    {   "perm_pk",      PLR_PERM_PKILL, TRUE    },
    {   "hardcore",     PLR_HARDCORE,   TRUE    },
    {   "nocancel",     PLR_NOCANCEL,   FALSE   },
    {   "nolocate",     PLR_NOLOCATE,   FALSE   },
    {   "noaccept",     PLR_NOACCEPT,   FALSE   },
    {   "nosurrender",  PLR_NOSURR,     FALSE   },
    {   "roleplay",     PLR_RP,         TRUE    },
    {   "inactive_helper", PLR_INACTIVE_HELPER, TRUE },
    {   "noexp",        PLR_NOEXP,      FALSE   },
    {   "nohelp",       PLR_NOHELP,     FALSE   },
    {   NULL,           0,  0   }
};

const struct flag_type affect_flags[] =
{
    { "blind",             AFF_BLIND,                TRUE  },
    { "invisible",         AFF_INVISIBLE,            TRUE  },
    { "detect_evil",       AFF_DETECT_EVIL,          TRUE  },
    { "detect_invis",      AFF_DETECT_INVIS,         TRUE  },
    { "detect_magic",      AFF_DETECT_MAGIC,         TRUE  },
    { "detect_hidden",     AFF_DETECT_HIDDEN,        TRUE  },
    { "detect_good",       AFF_DETECT_GOOD,          TRUE  },
    { "sanctuary",         AFF_SANCTUARY,            TRUE  },
    { "fade",              AFF_FADE,                 TRUE  },
    { "infrared",          AFF_INFRARED,             TRUE  },
    { "curse",             AFF_CURSE,                TRUE  },
    { "astral",            AFF_ASTRAL,               TRUE  },
    { "paralysis_poison",  AFF_PARALYSIS,            TRUE  },
    { "poison",            AFF_POISON,               TRUE  },
    { "protect_evil",      AFF_PROTECT_EVIL,         TRUE  },
    { "protect_good",      AFF_PROTECT_GOOD,         TRUE  },
    { "sneak",             AFF_SNEAK,                TRUE  },
    { "hide",              AFF_HIDE,                 TRUE  },
    { "sleep",             AFF_SLEEP,                FALSE  },
    { "charm",             AFF_CHARM,                FALSE  },
    { "flying",            AFF_FLYING,               TRUE  },
    { "pass_door",         AFF_PASS_DOOR,            TRUE  },
    { "haste",             AFF_HASTE,                TRUE  },
    { "calm",              AFF_CALM,                 TRUE  },
    { "plague",            AFF_PLAGUE,               TRUE  },
    { "weaken",            AFF_WEAKEN,               TRUE  },
    { "dark_vision",       AFF_DARK_VISION,          TRUE  },
    { "berserk",           AFF_BERSERK,              TRUE  },
    { "breathe_water",     AFF_BREATHE_WATER,        TRUE  },
    { "regeneration",      AFF_REGENERATION,         TRUE  },
    { "slow",              AFF_SLOW,                 TRUE  },
    { "battle_meter",      AFF_BATTLE_METER,         TRUE  },
    { "fear",              AFF_FEAR,                 TRUE  },
    { "detect_astral",     AFF_DETECT_ASTRAL,        TRUE  },
    { "shelter",           AFF_SHELTER,              TRUE  },
    { "chaos_fade",        AFF_CHAOS_FADE,           TRUE  },
    { "feeblemind",        AFF_FEEBLEMIND,           TRUE  },
    { "guard",             AFF_GUARD,                TRUE  },
    { "ritual",            AFF_RITUAL,               TRUE  },
    { "necrosis",          AFF_NECROSIS,             TRUE  },
    { "animate_dead",      AFF_ANIMATE_DEAD,         FALSE },
    { "elemental_shield",  AFF_ELEMENTAL_SHIELD,     FALSE },
    { "protect_magic",     AFF_PROTECT_MAGIC,        TRUE  },
    { "faerie_fire",       AFF_FAERIE_FIRE,          TRUE  },
    { "no_trace",          AFF_NO_TRACE,             TRUE  },  
    { "entangle",          AFF_ENTANGLE,             TRUE  },
    { "insane",            AFF_INSANE,               TRUE  },
    { "laugh",             AFF_LAUGH,                TRUE  },
    { "sore",              AFF_SORE,                 TRUE  },
    { "darkness",          AFF_DARKNESS,             TRUE  },
    { "tomb_rot",          AFF_TOMB_ROT,             TRUE  },
    { "deaths_door",       AFF_DEATHS_DOOR,          TRUE  },
    { "heroism",           AFF_HEROISM,              TRUE  },
    { "reflection",        AFF_REFLECTION,           TRUE  },
    { "roots",             AFF_ROOTS,                TRUE  },
    { "mantra",            AFF_MANTRA,               TRUE  },
    { "learning",          AFF_LEARN,                TRUE  },
    { "mana_burn",         AFF_MANA_BURN,            TRUE  },
    { "iron_maiden",       AFF_IRON_MAIDEN,          TRUE  },
    { "fleeing",           AFF_FLEE,                 FALSE },
    { "heal",              AFF_HEAL,                 TRUE  },
    { "overcharge",        AFF_OVERCHARGE,           TRUE  },
    { "giant strength",    AFF_GIANT_STRENGTH,       TRUE  },
    { "phase",             AFF_PHASE,                TRUE  }, 
    { "shroud of darkness",AFF_SHROUD,               TRUE  },
//    { "infectious_arrow",  AFF_INFECTIOUS_ARROW,     TRUE  },
//    { "fervent_rage",      AFF_FERVENT_RAGE,         TRUE  },
//    { "fervent_rage_cooldown",  AFF_FERVENT_RAGE_COOLDOWN,TRUE  },
//    { "paroxysm",          AFF_PAROXYSM,             TRUE  },
//    { "paroxysm_cooldown", AFF_PAROXYSM_COOLDOWN,    TRUE  },
//    { "rupture",           AFF_RUPTURE,              TRUE  },
    { "hallow",            AFF_HALLOW,               TRUE  },
    { "minor_fade",        AFF_MINOR_FADE,           TRUE  },
    { "replenish",         AFF_REPLENISH,            TRUE  },
    { "fortune",           AFF_FORTUNE,              TRUE  },
    { "shield",            AFF_SHIELD,               TRUE  },
    { "stone_skin",        AFF_STONE_SKIN,           TRUE  },
    { "petrified",         AFF_PETRIFIED,            TRUE  },
    { NULL,                0,                        0     }
};

const struct flag_type off_flags[] =
{
	{   "area_attack",      A,  TRUE    },
	{   "backstab",     B,  TRUE    },
	{   "bash",         C,  TRUE    },
	{   "berserk",      D,  TRUE    },
	{   "disarm",       E,  TRUE    },
	{   "dodge",        F,  TRUE    },
	{   "fade",         G,  TRUE    },
	{   "fast",         H,  TRUE    },
	{   "kick",         I,  TRUE    },
	{   "dirt_kick",        J,  TRUE    },
	{   "parry",        K,  TRUE    },
	{   "rescue",       L,  TRUE    },
	{   "tail",         M,  TRUE    },
	{   "trip",         N,  TRUE    },
	{   "crush",        O,  TRUE    },
	{   "assist_all",       P,  TRUE    },
	{   "assist_align",     Q,  TRUE    },
	{   "assist_race",      R,  TRUE    },
	{   "assist_players",   S,  TRUE    },
	{   "assist_guard",     T,  TRUE    },
	{   "assist_vnum",      U,  TRUE    },
    {   "distract",         V,  FALSE   },
    {   "entrap",           W,  TRUE    },
    {   "hunt",             X,  TRUE    },
	{   "armed",            aa, TRUE    },
	{   "circle",           bb, TRUE    },
    {   "petrify",          cc, TRUE    },
	{   NULL,           0,  0   }
};

const struct flag_type imm_flags[] =
{
	{   "summon",       A,  TRUE    },
	{   "charm",        B,  TRUE    },
	{   "magic",        C,  TRUE    },
	{   "weapon",       D,  TRUE    },
	{   "bash",         E,  TRUE    },
	{   "pierce",       F,  TRUE    },
	{   "slash",        G,  TRUE    },
	{   "fire",         H,  TRUE    },
	{   "cold",         I,  TRUE    },
	{   "light",        S,  TRUE    },
    {   "lightning",    J,  TRUE    },
	{   "acid",         K,  TRUE    },
	{   "poison",       L,  TRUE    },
	{   "negative",     M,  TRUE    },
	{   "holy",         N,  TRUE    },
	{   "energy",       O,  TRUE    },
	{   "mental",       P,  TRUE    },
	{   "disease",      Q,  TRUE    },
	{   "drowning",     R,  TRUE    },
	{   "sound",        T,  TRUE    },
        {   "sleep",        U,  TRUE    },
        {   "charmperson",  V,  TRUE    },
        {   "gaze",         W,  TRUE    },
	{   "wood",         X,  TRUE    },
	{   "silver",       Y,  TRUE    },
	{   "iron",         Z,  TRUE    },
	{   NULL,           0,  0   }
};

const struct flag_type form_flags[] =
{
	{   "edible",       FORM_EDIBLE,        TRUE    },
	{   "poison",       FORM_POISON,        TRUE    },
	{   "magical",      FORM_MAGICAL,       TRUE    },
	{   "instant_decay", FORM_INSTANT_DECAY, TRUE    },
	{   "other",        FORM_OTHER,     TRUE    },
	{   "animal",       FORM_ANIMAL,        TRUE    },
	{   "sentient",     FORM_SENTIENT,      TRUE    },
	{   "undead",       FORM_UNDEAD,        TRUE    },
	{   "construct",    FORM_CONSTRUCT,     TRUE    },
	{   "mist",         FORM_MIST,      TRUE    },
	{   "intangible",   FORM_INTANGIBLE,    TRUE    },
	{   "biped",        FORM_BIPED,     TRUE    },
	{   "agile",        FORM_AGILE,       TRUE    },
	{   "insect",       FORM_INSECT,        TRUE    },
	{   "spider",       FORM_SPIDER,        TRUE    },
	{   "crustacean",   FORM_CRUSTACEAN,    TRUE    },
	{   "worm",         FORM_WORM,      TRUE    },
	{   "blob",         FORM_BLOB,      TRUE    },
	{   "plant",        FORM_PLANT,        TRUE    },
	{   "mammal",       FORM_MAMMAL,        TRUE    },
	{   "bird",         FORM_BIRD,      TRUE    },
	{   "reptile",      FORM_REPTILE,       TRUE    },
	{   "snake",        FORM_SNAKE,     TRUE    },
	{   "dragon",       FORM_DRAGON,        TRUE    },
	{   "amphibian",    FORM_AMPHIBIAN,     TRUE    },
	{   "fish",         FORM_FISH ,     TRUE    },
	{   "cold_blood",   FORM_COLD_BLOOD,    TRUE    },
	{   "bright",       FORM_BRIGHT,    TRUE    },
	{   "tough",        FORM_TOUGH,     TRUE    },
	{   "sunburn",      FORM_SUNBURN,     TRUE    },
	{   "double-jointed", FORM_DOUBLE_JOINTED, TRUE    },
	{   "frost_aura",   FORM_FROST, TRUE    },
	{   "burning_aura", FORM_BURN,  TRUE    },
	{   "wise",         FORM_WISE,  TRUE    },
    {   "conductive",   FORM_CONDUCTIVE,    TRUE    },
    {   "constrict",    FORM_CONSTRICT,     TRUE    },
    {   "multi-headed", FORM_MULTI_HEADED,  TRUE    },
    {   "armored",      FORM_ARMORED,       TRUE    },
	{   NULL,           0,          0   }
};

const struct flag_type part_flags[] =
{
	{   "head",         PART_HEAD,      TRUE    },
	{   "arms",         PART_ARMS,      TRUE    },
	{   "legs",         PART_LEGS,      TRUE    },
	{   "heart",        PART_HEART,     TRUE    },
	{   "brains",       PART_BRAINS,        TRUE    },
	{   "guts",         PART_GUTS,      TRUE    },
	{   "hands",        PART_HANDS,     TRUE    },
	{   "feet",         PART_FEET,      TRUE    },
	{   "fingers",      PART_FINGERS,       TRUE    },
	{   "ear",          PART_EAR,       TRUE    },
	{   "eye",          PART_EYE,       TRUE    },
	{   "long_tongue",      PART_LONG_TONGUE,   TRUE    },
	{   "eyestalks",        PART_EYESTALKS,     TRUE    },
	{   "tentacles",        PART_TENTACLES,     TRUE    },
	{   "fins",         PART_FINS,      TRUE    },
	{   "wings",        PART_WINGS,     TRUE    },
	{   "tail",         PART_TAIL,      TRUE    },
	{   "claws",        PART_CLAWS,     TRUE    },
	{   "fangs",        PART_FANGS,     TRUE    },
	{   "horns",        PART_HORNS,     TRUE    },
	{   "scales",       PART_SCALES,        TRUE    },
	{   "tusks",        PART_TUSKS,     TRUE    },
	{   NULL,           0,          0   }
};

const struct flag_type comm_flags[] =
{
	{   "quiet",        COMM_QUIET,         TRUE    },
	{   "deaf",         COMM_DEAF,          TRUE    },
	{   "nowiz",        COMM_NOWIZ,         TRUE    },
	{   "noauction",    COMM_NOAUCTION,     TRUE    },
	{   "nogossip",     COMM_NOGOSSIP,      TRUE    },
	{   "noquestion",   COMM_NOQUESTION,    TRUE    },
	{   "nomusic",      COMM_NOMUSIC,       TRUE    },
	{   "noclan",       COMM_NOCLAN,        TRUE    },
	{   "noreligion",   COMM_NOREL,         TRUE    },
	{   "noquote",      COMM_NOQUOTE,       TRUE    },
	{   "shoutsoff",    COMM_SHOUTSOFF,     TRUE    },
	{   "noinfo",       COMM_NOINFO,        TRUE    },   
	{   "nonewbie",     COMM_NONEWBIE,      TRUE    },
	{   "nobitch" ,     COMM_NOBITCH,       TRUE    },
	{   "nogame",       COMM_NOGAME,        TRUE    },
	{   "compact",      COMM_COMPACT,       TRUE    },
	{   "brief",        COMM_BRIEF,         TRUE    },
	{   "prompt",       COMM_PROMPT,        TRUE    },
	{   "combine",      COMM_COMBINE,       TRUE    },
	{   "telnet_ga",    COMM_TELNET_GA,     TRUE    },
	{   "show_affects", COMM_SHOW_AFFECTS,  TRUE    },
	{   "show_worth",   COMM_SHOW_WORTH,    TRUE    },  /*06/07/98*/
	{   "show_attrib",  COMM_SHOW_ATTRIB,   TRUE    },  /*06/07/98*/
        {   "show_percent", COMM_SHOW_PERCENT,  TRUE    },
        {   "show_statbars",COMM_SHOW_STATBARS, TRUE    },
	{   "nogratz",      COMM_NOGRATZ,       TRUE    },  
	{   "afk",          COMM_AFK,           TRUE    },
	{   NULL,           0,          0   }
};


const struct flag_type mprog_flags[] =
{
	{   "act",          TRIG_ACT,       TRUE    },
	{   "bribe",        TRIG_BRIBE,     TRUE    },
	{   "death",        TRIG_DEATH,     TRUE    },
	{   "entry",        TRIG_ENTRY,     TRUE    },
	{   "fight",        TRIG_FIGHT,     TRUE    },
	{   "give",         TRIG_GIVE,      TRUE    },
	{   "greet",        TRIG_GREET,     TRUE    },
	{   "grall",        TRIG_GRALL,     TRUE    },
	{   "kill",         TRIG_KILL,      TRUE    },
	{   "hpcnt",        TRIG_HPCNT,     TRUE    },
	{   "random",       TRIG_RANDOM,        TRUE    },
	{   "speech",       TRIG_SPEECH,        TRUE    },
	{   "exit",         TRIG_EXIT,      TRUE    },
	{   "exall",        TRIG_EXALL,     TRUE    },
	{   "delay",        TRIG_DELAY,     TRUE    },
	{   "surrender",    TRIG_SURR,      TRUE    },
	{   "drbomb",       TRIG_DRBOMB,    TRUE    },
	{   "exbomb",       TRIG_EXBOMB,    TRUE    },
	{   "defeat",       TRIG_DEFEAT,    TRUE    },
	{   "social",       TRIG_SOCIAL,    TRUE    },
	{   "try",          TRIG_TRY,       TRUE    },
	{   "reset",        TRIG_RESET,       TRUE    },
	{   "mpcnt",        TRIG_MPCNT,     TRUE    },
    {   "spell",        TRIG_SPELL,     TRUE    },
    {   "call",         TRIG_CALL,      FALSE   },
    {   "timer",        TRIG_TIMER,     TRUE    },
    {   "command",      TRIG_COMMAND,   TRUE    },
	{   NULL,           0,          TRUE    }
};

const struct flag_type oprog_flags[] =
{
    {   "give",         OTRIG_GIVE,      TRUE    },
    {   "drop",         OTRIG_DROP,      TRUE    },
    {   "eat",          OTRIG_EAT,       TRUE    },
    {   "drink",        OTRIG_DRINK,     TRUE    },
    {   "sacrifice",    OTRIG_SACRIFICE, TRUE    },
    {   "wear",         OTRIG_WEAR,      TRUE    },
    {   "remove",       OTRIG_REMOVE,    TRUE    },
    {   "spell",        OTRIG_SPELL,     TRUE    },
    {   "speech",       OTRIG_SPEECH,    TRUE    },
    {   "try",          OTRIG_TRY,       TRUE    },
    {   "put",          OTRIG_PUT,       TRUE    },
    {   "get",          OTRIG_GET,       TRUE    },
    {   "rand",         OTRIG_RAND,      TRUE    },
    {   "greet",        OTRIG_GREET,     TRUE    },
    {   "call",         OTRIG_CALL,      FALSE   },
    {   "look",         OTRIG_LOOK,      TRUE    },
    {   "lore",         OTRIG_LORE,      TRUE    },
    {   "enter",        OTRIG_ENTER,     TRUE    },
    {   "timer",        OTRIG_TIMER,     TRUE    },
    {   "fight",        OTRIG_FIGHT,     TRUE    },
    {   "hit",          OTRIG_HIT,       TRUE    },
    {   "prehit",       OTRIG_PREHIT,    TRUE    },
    {   "quaff",        OTRIG_QUAFF,     TRUE    },
    {   "open",         OTRIG_OPEN,      TRUE    },
    {   "unlock",       OTRIG_UNLOCK,    TRUE    },
    {   "sit",          OTRIG_SIT,       TRUE    },
    {   "wake",         OTRIG_WAKE,      TRUE    },
    {   NULL,           0,          TRUE    }
};

const struct flag_type aprog_flags[] =
{
    {   "enter",        ATRIG_ENTER,     TRUE    },
	{   "exit",			ATRIG_EXIT,		 TRUE    },
	{   "renter",		ATRIG_RENTER,	 TRUE	 },
	{   "rexit",		ATRIG_REXIT,	 TRUE	 },
	{   "boot",			ATRIG_BOOT,		 TRUE	 },
	{   "shutdown",	    ATRIG_SHUTDOWN,  TRUE    },
	{   "quit",			ATRIG_QUIT,		 TRUE    },
    {   "void",         ATRIG_VOID,      TRUE    },
    {   "unvoid",       ATRIG_UNVOID,    TRUE    },
    {   "recall",       ATRIG_RECALL,    TRUE    },
    {   "call",         ATRIG_CALL,      FALSE   },
    {   "timer",        ATRIG_TIMER,     TRUE    },
    {   "death",        ATRIG_DEATH,     TRUE    },
    {   "connect",      ATRIG_CONNECT,   TRUE    },
    {   NULL,           0,          TRUE    }
};

const struct flag_type rprog_flags[] =
{
    {   "call",         RTRIG_CALL,      FALSE   },
    {   "timer",        RTRIG_TIMER,     TRUE    },
    {   "move",         RTRIG_MOVE,      TRUE    },
    {   "unlock",       RTRIG_UNLOCK,    TRUE    },
    {   "lock",         RTRIG_LOCK,      TRUE    },
    {   "open",         RTRIG_OPEN,      TRUE    },
    {   "close",        RTRIG_CLOSE,     TRUE    },
    {   "enter",        RTRIG_ENTER,     TRUE    },
    {   "exit",         RTRIG_EXIT,      TRUE    },
    {   "look",         RTRIG_LOOK,      TRUE    },
    {   "try",          RTRIG_TRY,       TRUE    },
    {   "command",      RTRIG_COMMAND,   TRUE    },
    {   "connect",      RTRIG_CONNECT,   TRUE    },
    {   NULL,           0,          TRUE    }
};

const struct flag_type area_flags[] =
{
	{   "changed",      AREA_CHANGED,       TRUE    },
	/*	{   "added",        AREA_ADDED,         TRUE    }, */
	/*	{   "loading",      AREA_LOADING,       FALSE   }, */
	{   "remort",       AREA_REMORT,        TRUE    },
	{   "clone",        AREA_CLONE,         FALSE   },
	{   "noquest",      AREA_NOQUEST,       TRUE    },
	{   "nohide",       AREA_NOHIDE,        TRUE    },
	{   "norepop",      AREA_NOREPOP,       TRUE    },
	{   NULL,           0,          0   }
};



const struct flag_type sex_flags[] =
{
	{   "male",         SEX_MALE,       TRUE    },
	{   "female",       SEX_FEMALE,     TRUE    },
	{   "neutral",      SEX_NEUTRAL,    TRUE    },
	{   "random",       3,              TRUE    },   /* ROM */
	{   "none",         SEX_NEUTRAL,    TRUE    },
	{   NULL,           0,          0   }
};



const struct flag_type exit_flags[] =
{
	{   "door",         EX_ISDOOR,      TRUE    },
	{   "closed",       EX_CLOSED,      TRUE    },
	{   "locked",       EX_LOCKED,      TRUE    },
	{   "pickproof",    EX_PICKPROOF,   TRUE    },
	{   "nopass",       EX_NOPASS,      TRUE    },

	{   "easy",         EX_EASY,        TRUE    },
	{   "hard",         EX_HARD,        TRUE    },
	{   "infuriating",  EX_INFURIATING, TRUE    },

	{   "noclose",      EX_NOCLOSE,     TRUE    },
	{   "nolock",       EX_NOLOCK,      TRUE    },
	{   "trapped",      EX_TRAPPED,     TRUE    },
    {   "hidden",       EX_HIDDEN,      TRUE    },
    {   "dormant",      EX_DORMANT,     TRUE    },
	{   NULL,           0,          0   }
};



const struct flag_type door_resets[] =
{
	{   "open and unlocked",    0,      TRUE    },
	{   "closed and unlocked",  1,      TRUE    },
	{   "closed and locked",    2,      TRUE    },
	{   NULL,           0,      0   }
};



const struct flag_type room_flags[] =
{
    {   "dark",         ROOM_DARK,          TRUE    },
    {   "no_mob",       ROOM_NO_MOB,        TRUE    },
    {   "indoors",      ROOM_INDOORS,       TRUE    },
    {   "no_scout",     ROOM_NO_SCOUT,      TRUE    },
    {   "tattoo_shop",  ROOM_TATTOO_SHOP,   TRUE    },
    {   "no_teleport",  ROOM_NO_TELEPORT,   TRUE    },
    {   "private",      ROOM_PRIVATE,       TRUE    },
    {   "safe",         ROOM_SAFE,          TRUE    },
    {   "solitary",     ROOM_SOLITARY,      TRUE    },
    {   "pet_shop",     ROOM_PET_SHOP,      TRUE    },
    {   "no_recall",    ROOM_NO_RECALL,     TRUE    },
    {   "imp_only",     ROOM_IMP_ONLY,      TRUE    },
    {   "gods_only",    ROOM_GODS_ONLY,     TRUE    },
    {   "heroes_only",  ROOM_HEROES_ONLY,   TRUE    },
    {   "newbies_only", ROOM_NEWBIES_ONLY,  TRUE    },
    {   "law",          ROOM_LAW,           TRUE    },
    {   "donation",     ROOM_DONATION,      TRUE    },
    {   "nowhere",      ROOM_NOWHERE,       TRUE    },
    {   "snare",        ROOM_SNARE,         FALSE   },   
    {   "blacksmith",   ROOM_BLACKSMITH,    FALSE   },   
    {   "peel",         ROOM_PEEL,          FALSE   },   
    {   "jail",         ROOM_JAIL,          TRUE    },   
    {   "no_quest",     ROOM_NO_QUEST,      TRUE    },   
    {   "hard_quest",   ROOM_HARD_QUEST,    TRUE    },
    {   "arena",        ROOM_ARENA,         TRUE    },   
    {   "barren",       ROOM_BARREN,        TRUE    },   
    {   "box_shop",	ROOM_BOX_SHOP,	    TRUE    },/*Where you buy storage boxes, should be bank actually*/
    {   "box_room",	ROOM_BOX_ROOM,      TRUE    },/*Where storage boxes load*/
    {   NULL,           0,                  0       } 
};



const struct flag_type sector_flags[] =
{
	{   "inside",   SECT_INSIDE,        TRUE    },
	{   "city",     SECT_CITY,      TRUE    },
	{   "field",    SECT_FIELD,     TRUE    },
	{   "forest",   SECT_FOREST,        TRUE    },
	{   "hills",    SECT_HILLS,     TRUE    },
	{   "mountain", SECT_MOUNTAIN,      TRUE    },
	{   "shallow",  SECT_WATER_SHALLOW,    TRUE    },
	{   "deep",	SECT_WATER_DEEP,  TRUE    },
	{   "underwater",SECT_UNDERWATER,        TRUE    },
	{   "air",      SECT_AIR,       TRUE    },
	{   "desert",   SECT_DESERT,        TRUE    },
        {   "underground", SECT_UNDERGROUND, TRUE },
	{   NULL,       0,          0   }
};



const struct flag_type type_flags[] =
{
	{   "light",        ITEM_LIGHT,     TRUE    },
	{   "scroll",       ITEM_SCROLL,    TRUE    },
	{   "wand",         ITEM_WAND,      TRUE    },
	{   "staff",        ITEM_STAFF,     TRUE    },
	{   "weapon",       ITEM_WEAPON,    TRUE    },
	{   "treasure",     ITEM_TREASURE,  TRUE    },
	{   "armor",        ITEM_ARMOR,     TRUE    },
	{   "potion",       ITEM_POTION,    TRUE    },
	{   "clothing",     ITEM_CLOTHING,  TRUE    },
	{   "furniture",    ITEM_FURNITURE, TRUE    },
	{   "trash",        ITEM_TRASH,     TRUE    },
	{   "container",    ITEM_CONTAINER, TRUE    },
	{   "drinkcontainer",   ITEM_DRINK_CON,     TRUE    },
	{   "key",          ITEM_KEY,       TRUE    },
	{   "food",         ITEM_FOOD,      TRUE    },
	{   "money",        ITEM_MONEY,     TRUE    },
	{   "boat",         ITEM_BOAT,      TRUE    },
	{   "npc_corpse",   ITEM_CORPSE_NPC,TRUE    },
	{   "pc_corpse",    ITEM_CORPSE_PC, FALSE   },
	{   "fountain",     ITEM_FOUNTAIN,  TRUE    },
	{   "pill",         ITEM_PILL,      TRUE    },
	{   "protect",      ITEM_PROTECT,   TRUE    },
	{   "map",          ITEM_MAP,       TRUE    },
	{   "portal",       ITEM_PORTAL,    TRUE    },
	{   "warpstone",    ITEM_WARP_STONE,TRUE    },
	{   "roomkey",      ITEM_ROOM_KEY,  TRUE    },
	{   "gem",          ITEM_GEM,       TRUE    },
	{   "jewelry",      ITEM_JEWELRY,   TRUE    },
	{   "jukebox",      ITEM_JUKEBOX,   TRUE    },
	{ "explosive",  ITEM_EXPLOSIVE,     TRUE  },
	{ "hogtie", ITEM_HOGTIE, TRUE},
	{ "dowsingstick", ITEM_DOWSING_STICK, TRUE},
	{ "silverherb", ITEM_SILVER_HERB, TRUE},     
	{ "blackherb", ITEM_BLACK_HERB, TRUE}, 
	{ "redherb", ITEM_RED_HERB, TRUE}, 
	{ "mottledherb", ITEM_MOTTLED_HERB, TRUE}, 
	{   "arrows",       ITEM_ARROWS,    TRUE    },
	{   NULL,           0,          0   }
};

const struct flag_type extra_flags[] =
{
	{   "glow",         ITEM_GLOW,      TRUE    },
	{   "hum",          ITEM_HUM,       TRUE    },
	{   "dark",         ITEM_DARK,      TRUE    },
	{   "lock",         ITEM_LOCK,      TRUE    },
	{   "evil",         ITEM_EVIL,      TRUE    },
	{   "invis",        ITEM_INVIS,     TRUE    },
	{   "magic",        ITEM_MAGIC,     TRUE    },
	{   "nodrop",       ITEM_NODROP,    TRUE    },
	{   "bless",        ITEM_BLESS,     TRUE    },
	{   "anti_good",    ITEM_ANTI_GOOD,     TRUE    },
	{   "anti_evil",    ITEM_ANTI_EVIL,     TRUE    },
	{   "anti_neutral", ITEM_ANTI_NEUTRAL,  TRUE    },
	{   "noremove",     ITEM_NOREMOVE,      TRUE    },
	{   "inventory",    ITEM_INVENTORY,     FALSE   },
	{   "nopurge",      ITEM_NOPURGE,       TRUE    },
	{   "rotdeath",     ITEM_ROT_DEATH,     TRUE    },
	{   "visdeath",     ITEM_VIS_DEATH,     TRUE    },
	{   "no_lore",      ITEM_NO_LORE,       TRUE    },
	{   "nonmetal",     ITEM_NONMETAL,      TRUE    },
	{   "nolocate",     ITEM_NOLOCATE,      TRUE    },
	{   "meltdrop",     ITEM_MELT_DROP,     TRUE    },
	{   "hadtimer",     ITEM_HAD_TIMER,     FALSE   },
	{   "sellextract",  ITEM_SELL_EXTRACT,  FALSE   },
	{   "random",       ITEM_RANDOM,        TRUE    },
	{   "burnproof",    ITEM_BURN_PROOF,    TRUE    },
	{   "nouncurse",    ITEM_NOUNCURSE,     TRUE    },
	{   "sticky",       ITEM_STICKY,        TRUE    },
	{   "jammed",       ITEM_JAMMED,        TRUE    },     
	{   "oneuse",       ITEM_ONE_USE,       TRUE    },
	{   "remort",       ITEM_REMORT,        TRUE    }, 
	{   "trapped",      ITEM_TRAPPED,       TRUE    },
	{   "easy_drop",    ITEM_EASY_DROP,     TRUE    },
    {   "heavy",        ITEM_HEAVY_ARMOR,   TRUE    },
	{   "allow_warrior",       ITEM_ALLOW_WARRIOR        , TRUE },
	{   "allow_thief",         ITEM_ALLOW_THIEF          , TRUE },
	{   "allow_cleric",        ITEM_ALLOW_CLERIC         , TRUE },
	{   "allow_mage",          ITEM_ALLOW_MAGE           , TRUE },
	{   "anti_warrior",        ITEM_ANTI_WARRIOR         , TRUE },
	{   "anti_thief",          ITEM_ANTI_THIEF           , TRUE },
	{   "anti_cleric",         ITEM_ANTI_CLERIC          , TRUE },
	{   "anti_mage",           ITEM_ANTI_MAGE            , TRUE },
	{   "class_warrior",       ITEM_CLASS_WARRIOR        , TRUE },
	{   "class_thief",         ITEM_CLASS_THIEF          , TRUE },
	{   "class_cleric",        ITEM_CLASS_CLERIC         , TRUE },
	{   "class_mage",          ITEM_CLASS_MAGE           , TRUE },
	{   "class_gladiator",     ITEM_CLASS_GLADIATOR      , TRUE },
	{   "class_samurai",       ITEM_CLASS_SAMURAI        , TRUE },
	{   "class_paladin",       ITEM_CLASS_PALADIN        , TRUE },
	{   "class_assassin",      ITEM_CLASS_ASSASSIN       , TRUE },
	{   "class_ninja",         ITEM_CLASS_NINJA          , TRUE },
	{   "class_monk",          ITEM_CLASS_MONK           , TRUE },
	{   "class_templar",       ITEM_CLASS_TEMPLAR        , TRUE },
	{   "class_illusionist",   ITEM_CLASS_ILLUSIONIST    , TRUE },
	{   "class_gunslinger",    ITEM_CLASS_GUNSLINGER     , TRUE },
	{   "class_ranger",        ITEM_CLASS_RANGER         , TRUE },
	{   "class_necromancer",   ITEM_CLASS_NECROMANCER    , TRUE },
	{   "no_extract",          ITEM_NO_EXTRACT           , TRUE },
        {   "questeq",             ITEM_QUESTEQ              , TRUE },
        {   "random_physical",     ITEM_RANDOM_PHYSICAL      , TRUE },
        {   "random_caster",       ITEM_RANDOM_CASTER        , TRUE },
	{   NULL,           0,          0   }
};



const struct flag_type wear_flags[] =
{
	{   "take",         ITEM_TAKE,          TRUE    },
	{   "finger",       ITEM_WEAR_FINGER,   TRUE    },
	{   "neck",         ITEM_WEAR_NECK,     TRUE    },
	{   "torso",        ITEM_WEAR_TORSO,    TRUE    },
	{   "head",         ITEM_WEAR_HEAD,     TRUE    },
	{   "legs",         ITEM_WEAR_LEGS,     TRUE    },
	{   "feet",         ITEM_WEAR_FEET,     TRUE    },
	{   "hands",        ITEM_WEAR_HANDS,    TRUE    },
	{   "arms",         ITEM_WEAR_ARMS,     TRUE    },
	{   "shield",       ITEM_WEAR_SHIELD,   TRUE    },
	{   "about",        ITEM_WEAR_ABOUT,    TRUE    },
	{   "waist",        ITEM_WEAR_WAIST,    TRUE    },
	{   "wrist",        ITEM_WEAR_WRIST,    TRUE    },
	{   "wield",        ITEM_WIELD,         TRUE    },
	{   "hold",         ITEM_HOLD,          TRUE    },
	{   "nosac",        ITEM_NO_SAC,        TRUE    },
	{   "translucent",  ITEM_TRANSLUCENT,   TRUE    },
	{   "float",        ITEM_WEAR_FLOAT,    TRUE    },
/*    {   "twohands",            ITEM_TWO_HANDS,         TRUE    }, */
	{   NULL,           0,          0   }
};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type apply_flags[] =
{
    {   "none",             APPLY_NONE,     TRUE    },
    {   "level",            APPLY_LEVEL,    TRUE    },
    {   "strength",         APPLY_STR,      TRUE    },
    {   "constitution",     APPLY_CON,      TRUE    },
    {   "vitality",         APPLY_VIT,      TRUE    },
    {   "agility",          APPLY_AGI,      TRUE    },
    {   "dexterity",        APPLY_DEX,      TRUE    },
    {   "intelligence",     APPLY_INT,      TRUE    },
    {   "wisdom",           APPLY_WIS,      TRUE    },
    {   "discipline",       APPLY_DIS,      TRUE    },
    {   "charisma",         APPLY_CHA,      TRUE    },
    {   "luck",             APPLY_LUC,      TRUE    },
    {   "stats",            APPLY_STATS,    TRUE    },
    {   "skills",           APPLY_SKILLS,   TRUE    },
    {   "hp",               APPLY_HIT,      TRUE    },
    {   "mana",             APPLY_MANA,     TRUE    },
    {   "move",             APPLY_MOVE,     TRUE    },
    {   "hitroll",          APPLY_HITROLL,  TRUE    },
    {   "damroll",          APPLY_DAMROLL,  TRUE    },
    {   "ac",               APPLY_AC,       TRUE    },
    {   "saves",            APPLY_SAVES,    TRUE    },
    {   "sex",              APPLY_SEX,      TRUE    },
    {   "age",              APPLY_AGE,      TRUE    },
    {   "height",           APPLY_HEIGHT,   TRUE    },
    {   "weight",           APPLY_WEIGHT,   TRUE    },
    {   NULL,               0,          0   }
};



/*
 * What is seen.
 */
const struct flag_type wear_loc_strings[] =
{
	{   "in the inventory", WEAR_NONE,  TRUE    },
	{   "as a light",       WEAR_LIGHT, TRUE    },
	{   "on the left finger",   WEAR_FINGER_L,  TRUE    },
	{   "on the right finger",  WEAR_FINGER_R,  TRUE    },
	{   "around the neck (1)",  WEAR_NECK_1,    TRUE    },
	{   "around the neck (2)",  WEAR_NECK_2,    TRUE    },
	{   "on the torso",     WEAR_TORSO,  TRUE    },
	{   "over the head",    WEAR_HEAD,  TRUE    },
	{   "on the legs",      WEAR_LEGS,  TRUE    },
	{   "on the feet",      WEAR_FEET,  TRUE    },
	{   "on the hands",     WEAR_HANDS, TRUE    },
	{   "on the arms",      WEAR_ARMS,  TRUE    },
	{   "as a shield",      WEAR_SHIELD,    TRUE    },
	{   "about the shoulders",  WEAR_ABOUT, TRUE    },
	{   "around the waist",     WEAR_WAIST, TRUE    },
	{   "on the left wrist",    WEAR_WRIST_L,   TRUE    },
	{   "on the right wrist",   WEAR_WRIST_R,   TRUE    },
	{   "wielded",              WEAR_WIELD, TRUE    },
	{   "held in the hands",    WEAR_HOLD,  TRUE    },
	{   "floating nearby",      WEAR_FLOAT, TRUE    },
	{   "wielded secondary", WEAR_SECONDARY, TRUE    },
	{   NULL,           0         , 0   }
};


const struct flag_type wear_loc_flags[] =
{
	{   "none",     WEAR_NONE,  TRUE    },
	{   "light",    WEAR_LIGHT, TRUE    },
	{   "lfinger",  WEAR_FINGER_L,  TRUE    },
	{   "rfinger",  WEAR_FINGER_R,  TRUE    },
	{   "neck1",    WEAR_NECK_1,    TRUE    },
	{   "neck2",    WEAR_NECK_2,    TRUE    },
	{   "torso",    WEAR_TORSO,  TRUE    },
	{   "head",     WEAR_HEAD,  TRUE    },
	{   "legs",     WEAR_LEGS,  TRUE    },
	{   "feet",     WEAR_FEET,  TRUE    },
	{   "hands",    WEAR_HANDS, TRUE    },
	{   "arms",     WEAR_ARMS,  TRUE    },
	{   "shield",   WEAR_SHIELD,    TRUE    },
	{   "about",    WEAR_ABOUT, TRUE    },
	{   "waist",    WEAR_WAIST, TRUE    },
	{   "lwrist",   WEAR_WRIST_L,   TRUE    },
	{   "rwrist",   WEAR_WRIST_R,   TRUE    },
	{   "wielded",  WEAR_WIELD, TRUE    },
	{   "hold",     WEAR_HOLD,  TRUE    },
	{   "floating", WEAR_FLOAT, TRUE    },
	{   "secondary", WEAR_SECONDARY, TRUE    },
	{   NULL,       0,      0   }
};

const struct flag_type container_flags[] =
{
	{   "closeable",    CONT_CLOSEABLE,      TRUE    },
	{   "pickproof",    CONT_PICKPROOF,      TRUE    },
	{   "easy",         CONT_EASY,           TRUE    },
	{   "hard",         CONT_HARD,           TRUE    },
	{   "infuriating",  CONT_INFURIATING,    TRUE    },
	{   "closed",       CONT_CLOSED,         TRUE    },
	{   "locked",       CONT_LOCKED,         TRUE    },
	{   "puton",        CONT_PUT_ON,         TRUE    },
	{   NULL,           0,      0   }
};

/*****************************************************************************
					  ROM - specific tables:
 ****************************************************************************/


const struct flag_type size_flags[] =
{
	{   "tiny",          SIZE_TINY,            TRUE    },
	{   "small",         SIZE_SMALL,           TRUE    },
	{   "medium",        SIZE_MEDIUM,          TRUE    },
	{   "large",         SIZE_LARGE,           TRUE    },
	{   "huge",          SIZE_HUGE,            TRUE    },
	{   "giant",         SIZE_GIANT,           TRUE    },
	{   NULL,              0,                    0       },
};


const struct flag_type weapon_class[] =
{
	{   "exotic",   WEAPON_EXOTIC,      TRUE    },
	{   "sword",    WEAPON_SWORD,       TRUE    },
	{   "dagger",   WEAPON_DAGGER,      TRUE    },
	{   "spear",    WEAPON_SPEAR,       TRUE    },
	{   "mace",     WEAPON_MACE,        TRUE    },
	{   "axe",      WEAPON_AXE,         TRUE    },
	{   "flail",    WEAPON_FLAIL,       TRUE    },
	{   "whip",     WEAPON_WHIP,        TRUE    },
	{   "polearm",  WEAPON_POLEARM,     TRUE    },
	{   "gun",      WEAPON_GUN,         TRUE    },
	{   "bow",      WEAPON_BOW,         TRUE    },
	{   NULL,       0,          0       }
};


const struct flag_type weapon_type2[] =
{
	{   "flaming",       WEAPON_FLAMING,       TRUE    },
	{   "frost",         WEAPON_FROST,         TRUE    },
	{   "vampiric",      WEAPON_VAMPIRIC,      TRUE    },
	{   "sharp",         WEAPON_SHARP,         TRUE    },
	{   "vorpal",        WEAPON_VORPAL,        TRUE    },
	{   "twohands",      WEAPON_TWO_HANDS,     TRUE    },
	{   "shocking",      WEAPON_SHOCKING,      TRUE    },
	{   "poison",        WEAPON_POISON,      TRUE    },
	{   "energy drain",  WEAPON_MANASUCK,      TRUE    },
	{   "exhaustion",    WEAPON_MOVESUCK,      TRUE    },
	{   "mind numbing",  WEAPON_DUMB,      TRUE    },
	{   "armor piercing",WEAPON_PUNCTURE,      TRUE    },
 /* Two new weapon flags - Astark */
        {   "paralysis poison",WEAPON_PARALYSIS_POISON, TRUE},
        {   "storming",      WEAPON_STORMING,      TRUE    },
	{   NULL,              0,                    0       }
};

const struct flag_type damage_type[] =
{
        {   "none",          DAM_NONE,             TRUE    },
	{   "other",         DAM_OTHER,            TRUE    },
	{   "harm",          DAM_HARM,             TRUE    },
	{   "bash",          DAM_BASH,             TRUE    },
	{   "pierce",        DAM_PIERCE,           TRUE    },
	{   "slash",         DAM_SLASH,            TRUE    },
	{   "fire",          DAM_FIRE,             TRUE    },
	{   "cold",          DAM_COLD,             TRUE    },
	{   "light",         DAM_LIGHT,            TRUE    },
	{   "lightning",     DAM_LIGHTNING,        TRUE    },
	{   "acid",          DAM_ACID,             TRUE    },
	{   "poison",        DAM_POISON,           TRUE    },
	{   "negative",      DAM_NEGATIVE,         TRUE    },
	{   "holy",          DAM_HOLY,             TRUE    },
	{   "energy",        DAM_ENERGY,           TRUE    },
	{   "mental",        DAM_MENTAL,           TRUE    },
	{   "charm",         DAM_CHARM,            TRUE    },
	{   "disease",       DAM_DISEASE,          TRUE    },
	{   "drowning",      DAM_DROWNING,         TRUE    },
	{   "sound",         DAM_SOUND,            TRUE    },
	{   NULL,          0,            0    }
};

const struct flag_type res_flags[] =
{
	{   "summon",    RES_SUMMON,        TRUE    },
	{   "charm",         RES_CHARM,            TRUE    },
	{   "magic",         RES_MAGIC,            TRUE    },
	{   "weapon",        RES_WEAPON,           TRUE    },
	{   "bash",          RES_BASH,             TRUE    },
	{   "pierce",        RES_PIERCE,           TRUE    },
	{   "slash",         RES_SLASH,            TRUE    },
	{   "fire",          RES_FIRE,             TRUE    },
	{   "cold",          RES_COLD,             TRUE    },
	{   "light",         RES_LIGHT,            TRUE    },
	{   "lightning",     RES_LIGHTNING,        TRUE    },
	{   "acid",          RES_ACID,             TRUE    },
	{   "poison",        RES_POISON,           TRUE    },
	{   "negative",      RES_NEGATIVE,         TRUE    },
	{   "holy",          RES_HOLY,             TRUE    },
	{   "energy",        RES_ENERGY,           TRUE    },
	{   "mental",        RES_MENTAL,           TRUE    },
   {   "disease",       RES_DISEASE,          TRUE    },
	{   "drowning",      RES_DROWNING,         TRUE    },
	{   "sound",    RES_SOUND,      TRUE    },
	{   "wood",     RES_WOOD,       TRUE    },
	{   "silver",   RES_SILVER,     TRUE    },
	{   "iron",     RES_IRON,       TRUE    },
	{   NULL,          0,            0    }
};


const struct flag_type vuln_flags[] =
{
	{   "summon",    VULN_SUMMON,       TRUE    },
	{   "charm",    VULN_CHARM,     TRUE    },
	{   "magic",         VULN_MAGIC,           TRUE    },
	{   "weapon",        VULN_WEAPON,          TRUE    },
	{   "bash",          VULN_BASH,            TRUE    },
	{   "pierce",        VULN_PIERCE,          TRUE    },
	{   "slash",         VULN_SLASH,           TRUE    },
	{   "fire",          VULN_FIRE,            TRUE    },
	{   "cold",          VULN_COLD,            TRUE    },
	{   "light",         VULN_LIGHT,           TRUE    },
	{   "lightning",     VULN_LIGHTNING,       TRUE    },
	{   "acid",          VULN_ACID,            TRUE    },
	{   "poison",        VULN_POISON,          TRUE    },
	{   "negative",      VULN_NEGATIVE,        TRUE    },
	{   "holy",          VULN_HOLY,            TRUE    },
	{   "energy",        VULN_ENERGY,          TRUE    },
	{   "mental",        VULN_MENTAL,          TRUE    },
	{   "disease",       VULN_DISEASE,         TRUE    },
	{   "drowning",      VULN_DROWNING,        TRUE    },
	{   "sound",         VULN_SOUND,           TRUE    },
	{   "wood",          VULN_WOOD,            TRUE    },
	{   "silver",        VULN_SILVER,          TRUE    },
	{   "iron",          VULN_IRON,            TRUE    },
	{   NULL,              0,                    0       }
};

const struct flag_type position_flags[] =
{
	{   "dead",           POS_DEAD,            FALSE   },
	{   "mortal",         POS_MORTAL,          FALSE   },
	{   "incap",          POS_INCAP,           FALSE   },
	{   "stunned",        POS_STUNNED,         FALSE   },
	{   "sleeping",       POS_SLEEPING,        TRUE    },
	{   "resting",        POS_RESTING,         TRUE    },
	{   "sitting",        POS_SITTING,         TRUE    },
	{   "fighting",       POS_FIGHTING,        FALSE   },
	{   "standing",       POS_STANDING,        TRUE    },
	{   NULL,             0,                   0       }
};

const struct flag_type portal_flags[]=
{
	{   "normal_exit",      GATE_NORMAL_EXIT,       TRUE    },
	{   "no_curse",         GATE_NOCURSE,           TRUE    },
	{   "go_with",          GATE_GOWITH,            TRUE    },
	{   "buggy",            GATE_BUGGY,             TRUE    },
	{   "random",           GATE_RANDOM,            TRUE    },
	{   "ignore_no_recall", GATE_IGNORE_NO_RECALL,  TRUE    },    
	{   "warfare",          GATE_WARFARE,           TRUE    },
	{   "astral",           GATE_ASTRAL,            TRUE    },
	{   "stay_area",        GATE_STAY_AREA,         TRUE    },
	{   NULL,         0,            0   }
};

const struct flag_type furniture_flags[]=
{
	{   "stand_at",   STAND_AT,     TRUE    },
	{   "stand_on",   STAND_ON,     TRUE    },
	{   "stand_in",   STAND_IN,     TRUE    },
	{   "sit_at",     SIT_AT,       TRUE    },
	{   "sit_on",     SIT_ON,       TRUE    },
	{   "sit_in",     SIT_IN,       TRUE    },
	{   "rest_at",    REST_AT,      TRUE    },
	{   "rest_on",    REST_ON,      TRUE    },
	{   "rest_in",    REST_IN,      TRUE    },
	{   "sleep_at",   SLEEP_AT,     TRUE    },
	{   "sleep_on",   SLEEP_ON,     TRUE    },
	{   "sleep_in",   SLEEP_IN,     TRUE    },
	{   "put_at",     PUT_AT,       TRUE    },
	{   "put_on",     PUT_ON,       TRUE    },
	{   "put_in",     PUT_IN,       TRUE    },
	{   "put_inside",     PUT_INSIDE,       TRUE    },
	{   NULL,         0,            0   }
};

const   struct  flag_type   apply_types []  =
{
	{   "affects",  TO_AFFECTS, TRUE    },
	{   "object",   TO_OBJECT,  FALSE   },
	{   "immune",   TO_IMMUNE,  TRUE    },
	{   "resist",   TO_RESIST,  TRUE    },
	{   "vuln",     TO_VULN,    TRUE    },
	{   "weapon",   TO_WEAPON,  FALSE   },
    {   "special",  TO_SPECIAL, FALSE   },
	{   NULL,       0,      TRUE    }
};

const   struct  bit_type    bitvector_type  []  =
{
	{   affect_flags,   "affect"    },
	{   apply_flags,    "apply"     },
	{   imm_flags,  "imm"       },
	{   res_flags,  "res"       },
	{   vuln_flags, "vuln"      },
	{   weapon_type2,   "weapon"    }
};

const struct flag_type con_states [] =
{
    { "playing",              CON_PLAYING,              TRUE },
    { "get_name",             CON_GET_NAME,             FALSE },
    { "get_old_password",     CON_GET_OLD_PASSWORD,     FALSE },
    { "confirm_new_name",     CON_CONFIRM_NEW_NAME,     FALSE },
    { "confirm_new_password", CON_CONFIRM_NEW_PASSWORD, FALSE },
    { "get_new_race",         CON_GET_NEW_RACE,         FALSE },
    { "get_new_sex",          CON_GET_NEW_SEX,          FALSE },
    { "get_new_class",        CON_GET_NEW_CLASS,        FALSE },
    { "get_alignment",        CON_GET_ALIGNMENT,        FALSE },
    { "default_choice",       CON_DEFAULT_CHOICE,       FALSE },
    { "gen_groups",           CON_GEN_GROUPS,           FALSE },
    { "pick_weapon",          CON_PICK_WEAPON,          FALSE },
    { "read_imotd",           CON_READ_IMOTD,           FALSE },
    { "break_connect",        CON_BREAK_CONNECT,        FALSE },
    { "get_creation_mode",    CON_GET_CREATION_MODE,    FALSE },
    { "roll_stats",           CON_ROLL_STATS,           FALSE },
    { "get_stat_priority",    CON_GET_STAT_PRIORITY,    FALSE },
    { "copyover_recover",     CON_COPYOVER_RECOVER,     FALSE },
    { "note_to",              CON_NOTE_TO,              FALSE },
    { "note_subject",         CON_NOTE_SUBJECT,         FALSE },
    { "note_expire",          CON_NOTE_EXPIRE,          FALSE },
    { "note_text",            CON_NOTE_TEXT,            FALSE },
    { "note_finish",          CON_NOTE_FINISH,          FALSE },
    { "penalty_severity",     CON_PENALTY_SEVERITY,     FALSE },
    { "penalty_confirm",      CON_PENALTY_CONFIRM,      FALSE },
    { "penalty_hours",        CON_PENALTY_HOURS,        FALSE },
    { "penalty_points",       CON_PENALTY_POINTS,       FALSE },
    { "penalty_penlist",      CON_PENALTY_PENLIST,      FALSE },
    { "penalty_finish",       CON_PENALTY_FINISH,       FALSE },
    { "get_colour",           CON_GET_COLOUR,           FALSE },
    { "lua_handler",          CON_LUA_HANDLER,          TRUE  },
    { "closed",               CON_CLOSED,               FALSE },
    { NULL, 0, TRUE }
};

const struct stance_type stances [] =
{
	{ "default",          STANCE_DEFAULT,             DAM_BASH,      "punch",
		&gsn_hand_to_hand,         TRUE, TRUE,  0  },
	{ "bear",             STANCE_BEAR,                DAM_BASH,      "maul", 
		&gsn_bear,                 TRUE, FALSE,  5  },
	{ "boa",              STANCE_BOA,                 DAM_BASH,      "constriction", 
		&gsn_boa,                  TRUE, FALSE,  3  },
	{ "dragon",           STANCE_DRAGON,              DAM_FIRE,      "flame", 
		&gsn_dragon,               TRUE, FALSE,  8  },
	{ "eagle",            STANCE_EAGLE,               DAM_SLASH,     "talon", 
		&gsn_eagle,                TRUE, FALSE,  5  },
	{ "eel",              STANCE_EEL,                 DAM_LIGHTNING, "shock", 
		&gsn_eel,                  TRUE, FALSE,  6  },
	{ "lion",             STANCE_LION,                DAM_PIERCE,    "bite", 
		&gsn_lion,                 TRUE, FALSE,  20 },
	{ "phoenix",          STANCE_PHOENIX,             DAM_FIRE,      "scorch", 
		&gsn_phoenix,              TRUE, FALSE,  6  },
	{ "porcupine",        STANCE_PORCUPINE,           DAM_PIERCE,    "twill", 
		&gsn_porcupine,            TRUE, FALSE,  11 },
	{ "rhino",            STANCE_RHINO,               DAM_PIERCE,    "charge", 
		&gsn_rhino,                TRUE, FALSE,  10 },
	{ "scorpion",         STANCE_SCORPION,            DAM_POISON,    "sting", 
		&gsn_scorpion,             TRUE, FALSE,  4  },
	{ "tiger",            STANCE_TIGER,               DAM_SLASH,     "claw", 
		&gsn_tiger,                TRUE, FALSE,  9  },
	{ "toad",             STANCE_TOAD,                DAM_BASH,      "kick", 
		&gsn_toad,                 TRUE, FALSE,  2  },
	{ "tortoise",         STANCE_TORTOISE,            DAM_PIERCE,    "snap", 
		&gsn_tortoise,             TRUE, FALSE,  2  },
	{ "unicorn",          STANCE_UNICORN,             DAM_ENERGY,    "horn", 
		&gsn_unicorn,              TRUE, FALSE,  12 },
	{ "finesse",          STANCE_FINESSE,             0,             "", 
		&gsn_finesse,              FALSE, TRUE,  6  },
	{ "rage",             STANCE_RAGE,                0,             "", 
		&gsn_rage,                 FALSE, TRUE,  6  },
	{ "retribution",      STANCE_RETRIBUTION,         0,             "", 
		&gsn_retribution,          FALSE, TRUE,  15 },
	{ "blade dance",      STANCE_BLADE_DANCE,         0,             "", 
		&gsn_blade_dance,          FALSE, TRUE,  25 },
	{ "shadowclaw",       STANCE_SHADOWCLAW,          DAM_HARM,      "shadowclaw", 
		&gsn_shadowclaw,           TRUE, TRUE,   4  },
	{ "shadowessence",    STANCE_SHADOWESSENCE,       DAM_COLD,      "shadowessence", 
		&gsn_shadowessence,        TRUE, TRUE,   4  },
	{ "shadowsoul",       STANCE_SHADOWSOUL,          DAM_NEGATIVE,  "shadowsoul", 
		&gsn_shadowsoul,           TRUE, TRUE,   5  },
	{ "shadowwalk",       STANCE_SHADOWWALK,          DAM_NEGATIVE,  "darkness", 
		&gsn_shadowwalk,           TRUE, TRUE,   8  },
	{ "ambush",           STANCE_AMBUSH,              0,             "", 
	  &gsn_ambush,               FALSE, TRUE,        25 },
	{ "bloodbath",        STANCE_BLOODBATH,           0,             "", 
		&gsn_bloodbath,            FALSE, TRUE,  16 },
	{ "kamikaze",         STANCE_KAMIKAZE,            0,             "", 
		&gsn_kamikaze,             FALSE, TRUE,  25 },
	{ "showdown",         STANCE_SHOWDOWN,            0,             "", 
		&gsn_showdown,             FALSE, TRUE,  6  },
	{ "target practice",  STANCE_TARGET_PRACTICE,     0,             "", 
		&gsn_target_practice,      FALSE, TRUE,  15 },
	{ "jihad",            STANCE_JIHAD,               0,             "", 
		&gsn_jihad,                FALSE, TRUE,  10 },
	{ "vampire hunting",  STANCE_VAMPIRE_HUNTING,     0,             "", 
		&gsn_vampire_hunting,      FALSE, TRUE,  12  },
	{ "witch hunting",    STANCE_WITCH_HUNTING,       0,             "", 
		&gsn_witch_hunting,        FALSE, TRUE,  12  },
	{ "werewolf hunting", STANCE_WEREWOLF_HUNTING,    0,             "", 
		&gsn_werewolf_hunting,     FALSE, TRUE,  12  },
	{ "inquisition",      STANCE_INQUISITION,         0,             "", 
		&gsn_inquisition,          FALSE, TRUE,  10 },
	{ "tempest",          STANCE_TEMPEST,             DAM_DROWNING,  "deluge", 
		&gsn_tempest,              TRUE, TRUE,   10 },
	{ "goblincleaver",    STANCE_GOBLINCLEAVER,       0,             "", 
		&gsn_goblincleaver,        FALSE, TRUE,   8 },
	{ "wendigo",          STANCE_WENDIGO,             DAM_LIGHTNING, "tornado", 
		&gsn_wendigo,              TRUE, TRUE,   20 },
	{ "korinn",           STANCE_KORINNS_INSPIRATION, DAM_HOLY,      "holy inspiration", 
		&gsn_korinns_inspiration,  FALSE, TRUE,   8 },
	{ "parademia",        STANCE_PARADEMIAS_BILE,     DAM_ACID,      "acidic bile", 
		&gsn_parademias_bile,      FALSE, TRUE,   8 },
	{ "swayde",           STANCE_SWAYDES_MERCY,       0,             "", 
		&gsn_swaydes_mercy,        FALSE, TRUE,   8 },
	{ "firewitch",        STANCE_FIREWITCHS_SEANCE,   0,             "", 
		&gsn_firewitchs_seance,    FALSE, TRUE,  10 },
	{ "bunny",            STANCE_BUNNY,               DAM_PIERCE,    "rabbit punch", 
		&gsn_bunny,                TRUE, TRUE,   12 },
	{ "anklebiter",       STANCE_ANKLEBITER,          0,             "", 
	  &gsn_anklebiter,           FALSE, TRUE,         8 },
	{ "arcana",           STANCE_ARCANA,              DAM_ENERGY,    "arcane magic", 
	  &gsn_arcana,                TRUE, TRUE,         6 },
	{ "dimensional blade",STANCE_DIMENSIONAL_BLADE,   0,             "", 
	  &gsn_dimensional_blade,     FALSE, TRUE,        25 },
	{ "elemental blade",  STANCE_ELEMENTAL_BLADE,     0,             "", 
	  &gsn_elemental_blade,     FALSE, TRUE,          15 },
        { "aversion",         STANCE_AVERSION,            0,             "",
          &gsn_aversion,            FALSE, TRUE,          12 },
        { "serpent",          STANCE_SERPENT,             DAM_DROWNING,  "flooding",
          &gsn_serpent,             TRUE, FALSE,          22 },
	{ NULL,               0,                          0,             "", 
		NULL,                      FALSE, FALSE,  0  }
};

const char* spell_target_names[] =
{
    "ignore",
    "char_offensive",
    "char_defensive",
    "char_self",
    "obj_inventory",
    "obj_char_defensive",
    "obj_char_offensive",
    "visible_char_offensive",
    "char_neutral",
    "ignore_offensive",
    "ignore_obj"
};

const char* spell_duration_names[] =
{
    "none",
    "special",
    "brief",
    "short",
    "normal",
    "long",
    "extreme"
};

const struct pkgrade_type pkgrade_table[] =
{
    /*  grade,  pkpoints        earned (by      lost (by        lost_in_warfare */
    /*                          killing),       dying),                         */
	{"",	   0,		  0,		  0,		  0},
	/* First entry is a null entry, so "Grade A" is grade_level 1. */
        {"{RA{x",   1650,           150,            150,            150},
        {"{RB{x",   1400,           125,            125,            110},
        {"{RC{x",   1200,           100,            100,             75},
        {"{YD{x",   1040,            85,             85,             51},
        {"{YE{x",    910,            65,             65,             33},
        {"{YF{x",    800,            55,             55,             28},
        {"{GG{x",    700,            50,             50,             25},
        {"{GH{x",    608,            46,             46,             23},
        {"{GI{x",    524,            42,             42,             21},
        {"{GJ{x",    448,            38,             38,             19},
        {"{GK{x",    380,            34,             34,             17},
        {"{CL{x",    320,            30,             30,             15},
        {"{CM{x",    266,            27,             27,             14},
        {"{CN{x",    218,            24,             24,             12},
        {"{CO{x",    176,            21,             21,             11},
        {"{CP{x",    140,            18,             18,              9},
        {"{BQ{x",    110,            15,             15,              8},
        {"{BR{x",     84,            13,             12,              6},
        {"{BS{x",     62,            11,              9,              5},
        {"{BT{x",     44,             9,              6,              3},
        {"{BU{x",     30,             7,              4,              2},
        {"{MV{x",     20,             5,              3,              2},
        {"{MW{x",     12,             4,              2,              1},
        {"{MX{x",      6,             3,              1,              1},
        {"{MY{x",      2,             2,              0,              0},
        {"{MZ{x",      0,             1,              0,              0} 
};

/*  type, limit, qps, exp, gold, obj, bit_vector */
const ACHIEVEMENT achievement_table [] =
{
    { ACHV_LEVEL,       2,   2,   25,     5,     1,       0,    ACHIEVE_LEVEL_1},
    { ACHV_LEVEL,      10,   3,   50,     5,     1,       0,    ACHIEVE_LEVEL_2},
    { ACHV_LEVEL,      25,   5,  100,    50,     3,       0,    ACHIEVE_LEVEL_3},
    { ACHV_LEVEL,      50,  10,  300,   250,     5,       0,    ACHIEVE_LEVEL_4},
    { ACHV_LEVEL,      90,  25,    0,   500,    10,       0,    ACHIEVE_LEVEL_5},
    { ACHV_LEVEL,      91,  30,    0,  1000,    15,       0,    ACHIEVE_LEVEL_6},
    { ACHV_LEVEL,      92,  35,    0,  1500,    20,       0,    ACHIEVE_LEVEL_7},
    { ACHV_LEVEL,      93,  40,    0,  2000,    25,       0,    ACHIEVE_LEVEL_8},
    { ACHV_LEVEL,      94,  45,    0,  2500,    30,       0,    ACHIEVE_LEVEL_9},
    { ACHV_LEVEL,      95,  50,    0,  3000,    35,       0,   ACHIEVE_LEVEL_10},
    { ACHV_LEVEL,      96,  55,    0,  4000,    40,       0,   ACHIEVE_LEVEL_11},
    { ACHV_LEVEL,      97,  60,    0,  5000,    45,       0,   ACHIEVE_LEVEL_12},
    { ACHV_LEVEL,      98,  65,    0,  6000,    50,       0,   ACHIEVE_LEVEL_13},
    { ACHV_LEVEL,      99,  70,    0,  7000,    50,       0,   ACHIEVE_LEVEL_14},
    { ACHV_LEVEL,     100,  75,    0,  8000,   100,       0,   ACHIEVE_LEVEL_15},
    /*                     570,  475, 40810,   430,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_MKILL,       1,   1,   15,     5,     1,       0,    ACHIEVE_MKILL_1},
    { ACHV_MKILL,      50,   2,   25,    10,     2,       0,    ACHIEVE_MKILL_2},
    { ACHV_MKILL,     100,   3,   50,    50,     3,       0,    ACHIEVE_MKILL_3},
    { ACHV_MKILL,     250,   5,   75,   150,     4,       0,    ACHIEVE_MKILL_4},
    { ACHV_MKILL,     500,  10,  100,   250,     5,       0,    ACHIEVE_MKILL_5},
    { ACHV_MKILL,    1000,  25,  150,  1000,    10,       0,    ACHIEVE_MKILL_6},
    { ACHV_MKILL,    2500,  25,  300,  2500,    25,       0,    ACHIEVE_MKILL_7},
    { ACHV_MKILL,    5000,  50,  450,  5000,    50,       0,    ACHIEVE_MKILL_8},
    { ACHV_MKILL,   10000,  75,  700, 10000,    75,       0,    ACHIEVE_MKILL_9},
    { ACHV_MKILL,   25000, 125, 1000, 25000,   150,       0,   ACHIEVE_MKILL_10},
    { ACHV_MKILL,   50000, 250, 2500, 50000,   300,       0,   ACHIEVE_MKILL_11},
    /*                     321, 5365, 93965,   625,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_REMORT,      1,  10,  100,   250,    10,       0,   ACHIEVE_REMORT_1},
    { ACHV_REMORT,      2,  15,  200,   500,    15,       0,   ACHIEVE_REMORT_2},
    { ACHV_REMORT,      3,  20,  300,  1000,    20,       0,   ACHIEVE_REMORT_3},
    { ACHV_REMORT,      4,  25,  400,  2000,    25,       0,   ACHIEVE_REMORT_4},
    { ACHV_REMORT,      5,  30,  500,  4000,    30,       0,   ACHIEVE_REMORT_5},
    { ACHV_REMORT,      6,  35,  600,  8000,    35,       0,   ACHIEVE_REMORT_6},
    { ACHV_REMORT,      7,  40,  700, 10000,    40,       0,   ACHIEVE_REMORT_7},
    { ACHV_REMORT,      8,  45,  800, 15000,    45,       0,   ACHIEVE_REMORT_8},
    { ACHV_REMORT,      9,  50,  900, 20000,    50,       0,   ACHIEVE_REMORT_9},
    { ACHV_REMORT,     10, 100, 1000, 25000,    55,       0,  ACHIEVE_REMORT_10},
    /*                     640, 5500, 85750,   325,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_QCOMP,       1,   5,   10,    10,     5,       0,    ACHIEVE_QCOMP_1},
    { ACHV_QCOMP,      10,  15,   25,   100,    10,       0,    ACHIEVE_QCOMP_2},
    { ACHV_QCOMP,      50,  20,  100,   500,    25,       0,    ACHIEVE_QCOMP_3},
    { ACHV_QCOMP,     100,  25,  200,  1000,    50,       0,    ACHIEVE_QCOMP_4},
    { ACHV_QCOMP,     250,  50,  500,  2500,    75,       0,    ACHIEVE_QCOMP_5},
    { ACHV_QCOMP,     500, 100, 1000,  5000,   100,       0,    ACHIEVE_QCOMP_6},
    { ACHV_QCOMP,     750, 150, 1500,  7500,   125,       0,    ACHIEVE_QCOMP_7},
    { ACHV_QCOMP,    1000, 200, 2000, 10000,   150,       0,    ACHIEVE_QCOMP_8},
    { ACHV_QCOMP,    2500, 300, 3000, 25000,   250,       0,    ACHIEVE_QCOMP_9},
    { ACHV_QCOMP,    5000, 400, 5000, 50000,   500,       0,   ACHIEVE_QCOMP_10},
    /*                    1265, 5365,101610,  1290,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */	
    { ACHV_WKILL,       1,  10,   25,    25,     5,       0, ACHIEVE_WARKILLS_1},
    { ACHV_WKILL,       5,  15,   50,    50,     5,       0, ACHIEVE_WARKILLS_2},
    { ACHV_WKILL,      10,  20,   75,   250,    10,       0, ACHIEVE_WARKILLS_3},
    { ACHV_WKILL,      25,  25,  100,  1000,    25,       0, ACHIEVE_WARKILLS_4},
    { ACHV_WKILL,      50,  50,  150,  2500,    50,       0, ACHIEVE_WARKILLS_5},
    { ACHV_WKILL,     100,  75,  300,  5000,   100,       0, ACHIEVE_WARKILLS_6},
    { ACHV_WKILL,     250, 100,  500, 10000,   250,       0, ACHIEVE_WARKILLS_7},
    { ACHV_WKILL,     500, 125, 1000, 20000,   500,       0, ACHIEVE_WARKILLS_8},
    /*                     420, 2200, 33825,   945,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_WWIN,        1,  10,   15,    25,     5,       0,  ACHIEVE_WARWINS_1},
    { ACHV_WWIN,        5,  15,   15,    50,    10,       0,  ACHIEVE_WARWINS_2},
    { ACHV_WWIN,       10,  20,   15,   250,    15,       0,  ACHIEVE_WARWINS_3},
    { ACHV_WWIN,       25,  25,   15,  1000,    20,       0,  ACHIEVE_WARWINS_4},
    { ACHV_WWIN,       50,  50,   15,  2500,    25,       0,  ACHIEVE_WARWINS_5},
    { ACHV_WWIN,      100,  75,   15,  5000,    50,       0,  ACHIEVE_WARWINS_6},
    { ACHV_WWIN,      250, 100,   15, 10000,   100,       0,  ACHIEVE_WARWINS_7},
    { ACHV_WWIN,      500, 125,   15, 20000,   250,       0,  ACHIEVE_WARWINS_8},
    /*                     420, 2200, 33825,   475,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_BEHEAD,      1,   5,   25,    25,     5,       0,  ACHIEVE_BEHEADS_1},
    { ACHV_BEHEAD,     10,  10,   50,    50,    10,       0,  ACHIEVE_BEHEADS_2},
    { ACHV_BEHEAD,     25,  15,   75,   100,    15,       0,  ACHIEVE_BEHEADS_3},
    { ACHV_BEHEAD,     50,  20,  100,   500,    20,       0,  ACHIEVE_BEHEADS_4},
    { ACHV_BEHEAD,    100,  25,  250,  1000,    25,       0,  ACHIEVE_BEHEADS_5},
    { ACHV_BEHEAD,    250,  50,  500,  2500,    50,       0,  ACHIEVE_BEHEADS_6},
    { ACHV_BEHEAD,    500,  75, 1000,  5000,   100,       0,  ACHIEVE_BEHEADS_7},
    { ACHV_BEHEAD,   1000, 100, 2000, 10000,   250,       0,  ACHIEVE_BEHEADS_8},
    /*                     300, 4000, 19175,   475,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */	
    { ACHV_PKILL,       1,  10,   50,   500,     5,       0,   ACHIEVE_PKILLS_1},
    { ACHV_PKILL,      10,  25,  250,  1000,    25,       0,   ACHIEVE_PKILLS_2},
    { ACHV_PKILL,      25,  50,  500,  2500,    50,       0,   ACHIEVE_PKILLS_3},
    { ACHV_PKILL,      50, 100, 1000,  5000,   100,       0,   ACHIEVE_PKILLS_4},
    { ACHV_PKILL,     100, 250, 2500, 10000,   250,       0,   ACHIEVE_PKILLS_5},
    /*                     435, 4300, 19000,   430,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */	
    { ACHV_AGE,        20,   5,   50,   250,     5,       0,      ACHIEVE_AGE_1},
    { ACHV_AGE,        50,  10,   50,   500,     5,       0,      ACHIEVE_AGE_2},
    { ACHV_AGE,       100,  25,   50,  1000,     5,       0,      ACHIEVE_AGE_3},
    { ACHV_AGE,       250,  50,   50,  1500,    10,       0,      ACHIEVE_AGE_4},
    { ACHV_AGE,       500,  50,   50,  3000,    25,       0,      ACHIEVE_AGE_5},
    /*                     140,  250,  6750,    50,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
//  { ACHV_TATT,        1,   0,    0,     0,     0,       0,     ACHIEVE_TATT_1},
//  { ACHV_TATT,        5,   0,    0,     0,     0,       0,     ACHIEVE_TATT_2},
//  { ACHV_TATT,       13,   0,    0,     0,     0,       0,     ACHIEVE_TATT_3},
    /*                       0,    0,     0,     0,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_MAXHP,     500,  10,   25,    50,     5,       0,    ACHIEVE_MAXHP_1},
    { ACHV_MAXHP,    1000,  15,   50,   100,    10,       0,    ACHIEVE_MAXHP_2},
    { ACHV_MAXHP,    5000,  20,  100,   500,    15,       0,    ACHIEVE_MAXHP_3},
    { ACHV_MAXHP,   10000,  25,  150,  1000,    20,       0,    ACHIEVE_MAXHP_4},
    { ACHV_MAXHP,   15000,  50,  300,  2500,    25,       0,    ACHIEVE_MAXHP_5},
    /*                     120,  625,  4150,    75,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_MAXMN,     500,  10,   25,    50,     5,       0,    ACHIEVE_MAXMN_1},
    { ACHV_MAXMN,    1000,  15,   50,   100,    10,       0,    ACHIEVE_MAXMN_2},
    { ACHV_MAXMN,    5000,  20,  100,   500,    15,       0,    ACHIEVE_MAXMN_3},
    { ACHV_MAXMN,   10000,  25,  150,  1000,    20,       0,    ACHIEVE_MAXMN_4},
    { ACHV_MAXMN,   15000,  50,  300,  2500,    25,       0,    ACHIEVE_MAXMN_5},
    /*                     120,  625,  4150,    75,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_MAXMV,     500,  10,   25,    50,     5,       0,    ACHIEVE_MAXMV_1},
    { ACHV_MAXMV,    1000,  15,   50,   100,    10,       0,    ACHIEVE_MAXMV_2},
    { ACHV_MAXMV,    5000,  20,  100,   500,    15,       0,    ACHIEVE_MAXMV_3},
    { ACHV_MAXMV,   10000,  25,  150,  1000,    20,       0,    ACHIEVE_MAXMV_4},
    /*                      70,  325,  1650,    25,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_EXPLORED,   10,   5,   25,    25,     5,       0, ACHIEVE_EXPLORED_1},
    { ACHV_EXPLORED,  100,   5,   50,    50,    10,       0, ACHIEVE_EXPLORED_2},
    { ACHV_EXPLORED,  500,  10,  100,    75,    15,       0, ACHIEVE_EXPLORED_3},
    { ACHV_EXPLORED, 1000,  25,  250,   100,    20,       0, ACHIEVE_EXPLORED_4},
    { ACHV_EXPLORED, 2500,  50,  250,  1000,    25,       0, ACHIEVE_EXPLORED_5},
    { ACHV_EXPLORED, 5000, 100,  250,  2500,    30,       0, ACHIEVE_EXPLORED_6},
    { ACHV_EXPLORED,10000, 150,  500,  5000,    35,       0, ACHIEVE_EXPLORED_7},
    { ACHV_EXPLORED,15000, 250, 1000, 10000,    50,       0, ACHIEVE_EXPLORED_8},
    /*                     595, 2425, 18750,   190,      NA,               */
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_MASKILLS,    1,   5,   25,   100,     5,       0, ACHIEVE_MASKILLS_1},
    { ACHV_MASKILLS,    3,  15,   75,   500,    10,       0, ACHIEVE_MASKILLS_2},
    { ACHV_MASKILLS,    5,  25,  125,  1000,    15,       0, ACHIEVE_MASKILLS_3},
    { ACHV_MASKILLS,   10,  50,  250,  2500,    20,       0, ACHIEVE_MASKILLS_4},
    { ACHV_MASKILLS,   15,  75,  500,  5000,    25,       0, ACHIEVE_MASKILLS_5},
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_GMSKILLS,    1,   5,   25,   100,     5,       0, ACHIEVE_GMSKILLS_1},
    { ACHV_GMSKILLS,    3,  15,   75,   500,    10,       0, ACHIEVE_GMSKILLS_2},
    { ACHV_GMSKILLS,    5,  25,  125,  1000,    15,       0, ACHIEVE_GMSKILLS_3},
    { ACHV_GMSKILLS,   10,  50,  250,  2500,    20,       0, ACHIEVE_GMSKILLS_4},
    { ACHV_GMSKILLS,   15,  75,  500,  5000,    25,       0, ACHIEVE_GMSKILLS_5},
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_RETRAINED,   1,   5,   25,   100,     5,       0, ACHIEVE_RETRAINED_1},
    { ACHV_RETRAINED,   3,  15,   75,   500,    10,       0, ACHIEVE_RETRAINED_2},
    { ACHV_RETRAINED,   5,  25,  125,  1000,    15,       0, ACHIEVE_RETRAINED_3},
    { ACHV_RETRAINED,  10,  50,  250,  2500,    20,       0, ACHIEVE_RETRAINED_4},
    { ACHV_RETRAINED,  15,  75,  500,  5000,    25,       0, ACHIEVE_RETRAINED_5},
    /*      type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
    { ACHV_QHCOMP,      1,   5,   25,   250,     5,       0,   ACHIEVE_QHCOMP_1},
    { ACHV_QHCOMP,     10,  10,   50,   500,    10,       0,   ACHIEVE_QHCOMP_2},
    { ACHV_QHCOMP,     50,  25,  100,   250,    25,       0,   ACHIEVE_QHCOMP_3},
    { ACHV_QHCOMP,    100,  50,  250,   250,    50,       0,   ACHIEVE_QHCOMP_4},
    { ACHV_QHCOMP,    250,  75,  500,   250,   100,       0,   ACHIEVE_QHCOMP_5},
    { ACHV_QHCOMP,    500, 100,  750,   250,   250,       0,   ACHIEVE_QHCOMP_6},
    { 0,                0,   0,    0,     0,     0,       0,                  0}
};
/*          type,   limit, qps,  exp,  gold, achpoints, obj,    bit_vector */
/* TOTAL FOR ALL ACHIEVE: 4821,39200,444660,      5220,  NA,               */


/*
msl_string achievement_display [] =
{
	"none",
	"Level",
	"M.Kills",
	"Remorts",
	"Q.Completed",
	"Warkills",
	"War Wins",
	"Beheads",
	"Pkills",
	"Age",
	"Max HP",
	"Max Mana",
	"Max Moves",
	"Tattoos"
};*/
/*
#define ACHV_NONE       0
#define ACHV_LEVEL      1
#define ACHV_MKILL      2
#define ACHV_REMORT     3
#define ACHV_QCOMP      4
#define ACHV_WKILL      5
#define ACHV_WWIN       6
#define ACHV_BEHEAD     7
#define ACHV_PKILL      8
#define ACHV_AGE        9
#define ACHV_MAXHP      10
#define ACHV_MAXMN      11
#define ACHV_MAXMV      12
#define ACHV_EXPLORED   13
#define ACHV_TATT       14
*/

const CHANNEL public_channel_table[] =
{
	/*sn		name		1st pers	3rd pers	color	color2	offbit			minlvl	extra check*/
	{&sn_gossip,	"Gossip",	"gossip",	"gossips",	'p',	'P',	COMM_NOGOSSIP,		3,	NULL},
	{&sn_auction,	"Auction",	"auction",	"auctions", 	'a',	'A',	COMM_NOAUCTION,		3,	NULL},
	{&sn_music,	"Music",	"MUSIC:",	"MUSIC:",	'e',	'E',	COMM_NOMUSIC,		3,	NULL},
	{&sn_question,	"Q/{jA",	"question",	"questions",	'q',	'Q',	COMM_NOQUESTION,	3,	NULL},
	{&sn_answer,	"Q/{jA",	"answer",	"answers",	'j',	'J',	COMM_NOQUESTION,	3,	NULL},
	{&sn_quote,	"Quote",	"quote",	"quotes",	'h',	'H',	COMM_NOQUOTE,		3,	NULL},
	{&sn_gratz,	"Gratz",	"gratz",	"gratzes",	'z',	'Z',	COMM_NOGRATZ,		3,	NULL},
	{&sn_gametalk,	"Gametalk",	"gametalk",	"gametalks",	'k',	'K',	COMM_NOGAME,		3,	NULL},
	{&sn_bitch,	"Bitch",	"bitch",	"bitches",	'f',	'F',	COMM_NOBITCH,		3,	NULL},
	{&sn_newbie,	"Newbie",	"[Newbie]:",	"[Newbie]:",	'n',	'N',	COMM_NONEWBIE,		0,	NULL},
	{&sn_immtalk,	"Immtalk",	"immtalk",	"immtalks",	'i',	'I',	COMM_NOWIZ,		0,	check_immtalk},
	{&sn_savantalk,	"Savantalk",	"savant",	"savants",	'7',	'8',	COMM_NOSAVANT,		0,	check_savant},
    {NULL}
};
