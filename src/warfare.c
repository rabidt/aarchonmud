/************************************************************************
 *                                   *
 *   Warfare system by Viper (1998) - All copyright notices of ROM   *
 *           original implementors retained.         *
 *                                   *
 *************************************************************************/
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "warfare.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "buffer_util.h"
#include "religion.h"
#include "lua_main.h"

DECLARE_DO_FUN(do_restore);
DECLARE_DO_FUN(do_look);

const char * const war_list[] =
{
    "{rArmageddon{6",
    "{cRace{6",
    "{gClass{6",
    "{yClan{6",
    "{mGender{6",
    "{bReligion{6",
    "{DDuel{6"
};

WAR_DATA war;

long last_war_time = 0;
long auto_war_time = 0;

static void warfare_to_char( CHAR_DATA *ch, const char *argument );

int get_warfare_reward( bool win )
{
    int base = UMIN(10, 4 + war.total_combatants);

    // auto-warfares give double reward
    if ( war.owner == 0 )
        base *= 2;

    // small base reward for participation
    if ( !win )
        return base;

    // bonus pool of base d6 split amongst winners
    return base + dice(base, 6) / UMAX(1, war.combatants);
}


DEF_DO_FUN(do_startwar)
{
    proc_startwar( ch, argument, FALSE );
}


/* Function that gets called during war_update, to start
   warfares automatically. Astark Nov 2012 */

void auto_war(void)
{
    const int level_range = 30;
    char buf[MSL];
    int toplevel, level;
    int player_count[LEVEL_HERO+1];
    long prob_sum = 0;
    DESCRIPTOR_DATA *desc;

    // 2 hours till next automatic warfare
    auto_war_time = current_time + 2*3600;

    war.type = ARMAGEDDON_WAR;

    // collect online characters elligible for warfare
    for ( level = 0; level <= LEVEL_HERO; level++)
        player_count[level] = 0;
    for ( desc = descriptor_list; desc != NULL; desc = desc->next )
    {
        if ( IS_PLAYING(desc->connected)
                && desc->character != NULL
                && !IS_NPC(desc->character)
                && !IS_IMMORTAL(desc->character)
                && desc->character->in_room != NULL
                && !IS_SET( desc->character->in_room->area->area_flags, AREA_REMORT )
           )
            for ( level = desc->character->level; level <= UMIN(LEVEL_HERO, desc->character->level+level_range); level++ )
                player_count[level] += 1;
    }
    // compute relative probabilities for a level to be selected - eliminating single-player cases
    for ( level = level_range; level <= LEVEL_HERO; level++ )
    {
        player_count[level] *= player_count[level] - 1;
        prob_sum += player_count[level];
    }
    // not enough players to get a warfare going?
    if ( prob_sum == 0 )
    {
        // try again in 5 minutes
        auto_war_time = current_time + 300;
        return;
    }
    // now select a level based on relative probabilities stored in player_count
    toplevel = 0;
    for ( level = level_range; level <= LEVEL_HERO; level++ )
    {
        if ( number_range(1, prob_sum) <= player_count[level] )
        {
            toplevel = level;
            break;
        }
        prob_sum -= player_count[level];
    }
    // safety-net
    if (toplevel == 0)
    {
        bugf("auto_war: no top_level selected");
        return;
    }

    war.max_level = toplevel;
    war.min_level = toplevel - level_range;

    war.on = TRUE;
    war.started = FALSE;
    war.combatants = 0;
    war.war_time_left = 9;
    war.first_combatant = NULL;
    war.owner = 0;

    if ( war.type == ARMAGEDDON_WAR )
        snprintf( buf, sizeof(buf), "An {cArmageddon{6 has been declared for levels %d to %d!\n\r",
                war.min_level, war.max_level);
    else
        snprintf( buf, sizeof(buf), "A %s war has been declared for levels %d to %d!\n\r",
                war_list[war.type], war.min_level, war.max_level);

    warfare_to_all( buf );
    snprintf( buf, sizeof(buf), "Type 'combat' to join or read 'help warfare' for info.\n\r" );
    warfare_to_all( buf );
    return;

}

