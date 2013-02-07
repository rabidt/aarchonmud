#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
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
void calc_stats args((CHAR_DATA *ch));
bool parse_roll_stats args((CHAR_DATA *ch,char *argument));
bool parse_stat_priority args((CHAR_DATA *ch, char *argument));
void do_help args((CHAR_DATA *ch, char *argument));
struct race_type* get_morph_race_type( CHAR_DATA *ch );
void show_pc_race_ratings( CHAR_DATA *ch, int race );
void set_affect_flag( CHAR_DATA *ch, AFFECT_DATA *paf );

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
    
    if (!IS_NPC(ch) && (ch->race==race_doppelganger) && (ch->pcdata->morph_race > 0))
    {
	int org_min, org_max, new_min, new_max,
	    ch_class_bonus, stat_roll, new_base, remort_bonus;
	struct pc_race_type *new_race_type;
	/* adjust base stat for new race */
	org_min = pc_race_table[race_doppelganger].min_stats[stat];
	org_max = pc_race_table[race_doppelganger].max_stats[stat];
	new_race_type = &pc_race_table[ch->pcdata->morph_race];
	new_min = new_race_type->min_stats[stat];
	new_max = new_race_type->max_stats[stat];
	ch_class_bonus = class_bonus( ch->class, stat );
	stat_roll = ch->perm_stat[stat] - ch_class_bonus - org_min;
	new_base = new_min
	    + (new_max - new_min) * stat_roll / (org_max - org_min)
	    + ch_class_bonus;
	/* remort bonus */
	remort_bonus = (ch->pcdata->remorts - new_race_type->remorts) * 
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
    max += class_bonus(ch->class, stat);
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
	return (ch->level + 10) * (dex-60) / 200;
};

