#include <luwra.hpp>
#include <iostream>
#include <string>

using namespace luwra;

void test(const Table& tbl) {
	int v = tbl["field"];
	std::cout << v << std::endl;

	tbl["field"] = 1338;
}

int main() {
	StateWrapper state;

	Table tbl(state);
	tbl["field"] = 1337;

	push(state, tbl);
	apply(state, test);

	int v = tbl["field"];
	std::cout << v << std::endl;

	return 0;
}
