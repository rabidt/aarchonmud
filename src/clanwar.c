/**************************************************************** 
 * "Clan Wars" by Brian Castle (aka "Rimbol"), written 11/1997. *
 * For use by Aarchon MUD, a ROM 2.4b4 based world.             *
 ****************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"

CLANWAR_DATA * clanwar_table;

CLANWAR_DATA *clanwar_lookup(sh_int clan_one, sh_int clan_two)
{
   CLANWAR_DATA *p;

   for (p = clanwar_table; p ; p = p->next)
   {
      if (p->clan_one == clan_one && p->clan_two == clan_two)
         return p;
   }

   return NULL;
}


void load_clanwars( void )
{
	FILE *fp;
	CLANWAR_DATA *p;
    const char *c1, *c2;
    int i,j;
   
	clanwar_table = NULL;
	
	fp = fopen (CLANWAR_FILE, "r");
	
	if (!fp)  /* If the file does not exist, no clans are at war. */ 
		return;
		
	c1 = str_dup(fread_word (fp));
	while (c1 && str_cmp(c1, "END"))
	{

      if ((i = clan_lookup(c1)) > 0)
      {
         if ((c2 = str_dup(fread_word (fp))))
         {
            if ((j = clan_lookup(c2)) > 0)
            {
               p = alloc_mem(sizeof(CLANWAR_DATA));
               p->clan_one       = i;
               p->clan_two       = j;
               p->initiator_name = str_dup(fread_word(fp));
               p->initiator_rank = fread_number(fp);
               p->initiator_rank = URANGE(1, p->initiator_rank, MAX_CLAN_RANK);
               p->truce_name     = str_dup(fread_word(fp));
               p->truce_timer    = fread_number(fp);
               p->status         = fread_number(fp);
               p->pkills         = fread_number(fp);
               p->next           = clanwar_table;
			
		      	clanwar_table = p;
            }
         }
         else
         {
            bugf ("Skipping unknown clan %s in " CLANWAR_FILE " file.(c2)", c2);
            fread_word(fp);   /* name */
            fread_number(fp); /* rank */
            fread_word(fp);   /* name */
            fread_number(fp); /* timer */
            fread_number(fp); /* status */
            fread_number(fp); /* pkills */

         }
         free_string(c2);
		}
      else
		{
			bugf ("Skipping unknown clan %s in " CLANWAR_FILE " file.(c1)", c1);
			fread_word(fp);   /* clan_two */
         fread_word(fp);   /* name */
         fread_number(fp); /* rank */
         fread_word(fp);   /* name */
         fread_number(fp); /* timer */
         fread_number(fp); /* status */
         fread_number(fp); /* pkills */
      }

      free_string(c1);
		c1 = str_dup(fread_word(fp));
	}
   free_string(c1);
	fclose (fp);		
}


void save_clanwars( void )
{
	FILE *fp;
	CLANWAR_DATA *p;
	
	if (!clanwar_table)
	{
		unlink (CLANWAR_FILE);
		return;
	}

	fp = fopen (CLANWAR_FILE, "w");
	
	if (!fp)
	{
		bug ("Could not open " CLANWAR_FILE " for writing",0);
		return;
	}
	
	for (p = clanwar_table; p ; p = p->next)
		fprintf (fp, "%s %s %s %d %s %d %d %d\n", 
         clan_table[p->clan_one].name,
         clan_table[p->clan_two].name, 
         p->initiator_name,
         p->initiator_rank, 
         p->truce_name,
         p->truce_timer,
         p->status,
         p->pkills);
		
	fprintf (fp, "END\n");
		
	fclose (fp);
}


