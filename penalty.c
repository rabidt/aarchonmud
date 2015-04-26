/*********************************************************************** 
 * New penalty system by Brian Castle (aka "Rimbol"), written 01/1999. *
 * For use by Aarchon MUD, a ROM 2.4b4 based world.                    *
 ***********************************************************************/

#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"
#include "recycle.h"


/* Local Prototypes */
bool process_penalty( CHAR_DATA *ch, const char *argument, const char *pentype );
char *penalty_status_name(int status);
void delete_penalty_node(PENALTY_DATA *node);
PENALTY_DATA *new_penalty(CHAR_DATA *imm, CHAR_DATA *victim);
void show_penalty_type(CHAR_DATA *ch, const char *penname);
void check_penlist();
void load_crime_list(void);
void save_crime_list(void);


/* Globals */
PENALTY_DATA *penalty_list;
CRIME_DATA *crime_list;
DECLARE_DO_FUN(do_look);


char *penalty_status_name(int status)
{
    static char buf[20];

    buf[0] = '\0';

    switch (status) 
    {
       case PENALTY_STATUS_PENDING:        strcat(buf, " Pending"); break;
       case PENALTY_STATUS_SERVING:        strcat(buf, " Serving"); break;
       case PENALTY_STATUS_PAROLE_PENDING: strcat(buf, " Parole Pending"); break;
       case PENALTY_STATUS_PARDON_PENDING: strcat(buf, " Pardon Pending"); break;
       case PENALTY_STATUS_PAROLED:        strcat(buf, " Paroled"); break;
       case PENALTY_STATUS_PARDONED:       strcat(buf, " Pardoned"); break;
       case PENALTY_STATUS_COMPLETE:       strcat(buf, " Complete"); break;
       case PENALTY_STATUS_NONE:
       default:
           break;
    }

	return ( buf[0] != '\0' ) ? buf+1 : "none";
}

DEF_DO_FUN(do_penlist)
{
    char arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
        show_penalties_by_player(ch, NULL, 0, 3);
    else if (!strcmp(arg, "cleanup"))
        check_penlist();
    else
        send_to_char("Syntax:  penlist           - Show penalty list\n\r"
                     "         penlist cleanup   - Remove deleted chars from list\n\r",ch);
}

DEF_DO_FUN(do_nochannel)
{
    process_penalty(ch, argument, "nochannel");
}

DEF_DO_FUN(do_noemote)
{
    process_penalty(ch, argument, "noemote");
}

DEF_DO_FUN(do_noshout)
{
    process_penalty(ch, argument, "noshout");
}

DEF_DO_FUN(do_notell)
{
    process_penalty(ch, argument, "notell");
}

DEF_DO_FUN(do_freeze)
{
    process_penalty(ch, argument, "freeze");
}

DEF_DO_FUN(do_jail)
{
    process_penalty(ch, argument, "jail");
}

DEF_DO_FUN(do_parole)
{
    process_penalty(ch, argument, "parole");
}

DEF_DO_FUN(do_pardon)
{
    process_penalty(ch, argument, "pardon");
}

DEF_DO_FUN(do_nonote)
{
    process_penalty(ch, argument, "nonote");
}

/* Check if another imm is already penalising victim to prevent
 * dealing out penalties twice --Bobble
 */
bool penalty_handled( CHAR_DATA *ch, CHAR_DATA *victim, const char *pentype )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *wch;
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	wch = d->original != NULL ? d->original : d->character;
	if ( wch != NULL 
	     && wch->pcdata != NULL 
	     && wch->pcdata->new_penalty != NULL
	     && !str_cmp(wch->pcdata->new_penalty->victim_name, victim->name)
	     && !str_cmp(wch->pcdata->new_penalty->penalty_type, pentype))
	{
	    act( "$N is already handing out this penalty.", ch, NULL, wch, TO_CHAR );
	    return TRUE;
	}
    }
    return FALSE;
}

