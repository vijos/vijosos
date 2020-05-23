#include "elf.h"
#include "error.h"
#include "vm.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"

int load_elf(pte_t *pt, void *elf, size_t len, uintptr_t *entry_va)
{
    uint8_t *u8 = (uint8_t *)elf;

    if (len < sizeof(Elf64_Ehdr))
    {
        puts("Loader: ELF file is too small (Elf64_Ehdr).\n");
        return -EOOB;
    }

    Elf64_Ehdr *elf_header = (Elf64_Ehdr *)u8;

    if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG))
    {
        printf("Loader: Not an ELF file (0x%08x).\n", *(uint64_t *)elf_header->e_ident);
        return -EBADPKT;
    }

    if (elf_header->e_type != ET_EXEC)
    {
        puts("Loader: Not an executable ELF file.\n");
        return -EBADPKT;
    }

    uintptr_t entry = elf_header->e_entry;
    if (entry_va) *entry_va = entry;

    printf("Loader: ELF Entry is at 0x%016lx.\n", entry);

    if (!elf_header->e_phnum)
    {
        puts("Loader: No program header.\n");
        return -EBADPKT;
    }

    if (elf_header->e_phentsize != sizeof(Elf64_Phdr))
    {
        puts("Loader: elf_header->e_phentsize != sizeof(Elf64_Phdr)\n");
        return -ENOIMPL;
    }

    size_t phsize = elf_header->e_phnum * sizeof(Elf64_Phdr);
    if (!(elf_header->e_phoff <= elf_header->e_phoff + phsize && elf_header->e_phoff + phsize <= len))
    {
        puts("Loader: ELF file is too small (Elf64_Phdr).\n");
        return -EOOB;
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)(u8 + elf_header->e_phoff);

    puts("+-----------------------------------------------------------------------------------------------------+\n"
         "|                                            PROGRAM HEADERS                                          |\n"
         "+-----------------------------------------------------------------------------------------------------+\n"
         "    TYPE OFFSET             VIRT ADDR          PHY ADDR           FILE SIZE          MEM SIZE  \n");
    for (int i = 0; i < elf_header->e_phnum; ++i)
    {
        printf("%02d: ", i);
        if (phdr[i].p_type == PT_LOAD)
        {
            printf("LOAD");
        }
        else
        {
            printf("????");
        }
        printf(" 0x%016lx 0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
               phdr[i].p_offset, phdr[i].p_vaddr,
               phdr[i].p_paddr, phdr[i].p_filesz, phdr[i].p_memsz);
    }

    for (int i = 0; i < elf_header->e_phnum; ++i)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            printf("Loader: Loading %02d... ", i);

            if (phdr[i].p_filesz && !(phdr[i].p_offset <= phdr[i].p_offset + phdr[i].p_filesz
                                      && phdr[i].p_offset + phdr[i].p_filesz <= len))
            {
                puts("ELF file is too small.\n");
                return -EOOB;
            }
            if (!(USER_BOTTOM <= phdr[i].p_vaddr
                  && phdr[i].p_vaddr <= phdr[i].p_vaddr + phdr[i].p_memsz
                  && phdr[i].p_vaddr + phdr[i].p_memsz <= USER_TOP))
            {
                puts("virtual address out of bound.\n");
                return -EOOB;
            }
            if (phdr[i].p_filesz > phdr[i].p_memsz)
            {
                puts("Bad ELF file (p_filesz > p_memsz).\n");
                return -EBADPKT;
            }

            uintptr_t flags = VM_U | VM_A;
            if (phdr[i].p_flags & PF_R) flags |= VM_R;
            if (phdr[i].p_flags & PF_W) flags |= VM_W | VM_D;
            if (phdr[i].p_flags & PF_X) flags |= VM_X;
            int ret = map_and_copy(pt, phdr[i].p_vaddr, flags,
                                   u8 + phdr[i].p_offset, phdr[i].p_filesz);
            if (ret < 0)
            {
                puts("out of memory.\n");
                return ret;
            }
            if (phdr[i].p_filesz < phdr[i].p_memsz)
            {
                puts("init .bss... ");
                size_t file_end = PGALIGN_FLOOR(phdr[i].p_vaddr + phdr[i].p_filesz),
                       mem_end = PGALIGN(phdr[i].p_vaddr + phdr[i].p_memsz);
                for (size_t pg = file_end; pg < mem_end; pg += PGSIZE)
                {
                    if (!map_page(pt, pg, flags))
                    {
                        puts("out of memory.\n");
                        return -EOOM;
                    }
                }
            }
            puts("done.\n");
        }
    }

    puts("Loader: done.\n");
    return 0;
}
