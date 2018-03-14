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
    
    
    fclose(in);
    
    return 0;
}