/* Apply penalty to character.  
If applied successfully return TRUE, else return FALSE. */
bool process_penalty( CHAR_DATA *ch, const char *argument, const char *pentype )
{
    sh_int pen;
    char arg[MIL];
    char buf[MSL];
    bool online;
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    PENALTY_DATA *p;
    
    d = NULL;
    victim = NULL;

    if (pentype[0] == '\0')
    {
        bug("process_penalty: missing penalty type",0);
        return FALSE;
    }

    pen = penalty_table_lookup(pentype);

    if (pen <= 0 && !strcmp(pentype, "parole") && !strcmp(pentype, "pardon"))
    {
        send_to_char("Command failed, check syntax.\n\r",ch);
        return FALSE;
    }

    if ( IS_NPC(ch) ) /* "Switch fido" followed by "freeze fido" would be 
                         ignorant, but possible.  This saves the poor fools. -Rim */
    {
        send_to_char("Return first.\n\r",ch);
        return FALSE;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Supply your victim's name.", ch );
        return FALSE;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        d = new_descriptor();
        if ( !load_char_obj(d, arg, TRUE) )
        {
            send_to_char("Character not found.\n\r", ch);
            free_char(d->character);
            free_descriptor(d);
            return FALSE;
        }
        victim = d->character;
        online = FALSE;
    }
    else
        online = TRUE;

    if ( IS_NPC(victim) && penalty_table[pen].bit == PENALTY_JAIL )
    {
        send_to_char("You cannot jail NPC's.\n\r",ch);
        if (!online) { free_char(victim); free_descriptor(d); }
        return FALSE;
    }

    if ( IS_NPC(victim) && victim->desc != NULL )
    {
        send_to_char( "That NPC is in use at the moment.\n\r", ch );
        if (!online) { free_char(victim); free_descriptor(d); }
        return FALSE;
    }
    
    if (!IS_NPC(victim) && get_trust( victim ) >= get_trust( ch ) )
    {
        send_to_char( "Fortunately, you failed.\n\r", ch );
        if (!online) { free_char(victim); free_descriptor(d); }
        return FALSE;
    }


    if (!strcmp(pentype, "parole") || !strcmp(pentype, "pardon")) 
    {
        if (IS_NPC(victim))
        {
            send_to_char("Use the penalty command again to remove penalties from NPC's.\n\r",ch);

            if (!online) { free_char(victim); free_descriptor(d); }
            return FALSE;
        }

	/* check if someone else already handles this penalty */
	if ( penalty_handled(ch, victim, "parole")
	     || penalty_handled(ch, victim, "pardon") )
	{
	    if (!online) { free_char(victim); free_descriptor(d); }
	    return FALSE;
	}

        if (show_penalties_by_player(ch, victim->name, TIME_PLAYED(victim), 1)) /* PC */
        {
            ch->pcdata->new_penalty = new_penalty(ch, victim);
    
            /* I'm misusing the new_penalty structure a bit here to pass parameters to the next CON state. */            
            if (!strcmp(pentype, "parole"))
                ch->pcdata->new_penalty->status = PENALTY_STATUS_PAROLE_PENDING;
            else
                ch->pcdata->new_penalty->status = PENALTY_STATUS_PARDON_PENDING;

            ch->pcdata->new_penalty->penalty_type = str_dup(pentype);
            ch->pcdata->new_penalty->duration = TIME_PLAYED(victim);
            ch->pcdata->new_penalty->changed_by = str_dup(ch->name);
            ch->pcdata->new_penalty->changed_time = current_time;

            ch->desc->connected = CON_PENALTY_PENLIST;  /* Nanny's turn */

            if (!online) { free_char(victim); free_descriptor(d); }
            return TRUE;
        }
        else
        {
            printf_to_char(ch, "%s has no penalties to %s.\n\r", capitalize(victim->name), pentype);
            if (!online) { free_char(victim); free_descriptor(d); }
            return FALSE;
        }
    }
    else  /* Seeking to apply a new penalty */
    {
        if (IS_NPC(victim))  /* With NPC's, the penalty commands work as before */
        {
            if ( IS_SET(victim->penalty, penalty_table[pen].bit) )
            {
                REMOVE_BIT(victim->penalty, penalty_table[pen].bit);
                
                printf_to_char(ch, "%s is no longer %s.\n\r", PERS(victim, ch), penalty_table[pen].apply_string);
                sprintf(buf,"%s: %s is no longer %s.", capitalize(ch->name), PERS(victim, ch), penalty_table[pen].apply_string);
                wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
            }
            else
            {
                SET_BIT(victim->penalty, penalty_table[pen].bit);
                
                printf_to_char(ch, "%s has been %s.\n\r", PERS(victim, ch), penalty_table[pen].apply_string);
                sprintf(buf,"%s has %s %s.", capitalize(ch->name), penalty_table[pen].apply_string, PERS(victim, ch));
                wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
            }                
            
            if (!online) { free_char(victim); free_descriptor(d); }
            return TRUE;
        }    
        
        for (p = penalty_list; p; p = p->next) /* PC */
        {
            if (!str_cmp(p->victim_name, victim->name) && !str_cmp(p->penalty_type, pentype)
                && p->status != PENALTY_STATUS_PARDON_PENDING
                && p->status != PENALTY_STATUS_PAROLE_PENDING)
            {
                printf_to_char(ch, "That player is already %s.\n\r", penalty_table[pen].apply_string);
                if (!online) { free_char(victim); free_descriptor(d); }
                return FALSE;
            }
        }

	/* check if someone else already handles this penalty */
	if ( penalty_handled( ch, victim, pentype ) )
	{
	    if (!online) { free_char(victim); free_descriptor(d); }
	    return FALSE;
	}
        
        ch->pcdata->new_penalty = new_penalty(ch, victim);
        
        ch->pcdata->new_penalty->penalty_type = str_dup(penalty_table[pen].name);
        ch->pcdata->new_penalty->status = PENALTY_STATUS_PENDING;
        
        ch->desc->connected = CON_PENALTY_SEVERITY;  /* Nanny's turn */
        
        show_penalty_type(ch, pentype);
        
        if (!online) { free_char(victim); free_descriptor(d); }
        return TRUE;
    }
    
    if (!online) 
    { 
        free_char(victim); 
        free_descriptor(d); 
    }

    return TRUE;
    
}

/* Display info about a type of penalty */
void show_penalty_type(CHAR_DATA *ch, const char *penname)
{
    int i, pen;

    pen = penalty_table_lookup(penname);

    if (!pen) 
        return;

    for (i=0; i < MAX_PENALTY_SEVERITY; i++)
    {
        if (ch->level < penalty_table[pen].sev_level[i])
            break;

        printf_to_char(ch, "{+%d{x. ", i+1);
        
        if (penalty_table[pen].sev_points[i] == -1)        
            printf_to_char(ch, "Points: {CSpecify{x  ");
        else
            printf_to_char(ch, "Points: {c%d{x  ", penalty_table[pen].sev_points[i]);

        if (penalty_table[pen].sev_duration[i] == -1)
            send_to_char("Hours: {CSpecify{x\n\r",ch);
        else if (penalty_table[pen].sev_duration[i] == -2)
            send_to_char("Hours: {YNo Expiration{x\n\r",ch);
        else
            printf_to_char(ch, "Hours: {c%.1f{x\n\r", (float)penalty_table[pen].sev_duration[i]/3600.0);
    }

    printf_to_char(ch, "{+Choose a %s severity:  {x\n\r", penname, i);
}


/* Format 1 = numbered list (for pardon and parole), 
   Format 2 = suitable for players (i.e. do_score),
   Format 3 = show all penalties as a report (i.e. do_penlist),
   If no penalties are found for this player, return will be 0. */

