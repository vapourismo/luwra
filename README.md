[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/vapourismo/luwra)

# Luwra
A header-only C++ library which provides a Lua wrapper with minimal overhead.

## Requirements
You need will need a C++14-compliant compiler and a compatible Lua version.

 Platform                                | Lua 5.1 <sup>1)</sup> | Lua 5.2 <sup>1)</sup> | Lua 5.3
-----------------------------------------|-----------------------|-----------------------|---------
 Linux (clang++ 3.6)                     | works                 | works                 | works
 Linux (g++ 5.1)                         | works                 | works                 | works
 FreeBSD <sup>2)</sup> (clang++ 3.6)     | works                 | works                 | works
 FreeBSD <sup>2)</sup> (g++ 5.1)         | works                 | works                 | works
 Everything else                         | untested              | untested              | untested

<sup>**1)**</sup> Has some integer quirks, that why the `types_numeric` test case fails.
<sup>**2)**</sup> You need GNU make (devel/gmake) to use the attached Makefile.

## Usage
Refer to the [wiki pages](https://github.com/vapourismo/luwra/wiki). In order to use the library
you must clone this repository and add its `lib/` folder to your include path.
