ARCH ?= $(shell ./getarch.sh)
BUILD ?= build
QUIET ?= @

BUILTINS_SRCS := \
	$(wildcard src/Builtins/*.cpp) \
	$(patsubst src/%.awk, $(BUILD)/%.cpp, $(wildcard src/Builtins/*Expr.awk)) \
	$(BUILD)/Builtins/Scanner.cpp \
	$(wildcard src/Builtins/Commands/*.cpp)

SRCS := $(wildcard src/*.cpp) \
	$(wildcard src/Arch/$(ARCH)/*.cpp) \
	$(BUILTINS_SRCS)

OBJS1 := $(patsubst src/%.cpp, $(BUILD)/%.o, $(SRCS)) # C++ sources
OBJS := $(patsubst $(BUILD)/%.cpp, $(BUILD)/%.o, $(OBJS1)) # Generated C++ sources

LLVM_CONFIG ?= llvm-config
ALL_CXXFLAGS := -Wall -g -Iinclude -I$(BUILD)/include -std=c++11 `$(LLVM_CONFIG) --cxxflags` -fno-strict-aliasing $(CXXFLAGS)
LIBS := `$(LLVM_CONFIG) --ldflags --libs $(ARCH) support` -lreadline
LIBS += `$(LLVM_CONFIG) --system-libs 2>/dev/null`

ops_table := src/Builtins/ops_table.txt
dir_guard = @mkdir -p $(@D)

# asmase linking
$(BUILD)/asmase: $(OBJS)
	$(dir_guard)
	@echo LD $@
	$(QUIET) $(CXX) $(ALL_CXXFLAGS) -o $@ $^ $(LIBS)

# C++ files
$(BUILD)/%.o: src/%.cpp
	$(dir_guard)
	@echo CXX $@
	$(QUIET) $(CXX) $(ALL_CXXFLAGS) -MMD -o $@ -c $<

# Generated C++ compilation
$(BUILD)/Builtins/%.o: $(BUILD)/Builtins/%.cpp
	$(dir_guard)
	@echo CXX $@
	$(QUIET) $(CXX) $(ALL_CXXFLAGS) -MMD -o $@ -c $<

# flex scanner
$(BUILD)/Builtins/Scanner.cpp: src/Builtins/Scanner.l
	$(dir_guard)
	@echo LEX $@
	$(QUIET) flex -o $@ $<

# AWK-generated AST header
$(BUILD)/include/Builtins/ValueAST.inc: src/Builtins/ValueAST.awk src/Builtins/ValueAST.inc $(ops_table)
	$(dir_guard)
	@echo AWK $@
	$(QUIET) AWKPATH="$(<D)" gawk -f $< $(ops_table) src/Builtins/ValueAST.inc > $@

# AWK-generated C++
$(BUILD)/%.cpp: src/%.awk $(ops_table)
	$(dir_guard)
	@echo AWK $@
	$(QUIET) AWKPATH="$(<D)" gawk -f $< $(ops_table) > $@

DEPS := $(OBJS:.o=.d)

-include $(DEPS)

src/Builtins.cpp $(BUILTINS_SRCS): $(BUILD)/include/Builtins/ValueAST.inc

.PHONY: clean
clean:
	rm -rf $(BUILD)