int ch_str_todam(CHAR_DATA *ch)
{
    int str = get_curr_stat(ch, STAT_STR);
    if ( str < 60 )
	return 0;
    else
	return (ch->level + 10) * (str-60) / 100;
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

int ch_prac_gains(CHAR_DATA *ch)
{
    int x1 = get_curr_stat(ch,class_table[ch->class].attr_prime)/30;
    int x2 = get_curr_stat(ch,class_table[ch->class].attr_second[0])/45;
    int x3 = get_curr_stat(ch,class_table[ch->class].attr_second[1])/45;
    int x = x1+x2+x3;
    return x;
};

int ch_agi_defensive(CHAR_DATA *ch)
{
    return (ch->level + 10) * agi_app_defensive(get_curr_stat(ch, STAT_AGI)) / 100;
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

int get_ac( CHAR_DATA *ch, int type )
{
    int ac = ch->armor[type];
    if ( IS_AWAKE(ch) )
	ac += ch_agi_defensive(ch);
    else
	ac += 50;
    ac -= get_curr_stat(ch, STAT_LUC) / 2;
    if ( IS_SET(ch->parts, PART_SCALES) )
	ac -= ch->level / 2;
    return ac;
}

int get_hitroll( CHAR_DATA *ch )
{
    int hitroll = ch->hitroll 
	+ ch_dex_tohit(ch)
	+ get_curr_stat(ch, STAT_LUC) / 10;
    /*
    if ( IS_NPC(ch) )
	hitroll += ch->level;
    */
    return hitroll;
}

int get_damroll( CHAR_DATA *ch )
{
    int damroll = ch->damroll
	+ ch_str_todam(ch)
	+ get_curr_stat(ch, STAT_LUC) / 20;
    return damroll;
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
        if ( stat == STAT_LUC )
	    return 0;
        else
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

void do_train( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    sh_int stat = - 1;
    int cost, max, inc;
    
    if ( IS_NPC(ch) )
        return;
    
   /*
    * Check for trainer.
    */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN) )
            break;
    }
    
    if ( mob == NULL )
    {
        if (get_skill(ch, gsn_introspection) > 1)
        {
            act( "$n thinks over what $e has experienced recently.", ch, NULL, NULL, TO_ROOM );
            if ((get_skill(ch,gsn_introspection)) > number_percent ()) 
                check_improve(ch,gsn_introspection,TRUE,8);
            else
            {
                send_to_char("You've learned nothing from your recent experiences.\n\r",ch);     
                check_improve(ch,gsn_introspection,FALSE,8);
                return;
            }
        }    
        else
        {
            send_to_char( "You can't do that here.\n\r", ch );
            return;
        }
    }
    
    if ( argument[0] == '\0' )
    {
        sprintf( buf, "You have %d training sessions.\n\r", ch->train );
        send_to_char( buf, ch );
        argument = "foo";
    }
    
    cost = 1;

    if ( !str_cmp(argument, "hp" ) )
        cost = 1;
    
    else if ( !str_cmp(argument, "mana" ) )
        cost = 1;
    
    else if ( !str_cmp(argument, "move" ) )
        cost = 1;
    
    else   
        for (stat=0; stat<MAX_STATS; stat++)
            if (!str_prefix(argument, stat_table[stat].name))
            {
                cost=1;
                break;
            } 
            
    if (stat==MAX_STATS)
    {
        strcpy( buf, "You can train:" );
        for (stat=0; stat<MAX_STATS; stat++)
            if ( (inc = train_stat_inc(ch, stat)) > 0 )
            {
		sprintf( buf2, " %s(+%d)", stat_table[stat].abbreviation, inc );
                strcat( buf, buf2); 
            }
            
            if (train_stat(ch->pcdata->trained_hit, ch)) strcat(buf, " hp");
            if (train_stat(ch->pcdata->trained_move, ch)) strcat(buf, " move");
            if (train_stat(ch->pcdata->trained_mana, ch)) strcat(buf, " mana");
            
            if ( buf[strlen(buf)-1] != ':' )
            {
                strcat( buf, ".\n\r" );
                send_to_char( buf, ch );
            }
            else
            {
            /*
            * This message dedicated to Jordan ... you big stud!
                */
                act( "You have nothing left to train, you $T!",
                    ch, NULL,
                    ch->sex == SEX_MALE   ? "big stud" :
                ch->sex == SEX_FEMALE ? "hot babe" :
                "wild thing",
                    TO_CHAR );
            }
            
            return;
    }

   /* Warning: Don't modify the amount of hp/mana/move training
      without adjusting the death_penalty method in fight.c
      a corresponding amount */    
    if (!str_cmp("hp",argument))
    {
        if ( cost > ch->train )
        {
            send_to_char( "You don't have enough training sessions.\n\r", ch );
            return;
        }
        
        if (!train_stat(ch->pcdata->trained_hit, ch))
        {
            send_to_char( "You cant train any more hps you freak.\n\r", ch);
            return;
        }
        
        ch->pcdata->trained_hit++;
        ch->train -= cost;

        update_perm_hp_mana_move(ch);

//        WAIT_STATE(ch, 2);
        WAIT_STATE(ch, 1);
        act( "Your durability increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's durability increases!",ch,NULL,NULL,TO_ROOM);
        return;
    }

   /* Warning: Don't modify the amount of hp/mana/move training
      without adjusting the death_penalty method in fight.c
      a corresponding amount */       
    if (!str_cmp("mana",argument))
    {
        if ( cost > ch->train )
        {
            send_to_char( "You don't have enough training sessions.\n\r", ch );
            return;
        }
        
        if (!train_stat(ch->pcdata->trained_mana, ch))
        {
            send_to_char( "You have as much mana as you possibly can, you freak.\n\r", ch);
            return;
        }
        

        ch->pcdata->trained_mana++;
        ch->train -= cost;
        update_perm_hp_mana_move(ch);

//        WAIT_STATE(ch, 2);
        WAIT_STATE(ch, 1);
        act( "Your power increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's power increases!",ch,NULL,NULL,TO_ROOM);
        return;
    }

   /* Warning: Don't modify the amount of hp/mana/move training
      without adjusting the death_penalty method in fight.c
      a corresponding amount */       
    if (!str_cmp("move",argument))
    {
        if ( cost > ch->train )
        {
            send_to_char( "You don't have enough training sessions.\n\r", ch );
            return;
        }
        
        if (!train_stat(ch->pcdata->trained_move, ch))
        {
            send_to_char( "You have as much stamina as you possibly can, you freak.\n\r", ch);
            return;
        }
        
        ch->pcdata->trained_move++;
        ch->train -= cost;
        update_perm_hp_mana_move(ch);
        
//        WAIT_STATE(ch, 2);
        WAIT_STATE(ch, 1);
        act( "Your stamina increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's stamina increases!",ch,NULL,NULL,TO_ROOM);
        return;
    }
    
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
        if (stat == STAT_LUC)
        {
            send_to_char("You cant upgrade your karma.\n\r", ch);
            return;
        }
        
        cost = ch->perm_stat[stat] - ch->pcdata->original_stats[stat];
        cost = cost*20+120;
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
        
	/*
        max -= ch->perm_stat[stat];
        cost = UMIN(3, max);
	*/
    } else {
        ch->train -= cost;
        
	/*
        max-=ch->perm_stat[stat];
        
        if (max>45)
            cost=5;
        else if (max>21)
            cost=4;
        else if (max>9)
            cost=3;
        else if (max>3)
            cost=2;
        else
            cost=1;
        cost=UMIN(1 + ch->perm_stat[stat]/15, cost);
	*/
    }
    
