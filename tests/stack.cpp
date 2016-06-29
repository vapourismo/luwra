#include <catch.hpp>
#include <luwra.hpp>

static
int test_function_1(int a, int b) {
	return a - b;
}

static
int test_function_2(int a, int b, int c) {
	return a + b * c;
}

static
void test_function_3(int) {

}

static
void test_function_4(int, int) {

}

TEST_CASE("StackInteraction") {
	luwra::StateWrapper state;

	luwra::push(state, 1);
	luwra::push(state, 2);
	luwra::push(state, 4);

	// Redundant function
	luwra::apply(state, 1, test_function_3);
	luwra::apply(state, 1, test_function_4);

	// Absolute index
	REQUIRE(luwra::apply(state, 1, test_function_1) == -1);
	REQUIRE(luwra::apply(state, 1, [](int a, int b) -> int { return a - b; }) == -1);
	REQUIRE(luwra::apply(state, 2, [](int a, int b) -> int { return a + b; }) == 6);
	REQUIRE(luwra::apply(state, 1, test_function_2) == 9);

	// Relative index
	REQUIRE(luwra::apply(state, -2, test_function_1) == -2);
	REQUIRE(luwra::apply(state, -3, test_function_1) == -1);
	REQUIRE(luwra::apply(state, -3, test_function_2) == 9);
	REQUIRE(luwra::apply(state, -3, [](int a, int b, int c) -> int { return a - b + c; }) == 3);
}
