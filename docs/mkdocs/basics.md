# Integration
Luwra does not provide a standalone version of Lua nor does it isolate its features. This means that
all functions and classes operate on
[lua_State](http://www.lua.org/manual/5.3/manual.html#lua_State) (or the alias
[State](/reference/namespaceluwra.html#a2c037b44385367826eb4e931b5b8197d)). Doing this allows you to
integrate Luwra however you like.

# Stack Interaction
Although Luwra provides a variety of features, its main concern is efficient and safe interaction
with the Lua stack.

A fundamental aspect of this is the abstract template [Value](/reference/structluwra_1_1Value.html).
Every type which can be pushed onto or read from the stack has a specialization of it.
Useful implementations are provided out of the box:

C++ type               | Pushable | Readable | Lua type
-----------------------|----------|----------|----------------------------
bool                   | yes      | yes      | boolean
signed char            | yes      | yes      | number (integer since 5.3)
signed short           | yes      | yes      | number (integer since 5.3)
signed int             | yes      | yes      | number (integer since 5.3)
signed long int        | yes      | yes      | number (integer since 5.3)
signed long long int   | yes      | yes      | number (integer since 5.3)
unsigned char          | yes      | yes      | number (integer since 5.3)
unsigned short         | yes      | yes      | number (integer since 5.3)
unsigned int           | yes      | yes      | number (integer since 5.3)
unsigned long int      | yes      | yes      | number (integer since 5.3)
unsigned long long int | yes      | yes      | number (integer since 5.3)
float                  | yes      | yes      | number
double                 | yes      | yes      | number
long double            | yes      | yes      | number
const char*            | yes      | yes      | string
std::string            | yes      | yes      | string
std::nullptr_t         | yes      | yes      | nil
std::tuple&lt;T&gt;    | yes      | no       | *depends on the tuple contents*
[lua_CFunction](http://www.lua.org/manual/5.3/manual.html#lua_CFunction) | yes | no | function
[NativeFunction&lt;R(A...)&gt;](/reference/structluwra_1_1NativeFunction_3_01R_07A_8_8_8_08_4.html) | no | yes | function
[FieldVector](/reference/namespaceluwra.html#ac090722c6d5d6b88b31895aad64788c2) | yes | no | table

**Note:** Some numeric types have a different size than their matching Lua type - they will be
truncated during push or read operations.

## Pushing C++ values
When pushing values onto the stack you can either use
[Value&lt;T&gt;::push](/reference/structluwra_1_1Value.html#aa376d68285606c206562b822e8187384) or the more
convenient [push](/reference/namespaceluwra.html#ae8e7eab11fc2cf3f258ffd81571066fa).

```c++
// Push an integer
luwra::push(lua, 1338);

// Push a number
luwra::push(lua, 13.37);

// Push a boolean
luwra::push(lua, false);

// Push a string
luwra::push(lua, "Hello World");

// Push a table
luwra::push(lua, luwra::FieldVector {
	{"one", 1},
	{1, "one"},
	{"nested", luwra::FieldVector {
		{"more", "fields"}
	}}
});
```

This produces the following stack layout:

Absolute Position | Relative Position | Value
------------------|-------------------|------
1                 | -5                | `1338`
2                 | -4                | `13.37`
3                 | -3                | `false`
4                 | -2                | `"Hello World"`
5                 | -1                | `{one = 1, [1] = "one", nested = {more = "fields"}}`

It is possible to provide a template parameter to `push` to enforce pushing a specific type.
In most cases you are probably better off by letting the compiler infer the template parameter.

## Reading Lua values
Simple retrieval of Lua values is done using
[read&lt;T&gt;](/reference/namespaceluwra.html#a4fe4e574680cf54a0f8d958740eb90ab). Consider the
stack layout from the previous example. This is how you would retrieve a value from the stack.

```c++
// Retrieve the integer at position 1
int value = luwra::read<int>(lua, 1);

// Similiar with a relative index
int value = luwra::read<int>(lua, -5);
```

## Read and type errors
What happens when a value which you are trying to read mismatches the expected type or cannot be
converted to it? Most `Value<T>` specializations use Lua's `luaL_check*` functions to retrieve
the values from the stack. This means that no exceptions will be thrown - instead the error handling
is delegated to the Lua VM. Have a look at the
[error handling documentation](http://www.lua.org/manual/5.3/manual.html#4.6) for more information.

# Globals
In order to convenient register values in the global namespace, Luwra provides
[setGlobal](/reference/namespaceluwra.html#afed27900ff117638937ad92e0217258d) and
[getGlobal](/reference/namespaceluwra.html#af0a7dbbbdb339227c6ecaaa46422e05b).


```c++
// Register in the global namespace
luwra::setGlobal(lua, "almostPi", 3.14);

// Retrieve from globals
double almostPi = luwra::getGlobal<double>(lua, "almostPi");
```
