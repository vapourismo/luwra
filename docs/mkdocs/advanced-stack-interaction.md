# Advanced stack interaction
Instead of extracting every Lua value seperately and pushing the result of your C++ function back
onto the stack again, you can use one of the following functions to make this process easier for
you.

## Manual stack layout
The function [direct][luwra-direct] lets you specify a *stack signature* in order to extract the
values and invoke a `Callable` with them.

### Without returning to Lua
Consider the following:

```c++
string result = foo(luwra::read<string>(lua, n), luwra::read<int>(lua, n + 1));
```

It could be rewritting like this:

```c++
string result = luwra::direct<string(string, int)>(lua, n, foo);
```

This will read all the required values off the stack, invoke `foo` with them and return its value to
you.

### Returning values to the stack
An alternative to [direct][luwra-direct] is [map][luwra-map]. It does exactly the same, with the
exception that it returns the resulting value back to the Lua stack.

```c++
luwra::map<string(string, int)>(lua, n, foo);
```

## Invoke a function with Lua values
[apply][luwra-apply] is similiar to [direct][luwra-direct]. It differs from `direct` by providing
specific overloads for function pointers and function objects. Although `direct` works with function pointers
and function objects, it is often more convenient to use `apply` since it allows the compiler to
infer the *stack signature* without providing a template parameter.

Provided a function `foo` which has been declared as used in the example above:

```c++
string foo(string bar, int baz);

// ... or with a function object
function<string(string, int)> foo = /* magic */;
```

One would use `foo` like this:

```c++
string result = luwra::apply(lua, n, foo);
```

It also works with Lambdas, because they are function objects aswell.

```c++
string result = luwra::apply(lua, n, [](string a, int b) -> string {
	// Magic
});
```

[luwra-direct]: /reference/namespaceluwra.html#aa20e363f38b3ae5a168cf40365f5646a
[luwra-apply]: /reference/namespaceluwra.html#a839077ddd9c3d0565a40c574bc8e9555
[luwra-map]: /reference/namespaceluwra.html#a9f24fc70cb48531cf1e3da6a3a741971
