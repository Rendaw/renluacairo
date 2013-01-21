#include "library.h"

extern "C"
{
	int LUA_API luaopen_cairo(lua_State *State);
}

LUALIB_API int luaopen_cairo(lua_State *State)
{
	RegisterEverything(State);
	return 1;
}

