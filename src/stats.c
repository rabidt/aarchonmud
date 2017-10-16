#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

void take_default_stats args((CHAR_DATA *ch));
void get_random_stats args((CHAR_DATA *ch));
void roll_dice args((CHAR_DATA *ch, bool take_default));
void do_help args((CHAR_DATA *ch, char *argument));
struct race_type* get_morph_race_type( CHAR_DATA *ch );
void show_pc_race_ratings( CHAR_DATA *ch, int race );

// structure for storing stat values prior to finalizing them
typedef struct min_max_rolled MIN_MAX_ROLLED;
struct min_max_rolled
{
    int min, max, rolled;
};

/* command for retrieving stats */
int get_curr_stat( CHAR_DATA *ch, int stat )
{
    int bonus;
    const int step = 5;
    
    if (stat == STAT_NONE)
        return 100;
    
    bonus = ch->mod_stat[stat];
    
    if (bonus > step)
    {
        int i = step, j = step;
        
        while (j < bonus)
        {
            i += step;
            j += i;
        }
        
        bonus = i - step + ((bonus + i - j) * step) / i;
    }
    
    if ( !IS_NPC(ch) && MULTI_MORPH(ch) && (ch->pcdata->morph_race > 0) )
    {
        int ch_class_bonus = class_bonus( ch->clss, stat );
        /* adjust base stat for new race */
        struct pc_race_type *new_race_type = &pc_race_table[ch->pcdata->morph_race];
        int new_max = new_race_type->max_stats[stat];
        int new_base;
        if ( IS_SET(race_table[ch->pcdata->morph_race].form, FORM_CONSTRUCT) )
        {
            new_base = new_max + ch_class_bonus;
        }
        else
        {
            int org_min = pc_race_table[ch->race].min_stats[stat];
            int org_max = pc_race_table[ch->race].max_stats[stat];
            int new_min = new_race_type->min_stats[stat];
            int org_remort_bonus = (ch->pcdata->remorts - pc_race_table[ch->race].remorts) *
                pc_race_table[ch->race].remort_bonus[stat];
            int stat_roll = ch->perm_stat[stat] - ch_class_bonus - org_remort_bonus - org_min;
            new_base = new_min
                + (new_max - new_min) * stat_roll / (org_max - org_min)
                + ch_class_bonus;
        }
        /* remort bonus */
        int remort_bonus = (morph_power(ch) - new_race_type->remorts) *
            new_race_type->remort_bonus[stat];
        /* sum it up */
        bonus += new_base - ch->perm_stat[stat] + remort_bonus;
    }
    else if (!IS_NPC(ch) && ch->race == race_naga && ch->pcdata->morph_race != 0)
    {
	if ((stat==STAT_CHA) || (stat==STAT_DEX) || (stat==STAT_VIT))
	    bonus += 10;
	else if ((stat==STAT_STR) || (stat==STAT_AGI) || (stat==STAT_CON))
	    bonus -= 10;
    }
    else if (ch->race == race_werewolf)
    {
	int weremod;

        if (weather_info.sunlight == SUN_DARK)
            weremod = 10;
        else if (weather_info.sunlight == SUN_RISE)
            weremod = -10;
        else
            weremod = 0;
        
        if (stat > 4)
            weremod *= -1;
        
        bonus += weremod;
    }
    
    return URANGE(1,ch->perm_stat[stat] + bonus, MAX_CURRSTAT);
}

int dice_lookup(char *stat)
{
    int i;

    for (i=0; stat_table[i].name!=NULL; i++)
        if (!str_prefix(stat, stat_table[i].name))
            break;

    return i;
}

int stat_lookup(char *stat)
{
    int i;

    for (i=0; i<MAX_STATS; i++)
        if (!str_prefix(stat, stat_table[i].name))
            break;

    return -1;
}

int remort_bonus (CHAR_DATA *ch, int stat )
{
    int r;
    
    if (ch->pcdata->remorts ==0) return 0;
    
    r=ch->pcdata->remorts - pc_race_table[ch->race].remorts;

    return r * pc_race_table[ch->race].remort_bonus[stat];
}

int class_bonus( int class, int stat )
{
    if (class_table[class].attr_prime == stat)
        return 10;
    
    if (class_table[class].attr_second[0] == stat
        || class_table[class].attr_second[1] == stat)
        return 5;
    
    return 0;
}

/* command for returning max training score */
int get_max_train( CHAR_DATA *ch, int stat )
{
    int max;
    
    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
        return MAX_CURRSTAT;
    
    max = pc_race_table[ch->race].max_stats[stat];
    max += class_bonus(ch->clss, stat);
    max += remort_bonus(ch, stat);
    
    if (!IS_SET(race_table[ch->race].form, FORM_CONSTRUCT))
    {
        max += ch->pcdata->original_stats[stat];
        max /= 2;
    }
    
    return UMIN(max,MAX_CURRSTAT);
}

int dex_app_tohit(int x)
{
    if ( x > 60 )
	return ((x-60)/8);
    else
	return 0;
};

int str_app_todam(int x)
{
    if ( x > 60 )
	return ((x-60)/6);
    else
	return 0;
};

int str_app_carry(int x)
{
    if (x>100) return ((x-90)*((x*(x+40))/500));
    return ((x*(x+40))/50);
};

int str_app_wield(int x)
{
    if (x<60) return (x*2);
    if (x<90) return (x*3-60);
    if (x<120) return (x*5-240);
    return (x*8-600);
};

int int_app_learn(int x)
{
    return (x+20);
};

int dis_app_practice(int x)
{
    /*return (x/16);*/
    int base = x/16;
    if ( number_range(0, 15) < (x % 16) )
	return base + 1;
    else
	return base;
};

int agi_app_defensive(int x)
{
    if ( x < 60 )
	return 0;
    else
	return -2 * (x-60);
};

int dex_app_extrahit(int x)
{
    return x / 6;
};

int con_app_shock(int x)
{
    return (100 - ((160-x)*(160-x))/256);
};

int con_app_hitp(int x)
{
    return ((x-50)/8);
};

int cha_app_aggro(int x)
{
    return ((x-60)/5);
}

int int_app_field(int x)
{
    return (x/3+2);
}

int wis_app_field(int x)
{
    return (75-(5*x)/16);
}

int dis_app_field(int x)
{
    return (x*10);  
}

int ch_dex_tohit(CHAR_DATA *ch)
{
    int dex = get_curr_stat(ch, STAT_DEX);
    if ( dex < 60 )
        return 0;
    else
        return (modified_level(ch) + 10) * (dex-60) / 200;
};

int ch_str_todam(CHAR_DATA *ch)
{
    int str = get_curr_stat(ch, STAT_STR);
    if ( str < 60 )
        return 0;
    else
        return (modified_level(ch) + 10) * (str-60) / 100;
};

int ch_str_carry(CHAR_DATA *ch)
{
    return (str_app_carry(get_curr_stat(ch, STAT_STR)));
};

int ch_str_wield(CHAR_DATA *ch)
{
    return(str_app_wield(get_curr_stat(ch, STAT_STR)));
};

int ch_int_learn(CHAR_DATA *ch)
{
    return (int_app_learn(get_curr_stat(ch, STAT_INT)));
};

/* Leaving this in tact, although it should not be called
   by any code any longer. Practices are now calculated by
   primary and secondary stats instead of just DIS - Astark 1-2-13 */

int ch_dis_practice(CHAR_DATA *ch)
{
    return (dis_app_practice(get_curr_stat(ch, STAT_DIS)));
};


/* New function for pracitce calculations. Uses prime and secondary
   stats to determine the gains - Astark 1-2-13 */

int ch_prac_gains(CHAR_DATA *ch, int for_level)
{
    int x1 = 100*get_curr_stat(ch,class_table[ch->clss].attr_prime)/30;
    int x2 = 100*get_curr_stat(ch,class_table[ch->clss].attr_second[0])/45;
    int x3 = 100*get_curr_stat(ch,class_table[ch->clss].attr_second[1])/45;
    int x = x1+x2+x3;
    return x * (1 + UMAX(0, for_level - LEVEL_MIN_HERO));
};

