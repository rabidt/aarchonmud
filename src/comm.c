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
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1996 Russ Taylor             *
 *   ROM has been brought to you by the ROM consortium          *
 *       Russ Taylor (rtaylor@efn.org)                  *
 *       Gabrielle Taylor                           *
 *       Brian Moore (zump@rom.org)                     *
 *   By using this code, you have agreed to follow the terms of the     *
 *   ROM license, in the file Rom24/doc/rom.license             *
 ***************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <arpa/telnet.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "timer.h"
#include "dxport.h"
#include "perfmon.h"
#include "lua_main.h"

/* command procedures needed */
DECLARE_DO_FUN(do_help      );
DECLARE_DO_FUN(do_look      );
DECLARE_DO_FUN(do_skills    );
DECLARE_DO_FUN(do_outfit    );
DECLARE_DO_FUN(do_unread    );
DECLARE_DO_FUN(do_asave     );

/* external functions */
void war_remove( CHAR_DATA *ch, bool killed );
bool flush_descriptor( DESCRIPTOR_DATA *d );
bool desc_cmp( DESCRIPTOR_DATA *d1, DESCRIPTOR_DATA *d2 );
void add_descriptor( DESCRIPTOR_DATA *d );
void install_other_handlers ( void );

/*
 * Malloc debugging stuff.
 */

#if defined(MALLOC_DEBUG)
extern  int malloc_debug    args( ( int  ) );
extern  int malloc_verify   args( ( void ) );
#endif




/*
 * Socket and TCP/IP stuff.
 */
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
/* 
   Linux shouldn't need these. If you have a problem compiling, try
   uncommenting accept and bind.
   int accept      args( ( int s, struct sockaddr *addr, int *addrlen ) );
   int bind        args( ( int s, struct sockaddr *name, int namelen ) );
 */
pid_t   waitpid         args( ( pid_t pid, int *status, int options ) );
pid_t   fork            args( ( void ) );
int     kill            args( ( pid_t pid, int sig ) );
int     pipe            args( ( int filedes[2] ) );
int     dup2            args( ( int oldfd, int newfd ) );
int     execl           args( ( const char *path, const char *arg, ... ) );

int close       args( ( int fd ) );

/*
 * Global variables.
 */
DESCRIPTOR_DATA *   descriptor_list;    /* All open descriptors     */
DESCRIPTOR_DATA *   g_d_next;     /* Next descriptor in loop  */
bool            god;        /* All new chars are gods!  */
bool            merc_down;      /* Shutdown         */
bool            wizlock;        /* Game is wizlocked        */
bool            newlock;        /* Game is newlocked        */
char            str_boot_time[MAX_INPUT_LENGTH];
time_t          current_time;   /* time of this pulse */    
bool            MOBtrigger = TRUE;  /* act() switch                 */


/*
 * OS-dependent local functions.
 */
void    game_loop_unix      args( ( int control ) );
int   init_socket       args( ( u_short port ) );
void    init_descriptor     args( ( int control ) );
bool    read_from_descriptor    args( ( DESCRIPTOR_DATA *d ) );

/*
 * Other local functions (OS-independent).
 */
int     main            args( ( int argc, char **argv ) );
bool    process_output      args( ( DESCRIPTOR_DATA *d, bool fPrompt ) );
void    read_from_buffer    args( ( DESCRIPTOR_DATA *d ) );
void    stop_idling     args( ( CHAR_DATA *ch ) );
void    bust_a_prompt           args( ( CHAR_DATA *ch ) );

/* Needs to be global because of do_copyover */
static u_short s_port;
static int s_control;

#ifdef UNITTEST
int main( int argc, char **argv )
{
    RunAllTests();
    return 0;
}

int aarchon_main( int argc, char **argv )
#else
int main( int argc, char **argv )
#endif
{
    struct timeval now_time;
    bool fCopyOver = FALSE;

    /*
     * Enable core dumps for this process
     */
    const struct rlimit core_limit = { RLIM_INFINITY, RLIM_INFINITY };
    setrlimit(RLIMIT_CORE, &core_limit);
    
    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug( 2 );
#endif

    /*
     * Init time.
     */
    gettimeofday( &now_time, NULL );
    current_time    = (time_t) now_time.tv_sec;
    strcpy( str_boot_time, ctime( &current_time ) );

    /* Log some info about the binary if present */
    log_string(bin_info_string);

    /*
     * Get the port number.
     */
    s_port = 7000;

    if ( argc > 1 )
    {
        if ( !is_number( argv[1] ) )
        {
            fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
            exit( 1 );
        }
        else if ( ( s_port = atoi( argv[1] ) ) <= 1024 )
        {
            fprintf( stderr, "Port number must be above 1024.\n" );
            exit( 1 );
        }


        /* Are we recovering from a copyover? */
        if ( argc > 2 )
        {
            if (!str_cmp(argv[2], "copyover")
                    && is_number(argv[3]))
            {
                fCopyOver = TRUE; 
                s_control = atoi(argv[3]);
            }
            else
                fCopyOver = FALSE;
        }
    }

    /*
     * Run the game.
     */
    if (!fCopyOver)
    {
        s_control = init_socket( s_port );
    }

    boot_db();
    DXPORT_init();

    snprintf( log_buf, sizeof(log_buf), "ROM is ready to rock on port %d.", s_port );
    log_string( log_buf );

    if (fCopyOver)
        copyover_recover();

    /* check aprog boot triggers */
    ap_boot_trigger();
#ifdef LIVETEST 
    RunAllTests();
#else
    game_loop_unix( s_control );
#endif
    close (s_control);

    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    exit( 0 );
    return 0;
}

int init_socket( u_short port )
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int fd;

    if ( system("touch SHUTDOWN.TXT") == -1 )
        log_error("init_socket: touch SHUTDOWN.TXT");
    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        log_error( "Init_socket: socket" );
        exit( 1 );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                (char *) &x, sizeof(x) ) < 0 )
    {
        log_error( "Init_socket: SO_REUSEADDR" );
        close(fd);
        exit( 1 );
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
        struct  linger  ld;

        ld.l_onoff  = 1;
        ld.l_linger = 1000;

        if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
                    (char *) &ld, sizeof(ld) ) < 0 )
        {
            log_error( "Init_socket: SO_DONTLINGER" );
            close(fd);
            exit( 1 );
        }
    }
#endif

    sa          = sa_zero;
    sa.sin_family   = AF_INET;
    sa.sin_port     = htons( port );

    if ( bind( fd, (struct sockaddr *) &sa, sizeof(sa) ) < 0 )
    {
        log_error("Init socket: bind" );
        close(fd);
        exit(1);
    }

    if ( listen( fd, 3 ) < 0 )
    {
        log_error("Init socket: listen");
        close(fd);
        exit(1);
    }

    if ( system("rm SHUTDOWN.TXT") == -1 )
        log_error("init_socket: rm SHUTDOWN.TXT");

    return fd;
}