void proc_startduel( CHAR_DATA *ch, const char *argument)
{
    CHAR_DATA *vic;
    char buf[MSL];
    char arg1[MIL];

    if ( war.on == TRUE )
    {
        send_to_char("There is already a war running!\n\r", ch );
        return;
    }

    argument = one_argument( argument, arg1 );

    vic = get_char_world( ch, arg1 );
    if (!vic || IS_NPC(vic))
    {
        ptc( ch, "Can't find player.\n\r");
        return;
    } 
    else if (vic==ch)
    {
        ptc( ch, "You can't duel yourself!\n\r");
        return;
    }

    war.min_level = 0;
    war.max_level = 100;
    war.type = DUEL_WAR;
    war.on = TRUE;
    war.started = FALSE;
    war.combatants = 0;
    war.war_time_left = 3;
    war.first_combatant = NULL;
    war.duel_target = vic->id;

    war.owner = ch->id;
    war.cost = 10;
    ch->pcdata->questpoints -= 10;

    snprintf( buf, sizeof(buf), "%s has challenged %s to a {DDuel{6!\n\r", ch->name, vic->name);

    warfare_to_all( buf );
    snprintf( buf, sizeof(buf), "Type 'combat' to join or read 'help warfare' for info.\n\r" );
    warfare_to_char( vic, buf );
    warfare_to_char( ch, buf );
    return;
}

void proc_startwar( CHAR_DATA *ch, const char *argument, bool pay )
{
    char buf[MSL];
    char arg1[MIL], arg2[MIL], arg3[MIL];

    if (IS_NPC(ch))
        return;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ((arg1[0]=='\0') || (arg2[0]=='\0') || ((arg3[0]=='\0') && (ch->level>100)))
    {
        if ( !pay )
            send_to_char("Syntax: startwar <type> <max level> <min level>\n\r", ch);
        else
            send_to_char("Syntax: quest buy warfare <type> <max level>\n\r", ch);
        return;
    }

    if ( war.on == TRUE )
    {
        send_to_char("There is already a war running!\n\r", ch );
        return;
    }

    if (current_time < last_war_time + 600 && get_trust(ch) < MAX_LEVEL)
    {
        int time = (last_war_time + 600 - current_time)/60;
        send_to_char("{YWarfare games are limited to once every 10 minutes.{x\n\r",ch);
        printf_to_char(ch, "   {+Next opportunity: {Y%d{x{+ minute%s.{x\n\r",
                time, time == 1 ? "" : "s" );
        return;
    }

    if (ch->level>100)
    {
        war.min_level = atoi( arg2 );
        war.max_level = atoi( arg3 );
    }
    else
    {
        war.max_level = atoi(arg2);
        if(war.max_level <= 30)
            war.min_level = 1;
        else if (war.max_level > 90)
            war.min_level = 60+3*(war.max_level-90);
        else
            war.min_level = war.max_level-30;
    }

    if ( !str_cmp( arg1, "armageddon" ) )
        war.type = ARMAGEDDON_WAR;
    else if ( !str_cmp( arg1, "class" ) )
        war.type = CLASS_WAR;
    else if ( !str_cmp( arg1, "race" ) )
        war.type = RACE_WAR;
    else if ( !str_cmp( arg1, "clan" ) )
        war.type = CLAN_WAR;
    else if ( !str_cmp( arg1, "gender" ) )
        war.type = GENDER_WAR;
    else if ( !str_cmp( arg1, "religion" ) )
        war.type = RELIGION_WAR;
    else
    {
        send_to_char("Valid war types are: armageddon, class, race, clan, religion, and gender.\n\r", ch );
        return;
    }

    if ( war.max_level <= war.min_level )
    {
        send_to_char("Min level must be lower than max level.\n\r", ch );
        return;
    }

    if ( war.min_level < 1 || war.max_level > 100 )
    {
        send_to_char("The level range is 1 to 100.\n\r", ch );
        return;
    }

    if (war.min_level + 50 < war.max_level)
    {
        snprintf(buf, sizeof(buf), "Dont you think a %d spread is a little excessive?\n\r",
                war.max_level - war.min_level);
        send_to_char(buf, ch);
        return;
    }

    war.on = TRUE;
    war.started = FALSE;
    war.combatants = 0;
    war.war_time_left = 3;
    war.first_combatant = NULL;

    if ( !pay )
    {
        war.owner = 0;
        war.cost = 0;
    }
    else
    {
        war.owner = ch->id;
        war.cost = 50;
        ch->pcdata->questpoints -= 50;
    }

    if (IS_IMMORTAL(ch) && ch->invis_level)
    {
        if ( war.type == ARMAGEDDON_WAR )
            snprintf( buf, sizeof(buf), "An {cArmageddon{6 has been declared for levels %d to %d!\n\r",
                    war.min_level, war.max_level);
        else
            snprintf( buf, sizeof(buf), "A %s war has been declared for levels %d to %d!\n\r",
                    war_list[war.type], war.min_level, war.max_level);
    }
    else
    {
        if ( war.type == ARMAGEDDON_WAR )
            snprintf( buf, sizeof(buf), "An {cArmageddon{6 for levels %d to %d has been instigated by %s!\n\r",
                    war.min_level, war.max_level, ch->name);
        else
            snprintf( buf, sizeof(buf), "A %s war has been declared for levels %d to %d by %s!\n\r",
                    war_list[war.type], war.min_level, war.max_level, ch->name);
    }
    warfare_to_all( buf );
    snprintf( buf, sizeof(buf), "Type 'combat' to join or read 'help warfare' for info.\n\r" );
    warfare_to_all( buf );
    return;
}

