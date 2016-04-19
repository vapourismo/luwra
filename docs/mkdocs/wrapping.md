# General
Luwra provides an easy way to turn any C or C++ function into a
[lua_CFunction](http://www.lua.org/manual/5.3/manual.html#lua_CFunction) which can be used by the
Lua VM. Note, all parameter types must be readable from the stack (`Value<T>::read` exists for all)
and the return type must be pushable (`Value<T>::push` exists).

## Wrap C/C++ functions
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
