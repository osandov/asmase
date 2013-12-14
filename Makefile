ARCH := $(shell ./getarch.sh)

SRCS := $(wildcard src/*.c) \
	$(wildcard src/arch/$(ARCH)/*.c) \
	$(wildcard src/llvm_assembler/*.cpp)

BUILD ?= build

OBJS1 := $(patsubst src/%.c, $(BUILD)/%.o, $(SRCS))
OBJS := $(patsubst src/%.cpp, $(BUILD)/c++/%.o, $(OBJS1))

ALL_CFLAGS := -Wall -g -std=c99 -Iinclude `llvm-config --cflags` $(CFLAGS)
ALL_CXXFLAGS := -Wall -g -Iinclude `llvm-config --cxxflags` $(CXXFLAGS)
LIBS := `llvm-config --ldflags --libs all-targets support` -lreadline

$(BUILD)/asmase: $(OBJS)
	$(CXX) $(ALL_CFLAGS) -o $@ $^ $(LIBS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(ALL_CFLAGS) -o $@ -c $<

$(BUILD)/c++/%.o: src/%.cpp | $(BUILD)
	$(CXX) $(ALL_CXXFLAGS) -o $@ -c $<

$(BUILD):
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/arch/$(ARCH)
	mkdir -p $(BUILD)/c++/llvm_assembler

.PHONY: clean
clean:
	rm -rf $(BUILD)
