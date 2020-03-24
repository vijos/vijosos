ARCH = riscv
PREFIX = riscv64-unknown-linux-gnu-
GCC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

DEFINES = -DPRINTF_DISABLE_SUPPORT_FLOAT -DPRINTF_DISABLE_SUPPORT_EXPONENTIAL \
          -DPRINTF_DISABLE_SUPPORT_LONG_LONG
CFLAGS = -fno-builtin -nostdlib -static -Wl,--gc-sections,--print-gc-sections -O2 -Wall \
         -Iinclude $(DEFINES)
LDFLAGS = -z separate-code

-include Makefile.config

.PHONY: all
all:
include arch/$(ARCH)/Makefrag

HEADERS=$(wildcard include/*.h) $(wildcard arch/$(ARCH)/include/*.h) $(wildcard driver/include/*.h)
SOURCES=$(wildcard *.c *.S) $(wildcard arch/$(ARCH)/*.c arch/$(ARCH)/*.S) $(wildcard driver/*.c driver/*.S)
OBJECTS=$(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.S,%.o,$(wildcard *.S)) \
        $(patsubst %.c,%.o,$(wildcard arch/$(ARCH)/*.c)) $(patsubst %.S,%.o,$(wildcard arch/$(ARCH)/*.S)) \
        $(patsubst %.c,%.o,$(wildcard driver/*.c)) $(patsubst %.S,%.o,$(wildcard driver/*.S))

.PHONY: all
all: $(OBJECTS) kernel tftp_root

.PHONY: asm
asm: kernel
	$(OBJDUMP) -xd $< | vi -

include/arch:
	-rm $@
	ln -s ../arch/$(ARCH)/include include/arch

include/driver:
	-rm $@
	ln -s ../driver/include include/driver

%.o: %.c $(HEADERS) include/arch include/driver
	$(GCC) $(CFLAGS) -c $< -o $@

%.o: %.S $(HEADERS) include/arch include/driver
	$(GCC) $(CFLAGS) -c $< -o $@

kernel: $(OBJECTS) arch/$(ARCH)/linker.ld
	$(GCC) -Tarch/$(ARCH)/linker.ld $(CFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: tftp_root
tftp_root: kernel riscv_soc.dtb
	mkdir -p $@
	cp kernel $@/
	cp riscv_soc.dtb $@/dtb

.PHONY: clean
clean:
	-rm *.o *.elf kernel arch/*/*.o
	-rm include/arch
	-rm include/driver
	-rm -r tftp_root
