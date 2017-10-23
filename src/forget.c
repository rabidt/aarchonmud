/***************************************** 
* Forget/remember by Dominic J. Eidson  * 
* <eidsod01@condor.stcloud.msus.edu>    *
*****************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"


DEF_DO_FUN(do_forget)
{
    CHAR_DATA *rch;
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int pos;
    bool found = FALSE;
    
    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;
    
    if (IS_NPC(rch))
        return;
    
    one_argument(smash_tilde_cc(argument), arg);
    
    if (arg[0] == '\0')
    {
        if (rch->pcdata->forget[0] == NULL)
        {
            send_to_char("You are not forgetting anyone.\n\r",ch);
            return;
        }
        send_to_char("You are currently forgetting:\n\r",ch);
        
        for (pos = 0; pos < MAX_FORGET; pos++)
        {
            if (rch->pcdata->forget[pos] == NULL)
                break;
            
            snprintf(buf, sizeof(buf),"    %s\n\r",rch->pcdata->forget[pos]);
            send_to_char(buf,ch);
        }
        return;
    }
    
    for (pos = 0; pos < MAX_FORGET; pos++)
    {
        if (rch->pcdata->forget[pos] == NULL)
            break;
        
        if (!str_cmp(arg,rch->pcdata->forget[pos]))
        {
            send_to_char("You have already forgotten that person.\n\r",ch);
            return;
        }
    }
    
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        CHAR_DATA *wch;
        
        if (!IS_PLAYING(d->connected) || !can_see(ch,d->character))
            continue;
        
        wch = ( d->original != NULL ) ? d->original : d->character;
        
        if (!can_see(ch,wch))
            continue;
        
        if (!str_cmp(arg,wch->name))
        {
            found = TRUE;
            if (wch == ch)
            {
                send_to_char("You forget yourself for a moment, but it passes.\n\r",ch);
                return;
            }
            if (IS_IMMORTAL(wch))
            {
                send_to_char("That person is very hard to forget.\n\r",ch);
                return;
            }
        }
    }
    
    if (!found)
    {
        send_to_char("No one by that name is playing.\n\r",ch);
        return;
    }
    
    for (pos = 0; pos < MAX_FORGET; pos++)
    {
        if (rch->pcdata->forget[pos] == NULL)
            break;
    }
    
    if (pos >= MAX_FORGET)
    {
        send_to_char("Sorry, you have reached the forget limit.\n\r",ch);
        return;
    }
    
    /* make a new forget */
    rch->pcdata->forget[pos]		= str_dup(arg);
    printf_to_char(ch, "You are now deaf to %s.\n\r", capitalize(arg));
}

DEF_DO_FUN(do_remember)
{
    CHAR_DATA *rch;
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    int pos;
    bool found = FALSE;
    
    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;
    
    if (IS_NPC(rch))
        return;
    
    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        if (rch->pcdata->forget[0] == NULL)
        {
            send_to_char("You are not forgetting anyone.\n\r",ch);
            return;
        }
        send_to_char("You are currently forgetting:\n\r",ch);
        
        for (pos = 0; pos < MAX_FORGET; pos++)
        {
            if (rch->pcdata->forget[pos] == NULL)
                break;
            
            snprintf(buf, sizeof(buf),"    %s\n\r",rch->pcdata->forget[pos]);
            send_to_char(buf,ch);
        }
        return;
    }
    
    for (pos = 0; pos < MAX_FORGET; pos++)
    {
        if (rch->pcdata->forget[pos] == NULL)
            break;
        
        if (found)
        {
            rch->pcdata->forget[pos-1]		= rch->pcdata->forget[pos];
            rch->pcdata->forget[pos]		= NULL;
            continue;
        }
        
        if(!strcmp(arg,rch->pcdata->forget[pos]))
        {
            send_to_char("Forget removed.\n\r",ch);
            free_string(rch->pcdata->forget[pos]);
            rch->pcdata->forget[pos] = NULL;
            found = TRUE;
        }
    }
    
    if (!found)
        send_to_char("No one by that name is forgotten.\n\r",ch);
}
