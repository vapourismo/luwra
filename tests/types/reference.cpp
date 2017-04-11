#include <catch.hpp>
#include <luwra.hpp>

#include <iostream>

using namespace luwra;

// Set the referenced boolean to true upon garbage collection
struct GCTrigger {
	bool& target;

	GCTrigger(bool& target): target(target) {
		target = false;
	}

	~GCTrigger() {
		target = true;
	}
};

TEST_CASE("RefLifecycle") {
	StateWrapper state;

	state.loadStandardLibrary();
	state.registerUserType<GCTrigger>();

	bool didCollect;

	// We instantiate a type which will set didCollect to true upon garbage collection
	construct<GCTrigger>(state, didCollect);

	// didCollect starts off as false
	REQUIRE(!didCollect);

	{
		// Create the reference
		RefLifecycle refLife(state, -1);

		// Remove the user data from the stack and perform a full garbage collection cycle
		lua_pop(state, 1);
		lua_gc(state, LUA_GCCOLLECT, 0);

		// Since the reference still exists, the user data must not have been collected
		REQUIRE(!didCollect);
	}

	// Our reference goes out of scope, therefore making the user data unreachable. In order to
	// trigger the finalizers, we must collect the dead user data.
	lua_gc(state, LUA_GCCOLLECT, 0);

	// At this point, the finalizer should have been invoked
	REQUIRE(didCollect);
}
