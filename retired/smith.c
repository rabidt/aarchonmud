#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"

#define MAX_WEAPON_SUBTYPES 8
#define MAX_ARMOR_SUBTYPES 12

DECLARE_DO_FUN(do_save);

bool	smith_welcome		args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_type			args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_subtype		args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_purchase		args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_material		args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_quality		args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_color			args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_personal		args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_keywords		args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_level			args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_inventory		args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	smith_select		args( ( DESCRIPTOR_DATA *d, char *argument ) );

void smith_show_obj	args((CHAR_DATA *ch, OBJ_DATA *obj));
void smith_sticky_obj	args((CHAR_DATA *ch, OBJ_DATA *obj));

struct weapon_subtype
{
	char *name;
	int hands;
	char *damtype;
	int dammult;
	int size;	
	int cost;
};

struct weapon_smith_type
{
	char *name;
	int type;
	int num_varieties;
	bool hard;
	struct weapon_subtype varieties[MAX_WEAPON_SUBTYPES];
};

const struct weapon_smith_type weapon_smith_table[]=
{

{ "axe",		WEAPON_AXE,		3,	TRUE,	{
{"hand axe",	1,	"chop",	99,	83,	8},
{"broad axe",	2,	"slice",	108,	197,	12},
{"battle axe",	2,	"cleave",	110,	267,	15}
}},

{ "dagger",		WEAPON_DAGGER,	4,	TRUE, {
{"switchblade",	1,	"sting",	82,	8,	1},
{"dagger",		1,	"pierce",	94,	25,	3},
{"knife",		1,	"slice",	95,	29,	4},
{"dirk",		1,	"stab",	96,	21,	6}
}},

{ "flail",		WEAPON_FLAIL,	2,	TRUE, {
{"flail",		1,	"beating",	93,	52,	4},
{"morningstar",	1,	"sting",	98,	56,	9}
}},

{ "gun",		WEAPON_GUN,		8,	TRUE, {
{"six shooter",	1,	"shot",	95,	22,	9},
{"handgun",		1,	"bullet",	99,	19,	16},
{"laser",		1,	"laser",	100,	27,	30},
{"musket",		2,	"spray",	100,	74,	6},
{"rifle",		2,	"shot",	104,	102,	13},
{"shotgun",		2,	"spray",	106,	128,	16},
{"assault rifle",	2,	"hollowpt",	108,	87,	20},
{"missle launcher", 2,	"concussion",110,	234,	26}
}},

{ "mace",		WEAPON_MACE,	4,	TRUE, {
{"club",		1,	"smash",	90,	54,	1},
{"hammer",		1,	"pound",	97,	46,	3},
{"mace",		1,	"crush",	99,	76,	9},
{"bat",		2,	"thwack",	100,	72,	4}
}},

{ "polearm",	WEAPON_POLEARM,	3,	TRUE, {
{"halberd",		2,	"cleave",	110,	144,	15},
{"pike",		2,	"thrust",	108,	123,	12},
{"war hammer",	2,	"pound",	109,	158,	14}
}},

{ "spear/staff",		WEAPON_SPEAR,	2,	TRUE, {
{"spear",		1,	"thrust",	96,	86,	2},
{"quarterstaff",	2,	"beating",	98,	70,	2}
}},

{ "sword",		WEAPON_SWORD,	7,	TRUE, {
{"rapier", 		1,	"thrust",	92,	47,	4},
{"short sword",	1,	"slice",	95,	62,	6},
{"falchion",	1,	"chop",	96,	61,	8},
{"scimitar",	1,	"slash",	99,	90,	9},
{"long sword",	1,	"cleave",	100,	100,	10},
{"two-handed sword", 2,	"slice",	108,	180,	13},
{"claymore",	2,	"cleave",	110,	204,	16}
}},

{ "whip",		WEAPON_WHIP,	2,	FALSE, {
{"bullwhip",	1,	"whip",	93,	31,	3},
{"cat 'o nine tails",1,	"whip",	98,	42,	8}
}},

{NULL}
};


struct armor_subtype
{
	char *name;
	bool hard;
	int acmult;
	int size;	
	int cost;
	int bashmod;
	int slashmod;
	int piercemod;
};

struct armor_smith_type
{
	char *name;
	int type;
	bool two;
	int num_varieties;
	struct armor_subtype varieties[MAX_ARMOR_SUBTYPES];
};

