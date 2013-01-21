/************************************************************************
*                                   *
*   Warfare system by Viper (1998) - All copyright notices of ROM   *
*           original implementors retained.         *
*                                   *
*************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
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

DECLARE_DO_FUN(do_restore);
DECLARE_DO_FUN(do_look);

void proc_startwar( CHAR_DATA *ch, char *argument, bool pay );

char * war_list[] =
{
    "{rArmageddon{6",
    "{cRace{6",
    "{gClass{6",
    "{yClan{6",
    "{mGender{6",
    "{bReligion{6"
};

WAR_DATA war;
long last_war_time = 0;



/* Added for the automated warfares - Astark Nov 2012 */

long auto_war_time = 0;

void do_startwar( CHAR_DATA *ch, char *argument )
{
    proc_startwar( ch, argument, FALSE );
}


/* Function that gets called during war_update, to start
warfares automatically. Astark Nov 2012 */

void auto_war(void)
{
    char buf[MSL];
    int toplevel;
    auto_war_time = current_time + 3600;

    if (number_bits(3))
    {
        switch ( number_range(0, 3) )
        {
        case 0: war.type = CLASS_WAR; break;
        case 1: war.type = RACE_WAR; break;
        case 2: war.type = CLAN_WAR; break;
        case 3: war.type = GENDER_WAR; break;
        }
    }
    else
        war.type = ARMAGEDDON_WAR;

    if (number_bits(1))
        toplevel = number_range(40,100);
    else
        toplevel = number_range(90,100);

    war.max_level = toplevel;
    war.min_level = toplevel - 30;

    war.on = TRUE;
    war.started = FALSE;
    war.combatants = 0;
    war.war_time_left = 4;
    war.first_combatant = NULL;
    war.reward = 4;
    war.owner = 0;

    if ( war.type == ARMAGEDDON_WAR )
        sprintf( buf, "An {cArmageddon{6 has been declared for levels %d to %d!\n\r",
	    war.min_level, war.max_level);
    else
        sprintf( buf, "A %s war has been declared for levels %d to %d!\n\r",
	    war_list[war.type], war.min_level, war.max_level);

    warfare( buf );
    sprintf( buf, "Type 'combat' to join or read 'help warfare' for info.\n\r" );
    warfare( buf );
    return;
    
}

void proc_startwar( CHAR_DATA *ch, char *argument, bool pay )
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
	    send_to_char("Syntax: startwar <type> <min level> <max level>\n\r", ch);
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
	sprintf(buf, "Dont you think a %d spread is a little excessive?\n\r",
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
	war.reward = 0;
	war.owner = 0;
    }
    else
    {
	war.owner = ch->id;
	war.reward = 12;
	ch->pcdata->questpoints -= 50;
    }

    if (IS_IMMORTAL(ch) && ch->invis_level)
    {
	if ( war.type == ARMAGEDDON_WAR )
	    sprintf( buf, "An {cArmageddon{6 has been declared for levels %d to %d!\n\r",
		     war.min_level, war.max_level);
	else
	    sprintf( buf, "A %s war has been declared for levels %d to %d!\n\r",
		     war_list[war.type], war.min_level, war.max_level);
    }
    else
    {
	if ( war.type == ARMAGEDDON_WAR )
	    sprintf( buf, "An {cArmageddon{6 for levels %d to %d has been instigated by %s!\n\r",
		     war.min_level, war.max_level, ch->name);
	else
	    sprintf( buf, "A %s war has been declared for levels %d to %d by %s!\n\r",
		     war_list[war.type], war.min_level, war.max_level, ch->name);
    }
    warfare( buf );
    sprintf( buf, "Type 'combat' to join or read 'help warfare' for info.\n\r" );
    warfare( buf );
    return;
}