int ch_agi_defensive(CHAR_DATA *ch)
{
    return (modified_level(ch) + 10) * agi_app_defensive(get_curr_stat(ch, STAT_AGI)) / 100;
};

int ch_dex_extrahit(CHAR_DATA *ch)
{
    return (dex_app_extrahit(get_curr_stat(ch, STAT_DEX)));
};

int ch_con_shock(CHAR_DATA *ch)
{
    return (con_app_shock(get_curr_stat(ch, STAT_CON)));
};

int ch_con_hitp(CHAR_DATA *ch)
{
    return (con_app_hitp(get_curr_stat(ch, STAT_CON)));
};

int ch_cha_aggro(CHAR_DATA *ch)
{
    return ( cha_app_aggro(get_curr_stat(ch, STAT_CHA)));
}

int ch_int_field(CHAR_DATA *ch)
{
    return ( int_app_field(get_curr_stat(ch, STAT_INT)));
}

int ch_wis_field(CHAR_DATA *ch)
{
    return ( wis_app_field(get_curr_stat(ch, STAT_WIS)));
}

int ch_dis_field(CHAR_DATA *ch)
{
    return ( dis_app_field(get_curr_stat(ch, STAT_DIS)));
}

int ch_luc_quest(CHAR_DATA *ch)
{
    return (get_curr_stat(ch, STAT_LUC)/6+2);
}

int get_ac( CHAR_DATA *ch )
{
    int ac = ch->armor;
    int defense_factor = 100;
    int level = modified_level(ch);
    
    if ( IS_AWAKE(ch) )
        ac += ch_agi_defensive(ch);
    else
        ac += 50;

    ac -= get_curr_stat(ch, STAT_LUC) / 2;

    if ( IS_SET(ch->parts, PART_SCALES) )
        ac -= level / 2;
    if ( IS_SET(ch->form, FORM_ARMORED) )
        ac -= level * 10;
    
    // level-based bonus
    if ( IS_NPC(ch) )
    {
        if (IS_SET(ch->act, ACT_WARRIOR))
            defense_factor += 20;
        if (IS_SET(ch->act, ACT_MAGE))
            defense_factor -= 20;
    }
    else
    {
        defense_factor = class_table[ch->clss].defense_factor;
    }
    ac -= (level + 10) * defense_factor/20;
        
    return ac;
}

int get_hitroll( CHAR_DATA *ch )
{
    int hitroll = ch->hitroll + ch_dex_tohit(ch) + get_curr_stat(ch, STAT_LUC) / 10;
    int attack_factor = 100;
    
    // level bonus
    if ( IS_NPC(ch) )
    {
        if (IS_SET(ch->act,ACT_WARRIOR))
            attack_factor += 20;
        if (IS_SET(ch->act,ACT_MAGE))
            attack_factor -= 20;
    }
    else
    {
        attack_factor = class_table[ch->clss].attack_factor;
    }
    hitroll += (modified_level(ch) + 10) * attack_factor/100;

    hitroll = hitroll * (400 - get_heavy_armor_penalty(ch)) / 400;
    
    return hitroll;
}

int get_damroll( CHAR_DATA *ch )
{
    int damroll = ch->damroll
	+ ch_str_todam(ch)
	+ get_curr_stat(ch, STAT_LUC) / 20;
    return damroll;
}

int get_spell_penetration( CHAR_DATA *ch, int level )
{
    if ( ch )
    {
        float sp = (level + 10) * (400 + get_curr_stat(ch, STAT_INT)) / 500.0;
        // higher attack factor means stronger focus on physical attacks
        sp *= (500 - class_table[ch->clss].attack_factor) / 400.0;
        // hitroll from items / buffs helps as well
        sp += ch->hitroll / 4.0;
        // bonus for using focus object
        int focus = get_obj_focus(ch) + get_dagger_focus(ch);
        sp += sp * UMIN(focus, 100) / 500.0;
        // inquisition
        if ( ch && ch->stance == STANCE_INQUISITION )
        {
            sp += sp / 3;
            sp += sp * get_skill_overflow(ch, gsn_inquisition) / 2000;
        }
        return sp;
    }
    else
        return (level + 10) * 6/5;
}

int max_hmm_train( int level )
{
  int hero_bonus = UMAX(0, level - (LEVEL_HERO - 10));
  int max_trained = (level + hero_bonus * (hero_bonus + 1) / 2) * 2;
  return max_trained;
}

bool train_stat(int trained, CHAR_DATA *ch)
{
  return trained < max_hmm_train( ch->level );
}

int stat_gain(CHAR_DATA *ch, int stat)
{
    int rem;
    
    stat = ch->perm_stat[stat];
    rem = stat%20;
    stat = (stat-rem)/20;
    
    rem = (5*rem>number_percent()) ? 1 : 0;
    
    return (stat+rem+5);
}

/* returns how much one train would raise given stat */
int train_stat_inc( CHAR_DATA *ch, int stat )
{
    int max, inc;

    max = get_max_train(ch, stat) - ch->perm_stat[stat];

    if ( max <= 0 )
	return 0;

    if ( IS_SET(race_table[ch->race].form, FORM_CONSTRUCT) )
    {
        return UMIN(3, max);
    }
    else
    {
        if ( max > 45 )
            inc = 5;
        else if ( max > 21 )
            inc = 4;
        else if ( max > 9 )
            inc = 3;
        else if ( max > 3 )
            inc = 2;
        else
            inc = 1;

        return UMIN(1 + ch->perm_stat[stat]/15, inc);
    }
}

int construct_train_cost( int from, int to )
{
    int total = 0;
    for ( ; from < to; from++ )
    {
        int base = UMAX(from/10 - 7, 0);
        int cost = 10 << base;
        total += cost;
    }
    return total;
}

void show_can_train( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int stat, inc;
    
    strcpy( buf, "You can train:" );
    for ( stat = 0; stat < MAX_STATS; stat++ )
        if ( (inc = train_stat_inc(ch, stat)) > 0 )
        {
            sprintf( buf2, " %s(+%d)", stat_table[stat].abbreviation, inc );
            strcat( buf, buf2); 
        }
        
    if ( train_stat(ch->pcdata->trained_hit, ch) )
        strcat(buf, " hp");
    if ( train_stat(ch->pcdata->trained_move, ch) )
        strcat(buf, " move");
    if ( train_stat(ch->pcdata->trained_mana, ch) )
        strcat(buf, " mana");
    
    if ( buf[strlen(buf)-1] != ':' )
    {
        strcat( buf, ".\n\r" );
        send_to_char( buf, ch );
    }
    else
    {
        act( "You have nothing left to train, you $T!", ch, NULL,
            ch->sex == SEX_MALE   ? "big stud" :
            ch->sex == SEX_FEMALE ? "hot babe" :
            "wild thing",
            TO_CHAR );
    }
}