//    WAIT_STATE(ch, 2);
      WAIT_STATE(ch, 1);
    
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

void do_stats( CHAR_DATA *ch, char *argument )
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
                sprintf(buf, " {%c%2d-%3d{x", (j%2)?'c':'y', pc_race_table[i].min_stats[j],
                pc_race_table[i].max_stats[j]);
                add_buf(output,buf);
	    }
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

void do_etls( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char racebuf[5];
    int i, j, race;
    BUFFER *output;
    
    if (argument[0] == '\0')
    {
        output = new_buf();

        add_buf(output,"\n\rThis table shows the number of experience points");
        add_buf(output,"\n\rrequired to gain one level.");
        add_buf(output,"\n\r\n\rTo get more information, type: HELP ETLS\n\r");
        
        add_buf(output, "Race");
        for (i=0; i<MAX_CLASS; i++)
        {
            sprintf(buf, "  %3s", class_table[i].who_name);
            add_buf(output, buf);
        }
        
        for (i=1; i<MAX_PC_RACE; i++)
        {
            for (j=0; j<4 && pc_race_table[i].who_name[j]; j++)
                racebuf[j]=pc_race_table[i].who_name[j];
            racebuf[j]='\0';
            
            sprintf(buf, "\n\r%s", racebuf);
            add_buf(output, buf);
            for (j=0; j<MAX_CLASS; j++)
            {
                sprintf(buf, " %3d0", pc_race_table[i].class_mult[j]);
                add_buf(output, buf);
            }
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
        
        add_buf(output, "Race");
	for( i=0; i<MAX_CLASS; i++ )
	{
            sprintf(buf, "  %3s", class_table[i].who_name);
            add_buf(output, buf);
	}

        for( i=1; i<MAX_PC_RACE; i++ )
        {
            if( pc_race_table[i].remorts > r_num )
                continue;
        
            for (j=0; j<4 && pc_race_table[i].who_name[j]; j++)
                racebuf[j]=pc_race_table[i].who_name[j];
            racebuf[j]='\0';
            sprintf(buf, "\n\r%s", racebuf);
            add_buf(output,buf);

            for (j=0; j<MAX_CLASS; j++)
            {
                sprintf(buf, " %3d0", pc_race_table[i].class_mult[j]);
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
         
        add_buf(output, "Race");
	for( i=0; i<MAX_CLASS; i++ )
	{
            sprintf(buf, "  %3s", class_table[i].who_name);
            add_buf(output, buf);
	}
        
        for( i=1; i<MAX_PC_RACE; i++ )
        {
            if( pc_race_table[i].remorts != r_num )
                continue;

            for (j=0; j<4 && pc_race_table[i].who_name[j]; j++)
                racebuf[j]=pc_race_table[i].who_name[j];
            racebuf[j]='\0';
            sprintf(buf, "\n\r%s", racebuf);
            add_buf(output,buf);

            for (j=0; j<MAX_CLASS; j++)
            {
                sprintf(buf, " %3d0", pc_race_table[i].class_mult[j]);
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
void do_showrace(CHAR_DATA *ch, char *argument)
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

    /* comment out while not imping new races --Bobble */
    
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
    float allowed_rating = (80 + 5 * pc_race_table[race].remorts) * 9.5;
    float rating = 0;

    /* for each stat */
    for ( stat = 0; stat < MAX_STATS; stat++ )
    {
	float stat_rating = (pc_race_table[race].min_stats[stat] 
			     + 3 * pc_race_table[race].max_stats[stat]) / 4.0;
	if ( stat == STAT_CHA )
	    stat_rating /= 2;
	rating += stat_rating;

	sprintf( buf, "%-7.2f", stat_rating );
	send_to_char( buf, ch );
    }
    /* and the total */
    sprintf( buf, "\n\r=> %.2f/%.2f\n\r", rating, allowed_rating );
    send_to_char( buf, ch );
}


void do_racelist(CHAR_DATA *ch, char *argument)
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

extern u_short port;
void roll_dice (CHAR_DATA *ch, bool take_default)
{
#ifdef TESTER
    int default_roll[15] = {100,95,95,90,90,85,80,75,70,65,65,50,40,30,20};
#else
    int default_roll[15] = {100,95,90,85,80,75,70,65,60,55,50,40,30,20,10};
#endif /* TESTER */
//    int default_roll[15] = {100,95,94,92,88,86,82,80,74,73,71,51,49,20,10};
    int i, j, swap, sum;
    
    if ( take_default )
    {
        for ( i = 0; i < 15; i++ )
		ch->gen_data->unused_die[i] = default_roll[i];
    }
    else
    {
	ch->gen_data->unused_die[0]=number_range(90,100);
	ch->gen_data->unused_die[1]=number_range(85,100);
	ch->gen_data->unused_die[2]=number_range(80,100);
	ch->gen_data->unused_die[3]=number_range(70,100);
	ch->gen_data->unused_die[4]=number_range(60,100);
	for (i=5; i<12; i++)
	    ch->gen_data->unused_die[i]=number_range(15,85);
	ch->gen_data->unused_die[12]=number_range(0, 50);
	ch->gen_data->unused_die[13]=number_range(0, 40);
	ch->gen_data->unused_die[14]=number_range(0, 20);
	
	for (i=0; i<14; i++)
	    for (j=i+1; j<15; j++)
		if (ch->gen_data->unused_die[j]>ch->gen_data->unused_die[i])
		{
		    swap = ch->gen_data->unused_die[j];
		    ch->gen_data->unused_die[j]=ch->gen_data->unused_die[i];
		    ch->gen_data->unused_die[i]=swap;
		}
    }
            
    for (i=0; i<15; i++)
	ch->gen_data->assigned_die[i]=-1;

    return;
}

void calc_stats(CHAR_DATA *ch)
{
    int i, j;
    int race=ch->race;
    long int stat, range;
    
    for (i=0; i<MAX_STATS; i++)
        ch->perm_stat[i]=pc_race_table[race].min_stats[i]+class_bonus(ch->class,i)
        +remort_bonus(ch, i);
    
    if (ch->pcdata->true_sex==SEX_MALE)
    {
        ch->perm_stat[STAT_AGI]-=5;
        ch->perm_stat[STAT_STR]+=5;
    }
    else if (ch->pcdata->true_sex==SEX_FEMALE)
    {
        ch->perm_stat[STAT_STR]-=5;
        ch->perm_stat[STAT_AGI]+=5;
    }
    
    ch->perm_stat[STAT_WIS]+=ch->alignment/125;
    ch->perm_stat[STAT_INT]-=ch->alignment/125;
    ch->perm_stat[STAT_LUC]+=ch->alignment/125;
    ch->perm_stat[STAT_CHA]-=ch->alignment/125;
    
    for (i=0; i<MAX_STATS; i++)
    {
        stat=0;
        for (j=0; stat_table[j].name!=NULL; j++)
            stat += stat_table[j].dice[i] *
            UMAX(ch->gen_data->assigned_die[j],0);
        range = pc_race_table[race].max_stats[i] - pc_race_table[race].min_stats[i];    
        if (ch->pcdata->true_sex==SEX_MALE)
        {
            if (i==STAT_STR) range-=5;
            else if (i==STAT_AGI) range+=5;
        }
        else if (ch->pcdata->true_sex==SEX_FEMALE)
        {
            if (i==STAT_AGI) range-=5;
            else if (i==STAT_STR) range+=5;
        }
        if (i==STAT_WIS)
            range-=ch->alignment/125;
        else if (i==STAT_INT)
            range+=ch->alignment/125;
        else if (i==STAT_CHA)
            range+=ch->alignment/125;
        else if (i==STAT_LUC)
            range-=ch->alignment/125;
        stat*=range;
        stat/=10000;
        ch->perm_stat[i] = (short)URANGE(1, ch->perm_stat[i] + stat, MAX_CURRSTAT);
        ch->pcdata->original_stats[i]=ch->perm_stat[i];
    }
    
    return;
}

void get_random_stats(CHAR_DATA *ch)
{
    int i,j, swap;
    
    roll_dice(ch, TRUE);
    
    for (i=0; stat_table[i].name!=NULL; i++)
    {
        ch->gen_data->assigned_die[i]=0;
        for (j=0; j<MAX_STATS; j++)
            ch->gen_data->assigned_die[i]+=(MAX_STATS-j) *
            stat_table[i].dice[ch->gen_data->stat_priority[j]];
    }
    
    for (i=0; stat_table[i].name!=NULL; i++)
        ch->gen_data->stat_priority[i]=i;
    
    for (i=0; i<14; i++)
        for (j=i+1; j<15; j++)
            if (ch->gen_data->assigned_die[j]>ch->gen_data->assigned_die[i])
            {
                swap = ch->gen_data->assigned_die[j];
                ch->gen_data->assigned_die[j]=ch->gen_data->assigned_die[i];
                ch->gen_data->assigned_die[i]=swap;
                swap = ch->gen_data->stat_priority[j];
                ch->gen_data->stat_priority[j]=ch->gen_data->stat_priority[i];
                ch->gen_data->stat_priority[i]=swap;
            }
            
    for (i=0; i<15; i++)
	ch->gen_data->assigned_die[ch->gen_data->stat_priority[i]]=
	    ch->gen_data->unused_die[i];
    
    calc_stats(ch);
    
    return;
}

void take_default_stats(CHAR_DATA *ch)
{
    int i;
    ch->gen_data = new_gen_data();
    ch->gen_data->stat_priority[0]=class_table[ch->class].attr_prime;
    ch->gen_data->stat_priority[1]=class_table[ch->class].attr_second[0];
    ch->gen_data->stat_priority[2]=class_table[ch->class].attr_second[1];
    for (i = 3; i<MAX_STATS; i++)
        ch->gen_data->stat_priority[i]=class_table[ch->class].stat_priority[i-3];
    
    get_random_stats(ch);
    
    free_gen_data(ch->gen_data);
    ch->gen_data=NULL;
    
    return;
}

bool parse_stat_priority (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int i,j;
    
    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0') return TRUE;
    
    if (!str_prefix(arg,"help"))
    {
        if (argument[0] == '\0')
        {
            do_help(ch,"priority header");
        } else do_help(ch,argument);
    }
    else if (!str_prefix(arg, "undo"))
    {
        for(i=0; i<MAX_STATS; i++)
            if (ch->gen_data->stat_priority[i]==-1) break;
            if (i==0)
            {
                send_to_char("There is nothing to undo.\n\r", ch);
                return TRUE;
            }
            ch->gen_data->unused_die[ch->gen_data->stat_priority[i-1]]=TRUE;
            ch->gen_data->stat_priority[i-1]=-1;
    }
    else
    {
        for (i=0; i<MAX_STATS; i++)
            if (!str_prefix(arg, stat_table[i].name)) break;
            if (i==MAX_STATS) return FALSE;
            if (!ch->gen_data->unused_die[i])
            {
                send_to_char("You've already assigned that.\n\r",ch);
                return TRUE;
            }
            for(j=0; j<MAX_STATS; j++)
                if (ch->gen_data->stat_priority[j]==-1) break;
                ch->gen_data->stat_priority[j]=i;
                ch->gen_data->unused_die[i]=FALSE;
    }
    
    strcpy(buf, "Statistics left to assign:");
    for (i=0; i<MAX_STATS; i++)
        if (ch->gen_data->unused_die[i])
        {
            strcat(buf, " ");           
            strcat(buf, stat_table[i].name);
        }
        
        strcat(buf, "\n\rCurrent statistic priority:");
        for (i=0; i<MAX_STATS; i++)
        {
            if (ch->gen_data->stat_priority[i]==-1) break;
            strcat(buf, " ");           
            strcat(buf, stat_table[ch->gen_data->stat_priority[i]].name);
        }
        
        strcat(buf, "\n\r");
        send_to_char(buf, ch);
        
        return TRUE;
}

bool parse_roll_stats (CHAR_DATA *ch,char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[100], buf2[100], buf3[5];
    int i,j,stat,die;
    int curr, train, min, max, sum;
    
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
        
        for(i=0; i<15; i++)
            if (die>ch->gen_data->unused_die[i]) break;
            
            for (j=14; j>i; j--)
                ch->gen_data->unused_die[j]=ch->gen_data->unused_die[j-1];
            ch->gen_data->unused_die[i] = die;  
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
    
    send_to_char("\n\rStat  Curr  Train Die   Min-Max            Attribute\n\r", ch);
    for(i=0; i<10; i++)
    {
        curr=ch->perm_stat[i];
        min=pc_race_table[ch->race].min_stats[i];
        if (ch->pcdata->true_sex==SEX_MALE)
        {
            if (i==STAT_AGI) min=UMAX(1,min-5);
            else if (i==STAT_STR) min+=5;
        }
        else if (ch->pcdata->true_sex==SEX_FEMALE)
        {
            if (i==STAT_STR) min=UMAX(1,min-5);
            else if (i==STAT_AGI) min+=5;
        }
        max=pc_race_table[ch->race].max_stats[i];
        
        j=class_bonus(ch->class, i);
        min+=j;
        max+=j;
        
        j=remort_bonus(ch, i);
        min+=j;
        max+=j;
        
        if (i==STAT_WIS)
            min=UMAX(1, min+ch->alignment/125);
        else if (i==STAT_INT)
            min=UMAX(1, min-ch->alignment/125);
        else if (i==STAT_CHA)
            min=UMAX(1, min-ch->alignment/125);
        else if (i==STAT_LUC)
            min=UMAX(1, min+ch->alignment/125);
        
        if (i>4) buf2[0]='\0';
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
        train=(curr+max)/2;
        
        sprintf(buf, " %3s  %3d   %3d   %3s   %3d-%3d            %s\n\r",
            stat_table[i].abbreviation, curr, train, buf3, min, max, buf2);
        send_to_char(buf, ch);
    }
    
    strcpy(buf, "\n\rDice left to distribute: ");
    sum = 0;
    for (i=0; i<15; i++)
    {
        if ((j=ch->gen_data->unused_die[i])==-1) break;
        if (i>0) strcat(buf, ", ");
        sprintf(buf3, "%d", j);
        strcat(buf, buf3);
	sum += j;
    }

    /*
    sprintf( buf2, " (sum=%d)", sum );
    strcat( buf, buf2 );
    */

    strcat(buf, "\n\r");

    send_to_char(buf, ch);
    
    return TRUE;
}

/* Bobble: recalculate a PC's permanent hp/mana/move
 * and adjust his max hp/mana/move accordingly
 * must be called after each level- or stat change or train 
 */
void update_perm_hp_mana_move(CHAR_DATA *ch)
{
    int new_hp, new_mana, new_move;
    int level_factor, train_factor, stat_factor, class_factor;
    int hero_bonus, train_bonus, max_train;
    
    /* PCs only */
    if (IS_NPC(ch) || ch->pcdata == NULL)
	return;
     
    hero_bonus = UMAX(0, ch->level - (LEVEL_HERO - 10));
    hero_bonus = hero_bonus * (hero_bonus + 1) / 2;
    level_factor = (ch->level + hero_bonus) * 600;
    train_factor = 100 + get_curr_stat(ch, STAT_DIS);
    max_train = max_hmm_train( ch->level );
    
    /* calculate hp */
    train_bonus = UMIN(max_train,ch->pcdata->trained_hit) * train_factor;
    stat_factor = 100 + get_curr_stat(ch, STAT_CON);
    class_factor = class_table[ch->class].hp_gain;
    new_hp = (level_factor + train_bonus) * stat_factor / 6000;
    new_hp = 100 + new_hp * class_factor / 100;
    /* size and form bonus */
    new_hp += (ch->level + 10) * (ch->size - SIZE_MEDIUM);
    if ( IS_SET(race_table[ch->race].form, FORM_CONSTRUCT) )
	new_hp += 2 * ch->pcdata->trained_hit;
    if ( IS_SET(race_table[ch->race].form, FORM_TOUGH) )
	new_hp += ch->level * 10;
    
    /* calculate mana */
    train_bonus = UMIN(max_train,ch->pcdata->trained_mana) * train_factor;
    stat_factor = 100 + get_curr_stat(ch, STAT_WIS);
    class_factor = class_table[ch->class].mana_gain;
    new_mana = (level_factor + train_bonus) * stat_factor / 6000;
    new_mana = 100 + new_mana * class_factor / 100;
    /* form bonus */
    if ( IS_SET(race_table[ch->race].form, FORM_WISE) )
	new_mana += ch->level * 10;
    
    /* calculate move */
    train_bonus = UMIN(max_train,ch->pcdata->trained_move) * train_factor;
    stat_factor = 100 + get_curr_stat(ch, STAT_AGI);
    class_factor = class_table[ch->class].move_gain;
    new_move = (level_factor + train_bonus) * stat_factor / 6000;
    new_move = 100 + new_move * class_factor / 100;
    /* form bonus */
    if ( IS_SET(race_table[ch->race].form, FORM_CONSTRUCT) )
	new_move += ch->pcdata->trained_move;
    if ( IS_SET(race_table[ch->race].form, FORM_AGILE) )
	new_move += ch->level * 10;
    
    /* adjust permanent and max hp/mana/move */
    ch->max_hit += new_hp - ch->pcdata->perm_hit;
    ch->max_mana += new_mana - ch->pcdata->perm_mana;
    ch->max_move += new_move - ch->pcdata->perm_move;
    ch->pcdata->perm_hit = new_hp;
    ch->pcdata->perm_mana = new_mana;
    ch->pcdata->perm_move = new_move;
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
    case TO_SPECIAL:
	break;
    default:
	bugf( "set_affect_flag: Invalid where (%d) on %s affect on %s",
	      paf->where,
	      paf->type > 0 ? skill_table[paf->type].name : "?",
	      ch->name );
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
    if ( ch->race == race_doppelganger && ch->pcdata->morph_race > 0)
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
    if ( ch->race == race_doppelganger && ch->pcdata->morph_race > 0)
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

/* class restriction stuff */

bool class_group_table[MAX_CLASS][4] =
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
    { 0, 0, 0, 1 }  // necromancer
};

bool class_can_use_obj( int class, OBJ_DATA *obj )
{
    int group, flag;
    bool allow_found = FALSE;

    /* check anti_group flags */
    for ( group = 0; group < 4; group++ )
    {
	flag = ITEM_ANTI_WARRIOR + group;
	if ( IS_SET(obj->extra_flags, flag)
	     && class_group_table[class][group] )
	    return FALSE;
    }

    /* check allow_group flags */
    for ( group = 0; group < 4; group++ )
    {
	flag = ITEM_ALLOW_WARRIOR + group;
	if ( IS_SET(obj->extra_flags, flag) )
	    if ( class_group_table[class][group] )
		return TRUE;
	    else
		allow_found = TRUE;
    }

    /* check class_ flags */
    for ( group = 0; group < MAX_CLASS; group++ )
    {
	flag = ITEM_CLASS_WARRIOR + group;
	if ( IS_SET(obj->extra_flags, flag) )
	    if ( class == group )
		return TRUE;
	    else
		allow_found = TRUE;
    }

    /* if no allow flags found, all classes can use object */
    return !allow_found;
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

