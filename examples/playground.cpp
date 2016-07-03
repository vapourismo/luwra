#include <luwra.hpp>
#include <iostream>

using namespace luwra;

struct A {
	int foo;

	A(int foo): foo(foo) {}

	A __add(const A& other) {
		return A(foo + other.foo);
	}
};

int main() {
	StateWrapper state;

	registerUserType<A(int)>(
		state,
		"A",
		{LUWRA_MEMBER(A, foo)},
		{LUWRA_MEMBER(A, __add)}
	);

	if (state.runString("return A(13) + A(37)") == LUA_OK)
		std::cout << read<A&>(state, -1).foo << std::endl;
	else
		std::cerr << read<std::string>(state, -1) << std::endl;

	return 0;
}