void clanwar_status(CHAR_DATA *ch, sh_int clan_number)
{
   CLANWAR_DATA *p, *q;
   sh_int i;
   char pstat[20], qstat[20];

   if (IS_NPC(ch))
   {
      send_to_char("Not for NPC's.\n\r",ch);
      return;
   }

   if (!clan_table[clan_number].active)
   {
      send_to_char("That is not a clan.\n\r",ch);
      return;
   }

   printf_to_char(ch, "Clanwar Activity for Clan %s:\n\r\n\r",capitalize(clan_table[clan_number].name));
   
   for (i = 0; i < MAX_CLAN; i++)
   {
      if (clan_number == i)
         continue;

      if (!clan_table[i].active)
         continue;

      p = clanwar_lookup(clan_number, i);

      if (p)
      {
         switch (p->status) {
         case CLANWAR_WAR:    sprintf(pstat, "{rWAR(%d){x", p->pkills);    break;
         case CLANWAR_TRUCE:  
            if (p->truce_timer < 0)
               sprintf(pstat, "{rWAR(%d)-Tr?{x", p->pkills);
            else
               sprintf(pstat, "{yTruce(%d){x", abs(p->truce_timer));  
            break;
         case CLANWAR_TREATY: strcpy(pstat, "{gTreaty{x"); break;
         default: return; break;
         }
      }
      else
         strcpy(pstat, "{cPeace{x");

      q = clanwar_lookup(i, clan_number);

      if (q)
      {
         switch (q->status) {
         case CLANWAR_WAR:    sprintf(qstat, "{rWAR(%d){x", q->pkills);    break;
         case CLANWAR_TRUCE:  
            if (q->truce_timer < 0)
               sprintf(qstat, "{rWAR(%d)-Tr?{x", q->pkills);
            else
               sprintf(qstat, "{yTruce(%d){x", abs(q->truce_timer));  
            break;
         case CLANWAR_TREATY: strcpy(qstat, "{gTreaty{x"); break;
         default: return; break;
         }
      }
      else
         strcpy(qstat, "{cPeace{x");


      printf_to_char(ch, "%s -> ", lpad(capitalize(clan_table[clan_number].name), 12, ' '));
      printf_to_char(ch, "%s: ",   lpad(capitalize(clan_table[i].name), 12, ' '));
      printf_to_char(ch, "%s  ",   rpad(pstat, 12, ' '));
      printf_to_char(ch, "%s -> ", lpad(capitalize(clan_table[i].name), 12, ' '));
      printf_to_char(ch, "%s: ",   lpad(capitalize(clan_table[clan_number].name), 12, ' '));
      printf_to_char(ch, "%s\n\r", rpad(qstat, 12, ' '));
          

   }
}


/* This is presently only called to add a WAR or TREATY record.
   TRUCE modifies an existing WAR record, and PEACE is the lack of
   any clanwar record. */
void add_clanwar_node(CHAR_DATA *ch, sh_int other_clan, int status)
{
   CLANWAR_DATA *p;
   
   p = alloc_mem (sizeof(CLANWAR_DATA));
   p->clan_one = ch->clan;
   p->clan_two = other_clan;
   p->initiator_name = str_dup(ch->name);
   p->initiator_rank = ch->pcdata->clan_rank;
   p->truce_name = NULL;
   p->truce_timer = 0;
   p->status = status;
   p->pkills  = 0;
   p->next = clanwar_table;
   clanwar_table = p;
}



int delete_clanwar_node(CLANWAR_DATA *node)
{
   if (node == clanwar_table)
   {
      clanwar_table = clanwar_table->next;
      free_string(node->initiator_name);
      free_string(node->truce_name);
      free_mem(node, sizeof(CLANWAR_DATA));
      node = NULL;
      return(1);
   }
   else
   {
      CLANWAR_DATA *p;

      for (p = clanwar_table; p->next; p = p->next)
      {
         if (p->next == node)
         {
            p->next = node->next;
            free_string(node->initiator_name);
            free_string(node->truce_name);
            free_mem(node, sizeof(CLANWAR_DATA));
            node = NULL;
            return(1);
         }
      }
   }
   return(0);
}


