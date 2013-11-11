#ifndef LUA_HELP_H
#define LUA_HELP_H
typedef struct luahelp_topic
{
    char *key;
    char *summary;
    char *arguments;
    char *syntax;
    char *notes;
} LUAHELP_TOPIC;

struct char_data;
void add_topic( const char *section, LUAHELP_TOPIC *topic );
void do_luahelp( struct char_data *ch, const char *argument ); 
#endif
