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
	lua_pushstring(State, Name.c_str());
	lua_createtable(State, 0, Values.size());
	for (auto const &Value : Values)
	{
		lua_pushstring(State, Value.first.c_str());
		lua_pushnumber(State, Value.second);
		lua_settable(State, -3);
	}
	lua_settable(State, -3);
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
		typedef ReturnType (*FunctionType)(ArgumentTypes...);
		FunctionType Function = FromVoidPointer<FunctionType>(lua_touserdata(State, lua_upvalueindex(1)));
		ReturnType ReturnValue = CallWrapper<FunctionType, ReturnType, std::tuple<ArgumentTypes...>, std::tuple<> >::Call(Function, State, 1);
		LuaValue<ReturnType>::Write(State, ToVoidPointer(Function), ReturnValue);
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
	lua_State *State, char const *Name, ReturnType (*Function)(ArgumentTypes...))
{
	typedef RegistrationCallback<ReturnType, ArgumentTypes...> CallbackType;

#ifndef NDEBUG
	unsigned int const InitialHeight = lua_gettop(State);
#endif
	lua_pushstring(State, Name);
	lua_pushlightuserdata(State, ToVoidPointer(Function));
	lua_pushcclosure(State, CallbackType::Callback, 1);
	lua_settable(State, -3);
#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight);
#endif
}

// Bulk registration
inline void RegisterSurfaceMethods(lua_State *State)
{
	// Perhaps some sort of metatable inheritance should be implemented, but for now I just duplicate the cfunctions in lua for "inheritance".
	//Register(State, "reference", cairo_surface_reference);
	Register(State, "destroy", cairo_surface_destroy);
	Register(State, "status", cairo_surface_status);
	Register(State, "finish", cairo_surface_finish);
	Register(State, "flush", cairo_surface_flush);
	//Register(State, "getdevice", cairo_surface_get_device);
	Register(State, "getfontoptions", cairo_surface_get_font_options);
	Register(State, "getcontent", cairo_surface_get_content);
	Register(State, "markdirty", cairo_surface_mark_dirty);
	Register(State, "markdirtyrectangle", cairo_surface_mark_dirty_rectangle);
	//Register(State, "setdeviceoffset", cairo_surface_set_device_offset); // TODO tuple
	Register(State, "getdeviceoffset", cairo_surface_get_device_offset);
	Register(State, "setfallbackresolution", cairo_surface_set_fallback_resolution);
	//Register(State, "getfallbackresolution", cairo_surface_get_fallback_resolution); // TODO tuple
	Register(State, "gettype", cairo_surface_get_type);
	Register(State, "getreferencecount", cairo_surface_get_reference_count);
	//Register(State, "setuserdata", cairo_surface_set_user_data); // Not useful?
	//Register(State, "getuserdata", cairo_surface_get_user_data); // Not useful?
	Register(State, "copypage", cairo_surface_copy_page);
	Register(State, "showpage", cairo_surface_show_page);
	Register(State, "hasshowtextglyphs", cairo_surface_has_show_text_glyphs);
	//Register(State, "setmimedata", cairo_surface_set_mime_data); // No
	//Register(State, "getmimedata", cairo_surface_get_mime_data); // No
	//Register(State, "supportsmimetype", cairo_surface_supports_mime_type); // ??
	//Register(State, "maptoimage", cairo_surface_map_to_image);
	//Register(State, "unmapimage", cairo_surface_unmap_image); // ??

#ifdef CAIRO_HAS_PNG_FUNCTIONS
	Register(State, "writetopng", cairo_surface_write_to_png);
#endif
}

inline void RegisterPatternMethods(lua_State *State)
{
	//Register(State, "reference", cairo_pattern_reference);
	Register(State, "destroy", cairo_pattern_destroy);
	Register(State, "status", cairo_pattern_status);
	Register(State, "setextend", cairo_pattern_set_extend);
	Register(State, "getextend", cairo_pattern_get_extend);
	Register(State, "setfilter", cairo_pattern_set_filter);
	Register(State, "getfilter", cairo_pattern_get_filter);
	Register(State, "setmatrix", cairo_pattern_set_matrix);
	Register(State, "getmatrix", cairo_pattern_get_matrix);
	Register(State, "gettype", cairo_pattern_get_type);
	Register(State, "getreferencecount", cairo_pattern_get_reference_count);
	//Register(State, "setuserdata", cairo_pattern_set_user_data); // Useful?
	//Register(State, "getuserdata", cairo_pattern_get_user_data); // Useful?
}