DEF_DO_FUN(do_train)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    sh_int stat = -1;
    int cost, max, inc;
    
    if ( IS_NPC(ch) )
        return;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        sprintf( buf, "You have %d training sessions.\n\r", ch->train );
        send_to_char( buf, ch );
        show_can_train(ch);
        return;
    }
    
    bool introspect = TRUE;
    CHAR_DATA *trainer = find_trainer(ch, ACT_TRAIN, &introspect);
    if ( !trainer && !introspect )
        return;
    
    if ( !str_cmp(arg, "hp") || !str_cmp(arg, "mana") || !str_cmp(arg, "move") )
    {
        // second argument lets you increase hp/mana/move by multiple points at once
        if ( (inc = atoi(argument)) < 1 )
            inc = 1;
        cost = inc;
        max = max_hmm_train(ch->level);
        
        if ( cost > ch->train )
        {
            send_to_char("You don't have enough training sessions.\n\r", ch);
            return;
        }
    
        if ( !str_cmp("hp", arg) )
        {
            if (ch->pcdata->trained_hit + inc > max )
            {
                send_to_char("You can't train that many hps you freak.\n\r", ch);
                return;
            }
            ch->pcdata->trained_hit += inc;
            ch->train -= cost;
            sprintf( buf, "Your durability increases! [%d %s spent].\n\r", cost, cost > 1 ? "trains" : "train");
            send_to_char( buf, ch );
            act("$n's durability increases!", ch, NULL, NULL, TO_ROOM);
        }
        else if ( !str_cmp("mana", arg) )
        {
            if (ch->pcdata->trained_mana + inc > max )
            {
                send_to_char("You can't train that much mana you freak.\n\r", ch);
                return;
            }
            ch->pcdata->trained_mana += inc;
            ch->train -= cost;
            sprintf( buf, "Your power increases! [%d %s spent].\n\r", cost, cost > 1 ? "trains" : "train");
            send_to_char( buf, ch );
            act("$n's power increases!", ch, NULL, NULL, TO_ROOM);
        }
        else if ( !str_cmp("move", arg) )
        {
            if (ch->pcdata->trained_move + inc > max )
            {
                send_to_char("You can't train that many moves you freak.\n\r", ch);
                return;
            }
            ch->pcdata->trained_move += inc;
            ch->train -= cost;
            sprintf( buf, "Your stamina increases! [%d %s spent].\n\r", cost, cost > 1 ? "trains" : "train");
            send_to_char( buf, ch );
            act("$n's stamina increases!", ch, NULL, NULL, TO_ROOM);
        }
        update_perm_hp_mana_move(ch);
        return;
    }
    
    // check for valid argument & find stat
    for ( stat = 0; stat < MAX_STATS; stat++ )
        if ( !str_prefix(arg, stat_table[stat].name) )
            break;
    
    if ( stat == MAX_STATS )
    {
        send_to_char("Syntax: train <str|...|luc>\n\r", ch);
        send_to_char("        train <hp|mana|move> [amount]\n\r", ch);
        return;
    }
    else
        cost = 1;
    
    if ( ch->perm_stat[stat]  >= (max=get_max_train(ch,stat)) )
    {
        act( "Your $T is already at maximum.", ch, NULL, stat_table[stat].name, TO_CHAR );
        return;
    }
    
    if ( cost > ch->train )
    {
        send_to_char( "You don't have enough training sessions.\n\r", ch );
        return;
    }
    
    if ( IS_SET(race_table[ch->race].form, FORM_CONSTRUCT) )
    {
        cost = construct_train_cost(ch->perm_stat[stat], ch->perm_stat[stat] + train_stat_inc(ch, stat));
        if (cost > ch->gold)
        {
            sprintf(buf, "You need %d gold to upgrade your %s.\n\r",
                cost, stat_table[stat].name);
            send_to_char(buf, ch);
            return;
        }
        ch->train -= 1;
        sprintf(buf, "You upgrade your %s systems for %d gold.\n\r",
            stat_table[stat].name, cost);
        send_to_char(buf, ch);
        ch->gold -= cost;
    } else {
        ch->train -= cost;
    }
    
    ch->perm_stat[stat] += train_stat_inc(ch, stat);
    update_perm_hp_mana_move(ch);
    act( "Your $T increases!", ch, NULL, stat_table[stat].name, TO_CHAR );
    act( "$n's $T increases!", ch, NULL, stat_table[stat].name, TO_ROOM );
}

void compute_mob_stats(CHAR_DATA *mob)
{
    sh_int base, i;
    
    /*
    if (mob->level<90) 
	base = 12+(2*mob->level)/3;
    else
	base = 2*mob->level-108;
    */
    base = 25 + mob->level * 3/4;

    for (i = 0; i < MAX_STATS; i ++)
        mob->perm_stat[i] = base + number_range(-5,5);
    
    if (IS_SET(mob->act,ACT_WARRIOR))
    {
        mob->perm_stat[STAT_STR] += 10;
        mob->perm_stat[STAT_CON] += 5;
        mob->perm_stat[STAT_INT] -= 15;
    }
    if (IS_SET(mob->act,ACT_THIEF))
    {
	mob->perm_stat[STAT_DEX] += 10;
	mob->perm_stat[STAT_AGI] += 5;
	mob->perm_stat[STAT_WIS] -= 10;
	mob->perm_stat[STAT_CHA] -= 5;
    }
    if (IS_SET(mob->act,ACT_CLERIC))
    {
	mob->perm_stat[STAT_WIS] += 10;
	mob->perm_stat[STAT_CHA] += 5;
	mob->perm_stat[STAT_DEX] -= 10;
	mob->perm_stat[STAT_AGI] -= 5;
    }
    if (IS_SET(mob->act,ACT_MAGE))
    {
	mob->perm_stat[STAT_INT] += 15;
	mob->perm_stat[STAT_STR] -= 10;
	mob->perm_stat[STAT_CON] -= 5;
    }
         
    mob->perm_stat[STAT_STR] += (mob->size - SIZE_MEDIUM) * 5;
    mob->perm_stat[STAT_AGI] -= (mob->size - SIZE_MEDIUM) * 5;
                
    for (i = 0; i < MAX_STATS; i ++)
	mob->perm_stat[i] = URANGE(1,mob->perm_stat[i],MAX_CURRSTAT);
                
    return;
}