DEF_DO_FUN(do_stopwar)
{
    if ( war.on == TRUE )
    {
        war_end( FALSE );
        warfare_to_all("Peace declared.\n\r" );
        return;
    }
    else
    {
        war_end( FALSE );
        send_to_char("There is no war running at the moment.\n\r", ch );
        return;
    }
    return;
}

DEF_DO_FUN(do_combat)
{
    ROOM_INDEX_DATA *location;
    char buf[MSL];

    if ( IS_NPC(ch) || ch->pcdata == NULL )
        return;

    if ( war.on == FALSE )
    {
        send_to_char("There is no war going on at the moment.\n\r", ch );
        return;
    }

    if ( IS_SET( ch->act, PLR_WAR) )
    {
        send_to_char("You're already in the war.\n\r", ch );
        return;
    }

    if (war.started == TRUE)
    {
        send_to_char("Too late, the war has already begun.\n\r", ch );
        return;
    }

    if ( (location = get_room_index( WAR_ROOM_PREP )) == NULL )
    {
        send_to_char("Warzone is not completed.\n\r", ch );
        return;
    }

    if ( ch->level < war.min_level || ch->level > war.max_level )
    {
        send_to_char("You cannot prepare for war at this time.\n\r", ch );
        return;
    }

    if ( war.type == DUEL_WAR && ch->id != war.owner && ch->id != war.duel_target )
    {
        send_to_char("Sorry, this duel doesn't include you!\n\r", ch);
        return;
    }

    if ( war.type == CLAN_WAR && 
            (!is_clan( ch ) || !clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_warfare ))
    {
        send_to_char("This is a clan war.  Clan members only.\n\r", ch );
        return;
    }


    if (IS_TAG(ch))
    {
        send_to_char("You cannot join a war during freeze tag.\n\r",ch);
        return;
    }

    if ( IS_REMORT( ch ) )
    {
        send_to_char("You are remorting, you have no time for silly war games!\n\r", ch );
        return;
    }

    if ( /*IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || IS_AFFECTED(ch, AFF_CURSE) ||*/ ch->was_in_room )
    {
        send_to_char("You have been forsaken and cannot join the war.\n\r", ch );
        return;
    }

    if (!IS_SET( ch->act, PLR_WAR ) )
        SET_BIT( ch->act, PLR_WAR );
    if ( war.combatants == 0 )
        war.first_combatant = ch;
    snprintf( buf, sizeof(buf), "%s (Level %d %s %s %s) has gone to war!\n\r", ch->name, ch->level,
            get_base_sex(ch) == 2 ? "female" : get_base_sex(ch) == 1 ? "male" : "sexless",
            race_table[ch->race].name, class_table[ch->clss].name );
    warfare_to_all( buf );
    war.combatants++;
    send_to_char("Prepare for battle, my child.\n\r", ch );

    affect_freeze_sn(ch, 0);
    die_follower( ch, true );
    ch->pcdata->warfare_hp=ch->hit;
    ch->pcdata->warfare_mana=ch->mana;
    ch->pcdata->warfare_move=ch->move;
    ch->hit = hit_cap(ch);
    ch->mana = mana_cap(ch);
    ch->move = move_cap(ch);
    // players only return to where they were if outside of Bastion when joining
    if ( ch->in_room->area != get_room_index(ROOM_VNUM_RECALL)->area )
        ch->was_in_room = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    do_look( ch, "" );
    return;
}

