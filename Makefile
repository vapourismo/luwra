# Utilities
RM              := rm -rf
CHDIR           := cd
EXEC            := exec

# Artifacts
EXAMPLE_DIR     := examples
EXAMPLE_SRCS    := types.cpp stack.cpp functions.cpp usertypes.cpp
EXAMPLE_DEPS    := $(EXAMPLE_SRCS:%.cpp=$(EXAMPLE_DIR)/%.d)
EXAMPLE_OBJS    := $(EXAMPLE_SRCS:%.cpp=$(EXAMPLE_DIR)/%.out)

# Compiler
CXX             ?= clang++
CXXFLAGS        += -std=c++14 -O2 -fno-exceptions -fno-rtti -fmessage-length=0 -Wall -Wextra \
                   -pedantic -Ilib
LDLIBS          += -llua

# Examples
examples: $(EXAMPLE_OBJS)

run-examples: examples
	@for ex in $(EXAMPLE_OBJS); do echo "> Example '$$ex'"; ./$$ex; done

clean:
	$(RM) $(EXAMPLE_OBJS) $(EXAMPLE_DEPS)

-include $(EXAMPLE_DEPS)

$(EXAMPLE_DIR)/%.out: $(EXAMPLE_DIR)/%.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MF$(@:%=%.d) -MT$@ -o$@ $< $(LDLIBS)

# Phony
.PHONY: examples clean