DEF_DO_FUN(do_stats)
{
    char buf[MAX_STRING_LENGTH];
    int i, j, race;
    BUFFER *output;
    int tier = -1;

    if (argument[0] == '\0')
    {
        output = new_buf();
        for (i=1; i<MAX_PC_RACE; i++)
        {
	    if (pc_race_table[i].remorts > tier)          
	    {
		tier = pc_race_table[i].remorts;
		sprintf(buf,    "\n\r\n\r++++++++++++++++++++++++++++++++ {CRemort %2d{x +++++++++++++++++++++++++++++++++\n\r",tier);
		add_buf(output, buf);
		add_buf(output, "={WRace{x= ={WStr{x== ={WCon{x== ={WVit{x== ={WAgi{x== ={WDex{x==");
            	add_buf(output, " ={WInt{x== ={WWis{x== ={WDis{x== ={WCha{x== ={WLuc{x==");
	    }


 
	    sprintf(buf, "\n\r{D%6s", pc_race_table[i].who_name);
            add_buf(output,buf);
            
            for (j=0; j<MAX_STATS; j++)
            {
                sprintf(buf, " {%c%2d-%3d", (j%2)?'c':'y', pc_race_table[i].min_stats[j],
                pc_race_table[i].max_stats[j]);
                add_buf(output,buf);
            }
            add_buf(output, "{x");
        }
        add_buf(output,"\n\r\n\rTo get more information, type: HELP STATS\n\r");
        
        page_to_char(buf_string(output),ch);
        free_buf(output);
    }
    else if ( argument[0] == '<' && argument[1] == ' '
    && (argument[2] == 'r' && argument[3] >= '0' && argument[3] <= '9') )
    {
	int r_num;

	if( argument[3] == '1' && argument[4] == '0' )
	    r_num = 10;
	else if( argument[4] == '\0' )
	    r_num = argument[3] - '0';
	else
	{
	    send_to_char( "Invalid remort number.\n\r", ch );
	    return;
	}

	output = new_buf();

	for( i=1; i<MAX_PC_RACE; i++ )
	{
	    if( pc_race_table[i].remorts > r_num )
             break;

            if (pc_race_table[i].remorts > tier)
            {
                tier = pc_race_table[i].remorts;
                sprintf(buf,    "\n\r\n\r++++++++++++++++++++++++++++++++ {CRemort %2d{x +++++++++++++++++++++++++++++++++\n\r",tier);
                add_buf(output, buf);
                add_buf(output, "={WRace{x= ={WStr{x== ={WCon{x== ={WVit{x== ={WAgi{x== ={WDex{x==");
                add_buf(output, " ={WInt{x== ={WWis{x== ={WDis{x== ={WCha{x== ={WLuc{x==");
            }


            sprintf(buf, "\n\r{D%6s", pc_race_table[i].who_name);
            add_buf(output,buf);
            
            for (j=0; j<MAX_STATS; j++)
            {
                sprintf(buf, " {%c%2d-%3d",(j%2)?'c':'y', pc_race_table[i].min_stats[j], pc_race_table[i].max_stats[j]);
                add_buf(output,buf);
		add_buf(output, "{x");
            }
	}	
        add_buf(output,"\n\r\n\rTo get more information, type: HELP STATS\n\r");
        
        page_to_char(buf_string(output),ch);
        free_buf(output);
    }
    else if ( argument[0] == 'r' && argument[1] >= '0' && argument[1] <= '9' )
    {
	int r_num;

	if( argument[1] == '1' && argument[2] == '0' )
	    r_num = 10;
	else if( argument[2] == '\0' )
	    r_num = argument[1] - '0';
	else
	{
	    send_to_char( "Invalid remort number.\n\r", ch );
	    return;
	}


	output = new_buf();
        sprintf(buf,    "\n\r\n\r++++++++++++++++++++++++++++++++ {CRemort %3d{x ++++++++++++++++++++++++++++++++\n\r", r_num);
        add_buf(output, buf);
        add_buf(output, "={WRace{x= ={WStr{x== ={WCon{x== ={WVit{x== ={WAgi{x== ={WDex{x==");
        add_buf(output, " ={WInt{x== ={WWis{x== ={WDis{x== ={WCha{x== ={WLuc{x==");


	for( i=1; i<MAX_PC_RACE; i++ )
	{
	    if( pc_race_table[i].remorts != r_num )
		continue;
            sprintf(buf, "\n\r{D%6s", pc_race_table[i].who_name);
            add_buf(output,buf);
            
            for (j=0; j<MAX_STATS; j++)
            {
                sprintf(buf, " {%c%2d-%3d",(j%2)?'c':'y', pc_race_table[i].min_stats[j],
                    pc_race_table[i].max_stats[j]);
                add_buf(output,buf);
		add_buf(output,"{x");
            }
	}	
        add_buf(output,"\n\r\n\rTo get more information, type: HELP STATS\n\r");
        
        page_to_char(buf_string(output),ch);
        free_buf(output);
    }
    else if (((race=race_lookup(argument)) == 0) || (!race_table[race].pc_race))
        send_to_char("That is not a valid race.\n\r", ch);
    else
    {
        send_to_char("={WStr{x== ={WCon{x== ={WVit{x== ={WAgi{x== ={WDex{x==", ch);
        send_to_char(" ={WInt{x== ={WWis{x== ={WDis{x== ={WCha{x== ={WLuc{x==\n\r", ch);
        
        for (j=0; j<MAX_STATS; j++)
        {
            sprintf(buf, "{%c%2d-%3d ", (j%2)?'c':'y', pc_race_table[race].min_stats[j],
                pc_race_table[race].max_stats[j]);
            send_to_char(buf, ch);
        }
        send_to_char("{x\n\r", ch);
    }
}

/* called by do_showrace */
void show_remort_bonus( CHAR_DATA *ch, int race )
{
    char buf[MAX_STRING_LENGTH];
    int i;
        
    for ( i = 0; i < MAX_STATS; i++)
    {
	sprintf( buf, "  +%d   ", pc_race_table[race].remort_bonus[i] );
	send_to_char( buf, ch );
    }
    send_to_char( "\n\r", ch );
}

DEF_DO_FUN(do_etls)
{
    char buf[MAX_STRING_LENGTH];
    int i, j, race;
    BUFFER *output;
    int tier = -1;
    
    if (argument[0] == '\0')
    {
        output = new_buf();

        add_buf(output,"\n\rThis table shows the number of experience points required to gain one level.");
        add_buf(output,"\n\rTo get more information, type: HELP ETLS");
        
        for (i=1; i<MAX_PC_RACE; i++)
        {
            if (pc_race_table[i].remorts > tier)          
            {
                tier = pc_race_table[i].remorts;
                sprintf(buf,    "\n\r\n\r+++++++++++++++++++++++++++++++++++++ {CRemort %2d{x ++++++++++++++++++++++++++++++++++++++\n\r",tier);
                add_buf(output, buf);
                add_buf(output, "{WRace{x    {yWar  {cThi  {yCle  {cMag  {yGla  {cSam  {yPal  {cAsn");
                add_buf(output, "  {yNin  {cMon  {yTem  {cIlu  {yGun  {cRan  {yNec{x  {cBar{x");
            }

	    sprintf(buf, "\n\r{D%6s", pc_race_table[i].who_name);
            add_buf(output,buf);

            for (j=0; j<MAX_CLASS; j++)
            {
                sprintf(buf, " {%c%3d0{x",(j%2)?'c':'y', pc_race_table[i].class_mult[j]);
                add_buf(output,buf);
            }

            add_buf(output, "{x");

        }
        
        page_to_char(buf_string(output),ch);
        free_buf(output);
    }
    else if ( argument[0] == '<' && argument[1] == ' '
    && (argument[2] == 'r' && argument[3] >= '0' && argument[3] <= '9') )
    {
        int r_num;
            
        if( argument[3] == '1' && argument[4] == '0' )
            r_num = 10;
        else if( argument[4] == '\0' ) 
            r_num = argument[3] - '0';
        else
        {
            send_to_char( "Invalid remort number.\n\r", ch );
            return;
        }
        
        output = new_buf();

        for( i=1; i<MAX_PC_RACE; i++ )
        {
            if( pc_race_table[i].remorts > r_num )
                break;

            if (pc_race_table[i].remorts > tier)
            {
                tier = pc_race_table[i].remorts;
                sprintf(buf,    "\n\r\n\r+++++++++++++++++++++++++++++++++++++ {CRemort %2d{x ++++++++++++++++++++++++++++++++++++++\n\r",tier);
                add_buf(output, buf);
                add_buf(output, "{WRace{x    {yWar  {cThi  {yCle  {cMag  {yGla  {cSam  {yPal  {cAsn");
                add_buf(output, "  {yNin  {cMon  {yTem  {cIlu  {yGun  {cRan  {yNec{x  {cBar{x");
            }
        
            sprintf(buf, "\n\r{D%6s", pc_race_table[i].who_name);
            add_buf(output,buf);

            for (j=0; j<MAX_CLASS; j++)
            {
                sprintf(buf, " {%c%3d0{x",(j%2)?'c':'y', pc_race_table[i].class_mult[j]);
                add_buf(output,buf);
            }
        }
        add_buf(output,"\n\r\n\rTo get more information, type: HELP ETLS\n\r");
    
        page_to_char(buf_string(output),ch);
        free_buf(output);
    }       
    else if ( argument[0] == 'r' && argument[1] >= '0' && argument[1] <= '9' )
    {
        int r_num;
            
        if( argument[1] == '1' && argument[2] == '0' )
            r_num = 10;
        else if( argument[2] == '\0' )
            r_num = argument[1] - '0';
        else
        {
            send_to_char( "Invalid remort number.\n\r", ch );
            return;
        }
        
        output = new_buf();
        sprintf(buf,    "\n\r\n\r+++++++++++++++++++++++++++++++++++++ {CRemort %2d{x ++++++++++++++++++++++++++++++++++++++\n\r", r_num);
        add_buf(output, buf);
        add_buf(output, "{WRace{x    {yWar  {cThi  {yCle  {cMag  {yGla  {cSam  {yPal  {cAsn");
        add_buf(output, "  {yNin  {cMon  {yTem  {cIlu  {yGun  {cRan  {yNec{x  {cBar{x");
         
        
        for( i=1; i<MAX_PC_RACE; i++ )
        {
            if( pc_race_table[i].remorts != r_num )
                continue;
            sprintf(buf, "\n\r{D%6s", pc_race_table[i].who_name);
            add_buf(output,buf);


            for (j=0; j<MAX_CLASS; j++)
            {
                sprintf(buf, " {%c%3d0{x",(j%2)?'c':'y', pc_race_table[i].class_mult[j]);
                add_buf(output,buf);
            }
        }   
        add_buf(output,"\n\r\n\rTo get more information, type: HELP ETLS\n\r");

        page_to_char(buf_string(output),ch);
        free_buf(output);
    }
    else if (((race=race_lookup(argument)) == 0) || (!race_table[race].pc_race))
        send_to_char("That is not a valid race.\n\r", ch);
    else
    {
        for (i=0; i<MAX_CLASS; i++)
        {
            sprintf(buf, " %3s ", class_table[i].who_name);
            send_to_char(buf, ch);
        }
        send_to_char("\n\r", ch);
        
        for (j=0; j<MAX_CLASS; j++)
        {
            sprintf(buf, "%3d0 ", pc_race_table[race].class_mult[j]);
            send_to_char(buf, ch);
        }
        send_to_char("\n\r", ch);
    }
}


