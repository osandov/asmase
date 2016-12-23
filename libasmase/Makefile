ARCH := x86

LLVM_CONFIG ?= llvm-config

CPPFLAGS := -Iinclude -Iarch/$(ARCH)/include -D_GNU_SOURCE -DLIBEXEC=\"/usr/libexec\"
CFLAGS := -std=gnu11 -Wall -Werror -O2 $(CPPFLAGS)
CXXFLAGS := -std=c++11 -Wall -Werror -O2 $(CPPFLAGS)
LDFLAGS := `$(LLVM_CONFIG) --ldflags --libs $(ARCH) support`
LDFLAGS += `$(LLVM_CONFIG) --system-libs 2>/dev/null`

TEST_LDFLAGS := -L. -Wl,-rpath . -lasmase

OBJS := assembler.o \
	libasmase.o \
	registers.o \
	arch/$(ARCH)/arch.o

.PHONY: all
all: libasmase.so asmase_tracee

asmase_tracee: tracee.c
	$(CC) $(CFLAGS) -static -o $@ $^

libasmase.so: $(OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^

assembler.o: assembler.cpp
	$(CXX) $(CXXFLAGS) -fpic -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -fpic -c -o $@ $<

TESTS := tests/test_assembler \
	tests/test_registers \
	tests/test_memory

.PHONY: tests
tests: $(TESTS)

tests/test_%: tests/test_%.c libasmase.so
	$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $<

.PHONY: check
check: tests
	@ for test in $(TESTS); do ./$$test; done

.PHONY: clean
clean:
	rm -f libasmase.so $(OBJS)
	rm -f $(TESTS)
