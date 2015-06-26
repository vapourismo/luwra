#include "catch.hpp"

#include <lua.hpp>
#include <luwra.hpp>

#include <memory>

struct A {
	int a;

	A() {}
};

TEST_CASE("usertypes_registration") {
	lua_State* state = luaL_newstate();

	REQUIRE(luwra::internal::user_type_id<A> == (void*) INTPTR_MAX);

	luwra::register_user_type<A>(state, {});
	REQUIRE(luwra::internal::user_type_id<A> != (void*) INTPTR_MAX);

	A* instance = new A;
	luwra::Value<A*>::push(state, instance);

	REQUIRE(luwra::internal::get_user_type_id(state, -1) == luwra::internal::user_type_id<A>);
	REQUIRE(luwra::internal::check_user_type<A>(state, -1) == instance);
	REQUIRE(luwra::Value<A*>::read(state, -1) == instance);

	lua_close(state);
	delete instance;
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

	luwra::register_user_type<B>(
		state,
		{
			{"n", luwra::wrap_field<B, int, &B::n>},
			{"cn", luwra::wrap_field<B, const int, &B::cn>},
			{"vn", luwra::wrap_field<B, volatile int, &B::vn>},
			{"cvn", luwra::wrap_field<B, const volatile int, &B::cvn>}
		}
	);

	B value(1338);
	luwra::register_global(state, "val", &value);

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
	luwra::register_user_type<C>(
		state,
		{
			{"foo1", luwra::wrap_method<C, int(int), &C::foo1>},
			{"foo2", luwra::wrap_method<const C, int(int), &C::foo2>},
			{"foo3", luwra::wrap_method<volatile C, int(int), &C::foo3>},
			{"foo4", luwra::wrap_method<const volatile C, int(int), &C::foo4>}
		}
	);

	C value(1337);
	luwra::register_global(state, "value", &value);

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
	luwra::register_user_type<std::shared_ptr<int>>(state, {});

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
	luwra::register_user_type<std::shared_ptr<int>>(state, {});

	std::shared_ptr<int> shared_var = std::make_shared<int>(1337);
	REQUIRE(shared_var.use_count() == 1);

	// Reference
	luwra::push(state, &shared_var);
	REQUIRE(shared_var.use_count() == 1);

	// Garbage collection
	lua_close(state);
	REQUIRE(shared_var.use_count() == 1);
}