int show_penalties_by_player(CHAR_DATA *ch, const char *victim_name, int victim_played, int format)
{
    PENALTY_DATA *p;
    int i = 0, pen;
    
    if (format == 3)
        send_to_char("{+Active Penalty Report:\n\r----------------------{x\n\r",ch);

    for (p = penalty_list; p; p = p->next)
    {
        if (format == 3 || !str_cmp(p->victim_name, victim_name))
        {
            pen = penalty_table_lookup(p->penalty_type);

            if (format == 1)
            {
                /* Ignore pending parole/pardon so we can't re-parole/re-pardon them */
                if (   p->status != PENALTY_STATUS_PARDON_PENDING
                    && p->status != PENALTY_STATUS_PAROLE_PENDING)
                {
                    i++;
                    printf_to_char(ch, "{+%d{x. {r%s{x by {c%s{x, severity {c%d{x, {c%d{x points",
                        i,
                        p->penalty_type,
                        capitalize(p->imm_name),
                        p->severity + 1,
                        p->points);
                    
                    if (p->duration < 0)
                        send_to_char(" {Y--> No Expiration{x\n\r",ch);
                    else
                        printf_to_char(ch, ", {C%.1f{x hours ({Y%.1f{x remaining)\n\r",
                        (float)p->duration / 3600.0,
                        (float)(p->start_time + p->duration - victim_played) / 3600.0);
                }
            }
            else if (format == 2)
            {
                i++;
                if (p->duration < 0)
                    printf_to_char(ch, "You are {R%s{x indefinitely.\n\r",
                       penalty_table[pen].apply_string);
                else
                    printf_to_char(ch, "You are {R%s{x for {C%.1f{x remaining hours.\n\r",
                       penalty_table[pen].apply_string,
                       (float)(p->start_time + p->duration - victim_played) / 3600.0);
            }
            else if (format == 3)
            {
                i++;
                printf_to_char(ch, "{+%d{x. {c%s{x: {r%s{x of {c%s{x by {c%s{x, severity {c%d{x, {c%d{x points",
                   i,
                   penalty_status_name(p->status),
                   p->penalty_type,
                   capitalize(p->victim_name),
                   capitalize(p->imm_name),
                   p->severity + 1,
                   p->points);

                if (p->duration < 0)
                   send_to_char(" {Y--> No Expiration{x",ch);
                else
                   printf_to_char(ch, ", {C%.1f{x hours",
                       (float)p->duration / 3600.0);

                printf_to_char(ch, "\n\r    Imposed {x%s{x",
                    (char *)ctime(&p->imposed_time));

            }
        }
    }

    if (format == 1)
        printf_to_char(ch, "{+Choose an active penalty for {c%s{x{+:{x  \n\r", capitalize(victim_name));
    else if (format == 3 && i == 0)
        send_to_char("  {c--No Active Penalties{x\n\r",ch);

    return i;
}


/* Load from disk.  Called from boot_db(). */
void load_penalties()
{
    FILE *fp;
    PENALTY_DATA *p;
    const char *ctmp;
    
    penalty_list = NULL;
    
    if ((fp = fopen (PENALTY_FILE, "r")) == NULL)  /* No penalties to load. */
        return;
    
    ctmp = str_dup(fread_word(fp));
    
	while (ctmp && str_cmp(ctmp, "END"))
    {
        p = alloc_mem(sizeof(PENALTY_DATA));
        
        p->victim_name    = ctmp;
        p->victim_level   = fread_number(fp);
        p->imm_name       = str_dup(fread_word(fp));
        p->imm_level      = fread_number(fp);
        p->penalty_type   = str_dup(fread_word(fp));
        p->severity       = fread_number(fp);
        p->status         = fread_number(fp);
        p->imposed_time   = fread_number(fp);
        p->start_time     = fread_number(fp);
        p->duration       = fread_number(fp);
        p->points         = fread_number(fp);
        p->jail_room      = fread_number(fp);
        p->changed_by     = str_dup(fread_word(fp));
        p->changed_time   = fread_number(fp);
        p->text           = NULL;
        
        p->next           = penalty_list;
        
        penalty_list = p;
        
        ctmp = str_dup(fread_word(fp));
    }

    free_string(ctmp);
    fclose (fp);		

    do_penlist(NULL, "cleanup");

}

/* Save changes to disk */
void save_penalties()
{
    FILE *fp;
    PENALTY_DATA *p;
    
    if (!penalty_list)
    {
        unlink (PENALTY_FILE);
        return;
    }
    
    fp = fopen (PENALTY_FILE, "w");
    
    if (!fp)
    {
        bug ("Could not open " PENALTY_FILE " for writing.",0);
        return;
    }
    
    for (p = penalty_list; p ; p = p->next)
        fprintf (fp, "%s %d %s %d %s %d %d %ld %ld %ld %d %d %s %ld\n", 
           p->victim_name,
           p->victim_level,
           p->imm_name,
           p->imm_level,
           p->penalty_type,
           p->severity,
           p->status,
           p->imposed_time,
           p->start_time,
           p->duration,
           p->points,
           p->jail_room,
           p->changed_by,
           p->changed_time);
    
    fprintf (fp, "END\n");  /* Sentinel value */

    fclose (fp);
}


/* Allocate a new penalty node and return a pointer */
PENALTY_DATA *new_penalty(CHAR_DATA *imm, CHAR_DATA *victim)
{
    PENALTY_DATA *p;

    p = alloc_mem(sizeof(PENALTY_DATA));
    p->imm_name      = str_dup(imm->name);
    p->imm_level     = imm->level;
    p->victim_name   = str_dup(victim->name);
    p->victim_level  = victim->level;
    p->imposed_time  = current_time;
    p->penalty_type  = 0;
    p->severity      = 0;
    p->status        = 0;
    p->start_time    = TIME_PLAYED(victim);
    p->duration      = 0;
    p->points        = 0;
    p->jail_room     = 0;
    p->changed_by    = str_dup(imm->name);
    p->changed_time  = current_time;
    p->text          = NULL;
    
    p->next = NULL;

    return p;
}


/* Remove a node from the linked list and free memory */
void delete_penalty_node(PENALTY_DATA *node)
{
   if (node == penalty_list)
   {
       penalty_list = penalty_list->next;
       free_string(node->imm_name);
       free_string(node->victim_name);
       free_string(node->changed_by);
       free_string(node->penalty_type);
       free_string(node->text);
       free_mem(node, sizeof(PENALTY_DATA));
       node = NULL;
       return;
   }
   else
   {
       PENALTY_DATA *p;
       
       for (p = penalty_list; p->next; p = p->next)
       {
           if (p->next == node)
           {
               p->next = node->next;
               free_string(node->imm_name);
               free_string(node->victim_name);
               free_string(node->changed_by);
               free_string(node->penalty_type);
               free_string(node->text);
               free_mem(node, sizeof(PENALTY_DATA));
               node = NULL;
               return;
           }
       }
   }
}