// return ch to room stored in was_in_room, or to default if nothing stored
void return_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *default_room )
{
    if ( ch->was_in_room )
    {
        char_to_room(ch, ch->was_in_room );
        ch->was_in_room = NULL;
    }
    else
        char_to_room(ch, default_room);
}

DEF_DO_FUN(do_warstatus)
{
    char buf[MSL];
    BUFFER *output;


    if( war.on == FALSE )
    {
        send_to_char( "There is no war going on currently.\n\r", ch );
        if( last_war_time > 0 )
        {
            snprintf( buf, sizeof(buf), "The last war started %ld minutes ago.\n\r", (current_time - last_war_time)/60 );
            send_to_char( buf, ch );
        }
        return;
    }

    output = new_buf();

    snprintf( buf, sizeof(buf), "{R----------------------------------------------------------------{x\n\r\n\r" );
    add_buf( output, buf );

    snprintf( buf, sizeof(buf), "{RThere is a level {Y%d{R to {Y%d %s{R war going on.{x",
            war.min_level, war.max_level, war_list[war.type] );
    add_buf( output, center(buf,65,' ') );
    add_buf( output, "\n\r" );


    snprintf( buf, sizeof(buf), "{RThere %s %d combatant%s in the war at this time.{x",
            war.combatants == 1 ? "is" : "are", war.combatants, war.combatants == 1 ? "" : "s" );
    add_buf( output, center(buf,65,' ') );
    add_buf( output, "\n\r" );

    if( war.started == TRUE  )
    {
        snprintf( buf, sizeof(buf), "{RType 'warsit' to see the status of the players in the war.{x" );
        add_buf( output, center(buf,65,' ') );
        add_buf( output, "\n\r" );

        if( last_war_time > 0 )
        {
            snprintf( buf, sizeof(buf), "{RThe war began %ld minutes ago.{x", (current_time - last_war_time)/60 );
            add_buf( output, center(buf,65,' ') );
            add_buf( output, "\n\r" );
        }
    }
    else
    {
        snprintf( buf, sizeof(buf), "{RThere %s %d tick%s left to join in the war.",
                war.war_time_left == 1 ? "is" : "are", war.war_time_left, war.war_time_left == 1 ? "" : "s" );
        add_buf( output, center(buf,65,' ') );
        add_buf( output, "\n\r" );
    }

    snprintf( buf, sizeof(buf), "\n\r{R----------------------------------------------------------------{x\n\r\n\r" );
    add_buf( output, buf );
    page_to_char( buf_string( output ), ch );
    return;
}

