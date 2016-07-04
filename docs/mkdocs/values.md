# Values
A fundamental part of Luwra is the [Value][luwra-value] template which acts as a type class. It is
used to define `read` and `push` behavior for certain types.

A type `T` is considered **readable** if a function `T Value<T>::read(State*, int)` exists. The
return type of that function need not be `T`, instead it can be anything that is convertible to `T`.

A type `T` is considered **pushable** if a funtion `void Value<T>::push(State*, T)` exists. In order
to avoid unnecessary copying, the second parameter type may also be `const T&`.

## Default Implementations
Several useful specializations are provided out of the box.

C++ type                       | Pushable | Readable | Lua type
-------------------------------|----------|----------|----------------------------
bool                           | yes      | yes      | boolean
signed char                    | yes      | yes      | number (integer since 5.3)
signed short                   | yes      | yes      | number (integer since 5.3)
signed int                     | yes      | yes      | number (integer since 5.3)
signed long int                | yes      | yes      | number (integer since 5.3)
signed long long int           | yes      | yes      | number (integer since 5.3)
unsigned char                  | yes      | yes      | number (integer since 5.3)
unsigned short                 | yes      | yes      | number (integer since 5.3)
unsigned int                   | yes      | yes      | number (integer since 5.3)
unsigned long int              | yes      | yes      | number (integer since 5.3)
unsigned long long int         | yes      | yes      | number (integer since 5.3)
float                          | yes      | yes      | number
double                         | yes      | yes      | number
long double                    | yes      | yes      | number
const char*                    | yes      | yes      | string
std::string                    | yes      | yes      | string
std::nullptr_t                 | yes      | yes      | nil
std::vector&lt;T&gt;           | yes      | no       | table
std::list&lt;T&gt;             | yes      | no       | table
std::map&lt;K, V&gt;           | yes      | no       | table
[lua_CFunction][lua-cfunction] | yes      | no       | function
[Function][luwra-function]     | yes      | yes      | function, table or userdata
[Table][luwra-table]           | yes      | yes      | table

**Note:** Some numeric types have a different size than their matching Lua type - they will be
truncated during `read` or `push` operations.

## Arbitrary and User Types
[Value][luwra-value] provides a catch-all generalization for types that do not have a specialization
of [Value][luwra-value]. Although these types are not known to Luwra, they are pushable and
readable.

Instances of these so-called **user types** are constructed on the Lua stack as a
[full userdata][lua-userdata]. Additionally, a metatable that is specific to the given user type is
attached to the userdata. This metatable allows us to check whether a userdata is an instance of a
specific user type.

`push` operations always copy or move instances of the user type onto the stack, whereas `read`
operations always reference the user type value on the stack.

By default, the metatables that are attached to the user type values are empty. Because of this,
they provide no functionality to Lua and are never destructed (underlying storage is just freed).
You can change this behavior, read more in the [User Types](/usertypes) section.

## Extending Value
You can customize the `read` and `push` behavior for your own type `T`. Simply modify the following
snippet and insert it outside of any namespace.

```c++
namespace luwra {
	template <>
	struct Value<T> {
		static inline
		T read(State* state, int index) {
			return /* Return the instance of T that you have read at the given index */;
		}

		static inline
		void push(State* state, const T& value) {
			// Push the given value on top of the stack
		}
	};
}
```

# Return Values
The template [ReturnValues][luwra-returnvalue] extends the `push` functionality on top of
[Value][luwra-value] by allowing more complex types to be pushed onto the stack.

[ReturnValues][luwra-returnvalue] makes it possible to use `std::tuple<...>` or `std::pair<...>` as
return type of user-provided functions in order to mimic the ability of Lua functions to return
multiple values at once.

[luwra-value]: /reference/structluwra_1_1Value.html
[lua-cfunction]: http://www.lua.org/manual/5.3/manual.html#lua_CFunction
[luwra-function]: /reference/structluwra_1_1Function.html
[luwra-table]: /reference/structluwra_1_1Table.html
[lua-userdata]: http://www.lua.org/manual/5.3/manual.html#lua_newuserdata
[luwra-returnvalue]: /reference/structluwra_1_1ReturnValue.html
