typedef struct timer_node TIMER_NODE;
void * register_lua_timer( int value);
void * register_CH_timer( CHAR_DATA *ch, int max );
void unregister_CH_timer( CHAR_DATA *ch );
void timer_update();

