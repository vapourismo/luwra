# Stack interaction
A fundamental aspect of this is the abstract template [Value][luwra-value]. Every type which can be
pushed onto or read from the stack has a specialization of it. Useful implementations are provided
out of the box:

C++ type                               | Pushable | Readable | Lua type
---------------------------------------|----------|----------|----------------------------
bool                                   | yes      | yes      | boolean
signed char                            | yes      | yes      | number (integer since 5.3)
signed short                           | yes      | yes      | number (integer since 5.3)
signed int                             | yes      | yes      | number (integer since 5.3)
signed long int                        | yes      | yes      | number (integer since 5.3)
signed long long int                   | yes      | yes      | number (integer since 5.3)
unsigned char                          | yes      | yes      | number (integer since 5.3)
unsigned short                         | yes      | yes      | number (integer since 5.3)
unsigned int                           | yes      | yes      | number (integer since 5.3)
unsigned long int                      | yes      | yes      | number (integer since 5.3)
unsigned long long int                 | yes      | yes      | number (integer since 5.3)
float                                  | yes      | yes      | number
double                                 | yes      | yes      | number
long double                            | yes      | yes      | number
const char*                            | yes      | yes      | string
std::string                            | yes      | yes      | string
std::nullptr_t                         | yes      | yes      | nil
std::tuple&lt;T...&gt;                 | yes      | no       | *depends on the tuple contents*
std::vector&lt;T&gt;                   | yes      | yes      | table
std::list&lt;T&gt;                     | yes      | yes      | table
std::map&lt;K, V&gt;                   | yes      | yes      | table
[lua_CFunction][lua-cfunction]         | yes      | no       | function, table or userdata
[NativeFunction][luwra-nativefunction] | yes      | yes      | function
[Table][luwra-table]                   | yes      | yes      | table

**Note:** Some numeric types have a different size than their matching Lua type - they will be
truncated during push or read operations.

## Extending supported types
If you are missing a type that cannot be used as a [user type](/user-types), you can add a
specialization of [Value][luwra-value]. All you need to do is modify the following snippet for your
type `T`.

```c++
namespace luwra {
	template <>
	struct Value<T> {
		static inline
		T read(State* state, int index) {
			return /* Return the instance of T that you have read at the given index */;
		}

		static inline
		size_t push(State* state, const T& value) {
			// Push the given value on top of the stack
			return /* Return how many values you have pushed onto the stack */;
		}
	};
}
```

## Pushing C++ values
When pushing values onto the stack you can either use `Value<T>::push` or the more convenient
[push][luwra-push].

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
luwra::push(lua, luwra::MemberMap {
	{"one", 1},
	{1, "one"},
	{"nested", luwra::MemberMap {
		{"more", "fields"}
	}}
});
```

**Note:** [luwra::MemberMap][luwra-membermap] is an alias for
`std:map<luwra::Pushable, luwra::Pushable>`. Its keys and values are constructible using any
pushable type.

This produces the following stack layout:

Absolute Position | Relative Position | Value
------------------|-------------------|------
1                 | -5                | `1338`
2                 | -4                | `13.37`
3                 | -3                | `false`
4                 | -2                | `"Hello World"`
5                 | -1                | `{one = 1, [1] = "one", nested = {more = "fields"}}`

It is possible to provide a template parameter to [push][luwra-push] to enforce pushing a specific
type. In most cases you are probably better off by letting the compiler infer the template
parameter.

## Reading Lua values
Simple retrieval of Lua values is done using [read&lt;T&gt;][luwra-read]. Consider the stack layout
from the previous example. This is how you would retrieve a value from the stack.

```c++
// Retrieve the integer at position 1
int value = luwra::read<int>(lua, 1);

// Similiar with a relative index
int value = luwra::read<int>(lua, -5);
```

## Read and type errors
What happens when a value mismatches the expected type or cannot be converted to it? Most
[Value][luwra-value] specializations use Lua's `luaL_check*` functions to retrieve the values from
the stack. This means that no exceptions will be thrown - instead the error handling is delegated to
the Lua VM. Have a look at the [error handling documentation][lua-errorhandling] for more
information.

[lua-cfunction]: http://www.lua.org/manual/5.3/manual.html#lua_CFunction
[lua-errorhandling]: http://www.lua.org/manual/5.3/manual.html#4.6
[luwra-value]: /reference/structluwra_1_1Value.html
[luwra-nativefunction]: /reference/structluwra_1_1NativeFunction.html
[luwra-table]: /reference/structluwra_1_1Table.html
[luwra-read]: /reference/namespaceluwra.html#a4fe4e574680cf54a0f8d958740eb90ab
[luwra-value-push]: /reference/structluwra_1_1Value.html#aa376d68285606c206562b822e8187384
[luwra-push]: /reference/namespaceluwra.html#ab6cf73d2416b43f1a90eb243a98cff5b
[luwra-membermap]: /reference/namespaceluwra.html#a2e12e40b973f0f56cb9a1dc91bef882a
