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

/* #define MAGIC_CHECKING */
/* comment out to turn "magic number" debugging of memory allocation/recycling off */
/* Bobble: creates error when trying to allocate blocks larger than 16k */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <execinfo.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include <lua.h>

#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "olc.h"
#include "buffer_util.h"
#include "mob_stats.h"

extern  int _filbuf     args( (FILE *) );

#if !defined(OLD_RAND)
void srandom(unsigned int);
int getpid();
time_t time(time_t *tloc);
#endif


/* externals for counting purposes */
extern  OBJ_DATA    *obj_free;
extern  CHAR_DATA   *char_free;
extern  DESCRIPTOR_DATA *descriptor_free;
extern  PC_DATA     *pcdata_free;
extern  AFFECT_DATA *affect_free;

static int STR_DUP_STRINGS = 0;
static int HIGHEST_STR_DUP_STRINGS = 0;

void format_init_flags( void );
void format_race_flags( void );
void load_area_file( FILE *fp, bool clone );
void rename_obj( OBJ_DATA *obj, char *name, char *short_descr, char *description );
void affect_spellup_mob( CHAR_DATA *mob );

/*
* Globals.
*/
HELP_DATA *     help_first;
HELP_DATA *     help_last;

HELP_AREA *     had_list;

SHOP_DATA *     shop_first;
SHOP_DATA *     shop_last;

NOTE_DATA *     note_free;

PROG_CODE *    mprog_list;
PROG_CODE *    oprog_list;
PROG_CODE *    aprog_list;
PROG_CODE *    rprog_list;

char            bug_buf     [2*MAX_INPUT_LENGTH];
CHAR_DATA *     char_list;
const char *    help_greeting;
char            log_buf     [2*MAX_INPUT_LENGTH];
KILL_DATA       kill_table  [MAX_LEVEL];
OBJ_DATA *      object_list;
TIME_INFO_DATA  time_info;
WEATHER_DATA    weather_info;
SORT_TABLE *    bounty_table;
int             jail_room_list[MAX_JAIL_ROOM];
/* bool            wait_for_auth = AUTH_STATUS_IMM_ON; */
bool            wait_for_auth = AUTH_STATUS_ENABLED;
bool            exits_fixed = FALSE;

sh_int  gsn_frenzy;
sh_int  gsn_mindflay;
sh_int  gsn_petrify;
sh_int  gsn_backstab;
sh_int  gsn_blackjack;
sh_int  gsn_circle;
sh_int  gsn_dodge;
sh_int  gsn_envenom;
sh_int  gsn_hide;
sh_int  gsn_peek;
sh_int  gsn_pick_lock;
sh_int  gsn_sneak;
sh_int  gsn_steal;
sh_int  gsn_disguise;
sh_int  gsn_disarm_trap;

sh_int  gsn_disarm;
sh_int  gsn_enhanced_damage;
sh_int  gsn_flanking;
sh_int  gsn_kick;
sh_int  gsn_gouge;
sh_int  gsn_chop;
sh_int  gsn_bite;
sh_int  gsn_acid_breath;
sh_int  gsn_fire_breath;
sh_int  gsn_frost_breath;
sh_int  gsn_gas_breath;
sh_int  gsn_lightning_breath;
sh_int  gsn_melee;
sh_int  gsn_brawl;
sh_int  gsn_guard;
sh_int  gsn_kung_fu;
sh_int  gsn_leg_sweep;
sh_int  gsn_endurance;
sh_int  gsn_uppercut;
sh_int  gsn_war_cry;
sh_int  gsn_draconic_breath;
sh_int  gsn_dual_wield;
sh_int  gsn_dual_dagger;
sh_int  gsn_dual_sword;
sh_int  gsn_dual_axe;
sh_int  gsn_dual_gun;
sh_int  gsn_tumbling;
sh_int  gsn_feint;
sh_int  gsn_distract;
sh_int  gsn_avoidance;
sh_int  gsn_parry;
sh_int  gsn_rescue;
sh_int  gsn_second_attack;
sh_int  gsn_third_attack;
sh_int  gsn_sustenance;   
sh_int  gsn_hunt;     
sh_int  gsn_pathfind;
sh_int  gsn_streetwise;
sh_int  gsn_ignite;
sh_int  gsn_forage;
sh_int  gsn_torch;
sh_int  gsn_shelter;
sh_int  gsn_firstaid;
sh_int  gsn_detoxify;
sh_int  gsn_tame;
sh_int  gsn_regeneration;
sh_int  gsn_drain_life;
sh_int  gsn_headbutt;
sh_int  gsn_net;
sh_int  gsn_mug;
sh_int  gsn_set_snare;
sh_int  gsn_aim;
sh_int  gsn_semiauto;
sh_int  gsn_fullauto;
sh_int  gsn_hogtie;
sh_int  gsn_elude;
sh_int  gsn_estimate;
sh_int  gsn_snipe;
sh_int  gsn_unjam;
sh_int  gsn_pistol_whip;
sh_int  gsn_burst;
sh_int  gsn_tight_grouping;
sh_int  gsn_drunken_fury;
sh_int  gsn_duck;
sh_int  gsn_true_grit;
sh_int  gsn_quick_draw;
sh_int  gsn_shoot_lock;
sh_int  gsn_thousand_yard_stare;
sh_int  gsn_raft;  
sh_int  gsn_giantfeller;
sh_int  gsn_woodland_combat;
sh_int  gsn_taxidermy;
sh_int  gsn_introspection;
sh_int  gsn_climbing;
sh_int  gsn_blindfighting;
sh_int  gsn_beast_mastery;
sh_int  gsn_camp_fire;
sh_int  gsn_treat_weapon;
sh_int  gsn_soothe;
sh_int  gsn_fishing;
sh_int  gsn_goblincleaver;
sh_int  gsn_tempest;
sh_int  gsn_wendigo;
sh_int  gsn_shield_bash;
sh_int  gsn_choke_hold;
sh_int  gsn_roundhouse;
sh_int  gsn_hurl;
sh_int  gsn_spit;
sh_int  gsn_peel;
sh_int  gsn_two_handed;
sh_int  gsn_infectious_arrow;
sh_int  gsn_critical;
//sh_int  gsn_power_thrust;
sh_int  gsn_power_attack;
sh_int  gsn_natural_resistance;
sh_int  gsn_quivering_palm;
sh_int  gsn_basic_apparition;
sh_int  gsn_holy_apparition;
sh_int  gsn_iron_hide;
sh_int  gsn_minor_fade;

sh_int  gsn_blindness;
sh_int  gsn_charm_person;
sh_int  gsn_curse;
sh_int  gsn_invis;
sh_int  gsn_astral;
sh_int  gsn_mass_invis;
sh_int  gsn_poison;
sh_int  gsn_phase;
sh_int  gsn_plague;
sh_int  gsn_sleep;
sh_int  gsn_sanctuary;
sh_int  gsn_fly;
sh_int  gsn_necrosis;
sh_int  gsn_ritual;
sh_int  gsn_fade;
sh_int  gsn_word_of_recall;
sh_int  gsn_fear;
sh_int  gsn_confusion;
sh_int  gsn_mass_confusion;
sh_int  gsn_feeblemind;
sh_int  gsn_haste;
sh_int  gsn_giant_strength;
sh_int  gsn_slow;
sh_int  gsn_iron_maiden;

/* new gsns */

sh_int  gsn_axe;
sh_int  gsn_dagger;
sh_int  gsn_flail;
sh_int  gsn_mace;
sh_int  gsn_polearm;
sh_int  gsn_gun;   
sh_int  gsn_bow;   
sh_int  gsn_shield_block;
sh_int  gsn_wrist_shield;
sh_int  gsn_spear;
sh_int  gsn_sword;
sh_int  gsn_whip;

sh_int  gsn_crush;
sh_int  gsn_craft;
sh_int  gsn_cursed_wound;
sh_int  gsn_bash;
sh_int  gsn_beheading;
sh_int  gsn_berserk;
sh_int  gsn_dirt;
sh_int  gsn_hand_to_hand;
sh_int  gsn_trip;
sh_int  gsn_assassination;
sh_int  gsn_fatal_blow;
sh_int  gsn_brutal_damage;
sh_int  gsn_razor_claws;
sh_int  gsn_rake;
sh_int  gsn_fervent_rage;
sh_int  gsn_fervent_rage_cooldown;
sh_int  gsn_paroxysm;
sh_int  gsn_paroxysm_cooldown;
sh_int  gsn_rupture;
sh_int  gsn_replenish;
sh_int  gsn_replenish_cooldown;
sh_int  gsn_smite;

sh_int  gsn_fast_healing;
sh_int  gsn_haggle;
sh_int	gsn_arcane_lore;
sh_int  gsn_lore;
sh_int  gsn_meditation;
sh_int  gsn_appraise;
sh_int  gsn_weapons_lore;

sh_int  gsn_scrolls;
sh_int  gsn_staves;
sh_int  gsn_wands;
sh_int	gsn_flee;
sh_int	gsn_retreat;
sh_int	gsn_entrapment;

sh_int  gsn_bear;
sh_int  gsn_boa;
sh_int	gsn_bunny;
sh_int  gsn_dragon;
sh_int  gsn_eagle;
sh_int  gsn_eel;
sh_int  gsn_lion;
sh_int  gsn_phoenix;
sh_int  gsn_porcupine;
sh_int  gsn_rhino;
sh_int  gsn_scorpion;
sh_int  gsn_tiger;
sh_int  gsn_toad;
sh_int  gsn_tortoise;
sh_int  gsn_unicorn;
sh_int  gsn_finesse;
sh_int  gsn_rage;
sh_int  gsn_retribution;
sh_int  gsn_serpent;
sh_int  gsn_blade_dance;
sh_int  gsn_shadowclaw;
sh_int  gsn_shadowessence;
sh_int  gsn_shadowsoul;
sh_int  gsn_shadowwalk;
sh_int  gsn_ambush;
sh_int  gsn_anklebiter;
sh_int  gsn_arcana;
sh_int  gsn_bloodbath;
sh_int	gsn_kamikaze;
sh_int  gsn_showdown;
sh_int  gsn_target_practice;
sh_int  gsn_stalk;
sh_int  gsn_immolation;
sh_int  gsn_epidemic;
sh_int  gsn_absolute_zero;
sh_int  gsn_electrocution;
sh_int  gsn_shadow_shroud;
sh_int  gsn_jihad;
sh_int  gsn_vampire_hunting;
sh_int  gsn_witch_hunting;
sh_int  gsn_werewolf_hunting;
sh_int  gsn_inquisition;
sh_int  gsn_vampiric_bite;
sh_int  gsn_venom_bite;
sh_int  gsn_maul;
sh_int  gsn_extra_attack;
sh_int  gsn_swimming;
sh_int  gsn_alertness;
sh_int  gsn_evasion;
sh_int  gsn_evasive;

sh_int  gsn_laughing_fit;
sh_int  gsn_deaths_door;
sh_int  gsn_blessed_darkness;
sh_int  gsn_tomb_rot;
sh_int  gsn_cone_of_exhaustion;
sh_int  gsn_zombie_breath;
sh_int  gsn_zone_of_damnation;
sh_int  gsn_decompose;
sh_int  gsn_overcharge;
sh_int  gsn_unearth;

sh_int  gsn_korinns_inspiration;
sh_int  gsn_parademias_bile;
sh_int  gsn_swaydes_mercy;
sh_int  gsn_quirkys_insanity;
sh_int  gsn_firewitchs_seance;
sh_int	gsn_bless;
sh_int	gsn_prayer;
sh_int  gsn_bodyguard;
sh_int  gsn_back_leap;
sh_int  gsn_mana_shield;
sh_int  gsn_leadership;
sh_int  gsn_reflection;
sh_int  gsn_prot_magic;
sh_int  gsn_focus;
sh_int  gsn_dagger_focus;
sh_int  gsn_anatomy;
sh_int  gsn_mimic;
sh_int  gsn_mirror_image;
sh_int  gsn_intimidation;
sh_int  gsn_dowsing;
sh_int  gsn_rustle_grub;
sh_int  gsn_slash_throat;
sh_int  gsn_puppetry;
sh_int  gsn_jump_up;
sh_int  gsn_survey;
sh_int  gsn_charge;
sh_int  gsn_enchant_arrow;
sh_int  gsn_fledging;
sh_int  gsn_mass_combat;
sh_int  gsn_double_strike;
sh_int  gsn_round_swing;
sh_int  gsn_sharp_shooting;
sh_int  gsn_puncture;
sh_int  gsn_scribe;
sh_int  gsn_alchemy;
sh_int  gsn_dimensional_blade;
sh_int  gsn_elemental_blade;
sh_int  gsn_ashura;
sh_int  gsn_shan_ya;
sh_int  gsn_dark_reaping;
sh_int  gsn_inspiring_song;
sh_int  gsn_ambidextrous;
sh_int  gsn_aura_of_menace;
sh_int  gsn_aversion;
sh_int  gsn_strafe;
/* sh_int  gsn_combo_attack; */
sh_int  gsn_magic_missile;
sh_int  gsn_acid_blast;
sh_int  gsn_armor;
sh_int  gsn_bless;
sh_int  gsn_blindness;
sh_int  gsn_burning_hands;
sh_int  gsn_call_lightning;
sh_int  gsn_calm;
sh_int  gsn_phantasmal_image;
sh_int  gsn_shroud_of_darkness;
sh_int  gsn_paralysis_poison;
sh_int  gsn_hailstorm;
sh_int  gsn_control_weather;
sh_int  gsn_call_lightning;
sh_int  gsn_lightning_bolt;
sh_int  gsn_monsoon;
sh_int  gsn_meteor_swarm;
sh_int  gsn_smotes_anachronism;
sh_int  gsn_enchant_armor;
sh_int  gsn_enchant_weapon;
sh_int  gsn_enchant_arrow;
sh_int  gsn_solar_flare;

sh_int  gsn_extend_spell;
sh_int  gsn_empower_spell;
sh_int  gsn_quicken_spell;
sh_int  gsn_chain_spell;
sh_int  gsn_wish;

sh_int  gsn_god_bless;
sh_int  gsn_god_curse;

sh_int  gsn_custom_affect;

sh_int race_werewolf;
sh_int race_doppelganger;
sh_int race_naga;
sh_int race_vampire;
sh_int race_rakshasa;
sh_int race_dragonborn;


/* channel slot numbers */
sh_int sn_gossip;
sh_int sn_auction;
sh_int sn_music;
sh_int sn_question;
sh_int sn_answer;
sh_int sn_quote;
sh_int sn_gratz;
sh_int sn_gametalk;
sh_int sn_bitch;
sh_int sn_immtalk;
sh_int sn_savantalk;
sh_int sn_newbie;

/*
* Locals.
*/
MOB_INDEX_DATA *    mob_index_hash      [MAX_KEY_HASH];
OBJ_INDEX_DATA *    obj_index_hash      [MAX_KEY_HASH];
ROOM_INDEX_DATA *   room_index_hash     [MAX_KEY_HASH];
char *              string_hash     [MAX_KEY_HASH];

AREA_DATA *     area_first;
AREA_DATA *     area_last;
AREA_DATA *     current_area;


char *  string_space;
char *  top_string;
char    str_empty[1] = "";

int  top_affect;
int  top_area;
int  top_ed;
int  top_exit;
int  top_help;
int  top_mob_index;
int  top_obj_index;
int  top_reset;
int  top_room;
int  top_shop;
int  top_vnum_room;      /* OLC */
int  top_vnum_mob;       /* OLC */
int  top_vnum_obj;       /* OLC */
int  top_mprog_index;    /* OLC */
int  top_oprog_index;    /* OLC */
int  top_aprog_index;    /* OLC */
int  top_rprog_index;    /* OLC */
int  lua_mprogs=0;
int  mobile_count = 0;
int  object_count = 0;
int  top_jail_room = -1;


/*
* Memory management.
* Increase MAX_STRING if you have too.
* Tune the others only if you understand what you're doing.
*/
#define         MAX_STRING  9000000
#define         MAX_PERM_BLOCK  131072
/*#define         MAX_MEM_LIST    16*/

void *          rgFreeList  [MAX_MEM_LIST];
const int       rgSizeList  [MAX_MEM_LIST]  =
{
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768, 65536,
    128*1024, 256*1024, 512*1024, 1024*1024-64
};

extern int         nAllocString;
int         sAllocString;
extern int         nAllocPerm;
int         sAllocPerm;

int area_version = 0;

/*
* Semi-locals.
*/
bool            fBootDb;
FILE *          fpArea;
char            strArea[MAX_INPUT_LENGTH];



/*
* Local booting procedures.
*/
void    init_mm         args( ( void ) );
void    load_area   args( ( FILE *fp ) );
void    new_load_area   args( ( FILE *fp ) );   /* OLC */
/*void    load_helps  args( ( FILE *fp ) );*/
void    load_helps  args( ( FILE *fp, char *fname ) );

void    load_mobiles    args( ( FILE *fp ) );
void    load_objects    args( ( FILE *fp ) );
void    load_resets args( ( FILE *fp ) );
void    load_rooms  args( ( FILE *fp ) );
void    load_shops  args( ( FILE *fp ) );
void    load_bossachievements args( ( FILE *fp ) );
void    load_specials   args( ( FILE *fp ) );
void    load_notes  args( ( void ) );
void    load_bans   args( ( void ) );
void    load_mobprogs   args( ( FILE *fp ) );
void    load_objprogs( FILE *fp );
void    load_areaprogs( FILE *fp );
void    load_roomprogs( FILE *fp );
void    load_wizlist    args( ( void ) );
void    load_clans      args( ( void ) );
void	load_skills();
void    count_stats();

void    fix_exits   args( ( void ) );
void    fix_mobprogs    args( ( void ) );
void    fix_objprogs( void );
void    fix_areaprogs( void );
void    fix_roomprogs( void );

void    reset_area  args( ( AREA_DATA * pArea ) );
void    load_clanwars args ( ( void ) );
void    load_crime_list args ( ( void ) );
void    load_penalties args ( ( void ) );
void    load_reserved   args( ( void ) );
void    sort_reserved   args( ( RESERVED_DATA *pRes ) );
void    channel_init();
void    reset_str_dup();
int     random_attack_type();
void    dump_str_dup();