void game_loop_unix( int control )
{
    static struct timeval null_time;
    struct timeval last_time;

    signal( SIGPIPE, SIG_IGN );
    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

    install_other_handlers ();

    /* Main loop */
    while ( !merc_down )
    {
        PERF_prof_reset();
        reset_mprog_history();
        
        PERF_PROF_ENTER( pr_, "Main loop" );

        check_lua_stack();

        fd_set in_set;
        fd_set out_set;
        fd_set exc_set;
        DESCRIPTOR_DATA *d;
        int maxdesc;

#if defined(MALLOC_DEBUG)
        if ( malloc_verify( ) != 1 )
            abort( );
#endif

        /*
         * Poll all active descriptors.
         */
        PERF_PROF_ENTER( pr_poll_desc_, "Poll descriptors" );

        FD_ZERO( &in_set  );
        FD_ZERO( &out_set );
        FD_ZERO( &exc_set );
        FD_SET( control, &in_set );

        maxdesc = control;

        for ( d = descriptor_list; d; d = d->next )
        {
            maxdesc = UMAX( maxdesc, d->descriptor );
            FD_SET( d->descriptor, &in_set  );
            FD_SET( d->descriptor, &out_set );
            FD_SET( d->descriptor, &exc_set );
        }

        if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
        {
            log_error( "Game_loop: select: poll" );
            exit( 1 );
        }
        PERF_PROF_EXIT( pr_poll_desc_ );

        /*
         * New connection?
         */
        if ( FD_ISSET( control, &in_set ) )
            init_descriptor( control );



        /*
         * Kick out the freaky folks.
         */
        for ( d = descriptor_list; d != NULL; d = g_d_next )
        {
            g_d_next = d->next;   
            if ( FD_ISSET( d->descriptor, &exc_set ) )
            {
                FD_CLR( (unsigned short)d->descriptor, &in_set  );
                FD_CLR( (unsigned short)d->descriptor, &out_set );
                d->outtop   = 0;
                close_socket( d );
            }
        }

        /*
         * Process input.
         */
        PERF_PROF_ENTER( pr_process_input_, "Process input" );

        #define MAX_LINE_COUNT 100 
        int linecnt=0; /* track number of lines processed
                        when processing lag free 
                      (cases where we pull a d_next=d trick*/
                        
        for ( d = descriptor_list; d != NULL; d = g_d_next )
        {
            g_d_next  = d->next;
            d->fcommand = FALSE;

            if ( FD_ISSET( d->descriptor, &in_set ) )
            {
                if ( d->character != NULL )
                    d->character->timer = 0;
                if ( !read_from_descriptor( d ) )
                {
                    FD_CLR( (unsigned short)d->descriptor, &out_set );
                    d->outtop   = 0;
                    close_socket( d );
                    continue;
                }
            }

            if (!IS_PLAYING(d->connected) && (d->inactive>4800))
            {
                write_to_buffer( d, "Logon timed out.\n\r", 0 );
                close_socket( d );
                continue;
            }

            if (d->character != NULL && d->character->daze > 0)
                --d->character->daze;

            if ( d->character != NULL && d->character->pcdata != NULL )
            {
                /* When pkill_timer > 0, a battle recently occurred and commands are restricted.
                 * When a player is at zero or less hp, pkill_timer does not get reduced:
                 we can't allow people to use the 'die' command to get out of being pkilled.
                 * When pkill_timer < 0, the player recently logged in and cannot begin battle. */
                if( d->character->pcdata->pkill_timer > 0 && d->character->hit > 0 )
                    --d->character->pcdata->pkill_timer;
                else if( d->character->pcdata->pkill_timer < 0 )
                    ++d->character->pcdata->pkill_timer;
            }
#ifdef LAG_FREE
            if (d->lag_free)
                d->character->wait=0;
#endif

            if (d->character != NULL && d->character->slow_move > 0)
                --d->character->slow_move;

            if ( d->character != NULL && d->character->wait > 0 )
            {
                --d->character->wait;
                // always accept input from switched imms
                // this prevents input-lock by mprogs that induce wait
                if ( !IS_NPC(d->character) )
                    continue;
            }

            if ( d->character != NULL && check_fear(d->character) )
                continue;

            if (d->connected != CON_NOTE_TEXT)
                read_from_buffer( d );
            if ( d->incomm[0] != '\0' )
            {
#ifdef LAG_FREE
                if (d->lag_free)
                    d_next=d;
#endif

                d->fcommand = TRUE;

                if ( d->pProtocol != NULL )
                    d->pProtocol->WriteOOB = 0;

                stop_idling( d->character );
                d->inactive=0;

                /* OLC */
                if ( d->showstr_point )
                    show_string( d, d->incomm );
                else
                {
                    if ( d->pString )
                    {
                        string_add( d->character, d->incomm );
                        /* little hack to have lag free pasting
                           in string editor */
                        linecnt++;
                        if (linecnt<MAX_LINE_COUNT)
                            g_d_next=d;
                        else
                            linecnt=0;
                        /* and wait for next pulse to grab
                           anything else*/
                    }
                    else
                    {
                        switch ( d->connected )
                        {
                            case CON_PLAYING:
                                if (run_lua_interpret(d))
                                {
                                    /* little hack to have lag free pasting
                                       into interpreter */
                                    linecnt++;
                                    if (linecnt<MAX_LINE_COUNT)
                                        g_d_next=d;
                                    else
                                        linecnt=0;
                                    /* and wait for next pulse to grab
                                       anything else*/

                                    break;
                                }
                                else if (run_olc_editor(d))
                                {
                                    break;
                                }
                                else
                                {
                                    substitute_alias( d, d->incomm );
                                    break;
                                }
                            default:
                                nanny (d, d->incomm);
                                break;
                        }                     /* end of switch */
                    }                        /* else, from if ( d->pString ) */
                }                           /* else, from if ( d->showstr_point ) */
                d->incomm[0]    = '\0';
                // ensure we have feedback that an input was processed (some commands only provide feedback via prompt)
                if ( d->outtop == 0 )
                    d->last_msg_was_prompt = FALSE;
            }                              /* if ( d->incomm[0] != '\0' ) */
            else if (d->connected == CON_LUA_PULSE_HANDLER)
            {
                /* some lua con handlers need to process with no input */
                lua_con_handler(d, NULL);
            }
            /* special handling for note editor */
            else if (d->connected == CON_NOTE_TEXT && d->inbuf[0] != '\0' )
            {
                handle_con_note_text( d, "" );
            }
            else
            {
                linecnt=0;
                d->inactive++;
                /* auto-action in combat */
                if ( d->connected == CON_PLAYING
                        && d->character != NULL
                        && d->character->fighting != NULL
                        && !IS_AFFECTED(d->character, AFF_PETRIFIED) )
                    run_combat_action( d );
            }

        }
        PERF_PROF_EXIT( pr_process_input_ );

        /*
         * Autonomous game motion.
         */
        update_handler( );

        /*
         * Output.
         */
        /* snooped chars first */
        PERF_PROF_ENTER( pr_output_, "Output" );

        for ( d = descriptor_list; d != NULL; d = g_d_next )
        {
            g_d_next = d->next;

            if ( d->snoop_by == NULL )
                continue;

            if ( ( d->fcommand || d->outtop > 0 || !d->last_msg_was_prompt )
                    &&   FD_ISSET(d->descriptor, &out_set) )
            {
                if ( !process_output( d, TRUE ) )
                {
                    d->outtop   = 0;
                    close_socket( d );
                }
            }
        }
        for ( d = descriptor_list; d != NULL; d = g_d_next )
        {
            g_d_next = d->next;

            if ( d->snoop_by != NULL )
                continue;

            if ( ( d->fcommand || d->outtop > 0 || !d->last_msg_was_prompt )
                    &&   FD_ISSET(d->descriptor, &out_set) )
            {
                if ( !process_output( d, TRUE ) )
                {
                    d->outtop   = 0;
                    close_socket( d );
                }
            }
        }
        PERF_PROF_EXIT( pr_output_ );
        PERF_PROF_EXIT( pr_ );

        /*
         * Synchronize to a clock.
         * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
         * Careful here of signed versus unsigned arithmetic.
         */
        {
            struct timeval now_time;
            long secDelta;
            long usecDelta;

            gettimeofday( &now_time, NULL );
            usecDelta   = ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
                + 1000000 / PULSE_PER_SECOND;
            secDelta    = ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
            while ( usecDelta < 0 )
            {
                usecDelta += 1000000;
                secDelta  -= 1;
            }

            while ( usecDelta >= 1000000 )
            {
                usecDelta -= 1000000;
                secDelta  += 1;
            }

            
            /* calculate the percentage of the pulse used */
            double usage=100-
                (double)( (secDelta*1000000)+(usecDelta) )/
                (double)(10000/PULSE_PER_SECOND);

            PERF_log_pulse( usage );
            if (usage >= 100)
            {
                log_string("Pulse usage > 100%. Trace info: ");

                if ( last_command[0] != '\0' )
                    logpf( "Last command: %s", last_command );

                /* catch other debug info too */
                if ( last_mprog[0] != '\0' )
                    logpf( "Last mprog: %s", last_mprog );

                if ( last_debug[0] != '\0' )
                    logpf( "Last debug: %s", last_debug );

                CHAR_DATA *ch;
                for ( ch = char_list ; ch ; ch = ch->next )
                {
                    if (IS_NPC(ch))
                        continue;

                    logpf("%-20s in %d", ch->name, ch->in_room ? ch->in_room->vnum : 0);
                }

                log_mprog_history();
                
                char buf[MSL * 12];
                PERF_prof_repr_pulse(buf, sizeof(buf));
                log_string(buf);
            }

            if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
            {
                struct timeval stall_time;

                stall_time.tv_usec = usecDelta;
                stall_time.tv_sec  = secDelta;
                if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 )
                {
                    log_error( "Game_loop: select: stall" );
                    exit( 1 );
                }
            }
        }

        gettimeofday( &last_time, NULL );
        current_time = (time_t) last_time.tv_sec;
    }

    return;
}

void init_descriptor( int control )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
    /*
       struct hostent *from;
     */
    int desc;
    unsigned int size;

    size = sizeof(sock);
    getsockname( control, (struct sockaddr *) &sock, &size );
    if ( ( desc = accept( control, (struct sockaddr *) &sock, &size) ) < 0 )
    {
        log_error( "New_descriptor: accept" );
        return;
    }

    /*
#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

#if !defined( AmigaTCP ) && !defined( WIN32 )
if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
{
log_error( "New_descriptor: fcntl: FNDELAY" );
return;
}
#endif
     */

#if defined(FNDELAY)
#undef FNDELAY
#define FNDELAY O_NONBLOCK
#endif
#if !defined(FNDELAY)
#define FNDELAY O_NONBLOCK
#endif

if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
{
    log_error( "New_descriptor: fcntl: FNDELAY" );
    return;
}   



/*
 * Cons a new descriptor.
 */
dnew = new_descriptor();

dnew->descriptor    = desc;
dnew->inactive=0;

size = sizeof(sock);
if ( getpeername( desc, (struct sockaddr *) &sock, &size ) < 0 )
{
    log_error( "New_descriptor: getpeername" );
    dnew->host = str_dup( "(unknown)" );
}
else
{
    /*
     * Would be nice to use inet_ntoa here but it takes a struct arg,
     * which ain't very compatible between gcc and system libraries.
     */
    int addr;

    addr = ntohl( sock.sin_addr.s_addr );
    snprintf( buf, sizeof(buf), "%d.%d.%d.%d",
            ( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF,
            ( addr >>  8 ) & 0xFF, ( addr       ) & 0xFF
           );
    snprintf( log_buf, sizeof(log_buf), "Sock.sinaddr:  %s", buf );
    log_string( log_buf );

    /* Disabled to kill the lag beast... temporarily. -Rim 9/8/98
       from = gethostbyaddr( (char *) &sock.sin_addr,
       sizeof(sock.sin_addr), AF_INET );
       dnew->host = str_dup( from ? from->h_name : buf );
     */
    dnew->host = str_dup( buf );  /* -Rim */
}
/*
 * Swiftest: I added the following to ban sites.  I don't
 * endorse banning of sites, but Copper has few descriptors now
 * and some people from certain sites keep abusing access by
 * using automated 'autodialers' and leaving connections hanging.
 *
 * Furey: added suffix check by request of Nickel of HiddenWorlds.
 */
if ( check_ban(dnew->host,BAN_ALL))
{
    write_to_descriptor( desc, "Your site has been banned from this mud.\n\r", 0, dnew->pProtocol->bSGA);
    close( desc );
    free_descriptor(dnew);
    return;
}
/*
 * Init descriptor data.
 */
/*
   dnew->next          = descriptor_list;
   descriptor_list     = dnew;
 */
add_descriptor( dnew );

ProtocolNegotiate(dnew);

/*
 * Send the greeting.
 */
    extern char * help_greeting;
    if ( help_greeting[0] == '.' )
        write_to_buffer( dnew, help_greeting+1, 0 );
    else
        write_to_buffer( dnew, help_greeting  , 0 );

}

void close_socket( DESCRIPTOR_DATA *dclose )
{
    CHAR_DATA *ch;
    AUTH_LIST *old_auth;


    lua_unregister_desc(dclose);

    if ( dclose->outtop > 0 )
        process_output( dclose, FALSE );

    if ( dclose->snoop_by != NULL )
    {
        write_to_buffer( dclose->snoop_by,
                "Your victim has left the game.\n\r", 0 );
    }

    {
        DESCRIPTOR_DATA *d;

        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->snoop_by == dclose )
                d->snoop_by = NULL;
        }
    }

    if ( ( ch = dclose->character ) != NULL )
    {
        snprintf( log_buf, sizeof(log_buf), "Closing link to %s.", ch->name );
        log_string( log_buf );

        /* Suggested bug fix by Ivan Toledo, added 1/10/98 by Rimbol */
        if ( ch->pet && ch->pet->in_room == NULL )
        {
            char_to_room( ch->pet, get_room_index(ROOM_VNUM_LIMBO) );
            extract_char( ch->pet, TRUE );
        }


        /* Link dead auth -- Rantic */
        old_auth = get_auth_name( ch->name );
        if ( old_auth != NULL && old_auth->state == AUTH_ONLINE )
        {
            old_auth->state = AUTH_LINK_DEAD;
            save_auth_list();
        }

        /* If ch is writing note or playing, just lose link otherwise clear char */
        if (!merc_down && IS_PLAYING(dclose->connected))
        {
            act( "$n has lost $s link.", ch, NULL, NULL, TO_ROOM );
            wiznet("Net death has claimed $N.",ch,NULL,WIZ_LINKS,0,0);

            /* Sweep linkdead players out of box rooms*/
            if ( IS_SET(ch->in_room->room_flags, ROOM_BOX_ROOM))
            {
                char_from_room( ch );
                char_to_room( ch, get_room_index( ROOM_VNUM_RECALL ) );
            }
            /* Removed:  probably won't be too probematic, since the person would be
               removed from war if they (a) disappear into the void, or (b) get removed from game */
            /* war_remove( ch, FALSE );  */

            /* add quit_save_char_obj here so we won't miss any players */
            /* no longer needed since linkdead chars are saved as well --Bobble */
            //quit_save_char_obj( ch );

            ch->desc = NULL;
        }
        else
        {
            logpf("close_socket: %s not playing", ch->name);
            /* can't do extract_char cause it's not on char list,
               but we still need to do a little cleanup*/
            CHAR_DATA *wch;
            for ( wch = char_list; wch != NULL; wch = wch->next )
            {
                if ( wch->reply == ch )
                    wch->reply = NULL;
                if ( ch->mprog_target == wch )
                    wch->mprog_target = NULL;
            }

            //unregister_lua( ch );
            free_char(dclose->original ? dclose->original : dclose->character );
        }
    }

    if ( g_d_next == dclose )
        g_d_next = g_d_next->next;   

    desc_from_descriptor_list(dclose);

    set_con_state(dclose, CON_CLOSED);

    close( dclose->descriptor );
    
    free_descriptor( dclose );
    return;
}



bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    int iStart = 0;
    int lenInbuf, maxRead;

    static char read_buf[MAX_PROTOCOL_BUFFER];
    read_buf[0] = '\0';

    /* Hold horses if pending command already. */
    if ( d->incomm[0] != '\0' )
        return TRUE;

    /* Check for overflow. */
    lenInbuf = strlen(d->inbuf);
    maxRead = sizeof(d->inbuf) - (lenInbuf + 1);
    
    if ( maxRead <= 0 )
    {
        snprintf( log_buf, sizeof(log_buf), "%s input overflow!", d->host );
        log_string( log_buf );
        write_to_descriptor( d->descriptor, "\n\r*** PUT A LID ON IT!!! ***\n\r", 0, d->pProtocol->bSGA );
        return FALSE;
    }

    /* Snarf input. */
    for ( ; ; )
    {
        int nRead;

        /* There is no more space in the input buffer for now */
        if ( iStart >= maxRead )
                break;


        nRead = read( d->descriptor, read_buf + iStart, maxRead - iStart );

        if ( nRead > 0 )
        {
            iStart += nRead;
            if ( read_buf[iStart-1] == '\n' || read_buf[iStart-1] == '\r' )
                break;
        }
        else if ( nRead == 0 )
        {
            log_string( "EOF encountered on read." );
            return FALSE;
        }
        else if ( errno == EWOULDBLOCK 
#if EAGAIN != EWOULDBLOCK // avoid duplicate check to avoid compiler warning
            || errno == EAGAIN 
#endif
            )
            break;
        else
        {
            log_error( "Read_from_descriptor" );
            return FALSE;
        }
    }

    read_buf[iStart] = '\0';
    if (read_buf[0]=='&')
    {
        /* allow to clear command buffer with '&' */
        strcpy( d->inbuf, "");
    }
    else
    {
        ProtocolInput( d, read_buf, iStart, d->inbuf + lenInbuf );
    }
    return TRUE;
}

/* check if command is pending */
bool is_command_pending( DESCRIPTOR_DATA *d )
{
    if (d == NULL)
    {
        bug( "is_command_pending: NULL pointer given", 0 );
        return FALSE;
    }

    /* it's safe to call read_from_buffer since the next call will just return */
    read_from_buffer( d );
    return d->incomm[0] != '\0';
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    bool immortal=FALSE;
    int i, j, k;

    if (d->character)
        immortal=IS_IMMORTAL(d->character);

    /*
     * Hold horses if pending command already.
     */
    if ( d->incomm[0] != '\0' )
        return;

    /*
     * Look for at least one new line.
     */
    for ( i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
        if ( d->inbuf[i] == '\0' )
            return;
    }

    /*
     * Canonical input processing.
     */
    for ( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
        if ( k >= MAX_INPUT_LENGTH - 2 )
        {
            write_to_descriptor( d->descriptor, "WARNING: Line too long. Content may be missing.\n\r", 0, d->pProtocol->bSGA);

            /* skip the rest of the line */
            for ( ; d->inbuf[i] != '\0'; i++ )
            {
                if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
                    break;
            }
            d->inbuf[i]   = '\n';
            d->inbuf[i+1] = '\0';
            break;
        }

        if ( d->inbuf[i] == '\b' && k > 0 )
            --k;
        else if ( isascii(d->inbuf[i]) && isprint(d->inbuf[i]) )
        {
            if ( immortal && (d->inbuf[i] == '~') )
            {
                if (d->inbuf[i+1] == '~')
                {
                    d->incomm[k++]= '~';
                    i++;
                }
                else
                {
                    d->incomm[k++] = '\t';
                }
            }
            else
                d->incomm[k++] = d->inbuf[i];
        }
    }

    /*
     * Finish off the line.
     */
    if ( k == 0 )
        d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */

    if ( k > 1 || d->incomm[0] == '!' )
    {
        if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
        {
            d->repeat = 0;
        }
        else
        {
            if (++d->repeat >= 25 && d->character
                    &&  d->connected == CON_PLAYING)
            {
                snprintf( log_buf, sizeof(log_buf), "%s input spamming!", d->host );
                log_string( log_buf );
                wiznet("Spam spam spam $N spam spam spam spam spam!",
                        d->character,NULL,WIZ_SPAM,0,get_trust(d->character));
                if (d->incomm[0] == '!')
                    wiznet(d->inlast,d->character,NULL,WIZ_SPAM,0,
                            get_trust(d->character));
                else
                    wiznet(d->incomm,d->character,NULL,WIZ_SPAM,0,
                            get_trust(d->character));

                d->repeat = 0;
                /*
                   write_to_descriptor( d->descriptor,
                   "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
                   strcpy( d->incomm, "quit" );
                 */
            }
        }
    }


    /*
     * Do '!' substitution.
     */
    if ( d->incomm[0] == '!' )
        strcpy( d->incomm, d->inlast );
    else
        strcpy( d->inlast, d->incomm );

    /*
     * Shift the input buffer.
     */
    bool got_n = FALSE, got_r=FALSE;

    for (;d->inbuf[i] == '\r' || d->inbuf[i] == '\n';i++)
    {
        if (d->inbuf[i] == '\r' && got_r++)
            break;

        else if (d->inbuf[i] == '\n' && got_n++)
            break;
    }
    

    for ( j = 0; ( d->inbuf[j] = d->inbuf[i+j] ) != '\0'; j++ )
        ;
    return;
}

void battle_prompt( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch;
    CHAR_DATA *victim;

    int percent;
    char wound[100];
    char *pbuff;
    char buf[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH*2];

    ch = d->character;
    if ( ch == NULL || (victim = ch->fighting) == NULL
            || !can_see(ch,victim) )
        return;


    if (victim->max_hit > 0)
        percent = victim->hit * 100 / victim->max_hit;
    else
        percent = -1;

    if (IS_AFFECTED(ch, AFF_BATTLE_METER))
    {
        char color;
        int i, bars;

        if (percent<0)
            snprintf(buf, sizeof(buf), "{W[              {RKILL{x               {W]{x\n\r");
        else
        {
            if (percent==100)
                color = 'W';
            else if (percent>85)
                color = 'G';
            else if (percent>66)
                color = 'g';
            else if (percent>50)
                color = 'Y';
            else if (percent>33)
                color = 'y';
            else if (percent>16)
                color = 'R';
            else
                color = 'r';

            snprintf(buf, sizeof(buf),
                    "{W[{%c                                {W] %s {W({%c%d%%{W){x\n\r",
                    color, IS_NPC(victim) ? victim->short_descr : victim->name, color, percent);

            bars=UMIN(((percent*32)/100), 32);
            for (i=0; i<bars; i++)
                buf[5+i]='|';
        }
    } else {

        if (percent >= 100)
            snprintf(wound, sizeof(wound),"is in excellent condition.");
        else if (percent >= 90)
            snprintf(wound, sizeof(wound),"has a few scratches.");
        else if (percent >= 75)
            snprintf(wound, sizeof(wound),"has some small wounds and bruises.");
        else if (percent >= 50)
            snprintf(wound, sizeof(wound),"has quite a few wounds.");
        else if (percent >= 30)
            snprintf(wound, sizeof(wound),"has some big nasty wounds and scratches.");
        else if (percent >= 15)
            snprintf(wound, sizeof(wound),"looks pretty hurt.");
        else if (percent >= 0)
            snprintf(wound, sizeof(wound),"is in awful condition.");
        else
            snprintf(wound, sizeof(wound),"is bleeding to death.");

        snprintf(buf, sizeof(buf),"%s %s \n\r", 
                IS_NPC(victim) ? victim->short_descr : victim->name,wound);
        buf[0]  = UPPER( buf[0] );
    }
    pbuff   = buffer;
    colourconv( pbuff, buf, d->character );
    write_to_buffer( d, buffer, 0);
}

/*
 * Low level output function.
 */
bool process_output( DESCRIPTOR_DATA *d, bool fPrompt )
{
    extern bool merc_down;

    /*
     * Bust a prompt.
     */
    if ( !d->pProtocol->WriteOOB && !merc_down && !d->last_msg_was_prompt )
    {
        if ( d->showstr_point )
            write_to_buffer( d, "[Hit Return to continue]\n\r", 0 );
        else if ( fPrompt && (d->pString || d->lua.interpret) && (d->connected == CON_PLAYING || d->connected == CON_PENALTY_FINISH ))
        {
            if (d->lua.interpret)
            {
                write_to_buffer( d, "lua", 3);
                if (d->lua.incmpl)
                    write_to_buffer( d, ">", 1);

            }
            write_to_buffer( d, "> ", 2 );
        }
        else if ( fPrompt && d->connected == CON_PLAYING )
        {
            CHAR_DATA *ch = d->character;

            battle_prompt(d);

            ch = d->original ? d->original : d->character;
            if (!IS_SET(ch->comm, COMM_COMPACT) )
                write_to_buffer( d, "\n\r", 2 );

            if ( IS_SET(ch->comm, COMM_PROMPT) )
                bust_a_prompt( d->character );
        }
    }
    
    /*
     * Snoop-o-rama.
     */
    if ( d->snoop_by != NULL && d->outtop > 0 && !d->last_msg_was_prompt )
    {
        if (d->character != NULL)
            write_to_buffer( d->snoop_by, d->character->name,0);
        write_to_buffer( d->snoop_by, "> ", 2 );
        write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
    }

    if (flush_descriptor(d))
    {
        d->last_msg_was_prompt = TRUE;
        return TRUE;
    }
    else
        return FALSE;
}

