ARCH := x86

LLVM_CONFIG ?= llvm-config

CPPFLAGS := -Iinclude -Iarch/$(ARCH)/include -D_GNU_SOURCE -DLIBEXEC=\"/usr/libexec\"
CFLAGS := -std=gnu11 -Wall -Werror -O2 $(CPPFLAGS)
CXXFLAGS := -std=c++11 -Wall -Werror -O2 $(CPPFLAGS)
LDFLAGS := `$(LLVM_CONFIG) --ldflags --libs $(ARCH) support`
LDFLAGS += `$(LLVM_CONFIG) --system-libs 2>/dev/null`

OBJS := assembler.o \
	libasmase.o \
	registers.o \
	arch/$(ARCH)/arch.o

.PHONY: all
all: libasmase.so asmase_tracee

asmase_tracee: tracee.c
	$(CC) $(CFLAGS) -o $@ $^ -lseccomp

libasmase.so: $(OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^

assembler.o: assembler.cpp
	$(CXX) $(CXXFLAGS) -fpic -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -fpic -c -o $@ $<

.PHONY: clean
clean:
	rm -f libasmase.so asmase_tracee $(OBJS)
