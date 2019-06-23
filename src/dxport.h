#ifndef DXPORT_H
#define DXPORT_H

#include "booltype.h"

typedef enum eDXPORT_rc
{
    eDXPORT_SUCCESS=0,
    /* statuses */
    eDXPORT_OPENED,
    eDXPORT_CLOSED,
} eDXPORT_rc;

eDXPORT_rc DXPORT_init( void );
eDXPORT_rc DXPORT_close( void );
eDXPORT_rc DXPORT_reload( void );
eDXPORT_rc DXPORT_player_connect(const char *player_name, const char *ip, time_t timestamp);
eDXPORT_rc DXPORT_mob_kill(const char *player_name, int mob_vnum, int mob_room, time_t timestamp);
eDXPORT_rc DXPORT_quest_request(
    const char *player_name,
    long quest_id,
    int player_level,
    bool is_hard,
    int giver_vnum,
    int obj_vnum,
    int mob_vnum,
    int room_vnum);
eDXPORT_rc DXPORT_quest_complete(
    const char *player_name,
    long quest_id,
    long end_time,
    int completer_vnum,
    int silver,
    int qp,
    int prac,
    int exp);
eDXPORT_rc DXPORT_quest_timeout(
    const char *player_name,
    long quest_id,
    long end_time);
eDXPORT_rc DXPORT_quest_giveup(
    const char *player_name,
    long quest_id,
    long end_time);
eDXPORT_rc DXPORT_status( void );


#endif // DXPORT_H