const struct armor_smith_type armor_smith_table[]=
{

{"body armor",ITEM_WEAR_TORSO,FALSE,	12, {
{"shirt",			FALSE,	50,	40,	1,	0,	0,	0},
{"gown",			FALSE,	55,	55,	4,	0,	-1,	1},	
{"tunic",			FALSE,	64,	50,	2,	0,	0,	0},
{"vest",			FALSE,	72,	55,	3,	-1,	0,	-1},
{"robe",			FALSE,	76,	110,	4,	-1,	1,	0},
{"reinforced armor",	FALSE,	80,	95,	6,	-2,	1,	1},
{"scale mail",		TRUE,		82,	152,	4,	1,	-2,	1},				
{"chain mail",		TRUE,		88,	135,	8,	-2,	2,	0},
{"banded mail",		TRUE,		94,	234,	7,	2,	-1,	-1},
{"plate mail",		TRUE,		100,	202,	10,	0,	0,	0},
{"field plate armor",	TRUE,		105,	328,	14,	2,	0,	-2},	
{"full plate armor",	TRUE,		110,	527,	20,	2,	2,	-4}
}},

{ "shields",	ITEM_WEAR_SHIELD,	FALSE,	5, {
{"buckler",			TRUE,		70,	40,	1,	3,	-2,	-1},
{"small shield",		TRUE,		80,	60,	1,	1,	-2,	0},
{"round shield",		TRUE,		90,	95,	2,	0,	0,	0},
{"large shield",		TRUE,		100,	135,	3,	-1,	1,	0},
{"tower shield",		TRUE,		110,	260,	4,	-1,	0,	1}
}},

{ "head gear",	ITEM_WEAR_HEAD,	FALSE,	7, {
{"beanie",			FALSE,	30,	10,	1,	1,	0,	-1},
{"hat",			FALSE,	50,	18,	1,	-2,	1,	1},
{"cap",			FALSE,	75,	38,	2,	-1,	-1,	2},
{"light helm",		FALSE,	90,	66,	4,	-2,	0,	2},
{"chain coif",		TRUE,		92,	101,	5,	-1,	1,	0},
{"field helmet",		TRUE,		100,	146,	8,	0,	0,	0},
{"visored helm",		TRUE,		110,	212,	16,	2,	0,	-2}
}},

{ "armwear",	ITEM_WEAR_ARMS,	FALSE,	6, {
{"sleeves",			FALSE,	60,	31,	1,	-2,	1,	1},
{"reinforced sleeves",	FALSE,	75,	77,	3,	-1,	0,	1},
{"chain sleeves",		TRUE,		80,	168,	5,	-1,	1,	0},
{"mail sleeves",		TRUE,		90,	240,	6,	0,	0,	0},
{"armplates",		TRUE,		100,	298,	8,	1,	-1,	0},
{"arm greaves",		TRUE,		110,	389,	13,	2,	-1,	-1}
}},

{ "legwear",	ITEM_WEAR_LEGS,	FALSE,	8, {
{"tights",		FALSE,	50,	25,	1,	-1,	1,	0},
{"skirt",			FALSE,	55,	31,	1,	-3,	2,	1},
{"pants",			FALSE,	60,	36,	1,	-2,	1,	1},
{"reinforced leggings",	FALSE,	75,	83,	3,	-1,	0,	1},
{"chain leggings",	TRUE,		80,	175,	5,	-1,	1,	0},
{"mail leggings",		TRUE,		90,	280,	6,	0,	0,	0},
{"legplates",		TRUE,		100,	310,	8,	1,	-1,	0},
{"leg greaves",		TRUE,		110,	405,	13,	2,	-1,	-1}
}},

{ "gloves",		ITEM_WEAR_HANDS,	FALSE,	4, {
{"mittens",			FALSE,	50,	48,	1,	-3,	2,	1},
{"gloves",			FALSE,	60,	80,	1,	-2,	1,	1},
{"reinforced gloves",	FALSE,	80,	120,	4,	-1,	-1,	2},
{"gauntlets",		TRUE,		100,	148,	8,	3,	-1,	-2}
}},

{ "boots",		ITEM_WEAR_FEET,	FALSE,	4, {
{"sandals",			FALSE,	40,	50,	1,	-3,	2,	1},
{"shoes",			FALSE,	65,	80,	2,	-2,	1,	1},
{"hiking boots",		FALSE,	80,	112,	3,	-1,	-1,	2},
{"combat boots",		TRUE,		100,	198,	6,	3,	-1,	-2}
}},

{ "overgarments",	ITEM_WEAR_ABOUT,	FALSE,	4, {
{"cape",			FALSE,	50,	55,	1,	-1,	1,	0},
{"jacket",			FALSE,	75,	92,	2,	-1,	-1,	2},
{"trench coat",		FALSE,	85,	108,	3,	-1,	1,	0},
{"hooded cloak",		FALSE,	100,	143,	4,	-1,	1,	0}
}},

{ "waist protection",	ITEM_WEAR_WAIST,	FALSE,	5, {
{"boxer shorts",		FALSE,	25,	29,	1,	-2,	1,	1},
{"belt",			FALSE,	35,	45,	1,	-1,	2,	-1},
{"codpiece",		FALSE,	50,	69,	1,	0,	0,	0},
{"loincloth",		FALSE,	80,	100,	2,	-1,	1,	0},
{"girdle",			TRUE,		100,	125,	5,	2,	0,	-1}
}},

{ "neckwear",	ITEM_WEAR_NECK,	TRUE,		4, {
{"necklace",		FALSE,	40,	15,	1,	0,	0,	0},
{"neckguard",		FALSE,	60,	45,	2,	-1,	1,	0},
{"chain neckguard",	TRUE,		80,	75,	5,	-2,	0,	2},
{"aventail",		TRUE,		100,	134,	9,	1,	0,	-1}
}},

{ "wristwear",	ITEM_WEAR_WRIST,	TRUE,		3, {
{"bracelet",		TRUE,		50,	11,	1,	0,	0,	0},
{"bracer",			FALSE,	75,	37,	1,	-2,	1,	1},
{"chain wristguard",	TRUE,		100,	88,	3,	-1,	1,	0}
}},

{NULL}
};


struct blacksmith_material_type
{
	char *name;
	int	level;
	int	cost;
	int	weight;
	int	difficulty;
	int	strength;
	int	hitroll;
	int	damroll;
	int	hp;
	int	mana;
	int	move;
	int	quality;
	int	magic;
	bool	hard;
};

const struct blacksmith_material_type blacksmith_material_table[]=
{
{"styrofoam",1,	1,	5,	2,	2,	-8,	-8,	-20,	-20,	-20,	-25,	0,	TRUE},
{"wood",	1,	7,	20,	5,	5,	-2,	-3,	0,	0,	0,	-15,	5,	TRUE},
{"stone",	1,	18,	180,	90,	15,	-2,	-1,	0,	0,	0,	-10,	0,	TRUE},
{"bronze",	2,	60,	110,	85,	40,	-1,	-1,	0,	0,	0,	-2,	0,	TRUE},
{"iron",	5,	100,	100,	100,	50,	0,	0,	0,	0,	0,	0,	0,	TRUE},
{"aluminum",6,	125,	45,	80,	45,	1,	-2,	0,	0,	40,	-3,	0,	TRUE},
{"ivory",	8,	240,	160,	115,	45,	2,	3,	40,	50,	-5,	-5,	18,	TRUE},
{"steel",	10,	180,	90,	120,	65,	1,	1,	20,	0,	10,	2,	0,	TRUE},
{"obsidian",15,	200,	150,	160,	70,	0,	2,	25,	25,	25,	4,	5,	TRUE},
{"gold",	20,	10000,220,	90,	30,	-1,	-1,	25,	200,	-10,	-8,	25,	TRUE},
{"silver",	25,	1280,	120,	110,	40,	3,	0,	20,	100,	0,	-1,	15,	TRUE},
{"elvish steel",35,320,	70,	200,	75,	3,	2,	30,	40,	30,	5,	8,	TRUE},
{"platinum",45,	20000,80,	300,	85,	4,	3,	40,	25,	30,	7,	12,	TRUE},
{"diamond",	60,	50000,110,	1000,	90,	5,	6,	20,	50,	25,	8,	16,	TRUE},
{"mithril",	75,	80000,90,	1500,	93,	7,	5,	50,	50,	50,	9,	10,	TRUE},
{"titanium",90,	120000,60,	2000,	94,	7,	7,	100,	0,	50,	10,	0,	TRUE},
{"adamantite",91,	200000,120,	2500,	95,	8,	8,	100,	25,	50,	12,	10,	TRUE},
{"cloudstuff",92,	300000,10,	3000,	96,	12,	8,	50,	150,	100,	13,	25,	TRUE},
{"corag",	94,	600000,160,	3500,	97,	9,	12,	150,	100,	75,	15,	5,	TRUE},
{"lunite",	96,	900000,140,	5000,	98,	10,	16,	125,	125,	175,	18,	12,	TRUE},
{"heliocyte",99,	1600000,80,	8000,	99,	20,	15,	150,	150,	150,	21,	12,	TRUE},
{"neutronium",100,5000000,100,16000,100,	20,	20,	200,	200,	200,	25,	0,	TRUE},

{"tin foil",1,	2,	3,	1,	3,	-6,	-10,	-25,	-25,	0,	-25,	0,	FALSE},
{"polyester",1,	15,	18,	15,	20,	-5,	0,	0,	0,	-20,	-20,	0,	FALSE},
{"rawhide",	1,	32,	50,	30,	40,	-1,	-1,	10,	-10,	0,	-10,	3,	FALSE},
{"cotton",	2,	25,	20,	20,	30,	1,	0,	5,	0,	5,	-12,	0,	FALSE},
{"fur",	3,	75,	45,	50,	35,	0,	2,	10,	10,	10,	-9,	10,	FALSE},
{"suade",	5,	60,	40,	35,	45,	1,	1,	0,	5,	5,	-4,	8,	FALSE},
{"spandex",	8,	85,	35,	30,	75,	3,	0,	-5,	0,	25,	-8,	0,	FALSE},
{"leather",	10,	50,	50,	40,	50,	0,	0,	0,	0,	0,	0,	5,	FALSE},
{"silk",	15,	215,	8,	40,	15,	2,	1,	0,	50,	20,	-7,	20,	FALSE},
{"heavy leather",20,89,	60,	50,	60,	0,	2,	10,	0,	-5,	2,	5,	FALSE},
{"studded leather",25,115,70,	75,	65,	-1,	3,	25,	0,	0,	4,	3,	FALSE},
{"plastic",	30,	145,	55,	130,	70,	4,	0,	10,	15,	30,	5,	0,	FALSE},
{"flak",	40,	330,	100,	205,	70,	2,	4,	25,	10,	10,	8,	0,	FALSE},
{"kevlar",	60,	462,	80,	288,	85,	3,	3,	50,	30,	35,	10,	0,	FALSE},
{"wyvern hide",80,2900,	95,	370,	90,	3,	7,	80,	80,	40,	12,	10,	FALSE},
{"dragon scale",90,12270,90,	570,	95,	8,	8,	90,	100,	90,	15,	15,	FALSE},
{"biofilm",93,	68000,30,	820,	98,	15,	6,	100,	50,	200,	18,	0,	FALSE},
{"ether",	96,	210000,15,	1410,	99,	12,	18,	100,	200,	150,	21,	25,	FALSE},
{"nova essence",99,1000000,10,2000,	100,	20,	20,	200,	200,	200,	25,	20,	FALSE},

{NULL}
};


