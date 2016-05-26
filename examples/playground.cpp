#include <luwra.hpp>
#include <iostream>

using namespace luwra;

struct A {
	A() {
		std::cout << "A()" << std::endl;
	}

	A(const A&) {
		std::cout << "A(const A&)" << std::endl;
	}

	A(A&&) {
		std::cout << "A(A&&)" << std::endl;
	}
};

int main() {
	StateWrapper state;

	// A a;

	const int i = 1338;
	Pushable m(i);

	push(state, m);

	return 0;
}
