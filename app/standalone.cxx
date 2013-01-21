#include <string>
#include <iostream>
#include <cassert>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

extern "C"
{
	int LUA_API luaopen_cairo(lua_State *State);
}

int main(int ArgumentCount, char **Arguments)
{
	lua_State *State = luaL_newstate();
	assert(lua_gettop(State) == 0);
	try
	{
		if (ArgumentCount < 2)
			throw std::string("You must specify a Lua script as the first argument.");

		if (State == nullptr) throw std::string("Failed to create Lua state.");
		luaL_openlibs(State);
		assert(lua_gettop(State) == 0);

		luaL_requiref(State, "cairo", luaopen_cairo, true);
		lua_pop(State, 1);
		assert(lua_gettop(State) == 0);

		// Set arguments table
#ifndef NDEBUG
		unsigned int const InitialHeight = lua_gettop(State);
#endif
		lua_newtable(State);
		for (unsigned int CurrentArgument = 2; CurrentArgument < (unsigned int)ArgumentCount; ++CurrentArgument)
		{
			lua_pushstring(State, Arguments[CurrentArgument]);
			lua_rawseti(State, -2, CurrentArgument + 1);
		}
		lua_setglobal(State, "arg");
#ifndef NDEBUG
		assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif

		// Load and run script
		assert(lua_gettop(State) == 0);
		lua_getglobal(State, "debug");
		lua_getfield(State, -1, "traceback");
		lua_remove(State, -2);

		int LoadError = luaL_loadfile(State, Arguments[1]);
		if (LoadError != LUA_OK)
			throw std::string("Unable to open script file; Error was:\n\n") + lua_tostring(State, -1);

		assert(lua_isfunction(State, 1));
		int Result = lua_pcall(State, 0, 0, 1);
		if (Result != LUA_OK)
			throw std::string("Error while running script; Error was:\n\n") + lua_tostring(State, -1);
	}
	catch (std::string &Error)
	{
		std::cerr << "Fatal Error: " << Error << std::endl;
		lua_close(State);
		return 1;
	}
	lua_close(State);
	return 0;
}

