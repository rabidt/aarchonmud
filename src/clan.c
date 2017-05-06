/********************************************************************* 
* File-based clans by Brian Castle (aka "Rimbol"), written 01/2000. *
* For use by Aarchon MUD, a ROM 2.4b4 based world.                  *
*                                                                   *
* Portions inspired by Smaug 1.4 codebase.                          *
*********************************************************************/

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"
#include "recycle.h"
#include "buffer_util.h"
#include "simsave.h"

/* Locals */
void load_clan_file (const char *filename);
void fread_clan( FILE *fp, int clannum );
void fread_clan_rank( FILE *fp, int clannum, int ranknum );

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value ) if ( !str_cmp( word, literal ) ) { field  = value; fMatch = TRUE; break; }


struct clan_data clan_table[MAX_CLAN];


/* 
  Read the clan list file (similar to area.lst).  -Rim 1/2000

  Note: It is assumed that clan number zero and rank number zero are null.  I've 
  made a clan file labelled "None" to override the hard-coded default initializations
  below if necessary, but do not make the mistake of using clan zero as a true clan. 
*/
void load_clans( void )
{
    FILE *fpList;
    const char *filename;
    int i,j;
    
    /* Initialize clan table */
    for (i = 0; i < MAX_CLAN; i++)
    {
        clan_table[i].allow_recruits  = 0;
        clan_table[i].creation_date   = 0;
        clan_table[i].donation        = ROOM_VNUM_DONATION;
        clan_table[i].hall            = ROOM_VNUM_RECALL;
        clan_table[i].invitation_only = 0;
        clan_table[i].mobdeaths       = 0;
        clan_table[i].mobkills        = 0;
        clan_table[i].name            = str_dup( "" );
        clan_table[i].patron          = str_dup( "" );
	clan_table[i].motd	      = str_dup( "" );
        clan_table[i].pdeaths         = 0;
        clan_table[i].pkills          = 0;
        clan_table[i].rank_count      = 0;
        clan_table[i].active          = 0;
        clan_table[i].who_color       = str_dup( "" );
        clan_table[i].who_name        = str_dup( "" );
        clan_table[i].min_align       = -1000;
        clan_table[i].max_align       = 1000;
        clan_table[i].changed         = 0;
        
        for (j = 0; j < MAX_CLAN_RANK; j++)
        {
            clan_table[i].rank_list[j].available_slots    = -1; /* -1 = unlimited */
            clan_table[i].rank_list[j].can_use_clantalk   = 0;
            clan_table[i].rank_list[j].clanwar_pkill      = 0;
            clan_table[i].rank_list[j].max_promote_rank   = 0;
            clan_table[i].rank_list[j].min_level          = 0;
            clan_table[i].rank_list[j].name               = str_dup ( "" );
            clan_table[i].rank_list[j].who_name           = str_dup ( "" );
            clan_table[i].rank_list[j].can_marry          = 0;
            clan_table[i].rank_list[j].can_note           = 0;
            clan_table[i].rank_list[j].can_warfare        = 1;
            clan_table[i].rank_list[j].can_declare_war    = 0;
            clan_table[i].rank_list[j].can_declare_truce  = 0;
            clan_table[i].rank_list[j].can_declare_treaty = 0;
        }
    }
    
    
    if ( ( fpList = fopen( CLAN_DIR CLAN_LIST, "r" ) ) == NULL )
    {
        log_error( CLAN_LIST );
        exit( 1 );
    }
    
    for ( ; ; )
    {
        filename = feof( fpList ) ? "$" : fread_word( fpList );
        
        if ( filename[0] == '$' )
            break;
        else
            load_clan_file( filename );
    }
    
    fclose( fpList );
    return;
}


/* Load a specific clan file referenced in the clan list. -Rim 1/2000*/
void load_clan_file(const char *filename)
{
    FILE *fp;
    char path[MAX_INPUT_LENGTH];
    int clannum = -1;
    int ranknum = -1;
    
    sprintf(path, "%s%s", CLAN_DIR, filename);
    
    if ( (fp = fopen( path, "r" ) ) == NULL )
    {
        bugf("Load_clan_file: Cannot open file %s.\n\r", filename);
        exit (1);
    }
    
    for ( ; ; )
    {
        char letter;
        const char *word;
        
        letter = fread_letter( fp );
        if ( letter == '*' )
        {
            fread_to_eol( fp );
            continue;
        }
        
        if ( letter != '#' )
        {
            bugf( "Load_clan_file: # not found in %s.", filename );
            break;
        }
        
        word    = fread_word( fp );
        
        if ( !str_cmp( word, "CLAN" ) ) 
        {
            clannum = fread_number(fp);
            
            if (clannum < 0 || clannum > MAX_CLAN)
            {
                bugf( "Load_clan_file: Invalid clan %d.", clannum);
                break;
            }
            else
                fread_clan( fp, clannum );
        }
        else if ( !str_cmp( word, "RANK" ) ) 
        {
            if (clannum < 0)
            {
                bug( "Load_clan_file: Found rank before clan record.", 0);
                break;
            }
            
            ranknum = fread_number(fp);
            
            if (ranknum < 0 || ranknum > MAX_CLAN_RANK)
            {
                bugf( "Load_clan_file: Invalid rank %d.", ranknum);
                break;
            }
            else
            {
                clan_table[clannum].rank_count++;
                fread_clan_rank ( fp, clannum, ranknum );
            }
        }
        else if ( !str_cmp( word, "END"  ) ) break;
        else
        {
            bugf( "Load_clan_file: bad section in %s.", filename );
            break;
        }
    }
    
    fclose(fp);
    
}


/* Read in a block of clan data from a clan file. -Rim 1/2000*/
void fread_clan( FILE *fp, int clannum )
{
    const char *word;
    bool fMatch;
    CLAN_DATA *clan;
    
    clan = &(clan_table[clannum]);
    
    
    for ( ; ; )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        
        switch ( UPPER(word[0]) )        {
        case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;
            
        case 'A':
            KEY( "Active",        clan->active,         fread_number( fp ) );
            KEY( "AllowRecruits", clan->allow_recruits, fread_number( fp ));
            break;
            
        case 'C':
            if ( !str_cmp( word, "CreationDate" ) )
            {
                clan->creation_date = fread_number( fp );
                if (clan->creation_date <= 0)
                {
                    clan->creation_date = current_time;
                    clan->changed = TRUE;
                }

                fMatch = TRUE;
            }
            break;
            
        case 'D':
            KEY( "Donation",    clan->donation, fread_number( fp ) );
            break;
            
        case 'E':
            if ( !str_cmp( word, "End" ) )
                return;
            break;

        case 'F':
            KEY( "Filename",  clan->filename, fread_string( fp ));
            break;
            
        case 'I':
            KEY( "InviteOnly",  clan->invitation_only, fread_number( fp ));
            break;
            
        case 'M':
            KEY( "MaxAlign",    clan->max_align, fread_number ( fp ) );
            KEY( "MinAlign",    clan->min_align, fread_number ( fp ) );
            KEY( "MobDeaths",   clan->mobdeaths, fread_number ( fp ) );
            KEY( "MobKills",    clan->mobkills,  fread_number ( fp ) );
	    KEY( "Motd",	clan->motd,	 fread_string ( fp ) );
            break;
            
        case 'N':
            KEY( "Name",        clan->name,      fread_string( fp ) );
            break;
            
        case 'P':
            KEY( "Patron",      clan->patron,    fread_string ( fp ) );
            KEY( "PDeaths",     clan->pdeaths,   fread_number ( fp ) );
            KEY( "PKills",      clan->pkills,    fread_number ( fp ) );
            break;
            
        case 'R':
            if ( !str_cmp( word, "RANK" ) )
            {
                fread_clan_rank(fp, clannum, fread_number(fp));
                fMatch = TRUE;
                break;
            }
            KEY( "Recall",      clan->hall, fread_number( fp ) );
            break;
            
        case 'W':
            KEY( "WhoName",     clan->who_name,  fread_string( fp ) );
            KEY( "WhoColor",    clan->who_color, fread_string( fp ) );
            break;
        }
    }
    
    if ( !fMatch )
        bugf( "Fread_clan: no match: %s", word );
}