/* Handle CON_PENALTY_SEVERITY state */
void penalty_severity( DESCRIPTOR_DATA *d, const char *argument )
{
   CHAR_DATA *ch = d->character;
   PENALTY_DATA *p = ch->pcdata->new_penalty;
   char buf[MAX_STRING_LENGTH];
   int pen, sev;
   
   if (!p)
   {
      d->connected = CON_PLAYING;
      bug ("penalty: In CON_PENALTY_SEVERITY, NULL ch->pcdata->new_penalty",0);
      return;
   }

   pen = penalty_table_lookup(p->penalty_type);   

   strcpy (buf, argument);

   if (!buf[0] || !is_number(buf))
   {
       show_penalty_type(ch, p->penalty_type);
       return;
   }
   
   sev = atoi(buf) - 1;

   if ( sev < 0 || sev >= MAX_PENALTY_SEVERITY ||
       ch->level < penalty_table[pen].sev_level[sev] ) 
   {
       show_penalty_type(ch, p->penalty_type);
       return;
   }

   p->severity = sev;
   p->duration = penalty_table[pen].sev_duration[sev];
   p->points   = penalty_table[pen].sev_points[sev];

   /* Toss to next state as appropriate */   
   if (p->duration == -1)    /* Specify */
   {
       send_to_char("Enter number of hours (1-25):\n\r",ch);
       d->connected = CON_PENALTY_HOURS;
   }
   else if (p->points == -1) /* Specify */
   {
       send_to_char("Enter number of demerit points (1-10):\n\r",ch);
       d->connected = CON_PENALTY_POINTS;
   }
   else
   {
       printf_to_char(ch, "This will {Y%s{x player {+%s{x ",
           p->penalty_type,
           p->victim_name);
       if (p->duration < 0)
           send_to_char("{Rindefinitely{x, ", ch);
       else
           printf_to_char(ch, "for {C%.1f{x hours, ",
              (float)p->duration / 3600.0);
       printf_to_char(ch, "adding {C%d{x demerit points. Proceed?\n\r",
           p->points);

       d->connected = CON_PENALTY_CONFIRM;
   }
}

/* Handle CON_PENALTY_HOURS state */
void penalty_hours( DESCRIPTOR_DATA *d, const char *argument )
{
    CHAR_DATA *ch = d->character;
    PENALTY_DATA *p = ch->pcdata->new_penalty;
    char buf[MAX_STRING_LENGTH];
    int hours;
    
    if (!p)
    {
        d->connected = CON_PLAYING;
        bug ("penalty: In CON_PENALTY_HOURS, NULL ch->pcdata->new_penalty",0);
        return;
    }
    
    strcpy (buf, argument);
    
    if (!buf[0] || !is_number(buf))
    {
        send_to_char("Enter number of hours (1-25):\n\r",ch);
        return;
    }
    
    hours = atoi(buf);
    
    if ( hours <= 0 || hours > 25 ) 
    {
        send_to_char("Enter number of hours (1-25):\n\r",ch);
        return;
    }
    
    p->duration = hours * 3600;
    
    /* Toss to next CON state. */
    if (p->points == -1) /* Specify */
    {
        send_to_char("Enter number of demerit points (1-10):\n\r",ch);
        d->connected = CON_PENALTY_POINTS;
    }
    else
    {
        printf_to_char(ch, "This will {Y%s{x player {+%s{x ",
            p->penalty_type,
            p->victim_name);
        if (p->duration < 0)
            send_to_char("{Rindefinitely{x, ", ch);
        else
            printf_to_char(ch, "for {C%.1f{x hours, ",
            (float)p->duration / 3600.0);
        printf_to_char(ch, "adding {C%d{x demerit points. Proceed?\n\r",
            p->points);
        
        d->connected = CON_PENALTY_CONFIRM;
    }
}

/* Handle CON_PENALTY_POINTS state */
void penalty_points( DESCRIPTOR_DATA *d, const char *argument )
{
   CHAR_DATA *ch = d->character;
   PENALTY_DATA *p = ch->pcdata->new_penalty;
   char buf[MAX_STRING_LENGTH];
   int points;
   
   if (!p)
   {
      d->connected = CON_PLAYING;
      bug ("penalty: In CON_PENALTY_POINTS, NULL ch->pcdata->new_penalty",0);
      return;
   }

   strcpy (buf, argument);

   if (!buf[0] || !is_number(buf))
   {
       send_to_char("Enter number of demerit points (1-10):\n\r",ch);
       return;
   }
   
   points = atoi(buf);

   if ( points <= 0 || points > 10 ) 
   {
       send_to_char("Enter number of demerit points (1-10):\n\r",ch);
       return;
   }

   p->points = points;
   
   /* Toss to next CON state. */
   printf_to_char(ch, "This will {Y%s{x player {+%s{x ",
       p->penalty_type,
       p->victim_name);
   if (p->duration < 0)
       send_to_char("{Rindefinitely{x, ", ch);
   else
       printf_to_char(ch, "for {C%.1f{x hours, ",
       (float)p->duration / 3600.0);
   printf_to_char(ch, "adding {C%d{x demerit points. Proceed?\n\r",
       p->points);
   
   d->connected = CON_PENALTY_CONFIRM;
}

/* Handle CON_PENALTY_CONFIRM state */
void penalty_confirm( DESCRIPTOR_DATA *d, const char *argument )
{
   CHAR_DATA *ch = d->character;
   PENALTY_DATA *p = ch->pcdata->new_penalty;

   if (!p)
   {
      d->connected = CON_PLAYING;
      bug ("penalty: In CON_PENALTY_CONFIRM, NULL ch->pcdata->new_penalty",0);
      return;
   }

   if (tolower(argument[0]) == 'y')
   {
       send_to_char("{+Describe your reason for taking this action ({WTHIS IS NOT OPTIONAL{x{+):{x\n\r", ch);
       string_append(ch, &p->text);
       d->connected = CON_PENALTY_FINISH;
   }
   else
   {
       free_string(p->imm_name);
       free_string(p->victim_name);
       free_string(p->changed_by);
       free_string(p->penalty_type);
       free_string(p->text);
       free_mem(p, sizeof(PENALTY_DATA));
       ch->pcdata->new_penalty = NULL;
       
       send_to_char("Action aborted.\n\r",ch);
       d->connected = CON_PLAYING;
       return;
   }
}

bool can_remove_penalty( CHAR_DATA *ch, PENALTY_DATA *pen )
{
   /* only law imms can pardon/parole other imms' penalties */
   if ( strcmp(pen->imm_name, ch->name) && !IS_SET(ch->act, PLR_LAW) )
       return FALSE;

   return TRUE;
}

