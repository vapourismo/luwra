# Stack Interaction
Luwra provides several easy ways to interact with the Lua virtual stack.

## Pushing Values
To get values onto the stack, use [push][luwra-push].

```c++
luwra::push(state, 1337);
```
```c++
luwra::push(state, 13.37);
```
```c++
luwra::push(state, "Hello World");
```
```c++
luwra::push(state, MyUserType("Hello", 5));
```

You can also push them all at once.

```c++
luwra::push(state, 1337, 13.37, "Hello World", MyUserType("Hello", 5));
```

## Reading Values
Reading values works with [read][luwra-read]. Assuming the stack has been prepared as it has been in
the previous section, you can extract the values like so.

```c++
int i = luwra::read<int>(state, 1);
```
```c++
double d = luwra::read<double>(state, 2);
```
```c++
std::string s = luwra::read<std::string>(state, 3);
```
```c++
MyUserType& u = luwra::read<MyUserType>(state, 4);
```

You can also let C++ infer the types for you.

```c++
int i = luwra::read(state, 1);
```
```c++
double d = luwra::read(state, 2);
```
```c++
std::string s = luwra::read(state, 3);
```
```c++
MyUserType& u = luwra::read(state, 4);
```

**Note:** Type inference does not work with every compiler. Particularly GCC before version 4.9.2 is
affected by this problem.

## Invoke Callables with Stack Values
[apply][luwra-apply] is a function that retrieves values from the stack in order to invoke a given
`Callable`. The types of values on the stack are deduced from the parameter types to the `Callable`.

```c++
std::string substring(const std::string& str, size_t len) {
	return str.substr(0, len);
}
```
```c++
luwra::push(state, "Hello World");
luwra::push(state, 5);

// Retrieve values and invoke 'substring'.
std::string result = luwra::apply(state, 1, substring);

// The string is essentially equal to the following.
std::result result = substring(luwra::read(state, 1), luwra::read(state, 2));

// You can also provide your own arguments before the stack values.
std::string result = luwra::apply(state, 2, substring, "My Own String");

// Alternatively
std::string result = substring("My Own String", luwra::read(state, 2));
```

If you wish to return the result of your function to the stack, simply use [map][luwra-map].

```c++
luwra::map(state, 1, substring);
std::string result = luwra::read(state, -1);
```

You can also provide function objects or lambdas to [apply][luwra-apply] and [map][luwra-map].

```c++
std::string result = luwra::apply(state, 1, [](const std::string& str, size_t len) {
	return str.substr(0, len);
});
```

[luwra-push]: /reference/namespaceluwra.html#a1335e9b5358d53d947b5e371cccc5dfa
[luwra-push-2]: /reference/namespaceluwra.html#a593a361b978a74274664828f0f0fe176
[luwra-read]: /reference/namespaceluwra.html#a3c459338d5baea22fc1ea7f6184ae14d
[luwra-apply]: /reference/namespaceluwra.html#aa9fee091aaa7a0073108054e316ffb28
[luwra-map]: /reference/namespaceluwra.html#a79e1ea58201d5ceec4097570c5f0237a
