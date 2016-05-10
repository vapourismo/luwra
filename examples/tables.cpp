#include <luwra.hpp>

#include <string>
#include <iostream>

using namespace luwra;

int main() {
	StateWrapper state;

	state["t1"] = FieldVector {};

	for (int i = 0; i < 10000; i++) {
		state["t1"]["value"] = i;
		int j = state["t1"]["value"];

		if (j != i) {
			return 1;
		}
	}

	return 0;
}
