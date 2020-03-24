#include "arch/asm.h"
#include "arch/csr.h"
#include "printf.h"

const char *const cause_description[] =
{
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store/AMO address misaligned",
    "Store/AMO access fault",
    "Environment call from U-mode",
    "Environment call from S-mode",
    "Reserved",
    "Environment call from M-mode",
    "Instruction page fault",
    "Load page fault",
    "Reserved",
    "Store/AMO page fault"
};

void trap_handler()
{
    printf("\n\nEXCEPTION:\n");
    uint64_t cause = read_csr(mcause);
    printf("mcause = 0x%016lx (%s)\n", cause, cause < 16 ? cause_description[cause] : "Reserved");
    printf("mepc   = 0x%016lx\n", read_csr(mepc));
    printf("mtval  = 0x%016lx\n", read_csr(mtval));
}