/* Note: Coming into this function, p->duration contains victim->playing. 
         This is just a hack to pass a parameter. -Rim */
/* Handle CON_PENALTY_PENLIST state (for pardon and parole) */
void penalty_penlist( DESCRIPTOR_DATA *d, const char *argument )
{
   CHAR_DATA *ch = d->character;
   PENALTY_DATA *p = ch->pcdata->new_penalty;
   PENALTY_DATA *q = NULL;
   char buf[MAX_STRING_LENGTH];
   int i, pick;
   int victim_playing = p->duration;
   
   if (!p)
   {
      d->connected = CON_PLAYING;
      bug ("penalty: In CON_PENALTY_PENLIST, NULL ch->pcdata->new_penalty",0);
      return;
   }

   strcpy (buf, argument);

   if (!buf[0] || !is_number(buf))
   {
       show_penalties_by_player(ch, p->victim_name, victim_playing, 1);
       return;
   }
   
   pick = atoi(buf);

   if ( pick <= 0 )
   {
       show_penalties_by_player(ch, p->victim_name, victim_playing, 1);
       return;
   }
   
   /* Validate and retrieve choice from list of active penalties */
   i = 0;
   for (q = penalty_list; q; q = q->next)
   {
       if (!str_cmp(q->victim_name, p->victim_name)
           && q->status != PENALTY_STATUS_PARDON_PENDING
           && q->status != PENALTY_STATUS_PAROLE_PENDING)
       {
           i++;
           if (i == pick)
               break;
       }
   }

   if (!q)
   {
       show_penalties_by_player(ch, p->victim_name, victim_playing, 1);
       return;
   }

   if ( !can_remove_penalty(ch, q) )
   {
       send_to_char( "You cannot remove this penalty.\n\r", ch );
       /* abort so imm won't get stuck if he can't remove any penalties */ 
       penalty_confirm( d, "n" );
       return;
   }

   p->next = q;  /* Hackish, but this keeps the choice until finish */

   /* Now set up confirm CON state */
   if (p->status == PENALTY_STATUS_PARDON_PENDING || p->status == PENALTY_STATUS_PARDONED)
       printf_to_char(ch, "{+You have chosen to {GPardon{x{+ player {r%s{x{+ of the following offense:{x\n\r",
          q->victim_name);
   else
       printf_to_char(ch, "{+You have chosen to {YParole{x{+ player {r%s{x{+ of the following offense:{x\n\r",
          q->victim_name);

   printf_to_char(ch, "{+%d{x. {r%s{x by {c%s{x, severity {c%d{x, {c%d{x points",
       pick,
       q->penalty_type,
       capitalize(q->imm_name),
       q->severity + 1,
       q->points);
   
   if (q->duration < 0)
       send_to_char(" {Y--> No Expiration{x\n\r", ch);
   else
       printf_to_char(ch, ", {C%.1f{x hours ({Y%.1f{x remaining)\n\r",
       (float)q->duration / 3600.0,
       (float)(q->start_time + q->duration - victim_playing) / 3600.0);   


   send_to_char("Proceed (Y/N)?\n\r",ch);

   d->connected = CON_PENALTY_CONFIRM;
}


/* Handle CON_PENALTY_FINISH state */
void penalty_finish( DESCRIPTOR_DATA *d, const char *argument )
{
    char buf[MSL], to_buf[MSL];   

    CHAR_DATA *ch = d->character;
    CHAR_DATA *victim = NULL;
    PENALTY_DATA *p = ch->pcdata->new_penalty;
    PENALTY_DATA *q = NULL;
    
    if (!p)
    {
        d->connected = CON_PLAYING;
        bug ("penalty: In CON_PENALTY_FINISH, NULL ch->pcdata->new_penalty",0);
        return;
    }

    /* New penalty to be added */
    if (p->status == PENALTY_STATUS_PENDING)
    {
        p->jail_room      = 0;

        if (p->duration < 0)
            sprintf(buf, "%s of %s for unlimited hours", capitalize(p->penalty_type), p->victim_name);                    
        else
            sprintf(buf, "%s of %s for %.1f hours", capitalize(p->penalty_type), p->victim_name, (float)p->duration / 3600.0);
        sprintf( to_buf, "imm %s", p->victim_name );
        make_note("Penalty", p->imm_name, to_buf, buf, 30, p->text);

        free_string(p->text);
        p->text           = NULL;

        /* Add it in.  Note that status is pending - char_update will set it active. */
        p->next           = penalty_list;
        penalty_list      = p;
        ch->pcdata->new_penalty = NULL;
        
        save_penalties();

        if ((victim = get_char_world(ch, p->victim_name)) == NULL)
            send_to_char("Complete.  Penalty will activate on the next tick for that player.\n\r", ch);
        else
        {
            send_to_char("Penalty applied.\n\r",ch);
            penalty_update(victim);
        }

        d->connected = CON_PLAYING;
    }
    else
    {
	bool delete = FALSE;
        /* Parole or Pardon */
        q = p->next; /* Recall here the hack that saved a pointer to the chosen */
                     /* penalty to be paroled/pardoned in p->next.              */
        
        if (!q)
            bug ("penalty: Lost reference to penalty to be paroled/pardoned.",0);
        else
        {
            if (q->status == PENALTY_STATUS_PENDING)
		/* It hasn't taken effect yet, just delete it */
                /* delete_penalty_node(q); */
		delete = TRUE;
            else
            {
                q->status = p->status;
                q->changed_by = str_dup(ch->name);
                q->changed_time = current_time;
            }

            sprintf(buf, "%s of %s %s",
               capitalize(q->penalty_type),
               q->victim_name,
               p->status == PENALTY_STATUS_PAROLE_PENDING ? "paroled" : "pardoned");


            if (q->duration > 0) /* Also recall p->duration has victim->playing in it at this point. */
                sprintf(buf, "%s, %.1f hrs remaining", buf, (float)(q->start_time + q->duration - p->duration) / 3600.0);
	    sprintf( to_buf, "imm %s", p->victim_name );
            make_note("Penalty", p->imm_name, to_buf, buf, 30, p->text);

	    if ( delete )
		delete_penalty_node(q);
        }

        
        free_string(p->imm_name);
        free_string(p->victim_name);
        free_string(p->changed_by);
        free_string(p->penalty_type);
        free_string(p->text);
        free_mem(p, sizeof(PENALTY_DATA));
        ch->pcdata->new_penalty = NULL;
        
        save_penalties();

        if ((victim = get_char_world(ch, p->victim_name)) == NULL)
            send_to_char("Complete.  Penalty will be removed on the next tick for that player.\n\r",ch);
        else
        {
            send_to_char("Penalty removed.\n\r",ch);
            penalty_update(victim);
        }

        d->connected = CON_PLAYING;
    }
}    
        

