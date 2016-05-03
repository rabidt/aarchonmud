/***********************************************************************
*                                                                      * 
*   Bard class skills and spells, intended for use by Aeaea MUD.       *
*   All rights are reserved.                                           *
*                                                                      * 
*   Core ranger group by Brian Castle a.k.a. "Rimbol".                 *
*   Another batch added by James Stewart a.k.a "Siva".                 * 
***********************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"

DEF_DO_FUN(do_wail)
{
    CHAR_DATA *victim;
    int skill;
    int dam, chance;

    if ((skill = get_skill(ch,gsn_scream)) == 0)
    {
        send_to_char("You scream your lungs out without effect.\n\r", ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL)
        return;

    
    chance = (100 + get_skill(ch,gsn_scream)) / 2;

    if ( check_hit(ch, victim, gsn_scream, DAM_SOUND, chance) )
    {
        dam = martial_damage( ch, victim, gsn_scream );

        full_dam(ch, victim, dam, gsn_scream, DAM_SOUND, TRUE);
        check_improve(ch, gsn_scream, TRUE, 3);
    } else {
        damage( ch, victim, 0, gsn_scream, DAM_SOUND, TRUE);
        check_improve(ch, gsn_scream, FALSE, 3);
    }
    return;
}


// completely ripped off do_stance. not ashamed
DEF_DO_FUN(do_sing)
{
    int i;

    if (argument[0] == '\0')
    {
        if (ch->song == 0)
        {
            send_to_char("What song do you wish to sing?\n\r", ch);
        } else {
            send_to_char("You are no longer singing.\n\r", ch);
            ch->song = 0;
        }
        return;
    }

    // only search known songs
    for ( i = 0; songs[i].name != NULL; i++ )
    {
        if ( !str_prefix(argument, songs[i].name) && get_skill(ch, *(songs[i].gsn)) )
        {
            break;
        }
    }
    if ( songs[i].name == NULL )
    {
        if (ch->song && songs[ch->song].name)
        {
            char buffer[80];
            if (sprintf(buffer, "You don't know that song so you continue to sing %s.\n\r", songs[ch->song].name) > 0)
            {
                send_to_char(buffer, ch);
                return;
            }
        }

        send_to_char("You don't know that song.\n\r", ch);
        return;
    }

    if ( ch->song == i )
    {
        send_to_char("You're already singing that song\n\r", ch);
        return;
    }

    ch->song = i;

    printf_to_char(ch, "You begin singing the %s.\n\r", songs[i].name);
    if ( ch->fighting != NULL )
    {
        char buf[MSL];
        sprintf( buf, "$n begins to sing the %s.", songs[i].name);
        act( buf, ch, NULL, NULL, TO_ROOM );
    }
}