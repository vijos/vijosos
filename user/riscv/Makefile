ARCH = riscv
PREFIX = riscv64-unknown-linux-gnu-
GCC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

CFLAGS = -fno-builtin -nostdlib -static -Wl,--gc-sections,--print-gc-sections -O2 -Wall \
         $(DEFINES)
LDFLAGS = -z separate-code

HEADERS=$(wildcard include/*.h)
SOURCES=$(wildcard *.c *.S)
OBJECTS=$(patsubst %.c,%.elf,$(wildcard *.c)) $(patsubst %.S,%.elf,$(wildcard *.S))

.PHONY: all
all: $(OBJECTS)

%.elf: %.c $(HEADERS) linker.ld
	$(GCC) -Tlinker.ld $(CFLAGS) $(LDFLAGS) $< -o $@

%.elf: %.S $(HEADERS) linker.ld
	$(GCC) -Tlinker.ld $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: clean
clean:
	-rm *.elf
