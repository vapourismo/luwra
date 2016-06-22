#include <luwra.hpp>
#include <iostream>
#include <string>

using namespace luwra;

struct A {
	void f() {
		std::cout << "Hello World" << std::endl;
	}
};

struct B: A {};

int main() {
	StateWrapper state;
	state.loadStandardLibrary();

	registerUserType<A()>(state, "A", {LUWRA_MEMBER(A, f)});
	registerUserType<B()>(state, "B", {LUWRA_MEMBER(B, f)});

	if (state.runString("B():f()") != LUA_OK)
		std::cerr << read<std::string>(state, -1) << std::endl;

	return 0;
}