/*
* Big mama top level function.
*/
void boot_db()
{
    /*
     * Make sure some dirs exist
     */
    struct stat st = {0};
    if (stat(AREA_BACKUP_DIR, &st) == -1)
    {
        mkdir(AREA_BACKUP_DIR, 0700);
    }
    
    /*
     * Init some data space stuff.
     */
    {
        if ( ( string_space = calloc( 1, MAX_STRING ) ) == NULL )
        {
            bug( "Boot_db: can't alloc %d string space.", MAX_STRING );
            exit( 1 );
        }
        top_string  = string_space;
        fBootDb     = TRUE;
    }
    
    /*
    * Init random number generator.
    */
    {
        init_mm( );
    }
    
    /*
    * Set time and weather.
    */
    {
        long lhour, lday, lmonth;
        
        lhour       = (current_time - 650336715)
            / (PULSE_TICK / PULSE_PER_SECOND);
        time_info.hour  = lhour  % 24;
        lday        = lhour  / 24;
        time_info.day   = lday   % 35;
        lmonth      = lday   / 35;
        time_info.month = lmonth % 17;
        time_info.year  = lmonth / 17;
        
        if ( time_info.hour <  5 ) weather_info.sunlight = SUN_DARK;
        else if ( time_info.hour <  6 ) weather_info.sunlight = SUN_RISE;
        else if ( time_info.hour < 19 ) weather_info.sunlight = SUN_LIGHT;
        else if ( time_info.hour < 20 ) weather_info.sunlight = SUN_SET;
        else                            weather_info.sunlight = SUN_DARK;
        
        weather_info.change = 0;
        weather_info.mmhg   = 960;
        if ( time_info.month >= 7 && time_info.month <=12 )
            weather_info.mmhg += number_range( 1, 50 );
        else
            weather_info.mmhg += number_range( 1, 80 );
        
        if ( weather_info.mmhg <=  980 ) weather_info.sky = SKY_LIGHTNING;
        else if ( weather_info.mmhg <= 1000 ) weather_info.sky = SKY_RAINING;
        else if ( weather_info.mmhg <= 1020 ) weather_info.sky = SKY_CLOUDY;
        else                                  weather_info.sky = SKY_CLOUDLESS;
        
    }
    
    bounty_table = NULL;

    format_init_flags();

    race_werewolf = race_lookup("werewolf");
    race_doppelganger = race_lookup("doppelganger");
    race_naga = race_lookup("naga");
    race_vampire = race_lookup("vampire");
    race_rakshasa = race_lookup("rakshasa");
    race_dragonborn = race_lookup("dragonborn");
    
    /*
    * Assign gsn's for skills which have them.
    */
    {
        int sn,i,j;
        
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].pgsn != NULL )
                *skill_table[sn].pgsn = sn;
        }
        
	/* pc races */
        for ( i = 0; i<MAX_PC_RACE; i++)
            for (j=0; j<pc_race_table[i].num_skills; j++)
            {
                sn = skill_lookup_exact(pc_race_table[i].skills[j]);
                if ( sn < 0 )
                    bugf("Unknown %s racial skill %s.", pc_race_table[i].name, pc_race_table[i].skills[j]);
                pc_race_table[i].skill_gsns[j] = sn;
            }
	/* morph races */
        for ( i = 0; morph_pc_race_table[i].name; i++)
            for (j=0; j < morph_pc_race_table[i].num_skills; j++)
            {
                sn = skill_lookup_exact(morph_pc_race_table[i].skills[j]);
                if ( sn < 0 )
                    bugf("Unknown %s morphed skill %s.", morph_pc_race_table[i].name, morph_pc_race_table[i].skills[j]);
                morph_pc_race_table[i].skill_gsns[j] = sn;
            }
    }

    log_string( "Initializing channels." );
    channel_init();

    log_string( "Loading clans" );
    load_clans();

    log_string( "Loading religions" );
    load_religions();

//    log_string( "Initialzing leaderboards");
  //  lboard_init();


    //log_string( "Loading leaderboard results" );
    //load_lboard_results();

    log_string( "Loading skills" );
    load_skills();
    update_skill_costs();
    update_group_costs();

    log_string( "Counting stats" );
    count_stats();

    log_string( "Lua init.");
    open_lua();

    log_string( "Loading leaderboards" );
    load_lboards();

    log_string( "Loading comm histories" );
    load_comm_histories();

    log_string("Initializing mudconfig." );
    mudconfig_init();

    log_string("Loading mudconfig." );
    load_mudconfig();

    log_string( "Loading areas" );
    /*
    * Read in all the area files.
    */
    {
        FILE *fpList;
        
        if ( ( fpList = fopen( AREA_LIST, "r" ) ) == NULL )
        {
            log_error( AREA_LIST );
            exit( 1 );
        }
        
        for ( ; ; )
        {
            strcpy( strArea, fread_word( fpList ) );
            if ( strArea[0] == '$' )
                break;
            
            if ( strArea[0] == '-' )
            {
                fpArea = stdin;
            }
            else
            {
                if ( ( fpArea = fopen( strArea, "r" ) ) == NULL )
                {
                    log_error( strArea );
                    exit( 1 );
                }
            }
            
	    load_area_file( fpArea, TRUE );
            
            if ( fpArea != stdin )
                fclose( fpArea );
            fpArea = NULL;
        }
        fclose( fpList );
    }
    
    // debug
    //log_mob_index();
    
    /*
    * Fix up exits.
    * Declare db booting over.
    * Reset all areas once.
    * Load up the various runtime files.
    */
    {
        log_string("Fixing exits");
        fix_exits( );
        log_string("Fixing mobprogs");
        fix_mobprogs( );
        log_string("Fixing objprogs");
        fix_objprogs( );
        log_string("Fixing areaprogs");
        fix_areaprogs( );
        log_string("Fixing roomprogs");
        fix_roomprogs( );

        
	/* load socials and disabled commands before resetting area,
	 * reset triggers might want to call 'em --Bobble */
        log_string("Loading socials");
        load_social_table();
        log_string("Loading disabled commands");
        load_disabled();

        log_string("Resetting areas");
        // reset multiple times to ensure the world isn't too empty
        {
            int count = 3;
            while ( count-- )
                area_update(TRUE);
        }
        log_string("Resetting herbs");
        reset_herbs_world();
        
        log_string("Loading note boards");
        load_boards(); /* Load all boards */
        save_notes();
        log_string("Loading remort list");
        remort_load();
        log_string("Loading clanwar table");
        load_clanwars();
        log_string("Loading crime list");
        load_crime_list();
        log_string("Loading penalties");
        load_penalties();
        log_string("Loading reserved names");
        load_reserved();
        log_string( "Loading auth namelist" );
        load_auth_list();
        log_string( "Loading wizlist" );
        load_wizlist();
        log_string("Loading bans");
        load_bans();
        log_string("Loading portals");
        load_portal_list();

    }

    fBootDb = FALSE;

    // start checking for memory leaks now that we're ready
    reset_str_dup();
    
    return;
}


void channel_init()
{
    int sn;
    for ( sn=0 ; ; sn++ )
    {
        if ( public_channel_table[sn].psn == NULL )
            break;
        *public_channel_table[sn].psn = sn;
    }
}

/* format all flags correctly --Bobble */

void format_init_flags( void )
{
    log_string( "Formatting flags" );
    format_race_flags();
}

void format_race_flags( void )
{
    int i;

    for ( i = 0; race_table[i].name != NULL; i++ )
    {
	bit_list_to_tflag( race_table[i].act );
	bit_list_to_tflag( race_table[i].affect_field );
	bit_list_to_tflag( race_table[i].off );
	bit_list_to_tflag( race_table[i].imm );
	bit_list_to_tflag( race_table[i].res );
	bit_list_to_tflag( race_table[i].vuln );
	bit_list_to_tflag( race_table[i].form );
	bit_list_to_tflag( race_table[i].parts );
    }

    for ( i = 0; morph_race_table[i].name != NULL; i++ )
    {
	bit_list_to_tflag( morph_race_table[i].act );
	bit_list_to_tflag( morph_race_table[i].affect_field );
	bit_list_to_tflag( morph_race_table[i].off );
	bit_list_to_tflag( morph_race_table[i].imm );
	bit_list_to_tflag( morph_race_table[i].res );
	bit_list_to_tflag( morph_race_table[i].vuln );
	bit_list_to_tflag( morph_race_table[i].form );
	bit_list_to_tflag( morph_race_table[i].parts );
    }
}

void load_area_file( FILE *fp, bool clone )
{
    int clone_vnums[100];
    int shift, nr, clone_nr = 0;

    current_area = NULL;
    area_version = 0;

    for ( ; ; )
    {
	const char *word;
                
	if ( fread_letter( fpArea ) != '#' )
	{
	    bug( "Boot_db: # not found.", 0 );
	    exit( 1 );
	}
	
	word = fread_word( fpArea );
                
	if ( word[0] == '$' ) break;
	else if ( !str_cmp( word, "AREA"     ) ) load_area    (fpArea);
	else if ( !str_cmp( word, "AREADATA" ) ) new_load_area(fpArea); /* OLC */
	else if ( !str_cmp( word, "HELPS"    ) ) load_helps   (fpArea, strArea);
	else if ( !str_cmp( word, "MOBILES"  ) ) load_mobiles (fpArea);
        else if ( !str_cmp( word, "MOBBLES"  ) ) load_mobbles (fpArea);
	else if ( !str_cmp( word, "MOBPROGS" ) ) load_mobprogs(fpArea);
    else if ( !str_cmp( word, "OBJPROGS" ) ) load_objprogs(fpArea);
	else if ( !str_cmp( word, "AREAPROGS" ) ) load_areaprogs(fpArea);
    else if ( !str_cmp( word, "ROOMPROGS" ) ) load_roomprogs(fpArea);
	else if ( !str_cmp( word, "OBJECTS"  ) ) load_objects (fpArea);
	else if ( !str_cmp( word, "RESETS"   ) ) load_resets  (fpArea);
	else if ( !str_cmp( word, "ROOMS"    ) ) load_rooms   (fpArea);
	else if ( !str_cmp( word, "SHOPS"    ) ) load_shops   (fpArea);
    else if ( !str_cmp( word, "BOSSACHV" ) ) load_bossachievements(fpArea);
	else if ( !str_cmp( word, "SPECIALS" ) ) load_specials(fpArea);
	else if ( !str_cmp( word, "VER"      ) ) 
	    area_version = fread_number ( fpArea );
	else if ( !str_cmp( word, "CLONE"    ) )
	    clone_vnums[clone_nr++] = fread_number( fpArea );
	else
	{
	    bug( "Boot_db: bad section name.", 0 );
	    exit( 1 );
	}
    }

    /* preserve cloning data */
    for ( nr = 0; nr < clone_nr; nr++ )
	current_area->clones[nr] = clone_vnums[nr];

    /* now clone area if needed */
    if ( clone )
	for ( nr = 0; nr < clone_nr; nr++ )
	{
	    shift = clone_vnums[nr] - current_area->min_vnum;
	    if ( !range_is_free(clone_vnums[nr], current_area->max_vnum + shift) )
	    {
		bug( "load_area_file: range for area cloning already occupied (%d)",
		     clone_vnums[nr] );
		continue;
	    }

	    shift_area( current_area, shift, TRUE );

	    SET_BIT( current_area->area_flags, AREA_CLONE );
	    rewind( fpArea );
	    load_area_file( fpArea, FALSE );
	}
}

/*
* Snarf an 'area' header line.
*/
void load_area( FILE *fp )
{
    AREA_DATA *pArea;
    
    pArea               = alloc_AREA();
    pArea->file_name    = fread_string(fp);
    
    flag_clear( pArea->area_flags );
    /* SET_BIT( pArea->area_flags, AREA_LOADING ); */       /* OLC */
    pArea->security     = 9;                    /* OLC */ /* 9 -- Hugin */
    pArea->builders     = str_dup( "None" );    /* OLC */
    pArea->vnum         = top_area;             /* OLC */
    
    pArea->name     = fread_string( fp );
    pArea->credits  = fread_string( fp );
    pArea->min_vnum = fread_number(fp);
    pArea->max_vnum = fread_number(fp);
  /* Added minlevel, maxlevel, and miniquests for new
    areas command - Astark Dec 2012 */
    pArea->minlevel = fread_number(fp);
    pArea->maxlevel = fread_number(fp);
    pArea->miniquests = fread_number(fp);
    pArea->age      = 15;
    pArea->nplayer  = 0;
    pArea->empty    = FALSE;
    pArea->atrig_timer=NULL;
    
    if ( !area_first )
        area_first = pArea;
    
    if ( area_last )
    {
        area_last->next = pArea;
        /* REMOVE_BIT(area_last->area_flags, AREA_LOADING); */       /* OLC */
    }
    
    area_last        = pArea;
    pArea->next      = NULL;
    current_area = pArea;

    top_area++;

    logpf("Loading area: %s\t\t%Min: %ld  Max: %ld  Sec: %d  Credits: %s",
        pArea->name, pArea->min_vnum, pArea->max_vnum, pArea->security, pArea->credits);
    return;
}

/*
* OLC
* Use these macros to load any new area formats that you choose to
* support on your MUD.  See the new_load_area format below for
* a short example.
*/
#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value ) if ( !str_cmp( word, literal ) ) {field  = value; fMatch = TRUE; break; }

#define SKEY( string, field ) if ( !str_cmp( word, string ) ) {free_string( field ); field = fread_string( fp ); fMatch = TRUE; break;}

/* OLC
* Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
* too.
*
* #AREAFILE
* Name   { All } Locke    Newbie School~
* Repop  A teacher pops in the room and says, 'Repop coming!'~
* Recall 3001
* End
*/
void new_load_area( FILE *fp )
{
    AREA_DATA *pArea;
    const char *word;
    bool      fMatch;
    
    pArea               = alloc_AREA();
    pArea->age          = 15;
    pArea->nplayer      = 0;
    pArea->reset_time   = 15;
    pArea->file_name     = str_dup( strArea );
    pArea->vnum         = top_area;
    pArea->name         = str_dup( "New Area" );
    pArea->builders     = str_dup( "" );
    pArea->comments        = str_dup( "" );
    pArea->security     = 9;                    /* 9 -- Hugin */
    pArea->min_vnum        = 0;
    pArea->save         = TRUE;
    pArea->max_vnum        = 0;
  /* Added min,max, and mini for new areas command - Astark Dec 2012 */
    pArea->minlevel     = 0;
    pArea->maxlevel     = 0;
    pArea->miniquests   = 0;
    flag_clear( pArea->area_flags );
    
    for ( ; ; )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        
        switch ( UPPER(word[0]) )
        {
      /* Added for the new areas command - Astark Dec 2012 */
        case 'A':
            if (!str_cmp(word, "AProg") )
            {
                PROG_LIST *pAprog;
                const char *word;
                int trigger = 0;

                pAprog              = alloc_ATRIG();
                word                = fread_word( fp );
                if ( (trigger = flag_lookup( word, aprog_flags )) == NO_FLAG )
                {   
                    bugf("load_area.AProg: invalid trigger '%s' for area %s.", word, pArea->name);
                    exit(1);
                }
                SET_BIT(pArea->aprog_flags, trigger );
                pAprog->trig_type   = trigger;
                pAprog->vnum        = fread_number( fp );
                pAprog->trig_phrase = fread_string( fp );
                pAprog->next        = pArea->aprogs;
                pArea->aprogs   = pAprog;
            }
            break;
        case 'M':
            KEY("Minlevel", pArea->minlevel, fread_number( fp ));
            KEY("Maxlevel", pArea->maxlevel, fread_number( fp ));
            KEY("Miniquests", pArea->miniquests, fread_number( fp ));
        case 'N':
            SKEY( "Name", pArea->name );
            if ( !str_cmp(word, "NoQuest"))
            {
                SET_BIT(pArea->area_flags,AREA_NOQUEST);
                break;
            }
            if ( !str_cmp(word, "NoHide"))
            {
                SET_BIT(pArea->area_flags,AREA_NOHIDE);
                break;
            }
            SKEY( "Comments", pArea->comments );
            break;
        case 'R':
            if ( !str_cmp(word, "Remort"))
                SET_BIT(pArea->area_flags,AREA_REMORT);
            break;
        case 'S':
            KEY( "Security", pArea->security, fread_number( fp ) );
            break;
        case 'T':
            KEY("Time", pArea->reset_time, fread_number( fp ));
        case 'V':
            if ( !str_cmp( word, "VNUMs" ) )
            {
                pArea->min_vnum = fread_number( fp );
                pArea->max_vnum = fread_number( fp );
            }
            break;
        case 'E':
            if ( !str_cmp( word, "End" ) )
            {
                fMatch = TRUE;
                if ( area_first == NULL )
                    area_first = pArea;
                if ( area_last  != NULL )
                    area_last->next = pArea;
                
                area_last    = pArea;
                pArea->next  = NULL;
                current_area = pArea;
                top_area++;
                
                return;
            }
            break;
        case 'B':
            SKEY( "Builders", pArea->builders );
            break;
        case 'C':
            SKEY( "Credits", pArea->credits );
            break;
        }
        
        if ( !fMatch )
        {
            // no nothing but avoid warning
        }
    }
    
}

/*
* Sets vnum range for area using OLC protection features.
*/
void assign_area_vnum( int vnum )
{
    if ( area_last->min_vnum == 0 || area_last->max_vnum == 0 )
        area_last->min_vnum = area_last->max_vnum = vnum;
    if ( vnum != URANGE( area_last->min_vnum, vnum, area_last->max_vnum ) )
    {
        if ( vnum < area_last->min_vnum )
            area_last->min_vnum = vnum;
        else
            area_last->max_vnum = vnum;
    }
}


/* Reserved names, ported from Smaug by Rimbol 3/99. */
void load_reserved( void )
{
    RESERVED_DATA *res;
    FILE *fp;
    
    if ( !(fp = fopen(RESERVED_LIST, "r" )) )
        return;
    
    for ( ; ; )
    {
        if ( feof( fp ) )
        {
            bug( "Load_reserved: no $ found.",0 );
            fclose(fp);
            return;
        }
        
        res = alloc_mem(sizeof(RESERVED_DATA));
        
        res->name = fread_string(fp);
        if (*res->name == '$')
            break;
        sort_reserved(res);
    }
    
    free_string(res->name);
    free_mem(res, sizeof(RESERVED_DATA));
    
    fclose(fp);
    return;
}


