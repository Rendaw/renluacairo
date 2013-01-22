#ifndef library_h
#define library_h

#include <cstring>
#include <string>
#include <typeinfo>
#include <cassert>
#include <iostream>

#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

// Various
static_assert(sizeof(void (*)(void)) == sizeof(void *), "Function and data pointers are of different sizes.  We need to store function pointers as data pointers in Lua.");
template <typename Type> void *ToVoidPointer(Type In) 
{
	static_assert(sizeof(Type) == sizeof(void *), "Unsafe pointer conversion.");
	void *Out; 
	*reinterpret_cast<Type *>(&Out) = In;
	return Out; 
}

template <typename Type> Type FromVoidPointer(void *In) 
{ 
	static_assert(sizeof(Type) == sizeof(void *), "Unsafe pointer conversion.");
	Type Out; 
	*reinterpret_cast<void **>(&Out) = In;
	return Out; 
}

template <typename Type> size_t TypeIDLength(void)
{
	static const size_t Length = strlen(typeid(Type).name());
	return Length;
}

//-- Enum registration
void RegisterEnum(lua_State *State, std::string const &Name, std::initializer_list<std::pair<std::string, int> > Values)
{
#ifndef NDEBUG
	lua_pushstring(State, Name.c_str());
	lua_gettable(State, -2);
	assert(lua_isnil(State, -1));
	lua_pop(State, 1);
	unsigned int const InitialHeight = lua_gettop(State);
#endif
	lua_pushstring(State, Name.c_str());
	lua_createtable(State, 0, Values.size());
	for (auto const &Value : Values)
	{
		lua_pushstring(State, Value.first.c_str());
		lua_pushnumber(State, Value.second);
		lua_settable(State, -3);
	}
	lua_settable(State, -3);
#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
}

//-- Metatable tools
template <typename PopulatorType> void CreateMetatable(lua_State *State, void *TypeUID, PopulatorType const &Populator)
{
#ifndef NDEBUG
	unsigned int const InitialHeight = lua_gettop(State);
#endif
	// Create method table
	lua_newtable(State);
	Populator();

	// Create metatable that points to method table
	lua_pushlightuserdata(State, TypeUID);
	lua_newtable(State);
	lua_pushstring(State, "__index");
	lua_pushvalue(State, -4);
	lua_settable(State, -3);
	lua_settable(State, LUA_REGISTRYINDEX);

	// Pop extra reference to method table
	lua_pop(State, 1);
#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
}

inline void SetMetatable(lua_State *State, void *TypeUID)
{
#ifndef NDEBUG
	unsigned int InitialHeight = lua_gettop(State);
#endif
	lua_pushlightuserdata(State, TypeUID);
	lua_gettable(State, LUA_REGISTRYINDEX);
	assert(!lua_isnil(State, -1)); // No metatable for this UUID
	lua_setmetatable(State, -2);
#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
}

inline void SetMetatableGarbageCollector(lua_State *State, void *TypeUID, lua_CFunction Function)
{
#ifndef NDEBUG
	unsigned int InitialHeight = lua_gettop(State);
#endif
	lua_pushlightuserdata(State, TypeUID);
	lua_gettable(State, LUA_REGISTRYINDEX);
	assert(!lua_isnil(State, -1)); // No metatable for this UUID

	lua_pushstring(State, "__gc");
	lua_pushcfunction(State, Function);
	lua_settable(State, -3);

	lua_pop(State, 1);
#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
}

//-- Templatized Lua stack IO
template <typename Type> struct LuaValue
{
	// Should handle ints and enums.  Unrecognized types will also get mapped this way.
	static Type Read(lua_State *State, unsigned int Position)
	{
		// No specialized read function implemented for this type.
		if (!lua_isnumber(State, Position)) 
			luaL_error(State, "Parameter %d must be of type \"%s\", but it is a \"%s\".", Position, typeid(Type).name(), lua_typename(State, lua_type(State, Position)));
		return (Type)lua_tonumber(State, Position);
	}

	static void Write(lua_State *State, void *, Type const &Value)
	{
		// No specialized write function implemented for this type.
	       	lua_pushinteger(State, Value); 
	}
};