/*  ==== This is delicate code - modify at your own risk ==== */
DEF_DO_FUN(do_clanwar)
{
   
   char arg1[MAX_STRING_LENGTH];
   char arg2[MAX_STRING_LENGTH];
   char arg3[MAX_STRING_LENGTH];
   sh_int clan_number = 0;
   CLANWAR_DATA *p, *q;
   sh_int pstatus; /* Current status of ch's clan toward other clan */
   sh_int qstatus; /* Current status of other clan toward ch's clan */
   sh_int dstatus; /* Desired status of ch's clan toward other clan */
   
   if (IS_NPC(ch))
   {
      send_to_char("Not for NPC's.\n\r",ch);
      return;
   }
   
   argument = one_argument(argument, arg1);
   argument = one_argument(argument, arg2);
   
   if (arg1[0] == '\0')  /* Just show status */
   {
      clanwar_status(ch, ch->clan);
      return;
   }

   if ((clan_number = clan_lookup(arg1)) <= 0)
   {
      send_to_char("That is not a valid clan.\n\r",ch);
      return;
   }
   
   if (!clan_table[clan_number].active)
   {
      send_to_char("That is not a valid clan.\n\r",ch);
      return;
   }

   if (!strcmp(arg2, "status"))
   {
      clanwar_status(ch, clan_number);
      return;
   }

   if (!clan_table[ch->clan].active)
   {
      send_to_char("You are not in a clan.\n\r",ch);
      return;
   }

   if (ch->clan == clan_number)
   {
      send_to_char("Alas, you cannot have a clanwar with yourself.\n\r",ch);
      return;
   }
   
   if (!strcmp(arg2, "war"))
      dstatus = CLANWAR_WAR;
   else if (!strcmp(arg2, "truce"))
      dstatus = CLANWAR_TRUCE;
   else if (!strcmp(arg2, "peace"))
      dstatus = CLANWAR_PEACE;
   else if (!strcmp(arg2, "treaty"))
      dstatus = CLANWAR_TREATY;
   else
   {
      send_to_char("Valid arguments are: status, war, truce, peace, and treaty.\n\r",ch);
      return;
   }

   p = clanwar_lookup(ch->clan, clan_number);
   pstatus = p ? p->status : CLANWAR_PEACE;
   
   q = clanwar_lookup(clan_number, ch->clan);
   qstatus = q ? q->status : CLANWAR_PEACE;

   /* NOTE:  Understand how this works before changing it.  This was built
             using a complex state transition diagram, and every permutation
             was individually tested. -Rim */

   switch (dstatus) {
   default:
      break;
   case CLANWAR_WAR:  /* Player wants to declare a war */
      {
         if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_declare_war)
         {
            send_to_char("You do not have the authority to declare clan wars.\n\r",ch);
            return;
         }
         
         switch (pstatus) {
         default:
            break;
         case CLANWAR_WAR:
            {
               switch (qstatus) {
               case CLANWAR_WAR:
               case CLANWAR_TRUCE:
                  printf_to_char(ch, "Your clan is already at war with clan %s.\n\r",
                     capitalize(clan_table[clan_number].name));
                  break;
               case CLANWAR_PEACE:
                  printf_to_char(ch, "Your clan has already declared war upon clan %s.\n\r",
                     capitalize(clan_table[clan_number].name));
                  break;
               case CLANWAR_TREATY:
               default: 
                  break;
               }
            }
            break;
            
         case CLANWAR_TRUCE:
            {
               switch (qstatus) {
               case CLANWAR_WAR:
                  p->status = CLANWAR_WAR;
                  p->truce_name = NULL;
                  p->truce_timer = 0;
                  sprintf(log_buf, "Clan %s is no longer seeking a truce with clan %s!\n\r",
                     capitalize(clan_table[ch->clan].name), 
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
               case CLANWAR_TRUCE:
                  p->status = CLANWAR_WAR;
                  p->truce_name = NULL;
                  p->truce_timer = 0;
                  q->status = CLANWAR_WAR;
                  q->truce_timer = 0;
                  q->truce_name = NULL;
                  sprintf(log_buf, "Clan %s has ended the truce with clan %s!\n\r",
                     capitalize(clan_table[ch->clan].name), 
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
               case CLANWAR_PEACE:
               case CLANWAR_TREATY:
               default:
                  break;
               }
            }
            break;
            
         case CLANWAR_PEACE:
            {
               switch(qstatus) {
               case CLANWAR_PEACE:
               case CLANWAR_WAR:
                  add_clanwar_node(ch, clan_number, CLANWAR_WAR);
                  
                  sprintf(log_buf, "Clan %s has declared WAR on clan %s!", 
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
               case CLANWAR_TREATY:
                  add_clanwar_node(ch, clan_number, CLANWAR_WAR);
                  delete_clanwar_node(q);  /* Set other clan to PEACE */
                  
                  sprintf(log_buf, "Clan %s has declared WAR on clan %s!", 
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
                  
               case CLANWAR_TRUCE:
               default:
                  break;
               }
            }
            break;
            
         case CLANWAR_TREATY:
            {
               switch(qstatus) {
                  
               case CLANWAR_PEACE:
                  if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_declare_treaty)
                  {
                     printf_to_char(ch, "Your clan is seeking a peace treaty with clan %s. You must have treaty authority in order to abort this.\n\r",
                        capitalize(clan_table[clan_number].name));
                     return;
                  }
                  
                  p->status = CLANWAR_WAR;
                  
                  sprintf(log_buf, "Clan %s has declared WAR on clan %s!", 
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
                  
               case CLANWAR_TREATY:
                  if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_declare_treaty)
                  {
                     printf_to_char(ch, "Your clan has a peace treaty with clan %s. You must have treaty authority in order to violate it.\n\r",
                        capitalize(clan_table[clan_number].name));
                     return;
                  }
                  p->status = CLANWAR_WAR;
                  
                  delete_clanwar_node(q);
                  
                  sprintf(log_buf, "Clan %s has violated their peace treaty and declared WAR on clan %s!", 
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
                  
               case CLANWAR_WAR:
               case CLANWAR_TRUCE:
               default:
                  break;
               }
            }
            break;
         }
      }
      break;
   case CLANWAR_PEACE:
      {
         if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_declare_war)
         {
            send_to_char("You do not have the authority to declare peace.\n\r",ch);
            return;
         }
         
         switch (pstatus) {
         default:
            break;
         case CLANWAR_TRUCE:
         case CLANWAR_WAR:
            {
               switch (qstatus) {
               case CLANWAR_TRUCE:  /* Set Q to WAR, flow thru to below */
                  free_string(q->truce_name);
                  q->truce_name = NULL;
                  q->truce_timer = 0;
                  q->status = CLANWAR_WAR;
               case CLANWAR_WAR:
                  delete_clanwar_node(p);
                  sprintf(log_buf, "Clan %s has withdrawn their declaration of war upon clan %s!\n\r",
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
               case CLANWAR_PEACE:
                  delete_clanwar_node(p);
                  sprintf(log_buf, "Clan %s is now at peace with clan %s!\n\r",
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
               case CLANWAR_TREATY:
               default:
                  break;
               }
            }
            break;
         case CLANWAR_PEACE:
            {
               switch (qstatus) {
               case CLANWAR_PEACE:
               case CLANWAR_TREATY:
               case CLANWAR_WAR:
                  printf_to_char(ch, "%s is not at war with %s.\n\r", 
                     capitalize(clan_table[ch->clan].name), 
                     capitalize(clan_table[clan_number].name));
                  break;
               case CLANWAR_TRUCE:
               default:
                  break;
               }
            }
            break;
         case CLANWAR_TREATY:
            {
               switch (qstatus) {
               case CLANWAR_TREATY:
                  if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_declare_treaty) 
                  {
                     printf_to_char(ch, "Your clan has a peace treaty with clan %s. You must have treaty authority in order to end it.\n\r",
                        capitalize(clan_table[clan_number].name));
                     return;
                  }

                  delete_clanwar_node(p);
                  delete_clanwar_node(q);

                  sprintf(log_buf, "Clan %s has ended their peace treaty with clan %s!\n\r",
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;

               case CLANWAR_PEACE:
                  if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_declare_treaty) 
                  {
                     printf_to_char(ch, "Your clan is seeking a peace treaty with clan %s. You must have treaty authority in order to abort this.\n\r",
                        capitalize(clan_table[clan_number].name));
                     return;
                  }

                  delete_clanwar_node(p);

                  sprintf(log_buf, "Clan %s is no longer seeking a peace treaty with clan %s.\n\r",
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;

               case CLANWAR_WAR:
               case CLANWAR_TRUCE:
               default:
                  break;
               }
            }
            break;
         }
      }
      break;
   case CLANWAR_TRUCE:
      {
         int hours;

         if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_declare_truce)
         {
            send_to_char("You do not have the authority to declare a truce.\n\r",ch);
            return;
         }
         
         argument = one_argument(argument, arg3);
         
         if ( is_number( arg3 ) )
         {
            hours = atoi(arg3);
            
            if ( hours < 20 || hours > 1000)
            {
               send_to_char("A truce may be between 20 and 1000 game hours.\n\r",ch);
               return;
            }
         }
         else
         {
            send_to_char("You must enter the number of hours (between 20 and 1000) for the truce.\n\r",ch);
            return;
         }
         
         
         switch (pstatus) {
         default:
            break;
         case CLANWAR_WAR:
            {
               switch (qstatus) {
               case CLANWAR_WAR:
                  p->status      = CLANWAR_TRUCE;
                  p->truce_name  = ch->name;
                  p->truce_timer = 0 - hours;

                  sprintf(log_buf, "Clan %s is seeking a truce with clan %s for %d hours.",
                     capitalize(clan_table[ch->clan].name), 
                     capitalize(clan_table[clan_number].name),
                     hours);

                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
               case CLANWAR_TRUCE:
                  p->status      = CLANWAR_TRUCE;
                  p->truce_name  = ch->name;
                  p->truce_timer = UMIN(hours, abs(q->truce_timer));
                  
                  q->truce_timer = p->truce_timer;

                  sprintf(log_buf, "Clans %s and %s are now at truce for %d hours.",
                     capitalize(clan_table[ch->clan].name), 
                     capitalize(clan_table[clan_number].name),
                     p->truce_timer);

                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
                  
               case CLANWAR_PEACE:
                  printf_to_char(ch, "Clan %s has not (yet) declared war upon clan %s.\n\r"
                     "If you would like a cease-fire, you will need to make peace with them.\n\r",
                     capitalize(clan_table[clan_number].name),
                     capitalize(clan_table[ch->clan].name));
                  break;
               case CLANWAR_TREATY:
               default:
                  break;
               }
            }
            break;

         case CLANWAR_TRUCE:
            {
               switch (qstatus) {
               case CLANWAR_TRUCE:
                  printf_to_char(ch, "Clan %s is already at truce with clan %s.\n\r",
                     capitalize(clan_table[clan_number].name),
                     capitalize(clan_table[ch->clan].name));
                  break;
               case CLANWAR_WAR:
                  printf_to_char(ch, "Clan %s is already seeking a truce with clan %s for %d hours.\n\r",
                     capitalize(clan_table[clan_number].name),
                     capitalize(clan_table[ch->clan].name),
                     abs(p->truce_timer));
                  break;
               case CLANWAR_PEACE:
               case CLANWAR_TREATY:
               default:
                  break;
               }
            }
            break;

         case CLANWAR_PEACE:
         case CLANWAR_TREATY:
            printf_to_char(ch, "Clan %s is not at war with clan %s.\n\r",
               capitalize(clan_table[ch->clan].name),
               capitalize(clan_table[clan_number].name));
            break;
         }
      }
      break;

   case CLANWAR_TREATY:
      {
         if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_declare_treaty)
         {
            send_to_char("You do not have the authority to make peace treaties.\n\r",ch);
            return;
         }

         switch (pstatus) {
         default:
            break;
         case CLANWAR_TRUCE:
         case CLANWAR_WAR:
            printf_to_char(ch, "You need to be at peace with %s before you may seek out a peace treaty with them.\n\r",
               capitalize(clan_table[clan_number].name));
            break;
         case CLANWAR_PEACE:
            {
               switch (qstatus) {
               case CLANWAR_TRUCE:
               case CLANWAR_WAR:
                  printf_to_char(ch, "You need to be at peace with %s before you may seek out a peace treaty with them.\n\r",
                     capitalize(clan_table[clan_number].name));
                  break;
               case CLANWAR_PEACE:
                  add_clanwar_node(ch, clan_number, CLANWAR_TREATY);
                  
                  sprintf(log_buf, "Clan %s is seeking a peace treaty with clan %s.\n\r", 
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));

                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
               case CLANWAR_TREATY:
                  add_clanwar_node(ch, clan_number, CLANWAR_TREATY);

                  sprintf(log_buf, "Clans %s and %s have signed a peace treaty!\n\r", 
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));

                  info_message(ch, log_buf, TRUE);
                  save_clanwars();
                  break;
               default:
                  break;
               }
            }
            break;
         case CLANWAR_TREATY:
            {
               switch (qstatus) {
               case CLANWAR_PEACE:
                  printf_to_char(ch, "Clan %s is already seeking out a peace treaty clan %s.\n\r",
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  break;
               case CLANWAR_TREATY:
                  printf_to_char(ch, "Clan %s has already signed a peace treaty with clan %s.\n\r",
                     capitalize(clan_table[ch->clan].name),
                     capitalize(clan_table[clan_number].name));
                  break;
                  
                  
               case CLANWAR_WAR:
               case CLANWAR_TRUCE:
               default:
                  break;
               }
            }
            break;
         }
      }
   }
}      


bool is_clanwar_opp( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CLANWAR_DATA *p, *q;
            
    p = clanwar_lookup(ch->clan, victim->clan);
    q = clanwar_lookup(victim->clan, ch->clan);
           
    /* not needed 
    if ( p && p->status == CLANWAR_TREATY
	 && q && q->status == CLANWAR_TREATY )
	return FALSE;
    */

    /* If either clan is at war with the other, and both players are of
       sufficient rank to be a "clanwar pkiller", the attack is valid */
    if ( ((p && p->status == CLANWAR_WAR) || (q && q->status == CLANWAR_WAR))
	 && clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].clanwar_pkill
	 && clan_table[victim->clan].rank_list[victim->pcdata->clan_rank].clanwar_pkill)
	return TRUE;
    else
	return FALSE;
}






