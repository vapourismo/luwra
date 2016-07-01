#include <luwra.hpp>
#include <iostream>

using namespace luwra;

int main() {
	StateWrapper state;

	push(state, 37.13);
	push(state, 13.37);

	map(state, 1, [](double a, double b, double c) {
		return a + b + c;
	}, -0.5);

	std::cout << read<double>(state, -1) << std::endl;

	return 0;
}
