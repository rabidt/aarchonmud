/* 
 * Passive Skills by Astark - crimsonsage@gmail.com - June 2012
 * This file gets called from update.c, in which each tick, a
 * check is done to see if players have any of the passive skils
 * listed in this file. If they do, the bonuses are applied
 */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include "magic.h"
#include "lookup.h"


// void  passive_update args( (CHAR_DATA *ch) );


void passive_update(CHAR_DATA *ch) 
{   

    if ( get_skill(ch, gsn_natural_resistance) >= 0)
        {
            AFFECT_DATA *af;
            affect_strip (ch, gsn_natural_resistance);
        }
            if ( get_skill(ch, gsn_natural_resistance) > 0)
        {   
            AFFECT_DATA af;
            af.where    = TO_AFFECTS;
            af.type     = gsn_natural_resistance;
            af.level    = ch->level;
  	    af.location = APPLY_SAVES;
            af.duration = -1;
            af.modifier = get_skill(ch, gsn_natural_resistance) / -10;
            affect_to_char(ch,&af);
        }
} 