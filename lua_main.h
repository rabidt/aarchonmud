#ifndef LUA_MAIN_H
#define LUA_MAIN_H

#define UD_TABLE_NAME "udtbl"
#define ENV_TABLE_NAME "envtbl"
#define INTERP_TABLE_NAME "interptbl"

/* Names of some functions declared on the lua side */
#define REGISTER_UD_FUNCTION "RegisterUd"
#define UNREGISTER_UD_FUNCTION "UnregisterUd"
#define GETSCRIPT_FUNCTION "GetScript"
#define SAVETABLE_FUNCTION "SaveTable"
#define LOADTABLE_FUNCTION "LoadTable"
#define TPRINTSTR_FUNCTION "glob_tprintstr"

#define RUNDELAY_VNUM -1
#define LOADSCRIPT_VNUM 0
#define MAX_LUA_SECURITY 9
double genrand(void);

int L_delay( lua_State *LS);
int L_cancel( lua_State *LS);
const char *check_fstring( lua_State *LS, int index, size_t size);
const char *check_string( lua_State *LS, int index, size_t size);
int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn);
void stackDump (lua_State *LS);
void dump_prog( CHAR_DATA *ch, const char *prog, bool numberlines);


const char *lua_str_dup( const char *str );
void lua_free_string( const char *str );



extern lua_State *g_mud_LS;
extern bool       g_LuaScriptInProgress;
int               g_ScriptSecurity;
int               g_LoopCheckCounter;
#endif