void war_update( void )
{
    char buf[MSL];
    ROOM_INDEX_DATA *random;
    int count = 0;
    CHAR_DATA *ch;

    if (current_time > auto_war_time && war.on == FALSE)
    {
        auto_war();
        return;
    }

    if (war.on == FALSE)
        return;

    if ( war.war_time_left > 0 )
    {
        if ( war.type == DUEL_WAR )
        {
            DESCRIPTOR_DATA *d;
            for ( d=descriptor_list ; d ; d=d->next)
            {
                if ( d->character && 
                        (  d->character->id == war.owner
                        || d->character->id == war.duel_target ) )
                {
                    snprintf( buf, sizeof(buf), "You are challenged to a {DDuel{6.\n\r");
                    warfare_to_char( d->character, buf );
                    snprintf( buf, sizeof(buf), "There %s %d tick%s left to join in the war.\n\r",
                            war.war_time_left == 1 ? "is" : "are", 
                            war.war_time_left, 
                            war.war_time_left == 1 ? "" : "s" );
                    warfare_to_char( d->character, buf );
                }
            }
        }
        else
        {
            snprintf( buf, sizeof(buf), "There is a level %d to %d %s war going on.\n\r",
                    war.min_level, war.max_level, war_list[war.type] );
            warfare_to_all( buf );
            snprintf( buf, sizeof(buf), "There %s %d tick%s left to join in the war.\n\r",
                    war.war_time_left == 1 ? "is" : "are", war.war_time_left, war.war_time_left == 1 ? "" : "s" );
            warfare_to_all( buf );
            snprintf( buf, sizeof(buf), "There %s %d combatant%s in the war at this time.\n\r",
                    war.combatants == 1 ? "is" : "are", war.combatants, war.combatants == 1 ? "" : "s" );
            warfare_to_all( buf );
        }
        war.war_time_left--;
        return;
    }
    if ( war.war_time_left == 0 && war.started == FALSE )
    {
        war.total_combatants = war.combatants;

        if ( war.combatants == 0 || war.combatants == 1 )
        {
            snprintf( buf, sizeof(buf), "Not enough combatants in the war; war cancelled.\n\r" );
            warfare_to_all( buf );
            war.started = FALSE;
            war_end( FALSE );
            return;
        }

        // traverse char_list to catch linkdead players
        for ( ch = char_list; ch; ch = ch->next )
        {
            if ( !PLR_ACT(ch, PLR_WAR) )
                continue;
            if ( !is_same_team( war.first_combatant, ch ) )
                break;
            count++;
        }
        if ( count == war.combatants )
        {
            if ( war.type == CLAN_WAR )
                warfare_to_all( "No opposing clans; war cancelled.\n\r" );
            else if ( war.type == RACE_WAR )
                warfare_to_all( "No opposing races; war cancelled.\n\r" );
            else if ( war.type == CLASS_WAR )
                warfare_to_all( "No opposing classes; war cancelled.\n\r" );
            else if ( war.type == GENDER_WAR )
                warfare_to_all( "No opposing genders; war cancelled.\n\r" );
            else if ( war.type == RELIGION_WAR )
                warfare_to_all( "No opposing religions; war cancelled.\n\r" );
            war_end( FALSE );
            return;
        }

        last_war_time = current_time;

        snprintf( buf, sizeof(buf), "The battle begins with %d combatants in the war!\n\r", war.combatants );
        warfare_to_all( buf );
        war.started = TRUE;
        for ( ch = char_list; ch; ch = ch->next )
        {
            if ( !PLR_ACT(ch, PLR_WAR) )
                continue;

            if (war.type==DUEL_WAR)
                random = get_room_index( number_range( DUEL_ROOM_FIRST, DUEL_ROOM_LAST ) );
            else
                random = get_room_index( number_range( WAR_ROOM_FIRST, WAR_ROOM_LAST ) );
            char_from_room( ch );
            char_to_room ( ch, random );
            do_look( ch, "" );
            /* Stop the Eq-Switching Cheating ... so what you're wearing
               as you enter the warzone gives you your hp/mana/move for the war. */
            ch->hit = hit_cap(ch);
            ch->mana = mana_cap(ch);
            ch->move = move_cap(ch);
        }
        return;
    }
}
static void warfare_to_char( CHAR_DATA *ch, const char *argument)
{
    ptc( ch, "{5WARFARE: {6%s{x", argument );
    return;
}

