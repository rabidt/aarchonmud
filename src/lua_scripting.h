#ifndef LUA_SCRIPTING_H
#define LUA_SCRIPTING_H


void lua_mob_program( lua_State *LS, const char *text, int pvnum, const char *source,
        CHAR_DATA *mob, CHAR_DATA *ch,
        void *arg1, sh_int arg1type,
        void *arg2, sh_int arg2type,
        int trig_type,
        int security );

bool lua_obj_program( lua_State *LS, const char *trigger, int pvnum, const char *source,
        OBJ_DATA *obj, OBJ_DATA *obj2,CHAR_DATA *ch1, CHAR_DATA *ch2,
        int trig_type,
        int security );

bool lua_area_program( lua_State *LS, const char *trigger, int pvnum, const char *source,
        AREA_DATA *area, CHAR_DATA *ch1,
        int trig_type,
        int security );

bool lua_room_program( lua_State *LS, const char *trigger, int pvnum, const char *source,
        ROOM_INDEX_DATA *room, 
        CHAR_DATA *ch1, CHAR_DATA *ch2,
        OBJ_DATA *obj1, OBJ_DATA *obj2,
        const char *text1,
        int trig_type,
        int security);
   
bool lua_load_mprog( lua_State *LS, int vnum, const char *code);
bool lua_load_oprog( lua_State *LS, int vnum, const char *code);
bool lua_load_aprog( lua_State *LS, int vnum, const char *code);
bool lua_load_rprog( lua_State *LS, int vnum, const char *code);

void check_mprog( lua_State *LS, int vnum, const char *code );
void check_oprog( lua_State *LS, int vnum, const char *code );
void check_aprog( lua_State *LS, int vnum, const char *code );
void check_rprog( lua_State *LS, int vnum, const char *code );


#endif