struct workmanship_type
{
	char *name;
	int costmult;
	int quality;
	int bonus;
	int weapon;
	tflag extra;
};

#define WF_SHARP I_BIT(WEAPON_SHARP)
#define WF_SHARP_VORPAL (I_BIT(WEAPON_SHARP)+I_BIT(WEAPON_VORPAL)) 

/*const*/ struct workmanship_type workmanship_table[]=
{
{"junk ",		5,	-40,	-10,	0,	{ITEM_ROT_DEATH,ITEM_MELT_DROP}},
{"flimsy ",		10,	-25,	-6,	0,	{ITEM_ROT_DEATH}},
{"crude ",		25,	-10,	-4,	0,	{}},
{"roughly hewn ",	50,	-5,	-2,	0,	{}},
{"utilitarian ",	90,	-2,	-1,	0,	{}},
{"standard ",   	100,	 0,	0,	0,	{ITEM_BURN_PROOF}},
{"well wrought ",	200,	 2,	1,	0,	{ITEM_BURN_PROOF}},
{"fine ",		450,	 5,	2,	0,	{ITEM_BLESS,ITEM_BURN_PROOF}},
{"exceptional ",	1000,	 8,	4,	WF_SHARP, {ITEM_BLESS,ITEM_NOPURGE,ITEM_BURN_PROOF}},
{"masterfully crafted ",2500,	12,	6,	WF_SHARP, {ITEM_GLOW,ITEM_BLESS,ITEM_NOPURGE,ITEM_BURN_PROOF}},
{"artifact quality ",	10000,  16,	8,	WF_SHARP_VORPAL, {ITEM_GLOW,ITEM_HUM,ITEM_BLESS,ITEM_NOPURGE,ITEM_MAGIC,ITEM_BURN_PROOF}},
{NULL}
};

struct dye_type
{
	char *precolor;
	char *display;
	char *name;
};

const struct dye_type dye_table[] =
{
	{"{r",	"{rred ", "red "},
	{"{R",	"{Rpink ", "pink "},
	{"{y",	"{ybrown ", "brown "},
	{"{Y",	"{Yyellow ", "yellow "},
	{"{g",	"{gforest green ", "forest "},
	{"{G",	"{Glime green ", "lime "},
	{"{c",	"{ccyan ", "cyan "},
	{"{C",	"{Csky blue ", "sky "},
	{"{b",	"{bnavy blue ", "navy "},
	{"{B",	"{Bbaby blue ", "baby "},				
	{"{m",	"{mviolet ", "violet "},
	{"{M",	"{Mmagenta ", "magenta "},
	{"{D",	"{Dgrey ", "grey "},
	{"{C",	"{Rr{Ya{Gi{Cn{Bb{Mo{Rw{C ", "rainbow "},
	{"{r",	"{rp{gl{ya{gi{yd{r ", "plaid "},
	{"{m",	"{Bp{Ca{Mi{Cs{Ml{Be{Cy{m ", "paisley "},
	{"{ ",	"{Ys{ t{Yr{ i{Yp{ e{Yd{  ", "striped "},
	{"{g",	"{gc{ya{Gm{go{Df{yl{Ga{gg{ye{g ", "camoflage "},
	{"{G",	"{Yglow {Gin the {Ydark{G ", "glow "},
	{NULL}
};

/* called in db.c */
void format_smithy_flags( void )
{
    int i;
    for ( i = 0; workmanship_table[i].name != NULL; i++ )
	bit_list_to_tflag( workmanship_table[i].extra );
}

int quality(int workmanship, int level)
{
	workmanship=workmanship_table[workmanship].bonus;
	workmanship=(workmanship*(level+30))/120;
	return workmanship;
}

int cost_by_level(int level)
{
	if (level > HERO) 
	    return 69;

	return (level*level);
}

int dam_by_level(int level)
{
	if (level > HERO) 
		return (25*(level - (HERO-2)));
	if (level >= (HERO-10)) 
		return (50 + 2 * (level - (HERO-10)));

	return ((23*(level-1))/44+3);
}

int ac_by_level(int level)
{
	if (level > HERO) 
		return (20*(level-(HERO-5)));
	if (level >= (HERO-10)) 
		return (4*(level-(HERO-26)));
	return ((7*(level-1))/11+1);
}

int durability(OBJ_DATA *obj)
{
	int strength=blacksmith_material_table[obj->value[3]].strength;
	int quality=2*workmanship_table[obj->value[2]].quality;

	if (quality>0)
		quality=(quality*(100-strength))/100+strength;
	else
		quality=(quality*strength)/100+strength;
	
	return quality;
}

