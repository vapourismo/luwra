#include <catch.hpp>
#include <luwra.hpp>

static int dummy_void_result = 0;

static void dummy1() { dummy_void_result = 1337; }
static void dummy2(int a) { dummy_void_result = 1337 - a; }
static void dummy3(int a, int b) { dummy_void_result = 1337 * a + b; }

static int dummy4() { return 1337; }
static int dummy5(int a) { return 1337 - a; }
static int dummy6(int a, int b) { return 1337 * a + b; }

#define LUWRA_GW_TEST_PTRS(param, type) \
	{ \
		luwra::CFunction wrap_auto = LUWRA_WRAP(param); \
		luwra::CFunction wrap_manual = &luwra::internal::Wrapper<type>::invoke<&param>; \
		REQUIRE(wrap_auto != nullptr); \
		REQUIRE(wrap_manual != nullptr); \
		REQUIRE(wrap_auto == wrap_manual); \
	}

#define LUWRA_GW_TEST_FUNC_VOID(func, params) \
	{ \
		dummy_void_result = 0; \
		func params; \
		 \
		int expected_result = dummy_void_result; \
		 \
		luwra::StateWrapper state; \
		state.set("func", LUWRA_WRAP(func)); \
		 \
		dummy_void_result = 0; \
		REQUIRE(state.runString("func" #params) == LUA_OK); \
		 \
		REQUIRE(expected_result == dummy_void_result); \
	}

#define LUWRA_GW_TEST_FUNC_NONVOID(func, params) \
	{ \
		int expected_result = func params; \
		\
		luwra::StateWrapper state; \
		state.set("func", LUWRA_WRAP(func)); \
		 \
		REQUIRE(state.runString("return func" #params) == LUA_OK); \
		int given_result = luwra::read<int>(state, -1); \
		 \
		REQUIRE(expected_result == given_result); \
	}

TEST_CASE("Wrapper<R(A...)>") {
	LUWRA_GW_TEST_PTRS(dummy1, void());
	LUWRA_GW_TEST_PTRS(dummy2, void(int));
	LUWRA_GW_TEST_PTRS(dummy3, void(int, int));

	LUWRA_GW_TEST_PTRS(dummy4, int());
	LUWRA_GW_TEST_PTRS(dummy5, int(int));
	LUWRA_GW_TEST_PTRS(dummy6, int(int, int));

	LUWRA_GW_TEST_FUNC_VOID(dummy1, ());
	LUWRA_GW_TEST_FUNC_VOID(dummy2, (13));
	LUWRA_GW_TEST_FUNC_VOID(dummy3, (13, 37));

	LUWRA_GW_TEST_FUNC_NONVOID(dummy4, ());
	LUWRA_GW_TEST_FUNC_NONVOID(dummy5, (13));
	LUWRA_GW_TEST_FUNC_NONVOID(dummy6, (13, 37));
}

struct Dummy {
	int field;
	const int const_field;

	Dummy(int a = 0, int b = 0):
		field(a),
		const_field(b)
	{}

	void dummy1() { field = 1337; }
	void dummy2(int a) { field = 1337 - a; }
	void dummy3(int a, int b) { field = 1337 * a + b; }

	int dummy4() { return 1337; }
	int dummy5(int a) { return 1337 - a; }
	int dummy6(int a, int b) { return 1337 * a + b; }

	void dummy7() volatile { field = 1337; }
	void dummy8(int a) volatile { field = 1337 - a; }
	void dummy9(int a, int b) volatile { field = 1337 * a + b; }

	int dummy10() volatile { return 1337; }
	int dummy11(int a) volatile { return 1337 - a; }
	int dummy12(int a, int b) volatile { return 1337 * a + b; }

	int dummy13() const { return 1337; }
	int dummy14(int a) const { return 1337 - a; }
	int dummy15(int a, int b) const { return 1337 * a + b; }

	int dummy16() const volatile { return 1337; }
	int dummy17(int a) const volatile { return 1337 - a; }
	int dummy18(int a, int b) const volatile { return 1337 * a + b; }
};

#define LUWRA_GW_TEST_METH_VOID(meth, params) \
	{ \
		Dummy d1; \
		d1.meth params; \
		 \
		luwra::StateWrapper state; \
		state.set("d2", Dummy()); \
		state.set("meth", LUWRA_WRAP(__LUWRA_NS_RESOLVE(Dummy, meth))); \
		 \
		Dummy& d2 = state.get<Dummy&>("d2"); \
		 \
		REQUIRE(state.runString("(function (...) meth(d2, ...) end)" #params) == LUA_OK); \
		REQUIRE(d1.field == d2.field); \
	}

#define LUWRA_GW_TEST_METH_NONVOID(meth, params) \
	{ \
		Dummy d1; \
		int expected_result = d1.meth params; \
		 \
		luwra::StateWrapper state; \
		state.set("d2", Dummy()); \
		state.set("meth", LUWRA_WRAP(__LUWRA_NS_RESOLVE(Dummy, meth))); \
		 \
		REQUIRE(state.runString("return (function (...) return meth(d2, ...) end)" #params) == LUA_OK); \
		int given_result = luwra::read<int>(state, -1); \
		REQUIRE(expected_result == given_result); \
	}

TEST_CASE("Wrapper<R (T::*)(A...)>") {
	LUWRA_GW_TEST_PTRS(Dummy::dummy1, void (Dummy::*)());
	LUWRA_GW_TEST_PTRS(Dummy::dummy2, void (Dummy::*)(int));
	LUWRA_GW_TEST_PTRS(Dummy::dummy3, void (Dummy::*)(int, int));

	LUWRA_GW_TEST_PTRS(Dummy::dummy4, int (Dummy::*)());
	LUWRA_GW_TEST_PTRS(Dummy::dummy5, int (Dummy::*)(int));
	LUWRA_GW_TEST_PTRS(Dummy::dummy6, int (Dummy::*)(int, int));

	LUWRA_GW_TEST_METH_VOID(dummy1, ());
	LUWRA_GW_TEST_METH_VOID(dummy2, (13));
	LUWRA_GW_TEST_METH_VOID(dummy3, (13, 37));

	LUWRA_GW_TEST_METH_NONVOID(dummy4, ());
	LUWRA_GW_TEST_METH_NONVOID(dummy5, (13));
	LUWRA_GW_TEST_METH_NONVOID(dummy6, (13, 37));
}

TEST_CASE("Wrapper<R (T::*)(A...) volatile>") {
	LUWRA_GW_TEST_PTRS(Dummy::dummy7, void (Dummy::*)() volatile);
	LUWRA_GW_TEST_PTRS(Dummy::dummy8, void (Dummy::*)(int) volatile);
	LUWRA_GW_TEST_PTRS(Dummy::dummy9, void (Dummy::*)(int, int) volatile);

	LUWRA_GW_TEST_PTRS(Dummy::dummy10, int (Dummy::*)() volatile);
	LUWRA_GW_TEST_PTRS(Dummy::dummy11, int (Dummy::*)(int) volatile);
	LUWRA_GW_TEST_PTRS(Dummy::dummy12, int (Dummy::*)(int, int) volatile);

	LUWRA_GW_TEST_METH_VOID(dummy7, ());
	LUWRA_GW_TEST_METH_VOID(dummy8, (13));
	LUWRA_GW_TEST_METH_VOID(dummy9, (13, 37));

	LUWRA_GW_TEST_METH_NONVOID(dummy10, ());
	LUWRA_GW_TEST_METH_NONVOID(dummy11, (13));
	LUWRA_GW_TEST_METH_NONVOID(dummy12, (13, 37));
}

TEST_CASE("Wrapper<R (T::*)(A...) const>") {
	LUWRA_GW_TEST_PTRS(Dummy::dummy13, int (Dummy::*)() const);
	LUWRA_GW_TEST_PTRS(Dummy::dummy14, int (Dummy::*)(int) const);
	LUWRA_GW_TEST_PTRS(Dummy::dummy15, int (Dummy::*)(int, int) const);

	LUWRA_GW_TEST_METH_NONVOID(dummy13, ());
	LUWRA_GW_TEST_METH_NONVOID(dummy14, (13));
	LUWRA_GW_TEST_METH_NONVOID(dummy15, (13, 37));
}

TEST_CASE("Wrapper<R (T::*)(A...) const volatile>") {
	LUWRA_GW_TEST_PTRS(Dummy::dummy16, int (Dummy::*)() const volatile);
	LUWRA_GW_TEST_PTRS(Dummy::dummy17, int (Dummy::*)(int) const volatile);
	LUWRA_GW_TEST_PTRS(Dummy::dummy18, int (Dummy::*)(int, int) const volatile);

	LUWRA_GW_TEST_METH_NONVOID(dummy16, ());
	LUWRA_GW_TEST_METH_NONVOID(dummy17, (13));
	LUWRA_GW_TEST_METH_NONVOID(dummy18, (13, 37));
}

TEST_CASE("Wrapper<const R T::*>") {
	LUWRA_GW_TEST_PTRS(Dummy::const_field, const int (Dummy::*));

	luwra::StateWrapper state;
	state.set("dummy", Dummy(13, 37));
	state.set("accessor", LUWRA_WRAP(Dummy::const_field));

	Dummy& dummy = state.get<Dummy&>("dummy");
	REQUIRE(dummy.const_field == 37);

	REQUIRE(state.runString("return accessor(dummy)") == LUA_OK);
	REQUIRE(luwra::read<int>(state, -1) == dummy.const_field);
}

TEST_CASE("Wrapper<R T::*>") {
	LUWRA_GW_TEST_PTRS(Dummy::field, int (Dummy::*));

	luwra::StateWrapper state;
	state.set("accessor", LUWRA_WRAP(Dummy::field));
	state.set("dummy", Dummy(13, 37));

	Dummy& dummy = state.get<Dummy&>("dummy");
	REQUIRE(dummy.field == 13);

	REQUIRE(state.runString("return accessor(dummy)") == LUA_OK);
	REQUIRE(luwra::read<int>(state, -1) == dummy.field);

	REQUIRE(state.runString("accessor(dummy, 1337)") == LUA_OK);
	REQUIRE(dummy.field == 1337);
}
