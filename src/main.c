#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <slink/elf/ELF.h>


int main(int argc, char **argv) {
    printf("--- S LINK ---\n");
    
    FILE *log_sym = fopen("log-sym.txt", "wb");
    FILE *log_sec = fopen("log-sec.txt", "wb");
    FILE *log_ehdr = fopen("log-ehdr.txt", "wb");

    assert(log_sym);
    assert(log_sec);
    assert(log_ehdr);

    printf("Inputs: %i\n", argc - 1);

    Elf *elfs = malloc(sizeof(Elf) * (unsigned) (argc - 1));

    for (int i = 1; i < argc; i++) {
        char *path = argv[i];
        Elf *elf = &elfs[i - 1];
        ELFRead(path, elf);
        elf->index = i - 1;
    }

    for (int i = 1; i < argc; i++) {

        Elf *elf = &elfs[i - 1];

        printf("\n");
        printf("File: %i [%s]\n", elf->index, elf->path);
        printf("shnum: %lu\n", elf->shnum);
        printf("shstrndx: %u\n", elf->shstrndx);

        ELFPrintEHdr(log_ehdr, elf, elf->ehdr);

        for (size_t k = 0; k < elf->shnum; k++) {
            Elf64_Shdr *shdr = &elf->shdrs[k];
            ELFPrintSHdr(log_sec, elf, shdr);
        }

        // printf("\n");
        // for (size_t k = 0; k < elf->ehdr->e_phnum; k++) {
        //     Elf64_Phdr *phdr = &elf->phdrs[k];
        //     ELFPrintPHdr(phdr);
        // }

        ELFPrintSymTab(log_sym, elf);
    }

    fclose(log_sym);
    fclose(log_sec);
    fclose(log_ehdr);

    return 0;
}
