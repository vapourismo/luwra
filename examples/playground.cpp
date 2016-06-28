#include <luwra.hpp>
#include <iostream>
#include <list>

int main() {
	luwra::StateWrapper state;

	std::tuple<int, float, int> test(13, 13.37, 37);
	luwra::internal::SpecialValuePusher<std::tuple<int, float, int>>::push(state, test);

	std::cout << state.read<int>(-3) << std::endl
	          << state.read<float>(-2) << std::endl
	          << state.read<int>(-1) << std::endl;

	return 0;
}
