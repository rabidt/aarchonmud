/* Grant, by Steven Carrico <scarrico@mail.transy.edu> */

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
#include "interp.h"
#include "recycle.h"


const   struct  pair_type        pair_table       [] =
{
    {"switch", "return",FALSE},
    {"reboo", "reboot",FALSE},
    {"shutdow", "shutdown",FALSE},
    {"sla", "slay",FALSE},
    {"", "",FALSE}
};


bool is_granted( CHAR_DATA *ch, DO_FUN *do_fun)
{
    GRANT_DATA *gran;
    
    ch = original_char( ch );
    
    for (gran = ch->pcdata->granted; gran != NULL; gran = gran->next)
        if (do_fun == gran->do_fun)
            return TRUE;
        
    return FALSE;
}

bool is_granted_name( CHAR_DATA *ch, char *name)
{
    GRANT_DATA *gran;
    
    ch = original_char( ch );
    
    for (gran = ch->pcdata->granted; gran != NULL; gran = gran->next)
        if (is_exact_name(gran->name,name))
            return TRUE;
        
    return FALSE;
}

int grant_duration(CHAR_DATA *ch, DO_FUN *do_fun)
{
    GRANT_DATA *gran;
    
    ch = original_char( ch );
    
    /*  Replace the x's in the line below with the name of
        a character that is allowed to grant commands to
        anyone, even if they don't have the command
        themselves.  This is useful when you add new
        imm commands, and need to give them to yourself.
        Additional names can be added as needed and
        should be seperated by spaces.  */
    
    /*
    if (is_exact_name(ch->name,"Rimbol Quirky"))
        return -1;
    */
    /* allow all IMPs to grant everything --Bobble */
    if ( get_trust(ch) == IMPLEMENTOR )
	return -1;

    for (gran = ch->pcdata->granted; gran != NULL; gran = gran->next)
        if (gran->do_fun == do_fun)
            return gran->duration;
        
    return 0;
}

void grant_add(CHAR_DATA *ch, char *name, DO_FUN *do_fun, int duration, int level)
{
    GRANT_DATA *gran;
    
    ch = original_char( ch );
    
    gran = alloc_mem(sizeof(*gran));
    gran->name = str_dup(name);
    gran->do_fun = do_fun;
    gran->duration = duration;
    gran->level = level;
    
    gran->next = ch->pcdata->granted;
    ch->pcdata->granted = gran;
    
    return;
}

void grant_remove(CHAR_DATA *ch, DO_FUN *do_fun, bool mshow)
{
    GRANT_DATA *p,*gran;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *rch;
    
    rch = original_char( ch );

    p = NULL;
    gran = rch->pcdata->granted;

    if (gran->do_fun == do_fun)
        rch->pcdata->granted=gran->next;
    else
    {
        for (gran = rch->pcdata->granted; gran != NULL; gran = gran->next)
        {
            if (gran->do_fun == do_fun) 
                break;

            p = gran;
        }
    }
        
    if (p != NULL) 
        p->next=gran->next;

    sprintf(buf,"You have lost access to the %s command.\n\r",gran->name);

    if (mshow) 
        send_to_char(buf,ch);

    free_string(gran->name);
    free_mem(gran,sizeof(*gran));

    return;
}

void grant_level( CHAR_DATA *ch, CHAR_DATA *victim, int level, int duration )
{
    int cmd;
    
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
        if (   cmd_table[cmd].level == level
            && !is_granted(victim, cmd_table[cmd].do_fun)
            && grant_duration(ch, cmd_table[cmd].do_fun) == -1)
        {
            grant_add(victim, 
                cmd_table[cmd].name, 
                cmd_table[cmd].do_fun, 
                duration, 
                cmd_table[cmd].level);
        }
        return;
}