/* Reserved names, ported from Smaug by Rimbol 3/99. */
void sort_reserved( RESERVED_DATA *pRes )
{
    RESERVED_DATA *res = NULL;
    
    if ( !pRes )
    {
        bug( "Sort_reserved: NULL pRes",0 );
        return;
    }
    
    pRes->next = NULL;
    pRes->prev = NULL;
    
    for ( res = first_reserved; res; res = res->next )
    {
            if ( strcasecmp(pRes->name, res->name) > 0 )
            {
                pRes->prev = res->prev;
                if ( !res->prev )
                    first_reserved   = pRes;
                else
                    res->prev->next  = pRes;
                
                res->prev  = pRes;
                pRes->next = res;
                
                break;
            }
    }
    
    if ( !res )
    {
        if ( !first_reserved )
            first_reserved       = pRes;
        else
            last_reserved->next  = pRes;
        
        pRes->next    = NULL;
        pRes->prev    = last_reserved;
        last_reserved = pRes;
    }
    
    return;
}



/*
* Snarf a help section.
*/
void load_helps( FILE *fp, char *fname )
{
    HELP_DATA *pHelp;
    int level;
    const char *keyword;
    
    for ( ; ; )
    {
        HELP_AREA * had;
        
        level    = fread_number( fp );
        keyword  = fread_string( fp );
        
        if ( keyword[0] == '$' )
            break;
        
        if ( !had_list )
        {
            had = new_had ();
            had->filename = str_dup( fname );
            had->area = current_area;
            if ( current_area )
                current_area->helps = had;
            had_list = had;
        }
        else if ( str_cmp( fname, had_list->filename ) )
        {
            had = new_had ();
            had->filename = str_dup( fname );
            had->area = current_area;
            if ( current_area )
                current_area->helps = had;
            had->next = had_list;
            had_list  = had;
        }
        else
            had = had_list;
        
        pHelp  = new_help( );
        pHelp->level = level;
        pHelp->keyword = keyword;
        
        pHelp->text = fread_string( fp );
        
        if ( !str_cmp( pHelp->keyword, "greeting" ) )
            help_greeting = pHelp->text;
        
        if ( help_first == NULL )
            help_first = pHelp;
        if ( help_last  != NULL )
            help_last->next = pHelp;
        
        help_last     = pHelp;
        pHelp->next       = NULL;
        
        if ( !had->first )
            had->first = pHelp;
        if ( !had->last )
            had->last  = pHelp;
        
        had->last->next_area  = pHelp;
        had->last     = pHelp;
        pHelp->next_area  = NULL;
        
        top_help++;
    }
    
    return;
}


/*
 * Add mobile template to global indices
 */
void index_mobile( MOB_INDEX_DATA *pMobIndex )
{
    int vnum = pMobIndex->vnum;
    int iHash = vnum % MAX_KEY_HASH;    
    pMobIndex->next = mob_index_hash[iHash];
    mob_index_hash[iHash] = pMobIndex;
    top_mob_index++;
    top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;  /* OLC */
    assign_area_vnum( vnum );                                  /* OLC */
    kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL-1)].number++;
    
    return;
}

/*
 * Debug: log state of mob_index_hash
 */
void log_mob_index()
{
    int mob_count = 0;
    int max_bucket_size = 0;
    long square_bucket_size = 0;
    int bucket_size;
    
    int iHash;
    MOB_INDEX_DATA* pMobIndex;
    
    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        bucket_size = 0;
        for ( pMobIndex = mob_index_hash[iHash]; pMobIndex != NULL; pMobIndex = pMobIndex->next )
        {
            bucket_size++;
            mob_count++;
        }
        if (bucket_size > max_bucket_size)
            max_bucket_size = bucket_size;
        square_bucket_size += bucket_size * bucket_size;
    }
    logpf( "mob_index_hash: buckets = %d, mobs = %d, max_bucket_size = %d, ave_access = %.2f",
           MAX_KEY_HASH, mob_count, max_bucket_size, (float)(square_bucket_size) / mob_count );
}

RESET_DATA* get_last_reset( RESET_DATA *reset_list )
{
    RESET_DATA *reset;

    if ( reset_list == NULL )
	return NULL;

    for ( reset = reset_list; reset->next != NULL; reset = reset->next )
	;

    return reset;
}
 
 /*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
 void new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset )
 {
     RESET_DATA *pr;
     
     if ( !pR )
         return;
     
     pr = get_last_reset( pR->reset_first );
     
     if ( !pr )
         pR->reset_first = pReset;
     else
	 pr->next = pReset;
     
     return;
 }
 
 
 
 /*
 * Snarf a reset section.
 */
 void load_resets( FILE *fp )
 {
     RESET_DATA *pReset;
     int rVnum = -1;
     
     if ( !area_last )
     {
         bug( "Load_resets: no #AREA seen yet.", 0 );
         exit( 1 );
     }
     
     for ( ; ; )
     {
         char letter;
         
         if ( ( letter = fread_letter( fp ) ) == 'S' )
             break;
         
         if ( letter == '*' )
         {
             fread_to_eol( fp );
             continue;
         }
         
         pReset		= new_reset_data();
         pReset->command = letter;
         /* if_flag */     fread_number( fp );
         pReset->arg1    = fread_number( fp );
         pReset->arg2    = fread_number( fp );
         pReset->arg3    = (letter == 'G' || letter == 'R')
             ? 0 : fread_number( fp );
         pReset->arg4    = (letter == 'P' || letter == 'M')
             ? fread_number(fp) : 0;
         fread_to_eol( fp );

         switch( pReset->command )
         {
             case 'M':
             case 'O':
                 rVnum = pReset->arg3;
                 break;

             case 'P':
             case 'G':
             case 'E':
                 break;

             case 'D':
                 free_reset_data( pReset );
                 continue; /* obsolete, removed */

             case 'R':
                 rVnum = pReset->arg1;
                 break;
         }

         if ( rVnum == -1 )
         {
             bugf( "load_resets : rVnum == -1" );
             exit(1);
         }

         new_reset( get_room_index_safe(rVnum), pReset );
     }
     
     return;
 }
 
 
 /*
 * Snarf a room section.
 */
void load_rooms( FILE *fp )
{
    ROOM_INDEX_DATA *pRoomIndex;

    int i;
    /* prepare lock flags */
    tflag lock_values[4] = { { EX_ISDOOR },
        { EX_ISDOOR, EX_PICKPROOF },
        { EX_ISDOOR, EX_NOPASS },
        { EX_ISDOOR, EX_NOPASS, EX_PICKPROOF } };
    for ( i = 0; i < 4; i++ )
        bit_list_to_tflag( lock_values[i] );

    if ( area_last == NULL )
    {
        bug( "Load_resets: no #AREA seen yet.", 0 );
        exit( 1 );
    }

    for ( ; ; )
    {
        int vnum;
        char letter;
        int door;
        int iHash;

        letter              = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_rooms: # not found.", 0 );
            exit( 1 );
        }

        vnum                = fread_number( fp );
        if ( vnum == 0 )
            break;

        if ( get_room_index( vnum ) != NULL )
        {
            bug( "Load_rooms: vnum %d duplicated.", vnum );
            exit( 1 );
        }

        pRoomIndex          = alloc_ROOM();
        pRoomIndex->owner       = str_dup("");
        pRoomIndex->people      = NULL;
        pRoomIndex->contents    = NULL;
        pRoomIndex->extra_descr = NULL;
        pRoomIndex->area        = area_last;
        pRoomIndex->vnum        = vnum;
        pRoomIndex->name        = fread_string( fp );
        pRoomIndex->description = fread_string( fp );
        /* Area number */         fread_number( fp );
        fread_tflag( fp, pRoomIndex->room_flags );


        if (IS_SET(pRoomIndex->room_flags, ROOM_JAIL) 
                && top_jail_room < (MAX_JAIL_ROOM - 1))
        {
            SET_BIT(pRoomIndex->room_flags, ROOM_NO_RECALL);
            SET_BIT(pRoomIndex->room_flags, ROOM_SAFE);
            jail_room_list[++top_jail_room] = pRoomIndex->vnum;
        } 

        pRoomIndex->sector_type     = fread_number( fp );
        pRoomIndex->light       = 0;
        for ( door = 0; door < MAX_DIR; door++ )
            pRoomIndex->exit[door] = NULL;

        /* defaults */
        pRoomIndex->heal_rate = 100;
        pRoomIndex->mana_rate = 100;
        pRoomIndex->clan      = 0;
        pRoomIndex->clan_rank = 0;

        for ( ; ; )
        {
            letter = fread_letter( fp );

            if ( letter == 'S' )
                break;

            if ( letter == 'H') /* healing room */
                pRoomIndex->heal_rate = fread_number(fp);

            else if ( letter == 'M') /* mana room */
                pRoomIndex->mana_rate = fread_number(fp);

            else if ( letter == 'C') /* clan */
            {
                if (pRoomIndex->clan)
                {
                    bug("Load_rooms: duplicate clan fields.",0);
                    exit(1);
                }
                pRoomIndex->clan = clan_lookup(fread_string(fp));
            }

            else if ( letter == 'R') /* Clan rank */
            {
                if (pRoomIndex->clan_rank)
                {
                    bug("Load_rooms: duplicate clan_rank fields.",0);
                    exit(1);
                }
                pRoomIndex->clan_rank = clan_rank_lookup(pRoomIndex->clan, fread_string(fp));
            }

            else if ( letter == 'D' )
            {
                EXIT_DATA *pexit;
                int locks;		 

                door = fread_number( fp );
                if ( door < 0 || door >= MAX_DIR )
                {
                    bug( "Fread_rooms: vnum %d has bad door number.", vnum );
                    exit( 1 );
                }

                pexit           = alloc_EXIT();
                pexit->description  = fread_string( fp );
                pexit->keyword      = fread_string( fp );

                if ( area_version < VER_EXIT_FLAGS )
                {
                    flag_clear( pexit->rs_flags  );       /* OLC */
                    locks = fread_number( fp );
                    if ( 1 <= locks && locks <= 4 )
                        flag_copy( pexit->rs_flags, lock_values[locks - 1] );
                }
                else
                    fread_tflag( fp, pexit->rs_flags );
                flag_copy( pexit->exit_info, pexit->rs_flags );

                pexit->key      = fread_number( fp );
                pexit->u1.vnum      = fread_number( fp );
                pexit->orig_door    = door;         /* OLC */

                /*
                   switch ( locks )
                   {
                   case 1: pexit->exit_info = EX_ISDOOR;               
                   pexit->rs_flags  = EX_ISDOOR;            break;
                   case 2: pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
                   pexit->rs_flags  = EX_ISDOOR | EX_PICKPROOF; break;
                   case 3: pexit->exit_info = EX_ISDOOR | EX_NOPASS;    
                   pexit->rs_flags  = EX_ISDOOR | EX_NOPASS;    break;
                   case 4: pexit->exit_info = EX_ISDOOR|EX_NOPASS|EX_PICKPROOF;
                   pexit->rs_flags  = EX_ISDOOR|EX_NOPASS|EX_PICKPROOF;
                   break;
                   case 5:
                   pexit->exit_info = EX_HIDDEN;
                   pexit->rs_flags = EX_HIDDEN;
                   break;
                   }
                 */

                pRoomIndex->exit[door]  = pexit;
                top_exit++;
            }
            else if ( letter == 'E' )
            {
                EXTRA_DESCR_DATA *ed;

                ed          = alloc_perm( sizeof(*ed) );
                ed->keyword     = fread_string( fp );
                ed->description     = fread_string( fp );
                ed->next        = pRoomIndex->extra_descr;
                pRoomIndex->extra_descr = ed;
                top_ed++;
            }

            else if (letter == 'N')
            {
                pRoomIndex->comments = fread_string(fp);
            }

            else if (letter == 'O')
            {
                if (pRoomIndex->owner[0] != '\0')
                {
                    bug("Load_rooms: duplicate owner.",0);
                    exit(1);
                }

                pRoomIndex->owner = fread_string(fp);
            }

            else if (letter == 'P')
            {
                PROG_LIST *pRprog;
                const char *word;
                int trigger=0;

                pRprog = alloc_RTRIG();
                word=fread_word( fp );
                if ( ( trigger=flag_lookup( word, rprog_flags)) == NO_FLAG)
                {
                    bugf("load_rooms.P: invalid trigger '%s' for room %d.",
                            word, pRoomIndex->vnum);
                    exit(1);
                }
                SET_BIT( pRoomIndex->rprog_flags, trigger );
                pRprog->trig_type      = trigger;
                pRprog->vnum           = fread_number( fp );
                pRprog->trig_phrase    = fread_string( fp );
                pRprog->next           = pRoomIndex->rprogs;
                pRoomIndex->rprogs     = pRprog;
            }


            else
            {
                bug( "Load_rooms: vnum %d has flag not 'DES'.", vnum );
                exit( 1 );
            }
        }

        if (!pRoomIndex->comments)
            pRoomIndex->comments = str_dup( "" );

        iHash           = vnum % MAX_KEY_HASH;
        pRoomIndex->next    = room_index_hash[iHash];
        room_index_hash[iHash]  = pRoomIndex;
        top_room++;
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room; /* OLC */
        assign_area_vnum( vnum );                                    /* OLC */
    }

    return;
}

void load_bossachievements( FILE *fp )
{
    BOSSACHV *pBoss=NULL;

    for ( ; ; )
    {
        MOB_INDEX_DATA *pMobIndex;
        int vnum=fread_number(fp);

        if ( vnum==0 ) /* end of section */
            break;

        pBoss = alloc_BOSSACHV();
        
        pBoss->exp_reward = fread_number(fp);
        pBoss->gold_reward = fread_number(fp);
        pBoss->quest_reward = fread_number(fp);
        pBoss->ach_reward = fread_number(fp);

        pMobIndex=get_mob_index(vnum);
        pMobIndex->boss_achieve = pBoss;
    }

    return;
}

/*
* Snarf a shop section.
*/
void load_shops( FILE *fp )
{
    SHOP_DATA *pShop;
    
    for ( ; ; )
    {
        MOB_INDEX_DATA *pMobIndex;
        int iTrade;
        int keeper;

        keeper=fread_number(fp);
        if ( keeper == 0 )
            break;
        
        pShop           = alloc_SHOP(); 
        pShop->keeper       = keeper;

        for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
            pShop->buy_type[iTrade] = fread_number( fp );
        pShop->profit_buy   = fread_number( fp );
        pShop->profit_sell  = fread_number( fp );
        pShop->open_hour    = fread_number( fp );
        pShop->close_hour   = fread_number( fp );
        fread_to_eol( fp );
        pMobIndex       = get_mob_index_safe( pShop->keeper );
        pMobIndex->pShop    = pShop;
        
        if ( shop_first == NULL )
            shop_first = pShop;
        if ( shop_last  != NULL )
            shop_last->next = pShop;
        
        shop_last   = pShop;
        pShop->next = NULL;
        top_shop++;
    }
    
    return;
}


/*
* Snarf spec proc declarations.
*/
void load_specials( FILE *fp )
{
    for ( ; ; )
    {
        MOB_INDEX_DATA *pMobIndex;
        char letter;
        
        switch ( letter = fread_letter( fp ) )
        {
        default:
            bug( "Load_specials: letter '%c' not *MS.", letter );
            exit( 1 );
            
        case 'S':
            return;
            
        case '*':
            break;
            
        case 'M':
            pMobIndex       = get_mob_index_safe ( fread_number ( fp ) );
            pMobIndex->spec_fun = spec_lookup   ( fread_word   ( fp ) );
            if ( pMobIndex->spec_fun == 0 )
            {
                bug( "Load_specials: 'M': vnum %d.", pMobIndex->vnum );
                exit( 1 );
            }
            break;
        }
        
        fread_to_eol( fp );
    }
}


void fix_exits( void )
{
    ROOM_INDEX_DATA *pRoomIndex;
    EXIT_DATA *pexit;
    RESET_DATA *pReset;
    ROOM_INDEX_DATA *iLastRoom, *iLastObj;
    int iHash;
    int door;
    
    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for ( pRoomIndex  = room_index_hash[iHash];
	      pRoomIndex != NULL;
	      pRoomIndex  = pRoomIndex->next )
        {
            /*-----Added in Olc 1.81, may be buggy-----*/
            iLastRoom = iLastObj = NULL;
            
            /* OLC : New reset control */
            for ( pReset = pRoomIndex->reset_first; pReset; pReset = pReset->next )
            {
                switch( pReset->command )
                {
                default:
                    bugf( "fix_exits : room %d with reset cmd %c", pRoomIndex->vnum, pReset->command );
                    exit(1);
                    break;
                    
                case 'M':
                    get_mob_index_safe( pReset->arg1 );
                    iLastRoom = get_room_index_safe( pReset->arg3 );
                    break;
                    
                case 'O':
                    get_obj_index_safe( pReset->arg1 );
                    iLastObj = get_room_index_safe( pReset->arg3 );
                    break;
                    
                case 'P':
                    get_obj_index_safe( pReset->arg1 );
                    if (iLastObj == NULL)
                    {
                        bugf( "fix_exits : reset in room %d with iLastObj NULL", pRoomIndex->vnum );
                        exit(1);
                    }
                    break;
                    
                case 'G':
                case 'E':
                    get_obj_index_safe( pReset->arg1 );
                    if (iLastRoom == NULL)
                    {
                        bugf( "fix_exits : reset in room %d with iLastRoom NULL", pRoomIndex->vnum );
                        exit(1);
                    }
                    iLastObj = iLastRoom;
                    break;
                    
                case 'D':
                    bugf( "???" );
                    break;
                    
                case 'R':
                    get_room_index_safe( pReset->arg1 );
                    if ( pReset->arg2 < 0 || pReset->arg2 > MAX_DIR )
                    {
                        bugf( "fix_exits : reset in room %d with arg2 %d >= MAX_DIR",
                            pRoomIndex->vnum, pReset->arg2 );
                        exit(1);
                    }
                    break;
                } /* switch */
            } /* for */
            /*-----*/    
            
            for ( door = 0; door < MAX_DIR; door++ )
            {
                if ( ( pexit = pRoomIndex->exit[door] ) != NULL )
                {
                    if ( pexit->u1.vnum <= 0 
                        || get_room_index(pexit->u1.vnum) == NULL)
                        pexit->u1.to_room = NULL;
                    else
                    {
                        pexit->u1.to_room = get_room_index_safe( pexit->u1.vnum );
                    }
                }
            }
        }
    }
    
    exits_fixed = TRUE;
    return;
}