void warfare_to_all( const char *argument )
{
    DESCRIPTOR_DATA *d;

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (IS_PLAYING(d->connected))
                && d->character != NULL
                && !IS_SET( d->character->comm, COMM_NOWAR )
                && !IS_SET( d->character->comm, COMM_QUIET ) )
            warfare_to_char( d->character, argument );
    }
    return;
}

DEF_DO_FUN(do_nowar)
{
    if ( IS_SET( ch->comm, COMM_NOWAR ) )
    {
        send_to_char("You will now hear {5warfare{x messages.\n\r", ch );
        REMOVE_BIT( ch->comm, COMM_NOWAR );
        return;
    }
    else
    {
        send_to_char("You will no longer hear {5warfare{x messages.\n\r", ch );
        SET_BIT( ch->comm, COMM_NOWAR );
        return;
    }
}


void war_end( bool success )
{
    CHAR_DATA *ch;
    int points=0;
    char buf[80];

    // bought warfares give refund to owner if canceled
    // auto/imm warfares give fail reward to any who joined
    if ( !success && war.owner )
    {
        // refund owner
        for ( ch = char_list; ch; ch = ch->next )
            if ( !IS_NPC(ch) && ch->id == war.owner )
            {
                ptc( ch, "You are refunded your %d qp fee.\n\r", war.cost );   
                ch->pcdata->questpoints += war.cost;
                break;
            }
        // no award for anyone - that would be an exploit
        points = 0;
    }
    else if ( war.type != DUEL_WAR )
    {
        points = get_warfare_reward(success);
        snprintf(buf, sizeof(buf), "You are awarded %d quest points.\n\r", points);
    }

    for ( ch = char_list; ch; ch = ch->next )
    {
        if ( !PLR_ACT(ch, PLR_WAR) )
            continue;
        
        if ( points )
        {
            send_to_char(buf, ch);
            ch->pcdata->questpoints += points;
        }

        stop_fighting( ch, TRUE );
        char_from_room( ch );
        return_to_room( ch, get_room_index(success ? WAR_ROOM_WINNER : ROOM_VNUM_TEMPLE) );
        REMOVE_BIT( ch->act, PLR_WAR );
        affect_strip(ch, 0);
        affect_unfreeze_sn(ch, 0);
        ch->hit = UMAX(1, ch->pcdata->warfare_hp);
        ch->move = ch->pcdata->warfare_move;
        ch->mana = ch->pcdata->warfare_mana;
        update_pos( ch );
        do_look( ch, "" );

        if ( success )
        {
            if ( war.type == ARMAGEDDON_WAR )
                ch->pcdata->armageddon_won++;
            else if ( war.type == CLAN_WAR )
                ch->pcdata->clan_won++;
            else if ( war.type == RACE_WAR )
                ch->pcdata->race_won++;
            else if ( war.type == CLASS_WAR )
                ch->pcdata->class_won++;
            else if ( war.type == GENDER_WAR )
                ch->pcdata->gender_won++;
            else if ( war.type == RELIGION_WAR )
                ch->pcdata->religion_won++;
            else if ( war.type == DUEL_WAR )
                ch->pcdata->duel_won++;
        }
    }

    war.on = FALSE;
    war.started = FALSE;
    return;
}

void add_war_kills( CHAR_DATA *ch )
{
    ch->pcdata->war_kills++;
    update_lboard( LBOARD_WKILL, ch, ch->pcdata->war_kills, 1);
    switch( war.type )
    {
        case ARMAGEDDON_WAR:
            ch->pcdata->armageddon_kills++;
            break;
        case CLAN_WAR:
            ch->pcdata->clan_kills++;
            break;
        case RACE_WAR:
            ch->pcdata->race_kills++;
            break;
        case CLASS_WAR:
            ch->pcdata->class_kills++;
            break;
        case GENDER_WAR:
            ch->pcdata->gender_kills++;
            break;
        case RELIGION_WAR:
            ch->pcdata->religion_kills++;
            break;
        case DUEL_WAR:
            ch->pcdata->duel_kills++;
            break;
        default:
            bug( "Add_war_kills: no war.type", 0 );
            break;
    }
    return;
}