void do_stopwar( CHAR_DATA *ch, char *argument )
{
    if ( war.on == TRUE )
    {
        war_end( FALSE );
        warfare("Peace declared.\n\r" );
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

void do_combat( CHAR_DATA *ch, char *argument )
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
    
    if ( IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || IS_AFFECTED(ch, AFF_CURSE) )
    {
        send_to_char("You have been forsaken and cannot join the war.\n\r", ch );
        return;
    }
    
    if (!IS_SET( ch->act, PLR_WAR ) )
        SET_BIT( ch->act, PLR_WAR );
    if ( war.combatants == 0 )
        war.first_combatant = ch;
    sprintf( buf, "%s (Level %d %s %s %s) has gone to war!\n\r", ch->name, ch->level,
		ch->pcdata->true_sex == 2 ? "female" : "male",
		race_table[ch->race].name, class_table[ch->class].name );
    warfare( buf );
    war.combatants++;
    send_to_char("Prepare for battle, my child.\n\r", ch );

    if ( war.type != RELIGION_WAR )
    {
        affect_strip( ch, gsn_god_bless );
        affect_strip( ch, gsn_god_curse );
    }
    die_follower( ch );
    ch->pcdata->warfare_hp=ch->hit;
    ch->pcdata->warfare_mana=ch->mana;
    ch->pcdata->warfare_move=ch->move;
    ch->hit = ch->max_hit;
    ch->mana = ch->max_mana;
    ch->move = ch->max_move;
    ch->pcdata->total_wars++;
    char_from_room( ch );
    char_to_room( ch, location );
    do_look( ch, "" );
    return;
}

void do_warstatus( CHAR_DATA *ch, char *argument )
{
    char buf[MSL];
    BUFFER *output;
    

    if( war.on == FALSE )
    {
	send_to_char( "There is no war going on currently.\n\r", ch );
	if( last_war_time > 0 )
	{
            sprintf( buf, "The last war started %d minutes ago.\n\r", (current_time - last_war_time)/60 );
	    send_to_char( buf, ch );
	}
	return;
    }

    output = new_buf();

    sprintf( buf, "{R----------------------------------------------------------------{x\n\r\n\r" );
    add_buf( output, buf );

    sprintf( buf, "{RThere is a level {Y%d{R to {Y%d %s{R war going on.{x",
        war.min_level, war.max_level, war_list[war.type] );
    add_buf( output, center(buf,65,' ') );
    add_buf( output, "\n\r" );


    sprintf( buf, "{RThere %s %d combatant%s in the war at this time.{x",
        war.combatants == 1 ? "is" : "are", war.combatants, war.combatants == 1 ? "" : "s" );
    add_buf( output, center(buf,65,' ') );
    add_buf( output, "\n\r" );

    if( war.started == TRUE  )
    {
        sprintf( buf, "{RType 'warsit' to see the status of the players in the war.{x" );
        add_buf( output, center(buf,65,' ') );
        add_buf( output, "\n\r" );

	if( last_war_time > 0 )
	{
            sprintf( buf, "{RThe war began %d minutes ago.{x", (current_time - last_war_time)/60 );
	    add_buf( output, center(buf,65,' ') );
            add_buf( output, "\n\r" );
	}
    }
    else
    {
        sprintf( buf, "{RThere %s %d tick%s left to join in the war.",
        war.war_time_left == 1 ? "is" : "are", war.war_time_left, war.war_time_left == 1 ? "" : "s" );
        add_buf( output, center(buf,65,' ') );
        add_buf( output, "\n\r" );
    }
 
    sprintf( buf, "\n\r{R----------------------------------------------------------------{x\n\r\n\r" );
    add_buf( output, buf );
    page_to_char( buf_string( output ), ch );
    return;
}

void war_update( void )
{
    char buf[MSL];
    DESCRIPTOR_DATA *d;
    ROOM_INDEX_DATA *random;
    BUFFER *output;
    int count = 0;
    
    if (current_time > auto_war_time && war.on == FALSE)
        auto_war();
    else if (war.on == FALSE)
        return;

    
    output = new_buf();
    if ( war.war_time_left > 0 )
    {
        sprintf( buf, "There %s %d tick%s left to join in the war.\n\r",
            war.war_time_left == 1 ? "is" : "are", war.war_time_left, war.war_time_left == 1 ? "" : "s" );
        warfare( buf );
        sprintf( buf, "There %s %d combatant%s in the war at this time.\n\r",
            war.combatants == 1 ? "is" : "are", war.combatants, war.combatants == 1 ? "" : "s" );
        warfare( buf );
        war.war_time_left--;
        return;
    }
    if ( war.war_time_left == 0 && war.started == FALSE )
    {
        if ( war.combatants == 0 || war.combatants == 1 )
        {
            sprintf( buf, "Not enough combatants in the war; war cancelled.\n\r" );
            warfare( buf );
            war.started = FALSE;
            war_end( FALSE );
            return;
        }
        
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( !(d->connected == CON_PLAYING || IS_WRITING_NOTE( d->connected ))
                || d->character == NULL
                || !IS_SET( d->character->act, PLR_WAR ) )
                continue;
            if ( !is_same_team( war.first_combatant, d->character ) )
                break;
            count++;
        }
        if ( count == war.combatants )
        {
            if ( war.type == CLAN_WAR )
                warfare( "No opposing clans; war cancelled.\n\r" );
            else if ( war.type == RACE_WAR )
                warfare( "No opposing races; war cancelled.\n\r" );
            else if ( war.type == CLASS_WAR )
                warfare( "No opposing classes; war cancelled.\n\r" );
            else if ( war.type == GENDER_WAR )
                warfare( "No opposing genders; war cancelled.\n\r" );
            else if ( war.type == RELIGION_WAR )
                warfare( "No opposing religions; war cancelled.\n\r" );
            war_end( FALSE );
            return;
        }
        
	last_war_time = current_time;

	war.reward += war.combatants*9;
	war.reward = UMIN(90, war.reward);

        sprintf( buf, "The battle begins with %d combatants in the war!\n\r", war.combatants );
        warfare( buf );
        war.started = TRUE;
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->character == NULL || !IS_SET( d->character->act, PLR_WAR ) )
                continue;
            random = get_room_index( number_range( WAR_ROOM_FIRST, WAR_ROOM_LAST ) );
            char_from_room( d->character );
            char_to_room ( d->character, random );
            do_look( d->character, "" );
	    /* Stop the Eq-Switching Cheating ... so what you're wearing
		as you enter the warzone gives you your hp/mana/move for the war. */
	    d->character->hit = d->character->max_hit;
	    d->character->mana = d->character->max_mana;
	    d->character->move = d->character->max_move;
        }
        return;
    }
}

