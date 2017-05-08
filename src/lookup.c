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
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "tables.h"

const char *name_lookup(const int bit, const struct flag_type *flag_table)
{
    int i;

    for (i=0; flag_table[i].name != NULL; i++)
    {
        if (flag_table[i].bit == bit)
            return flag_table[i].name;
    }
    return NULL;
}

int index_lookup(const int bit, const struct flag_type *flag_table)
{
    int i;

    for (i=0; flag_table[i].name != NULL; i++)
    {
        if (flag_table[i].bit == bit)
            return i;
    }
    return -1;
}

int flag_lookup (const char *name, const struct flag_type *flag_table)
{
   int flag;
   
   for (flag = 0; flag_table[flag].name != NULL; flag++)
   {
      if (LOWER(name[0]) == LOWER(flag_table[flag].name[0])
         &&  !str_prefix(name,flag_table[flag].name))
         return flag_table[flag].bit;
   }
   
   return NO_FLAG;
}

int clan_lookup(const char *name)
{
   int clan;
   
   for (clan = 0; clan < MAX_CLAN; clan++)
   {
      if (clan_table[clan].active 
          && LOWER(name[0]) == LOWER(clan_table[clan].name[0])
          && !str_prefix(name,clan_table[clan].name))
         return clan;
   }
   
   return 0;
}

int clan_rank_lookup(sh_int clan, const char *name)
{
   int rank;

   /* accept numeric values */
   if ( is_number(name) )
   {
       rank = atoi(name);
       if ( IS_BETWEEN(1, rank, clan_table[clan].rank_count) )
	   return rank;
       else
	   return 0;
   }
   
   for (rank = 1; rank <= clan_table[clan].rank_count; rank++)
   {
      if (LOWER(name[0]) == LOWER(clan_table[clan].rank_list[rank].name[0])
         &&  !str_prefix(name,clan_table[clan].rank_list[rank].name))
         return rank;
   }
   
   return 0;
}

int penalty_table_lookup(const char *name)
{
   int pen;
   
   for (pen = 0; pen < MAX_PENALTY; pen++)
   {
      if (LOWER(name[0]) == LOWER(penalty_table[pen].name[0])
         &&  !str_prefix(name, penalty_table[pen].name))
         return pen;
   }
   
   return 0;
}


int position_lookup (const char *name)
{
   int pos;
   
   for (pos = 0; position_table[pos].name != NULL; pos++)
   {
      if (LOWER(name[0]) == LOWER(position_table[pos].name[0])
         &&  !str_prefix(name,position_table[pos].name))
         return pos;
   }
   
   return -1;
}

int sex_lookup (const char *name)
{
   int sex;
   
   for (sex = 0; sex_table[sex].name != NULL; sex++)
   {
      if (LOWER(name[0]) == LOWER(sex_table[sex].name[0])
         &&  !str_prefix(name,sex_table[sex].name))
         return sex;
   }
   
   return -1;
}

int size_lookup (const char *name)
{
   int size;
   
   for ( size = 0; size_table[size].name != NULL; size++)
   {
      if (LOWER(name[0]) == LOWER(size_table[size].name[0])
         &&  !str_prefix( name,size_table[size].name))
         return size;
   }
   
   return -1;
}


/* returns race number */
int race_lookup (const char *name)
{
   int race;
   
   for ( race = 0; race_table[race].name != NULL; race++)
   {
      if (LOWER(name[0]) == LOWER(race_table[race].name[0])
         &&  !str_prefix(name, race_table[race].name))
         return race;
   }
   
   return 0;
} 

/* returns pc race number */
int pc_race_lookup (const char *name)
{
    int race;
    
    for ( race = 0; pc_race_table[race].name != NULL; race++)
    {
        if (LOWER(name[0]) == LOWER(pc_race_table[race].name[0])
           &&  !str_prefix(name, pc_race_table[race].name))
           return race;
    }
    
    return 0;
}

/* returns race number */
int subclass_lookup (const char *name)
{
    int subclass;
   
    for ( subclass = 1; subclass_table[subclass].name != NULL; subclass++)
    {
        if ( LOWER(name[0]) == LOWER(subclass_table[subclass].name[0])
            && !str_prefix(name, subclass_table[subclass].name) )
            return subclass;
    }

    return 0;
} 

/* returns alignment */
int align_lookup (const char *name)
{
    int align;

    for ( align = 0; align_table[align].name != NULL; align++)
    {
        if (LOWER(name[0]) == LOWER(align_table[align].name[0])
        && !str_prefix(name, align_table[align].name))
        {
        align = align_table[align].align;
        return align;
        }
    }

    return 0;
}

int item_lookup(const char *name)
{
   int type;
   
   for (type = 0; item_table[type].name != NULL; type++)
   {
      if (LOWER(name[0]) == LOWER(item_table[type].name[0])
         &&  !str_prefix(name,item_table[type].name))
         return item_table[type].type;
   }
   
   return -1;
}

int liq_lookup (const char *name)
{
   int liq;
   
   for ( liq = 0; liq_table[liq].liq_name != NULL; liq++)
   {
      if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0])
         && !str_prefix(name,liq_table[liq].liq_name))
         return liq;
   }
   
   return -1;
}

int stance_lookup(const char *name)
{
   int stance = 0;

   while (stances[stance].name != NULL)
   {
      if (LOWER(name[0]) == LOWER(stances[stance].name[0])
         &&  !str_prefix(name,stances[stance].name))
         return stance;
      stance++;
   }
   
   return 0;
}