/*
* Load roomprogs section
*/
void load_roomprogs( FILE *fp )
{
    PROG_CODE *pRprog;

    if ( area_last == NULL )
    {
        bug( "Load_roomprogs: no #AREA seen yet.", 0 );
        exit( 1 );
    }

    for ( ; ; )
    {
        int vnum;
        char letter;

        letter        = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_roomprogs: # not found.", 0 );
            exit( 1 );
        }

        vnum         = fread_number( fp );
        if ( vnum == 0 )
            break;

        if ( get_rprog_index( vnum ) != NULL )
        {
            bug( "Load_roomprogs: vnum %d duplicated.", vnum );
            exit( 1 );
        }

        pRprog      = alloc_PROG();
        pRprog->vnum    = vnum;

        const char *word;
        for ( ; ; )
        {
            word=fread_word(fp);

            if (!strcmp( word, "SEC" ) )
            {
                pRprog->security=fread_number(fp);
            }
            else if (!strcmp( word, "CODE" ) )
            {
                pRprog->code=fread_string(fp);
            }
            else if (!strcmp( word, "End" ) )
            {
                break;
            }
            else
            {
                bugf("Unrecognized word in load_roomprogs: %s", word);
                exit(1);
            }
        }

        if ( rprog_list == NULL )
            rprog_list = pRprog;
        else
        {
            pRprog->next = rprog_list;
            rprog_list  = pRprog;
        }
        top_rprog_index++;
    }
    return;
}

/*
* Load areaprogs section
*/
void load_areaprogs( FILE *fp )
{
    PROG_CODE *pAprog;

    if ( area_last == NULL )
    {
        bug( "Load_areaprogs: no #AREA seen yet.", 0 );
        exit( 1 );
    }

    for ( ; ; )
    {
        int vnum;
        char letter;

        letter        = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_areaprogs: # not found.", 0 );
            exit( 1 );
        }

        vnum         = fread_number( fp );
        if ( vnum == 0 )
            break;

        if ( get_aprog_index( vnum ) != NULL )
        {
            bug( "Load_areaprogs: vnum %d duplicated.", vnum );
            exit( 1 );
        }

        pAprog      = alloc_PROG();
        pAprog->vnum    = vnum;

        if ( area_version < VER_NEW_PROG_FORMAT )
        {
            /* old code for old format */
            pAprog->code    = fread_string( fp );
        }
        else
        {
            /* new code for new format */
            const char *word;
            for ( ; ; )
            {
                word=fread_word(fp);

                if (!strcmp( word, "SEC" ) )
                {
                    pAprog->security=fread_number(fp);
                }
                else if (!strcmp( word, "CODE" ) )
                {
                    pAprog->code=fread_string(fp);
                }
                else if (!strcmp( word, "End" ) )
                {
                    break;
                }
                else
                {
                    bugf("Unrecognized word in load_areaprogs: %s", word);
                    exit(1);
                }
            }
        }

        if ( aprog_list == NULL )
            aprog_list = pAprog;
        else
        {
            pAprog->next = aprog_list;
            aprog_list  = pAprog;
        }
        top_aprog_index++;
    }
    return;
}

/*
* Load objprogs section
*/
void load_objprogs( FILE *fp )
{
    PROG_CODE *pOprog;

    if ( area_last == NULL )
    {
        bug( "Load_objprogs: no #AREA seen yet.", 0 );
        exit( 1 );
    }

    for ( ; ; )
    {
        int vnum;
        char letter;

        letter        = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_objprogs: # not found.", 0 );
            exit( 1 );
        }

        vnum         = fread_number( fp );
        if ( vnum == 0 ) /* end of section */
            break;

        if ( get_oprog_index( vnum ) != NULL )
        {
            bug( "Load_objprogs: vnum %d duplicated.", vnum );
            exit( 1 );
        }

        pOprog      = alloc_PROG();
        pOprog->vnum    = vnum;

        if ( area_version < VER_NEW_PROG_FORMAT )
        {
            /* old code for old format */
            pOprog->code    = fread_string( fp );
        }
        else
        {
            /* new code for new format */
            for ( ; ; )
            {
                const char *word=fread_word(fp);

                if (!strcmp(word, "CODE") )
                {
                    pOprog->code=fread_string(fp);
                }
                else if ( !strcmp(word, "SEC") )
                {
                    pOprog->security=fread_number(fp);
                }
                else if ( !strcmp(word, "End") )
                {
                    break;
                }
                else
                {
                    bugf("Unrecognized word in load_objprogs: %s", word);
                    exit(1);
                }
            }
        }
                

        if ( oprog_list == NULL )
            oprog_list = pOprog;
        else
        {
            pOprog->next = oprog_list;
            oprog_list  = pOprog;
        }
        top_oprog_index++;
    }
    return;
}
/*
* Load mobprogs section
*/
void load_mobprogs( FILE *fp )
{
    PROG_CODE *pMprog;
    
    if ( area_last == NULL )
    {
        bug( "Load_mobprogs: no #AREA seen yet.", 0 );
        exit( 1 );
    }
    
    for ( ; ; )
    {
        int vnum;
        char letter;
        
        letter        = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_mobprogs: # not found.", 0 );
            exit( 1 );
        }
        
        vnum         = fread_number( fp );
        if ( vnum == 0 )
            break;
        
        if ( get_mprog_index( vnum ) != NULL )
        {
            bug( "Load_mobprogs: vnum %d duplicated.", vnum );
            exit( 1 );
        }
        
        pMprog      = alloc_PROG();
        pMprog->vnum    = vnum;
        pMprog->is_lua  = FALSE; /* new progs default to true but
                                    when loading we need to default to false */

        if ( area_version < VER_NEW_PROG_FORMAT )
        {
            /* old code for old format */

            /* some funko stuff when loading old files that don't have is_lua data*/
            const char * tempStr = fread_string( fp );
            if ( !strcmp( tempStr, "IS_LUA" ) )
            {
                pMprog->is_lua = TRUE;
                pMprog->code = fread_string( fp );
            }
            else if ( !strcmp( tempStr, "NOT_LUA" ) )
            {
                pMprog->is_lua = FALSE;
                pMprog->code = fread_string( fp );
            }
            else
            {
                pMprog->code    = tempStr;
            }
        }
        else
        {
            /* new code for new format */
            const char *word;
            for ( ; ; )
            {
                word=fread_word(fp);

                if (!strcmp(word, "LUA" ))
                {
                    pMprog->is_lua=fread_number(fp);
                }
                else if (!strcmp(word, "SEC"))
                {
                    pMprog->security=fread_number(fp);
                }
                else if (!strcmp(word, "CODE"))
                {
                    pMprog->code=fread_string(fp);
                }
                else if (!strcmp(word, "End") )
                {
                    break;
                }
                else
                {
                    bugf("Unrecognized word in load_mobprogs: %s", word);
                    exit(1);
                }
                
            } /* end for loop */
        } /* end else */

        if ( mprog_list == NULL )
            mprog_list = pMprog;
        else
        {
            pMprog->next = mprog_list;
            mprog_list  = pMprog;
        }

        if (pMprog->is_lua)
            lua_mprogs++;
        top_mprog_index++;
    } /* end for loop */
    return;
}

/*
*  Translate mobprog vnums pointers to real code
*/
void fix_mobprogs( void )
{
    MOB_INDEX_DATA *pMobIndex;
    PROG_LIST        *list;
    PROG_CODE        *prog;
    int iHash;
    int mprog_count = 0;
    
    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for ( pMobIndex   = mob_index_hash[iHash];
        pMobIndex   != NULL;
        pMobIndex   = pMobIndex->next )
        {
            for( list = pMobIndex->mprogs; list != NULL; list = list->next )
            {
                if ( ( prog = get_mprog_index( list->vnum ) ) != NULL )
                {
                    mprog_count++;
                    list->script = prog;
                }
                else
                {
                    bug( "Fix_mobprogs: code vnum %d not found.", list->vnum );
                    exit( 1 );
                }
            }
        }
    }
    //debug
    //logpf("Fix_mobprogs: %d mprogs fixed.", mprog_count);
}

void fix_objprogs( void )
{
    OBJ_INDEX_DATA *pObjIndex;
    PROG_LIST        *list;
    PROG_CODE        *prog;
    int iHash;
    int oprog_count = 0;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for ( pObjIndex   = obj_index_hash[iHash];
        pObjIndex   != NULL;
        pObjIndex   = pObjIndex->next )
        {
            for( list = pObjIndex->oprogs; list != NULL; list = list->next )
            {
                if ( ( prog = get_oprog_index( list->vnum ) ) != NULL )
                {
                    oprog_count++;
                    list->script = prog;
                }
                else
                {
                    bug( "Fix_objprogs: code vnum %d not found.", list->vnum );
                    exit( 1 );
                }
            }
        }
    }
    //debug
    //logpf("Fix_mobprogs: %d mprogs fixed.", mprog_count);
}

void fix_areaprogs( void )
{
    AREA_DATA *pArea;
    PROG_LIST        *list;
    PROG_CODE        *prog;
    int aprog_count = 0;

    for ( pArea   = area_first;
        pArea   != NULL;
        pArea   = pArea->next )
    {
        for( list = pArea->aprogs; list != NULL; list = list->next )
        {
            if ( ( prog = get_aprog_index( list->vnum ) ) != NULL )
            {
                aprog_count++;
                list->script = prog;
            }
            else
            {
                bug( "Fix_areaprogs: code vnum %d not found.", list->vnum );
                exit( 1 );
            }
            aprog_setup( pArea );
        }
    }
    
    //debug
    //logpf("Fix_mobprogs: %d mprogs fixed.", mprog_count);
}

void fix_roomprogs( void )
{
    ROOM_INDEX_DATA *pRoom;
    PROG_LIST        *list;
    PROG_CODE        *prog;
    int iHash;
    int rprog_count = 0;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for ( pRoom = room_index_hash[iHash];
        pRoom   != NULL;
        pRoom   = pRoom->next )
        {
            for( list = pRoom->rprogs; list != NULL; list = list->next )
            {
                if ( ( prog = get_rprog_index( list->vnum ) ) != NULL )
                {
                    rprog_count++;
                    list->script = prog;
                }
                else
                {
                    bug( "Fix_roomprogs: code vnum %d not found.", list->vnum );
                    exit( 1 );
                }
                rprog_setup( pRoom );
            }
        }
    }
    //debug
    //logpf("Fix_mobprogs: %d mprogs fixed.", mprog_count);
}

/*
* Repopulate areas periodically.
*/
void area_update( bool all )
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
    
    for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
    {
        if ( !all && IS_SET(pArea->area_flags, AREA_NOREPOP) )
            continue;
        // check age (=time since last reset)
        if ( !fBootDb )
        {
            if ( ++pArea->age < 3 ) // just reset
                continue;
            if ( pArea->age < 2*pArea->reset_time ) // reset not overdue yet
            {
                if ( pArea->empty ) // area hasn't been visited since last reset
                    continue;
                // don't reset until we must while players are in the area
                if ( pArea->nplayer > 0 && pArea->age < pArea->reset_time )
                    continue;
            }
        }
        // reset
        reset_area(pArea);
        sprintf(buf, "%s has just been reset.", pArea->name);
        wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);
        pArea->age = number_range(0, 3);
        if ( pArea->nplayer == 0 )
            pArea->empty = TRUE;
    }
}



/* OLC
* Reset one room.  Called by reset_area and olc.
*/
void reset_room( ROOM_INDEX_DATA *pRoom )
{
    RESET_DATA  *pReset;
    CHAR_DATA   *pMob;
    CHAR_DATA   *mob;
    OBJ_DATA    *pObj;
    CHAR_DATA   *LastMob = NULL;
    OBJ_DATA    *LastObj = NULL;
    int iExit;
    int level = 0;
    bool last;
    
    if ( !pRoom )
        return;
    
    pMob        = NULL;
    last        = FALSE;
    
    for ( iExit = 0;  iExit < MAX_DIR;  iExit++ )
    {
        EXIT_DATA *pExit;
        if ( ( pExit = pRoom->exit[iExit] )
            /*  && !IS_SET( pExit->exit_info, EX_BASHED )   ROM OLC */ )  
        {
            flag_copy( pExit->exit_info, pExit->rs_flags );
            if ( ( pExit->u1.to_room != NULL )
                && ( ( pExit = pExit->u1.to_room->exit[rev_dir[iExit]] ) ) )
            {
                /* nail the other side */
                flag_copy( pExit->exit_info, pExit->rs_flags );
            }
        }
    }
    
    for ( pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next )
    {
        MOB_INDEX_DATA  *pMobIndex;
        OBJ_INDEX_DATA  *pObjIndex;
        OBJ_INDEX_DATA  *pObjToIndex;
        ROOM_INDEX_DATA *pRoomIndex;
        char buf[MAX_STRING_LENGTH];
        int count,limit=0;
        
        switch ( pReset->command )
        {
        default:
            bug( "Reset_room: bad command %c.", pReset->command );
            break;
            
        case 'M':
            if ( !( pMobIndex = get_mob_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'M': bad vnum %d.", pReset->arg1 );
                continue;
            }
            
            if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) == NULL )
            {
                bug( "Reset_area: 'M': bad room vnum %d.", pReset->arg3 );
                continue;
            }
            if ( pMobIndex->count >= pReset->arg2 )
            {
                last = FALSE;
                break;
            }
            /* */
            count = 0;
            for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room)
                if (mob->pIndexData == pMobIndex)
                    
                {
                    count++;
                    if (count >= pReset->arg4)
                    {
                        last = FALSE;
                        break;
                    }
                }
                
                if (count >= pReset->arg4)
                    break;
                
                /* */
                
                pMob = create_mobile( pMobIndex );
                
                /*
                * Some more hard coding.
                */

		/* 
                if ( room_is_dark( pRoom ) )
                    SET_BIT(pMob->affect_field, AFF_INFRARED);
		*/

		/*
		 * Pet shop mobiles get ACT_PET set.
		 */
                {
                    ROOM_INDEX_DATA *pRoomIndexPrev;
                    
                    pRoomIndexPrev = get_room_index( pRoom->vnum - 1 );
                    if ( pRoomIndexPrev
                        && IS_SET( pRoomIndexPrev->room_flags, ROOM_PET_SHOP ) )
                        SET_BIT( pMob->act, ACT_PET);
                }
                
                pMob->zone = pRoomIndex->area;
                
                char_to_room( pMob, pRoom );
		
                LastMob = pMob;
                level  = URANGE( 0, pMob->level - 2, LEVEL_HERO - 1 ); /* -1 ROM */
                last = TRUE;

		if ( HAS_TRIGGER(pMob, TRIG_RESET) )
		{
		    mp_percent_trigger( pMob, NULL, NULL, 0, NULL, 0,TRIG_RESET );
		    /* safety-net if mob kills himself with mprog */
		    if ( IS_DEAD(pMob) )
		    {
			bug( "Reset_room: mob %d killed upon reset",
			     pReset->arg1 );
			last = FALSE;
			LastMob = NULL;
		    }
		}
                
                break;
                
        case 'O':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'O' 1 : bad vnum %d", pReset->arg1 );
                sprintf (buf,"%d %d %d %d",pReset->arg1, pReset->arg2, pReset->arg3,
                    pReset->arg4 );
                bug(buf,1);
                continue;
            }
            
            if ( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
                bug( "Reset_room: 'O' 2 : bad vnum %d.", pReset->arg3 );
                sprintf (buf,"%d %d %d %d",pReset->arg1, pReset->arg2, pReset->arg3,
                    pReset->arg4 );
                bug(buf,1);
                continue;
            }
            
            if ( /* pRoom->area->nplayer > 0
		 ||*/ count_obj_list( pObjIndex, pRoom->contents ) > 0 )
            {
                last = FALSE;
                break;
            }
            
            pObj = create_object( pObjIndex, /* UMIN - ROM OLC */
				  UMIN(number_fuzzy( level ), LEVEL_HERO -1) );
            //pObj->cost = 0;
	    check_enchant_obj( pObj );
            obj_to_room( pObj, pRoom );
            last = TRUE;
            break;
            
        case 'P':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'P': bad vnum %d.", pReset->arg1 );
                continue;
            }
            
            if ( !( pObjToIndex = get_obj_index( pReset->arg3 ) ) )
            {
                bug( "Reset_room: 'P': bad vnum %d.", pReset->arg3 );
                continue;
            }
            
            if (pReset->arg2 > 50) /* old format */
                limit = 6;
            else if (pReset->arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = pReset->arg2;
            
            if ( /* pRoom->area->nplayer > 0
		 ||*/ ( LastObj = get_obj_type( pObjToIndex ) ) == NULL
		 || ( LastObj->in_room == NULL && !last)
		 || ( pObjIndex->count >= limit /* && number_range(0,4) != 0 */ )
		 || ( count = count_obj_list( pObjIndex, LastObj->contains ) ) > pReset->arg4  )
            {
                last = FALSE;
                break;
            }
            /* lastObj->level  -  ROM */
            
            while (count < pReset->arg4)
            {
                pObj = create_object( pObjIndex, number_fuzzy( LastObj->level ) );
		check_enchant_obj( pObj );
                obj_to_obj( pObj, LastObj );
                count++;
                if (pObjIndex->count >= limit)
                    break;
            }
            
            /* fix object lock state! */
            LastObj->value[1] = LastObj->pIndexData->value[1];
            last = TRUE;
            break;
            
        case 'G':
        case 'E':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'E' or 'G': bad vnum %d.", pReset->arg1 );
                continue;
            }
            
            if ( !last )
                break;
            
            if ( !LastMob )
            {
                bug( "Reset_room: 'E' or 'G': null mob for vnum %d.",
                    pReset->arg1 );
                last = FALSE;
                break;
            }
            
            if ( LastMob->pIndexData->pShop )   /* Shop-keeper? */
            {
                pObj = create_object( pObjIndex, 0 );
                SET_BIT( pObj->extra_flags, ITEM_INVENTORY );  /* ROM OLC */
                
            }
            else   /* ROM OLC else version */
            {
                int limit;
                if (pReset->arg2 > 50 )  /* old format */
                    limit = 6;
                else if ( pReset->arg2 == -1 || pReset->arg2 == 0 )  /* no limit */
                    limit = 999;
                else
                    limit = pReset->arg2;
                
                if ( pObjIndex->count < limit || number_range(0,4) == 0 )
                {
                    pObj = create_object( pObjIndex, 
                        UMIN( number_fuzzy( level ), LEVEL_HERO - 1 ) );
                    /* error message if it is too high */
                    if (pObj->level > LastMob->level + 10)
                        fprintf(stderr,
                        "Err: obj %s (%d) -- %d, mob %s (%d) -- %d\n",
                        pObj->short_descr,pObj->pIndexData->vnum,pObj->level,
                        LastMob->short_descr,LastMob->pIndexData->vnum,LastMob->level);
                }
                else
                    break;
            }
            
            
	    check_enchant_obj( pObj );
            obj_to_char( pObj, LastMob );
            if ( pReset->command == 'E' )
	    {
                equip_char( LastMob, pObj, pReset->arg3 );
		/* special bow handling */
		if ( pObj != NULL && pObj->item_type == ITEM_WEAPON
		     && pObj->value[0] == WEAPON_BOW )
		    equip_new_arrows( LastMob );
		/* Restore hit,mana,move in case the eq changes them */
                LastMob->hit     =       LastMob->max_hit;
                LastMob->mana    =       LastMob->max_mana;
                LastMob->move    =       LastMob->max_move;
	    }
            last = TRUE;
            break;
            
        case 'D':
            break;
            
        case 'R':
            if ( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'R': bad vnum %d.", pReset->arg1 );
                continue;
            }
            
            {
                EXIT_DATA *pExit;
                int d0;
                int d1;
                
                for ( d0 = 0; d0 < pReset->arg2 - 1; d0++ )
                {
                    d1                   = number_range( d0, pReset->arg2-1 );
                    pExit                = pRoomIndex->exit[d0];
                    pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                    pRoomIndex->exit[d1] = pExit;
                }
            }
            break;
        }
    }
    
    /* stuff after resets are executed */

    //reset_herbs( pRoom );

    /* arm mobs with default weapons */
    for ( mob = pRoom->people; mob != NULL; mob = mob->next_in_room )
	arm_npc( mob );

    return;
}