template <> struct LuaValue<double>
{
	static double Read(lua_State *State, unsigned int Position)
	{
		if (!lua_isnumber(State, Position)) 
			luaL_error(State, "Parameter %d must be a double, but it was not.", Position);
		return lua_tonumber(State, Position);
	}

	static void Write(lua_State *State, void *, double const &Value)
		{ lua_pushnumber(State, Value); }
};

template <> struct LuaValue<char *>
{
	static char const *Read(lua_State *State, unsigned int Position)
	{
		if (!lua_isstring(State, Position)) 
			luaL_error(State, "Parameter %d must be a string, but it was not.", Position);
		return lua_tostring(State, Position);
	}

	static void Write(lua_State *State, void *, char * const &Value)
		{ lua_pushstring(State, Value); }
};

template <> struct LuaValue<char const *>
{
	static char const *Read(lua_State *State, unsigned int Position)
	{
		if (!lua_isstring(State, Position)) 
			luaL_error(State, "Parameter %d must be a string, but it was not.", Position);
		return lua_tostring(State, Position);
	}

	static void Write(lua_State *State, void *, char const * const &Value)
		{ lua_pushstring(State, Value); }
};

template <typename Type> struct LuaValue<Type *>
{
	static Type *Read(lua_State *State, unsigned int Position)
	{
		try
		{
			if (!lua_istable(State, Position)) 
				throw std::string(lua_typename(State, lua_type(State, Position)));
			lua_pushstring(State, "_type");
			lua_gettable(State, Position);
			if (lua_isnil(State, -1)) throw std::string("table");
			size_t TypeLength;
			lua_tolstring(State, -1, &TypeLength);
			if (TypeLength != TypeIDLength<Type *>()) 
				throw std::string(lua_tostring(State, -1));
			if (strncmp(typeid(Type *).name(), lua_tostring(State, -1), TypeLength) != 0) 
				throw std::string(lua_tostring(State, -1));
			lua_pushstring(State, "_data");
			lua_gettable(State, Position);
			if (!lua_isuserdata(State, -1)) throw std::string("table");
			return reinterpret_cast<Type *>(lua_touserdata(State, -1));
		}
		catch (std::string &Typename)
		{
			luaL_error(State, "Parameter %d must be of type \"%s\", but it was a \"%s\".", Position, typeid(Type *).name(), Typename.c_str());
			return nullptr; // Unreachable
		}
	}

	static void Write(lua_State *State, void *TypeUID, Type *const &Value)
	{
#ifndef NDEBUG
		unsigned int InitialHeight = lua_gettop(State);
#endif
		lua_newtable(State);

		lua_pushstring(State, "_data");
		lua_pushlightuserdata(State, Value);
		lua_settable(State, -3);

		lua_pushstring(State, "_type");
		lua_pushstring(State, typeid(Type *).name());
		lua_settable(State, -3);

		SetMetatable(State, TypeUID);
#ifndef NDEBUG
		assert((unsigned int)lua_gettop(State) == InitialHeight + 1);
		assert(lua_istable(State, -1));
#endif
	}
};

//-- A lua->c function call wrapper
template <typename... Catchall> struct CallWrapper {};
/*template
<
	typename FunctionType,
	typename ReturnType,
	typename... UnreadTypes,
	typename... ReadTypes
> struct Call
{
	static ReturnType Call(FunctionType Function, lua_State *State, int Position, ReadTypes... ReadValues)
	{
		assert(false); // Specializations should be called for all parameter combinations.
	}
};*/

// Specializations with non-void return type
template 
<
	typename FunctionType, 
	typename ReturnType, 
	typename UnreadType, 
	typename... OtherUnreadTypes, 
	typename... ReadTypes
