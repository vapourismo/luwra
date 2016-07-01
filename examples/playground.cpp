#include <luwra.hpp>
#include <iostream>

using namespace luwra;

int main() {
	StateWrapper state;

	push(state, 37.13);
	push(state, 13.37);

	double r = apply(state, 1, [](double a, double b) {
		return a + b;
	});

	std::cout << r << std::endl;

	return 0;
}
