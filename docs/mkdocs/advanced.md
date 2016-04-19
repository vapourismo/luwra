# Operating on Lua values with C++ functions
Instead of extracting every Lua value seperately and pushing the result of your C++ function back
onto the stack again, you can use one of the following functions to make this process easier for you.

## Invoke a Callable with Lua values
The function [direct&lt;S&gt;](/reference/namespaceluwra.html#aa20e363f38b3ae5a168cf40365f5646a)
lets you specify a *stack signature* in order to extract the values and invoke a `Callable` with
them.

Consider the following:

```c++
string result = foo(luwra::read<string>(lua, n), luwra::read<int>(lua, n + 1));
```

It could be rewritting like this:

```c++
string result = luwra::direct<string(string, int)>(lua, n, foo);
```

**Note:** The result of `foo` is not pushed onto the stack. Except for the extraction of Lua values,
everything happens on the C++ side.

## Invoke a function with Lua values
[apply](/reference/namespaceluwra.html#a839077ddd9c3d0565a40c574bc8e9555) is similiar to
[direct](/reference/namespaceluwra.html#aa20e363f38b3ae5a168cf40365f5646a). The function `apply`
provides specific overloads for function pointers and function objects. Although `direct` works
with function pointers and function objects, it is often more convenient to use `apply` since it
allows the compiler to infer the *stack signature* without providing a template parameter.

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
