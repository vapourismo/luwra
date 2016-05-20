# Wrapping
Luwra provides a simple way to generate Lua [C functions][lua-cfunction] from functions and class
members like methods and accessors using the `LUWRA_WRAP` macro. These kind of C functions are
useful, because they work just like regular Lua functions within the Lua virtual machine.
Registering these functions is the most straightforward way of providing the functionality of your
application to Lua.

## Functions
When wrapping functions, one must consider that all parameter types must be read from the
stack and the return type must be pushed onto the stack.

### Example
Lets assume you want to make the following function available in Lua.

```c++
int my_function(const char* a, int b) {
    return /* magic */;
}
```

First, you must generate a Lua [C function][lua-cfunction]. One utilizes the `LUWRA_WRAP` macro for
this.

```c++
lua_CFunction cfun = LUWRA_WRAP(my_function);
```

**Note:** Do not provide the address of your function (e.g. `&my_function`) to any wrapping macro.
The macro will take care of this itself. You must provide only the name of the function.

Once you have the C function, you can register it in the global namespace.

```c++
luwra::setGlobal(lua, "my_function", cfun);
```

Invoking the function in Lua is fairly straightforward.

```lua
print(my_function("Hello World", 1337))
```

### Performance
[C functions][lua-cfunction] are dynamically created at compile-time. All of the functions involved
in wrapping are marked as `inline`, which means modern compilers produce wrapper functions with zero
overhead, when optimization is turned on.

For the example above, the resulting code would look similiar to the following.

```c++
static int cfun(lua_State* state) {
	lua_pushinteger(
		state,
		my_function(
			luaL_checkstring(state, 1),
			luaL_checkinteger(state, 1)
		)
	);
	return 1;
}
```

## Class members
Although a little trickier, it is also possible to turn C++ field accessors and methods into Lua
[C functions][lua-cfunction]. The resulting Lua functions expect the first (or `self`) parameter to
be an instance of the type which the wrapped field or method belongs to.

**Note:** Before you wrap fields and methods manually, you might want to take a look at the
[User Types](/user-types/) section.

### Example
This example will operate on the following structure.

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

Wrapping field accessors and methods works identical to wrapping functions.

```c++
lua_CFunction cfun_x     = LUWRA_WRAP(Point::x),
              cfun_y     = LUWRA_WRAP(Point::y),
              cfun_scale = LUWRA_WRAP(Point::scale);

// Register in global namespace
luwra::setGlobal(lua, "x", cfun_x);
luwra::setGlobal(lua, "y", cfun_y);
luwra::setGlobal(lua, "scale", cfun_scale);
```

Usage in Lua is analogous to function usage.

```lua
-- Instantiate 'Point' here, have a look at the User Types section to find out how to do this
local my_point = ...

-- Access 'x' and 'y' field
print(x(my_point), y(my_point))

-- Set 'x' and 'y' field
x(my_point, 13.37)
y(my_point, 73.31)

-- Invoke 'scale' method
scale(my_point, 2)
```

[lua-cfunction]: http://www.lua.org/manual/5.3/manual.html#lua_CFunction
