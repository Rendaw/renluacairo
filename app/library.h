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
template <typename Base> struct PointerWithoutConst { typedef Base *Type; };
template <typename Base> struct PointerWithoutConst<Base const *> { typedef Base *Type; };

template <typename... Catchall> struct ReverseTuple {};

template <typename Next, typename... Remaining, typename... Done> struct ReverseTuple<std::tuple<Next, Remaining...>, std::tuple<Done...> >
{
	typedef typename ReverseTuple<std::tuple<Remaining...>, std::tuple<Next, Done...> >::Tuple Tuple;
};

template <typename... Done> struct ReverseTuple<std::tuple<>, std::tuple<Done...> >
{
	typedef std::tuple<Done...> Tuple;
};

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

//-- Templatized Lua stack IO
template <typename Type> struct LuaValue
{
	// Should handle ints and enums.  Unrecognized types will also get mapped this way.
	static Type Read(lua_State *State, int Position)
	{
		// No specialized read function implemented for this type.
		if (!lua_isnumber(State, Position)) 
			luaL_error(State, "Parameter %d must be of type \"%s\", but it is a \"%s\".", Position < 0 ? lua_gettop(State) + 1 + Position : Position, typeid(Type).name(), lua_typename(State, lua_type(State, Position)));
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
	static double Read(lua_State *State, int Position)
	{
		if (!lua_isnumber(State, Position)) 
			luaL_error(State, "Parameter %d must be a double, but it is a \"%s\".", Position < 0 ? lua_gettop(State) + 1 + Position : Position, lua_typename(State, lua_type(State, Position)));
		return lua_tonumber(State, Position);
	}

	static void Write(lua_State *State, void *, double const &Value)
		{ lua_pushnumber(State, Value); }
};

template <> struct LuaValue<char *>
{
	static char const *Read(lua_State *State, int Position)
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
	static char const *Read(lua_State *State, int Position)
	{
		if (!lua_isstring(State, Position)) 
			luaL_error(State, "Parameter %d must be a string, but it was not.", Position);
		return lua_tostring(State, Position);
	}

	static void Write(lua_State *State, void *, char const * const &Value)
		{ lua_pushstring(State, Value); }
};

inline void SetMetatable(lua_State *State, void *TypeUID);
template <typename Type> struct LuaValue<Type *>
{
	static Type *Read(lua_State *State, int Position)
	{
		try
		{
			if (!lua_istable(State, Position)) 
				throw std::string(lua_typename(State, lua_type(State, Position)));

#ifndef NDEBUG
			unsigned int InitialHeight = lua_gettop(State);
#endif

			lua_pushstring(State, "_type");
			lua_gettable(State, Position);
			if (lua_isnil(State, -1)) throw std::string("table");
			size_t TypeLength;
			lua_tolstring(State, -1, &TypeLength);
			if (TypeLength == TypeIDLength<Type *>())
			{
				if (strncmp(typeid(Type *).name(), lua_tostring(State, -1), TypeLength) != 0)
					throw std::string(lua_tostring(State, -1));
			}
			else if (TypeLength == TypeIDLength<typename PointerWithoutConst<Type *>::Type>())
			{
				if (strncmp(typeid(typename PointerWithoutConst<Type *>::Type).name(), lua_tostring(State, -1), TypeLength) != 0)
					throw std::string(lua_tostring(State, -1));
			}
			else throw std::string(lua_tostring(State, -1));
			lua_pop(State, 1);

			lua_pushstring(State, "_data");
			lua_gettable(State, Position);
			if (!lua_isuserdata(State, -1)) throw std::string("table");
			Type *Out = reinterpret_cast<Type *>(lua_touserdata(State, -1));
			lua_pop(State, 1);

#ifndef NDEBUG
			assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif

			return Out;
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
		assert(TypeUID != nullptr);
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

template <typename... Catchall> struct DestructorCallback {};

template <typename InputType> struct DestructorCallback<void, InputType>
{
	static int Callback(lua_State *State)
	{
		typedef void (*FunctionType)(InputType);
		FunctionType Function = FromVoidPointer<FunctionType>(lua_touserdata(State, lua_upvalueindex(1)));
		InputType Input = LuaValue<InputType>::Read(State, 1);
		Function(Input);
		return 0;
	}

	static void PushCallback(lua_State *State, void (*Function)(InputType))
	{
		lua_pushlightuserdata(State, ToVoidPointer(Function));
		lua_pushcclosure(State, Callback, 1);
	}
};

template <> struct DestructorCallback<int, lua_State *>
{
	static void PushCallback(lua_State *State, lua_CFunction Function)
	{
		lua_pushcfunction(State, Function);
	}
};

template <typename ReturnType, typename InputType> void SetMetatableGarbageCollector(lua_State *State, void *TypeUID, ReturnType (*Function)(InputType))
{
#ifndef NDEBUG
	unsigned int InitialHeight = lua_gettop(State);
#endif
	lua_pushlightuserdata(State, TypeUID);
	lua_gettable(State, LUA_REGISTRYINDEX);
	assert(!lua_isnil(State, -1)); // No metatable for this UUID

	lua_pushstring(State, "__gc");
	DestructorCallback<ReturnType, InputType>::PushCallback(State, Function);
	lua_settable(State, -3);

	lua_pop(State, 1);
#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
}


//-- Regular function registration
template <typename... Catchall> struct CallWrapper {};

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

template <typename ReturnType, typename... ArgumentTypes> void Register(
	lua_State *State, char const *Name, ReturnType (*Function)(ArgumentTypes...), void *ReturnTypeUID = nullptr)
{
#ifndef NDEBUG
	unsigned int const InitialHeight = lua_gettop(State);
#endif
	lua_pushstring(State, Name);
	int ClosureDataCount = 1;
	lua_pushlightuserdata(State, ToVoidPointer(Function));
	if (ReturnTypeUID != nullptr) 
	{
		assert(typeid(ReturnType) != typeid(void));
		// This is used to override the metatable associated with returned objects (or pointers to)
		lua_pushlightuserdata(State, ReturnTypeUID);
		ClosureDataCount += 1;
	}
	lua_pushcclosure(State, RegistrationCallback<ReturnType, ArgumentTypes...>::Callback, ClosureDataCount);
	lua_settable(State, -3);
#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
}

// -- Multiple return value function registration
namespace MultipleReturn
{
	// The outputs must be handled in reverse order so that they are pushed to the lua stack in the correct order.
	// This also lets us optimistically handle input/output functions (pointer arguments used for input and output)
	// -- Arguments are read (if available) in reverse order from the top of the stack and initialize the output storage
	// Note: Output/IO parameters are reversed using the ReverseTuple type above, so all further processing is actually in reverse order (back to front).
	template <bool Initialize, typename... Catchall> struct CallWrapper { };
	
	template <bool Initialize, typename... Catchall> struct RegistrationCallback {};

	// Non-void return specializations
	template 
	<
		bool Initialize,
		typename FunctionType,
		typename ReturnType,
		typename InputType,
		typename UnallocatedType,
		typename... UnallocatedTypes,
		typename... OutputTypes
	> struct CallWrapper<
		Initialize, 
		FunctionType, 
		ReturnType,
		InputType, 
		std::tuple<UnallocatedType *, UnallocatedTypes...>, 
		std::tuple<OutputTypes...> > 
	{
		static unsigned int Call(FunctionType Function, lua_State *State, InputType const &Input, unsigned int Count, OutputTypes... AllocatedPointers)
		{
			UnallocatedType Storage;
			if (Initialize)
			{
				Storage = LuaValue<UnallocatedType>::Read(State, -1);
				lua_pop(State, 1);
			}
			unsigned int Read = CallWrapper<
				Initialize,
				FunctionType, 
				ReturnType,
				InputType,
				std::tuple<UnallocatedTypes...>, 
				std::tuple<UnallocatedType *, OutputTypes...> 
				>::Call(Function, State, Input, Count + 1, &Storage, AllocatedPointers...);
			LuaValue<UnallocatedType>::Write(State, nullptr, Storage);
			return Read;
		}
	};

	template
	<
		bool Initialize,
		typename FunctionType,
		typename ReturnType,
		typename InputType,
		typename... OutputTypes
	> struct CallWrapper<
		Initialize, 
		FunctionType, 
		ReturnType,
		InputType, 
		std::tuple<>, 
		std::tuple<OutputTypes...> >
	{
		static unsigned int Call(FunctionType Function, lua_State *State, InputType const &Input, unsigned int Count, OutputTypes... AllocatedPointers)
		{
			ReturnType Return = Function(Input, AllocatedPointers...);
			lua_settop(State, 0);
			LuaValue<ReturnType>::Write(State, nullptr, Return);
			return 1 + Count;
		}
	};

	template 
	<
		bool Initialize, 
		typename ReturnType,
		typename InputType, 
		typename... UnallocatedTypes, 
		typename... OutputTypes
	> struct RegistrationCallback<Initialize, ReturnType, InputType, std::tuple<UnallocatedTypes...>, std::tuple<OutputTypes...> >
	{
		static int Callback(lua_State *State)
		{
			typedef ReturnType (*FunctionType)(InputType, OutputTypes...);
			FunctionType Function = FromVoidPointer<FunctionType>(lua_touserdata(State, lua_upvalueindex(1)));
			InputType Input = LuaValue<InputType>::Read(State, 1);
			return CallWrapper<
				Initialize, 
				FunctionType, 
				ReturnType, 
				InputType, 
				std::tuple<UnallocatedTypes...>, 
				std::tuple<> >::Call(Function, State, Input, 0);
		}
	};
	

	// Void return type specializations
	template 
	<
		bool Initialize,
		typename FunctionType,
		typename InputType,
		typename UnallocatedType,
		typename... UnallocatedTypes,
		typename... OutputTypes
	> struct CallWrapper<
		Initialize, 
		FunctionType, 
		void,
		InputType, 
		std::tuple<UnallocatedType *, UnallocatedTypes...>, 
		std::tuple<OutputTypes...> > 
	{
		static unsigned int Call(FunctionType Function, lua_State *State, InputType const &Input, unsigned int Count, OutputTypes... AllocatedPointers)
		{
			UnallocatedType Storage;
			if (Initialize)
			{
				Storage = LuaValue<UnallocatedType>::Read(State, -1);
				lua_pop(State, 1);
			}
			unsigned int Read = CallWrapper<
				Initialize,
				FunctionType, 
				void,
				InputType,
				std::tuple<UnallocatedTypes...>, 
				std::tuple<UnallocatedType *, OutputTypes...> 
				>::Call(Function, State, Input, Count + 1, &Storage, AllocatedPointers...);
			LuaValue<UnallocatedType>::Write(State, nullptr, Storage);
			return Read;
		}
	};

	template
	<
		bool Initialize,
		typename FunctionType,
		typename InputType,
		typename... OutputTypes
	> struct CallWrapper<
		Initialize, 
		FunctionType, 
		void,
		InputType, 
		std::tuple<>, 
		std::tuple<OutputTypes...> >
	{
		static unsigned int Call(FunctionType Function, lua_State *State, InputType const &Input, unsigned int Count, OutputTypes... AllocatedPointers)
		{
			Function(Input, AllocatedPointers...);
			lua_settop(State, 0);
			return Count;
		}
	};

	template 
	<
		bool Initialize, 
		typename InputType, 
		typename... UnallocatedTypes, 
		typename... OutputTypes
	> struct RegistrationCallback<Initialize, void, InputType, std::tuple<UnallocatedTypes...>, std::tuple<OutputTypes...> >
	{
		static int Callback(lua_State *State)
		{
			typedef void (*FunctionType)(InputType, OutputTypes...);
			FunctionType Function = FromVoidPointer<FunctionType>(lua_touserdata(State, lua_upvalueindex(1)));
			InputType Input = LuaValue<InputType>::Read(State, 1);
			return CallWrapper<
				Initialize, 
				FunctionType, 
				void, 
				InputType, 
				std::tuple<UnallocatedTypes...>, 
				std::tuple<> >::Call(Function, State, Input, 0);
		}
	};

	template <typename ReturnType, typename InputType, typename... OutputTypes> void Register(
		lua_State *State, char const *Name, ReturnType (*Function)(InputType, OutputTypes...))
	{
		// Registers a function that uses pointers to output multiple values.
		// Assumes:
		// . First argument is an input
		// . Second+ arguments are outputs, using pointers
#ifndef NDEBUG
		unsigned int const InitialHeight = lua_gettop(State);
#endif
		lua_pushstring(State, Name);
		lua_pushlightuserdata(State, ToVoidPointer(Function));
		lua_pushcclosure(State, RegistrationCallback<false, ReturnType, InputType, typename ReverseTuple<std::tuple<OutputTypes...>, std::tuple<> >::Tuple, std::tuple<OutputTypes...> >::Callback, 1);
		lua_settable(State, -3);
#ifndef NDEBUG
		assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
	}
}

namespace InputOutput
{
	template <typename ReturnType, typename InputType, typename... OutputTypes> void Register(
		lua_State *State, char const *Name, ReturnType (*Function)(InputType, OutputTypes...))
	{
		// Registers a function that uses pointers to input and output multiple values.
		// Assumes:
		// . First argument is an input
		// . Second+ arguments are both inputs and outputs, using pointers
#ifndef NDEBUG
		unsigned int const InitialHeight = lua_gettop(State);
#endif
		lua_pushstring(State, Name);
		lua_pushlightuserdata(State, ToVoidPointer(Function));
		lua_pushcclosure(State, ::MultipleReturn::RegistrationCallback<true, ReturnType, InputType, std::tuple<OutputTypes...>, std::tuple<OutputTypes...> >::Callback, 1);
		lua_settable(State, -3);
#ifndef NDEBUG
		assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
	}
}

// Enumeration registration
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

#endif

