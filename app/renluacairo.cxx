#include <cstring>
#include <string>
#include <typeinfo>
#include <cassert>

#include <cairo/cairo.h>
extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

// Various
template <typename Type> void TypeIDLength(void)
{
	static const size_t Length = strlen(typeid(Type).name());
	return Length;
}

//-- Metatable tools
template <typename PopulatorType> void CreateMetatable(lua_State *State, void *TypeUID, PopulatorType const &Populator)
{
#ifndef NDEBUG
	unsigned int const InitialHeight = lua_gettop(State);
#endif
	// Create method table
	lua_pushlightuserdata(State, TypeUID);
	lua_newtable(State);
	Populator();

	// Create metatable that points to method table
	lua_newtable(State);
	lua_pushstring(State, "__index");
	lua_pushvalue(State, -3);
	lua_settable(State, -3);
	lua_settable(State, LUA_REGISTRYINDEX);

	// Pop extra reference to method table
	lua_pop(State, 1);
#ifndef NDEBUG
	assert(lua_gettop(State) == InitialHeight);
#endif
}

void SetMetatable(lua_State *State, void *TypeUID)
{
	lua_pushlightuserdata(State, TypeUID);
	lua_gettable(State, LUA_REGISTRYINDEX);
	assert(!lua_isnil(State, -1)); // No metatable for this UUID
	lua_setmetatable(State, -2);
}

//-- Templatized Lua stack IO
template <typename Type> struct LuaValue
{
	Type Read(lua_State *State, unsigned int Position)
		{ assert(false); /* No specialized read function implemented for this type. */ }

	void Write(lua_State *State, void *TypeUID, Type const &Value)
		{ assert(false); /* No specialized write function implemented for this type. */ }
};

template <> struct LuaValue<int>
{
	int Read(lua_State *State, unsigned int Position)
	{
		if (!lua_isnumber(State, Position)) 
			luaL_error(State, "Parameter %d must be an integer, but it was not.", Position);
		return lua_tonumber(State, Position);
	}

	void Write(lua_State *State, void *, int const &Value)
		{ lua_pushinteger(State, Value); }
};

template <typename Type> struct LuaValue<Type *>
{
	Type *Read(lua_State *State, unsigned int Position)
	{
		try
		{
			if (!lua_istable(State, Position)) throw 0;
			lua_pushstring(State, "type");
			lua_gettable(State, Position);
			if (strncmp(typeid(Type).name(), lua_tostring(State, -1), TypeIDLength<Type>()) != 0)
				throw 0;
			lua_pushstring(State, "data");
			lua_gettable(State, Position);
			return lua_touserdata(State, -1);
		}
		catch (...)
		{
			luaL_error(State, "Parameter %d must be of type \"%s\", but it was not.", Position, typeid(Type).name());
			return nullptr; // Unreachable
		}
	}

	void Write(lua_State *State, void *TypeUID, Type *const &Value)
	{
		lua_newtable(State);

		lua_pushlightuserdata(State, Value);
		lua_pushstring(State, "data");
		lua_settable(State, -3);

		lua_pushstring(State, typeid(Type).name());
		lua_pushstring(State, "type");
		lua_settable(State, -3);

		SetMetatable(State, TypeUID);
	}
};

//-- A lua->c function call wrapper
template
<
	typename FunctionType,
	typename ReturnType,
	typename... UnreadTypes,
	typename... ReadTypes
>
ReturnType Call(FunctionType Function, lua_State *State, int Position, ReadTypes... ReadValues)
{
	assert(false); // Specializations should be called for all parameter combinations.
}

// Specializations with non-void return type
template 
<
	typename FunctionType, 
	typename ReturnType, 
	typename UnreadType, 
	typename... OtherUnreadTypes, 
	typename... ReadTypes
> 
ReturnType Call<FunctionType, ReturnType, UnreadType, OtherUnreadTypes..., ReadTypes...>(
	FunctionType Function, lua_State *State, int Position, ReadTypes... ReadValues)
{
	return Call<FunctionType, ReturnType, OtherUnreadTypes..., UnreadType, ReadTypes...>(
		Function, State, Position + 1, Read<UnreadType>(State, Position), ReadValues...);
}

template
<
	typename FunctionType,
	typename ReturnType,
	typename... ReadTypes
