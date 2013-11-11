#ifndef LUA_COMMON_H
#define LUA_COMMON_H

#define UD_TABLE_NAME "udtbl"
#define ENV_TABLE_NAME "envtbl"
#define INTERP_TABLE_NAME "interptbl"

/* Names of some functions declared on the lua side */
#define REGISTER_UD_FUNCTION "RegisterUd"
#define UNREGISTER_UD_FUNCTION "UnregisterUd"
#define GETSCRIPT_FUNCTION "GetScript"
#define SAVETABLE_FUNCTION "SaveTable"
#define LOADTABLE_FUNCTION "LoadTable"
#define TPRINTSTR_FUNCTION "tprintstr"

#define LOADSCRIPT_VNUM 0

int L_delay( lua_State *LS);
int L_cancel( lua_State *LS);
const char *check_fstring( lua_State *LS, int index);
int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn);
void stackDump (lua_State *LS);
#endif