int affmult(OBJ_DATA *obj)
{
	if (obj->weight==ITEM_WEAPON)
		return weapon_smith_table[obj->value[0]].varieties[obj->value[1]].dammult;
	else
		return armor_smith_table[obj->value[0]].varieties[obj->value[1]].acmult;
}

int weight(OBJ_DATA *obj)
{
	double w;

	if (obj->weight==ITEM_WEAPON)
		w=(double)(weapon_smith_table[obj->value[0]].varieties[obj->value[1]].size);
	else
		w=(double)(armor_smith_table[obj->value[0]].varieties[obj->value[1]].size);

	w*=(double)(blacksmith_material_table[obj->value[3]].weight);
	w/=100.0;

	return ((int) w);
}

long smith_cost(OBJ_DATA *obj, int material, CHAR_DATA *ch)
{
	double mcost=0, wcost=0;

	if (obj->weight==ITEM_WEAPON)
	{
		wcost=weapon_smith_table[obj->value[0]].varieties[obj->value[1]].cost;
		mcost=weapon_smith_table[obj->value[0]].varieties[obj->value[1]].size;
	}
	else
	{
		wcost=armor_smith_table[obj->value[0]].varieties[obj->value[1]].cost;
		mcost=armor_smith_table[obj->value[0]].varieties[obj->value[1]].size;
	}

	wcost*=cost_by_level(obj->level);
	wcost*=blacksmith_material_table[material].difficulty;
	wcost*=workmanship_table[obj->value[2]].costmult;
	wcost/=10000;

	mcost*=blacksmith_material_table[material].cost;	

	if (obj->value[4]>-1) wcost+= 2*obj->level;

	wcost+=mcost;
	wcost/=2;
	obj->cost=(long)(wcost);
	
	wcost*=1000- get_skill(ch, gsn_haggle);
	wcost/=900;		

	return ((long)wcost);	
}

char *smith_name (OBJ_DATA *obj, char *name)
{
	char buf[MAX_STRING_LENGTH];

	if (obj->short_descr!=NULL) return obj->short_descr;

	if (obj->value[4]>-1)
		sprintf(name, "%s", dye_table[obj->value[4]].precolor);
	else
		sprintf(name, "{ ");

	if (obj->owner!=NULL)
	{
		sprintf(buf, "%s's ", obj->owner);
		if (buf[0]>='a'&&buf[0]<='z')
			buf[0]+='A'-'a';
		strcat(name, buf);
	}

	if (str_cmp(workmanship_table[obj->value[2]].name, "standard "))
		strcat(name,  workmanship_table[obj->value[2]].name);


	if (obj->value[4]>-1)
		strcat(name, dye_table[obj->value[4]].display);

	strcat(name, blacksmith_material_table[obj->value[3]].name);
	strcat(name, " ");

	if (obj->weight==ITEM_WEAPON)
		strcat(name, weapon_smith_table[obj->value[0]].varieties[obj->value[1]].name);
	else
		strcat(name, armor_smith_table[obj->value[0]].varieties[obj->value[1]].name);

	strcat(name, "{ ");
	
	return name;
}

char *smith_keyword (OBJ_DATA *obj, char *name)
{
	char buf[MAX_STRING_LENGTH];

	if (obj->name!=NULL) return obj->name;

	if (obj->owner!=NULL)
	{
		sprintf(buf, "%ss ", obj->owner);
	}
	else
		buf[0]= '\0';

	if (str_cmp(workmanship_table[obj->value[2]].name, "standard "))
		sprintf(name, "%s%s", buf,  workmanship_table[obj->value[2]].name);
	else
		sprintf(name, "%s", buf);

	if (obj->value[4]>-1)
		strcat(name, dye_table[obj->value[4]].name);

	strcat(name, blacksmith_material_table[obj->value[3]].name);
	strcat(name, " ");

	if (obj->weight==ITEM_WEAPON)
		strcat(name, weapon_smith_table[obj->value[0]].varieties[obj->value[1]].name);
	else
		strcat(name, armor_smith_table[obj->value[0]].varieties[obj->value[1]].name);

	strcat(name, " blacksmith");
	
	return name;
}

void smith_show_weapon (CHAR_DATA *ch, OBJ_DATA *obj)
{
	char buf[MAX_STRING_LENGTH];

	double dam=weapon_smith_table[obj->value[0]].varieties[obj->value[1]].dammult;

	dam*=100+blacksmith_material_table[obj->value[3]].quality;
	dam*=100+workmanship_table[obj->value[2]].quality;
	dam*=dam_by_level(obj->level);
	dam/=1000000.0;

	sprintf(buf, "Ave dam: %d,  %s damage\n\r", (int)dam,
		weapon_smith_table[obj->value[0]].varieties[obj->value[1]].damtype);
	send_to_char(buf, ch);   
}

void smith_show_armor (CHAR_DATA *ch, OBJ_DATA *obj)
{
	char buf[MAX_STRING_LENGTH];

	double ac=armor_smith_table[obj->value[0]].varieties[obj->value[1]].acmult;
	double pierce=armor_smith_table[obj->value[0]].varieties[obj->value[1]].piercemod;
	double bash=armor_smith_table[obj->value[0]].varieties[obj->value[1]].bashmod;
	double slash=armor_smith_table[obj->value[0]].varieties[obj->value[1]].slashmod;
	double exotic=blacksmith_material_table[obj->value[3]].magic;
	int aclev=ac_by_level(obj->level);

	ac*=100+blacksmith_material_table[obj->value[3]].quality;
	ac*=100+workmanship_table[obj->value[2]].quality;
	ac*=aclev;
	ac/=40000.0;

	exotic = (75.0+exotic)*ac;
	exotic /= 100;

	pierce=((20+pierce)*ac)/20;
	bash=((20+bash)*ac)/20;
	slash=((20+slash)*ac)/20;

	sprintf(buf, "Pierce: %d  Bash: %d  Slash: %d  Magic: %d\n\r",
		(int)(pierce/100), (int)(bash/100), (int)(slash/100), (int)(exotic/100));
	send_to_char(buf, ch);
}