DECLARE_DO_FUN( do_raceskills);
DEF_DO_FUN(do_showrace)
{
    char buf[MAX_STRING_LENGTH];
    int race, part;
    tflag combat_parts, special_forms;
    
    if ((argument[0] == '\0') && race_table[ch->race].pc_race)
    {
        race = ch->race;
        argument = race_table[race].name;
    }
    else if (((race=race_lookup(argument)) == 0) || (!race_table[race].pc_race))
    {
        send_to_char("Syntax: showrace <pc race>\n\r", ch);
        return;
    }
    
    sprintf(buf, "Race: %s      Remort: %d      Size: %s      Gender: %s\n\r",
        pc_race_table[race].name,
        pc_race_table[race].remorts,
        size_table[pc_race_table[race].size].name,
        sex_table[pc_race_table[race].gender].name);
    send_to_char(buf, ch);
    
    sprintf(buf, "Vulnerabilities: %s\n\r", imm_bits_name(race_table[race].vuln));
    send_to_char(buf, ch);
    
    sprintf(buf, "Resistances: %s\n\r", imm_bits_name(race_table[race].res));
    send_to_char(buf, ch);
    
    if ( !flag_is_empty(race_table[race].imm) )
    {
        sprintf(buf, "Immunities: %s\n\r", imm_bits_name(race_table[race].imm));
        send_to_char(buf, ch);
    }
    
    if ( !flag_is_empty(race_table[race].affect_field) )
    {
        sprintf(buf, "Affected by: %s\n\r", affect_bits_name(race_table[race].affect_field));
        send_to_char(buf, ch);
    }
    
    /* show combat-related body parts */
    flag_clear( combat_parts );
    for ( part = PART_CLAWS; part <= PART_TUSKS; part++ )
	if ( IS_SET(race_table[race].parts, part) )
	    SET_BIT( combat_parts, part );
    if ( !flag_is_empty(combat_parts) )
    {
        sprintf(buf, "Natural Weaponry: %s\n\r", flag_bits_name(part_flags, combat_parts) );
        send_to_char(buf, ch);
    }    

    /* show special forms */
    flag_clear( special_forms );
#define SFORM(f) if (IS_SET(race_table[race].form,f)) SET_BIT(special_forms,f)
    SFORM( FORM_UNDEAD );
    SFORM( FORM_AGILE );
    SFORM( FORM_CONSTRUCT );
    SFORM( FORM_BRIGHT );
    SFORM( FORM_TOUGH );
    SFORM( FORM_PLANT );
    SFORM( FORM_SUNBURN );
    SFORM( FORM_DOUBLE_JOINTED );
    SFORM( FORM_FROST );
    SFORM( FORM_WISE );
    SFORM( FORM_BURN );
    SFORM( FORM_CONDUCTIVE );
    SFORM( FORM_CONSTRICT );
    SFORM( FORM_MULTI_HEADED );
    SFORM( FORM_ARMORED );
    SFORM( FORM_PESTILENT );
#undef SFORM

    if ( !flag_is_empty(special_forms) )
    {
        sprintf(buf, "Specialty: %s\n\r",
		flag_bits_name(form_flags, special_forms) );
        send_to_char(buf, ch);
    }    

    send_to_char("\n\r", ch);
    do_stats(ch, argument);
    show_remort_bonus(ch, race);

    if ( IS_IMMORTAL( ch ) )
	show_pc_race_ratings( ch, race );
    
    send_to_char("\n\r", ch);
    do_etls(ch, argument);
    send_to_char("\n\r", ch);
    do_raceskills(ch, argument);
}

void show_pc_race_ratings( CHAR_DATA *ch, int race )
{
    char buf[MIL];
    int stat;
    int remorts = pc_race_table[race].remorts;
    bool construct = IS_SET(race_table[race].form, FORM_CONSTRUCT);
    
    int allowed_max = ((construct ? 90 : 100) + 5*remorts) * 10;
    int allowed_min = (20 + 5*remorts) * 10;
    int trade_factor = (remorts == 0 ? 3 : 5);
    int allowed_rating = allowed_min + allowed_max * trade_factor;
    
    int min_sum = 0;
    int max_sum = 0;

    /* for each stat */
    for ( stat = 0; stat < MAX_STATS; stat++ )
    {
        min_sum += pc_race_table[race].min_stats[stat];
        max_sum += pc_race_table[race].max_stats[stat];
    }
    /* and the total */
    if (construct) {
        char min_color = min_sum > allowed_min ? 'r' : min_sum < allowed_min ? 'g' : 'x';
        char max_color = max_sum > allowed_max ? 'r' : max_sum < allowed_max ? 'g' : 'x';
        sprintf( buf, "\n\rStat Rating: Min = {%c%d{x/%d, Max = {%c%d{x/%d{x\n\r", min_color, min_sum, allowed_min, max_color, max_sum, allowed_max );
    } else {
        int rating = min_sum + max_sum * trade_factor;
        char rating_color = rating > allowed_rating ? 'r' : rating < allowed_rating ? 'g' : 'x';
        sprintf( buf, "\n\rStat Rating: {%c%d{x/%d\n\r", rating_color, rating, allowed_rating );
    }
    send_to_char( buf, ch );        
}

DEF_DO_FUN(do_racelist)
{
    char buf[MAX_STRING_LENGTH];
    int i, j=0, tier = -1;
    
    for (i=1; race_table[i].name && race_table[i].pc_race; i++)
    {
        if (pc_race_table[i].remorts > tier)
        {
            tier = pc_race_table[i].remorts;
            sprintf(buf, "%s              === Remort Tier #%d ===\n\r",
                (tier>0) ? "\n\r\n\r" : "", tier);
            send_to_char(buf, ch);
            j=0;
        }
        else if (++j > 5)
        {
            send_to_char("\n\r", ch);
            j=0;
        }
        
        sprintf(buf, "%-12s ", pc_race_table[i].name);
        send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);
}

void roll_dice (CHAR_DATA *ch, bool take_default)
{
    int minimum_roll[15] = {85,85,85,85,85,80,80,75,70,60,50,40,30,15,0};
    int i, j, swap;
    
    if ( take_default )
    {
        for ( i = 0; i < 15; i++ )
            ch->gen_data->unused_die[i] = minimum_roll[i] + 10;
    }
    else
    {
        for ( i = 0; i < 15; i++ )
            ch->gen_data->unused_die[i] = minimum_roll[i] + dice(3,6) - 3;
        // sort
        for (i=0; i<14; i++)
            for (j=i+1; j<15; j++)
                if (ch->gen_data->unused_die[j]>ch->gen_data->unused_die[i])
                {
                    swap = ch->gen_data->unused_die[j];
                    ch->gen_data->unused_die[j] = ch->gen_data->unused_die[i];
                    ch->gen_data->unused_die[i] = swap;
                }
    }
            
    for (i=0; i<15; i++)
        ch->gen_data->assigned_die[i] = -1;

    return;
}