>
ReturnType Call<FunctionType, ReturnType, ReadTypes...>(
	FunctionType Function, lua_State *State, int Position, ReadTypes... ReadValues)
{
	return Function(ReadValues);
}

// Void return specializations
template 
<
	typename FunctionType, 
	typename UnreadType, 
	typename... OtherUnreadTypes, 
	typename... ReadTypes
> 
void Call<FunctionType, void, UnreadType, OtherReadTypes..., ReadTypes...>(
	FunctionType Function, lua_State *State, int Position, ReadTypes... ReadValues)
{
	Call<FunctionType, ReturnType, OtherUnreadTypes..., UnreadType, ReadTypes...>(
		Function, State, Position + 1, LuaValue<UnreadType>::Read(State, Position), ReadValues...);
}

template
<
	typename FunctionType,
	typename... ReadTypes
>
void Call<FunctionType, void, ReadTypes...>(
	FunctionType Function, lua_State *State, int Position, ReadTypes... ReadValues)
{
	Function(ReadValues);
}

//-- Function registrar
// Interface
template <typename ReturnType, typename... ArgumentTypes> void Register(
	lua_State *State, char const *Name, ReturnType (*Function)(ArgumentTypes...))
{
	typedef RegistrationCallback<ReturnType (*)(ArgumentTypes...), ReturnType, ArgumentTypes...> CallbackType;
	lua_pushlightuserdata(State, (void *)CallbackType::Callback);
	lua_pushlightuserdata(State, (void *)Function);
	lua_settable(State, LUA_REGISTRYINDEX);
	
	lua_pushstring(State, Name);
	lua_pushcfunction(State, CallbackType::Callback);
	lua_settable(State, -2);
}

// Implementation
template <typename ReturnType, typename... ArgumentTypes> struct RegistrationCallback
{
	static int Callback(lua_State *State)
	{
		typedef ReturnType (*)(ArgumentTypes...) FunctionType;
		lua_pushlightuserdata(State, (void *)Callback);
		lua_gettable(State, LUA_REGISTRYINDEX);
		FunctionType Function = (FunctionType)lua_touserdata(State, -1);
		ReturnType ReturnValue = Call<FunctionType, ReturnType, ArgumentTypes>(Function, State, 1);
		LuaValue<Type>::Write(State, Function, ReturnValue);
		return 1;
	}
};

template <typename... ArgumentTypes> struct RegistrationCallback<void, ArgumentTypes...>
{
	static int Callback(lua_State *State)
	{
		typedef void (*)(ArgumentTypes...) FunctionType;
		lua_pushlightuserdata(State, (void *)Callback);
		lua_gettable(State, LUA_REGISTRYINDEX);
		FunctionType Function = (FunctionType)lua_touserdata(State, -1);
		Call<FunctionType, ReturnType, ArgumentTypes>(Function, State, 1);
		return 0;
	}
};

int main(int ArgumentCount, char **Arguments)
{
	lua_State *State = luaL_newstate();
	try
	{
		if (ArgumentCount < 2)
			throw std::string("You must specify a Lua script as the first argument.");

		if (State == nullptr) throw std::string("Failed to create Lua state.");
		luaL_openlibs(State);
		lua_newtable(State);
#ifndef NDEBUG
		unsigned int NamespaceHeight = lua_gettop(State);
#endif
		
		/*CreateMetatable(State, cairo_image_surface_create, [&](void)
		{
			Register(State, 
		});*/
		Register(State, "imagesurface", cairo_image_surface_create);

		lua_setglobal(State, "cairo");

		// Set arguments table
		lua_newtable(State);
		for (unsigned int CurrentArgument = 2; CurrentArgument < ArgumentCount; CurrentArgument++)
		{
			lua_pushstring(Arguments[CurrentArgument]);
			lua_rawseti(State, -2, CurrentArgument + 1);
		}
		lua_setglobal(State, "arg");

		// Load and run script
		lua_getglobal(Instance, "debug");
		lua_getfield(Instance, -1, "traceback");
		lua_remove(Instance, -2);

		int LoadError = luaL_loadfile(State, Arguments[1]);
		if (LoadError != LUA_OK)
			throw std::string("Unable to open script file; Error was: ") + lua_tostring(State, -1);

		int Result = lua_pcall(State, 0, 0, 1);
		if (Result != LUA_OK)
			throw std::string("Error while running script; Error was: ") + lua_tostring(State, -1);
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

