#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include <slink/elf/ELF.h>


int main(int argc, char **argv) {
    printf("--- S LINK ---\n");
    
    char *file = argv[0];
    printf("Input: [%s]\n", file);
    FILE *in = fopen(file, "rb");
    assert(in);
    
    fpos_t pos;
    fgetpos(in, &pos);
    
    ELFIdent ident;
    fread(&ident, sizeof(ident), 1, in);
    printf("\n");
    ELFPrintIdent(&ident);
    
    fsetpos(in, &pos);
    
    Elf64_Ehdr ehdr;
    fread(&ehdr, sizeof(ehdr), 1, in);
    printf("\n");
    ELFPrintEHdr(&ehdr);
    
    if (ehdr.e_phoff != 0) {
    
        char *phdrs_raw = malloc((size_t) (ehdr.e_phnum * ehdr.e_phentsize));
        
        assert(ehdr.e_phoff <= LONG_MAX);
        fseek(in, (long int) ehdr.e_phoff, SEEK_SET);
        fread(phdrs_raw, ehdr.e_phentsize, ehdr.e_phnum, in);
    
        for (int i = 0; i < ehdr.e_phnum; i++) {
            Elf64_Phdr *phdr = (Elf64_Phdr *) &phdrs_raw[i * ehdr.e_phentsize];
            printf("\n");
            ELFPrintPHdr(phdr);
        }
    }
    
    fclose(in);
    
    return 0;
}