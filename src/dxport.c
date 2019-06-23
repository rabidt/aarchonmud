#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>
#include "dxport.h"


void    logpf( const char *fmt, ... );
void    bugf( const char *fmt, ... );


static const char *SOCK_PATH = ".DXPORT";

static const char MSG_START = '\x2';
static const char PARAM_DELIM = '\x1e';
static const char MSG_END = '\x3';

#define SOCKFD_ENV "DXPORT_sockfd"


static int sockfd = 0;
static void set_sockfd(int fd)
{
    sockfd = fd;
    if (0 == fd)
    {
        int rc;
        if (0 != (rc = unsetenv(SOCKFD_ENV)))
        {
            bugf("unsetenv result: %d", rc);
        }
    }
    else
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", fd);
        int rc;
        if (0 != (rc = setenv(SOCKFD_ENV, buf, 1)))
        {
            bugf("setenv result: %d", rc);
        }
    }
}

#define BUF_SIZE 1024
static char msg_buf[BUF_SIZE];


eDXPORT_rc write_msg_buf( void )
{
    if (sockfd == 0)
        return eDXPORT_CLOSED;
    //struct timeval t1, t2;
    //double elapsedTime;    
    //gettimeofday(&t1, NULL);
    size_t len = strlen(msg_buf);
    int written = write(sockfd, msg_buf, len);
    //gettimeofday(&t2, NULL);
    //elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    //elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
    //printf("Elapsed: %d\n", elapsedTime);


    if (written < 0 || len != (size_t)written)
    {
        int rtn = close(sockfd);
        if (rtn != 0)
        {
            bugf("close returned %d", rtn);
        }
        set_sockfd(0);
        return eDXPORT_CLOSED;
    }
    else
    {
        return eDXPORT_SUCCESS;
    }
}

eDXPORT_rc DXPORT_reload( void )
{
    snprintf(msg_buf, BUF_SIZE, "%c%s%c",
            MSG_START,
            "reload",
            MSG_END);
    return write_msg_buf();
}

eDXPORT_rc DXPORT_mob_kill(const char *player_name, int mob_vnum, int mob_room, time_t timestamp)
{
    snprintf(msg_buf, BUF_SIZE, "%c%s%c%s%c%d%c%d%c%d%c",
            MSG_START,
            "mob_kill",
            PARAM_DELIM,
            player_name,
            PARAM_DELIM,
            mob_vnum,
            PARAM_DELIM,
            mob_room,
            PARAM_DELIM,
            (int)timestamp,
            MSG_END);
    return write_msg_buf();
}


eDXPORT_rc DXPORT_player_connect(const char *player_name, const char *ip, time_t timestamp)
{
    snprintf(msg_buf, BUF_SIZE, "%c%s%c%s%c%s%c%d%c",
            MSG_START,
            "player_connect",
            PARAM_DELIM,
            player_name,
            PARAM_DELIM,
            ip,
            PARAM_DELIM,
            (int)timestamp,
            MSG_END);
    return write_msg_buf();
}


eDXPORT_rc DXPORT_quest_request(
    const char *player_name,
    long quest_id,
    bool is_hard,
    int giver_vnum,
    int obj_vnum,
    int mob_vnum,
    int room_vnum)
{
    snprintf(msg_buf, BUF_SIZE, "%c%s%c%s%c%ld%c%d%c%d%c%d%c%d%c%d%c",
            MSG_START,
            "quest_request",
            PARAM_DELIM,
            player_name,
            PARAM_DELIM,
            quest_id,
            PARAM_DELIM,
            (int)is_hard,
            PARAM_DELIM,
            giver_vnum,
            PARAM_DELIM,
            obj_vnum,
            PARAM_DELIM,
            mob_vnum,
            PARAM_DELIM,
            room_vnum,
            MSG_END);
    return write_msg_buf();
}

eDXPORT_rc DXPORT_quest_complete(
    const char *player_name,
    long quest_id,
    long end_time,
    int completer_vnum,
    int silver,
    int qp,
    int prac,
    int exp)
{
    snprintf(msg_buf, BUF_SIZE, "%c%s%c%s%c%ld%c%ld%c%d%c%d%c%d%c%d%c%d%c",
        MSG_START,
        "quest_complete",
        PARAM_DELIM,
        player_name,
        PARAM_DELIM,
        quest_id,
        PARAM_DELIM,
        end_time,
        PARAM_DELIM,
        completer_vnum,
        PARAM_DELIM,
        silver,
        PARAM_DELIM,
        qp,
        PARAM_DELIM,
        prac,
        PARAM_DELIM,
        exp,
        MSG_END);
    return write_msg_buf();
}

eDXPORT_rc DXPORT_quest_timeout(
    const char *player_name,
    long quest_id,
    long end_time)
{
    snprintf(msg_buf, BUF_SIZE, "%c%s%c%s%c%ld%c%ld%c",
        MSG_START,
        "quest_timeout",
        PARAM_DELIM,
        player_name,
        PARAM_DELIM,
        quest_id,
        PARAM_DELIM,
        end_time,
        MSG_END);
    return write_msg_buf();
}

eDXPORT_rc DXPORT_quest_giveup(
    const char *player_name,
    long quest_id,
    long end_time)
{
    snprintf(msg_buf, BUF_SIZE, "%c%s%c%s%c%ld%c%ld%c",
        MSG_START,
        "quest_giveup",
        PARAM_DELIM,
        player_name,
        PARAM_DELIM,
        quest_id,
        PARAM_DELIM,
        end_time,
        MSG_END);
    return write_msg_buf();
}

eDXPORT_rc DXPORT_init( void )
{
    {
        // copyover case. Connection is still open, just need to know the fd number.
        const char *fd_str = getenv(SOCKFD_ENV);
        if (fd_str != NULL)
        {
            sockfd = atoi(fd_str);
            return eDXPORT_SUCCESS;
        }
    }

    struct sockaddr_un server;
    int sock;
    size_t size;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return eDXPORT_CLOSED;
    }

    server.sun_family = AF_UNIX;
    strncpy(server.sun_path, SOCK_PATH, sizeof(server.sun_path));

    size = SUN_LEN(&server);

    if (connect(sock, (struct sockaddr *) &server, size) < 0)
    {
        return eDXPORT_CLOSED;
    }
    else
    {
        set_sockfd(sock);
        return eDXPORT_SUCCESS;
    }
}


eDXPORT_rc DXPORT_close( void )
{
    if (sockfd == 0)
    {
        return eDXPORT_CLOSED;
    }

    int rtn = close(sockfd);
    if (rtn != 0)
    {
        bugf("close returned %d", rtn);
    }

    set_sockfd(0);

    return eDXPORT_SUCCESS;
}


eDXPORT_rc DXPORT_status( void )
{
    return (sockfd == 0) ? eDXPORT_CLOSED : eDXPORT_OPENED;
}
