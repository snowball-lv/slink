#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include <slink/elf/ELF.h>


static void process_obj(FILE *in);

int main(int argc, char **argv) {
    printf("--- S LINK ---\n");
    
    printf("\n");
    int in_cnt = argc - 1;
    printf("Inputs: %d\n", in_cnt);
    
    
    for (int i = 1; i < 2; i++) {
        
        char *name = argv[i];
        printf("\n");
        printf("File: [%s]\n", name);
        
        FILE *in = fopen(name, "rb");
        assert(in);
        process_obj(in);
        fclose(in);
    }
    
    return 0;
    
    for (int i = 1; i < 2; i++) {
        
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
            
            // for (uint64_t k = 0; k < shnum; k++) {
            //     
            //     Elf64_Shdr *shdr = (Elf64_Shdr *) &shdrs_raw[k * ehdr.e_shentsize];
            //     printf("\n");
            //     
            //     char *str_tab = sec_datas[ehdr.e_shstrndx];
            //     printf("sh_name: %u [%s]\n", shdr->sh_name, &str_tab[shdr->sh_name]);
            //     
            //     printf("sh_type: %u [%s]\n", shdr->sh_type, ELFSectionTypeName(shdr->sh_type));
            //     
            //     printf("sh_flags: 0x%lx\n", shdr->sh_flags);
            //     for (int fi = 0; fi < ELF_SHFS_CNT; fi++) {
            //         Elf64_Xword f = ELF_SHFS[fi];
            //         if (f & shdr->sh_flags) {
            //             printf("    [%s]\n", ELFSectionFlagName(f & shdr->sh_flags));
            //         }
            //     }
            //     
            //     // printf("sh_addr: %lu\n", shdr->sh_addr);
            //     // printf("sh_offset: %lu\n", shdr->sh_offset);
            //     // printf("sh_size: %lu\n", shdr->sh_size);
            //     // printf("sh_link: %u\n", shdr->sh_link);
            //     // printf("sh_info: %u\n", shdr->sh_info);
            //     // printf("sh_addralign: %lu\n", shdr->sh_addralign);
            //     // printf("sh_entsize: %lu\n", shdr->sh_entsize);
            // }
            
            for (uint64_t k = 0; k < shnum; k++) {
                Elf64_Shdr *shdr = (Elf64_Shdr *) &shdrs_raw[k * ehdr.e_shentsize];
                if (shdr->sh_type == SHT_SYMTAB) {
                
                    printf("\n");
                    
                    char *str_tab = sec_datas[ehdr.e_shstrndx];
                    printf("sh_name: %u [%s]\n", shdr->sh_name, &str_tab[shdr->sh_name]);
                    printf("sh_offset: %lu\n", shdr->sh_offset);
                    printf("sh_size: %lu\n", shdr->sh_size);
                    printf("sh_entsize: %lu\n", shdr->sh_entsize);
                    
                    printf("\n");
                    
                    char *data = sec_datas[k];
                    for (size_t off = 0; off < shdr->sh_size; off += shdr->sh_entsize) {
                        
                        Elf64_Sym *sym = (Elf64_Sym *) &data[off];
                        char *str_tab = sec_datas[shdr->sh_link];
                        
                    	// printf("st_name: %u [%s]\n", sym->st_name, &str_tab[sym->st_name]);
                    	// printf("st_info: %u\n", sym->st_info);
                        // printf("    %u [%s]\n", ELF64_ST_BIND(sym->st_info), ELFSymBindingName(ELF64_ST_BIND(sym->st_info)));
                        // printf("    %u [%s]\n", ELF64_ST_TYPE(sym->st_info), ELFSymTypeName(ELF64_ST_TYPE(sym->st_info)));
                    	// printf("st_other: %u\n", sym->st_other);
                        // printf("    %u [%s]\n", ELF64_ST_VISIBILITY(sym->st_other), ELFSymVisibilityName(ELF64_ST_VISIBILITY(sym->st_other)));
                    	
                        char *sh_name = ELFSpecialSectionName(sym->st_shndx);
                        if (sh_name == 0) {
                            char *sh_str_tab = sec_datas[ehdr.e_shstrndx];
                            Elf64_Shdr *shdr = (Elf64_Shdr *) &shdrs_raw[sym->st_shndx * ehdr.e_shentsize];
                            sh_name = &sh_str_tab[shdr->sh_name];
                            
                        }
                        // printf("st_shndx: %u [%s]\n", sym->st_shndx, sh_name);
                    	// 
                        // printf("st_value: %lu\n", sym->st_value);
                    	// printf("st_size: %lu\n", sym->st_size);
                        
                    	printf("[%s] [%s]\n", sh_name, &str_tab[sym->st_name]);
                    }
                }
            }
        }
        
        fclose(in);
    }
    
    return 0;
}

