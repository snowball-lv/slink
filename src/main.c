#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <slink/elf/ELF.h>


int main(int argc, char **argv) {
    printf("--- S LINK ---\n");
    
    printf("\n");
    int in_cnt = argc - 1;
    printf("Inputs: %d\n", in_cnt);
    
    for (int i = 1; i < argc; i++) {
        
        char *name = argv[i];
        printf("\n");
        printf("File: [%s]\n", name);
        
        FILE *in = fopen(name, "rb");
        assert(in);
        
        fpos_t pos;
        fgetpos(in, &pos);
        
        ELFIdent ident;
        fread(&ident, sizeof(ident), 1, in);      
        
        printf("\n");  
        printf("FileIdent: 0x%x %.3s\n", ident.FileIdent[0], &ident.FileIdent[1]);
        printf("FileClass: %u [%s]\n", ident.FileClass, ELFFileClassName(ident.FileClass));
        printf("DataEncoding: %u [%s]\n", ident.DataEncoding, ELFDataEncodingName(ident.DataEncoding));
        printf("FileVersion: %u [%s]\n", ident.FileVersion, ELFFileVersionName(ident.FileVersion));
        printf("OSABIIdent: %u [%s]\n", ident.OSABIIdent, ELFOSABIIdentName(ident.OSABIIdent));
        printf("ABIVersion: %u\n", ident.ABIVersion);
        
        fsetpos(in, &pos);
        
        Elf64_Ehdr ehdr;
        fread(&ehdr, sizeof(ehdr), 1, in);
        
        fclose(in);
    }
    
    return 0;
}
