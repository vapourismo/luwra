[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/vapourismo/luwra)

# Luwra
A header-only C++ library which provides a Lua wrapper with minimal overhead.

## Usage
Refer to the [wiki pages](https://github.com/vapourismo/luwra/wiki). In order to use the library
you must clone this repository and add its `lib/` folder to your include path.

## Requirements
You need a C++14-compliant compiler and at least Lua 5.1 to get this library to work. I recommend
using Lua 5.3 or later, to avoid the messy `lua_Integer` situation. LuaJIT 2.0 seems to work aswell.

## Tests
The attached GNU `Makefile` allows you to run both examples and tests using `make examples` and
`make test` respectively. You might need to adjust `LUA_*` variables, so Luwra finds the
Lua headers and library.

Assuming all headers are located in `/usr/include/lua5.3` and the shared object name is
`liblua5.3.so`, you need to invoke this:

```
make LUA_INCDIR=/usr/include/lua5.3 LUA_LIBNAME=lua5.3 test
```

Assertions relying on `lua_Integer` will fail, due to integer quirks in Lua. This should only
concern you if your application expects Lua integers to work like normal integers, because they
don't. Since integers are internally stored as floating-point numbers before Lua 5.3, you might
consider treating them as such in your application.