/* create a default weapon for the mob --Bobble */
void arm_npc( CHAR_DATA *mob )
{
    OBJ_DATA *obj;
    int level;

    if ( mob == NULL || !IS_NPC(mob) || !IS_SET(mob->off_flags, OFF_ARMED) )
	return;
    else
	REMOVE_BIT( mob->off_flags, OFF_ARMED );

    if ( get_eq_char(mob, WEAR_WIELD) != NULL
	 || (obj = create_object(get_obj_index(OBJ_VNUM_MOB_WEAPON), 0)) == NULL )
	return;

    /* adjust to level */
    if ( mob->level <= 100 )
        level = number_range(mob->level * 6/10, mob->level * 9/10);
    else
    {
        int min_level = UMIN(90, mob->level * 6/10);
        int max_level = 80 + mob->level/10;
        level = number_range(min_level, max_level);
    }
    obj->level = URANGE(1, level, 100);
    
    /* determin weapon type */
    if ( IS_SET(mob->act, ACT_GUN) )
	obj->value[0] = WEAPON_GUN;
    else if ( IS_SET(mob->act, ACT_THIEF)
	 || IS_SET(mob->off_flags, OFF_BACKSTAB)
	 || IS_SET(mob->off_flags, OFF_CIRCLE) )
	obj->value[0] = WEAPON_DAGGER;
    else if ( IS_SET(mob->act, ACT_WARRIOR) )
	if ( IS_SET(mob->off_flags, OFF_PARRY) || number_bits(1) )
	    obj->value[0] = WEAPON_SWORD;
	else if ( number_bits(1) )
	    obj->value[0] = WEAPON_POLEARM;
	else
	    obj->value[0] = WEAPON_AXE;
    else if ( IS_SET(mob->act, ACT_CLERIC) )
	obj->value[0] = WEAPON_MACE;
    else if ( IS_SET(mob->act, ACT_MAGE) )
	obj->value[0] = WEAPON_SPEAR;
    else
    {
	switch ( number_range(0,8) )
	{
	case 0: obj->value[0] = WEAPON_SWORD; break;
	case 1: obj->value[0] = WEAPON_DAGGER; break;
	case 2: obj->value[0] = WEAPON_SPEAR; break;
	case 3: obj->value[0] = WEAPON_MACE; break;
	case 4: obj->value[0] = WEAPON_AXE; break;
	case 5: obj->value[0] = WEAPON_FLAIL; break;
	case 6: obj->value[0] = WEAPON_WHIP; break;
	case 7: obj->value[0] = WEAPON_POLEARM; break;
	default: // low chance for exotic weapon :)
	    if ( number_bits(2) )
		obj->value[0] = WEAPON_SWORD;
	    else
		obj->value[0] = WEAPON_EXOTIC;
	}
    }
    
    /* set proper descriptions and flags */
    switch ( obj->value[0] )
    {
    default:
    case WEAPON_EXOTIC:
	obj->value[3] = random_attack_type();
	rename_obj( obj, "exotic weapon", "Exotic Weapon", "Some exotic weapon." );
	obj->weight = number_range( 2, 10 );
	break;
    case WEAPON_SWORD:
	obj->value[3] = attack_lookup("slice");
	rename_obj( obj, "simple sword", "Simple Sword", "A simple sword." );
	obj->weight = 5;
	break;
    case WEAPON_DAGGER:
	obj->value[3] = attack_lookup("stab");
	rename_obj( obj, "simple dagger", "Simple Dagger", "A simple dagger." );
	obj->weight = 2;
	break;
    case WEAPON_SPEAR:
	if ( IS_SET(mob->act, ACT_MAGE) )
	{
	    obj->value[3] = attack_lookup("beating");
	    rename_obj( obj, "simple staff", "Simple Staff", "A simple staff." );
	}
	else
	{
	    obj->value[3] = attack_lookup("sting");
	    rename_obj( obj, "simple spear", "Simple Spear", "A simple spear." );
	}
	obj->weight = 4;
	break;
    case WEAPON_MACE:
	obj->value[3] = attack_lookup("crush");
	rename_obj( obj, "simple club", "Simple Club", "A simple club." );
	obj->weight = 10;
	break;
    case WEAPON_AXE:
	obj->value[3] = attack_lookup("cleave");
	rename_obj( obj, "simple axe", "Simple Axe", "A simple axe." );
	obj->weight = 8;
	break;
    case WEAPON_FLAIL:
	obj->value[3] = attack_lookup("smash");
	rename_obj( obj, "simple flail", "Simple Flail", "A simple flail." );
	obj->weight = 8;
	break;
    case WEAPON_WHIP:
	obj->value[3] = attack_lookup("thwack");
	rename_obj( obj, "simple whip", "Simple whip", "A simple whip." );
	obj->weight = 3;
	break;
    case WEAPON_POLEARM:
	obj->value[3] = attack_lookup("chomp");
	rename_obj( obj, "simple polearm", "Simple polearm", "A simple polearm." );
	obj->weight = 6;
	break;
    case WEAPON_GUN:
	obj->value[3] = attack_lookup("bullet");
	rename_obj( obj, "simple gun", "Simple gun", "A simple gun." );
	obj->weight = 4;
	break;
    }
    obj->weight *= 10;
    obj->weight += number_range( -10, 10 );

    // set weapon damage based on level and type
    set_weapon_dam(obj, weapon_dam_spec(level, obj->value[0], FALSE));
    
    /* override damtype with mob's setting? */
    if (mob->pIndexData->dam_type != DAM_NONE)
    {
        obj->value[3]=mob->pIndexData->dam_type;
    }

    /* equip weapon */
    obj_to_char( obj, mob );
    equip_char( mob, obj, WEAR_WIELD );

    // mobs may dual-wield, have a two-handed weapon, or weapon + shield
    bool can_twohand = obj->value[0] != WEAPON_DAGGER && obj->value[0] != WEAPON_WHIP;
    bool can_dual = !IS_SET(mob->act, ACT_CLERIC) && !IS_SET(mob->act, ACT_MAGE);
    bool can_shield = !IS_SET(mob->act, ACT_THIEF) && !IS_SET(mob->act, ACT_MAGE) && !IS_SET(mob->act, ACT_GUN);
    
    if ( can_twohand && (obj->value[0] == WEAPON_POLEARM || per_chance(25)) )
    {
        I_SET_BIT( obj->value[4], WEAPON_TWO_HANDS );
        obj->weight = obj->weight * 3/2;
        set_weapon_dam(obj, weapon_dam_spec(level, obj->value[0], TRUE));
    }
    else if ( can_dual && per_chance(obj->value[0] == WEAPON_DAGGER || obj->value[0] == WEAPON_GUN ? 66 : 33) )
    {
        OBJ_DATA *secondary = create_object(get_obj_index(OBJ_VNUM_MOB_WEAPON), 0);
        if ( secondary != NULL )
        {
            clone_object(obj, secondary);
            obj_to_char(secondary, mob);
            equip_char(mob, secondary, WEAR_SECONDARY);
        }
    }
    else if ( can_shield && (IS_SET(mob->act, ACT_WARRIOR) || per_chance(50)) )
    {
        OBJ_DATA *shield = create_object(get_obj_index(OBJ_VNUM_MOB_SHIELD), 0);
        if ( shield != NULL )
        {
            shield->level = obj->level;
            shield->value[0] = armor_class_by_level(obj->level);
            obj_to_char(shield, mob);
            equip_char(mob, shield, WEAR_SHIELD);
        }
    }
}

void rename_obj( OBJ_DATA *obj, char *name, char *short_descr, char *description )
{
    if ( obj == NULL || name == NULL || short_descr == NULL || description == NULL )
	return;

    free_string( obj->name );
    obj->name = str_dup( name );
    free_string( obj->short_descr );
    obj->short_descr = str_dup( short_descr );
    free_string( obj->description );
    obj->description = str_dup( description );
}

/* randomly choose an attack type */
int random_attack_type()
{
    int i;
    
    for ( i = 1; attack_table[i].name != NULL; i++ )
	;
    return number_range( 1, i-1 );
}

/* OLC
* Reset one area.
*/
void reset_area( AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoom;
    int  vnum;
    
    if ( pArea == NULL )
    {
	bug( "reset_area: NULL area", 0 );
	return;
    }

    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pRoom = get_room_index(vnum) ) )
            reset_room(pRoom);
    }
    
    return;
}


void purge_room( ROOM_INDEX_DATA *pRoom )
{
    CHAR_DATA *vnext;
    CHAR_DATA *victim;
    OBJ_DATA  *obj_next;
    OBJ_DATA  *obj;
    
    for ( victim = pRoom->people; victim != NULL; victim = vnext )
    {
        vnext = victim->next_in_room;
        if ( IS_NPC(victim) && !IS_SET(victim->act,ACT_NOPURGE))
            extract_char( victim, TRUE );
    }
    
    for ( obj = pRoom->contents; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;
        if ( !IS_OBJ_STAT(obj,ITEM_NOPURGE) )
            extract_obj( obj );
    }
    
    return;
}


void purge_area( AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoom;
    int  vnum;
    
    if ( pArea == NULL )
    {
	bug( "purge_area: NULL area", 0 );
	return;
    }

    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pRoom = get_room_index(vnum) ) )
            purge_room(pRoom);
    }
    
    return;
}


/*
* Create an instance of a mobile.
*/

CHAR_DATA *create_mobile( MOB_INDEX_DATA *pMobIndex )
{
    CHAR_DATA *mob;
    tflag mob_pen = {};/*{ PENALTY_NOCHANNEL, PENALTY_NOSHOUT, PENALTY_NOTELL };*/
    bit_list_to_tflag( mob_pen );
    
    mobile_count++;
    
    if ( pMobIndex == NULL )
    {
        bug( "Create_mobile: NULL pMobIndex.", 0 );
        exit( 1 );
    }
    
    mob = new_char();
    
    mob->pIndexData    = pMobIndex;
    
    mob->name       = str_dup( pMobIndex->player_name );    /* OLC */
    mob->short_descr    = str_dup( pMobIndex->short_descr );    /* OLC */
    mob->long_descr = str_dup( pMobIndex->long_descr );     /* OLC */
    mob->description    = str_dup( pMobIndex->description );    /* OLC */
    mob->id        = get_mob_id();
    mob->spec_fun  = pMobIndex->spec_fun;
    mob->prompt        = NULL;
    mob->mprog_target   = NULL;
    
    /* read from prototype */
    mob->group      = pMobIndex->group;
    flag_copy( mob->act, pMobIndex->act );
    flag_copy( mob->penalty, mob_pen );
    flag_copy( mob->affect_field, pMobIndex->affect_field );
    mob->alignment      = pMobIndex->alignment;
    mob->level          = UMAX( 1, pMobIndex->level );

    mob->dam_type       = pMobIndex->dam_type;
    if (mob->dam_type == 0)
        switch(number_range(1,3))
    {
            case (1): mob->dam_type = 3;        break;  /* slash */
            case (2): mob->dam_type = 7;        break;  /* pound */
            case (3): mob->dam_type = 11;       break;  /* pierce */
    }
    flag_copy( mob->off_flags, pMobIndex->off_flags );
    flag_copy( mob->imm_flags, pMobIndex->imm_flags );
    flag_copy( mob->res_flags, pMobIndex->res_flags );
    flag_copy( mob->vuln_flags, pMobIndex->vuln_flags );
    mob->start_pos      = pMobIndex->start_pos;
    mob->default_pos    = pMobIndex->default_pos;
    mob->sex            = pMobIndex->sex;
    if (mob->sex == 3) /* random sex */
        mob->sex = number_range(1,2);
    mob->race           = pMobIndex->race;
    flag_copy( mob->form, pMobIndex->form );
    flag_copy( mob->parts, pMobIndex->parts );
    mob->size           = pMobIndex->size;
    
    // set calm threshold to prevent mobs exhausting themselves completely
    // mainly for summons, but also to allow fleeing, stancing, etc.
    mob->calm = 10;

    // money money money
    long wealth = mob_base_wealth( pMobIndex );
    wealth = number_range(wealth/2, wealth * 3/2);
    mob->gold = number_range(wealth/200,wealth/100);
    mob->silver = wealth - (mob->gold * 100);
    
    // level dependent stats (hp, damage, ...)
    set_mob_level( mob, mob-> level );
    
    /* let's get some spell action */
    affect_spellup_mob( mob );

    mob->position = mob->start_pos;    

    mprog_setup(mob);
    
    /* link the mob to the world list */
    char_list_insert(mob);
    pMobIndex->count++;

    return mob;
}

void check_affect_add( CHAR_DATA *mob, int affect, int sn )
{
    AFFECT_DATA af;

    if ( IS_SET(race_table[mob->race].affect_field, affect) )
	return;

    if ( IS_AFFECTED(mob, affect) )
    {
	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = mob->level;
	af.duration  = -1;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = affect;
	affect_to_char( mob, &af );
    }
}

void affect_spellup_mob( CHAR_DATA *mob )
{
    check_affect_add( mob, AFF_SANCTUARY,     skill_lookup("sanctuary") );
    check_affect_add( mob, AFF_HASTE,         skill_lookup("haste") );
    check_affect_add( mob, AFF_PROTECT_EVIL,  skill_lookup("protection evil") );
    check_affect_add( mob, AFF_PROTECT_GOOD,  skill_lookup("protection good") );
    check_affect_add( mob, AFF_FADE,          skill_lookup("fade") );
    check_affect_add( mob, AFF_PROTECT_MAGIC, skill_lookup("protection magic") );
    check_affect_add( mob, AFF_REFLECTION,    skill_lookup("reflection") );
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CHAR_DATA *parent, CHAR_DATA *clone)
{
    int i;
    AFFECT_DATA *paf;
    
    if ( parent == NULL || clone == NULL || !IS_NPC(parent))
        return;
    
    /* start fixing values */ 
    clone->name     = str_dup(parent->name);
    clone->version  = parent->version;
    clone->short_descr  = str_dup(parent->short_descr);
    clone->long_descr   = str_dup(parent->long_descr);
    clone->description  = str_dup(parent->description);
    clone->group    = parent->group;
    clone->sex      = parent->sex;
    clone->class    = parent->class;
    clone->race     = parent->race;
    clone->level    = parent->level;
    clone->trust    = 0;
    clone->timer    = parent->timer;
    clone->wait     = parent->wait;
    clone->hit      = parent->hit;
    clone->max_hit  = parent->max_hit;
    clone->mana     = parent->mana;
    clone->max_mana = parent->max_mana;
    clone->move     = parent->move;
    clone->max_move = parent->max_move;
    clone->gold     = parent->gold;
    clone->silver   = parent->silver;
    clone->exp      = parent->exp;
    flag_copy( clone->act, parent->act );
    flag_copy( clone->comm, parent->comm );
    flag_copy( clone->imm_flags, parent->imm_flags );
    flag_copy( clone->res_flags, parent->res_flags );
    flag_copy( clone->vuln_flags, parent->vuln_flags );
    clone->invis_level  = parent->invis_level;
    flag_copy( clone->affect_field, parent->affect_field );
    clone->position = parent->position;
    clone->practice = parent->practice;
    clone->train    = parent->train;
    clone->saving_throw = parent->saving_throw;
    clone->alignment    = parent->alignment;
    clone->hitroll  = parent->hitroll;
    clone->damroll  = parent->damroll;
    clone->wimpy    = parent->wimpy;
    clone->calm     = parent->calm;
    flag_copy( clone->form, parent->form );
    flag_copy( clone->parts, parent->parts );
    clone->size     = parent->size;
    flag_copy( clone->off_flags, parent->off_flags );
    clone->dam_type = parent->dam_type;
    clone->start_pos    = parent->start_pos;
    clone->default_pos  = parent->default_pos;
    clone->spec_fun = parent->spec_fun;
    
    clone->armor = parent->armor;
    
    for (i = 0; i < MAX_STATS; i++)
    {
        clone->perm_stat[i] = parent->perm_stat[i];
        clone->mod_stat[i]  = parent->mod_stat[i];
    }
    
    for (i = 0; i < 3; i++)
        clone->damage[i]    = parent->damage[i];
    
    /* now add the affects */
    for (paf = parent->affected; paf != NULL; paf = paf->next)
        affect_to_char(clone,paf);
    
}

