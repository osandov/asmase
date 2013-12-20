ARCH ?= $(shell ./getarch.sh)

SRCS := $(wildcard src/*.c) \
	$(wildcard src/*.cpp) \
	$(wildcard src/arch/$(ARCH)/*.c) \

BUILD ?= build

OBJS1 := $(patsubst src/%.c, $(BUILD)/%.o, $(SRCS))
OBJS := $(patsubst src/%.cpp, $(BUILD)/%.o, $(OBJS1))

LLVM_CONFIG ?= llvm-config

ALL_CFLAGS := -Wall -g -std=c99 -Iinclude `$(LLVM_CONFIG) --cflags` $(CFLAGS)
ALL_CXXFLAGS := -Wall -g -Iinclude `$(LLVM_CONFIG) --cxxflags` $(CXXFLAGS)
LIBS := `$(LLVM_CONFIG) --ldflags --libs $(ARCH) support` -lreadline

dir_guard = @mkdir -p $(@D)

$(BUILD)/asmase: $(OBJS)
	$(dir_guard)
	$(CXX) $(ALL_CXXFLAGS) -o $@ $^ $(LIBS)

$(BUILD)/%.o: src/%.c
	$(dir_guard)
	$(CC) $(ALL_CFLAGS) -o $@ -c $<

$(BUILD)/%.o: src/%.cpp
	$(dir_guard)
	$(CXX) $(ALL_CXXFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf $(BUILD)