/* Pick a random room from the jail room list, and if it's occupied begin
   traversing the list.  If you come back to where you started and all rooms
   are occupied, go ahead and (reluctantly) reuse the room. -Rim 1/99 */
ROOM_INDEX_DATA *find_jail_room(void)
{
    ROOM_INDEX_DATA *room;
    int choice, pivot;
    PENALTY_DATA *p;
    bool found;
    
    if (top_jail_room < 0)  /* No jail rooms found on the MUD at boot time */
        return NULL;

    choice = number_range(0, top_jail_room);
    pivot = choice;

    for ( ; ; )
    {
        room = get_room_index(jail_room_list[choice]);
        
        if (room != NULL && IS_SET(room->room_flags, ROOM_JAIL))
        {
            found = FALSE;
            
            for (p = penalty_list; p; p = p->next)
                if (p->jail_room == jail_room_list[choice])
                    found = TRUE;

            if (!found)
                break;
        }
        
        choice++;
        
        if (choice > top_jail_room)
            choice = 0;
        
        if (choice == pivot)  /* All rooms occupied, so punt search and reuse original choice. */
        {
            room = get_room_index(jail_room_list[pivot]);

            if (room != NULL && !IS_SET(room->room_flags, ROOM_JAIL))
                room = NULL;

            break;
        }
    }
    
    return room;
}

        
/* Apply penalty changes to player, and update penalty records accordingly.
   Called once per tick, and at character login. -Rim 1/99 */
void penalty_update(CHAR_DATA *ch)
{
    PENALTY_DATA *p, *p_next;
    long bit = 0;
    int pen;

    if (ch == NULL)
        return;

    /* Skip if NPC or linkdead */
    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    for (p = penalty_list; p; p = p_next)
    {
	p_next = p->next;
        if (!str_cmp(p->victim_name, ch->name))
        {
            pen = penalty_table_lookup(p->penalty_type);
            bit = penalty_table[pen].bit;      

            switch (p->status) 
            {
            case PENALTY_STATUS_PENDING:

                p->status = PENALTY_STATUS_SERVING;
                
                ch->pcdata->demerit_points += p->points;
                SET_BIT(ch->penalty, bit);
                
                if (bit == PENALTY_JAIL) /* Allocate a jail room */
                {
                    ROOM_INDEX_DATA *room;
                    
                    if ((room = find_jail_room()) == NULL)
                    {                  
                        bugf("Unable to find a jail room for %s, penalty aborted", p->victim_name);
                        delete_penalty_node(p);
			p = NULL;
                    }
                    else /* Stuff them in the jail room */
                    {
                        p->jail_room = room->vnum;

                        if ( ch->fighting != NULL )
                            stop_fighting( ch, TRUE );
                        
                        stop_follower( ch );  /* Remove any groups */
                        
                        act( "$n is yanked from the room by an invisible hand!", ch, NULL, NULL, TO_ROOM );
                        char_from_room( ch );
                        char_to_room( ch, room );
                        do_look( ch, "auto" );
                    }
                }                
                save_penalties();
		if (p == NULL)
		  break;

                if (p->duration < 0)
                    printf_to_char(ch, "You have been %s indefinitely.\n\r", penalty_table[pen].apply_string);
                else
                    printf_to_char(ch, "You have been %s for %.1f hours.\n\r", 
                        penalty_table[pen].apply_string, (float)p->duration / 3600.0);

                printf_to_wiznet(ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0, 
                    "%s has been %s by %s.", 
                    capitalize(ch->name), 
                    penalty_table[pen].apply_string, 
                    capitalize(p->imm_name)); 
                
                break;

            case PENALTY_STATUS_SERVING:
                /* Check sentence duration against played, delete if expired, trans player if jailed */
                if (p->duration >= 0 && TIME_PLAYED(ch) > p->start_time + p->duration)
                {
                    printf_to_char(ch, "You are no longer %s - you have completed your sentence.\n\r", penalty_table[pen].apply_string);
                    printf_to_wiznet(ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0, 
                        "%s is no longer %s : Sentence complete.", 
                        capitalize(p->victim_name), 
                        penalty_table[pen].apply_string);
                    REMOVE_BIT(ch->penalty, bit);
                    delete_penalty_node(p);
                    
                    if (bit == PENALTY_JAIL)
                    {
                        ROOM_INDEX_DATA *room;
                        
                        room = get_room_index( ROOM_VNUM_RECALL );
                        
                        char_from_room( ch );
                        char_to_room( ch, room );
                        act( "$n is dropped into the room from above.", ch, NULL, NULL, TO_ROOM );
                        do_look( ch, "auto" );
                    }
                    save_penalties();
                    break;
                }
                else if (!IS_SET(ch->penalty, bit))
                {
                    SET_BIT(ch->penalty, bit);

                    if (p->duration < 0)
                        printf_to_char(ch, "You remain %s indefinitely.\n\r", penalty_table[pen].apply_string);
                    else
                        printf_to_char(ch, "{+You remain {y%s{x{+ for {y%.1f{x{+ more hours.\n\r", 
                            penalty_table[pen].apply_string, (float)(p->start_time + p->duration - TIME_PLAYED(ch)) / 3600.0);

                }
                if (bit == PENALTY_JAIL  && ch->in_room)
                    if (ch->in_room->vnum != p->jail_room)
                    {
                        ROOM_INDEX_DATA *room;
                        room = get_room_index( p->jail_room );                            
                        
                        if (room == NULL)
                            break;

                        if ( ch->fighting != NULL )
                            stop_fighting( ch, TRUE );
                        
                        ch->leader = NULL; /* Remove any groups */
                        
                        char_from_room( ch );
                        char_to_room( ch, room );
                        do_look( ch, "auto" );
                    }
                    
                break;

            case PENALTY_STATUS_PAROLE_PENDING:

                ch->pcdata->demerit_points -= p->points / 2;
                ch->pcdata->demerit_points = UMAX(0, ch->pcdata->demerit_points);
                
                REMOVE_BIT(ch->penalty, bit);
                
                if (bit == PENALTY_JAIL)
                {
                    ROOM_INDEX_DATA *room;
                    
                    room = get_room_index( ROOM_VNUM_RECALL );
                    
                    char_from_room( ch );
                    char_to_room( ch, room );
                    act( "$n is dropped into the room from above.", ch, NULL, NULL, TO_ROOM );
                    do_look( ch, "auto" );
                }

                printf_to_char(ch, "You are no longer %s - you have been paroled by %s.\n\r", penalty_table[pen].apply_string, p->changed_by );
                printf_to_wiznet(ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0, 
                    "%s is no longer %s : Paroled by %s.", 
                    capitalize(p->victim_name),
                    penalty_table[pen].apply_string, p->changed_by);
                
                delete_penalty_node(p);
                save_penalties();
                break;
                
            case PENALTY_STATUS_PARDON_PENDING:
                
                ch->pcdata->demerit_points -= p->points;
                ch->pcdata->demerit_points = UMAX(0, ch->pcdata->demerit_points);
                
                REMOVE_BIT(ch->penalty, bit);
                
                if (bit == PENALTY_JAIL)
                {
                    ROOM_INDEX_DATA *room;
                    
                    room = get_room_index( ROOM_VNUM_RECALL );
                    
                    char_from_room( ch );
                    char_to_room( ch, room );
                    act( "$n is dropped into the room from above.", ch, NULL, NULL, TO_ROOM );
                    do_look( ch, "auto" );
                }

                printf_to_char(ch, "You are no longer %s - you have been pardoned by %s.\n\r", penalty_table[pen].apply_string, p->changed_by );
                printf_to_wiznet(ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0, 
                    "%s is no longer %s : Pardoned by %s.", 
                    capitalize(p->victim_name),
                    penalty_table[pen].apply_string, p->changed_by);

                delete_penalty_node(p);
                save_penalties();                
                break;

            default:
                /* The remaining 3 (4 if you include NONE) are for reporting only */
                break;
            }
        }
    }
}


