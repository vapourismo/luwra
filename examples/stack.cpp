#include <luwra.hpp>

#include <iostream>

static
double sum3(int a, int b, double c) {
	return (a + b) * c;
}

int main() {
	luwra::StateWrapper state;
	state.loadStandardLibrary();

	// Build stack
	state.push(13);
	state.push(37);
	state.push(42.2);

	// Each value can be retrieved individually.
	std::cout << "a = " << state.read<int>(1) << std::endl;
	std::cout << "b = " << state.read<int>(2) << std::endl;
	std::cout << "c = " << state.read<double>(3) << std::endl;

	// ... which is a little cumbersome. Instead we might apply a fitting function to our stack.
	std::cout << "(a + b) * c = "
	          << state.apply(sum3)
	          << std::endl;

	return 0;
}