/* Read in a block of clan rank data from a clan file. -Rim 1/2000*/
void fread_clan_rank( FILE *fp, int clannum, int ranknum )
{
    const char *word;
    bool fMatch;
    CLAN_RANK_DATA *rank;
    
    rank = &(clan_table[clannum].rank_list[ranknum]);
    
    
    for ( ; ; )
    {
        word   = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        
        switch ( UPPER(word[0]) )        {
        case '*':
            fMatch = TRUE;
            fread_to_eol( fp );
            break;
            
        case 'C':
            KEY( "ClanwarPK",  rank->clanwar_pkill,    fread_number( fp ) );
            KEY( "Clantalk",   rank->can_use_clantalk, fread_number( fp ) );
            break;
            
        case 'D':
            KEY( "DeclTreaty", rank->can_declare_treaty, fread_number( fp ) );
            KEY( "DeclTruce",  rank->can_declare_truce,  fread_number( fp ) );
            KEY( "DeclWar",    rank->can_declare_war,    fread_number( fp ) );
            break;
            
        case 'E':
            if ( !str_cmp( word, "End" ) )
                return;
            break;

        case 'I':
            KEY( "Invite",     rank->can_invite,       fread_number( fp ) );
            break;
            
        case 'M':
            KEY( "MaxCount",   rank->available_slots,  fread_number( fp ) );
            KEY( "MaxPromote", rank->max_promote_rank, fread_number( fp ) );
            KEY( "Marry",      rank->can_marry,        fread_number( fp ) );
            KEY( "MinLevel",   rank->min_level,        fread_number( fp ) );
            break;
            
        case 'N':
            KEY( "Name",       rank->name,             fread_string( fp ) );
            KEY( "Note",       rank->can_note,         fread_number( fp ) );
            break;

	case 'S':
	    KEY( "SetMotd",    rank->can_set_motd,     fread_number( fp ) );
	    break;

        case 'W':
            KEY( "WhoName",    rank->who_name,         fread_string( fp ) );
            break;
        }
    }
    
    if ( !fMatch )
        bugf( "Fread_clan_rank: no match: %s", word );
}

/* Validate the clan and rank of any clan eq on a player, and force them to
   drop anything they are not allowed to wear.  -Sardonic 12/99 */
void check_clan_eq(CHAR_DATA *victim)
{
    int iWear = 0;
    OBJ_DATA* obj;

    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
        if ( ( obj = get_eq_char( victim, iWear ) ) == NULL )
            continue;
        
        if (IS_NPC(victim) || (obj->clan&&obj->clan!=victim->clan)||(obj->rank&&obj->rank>victim->pcdata->clan_rank))
        {
            obj_from_char( obj );			
            obj_to_room( obj, victim->in_room );
            act( "$n is zapped by $p.", victim, obj, NULL, TO_ROOM );
            act( "You are zapped by $p.", victim, obj, NULL, TO_CHAR );
        }
    }
}

/* Determine if the rank desired is available, and make the appropriate change to the
   available_slots counters. -Rim 1/2000*/
bool rank_available(int clan, int current_rank, int new_rank)
{
    /* If desired rank is a rejection of the clan, only need to make the vacated slot
       available again by incrementing the current rank's counter. */
    if (new_rank == 0)
    {
        if (clan_table[clan].rank_list[current_rank].available_slots >= 0)
        {
            clan_table[clan].rank_list[current_rank].available_slots++;
            clan_table[clan].changed = TRUE;
            
            return TRUE;
        }
        else  /* Current rank is not restricted, no need to increment. */
            return TRUE;
    }

    /* If desired rank is restricted and no positions available, disallow the change.
       Note that this can happen with both a demotion and a promotion! */
    if (clan_table[clan].rank_list[new_rank].available_slots == 0)
    {
        return FALSE;
    }

    /* The new rank is available.  Now increment current rank and decrement
       desired rank as appropriate, and return a positive result. */
    if (clan_table[clan].rank_list[current_rank].available_slots >= 0)
    {
        clan_table[clan].rank_list[current_rank].available_slots++;
        clan_table[clan].changed = TRUE;
    }

    if (clan_table[clan].rank_list[new_rank].available_slots >= 0)
    {
        clan_table[clan].rank_list[new_rank].available_slots--;
        clan_table[clan].changed = TRUE;
    }

    return TRUE;
}


