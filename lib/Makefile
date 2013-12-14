LEVEL = ..
DIRS = llvm_assembler $(ASIP_ARCH)_tracing

include $(LEVEL)/Makefile.common

ifeq ($(ARCH), x86_64)
    ASIP_ARCH = x86
else
    ASIP_ARCH = $(ARCH)
endif
