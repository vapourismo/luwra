# Utilities
RM              = rm -rf
CHDIR           = cd
EXEC            = exec

# Test artifacts
TEST_DIR        := tests
TEST_OUT        := $(TEST_DIR)/all
TEST_SRCS       := all.cpp auxiliary.cpp types.cpp stack.cpp functions.cpp usertypes.cpp \
                   wrappers.cpp tables.cpp internal/indexsequence.cpp internal/typelist.cpp \
                   internal/types.cpp
TEST_DEPS       := $(TEST_SRCS:%.cpp=$(TEST_DIR)/%.d)
TEST_OBJS       := $(TEST_SRCS:%.cpp=$(TEST_DIR)/%.o)

# Example artifacts
EXAMPLE_DIR     := examples
EXAMPLE_SRCS    := types.cpp stack.cpp functions.cpp usertypes.cpp state.cpp tables.cpp
EXAMPLE_DEPS    := $(EXAMPLE_SRCS:%.cpp=$(EXAMPLE_DIR)/%.d)
EXAMPLE_OBJS    := $(EXAMPLE_SRCS:%.cpp=$(EXAMPLE_DIR)/%.out)

# Playground artifacts
PLAYGROUND_SRC  := playground.cpp
PLAYGROUND_DEP  := $(PLAYGROUND_SRC:%.cpp=$(EXAMPLE_DIR)/%.d)
PLAYGROUND_OBJ  := $(PLAYGROUND_SRC:%.cpp=$(EXAMPLE_DIR)/%.out)

# Lua-specific
LUA_INCDIR      = /usr/include
LUA_LIBDIR      = /usr/lib
LUA_LIBNAME     = lua

# Compiler
CXX             ?= clang++
USECXXFLAGS     += $(CXXFLAGS) -std=c++11 -O0 -g -DDEBUG -fmessage-length=0 -Wall -Wextra \
                   -pedantic -D_GLIBCXX_USE_C99 -Ilib -I$(LUA_INCDIR) -Ideps/catch/include
USELDFLAGS      += $(LDFLAGS) -L$(LUA_LIBDIR)
USELDLIBS       += $(LDLIBS) -lm -l$(LUA_LIBNAME) -ldl

# Default targets
all: test examples

clean:
	$(RM) $(EXAMPLE_OBJS) $(EXAMPLE_DEPS)
	$(RM) $(TEST_OUT) $(TEST_OBJS) $(TEST_DEPS)
	$(RM) $(PLAYGROUND_DEP) $(PLAYGROUND_OBJ)

# Documentation
docs:
	mkdocs build --clean
	doxygen

push-gh-pages:
	git subtree push --prefix docs/output origin gh-pages

# Tests
test: $(TEST_OUT)
	./$(TEST_OUT)

-include $(TEST_DEPS)

$(TEST_OUT): $(TEST_OBJS)
	$(CXX) $(USELDFLAGS) -o$@ $(TEST_OBJS) $(USELDLIBS)

$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp Makefile
	$(CXX) -c $(USECXXFLAGS) -MMD -MF$(@:%.o=%.d) -MT$@ -o$@ $<

# Examples
examples: $(EXAMPLE_OBJS)
	@for ex in $(EXAMPLE_OBJS); do echo "> Example '$$ex'"; ./$$ex || exit 1; done

-include $(EXAMPLE_DEPS)
-include $(PLAYGROUND_DEP)

$(EXAMPLE_DIR)/%.out: $(EXAMPLE_DIR)/%.cpp Makefile
	$(CXX) $(USECXXFLAGS) $(USELDFLAGS) -MMD -MF$(<:%.cpp=%.d) -MT$@ -o$@ $< $(USELDLIBS)

$(PLAYGROUND_OBJ): $(EXAMPLE_DIR)/$(PLAYGROUND_SRC) Makefile
	$(CXX) $(USECXXFLAGS) $(USELDFLAGS) -MMD -MF$(<:%.cpp=%.d) -MT$@ -o$@ $< $(USELDLIBS) -lprofiler

# Playground
playground-prof: $(PLAYGROUND_OBJ)
	CPUPROFILE=./cpuprofile.prof ./$(PLAYGROUND_OBJ)
	pprof --pdf ./$(PLAYGROUND_OBJ) ./cpuprofile.prof > cpuprofile.pdf
	xdg-open cpuprofile.pdf

playground: $(PLAYGROUND_OBJ)
	./$(PLAYGROUND_OBJ)

# Phony
.PHONY: all clean docs test examples playground playground-prof
