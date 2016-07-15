#include <iostream>
#include <memory>
#include <luwra.hpp>

using namespace luwra;

int main() {
	StateWrapper state;

	state.push(13);

	{
		StateWrapper stateCopy(state);
		state.push(37);

		std::cout << stateCopy.state.use_count() << std::endl;
	}

	int a = state.read(1);
	int b = state.read(2);

	std::cout << a << std::endl << b << std::endl;

	return 0;
}
