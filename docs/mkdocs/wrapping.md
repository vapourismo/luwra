# General
Luwra provides a simple way to generate
Lua [C functions](http://www.lua.org/manual/5.3/manual.html#lua_CFunction) from functions and class
members like methods and accessors using the `LUWRA_WRAP` macro. These kind of C functions are
useful, because they work just like regular Lua functions within the Lua virtual machine.

## Functions
When wrapping functions, one must consider that all parameter types must be able to be read from the
stack and the return type must be able to be pushed onto the stack.

Assuming you have a function similiar to this:

```c++
int my_function(const char* a, int b) {
    return /* magic */;
}
```

You can easily wrap it using the `LUWRA_WRAP` macro:

```c++
// Convert to lua_CFunction
lua_CFunction cfun = LUWRA_WRAP(my_function);

// Do something with it, for example set it as a Lua global function
luwra::setGlobal(lua, "my_function", cfun);
```

**Note:** Do not provide the address of your function (e.g. `&my_function`) to any wrapping macro.
The macros will take care of this themselves. You must provide only the name of the function.

Calling the function from Lua is fairly straightforward:

```lua
local my_result = my_function("Hello World", 1337)
print(my_result)
```
## Methods and fields
It is also possible to turn C++ field accessors and methods into `lua_CFunction`s. It is a little
trickier than wrapping normal functions. The resulting Lua functions expect the first (or `self`)
parameter to be a user type instance of the type which the wrapped field or method belongs to.

**Note:** Before you wrap fields and methods manually, you might want to take a look at the
[User Types](/user-types/) section.

The next examples will operate on the following structure:

```c++
struct Point {
    double x, y;

    // ...

    void scale(double f) {
        x *= f;
        y *= f;
    }
};
```

In order to wrap `x`, `y` and `scale` we utilize the `LUWRA_WRAP` macro again:

```c++
lua_CFunction cfun_x     = LUWRA_WRAP(Point::x),
              cfun_y     = LUWRA_WRAP(Point::y),
              cfun_scale = LUWRA_WRAP(Point::scale);

// Register as globals
luwra::setGlobal(lua, "x", cfun_x);
luwra::setGlobal(lua, "y", cfun_y);
luwra::setGlobal(lua, "scale", cfun_scale);
```

Usage looks like this:

```lua
local my_point = -- Magic

-- Access 'x' and 'y' field
print(x(my_point), y(my_point))

-- Set 'x' and 'y' field
x(my_point, 13.37)
y(my_point, 73.31)

-- Invoke 'scale' method
scale(my_point, 2)
```