/*
* Promote / demote clan rankings, by Rimbol. (9/13/97)
*/
DEF_DO_FUN(do_rank)
{
    char arg_char[MAX_INPUT_LENGTH]; /* Character name */
    char arg_rank[MAX_INPUT_LENGTH]; /* Ranking desired */
    char arg_clan[MAX_INPUT_LENGTH]; /* Clan name (IMP only) */
    
    int rank; /* Destination rank number */
    
    CHAR_DATA *victim;


    
    argument = one_argument( argument, arg_char );
    argument = one_argument( argument, arg_rank );
    argument = one_argument( argument, arg_clan );
    
    if ( IS_NPC(ch) )
    {
        send_to_char( "Switch back first.\n\r",ch);
        return;
    }
    
    if ( !clan_table[ch->clan].active && get_trust(ch) < MAX_LEVEL )
    {
        send_to_char( "You are not a member of a clan.\n\r",ch);
        return;
    }
    
    if (clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].max_promote_rank < 1
        && get_trust(ch) < MAX_LEVEL )
    {
        send_to_char( "Clan members of your rank cannot promote or demote others.\n\r",ch);
        return;
    }
    
    if ( arg_char[0] == '\0' || arg_rank[0] == '\0')
    {
        send_to_char( "Syntax: rank <player> <new rank>\n\r",ch);
        send_to_char( "        rank <player> promote\n\r",ch);
        send_to_char( "        rank <player> demote\n\r",ch);
        send_to_char( "        rank <player> boot\n\r",ch);
        send_to_char( "See HELP RANK for a list of possible clan rankings.\n\r",ch);
        return;
    }
    
    if ((victim = get_char_world(ch, arg_char)) == NULL)
    {
        send_to_char("You can't detect any such player.\n\r", ch);
        return;
    }
    
    if ( IS_NPC(victim) )
    {
        send_to_char("Yeah right.\n\r",ch);
        return;
    }
    
    if ( !clan_table[victim->clan].active || victim->pcdata->clan_rank == 0)
    {
        send_to_char( "Clanless characters may not be promoted.  Have them recruit\n\r",ch);
        send_to_char( "themselves to your clan first.  (see HELP RECRUIT)\n\r",ch);
        return;
    }
    
    /*  Not sure about this.  It's history for the moment.
    if ( ( victim = get_char_room( ch, arg_char ) ) == NULL )
    {
    send_to_char( "They must be present for you to do that.\n\r", ch );
    return;
    }
    */ 
    
    if ( victim->clan != ch->clan )
    {
        if ( get_trust(ch) == MAX_LEVEL )  /* Allow IMP to rank chars in other clans */
        {
            if (victim->clan == clan_lookup(arg_clan))
            {
                /* Command is allowed, and will be executed as normal below. */
            }
            else
            {
                send_to_char("(IMP Only) If you want to promote/demote against a clan other than\n\r",ch);
                send_to_char("           your own, specify that clan name (in full) as a third\n\r",ch);
                send_to_char("           argument.\n\r",ch);
                return;
            }
        }
        else
        {
            send_to_char("You cannot change the rank of a player who is not in your clan.\n\r",ch);
            
            return;
        }
    }
    
    /* Figure out the numeric rank being attempted */
    if (!strcmp(arg_rank, "promote"))
        rank = UMIN(victim->pcdata->clan_rank + 1, clan_table[victim->clan].rank_count);
    else if (!strcmp(arg_rank, "demote"))
        rank = UMAX(victim->pcdata->clan_rank - 1, 1); /* Must use boot to get to 0 */
    else if (!strcmp(arg_rank, "boot"))
        rank = 0;
    else if ((rank = clan_rank_lookup(victim->clan, arg_rank)) < 1)
    {
        send_to_char("That is not a clan rank.  Please read HELP RANK.\n\r",ch);
        return;
    }
    
    if (clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].max_promote_rank < victim->pcdata->clan_rank
        && get_trust(ch) < MAX_LEVEL )
    {
        send_to_char("Your patron will not allow you to change that person's rank.\n\r",ch);
        return;
    }
    
    if (clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].max_promote_rank < rank
        && get_trust(ch) < MAX_LEVEL )
    {
        printf_to_char(ch, "You may only affect ranks %s or lower.\n\r",
            capitalize(clan_table[ch->clan].rank_list[clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].max_promote_rank].name));
        return;
    }

    if (victim->pcdata->clan_rank == rank) /* User is either being ignorant or needs to use "boot" */
    {
        printf_to_char(ch, "%s is already rank %s.  Nothing was done.\n\r", victim->name, capitalize(clan_table[victim->clan].rank_list[victim->pcdata->clan_rank].name));
        return;
    }
    else if (victim->pcdata->clan_rank < rank)  /* Trying to promote */
    {
        if (victim->level < clan_table[victim->clan].rank_list[rank].min_level && victim->pcdata->remorts < 1)
        {
            printf_to_char(ch, "%s must attain level %d before they may be promoted to %s.\n\r",
                victim->name, clan_table[victim->clan].rank_list[rank].min_level, capitalize(clan_table[victim->clan].rank_list[rank].name));
            return;
        }

        sprintf(log_buf, "%s: %s has been raised to the rank of %s within clan %s!",
            ch->name, victim->name, capitalize(clan_table[victim->clan].rank_list[rank].name), 
            capitalize(clan_table[victim->clan].name));
    }
    else if (rank == 0)  /* Trying to boot */
    {
        sprintf(log_buf, "%s has booted %s out of clan %s!",
        ch->name, victim->name, capitalize(clan_table[victim->clan].name));
    }
    else  /* Trying to demote */
    {
        sprintf(log_buf, "%s: %s has been demoted to the rank of %s within clan %s!",
        ch->name, victim->name, capitalize(clan_table[victim->clan].rank_list[rank].name),
        capitalize(clan_table[victim->clan].name));
    }

    /* Note!  This function call modifies the counters for available positions of each
       rank, and thus must be made just prior to performing the rank change!  Doing so
       before the possibility of any other failure condition will upset the counters. */
    if (!rank_available(victim->clan, victim->pcdata->clan_rank, rank))
    {
        printf_to_char(ch, "There are no more %s positions available.  If you are trying to demote, try a lower rank.\n\r",
            capitalize(clan_table[victim->clan].rank_list[rank].name));
        return;
    }

    log_string(log_buf);
    
    if (IS_IMMORTAL(victim))
        printf_to_char(victim, "You are now rank %s within your clan.\n\r",
        capitalize(clan_table[victim->clan].rank_list[rank].name));
    else
        info_message(ch, log_buf, TRUE);
    
    
    victim->pcdata->clan_rank = rank;
    
    if (rank == 0)
        victim->clan = 0;

    check_clan_eq(victim);
    
    return;
}    

/*
* Recruit command, by Rimbol.  (9/13/97)
*/
DEF_DO_FUN(do_recruit)
{
    char arg1[MAX_INPUT_LENGTH]; /* Character name */
    int clan, i;
    
    argument = one_argument( argument, arg1 );
    
    if (IS_NPC(ch))
    {
        send_to_char("Not for NPC's.\n\r",ch);
        return;
    }
    
    if (arg1[0] == '\0')
    {
        send_to_char("What clan do you wish to become a recruit of?\n\r",ch);
        return;
    }
    
    if ((clan = clan_lookup(arg1)) == 0)
    {
        send_to_char("That clan does not exist.\n\r",ch);
        return;
    }
    
    if (ch->clan != 0)
    {
        send_to_char("You must first reject your current clan before you may join another.\n\r",ch);
        return;
    }
    
    if (clan_table[clan].active == FALSE)
    {
        send_to_char("Joining inactive clans is not allowed.\n\r",ch);
        return;
    }
    
    if (clan_table[clan].allow_recruits == FALSE)
    {
        send_to_char("That clan is not accepting recruits right now.\n\r", ch);
        return;
    }

    if (clan_table[clan].invitation_only &&
        ch->pcdata->invitation[clan] == NULL)
    {
        send_to_char("That clan is invitation only, and you have not been invited.\n\r", ch);
        return;
    }

    if (   ch->alignment < clan_table[clan].min_align
        || ch->alignment > clan_table[clan].max_align)
    {
        printf_to_char(ch,"You are too %s to join that clan.\n\r",
            ch->alignment < clan_table[clan].min_align ? "{revil{x" : "{wgood{x");
        return;
    }

    /* If you've remorted you can be recruited below level 10 - Astark Dec 2012 */
    if (ch->level < clan_table[clan].rank_list[1].min_level && ch->pcdata->remorts < 1)
    {
        printf_to_char(ch, "You must reach level %d before you may recruit yourself to a clan.\n\r",
            clan_table[clan].rank_list[1].min_level);
        return;
    }

    if (!rank_available(clan, 0, 1))
    {
        printf_to_char(ch, "There are no more %s positions available.\n\r",
            clan_table[clan].rank_list[1].name);
        return;
    }
    
    ch->clan = clan;
    ch->pcdata->clan_rank = 1;

    
    if (clan_table[clan].invitation_only)
        sprintf(log_buf, "By invitation of %s, %s has become a recruit of clan %s!", 
        ch->pcdata->invitation[clan], 
        ch->name, 
        capitalize(clan_table[clan].name));
    else
        sprintf(log_buf, "%s has become a recruit of clan %s!", ch->name, capitalize(clan_table[clan].name));
    
    info_message(ch, log_buf, TRUE);
    log_string(log_buf);


    /* Player has chosen.  All invitations are now cleared. */
    for (i = 0; i < MAX_CLAN; i++)
    {
        free_string(ch->pcdata->invitation[i]);
        ch->pcdata->invitation[i] = NULL;
    }
}