void revoke_level( CHAR_DATA *ch, CHAR_DATA *victim, int level )
{
    int cmd;
    
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
        if ( cmd_table[cmd].level == level
            && is_granted(victim, cmd_table[cmd].do_fun)
            && grant_duration(ch, cmd_table[cmd].do_fun) == -1)
        {
            grant_remove(victim,cmd_table[cmd].do_fun,FALSE);
        }
        return;
}

void do_grant( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL, *rch, *rvictim = NULL;
    int  dur,cmd,x;
    bool found = FALSE;
    DESCRIPTOR_DATA *d;
    
    argument = one_argument(argument,arg1);
    argument = one_argument(argument,arg2);
    one_argument(argument,arg3);
    

    rch = original_char( ch );
    
    if (arg1[0] == '\0')
    {
        send_to_char("Grant who, what?\n\r",ch);
        return;
    }
    
    /*
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        rvictim = d->original ? d->original : d->character;
        
        if (rvictim == NULL) 
            continue;
        
        if (!str_cmp(rvictim->name,arg1))
        {
            victim = d->character;
            break;
        }
    }
    */    

    /* Bobble: fix for linkdead victims */
    victim = get_char_world( rch, arg1 );
    if ( victim != NULL && IS_NPC(victim) )
    {
        send_to_char("Not on mobs.\n\r",ch);
	return;
    }

    
    if (victim == NULL && !str_cmp("self",arg1))
    {
        victim = ch;
    }
    
    if (victim == NULL)
    {
        send_to_char("Player not found.\n\r",ch);
        return;
    }

    rvictim = original_char( victim );
    
    if (arg2[0] == '\0')
    {
        int col = 0;
        int lvl;
        
        printf_to_char(ch, "%s has not been granted the following commands:\n\r", rvictim->name);
        
        for ( lvl = IM; lvl <= ( L1 + 1 ) ; lvl++ )
            for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
                if (cmd_table[cmd].level >= LEVEL_IMMORTAL
                    && !is_granted(victim,cmd_table[cmd].do_fun)
                    && cmd_table[cmd].level == lvl)
                {
                    printf_to_char( ch,"[L%3d] %-12s", cmd_table[cmd].level, cmd_table[cmd].name );

                    if ( ++col % 4 == 0 )
                        send_to_char( "\n\r", ch );
                }
                if ( col % 4 != 0 )
                    send_to_char( "\n\r", ch);
                return;
    }
    
    dur = arg3[0] == '\0' ? -1 : (is_number(arg3) ? atoi(arg3) : 0);
    
    if (dur < 1 && dur != -1)
    {
        send_to_char("Invalid duration!\n\r",ch);
        return;
    }
    
    if (is_number(arg2))
    {
        if (atoi(arg2) < LEVEL_IMMORTAL || atoi(arg2) > MAX_LEVEL)
        {
            send_to_char("Invalid grant level.\n\r",ch);
            return;
        }

	

        grant_level(ch, victim, atoi(arg2), dur );
        printf_to_char(victim, "You have been granted level %d commands. ", atoi(arg2));
        printf_to_char(ch, "You have granted level %d commands to %s. ", atoi(arg2), victim->name);        

        if (dur > 0)
        {
            printf_to_char(victim, "Expiration: %d tick(s).\n\r", dur);
            printf_to_char(ch, "Expiration: %d tick(s).\n\r", dur);
        }
        else
        {
            send_to_char("\n\r",victim);
            send_to_char("\n\r",ch);
        }

        return;
    }
    
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
        if ( arg2[0] == cmd_table[cmd].name[0]
            &&   is_exact_name( arg2, cmd_table[cmd].name ) )
        {
            found = TRUE;
            break;
        }
        
        if (found)
        {
            if (cmd_table[cmd].level < LEVEL_IMMORTAL)
            {
                send_to_char("You can only grant immortal commands.\n\r",ch);
                return;
            }
            
            if (grant_duration(ch,cmd_table[cmd].do_fun) != -1)
            {
                send_to_char("You can't grant that!\n\r",ch);
                return;
            }
            
            if (is_granted(victim,cmd_table[cmd].do_fun))
            {
                send_to_char("They already have that command!\n\r",ch);
                return;
            }
            
            grant_add(victim, cmd_table[cmd].name, cmd_table[cmd].do_fun,
                dur, cmd_table[cmd].level);
            
            printf_to_char(ch,"%s has been granted the %s command. ",rvictim->name,
                cmd_table[cmd].name);

            printf_to_char(victim,"%s has granted you the %s command. ",rch->name,
                cmd_table[cmd].name);

            if (dur > 0)
            {
                printf_to_char(victim, "Expiration: %d tick(s).\n\r", dur);
                printf_to_char(ch, "Expiration: %d tick(s).\n\r", dur);
            }
            else
            {
                send_to_char("\n\r",victim);
                send_to_char("\n\r",ch);
            }


            for (x = 0; pair_table[x].first[0] != '\0'; x++)
                if (!str_cmp(arg2,pair_table[x].first)
                    && !is_granted_name(victim,pair_table[x].second))
                {
                    sprintf(buf,"%s %s %s",rvictim->name, pair_table[x].second, arg3);
                    do_grant(ch,buf);
                }
                else if (!str_cmp(arg2,pair_table[x].second)
                    && pair_table[x].one_way != TRUE
                    && !is_granted_name(victim,pair_table[x].first))
                {
                    sprintf(buf,"%s %s %s",rvictim->name,pair_table[x].first, arg3);
                    do_grant(ch,buf);
                }
                
                return;
        }

        send_to_char("Command not found!\n\r",ch);
        return;
}

