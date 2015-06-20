#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>

struct Point {
	// Luwra needs the MetatableName field in order to add a meta table to the Lua registry
	static constexpr
	const char* MetatableName = "Point";

	lua_Number x, y;

	Point(lua_Number x, lua_Number y):
		x(x), y(y)
	{
		std::cout << "Construct Point(" << x << ", " << y << ")" << std::endl;
	}

	~Point() {
		std::cout << "Destruct Point(" << x << ", " << y << ")" << std::endl;
	}

	void scale(lua_Number f) {
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

	// Register the metatable for our Point type.
	// This function also registers a garbage-collector hook and a string representation function.
	// Both can be overwritten using the third parameter, which lets you add custom meta methods.
	luwra::register_type_metatable<Point>(
		state,
		// Methods which shall be availabe in the Lua user data, need to be declared here
		{
			{"scale", luwra::WrapMethod<Point, void(lua_Number), &Point::scale>},
		},
		// Meta methods may be registered aswell
		{
			{"__tostring", luwra::WrapMethod<Point, std::string(), &Point::toString>}
		}
	);

	// What's left, is register a constructor for our type.
	// We have to specify which parameters our constructor takes, because there might be more than
	// one constructor to deal with.
	auto wrapped_ctor = luwra::WrapConstructor<Point, lua_Number, lua_Number>;
	lua_pushcfunction(state, wrapped_ctor);
	lua_setglobal(state, "Point");

	// Invoke the attached script
	if (luaL_loadfile(state, "methods.lua") != 0 || lua_pcall(state, 0, LUA_MULTRET, 0) != 0) {
		const char* error_msg = lua_tostring(state, -1);
		std::cerr << "An error occured: " << error_msg << std::endl;

		lua_close(state);
		return 1;
	} else {
		lua_close(state);
		return 0;
	}
}