/*
* Reject command, by Rimbol.  (9/13/97)
*/
DEF_DO_FUN(do_reject)
{
    char arg1[MAX_INPUT_LENGTH]; /* Character name */
    int clan;
    int iWear = 0;
    OBJ_DATA* obj;
    
    argument = one_argument( argument, arg1 );
    
    if (IS_NPC(ch))
    {
        send_to_char("Not for NPC's.\n\r",ch);
        return;
    }
    
    if ( IS_SET( ch->act, PLR_WAR ) )
    {
        send_to_char("Wait until the war is over before rejecting!\n\r", ch );
        return;
    }
    
    if ( in_pkill_battle( ch ) || ch->pcdata->pkill_timer > 0 )
    {
	send_to_char( "You won't escape that easily!\n\r", ch );
	return;
    }

    if (arg1[0] == '\0')
    {
        send_to_char("Syntax: reject <clan name>\n\r",ch);
        return;
    }
    
    if ((clan = clan_lookup(arg1)) == 0)
    {
        send_to_char("That clan does not exist.\n\r",ch);
        return;
    }
    
    if (ch->clan != clan)
    {
        send_to_char("You are not a member of that clan.\n\r",ch);
        return;
    }

    rank_available(ch->clan, ch->pcdata->clan_rank, 0); /* Return code irrelevant, no restrictions on reject. */
    
    ch->clan = 0;
    ch->pcdata->clan_rank = 0;
    
    sprintf(log_buf, "%s has rejected clan %s!", ch->name, capitalize(clan_table[clan].name));
    
    info_message(ch, log_buf, TRUE);
    log_string(log_buf);
    
    /* take their clan eq - Sardonic 11/99 */
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
        if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
            continue;
        
        if ((obj->clan && obj->clan != ch->clan) || (obj->rank && obj->rank > ch->pcdata->clan_rank))
        {
            obj_from_char( obj );			
            obj_to_room( obj, ch->in_room );
            act( "$n is zapped by $p.", ch, obj, NULL, TO_ROOM );
            act( "You are zapped by $p.", ch, obj, NULL, TO_CHAR );
        }
    }
}


/* Display current status of a given clan's setting. -Rim 1/2000*/
DEF_DO_FUN(do_clanreport)
{
    char arg1[MIL];
    char arg2[MIL];
    sh_int clan = 0;
    bool  showall   = FALSE;  /* Toggle to show all info (for imms) */
    bool  showranks = FALSE;
    int j;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    char timebuf[25];
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( (clan = clan_lookup(arg1)) < 1 )
    {
        send_to_char("Clan not found.\n\r",ch);
        return;
    }
    
    if ( !str_cmp(arg2, "rank") )
        showranks = TRUE;
    
    if (IS_IMMORTAL(ch))
        showall = TRUE;
    
    strftime(timebuf, 25, "%B %d, %Y", localtime( &(clan_table[clan].creation_date)));
    
    buffer = new_buf();
    
    sprintf(buf, "{+Report for clan %s%s{x:\n\r\n\r", 
        clan_table[clan].who_color, 
        capitalize(clan_table[clan].name));
    add_buf(buffer, buf);
    
    sprintf(buf, "{+           Currently Active{x: {c%s{x\n\r", clan_table[clan].active ? "{gYes{x" : "{rNo{x");
    add_buf(buffer, buf);
    sprintf(buf, "{+                    Created{x: {c%s{x\n\r", timebuf);
    add_buf(buffer, buf);
    sprintf(buf, "{+                     Patron{x: {c%s{x\n\r", clan_table[clan].patron);
    add_buf(buffer, buf);
    sprintf(buf, "{+Currently allowing recruits{x: {c%s{x\n\r", clan_table[clan].allow_recruits ? "{gYes{x" : "{rNo{x");
    add_buf(buffer, buf);
    sprintf(buf, "{+Recruits by invitation only{x: {c%s{x\n\r", clan_table[clan].invitation_only ? "{gYes{x" : "{rNo{x");
    add_buf(buffer, buf);
    sprintf(buf, "{+          Minimum alignment{x: {c%d{x\n\r", clan_table[clan].min_align);
    add_buf(buffer, buf);
    sprintf(buf, "{+          Maximum alignment{x: {c%d{x\n\r", clan_table[clan].max_align);
    add_buf(buffer, buf);
    sprintf(buf, "{+             Deaths by mobs{x: {c%ld{x\n\r", clan_table[clan].mobdeaths);
    add_buf(buffer, buf);
    sprintf(buf, "{+                Mobs killed{x: {c%ld{x\n\r", clan_table[clan].mobkills);
    add_buf(buffer, buf);
    sprintf(buf, "{+          Deaths by players{x: {c%ld{x\n\r", clan_table[clan].pdeaths);
    add_buf(buffer, buf);
    sprintf(buf, "{+             Players killed{x: {c%ld{x\n\r", clan_table[clan].pkills);
    add_buf(buffer, buf);
    
    if (showall)
    {
        sprintf(buf, "{+              Donation vnum{x: {c%d{x\n\r", clan_table[clan].donation);
        add_buf(buffer, buf);
        sprintf(buf, "{+                Recall vnum{x: {c%d{x\n\r", clan_table[clan].hall);
        add_buf(buffer, buf);
    }        
    
    sprintf(buf, "{+            Number of ranks{x: {c%d{x\n\r", clan_table[clan].rank_count);
    add_buf(buffer, buf);
    
    if (!showranks)
    {
        sprintf(buf, "Type 'clanreport <clan_name> rank' to display details about the clan's ranks.\n\r");
        add_buf(buffer, buf);
        page_to_char(buf_string(buffer), ch);
        free_buf(buffer);
        
        return;
    }
    

    for (j = 1; j <= clan_table[clan].rank_count; j++)
    {
        sprintf(buf, "\n\rDetail for rank {c%d{x - %s%s{x [%s%s-%s{x]:\n\r\n\r",
            j,
            clan_table[clan].who_color,
            capitalize(clan_table[clan].rank_list[j].name),
            clan_table[clan].who_color,
            clan_table[clan].who_name,
            clan_table[clan].rank_list[j].who_name);
        add_buf(buffer, buf);
        
        if (clan_table[clan].rank_list[j].available_slots >= 0)
        {
            sprintf(buf, "{+        Open positions{x: {c%d{x\n\r", clan_table[clan].rank_list[j].available_slots);
            add_buf(buffer, buf);
        }
        else
        {
            sprintf(buf, "{+        Open positions{x: {cUnlimited{x\n\r");
            add_buf(buffer, buf);
        }
        
        sprintf(buf, "{+         Minimum level{x: {c%d{x\n\r", clan_table[clan].rank_list[j].min_level);
        add_buf(buffer, buf);
        
        if (clan_table[clan].rank_list[j].max_promote_rank > 0)
        {
            sprintf(buf, "{+     May promote up to{x: %s%s{x\n\r", 
                clan_table[clan].who_color,
                capitalize(clan_table[clan].rank_list[clan_table[clan].rank_list[j].max_promote_rank].name));
            add_buf(buffer, buf);
        }
        else
        {
            sprintf(buf, "{+     May promote up to{x: {c(None){x\n\r");
            add_buf(buffer, buf);
        }
        
        sprintf(buf, "{+          Use clantalk{x: %7s \t\t", clan_table[clan].rank_list[j].can_use_clantalk ? "{gYes{x" : "{rNo{x" );
        add_buf(buffer, buf);
        sprintf(buf, "{+       Read clan notes{x: %3s\n\r", clan_table[clan].rank_list[j].can_note ? "{gYes{x" : "{rNo{x");
        add_buf(buffer, buf);
        sprintf(buf, "{+    Marry clan members{x: %7s \t\t", clan_table[clan].rank_list[j].can_marry ? "{gYes{x" : "{rNo{x");
        add_buf(buffer, buf);
        sprintf(buf, "{+Join Warfare clan wars{x: %3s\n\r", clan_table[clan].rank_list[j].can_warfare ? "{gYes{x" : "{rNo{x");
        add_buf(buffer, buf);
        sprintf(buf, "{+         Invite others{x: %7s \t\t", clan_table[clan].rank_list[j].can_invite ? "{gYes{x" : "{rNo{x");
        add_buf(buffer, buf);
	sprintf(buf, "{+              Set MOTD{x: %3s\n\r", clan_table[clan].rank_list[j].can_set_motd ? "{gYes{x" : "{rNo{x");
	add_buf(buffer, buf);
        sprintf(buf, "{+         Clanwar pkill{x: %7s \t\t", clan_table[clan].rank_list[j].clanwar_pkill ? "{gYes{x" : "{rNo{x");
        add_buf(buffer, buf);
        sprintf(buf, "{+       Declare Clanwar{x: %3s\n\r", clan_table[clan].rank_list[j].can_declare_war ? "{gYes{x" : "{rNo{x");
        add_buf(buffer, buf);
        sprintf(buf, "{+ Declare Clanwar Truce{x: %7s \t\t", clan_table[clan].rank_list[j].can_declare_truce ? "{gYes{x" : "{rNo{x");
        add_buf(buffer, buf);
        sprintf(buf, "{+Declare Clanwar Treaty{x: %3s\n\r", clan_table[clan].rank_list[j].can_declare_treaty ? "{gYes{x" : "{rNo{x");        
        add_buf(buffer, buf);
        
    }
    
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    
}