void warfare( char *argument )
{
    DESCRIPTOR_DATA *d;
    char buf[MSL];
    
    sprintf( buf, "{5WARFARE: {6%s{x", argument );
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( (d->connected == CON_PLAYING || IS_WRITING_NOTE(d->connected))
            && d->character != NULL
            && !IS_SET( d->character->comm, COMM_NOWAR )
            && !IS_SET( d->character->comm, COMM_QUIET ) )
            send_to_char( buf, d->character );
    }
    return;
}

void do_nowar( CHAR_DATA *ch, char *argument )
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
    DESCRIPTOR_DATA *d;
    CHAR_DATA *rch;
    int points;
    char buf[80];
    
    if (points = war.reward/UMAX(war.combatants, 1))
	sprintf(buf, "You are awarded %d quest points.\n\r", points);

    if ( success )
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( (d->connected != CON_PLAYING && !IS_WRITING_NOTE( d->connected ))
                || d->character == NULL || IS_NPC(d->character)
                || !IS_SET( d->character->act, PLR_WAR ) )
                continue;

	    if (points)
	    {
		send_to_char(buf, d->character);
		d->character->pcdata->questpoints += points;
	    }
            
	    stop_fighting( d->character, TRUE );
            char_from_room( d->character );
            char_to_room( d->character, get_room_index( WAR_ROOM_WINNER ) );
            REMOVE_BIT( d->character->act, PLR_WAR );
	    affect_strip_offensive( d->character );
	    d->character->hit= UMAX(1, d->character->pcdata->warfare_hp);
	    d->character->move=d->character->pcdata->warfare_move;
	    d->character->mana=d->character->pcdata->warfare_mana;
            update_pos( d->character );
            do_look( d->character, "" );
            if ( war.type == ARMAGEDDON_WAR )
                d->character->pcdata->armageddon_won++;
            else if ( war.type == CLAN_WAR )
                d->character->pcdata->clan_won++;
            else if ( war.type == RACE_WAR )
                d->character->pcdata->race_won++;
            else if ( war.type == CLASS_WAR )
                d->character->pcdata->class_won++;
            else if ( war.type == GENDER_WAR )
                d->character->pcdata->gender_won++;
            else if ( war.type == RELIGION_WAR )
                d->character->pcdata->religion_won++;
        }
    }
    else
    {
	for (rch = char_list; rch; rch=rch->next)
	    if (!IS_NPC(rch) && rch->id==war.owner)
	    {
		send_to_char("You are refunded your 50 qp fee.\n\r", rch);
		rch->pcdata->questpoints += 50;
	    }

        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( !(d->connected == CON_PLAYING || IS_WRITING_NOTE( d->connected ))
                || d->character == NULL || IS_NPC(d->character)
                || !IS_SET( d->character->act, PLR_WAR ) )
                continue;
            REMOVE_BIT( d->character->act, PLR_WAR );
	    affect_strip_offensive( d->character );
	    d->character->hit= UMAX(1, d->character->pcdata->warfare_hp);
	    d->character->move=d->character->pcdata->warfare_move;
	    d->character->mana=d->character->pcdata->warfare_mana;
	    stop_fighting( d->character, TRUE );
            char_from_room( d->character );
            char_to_room( d->character, get_room_index( ROOM_VNUM_TEMPLE ) );
            do_look( d->character, "" );
            d->character->pcdata->total_wars--;
        }
    }

    war.on = FALSE;
    war.started = FALSE;
    return;
}

