#ifndef registration_h
#define registration_h

#include "library.h"

// Matrix stuff
static int DestroyMatrix(lua_State *State)
{
	cairo_matrix_t *Matrix = LuaValue<cairo_matrix_t *>::Read(State, 1);
	delete Matrix;
	lua_settop(State, 0);
	return 0;
}

cairo_matrix_t *CreateMatrix(double xx, double yx, double xy, double yy, double x0, double y0)
{ 
	cairo_matrix_t *Out = new cairo_matrix_t;
	cairo_matrix_init(Out, xx, yx, xy, yy, x0, y0);
	return Out;
}

cairo_matrix_t *CreateIdentityMatrix(void)
{ 
	cairo_matrix_t *Out = new cairo_matrix_t;
	cairo_matrix_init_identity(Out);
	return Out;
}

cairo_matrix_t *CreateTranslateMatrix(double tx, double ty)
{ 
	cairo_matrix_t *Out = new cairo_matrix_t;
	cairo_matrix_init_translate(Out, tx, ty);
	return Out;
}

cairo_matrix_t *CreateScaleMatrix(double sx, double sy)
{ 
	cairo_matrix_t *Out = new cairo_matrix_t;
	cairo_matrix_init_scale(Out, sx, sy);
	return Out;
}

cairo_matrix_t *CreateRotateMatrix(double radians)
{ 
	cairo_matrix_t *Out = new cairo_matrix_t;
	cairo_matrix_init_rotate(Out, radians);
	return Out;
}