/* Invite another player to join your clan. -Rim 1/2000*/
DEF_DO_FUN(do_invite)
{
    CHAR_DATA *victim ;
    char arg1[MAX_STRING_LENGTH] ;
    char arg2[MAX_STRING_LENGTH] ;
    int clannum = 0;

	if (IS_NPC(ch))
	{
		send_to_char("You cant invite anyone anywhere.\n\r", ch);
		return;
	}
    
    argument = one_argument(argument,arg1) ;
    argument = one_argument(argument,arg2) ;

    if (arg1[0] == '\0')
    {
        DESCRIPTOR_DATA *d;
        bool found = FALSE;

        if (!clan_table[ch->clan].active)
        {
            send_to_char("You are not a member of a clan.\n\r", ch);
            return;
        }

        printf_to_char(ch, "{+Online players that are invited to join %s%s{x:\n\r\n\r",
            clan_table[ch->clan].who_color,
            capitalize(clan_table[ch->clan].name));

        for ( d = descriptor_list; d; d = d->next )
        {
            if ( (IS_PLAYING(d->connected))
                && ( victim = d->character ) != NULL
                &&   !IS_NPC(victim)
                &&   victim->in_room != NULL
                &&   !IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
                &&   can_see( ch, victim )
                &&   victim->pcdata->invitation[ch->clan] != NULL)
            {
                found = TRUE;
                sprintf(log_buf, "{y%s{x, invited by %s%s{x.\n\r",
                    victim->name,
                    clan_table[ch->clan].who_color,
                    victim->pcdata->invitation[ch->clan]);
                send_to_char(log_buf, ch);
            }
        }
        if ( !found )
            send_to_char( "None.\n\r", ch );
        return;
    }


    if (get_trust(ch) == MAX_LEVEL && arg2[0] != '\0')
    {
        clannum = clan_lookup(arg2);

        if (clannum <= 0)
        {
            send_to_char("Clan not found.\n\r", ch);
            return;
        }
    }
    else
    {
        clannum = ch->clan;
    }

    if (!clan_table[clannum].active)
    {
        send_to_char("You are not a member of a clan.\n\r", ch);
        return;
    }

    if (get_trust(ch) < MAX_LEVEL && !clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_invite)
    {
        send_to_char("You are not of sufficient rank to invite other players to join your clan.\n\r", ch);
        return;
    }
    
    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("That player is not currently online.\n\r", ch) ;
        return ;
    }
    
    if (IS_NPC(victim))
    {
        send_to_char("No NPC's.\n\r",ch);
        return;
    }

    if (victim->clan == clannum)
    {
        send_to_char("That player is already a member of your clan!\n\r", ch);
        return;
    }

    if (victim->pcdata->invitation[clannum] != NULL)
    {
        sprintf(log_buf, "That player has already been invited by %s.\n\r",
            victim->pcdata->invitation[clannum]);
        send_to_char(log_buf, ch);
        return;
    }
    
    victim->pcdata->invitation[clannum] = str_dup(ch->name);
    
    printf_to_char(victim, "You have been invited by %s to join clan %s!\n\r",
        ch->name, capitalize(clan_table[clannum].name));

    printf_to_char(ch, "You have invited %s to join your clan.\n\r", victim->name);    
    
    return;
}

/* Save all clans. -Rim 1/2000*/
/*
void save_all_clans(void)
{
    int i;

    for (i = 0; i < MAX_CLAN; i++)
        if (clan_table[i].changed == TRUE)
            save_clan_file(i);
}
*/

/* Save a clan file to memory. -Bobble 6/2000 */
MEMFILE* mem_save_clan_file( int clannum )
{
    MEMFILE *mf;
    DBUFFER *buf;
    char path[MAX_INPUT_LENGTH];
    CLAN_DATA *clan;
    CLAN_RANK_DATA *rank;
    int i;

#if defined(SIM_DEBUG)
    log_string("mem_save_clan_file: start");
#endif

    clan = &(clan_table[clannum]);  /* for readability */
    
    sprintf(path, "%s%s", CLAN_DIR, clan_table[clannum].filename);
    
    mf = memfile_new( path, 1024 );
    if (mf == NULL)
    {
      bug("mem_save_clan_file: out of memory", 0);
      return NULL;
    }
    buf = mf->buf;

    bprintf(buf, "#CLAN          %d\n",  clannum);
    bprintf(buf, "Name           %s~\n", clan->name);
    bprintf(buf, "Filename       %s~\n", clan->filename);
    bprintf(buf, "Active         %d\n",  clan->active);
    bprintf(buf, "AllowRecruits  %d\n",  clan->allow_recruits);
    bprintf(buf, "InviteOnly     %d\n",  clan->invitation_only);
    bprintf(buf, "WhoName        %s~\n", clan->who_name);
    bprintf(buf, "WhoColor       %s~\n", clan->who_color);
    bprintf(buf, "Patron         %s~\n", clan->patron);
    bprintf(buf, "Motd           %s~\n", clan->motd);
    bprintf(buf, "Recall         %d\n",  clan->hall);
    bprintf(buf, "Donation       %d\n",  clan->donation);
    bprintf(buf, "CreationDate   %ld\n", clan->creation_date);
    bprintf(buf, "PKills         %ld\n", clan->pkills);
    bprintf(buf, "PDeaths        %ld\n", clan->pdeaths);
    bprintf(buf, "MobKills       %ld\n", clan->mobkills);
    bprintf(buf, "MobDeaths      %ld\n", clan->mobdeaths);
    bprintf(buf, "MinAlign       %d\n",  clan->min_align);
    bprintf(buf, "MaxAlign       %d\n",  clan->max_align);
    bprintf(buf, "End\n");


    for (i = 1; i <= clan->rank_count; i++)
    {
        rank = &(clan->rank_list[i]);

        bprintf(buf, "\n#RANK          %d\n", i);
        bprintf(buf, "Name           %s~\n", rank->name);
        bprintf(buf, "WhoName        %s~\n", rank->who_name);
        bprintf(buf, "MinLevel       %d\n",  rank->min_level);
        bprintf(buf, "MaxPromote     %d\n",  rank->max_promote_rank);
        bprintf(buf, "MaxCount       %d\n",  rank->available_slots);
        bprintf(buf, "ClanwarPK      %d\n",  rank->clanwar_pkill);
        bprintf(buf, "Clantalk       %d\n",  rank->can_use_clantalk);
        bprintf(buf, "Invite         %d\n",  rank->can_invite);
	bprintf(buf, "SetMotd        %d\n",  rank->can_set_motd);
        bprintf(buf, "Marry          %d\n",  rank->can_marry);
        bprintf(buf, "Note           %d\n",  rank->can_note);
        bprintf(buf, "Warfare        %d\n",  rank->can_warfare);
        bprintf(buf, "DeclWar        %d\n",  rank->can_declare_war);
        bprintf(buf, "DeclTruce      %d\n",  rank->can_declare_truce);
        bprintf(buf, "DeclTreaty     %d\n",  rank->can_declare_treaty);
        bprintf(buf, "End\n");
    }

    bprintf(buf, "\n#END\n");

    /* check for overflow */
    if (buf->overflowed)
    {
      bug("mem_save_clan_file: buffer overflow", 0);
      memfile_free(mf);
      return NULL;
    }

    clan_table[clannum].changed = FALSE;

#if defined(SIM_DEBUG)
    log_string("mem_save_clan_file: start");
#endif

    return mf;
}

