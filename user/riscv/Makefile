ARCH = riscv
PREFIX = riscv64-unknown-linux-gnu-
GCC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

CFLAGS = -fno-builtin -nostdlib -static -Wl,--gc-sections,--print-gc-sections -O2 -Wall \
         -Iinclude $(DEFINES)
LDFLAGS = -z separate-code

HEADERS=$(wildcard include/*.h)
SOURCES=$(wildcard *.c *.S)
OBJECTS=$(patsubst %.c,%.elf,$(wildcard *.c)) $(patsubst %.S,%.elf,$(wildcard *.S))

.PHONY: all
all: hello.elf

init.o: init.S $(HEADERS) linker.ld
	$(GCC) $(CFLAGS) -c $< -o $@

syscall.o: syscall.c $(HEADERS) linker.ld
	$(GCC) $(CFLAGS) -c $< -o $@

%.elf: %.c $(HEADERS) linker.ld init.o syscall.o
	$(GCC) -Tlinker.ld $(CFLAGS) $(LDFLAGS) $< init.o syscall.o -o $@

%.elf: %.S $(HEADERS) linker.ld init.o syscall.o
	$(GCC) -Tlinker.ld $(CFLAGS) $(LDFLAGS) $< init.o syscall.o -o $@

.PHONY: clean
clean:
	-rm *.elf
