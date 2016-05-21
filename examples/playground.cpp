#include <luwra.hpp>

#include <string>
#include <iostream>
#include <functional>

using namespace luwra;

static int foo(int a, int b) {
	return a + b;
}

int main() {
	StateWrapper state;
	push(state, 13);
	push(state, 37);

	std::function<int(int, int)> bar(&foo);
	std::function<int(int, int)> baz([](int a, int b) -> int {
		return a + b;
	});

	std::cout << apply(state, 1, &foo) << std::endl;
	std::cout << apply(state, 1, bar) << std::endl;
	std::cout << apply(state, 1, baz) << std::endl;
	std::cout << apply(state, 1, [](int a, int b) -> int {
		return a + b;
	}) << std::endl;

	return 0;
}