/* Save a clan file. -Rim 1/2000 */
/*
void save_clan_file(int clannum)
{
    FILE *fp;
    char path[MAX_INPUT_LENGTH];
    CLAN_DATA *clan;
    CLAN_RANK_DATA *rank;
    int i;

    clan = &(clan_table[clannum]);  // for readability
    
    sprintf(path, "%s%s", CLAN_DIR, clan_table[clannum].filename);
    
    fclose(fpReserve);
    if ( (fp = fopen( path, "w" ) ) == NULL )
    {
        bugf("Save_clan_file: Cannot open file %s.\n\r", path);
        exit (1);
    }

    fprintf(fp, "#CLAN          %d\n",  clannum);
    rfprintf(fp, "Name           %s~\n", clan->name);
    rfprintf(fp, "Filename       %s~\n", clan->filename);
    fprintf(fp, "Active         %d\n",  clan->active);
    fprintf(fp, "AllowRecruits  %d\n",  clan->allow_recruits);
    fprintf(fp, "InviteOnly     %d\n",  clan->invitation_only);
    rfprintf(fp, "WhoName        %s~\n", clan->who_name);
    rfprintf(fp, "WhoColor       %s~\n", clan->who_color);
    rfprintf(fp, "Patron         %s~\n", clan->patron);
    fprintf(fp, "Recall         %d\n",  clan->hall);
    fprintf(fp, "Donation       %d\n",  clan->donation);
    fprintf(fp, "CreationDate   %ld\n", clan->creation_date);
    fprintf(fp, "PKills         %ld\n", clan->pkills);
    fprintf(fp, "PDeaths        %ld\n", clan->pdeaths);
    fprintf(fp, "MobKills       %ld\n", clan->mobkills);
    fprintf(fp, "MobDeaths      %ld\n", clan->mobdeaths);
    fprintf(fp, "MinAlign       %d\n",  clan->min_align);
    fprintf(fp, "MaxAlign       %d\n",  clan->max_align);
    fprintf(fp, "End\n");


    for (i = 1; i <= clan->rank_count; i++)
    {
        rank = &(clan->rank_list[i]);

        fprintf(fp, "\n#RANK          %d\n", i);
        rfprintf(fp, "Name           %s~\n", rank->name);
        rfprintf(fp, "WhoName        %s~\n", rank->who_name);
        fprintf(fp, "MinLevel       %d\n",  rank->min_level);
        fprintf(fp, "MaxPromote     %d\n",  rank->max_promote_rank);
        fprintf(fp, "MaxCount       %d\n",  rank->available_slots);
        fprintf(fp, "ClanwarPK      %d\n",  rank->clanwar_pkill);
        fprintf(fp, "Clantalk       %d\n",  rank->can_use_clantalk);
        fprintf(fp, "Invite         %d\n",  rank->can_invite);
        fprintf(fp, "Marry          %d\n",  rank->can_marry);
        fprintf(fp, "Note           %d\n",  rank->can_note);
        fprintf(fp, "Warfare        %d\n",  rank->can_warfare);
        fprintf(fp, "DeclWar        %d\n",  rank->can_declare_war);
        fprintf(fp, "DeclTruce      %d\n",  rank->can_declare_truce);
        fprintf(fp, "DeclTreaty     %d\n",  rank->can_declare_treaty);
        fprintf(fp, "End\n");
    }

    fprintf(fp, "\n#END\n");


    fclose (fp);
    fpReserve = fopen( NULL_FILE, "r" );

    clan_table[clannum].changed = FALSE;
}
*/

/* Periodically check the changed flags on each clan, and if one is changed then
   save it to disk. -Rim 1/2000 */
/*
void clan_update(void)
{
    static int clan_timer = 0;
    int i;

    clan_timer++;

    if (clan_timer % 3)
        return;  // Execute every third tick for performance reasons.

    clan_timer = 0;

    for (i = 1; i < MAX_CLAN; i++)
    {
        if (clan_table[i].changed)
        {
            save_clan_file(i);
        }
    }
}
*/