void do_revoke( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL, *rvictim = NULL;
    DESCRIPTOR_DATA *d;
    int cmd, x;
    bool had_return, found = FALSE;
    
    argument = one_argument(argument,arg1);
    one_argument(argument,arg2);
    
    if (arg1[0] == '\0' )
    {
        send_to_char("Revoke who, what?\n\r",ch);
        return;
    }
    
    /*
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        rvictim = d->original ? d->original : d->character;
        
        if (rvictim == NULL) 
            continue;
        
        if (!str_cmp(rvictim->name,arg1))
        {
            victim = d->character;
            break;
        }
    }
    */
    
    /* Bobble: fix for linkdead victims */
    victim = get_char_world( ch, arg1 );
    if ( victim != NULL && IS_NPC(victim) )
    {
        send_to_char("Not on mobs.\n\r",ch);
	return;
    }

    if (victim == NULL && !str_cmp("self",arg1))
    {
        victim = ch;
    }
    
    if (victim == NULL)
    {
        send_to_char("Player linkdead or not found.\n\r",ch);
        return;
    }
    
    rvictim = original_char( victim );

    had_return = is_granted_name(victim,"return");
    
    if (arg2[0] == '\0')
    {
        int col = 0, lvl;
        
        printf_to_char(ch,"%s has been granted the following commands:\n\r", rvictim->name);
        
        for ( lvl = IM; lvl <= ( L1 + 1 ) ; lvl++ )
            for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
                if (cmd_table[cmd].level >= LEVEL_IMMORTAL
                    && is_granted(victim,cmd_table[cmd].do_fun)
                    && cmd_table[cmd].level == lvl )
                {
                    printf_to_char( ch,"[L%3d] %-12s", cmd_table[cmd].level, cmd_table[cmd].name );

                    if ( ++col % 4 == 0 )
                        send_to_char( "\n\r", ch );
                }
                if ( col % 4 != 0 )
                    send_to_char( "\n\r", ch);
                return;
    }
    
    if (is_number(arg2))
    {
        if (atoi(arg2) < LEVEL_IMMORTAL || atoi(arg2) > MAX_LEVEL)
        {
            send_to_char("Invalid revoke level.\n\r",ch);
            return;
        }

        revoke_level(ch, victim, atoi(arg2));
        printf_to_char(ch, "You have revoked level %d commands from %s.\n\r", atoi(arg2), victim->name);
        printf_to_char(victim, "You have lost access to level %d commands.\n\r", atoi(arg2));

        if (had_return && !is_granted_name(victim,"return") && rvictim != victim) 
            do_return(victim,"");
        
        return;
    }
    
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
        if ( arg2[0] == cmd_table[cmd].name[0] && is_exact_name( arg2, cmd_table[cmd].name ) )
        {
            found = TRUE;
            break;
        }
        
        if (found)
        {
            char buf[MAX_STRING_LENGTH];
            
            if (grant_duration(ch,cmd_table[cmd].do_fun) != -1)
            {
                send_to_char("You can't revoke that!\n\r",ch);
                return;
            }
            
            if (!is_granted(victim,cmd_table[cmd].do_fun))
            {
                send_to_char("They don't have that command!\n\r",ch);
                return;
            }
            
            grant_remove(victim,cmd_table[cmd].do_fun,TRUE);
            
            printf_to_char(ch,"%s has lost access to the %s command.\n\r",
                rvictim->name,cmd_table[cmd].name);

            for (x = 0; pair_table[x].first[0] != '\0'; x++)
                if (!str_cmp(arg2,pair_table[x].first)
                    && is_granted_name(victim,pair_table[x].second))
                {
                    sprintf(buf,"%s %s",rvictim->name,pair_table[x].second);
                    do_revoke(ch,buf);
                }
                else if (!str_cmp(arg2,pair_table[x].second)
                    && pair_table[x].one_way != TRUE
                    && is_granted_name(victim,pair_table[x].first))
                {
                    sprintf(buf,"%s %s",rvictim->name,pair_table[x].first);
                    do_revoke(ch,buf);
                }
                
                if (had_return && !is_granted_name(victim,"return") &&
                    rvictim != victim) do_return(victim,"");
                
                return;
        }
        send_to_char("Command not found!\n\r",ch);
        return;
}