void add_war_kills( CHAR_DATA *ch )
{
    ch->pcdata->war_kills++;
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
    default:
        bug( "Add_war_kills: no war.type", 0 );
        break;
    }
    return;
}

void do_warsit( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    BUFFER *output;
    char buf[MSL];
    int hp_percent = 0;
    
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
    sprintf( buf, "{c%-10s %-8s %-4s %-8s %-3s %-6s %-3s %-8s %-10s %-10s{x\n\r",
        "Name", "Gender", "%%hp", "Position", "Lvl", "Race", "Cls", "God", "Clan", "Fighting");
    add_buf( output, buf );
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	char *god_name;

        if ( !(d->connected == CON_PLAYING || IS_WRITING_NOTE( d->connected ))
            || d->character == NULL
            || !IS_SET( d->character->act, PLR_WAR ) )
            continue;

	god_name = get_god_name(d->character);

        hp_percent = (d->character->hit*100)/d->character->max_hit;
        sprintf( buf, "%-10s %-8s %-3d%% %-8s %3d %-6s %-3s %-8s %-10s %-10s\n\r",
		 d->character->name,
		 d->character->pcdata->true_sex == 2 ? "female"
		 : d->character->pcdata->true_sex == 1 ? "male" : "sexless",
		 hp_percent,
		 position_table[d->character->position].name,
		 d->character->level,
		 pc_race_table[d->character->race].who_name,
		 class_table[d->character->class].who_name,
		 god_name ? god_name : "none",
		 (d->character->clan) ? clan_table[d->character->clan].name : "",
		 (d->character->fighting) ? d->character->fighting->name : "" );
        add_buf( output, buf );
    }
    page_to_char( buf_string(output), ch );
    return;
}