// Bulk registration
inline void RegisterSurfaceMethods(lua_State *State)
{
	// Perhaps some sort of metatable inheritance should be implemented, but for now I just duplicate the cfunctions in lua for "inheritance".
	Register(State, "status", cairo_surface_status);
	Register(State, "finish", cairo_surface_finish);
	Register(State, "flush", cairo_surface_flush);
	//Register(State, "getdevice", cairo_surface_get_device);
	Register(State, "getfontoptions", cairo_surface_get_font_options);
	Register(State, "getcontent", cairo_surface_get_content);
	Register(State, "markdirty", cairo_surface_mark_dirty);
	Register(State, "markdirtyrectangle", cairo_surface_mark_dirty_rectangle);
	Register(State, "setdeviceoffset", cairo_surface_set_device_offset);
	RegisterMultipleReturn(State, "getdeviceoffset", cairo_surface_get_device_offset);
	Register(State, "setfallbackresolution", cairo_surface_set_fallback_resolution);
	RegisterMultipleReturn(State, "getfallbackresolution", cairo_surface_get_fallback_resolution);
	Register(State, "gettype", cairo_surface_get_type);
	//Register(State, "getreferencecount", cairo_surface_get_reference_count); // Useful?
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

inline void RegisterEverything(lua_State *State)
{
#ifndef NDEBUG
	unsigned int const InitialHeight = lua_gettop(State);
#endif
	lua_newtable(State);

	RegisterEnum(State, "antialias", {
		{"DEFAULT", CAIRO_ANTIALIAS_DEFAULT},
		{"NONE", CAIRO_ANTIALIAS_NONE},
		{"GRAY", CAIRO_ANTIALIAS_GRAY},
		{"SUBPIXEL", CAIRO_ANTIALIAS_SUBPIXEL},
	});

	RegisterEnum(State, "fillrule", {
		{"WINDING", CAIRO_FILL_RULE_WINDING},
		{"EVENODD", CAIRO_FILL_RULE_EVEN_ODD},
	});

	RegisterEnum(State, "linecap", {
		{"BUTT", CAIRO_LINE_CAP_BUTT},
		{"ROUND", CAIRO_LINE_CAP_ROUND},
		{"SQUARE", CAIRO_LINE_CAP_SQUARE},
	});

	RegisterEnum(State, "linejoin", {
		{"MITER", CAIRO_LINE_JOIN_MITER},
		{"ROUND", CAIRO_LINE_JOIN_ROUND},
		{"BEVEL", CAIRO_LINE_JOIN_BEVEL},
	});

	RegisterEnum(State, "operator", {
		{"CLEAR", CAIRO_OPERATOR_CLEAR},
		{"SOURCE", CAIRO_OPERATOR_SOURCE},
		{"OVER", CAIRO_OPERATOR_OVER},
		{"IN", CAIRO_OPERATOR_IN},
		{"OUT", CAIRO_OPERATOR_OUT},
		{"ATOP", CAIRO_OPERATOR_ATOP},
		{"DEST", CAIRO_OPERATOR_DEST},
		{"DESTOVER", CAIRO_OPERATOR_DEST_OVER},
		{"DESTIN", CAIRO_OPERATOR_DEST_IN},
		{"DESTOUT", CAIRO_OPERATOR_DEST_OUT},
		{"DESTATOP", CAIRO_OPERATOR_DEST_ATOP},
		{"XOR", CAIRO_OPERATOR_XOR},
		{"ADD", CAIRO_OPERATOR_ADD},
		{"SATURATE", CAIRO_OPERATOR_SATURATE},
		{"MULTIPLY", CAIRO_OPERATOR_MULTIPLY},
		{"SCREEN", CAIRO_OPERATOR_SCREEN},
		{"OVERLAY", CAIRO_OPERATOR_OVERLAY},
		{"DARKEN", CAIRO_OPERATOR_DARKEN},
		{"LIGHTEN", CAIRO_OPERATOR_LIGHTEN},
		{"COLORDODGE", CAIRO_OPERATOR_COLOR_DODGE},
		{"COLORBURN", CAIRO_OPERATOR_COLOR_BURN},
		{"HARDLIGHT", CAIRO_OPERATOR_HARD_LIGHT},
		{"SOFTLIGHT", CAIRO_OPERATOR_SOFT_LIGHT},
		{"DIFFERENCE", CAIRO_OPERATOR_DIFFERENCE},
		{"EXCLUSION", CAIRO_OPERATOR_EXCLUSION},
		{"HSLHUE", CAIRO_OPERATOR_HSL_HUE},
		{"HSLSATURATION", CAIRO_OPERATOR_HSL_SATURATION},
		{"HSLCOLOR", CAIRO_OPERATOR_HSL_COLOR},
		{"HSLLUMINOSITY", CAIRO_OPERATOR_HSL_LUMINOSITY},
	});

	RegisterEnum(State, "path", {
		{"MOVETO", CAIRO_PATH_MOVE_TO},
		{"LINETO", CAIRO_PATH_LINE_TO},
		{"CURVETO", CAIRO_PATH_CURVE_TO},
		{"CLOSEPATH", CAIRO_PATH_CLOSE_PATH},
	});

	RegisterEnum(State, "extend", {
		{"NONE", CAIRO_EXTEND_NONE},
		{"REPEAT", CAIRO_EXTEND_REPEAT},
		{"REFLECT", CAIRO_EXTEND_REFLECT},
		{"PAD", CAIRO_EXTEND_PAD},
	});

	RegisterEnum(State, "filter", {
		{"FAST", CAIRO_FILTER_FAST},
		{"GOOD", CAIRO_FILTER_GOOD},
		{"BEST", CAIRO_FILTER_BEST},
		{"NEAREST", CAIRO_FILTER_NEAREST},
		{"BILINEAR", CAIRO_FILTER_BILINEAR},
		{"GAUSSIAN", CAIRO_FILTER_GAUSSIAN},
	});

	RegisterEnum(State, "patterntype", {
		{"SOLID", CAIRO_PATTERN_TYPE_SOLID},
		{"SURFACE", CAIRO_PATTERN_TYPE_SURFACE},
		{"LINEAR", CAIRO_PATTERN_TYPE_LINEAR},
		{"RADIAL", CAIRO_PATTERN_TYPE_RADIAL},
	});

	RegisterEnum(State, "regionoverlap", {
		{"IN", CAIRO_REGION_OVERLAP_IN},
		{"OUT", CAIRO_REGION_OVERLAP_OUT},
		{"PART", CAIRO_REGION_OVERLAP_PART},
	});

	RegisterEnum(State, "devicetype", {
		{"DRM", CAIRO_DEVICE_TYPE_DRM},
		{"GL", CAIRO_DEVICE_TYPE_GL},
		{"SCRIPT", CAIRO_DEVICE_TYPE_SCRIPT},
		{"XCB", CAIRO_DEVICE_TYPE_XCB},
		{"XLIB", CAIRO_DEVICE_TYPE_XLIB},
		{"XML", CAIRO_DEVICE_TYPE_XML},
	});

	RegisterEnum(State, "content", {
		{"COLOR", CAIRO_CONTENT_COLOR},
		{"ALPHA", CAIRO_CONTENT_ALPHA},
		{"COLORALPHA", CAIRO_CONTENT_COLOR_ALPHA},
	});


	RegisterEnum(State, "surfacetype", {
		{"IMAGE", CAIRO_SURFACE_TYPE_IMAGE},
		{"PDF", CAIRO_SURFACE_TYPE_PDF},
		{"PS", CAIRO_SURFACE_TYPE_PS},
		{"XLIB", CAIRO_SURFACE_TYPE_XLIB},
		{"XCB", CAIRO_SURFACE_TYPE_XCB},
		{"GLITZ", CAIRO_SURFACE_TYPE_GLITZ},
		{"QUARTZ", CAIRO_SURFACE_TYPE_QUARTZ},
		{"WIN32", CAIRO_SURFACE_TYPE_WIN32},
		{"BEOS", CAIRO_SURFACE_TYPE_BEOS},
		{"DIRECTFB", CAIRO_SURFACE_TYPE_DIRECTFB},
		{"SVG", CAIRO_SURFACE_TYPE_SVG},
		{"OS2", CAIRO_SURFACE_TYPE_OS2},
		{"WIN32PRINTING", CAIRO_SURFACE_TYPE_WIN32_PRINTING},
		{"QUARTZIMAGE", CAIRO_SURFACE_TYPE_QUARTZ_IMAGE},
		{"SCRIPT", CAIRO_SURFACE_TYPE_SCRIPT},
		{"QT", CAIRO_SURFACE_TYPE_QT},
		{"RECORDING", CAIRO_SURFACE_TYPE_RECORDING},
		{"VG", CAIRO_SURFACE_TYPE_VG},
		{"GL", CAIRO_SURFACE_TYPE_GL},
		{"DRM", CAIRO_SURFACE_TYPE_DRM},
		{"TEE", CAIRO_SURFACE_TYPE_TEE},
		{"XML", CAIRO_SURFACE_TYPE_XML},
		{"SKIA", CAIRO_SURFACE_TYPE_SKIA},
		{"SUBSURFACE", CAIRO_SURFACE_TYPE_SUBSURFACE},
	});

	RegisterEnum(State, "format", {
		{"INVALID", CAIRO_FORMAT_INVALID},
		{"ARGB32", CAIRO_FORMAT_ARGB32},
		{"RGB24", CAIRO_FORMAT_RGB24},
		{"A8", CAIRO_FORMAT_A8},
		{"A1", CAIRO_FORMAT_A1},
		{"RGB16565", CAIRO_FORMAT_RGB16_565}
	});
	
	static char PatternMetatable;
	
	CreateMetatable(State, ToVoidPointer(cairo_create), [&](void)
	{
		//Register(State, "reference", cairo_reference); // Useful?
		//Register(State, "destroy", cairo_destroy); // Useful?
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
		RegisterWithMetatable(State, "getsource", Reference(cairo_get_source, cairo_pattern_reference), ToVoidPointer(&PatternMetatable));
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
		RegisterMultipleReturn(State, "clipextents", cairo_clip_extents);
		Register(State, "inclip", cairo_in_clip);
		Register(State, "resetclip", cairo_reset_clip);
		//Register(State, "rectanglelistdestroy", cairo_rectangle_list_destroy); // Needs custom lua wrapper for array information
		//Register(State, "copycliprectanglelist", cairo_copy_clip_rectangle_list); // ""
		Register(State, "fill", cairo_fill);
		Register(State, "fillpreserve", cairo_fill_preserve);
		RegisterMultipleReturn(State, "fillextents", cairo_fill_extents);
		Register(State, "infill", cairo_in_fill);
		Register(State, "mask", cairo_mask);
		Register(State, "masksurface", cairo_mask_surface);
		Register(State, "paint", cairo_paint);
		Register(State, "paintwithalpha", cairo_paint_with_alpha);
		Register(State, "stroke", cairo_stroke);
		Register(State, "strokepreserve", cairo_stroke_preserve);
		RegisterMultipleReturn(State, "strokeextents", cairo_stroke_extents);
		Register(State, "instroke", cairo_in_stroke);
		Register(State, "copypage", cairo_copy_page);
		Register(State, "showpage", cairo_show_page);

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
		Register(State, "glyphpath", cairo_glyph_path);
		Register(State, "textpath", cairo_text_path);
		Register(State, "relcurveto", cairo_rel_curve_to);
		Register(State, "rellineto", cairo_rel_line_to);
		Register(State, "relmoveto", cairo_rel_move_to);
		RegisterMultipleReturn(State, "pathextents", cairo_path_extents);

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
	SetMetatableGarbageCollector(State, ToVoidPointer(cairo_create), cairo_destroy);
	Register(State, "context", cairo_create);

	CreateMetatable(State, ToVoidPointer(&PatternMetatable), [&](void)
	{
		Register(State, "status", cairo_pattern_status);
		Register(State, "setextend", cairo_pattern_set_extend);
		Register(State, "getextend", cairo_pattern_get_extend);
		Register(State, "setfilter", cairo_pattern_set_filter);
		Register(State, "getfilter", cairo_pattern_get_filter);
		Register(State, "setmatrix", cairo_pattern_set_matrix);
		Register(State, "getmatrix", cairo_pattern_get_matrix);
		Register(State, "gettype", cairo_pattern_get_type);
		Register(State, "getreferencecount", cairo_pattern_get_reference_count);
		
		// RGB patterns only	
		RegisterMultipleReturn(State, "getrgba", cairo_pattern_get_rgba);
			
		// Linear patterns only
		RegisterMultipleReturn(State, "getlinearpoints", cairo_pattern_get_linear_points);
		Register(State, "addcolorstoprgb", cairo_pattern_add_color_stop_rgb);
		Register(State, "addcolorstoprgba", cairo_pattern_add_color_stop_rgba);
		RegisterMultipleReturn(State, "getcolorstopcount", cairo_pattern_get_color_stop_count);
		//RegisterMultipleReturn(State, "getcolorstoprgba", cairo_pattern_get_color_stop_rgba); // Need new specialization for 1 input bunch of outputs?
		
		// Radial patterns only
		RegisterMultipleReturn(State, "getradialcircles", cairo_pattern_get_radial_circles);
		Register(State, "addcolorstoprgb", cairo_pattern_add_color_stop_rgb);
		Register(State, "addcolorstoprgba", cairo_pattern_add_color_stop_rgba);
		RegisterMultipleReturn(State, "getcolorstopcount", cairo_pattern_get_color_stop_count);
		//RegisterMultipleReturn(State, "getcolorstoprgba", cairo_pattern_get_color_stop_rgba); // Same as above
		
		// Surface patterns only
		RegisterWithMetatable(State, "getsurface", cairo_pattern_get_surface, ToVoidPointer(cairo_surface_create_similar));
		// Mesh patterns only
		/*Register(State, "beginpatch", cairo_mesh_pattern_begin_patch);
		Register(State, "endpatch", cairo_mesh_pattern_end_patch);
		Register(State, "moveto", cairo_mesh_pattern_move_to);
		Register(State, "lineto", cairo_mesh_pattern_line_to);
		Register(State, "curveto", cairo_mesh_pattern_curve_to);
		Register(State, "setcontrolpoint", cairo_mesh_pattern_set_control_point);
		Register(State, "setcornercolorrgb", cairo_mesh_pattern_set_corner_color_rgb);
		Register(State, "setcornercolorrgba", cairo_mesh_pattern_set_corner_color_rgba);
		RegisterMultipleReturn(State, "getpatchcount", cairo_mesh_pattern_get_patch_count);
		Register(State, "getpath", cairo_mesh_pattern_get_path);
		RegisterMultipleReturn(State, "getcontrolpoint", cairo_mesh_pattern_get_control_point);
		RegisterMultipleReturn(State, "getcornercolorrgba", cairo_mesh_pattern_get_corner_color_rgba); */
	});
	SetMetatableGarbageCollector(State, ToVoidPointer(&PatternMetatable), cairo_pattern_destroy);

	RegisterWithMetatable(State, "rgbpattern", cairo_pattern_create_rgb, ToVoidPointer(&PatternMetatable));
	RegisterWithMetatable(State, "rgbapattern", cairo_pattern_create_rgba, ToVoidPointer(&PatternMetatable));
		
	RegisterWithMetatable(State, "linearpattern", cairo_pattern_create_linear, ToVoidPointer(&PatternMetatable));
		
	RegisterWithMetatable(State, "radialpattern", cairo_pattern_create_radial, ToVoidPointer(&PatternMetatable));
		
	RegisterWithMetatable(State, "surfacepattern", cairo_pattern_create_for_surface, ToVoidPointer(&PatternMetatable));
		
	//RegisterWithMetatable(State, "createmesh", cairo_pattern_create_mesh, ToVoidPointer(&PatternMetatable)); // 1.12 

	// Regions
	CreateMetatable(State, ToVoidPointer(cairo_region_create), [&](void)
	{
		Register(State, "copy", cairo_region_copy);
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
	SetMetatableGarbageCollector(State, ToVoidPointer(&cairo_region_create), cairo_region_destroy);
	Register(State, "region", cairo_region_create);
	RegisterWithMetatable(State, "rectanglecairoregion", cairo_region_create_rectangle, ToVoidPointer(cairo_region_create));
	RegisterWithMetatable(State, "cairoregionfromrectangles", cairo_region_create_rectangles, ToVoidPointer(cairo_region_create));

	// Matrices
	CreateMetatable(State, ToVoidPointer(CreateMatrix), [&](void)
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
		RegisterInputOutput(State, "transformdistance", cairo_matrix_transform_distance);
		RegisterInputOutput(State, "transformpoint", cairo_matrix_transform_point);
	});
	SetMetatableGarbageCollector(State, ToVoidPointer(CreateMatrix), DestroyMatrix);
	Register(State, "matrix", CreateMatrix);
	RegisterWithMetatable(State, "identitymatrix", CreateIdentityMatrix, ToVoidPointer(CreateMatrix));
	RegisterWithMetatable(State, "translatematrix", CreateTranslateMatrix, ToVoidPointer(CreateMatrix));
	RegisterWithMetatable(State, "scalematrix", CreateScaleMatrix, ToVoidPointer(CreateMatrix));
	RegisterWithMetatable(State, "rotatematrix", CreateRotateMatrix, ToVoidPointer(CreateMatrix));
	
	CreateMetatable(State, ToVoidPointer(cairo_surface_create_similar), [&](void)
	{
		RegisterSurfaceMethods(State);
	});
	SetMetatableGarbageCollector(State, ToVoidPointer(&cairo_surface_create_similar), cairo_surface_destroy);
	Register(State, "similarsurface", cairo_surface_create_similar);
	//RegisterWithMetatable(State, "similarimagesurface", cairo_surface_create_similar_image, ToVoidPointer(cairo_surface_create_similar)); // 1.12
	RegisterWithMetatable(State, "rectanglesurface", cairo_surface_create_for_rectangle, ToVoidPointer(cairo_surface_create_similar));

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
	SetMetatableGarbageCollector(State, ToVoidPointer(&cairo_image_surface_create), cairo_surface_destroy);
	Register(State, "imagesurface", cairo_image_surface_create);
#endif

#ifdef CAIRO_HAS_PNG_FUNCTIONS
	CreateMetatable(State, ToVoidPointer(cairo_image_surface_create_from_png), [&](void)
	{
		RegisterSurfaceMethods(State);
	});
	SetMetatableGarbageCollector(State, ToVoidPointer(&cairo_image_surface_create_from_png), cairo_surface_destroy);
	Register(State, "imagesurfacefrompng", cairo_image_surface_create_from_png);
#endif

#ifdef CAIRO_HAS_RECORDING_SURFACE
	CreateMetatable(State, ToVoidPointer(cairo_recording_surface_create), [&](void)
	{
		RegisterSurfaceMethods(State);
		RegisterMultipleReturn(State, "inkextents", cairo_recording_surface_ink_extents);
		//Register(State, "getextents", cairo_recording_surface_get_extents); // 1.12
	});
	SetMetatableGarbageCollector(State, ToVoidPointer(&cairo_recording_surface_create), cairo_surface_destroy);
	Register(State, "recordingsurface", cairo_recording_surface_create);
#endif

#ifdef CAIRO_HAS_SVG_SURFACE
	RegisterEnum(State, "svgversion", {
		{"11", CAIRO_SVG_VERSION_1_1},
		{"12", CAIRO_SVG_VERSION_1_2},
	});

	CreateMetatable(State, ToVoidPointer(cairo_svg_surface_create), [&](void)
	{
		RegisterSurfaceMethods(State);
		Register(State, "restricttoversion", cairo_svg_surface_restrict_to_version);
	});
	SetMetatableGarbageCollector(State, ToVoidPointer(&cairo_svg_surface_create), cairo_surface_destroy);
	Register(State, "svgsurface", cairo_svg_surface_create);
#endif

#ifndef NDEBUG
	assert((unsigned int)lua_gettop(State) == InitialHeight + 1);
#endif
}

#endif

