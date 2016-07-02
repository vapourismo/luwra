#include <luwra.hpp>
#include <iostream>

using namespace luwra;

int main() {
	StateWrapper state;

	push(state, 37.13);
	push(state, 13.37);

	std::cout << equal(state, {1, 2}) << std::endl;

	return 0;
}