void smith_show_obj (CHAR_DATA *ch, OBJ_DATA *obj)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	double funds;
	int mult=affmult(obj);

	sprintf(buf, "\n\rYou are buying a %s.\n\r", smith_name(obj, buf2));
	send_to_char(buf, ch);

	if (obj->weight==ITEM_WEAPON)
		smith_show_weapon(ch, obj);
	else
		smith_show_armor(ch, obj);

	sprintf(buf, "Hitbonus: %d  Dambonus: %d  HPbonus: %d  Manabonus: %d  Movebonus: %d\n\r",
		(mult*(blacksmith_material_table[obj->value[3]].hitroll+ quality(obj->value[2], obj->level)))/100,
		(mult*(blacksmith_material_table[obj->value[3]].damroll+ quality(obj->value[2], obj->level)))/100,
		(mult*blacksmith_material_table[obj->value[3]].hp)/100,
		(mult*blacksmith_material_table[obj->value[3]].mana)/100,
		(mult*blacksmith_material_table[obj->value[3]].move)/100);

	send_to_char(buf, ch);

	sprintf(buf, "Durability: %d%%    Weight: %3.1f\n\r",
		durability(obj), ((float)weight(obj))/10.0);
	send_to_char(buf, ch);

	funds = 100.0*((double)ch->pcdata->bank) + 100.0*((double)ch->gold) + (double)ch->silver;

	sprintf(buf, "Total cost: %ld silver     Available funds: %14.0f silver\n\r\n\r",
		smith_cost(obj, obj->value[3], ch),
		funds);

	send_to_char(buf, ch);
}

void affectify(OBJ_DATA *obj)
{
	AFFECT_DATA *paf; 
	int mult = affmult(obj);
	int hit=blacksmith_material_table[obj->value[3]].hitroll+ quality(obj->value[2], obj->level);
	int dam=blacksmith_material_table[obj->value[3]].damroll+ quality(obj->value[2], obj->level);
	int hp=blacksmith_material_table[obj->value[3]].hp;
	int mana=blacksmith_material_table[obj->value[3]].mana;
	int move=blacksmith_material_table[obj->value[3]].move;
	hit = (hit*mult)/100;
	dam = (dam*mult)/100;
	hp = (hp*mult)/100;
	mana = (mana*mult)/100;
	move = (move*mult)/100;

	obj->affected=NULL;

	if (hit!=0)
	{
	paf = new_affect();
	paf->where  = TO_OBJECT;
	paf->type   = 0;
	paf->level  = obj->level;
	paf->duration   = -1;
	paf->location   = APPLY_HITROLL;
	paf->modifier   = hit;
	paf->bitvector  = 0;
	paf->next   = obj->affected;
	obj->affected   = paf;
	}
	
	if (dam!=0)
	{
	paf = new_affect();
	paf->where  = TO_OBJECT;
	paf->type   = 0;
	paf->level  = obj->level;
	paf->duration   = -1;
	paf->location   = APPLY_DAMROLL;
	paf->modifier   = dam;
	paf->bitvector  = 0;
	paf->next   = obj->affected;
	obj->affected   = paf;
	}

	if (hp!=0)
	{
	paf = new_affect();
	paf->where  = TO_OBJECT;
	paf->type   = 0;
	paf->level  = obj->level;
	paf->duration   = -1;
	paf->location   = APPLY_HIT;
	paf->modifier   = hp;
	paf->bitvector  = 0;
	paf->next   = obj->affected;
	obj->affected   = paf;
	}

	if (mana!=0)
	{
	paf = new_affect();
	paf->where  = TO_OBJECT;
	paf->type   = 0;
	paf->level  = obj->level;
	paf->duration   = -1;
	paf->location   = APPLY_MANA;
	paf->modifier   = mana;
	paf->bitvector  = 0;
	paf->next   = obj->affected;
	obj->affected   = paf;
	}

	if (move!=0)
	{
	paf = new_affect();
	paf->where  = TO_OBJECT;
	paf->type   = 0;
	paf->level  = obj->level;
	paf->duration   = -1;
	paf->location   = APPLY_MOVE;
	paf->modifier   = move;
	paf->bitvector  = 0;
	paf->next   = obj->affected;
	obj->affected   = paf;
	}
}

void smith_finish_weapon(OBJ_DATA *obj)
{
	struct weapon_subtype weapon=weapon_smith_table[obj->value[0]].varieties[obj->value[1]];
	struct blacksmith_material_type material=blacksmith_material_table[obj->value[3]];
	struct workmanship_type quality=workmanship_table[obj->value[2]];

	double dam=weapon.dammult;

	dam*=100+material.quality;
	dam*=100+quality.quality;
	dam*=dam_by_level(obj->level);
	dam/=1000000.0;

	obj->value[0]=weapon_smith_table[obj->value[0]].type;
	obj->value[1]=2;
	obj->value[2]=(int)dam-1;
	obj->value[3]=attack_lookup(weapon.damtype);
	obj->value[4]=quality.weapon;
	if (weapon.hands==2)
	    I_SET_BIT(obj->value[4], WEAPON_TWO_HANDS);
}

void smith_finish_armor(OBJ_DATA *obj)
{
	struct armor_subtype armor=armor_smith_table[obj->value[0]].varieties[obj->value[1]];
	struct blacksmith_material_type material=blacksmith_material_table[obj->value[3]];
	struct workmanship_type quality=workmanship_table[obj->value[2]];
	int aclev=ac_by_level(obj->level);

	double ac=armor.acmult;
	double pierce=armor.piercemod;
	double bash=armor.bashmod;
	double slash=armor.slashmod;
	double exotic=material.magic;

	ac*=100+material.quality;
	ac*=100+quality.quality;
	ac*=aclev;
	ac/=40000.0;

	exotic = (75.0+exotic)*ac;
	exotic /= 100;

	pierce=((20+pierce)*ac)/20;
	bash=((20+bash)*ac)/20;
	slash=((20+slash)*ac)/20;

	SET_BIT( obj->wear_flags, armor_smith_table[obj->value[0]].type );

	obj->value[0]=(int)(pierce/100);
	obj->value[1]=(int)(bash/100);
	obj->value[2]=(int)(slash/100);
	obj->value[3]=(int)(exotic/100);
}

bool smith_finish_obj (CHAR_DATA *ch, OBJ_DATA *obj)
{
	double funds = 100.0*((double)ch->pcdata->bank) + 100.0*((double)ch->gold) + (double)ch->silver;
	int cost = smith_cost(obj, obj->value[3], ch);
	int change;
	char buf[MAX_STRING_LENGTH];

	if ( cost < 0 )
	{
	    send_to_char( "A negative cost looks buggy. Please report to an imm.\n\r", ch );
	    return FALSE;
	}

	if (funds<((double)cost))
	{
		send_to_char("I'm sorry, you cant afford it.  Why dont you hit Dicey Nite's?\n\r",ch);
		return FALSE;
	}
	
	change = cost%100;
	cost-=change;
	cost/=100;

	if (ch->silver>=change)
		ch->silver-=change;
	else
	{
		ch->silver+=100-change;
		cost++;
	}

	if (cost>ch->gold)
	{
		cost-=ch->gold;
		ch->gold=0;
		ch->pcdata->bank-=cost;
		send_to_char("You use your Bastion Select Ultra Premium Mithril Card to pay for your purchase.\n\r", ch);
	}
	else
		ch->gold-=cost;

	affectify(obj);

	if (obj->owner!=NULL)
		ch->pcdata->questpoints-=400;

	if (obj->short_descr!=NULL)
	{
		ch->pcdata->questpoints-=250;
	}
	else
	{
		smith_name(obj, buf);
		obj->short_descr=str_dup(buf);
		smith_keyword(obj, buf);
		obj->name=str_dup(buf);
	}

	sprintf(buf, "A %s has been left here.", obj->short_descr);
	obj->description=str_dup(buf);

	obj->item_type=obj->weight;
	obj->weight=weight(obj);
	obj->durability=durability(obj);
	obj->material=str_dup(blacksmith_material_table[obj->value[3]].name);

	flag_clear( obj->extra_flags );
	if ( obj->owner != NULL )
	    SET_BIT( obj->extra_flags, ITEM_STICKY );

	flag_set_field( obj->extra_flags, workmanship_table[obj->value[2]].extra );
	if (!(blacksmith_material_table[obj->value[3]].hard))
	    SET_BIT( obj->extra_flags, ITEM_NONMETAL );

	if (obj->item_type==ITEM_WEAPON)
		smith_finish_weapon(obj);
	else
		smith_finish_armor(obj);

	return TRUE;
}

