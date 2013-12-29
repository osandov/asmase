ARCH ?= $(shell ./getarch.sh)
BUILD ?= build

BUILTINS_SRCS := \
	$(wildcard src/Builtins/*.cpp) \
	$(wildcard src/Builtins/Commands/*.cpp) \
	$(patsubst src/%.awk, $(BUILD)/%.cpp, $(wildcard src/Builtins/*Expr.awk)) \
	$(BUILD)/Builtins/Scanner.cpp

SRCS := $(wildcard src/*.c) \
	$(wildcard src/*.cpp) \
	$(wildcard src/arch/$(ARCH)/*.c) \
	$(BUILTINS_SRCS)

OBJS1 := $(patsubst src/%.c, $(BUILD)/%.o, $(SRCS)) # C sources
OBJS2 := $(patsubst src/%.cpp, $(BUILD)/%.o, $(OBJS1)) # C++ sources
OBJS := $(patsubst $(BUILD)/%.cpp, $(BUILD)/%.o, $(OBJS2)) # Generated C++ sources

LLVM_CONFIG ?= llvm-config

COMMON_FLAGS := -Wall -g -Iinclude -I$(BUILD)/include
ALL_CFLAGS := $(COMMON_FLAGS) -std=c99 `$(LLVM_CONFIG) --cflags` $(CFLAGS)
ALL_CXXFLAGS := $(COMMON_FLAGS) -std=c++11 `$(LLVM_CONFIG) --cxxflags` $(CXXFLAGS)
LIBS := `$(LLVM_CONFIG) --ldflags --libs $(ARCH) support` -lreadline

ops_table := src/Builtins/ops_table.txt
dir_guard = @mkdir -p $(@D)

# asmase linking
$(BUILD)/asmase: $(OBJS)
	$(dir_guard)
	@$(CXX) $(ALL_CXXFLAGS) -o $@ $^ $(LIBS)
	@echo LD $@

# C files
$(BUILD)/%.o: src/%.c
	$(dir_guard)
	@$(CC) $(ALL_CFLAGS) -MMD -o $@ -c $<
	@echo CC $@

# C++ files
$(BUILD)/%.o: src/%.cpp
	$(dir_guard)
	@$(CXX) $(ALL_CXXFLAGS) -MMD -o $@ -c $<
	@echo CXX $@

# Generated C++ compilation
$(BUILD)/Builtins/%.o: $(BUILD)/Builtins/%.cpp
	$(dir_guard)
	@$(CXX) $(ALL_CXXFLAGS) -MMD -o $@ -c $<
	@echo CXX $@

# flex scanner
$(BUILD)/Builtins/Scanner.cpp: src/Builtins/Scanner.l
	$(dir_guard)
	flex -o $@ $<
	@echo LEX $@

# AWK-generated AST header
$(BUILD)/include/Builtins/ValueAST.inc: src/Builtins/ValueAST.awk src/Builtins/ValueAST.inc $(ops_table)
	$(dir_guard)
	@AWKPATH="$(<D)" gawk -f $< $(ops_table) src/Builtins/ValueAST.inc > $@
	@echo AWK $@

# AWK-generated C++
$(BUILD)/%.cpp: src/%.awk $(ops_table)
	$(dir_guard)
	@AWKPATH="$(<D)" gawk -f $< $(ops_table) > $@
	@echo AWK $@

DEPS := $(OBJS:.o=.d)

-include $(DEPS)

src/builtins.cpp $(BUILTINS_SRCS): $(BUILD)/include/Builtins/ValueAST.inc

.PHONY: clean
clean:
	rm -rf $(BUILD)