inline void RegisterEverything(lua_State *State)
{
#ifndef NDEBUG
	unsigned int const InitialHeight = lua_gettop(State);
#endif
	lua_newtable(State);

	RegisterEnum(State, "format", {
		{"INVALID", CAIRO_FORMAT_INVALID},
		{"ARGB32", CAIRO_FORMAT_ARGB32},
		{"RGB24", CAIRO_FORMAT_RGB24},
		{"A8", CAIRO_FORMAT_A8},
		{"A1", CAIRO_FORMAT_A1},
		{"RGB16565", CAIRO_FORMAT_RGB16_565}
	});

	CreateMetatable(State, ToVoidPointer(cairo_create), [&](void)
	{
		//Register(State, "reference", cairo_reference);
		Register(State, "destroy", cairo_destroy);
		Register(State, "status", cairo_status);
		Register(State, "save", cairo_save);
		Register(State, "restore", cairo_restore);
		//Register(State, "gettarget", cairo_get_target);
		Register(State, "pushgroup", cairo_push_group);
		Register(State, "pushgroupwithcontent", cairo_push_group_with_content);
		//Register(State, "popgroup", cairo_pop_group);
		Register(State, "popgrouptosource", cairo_pop_group_to_source);
		//Register(State, "getgrouptarget", cairo_get_group_target);
		Register(State, "setsourcergb", cairo_set_source_rgb);
		Register(State, "setsourcergba", cairo_set_source_rgba);
		Register(State, "setsource", cairo_set_source);
		Register(State, "setsourcesurface", cairo_set_source_surface);
		//Register(State, "getsource", cairo_get_source);
		Register(State, "setantialias", cairo_set_antialias);
		Register(State, "getantialias", cairo_get_antialias);
		Register(State, "setdash", cairo_set_dash);
		Register(State, "getdashcount", cairo_get_dash_count);
		Register(State, "getdash", cairo_get_dash);
		Register(State, "setfillrule", cairo_set_fill_rule);
		Register(State, "getfillrule", cairo_get_fill_rule);
		Register(State, "setlinecap", cairo_set_line_cap);
		Register(State, "getlinecap", cairo_get_line_cap);
		Register(State, "setlinejoin", cairo_set_line_join);
		Register(State, "getlinejoin", cairo_get_line_join);
		Register(State, "setlinewidth", cairo_set_line_width);
		Register(State, "getlinewidth", cairo_get_line_width);
		Register(State, "setmiterlimit", cairo_set_miter_limit);
		Register(State, "getmiterlimit", cairo_get_miter_limit);
		Register(State, "setoperator", cairo_set_operator);
		Register(State, "getoperator", cairo_get_operator);
		Register(State, "settolerance", cairo_set_tolerance);
		Register(State, "gettolerance", cairo_get_tolerance);
		Register(State, "clip", cairo_clip);
		Register(State, "clippreserve", cairo_clip_preserve);
		//Register(State, "clipextents", cairo_clip_extents); // TODO Make wrapper function which returns tuple
		Register(State, "inclip", cairo_in_clip);
		Register(State, "resetclip", cairo_reset_clip);
		//Register(State, "rectanglelistdestroy", cairo_rectangle_list_destroy);
		//Register(State, "copycliprectanglelist", cairo_copy_clip_rectangle_list);
		Register(State, "fill", cairo_fill);
		Register(State, "fillpreserve", cairo_fill_preserve);
		//Register(State, "fillextents", cairo_fill_extents); // TODO Same as clipextents.
		Register(State, "infill", cairo_in_fill);
		//Register(State, "mask", cairo_mask);
		Register(State, "masksurface", cairo_mask_surface);
		Register(State, "paint", cairo_paint);
		Register(State, "paintwithalpha", cairo_paint_with_alpha);
		Register(State, "stroke", cairo_stroke);
		Register(State, "strokepreserve", cairo_stroke_preserve);
		//Register(State, "strokeextents", cairo_stroke_extents); // TODO Same as clipextents
		Register(State, "instroke", cairo_in_stroke);
		Register(State, "copypage", cairo_copy_page);
		Register(State, "showpage", cairo_show_page);
		Register(State, "getreferencecount", cairo_get_reference_count);
		//Register(State, "setuserdata", cairo_set_user_data); // Is this useful?
		//Register(State, "getuserdata", cairo_get_user_data); // Is this useful?

		// Path methods
		Register(State, "copypath", cairo_copy_path);
		Register(State, "copypathflat", cairo_copy_path_flat);
		Register(State, "pathdestroy", cairo_path_destroy);
		Register(State, "appendpath", cairo_append_path);
		Register(State, "hascurrentpoint", cairo_has_current_point);
		Register(State, "getcurrentpoint", cairo_get_current_point);
		Register(State, "newpath", cairo_new_path);
		Register(State, "newsubpath", cairo_new_sub_path);
		Register(State, "closepath", cairo_close_path);
		Register(State, "arc", cairo_arc);
		Register(State, "arcnegative", cairo_arc_negative);
		Register(State, "curveto", cairo_curve_to);
		Register(State, "lineto", cairo_line_to);
		Register(State, "moveto", cairo_move_to);
		Register(State, "rectangle", cairo_rectangle);
		//Register(State, "glyphpath", cairo_glyph_path);
		Register(State, "textpath", cairo_text_path);
		Register(State, "relcurveto", cairo_rel_curve_to);
		Register(State, "rellineto", cairo_rel_line_to);
		Register(State, "relmoveto", cairo_rel_move_to);
		//Register(State, "pathextents", cairo_path_extents); // TODO Tuple

		// Transformation methods
		Register(State, "translate", cairo_translate);
		Register(State, "scale", cairo_scale);
		Register(State, "rotate", cairo_rotate);
		Register(State, "transform", cairo_transform);
		Register(State, "setmatrix", cairo_set_matrix);
		Register(State, "getmatrix", cairo_get_matrix);
		Register(State, "identitymatrix", cairo_identity_matrix);
		Register(State, "usertodevice", cairo_user_to_device);
		Register(State, "usertodevicedistance", cairo_user_to_device_distance);
		Register(State, "devicetouser", cairo_device_to_user);
		Register(State, "devicetouserdistance", cairo_device_to_user_distance);
	});
	Register(State, "context", cairo_create);

	CreateMetatable(State, ToVoidPointer(cairo_pattern_create_rgb), [&](void)
	{
		RegisterPatternMethods(State);
		//Register(State, "getrgba", cairo_pattern_get_rgba); // TODO tuple
	});
	Register(State, "rgbpattern", cairo_pattern_create_rgb);
	//Register(State, "rgbapattern", cairo_pattern_create_rgba); // TODO metatable reuse
		
	CreateMetatable(State, ToVoidPointer(cairo_pattern_create_linear), [&](void)
	{
		RegisterPatternMethods(State);
		//Register(State, "getlinearpoints", cairo_pattern_get_linear_points); // TODO tuple
		Register(State, "addcolorstoprgb", cairo_pattern_add_color_stop_rgb);
		Register(State, "addcolorstoprgba", cairo_pattern_add_color_stop_rgba);
		//Register(State, "getcolorstopcount", cairo_pattern_get_color_stop_count); // TODO tuple
		//Register(State, "getcolorstoprgba", cairo_pattern_get_color_stop_rgba); // TODO tuple
	});
	Register(State, "linearpattern", cairo_pattern_create_linear);
		
	CreateMetatable(State, ToVoidPointer(cairo_pattern_create_radial), [&](void)
	{
		RegisterPatternMethods(State);
		//Register(State, "getradialcircles", cairo_pattern_get_radial_circles);
		Register(State, "addcolorstoprgb", cairo_pattern_add_color_stop_rgb);
		Register(State, "addcolorstoprgba", cairo_pattern_add_color_stop_rgba);
		//Register(State, "getcolorstopcount", cairo_pattern_get_color_stop_count); // TODO tuple
		//Register(State, "getcolorstoprgba", cairo_pattern_get_color_stop_rgba); // TODO tuple
	});
	Register(State, "radialpattern", cairo_pattern_create_radial);
		
	CreateMetatable(State, ToVoidPointer(cairo_pattern_create_for_surface), [&](void)
	{
		RegisterPatternMethods(State);
		//Register(State, "getsurface", cairo_pattern_get_surface);
	});
	Register(State, "surfacepattern", cairo_pattern_create_for_surface);
		
	/*CreateMetatable(State, ToVoidPointer(cairo_pattern_create_mesh), [&](void)
	{
		RegisterPatternMethods(State);
		Register(State, "beginpatch", cairo_mesh_pattern_begin_patch);
		Register(State, "endpatch", cairo_mesh_pattern_end_patch);
		Register(State, "moveto", cairo_mesh_pattern_move_to);
		Register(State, "lineto", cairo_mesh_pattern_line_to);
		Register(State, "curveto", cairo_mesh_pattern_curve_to);
		Register(State, "setcontrolpoint", cairo_mesh_pattern_set_control_point);
		Register(State, "setcornercolorrgb", cairo_mesh_pattern_set_corner_color_rgb);
		Register(State, "setcornercolorrgba", cairo_mesh_pattern_set_corner_color_rgba);
		//Register(State, "getpatchcount", cairo_mesh_pattern_get_patch_count); // TODO tuple
		Register(State, "getpath", cairo_mesh_pattern_get_path);
		//Register(State, "getcontrolpoint", cairo_mesh_pattern_get_control_point); // TODO tuple
		Register(State, "getcornercolorrgba", cairo_mesh_pattern_get_corner_color_rgba); // TODO tuple
	});
	Register(State, "createmesh", cairo_pattern_create_mesh);*/ // Unknown for some reason

	// Regions
	CreateMetatable(State, ToVoidPointer(cairo_region_create), [&](void)
	{
		Register(State, "copy", cairo_region_copy);
		//Register(State, "reference", cairo_region_reference);
		Register(State, "destroy", cairo_region_destroy);
		Register(State, "status", cairo_region_status);
		Register(State, "getextents", cairo_region_get_extents);
		Register(State, "numrectangles", cairo_region_num_rectangles);
		Register(State, "getrectangle", cairo_region_get_rectangle);
		Register(State, "isempty", cairo_region_is_empty);
		Register(State, "containspoint", cairo_region_contains_point);
		Register(State, "containsrectangle", cairo_region_contains_rectangle);
		Register(State, "equal", cairo_region_equal);
		Register(State, "translate", cairo_region_translate);
		Register(State, "intersect", cairo_region_intersect);
		Register(State, "intersectrectangle", cairo_region_intersect_rectangle);
		Register(State, "subtract", cairo_region_subtract);
		Register(State, "subtractrectangle", cairo_region_subtract_rectangle);
		Register(State, "union", cairo_region_union);
		Register(State, "unionrectangle", cairo_region_union_rectangle);
		Register(State, "xor", cairo_region_xor);
		Register(State, "xorrectangle", cairo_region_xor_rectangle);
	});
	Register(State, "region", cairo_region_create);
	//Register(State, "rectanglecairoregion", cairo_region_create_rectangle);
	//Register(State, "cairoregionfromrectangles", cairo_region_create_rectangles);

	// Matrices
	// TODO custom constructor
	// TODO __gc hook to delete pointer
	/*CreateMetatable(State, ToVoidPointer(CreateMatrix), [&](void)
	{
		Register(State, "init", cairo_matrix_init);
		Register(State, "initidentity", cairo_matrix_init_identity);
		Register(State, "inittranslate", cairo_matrix_init_translate);
		Register(State, "initscale", cairo_matrix_init_scale);
		Register(State, "initrotate", cairo_matrix_init_rotate);
		Register(State, "translate", cairo_matrix_translate);
		Register(State, "scale", cairo_matrix_scale);
		Register(State, "rotate", cairo_matrix_rotate);
		Register(State, "invert", cairo_matrix_invert);
		Register(State, "multiply", cairo_matrix_multiply);
		//Register(State, "transformdistance", cairo_matrix_transform_distance); // TODO Input/output
		//Register(State, "transformpoint", cairo_matrix_transform_point); // TODO Input/output
	});
	Register(State, "matrix", CreateMatrix);*/
	
	// TODO Allow metatable reuse through another parameter to register (default null, otherwise metatable id)
	/*CreateMetatable(State, ToVoidPointer(cairo_image_surface_create), [&](void)
	{
		RegisterSurfaceMethods(State);
	});
	Register(State, "similarimagesurface", cairo_
	Register(State, "createsimilar", cairo_surface_create_similar);
	Register(State, "createsimilarimage", cairo_surface_create_similar_image);
	Register(State, "createforrectangle", cairo_surface_create_for_rectangle);*/

#ifdef CAIRO_HAS_IMAGE_SURFACE
	CreateMetatable(State, ToVoidPointer(cairo_image_surface_create), [&](void)
	{
		RegisterSurfaceMethods(State);
		Register(State, "getdata", cairo_image_surface_get_data);
		Register(State, "getformat", cairo_image_surface_get_format);
		Register(State, "getwidth", cairo_image_surface_get_width);
		Register(State, "getheight", cairo_image_surface_get_height);
		Register(State, "getstride", cairo_image_surface_get_stride);
	});
	Register(State, "imagesurface", cairo_image_surface_create);
#endif

#ifdef CAIRO_HAS_PNG_FUNCTIONS
	CreateMetatable(State, ToVoidPointer(cairo_image_surface_create_from_png), [&](void)
	{
		RegisterSurfaceMethods(State);
	});
	Register(State, "imagesurfacefrompng", cairo_image_surface_create_from_png);
#endif

/*
#ifdef CAIRO_HAS_RECORDING_SURFACE
	CreateMetatable(State, ToVoidPointer(cairo_recording_surface_create), [&](void)
	{
		RegisterSurfaceMethods(State);
		//Register(State, "inkextents", cairo_recording_surface_ink_extents); // TODO Tuple
		Register(State, "getextents", cairo_recording_surface_get_extents);
	});
	Register(State, "recordingsurface", cairo_recording_surface_create);
#endif
*/ // ? Missing?

#ifdef CAIRO_HAS_SVG_SURFACE
	CreateMetatable(State, ToVoidPointer(cairo_svg_surface_create), [&](void)
	{
		RegisterSurfaceMethods(State);
		Register(State, "restricttoversion", cairo_svg_surface_restrict_to_version);
	});
	Register(State, "svgsurface", cairo_svg_surface_create);
#endif

#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight + 1);
#endif
}

#endif

