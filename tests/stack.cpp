#include <catch.hpp>
#include <luwra.hpp>

namespace {
	struct A {
		std::shared_ptr<size_t> copyCounter;
		std::shared_ptr<size_t> moveCounter;

		A():
			copyCounter(new size_t(0)),
			moveCounter(new size_t(0))
		{}

		A(const A& a):
			copyCounter(a.copyCounter),
			moveCounter(a.moveCounter)
		 {
			(*a.copyCounter)++;
		}

		A(A&& a):
			copyCounter(a.copyCounter),
			moveCounter(a.moveCounter)
		 {
			(*a.moveCounter)++;
		}
	};
}

TEST_CASE("push") {
	luwra::StateWrapper state;

	SECTION("perfect forwarding") {
		A onlyCopy;
		luwra::push(state, onlyCopy);

		REQUIRE(*onlyCopy.copyCounter == 1);
		REQUIRE(*onlyCopy.moveCounter == 0);
		REQUIRE(lua_gettop(state) == 1);

		A onlyMove;
		luwra::push(state, std::move(onlyMove));

		REQUIRE(*onlyMove.copyCounter == 0);
		REQUIRE(*onlyMove.moveCounter == 1);
		REQUIRE(lua_gettop(state) == 2);
	}

	SECTION("single") {
		luwra::push(state, 1337);

		REQUIRE(lua_gettop(state) == 1);
	}

	SECTION("multiple") {
		luwra::push(state, 13);
		luwra::push(state, 37);

		REQUIRE(lua_gettop(state) == 2);
	}
}

namespace {
	struct B {
		int x;

		B(int x): x(x) {}

		bool operator ==(const B& other) const {
			return x == other.x;
		}
	};
}

TEST_CASE("read") {
	luwra::StateWrapper state;

	SECTION("return type") {
		using ReadReturnType1 =
			decltype(luwra::read<int>(std::declval<luwra::State*>(), std::declval<int>()));
		using ValueReadReturnType1 =
			decltype(luwra::Value<int>::read(std::declval<luwra::State*>(), std::declval<int>()));

		REQUIRE((std::is_same<ReadReturnType1, ValueReadReturnType1>::value));

		using ReadReturnType2 =
			decltype(luwra::read<A>(std::declval<luwra::State*>(), std::declval<int>()));
		using ValueReadReturnType2 =
			decltype(luwra::Value<A>::read(std::declval<luwra::State*>(), std::declval<int>()));

		REQUIRE((std::is_same<ReadReturnType2, ValueReadReturnType2>::value));

		using ReadReturnType3 =
			decltype(luwra::read<A&>(std::declval<luwra::State*>(), std::declval<int>()));
		using ValueReadReturnType3 =
			decltype(luwra::Value<A&>::read(std::declval<luwra::State*>(), std::declval<int>()));

		REQUIRE((std::is_same<ReadReturnType3, ValueReadReturnType3>::value));
	}

	SECTION("value") {
		luwra::push(state, 13, 37);

		REQUIRE(luwra::read<int>(state, 1) == 13);
		REQUIRE(luwra::read<int>(state, -2) == 13);

		REQUIRE(luwra::read<int>(state, 2) == 37);
		REQUIRE(luwra::read<int>(state, -1) == 37);
	}

	SECTION("user type") {
		B b {1337};
		luwra::push(state, b);

		REQUIRE(luwra::read<B>(state, 1) == b);
		REQUIRE(luwra::read<B>(state, -1) == b);

		REQUIRE(luwra::read<const B&>(state, 1) == b);
		REQUIRE(luwra::read<const B&>(state, -1) == b);

		REQUIRE(luwra::read<B&&>(state, 1) == b);
		REQUIRE(luwra::read<B&&>(state, -1) == b);
	}
}


TEST_CASE("pushReturn") {
	luwra::StateWrapper state;

	SECTION("single") {
		REQUIRE(pushReturn(state, 13) == 1);
		REQUIRE(lua_gettop(state) == 1);
	}

	SECTION("multiple") {
		REQUIRE(pushReturn(state, 13, 37) == 2);
		REQUIRE(lua_gettop(state) == 2);
	}
}

namespace {
	int foo(int a, int b, int c) {
		return a + b + c;
	}
}

TEST_CASE("apply") {
	luwra::StateWrapper state;

	SECTION("no extra arguments") {
		luwra::push(state, -50, 1337, 3713);

		REQUIRE(luwra::apply(state, 1, foo) == 5000);
		REQUIRE(luwra::apply(state, 1, [](int a, int b, int c) {
			return foo(a, b, c);
		}) == 5000);
	}

	SECTION("extra arguments") {
		luwra::push(state, 1337, 3713);

		REQUIRE(luwra::apply(state, 1, foo, -50) == 5000);
		REQUIRE(luwra::apply(state, 1, [](int a, int b, int c) {
			return foo(a, b, c);
		}, -50) == 5000);
	}

	SECTION("extra argument lvalue reference") {
		int arg = 13;

		REQUIRE(luwra::apply(state, 1, [](int& v) {
			return v *= 2;
		}, arg) == 26);
		REQUIRE(arg == 26);

		REQUIRE(luwra::apply(state, 1, [](const int& v) {
			return v * 2;
		}, arg) == 52);

		REQUIRE(luwra::apply(state, 1, [](const int& v) {
			return v * 2;
		}, 13) == 26);
	}

	SECTION("extra argument rvalue reference") {
		REQUIRE(luwra::apply(state, 1, [](int&& v) {
			return v *= 2;
		}, 13) == 26);
	}

	SECTION("stack argument lvalue reference") {
		luwra::push(state, 13, 37);

		REQUIRE(luwra::apply(state, 1, [](const int& a, const int& b) {
			return a + b;
		}) == 50);

		B& a = luwra::construct<B>(state, 1337);

		REQUIRE(luwra::apply(state, 3, [](B& b){
			return b.x *= 2;
		}) == 2674);
		REQUIRE(a.x == 2674);
	}

	SECTION("stack argument rvalue reference") {
		luwra::push(state, 13, 37);

		REQUIRE(luwra::apply(state, 1, [](int&& a, int&& b) {
			return a + b;
		}) == 50);
	}
}

TEST_CASE("map") {

}
