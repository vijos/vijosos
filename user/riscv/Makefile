ARCH = riscv
PREFIX = riscv64-unknown-linux-gnu-
GCC = $(PREFIX)gcc
CC = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

LIBC = ../../../musl-1.2.0

LIB = $(LIBC)/lib

CFLAGS = -fno-builtin -nostdlib -static -Wl,--gc-sections,--print-gc-sections -O2 -Wall \
         -march=rv64imac -I$(LIBC)/include -I$(LIBC)/obj/include $(DEFINES)
LDFLAGS = -z separate-code -L$(LIB) #-Tlinker.ld

HEADERS=$(wildcard include/*.h)
SOURCES=$(wildcard *.c *.S)
OBJECTS=$(patsubst %.c,%.elf,$(wildcard *.c)) $(patsubst %.S,%.elf,$(wildcard *.S))

.PHONY: all
all: hello.elf

%.elf: %.c $(HEADERS) linker.ld
	$(GCC) $(CFLAGS) $(LDFLAGS) $(LIB)/crt1.o $(LIB)/crti.o $< -lc $(LIB)/crtn.o -o $@

%.elf: %.S $(HEADERS) linker.ld
	$(GCC) $(CFLAGS) $(LDFLAGS) $< -lc -o $@

.PHONY: clean
clean:
	-rm *.elf
