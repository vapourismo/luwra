#include <catch.hpp>
#include <luwra.hpp>

#include <cstring>
#include <string>
#include <utility>
#include <type_traits>

#if LUA_VERSION_NUM >= 503

template <typename I>
struct NumericTest {
	static
	void test(lua_State* state) {
		const I max_value = std::numeric_limits<I>::max();
		const I min_value = std::numeric_limits<I>::lowest();
		const I avg_value = (max_value + min_value) / 2;

		// Largest value
		CHECK((luwra::push(state, max_value) == 1));
		CHECK((luwra::read<I>(state, -1) == max_value));

		// Lowest value
		CHECK((luwra::push(state, min_value) == 1));
		CHECK((luwra::read<I>(state, -1) == min_value));

		// Average value
		CHECK((luwra::push(state, avg_value) == 1));
		CHECK((luwra::read<I>(state, -1) == avg_value));
	}
};

TEST_CASE("NumberLimits") {
	luwra::StateWrapper state;

	// Integer-based types
	NumericTest<signed char>::test(state);
	NumericTest<unsigned char>::test(state);
	NumericTest<signed short>::test(state);
	NumericTest<unsigned short>::test(state);
	NumericTest<signed int>::test(state);
	NumericTest<unsigned int>::test(state);
	NumericTest<signed long int>::test(state);
	NumericTest<unsigned long int>::test(state);
	NumericTest<signed long long int>::test(state);
	NumericTest<unsigned long long int>::test(state);

	// Number-based types
	NumericTest<float>::test(state);
	NumericTest<double>::test(state);
	NumericTest<long double>::test(state);
}

#endif /* LUA_VERSION_NUM >= 503 */

TEST_CASE("Numbers") {
	luwra::StateWrapper state;

	REQUIRE(luwra::push(state, 1337) == 1);
	REQUIRE(luwra::push(state, 13.37) == 1);

	REQUIRE(luwra::read<int>(state, -2) == 1337);
	REQUIRE(luwra::read<float>(state, -1) == 13.37f);
}

TEST_CASE("Strings") {
	luwra::StateWrapper state;

	const char* test_cstr = "Luwra Test String";
	std::string test_str(test_cstr);

	// Safety first
	REQUIRE(test_str == test_cstr);

	// Push both strings
	REQUIRE(luwra::push(state, test_cstr) == 1);
	REQUIRE(luwra::push(state, test_str) == 1);

	// They must be equal to Lua
	REQUIRE(luwra::equal(state, -1, -2));

	// Extraction as C string must not change the string's value
	const char* l_cstr1 = luwra::read<const char*>(state, -1);
	const char* l_cstr2 = luwra::read<const char*>(state, -2);

	REQUIRE(std::strcmp(test_cstr,        l_cstr1) == 0);
	REQUIRE(std::strcmp(test_cstr,        l_cstr2) == 0);
	REQUIRE(std::strcmp(test_str.c_str(), l_cstr1) == 0);
	REQUIRE(std::strcmp(test_str.c_str(), l_cstr2) == 0);
	REQUIRE(std::strcmp(l_cstr1,          l_cstr2) == 0);

	// Extraction as C++ string must not change the string's value
	std::string l_str1 = luwra::read<std::string>(state, -1);
	std::string l_str2 = luwra::read<std::string>(state, -2);

	REQUIRE(l_str1   == test_cstr);
	REQUIRE(l_str2   == test_cstr);
	REQUIRE(test_str == l_str1);
	REQUIRE(test_str == l_str2);
	REQUIRE(l_str1   == l_str2);
}

TEST_CASE("Tuples") {
	luwra::StateWrapper state;

	int a = 13;
	std::string b("Hello");
	float c = 0.37;

	// Push normal tuple
	auto tuple = std::make_tuple(a, b, c);
	REQUIRE(luwra::push(state, tuple) == 3);

	// Push nested tuple
	auto tuple_nested = std::make_tuple(a, b, c, tuple);
	REQUIRE(luwra::push(state, tuple_nested) == 6);
}

TEST_CASE("Boolean") {
	luwra::StateWrapper state;

	bool value = true;

	REQUIRE(luwra::push(state, value) == 1);
	REQUIRE(luwra::read<bool>(state, -1) == value);
}

TEST_CASE("Pushable") {
	luwra::StateWrapper state;

	luwra::Pushable pushable(1337);
	REQUIRE(luwra::push(state, pushable) == 1);

	REQUIRE(luwra::read<int>(state, -1) == 1337);
}
