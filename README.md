# Luwra
A header-only C++ library which provides a Lua wrapper with minimal overhead.

NOTE: Luwra is under heavy development the documentation might not be current.

## Usage
Most of Luwra's features are based on template specialization. If you are not familiar with
templates in C++, I highly recommend you inform yourself about them. Otherwise the following
examples will not be of use to you.

### Types
A template `Value<T>` exists to capsulate push and check mechanisms for a type `T`. Default
specializations are implemented for C/C++ numeric types, `bool`, `const char*`,
`std::string`, `Arbitrary` and `U&` where `U` is a user type.

The `Arbitrary` struct symbolizes any value on the stack. Instances of `Arbitrary` can be seen as
references to an index on an execution stack. Note, these kind of references are only valid as long
as their referenced value exists at the given index on the given stack.

The `Value<U&>` specialization is designated to the instantiation and reference of a user data
type `U`.

Any template specialization of `Value` which is expected to work with Luwra must provide a
compatible interface:

```c++
template <>
struct Value<T> {
	/**
	 * Read T value at index n.
	 */
	static inline
	T Read(State* state, int n);

	/**
	 * Push T value onto the stack and return how many values you have pushed.
	 */
	static inline
	int Push(State* state, T value);
};
```

[Example](https://github.com/vapourismo/luwra/blob/master/examples/types.cpp)

### Stack
Instead of retrieving each value from the stack seperately, you can make use of `apply` which lets
you invoke a function, whose parameters map to the stack layout, using the stack values.

Assuming your stack looks like this

Position | Value
---------|-----------
 3       | c = 42.32
 2       | b = 73.31
 1       | a = 13.37

and you have a function with a signature like this:

```c++
lua_Number foo(lua_Number a, lua_Number b, lua_Number c);
```

Simply apply the function:

```c++
lua_Integer result = Apply(lua_state, foo);
```

The above statement is equivalent to

```c++
lua_Integer result = foo(
	luaL_checkinteger(lua_state, 1),
	luaL_checkinteger(lua_state, 2),
	luaL_checkinteger(lua_state, 3)
);
```

[Example](https://github.com/vapourismo/luwra/blob/master/examples/stack.cpp)

### Functions
Luwra's core feature is to wrap C/C++ functions so they can be used with the Lua VM.
All it takes is the signature of the function you want to wrap and a pointer to it.
Wrapping them is as easy as this:

```c++

// The function to be wrapped
lua_Number my_add(lua_Number a, lua_Number b) {
	return a + b;
}

// ...

lua_CFunction cfunc = WrapFunction<lua_Number(lua_Number, lua_Number), my_add>;
```

[Example](https://github.com/vapourismo/luwra/blob/master/examples/functions.cpp)

### User types
Something is going be here soon.