/* hard-coded pill prices */
int spell_base_cost( int sn )
{
    int power;

    if ( sn <= 0 )
	return 0;

    power = skill_table[sn].min_mana * (skill_table[sn].beats + 1);

    if ( skill_table[sn].minimum_position < POS_STANDING )
	power /= 2;

    return (int)sqrt( power );
}

int spell_obj_cost( int level, int base_cost )
{
    int power = (level * 3/4 + 25) * base_cost / 10;
    return power * power / 2;
}

/*
* Create an instance of an object.
*/
OBJ_DATA *create_object( OBJ_INDEX_DATA *pObjIndex, int level )
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    int i;
    
    if ( pObjIndex == NULL )
    {
        bug( "Create_object: NULL pObjIndex.", 0 );
	/*
        exit( 1 );
	*/
	return NULL;
    }
    object_count++;
    
    obj = new_obj();
    
    obj->pIndexData = pObjIndex;
    obj->in_room    = NULL;
    obj->owner      = NULL;
    
    obj->level = pObjIndex->level;
    obj->wear_loc  = -1;
    
    obj->name       = str_dup( pObjIndex->name );           /* OLC */
    obj->short_descr    = str_dup( pObjIndex->short_descr );    /* OLC */
    obj->description    = str_dup( pObjIndex->description );    /* OLC */
    obj->material  = str_dup(pObjIndex->material);
    obj->item_type = pObjIndex->item_type;
    flag_copy( obj->extra_flags, pObjIndex->extra_flags );
    flag_copy( obj->wear_flags, pObjIndex->wear_flags );
    obj->value[0]   = pObjIndex->value[0];
    obj->value[1]   = pObjIndex->value[1];
    obj->value[2]   = pObjIndex->value[2];
    obj->value[3]   = pObjIndex->value[3];
    obj->value[4]   = pObjIndex->value[4];
    obj->weight     = pObjIndex->weight;
    obj->clan     = pObjIndex->clan;
    obj->rank     = pObjIndex->rank;
    obj->timer      = 0;
    
    obj->cost = pObjIndex->cost;
    
    /*
     * Mess with object properties.
     */
    switch ( obj->item_type )
    {
    default:
        bug( "Read_object: vnum %d bad type.", pObjIndex->vnum );
        break;
        
    case ITEM_LIGHT:
        if (obj->value[2] == 999)
            obj->value[2] = -1;
        break;
        
    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_MAP:
    case ITEM_CLOTHING:
    case ITEM_PORTAL:
    case ITEM_EXPLOSIVE:
    case ITEM_TREASURE:
    case ITEM_WARP_STONE:
    case ITEM_ROOM_KEY:
    case ITEM_GEM:
    case ITEM_JEWELRY:
    case ITEM_HOGTIE:
    case ITEM_DOWSING_STICK:
    case ITEM_BLACK_HERB:
    case ITEM_RED_HERB:
    case ITEM_SILVER_HERB:
    case ITEM_MOTTLED_HERB:
    case ITEM_ARROWS:
        break;
        
    case ITEM_JUKEBOX:
        for (i = 0; i < 5; i++)
            obj->value[i] = -1;
        break;
        
    case ITEM_WAND:
    case ITEM_STAFF:
	/*
        if (level != -1 && !pObjIndex->new_format)
        {
            obj->value[0]   = number_fuzzy( obj->value[0] );
            obj->value[1]   = number_fuzzy( obj->value[1] );
            obj->value[2]   = obj->value[1];
        }
        if (!pObjIndex->new_format)
            obj->cost *= 2;
	*/
	/* hard-coded pill prices */
	{
	    int base = spell_base_cost( obj->value[3] );
	    obj->cost = spell_obj_cost( obj->value[0], base );
	    if ( obj->item_type == ITEM_WAND )
		obj->cost /= 2;
	    if ( obj->item_type == ITEM_STAFF )
		obj->cost /= 4;
	}
        break;
        
    case ITEM_WEAPON:
        break;
        
    case ITEM_ARMOR:
        break;
        
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
	/*
        if (level != -1 && !pObjIndex->new_format)
            obj->value[0] = number_fuzzy( number_fuzzy( obj->value[0] ) );
	*/
	/* hard-coded pill prices */
	{
	    int base, i;
	    base = 0;
	    for ( i = 1; i <= 4; i++ )
	    {
		if ( obj->value[i] <= 0 )
		    continue;
		
		base += spell_base_cost(obj->value[i]);
	    }
	    obj->cost = spell_obj_cost( obj->value[0], base );
	    if ( obj->item_type == ITEM_POTION )
		obj->cost /= 2;
	    else if ( obj->item_type == ITEM_SCROLL )
		obj->cost /= 8;
	}
        break;
        
    case ITEM_MONEY:
        break;
    }
    
    for (paf = pObjIndex->affected; paf != NULL; paf = paf->next) 
        if ( paf->location == APPLY_SPELL_AFFECT )
            affect_to_obj(obj,paf);

    oprog_setup( obj );
        
    obj->next       = object_list;
    object_list     = obj;
    pObjIndex->count++;
        
    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(OBJ_DATA *parent, OBJ_DATA *clone)
{
    int i;
    AFFECT_DATA *paf;
    EXTRA_DESCR_DATA *ed,*ed_new;
    
    if (parent == NULL || clone == NULL)
        return;
    
    /* start fixing the object */
    clone->name     = str_dup(parent->name);
    clone->short_descr  = str_dup(parent->short_descr);
    clone->description  = str_dup(parent->description);
    clone->item_type    = parent->item_type;
    flag_copy( clone->extra_flags, parent->extra_flags );
    flag_copy( clone->wear_flags, parent->wear_flags );
    clone->weight   = parent->weight;
    clone->cost     = parent->cost;
    clone->level    = parent->level;
    clone->material = str_dup(parent->material);
    clone->timer    = parent->timer;
    
    for (i = 0;  i < 5; i ++)
        clone->value[i] = parent->value[i];
    
    /* affects */
    for (paf = parent->affected; paf != NULL; paf = paf->next) 
        affect_to_obj(clone,paf);
    
    /* extended desc */
    for (ed = parent->extra_descr; ed != NULL; ed = ed->next)
    {
        ed_new                  = new_extra_descr();
        ed_new->keyword     = str_dup( ed->keyword);
        ed_new->description     = str_dup( ed->description );
        ed_new->next            = clone->extra_descr;
        clone->extra_descr      = ed_new;
    }
    
}



/*
* Clear a new character.
*/
void clear_char( CHAR_DATA *ch )
{
    static CHAR_DATA ch_zero;
    int i;
    
    *ch             = ch_zero;
    ch->name            = &str_empty[0];
    ch->short_descr     = &str_empty[0];
    ch->long_descr      = &str_empty[0];
    ch->description     = &str_empty[0];
    ch->prompt                  = &str_empty[0];
    ch->logon           = current_time;
    ch->lines           = PAGELEN;
    ch->armor           = 100;
    ch->position        = POS_STANDING;
    ch->hit         = 20;
    ch->max_hit         = 20;
    ch->mana            = 100;
    ch->max_mana        = 100;
    ch->move            = 100;
    ch->max_move        = 100;
    ch->on          = NULL;
    for (i = 0; i < MAX_STATS; i ++)
    {
        ch->perm_stat[i] = 60; 
        ch->mod_stat[i] = 60;
    }
    return;
}

/*
* Get an extra description from a list.
*/
const char * get_extra_descr( const char *name, EXTRA_DESCR_DATA *ed )
{
    for ( ; ed != NULL; ed = ed->next )
    {
        if ( is_name( (char *) name, ed->keyword ) )
            return ed->description;
    }
    return NULL;
}

MOB_INDEX_DATA *get_mob_index_safe( int vnum )
{
    MOB_INDEX_DATA *index = get_mob_index(vnum);
    if ( !index )
    {
        bug( "get_mob_index_safe: bad vnum %d.", vnum );
        exit( 1 );
    }
    return index;
}

/*
* Translates mob virtual number to its mob index struct.
* Hash table lookup.
*/
MOB_INDEX_DATA *get_mob_index( int vnum )
{
    MOB_INDEX_DATA *pMobIndex;
    
    for ( pMobIndex  = mob_index_hash[vnum % MAX_KEY_HASH];
    pMobIndex != NULL;
    pMobIndex  = pMobIndex->next )
    {
        if ( pMobIndex->vnum == vnum )
            return pMobIndex;
    }
    
    return NULL;
}

OBJ_INDEX_DATA *get_obj_index_safe( int vnum )
{
    OBJ_INDEX_DATA *index = get_obj_index(vnum);
    if ( !index )
    {
        bug( "get_obj_index_safe: bad vnum %d.", vnum );
        exit( 1 );
    }
    return index;
}

/*
* Translates obj virtual number to its obj index struct.
* Hash table lookup.
*/
OBJ_INDEX_DATA *get_obj_index( int vnum )
{
    OBJ_INDEX_DATA *pObjIndex;
    
    for ( pObjIndex  = obj_index_hash[vnum % MAX_KEY_HASH];
	  pObjIndex != NULL;
	  pObjIndex  = pObjIndex->next )
    {
        if ( pObjIndex->vnum == vnum )
            return pObjIndex;
    }
    
    return NULL;
}

ROOM_INDEX_DATA *get_room_index_safe( int vnum )
{
    ROOM_INDEX_DATA *index = get_room_index(vnum);
    if ( !index )
    {
        bug( "get_room_index_safe: bad vnum %d.", vnum );
        exit( 1 );
    }
    return index;
}

/*
* Translates room virtual number to its room index struct.
* Hash table lookup.
*/
ROOM_INDEX_DATA *get_room_index( int vnum )
{
    ROOM_INDEX_DATA *pRoomIndex;
    
    for ( pRoomIndex  = room_index_hash[vnum % MAX_KEY_HASH];
    pRoomIndex != NULL;
    pRoomIndex  = pRoomIndex->next )
    {
        if ( pRoomIndex->vnum == vnum )
            return pRoomIndex;
    }
    
    return NULL;
}

PROG_CODE *get_mprog_index( int vnum )
{
    PROG_CODE *prg;
    for( prg = mprog_list; prg; prg = prg->next )
    {
        if ( prg->vnum == vnum )
            return( prg );
    }
    return NULL;
}    

PROG_CODE *get_oprog_index( int vnum )
{
    PROG_CODE *prg;
    for( prg = oprog_list; prg; prg = prg->next )
    {
        if ( prg->vnum == vnum )
            return( prg );
    }
    return NULL;
}

PROG_CODE *get_aprog_index( int vnum )
{
    PROG_CODE *prg;
    for( prg = aprog_list; prg; prg = prg->next )
    {
        if ( prg->vnum == vnum )
            return( prg );
    }
    return NULL;
}

PROG_CODE *get_rprog_index( int vnum )
{
    PROG_CODE *prg;
    for( prg = rprog_list; prg; prg = prg->next )
    {
        if ( prg->vnum == vnum )
            return( prg );
    }
    return NULL;
}

/*
* Read a letter from a file.
*/
char fread_letter( FILE *fp )
{
    char c;
    
    do
    {
        c = getc( fp );
    }
    while ( isspace(c) );
    
    return c;
}



/*
* Read a number from a file.
*/
int fread_number( FILE *fp )
{
    int number;
    bool sign;
    char c;
    
    do
    {
        c = getc( fp );
    }
    while ( isspace(c) );
    
    number = 0;
    
    sign   = FALSE;
    if ( c == '+' )
    {
        c = getc( fp );
    }
    else if ( c == '-' )
    {
        sign = TRUE;
        c = getc( fp );
    }
    
    if ( !isdigit(c) )
    {
        bugf( "Fread_number: bad format. (%c)", c );
    }
    
    while ( isdigit(c) )
    {
        number = number * 10 + c - '0';
        c      = getc( fp );
    }
    
    if ( sign )
        number = 0 - number;
    
    if ( c == '|' )
        number += fread_number( fp );
    else if ( c != ' ' )
        ungetc( c, fp );
    
    return number;
}

long fread_flag( FILE *fp)
{
    int number;
    char c;
    bool negative = FALSE;
    
    do
    {
        c = getc(fp);
    }
    while ( isspace(c));
    
    if (c == '-')
    {
        negative = TRUE;
        c = getc(fp);
    }
    
    number = 0;
    
    if (!isdigit(c))
    {
        while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
        {
            number += flag_convert(c);
            c = getc(fp);
        }
    }
    
    while (isdigit(c))
    {
        number = number * 10 + c - '0';
        c = getc(fp);
    }
    
    if (c == '|')
        number += fread_flag(fp);
    
    else if  ( c != ' ')
        ungetc(c,fp);
    
    if (negative)
        return -1 * number;
    
    return number;
}

long flag_convert(char letter )
{
    long bitsum = 0;
    char i;
    
    if ('A' <= letter && letter <= 'Z') 
    {
        bitsum = 1;
        for (i = letter; i > 'A'; i--)
            bitsum *= 2;
    }
    else if ('a' <= letter && letter <= 'z')
    {
        bitsum = 67108864; /* 2^26 */
        for (i = letter; i > 'a'; i --)
            bitsum *= 2;
    }
    
    return bitsum;
}




/*
* Read and allocate space for a string from a file.
* These strings are read-only and shared.
* Strings are hashed:
*   each string prepended with hash pointer to prev string,
*   hash code is simply the string length.
*   this function takes 40% to 50% of boot-up time.
*/
const char *fread_string( FILE *fp )
{
    char *plast;
    char c;
    
    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        bug( "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
        exit( 1 );
    }
    
    /*
    * Skip blanks.
    * Read first char.
    */
    do
    {
        c = getc( fp );
    }
    while ( isspace(c) );
    
    // strings are optionally started with '^' to enable leading whitespace characters
    // this special character must be stripped here
    if ( c == '^' )
    {
        c = getc(fp);
    }

    // strings are terminated by '~', we intern the empty string for efficiency
    if ( ( *plast++ = c ) == '~' )
        return &str_empty[0];
    
    for ( ;; )
    {
    /*
    * Back off the char type lookup,
    *   it was too dirty for portability.
    *   -- Furey
        */
        
        switch ( *plast = getc(fp) )
        {
        default:
            plast++;
            break;
            
        case EOF:
            /* temp fix */
            bug( "Fread_string: EOF", 0 );
            return NULL;
            /* exit( 1 ); */
            break;
            
        case '\n':
            plast++;
            *plast++ = '\r';
            break;
            
        case '\r':
            break;
            
        case '~':
            plast++;
            {
                union
                {
                    char *  pc;
                    char    rgc[sizeof(char *)];
                } u1;
                int ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;
                
                plast[-1] = '\0';
                // intern string if possible to save memory
                iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                {
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash    += sizeof(char *);
                    
                    if ( top_string[sizeof(char *)] == pHash[0]
                        &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
                        return pHash;
                }
                
                if ( fBootDb )
                {
                    pString     = top_string;
                    top_string      = plast;
                    u1.pc       = string_hash[iHash];
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash]  = pString;
                    
                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                }
                else
                {
                    return str_dup( top_string + sizeof(char *) );
                }
            }
        }
    }
}

/* new slightly different, simpler and working version by Bobble */
const char *fread_string_eol( FILE *fp )
{
    static char buf[MSL];
    char c;
    int i = 0;
    bool done = FALSE;

    while ( !done && i < MSL )
    {
	c = getc( fp );

	switch ( c )
	{
	default: 
	    buf[i++] = c;
	    break;
	case EOF:
	case '\n':
	    buf[i++] = '\0';
	    done = TRUE;
	    break;
	case '\r':
	    break;
	}
    }

    if ( i == MSL )
    {
        bug( "fread_string_eol: string too long", 0 );
        buf[0] = '\0';
    }
	
    return buf;
}

/* seems buggy..*/
const char *fread_string_eol_old( FILE *fp )
{
    static bool char_special[256-EOF];
    char *plast;
    char c;
    
    if ( char_special[EOF-EOF] != TRUE )
    {
        char_special[EOF -  EOF] = TRUE;
        char_special['\n' - EOF] = TRUE;
        char_special['\r' - EOF] = TRUE;
    }
    
    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        bug( "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
        exit( 1 );
    }
    
    /*
    * Skip blanks.
    * Read first char.
    */
    do
    {
        c = getc( fp );
    }
    while ( isspace(c) );
    
    if ( ( *plast++ = c ) == '\n')
        return &str_empty[0];
    
    for ( ;; )
    {
        if ( !char_special[ ( *plast++ = getc( fp ) ) - EOF ] )
            continue;
        
        switch ( plast[-1] )
        {
        default:
            break;
            
        case EOF:
	    /*
            bug( "Fread_string_eol  EOF", 0 );
            exit( 1 );
            break;
	    */
            
        case '\n':  case '\r':
            {
                union
                {
                    char *      pc;
                    char        rgc[sizeof(char *)];
                } u1;
                int ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;
                
                plast[-1] = '\0';
                iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                {
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash    += sizeof(char *);
                    
                    if ( top_string[sizeof(char *)] == pHash[0]
                        &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
                        return pHash;
                }
                
                if ( fBootDb )
                {
                    pString             = top_string;
                    top_string          = plast;
                    u1.pc               = string_hash[iHash];
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash]  = pString;
                    
                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                }
                else
                {
                    return str_dup( top_string + sizeof(char *) );
                }
            }
        }
    }
}



