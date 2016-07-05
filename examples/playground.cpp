#include <luwra.hpp>
#include <iostream>
#include <string>

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

	push(state, 1337);
	push(state, 13.37);
	push(state, "Hello World");
	push(state, A(1337));

	int i = read(state, 1);
	double d = read(state, 2);
	std::string s = read(state, 3);
	A& u = read(state, 4);

	std::cout << i << std::endl;
	std::cout << d << std::endl;
	std::cout << s << std::endl;
	std::cout << u.foo << std::endl;

	u.foo = 0;

	apply(state, 4, [](A x) {
		std::cout << x.foo << std::endl;
	});

	return 0;
}
