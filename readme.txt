Lua bindings for Cairo written in C++ making heavy use of C++11 variadic templates and such.  

Written by Rendaw (at zarbosoft.com)

Put the generated cairo.so in `lua -e "print(package.cpath)"`, $LUA_PATH, or $LUA_CPATH
Require with: require "cairo"
Function and enum reference in app/registration.h

Note: This doesn't work on my copy of g++-4.7, clang 3.0.  It works on g++-4.8, but with a small ugly workaround (committed).  I haven't tried with a Microsoft compiler.

The goal was to see how little work I could do to implement the bindings on a per-function basis.  I used variadic templates to automatically determine function inputs and outputs and generate the appropriate Lua binding code.  It works, although there is one function that breaks the argument pattern of the other functions (a get method for linear gradients, maybe?).  