/*
* Read to end of line (for comments).
*/
void fread_to_eol( FILE *fp )
{
    char c;
    
    do
    {
        c = getc( fp );
    }
    while ( c != '\n' && c != '\r' );
    
    do
    {
        c = getc( fp );
    }
    while ( c == '\n' || c == '\r' );
    
    ungetc( c, fp );
    return;
}



/*
* Read one word (into static buffer).
*/
const char *fread_word( FILE *fp )
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;
    
    do
    {
        cEnd = getc( fp );
    }
    while ( isspace( cEnd ) );
    
    if ( cEnd == '\'' || cEnd == '"' )
    {
        pword   = word;
    }
    else
    {
        word[0] = cEnd;
        pword   = word+1;
        cEnd    = ' ';
    }
    
    for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
        *pword = getc( fp );
        if ( *pword == EOF
	     || cEnd == ' ' ? isspace(*pword) : *pword == cEnd )
        {
            if ( cEnd == ' ' )
                ungetc( *pword, fp );
            *pword = '\0';
            return word;
        }
    }
    
    bug( "Fread_word: word too long.", 0 );
    exit( 1 );
    return NULL;
}

/*
* Allocate some ordinary memory,
*   with the expectation of freeing it someday.
*/
void *alloc_mem( int sMem )
{
    void *pMem;
    int iList;
    
#ifdef MAGIC_CHECKING
    int *magic;
    sMem += sizeof(*magic);
#endif
    
    for ( iList = 0; iList < MAX_MEM_LIST; iList++ )
    {
        if ( sMem <= rgSizeList[iList] )
            break;
    }
    
    if ( iList == MAX_MEM_LIST )
    {
        bug( "Alloc_mem: size %d too large.", sMem );
        exit( 1 );
    }
    
    if ( rgFreeList[iList] == NULL )
    {
        pMem              = alloc_perm( rgSizeList[iList] );
    }
    else
    {
        pMem              = rgFreeList[iList];
        rgFreeList[iList] = * ((void **) rgFreeList[iList]);
        // clear memory before usage - alloc_perm does it too, so the two can be used (more or less) interchangeably
        memset(pMem, 0, sMem);
    }
    
#ifdef MAGIC_CHECKING
    magic = (int *) pMem;
    *magic = MAGIC_NUM;
    
    /* pMem += sizeof(*magic);    mkw:illegal use of (void*) */
    pMem = (void*) (((char*)pMem) + sizeof (*magic));
    /* pMem = (void*) magic[1];   // mkw:should have intended effect */
#endif
    
    
    return pMem;
}



/*
* Free some memory.
* Recycle it back onto the free list for blocks of that size.
*/
void free_mem( void *pMem, int sMem )
{
    int iList;
    
#ifdef MAGIC_CHECKING
    int *magic;
    
    pMem = (void*) (((char*)pMem) - sizeof (*magic));
    magic = (int *) pMem;
    
    if (*magic != MAGIC_NUM)
    {
        bug("Attempt to recyle invalid memory of size %d.",sMem);
        bug((char*) pMem + sizeof(*magic),0);
        return;
    }
    *magic = 0;
    sMem += sizeof(*magic);
#endif
    
    for ( iList = 0; iList < MAX_MEM_LIST; iList++ )
    {
        if ( sMem <= rgSizeList[iList] )
            break;
    }
    
    if ( iList == MAX_MEM_LIST )
    {
        bug( "Free_mem: size %d too large.", sMem );
        exit( 1 );
    }
    
    * ((void **) pMem) = rgFreeList[iList];
    rgFreeList[iList]  = pMem;
    
    return;
}


/*
* Allocate some permanent memory.
* Permanent memory is never freed,
*   pointers into it may be copied safely.
*/
void *alloc_perm( int sMem )
{
    static char *pMemPerm;
    static int iMemPerm;
    void *pMem;
    
    while ( sMem % sizeof(long) != 0 )
        sMem++;
    if ( sMem > MAX_PERM_BLOCK )
    {
        bug( "Alloc_perm: %d too large.", sMem );
        exit( 1 );
    }
    
    if ( pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK )
    {
        iMemPerm = 0;
        if ( ( pMemPerm = calloc( 1, MAX_PERM_BLOCK ) ) == NULL )
        {
            log_error( "Alloc_perm" );
            exit( 1 );
        }
    }
    
    pMem        = pMemPerm + iMemPerm;
    iMemPerm   += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}


/*
* Hashtable of strings recently allocated and not freed
* Used for debugging memory leaks
*/
#define MAX_STR_DUP_KEY 1009
static const char* str_dup_hash[MAX_STR_DUP_KEY];
static bool str_dup_ready = FALSE;

void reset_str_dup()
{
    memset(str_dup_hash, 0, MAX_STR_DUP_KEY * sizeof(char*));
    str_dup_ready = TRUE;
    return;    
}

void remember_str_dup(const char *str)
{
    int key = ((unsigned long)str) % MAX_STR_DUP_KEY;
    
    if (str_dup_hash[key] == NULL)
        str_dup_hash[key] = str;
    STR_DUP_STRINGS += 1;
    // auto-dump recently duplicated strings when memory leak is suspected
    if (HIGHEST_STR_DUP_STRINGS < STR_DUP_STRINGS)
    {
        HIGHEST_STR_DUP_STRINGS = STR_DUP_STRINGS;
        if (HIGHEST_STR_DUP_STRINGS % 1000 == 0 && str_dup_ready)
            dump_str_dup();
    }
    return;
}

void forget_str_dup(const char *str)
{
    int key = ((unsigned long)str) % MAX_STR_DUP_KEY;
    
    if (str_dup_hash[key] == str)
        str_dup_hash[key] = NULL;
    STR_DUP_STRINGS -= 1;
    return;
}

void dump_str_dup()
{
    int key;
    
    log_string("Strings duplicated but never freed (sample):");
    for (key = 0; key < MAX_STR_DUP_KEY; key++)
    {
        if (str_dup_hash[key] != NULL)
            fprintf(stdout, "> %s\n\r", str_dup_hash[key]);
    }
    reset_str_dup();
    return;
}

/*
* Duplicate a string into dynamic memory.
* Fread_strings are read-only and shared.
*/
const char *str_dup( const char *str )
{
    if ( str == NULL )
        return NULL;
    
    if ( str[0] == '\0' )
        return &str_empty[0];
    
    if ( str >= string_space && str < top_string )
        return str;
    
    char *str_new = alloc_mem( strlen(str) + 1 );
    strcpy( str_new, str );
    remember_str_dup( str_new );
    return str_new;
}

/*
* Free a string.
* Null is legal here to simplify callers.
* Read-only shared strings are not touched.
*/
void free_string( const char *pstr )
{
    if ( pstr == NULL
        ||   pstr == &str_empty[0]
        || ( pstr >= string_space && pstr < top_string ) )
        return;
    
    // pstr is no shared, so we deallocate it, ignoring const
    forget_str_dup( pstr );
    free_mem( (char*)pstr, strlen(pstr) + 1 );
    return;
}

/*
 * replaces a shared string with a capitalized version
 * free_string(str) is called, so will fail on non-shared strings
 */
const char *upper_realloc( const char *str )
{
    char buf[MSL];
    
    if ( str[0] == UPPER(str[0]) )
        return str;
    
    buf[0] = UPPER(str[0]);
    strcpy(buf+1, str+1);
    
    free_string(str);
    return str_dup(buf);
}

/*
 * replaces a shared string with a trimmed version
 * free_string(str) is called, so will fail on non-shared strings
 */
const char *trim_realloc( const char *str )
{
    if ( *str == '\0' )
        return str;
    
    const char *first = ltrim(str);
    if ( *first == '\0' )
    {
        free_string(str);
        return str_empty;
    }
    
    const char *last = str + strlen(str) - 1;
    while ( last >= first && isspace(*last) )
        last--;
    
    // copy substring
    char buf[MSL];
    strcpy(buf, first);
    buf[1 + (last-first)] = '\0';
    
    free_string(str);
    return str_dup(buf);
}

/* Erwins sample for sorting areas. Change the relational
   operators to reverse sorting. Currently it goes from lowest
   to highest. Can make a duplicate function later with the
   operators reversed so players can add arguments to the areas
   command and sort however they want */

int compare_area (const void *v1, const void *v2)
{
	AREA_DATA *a1 = *(AREA_DATA**)v1;
	AREA_DATA *a2 = *(AREA_DATA**)v2;

	if (a1->minlevel < a2->minlevel)
		return -1; 
	else if (a1->minlevel > a2->minlevel)
		return +1;
	else
		return 0;
}

int compare_area_max (const void *v1, const void *v2)
{
	AREA_DATA *a1 = *(AREA_DATA**)v1;
	AREA_DATA *a2 = *(AREA_DATA**)v2;

	if (a1->maxlevel > a2->maxlevel )
		return -1; 
	else if (a1->maxlevel < a2->maxlevel )
		return +1;
	else
		return 0;
} 

#define MAX_AREAS 1000


/* New areas command that displays areas in order from lowest to
   highest level by default. Add arguments later for additional
   sorting */

void do_areas( CHAR_DATA *ch )
{
    char buf[MSL];
    AREA_DATA *pArea1;
    AREA_DATA *sorted_areas[MAX_AREAS];
    BUFFER *output;
    int count=0;
    int i;
     
    output = new_buf();

    /* The first loop to cycle through all areas that meet our criteria
       of being "in-game" (greater than 4 security) */        
    
    for ( pArea1 = area_first; pArea1 != NULL ; pArea1 = pArea1->next )
    {
        if (is_area_ingame(pArea1))
        {
            sorted_areas[count] = pArea1;
            count++;
        }
    }

//    argument = one_argument( argument, arg );

    /* Quicksort - standard C function that is doing all of the hard work
       for us */

	qsort(
                sorted_areas,		  /* Where does the data to sort start? */
                count,	                  /* How many elements? */
                sizeof(sorted_areas[0]),  /* How big is each element? */
                compare_area);	          /* Uses the above "int compare_area" to do the sorting */


    /* Loop for displaying the information. Color gets stripped from area name
       for formatting purposes */
    send_to_char("{DArea Name                {WMinLvl   {CMaxLvl  {WMiniQsts  {DBuilders\n\r{x",ch);

    for (i=0; i != count; i++)
    {
        sprintf (buf,"%-26s  {w%-4d{x-  {c%-4d      {m%-4d{x   %-14s \n\r",
            remove_color(sorted_areas[i]->name), /* Uses area_name without color */
            sorted_areas[i]->minlevel,   /* The new min-level field */
            sorted_areas[i]->maxlevel,   /* The new max-level field */
            sorted_areas[i]->miniquests, /* The new miniquests field */
            sorted_areas[i]->credits);   /* Credits field is now only the people who built area */
        add_buf(output, buf);
    }
    page_to_char(buf_string(output),ch);
    free_buf(output);
      
    return;
} 

DEF_DO_FUN(do_memory)
{
    ptc( ch, "          %-5s %s\n\r", "C", "Lua" );
    ptc( ch, "Affects   %5d %5d\n\r", top_affect, count_AFFECT()); 
    ptc( ch, "Areas     %5d %5d\n\r", top_area, count_AREA()); 
    ptc( ch, "ExDes     %5d\n\r", top_ed        ); 
    ptc( ch, "Exits     %5d %5d\n\r", top_exit, count_EXIT()); 
    ptc( ch, "Helps     %5d %5d\n\r", top_help, count_HELP()); 
    ptc( ch, "Socials   %5d\n\r", maxSocial  ); 
    ptc( ch, "Resets    %5d %5d\n\r", top_reset, count_RESET()); 
    ptc( ch, "Rooms     %5d %5d\n\r", top_room, count_ROOM()); 
    ptc( ch, "Shops     %5d %5d\n\r", top_shop, count_SHOP()); 
    ptc( ch, "\n\r");
    ptc( ch, "Mobs      %5d %5d\n\r", top_mob_index, count_MOBPROTO());
    ptc( ch, " (in use) %5d\n\r", mobile_count  );
    ptc( ch, "Objs      %5d %5d\n\r", top_obj_index, count_OBJPROTO());
    ptc( ch, " (in use) %5d %5d\n\r", object_count, count_OBJ());
    ptc( ch, "\n\r");
    ptc( ch, "Mprogs    %5d\n\r", top_mprog_index);
    ptc( ch, " (lua)    %5d\n\r", lua_mprogs);
    ptc( ch, "Oprogs    %5d\n\r", top_oprog_index);
    ptc( ch, "Aprogs    %5d\n\r", top_aprog_index);
    ptc( ch, "Rprogs    %5d\n\r", top_rprog_index);
    ptc( ch, "Total     %5d %5d\n\r",
            top_mprog_index + top_oprog_index + top_aprog_index + top_rprog_index,
            count_PROG() );
    ptc( ch, "\n\r");
    
    ptc( ch, "Strings %5d strings of %7d bytes (max %d).\n\r",
        nAllocString, sAllocString, MAX_STRING );
    
    ptc( ch, "Perms   %5d blocks  of %7d bytes.\n\r",
        nAllocPerm, sAllocPerm );
    ptc( ch, "STR_DUP_STRINGS         %d\n\r", STR_DUP_STRINGS);
    ptc( ch, "HIGHEST_STR_DUP_STRINGS %d\n\r", HIGHEST_STR_DUP_STRINGS);
    ptc( ch, "\n\r");
    ptc( ch, "Lua usage:        %dk\n\r", GetLuaMemoryUsage());
    ptc( ch, "Lua game objects: %d\n\r", GetLuaGameObjectCount());
    ptc( ch, "Lua environments: %d\n\r", GetLuaEnvironmentCount());

    ptc( ch, "\n\r");
    ptc( ch, "BINARY INFO%s\n\r", bin_info_string );
    
    return;
}

/*
* Stick a little fuzz on a number.
*/
int number_fuzzy( int number )
{
    switch ( number_bits( 2 ) )
    {
    case 0:  number -= 1; break;
    case 3:  number += 1; break;
    }
    
    return UMAX( 1, number );
}



/*
* Generate a random number.
*/
int number_range( int from, int to )
{
    int power;
    int number;
    
    if (from == 0 && to == 0)
        return 0;
    
    if ( ( to = to - from + 1 ) <= 1 )
        return from;
    
    for ( power = 2; power < to; power <<= 1 )
        ;
    
    while ( ( number = number_mm() & (power -1 ) ) >= to )
        ;
    
    return from + number;
}



/*
* Generate a percentile roll.
*/
int number_percent( void )
{
    int percent;
    
    while ( (percent = number_mm() & (128-1) ) > 99 )
        ;
    
    return 1 + percent;
}



/*
* Generate a random door.
*/
int number_door( void )
{
    int door;
    
    while ( ( door = number_mm() & 15 ) > MAX_DIR-1)
        ;
    
    return door;
}

int number_bits( int width )
{
    return number_mm( ) & ( ( 1 << width ) - 1 );
}




/*
* I've gotten too many bad reports on OS-supplied random number generators.
* This is the Mitchell-Moore algorithm from Knuth Volume II.
* Best to leave the constants alone unless you've read Knuth.
* -- Furey
*/

/* I noticed streaking with this random number generator, so I switched
back to the system srandom call.  If this doesn't work for you, 
define OLD_RAND to use the old system -- Alander */

#if defined (OLD_RAND)
static  int     rgiState[2+55];
#endif

void init_mm( )
{
#if defined (OLD_RAND)
    int *piState;
    int iState;
    
    piState     = &rgiState[2];
    
    piState[-2] = 55 - 55;
    piState[-1] = 55 - 24;
    
    piState[0]  = ((int) current_time) & ((1 << 30) - 1);
    piState[1]  = 1;
    for ( iState = 2; iState < 55; iState++ )
    {
        piState[iState] = (piState[iState-1] + piState[iState-2])
            & ((1 << 30) - 1);
    }
#else
    srandom(time(NULL)^getpid());
#endif
    return;
}



long number_mm( void )
{
#if defined (OLD_RAND)
    int *piState;
    int iState1;
    int iState2;
    int iRand;
    
    piState             = &rgiState[2];
    iState1             = piState[-2];
    iState2             = piState[-1];
    iRand               = (piState[iState1] + piState[iState2])
        & ((1 << 30) - 1);
    piState[iState1]    = iRand;
    if ( ++iState1 == 55 )
        iState1 = 0;
    if ( ++iState2 == 55 )
        iState2 = 0;
    piState[-2]         = iState1;
    piState[-1]         = iState2;
    return iRand >> 6;
#else
    return random() >> 6;
#endif
}


/*
* Roll some dice.
*/
int dice( int number, int size )
{
    int idice;
    int sum;
    
    switch ( size )
    {
    case 0: return 0;
    case 1: return number;
    }
    
    for ( idice = 0, sum = 0; idice < number; idice++ )
        sum += number_range( 1, size );
    
    return sum;
}


/*
* Simple linear interpolation.
*/
int interpolate( int level, int value_00, int value_maxL )
{
    return value_00 + level * (value_maxL - value_00) / MAX_LEVEL;
}


/*
* Removes the tildes from a string.
* Used for player-entered strings that go into disk files.
*/
void smash_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
    {
        if ( *str == '~' )
            *str = '-';
    }
}

const char* smash_tilde_cc( const char *str )
{
    if ( strchr(str, '~') == NULL )
        return str;
        
    static char buf[MSL];
    char *next = buf;
        
    for ( ; *str != '\0'; str++, next++ )
    {
        if ( *str == '~' )
            *next = '-';
        else
            *next = *str;
    }
    *next = '\0';
    
    return buf;
}

char* smash_tilde_cpy( char *dest, const char *source )
{
    strcpy(dest, source);
    smash_tilde(dest);
    return dest;
}


/*
* Compare strings, case insensitive.
* Return TRUE if different
*   (compatibility with historical functions).
*/
bool str_cmp( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
        bug( "Str_cmp: null astr.", 0 );
        return TRUE;
    }
    
    if ( bstr == NULL )
    {
        bug( "Str_cmp: null bstr.", 0 );
        return TRUE;
    }
    
    for ( ; *astr || *bstr; astr++, bstr++ )
    {
        if ( LOWER(*astr) != LOWER(*bstr) )
            return TRUE;
    }
    
    return FALSE;
}


