#include <catch.hpp>

#include <lua.hpp>
#include <luwra.hpp>

#include <memory>

struct A {
	int a;

	A(int x = 1338): a(x) {}
};

TEST_CASE("usertypes_registration") {
	lua_State* state = luaL_newstate();

	// Registration
	luwra::registerUserType<A>(state);

	// Reference
	A* instance = new A;
	luwra::Value<A*>::push(state, instance);

	// Type checks
	REQUIRE(luwra::internal::check_user_type<A>(state, -1) == instance);
	REQUIRE(luwra::Value<A*>::read(state, -1) == instance);

	lua_close(state);
	delete instance;
}

TEST_CASE("usertypes_ctor") {
	lua_State* state = luaL_newstate();

	// Registration
	luwra::registerUserType<A>(state);
	luwra::setGlobal(state, "A", LUWRA_WRAP_CONSTRUCTOR(A, int));

	// Construction
	REQUIRE(luaL_dostring(state, "return A(73)") == 0);

	// Check
	A* instance = luwra::read<A*>(state, -1);
	REQUIRE(instance != nullptr);
	REQUIRE(instance->a == 73);

	lua_close(state);
}

struct B {
	int n;
	const int cn;
	volatile int vn;
	const volatile int cvn;

	B(int val):
		n(val),
		cn(val),
		vn(val),
		cvn(val)
	{}
};

TEST_CASE("usertypes_wrap_fields") {
	lua_State* state = luaL_newstate();

	// Registration
	luwra::registerUserType<B>(
		state,
		{
			LUWRA_MEMBER(B, n),
			LUWRA_MEMBER(B, cn),
			LUWRA_MEMBER(B, vn),
			LUWRA_MEMBER(B, cvn)
		}
	);

	// Instantiation
	B value(1338);
	luwra::setGlobal(state, "val", &value);

	// Unqualified get
	REQUIRE(luaL_dostring(state, "return val:n()") == 0);
	REQUIRE(luwra::read<int>(state, -1) == value.n);

	// Unqualified set
	REQUIRE(luaL_dostring(state, "val:n(42)") == 0);
	REQUIRE(value.n == 42);

	// 'const'-qualified get
	REQUIRE(luaL_dostring(state, "return val:cn()") == 0);
	REQUIRE(luwra::read<int>(state, -1) == value.cn);

	// 'const'-qualified set
	REQUIRE(luaL_dostring(state, "val:cn(42)") == 0);
	REQUIRE(value.cn == 1338);

	// 'volatile' get
	REQUIRE(luaL_dostring(state, "return val:vn()") == 0);
	REQUIRE(luwra::read<int>(state, -1) == value.vn);

	// 'volatile' set
	REQUIRE(luaL_dostring(state, "val:vn(42)") == 0);
	REQUIRE(value.vn == 42);

	// 'const volatile'-qualified get
	REQUIRE(luaL_dostring(state, "return val:cvn()") == 0);
	REQUIRE(luwra::read<int>(state, -1) == value.cvn);

	// 'const volatile'-qualified set
	REQUIRE(luaL_dostring(state, "val:cvn(42)") == 0);
	REQUIRE(value.cvn == 1338);

	lua_close(state);
}

struct C {
	int prop;

	C(int val):
		prop(val)
	{}

	int foo1(int x) {
		return prop += x;
	}

	int foo2(int x) const {
		return prop + x;
	}

	int foo3(int x) volatile {
		return prop -= x;
	}

	int foo4(int x) const volatile {
		return prop - x;
	}
};

TEST_CASE("usertypes_wrap_methods") {
	lua_State* state = luaL_newstate();

	// Registration
	luwra::registerUserType<C>(
		state,
		{
			LUWRA_MEMBER(C, foo1),
			LUWRA_MEMBER(C, foo2),
			LUWRA_MEMBER(C, foo3),
			LUWRA_MEMBER(C, foo4)
		}
	);

	// Instantiation
	C value(1337);
	luwra::setGlobal(state, "value", &value);

	// Unqualified method
	REQUIRE(luaL_dostring(state, "return value:foo1(63)") == 0);
	REQUIRE(value.prop == 1400);
	REQUIRE(luwra::read<int>(state, -1) == value.prop);

	// 'const'-qualified method
	REQUIRE(luaL_dostring(state, "return value:foo2(44)") == 0);
	REQUIRE(value.prop == 1400);
	REQUIRE(luwra::read<int>(state, -1) == 1444);

	// 'volatile'-qualified method
	REQUIRE(luaL_dostring(state, "return value:foo3(400)") == 0);
	REQUIRE(value.prop == 1000);
	REQUIRE(luwra::read<int>(state, -1) == value.prop);

	// 'const volatile'-qualified method
	REQUIRE(luaL_dostring(state, "return value:foo4(334)") == 0);
	REQUIRE(value.prop == 1000);
	REQUIRE(luwra::read<int>(state, -1) == 666);

	lua_close(state);
}

TEST_CASE("usertypes_gchook_tref") {
	lua_State* state = luaL_newstate();

	// Registration
	luwra::registerUserType<std::shared_ptr<int>>(state);

	// Instantiation
	std::shared_ptr<int> shared_var = std::make_shared<int>(1337);
	REQUIRE(shared_var.use_count() == 1);

	// Copy construction
	luwra::push<std::shared_ptr<int>&>(state, shared_var);
	REQUIRE(shared_var.use_count() == 2);

	// Garbage collection
	lua_close(state);
	REQUIRE(shared_var.use_count() == 1);
}

TEST_CASE("usertypes_gchook_tptr") {
	lua_State* state = luaL_newstate();

	// Registration
	luwra::registerUserType<std::shared_ptr<int>>(state);

	// Instantiation
	std::shared_ptr<int> shared_var = std::make_shared<int>(1337);
	REQUIRE(shared_var.use_count() == 1);

	// Reference
	luwra::push(state, &shared_var);
	REQUIRE(shared_var.use_count() == 1);

	// Garbage collection
	lua_close(state);
	REQUIRE(shared_var.use_count() == 1);
}
