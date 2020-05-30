ARCH = riscv
PREFIX = riscv64-unknown-linux-gnu-
GCC = $(PREFIX)gcc
CC = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

LIBC = ../../../musl-1.2.0
LIBSTDCXX = ../../../riscv-gcc/libstdc++-v3/build
RISCV = /opt/riscv

CFLAGS = -fno-builtin -nostdlib -static -Wl,--gc-sections -O2 -Wall \
         -march=rv64imac \
         -I$(LIBC)/include -I$(LIBC)/obj/include -I$(LIBC)/arch/riscv64 -I$(LIBC)/arch/generic $(DEFINES)

CCFLAGS = -nostdinc++ \
          -I$(LIBSTDCXX)/include \
          -I$(LIBSTDCXX)/include/riscv64-unknown-linux-gnu \
          -I$(LIBSTDCXX)/../libsupc++

LDFLAGS = -z separate-code -L$(LIBC)/lib -L$(LIBSTDCXX)/src/.libs -L$(LIBSTDCXX)/libsupc++/.libs #-Tlinker.ld

CRTBEGIN_OBJ := $(shell $(GCC) $(CFLAGS) -print-file-name=crtbeginT.o)
CRTEND_OBJ := $(shell $(GCC) $(CFLAGS) -print-file-name=crtend.o)

HEADERS=$(wildcard *.h)
SOURCES=$(wildcard *.c *.cpp)
OBJECTS=$(patsubst %.c,%.elf,$(wildcard *.c)) $(patsubst %.cpp,%.elf,$(wildcard *.cpp))

PRE_OBJS = $(LIBC)/lib/crt1.o $(LIBC)/lib/crti.o $(CRTBEGIN_OBJ)
POST_OBJS = $(CRTEND_OBJ) $(LIBC)/lib/crtn.o

.PHONY: all
all: hello.elf helloxx.elf empty.elf

%.elf: %.c $(HEADERS) linker.ld
	$(GCC) $(CFLAGS) $(LDFLAGS) $(PRE_OBJS) $< -Wl,--start-group -lgcc -lgcc_eh -lc -Wl,--end-group $(POST_OBJS) -o $@

%.elf: %.cpp $(HEADERS) linker.ld
	$(CC) $(CCFLAGS) $(CFLAGS) $(LDFLAGS) $(PRE_OBJS) $< -lstdc++ -lsupc++ -Wl,--start-group -lgcc -lgcc_eh -lc -Wl,--end-group $(POST_OBJS) -o $@

%.elf: %.S $(HEADERS) linker.ld
	$(GCC) $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: clean
clean:
	-rm *.elf
