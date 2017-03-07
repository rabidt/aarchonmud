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


static int sockfd = 0;

#define BUF_SIZE 1024
static char msg_buf[BUF_SIZE];


eDXPORT_rc write_msg_buf()
{
    if (sockfd == 0)
        return eDXPORT_CLOSED;
    //struct timeval t1, t2;
    //double elapsedTime;    
    //gettimeofday(&t1, NULL);
    int len = strlen(msg_buf);
    int written = write(sockfd, msg_buf, len);
    //gettimeofday(&t2, NULL);
    //elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    //elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
    //printf("Elapsed: %d\n", elapsedTime);


    if (len != written)
    {
        int rtn = close(sockfd);
        if (rtn != 0)
        {
            bugf("close returned %d", rtn);
        }
        sockfd = 0;
        return eDXPORT_CLOSED;
    }
    else
    {
        return eDXPORT_SUCCESS;
    }
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


eDXPORT_rc DXPORT_init()
{
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
        sockfd = sock;
        return eDXPORT_SUCCESS;
    }
}


eDXPORT_rc DXPORT_close()
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

    sockfd = 0;

    return eDXPORT_SUCCESS;
}


eDXPORT_rc DXPORT_status()
{
    return (sockfd == 0) ? eDXPORT_CLOSED : eDXPORT_OPENED;
}