DEF_DO_FUN(do_warsit)
{
    CHAR_DATA *wch;
    BUFFER *output;
    char buf[MSL];
    int hp_percent, mana_percent, move_percent;

    if ( war.on == FALSE )
    {
        send_to_char("There is no war going on at the moment.\n\r", ch );
        return;
    }

    if ( war.started == FALSE )
    {
        /* send_to_char( "The war hasn't started yet.\n\r", ch ); */
        do_warstatus( ch, argument );
        return;
    }

    output = new_buf();
    snprintf( buf, sizeof(buf), "{c%-10s %-8s %-4s %-4s %-4s %-3s %-6s %-3s %-8s %-10s %-10s{x\n\r",
            "Name", "Gender", "hp", "mana", "move", "Lvl", "Race", "Cls", "God", "Clan", "Fighting");
    add_buf( output, buf );
    
    for ( wch = char_list; wch; wch = wch->next )
    {
        if ( !PLR_ACT(wch, PLR_WAR) )
            continue;

        const char *god_name = get_god_name(wch);

        hp_percent = (wch->hit*100) / wch->max_hit;
        mana_percent = (wch->mana*100) / wch->max_mana;
        move_percent = (wch->move*100) / wch->max_move;
        snprintf( buf, sizeof(buf), "%-10s %-8s %3d%% %3d%% %3d%% %3d %-6s %-3s %-8s %-10s %-10s\n\r",
                wch->name,
                get_base_sex(wch) == 2 ? "female" : get_base_sex(wch) == 1 ? "male" : "sexless",
                hp_percent,
                mana_percent,
                move_percent,
                wch->level,
                pc_race_table[wch->race].who_name,
                class_table[wch->clss].who_name,
                god_name ? god_name : "none",
                (wch->clan) ? clan_table[wch->clan].name : "",
                (wch->fighting) ? wch->fighting->name : "" );
        add_buf( output, buf );
    }
    page_to_char( buf_string(output), ch );
    return;
}

void war_remove( CHAR_DATA *ch, bool killed )
{
    CHAR_DATA *wch;
    char buf[MSL];

    if (IS_NPC(ch) || !IS_SET( ch->act, PLR_WAR ) )
        return;

    stop_fighting( ch, TRUE );
    affect_strip(ch, 0);
    affect_unfreeze_sn(ch, 0);
    ch->hit= UMAX(1, ch->pcdata->warfare_hp);
    ch->move=ch->pcdata->warfare_move;
    ch->mana=ch->pcdata->warfare_mana;
    update_pos( ch );

    if ( war.started )
    {
        char_from_room( ch );
        return_to_room( ch, get_room_index(WAR_ROOM_LOSER) );
        switch ( war.type )
        {
            case ARMAGEDDON_WAR:
                ch->pcdata->armageddon_lost++;
                break;
            case CLAN_WAR:
                ch->pcdata->clan_lost++;
                break;
            case RACE_WAR:
                ch->pcdata->race_lost++;
                break;
            case CLASS_WAR:
                ch->pcdata->class_lost++;
                break;
            case GENDER_WAR:
                ch->pcdata->gender_lost++;
                break;
            case RELIGION_WAR:
                ch->pcdata->religion_lost++;
                break;
            case DUEL_WAR:
                ch->pcdata->duel_lost++;
                break;
        }
    }
    else
    {
        char_from_room( ch );
        return_to_room( ch, get_room_index(ROOM_VNUM_TEMPLE) );
    }
    do_look( ch, "" );

    if ( !killed )
    {
        snprintf( buf, sizeof(buf), "%s has been kicked out of the war!\n\r", ch->name );
        warfare_to_all( buf );
    }
    else if ( war.type != DUEL_WAR )
    {
        int joinbonus = get_warfare_reward(FALSE);
        ptc(ch, "You are awarded %d quest points for your bravery!\n\r", joinbonus); 
        ch->pcdata->questpoints += joinbonus;
    }

    /* decrememnt the combatant counter */
    war.combatants--;

    /* remove the war bit */
    REMOVE_BIT( ch->act, PLR_WAR );

    /* replace first_combatant if that was the person removed */
    if ( ch == war.first_combatant )
    {
        for ( wch = char_list; wch; wch = wch->next )
            if ( PLR_ACT(wch, PLR_WAR) && wch != ch )
            {
                war.first_combatant = wch;
                break;
            }
        if ( ch == war.first_combatant )
            bug( "war_remove: no remaining players found (expected %d)", 
                    war.combatants );
    }

    /* check to see if the war is over with this removal */
    check_war_win();

    return;
}