/* Set function to change clan settings. -Rim 1/2000 */
DEF_DO_FUN(do_cset)
{
    char arg_buf[MAX_INPUT_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CLAN_DATA *clan;
    int clannum, value;
    bool found = FALSE;
    
    argument = smash_tilde_cpy( arg_buf, argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  set clan <clan_name> <field> <value>\n\r",ch); 
        send_to_char("  Field being one of:\n\r",          ch );
        send_to_char("    active recruits invitation ('yes' or 'no')\n\r",    ch );
        send_to_char("    minalign maxalign (numeric)\n\r",ch);
        send_to_char("    recall donation (vnums)\n\r",  ch );
        send_to_char("    patron color (text)\n\r",  ch );
        send_to_char("  set clan <clan_name> rank <rank> <field> <value>\n\r",ch); 
        send_to_char("  Field being one of:\n\r",          ch );
        send_to_char("    minlevel available (numeric)\n\r",ch);
        send_to_char("    maxpromote (rank)\n\r",ch);
        send_to_char("    clantalk notes marry warfare invite ('yes' or 'no')\n\r",ch);
        send_to_char("    clanwar declare truce treaty motd ('yes' or 'no')\n\r",ch);
        return;
    }

    for (clannum = 1; clannum < MAX_CLAN; clannum++)
    {
        if (   LOWER(arg1[0]) == LOWER(clan_table[clannum].name[0])
            && !str_cmp(arg1, clan_table[clannum].name))
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        send_to_char("Clan not found.\n\r", ch);
        return;
    }

    clan = &(clan_table[clannum]);
    
    /* check if setting for specific rank */
    if ( !strcmp(arg2, "rank") )
    {
	int rank;

	/* check rank */
	argument = one_argument( argument, arg3 );
	rank = clan_rank_lookup(clannum, arg3);
	if ( !IS_BETWEEN(1, rank, clan->rank_count) )
	{
	    send_to_char( "Rank not found.\n\r", ch );
	    return;
	}

	/* now parse field and value - reuse arg1 and arg2 */
	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	
	if ( !strcmp(arg1, "minlevel") )
	{
	    int value = atoi(arg2);
	    if ( !IS_BETWEEN(1, value, MAX_LEVEL) )
	    {
		send_to_char( "Invalid level.\n\r", ch );
		return;
	    }
	    clan->rank_list[rank].min_level = value;
	}
	else if ( !strcmp(arg1, "available") )
	{
	    int value = atoi(arg2);
	    if ( !is_number(arg2) || value < -1 )
	    {
		send_to_char( "Value must be non-negative or -1 (unlimited).\n\r", ch );
		return;
	    }
	    clan->rank_list[rank].available_slots = value;
	}
	else if ( !strcmp(arg1, "maxpromote") )
	{
	    int value = clan_rank_lookup(clannum, arg2);
	    if ( !IS_BETWEEN(1, value, clan->rank_count) )
	    {
		send_to_char( "Max Promote Rank not found.\n\r", ch );
		return;
	    }
	    clan->rank_list[rank].max_promote_rank = value;
	}
	else
	{
	    /* parse 'yes' or 'no' parameter first */
	    bool value;
	    if ( !strcmp(arg2, "yes") )
		value = TRUE;
	    else if ( !strcmp(arg2, "no") )
		value = FALSE;
	    else
	    {
		send_to_char( "Value must be 'yes' or 'no'.\n\r", ch );
		return;
	    }

	    /* find proper field */
	    if ( !strcmp(arg1, "clantalk") )
		clan->rank_list[rank].can_use_clantalk = value;
	    else if ( !strcmp(arg1, "notes") )
		clan->rank_list[rank].can_note = value;
	    else if ( !strcmp(arg1, "marry") )
		clan->rank_list[rank].can_marry = value;
	    else if ( !strcmp(arg1, "warfare") )
		clan->rank_list[rank].can_warfare = value;
	    else if ( !strcmp(arg1, "invite") )
		clan->rank_list[rank].can_invite = value;
	    else if ( !strcmp(arg1, "motd") )
		clan->rank_list[rank].can_set_motd = value;
	    else if ( !strcmp(arg1, "clanwar") )
		clan->rank_list[rank].clanwar_pkill = value;
	    else if ( !strcmp(arg1, "declare") )
		clan->rank_list[rank].can_declare_war = value;
	    else if ( !strcmp(arg1, "truce") )
		clan->rank_list[rank].can_declare_truce = value;
	    else if ( !strcmp(arg1, "treaty") )
		clan->rank_list[rank].can_declare_treaty = value;
	    else
	    {
		send_to_char( "Field not found.\n\r", ch );
		return;
	    }
	}
	/* ok, that worked */
	send_to_char( "Ok.\n\r", ch );
	clan->changed = TRUE;
	return;
    }

    /* Snarf the value (which need not be numeric). */
    value = is_number( arg3 ) ? atoi( arg3 ) : -1001;
    
    if ( !str_prefix( arg2, "active" ) )
    {
        if ( !str_cmp( arg3, "yes") )
        {
            clan->active = TRUE;
            clan->changed = TRUE;

            send_to_char("Clan is now active.\n\r", ch);
            return;
        }
        else if ( !str_cmp( arg3, "no" ) )
        {
            clan->active = FALSE;
            clan->changed = TRUE;

            send_to_char("Clan is now inactive.\n\r", ch);
            return;
        }
        else
        {
            send_to_char( "Values are 'yes' or 'no'.\n\r", ch );
            return;
        }
    }

    if (clan_table[clannum].active == FALSE)
    {
        send_to_char("That clan is not active.\n\r", ch);
        return;
    }

    if ( !str_prefix( arg2, "recruits" ) )
    {
        if ( !str_cmp( arg3, "yes") )
        {
            clan->allow_recruits = TRUE;
            clan->changed = TRUE;

            send_to_char("Clan is now accepting new recruits.\n\r", ch);
            return;
        }
        else if ( !str_cmp( arg3, "no" ) )
        {
            clan->allow_recruits = FALSE;
            clan->changed = TRUE;

            send_to_char("Clan is no longer accepting recruits.\n\r", ch);
            return;
        }
        else
        {
            send_to_char( "Values are 'yes' or 'no'.\n\r", ch );
            return;
        }
    }
    

    if ( !str_prefix( arg2, "invitation" ) )
    {
        if ( !str_cmp( arg3, "yes") )
        {
            clan->invitation_only = TRUE;
            clan->changed = TRUE;
            send_to_char("Clan now requires an invitation for new recruits.\n\r",ch);
            return;
        }
        else if ( !str_cmp( arg3, "no" ) )
        {
            clan->invitation_only = FALSE;
            clan->changed = TRUE;
            send_to_char("Players may now recruit to that clan without an invitation.\n\r", ch);
            return;
        }
        else
        {
            send_to_char( "Values are 'yes' or 'no'.\n\r", ch );
            return;
        }
    }

    if ( !str_prefix( arg2, "minalign" ) )
    {
        if (value < -1000 || value > 1000)
        {
            send_to_char("Alignment must be between -1000 and 1000.\n\r",ch);
            return;
        }

        if (value > (clan->max_align - 500))
        {
            send_to_char("Clans must have an alignment range of at least 500 points.\n\r",ch);
            return;
        }

        clan->min_align = value;
        clan->changed = TRUE;

        printf_to_char(ch, "Minimum alignment value set to: %d.\n\r", clan->min_align);
        return;
    }

    if ( !str_prefix( arg2, "maxalign" ) )
    {
        if (value < -1000 || value > 1000)
        {
            send_to_char("Alignment must be between -1000 and 1000.\n\r",ch);
            return;
        }

        if (value < (clan->min_align + 500))
        {
            send_to_char("Clans must have an alignment range of at least 500 points.\n\r",ch);
            return;
        }

        clan->max_align = value;
        clan->changed = TRUE;

        printf_to_char(ch, "Maximum alignment value set to: %d.\n\r", clan->max_align);
        return;
    }


    if ( !str_prefix( arg2, "recall" ) )
    {
        ROOM_INDEX_DATA * pRoom;

        if ( (pRoom = get_room_index( value )) == NULL )
        {
            send_to_char("Room not found.\n\r", ch);
            return;
        }

        if (   IS_SET(pRoom->room_flags, ROOM_NOWHERE)
            || IS_SET(pRoom->room_flags, ROOM_PRIVATE)
            || IS_SET(pRoom->room_flags, ROOM_SOLITARY)
            || IS_SET(pRoom->room_flags, ROOM_IMP_ONLY)
            || IS_SET(pRoom->room_flags, ROOM_PET_SHOP)
            || IS_SET(pRoom->room_flags, ROOM_GODS_ONLY)
            || IS_SET(pRoom->room_flags, ROOM_HEROES_ONLY)
            || IS_SET(pRoom->room_flags, ROOM_JAIL)
            || IS_SET(pRoom->room_flags, ROOM_NEWBIES_ONLY))
        {
            send_to_char("That is not an acceptable room.\n\r", ch);
            return;
        }

        clan->hall = pRoom->vnum;
        clan->changed = TRUE;

        printf_to_char(ch, "Recall room set to: %d.\n\r", clan->hall);
        return;
    }

    if ( !str_prefix( arg2, "donation" ) )
    {
        ROOM_INDEX_DATA * pRoom;

        if ( (pRoom = get_room_index( value )) == NULL )
        {
            send_to_char("Room not found.\n\r", ch);
            return;
        }

        if (   IS_SET(pRoom->room_flags, ROOM_NOWHERE)
            || IS_SET(pRoom->room_flags, ROOM_PRIVATE)
            || IS_SET(pRoom->room_flags, ROOM_SOLITARY)
            || IS_SET(pRoom->room_flags, ROOM_IMP_ONLY)
            || IS_SET(pRoom->room_flags, ROOM_PET_SHOP)
            || IS_SET(pRoom->room_flags, ROOM_GODS_ONLY)
            || IS_SET(pRoom->room_flags, ROOM_HEROES_ONLY)
            || IS_SET(pRoom->room_flags, ROOM_JAIL)
            || IS_SET(pRoom->room_flags, ROOM_NEWBIES_ONLY))
        {
            send_to_char("That is not an acceptable room.\n\r", ch);
            return;
        }

        if ( !IS_SET(pRoom->room_flags, ROOM_DONATION) )
        {
            send_to_char("That room is not flagged as a donation room.\n\r", ch);
            return;
        }

        clan->donation = pRoom->vnum;
        clan->changed = TRUE;

        printf_to_char(ch, "Donation room set to: %d.\n\r", clan->donation);
        return;
    }

    if ( !str_prefix( arg2, "patron" ) )
    {
        clan->patron = str_dup(arg3);
        clan->changed = TRUE;

        printf_to_char(ch, "Patron string set to: '%s'.\n\r", clan->patron);
        return;
    }

    if ( !str_prefix( arg2, "color" ) )
    {
        clan->who_color = str_dup(arg3);
        clan->changed = TRUE;

        sprintf(log_buf, "Color string set to: '%s'.\n\r", clan->who_color);
        send_to_char_bw(log_buf, ch);
        return;
    }

    /* Generate usage message. */
    do_cset( ch, "" );
    return;
}

/*
DEF_DO_FUN(do_clan_dump)
{
    char arg[MIL];
    long vnum;
    OBJ_INDEX_DATA *pObjIndex;
    int clan;
    int nMatch = 0;
    
    argument = one_argument( argument, arg );
    
    if (arg[0] == '\0')
    {
        send_to_char("List what clan's items?\n\r", ch);
        return;
    }
    
    if ((clan = clan_lookup(arg)) <= 0)
    {
        send_to_char("Enter a valid clan.\n\r", ch);
        return;
    }
    
    printf_to_char(ch, "Objects for clan %s:\n\r\n\r", clan_table[clan].name);
    
    for (vnum = 0; nMatch < top_obj_index; vnum++)
    {
        nMatch++;
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
            if (pObjIndex->clan == clan)
            {
                printf_to_char(ch, "%d\t%s", pObjIndex->vnum, pObjIndex->name);
                
                if (pObjIndex->rank > 0)
                    printf_to_char(ch, "\tRank: %s\n\r", clan_table[clan].rank_list[pObjIndex->rank].name);
                else
                    send_to_char("\n\r", ch);
            }
        }
    }
}
*/

void clan_dump_obj(CHAR_DATA *ch, int clan);
void clan_dump_room(CHAR_DATA *ch, int clan);

DEF_DO_FUN(do_clan_dump)
{
    char arg1[MIL];
    char arg2[MIL];
    int clan;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if (arg1[0] == '\0')
    {
        send_to_char("List what clan's items?\n\r", ch);
        return;
    }
    
    if ((clan = clan_lookup(arg1)) <= 0)
    {
        send_to_char("Enter a valid clan.\n\r", ch);
        return;
    }

    if (!str_cmp(arg2, "obj"))
        clan_dump_obj(ch, clan);
    else if (!str_cmp(arg2, "room"))
        clan_dump_room(ch, clan);
    else
    {
        clan_dump_obj(ch, clan);
        clan_dump_room(ch, clan);
    }
}


void clan_dump_obj(CHAR_DATA *ch, int clan)
{
    OBJ_INDEX_DATA  *pObjIndex;
	int vnum;
	int nMatch;
    
	nMatch  = 0;

    printf_to_char(ch, "Objects for clan %s:\n\r\n\r", capitalize(clan_table[clan].name));

    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
    {
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
            nMatch++;
            if (pObjIndex->clan == clan)
            {
                printf_to_char(ch, "%d\t\t%s", pObjIndex->vnum, pObjIndex->short_descr);
                
                if (pObjIndex->rank > 0)
                    printf_to_char(ch, "\t\tRank: %s\n\r", clan_table[clan].rank_list[pObjIndex->rank].name);
                else
                    send_to_char("\n\r", ch);
            }
        }
    }
}


