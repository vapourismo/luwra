# User Types
A user type is a collection of class members bundled into a metatable. Before user types can be used
in Lua, you must register such metatable in Lua's registry.

The following examples work on this class:

```c++
struct Point {
    double x, y;

    Point(double x, double y):
        x(x), y(y)
    {
        std::cout << "Construct Point(" << x << ", " << y << ")" << std::endl;
    }

    ~Point() {
        std::cout << "Destruct Point(" << x << ", " << y << ")" << std::endl;
    }

    void scale(double f) {
        x *= f;
        y *= f;
    }

    std::string __tostring() {
        return "<Point(" + std::to_string(x) + ", " + std::to_string(y) + ")>";
    }
};
```

## Register user type with constructor
[registerUserType&lt;S&gt;](/reference/namespaceluwra.html#a683f6075862f8fc5cad0d76445f2f607) allows
you to register a metatable and constructor in the global namespace. The template parameter to
`registerUserType` is a signature in the form of `U(A...)` where `U` is your user type and `A...`
the parameter types to the constructor which you want to register.

By default, the function generates a garbage-collector hook and a string representation function.
If you add a `__gc` or `__tostring` meta method to your type, these auto-generated functions will be
overridden.

See this example:

```c++
luwra::registerUserType<Point(double, double)>(
    lua,

    // Constructor name
    "Point",

    // Methods need to be declared here
    {
        LUWRA_MEMBER(Point, scale),
        LUWRA_MEMBER(Point, x),
        LUWRA_MEMBER(Point, y)
    },

    // Meta methods may be registered aswell
    {
        LUWRA_MEMBER(Point, __tostring)
    }
);
```

Parameter 3 and 4 are instances of
[FieldVector](/reference/namespaceluwra.html#ac090722c6d5d6b88b31895aad64788c2). The `LUWRA_MEMBER` macro
generates a `std::pair<Pushable, Pushable>` expression.

```c++
LUWRA_MEMBER(Point, scale) === {"scale", LUWRA_WRAP(Point::scale)}
```

`Pushable` has an implicit constructor, which makes it convenient to add other types of fields:

```c++
luwra::registerUserType<Point(double, double)>(
    lua,

    // Constructor name
    "Point",

    // Methods need to be declared here
    {
        {"scale", LUWRA_WRAP(Point::scale)},
        {"x",     LUWRA_WRAP(Point::x)},
        {"y",     LUWRA_WRAP(Point::y)},
        {"magic", luwra::FieldVector {
            {"number", 1337},
            {"string", "Hello World"}
        }}
    },

    // Meta methods may be registered aswell
    {
        {"__tostring", LUWRA_WRAP(Point::__tostring)}
    }
);
```

## Register user type without constructor
To register only the metatable associated with a user type, simply omit the constructor parameters
and name from the call to `registerUserType`.

```c++
luwra::registerUserType<Point>(
    lua,

    // Methods need to be declared here
    {
        {"scale", LUWRA_WRAP(Point::scale)},
        {"x",     LUWRA_WRAP(Point::x)},
        {"y",     LUWRA_WRAP(Point::y)},
        {"magic", luwra::FieldVector {
            {"number", 1337},
            {"string", "Hello World"}
        }}
    },

    // Meta methods may be registered aswell
    {
        {"__tostring", LUWRA_WRAP(Point::__tostring)}
    }
);
```

It is still possible to provide a constructor using the `LUWRA_WRAP_CONSTRUCTOR` macro:

```c++
lua_CFunction ctor = LUWRA_WRAP_CONSTRUCTOR(Point, double, double);
luwra::setGlobal(lua, "Point", ctor);
```

## Usage in Lua
After you have registered your user type using one of the given methods, you can start using it in
Lua:

```lua
-- Instantiate 'Point'
local point = Point(13, 37)

-- Invoke 'scale' method
point:scale(1.5)

-- Convert to string via '__tostring' meta method
print(point)

-- Read properties 'x' and 'y'
print(point:x(), point:y())

-- Set property 'x'
point:x(point.magic.number)
```

## Manually constructing a user type
Provided you already registered your user type, one can create it from the C++ side aswell.
[construct&lt;U&gt;](/reference/namespaceluwra.html#af079dcca8e67d88e5cfdc7e8872cf5d7) provides this
functionality. Given the user type and constructor parameters, it will construct the user type on
top of the stack:

```c++
Point& my_point = luwra::construct<Point>(lua, 13.37, 73.31);

// Changes on C++ side will be visible in Lua
my_point.scale(2);
```

## Registry names
When registering the metatable for a user type, an automatically generated name will be used to
store it in the registry. When Luwra is used in a single executable or shared library, name
collisions should not happen. If your application consists of multiple seperate compiled units, it
is highly recommended to prevent name collisions by defining the `LUWRA_REGISTRY_PREFIX` macro
before including the Luwra headers. This macro changes the prefix for auto-generated registry names.

```c++
#define LUWRA_REGISTRY_PREFIX "MyProject#"
#include <luwra.hpp>
```

Another way to prevent collisons is to give each user type its individual registry name. This can be
done using the `LUWRA_DEF_REGISTRY_NAME` macro.

```c++
struct MyUserType {
    // ...
};

LUWRA_DEF_REGISTRY_NAME(MyUserType, "MyUserType")
```

Choosing this method will not prefix the registry name with the value of `LUWRA_REGISTRY_PREFIX`.