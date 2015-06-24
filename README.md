[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/vapourismo/luwra)

# Luwra
A header-only C++ library which provides a Lua wrapper with minimal overhead.

## Usage
Refer to the [wiki pages](https://github.com/vapourismo/luwra/wiki). In order to use the library
you must clone this repository and add its `lib/` folder to your include path.

## Requirements
You need a C++14-compliant compiler and at least Lua 5.1 to get this library to work.

## Tests
The attached GNU `Makefile` allows you to run both examples and tests using `make examples` and
`make test` respectively. You might need to adjust the `LUA_*` variables, so Luwra finds the
Lua headers and library.

So far all tests have been run on recent versions of Arch Linux and FreeBSD, with following results.

 Compiler    | Lua 5.1             | Lua 5.2             | Lua 5.3
-------------|---------------------|---------------------|---------
 clang++ 3.5 | partial<sup>*</sup> | partial<sup>*</sup> | passes
 clang++ 3.6 | partial<sup>*</sup> | partial<sup>*</sup> | passes
 g++ 5.1     | partial<sup>*</sup> | partial<sup>*</sup> | passes

<sup>*</sup> Assertions relying on `lua_Integer` will fail, due to integer quirks in Lua.
