#include <catch.hpp>
#include <luwra.hpp>

#include <cstring>
#include <string>
#include <utility>
#include <type_traits>
#include <vector>
#include <list>
#include <initializer_list>

#include <iostream>
#include <memory>

struct A {
	std::unique_ptr<size_t> copyCounter;
	std::unique_ptr<size_t> moveCounter;

	A():
		copyCounter(new size_t(0)),
		moveCounter(new size_t(0))
	{}

	A(const A& a):
		copyCounter(new size_t(0)),
		moveCounter(new size_t(0))
	 {
		(*a.copyCounter)++;
	}

	A(A&& a):
		copyCounter(new size_t(0)),
		moveCounter(new size_t(0))
	 {
		(*a.moveCounter)++;
	}
};

TEST_CASE("push") {
	luwra::StateWrapper state;

	SECTION("perfect forwarding") {
		A onlyCopy;
		luwra::push(state, onlyCopy);
		REQUIRE(*onlyCopy.copyCounter == 1);
		REQUIRE(*onlyCopy.moveCounter == 0);

		A onlyMove;
		luwra::push(state, std::move(onlyMove));
		REQUIRE(*onlyMove.copyCounter == 0);
		REQUIRE(*onlyMove.moveCounter == 1);
	}
}

TEST_CASE("Value<nullptr_t>") {
	luwra::StateWrapper state;

	SECTION("read") {
		lua_pushnil(state);
		REQUIRE(luwra::read<std::nullptr_t>(state, -1) == nullptr);
	}

	SECTION("push") {
		luwra::push(state, nullptr);
		REQUIRE(lua_type(state, -1) == LUA_TNIL);
		REQUIRE(luwra::read<std::nullptr_t>(state, -1) == nullptr);
	}
}

TEST_CASE("Value<State*>") {
	luwra::StateWrapper state;

	SECTION("read") {
		luwra::State* thread = lua_newthread(state);
		REQUIRE(luwra::read<luwra::State*>(state, -1) == thread);
	}
}

TEST_CASE("Value<Integer>") {
	luwra::StateWrapper state;
	luwra::Integer value = 1337;

	SECTION("read") {
		lua_pushinteger(state, value);
		REQUIRE(luwra::read<luwra::Integer>(state, -1) == value);
	}

	SECTION("push") {
		luwra::push(state, value);
		REQUIRE(lua_type(state, -1) == LUA_TNUMBER);
		REQUIRE(luwra::read<luwra::Integer>(state, -1) == value);
	}
}

TEST_CASE("Value<Number>") {
	luwra::StateWrapper state;
	luwra::Number value = 13.37;

	SECTION("read") {
		lua_pushnumber(state, value);
		REQUIRE(luwra::read<luwra::Number>(state, -1) == value);
	}

	SECTION("push") {
		luwra::push(state, value);
		REQUIRE(lua_type(state, -1) == LUA_TNUMBER);
		REQUIRE(luwra::read<luwra::Number>(state, -1) == value);
	}
}

TEST_CASE("Value<const char*>") {
	luwra::StateWrapper state;
	const char* value = "Hello World";

	SECTION("read") {
		lua_pushstring(state, value);
		REQUIRE(strcmp(luwra::read<const char*>(state, -1), value) == 0);
	}

	SECTION("push") {
		luwra::push(state, value);
		REQUIRE(lua_type(state, -1) == LUA_TSTRING);
		REQUIRE(strcmp(luwra::read<const char*>(state, -1), value) == 0);
	}
}

TEST_CASE("Value<string>") {
	luwra::StateWrapper state;
	std::string value("Hello World");

	SECTION("read") {
		lua_pushstring(state, value.c_str());
		REQUIRE(luwra::read<std::string>(state, -1) == value);
	}

	SECTION("push") {
		luwra::push(state, value);
		REQUIRE(lua_type(state, -1) == LUA_TSTRING);
		REQUIRE(luwra::read<std::string>(state, -1) == value);
	}
}


// TODO: Move this somewhere else.
TEST_CASE("Tuples") {
	luwra::StateWrapper state;

	int a = 13;
	std::string b("Hello");
	float c = 0.37;

	// Tuples aren't ordinarily pushable. But you can use them as return value.
	size_t ret =
		state.map(
			1,
			[&]() -> std::tuple<int, std::string, float> {
				return std::make_tuple(a, b, c);
			}
		);

	REQUIRE(ret == 3);
	REQUIRE(lua_gettop(state) == 3);

	REQUIRE(state.read<int>(-3) == a);
	REQUIRE(state.read<std::string>(-2) == b);
	REQUIRE(state.read<float>(-1) == c);

	ret =
		state.map(
			1,
			[&]() -> std::tuple<int, std::string, float, std::tuple<int, std::string, float>> {
				return std::make_tuple(a, b, c, std::make_tuple(a, b, c));
			}
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

TEST_CASE("Value<bool>") {
	luwra::StateWrapper state;

	for (int i = 0; i < 2; i++) {
		bool value = i == 1;

		SECTION("read") {
			lua_pushboolean(state, value);
			REQUIRE(luwra::read<bool>(state, -1) == value);
		}

		SECTION("push") {
			luwra::push(state, value);
			REQUIRE(lua_type(state, -1) == LUA_TBOOLEAN);
			REQUIRE(luwra::read<bool>(state, -1) == value);
		}
	}
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