// calculate min, max and current stat for die allocation
MIN_MAX_ROLLED* calc_min_max_rolled(CHAR_DATA *ch, int stat)
{
    static MIN_MAX_ROLLED result;

    int bonus, roll, i;
    int race = ch->race;
    int sex = ch->pcdata->true_sex;
    
    result.min = pc_race_table[race].min_stats[stat];
    result.max = pc_race_table[race].max_stats[stat];
    
    bonus = class_bonus(ch->clss, stat) + remort_bonus(ch, stat);
    result.min += bonus;
    result.max += bonus;
    
    // gender and align tweaks
    switch (stat)
    {
        case STAT_STR:
            result.min += (sex == SEX_MALE ? 5 : -5);
            break;
        case STAT_AGI:
            result.min += (sex == SEX_MALE ? -5 : 5);
            break;
        case STAT_INT:
        case STAT_CHA:
            result.min -= ch->alignment/125;
            break;            
        case STAT_WIS:
        case STAT_LUC:
            result.min += ch->alignment/125;
            break;
        default:
            break;
    }
    result.min = UMAX(1, result.min);
    
    // get average die assigned to stat (times 100)
    roll = 0;
    for (i=0; stat_table[i].name != NULL; i++)
        roll += stat_table[i].dice[stat] * UMAX(ch->gen_data->assigned_die[i],0);
    
    // rolled value lies between min and max, based on roll
    result.rolled = result.min + (result.max - result.min) * roll / 10000;
    
    return &result;
}

void calc_stats(CHAR_DATA *ch)
{
    int i;

    for (i=0; i<MAX_STATS; i++)
    {
        MIN_MAX_ROLLED *values = calc_min_max_rolled(ch, i);
        ch->perm_stat[i] = (short)URANGE(1, values->rolled, MAX_CURRSTAT);
        ch->pcdata->original_stats[i] = ch->perm_stat[i];        
    }

    return;
}

// assign die rolls to stats based on class and race
// maximizes the class-weighted sum of final stats
void auto_assign_stats(CHAR_DATA *ch)
{
    int i,j;
    int base_stat_weights[MAX_STATS];
    int extended_stat_weights[MAX_EXT_STATS];
    
    // compute class factor for base stats
    for (i = 0; i < MAX_STATS; i++)
        base_stat_weights[i] = class_table[ch->clss].stat_weights[i];
    // bonus for primary/secondary stats, decreasing for higher remorts (practices get less important)
    int remort_level = ch->pcdata->remorts;
    base_stat_weights[class_table[ch->clss].attr_prime] += 4 * (15 - remort_level);
    base_stat_weights[class_table[ch->clss].attr_second[0]] += 3 * (15 - remort_level);
    base_stat_weights[class_table[ch->clss].attr_second[1]] += 3 * (15 - remort_level);
    // bonus for int at low remort (needed for practicing skills)
    if (remort_level < 2)
        base_stat_weights[STAT_INT] += (2-remort_level)*10;

    // multiply with racial range
    for (i = 0; i < MAX_STATS; i++)
    {
        MIN_MAX_ROLLED *values = calc_min_max_rolled(ch, i);
        base_stat_weights[i] *= values->max - values->min;
    }    
    
    // weights for each extended stat (including body, mind, tough, speed, wit)
    for (i = 0; i < MAX_EXT_STATS; i++)
    {
        extended_stat_weights[i] = 0;
        for (j = 0; j < MAX_STATS; j++)
            extended_stat_weights[i] += base_stat_weights[j] * stat_table[i].dice[j];        
    }
    
    // Assign currently unassigned dice to unassigned extended attributes
    for (i = 0; i < MAX_EXT_STATS; i++)
    {
        if (ch->gen_data->unused_die[i] == -1)
            break;
        // find highest-ranking unassigned stat
        int max_weighted_stat = -1;
        for (j = 0; j < MAX_EXT_STATS; j++)
            if ( ch->gen_data->assigned_die[j] == -1 )
                if ( max_weighted_stat == -1 || extended_stat_weights[j] > extended_stat_weights[max_weighted_stat] )
                    max_weighted_stat = j;
        // assign die (unassigned dice are ordered, so we it is highest unassigned one)
        ch->gen_data->assigned_die[max_weighted_stat] = ch->gen_data->unused_die[i];
        ch->gen_data->unused_die[i] = -1;
    }

    return;
}

void take_default_stats(CHAR_DATA *ch)
{
    ch->gen_data = new_gen_data();

    roll_dice(ch, TRUE);
    auto_assign_stats(ch);
    calc_stats(ch);

    free_gen_data(ch->gen_data);
    ch->gen_data=NULL;
    
    return;
}

void insert_die(CHAR_DATA *ch, int die)
{
    int i;

    for ( i = 0; i < MAX_EXT_STATS; i++ )
        if ( die > ch->gen_data->unused_die[i] )
        {
            int swap = ch->gen_data->unused_die[i];
            ch->gen_data->unused_die[i] = die;
            die = swap;
        }
}

bool parse_roll_stats (CHAR_DATA *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[100];
    int i,j,stat,die;
    
    argument = one_argument(argument,arg);
    argument = one_argument(argument,arg2);
    argument = one_argument(argument,arg3);
    
    if (arg[0] == '\0') return TRUE;
    
    if (!str_prefix(arg,"help"))
    {
        if (arg2[0] == '\0')
        {
            do_help(ch,"header rollstat");
            return TRUE;
        }
        
        do_help(ch,arg2);
        return TRUE;
    }
    else if (!str_prefix(arg, "reroll"))
    {
        roll_dice( ch, !strcmp(arg2, "default") );
        calc_stats( ch );
    }
    else if (!str_prefix(arg, "unassign"))
    {
        if ( strcmp(arg2, "all") == 0 )
        {
            for ( stat = 0; stat < MAX_EXT_STATS; stat++ )
            {
                die = ch->gen_data->assigned_die[stat];
                if ( die != -1 )
                {
                    insert_die( ch, die );
                    ch->gen_data->assigned_die[stat] = -1;
                }
            }
            calc_stats(ch);
        }
        else
        {
            if (arg2[0]=='\0'||(stat=dice_lookup(arg2))==15)
            {
                do_help(ch,"unassign");
                return TRUE;
            }
            if ((die=ch->gen_data->assigned_die[stat])==-1)
            {
                send_to_char("No die has been assigned to that attribute.\n\r",ch);
                return TRUE;
            }
            ch->gen_data->assigned_die[stat]=-1;
            calc_stats(ch);
            insert_die(ch, die);
        }
    }
    else if (!str_prefix(arg, "assign"))
    {
        if (arg2[0]=='\0'||arg3[0]=='\0'||!is_number(arg3)
                ||(stat=dice_lookup(arg2))==15)
        {
            do_help(ch, "assign");
            return TRUE;
        }
        if (ch->gen_data->assigned_die[stat]!=-1)
        {
            send_to_char("A die has already been assigned to that attribute.\n\r",ch);
            return TRUE;
        }
        die=atoi(arg3);
        for (i=0; i<15; i++)
            if (ch->gen_data->unused_die[i]==die)
                break;
        if (i==15||die==-1)
        {
            sprintf(buf, "You dont have any %d dice to assign.\n\r", die);
            send_to_char(buf,ch);
            return TRUE;
        }
        ch->gen_data->assigned_die[stat]=die;
        calc_stats(ch);
        for(j=i; j<14; j++)
            ch->gen_data->unused_die[j]=ch->gen_data->unused_die[j+1];
        ch->gen_data->unused_die[14]=-1;
    }
    else if (str_prefix(arg, "show")) return FALSE;
    
    show_dice(ch);
    
    return TRUE;
}

