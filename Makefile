# Utilities
RM              = rm -rf
CHDIR           = cd
EXEC            = exec

# Test artifacts
TEST_DIR        := tests
TEST_OUT        := $(TEST_DIR)/all
TEST_SRCS       := all.cpp auxiliary.cpp types.cpp stack.cpp functions.cpp usertypes.cpp
TEST_DEPS       := $(TEST_SRCS:%.cpp=$(TEST_DIR)/%.d)
TEST_OBJS       := $(TEST_SRCS:%.cpp=$(TEST_DIR)/%.o)

# Example artifacts
EXAMPLE_DIR     := examples
EXAMPLE_SRCS    := types.cpp stack.cpp functions.cpp usertypes.cpp state.cpp
EXAMPLE_DEPS    := $(EXAMPLE_SRCS:%.cpp=$(EXAMPLE_DIR)/%.d)
EXAMPLE_OBJS    := $(EXAMPLE_SRCS:%.cpp=$(EXAMPLE_DIR)/%.out)

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

# Miscellaneous
DOCS_OUT        := docs/output

# Default targets
all: test examples

clean:
	$(RM) $(EXAMPLE_OBJS) $(EXAMPLE_DEPS) $(TEST_OUT) $(TEST_OBJS) $(TEST_DEPS) $(DOCS_OUT)

# Documentation
docs:
	mkdocs build
	doxygen

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
	@for ex in $(EXAMPLE_OBJS); do echo "> Example '$$ex'"; ./$$ex; done

-include $(EXAMPLE_DEPS)

$(EXAMPLE_DIR)/%.out: $(EXAMPLE_DIR)/%.cpp Makefile
	$(CXX) $(USECXXFLAGS) $(USELDFLAGS) -MMD -MF$(<:%.cpp=%.d) -MT$@ -o$@ $< $(USELDLIBS)

# Phony
.PHONY: all clean docs test examples
