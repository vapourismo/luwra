#include "catch.hpp"

#include <lua.hpp>
#include <luwra.hpp>


#include <cstring>
#include <string>
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

		// Lowest value
		REQUIRE(luwra::Value<I>::push(state, min_value) == 1);
		REQUIRE(luwra::Value<I>::read(state, -1) == min_value);

		// Average value
		REQUIRE(luwra::Value<I>::push(state, avg_value) == 1);
		REQUIRE(luwra::Value<I>::read(state, -1) == avg_value);
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

TEST_CASE("Test Value specialization for string types", "types_string") {
	lua_State* state = luaL_newstate();

	const char* test_cstr = "Luwra Test String";
	std::string test_str(test_cstr);

	// Safety first
	REQUIRE(test_str == test_cstr);

	// Push both strings
	REQUIRE(luwra::Value<const char*>::push(state, test_cstr) == 1);
	REQUIRE(luwra::Value<std::string>::push(state, test_str) == 1);

	// They must be equal to Lua
	REQUIRE(lua_compare(state, -1, -2, LUA_OPEQ));

	// Extraction as C string must not change the string's value
	const char* l_cstr1 = luwra::Value<const char*>::read(state, -1);
	const char* l_cstr2 = luwra::Value<const char*>::read(state, -2);

	REQUIRE(std::strcmp(test_cstr,        l_cstr1) == 0);
	REQUIRE(std::strcmp(test_cstr,        l_cstr2) == 0);
	REQUIRE(std::strcmp(test_str.c_str(), l_cstr1) == 0);
	REQUIRE(std::strcmp(test_str.c_str(), l_cstr2) == 0);
	REQUIRE(std::strcmp(l_cstr1,          l_cstr2) == 0);

	// Extraction as C++ string must not change the string's value
	std::string l_str1 = luwra::Value<std::string>::read(state, -1);
	std::string l_str2 = luwra::Value<std::string>::read(state, -2);

	REQUIRE(l_str1   == test_cstr);
	REQUIRE(l_str2   == test_cstr);
	REQUIRE(test_str == l_str1);
	REQUIRE(test_str == l_str2);
	REQUIRE(l_str1   == l_str2);

	lua_close(state);
}

TEST_CASE("Test Value specialization for tuples", "types_tuple") {
	lua_State* state = luaL_newstate();

	int a = 13;
	std::string b("Hello");
	float c = 0.37;

	// Push normal tuple
	auto tuple = std::make_tuple(a, b, c);
	REQUIRE(luwra::Value<decltype(tuple)>::push(state, tuple) == 3);

	// Push nested tuple
	auto tuple_nested = std::make_tuple(a, b, c, tuple);
	REQUIRE(luwra::Value<decltype(tuple_nested)>::push(state, tuple_nested) == 6);

	lua_close(state);
}

TEST_CASE("Test Value specialization for bool") {
	lua_State* state = luaL_newstate();

	bool value = true;

	REQUIRE(luwra::Value<bool>::push(state, value) == 1);
	REQUIRE(luwra::Value<bool>::read(state, -1) == value);

	lua_close(state);
}
