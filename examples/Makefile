# Utilities
RM              = rm -rf
EXEC            = exec

# Artifacts
OBJS            = functions.out
DEPS            = $(OBJS:%=%.d)

# Compiler
CXX             ?= clang++
CXXFLAGS        += -std=c++14 -O2 -fno-exceptions -fno-rtti -fmessage-length=0 -Wall -Wextra \
                   -pedantic -I../lib
LDLIBS          += -llua

# Default Targets
all: $(OBJS)
	$(foreach OBJ,$(OBJS),$(EXEC) ./$(OBJ))

clean:
	$(RM) $(DEPS) $(OBJS)

# Objects
-include $(DEPS)

%.out: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MF$(@:%=%.d) -MT$@ -o$@ $< $(LDLIBS)

# Phony
.PHONY: clean