/* flush the descriptor's buffered text */
bool flush_descriptor( DESCRIPTOR_DATA *d )
{
    /* if nothing to write, return - otherwise write_to_descriptor tries
       to determin it's own string length */
    if (d->outtop == 0)
        return TRUE;

    int written = write_to_descriptor(d->descriptor, d->outbuf, d->outtop, d->pProtocol->bSGA);
    if ( !written )
    {
        d->outtop = 0;
        return FALSE;
    }
    else
    {
        if ( written < d->outtop )
        {
            int i;
            logpf("flush_descriptor: only %d out of %d chars written", written, d->outtop);
            // copy remaining chars to beginning of string
            for ( i = written; i < d->outtop; i++ )
                d->outbuf[i-written] = d->outbuf[i];
            d->outtop -= written;
        }
        else
            d->outtop = 0;
        return TRUE;
    }
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MSL];
    const char *str;
    const char *i;
    char *point;
    char *pbuff;
    char buffer[ MAX_STRING_LENGTH*2 ];
    char doors[MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool insane;
    const char *dir_name_lookup[] = {"N","E","S","W","U","D","Ne","Se","Sw","Nw"};
    int door;
    int pct;

    point = buf;
    str = ch->prompt;
    if( !str || str[0] == '\0')
    {
        snprintf( buf, sizeof(buf), "{g<{r%d{g/%dhp {c%d{g/%dm {y%d{getl>{x %s",     
                ch->hit,ch->max_hit,ch->mana,ch->max_mana, IS_NPC(ch) ? 0 :
                (ch->level + 1) * exp_per_level(ch) - ch->exp,
                ch->prefix);
        send_to_char(buf,ch);
        return;
    }

    if (IS_SET(ch->comm,COMM_AFK))
    {
        send_to_char("<AFK> ",ch);
        return;
    }

    if (IS_SET(ch->act, PLR_UNAUTHED))
        send_to_char("{y[Pre-Auth]{x ",ch);

    insane = IS_AFFECTED(ch, AFF_INSANE);

    while( *str != '\0' )
    {
        if( *str != '%' )
        {
            *point++ = *str++;
            continue;
        }
        ++str;
        switch( *str )
        {
            default :
                i = " "; break;
            case 'b':
                snprintf( buf3, sizeof(buf3), "%s%s%s%s%s%s%s%s" ,
                NPC_OFF(ch, OFF_RESCUE) || PLR_ACT(ch, PLR_AUTORESCUE) ? "{WR{x" : " ",
                is_affected(ch, gsn_bless) || is_affected(ch, gsn_prayer) ? "{WB{x" : get_skill(ch, gsn_bless) > 1 ? "{Rb{x" : " ",
                IS_AFFECTED(ch, AFF_FLYING) ? "{WF{x" : get_skill(ch, gsn_fly) > 1 ? "{Rf{x" : " ",
                is_affected(ch, gsn_giant_strength) ? "{WG{x" : get_skill(ch, gsn_giant_strength) > 1 ? "{Rg{x" : " ",
                IS_AFFECTED(ch, AFF_HASTE) ? "{WH{x" : !IS_AFFECTED(ch, AFF_SLOW) && get_skill(ch, gsn_haste) > 1 ? "{Rh{x" : " ",
                IS_AFFECTED(ch, AFF_SANCTUARY) ? "{WS{x" : get_skill(ch, gsn_sanctuary) > 1 ? "{Rs{x" : " ",
                is_affected(ch, gsn_war_cry) ? "{WW{x" : get_skill(ch, gsn_war_cry) > 1 ? "{Rw{x" : " ",
                IS_AFFECTED(ch, AFF_BERSERK) ? "{WZ{x" : get_skill(ch, gsn_frenzy) > 1 ? "{Rz{x" : " ");
                snprintf( buf2, sizeof(buf2), "%s", buf3 );
                i = buf2; break;
            case 'e':
                found = FALSE;
                doors[0] = '\0';
                for (door = 0; door < MAX_DIR; door++)
                {
                    if ((pexit = ch->in_room->exit[door]) != NULL
                            &&  pexit ->u1.to_room != NULL
                            &&  (can_see_room(ch,pexit->u1.to_room)
                                ||   (IS_AFFECTED(ch,AFF_INFRARED) 
                                    &&    !IS_AFFECTED(ch,AFF_BLIND)))
                            &&  !IS_SET(pexit->exit_info,EX_CLOSED)
                            &&  !IS_SET(pexit->exit_info,EX_DORMANT))
                    {
                        found = TRUE;
                        strcat(doors,dir_name_lookup[door]);
                    }
                }
                if (!found)
                    strcat(buf,"none");
                snprintf(buf2, sizeof(buf2),"%s",doors);
                i = buf2; break;
                /* Players can track new rooms that they've visited - Astark Nov 2012 */
            case 'E' :
                /* Oops! forgot the NPC check here. Was causing a crash. Astark 1-17-13 */
                if (!IS_NPC(ch))
                {
                    snprintf(buf2, sizeof(buf2),"%d", ch->pcdata->explored->set );
                    i = buf2; break;
                }
            case 'c' :
                snprintf(buf2, sizeof(buf2),"%s","\n\r");
                i = buf2; break;
            case 'h' :
                if (insane)
                    snprintf( buf2, sizeof(buf2), "%d", number_range(1, ch->max_hit));
                else
                    snprintf( buf2, sizeof(buf2), "%d", ch->hit );
                i = buf2; break;
            case 'H' :
                if ( ch->hit_cap_delta )
                    snprintf( buf2, sizeof(buf2), "%d[%d]", hit_cap(ch), ch->max_hit );
                else
                    snprintf( buf2, sizeof(buf2), "%d", ch->max_hit );
                i = buf2; break;
            case 'm' :
                if (insane)
                    snprintf( buf2, sizeof(buf2), "%d", number_range(1, ch->max_mana));
                else
                    snprintf( buf2, sizeof(buf2), "%d", ch->mana );
                i = buf2; break;
            case 'M' :
                if ( ch->mana_cap_delta )
                    snprintf( buf2, sizeof(buf2), "%d[%d]", mana_cap(ch), ch->max_mana );
                else
                    snprintf( buf2, sizeof(buf2), "%d", ch->max_mana );
                i = buf2; break;
            case 'v' :
                if (insane)
                    snprintf( buf2, sizeof(buf2), "%d", number_range(1, ch->max_move) );
                else
                    snprintf( buf2, sizeof(buf2), "%d", ch->move );
                i = buf2; break;
            case 'V' :
                if ( ch->move_cap_delta )
                    snprintf( buf2, sizeof(buf2), "%d[%d]", move_cap(ch), ch->max_move );
                else
                    snprintf( buf2, sizeof(buf2), "%d", ch->max_move );
                i = buf2; break;

                /* Battle prompts by Brian Castle. Idea from NB's snippet. */
            case 'y' :     /* Self */
                if (!IS_NPC(ch))
                {
                    pct = ch->max_hit > 0 ? (ch->hit * 100) / ch->max_hit : -1;
                    if ( pct >= 100 ) snprintf( buf2, sizeof(buf2), "Excl" );
                    else if ( pct >=  90 ) snprintf( buf2, sizeof(buf2), "Scrt" );
                    else if ( pct >=  75 ) snprintf( buf2, sizeof(buf2), "Smal" );
                    else if ( pct >=  50 ) snprintf( buf2, sizeof(buf2), "QFew" );
                    else if ( pct >=  30 ) snprintf( buf2, sizeof(buf2), "BigN" );
                    else if ( pct >=  15 ) snprintf( buf2, sizeof(buf2), "Hurt" );
                    else if ( pct >=  0  ) snprintf( buf2, sizeof(buf2), "Awfl" );
                    else                   snprintf( buf2, sizeof(buf2), "Dyng" );
                    i = buf2;
                }
                else
                    continue;
                break;
            case 'f' :    /* Target (if fighting) */
                if (!IS_NPC(ch) && ch->fighting)
                {
                    pct = ch->fighting->max_hit > 0 ? (ch->fighting->hit * 100)
                        / ch->fighting->max_hit : -1;
                    if ( pct >= 100 ) snprintf( buf2, sizeof(buf2), "Excl" );
                    else if ( pct >=  90 ) snprintf( buf2, sizeof(buf2), "Scrt" );
                    else if ( pct >=  75 ) snprintf( buf2, sizeof(buf2), "Smal" );
                    else if ( pct >=  50 ) snprintf( buf2, sizeof(buf2), "QFew" );
                    else if ( pct >=  30 ) snprintf( buf2, sizeof(buf2), "BigN" );
                    else if ( pct >=  15 ) snprintf( buf2, sizeof(buf2), "Hurt" );
                    else if ( pct >=  0  ) snprintf( buf2, sizeof(buf2), "Awfl" );
                    else                   snprintf( buf2, sizeof(buf2), "Dyng" );
                    i = buf2;
                }
                else
                {
                    str++;
                    continue;
                }
                break;
            case 't' :    /* Tank (if fighting & someone else is tanking)*/
                if (!IS_NPC(ch) && ch->fighting)
                {
                    if (ch->fighting->fighting && ch->fighting->fighting != ch)
                    {
                        pct = ch->fighting->fighting->max_hit > 0 ? (ch->fighting->fighting->hit * 100) / ch->fighting->fighting->max_hit : -1;
                        if ( pct <= 25 ) snprintf( buf2, sizeof(buf2), "{R%d(%d%%){x", ch->fighting->fighting->hit, pct);
                        else snprintf( buf2, sizeof(buf2), "%d(%d%%)", ch->fighting->fighting->hit, pct);
                        i = buf2;
                    }
                    else
                    {            
                        str++;
                        continue;
                    }
                }
                else
                {
                    str++;
                    continue;
                }
                break;
            case 'T' : /* current mud time */
                snprintf( buf2, sizeof(buf2), "%d%s",
                        (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
                        time_info.hour >= 12 ? "pm" : "am" );
                i = buf2; break;
            case 'x' :
                snprintf( buf2, sizeof(buf2), "%d", ch->exp );
                i = buf2; break;
            case 'X' :
                snprintf(buf2, sizeof(buf2), "%d", IS_NPC(ch) ? 0 :
                        (ch->level + 1) * exp_per_level(ch) - ch->exp);
                i = buf2; break;
            case 'F' :
                snprintf(buf2, sizeof(buf2), "%ld", IS_NPC(ch) ? 0 : ch->pcdata->field );
                i = buf2; break;
            case 'g' :
                snprintf( buf2, sizeof(buf2), "%ld", ch->gold);
                i = buf2; break;
            case 's' :
                snprintf( buf2, sizeof(buf2), "%ld", ch->silver);
                i = buf2; break;
            case 'q' :
                snprintf( buf2, sizeof(buf2), "%d", IS_NPC(ch) ? 0 : ch->pcdata->questpoints);
                i = buf2; break;
            case 'a' :
                if( ch->level > 9 )
                    snprintf( buf2, sizeof(buf2), "%d", ch->alignment );
                else
                    snprintf( buf2, sizeof(buf2), "%s", IS_GOOD(ch) ? "good" : IS_EVIL(ch) ?
                            "evil" : "neutral" );
                i = buf2; break;
            case 'l' :
                snprintf( buf2, sizeof(buf2), "%d", ch->level);
                i = buf2; break;
            case 'r' :
                if( ch->in_room != NULL )
                    snprintf( buf2, sizeof(buf2), "%s", 
                            ((!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT)) ||
                             (!IS_AFFECTED(ch,AFF_BLIND) && !room_is_dark( ch->in_room )))
                            ? ch->in_room->name : "darkness");
                else
                    snprintf( buf2, sizeof(buf2), " " );
                i = buf2; break;
            case 'R' :
                if( IS_IMMORTAL( ch ) && ch->in_room != NULL )
                    snprintf( buf2, sizeof(buf2), "%d", ch->in_room->vnum );
                else
                    snprintf( buf2, sizeof(buf2), " " );
                i = buf2; break;
            case 'z' :
                /* Allowing morts to use this, 12/22/97 -BC */
                if( ch->in_room != NULL )
                    snprintf( buf2, sizeof(buf2), "%s", ch->in_room->area->name );
                else
                    snprintf( buf2, sizeof(buf2), " " );
                i = buf2; break;
            case 'Z' :
                if ( ch->in_room != NULL )
                    snprintf( buf2, sizeof(buf2), "%s", flag_bit_name(sector_flags, ch->in_room->sector_type));
                else
                    snprintf( buf2, sizeof(buf2), " " );
                i = buf2; break;
            case '%' :
                snprintf( buf2, sizeof(buf2), "%%" );
                i = buf2; break;
            case 'o' :
                snprintf( buf2, sizeof(buf2), "%s", olc_ed_name(ch) );
                i = buf2; break;
            case 'O' :
                snprintf( buf2, sizeof(buf2), "%s", olc_ed_vnum(ch) );
                i = buf2; break;
            case 'Q' :
                snprintf( buf2, sizeof(buf2), "%d", IS_NPC(ch) ? 0 : ch->pcdata->nextquest );
                i = buf2; break;
            case 'C' :
                snprintf( buf2, sizeof(buf2), "%d", IS_NPC(ch) ? 0 : ch->pcdata->countdown );
                i = buf2; break;
            case 'S' :
                snprintf( buf2, sizeof(buf2), "%s", stances[ch->stance].name );
                i = buf2; break;
            case 'd' :
                snprintf( buf2, sizeof(buf2), "%s%s%s",
                        IS_AFFECTED(ch, AFF_DETECT_INVIS) ? "i" : "",
                        IS_AFFECTED(ch, AFF_DETECT_HIDDEN) ? "h" : "",
                        IS_AFFECTED(ch, AFF_DETECT_ASTRAL) ? "a" : "" );
                i = buf2; break;

            case 'p' :
                if( !IS_NPC(ch) && MULTI_MORPH(ch) )
                {
                    if( ch->pcdata->morph_race > 0 )
                        snprintf( buf2, sizeof(buf2), "%s", race_table[ch->pcdata->morph_race].name );
                    else
                        snprintf( buf2, sizeof(buf2), "unmorphed" );
                }
                else if( !IS_NPC(ch) && ch->race == race_naga )
                {
                    if( ch->pcdata->morph_race == 0 )
                        snprintf( buf2, sizeof(buf2), "serpent" );
                    else
                        snprintf( buf2, sizeof(buf2), "humanoid" );
                }
                else strcpy(buf2, "");
                i = buf2; break;

            case 'P' :
                if( !IS_NPC(ch) && MULTI_MORPH(ch) )
                {
                    if( ch->pcdata->morph_race > 0 )
                        snprintf( buf2, sizeof(buf2), "%d", ch->pcdata->morph_time );
                    else
                        snprintf( buf2, sizeof(buf2), "..." );
                }
                else strcpy(buf2, "");
                i = buf2; break;
            case 'n' :
                snprintf( buf2, sizeof(buf2), "%s", songs[ch->song].name );
                i = buf2; break;
        }
        ++str;
        while( (*point = *i) != '\0' )
            ++point, ++i;
    }
    *point  = '\0';
    pbuff   = buffer;
    colourconv( pbuff, buf, ch );
    write_to_buffer( ch->desc, buffer, 0 );

    if (ch->prefix[0] != '\0')
        write_to_buffer(ch->desc,ch->prefix,0);
    return;
}


