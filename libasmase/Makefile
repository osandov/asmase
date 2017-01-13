ARCH := x86

LLVM_CONFIG ?= llvm-config

CPPFLAGS := -Iinclude -Iarch/$(ARCH)/include -D_GNU_SOURCE -DLIBEXEC=\"/usr/libexec\"
COMMON_FLAGS := -Wall -Werror -O2 -fpic -fvisibility=hidden $(CPPFLAGS) -g
CFLAGS := -std=gnu11 $(COMMON_FLAGS)
CXXFLAGS := -std=c++11 $(COMMON_FLAGS)
LDFLAGS := -fpic -fvisibility=hidden -pie -Wl,-E -ldl -lseccomp
LDFLAGS += `$(LLVM_CONFIG) --ldflags --libs $(ARCH) support`
LDFLAGS += `$(LLVM_CONFIG) --system-libs 2>/dev/null`

OBJS := assembler.o \
	libasmase.o \
	registers.o \
	tracee.o \
	arch/$(ARCH)/arch.o

libasmase.so: $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f libasmase.so $(OBJS)