void check_penlist()
{
    PENALTY_DATA *p, *np;

    for (p = penalty_list; p; p = np)
    {
        np = p->next;

        if (!exists_player(p->victim_name))
            delete_penalty_node(p);
    }

    save_penalties();
}

void save_crime_list(void)
{
    CRIME_DATA *pcrime;
    FILE *fp;
    bool found = FALSE;
    
    if ( ( fp = fopen( CRIME_FILE, "w" ) ) == NULL )
    {
        log_error( CRIME_FILE );
    }
    
    for (pcrime = crime_list; pcrime != NULL; pcrime = pcrime->next)
    {
        found = TRUE;
        rfprintf(fp,"%s %s~\n",pcrime->name, pcrime->desc);
    }
    
    fclose(fp);
    if (!found)
        unlink(CRIME_FILE);
}

void load_crime_list(void)
{
    FILE *fp;
    CRIME_DATA *crime_last;
    
    if ( ( fp = fopen( CRIME_FILE, "r" ) ) == NULL )
    {
        bug("Error opening crime list file.", 0);
        return;
    }
    
    crime_last = NULL;

    for ( ; ; )
    {
        CRIME_DATA *pcrime;
        if ( feof(fp) )
        {
            fclose( fp );
            return;
        }
        
        pcrime = new_crime();
        
        pcrime->name = str_dup(fread_word(fp));
        pcrime->desc = str_dup(fread_string(fp));
        fread_to_eol(fp);
        
        if (crime_list == NULL)
            crime_list = pcrime;
        else
            crime_last->next = pcrime;
        crime_last = pcrime;
    }
}


DEF_DO_FUN(do_crimelist)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CRIME_DATA *cr;
    CRIME_DATA *prev;
    
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    
    if (!*arg1 || ch->level < L2) /* Display list */
    {
        send_to_char("\n\r-- Crime List --\n\r", ch);
        for (cr = crime_list; cr; cr = cr->next)
            printf_to_char(ch, "%-10s : %s\n\r", cr->name, cr->desc);
        return;
    }
    
    if ( !str_cmp( arg1, "delete" ) )
    {
        prev = NULL;
        
        for (cr = crime_list; cr; prev = cr, cr = cr->next)
            if (!str_cmp(arg2, cr->name))
            {
                if ( prev == NULL )
                    crime_list = cr->next;
                else                        
                    prev->next = cr->next;
                
                free_crime(cr);
                save_crime_list();
                
                send_to_char("Crime type deleted from list.\n\r", ch);
                return;
            }
    }
    
    if ( !str_cmp( arg1, "add" ) )
    {
        for (cr = crime_list; cr; cr = cr->next)
            if (!str_cmp(arg2, cr->name))
            {
                send_to_char("That crime type is already in the list.\n\r", ch);
                return;
            }
            
            cr = new_crime();
            
            cr->name      = str_dup(arg2);
            cr->desc      = str_dup(argument);
            
            cr->next = crime_list;
            crime_list = cr;
            
            save_crime_list();
            
            send_to_char("Crime type added.\n\r", ch);
            return;
    }
    
    send_to_char("Syntax:\n\r"
        "crimelist                             - View list of crime types.\n\r"
        "crimelist add    <name> <description> - Add new crime type to list.\n\r"
        "crimelist delete <name>               - Delete a type from the list.\n\r", ch );
}

int get_crime_count( CHAR_DATA *ch, const char *crime_name, const char *imm_name )
{
    CRIME_DATA *cr;
    bool check_imm = imm_name[0] != '\0';
    int tally = 0;

    for (cr = ch->pcdata->crimes; cr; cr = cr->next)
    {
	if ( !is_exact_name(crime_name, cr->name) 
	     || (check_imm && !is_exact_name(imm_name, cr->imm_name)) )
	    continue;

	if (cr->forgive == 0)
	    tally++;
	else
	    tally--;
    }

    return tally;
}

