#ifndef LUA_SCRIPTING_H
#define LUA_SCRIPTING_H


void unregister_lua( void *ptr );

void lua_mob_program( char *text, int pvnum, char *source,
        CHAR_DATA *mob, CHAR_DATA *ch,
        const void *arg1, sh_int arg1type,
        const void *arg2, sh_int arg2type,
        int trig_type,
        int security );

bool lua_obj_program( char *trigger, int pvnum, char *source,
        OBJ_DATA *obj, OBJ_DATA *obj2,CHAR_DATA *ch1, CHAR_DATA *ch2,
        int trig_type,
        int security );

bool lua_area_program( char *trigger, int pvnum, char *source,
        AREA_DATA *area, CHAR_DATA *ch1,
        int trig_type,
        int security );

   
bool lua_load_mprog( lua_State *LS, int vnum, char *code);
bool lua_load_oprog( lua_State *LS, int vnum, char *code);
bool lua_load_aprog( lua_State *LS, int vnum, char *code);

        
void do_lboard( CHAR_DATA *ch, char *argument);
void do_lhistory( CHAR_DATA *ch, char *argument);
void update_lboard( int lboard_type, CHAR_DATA *ch, int current, int increment );
void save_lboards();
void load_lboards();
void check_lboard_reset();
void do_lua(CHAR_DATA *ch, char *argument);
bool run_lua_interpret( DESCRIPTOR_DATA *d );
void lua_unregister_desc( DESCRIPTOR_DATA *d );
void unregister_lua( void *ptr );
void run_delayed_function( TIMER_NODE *tmr );
void open_lua();
bool run_lua_interpret( DESCRIPTOR_DATA *d);
void do_luai( CHAR_DATA *ch, char *argument);

int GetLuaMemoryUsage();
int GetLuaGameObjectCount();
int GetLuaEnvironmentCount();

extern lua_State *g_mud_LS;
extern bool g_LuaScriptInProgress;
#endif
