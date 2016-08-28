#include <iostream>
#include <memory>
#include <luwra.hpp>

using namespace luwra;

int main() {
	StateWrapper state;

	push(state, 1337);
	push(state, 7731, 1337);

	std::cout << read<int>(state, -3) << std::endl
	          << read<int>(state, -2) << std::endl
	          << read<int>(state, -1) << std::endl;

	return 0;
}
