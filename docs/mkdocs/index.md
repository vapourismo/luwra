# Installation
Luwra is a header-only library, which means that nothing has to be compiled in order to use it.
Simply clone the [repository](https://github.com/vapourismo/luwra) or
[download](https://github.com/vapourismo/luwra/archive/master.zip) and extract it to a directory of
your preference.

For your application to be able to reach the `lib/luwra.hpp` header file, you must add
`/path/to/luwra/lib` to the list of include paths. With Clang and GCC that is done using the
`-I/path/to/luwra/lib` command-line parameter.

Now you can simply `#include <luwra.hpp>` in your C++ files and start using Luwra.

# Reference Manual
A reference manual exists [here](/reference/).
