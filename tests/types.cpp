#include <catch.hpp>
#include <luwra.hpp>

#include <cstring>
#include <string>
#include <utility>
#include <type_traits>
#include <vector>
#include <list>
#include <initializer_list>

// Numbers are royally fucked. They might or might not be stored in a floating-point number, which
// makes testing for integer limits pointless.
//
// template <typename I>
// struct NumericTest {
// 	static
// 	void test(lua_State* state) {
// 		const I max_value = std::numeric_limits<I>::max();
// 		const I min_value = std::numeric_limits<I>::lowest();
// 		const I avg_value = (max_value + min_value) / 2;

// 		// Largest value
// 		CHECK((luwra::push(state, max_value) == 1));
// 		CHECK((luwra::read<I>(state, -1) == max_value));

// 		// Lowest value
// 		CHECK((luwra::push(state, min_value) == 1));
// 		CHECK((luwra::read<I>(state, -1) == min_value));

// 		// Average value
// 		CHECK((luwra::push(state, avg_value) == 1));
// 		CHECK((luwra::read<I>(state, -1) == avg_value));
// 	}
// };

// TEST_CASE("NumberLimits") {
// 	luwra::StateWrapper state;

// 	// Integer-based types
// 	NumericTest<signed char>::test(state);
// 	NumericTest<unsigned char>::test(state);
// 	NumericTest<signed short>::test(state);
// 	NumericTest<unsigned short>::test(state);
// 	NumericTest<signed int>::test(state);
// 	NumericTest<unsigned int>::test(state);
// 	NumericTest<signed long int>::test(state);
// 	NumericTest<unsigned long int>::test(state);
// 	NumericTest<signed long long int>::test(state);
// 	NumericTest<unsigned long long int>::test(state);

// 	// Number-based types
// 	NumericTest<float>::test(state);
// 	NumericTest<double>::test(state);
// 	NumericTest<long double>::test(state);
// }

TEST_CASE("Numbers") {
	luwra::StateWrapper state;

	luwra::push(state, 1337);
	luwra::push(state, 13.37);

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
	luwra::push(state, test_cstr);
	luwra::push(state, test_str);

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

	// Tuples aren't ordinarily pushable. But you can use them as return value.
	size_t ret =
		state.map<std::tuple<int, std::string, float>()>(
			[&]() { return std::make_tuple(a, b, c); }
		);

	REQUIRE(ret == 3);
	REQUIRE(lua_gettop(state) == 3);

	REQUIRE(state.read<int>(-3) == a);
	REQUIRE(state.read<std::string>(-2) == b);
	REQUIRE(state.read<float>(-1) == c);

	ret =
		state.map<std::tuple<int, std::string, float, std::tuple<int, std::string, float>>()>(
			[&]() { return std::make_tuple(a, b, c, std::make_tuple(a, b, c)); }
		);

	REQUIRE(ret == 6);
	REQUIRE(lua_gettop(state) == 9);

	REQUIRE(luwra::read<int>(state, -6) == a);
	REQUIRE(luwra::read<std::string>(state, -5) == b);
	REQUIRE(luwra::read<float>(state, -4) == c);
	REQUIRE(luwra::read<int>(state, -3) == a);
	REQUIRE(luwra::read<std::string>(state, -2) == b);
	REQUIRE(luwra::read<float>(state, -1) == c);
}

TEST_CASE("Boolean") {
	luwra::StateWrapper state;

	luwra::push(state, true);
	REQUIRE(luwra::read<bool>(state, -1) == true);

	luwra::push(state, false);
	REQUIRE(luwra::read<bool>(state, -1) == false);
}

TEST_CASE("Pushable") {
	luwra::StateWrapper state;

	luwra::Pushable pushable(1337);
	luwra::push(state, pushable);

	REQUIRE(luwra::read<int>(state, -1) == 1337);
}

TEST_CASE("Value<vector>") {
	luwra::StateWrapper state;
	state.loadStandardLibrary();

	std::vector<int> v {1, 2, 3, 4, 5};
	luwra::push(state, v);

	state["v"] = v;

	REQUIRE(state.runString(
		"x = 0\n"
		"for i, j in ipairs(v) do x = x + i + j end\n"
		"return x"
	) == LUA_OK);

	REQUIRE(luwra::read<int>(state, -1) == 30);
}

TEST_CASE("Value<list>") {
	luwra::StateWrapper state;
	state.loadStandardLibrary();

	std::list<int> v {1, 2, 3, 4, 5};
	luwra::push(state, v);

	state["v"] = v;

	REQUIRE(state.runString(
		"x = 0\n"
		"for i, j in ipairs(v) do x = x + i + j end\n"
		"return x"
	) == LUA_OK);

	REQUIRE(luwra::read<int>(state, -1) == 30);
}

TEST_CASE("Value<map>") {
	luwra::StateWrapper state;

	std::map<luwra::Pushable, luwra::Pushable> v {
		{"hello", 13},
		{37, "world"}
	};
	luwra::push(state, v);

	state["v"] = v;

	REQUIRE(state.runString("return v.hello, v[37]") == LUA_OK);

	REQUIRE(luwra::read<int>(state, -2) == 13);
	REQUIRE(luwra::read<std::string>(state, -1) == "world");
}
