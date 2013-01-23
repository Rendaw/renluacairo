Lua bindings for Cairo written in C++ making heavy use of C++11 variadic templates and such.  

Written by Rendaw (at zarbosoft.com)

The goal was to see how little work I could do to implement the bindings on a per-function basis.  I used variadic templates to automatically determine function inputs and outputs and generate the appropriate Lua binding code.  It works, although there is one function that breaks the argument pattern of the other functions (a get method for linear gradients, maybe?).  

Safetly, automatically dereferencing/referencing objects seems impossible using C++11 template magic.  I got stuck trying to automatically reference objects when requested from objects that generated them internally.  Maybe objects aren't actually generated internally, and I just need to link the lua objects together, but that's an issue I haven't looked at.

TODO: Referencing, dereferencing
TODO: Text methods
TODO: Pango?

