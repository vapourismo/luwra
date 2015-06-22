#include "catch.hpp"

#include <lua.hpp>
#include <luwra.hpp>

#include <iostream>
#include <utility>
#include <type_traits>

template <typename I>
struct NumericTest {
	static
	void test(lua_State* state) {
		const I max_value = std::numeric_limits<I>::max();
		const I min_value = std::numeric_limits<I>::lowest();
		const I avg_value = (max_value + min_value) / 2;

		// Largest value
		REQUIRE(luwra::Value<I>::push(state, max_value) == 1);
		REQUIRE(luwra::Value<I>::read(state, -1) == max_value);
		lua_pop(state, 1);

		// Lowest value
		REQUIRE(luwra::Value<I>::push(state, min_value) == 1);
		REQUIRE(luwra::Value<I>::read(state, -1) == min_value);
		lua_pop(state, 1);

		// Average value
		REQUIRE(luwra::Value<I>::push(state, avg_value) == 1);
		REQUIRE(luwra::Value<I>::read(state, -1) == avg_value);
		lua_pop(state, 1);
	}
};

struct TautologyTest {
	static
	void test(lua_State*) {}
};

template <typename B, typename I>
using SelectNumericTest =
	typename std::conditional<
		luwra::internal::NumericContainedValueBase<I, B>::qualifies,
		NumericTest<I>,
		TautologyTest
	>::type;

TEST_CASE("Test Value specialization for numeric C types", "types_numeric") {
	lua_State* state = luaL_newstate();

	// Integer-based types
	SelectNumericTest<lua_Integer, signed short>::test(state);
	SelectNumericTest<lua_Integer, unsigned short>::test(state);
	SelectNumericTest<lua_Integer, signed int>::test(state);
	SelectNumericTest<lua_Integer, unsigned int>::test(state);
	SelectNumericTest<lua_Integer, signed long int>::test(state);
	SelectNumericTest<lua_Integer, unsigned long int>::test(state);
	SelectNumericTest<lua_Integer, signed long long int>::test(state);
	SelectNumericTest<lua_Integer, unsigned long long int>::test(state);

	// Number-based types
	SelectNumericTest<lua_Number, float>::test(state);
	SelectNumericTest<lua_Number, double>::test(state);
	SelectNumericTest<lua_Number, long double>::test(state);

	lua_close(state);
}
