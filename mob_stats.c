#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include "merc.h"
#include "mudconfig.h"

// wealth

long level_base_wealth( int level )
{
    return (long)(level * level * (level / 15.0 + 2) * 0.2);
}

long mob_base_wealth( MOB_INDEX_DATA *pMobIndex )
{  
    int level = pMobIndex->level;
    float factor = 1;
    
    if ( pMobIndex->pShop != NULL )
    {
        level = 120;
        factor = 5;
    }
    factor = factor * pMobIndex->wealth_percent / 100;
    factor *= cfg_gold_mult;

    return (long)(level_base_wealth(level) * factor);
}

// hitpoints

int level_base_hp( int level )
{
    return (5 + level) * (10 + level * 3/4);
}

int mob_base_hp( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int hp = level_base_hp( level ) * pMobIndex->hitpoint_percent / 100;
    return UMAX(1, hp);
}

// mana

int level_base_mana( int level )
{
    return (5 + level) * (10 + level * 1/4);
}

int mob_base_mana( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int mana = level_base_mana( level ) * pMobIndex->mana_percent / 100;
    return mana;
}

// moves

int level_base_move( int level )
{
    return (5 + level) * (10 + level * 1/4);
}

int mob_base_move( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int move = level_base_move( level ) * pMobIndex->move_percent / 100;
    return move;
}

// armor

int level_base_ac( int level )
{
    return level * -6;
}

int mob_base_ac( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int ac = level_base_ac( level ) * pMobIndex->ac_percent / 100;
    return 100 + ac;
}

// saves

int level_base_saves( int level )
{
    return -level/2;
}

int mob_base_saves( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int saves = level_base_saves( level ) * pMobIndex->saves_percent / 100;
    return saves;
}

// hitroll

int level_base_hitroll( int level )
{
    return level;
}

int mob_base_hitroll( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int hitroll = level_base_hitroll( level ) * pMobIndex->hitroll_percent / 100;
    return hitroll;
}

// damroll

int level_base_damroll( int level )
{
    return level;
}

int mob_base_damroll( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int damroll = level_base_damroll( level ) * pMobIndex->damage_percent / 100;
    return damroll;
}

// damage

int level_base_damage( int level )
{
    return 2 + level;
}

int mob_base_damage( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int damage = level_base_damage( level ) * pMobIndex->damage_percent / 100;
    return damage;
}

// number of attacks (in percent)

int level_base_attacks( int level )
{
    return 100 + level * 5/3;
}

// note: this should match the calculation in mob_hit (fight.c)
int mob_base_attacks( MOB_INDEX_DATA *pMobIndex, int level )
{
    int attacks = level_base_attacks( level );
    if ( IS_SET(pMobIndex->act, ACT_STAGGERED) )
        attacks = UMAX(100, attacks/2);
    if ( IS_SET(pMobIndex->off_flags, OFF_FAST) )
        attacks = attacks * 3/2;
    if ( IS_SET(pMobIndex->affect_field, AFF_GUARD) )
        attacks -= 50;
    if ( IS_SET(pMobIndex->affect_field, AFF_HASTE) )
        attacks += 150;
    if ( IS_SET(pMobIndex->affect_field, AFF_SLOW) )
        attacks -= UMAX(0, attacks - 100) / 2;
    return attacks;
}