void do_gstat( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    GRANT_DATA *grant;
    CHAR_DATA *victim;
    int col = 0;
    
    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("Gstat who?\n\r",ch);
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if (IS_NPC(victim))
    {
        send_to_char("Not on mobs.\n\r",ch);
        return;
    }
    
    if (get_trust(ch) < get_trust(victim))
    {
        send_to_char("You can't do that.\n\r",ch);
        return;
    }
    
    buffer = new_buf();
    
    sprintf(buf,"Grant status for %s:\n\r\n\r",victim->name );
    
    add_buf(buffer,buf);
    
    for (grant = victim->pcdata->granted; grant != NULL; grant = grant->next)
    {
        char ds[50];
        char str[25];
        char s2[25];
        int x, sl;
        
        sprintf(ds, "%d", grant->duration);
        str[0] = '\0';
        sl = (int)((6 - strlen(ds)) / 2);
        
        for (x = 0; x < sl; x++)
            strcat(str," ");
        
        strcpy(s2,str);
        
        if ((strlen(str) + strlen(ds)) % 2 == 1) 
            strcat(s2," ");
        
        if (grant->duration == -1)
            sprintf(buf,"[ perm ] %-11s",grant->name);
        else
            sprintf(buf,"[%s%d%s] %-11s",str, grant->duration, s2, grant->name);
        
        add_buf(buffer,buf);
        
        col++;
        col %= 4;
        
        if (col == 0) 
            add_buf(buffer,"\n\r");
    }
    
    if (col != 0) 
        add_buf(buffer,"\n\r");
    
    page_to_char(buf_string(buffer),ch);
    
    free_buf(buffer);
    return;
}

/* This is a placeholder function.  To grant or revoke the "immflag" command acts
   as a trigger to enable or disable all immortal commands for a given player. */
void do_immflag( CHAR_DATA *ch, char *argument )
{
    send_to_char( "Huh?\n\r", ch );
    return;
}
