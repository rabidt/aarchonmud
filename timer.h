#ifndef TIMER_H
#define TIMER_H
typedef void TIMER_CFUN ( void ); 
typedef struct timer_node TIMER_NODE;
struct char_data;
struct obj_data;
struct area_data;
struct room_index_data;

char* print_timer_list();
TIMER_NODE * register_c_timer( int value, TIMER_CFUN func );
TIMER_NODE * register_lua_timer( int value, const char *tag);
/*bool*/ unsigned char unregister_lua_timer( TIMER_NODE *tmr, const char *tag);
TIMER_NODE * register_ch_timer( struct char_data *ch, int max );
TIMER_NODE * register_obj_timer( struct obj_data *obj, int max );
TIMER_NODE * register_area_timer( struct area_data *area, int max );
TIMER_NODE * register_room_timer( struct room_index_data *room, int max );
void unregister_ch_timer( struct char_data *ch );
void unregister_obj_timer( struct obj_data *obj );
void unregister_timer_node( TIMER_NODE *tmr );
void timer_update();

#endif