> struct CallWrapper<FunctionType, ReturnType, std::tuple<UnreadType, OtherUnreadTypes...>, std::tuple<ReadTypes...> >
{
	static ReturnType Call(FunctionType Function, lua_State *State, int Position, ReadTypes... ReadValues)
	{
		return CallWrapper<FunctionType, ReturnType, std::tuple<OtherUnreadTypes...>, std::tuple<ReadTypes..., UnreadType> >::Call(
			Function, State, Position + 1, ReadValues..., LuaValue<UnreadType>::Read(State, Position));
	}
};

template
<
	typename FunctionType,
	typename ReturnType,
	typename... ReadTypes
> struct CallWrapper<FunctionType, ReturnType, std::tuple<>, std::tuple<ReadTypes...> >
{
	static ReturnType Call(FunctionType Function, lua_State *, int, ReadTypes... ReadValues)
	{
		return Function(ReadValues...);
	}
};

// Void return specializations
template 
<
	typename FunctionType, 
	typename UnreadType, 
	typename... OtherUnreadTypes, 
	typename... ReadTypes
> struct CallWrapper<FunctionType, void, std::tuple<UnreadType, OtherUnreadTypes...>, std::tuple<ReadTypes...> >
{
	static void Call(FunctionType Function, lua_State *State, int Position, ReadTypes... ReadValues)
	{
		CallWrapper<FunctionType, void, std::tuple<OtherUnreadTypes...>, std::tuple<ReadTypes..., UnreadType> >::Call(
			Function, State, Position + 1, ReadValues..., LuaValue<UnreadType>::Read(State, Position));
	}
};

template
<
	typename FunctionType,
	typename... ReadTypes
> struct CallWrapper<FunctionType, void, std::tuple<>, std::tuple<ReadTypes...> >
{
	static void Call(FunctionType Function, lua_State *, int, ReadTypes... ReadValues)
	{
		Function(ReadValues...);
	}
};

//-- Function registrar
// Implementation
template <typename ReturnType, typename... ArgumentTypes> struct RegistrationCallback
{
	static int Callback(lua_State *State)
	{	
		// Closure data
		// 1 is bound function
		// 2 is an override for return table metadata uid, if applicable (use bound function otherwise)
		typedef ReturnType (*FunctionType)(ArgumentTypes...);
		FunctionType Function = FromVoidPointer<FunctionType>(lua_touserdata(State, lua_upvalueindex(1)));
		ReturnType ReturnValue = CallWrapper<FunctionType, ReturnType, std::tuple<ArgumentTypes...>, std::tuple<> >::Call(Function, State, 1);

		void *TypeUID;
		if (lua_isuserdata(State, lua_upvalueindex(2)))
			TypeUID = lua_touserdata(State, lua_upvalueindex(2));
		else TypeUID = ToVoidPointer(Function);
		LuaValue<ReturnType>::Write(State, TypeUID, ReturnValue);
		return 1;
	}
};

template <typename... ArgumentTypes> struct RegistrationCallback<void, ArgumentTypes...>
{
	static int Callback(lua_State *State)
	{
		typedef void (*FunctionType)(ArgumentTypes...);
		FunctionType Function = FromVoidPointer<FunctionType>(lua_touserdata(State, lua_upvalueindex(1)));
		CallWrapper<FunctionType, void, std::tuple<ArgumentTypes...>, std::tuple<> >::Call(Function, State, 1);
		return 0;
	}
};

// Registration
template <typename ReturnType, typename... ArgumentTypes> void Register(
	lua_State *State, char const *Name, ReturnType (*Function)(ArgumentTypes...), void *ReturnTypeUID = nullptr)
{
	typedef RegistrationCallback<ReturnType, ArgumentTypes...> CallbackType;

#ifndef NDEBUG
	unsigned int const InitialHeight = lua_gettop(State);
#endif
	lua_pushstring(State, Name);
	int ClosureDataCount = 1;
	lua_pushlightuserdata(State, ToVoidPointer(Function));
	if (ReturnTypeUID != nullptr) 
	{
		assert(typeid(ReturnType) != typeid(void));
		lua_pushlightuserdata(State, ReturnTypeUID);
		ClosureDataCount += 1;
	}
	lua_pushcclosure(State, CallbackType::Callback, ClosureDataCount);
	lua_settable(State, -3);
#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
}

#endif