static void process_obj(FILE *in) {
    
    fpos_t pos;
    fgetpos(in, &pos);
    
    ELFIdent ident;
    fread(&ident, sizeof(ident), 1, in);      
    
    // check magic number, bits, endianness
    assert(strncmp(ident.FileIdent, ELF_MAGIC, 4) == 0);
    assert(ident.FileClass == ELFCLASS64);
    assert(ident.DataEncoding == ELFDATA2LSB);
    
    fsetpos(in, &pos);
    
    Elf64_Ehdr ehdr;
    fread(&ehdr, sizeof(ehdr), 1, in);
    
    // make sure it's an amd64 relocatable
    assert(ehdr.e_type == ET_REL);
    assert(ehdr.e_machine == EM_X86_64);
    assert(ehdr.e_version== EV_CURRENT);
    assert(ehdr.e_flags == 0);
    
    // ...without any executable data
    assert(ehdr.e_entry == 0);
    assert(ehdr.e_phoff == 0);
    assert(ehdr.e_phentsize == 0);
    assert(ehdr.e_phnum == 0);
    
    // pointers to section headers and data
    size_t shnum = 0;
    char *shdrs_raw = 0;
    char **sec_data_array = 0;
    
    // read in section data if available
    if (ehdr.e_shoff != 0) {
        
        assert(ehdr.e_shoff <= LONG_MAX);
        fseek(in, (long int) ehdr.e_shoff, SEEK_SET);
        
        // read in null section header
        fpos_t pos;
        fgetpos(in, &pos);
        Elf64_Shdr shdr_null;
        fread(&shdr_null, sizeof(shdr_null), 1, in);
        fsetpos(in, &pos);
        
        // get real number of section entries
        assert(sizeof(size_t) >= sizeof(ehdr.e_shnum));
        shnum = ehdr.e_shnum;
        if (shnum == SHN_UNDEF) {
            shnum = shdr_null.sh_size;
        } else {
            assert(shdr_null.sh_size == 0);
        }
        
        // allocate space for section headers
        shdrs_raw = malloc(shnum * ehdr.e_shentsize);
        fread(shdrs_raw, ehdr.e_shentsize, shnum, in);
        
        // allcoate space for section data
        sec_data_array = malloc(shnum * sizeof(void *));
        
        // read in section data
        for (size_t i = 0; i < shnum; i++) {
            Elf64_Shdr *shdr = (Elf64_Shdr *) &shdrs_raw[i * ehdr.e_shentsize];
            char *data = 0;
            if (shdr->sh_type != SHT_NOBITS) {
                data = malloc(shdr->sh_size);
                assert(ehdr.e_shoff <= LONG_MAX);
                fseek(in, (long int) shdr->sh_offset, SEEK_SET);
                fread(data, shdr->sh_size, 1, in);
            }    
            sec_data_array[i] = data;
        }
    }
    
    // symbol table header and data
    Elf64_Shdr *sym_tab_shdr = 0;
    char *sym_tab_data = 0;
    
    // find symbol table header and data
    for (size_t i = 0; i < shnum; i++) {
        Elf64_Shdr *shdr = (Elf64_Shdr *) &shdrs_raw[i * ehdr.e_shentsize];
        if (shdr->sh_type == SHT_SYMTAB) {
            sym_tab_shdr = shdr;
            sym_tab_data = sec_data_array[i];
            break;
        }
    }
    
    if (sym_tab_shdr != 0) {
        char *str_tab = sec_data_array[sym_tab_shdr->sh_link];
        for (size_t off = 0; off < sym_tab_shdr->sh_size; off += sym_tab_shdr->sh_entsize) {
            
            Elf64_Sym *sym = (Elf64_Sym *) &sym_tab_data[off];

            char *sh_name = ELFSpecialSectionName(sym->st_shndx);
            if (sh_name == 0) {
                char *sh_str_tab = sec_data_array[ehdr.e_shstrndx];
                Elf64_Shdr *shdr = (Elf64_Shdr *) &shdrs_raw[sym->st_shndx * ehdr.e_shentsize];
                sh_name = &sh_str_tab[shdr->sh_name];
            }
            
            printf("[%s] [%s]\n", sh_name, &str_tab[sym->st_name]);
        }
    }
}