void smith_sticky_obj (CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (ch->pcdata->questpoints<400||(obj->short_descr!=NULL&&ch->pcdata->questpoints<650))
	{
		send_to_char("\n\rYou dont have enough quest points.\n\r",ch);
		return;
	}

	obj->owner = str_dup(ch->name);
}

bool smith_welcome (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj;
	int n;

	if (d==NULL||IS_NPC(d->character)) return FALSE;

	if (argument[0]=='\0')
	{
		send_to_char("\n\r\n\rWelcome to my humble shop, ",ch);
		send_to_char(ch->name, ch);
		send_to_char("!\n\r\n\r", ch);

		set_creation_state(d, CREATION_BLACKSMITH);
		set_con_state(d, CON_SMITH_WELCOME);

		send_to_char("Would you like to browse our list of:\n\r",ch);
		send_to_char("1) Weapons, or\n\r2) Armor\n\r\n\r",ch);

		send_to_char("Or perhaps you would like to:\n\r",ch);
		send_to_char("3) sell or repair something in your inventory\n\r\n\r\n\r",ch);
	}
	else if (is_number(argument))
	{
		n=atoi(argument);
		if (n<1 || n>3)
			send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
		else
		{
			if (n==3)
				return smith_select(d, "");		

			if (ch->pcdata->smith==NULL)
			{
				obj = create_object( get_obj_index( OBJ_VNUM_BLACKSMITH ), 0 );
				ch->pcdata->smith=obj;
				obj->value[4]=-1;
				obj->short_descr=NULL;
				obj->owner=NULL;
			}

			if (n==2)
				ch->pcdata->smith->weight=ITEM_ARMOR;
			else if (n==1)
			{
			    ch->pcdata->smith->weight=ITEM_WEAPON;
			    flag_clear( ch->pcdata->smith->wear_flags );
			    SET_BIT( ch->pcdata->smith->wear_flags, ITEM_WIELD );
			    SET_BIT( ch->pcdata->smith->wear_flags, ITEM_TAKE );
			}

			return TRUE;
		}		
	}
	else if (!str_cmp(argument, "bye"))
	{
		if (ch->pcdata->smith!=NULL) extract_obj(ch->pcdata->smith);
		ch->pcdata->smith=NULL;
		d->connected=CON_PLAYING;
		send_to_char("\n\rGoodbye, ",ch);
		send_to_char(ch->name, ch);
		send_to_char(".  Come again soon!\n\r\n\r",ch);
		return FALSE;
	}
	else
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);


	send_to_char("\n\rEnter a #, or bye> ", ch);
	return FALSE;
}

bool smith_level (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;
	int n;

	if (argument[0]=='\0')
	{
		set_con_state(d, CON_SMITH_LEVEL);

		send_to_char("\n\r\n\r\n\rWhat level would you like the item to be for (1-110)?\n\r", ch);
	}
	else if (is_number(argument))
	{
		n=atoi(argument);

		if (n>110||n<1)
		{
			send_to_char("\n\rI dont think you need a level ",ch);
			send_to_char(argument, ch);
			send_to_char(" item.\n\r", ch);
		}
		else
		{
			obj->level=n;
			return TRUE;
		}
	}
	else if (!str_cmp(argument, "bye"))
	{
		extract_obj(obj);
		ch->pcdata->smith=NULL;
		d->connected=CON_PLAYING;
		send_to_char("\n\rGoodbye, ",ch);
		send_to_char(ch->name, ch);
		send_to_char(".  Come again soon!\n\r\n\r",ch);
		return FALSE;
	}
	else if (!str_cmp(argument, "restart"))
	{
		return smith_welcome(d, "");
	}
	else
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);

	send_to_char("\n\rEnter a #, restart, or bye> ", ch);
	return FALSE;
}

bool smith_type (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;
	int n, i;
	char buf[MAX_STRING_LENGTH];

	if (argument[0]=='\0')
	{
		set_con_state(d, CON_SMITH_TYPE);

		if (obj->weight==ITEM_WEAPON)
		{
			send_to_char("\n\rThe following categories of weapons are available:\n\r",ch);
			for(i=0; weapon_smith_table[i].name!=NULL; i++)
			{
				sprintf(buf, "%2d)  %s\n\r", i+1, weapon_smith_table[i].name);
				send_to_char(buf, ch);
			}			 
		}
		else
		{
			send_to_char("\n\rThe following categories of armor are available:\n\r",ch);
			for(i=0; armor_smith_table[i].name!=NULL; i++)
			{
				sprintf(buf, "%2d)  %s\n\r", i+1, armor_smith_table[i].name);
				send_to_char(buf, ch);
			}			 
		}
	}
	else if (is_number(argument))
	{
		n=atoi(argument);

		if (obj->weight==ITEM_WEAPON)
		{
			for(i=0; weapon_smith_table[i].name!=NULL; i++);
			if (n>i||n<1)
				send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
			else
			{
				obj->value[0]=n-1;
				return TRUE;		
			}
		}
		else
		{
			for(i=0; armor_smith_table[i].name!=NULL; i++);
			if (n>i||n<1)
				send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
			else
			{
			    flag_clear( obj->wear_flags );
			    SET_BIT( obj->wear_flags, ITEM_TAKE );
			    SET_BIT( obj->wear_flags, armor_smith_table[i].type );
			    obj->value[0]=n-1;
			    return TRUE;		
			}
		}
	}
	else if (!str_cmp(argument, "bye"))
	{
		extract_obj(obj);
		ch->pcdata->smith=NULL;
		d->connected=CON_PLAYING;
		send_to_char("\n\rGoodbye, ",ch);
		send_to_char(ch->name, ch);
		send_to_char(".  Come again soon!\n\r\n\r",ch);
		return FALSE;
	}
	else if (!str_cmp(argument, "backup"))
	{
		return smith_level(d, "");
	}
	else if (!str_cmp(argument, "restart"))
	{
		return smith_welcome(d, "");
	}
	else
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);

	send_to_char("\n\rEnter a #, backup, restart, or bye> ", ch);
	return FALSE;
}

