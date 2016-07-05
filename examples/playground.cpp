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

	push(state, MemberMap {
		{"a", A(1337)}
	});

	Table table(state, -1);

	A& a1 = table["a"];
	const A& a2 = table["a"];
	A a3 = table["a"];

	std::cout << a1.foo << std::endl;
	std::cout << a2.foo << std::endl;
	std::cout << a3.foo << std::endl;

	return 0;
}