/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{
    txt = ProtocolOutput( d, txt, &length );
    if ( d->pProtocol==NULL )
        bugf("pProtocol null");
    if ( d->pProtocol->WriteOOB > 0 )
        --d->pProtocol->WriteOOB;

    /*
     * Find length in case caller didn't.
     */
    if ( length <= 0 )
        length = strlen(txt);

    /*
     * Initial \n\r if needed.
     */
    if ( d->outtop == 0 && !d->fcommand && !d->pProtocol->WriteOOB )
    {
        d->outbuf[0]    = '\n';
        d->outbuf[1]    = '\r';
        d->outtop   = 2;
    }

    /* set last_msg_was_prompt to FALSE regardless of message type - 
       if it was a prompt, the prompt method sets it to TRUE afterwards */
    d->last_msg_was_prompt = FALSE;

    /*
     * Expand the buffer as needed.
     */
    while ( d->outtop + length >= d->outsize )
    {
        char *outbuf;

        if (d->outsize >= 32768)
        {
            /* just trash new messages.. --Bobble */
            //bug("Buffer overflow. Closing.\n\r",0);
            //close_socket(d);
            bug("Buffer overflow. Clearing buffer.\n\r", 0);
            d->outtop = 0;
            d->outbuf[0] = '\0';
            /* get the spammer.. */
            punish_spam();
            return;
        }
        outbuf      = alloc_mem( 2 * d->outsize );
        strncpy( outbuf, d->outbuf, d->outtop );
        free_mem( d->outbuf, d->outsize );
        d->outbuf   = outbuf;
        d->outsize *= 2;
    }

    /*
     * Copy. And *bonk* Russ Taylor.
     * (Added 1/10/98 by Rimbol, suggested bug fix
     *  by Alexander A. Isavnin <isavnin@redline.ru>)
     */
    strncpy( d->outbuf + d->outtop, txt , length);
    d->outtop += length;
    d->outbuf[d->outtop]='\0';

    /*
     * Copy.
     */
    /*  Here's the stock version of these lines. -Rim
        strcpy( d->outbuf + d->outtop, txt );
        d->outtop += length;
        return;
     */

    /* flush buffer immediately, useful for debugging */
    if (global_immediate_flush)
        flush_descriptor(d);
}



/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
#define MAX_BLOCK_SIZE 32768 
int write_to_descriptor( int desc, char *txt, int length, bool SGA)
{
    const char const IAC_GA[] = { IAC, GA };
    static char outbuf[MAX_BLOCK_SIZE + sizeof(IAC_GA)];

    int iStart;
    int nWrite, nWritten = 0;
    char *write_src;
    int write_src_cnt;

    if ( length <= 0 )
        length = strlen(txt);
    
    // limit total output written "in one go" to avoid write errors
    length = UMIN(length, MAX_BLOCK_SIZE );

    if (!SGA)
    {
        memcpy(outbuf, txt, length);
        memcpy(outbuf + length, IAC_GA, sizeof(IAC_GA));
        write_src = outbuf;
        write_src_cnt = length + sizeof(IAC_GA);
    }
    else
    {
        write_src = txt;
        write_src_cnt = length;
    }

    for ( iStart = 0; iStart < write_src_cnt; iStart += nWrite )
    {
        int nBlock = UMIN( write_src_cnt - iStart, MAX_BLOCK_SIZE );
        if ( ( nWrite = write( desc, write_src + iStart, nBlock ) ) < 0 )
        {
            log_error( "Write_to_descriptor" );
            return 0;
        }
        nWritten += nWrite;
    } 
    
    if (!SGA)
        nWritten -= sizeof(IAC_GA);

    return nWritten;
}
#undef MAX_BLOCK_SIZE

