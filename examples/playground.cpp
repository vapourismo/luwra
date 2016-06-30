#include <luwra.hpp>
#include <iostream>

using namespace luwra::internal;

int main() {
	using T = typename experimental::_DropTypes<1, int, int>::Result;

	return 0;
}