/*
* Compare strings, case insensitive, for prefix matching.
* Return TRUE if astr not a prefix of bstr
*   (compatibility with historical functions).
*/
bool str_prefix( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
        bug( "Strn_cmp: null astr.", 0 );
        return TRUE;
    }
    
    if ( bstr == NULL )
    {
        bug( "Strn_cmp: null bstr.", 0 );
        return TRUE;
    }
    
    for ( ; *astr; astr++, bstr++ )
    {
        if ( LOWER(*astr) != LOWER(*bstr) )
            return TRUE;
    }
    
    return FALSE;
}



/*
* Compare strings, case insensitive, for match anywhere.
* Returns TRUE is astr not part of bstr.
*   (compatibility with historical functions).
*/
bool str_infix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;
    
    if ( ( c0 = LOWER(astr[0]) ) == '\0' )
        return FALSE;
    
    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    
    for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
    {
        if ( c0 == LOWER(bstr[ichar]) && !str_prefix( astr, bstr + ichar ) )
            return FALSE;
    }
    
    return TRUE;
}



/*
* Compare strings, case insensitive, for suffix matching.
* Return TRUE if astr not a suffix of bstr
*   (compatibility with historical functions).
*/
bool str_suffix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;
    
    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if ( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
        return FALSE;
    else
        return TRUE;
}



/*
* Returns an initial-capped string.
*/
char *capitalize( const char *str )
{
    static SR_BUF sr_buf;
    char *strcap = next_sr_buf( &sr_buf );
    int i;
    
    if (str == NULL)
    {
        bug("NULL string passed to capitalize function.", 0);
        return NULL;
    }
    
    for ( i = 0; str[i] != '\0'; i++ )
        strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}

/*
* Returns a DIFFERENT initial-capped string than capitalize() does.
*
* This very foul hack was necessary because capitalize acts upon constant
* data and returns a pointer to a static variable.  If it is desired to
* capitalize two values in a single call to sprintf(), for example, the
* second call to capitalize() will overwrite the value in strcap before
* either value is printed.  Making strcap a local variable is an insufficient
* answer, as the returned pointer would then refer to strcap after that
* variable goes out of scope, and the contents of that memory would be
* unreliable.  Sorry, I don't care for this either.  --Rimbol 9/13/97
*/
/* capitalize now uses ring buffer for return strings, thus capitalize_2
 * is no longer needed --Bobble
char *capitalize_2( const char *str )
{
    static char strcap[MAX_STRING_LENGTH];
    int i;
    
    if (str == NULL)
    {
        bug("NULL string passed to capitalize_2 function.", 0);
        return NULL;
    }
    
    for ( i = 0; str[i] != '\0'; i++ )
        strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}
*/

/* capitalizes all words in a string --Bobble */
char* cap_all( const char* str )
{
    static SR_BUF sr_buf;
    char *buf = next_sr_buf( &sr_buf );
    int i;
    
    if (str == NULL)
    {
        bug("decap: NULL string given", 0);
        return NULL;
    }

    for ( i = 0; str[i] != '\0'; i++ )
	if ( i == 0 || buf[i-1] == ' ' )
	    buf[i] = UPPER(str[i]);
	else
	    buf[i] = LOWER(str[i]);

    buf[i] = '\0';
    return buf;
}    

/*
* Append a string to a file.
*/
void append_file( CHAR_DATA *ch, const char *file, const char *str )
{
    FILE *fp;
    /*  char cur_time[25];
    
     strftime( cur_time, 25, "%m/%d/%Y %H:%M:%S", localtime( &current_time ) );
    */  
    if ( IS_NPC(ch) || str[0] == '\0' )
        return;
    
    if ( ( fp = fopen( file, "a" ) ) == NULL )
    {
        log_error( file );
        send_to_char( "Could not open the file!\n\r", ch );
    }
    else
    {
        fprintf(fp, "%ld | %5d | %s | %s\n", 
            current_time, 
            ch->in_room ? ch->in_room->vnum : 0, 
            ch->name, 
            str );
        
        fclose( fp );
    }
    
    return;
}



/*
* Reports a bug.
*/
void bug( const char *fmt, int param )
{
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, fmt, param);
    bug_string(buf);
}

void bug_string( const char *str )
{
    static bool bug_in_progress=FALSE;
    bool recurse;
    char buf[MAX_STRING_LENGTH];

    recurse=bug_in_progress;
    bug_in_progress=TRUE;
    
    if ( fpArea != NULL )
    {
        int iLine;
        int iChar;
        
        if ( fpArea == stdin )
        {
            iLine = 0;
        }
        else
        {
            iChar = ftell( fpArea );
            fseek( fpArea, 0, 0 );
            for ( iLine = 0; ftell( fpArea ) < iChar; iLine++ )
            {
                while ( getc( fpArea ) != '\n' )
                    ;
            }
            fseek( fpArea, iChar, 0 );
        }
        
        sprintf( buf, "[*****] FILE: %s LINE: %d", strArea, iLine );
        log_string( buf );

        if ( !recurse )
            wiznet( buf, NULL, NULL, WIZ_BUGS, 0, 0 );
    }
    
    sprintf(buf, "[*****] BUG: %s", str );
    if (!recurse)
    {
        strcat( buf, "\n\rLast command: " );
        strcat( buf, last_command );
    }

    log_string( buf );
    log_trace();
    
    if ( !recurse )
    {
        wiznet( buf, NULL, NULL, WIZ_BUGS, 0, 0 );
        bug_in_progress=FALSE;
    }
    else
    {
        log_string("[*****] BUG: recursion in bug(), not sending to wiznet");
    }

    return;
}

// return first (minimal) substring of s delimited by c_start and c_end
char* substr_delim(const char *s, char c_start, char c_end)
{
    static char ss_buf[MAX_STRING_LENGTH];
    int ss_next = 0;
    bool found = FALSE;
    
    while (*s != 0)
    {
        if (!found)
            found = (*s == c_start);
        else
        {
            if (*s == c_end)
                break;
            else
                ss_buf[ss_next++] = *s; 
        }
        s++;
    }
    // terminate string
    ss_buf[ss_next] = 0;
    
    return ss_buf;
}

// logs a backtrace
void log_trace()
{
    const int MAX_TRACE = 32;
    void* buffer[MAX_TRACE];
    int trace_size = backtrace (buffer, MAX_TRACE);
    char **trace_msg = backtrace_symbols (buffer, trace_size);
    char address_buf[MAX_STRING_LENGTH], cmd[MAX_STRING_LENGTH];
    int i;
    
    if (trace_msg == NULL || trace_size < 2)
        return;

    // trace_msg[i] contains the address in hexadecimal in the form "..aeaea() [0x12345678]"
    // first, we extract the address information
    address_buf[0] = 0;
    // we start at 1 to skip call of log_trace        
    for (i = 1; i < trace_size; i++)
    {            
        if (strstr(trace_msg[i], "aeaea") != NULL)
        {
            if ( strlen(address_buf) != 0 )
                strcat(address_buf, " ");
            strcat(address_buf, substr_delim(trace_msg[i], '[', ']'));
        }
        else
            break;
    }    
    free(trace_msg);
    
    // second, we feed the addresses into addr2line to get readable information
    //sprintf(cmd, "addr2line -pfs -e ../src/aeaea %s", address_buf);
    sprintf(cmd, "addr2line -fs -e ../src/aeaea %s", address_buf);
    log_string(cmd);
    if ( system(cmd) == -1 )
    {
        // ignore error
    }

    return;
}

/*
* Writes a string to the log.
*/
void log_string( const char *str )
{
    char *strtime;
    
    strtime                    = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';
    fprintf( stderr, "%s :: %s\n", strtime, str );
    return;
}





void log_error( const char *str )
{
    char *strtime;
    static char buf[MIL];

    strtime                    = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';
    sprintf( buf, "%s :: %s", strtime, str );
    perror( buf );
    return;
}

/* Writes a string to a special cheating log-file */
void cheat_log( const char *str )
{
    FILE *fp;
    char ts[MSL];

    fp = fopen (CHEAT_LIST, "a");
    
    if (!fp)
    {
        bug ("Could not open " CHEAT_LIST " for writing", 0);
        return;
    }
    
    /* this is the main thing ;) */
    
    /* have to add the EOL to timestamp or it won't have one, weird */
    strcpy( ts, ctime(&current_time));
    ts[strlen(ts)-1] = '\0';
    fprintf( fp, "%s::%s\n",ts, str );

    fclose (fp);
}

DEF_DO_FUN(do_cheatlog)
{
    FILE *fp;
    BUFFER *output;
    const char *buf;
    bool is_eof = FALSE;

    if ( argument[0] == '\0' )
    {
	fp = fopen (CHEAT_LIST, "r");
    
	if (!fp)
	{
	    bug ("Could not open " CHEAT_LIST " for reading", 0);
	    send_to_char( "No cheating log found.\n\r", ch );
	    return;
	}

	send_to_char( "Cheating log:\n\r", ch );
	output = new_buf();
	while ( !is_eof )
	{
	    buf = fread_string_eol( fp );
	    is_eof = buf[0] == '\0';
	    if ( !is_eof )
	    {
		if ( !add_buf(output, buf) )
		    break;
		add_buf( output, "\n\r" );
	    }
	}

	fclose (fp);

	page_to_char( buf_string(output), ch );
	free_buf(output);

	return;
    }
    
    if ( !strcmp(argument, "clear") )
    {
	fp = fopen (CHEAT_LIST, "w");
    
	if (!fp)
	{
	    bug ("Could not open " CHEAT_LIST " for writing", 0);
	    return;
	}
    
	fclose (fp);

	send_to_char( "Cheating log cleared.\n\r", ch );
	return;
    }

    send_to_char( "Syntax: cheatlog [clear]\n\r", ch );
}

/*
* This function is here to aid in debugging.
* If the last expression in a function is another function call,
*   gcc likes to generate a JMP instead of a CALL.
* This is called "tail chaining."
* It hoses the debugger call stack for that call.
* So I make this the last call in certain critical functions,
*   where I really need the call stack to be right for debugging!
*
* If you don't understand this, then LEAVE IT ALONE.
* Don't remove any calls to tail_chain anywhere.
*
* -- Furey
*/
void tail_chain( void )
{
    return;
}


/*******************************************************************************
 * Methods for reading from a buffer;                                          *
 * like file reading methods but modified to read from buffer instead of file  *
 * by Henning Koehler <koehlerh@in.tum.de>                                     *
 *******************************************************************************/

/*
* Read a letter from a buffer.
*/
char bread_letter( RBUFFER *rbuf )
{
    char c;
#if defined(BREAD_DEBUG)
   log_string("bread_letter: start");
#endif
    
    do
    {
        c = bgetc( rbuf );
    }
    while ( isspace(c) );
    
#if defined(BREAD_DEBUG)
   log_string("bread_letter: done");
#endif
    return c;
}

/*
* Read a number from a buffer.
*/
int bread_number( RBUFFER *rbuf )
{
    int number;
    bool sign;
    char c;
    
#if defined(BREAD_DEBUG)
   log_string("bread_number: start");
#endif
    do
    {
        c = bgetc( rbuf );
    }
    while ( isspace(c) );
    
    number = 0;
    
    sign   = FALSE;
    if ( c == '+' )
    {
        c = bgetc( rbuf );
    }
    else if ( c == '-' )
    {
        sign = TRUE;
        c = bgetc( rbuf );
    }
    
    if ( !isdigit(c) )
    {
        bug( "bread_number: bad format.", 0 );
    }
    
    while ( isdigit(c) )
    {
        number = number * 10 + c - '0';
        c      = bgetc( rbuf );
    }
    
    if ( sign )
        number = 0 - number;
    
    if ( c == '|' )
        number += bread_number( rbuf );
    else if ( c != ' ' )
        bungetc( rbuf );
    
#if defined(BREAD_DEBUG)
   log_string("bread_number: done");
#endif
    return number;
}

/* read a flag from a buffer
 */
long bread_flag( RBUFFER *rbuf )
{
    int number;
    char c;
    bool negative = FALSE;
#if defined(BREAD_DEBUG)
   log_string("bread_flag: start");
#endif
    
    do
    {
        c = bgetc(rbuf);
    }
    while ( isspace(c));
    
    if (c == '-')
    {
        negative = TRUE;
        c = bgetc(rbuf);
    }
    
    number = 0;
    
    if (!isdigit(c))
    {
        while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
        {
            number += flag_convert(c);
            c = bgetc(rbuf);
        }
    }
    
    while (isdigit(c))
    {
        number = number * 10 + c - '0';
        c = bgetc(rbuf);
    }
    
    if (c == '|')
        number += bread_flag(rbuf);
    
    else if  ( c != ' ')
        bungetc(rbuf);
    
    if (negative)
        return -1 * number;
    
#if defined(BREAD_DEBUG)
   log_string("bread_flag: done");
#endif
    return number;
}

/*
* Read and allocate space for a string from a buffer.
* These strings are read-only and shared.
* Strings are hashed:
*   each string prepended with hash pointer to prev string,
*   hash code is simply the string length.
*   this function takes 40% to 50% of boot-up time.
*/
const char* bread_string( RBUFFER *rbuf )
{
    char *plast;
    char c;
#if defined(BREAD_DEBUG)
   log_string("bread_string: start");
#endif
    
    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        bug( "bread_string: MAX_STRING %d exceeded.", MAX_STRING );
        exit( 1 );
    }
    
    /*
    * Skip blanks.
    * Read first char.
    */
    do
    {
        c = bgetc( rbuf );
    }
    while ( isspace(c) );
    
    // strings are optionally started with '^' to enable leading whitespace characters
    // this special character must be stripped here
    if ( c == '^' )
    {
        c = bgetc(rbuf);
    }

    // strings are terminated by '~', we intern the empty string for efficiency
    if ( ( *plast++ = c ) == '~' )
        return &str_empty[0];
    
    for ( ;; )
    {
    /*
    * Back off the char type lookup,
    *   it was too dirty for portability.
    *   -- Furey
        */
        
        switch ( *plast = bgetc( rbuf ) )
        {
        default:
            plast++;
            break;
            
        case EOF:
            /* temp fix */
            bug( "bread_string: EOF", 0 );
            return NULL;
            /* exit( 1 ); */
            break;
            
        case '\n':
            plast++;
            *plast++ = '\r';
            break;
            
        case '\r':
            break;
            
        case '~':
            plast++;
            {
                union
                {
                    char *  pc;
                    char    rgc[sizeof(char *)];
                } u1;
                int ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;
                
                plast[-1] = '\0';
                // intern string if possible to save memory
                iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                {
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash    += sizeof(char *);
                    
                    if ( top_string[sizeof(char *)] == pHash[0]
                        &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
                        return pHash;
                }
                
                if ( fBootDb )
                {
                    pString     = top_string;
                    top_string      = plast;
                    u1.pc       = string_hash[iHash];
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash]  = pString;
                    
                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                }
                else
                {
                    return str_dup( top_string + sizeof(char *) );
                }
            }
        }
    }
}

const char* bread_string_eol( RBUFFER *rbuf )
{
    static bool char_special[256-EOF];
    char *plast;
    char c;
#if defined(BREAD_DEBUG)
   log_string("bread_string_eol: start");
#endif
    
    if ( char_special[EOF-EOF] != TRUE )
    {
        char_special[EOF -  EOF] = TRUE;
        char_special['\n' - EOF] = TRUE;
        char_special['\r' - EOF] = TRUE;
    }
    
    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        bug( "bread_string: MAX_STRING %d exceeded.", MAX_STRING );
        exit( 1 );
    }
    
    /*
    * Skip blanks.
    * Read first char.
    */
    do
    {
        c = bgetc( rbuf );
    }
    while ( isspace(c) );
    
    if ( ( *plast++ = c ) == '\n')
        return &str_empty[0];
    
    for ( ;; )
    {
        if ( !char_special[ ( *plast++ = bgetc( rbuf ) ) - EOF ] )
            continue;
        
        switch ( plast[-1] )
        {
        default:
            break;
            
        case EOF:
	    /*
            bug( "bread_string_eol  EOF", 0 );
            exit( 1 );
            break;
	    */
            
        case '\n':  case '\r':
            {
                union
                {
                    char *      pc;
                    char        rgc[sizeof(char *)];
                } u1;
                int ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;
                
                plast[-1] = '\0';
                iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                {
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash    += sizeof(char *);
                    
                    if ( top_string[sizeof(char *)] == pHash[0]
                        &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
                        return pHash;
                }
                
                if ( fBootDb )
                {
                    pString             = top_string;
                    top_string          = plast;
                    u1.pc               = string_hash[iHash];
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash]  = pString;
                    
                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                }
                else
                {
                    return str_dup( top_string + sizeof(char *) );
                }
            }
        }
    }
}



/*
* Read to end of line (for comments).
*/
void bread_to_eol( RBUFFER *rbuf )
{
    char c;
#if defined(BREAD_DEBUG)
   log_string("bread_to_eol: start");
#endif
    
    do
    {
        c = bgetc( rbuf );
    }
    while ( c != '\n' && c != '\r' );
    
    do
    {
        c = bgetc( rbuf );
    }
    while ( c == '\n' || c == '\r' );
    
    bungetc( rbuf );
#if defined(BREAD_DEBUG)
   log_string("bread_to_eol: done");
#endif
    return;
}



/*
* Read one word (into static buffer).
*/
const char* bread_word( RBUFFER *rbuf )
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;
#if defined(BREAD_DEBUG)
   log_string("bread_word: start");
#endif
    
    do
    {
        cEnd = bgetc( rbuf );
    }
    while ( isspace( cEnd ) );
    
    if ( cEnd == '\'' || cEnd == '"' )
    {
        pword   = word;
    }
    else
    {
        word[0] = cEnd;
        pword   = word+1;
        cEnd    = ' ';
    }
    
    for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
        *pword = bgetc( rbuf );
        if ( *pword == EOF
	     || cEnd == ' ' ? isspace(*pword) : *pword == cEnd )
        {
            if ( cEnd == ' ' )
                bungetc( rbuf );
            *pword = '\0';
#if defined(BREAD_DEBUG)
   log_string("bread_word: done");
#endif
            return word;
        }
    }
    
    bug( "bread_word: word too long.", 0 );
    exit( 1 );
    return NULL;
}