void stop_idling( CHAR_DATA *ch )
{

    if ( ch == NULL || ch->desc == NULL
            || (!IS_PLAYING(ch->desc->connected)) )
        return;

    /* Removed before implementation because the testers didn't like it...
       if ( IS_SET(ch->comm, COMM_AFK) && str_cmp(ch->desc->incomm,"afk") )
       do_afk( ch, "" );
     */

    if ( ch->was_in_room == NULL || ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
        return;

    ch->timer = 0;
    char_from_room( ch );
    if ( IS_SET(ch->was_in_room->room_flags, ROOM_BOX_ROOM) )
    {
        char_to_room( ch, get_room_index( ROOM_VNUM_RECALL));
    }
    else
    {
        char_to_room( ch, ch->was_in_room );
    }

    ch->was_in_room = NULL;

    ap_unvoid_trigger(ch);
    act( "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
    return;
}



/*
 * Write to one char.
 */
void send_to_char_bw( const char *txt, CHAR_DATA *ch )
{
    if ( txt != NULL && ch->desc != NULL )
        write_to_buffer( ch->desc, txt, strlen(txt) );
    return;
}

/*
 * Write to one char, new colour version, by Lope.
 */
void send_to_char( const char *txt, CHAR_DATA *ch )
{
    send_to_char_new( txt, ch, FALSE);
}

void send_to_char_new( const char *txt, CHAR_DATA *ch, bool raw )
{
    const  char    *point;
    char    *point2;
    char    buf[ MAX_STRING_LENGTH*4 ];
    int skip = 0;

    // players may overwrite whether color is sent in raw format
    if ( IS_SET(ch->act, PLR_COLOUR_VERBATIM) )
        raw = TRUE;
    
    buf[0] = '\0';
    point2 = buf;
    if( txt && ch->desc )
    {
        if ( raw )
        {
            for( point = txt ; *point ; point++ )
            {
                // escape MXP
                if ( *point == '\t' && *(point+1) == '<' )
                    *(point2++) = '~';
                else
                    *(point2++) = *point;
            }
        }
        else if ( IS_SET(ch->act, PLR_COLOUR) )
        {
            for( point = txt ; *point ; point++ )
            {
                if( *point == '{' )
                {
                    point++;
                    skip = colour( *point, ch, point2 );
                    while( skip-- > 0 )
                        ++point2;
                    continue;
                }
                *(point2++) = *point;
            }
        }
        else
        {
            for( point = txt ; *point ; point++ )
            {
                if( *point == '{' )
                {
                    point++;
                    continue;
                }
                *(point2++) = *point;
            }
        }
        *point2 = '\0';
        write_to_buffer(ch->desc, buf, point2 - buf);
    }
    return;
}

/*
 * Send a page to one char.
 */
void page_to_char_bw( const char *txt, CHAR_DATA *ch )
{
    if ( txt == NULL || ch->desc == NULL)


        if (ch->lines == 0 )
        {
            send_to_char_bw(txt,ch);
            return;
        }

    ch->desc->showstr_head = malloc(strlen(txt) + 1);
    strcpy(ch->desc->showstr_head,txt);
    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string(ch->desc,"");
}


/*
 * Page to one char, new colour version, by Lope.
 */
#define MAX_BUF_INDEX (MAX_STRING_LENGTH*12)
void page_to_char( const char *txt, CHAR_DATA *ch )
{
    page_to_char_new( txt, ch, FALSE );
}

void page_to_char_new( const char *txt, CHAR_DATA *ch, bool raw )
{
    const  char    *point;
    char    *point2;
    char    buf[ MAX_BUF_INDEX + MSL ]; // some safety space
    int skip = 0;

    /* safety-net for super-long texts */
    if ( strlen(txt) > MAX_BUF_INDEX )
    {
        bugf( "page_to_char: string paged to %s too long: %d",
                ch->name, strlen(txt) );
        return;
    }
    
    // players may overwrite whether color is sent in raw format
    if ( IS_SET(ch->act, PLR_COLOUR_VERBATIM) )
        raw = TRUE;    

    buf[0] = '\0';
    point2 = buf;
    if( txt && ch->desc )
    {
        if ( raw )
        {
            for ( point = txt ; *point ; point++ )
            {
                // escape MXP
                if ( *point == '\t' && *(point+1) == '<' )
                    *(point2++) = '~';
                else
                    *(point2++) = *point;
            }
        }
        else if ( IS_SET(ch->act, PLR_COLOUR) )
        {
            for( point = txt ; *point ; point++ )
            {
                /* safety net --Bobble */
                if ( point2 - buf > MAX_BUF_INDEX )
                {
                    bugf( "page_to_char: string paged to %s too long",
                            ch->name );
                    return;
                }

                if ( *point == '{' )
                {
                    point++;
                    skip = colour( *point, ch, point2 );
                    while( skip-- > 0 )
                        ++point2;
                    continue;
                }
                *(point2++) = *point;
            }           
        }
        else
        {
            for( point = txt ; *point ; point++ )
            {
                if( *point == '{' )
                {
                    point++;
                    continue;
                }
                *(point2++) = *point;
            }
        }
        *point2 = '\0';
        ch->desc->showstr_head = malloc(strlen(buf) + 1);
        strcpy(ch->desc->showstr_head, buf);
        ch->desc->showstr_point = ch->desc->showstr_head;
        show_string(ch->desc, "");
    }
    return;
}

/* string pager */
void show_string(struct descriptor_data *d, char *input)
{
    char buffer[4*MAX_STRING_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;

    one_argument(input,buf);
    // quick-and-dirty trick for quick downloading of notes */
    if (buf[0] != '\0' && strcmp(buf, "note") )
    {
        if (d->showstr_head)
        {
            free(d->showstr_head);
            d->showstr_head = 0;
        }
        d->showstr_point  = 0;
        return;
    }

    if (d->character)
        show_lines = d->character->lines;
    else
        show_lines = 0;

    for (scan = buffer; ; scan++, d->showstr_point++)
    {
        if ( (scan - buffer) >= (MSL*4))
        {
            bugf( "show_string: buffer overflow for %s, first 100 chars below\n\r%.100s",
                    d->character ? d->character->name : "NULL",
                    buffer);
            return;
        }
        if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
                && (toggle = -toggle) < 0)
            lines++;

        else if (!*scan || (show_lines > 0 && lines >= show_lines))
        {
            *scan = '\0';
            write_to_buffer(d,buffer,strlen(buffer));
            for (chk = d->showstr_point; isspace(*chk); chk++)
                ;
            {
                if (!*chk)
                {
                    if (d->showstr_head)
                    {
                        free(d->showstr_head);
                        d->showstr_head = 0;
                    }
                    d->showstr_point  = 0;
                }
            }
            return;
        }
    }
    return;
}


/* non-trigger act */
void nt_act( const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type )
{
    bool old_state = MOBtrigger;
    MOBtrigger = FALSE;
    act( format, ch, arg1, arg2, type );
    MOBtrigger = old_state;
}
void nt_act_new ( const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
        int type, int min_pos )
{
    bool old_state = MOBtrigger;
    MOBtrigger = FALSE;
    act_new( format, ch, arg1, arg2, type, min_pos );
    MOBtrigger = old_state;
}

void act( const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
        int type )
{
    act_new(format,ch,arg1,arg2,type,POS_RESTING);
}

void act_new( const char *format, CHAR_DATA *ch, const void *arg1, 
        const void *arg2, int type, int min_pos )
{
    act_new_gag(format, ch, arg1, arg2, type, min_pos, 0, FALSE);
}

void act_gag(const char *format, CHAR_DATA *ch, const void *arg1, 
        const void *arg2, int type, long gag_type)
{
    act_new_gag(format, ch, arg1, arg2, type, POS_RESTING, gag_type, FALSE);
}

void act_see( const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type )
{
    act_new_gag(format, ch, arg1, arg2, type, POS_RESTING, 0, TRUE);
}


/* like act_new, but checks if the information should be gagged
 */
void act_new_gag( const char *format, CHAR_DATA *ch, const void *arg1, 
        const void *arg2, int type, int min_pos, long gag_type,
        bool see_only )
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };

    sh_int arg1_type=ACT_ARG_UNDEFINED;
    sh_int arg2_type=ACT_ARG_UNDEFINED;

    CHAR_DATA      *to;
    CHAR_DATA      *vch = ( CHAR_DATA * ) arg2;
    OBJ_DATA       *obj1 = ( OBJ_DATA  * ) arg1;
    OBJ_DATA       *obj2 = ( OBJ_DATA  * ) arg2;
    const  char    *str;
    const char     *i = NULL;
    char       *point;
    char       *pbuff;
    char       buffer[ MAX_STRING_LENGTH*2 ];
    char       buf[ MAX_STRING_LENGTH   ];
    char       fname[ MAX_INPUT_LENGTH  ];
    CHAR_DATA      *next_char;
    bool act_wizi;

    /*
     * Discard null and zero-length messages.
     */
    if( !format || !*format )  
        return;

    /* discard null rooms and chars */
    if( !ch || !ch->in_room ) 
        return;

    to = ch->in_room->people;
    if( type == TO_VICT )
    {
        if ( !vch )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

        if (!vch->in_room)
            return;

        to = vch->in_room->people;
        arg2_type=ACT_ARG_CHARACTER;
    }

    act_wizi = IS_NPC(ch) && IS_SET(ch->act, ACT_WIZI);

    for( ; to ; to = next_char )
    {
        next_char = to->next_in_room;

        if ( gag_type > 0 && IS_SET(to->gag, gag_type) )
            continue;
        // aura and sunburn are also types of damage
        if ( (gag_type == GAG_AURA || gag_type == GAG_SUNBURN) && IS_SET(to->gag, GAG_DAMAGE) )
            continue;

        if (act_wizi && !IS_IMMORTAL(to))
            continue;

        /*  Mobprogram fix with acts */
        if ( (to->desc == NULL
                    && (!IS_NPC(to) || !HAS_TRIGGER(to, TRIG_ACT)))
                || to->position < min_pos )
            continue;

        if( ( type == TO_CHAR ) && to != ch )  
            continue;
        if( type == TO_GROUP && !is_same_group(to, ch) )
            continue;
        if( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if( type == TO_ROOM && to == ch )
            continue;
        if( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;
        if( type == TO_ROOM_UNAUTHED 
                && (to == ch 
                    || (!NOT_AUTHED(to) && !IS_IMMORTAL(to))))
            continue;

        if ( see_only )
        {
            int chance = 50 + get_skill(to, gsn_alertness)/2;

            if ( !check_see(to, ch) || (!IS_NPC(ch) && per_chance(chance)) )
                continue;
        }

        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }
            ++str;
            i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                    default:  bug( "Act: bad code %d.", *str );
                              i = " <@@@> ";                                break;
                              /* Thx alex for 't' idea */
                    case 't': if ( arg1 == NULL ) { i = " <@@@> ";}
                                  else { i = (char *) arg1; arg1_type=ACT_ARG_TEXT;}
                                  break;
                    case 'T': i = (char *) arg2; arg2_type=ACT_ARG_TEXT;    break;
                              //		case 'n': i = PERS( ch,  to  );                         break;
                              //		case 'N': i = PERS( vch, to  );                         break;
                    case 'n': i = get_mimic_PERS( ch,  to );                         break;
                    case 'N': i = get_mimic_PERS( vch,  to );                         break;
                    case 'e': i = he_she  [URANGE(0, ch  ->sex, 2)];        break;
                    case 'E': i = he_she  [URANGE(0, vch ->sex, 2)];        break;
                    case 'm': i = him_her [URANGE(0, ch  ->sex, 2)];        break;
                    case 'M': i = him_her [URANGE(0, vch ->sex, 2)];        break;
                    case 's': i = his_her [URANGE(0, ch  ->sex, 2)];        break;
                    case 'S': i = his_her [URANGE(0, vch ->sex, 2)];        break;

                    case 'p':
                              i = can_see_obj( to, obj1 )
                                  ? obj1->short_descr
                                  : "something";
                              arg1_type=ACT_ARG_OBJ;
                              break;

                    case 'P':
                              i = can_see_obj( to, obj2 )
                                  ? obj2->short_descr
                                  : "something";
                              arg2_type=ACT_ARG_OBJ;
                              break;

                    case 'd':
                              if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                              {
                                  i = "door";
                              }
                              else
                              {
                                  one_argument( (char *) arg2, fname );
                                  i = fname;
                                  arg2_type=ACT_ARG_TEXT;
                              }
                              break;
                }
            }

            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }

        *point++ = '\n';
        *point++ = '\r';
        *point   = '\0';  
        buf[0]   = UPPER(buf[0]);
        pbuff    = buffer;
        colourconv( pbuff, buf, to );
        if (to->desc && (IS_PLAYING(to->desc->connected )))
        {
            write_to_buffer( to->desc, buffer, 0 );
        }
        else
            if ( MOBtrigger )
                mp_act_trigger( buf, to, ch, arg1, arg1_type, arg2, arg2_type, TRIG_ACT );
    }

    return;
}

void recho( const char *msg, ROOM_INDEX_DATA *room )
{
    CHAR_DATA * ch;
    char buf[MSL];
    
    if ( !room || !room->people )
        return;

    snprintf(buf, sizeof(buf), "%s\n\r", msg);
    for ( ch = room->people; ch; ch = ch->next_in_room )
        send_to_char(buf, ch);
}

