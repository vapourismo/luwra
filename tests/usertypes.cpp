#include "catch.hpp"

#include <lua.hpp>
#include <luwra.hpp>

#include <memory>

struct A {
	int a;

	A() {}
};

TEST_CASE("usertypes_local_state") {
	lua_State* state = luaL_newstate();

	REQUIRE(luwra::internal::user_type_id<A> == (void*) INTPTR_MAX);

	luwra::register_user_type<A>(state, {});
	REQUIRE(luwra::internal::user_type_id<A> != (void*) INTPTR_MAX);

	A* instance = new A;
	luwra::Value<A*>::push(state, instance);

	REQUIRE(luwra::internal::get_user_type_id(state, -1) == luwra::internal::user_type_id<A>);
	REQUIRE(luwra::internal::check_user_type<A>(state, -1) == instance);

	lua_close(state);
}

struct B {
	int prop;

	int increment(int x) {
		return prop += x;
	}

	B* add(const B* other) {
		return new B {prop + other->prop};
	}
};

TEST_CASE("usertypes_lua_usage") {
	lua_State* state = luaL_newstate();
	luwra::register_user_type<B>(
		state,
		{
			{"increment", luwra::wrap_method<B, int(int), &B::increment>},
			{"prop", luwra::wrap_property<B, int, &B::prop>}
		},
		{
			{"__add", luwra::wrap_method<B, B*(const B*), &B::add>}
		}
	);

	B* value = new B {1337};
	luwra::register_global(state, "value", value);

	REQUIRE(luaL_dostring(state, "return value:prop()") == 0);
	REQUIRE(luwra::Value<int>::read(state, -1) == value->prop);

	REQUIRE(luaL_dostring(state, "value:prop(7331)") == 0);
	REQUIRE(value->prop == 7331);

	REQUIRE(luaL_dostring(state, "value:increment(-7329)") == 0);
	REQUIRE(value->prop == 2);

	REQUIRE(luaL_dostring(state, "return value + value") == 0);
	REQUIRE(luwra::Value<B*>::read(state, -1)->prop == 4);

	lua_close(state);
}

TEST_CASE("usertypes_gchook_tref") {
	lua_State* state = luaL_newstate();
	luwra::register_user_type<std::shared_ptr<int>>(state, {});

	std::shared_ptr<int> shared_var = std::make_shared<int>(1337);
	REQUIRE(shared_var.use_count() == 1);

	luwra::Value<std::shared_ptr<int>&>::push(state, shared_var);
	REQUIRE(shared_var.use_count() == 2);

	lua_close(state);
	REQUIRE(shared_var.use_count() == 1);
}

TEST_CASE("usertypes_gchook_tptr") {
	lua_State* state = luaL_newstate();
	luwra::register_user_type<std::shared_ptr<int>>(state, {});

	std::shared_ptr<int> shared_var = std::make_shared<int>(1337);
	REQUIRE(shared_var.use_count() == 1);

	luwra::Value<std::shared_ptr<int>*>::push(state, &shared_var);
	REQUIRE(shared_var.use_count() == 1);

	lua_close(state);
	REQUIRE(shared_var.use_count() == 1);
}