void show_dice(CHAR_DATA *ch)
{
    int i, die, train;
    char buf[100], buf2[100], buf3[5];
    
    send_to_char("\n\rStat  Curr  Train Die   Min-Max            Attribute\n\r", ch);
    
    for(i=0; i<10; i++)
    {
        MIN_MAX_ROLLED *values = calc_min_max_rolled(ch, i);
        
        if (i>4)
            buf2[0]='\0';
        else
        {
            if (ch->gen_data->assigned_die[i+10]==-1)
                strcpy(buf3, " --");
            else
                sprintf(buf3, "%3d", ch->gen_data->assigned_die[i+10]);
            sprintf(buf2, "%10s %3s", stat_table[i+10].name, buf3);
        }
        
        if (ch->gen_data->assigned_die[i]==-1)
            strcpy(buf3, " --");
        else
            sprintf(buf3, "%3d", ch->gen_data->assigned_die[i]);
        
        train = (values->rolled + values->max) / 2;
        
        sprintf(buf, " %3s  %3d   %3d   %3s   %3d-%3d            %s\n\r",
            stat_table[i].abbreviation, values->rolled, train, buf3, values->min, values->max, buf2);
        send_to_char(buf, ch);
    }
    
    strcpy(buf, "\n\rDice left to distribute: ");
    for (i=0; i<15; i++)
    {
        if ((die=ch->gen_data->unused_die[i])==-1) break;
        if (i>0) strcat(buf, ", ");
        sprintf(buf3, "%d", die);
        strcat(buf, buf3);
    }
    strcat(buf, "\n\r");
    send_to_char(buf, ch);
    
    // show total of roll
    if ( ch->gen_data->unused_die[14] >= 0 )
    {
        int total = 0;
        for ( i = 0; i < 15; i++ )
            total += ch->gen_data->unused_die[i];
        printf_to_char(ch, "Total (sum of all dice): %d\n\r", total);
    }
    
    return;
}

// account for negative (or positive?) level affects
int modified_level( CHAR_DATA *ch )
{
    int level = ch->level + ch->mod_level;
    int max = IS_NPC(ch) ? 200 : IS_IMMORTAL(ch) ? MAX_LEVEL : LEVEL_HERO;
    return URANGE(1, level, max);
}

int get_pc_hitdice( int level )
{
    int hero_bonus = UMAX(0, level - (LEVEL_HERO - 10));
    hero_bonus = hero_bonus * (hero_bonus + 1) / 2;
    return level + hero_bonus;
}

int get_hero_factor( int level )
{
    int hlevel = UMAX(0, level - (LEVEL_HERO - 10));
    // advancement: 1,1,2,2,3,3,4,4,5,5
    int hero_bonus = (hlevel + 1) / 2 * (hlevel / 2 + 1);
    return 100 + hero_bonus;
}

int softcap_adjust( int trained, int cap )
{
    if ( trained <= cap )
        return trained;
    return cap + (trained - cap) / 5;
}

/* Bobble: recalculate a PC's permanent hp/mana/move
 * and adjust his max hp/mana/move accordingly
 * must be called after each level- or stat change or train 
 */
void update_perm_hp_mana_move(CHAR_DATA *ch)
{
    int new_hp, new_mana, new_move;
    int trained_hp_bonus, trained_mana_bonus, trained_move_bonus;
    int level_factor, train_factor, stat_factor, class_factor;
    int max_train;
    
    /* PCs only */
    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;
    
    int level = modified_level(ch);
    level_factor = get_pc_hitdice(level);
    train_factor = 100 + get_curr_stat(ch, STAT_DIS);
    max_train = max_hmm_train(level);
    
    /* calculate hp */
    stat_factor = 100 + get_curr_stat(ch, STAT_CON);
    class_factor = class_table[ch->clss].hp_gain;
    new_hp = 100 + level_factor * stat_factor * class_factor / 1000;
    /* size and form bonus */
    new_hp += (level + 10) * (get_ch_size(ch, true) - SIZE_MEDIUM);
    if ( IS_SET(ch->form, FORM_TOUGH) )
        new_hp += level * 10;
    if ( has_subclass(ch, subclass_juggernaut) )
        new_hp += 3 * level_factor;
    /* train bonus */
    trained_hp_bonus = UMIN(max_train,ch->pcdata->trained_hit) * train_factor * class_factor / 2000;
    if ( IS_SET(ch->form, FORM_CONSTRUCT) )
        trained_hp_bonus += 2 * UMIN(max_train,ch->pcdata->trained_hit);
    
    /* calculate mana */
    stat_factor = 100 + get_curr_stat(ch, STAT_WIS);
    class_factor = class_table[ch->clss].mana_gain;
    new_mana = 100 + level_factor * stat_factor * class_factor / 1000;
    /* form bonus */
    if ( IS_SET(ch->form, FORM_WISE) )
        new_mana += level * 10;
    /* train bonus */
    trained_mana_bonus = UMIN(max_train,ch->pcdata->trained_mana) * train_factor * class_factor / 2000;
    
    /* calculate move */
    stat_factor = 100 + get_curr_stat(ch, STAT_AGI);
    class_factor = class_table[ch->clss].move_gain;
    new_move = 100 + level_factor * stat_factor * class_factor / 1000;
    /* form bonus */
    if ( IS_SET(ch->form, FORM_AGILE) )
        new_move += level * 10;
    /* train bonus */
    trained_move_bonus = UMIN(max_train,ch->pcdata->trained_move) * train_factor * class_factor / 2000;
    if ( IS_SET(ch->form, FORM_CONSTRUCT) )
        trained_move_bonus += UMIN(max_train,ch->pcdata->trained_move);
    
    /* adjust permanent and max hp/mana/move */
    ch->pcdata->perm_hit = new_hp;
    ch->pcdata->perm_mana = new_mana;
    ch->pcdata->perm_move = new_move;
    
    int hero_factor = get_hero_factor(level);
    ch->max_hit = new_hp + ch->pcdata->temp_hit * hero_factor / 100;
    ch->max_mana = new_mana + ch->pcdata->temp_mana * hero_factor / 100;
    ch->max_move = new_move + ch->pcdata->temp_move * hero_factor / 100;
    
    /* trained hp/mana/move, subject to cap */
    ch->max_hit += softcap_adjust(trained_hp_bonus, ch->max_hit / 2);
    ch->max_mana += softcap_adjust(trained_mana_bonus, ch->max_mana / 2);
    ch->max_move += softcap_adjust(trained_move_bonus, ch->max_move / 2);
}

void get_hmm_softcap( CHAR_DATA *ch, int *hp_cap, int *mana_cap, int *move_cap )
{
    int base_hp, base_mana, base_move;
    int train_factor, gain_per_train;

    if (IS_NPC(ch))
    {
        *hp_cap = *mana_cap = *move_cap = 0;
        return;
    }

    int hero_factor = get_hero_factor(modified_level(ch));
    base_hp = ch->pcdata->perm_hit + ch->pcdata->temp_hit * hero_factor / 100;
    base_mana = ch->pcdata->perm_mana + ch->pcdata->temp_mana * hero_factor / 100;
    base_move = ch->pcdata->perm_move + ch->pcdata->temp_move * hero_factor / 100;

    train_factor = 100 + get_curr_stat(ch, STAT_DIS);
    // hp
    gain_per_train = class_table[ch->clss].hp_gain * train_factor;
    if ( IS_SET(race_table[ch->race].form, FORM_CONSTRUCT) )
        gain_per_train += 4000;
    *hp_cap = base_hp * 1000 / gain_per_train;
    // mana
    gain_per_train = class_table[ch->clss].mana_gain * train_factor;
    *mana_cap = base_mana * 1000 / gain_per_train;
    // move
    gain_per_train = class_table[ch->clss].move_gain * train_factor;
    if ( IS_SET(race_table[ch->race].form, FORM_CONSTRUCT) )
        gain_per_train += 2000;
    *move_cap = base_move * 1000 / gain_per_train;

    return;
}