bool smith_subtype (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;
	int n, i;
	char buf[MAX_STRING_LENGTH];

	if (argument[0]=='\0')
	{
		set_con_state(d, CON_SMITH_SUBTYPE);

		if (obj->weight==ITEM_WEAPON)
		{
			sprintf(buf, "The following types of %s are availaible:\n\r",
				weapon_smith_table[obj->value[0]].name);
			send_to_char(buf, ch);
			send_to_char("<#> <================> <hands> <=damage type=> <dam %> <bulk> <base cost>\n\r", ch);

			for(i=0; i<weapon_smith_table[obj->value[0]].num_varieties; i++)
			{
				sprintf(buf, "%2d) %18s    %d    %14s    %3d%%   %3d     %d\n\r", i+1,
					weapon_smith_table[obj->value[0]].varieties[i].name,
					weapon_smith_table[obj->value[0]].varieties[i].hands,
					weapon_smith_table[obj->value[0]].varieties[i].damtype,
					weapon_smith_table[obj->value[0]].varieties[i].dammult,
					weapon_smith_table[obj->value[0]].varieties[i].size,
					weapon_smith_table[obj->value[0]].varieties[i].cost);
				send_to_char(buf, ch);
			}			 
		}
		else
		{
			sprintf(buf, "The following types of %s are availaible:\n\r",
				armor_smith_table[obj->value[0]].name);
			send_to_char(buf, ch);
			send_to_char("<#> <==================> <coverage> <bulk> <base cost>\n\r",ch);

			for(i=0; i<armor_smith_table[obj->value[0]].num_varieties; i++)
			{
				sprintf(buf, "%2d) %19s     %3d%%     %3d      %d\n\r", i+1,
					armor_smith_table[obj->value[0]].varieties[i].name,
					armor_smith_table[obj->value[0]].varieties[i].acmult,
					armor_smith_table[obj->value[0]].varieties[i].size,
					armor_smith_table[obj->value[0]].varieties[i].cost);

				send_to_char(buf, ch);
			}			 
		}
	}
	else if (is_number(argument))
	{
		n=atoi(argument);

		if (obj->weight==ITEM_WEAPON)
		{
			if (n>weapon_smith_table[obj->value[0]].num_varieties||n<1)
				send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
			else
			{
				obj->value[1]=n-1;
				return TRUE;		
			}
		}
		else
		{
			if (n>armor_smith_table[obj->value[0]].num_varieties||n<1)
				send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
			else
			{
				obj->value[1]=n-1;
				return TRUE;		
			}
		}
	}
	else if (!str_cmp(argument, "bye"))
	{
		extract_obj(obj);
		ch->pcdata->smith=NULL;
		d->connected=CON_PLAYING;
		send_to_char("\n\rGoodbye, ",ch);
		send_to_char(ch->name, ch);
		send_to_char(".  Come again soon!\n\r\n\r",ch);
		return FALSE;
	}
	else if (!str_cmp(argument, "backup"))
	{
		return smith_type(d, "");
	}
	else if (!str_cmp(argument, "restart"))
	{
		return smith_welcome(d, "");
	}
	else
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);

	send_to_char("\n\rEnter a #, backup, restart, or bye> ", ch);
	return FALSE;
}

bool smith_quality (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;
	int n, i;
	char buf[MAX_STRING_LENGTH];

	if (argument[0]=='\0')
	{
		set_con_state(d, CON_SMITH_QUALITY);

		send_to_char("\n\rYou may select from the following levels of craftsmanship:\n\r",ch);
		send_to_char("<#> <===================> <cost> <quality bonus> <hitdam bonus>\n\r", ch);
		for(i=0; workmanship_table[i].name!=NULL; i++)
		{
			sprintf(buf, "%2d) %20s  %5d%%       %3d%%           %2d\n\r", i+1,
				workmanship_table[i].name,
				workmanship_table[i].costmult,
				workmanship_table[i].quality,
				quality(i, obj->level));
			send_to_char(buf, ch);
		}			 
	}
	else if (is_number(argument))
	{
		n=atoi(argument);

		for(i=0; workmanship_table[i].name!=NULL; i++);
		if (n>i||n<1)
			send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
		else
		{
			obj->value[2]=n-1;
			return TRUE;		
		}

	}
	else if (!str_cmp(argument, "bye"))
	{
		extract_obj(obj);
		ch->pcdata->smith=NULL;
		d->connected=CON_PLAYING;
		send_to_char("\n\rGoodbye, ",ch);
		send_to_char(ch->name, ch);
		send_to_char(".  Come again soon!\n\r\n\r",ch);
		return FALSE;
	}
	else if (!str_cmp(argument, "backup"))
	{
		return smith_subtype(d, "");
	}
	else if (!str_cmp(argument, "restart"))
	{
		return smith_welcome(d, "");
	}
	else
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);

	send_to_char("\n\rEnter a #, backup, restart, or bye> ", ch);
	return FALSE;
}

bool smith_material (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;
	int n, i, j;
	bool hard=FALSE;
	char buf[MAX_STRING_LENGTH];

	if (argument[0]=='\0')
	{
		set_con_state(d, CON_SMITH_MATERIAL);

		if (((obj->weight==ITEM_WEAPON)&&
			(weapon_smith_table[obj->value[0]].hard))
			|| ((obj->weight==ITEM_ARMOR)&&
			(armor_smith_table[obj->value[0]].varieties[obj->value[1]].hard)))
				hard=TRUE;
		j=0;

		for (i=0; blacksmith_material_table[i].name!=NULL; i++)
			if ((blacksmith_material_table[i].hard==hard)&&
				(obj->level>=blacksmith_material_table[i].level))
		{
			j++;
			sprintf(buf, "%2d) %20s %10ld silver\n\r", j,
				blacksmith_material_table[i].name, smith_cost(obj, i, ch));
			send_to_char(buf, ch);
		}			
	}
	else if (is_number(argument))
	{
		n=atoi(argument);
		j=0;

		if (((obj->weight==ITEM_WEAPON)&&
			(weapon_smith_table[obj->value[0]].hard))
			|| ((obj->weight==ITEM_ARMOR)&&
			(armor_smith_table[obj->value[0]].varieties[obj->value[1]].hard)))
				hard=TRUE;

		for (i=0; blacksmith_material_table[i].name!=NULL; i++)
			if ((blacksmith_material_table[i].hard==hard)&&
				(obj->level>=blacksmith_material_table[i].level))
		{
			j++;
			if (j==n)
			{
				obj->value[3]=i;
				return TRUE;
			}		
		}	
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
	}
	else if (!str_cmp(argument, "bye"))
	{
		extract_obj(obj);
		ch->pcdata->smith=NULL;
		d->connected=CON_PLAYING;
		send_to_char("\n\rGoodbye, ",ch);
		send_to_char(ch->name, ch);
		send_to_char(".  Come again soon!\n\r\n\r",ch);
		return FALSE;
	}
	else if (!str_cmp(argument, "backup"))
	{
		return smith_quality(d, "");
	}
	else if (!str_cmp(argument, "restart"))
	{
		return smith_welcome(d, "");
	}
	else
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);

	send_to_char("\n\rEnter a #, backup, restart, or bye> ", ch);
	return FALSE;
}