DEF_DO_FUN(do_review)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CRIME_DATA *crtype;
    CRIME_DATA *cr;
    char timebuf[25];
    int tally;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' )
    {
        send_to_char( "Review whom?\n\r", ch );
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("No such person is online at the moment.\n\r", ch);
        return ;
    }

    if (get_trust(victim) >= get_trust(ch) && victim != ch)
    {
        send_to_char("Let someone else review your superiors.\n\r", ch) ;
        return ;
    }

    if (IS_IMMORTAL(victim) && get_trust(ch) < MAX_LEVEL)
    {
        send_to_char("Only an Implementor may review an immortal.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r",ch);
        return;
    }

    if (arg2[0] != '\0' && !str_cmp(arg2, "full"))
    {
        
        printf_to_char(ch, "{CFull criminal record for: {y%s{x\n\r\n\r", capitalize(victim->name));
        
        if (victim->pcdata->crimes == NULL)
            send_to_char("No crimes to report.\n\r", ch);
        else
            for (cr = victim->pcdata->crimes; cr != NULL; cr = cr->next)
            {
                strftime(timebuf, 25, "%B %d, %Y", localtime(&(cr->timestamp)));
                
                printf_to_char(ch, "{cCrime: {y%-10s{c  %s: {w%s{c  By: {w%s{c\n\r",
                    cr->name,
                    cr->forgive ? " Forgiven" : "Convicted",
                    timebuf,
                    capitalize(cr->imm_name));
            }
    }
    else
    {
        printf_to_char(ch, "{CCriminal record summary for: {y%s{x\n\r\n\r", capitalize(victim->name));

        if (victim->pcdata->crimes == NULL)
            send_to_char("No crimes to report.\n\r", ch);
        else
            for (crtype = crime_list; crtype; crtype = crtype->next)
            {
                tally = get_crime_count( victim, crtype->name, "" );
                printf_to_char(ch, "{y%10s{c:  %s%2d{x\n\r",
                    capitalize(crtype->name),
                    tally > 0 ? "{g" : "{r",
                    tally);
            }
    }
}


DEF_DO_FUN(do_punish)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CRIME_DATA *cr, *newcr;
    bool found = FALSE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );    


    if ( arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char( "Syntax: punish <player> <crime type>\n\r\n\r", ch);
        send_to_char( "Use the crimelist command to view available crime types.\n\r", ch);
        send_to_char( "   (You must use a valid keyword for the crime type.)\n\r", ch);
        return;
    }    
    
    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("No such person online at the moment.\n\r", ch);
        return ;
    }
    
    if (get_trust(victim) >= get_trust(ch))
    {
        send_to_char("You don't have the authority to do that.\n\r", ch) ;
        return ;
    }

    if (IS_IMMORTAL(victim) && get_trust(ch) < MAX_LEVEL)
    {
        send_to_char("Only an Implementor may punish an immortal.\n\r", ch);
        return;
    }
    
    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r",ch);
        return;
    }

    for (cr = crime_list; cr; cr = cr->next)
    {
        if (is_exact_name(cr->name, arg2)) /* Deliberately force imm to type full name of crime */
            found = TRUE;
    }

    if (!found)
    {
        printf_to_char(ch, "Crime type %s not found.\n\r", arg2);
        send_to_char("A list of valid types may be found using the crimelist command.\n\r", ch);
        return;
    }

    newcr = new_crime();
    newcr->imm_name = str_dup(ch->name);
    newcr->name = str_dup(arg2);
    newcr->timestamp = current_time;
    newcr->forgive = 0;
 

    cr = victim->pcdata->crimes;

    if (!cr) /* List is empty, insert as only node */
    {
        victim->pcdata->crimes = newcr;
    }
    else     /* Insert at end of list so history will report in chronological order */
    {
        for ( ; cr->next; cr = cr->next)
            ;

        cr->next = newcr;
    }

    send_to_char("Punish successful.\n\r", ch);
}



DEF_DO_FUN(do_forgive)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CRIME_DATA *cr;
    CRIME_DATA *newcr;
    bool found = FALSE;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );    
    
    
    if ( arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char( "Syntax: forgive <player> <crime type>\n\r\n\r", ch);
        send_to_char( "Use the crimelist command to view available crime types.\n\r", ch);
        send_to_char( "   (You must use a valid keyword for the crime type.)\n\r", ch);
        return;
    }    
    
    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("No such person online at the moment.\n\r", ch);
        return ;
    }
    
    if (get_trust(victim) >= get_trust(ch))
    {
        send_to_char("You don't have the authority to do that.\n\r", ch) ;
        return ;
    }
    
    if (IS_IMMORTAL(victim) && get_trust(ch) < MAX_LEVEL)
    {
        send_to_char("Only an Implementor may forgive an immortal.\n\r", ch);
        return;
    }
    
    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r",ch);
        return;
    }
    
    for (cr = crime_list; cr; cr = cr->next)
    {
        if (is_exact_name(cr->name, arg2)) /* Deliberately force imm to type full name of crime */
            found = TRUE;
    }
    
    if (!found)
    {
        printf_to_char(ch, "Crime type %s not found.\n\r", arg2);
        send_to_char("A list of valid types may be found using the crimelist command.\n\r", ch);
        return;
    }
    
    for (cr = victim->pcdata->crimes; cr; cr = cr->next)
    {
        if (is_exact_name(cr->name, arg2)) /* Deliberately force imm to type full name of crime */
            found = TRUE;
    }
    
    if (!found)
    {
        send_to_char("That player has never been convicted of that crime.\n\r", ch);
        return;
    }
    
    /* only law imms may forgive punishments imposed by others */
    if ( IS_SET(ch->act, PLR_LAW) )
	found = get_crime_count( victim, arg2, "" ) > 0;
    else
	found = get_crime_count( victim, arg2, ch->name ) > 0;

    if (!found)
    {
        send_to_char( "You cannot forgive that crime.\n\r", ch);
        return;
    }

    newcr = new_crime();
    newcr->imm_name = str_dup(ch->name);
    newcr->name = str_dup(arg2);
    newcr->timestamp = current_time;
    newcr->forgive = 1;
    
    for ( cr = victim->pcdata->crimes; cr->next; cr = cr->next)
        ;
    
    cr->next = newcr;

    send_to_char("Forgive successful.\n\r", ch);
    
}






