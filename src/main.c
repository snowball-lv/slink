#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

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
        
        printf("\n");
        
        printf("e_type: %u [%s]\n", ehdr.e_type, ELFTypeName(ehdr.e_type));
        printf("e_machine: %u [%s]\n", ehdr.e_machine, ELFMachineName(ehdr.e_machine));
        printf("e_version: %u [%s]\n", ehdr.e_version, ELFVersionName(ehdr.e_version));
        
        printf("e_entry: %lu\n", ehdr.e_entry);
        printf("e_phoff: %lu\n", ehdr.e_phoff);
        printf("e_shoff: %lu\n", ehdr.e_shoff);
        printf("e_flags: %u\n", ehdr.e_flags);
        printf("e_ehsize: %u\n", ehdr.e_ehsize);
        printf("e_phentsize: %u\n", ehdr.e_phentsize);
        printf("e_phnum: %u\n", ehdr.e_phnum);
        printf("e_shentsize: %u\n", ehdr.e_shentsize);
        printf("e_shnum: %u\n", ehdr.e_shnum);
        printf("e_shstrndx: %u\n", ehdr.e_shstrndx);
        
        if (ehdr.e_shoff != 0) {
            
            assert(ehdr.e_shoff <= LONG_MAX);
            fseek(in, (long int) ehdr.e_shoff, SEEK_SET);
            
            uint64_t shnum = ehdr.e_shnum;
            if (shnum == SHN_UNDEF) {
                fpos_t pos;
                fgetpos(in, &pos);
                Elf64_Shdr shdr;
                fread(&shdr, sizeof(shdr), 1, in);
                shnum = shdr.sh_size;
                fsetpos(in, &pos);
            }
            
            printf("\n");
            printf("real shnum: %lu\n", shnum);
            
            char *shdrs_raw = malloc(shnum * ehdr.e_shentsize);
            fread(shdrs_raw, ehdr.e_shentsize, shnum, in);
            
            char **sec_datas = malloc(shnum * sizeof(void *));

            for (uint64_t k = 0; k < shnum; k++) {
                Elf64_Shdr *shdr = (Elf64_Shdr *) &shdrs_raw[k * ehdr.e_shentsize];
                if (shdr->sh_type != SHT_NOBITS) {
                    char *data = malloc(shdr->sh_size);
                    
                    assert(ehdr.e_shoff <= LONG_MAX);
                    fseek(in, (long int) shdr->sh_offset, SEEK_SET);
                    fread(data, shdr->sh_size, 1, in);
                    
                    sec_datas[k] = data;
                } else {
                    sec_datas[k] = 0;
                }
            }
            
            for (uint64_t k = 0; k < shnum; k++) {
                
                Elf64_Shdr *shdr = (Elf64_Shdr *) &shdrs_raw[k * ehdr.e_shentsize];
                printf("\n");
                
                char *str_tab = sec_datas[ehdr.e_shstrndx];
                printf("sh_name: %u [%s]\n", shdr->sh_name, &str_tab[shdr->sh_name]);
                
                printf("sh_type: %u [%s]\n", shdr->sh_type, ELFSectionTypeName(shdr->sh_type));
                
                printf("sh_flags: 0x%lx\n", shdr->sh_flags);
                for (int fi = 0; fi < ELF_SHFS_CNT; fi++) {
                    Elf64_Xword f = ELF_SHFS[fi];
                    if (f & shdr->sh_flags) {
                        printf("    [%s]\n", ELFSectionFlagName(f & shdr->sh_flags));
                    }
                }
                
                // printf("sh_addr: %lu\n", shdr->sh_addr);
                // printf("sh_offset: %lu\n", shdr->sh_offset);
                // printf("sh_size: %lu\n", shdr->sh_size);
                // printf("sh_link: %u\n", shdr->sh_link);
                // printf("sh_info: %u\n", shdr->sh_info);
                // printf("sh_addralign: %lu\n", shdr->sh_addralign);
                // printf("sh_entsize: %lu\n", shdr->sh_entsize);
            }
        }
        
        fclose(in);
    }
    
    return 0;
}
