ARCH := x86

LLVM_CONFIG ?= llvm-config

CPPFLAGS := -D_GNU_SOURCE -DARCH_HEADER=\"$(ARCH).h\"
COMMON_FLAGS := -Wall -Werror -O2 -fpic -fvisibility=hidden -g
CFLAGS := -std=gnu11 $(COMMON_FLAGS)
CXXFLAGS := -std=c++11 $(COMMON_FLAGS)
LDFLAGS := -shared -lseccomp
LDFLAGS += `$(LLVM_CONFIG) --ldflags --libs $(ARCH) support`
LDFLAGS += `$(LLVM_CONFIG) --system-libs 2>/dev/null`

OBJS := assembler.o \
	libasmase.o \
	registers.o \
	tracee.o \
	$(ARCH).o

libasmase.so: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f libasmase.so $(OBJS)
