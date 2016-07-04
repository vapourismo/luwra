# Installation
Luwra is a header-only library, which means that nothing has to be compiled in order to use it.
Simply clone the [repository][luwra-repo] or
[download][luwra-download] it and extract it to a directory
of your preference.

For your application to be able to reach the `lib/luwra.hpp` header file, you must add
`/path/to/luwra/lib` to the list of include paths. With Clang and GCC that is done using the
`-I/path/to/luwra/lib` command-line parameter.

Now you can simply `#include <luwra.hpp>` in your C++ files and start using Luwra.

# Integration
This library does not provide a standalone version of Lua nor does it isolate its features. This
means that all functions and classes can operate on [lua_State][lua-state] (or the alias
[State][luwra-state]). Doing this allows you to integrate Luwra however you like.

Nevertheless, you must have a version of Lua installed. Luwra will include the necessary header
files, but it can't link against the Lua library itself.

# Reference Manual
A reference manual exists [here][luwra-refmanual].

[lua-state]: http://www.lua.org/manual/5.3/manual.html#lua_State
[luwra-repo]: https://github.com/vapourismo/luwra
[luwra-download]: https://github.com/vapourismo/luwra/archive/master.zip
[luwra-refmanual]: /reference/namespaceluwra.html
[luwra-state]: /reference/namespaceluwra.html#a2c037b44385367826eb4e931b5b8197d