int colour( char type, CHAR_DATA *ch, char *string )
{
    PC_DATA   *col;
    char   code[ 20 ];
    char   *p = '\0';

    if( ch && IS_NPC( ch ) )
        return( 0 );

    col = ch->pcdata;

    switch( type )
    {
        default:
            strcpy( code, CLEAR );
            break;
        case 'x':
            strcpy( code, CLEAR );
            break;
#define CUSTC(chr, field) \
    case chr: \
        if ( col->field[1] == COLOUR_NONE ) \
        { \
            snprintf( code, sizeof(code), "%s%s", CLEAR, col->field[2] ? "\a" : ""); \
        } \
        else \
        { \
            snprintf( code, sizeof(code), "[%d;3%dm%s", \
                    col->field[0], \
                    col->field[1], \
                    col->field[2] ? "\a" : ""); \
        } \
        break;

        CUSTC('o', room_title);
        CUSTC('O', room_exits);
        CUSTC('p', gossip);
        CUSTC('P', gossip_text);
        CUSTC('a', auction);
        CUSTC('A', auction_text);
        CUSTC('e', music);
        CUSTC('E', music_text);
        CUSTC('q', question);
        CUSTC('Q', question_text);
        CUSTC('j', answer);
        CUSTC('J', answer_text);
        CUSTC('h', quote);
        CUSTC('H', quote_text);
        CUSTC('z', gratz);
        CUSTC('Z', gratz_text);
        CUSTC('i', immtalk);
        CUSTC('I', immtalk_text);
        CUSTC('7', savantalk);
        CUSTC('8', savantalk_text);
        CUSTC('u', shouts);
        CUSTC('U', shouts_text);
        CUSTC('t', tells);
        CUSTC('T', tell_text);
        CUSTC('1', info);
        CUSTC('2', info_text);
        CUSTC('k', gametalk);
        CUSTC('K', gametalk_text);
        CUSTC('f', bitch);
        CUSTC('F', bitch_text);
        CUSTC('n', newbie);
        CUSTC('N', newbie_text);
        CUSTC('l', clan);
        CUSTC('L', clan_text);
        CUSTC('s', say);
        CUSTC('S', say_text);
        CUSTC('3', gtell);
        CUSTC('4', gtell_text);
        CUSTC('V', wiznet);
        CUSTC('5', warfare);
        CUSTC('6', warfare_text);
        CUSTC('9', proclaim);
        CUSTC('0', proclaim_text);
#undef CUSTC

        case 'b':
            strcpy( code, C_BLUE );
            break;
        case 'c':
            strcpy( code, C_CYAN );
            break;
        case 'g':
            strcpy( code, C_GREEN );
            break;
        case 'm':
            strcpy( code, C_MAGENTA );
            break;
        case 'r':
            strcpy( code, C_RED );
            break;
        case 'w':
            strcpy( code, C_WHITE );
            break;
        case 'y':
            strcpy( code, C_YELLOW );
            break;
        case 'B':
            strcpy( code, C_B_BLUE );
            break;
        case 'C':
            strcpy( code, C_B_CYAN );
            break;
        case 'G':
            strcpy( code, C_B_GREEN );
            break;
        case 'M':
            strcpy( code, C_B_MAGENTA );
            break;
        case 'R':
            strcpy( code, C_B_RED );
            break;
        case 'W':
            strcpy( code, C_B_WHITE );
            break;
        case 'Y':
            strcpy( code, C_B_YELLOW );
            break;
        case 'D':
            strcpy( code, C_D_GREY );
            break;
        case '*':
            snprintf( code, sizeof(code), "%c", 007 );
            break;
            /* removed Sept 98 by Q, due to abuse by mortals
               case '/':
               snprintf( code, sizeof(code), "%c", 012 );
               break;
             */
        case '{':
            snprintf( code, sizeof(code), "%c", '{' );
            break;
        case '+':
            strcpy( code, BOLD );
            break;
        case 'v':
            strcpy( code, REVERSE );
            break;
        case '%':
            strcpy( code, BLINK );
            break;
    }

    p = code;
    while( *p != '\0' )
    {
        *string = *p++;
        *++string = '\0';
    }

    return( strlen( code ) );
}

void colourconv( char *buffer, const char *txt, CHAR_DATA *ch )
{
    const  char    *point;
    int skip = 0;

    if( ch->desc && txt )
    {
        if( IS_SET( ch->act, PLR_COLOUR ) )
        {
            for( point = txt ; *point ; point++ )
            {
                if( *point == '{' )
                {
                    point++;
                    skip = colour( *point, ch, buffer );
                    while( skip-- > 0 )
                        ++buffer;
                    continue;
                }
                *buffer = *point;
                *++buffer = '\0';
            }           
            *buffer = '\0';
        }
        else
        {
            for( point = txt ; *point ; point++ )
            {
                if( *point == '{' )
                {
                    point++;
                    continue;
                }
                *buffer = *point;
                *++buffer = '\0';
            }
            *buffer = '\0';
        }
    }
    return;
}

/* returns a string from text where the color flags are removed */
char* remove_color( const char *txt )
{
    int ti, bi; /* txt- and buffer index */
    static char buffer[MSL];

    bi = 0;
    for( ti = 0 ; txt[ti] != '\0'; ti++ )
    {
        if( txt[ti] == '{' )
        {
            ti++;
            continue;
        }
        buffer[bi++] = txt[ti];
    }
    buffer[bi] = '\0';
    return buffer;
}

void logpf (const char * fmt, ...)
{
    char buf [2*MSL];
    va_list args;
    va_start (args, fmt);
    vsnprintf (buf, sizeof(buf), fmt, args);
    va_end (args);

    log_string (buf);
}

/* source: EOD, by John Booth <???> */
void printf_to_char (CHAR_DATA *ch, const char *fmt, ...)
{
    char buf [2*MSL];
    va_list args;
    va_start (args, fmt);
    vsnprintf (buf, sizeof(buf), fmt, args);
    va_end (args);

    send_to_char (buf, ch);
}

/* bugf - a handy little thing - remember to #include stdarg.h */
/* Source: Erwin S. Andreasen */
void bugf (const char * fmt, ...)
{
    char buf [2*MSL];
    va_list args;
    va_start (args, fmt);
    vsnprintf (buf, sizeof(buf), fmt, args);
    va_end (args);

    bug_string(buf);
}

/* Rim 1/99 */
void printf_to_wiznet(CHAR_DATA *ch, OBJ_DATA *obj, long flag, long flag_skip, int min_level, const char *fmt, ...)
{
    char buf [2*MSL];
    va_list args;
    va_start (args, fmt);
    vsnprintf (buf, sizeof(buf), fmt, args);
    va_end (args);

    wiznet (buf, ch, obj, flag, flag_skip, min_level);
}

bool add_buff(BUFFER *buffer, const char *fmt, ...)
{
    char buf [2*MSL];
    va_list args;
    va_start (args, fmt);
    vsnprintf (buf, sizeof(buf), fmt, args);
    va_end (args);
    
    return add_buf(buffer, buf);
}

bool add_buff_pad(BUFFER *buffer, int pad_length, const char *fmt, ...)
{
    char buf [2*MSL];
    int i;
    va_list args;
    va_start (args, fmt);
    vsnprintf (buf, sizeof(buf), fmt, args);
    va_end (args);
    // pad
    for ( i = strlen_color(buf); i < pad_length; i++ )
        strcat( buf, " " );
    
    return add_buf(buffer, buf);
}

#define CH(descriptor)  ((descriptor)->original ? (descriptor)->original : (descriptor)->character)

/* This file holds the copyover data */
#define COPYOVER_FILE "copyover.data"

/* This is the executable file */
#define EXE_FILE	  "../../bin/aeaea"

/*  Copyover - Original idea: Fusion of MUD++
 *  Adapted to Diku by Erwin S. Andreasen, <erwin@pip.dknet.dk>
 *  http://pip.dknet.dk/~pip1773
 *  Changed into a ROM patch after seeing the 100th request for it :)
 */
static void copyover_mud( const char *argument )
{
    FILE *fp;
    DESCRIPTOR_DATA *d, *d_next;
    char buf [200];
    char arg0[50], arg1[10], arg2[10], arg3[10];


    fp = fopen (COPYOVER_FILE, "w");

    if (!fp)
    {
        bugf("Copyover file not writeable, aborted.");
        logpf ("Could not write to copyover file: %s", COPYOVER_FILE);
        log_error ("copyover_mud:fopen");
        return;
    }

    //do_asave (NULL, ""); /* autosave changed areas */

    if ( argument[0] != '\0' )
        snprintf( buf, sizeof(buf), "\n\r%s", argument );
    else
        strcpy( buf, "" );

    strcat (buf, "\n\r\n\rThe world slows down around you as it fades from your vision.\n\r");
    strcat (buf, "\n\rAs if in a bizarre waking dream, you lurch forward into the darkness...\n\r");


    /* Loop through connected players, ensure quests aren't snarfed */
    for (d = descriptor_list; d; d = d_next)
    {
        CHAR_DATA *curr;
        curr = d->original ? d->original : d->character;
        d_next = d->next;	/* Nice and safe, we go.. */
        if( curr && curr->pcdata && curr->pcdata->countdown != 0 )
        {
            curr->pcdata->countdown = 0;
            curr->pcdata->nextquest = 0;
        }
    }

    final_save_all();
    
    /* For each playing descriptor, save its state */
    for (d = descriptor_list; d ; d = d_next)
    {
        CHAR_DATA * och = CH (d);
        d_next = d->next; /* We delete from the list , so need to save this */

        if (!d->character || (!IS_PLAYING(d->connected) )) /* drop those logging on */
        {
            write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0, d->pProtocol->bSGA);
            close_socket (d); /* throw'em out */
        }
        else
        {
            fprintf (fp, "%d %s %s %s\n", d->descriptor, och->name, d->host, CopyoverGet(d) );

            write_to_descriptor (d->descriptor, buf, 0, d->pProtocol->bSGA);
        }
    }

    if (DXPORT_status() == eDXPORT_OPENED)
    {
        DXPORT_close();
    }

    fprintf (fp, "-1\n");
    fclose (fp);

    /* exec - descriptors are inherited */

    snprintf (arg0, sizeof(arg0), "%s", "aeaea");
    snprintf (arg1, sizeof(arg1), "%d", s_port);
    snprintf (arg2, sizeof(arg2), "%s", "copyover");
    snprintf (arg3, sizeof(arg3), "%d", s_control);
    logpf( "copyover_mud: executing '%s %s %s %s'", arg0, arg1, arg2, arg3 );
    execl (EXE_FILE, arg0, arg1, arg2, arg3, (char *) NULL);

    /* Failed - sucessful exec will not return */

    log_error ("copyover_mud: execl");
    bugf("Copyover FAILED!");
}

static TIMER_NODE *copyover_timer=NULL;
static int copyover_countdown=0;

