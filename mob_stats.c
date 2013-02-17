#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include "merc.h"

// wealth

long level_base_wealth( int level )
{
    return (long)(level * level * (level / 15.0 + 2));
}

long mob_base_wealth( MOB_INDEX_DATA *pMobIndex )
{  
    int level = pMobIndex->level;
    float factor = 0.2;
    
    if ( pMobIndex->pShop != NULL )
    {
        level = 120;
        factor = 1.0;
    }
    factor = factor * pMobIndex->wealth_percent / 100;    

    return (long)(level_base_wealth(level) * factor);
}

// hitpoints

int level_base_hp( int level )
{
    return 100;
}

int mob_base_hp( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int hp = level_base_hp( level ) * pMobIndex->hitpoint_percent / 100;
    return hp;
}

// mana

int level_base_mana( int level )
{
    return 100;
}

int mob_base_mana( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int mana = level_base_mana( level ) * pMobIndex->mana_percent / 100;
    return mana;
}

// moves

int level_base_move( int level )
{
    return 100;
}

int mob_base_move( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int move = level_base_move( level ) * pMobIndex->move_percent / 100;
    return move;
}

// armor

int level_base_ac( int level )
{
    return 100;
}

int mob_base_ac( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int ac = level_base_ac( level ) * pMobIndex->ac_percent / 100;
    return ac;
}

// saves

int level_base_saves( int level )
{
    return 100;
}

int mob_base_saves( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int saves = level_base_saves( level ) * pMobIndex->saves_percent / 100;
    return saves;
}

// hitroll

int level_base_hitroll( int level )
{
    return 100;
}

int mob_base_hitroll( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int hitroll = level_base_hitroll( level ) * pMobIndex->hitroll_percent / 100;
    return hitroll;
}

// damroll

int level_base_damroll( int level )
{
    return 100;
}

int mob_base_damroll( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int damroll = level_base_damroll( level ) * pMobIndex->damage_percent / 100;
    return damroll;
}

// damage

int level_base_damage( int level )
{
    return 100;
}

int mob_base_damage( MOB_INDEX_DATA *pMobIndex, int level )
{    
    int damage = level_base_damage( level ) * pMobIndex->damage_percent / 100;
    return damage;
}