/* set a mob race and update fields accordingly */
void set_mob_race( CHAR_DATA *ch, int race )
{
    if ( !IS_NPC(ch) )
    {
        bugf("set_mob_race called on PC: %s", ch->name);
        return;
    }
    int pc_race;
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    struct race_type *race_en=&race_table[race];

    /* set the race */
    ch->race=race;

    /* set size if it's a pc race, else mob's default */
    if ( race_en->pc_race )
    {
        pc_race=pc_race_lookup( race_en->name );
        if (pc_race==0)
        {
            bugf("set_mob_race: Couldn't find pc_race");
            return;
        }
        
        ch->size = pc_race_table[pc_race].size;
    }
    else
    {
        ch->size = ch->pIndexData->size;
    }
    
    flag_copy( ch->form, race_en->form );
    flag_copy( ch->parts, race_en->parts ); 

    /* reset flags to racial defaults */
    flag_copy( ch->affect_field, race_en->affect_field );
    flag_copy( ch->imm_flags, race_en->imm );
    flag_copy( ch->res_flags, race_en->res );
    flag_copy( ch->vuln_flags, race_en->vuln );

    /* add spell flags */
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    set_affect_flag( ch, paf );

    /* add object flags */
    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
    if (obj->wear_loc == -1)
        continue;

    for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        set_affect_flag( ch, paf );

    for (paf = obj->affected; paf != NULL; paf = paf->next)
        set_affect_flag( ch, paf );
    }

    return;
}

/* Bobble: update all affect, immune, vuln, resist flags on ch
 */
void update_flags( CHAR_DATA *ch )
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    struct race_type *race;

    /* only for PCs */
    if ( IS_NPC(ch) )
	return;
    
    /* get current race */
    race = get_morph_race_type( ch );

    /* reset flags to racial defaults */
    flag_copy( ch->affect_field, race->affect_field );
    flag_copy( ch->imm_flags, race->imm );
    flag_copy( ch->res_flags, race->res );
    flag_copy( ch->vuln_flags, race->vuln );

    /* add spell flags */
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
	set_affect_flag( ch, paf );

    /* add object flags */
    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
	if (obj->wear_loc == -1)
	    continue;
            
	for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
	    set_affect_flag( ch, paf );

	for (paf = obj->affected; paf != NULL; paf = paf->next)
	    set_affect_flag( ch, paf );
    }
}

/* sets the bitvector of paf to the correct flag
 */
void set_affect_flag( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    if ( paf->bitvector < 1 )
	return;

    switch ( paf->where )
    {
    case TO_AFFECTS:
	SET_AFFECT( ch, paf->bitvector );
	break;
    case TO_IMMUNE:
	SET_BIT( ch->imm_flags, paf->bitvector );
	break;
    case TO_RESIST:
	SET_BIT( ch->res_flags, paf->bitvector );
	break;
    case TO_VULN:
	SET_BIT( ch->vuln_flags, paf->bitvector );
	break;
    case TO_OBJECT:
	break;
    case TO_WEAPON:
        break;
    case TO_SPECIAL:
	break;
    default:
	bugf( "set_affect_flag: Invalid where (%d) on %s affect on %s",
	      paf->where,
	      paf->type > 0 ? skill_table[paf->type].name : "?",
	      ch->name );
        log_trace();
	break;
    }    
}

/* get the race_type for a morphed char
 */
struct race_type* get_morph_race_type( CHAR_DATA *ch )
{
    if ( IS_NPC(ch) )
	return &race_table[ch->race];

    /* doppelganger */
    if ( MULTI_MORPH(ch) && ch->pcdata->morph_race > 0 )
    {
	/* watch out for morphing into other morph races */
	if ( ch->pcdata->morph_race == race_naga )
	    return &morph_race_table[MORPH_NAGA_SERPENT];
	else
	    return &race_table[ch->pcdata->morph_race];
    }

    /* naga */
    if ( ch->race == race_naga )
    {
	if ( ch->pcdata->morph_race == 0 )
	    return &morph_race_table[MORPH_NAGA_SERPENT];
	else
	    return &morph_race_table[MORPH_NAGA_HUMAN];
    }

    /* werewolf */
    if ( ch->race == race_werewolf )
    {
	if ( weather_info.sunlight == SUN_DARK 
	     || weather_info.sunlight == SUN_SET )
	    return &race_table[race_werewolf];
	else
	    return &morph_race_table[MORPH_WOLFMAN];
    }
    
    if ( ch->race == race_dragonborn && ch->pcdata->morph_race > 0 )
        return &morph_race_table[ch->pcdata->morph_race];
    
    return &race_table[ch->race];
}

/* get the pc_race_type for a morphed char
 */
struct pc_race_type* get_morph_pc_race_type( CHAR_DATA *ch )
{
    if ( IS_NPC(ch) )
    {
	bug( "get_morph_pc_race_type: char is NPC", 0 );
	return NULL;
    }

    /* doppelganger */
    if ( MULTI_MORPH(ch) && ch->pcdata->morph_race > 0 )
	return &pc_race_table[ch->pcdata->morph_race];

    /* naga */
    if ( ch->race == race_naga )
    {
	if ( ch->pcdata->morph_race == 0 )
	    return &morph_pc_race_table[MORPH_NAGA_SERPENT];
	else
	    return &morph_pc_race_table[MORPH_NAGA_HUMAN];
    }
    
    /* werewolf */
    if ( ch->race == race_werewolf )
    {
	if ( weather_info.sunlight == SUN_DARK
	     || weather_info.sunlight == SUN_SET )
	    return &pc_race_table[race_werewolf];
	else
	    return &morph_pc_race_table[MORPH_WOLFMAN];
    }

    return &pc_race_table[ch->race];
}

/* class restriction stuff for ITEM_ALLOW_* */
/* { Thf, War, Clr, Mag } */
const bool class_group_table[MAX_CLASS][4] =
{
    { 1, 0, 0, 0 }, // warrior
    { 0, 1, 0, 0 }, // thief
    { 0, 0, 1, 0 }, // cleric
    { 0, 0, 0, 1 }, // mage
    { 1, 0, 0, 0 }, // gladiator
    { 1, 0, 0, 0 }, // samurai
    { 1, 0, 1, 0 }, // paladin
    { 1, 1, 0, 0 }, // assassin
    { 1, 1, 0, 0 }, // ninja
    { 1, 0, 1, 0 }, // monk
    { 0, 0, 1, 1 }, // templar
    { 0, 1, 0, 1 }, // illusionist
    { 1, 1, 0, 0 }, // gunslinger
    { 1, 1, 0, 0 }, // ranger
    { 0, 0, 0, 1 }, // necromancer
    { 1, 0, 0, 0 }  // bard
};

bool class_can_use( int class, tflag xtra_flags )
{
    int group, flag;
    bool allow_found = FALSE;

    /* check anti_group flags */
    for ( group = 0; group < 4; group++ )
    {
        flag = ITEM_ANTI_WARRIOR + group;
        if ( IS_SET(xtra_flags, flag) && class_group_table[class][group] )
            return FALSE;
    }

    /* check allow_group flags */
    for ( group = 0; group < 4; group++ )
    {
        flag = ITEM_ALLOW_WARRIOR + group;
        if ( IS_SET(xtra_flags, flag) )
        {
            if ( class_group_table[class][group] )
                return TRUE;
            else
                allow_found = TRUE;
        }
    }

    /* check class_ flags */
    for ( group = 0; group < MAX_CLASS; group++ )
    {
        flag = ITEM_CLASS_WARRIOR + group;
        if ( IS_SET(xtra_flags, flag) )
        {
            if ( class == group )
                return TRUE;
            else
                allow_found = TRUE;
        }
    }

    /* if no allow flags found, all classes can use object */
    return !allow_found;
}

bool class_can_use_obj( int class, OBJ_DATA *obj )
{
    return class_can_use( class, obj->extra_flags );
}

int classes_can_use( tflag xtra_flags )
{
    int class;
    int count = 0;
    
    for (class = 0; class <= MAX_CLASS; class++)
        if ( class_can_use(class, xtra_flags) )
            count++;
    return count;
}

/* encumberance */
int get_encumberance( CHAR_DATA *ch )
{
    int max = can_carry_w(ch);
    int no_encumber = UMIN( max - 1, max / 2 );
    int encumber = 100 * (get_carry_weight(ch) - no_encumber) / (max - no_encumber);
    if ( IS_NPC(ch) )
	return 0;
    else
	return URANGE( 0, encumber, 100 );
}

