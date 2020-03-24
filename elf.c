#include "elf.h"
#include "stdio.h"
#include "string.h"

void *load_elf(void *elf)
{
    uint8_t *u8 = (uint8_t *)elf;

    Elf64_Ehdr *elf_header = (Elf64_Ehdr *)u8;

    if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG))
    {
        puts("Loader: Not an ELF file.\n");
        return 0;
    }

    if (elf_header->e_type != ET_EXEC)
    {
        puts("Loader: Not an executable ELF file.\n");
        return 0;
    }

    void *kernel_entry = (void *)elf_header->e_entry;

    printf("Loader: Kernel Entry is at 0x%016lx.\n", kernel_entry);

    if (!elf_header->e_phnum)
    {
        puts("Loader: No program header.\n");
        return 0;
    }

    if (elf_header->e_phentsize != sizeof(Elf64_Phdr))
    {
        puts("Loader: elf_header->e_phentsize != sizeof(Elf64_Phdr)\n");
        return 0;
    }

    Elf64_Phdr *program_header = (Elf64_Phdr *)(u8 + elf_header->e_phoff);

    puts("+-----------------------------------------------------------------------------------------------------+\n"
         "|                                            PROGRAM HEADERS                                          |\n"
         "+-----------------------------------------------------------------------------------------------------+\n"
         "    TYPE OFFSET             VIRT ADDR          PHY ADDR           FILE SIZE          MEM SIZE  \n");
    for (int i = 0; i < elf_header->e_phnum; ++i)
    {
        printf("%02d: ", i);
        if (program_header[i].p_type == PT_LOAD)
        {
            printf("LOAD");
        }
        else
        {
            printf("????");
        }
        printf(" 0x%016lx 0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
               program_header[i].p_offset, program_header[i].p_vaddr,
               program_header[i].p_paddr, program_header[i].p_filesz, program_header[i].p_memsz);
    }

    for (int i = 0; i < elf_header->e_phnum; ++i)
    {
        if (program_header[i].p_type == PT_LOAD)
        {
            printf("Loader: Loading %02d... ", i);

            memcpy((void *)program_header[i].p_paddr, u8 + program_header[i].p_offset,
                   program_header[i].p_filesz);

            if (program_header[i].p_filesz < program_header[i].p_memsz)
            {
                memset((void *)(program_header[i].p_paddr + program_header[i].p_filesz),
                       0, program_header[i].p_memsz - program_header[i].p_filesz);
                puts("init .bss... ");
            }
            puts("done.\n");
        }
    }

    puts("Loader: done.\n");
    return kernel_entry;
}