void war_remove( CHAR_DATA *ch, bool killed )
{
    DESCRIPTOR_DATA *d;
    char buf[MSL];
    
    if (IS_NPC(ch) || !IS_SET( ch->act, PLR_WAR ) )
        return;
    
    stop_fighting( ch, TRUE );
    affect_strip_offensive( ch );
    ch->hit= UMAX(1, ch->pcdata->warfare_hp);
    ch->move=ch->pcdata->warfare_move;
    ch->mana=ch->pcdata->warfare_mana;
    update_pos( ch );

    if ( war.started )
    {
	char_from_room( ch );
	char_to_room( ch, get_room_index(WAR_ROOM_LOSER) );
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
	}
    }
    else
    {
	ch->pcdata->total_wars--;
	char_from_room( ch );
	char_to_room( ch, get_room_index(ROOM_VNUM_TEMPLE) );
    }
    do_look( ch, "" );

    if (!killed)
    {
	sprintf( buf, "%s has been kicked out of the war!\n\r", ch->name );
	warfare( buf );
    }
    
    /* decrememnt the combatant counter */
    war.combatants--;
    
    /* remove the war bit */
    REMOVE_BIT( ch->act, PLR_WAR );
    
    /* replace first_combatant if that was the person removed */
    if ( ch == war.first_combatant )
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
            if ( d->character != NULL
		 && !IS_NPC( d->character )
		 && IS_SET( d->character->act, PLR_WAR )
		 && d->character != ch )
            {
                war.first_combatant = d->character;
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
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int count=0;

    /* war must start before it can be won */
    if (!war.started)
        return;    

    ch = war.first_combatant;
    
    if ( war.type == ARMAGEDDON_WAR && war.combatants == 1 )
    {
        sprintf( buf, "%s emerges from the battlefields victorious!\n\r", ch->name );
        warfare( buf );
        war_end( TRUE );
    }
    else
    {
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( !( d->connected == CON_PLAYING || IS_WRITING_NOTE( d->connected ) )
                || d->character == NULL
                || !IS_SET( d->character->act, PLR_WAR )
                || d->character == ch )
                continue;
            if ( !is_same_team( ch, d->character ) )
                break;
            count++;
        }
        
        if ( count == war.combatants - 1 )
        {
            if ( war.type == CLAN_WAR )
                sprintf( buf, "The %s clan has won the war!\n\r", clan_table[ch->clan].name );
            else if ( war.type == CLASS_WAR )
                sprintf( buf, "The %ss have won the war!\n\r", class_table[ch->class].name );
            else if ( war.type == RACE_WAR )
                sprintf( buf, "The %ss have won the war!\n\r", race_table[ch->race].name );
            else if ( war.type == GENDER_WAR )
                sprintf( buf, "The %s have won the war!\n\r", ch->pcdata->true_sex == 2 ? "females" : ch->pcdata->true_sex == 1 ? "males" : "sexless" );
	    else if ( war.type == RELIGION_WAR )
	    {
		RELIGION_DATA *rel = get_religion(ch);
                sprintf( buf, "The %s have won the war!\n\r",
			 rel == NULL ? "atheists" : rel->name );
	    }
	    else /* paranoid */
		sprintf( buf, "The war has been won!\n\r" );
            buf[4] = UPPER(buf[4]);
            warfare( buf );
            war_end( TRUE );
        }
    }
    
    return;
}

bool is_same_team( CHAR_DATA *ch1, CHAR_DATA *ch2 )
{
    if ( IS_NPC(ch1) || IS_NPC(ch2) )
	return FALSE;
    if ( war.type == ARMAGEDDON_WAR )
        return FALSE;
    if ( ch1 == ch2 )
        return TRUE;
    if ( war.type == CLAN_WAR )
        return ( ch1->clan == ch2->clan );
    if ( war.type == RACE_WAR )
        return ( ch1->race == ch2->race );
    if ( war.type == CLASS_WAR )
        return ( ch1->class == ch2->class );
    if ( war.type == GENDER_WAR )
        return ( ch1->pcdata->true_sex == ch2->pcdata->true_sex );
    if ( war.type == RELIGION_WAR )
        return ( get_religion(ch1) == get_religion(ch2) );
    
    return FALSE;
}

bool in_religion_war( CHAR_DATA *ch )
{
    return IS_SET(ch->act,PLR_WAR) && war.type == RELIGION_WAR;
}
