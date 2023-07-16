#include <luwra.hpp>

#include <iostream>

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

	void scale(double f) noexcept {
		x *= f;
		y *= f;
	}

	std::string __tostring() const noexcept {
		return "<Point(" + std::to_string(x) + ", " + std::to_string(y) + ")>";
	}
};

LUWRA_DEF_REGISTRY_NAME(Point, "Point")

int main() {
	luwra::StateWrapper state;
	state.loadStandardLibrary();

	// Register our user type.
	// This function also registers a garbage-collector hook and a string representation function.
	// Both can be overwritten using the third parameter, which lets you add custom meta methods.
	state.registerUserType<Point (double, double)>(
		// Constructor name
		"Point",
		// Methods which shall be availabe in the Lua user data, need to be declared here
		{
			LUWRA_MEMBER(Point, scale),
			LUWRA_MEMBER(Point, x),
			LUWRA_MEMBER(Point, y),
			{"magic", luwra::MemberMap {
				{"number", 1337},
				{"string", "Hello World"}
			}}
		},
		// Meta methods may be registered aswell
		{
			LUWRA_MEMBER(Point, __tostring)
		}
	);

	// Load Lua code
	const char* code = (
		// Instantiate type
		"local p = Point(13, 37)\n"
		"print('p =', p)\n"

		// Invoke 'scale' method
		"p:scale(2)\n"
		"print('p =', p)\n"

		// Access 'x' and 'y' property
		"print('p.x =', p:x())\n"
		"print('p.y =', p:y())\n"

		// Modify 'x' property
		"p:x(10)\n"
		"print('p.x =', p:x())\n"

		"print('magicNumber', p.magic.number)\n"
		"print('magicString', p.magic.string)"
	);

	// Invoke the attached script
	if (state.runString(code) != LUA_OK) {
		std::cerr << "An error occured: " << state.read<std::string>(-1) << std::endl;
		return 1;
	} else {
		return 0;
	}
}
