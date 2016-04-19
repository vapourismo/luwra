# Function wrapping
Luwra provides an easy way to turn any C or C++ function into a
[lua_CFunction](http://www.lua.org/manual/5.3/manual.html#lua_CFunction) which can be used by the
Lua VM. Note, all parameter types must be readable from the stack (`Value<T>::read` exists for all)
and the return type must be pushable (`Value<T>::push` exists).