void clan_dump_room(CHAR_DATA *ch, int clan)
{
    ROOM_INDEX_DATA *pRoomIndex;
	int vnum;
    
    printf_to_char(ch, "Rooms for clan %s:\n\r\n\r", capitalize(clan_table[clan].name));    

    for ( vnum = 0; vnum < top_vnum_room; vnum++ )
    {
        if ( ( pRoomIndex = get_room_index( vnum ) ) != NULL )
        {
            if ( pRoomIndex->clan == clan)
            {
                printf_to_char(ch, "%d\t\t%s", pRoomIndex->vnum, pRoomIndex->name);
                
                if (pRoomIndex->clan_rank > 0)
                    printf_to_char(ch, "\t\tRank: %s\n\r", clan_table[clan].rank_list[pRoomIndex->clan_rank].name);
                else
                    send_to_char("\n\r", ch);
            }
        }
    }
}

DEF_DO_FUN(do_cmotd)
{
    if ( IS_NPC(ch) )
	return;

    if (!is_clan(ch) || !clan_table[ch->clan].active)
    {
        send_to_char("You aren't in a clan.\n\r",ch);
        return;
    }

    if ( argument[0] == '\0' )
    {
	if ( !strcmp( clan_table[ch->clan].motd , "" ) )
	    return;

	send_to_char("\n",ch);
    	send_to_char("{D***********************************{yClan MOTD{D************************************{x\n\r",ch);
    	send_to_char(clan_table[ch->clan].motd,ch);
    	send_to_char("{x\n\r",ch);
    	send_to_char("{D********************************************************************************{x\n\r",ch);
	return;
    }
    else
    {
	/* Set the cmotd if they can */
	if (!clan_table[ch->clan].rank_list[ch->pcdata->clan_rank].can_set_motd)
	{
	    send_to_char("You don't have the authority to set the clan MOTD.\n\r",ch);
	    return;
	}
	else if ( !str_prefix( argument, "clear" ) )
	{
	    free_string( clan_table[ch->clan].motd) ;
	    clan_table[ch->clan].motd = str_dup("");
	    send_to_char("Clan MOTD is cleared.\n\r", ch);
	    clan_table[ch->clan].changed = TRUE;
	}
	else
	{
	    free_string( clan_table[ch->clan].motd );
	    clan_table[ch->clan].motd = str_dup(argument);
	    clan_table[ch->clan].changed = TRUE;
	    printf_to_char(ch, "Clan MOTD set to: %s\n\r", argument);
	    return;
	}
    }
}

