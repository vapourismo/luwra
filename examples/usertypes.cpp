#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>

using namespace luwra;

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

	std::string toString() {
		return "<Point(" + std::to_string(x) + ", " + std::to_string(y) + ")>";
	}
};

int main() {
	lua_State* state = luaL_newstate();
	luaL_openlibs(state);

	// Register our user type.
	// This function also registers a garbage-collector hook and a string representation function.
	// Both can be overwritten using the third parameter, which lets you add custom meta methods.
	RegisterUserType<Point>(
		state,
		// Methods which shall be availabe in the Lua user data, need to be declared here
		{
			{"scale", WrapMethod<Point, void(double), &Point::scale>},
			{"x",     WrapProperty<Point, double, &Point::x>},
			{"y",     WrapProperty<Point, double, &Point::y>}
		},
		// Meta methods may be registered aswell
		{
			{"__tostring", WrapMethod<Point, std::string(), &Point::toString>}
		}
	);

	// What's left, is registering a constructor for our type.
	// We have to specify which parameters our constructor takes, because there might be more than
	// one constructor to deal with.
	Push(state, WrapConstructor<Point, double, double>);
	lua_setglobal(state, "Point");

	// Invoke the attached script
	if (luaL_loadfile(state, "usertypes.lua") != 0 || lua_pcall(state, 0, LUA_MULTRET, 0) != 0) {
		const char* error_msg = lua_tostring(state, -1);
		std::cerr << "An error occured: " << error_msg << std::endl;

		lua_close(state);
		return 1;
	} else {
		lua_close(state);
		return 0;
	}
}