bool smith_purchase (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;
	int n;
	char buf[MAX_STRING_LENGTH];

	if (argument[0]=='\0')
	{
		set_con_state(d, CON_SMITH_PURCHASE);

		send_to_char("\n\r",ch);
		smith_show_obj(ch, obj);

		if ((obj->value[4]==-1)&&(obj->short_descr==NULL))
		{
			sprintf(buf, "1)  Dye this item a custom color (%d silver).\n\r", 2*obj->level);
			send_to_char(buf, ch);
			send_to_char("2)  Rename the item (250 quest points).\n\r", ch);
		}
		if (obj->owner==NULL)
			send_to_char("3)  Make this item sticky to you (400 quest points).\n\r", ch);
		send_to_char("4)  Buy this item as is.\n\r* option 1 and 2 are mutually exclusive\n\r\n\r", ch);

	}
	else if (is_number(argument))
	{
		n=atoi(argument);

		if (n>4||n<1)
			send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
		else
		{
			if (n==4) 
			{
				if (smith_finish_obj(ch, obj))
				{
					sprintf(buf, "\n\rEnjoy your %s, %s!\n\r\n\r",
						obj->short_descr, ch->name);
					send_to_char(buf, ch);
					obj_to_char(ch->pcdata->smith, ch);
					ch->pcdata->smith=NULL;
					return TRUE;
				}
			}
			else if (n==3)
			{
				if (obj->owner==NULL)
				{
					smith_sticky_obj(ch, obj);
					return smith_purchase(d, "");
				}
				else
					send_to_char("\n\rYou've already made it sticky.\n\r", ch);
			}
			else if ((obj->value[4]!=-1)||(obj->short_descr!=NULL))
			{
				send_to_char("\n\rYou've already customized it.\n\r", ch);
			}
			else if (n==2)
				return smith_personal(d, "");
			else return smith_color(d, "");	
		}
	}
	else if (!str_cmp(argument, "bye"))
	{
		extract_obj(obj);
		ch->pcdata->smith=NULL;
		d->connected=CON_PLAYING;
		send_to_char("\n\rGoodbye, ",ch);
		send_to_char(ch->name, ch);
		send_to_char(".  Come again soon!\n\r\n\r",ch);
		return FALSE;
	}
	else if (!str_cmp(argument, "backup"))
	{
		obj->value[4]=-1;
		free_string(obj->short_descr);
		free_string(obj->name);
		free_string(obj->owner);
		obj->name=NULL;
		obj->short_descr=NULL;
		obj->owner=NULL;
		return smith_material(d, "");
	}
	else if (!str_cmp(argument, "restart"))
	{
		obj->value[4]=-1;
		free_string(obj->short_descr);
		free_string(obj->name);
		free_string(obj->owner);
		obj->name=NULL;
		obj->short_descr=NULL;
		obj->owner=NULL;
		return smith_welcome(d, "");
	}
	else
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);

	send_to_char("\n\rEnter a #, backup, restart, or bye> ", ch);
	return FALSE;
}

bool smith_color (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;
	int n, i;
	char buf[MAX_STRING_LENGTH];

	if (argument[0]=='\0')
	{
		set_con_state(d, CON_SMITH_COLOR);

		send_to_char("\n\rYou may select from one of the following patinas:\n\r",ch);
		for(i=0; dye_table[i].display!=NULL; i++)
		{
			sprintf(buf, "%2d)  %s{ \n\r", i+1, dye_table[i].display);
			send_to_char(buf, ch);
		}			 
	}
	else if (is_number(argument))
	{
		n=atoi(argument);

		for(i=0; dye_table[i].display!=NULL; i++);
		if (n>i||n<1)
			send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);
		else
		{
			obj->value[4]=n-1;
			return TRUE;		
		}
	}
	else if (!str_cmp(argument, "bye"))
	{
		extract_obj(obj);
		ch->pcdata->smith=NULL;
		d->connected=CON_PLAYING;
		send_to_char("\n\rGoodbye, ",ch);
		send_to_char(ch->name, ch);
		send_to_char(".  Come again soon!\n\r\n\r",ch);
		return FALSE;
	}
	else if (!str_cmp(argument, "backup"))
	{
		return smith_purchase(d, "");
	}
	else if (!str_cmp(argument, "restart"))
	{
		obj->value[4]=-1;
		free_string(obj->short_descr);
		free_string(obj->name);
		free_string(obj->owner);
		obj->name=NULL;
		obj->short_descr=NULL;
		obj->owner=NULL;
		return smith_welcome(d, "");
	}
	else
		send_to_char("\n\rI'm sorry, I dont understand.\n\r", ch);

	send_to_char("\n\rEnter a #, backup, restart, or bye> ", ch);
	return FALSE;
}

bool smith_personal (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;

	if (argument[0]=='\0')
	{
		if (ch->pcdata->questpoints<250||(obj->owner!=NULL&&ch->pcdata->questpoints<650))
		{
			send_to_char("\n\rYou dont have enough quest points.\n\r",ch);
			return smith_purchase(d, "");
		}
		set_con_state(d, CON_SMITH_PERSONAL);
		send_to_char("\n\rPlease type in the short description of your item as you would like it to appear in your inventory:\n\r",ch);
	}
	else
	{
		obj->short_descr=str_dup(argument);
		return TRUE;
	}
	return FALSE;
}

bool smith_keywords (DESCRIPTOR_DATA *d, char *argument)
{
	CHAR_DATA *ch=d->character;
	OBJ_DATA *obj=ch->pcdata->smith;

	if (argument[0]=='\0')
	{
		set_con_state(d, CON_SMITH_KEYWORDS);
		send_to_char("\n\rPlease type in a few keywords that you will use to manipulate the item:\n\r",ch);
	}
	else
	{
		obj->name=str_dup(argument);
		return TRUE;
	}
	return FALSE;
}

bool smith_select (DESCRIPTOR_DATA *d, char *argument)
{
	send_to_char("This option is not yet functional.\n\r", d->character);
	d->connected=CON_PLAYING;
	return FALSE;
}

bool smith_inventory (DESCRIPTOR_DATA *d, char *argument)
{

	return FALSE;
}