void handle_copyover_timer( void )
{
    copyover_timer=NULL;
    
    if (copyover_countdown<1)
    {
        copyover_mud("");
        return;
    }
    
    /* gecho the stuff */
    DESCRIPTOR_DATA *d;
    for ( d=descriptor_list; d; d=d->next )
    {
        if ( IS_PLAYING(d->connected ) )
        {
            ptc( d->character, "{R!!!{WA copyover will occur in %d seconds{R!!!{x\n\r", copyover_countdown);
        }
    }  
  
    int delay;
    if (copyover_countdown <= 3)
    {
        delay = 1;
    }
    else if (copyover_countdown <=10)
    {
        /* target 3 seconds */
        delay = copyover_countdown-3;
    }
    else if (copyover_countdown <= 30)
    {
        /* target 10 seconds */
        delay = copyover_countdown-10;
    }
    else if (copyover_countdown <= 60)
    {
        /* target 30 seconds */
        delay = copyover_countdown-30;
    }
    else if (copyover_countdown <= 120)
    {
        /* target 60 seconds */
        delay = copyover_countdown-60;
    }
    else
    {
        /* target 120 seconds */
        delay = copyover_countdown-120;
    }

    copyover_countdown -= delay;
    copyover_timer = register_c_timer( delay, handle_copyover_timer);
    return;
}

DEF_DO_FUN( do_copyover )
{
    if (argument[0]=='\0')
    {
        ptc(ch, "%s\n\rPlease confirm, do you want to copyover?\n\r",
            bin_info_string);

        confirm_yes_no( ch->desc,
            do_copyover,
            "confirm",
            NULL,
            NULL);
        return;
    }
    else if (!strcmp(argument, "confirm"))
    {
        copyover_mud("");
        return;
    }
    else if (!strcmp(argument, "cancel"))
    {
        if (!copyover_timer)
        {
            send_to_char( "There is no pending copyover.\n\r", ch );
            return;
        }

        unregister_timer_node( copyover_timer );
        copyover_timer=NULL;

        DESCRIPTOR_DATA *d;
        for ( d=descriptor_list; d; d=d->next )
        {
            if ( IS_PLAYING(d->connected ) )
            {
                ptc( d->character, "{R!!!{WThe copyover has been cancelled{R!!!{x\n\r");
            }
        }
        
    }
    else if (is_number(argument))
    {
        copyover_countdown=atoi(argument);
        handle_copyover_timer();
        return;
    }
}

/* Recover from a copyover - load players */
void copyover_recover ( void )
{
    DESCRIPTOR_DATA *d;
    FILE *fp;
    char name [100];
    char host[MSL];
    char protocol[MSL];
    int desc;
    bool fOld;

    logpf ("Copyover recovery initiated");

    fp = fopen (COPYOVER_FILE, "r");

    bounty_table = NULL;

    if (!fp) /* there are some descriptors open which will hang forever then ? */
    {
        log_error ("copyover_recover:fopen");
        logpf ("Copyover file not found. Exiting.\n\r");
        exit (1);
    }

    unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading	*/

    for (;;)
    {
        int matched = fscanf (fp, "%d %s %s %s\n", &desc, name, host, protocol );
        if ( matched < 4 )
            logpf("copyover_recover: fscanf returned %d", matched);

        if (desc == -1)
            break;

        /* Write something, and check if it goes error-free */		
        if (!write_to_descriptor (desc, "\n\ra light at the end of the tunnel...\n\r", 0, FALSE))
        {
            close (desc); /* nope */
            continue;
        }

        d = new_descriptor();
        d->descriptor = desc;

        d->host = str_dup (host);
        CopyoverSet( d, protocol);
        /*
           d->next = descriptor_list;
           descriptor_list = d;
         */
        add_descriptor( d );

        d->connected = CON_COPYOVER_RECOVER; /* -15, so close_socket frees the char */


        /* Now, find the pfile */

        fOld = load_char_obj (d, name, FALSE);

        if (!fOld) /* Player file not found?! */
        {
            write_to_descriptor (desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r", 0, d->pProtocol->bSGA);
            close_socket (d);			
        }
        else /* ok! */
        {
            write_to_descriptor (desc, "\n\rand reality resumes around you, as if nothing had ever happened.\n\r",0, d->pProtocol->bSGA);

            if ( d && d->pProtocol && d->character)
            {
                if (IS_SET(d->character->act, PLR_COLOUR))
                {
                    d->pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt = 1;
                }
                else
                {
                    d->pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt = 0;   
                }
            }

            /* Just In Case */
            if (!d->character->in_room)
                d->character->in_room = get_room_index (ROOM_VNUM_TEMPLE);

            /* Insert in the char_list */
            char_list_insert(d->character);

            char_to_room (d->character, d->character->in_room);
            do_look (d->character, "auto");
            act ("$n materializes!", d->character, NULL, NULL, TO_ROOM);
            d->connected = CON_PLAYING;
            update_bounty(d->character);
            reset_char(d->character);

            if (d->character->pet != NULL)
            {
                char_to_room(d->character->pet,d->character->in_room);
                act("$n materializes!.",d->character->pet,NULL,NULL,TO_ROOM);
            }

            /* Auth list status now updates correctly after a copyover. */
            /* Also, authorizing a character who was online during a */
            /* copyover, after the copyover, now works correctly. */
            /* Jan 14, 2006 - Elik */
            /* This does have the side effects of repeating, for instance, */
            /* the name change message, and such.  The option, I guess, is */
            /* to rip some guts out of check_auth_state and put them here, */
            /* mostly the check for the character having been offline,     */
            /* but I'd rather have a small function in auth that did that, */
            /* and call it from check_auth_state, and from here, than      */
            /* spread code around. */
            check_auth_state(d->character);
        }

    }
    fclose (fp);
}

/* returns the original char for switched chars */
CHAR_DATA* original_char( CHAR_DATA *ch )
{
    if ( ch == NULL )
    {
        bug( "original_char: NULL pointer given", 0 );
        return NULL;
    }

    if ( ch->desc != NULL && ch->desc->original != NULL )
        return ch->desc->original;
    else
        return ch;
}

/* returns whether two chars have the same IP */
bool is_same_player( CHAR_DATA *ch1, CHAR_DATA *ch2 )
{
    if ( ch1 == NULL || ch2 == NULL
            || ch1->desc == NULL || ch2->desc == NULL )
        return FALSE;

    if ( !strcmp(ch1->desc->host, ch2->desc->host) )
        return TRUE;
    else
        return FALSE;
}

/* adds a descriptor to descriptor_list, sorted by IP */
void add_descriptor( DESCRIPTOR_DATA *d )
{
    DESCRIPTOR_DATA *last_d;

    if ( descriptor_list == NULL || desc_cmp(d, descriptor_list) )
    {
        d->next = descriptor_list;
        descriptor_list = d;
        return;
    }

    last_d = descriptor_list;
    /* INV: last_d < d */
    while ( last_d->next != NULL && !desc_cmp(d, last_d->next) )
        last_d = last_d->next;
    /* last_d < d <= last_d->next */
    d->next = last_d->next;
    last_d->next = d;
}

/* returns if d1 `<=` d2 */
bool desc_cmp( DESCRIPTOR_DATA *d1, DESCRIPTOR_DATA *d2 )
{
    if ( d1 == NULL || d2 == NULL
            || d1->host == NULL || d2->host == NULL )
    {
        bugf( "desc_cmp: NULL descriptor or host" );
        return FALSE;
    }

    return strcmp( d1->host, d2->host ) <= 0;
}


/* Stuff for catching the last command in case of a crash
 * original code by Erwin S. Andreasen, erwin@andreasen.org
 */

/* Write last command */
void write_last_command ( void )
{
    static bool log_done = FALSE;

    if ( log_done )
        return;
    else
        log_done = TRUE;

    if ( last_command[0] != '\0' )
        log_string( last_command );

    /* catch other debug info too */
    if ( last_mprog[0] != '\0' )
        log_string( last_mprog );

    if ( last_debug[0] != '\0' )
        log_string( last_debug );
}

void nasty_signal_handler (int no)
{
    static bool log_done = FALSE;

    if ( log_done )
        return;
    else
        log_done = TRUE;

    switch ( no )
    {
        case SIGSEGV: log_string( "segmentation error" );break;
        case SIGFPE: log_string( "floating point exception" );break;
        case SIGBUS: log_string( "SIGBUS" );break;
                     /*case ALLOCMEM: log_string( "ALLOCMEM" );break;*/
        case SIGABRT: log_string( "SIGABRT" );break;
        case SIGILL: log_string( "SIGILL" );break;
        case SIGINT: log_string( "SIGINT" );break;
        default: logpf( "unknown error: %d", no ); break;
    }

    write_last_command();

    pid_t forkpid=fork();
    if (forkpid>0)
    {
        /* wait for forked process to exit */
        waitpid(forkpid, NULL, 0);
        /* try to catch things with a copyover */
        copyover_mud ( "system error: trying to recover with copyover" );
        exit(0);
    }
    else
    {
        /* forked process */
        abort(); /* make the core */
    }
}

/* Call this before starting the game_loop */
void install_other_handlers ( void )
{
    last_command[0] = '\0';
    last_mprog[0] = '\0';
    last_debug[0] = '\0';

    if (atexit (write_last_command) != 0)
    {
        perror ("install_other_handlers:atexit");
        exit (1);
    }

    // make sure we don't use BSD style of signal handling..
    static struct sigaction act;
    act.sa_handler = nasty_signal_handler;
    //act.sa_mask = ???;
    act.sa_flags = SA_NODEFER;

    sigaction (SIGSEGV, &act, NULL);
    sigaction (SIGFPE, &act, NULL);
    sigaction (SIGBUS, &act, NULL);
    sigaction (SIGABRT, &act, NULL);
    sigaction (SIGILL, &act, NULL);
    sigaction (SIGINT, &act, NULL);
    /* should probably check return code here */
    signal (SIGSEGV, nasty_signal_handler);

    /* Possibly other signals could be caught? */
    signal (SIGFPE, nasty_signal_handler);
    signal (SIGBUS, nasty_signal_handler);
    /*signal (ALLOCMEM, nasty_signal_handler);*/
    signal (SIGABRT, nasty_signal_handler);
    signal (SIGILL, nasty_signal_handler);
    signal (SIGINT, nasty_signal_handler);
}

const char *bin_info_string=
"\n\r"
#ifdef MKTIME
"Make time: "
MKTIME
"\n\r"
#endif
#ifdef BRANCH
"Branch: "
BRANCH
"\n\r"
#endif
#ifdef PARENT
"Parent: "
PARENT
"\n\r"
#endif
"Game modes defined:\n\r"
#ifdef TESTER
"TESTER defined\n\r"
#endif
#ifdef REMORT
"REMORT defined\n\r"
#endif
#ifdef BUILDER
"BUILDER defined\n\r"
#endif
;

void final_save_all( void )
{
    final_player_save();
    save_lboards();
    save_comm_histories();
}