void check_war_win( void )
{
    CHAR_DATA *ch, *wch;
    char buf[MAX_STRING_LENGTH];
    int count=0;

    /* war must start before it can be won */
    if (!war.started)
        return;    

    ch = war.first_combatant;

    if ( (war.type == ARMAGEDDON_WAR || war.type == DUEL_WAR)
            && war.combatants == 1 )
    {
        snprintf( buf, sizeof(buf), "%s emerges from the battlefields victorious!\n\r", ch->name );
        warfare_to_all( buf );
        war_end( TRUE );
    }
    else
    {
        for ( wch = char_list; wch; wch = wch->next )
        {
            if ( !PLR_ACT(wch, PLR_WAR) || wch == ch )
                continue;
            if ( !is_same_team( ch, wch ) )
                break;
            count++;
        }

        if ( count == war.combatants - 1 )
        {
            if ( war.type == CLAN_WAR )
                snprintf( buf, sizeof(buf), "The %s clan has won the war!\n\r", clan_table[ch->clan].name );
            else if ( war.type == CLASS_WAR )
                snprintf( buf, sizeof(buf), "The %ss have won the war!\n\r", class_table[ch->clss].name );
            else if ( war.type == RACE_WAR )
                snprintf( buf, sizeof(buf), "The %ss have won the war!\n\r", race_table[ch->race].name );
            else if ( war.type == GENDER_WAR )
                snprintf( buf, sizeof(buf), "The %s have won the war!\n\r", get_base_sex(ch) == 2 ? "females" : get_base_sex(ch) == 1 ? "males" : "sexless" );
            else if ( war.type == RELIGION_WAR )
            {
                if ( has_god(ch) )
                    snprintf( buf, sizeof(buf), "The followers of %s have won the war!\n\r", get_god_name(ch) );
                else
                    snprintf( buf, sizeof(buf), "The atheists have won the war!\n\r" );
            }
            else /* paranoid */
                snprintf( buf, sizeof(buf), "The war has been won!\n\r" );
            buf[4] = UPPER(buf[4]);
            warfare_to_all( buf );
            war_end( TRUE );
        }
    }

    return;
}

bool is_same_team( CHAR_DATA *ch1, CHAR_DATA *ch2 )
{
    if ( ch1 == ch2 )
        return TRUE;
    if ( IS_NPC(ch1) || IS_NPC(ch2) )
        return FALSE;
    if ( war.type == ARMAGEDDON_WAR )
        return FALSE;
    if ( war.type == DUEL_WAR )
        return FALSE;
    if ( war.type == CLAN_WAR )
        return ( ch1->clan == ch2->clan );
    if ( war.type == RACE_WAR )
        return ( ch1->race == ch2->race );
    if ( war.type == CLASS_WAR )
        return ( ch1->clss == ch2->clss );
    if ( war.type == GENDER_WAR )
        return ( get_base_sex(ch1) == get_base_sex(ch2) );
    if ( war.type == RELIGION_WAR )
        return strcmp(get_god_name(ch1), get_god_name(ch2)) == 0;

    return FALSE;
}

bool in_religion_war( CHAR_DATA *ch )
{
    return IS_SET(ch->act,PLR_WAR) && war.type == RELIGION_WAR;
}
