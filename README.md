[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/vapourismo/luwra)
[![Build Status](https://travis-ci.org/vapourismo/luwra.svg?branch=master)](https://travis-ci.org/vapourismo/luwra)
# Luwra
A header-only C++ library which provides a Lua wrapper with minimal overhead.

## Usage
Refer to the [wiki pages](https://github.com/vapourismo/luwra/wiki) or the
[documentation](http://vapourismo.github.io/luwra/docs). In order to use the library you must clone this
repository and include `lib/luwra.hpp`.

Have a question? Simply [ask](https://gitter.im/vapourismo/luwra) or open an issue.

## Examples
In the following examples `lua` refers to an instance of `lua_State*`.

Easily push values onto the stack:

```c++
// Push an integer
luwra::push(lua, 1338);

// Push a number
luwra::push(lua, 13.37);

// Push a boolean
luwra::push(lua, false);

// Push a string
luwra::push(lua, "Hello World");
```

Or retrieve them:

```c++
// Your function
int my_fun(int a, int b) {
    return a + b;
}

// Prepare stack
luwra::push(lua, 13);
luwra::push(lua, 37);

// Apply your function
int result = luwra::apply(lua, my_fun);

// which is equivalent to
int result = luwra::apply(lua, 1, my_fun);

// and equivalent to
int result = luwra::apply(lua, -2, my_fun);

// All of this is essentially syntactic sugar for
int result = my_fun(luwra::read<int>(lua, 1), luwra::read<int>(lua, 2));
```

Generate a C function which can be used by Lua:

```c++
// Assuming your function looks something like this
int my_function(const char* a, int b) {
    // ...
}

// Convert to lua_CFunction
lua_CFunction cfun = LUWRA_WRAP(my_function);

// Do something with it, for example set it as a Lua global function
luwra::setGlobal(lua, "my_function", cfun);
```

```lua
-- Invoke the registered function
local my_result = my_function("Hello World", 1337)
print(my_result)
```

Or register your own class:

```c++
struct Point {
    double x, y;

    Point(double x, double y):
        x(x), y(y)
    {
        std::cout << "Construct Point(" << x << ", " << y << ")" << std::endl;
    }

    ~Point() {
        std::cout << "Destruct Point(" << x << ", " << y << ")" << std::endl;
    }

    void scale(double f) {
        x *= f;
        y *= f;
    }

    std::string __tostring() {
        return "<Point(" + std::to_string(x) + ", " + std::to_string(y) + ")>";
    }
};

// Register the metatable and constructor
luwra::registerUserType<Point(double, double)>(
    lua,

    // Constructor name
    "Point",

    // Methods need to be declared here
    {
        LUWRA_MEMBER(Point, scale),
        LUWRA_MEMBER(Point, x),
        LUWRA_MEMBER(Point, y)
    },

    // Meta methods may be registered aswell
    {
        LUWRA_MEMBER(Point, __tostring)
    }
);
```

```lua
-- Instantiate 'Point'
local point = Point(13, 37)

-- Invoke 'scale' method
point:scale(1.5)

-- Convert to string via '__tostring' meta method
print(point)

-- Read properties 'x' and 'y'
print(point:x(), point:y())

-- Set property 'x'
point:x(4.2)
```

## Requirements
You need a C++11-compliant compiler and at least Lua 5.1 to get this library to work. I recommend
using Lua 5.3 or later, to avoid the messy `lua_Integer` situation. LuaJIT 2.0 seems to work, apart
from user types, which fail for yet unknown reasons.

## Tests
The attached GNU `Makefile` allows you to run both examples and tests using `make examples` and
`make test` respectively. You might need to adjust `LUA_*` variables, so Luwra finds the
Lua headers and library.

Assuming all headers are located in `/usr/include/lua5.3` and the shared object name is
`liblua5.3.so`, you need to invoke this:

```
make LUA_INCDIR=/usr/include/lua5.3 LUA_LIBNAME=lua5.3 test
```
