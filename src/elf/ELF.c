#include <slink/elf/ELF.h>

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>


char *ELFFileClassName(uint8_t FileClass) {
    switch (FileClass) {
        case 0: return "ELFCLASSNONE";
        case 1: return "ELFCLASS32";
        case 2: return "ELFCLASS64";
    }
    return "{invalid file class}";
}

char *ELFDataEncodingName(uint8_t DataEncoding) {
    switch (DataEncoding) {
        case 0: return "ELFDATANONE";
        case 1: return "ELFDATA2LSB";
        case 2: return "ELFDATA2MSB";
    }
    return "{invalid data encoding}";
}

char *ELFFileVersionName(uint8_t FileVersion) {
    switch (FileVersion) {
        case 0: return "EV_NONE";
        case 1: return "EV_CURRENT";
    }
    return "{invalid file version}";
}

char *ELFOSABIIdentName(uint8_t OSABIIdent) {
    switch (OSABIIdent) {
        
        case 0: return "ELFOSABI_NONE";
        case 1: return "ELFOSABI_HPUX";
        case 2: return "ELFOSABI_NETBSD";
        case 3: return "ELFOSABI_LINUX";
        case 6: return "ELFOSABI_SOLARIS";
        case 7: return "ELFOSABI_AIX";
        case 8: return "ELFOSABI_IRIX";
        case 9: return "ELFOSABI_FREEBSD";
        case 10: return "ELFOSABI_TRU64";
        case 11: return "ELFOSABI_MODESTO";
        case 12: return "ELFOSABI_OPENBSD";
        case 13: return "ELFOSABI_OPENVMS";
        case 14: return "ELFOSABI_NSK";
        
        default:
            if (64 <= OSABIIdent /* || OSABIIdent <= 255 */) {
                return "Architecture-specific";
            }
    }
    return "{invalid os/abi ident}";
}

char *ELFTypeName(Elf64_Half e_type) {
    switch (e_type) {
        case 0: return "ET_NONE";
        case 1: return "ET_REL";
        case 2: return "ET_EXEC";
        case 3: return "ET_DYN";
        case 4: return "ET_CORE";
        case 0xfe00: return "ET_LOOS";
        case 0xfeff: return "ET_HIOS";
        case 0xff00: return "ET_LOPROC";
        case 0xffff: return "ET_HIPROC";
    }
    return "{invalid type}";
}

char *ELFMachineName(Elf64_Half e_machine) {
    switch (e_machine) {
        
        case 0: return "EM_NONE";
        case 1: return "EM_M32";
        case 2: return "EM_SPARC";
        case 3: return "EM_386";
        case 4: return "EM_68K";
        case 5: return "EM_88K";
        case 6: return "reserved";
        case 7: return "EM_860";
        case 8: return "EM_MIPS";
        case 9: return "EM_S370";
        case 10: return "EM_MIPS_RS3_LE";
        // case 11 ... 14: return "reserved";
        case 15: return "EM_PARISC";
        case 16: return "reserved";
        case 17: return "EM_VPP500";
        case 18: return "EM_SPARC32PLUS";
        case 19: return "EM_960";
        case 20: return "EM_PPC";
        case 21: return "EM_PPC64";
        case 22: return "EM_S390";
        // case 23 ... 35: return "reserved";
        case 36: return "EM_V800";
        case 37: return "EM_FR20";
        case 38: return "EM_RH32";
        case 39: return "EM_RCE";
        case 40: return "EM_ARM";
        case 41: return "EM_ALPHA";
        case 42: return "EM_SH";
        case 43: return "EM_SPARCV9";
        case 44: return "EM_TRICORE";
        case 45: return "EM_ARC";
        case 46: return "EM_H8_300";
        case 47: return "EM_H8_300H";
        case 48: return "EM_H8S";
        case 49: return "EM_H8_500";
        case 50: return "EM_IA_64";
        case 51: return "EM_MIPS_X";
        case 52: return "EM_COLDFIRE";
        case 53: return "EM_68HC12";
        case 54: return "EM_MMA";
        case 55: return "EM_PCP";
        case 56: return "EM_NCPU";
        case 57: return "EM_NDR1";
        case 58: return "EM_STARCORE";
        case 59: return "EM_ME16";
        case 60: return "EM_ST100";
        case 61: return "EM_TINYJ";
        case 62: return "EM_X86_64";
        case 63: return "EM_PDSP";
        case 64: return "EM_PDP10";
        case 65: return "EM_PDP11";
        case 66: return "EM_FX66";
        case 67: return "EM_ST9PLUS";
        case 68: return "EM_ST7";
        case 69: return "EM_68HC16";
        case 70: return "EM_68HC11";
        case 71: return "EM_68HC08";
        case 72: return "EM_68HC05";
        case 73: return "EM_SVX";
        case 74: return "EM_ST19";
        case 75: return "EM_VAX";
        case 76: return "EM_CRIS";
        case 77: return "EM_JAVELIN";
        case 78: return "EM_FIREPATH";
        case 79: return "EM_ZSP";
        case 80: return "EM_MMIX";
        case 81: return "EM_HUANY";
        case 82: return "EM_PRISM";
        case 83: return "EM_AVR";
        case 84: return "EM_FR30";
        case 85: return "EM_D10V";
        case 86: return "EM_D30V";
        case 87: return "EM_V850";
        case 88: return "EM_M32R";
        case 89: return "EM_MN10300";
        case 90: return "EM_MN10200";
        case 91: return "EM_PJ";
        case 92: return "EM_OPENRISC";
        case 93: return "EM_ARC_A5";
        case 94: return "EM_XTENSA";
        case 95: return "EM_VIDEOCORE";
        case 96: return "EM_TMM_GPP";
        case 97: return "EM_NS32K";
        case 98: return "EM_TPC";
        case 99: return "EM_SNP1K";
        case 100: return "EM_ST200";
        
        default:
            if (11 <= e_machine || e_machine <= 14) {
                return "reserved";
            } else if (23 <= e_machine || e_machine <= 35) {
                return "reserved";
            }
    }
    return "{invalid machine}";
}

char *ELFVersionName(Elf64_Word e_version) {
    switch (e_version) {
        case 0: return "EV_NONE";
        case 1: return "EV_CURRENT";
    }
    return "{invalid version}";
}

char *ELFSectionTypeName(Elf64_Word sh_type) {
    switch (sh_type) {
        case 0: return "SHT_NULL";
        case 1: return "SHT_PROGBITS";
        case 2: return "SHT_SYMTAB";
        case 3: return "SHT_STRTAB";
        case 4: return "SHT_RELA";
        case 5: return "SHT_HASH";
        case 6: return "SHT_DYNAMIC";
        case 7: return "SHT_NOTE";
        case 8: return "SHT_NOBITS";
        case 9: return "SHT_REL";
        case 10: return "SHT_SHLIB";
        case 11: return "SHT_DYNSYM";
        case 14: return "SHT_INIT_ARRAY";
        case 15: return "SHT_FINI_ARRAY";
        case 16: return "SHT_PREINIT_ARRAY";
        case 17: return "SHT_GROUP";
        case 18: return "SHT_SYMTAB_SHNDX";
        case 0x60000000: return "SHT_LOOS";
        case 0x6fffffff: return "SHT_HIOS";
        case 0x70000000: return "SHT_LOPROC";
        case 0x7fffffff: return "SHT_HIPROC";
        case 0x80000000: return "SHT_LOUSER";
        case 0xffffffff: return "SHT_HIUSER";
    }
    return "{invalid section type}";
}

Elf64_Xword ELF_SHFS[] = {
    0x1,
    0x2,
    0x4,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x0ff00000,
    0xf0000000,
};
int ELF_SHFS_CNT = sizeof(ELF_SHFS) / sizeof(ELF_SHFS[0]);

char *ELFSectionFlagName(Elf64_Xword sh_flag) {
    switch (sh_flag) {
        case 0x1: return "SHF_WRITE";
        case 0x2: return "SHF_ALLOC";
        case 0x4: return "SHF_EXECINSTR";
        case 0x10: return "SHF_MERGE";
        case 0x20: return "SHF_STRINGS";
        case 0x40: return "SHF_INFO_LINK";
        case 0x80: return "SHF_LINK_ORDER";
        case 0x100: return "SHF_OS_NONCONFORMING";
        case 0x200: return "SHF_GROUP";
        case 0x400: return "SHF_TLS";
        case 0x0ff00000: return "SHF_MASKOS";
        case 0xf0000000: return "SHF_MASKPROC";
    }
    return "{invalid section flag}";
}

Elf64_Word ELF_SPFS[] = {
    0x1,
    0x2,
    0x4,
    0x0ff00000,
    0xf0000000,
};
int ELF_SPFS_CNT = sizeof(ELF_SPFS) / sizeof(ELF_SPFS[0]);

char *ELFSegmentPermissionFlagName(Elf64_Word p_flags) {
    switch (p_flags) {
        case 0x1: return "PF_X";
        case 0x2: return "PF_W";
        case 0x4: return "PF_R";
        case 0x0ff00000: return "SHF_MASKOS";
        case 0xf0000000: return "SHF_MASKPROC";
    }
    return "{invalid section flag}";
}

char *ELFSymBindingName(unsigned char st_bind) {
    switch (st_bind) {
        case 0: return "STB_LOCAL";
        case 1: return "STB_GLOBAL";
        case 2: return "STB_WEAK";
        
        // case 10: return "STB_LOOS";
        // case 12: return "STB_HIOS";
        case 10:
        case 11:
        case 12:
            return "OS specific";
        
        // case 13: return "STB_LOPROC";
        // case 15: return "STB_HIPROC";
        case 13:
        case 14:
        case 15:
            return "Proc specific";
    }
    return "{invalid sym binding}";
}

char *ELFSymTypeName(unsigned char st_type) {
    switch (st_type) {
        case 0: return "STT_NOTYPE";
        case 1: return "STT_OBJECT";
        case 2: return "STT_FUNC";
        case 3: return "STT_SECTION";
        case 4: return "STT_FILE";
        case 5: return "STT_COMMON";
        case 6: return "STT_TLS";
        
        // case 10: return "STT_LOOS";
        // case 12: return "STT_HIOS";
        case 10:
        case 11:
        case 12:
            return "OS specific";
        
        // case 13: return "STT_LOPROC";
        // case 15: return "STT_HIPROC";
        case 13:
        case 14:
        case 15:
            return "Proc specific";
        
    }
    return "{invalid sym type}";
}

char *ELFSymVisibilityName(unsigned char st_other) {
    switch (st_other) {
        case 0: return "STV_DEFAULT";
        case 1: return "STV_INTERNAL";
        case 2: return "STV_HIDDEN";
        case 3: return "STV_PROTECTED";
    }
    return "{invalid sym visibility}";
}

char *ELFSpecialSectionName(Elf64_Word index) {
    switch (index) {
        case 0:         return "SHN_UNDEF";
        case 0xfff1:    return "SHN_ABS";
        case 0xfff2:    return "SHN_COMMON";
        case 0xffff:    return "SHN_XINDEX";
        default:
            if (0xff00 <= index /* && index <= 0xffff */) {
                if (0xff00 <= index && index <= 0xff1f) {
                    return "Proc specific";
                } else if (0xff20 <= index && index <= 0xff3f) {
                    return "OS specific";
                }
                return "Reserved";
            }
    }
    return 0;
}

int ELFIsShNdxSpecial(Elf64_Half index) {
    switch (index) {

        case 0:         /* return "SHN_UNDEF"; */
        case 0xfff1:    /* return "SHN_ABS"; */
        case 0xfff2:    /* return "SHN_COMMON"; */
        case 0xffff:    /* return "SHN_XINDEX"; */
            return 1;

        default:
            if (0xff00 <= index /* && index <= 0xffff */) {

                // if (0xff00 <= index && index <= 0xff1f) {
                //     return "Proc specific";
                // } else if (0xff20 <= index && index <= 0xff3f) {
                //     return "OS specific";
                // }
                // return "Reserved";

                return 1;
            }
    }
    return 0;
}

void ELFPrintIdent(ELFIdent *ident) {
    printf("FileIdent: %u [%.3s]\n", ident->FileIdent[0], &ident->FileIdent[1]);
    printf("FileClass: %u [%s]\n", ident->FileClass, ELFFileClassName(ident->FileClass));
    printf("DataEncoding: %u [%s]\n", ident->DataEncoding, ELFDataEncodingName(ident->DataEncoding));
    printf("FileVersion: %u [%s]\n", ident->FileVersion, ELFVersionName(ident->FileVersion));
    printf("OSABIIdent: %u [%s]\n", ident->OSABIIdent, ELFOSABIIdentName(ident->OSABIIdent));
    printf("ABIVersion: %u\n", ident->ABIVersion);
}

void ELFPrintEHdr(FILE *file, Elf *elf, Elf64_Ehdr *ehdr) {

    fprintf(file, "\n");
    fprintf(file, "File: %u [%s]\n", elf->index, elf->path);

    // printf("e_ident: %u\n", ehdr->e_ident);
    fprintf(file, "e_type: %u [%s]\n", ehdr->e_type, ELFTypeName(ehdr->e_type));
    fprintf(file, "e_machine: %u [%s]\n", ehdr->e_machine, ELFMachineName(ehdr->e_machine));
    fprintf(file, "e_version: %u [%s]\n", ehdr->e_version, ELFVersionName(ehdr->e_version));
    fprintf(file, "e_entry: %lu\n", ehdr->e_entry);
    fprintf(file, "e_phoff: %lu\n", ehdr->e_phoff);
    fprintf(file, "e_shoff: %lu\n", ehdr->e_shoff);
    fprintf(file, "e_flags: %u\n", ehdr->e_flags);
    fprintf(file, "e_ehsize: %u\n", ehdr->e_ehsize);
    fprintf(file, "e_phentsize: %u\n", ehdr->e_phentsize);
    fprintf(file, "e_phnum: %u\n", ehdr->e_phnum);
    fprintf(file, "e_shentsize: %u\n", ehdr->e_shentsize);
    fprintf(file, "e_shnum: %u\n", ehdr->e_shnum);
    fprintf(file, "e_shstrndx: %u\n", ehdr->e_shstrndx);
}

void ELFPrintPHdr(Elf64_Phdr *phdr) {
    printf("p_type: 0x%x [%s]\n", phdr->p_type, ELFSegmentTypeName(phdr->p_type));
    // printf("p_flags: %u\n", phdr->p_flags);
    // for (int i = 0; i < ELF_SPFS_CNT; i++) {
    //     Elf64_Word mf = ELF_SPFS[i] & phdr->p_flags;
    //     if (mf) {
    //         printf("    [%s]\n", ELFSegmentPermissionFlagName(mf));
    //     }
    // }
    // printf("p_offset: 0x%lx\n", phdr->p_offset);
    // printf("p_vaddr: 0x%lx\n", phdr->p_vaddr);
    // printf("p_paddr: 0x%lx\n", phdr->p_paddr);
    // printf("p_filesz: %lu\n", phdr->p_filesz);
    // printf("p_memsz: %lu\n", phdr->p_memsz);
    // printf("p_align: %lu\n", phdr->p_align);
}

char *ELFSegmentTypeName(Elf64_Word p_type) {
    switch (p_type) {
        case 0: return "PT_NULL";
        case 1: return "PT_LOAD";
        case 2: return "PT_DYNAMIC";
        case 3: return "PT_INTERP";
        case 4: return "PT_NOTE";
        case 5: return "PT_SHLIB";
        case 6: return "PT_PHDR";
        case 7: return "PT_TLS";
        default:
            if (0x60000000 <= p_type && p_type <= 0x6fffffff) {
                return "OS specific";
            } else if (0x70000000 <= p_type && p_type <= 0x7fffffff) {
                return "Proc specific";
            }
        
    }
    return "{invalid segment type}";
}

static long int flength(FILE *file) {
    fpos_t pos;
    fgetpos(file, &pos);
    fseek(file, 0, SEEK_END);
    long int length = ftell(file);
    fsetpos(file, &pos);
    return length;
}

static void ELFReadFD(FILE *in, Elf *elf) {

    long int length = flength(in);
    assert(length >= 0);
    
    elf->raw = malloc((size_t) length);
    fread(elf->raw, (size_t) length, 1, in);

    // set pointer to elf header
    elf->ehdr = (Elf64_Ehdr *) elf->raw;

    // find real section count
    elf->shnum = 0;
    if (elf->ehdr->e_shoff != 0) {
        elf->shnum = elf->ehdr->e_shnum;
        if (elf->shnum == 0) {
            Elf64_Shdr *shdr = (Elf64_Shdr *) &elf->raw[elf->ehdr->e_shoff];
            elf->shnum = shdr->sh_size;
        }
    }

    // find section headers
    elf->shdrs = 0;
    if (elf->shnum > 0) {

        // set pointer to section header table
        assert(elf->ehdr->e_shentsize == sizeof(Elf64_Shdr));
        elf->shdrs = (Elf64_Shdr *) &elf->raw[elf->ehdr->e_shoff];
    }

    // find program headers
    elf->phdrs = 0;
    if (elf->ehdr->e_phnum > 0) {

        // set pointer to program header table
        assert(elf->ehdr->e_phentsize == sizeof(Elf64_Phdr));
        elf->phdrs = (Elf64_Phdr *) &elf->raw[elf->ehdr->e_phoff];
    }

    // find real section name string table index
    elf->shstrndx = elf->ehdr->e_shstrndx;
    if (elf->shstrndx == SHN_XINDEX) {
        Elf64_Shdr *shdr = &elf->shdrs[0];
        elf->shstrndx = shdr->sh_link;
    }

    // set section name string table pointer
    elf->sec_name_str_tab = 0;
    if (elf->shstrndx != SHN_UNDEF) {
        Elf64_Shdr *shdr = &elf->shdrs[elf->shstrndx];
        elf->sec_name_str_tab = &elf->raw[shdr->sh_offset];
    }

    // set poitner to symtab
    elf->sym_tab = 0;
    for (size_t i = 0; i < elf->shnum; i++) {
        Elf64_Shdr *shdr = &elf->shdrs[i];
        if (shdr->sh_type == SHT_SYMTAB) {

            assert(shdr->sh_entsize == sizeof(Elf64_Sym));

            elf->sym_tab_sh = shdr;
            elf->sym_tab = (Elf64_Sym *) &elf->raw[shdr->sh_offset];
            elf->sym_cnt = shdr->sh_size / shdr->sh_entsize;

            Elf64_Shdr *strsh = &elf->shdrs[shdr->sh_link];
            elf->sym_str_tab = &elf->raw[strsh->sh_offset];
        }
    }
}

void ELFRead(char *path, Elf *elf) {
    FILE *in = fopen(path, "rb");
    assert(in);
    elf->path = path;
    ELFReadFD(in, elf);
    fclose(in);
}

void ELFReadFromMem(char *name, char *mem, size_t size, Elf *elf) {

    FILE *tmp = tmpfile();
    assert(tmp);
    fwrite(mem, 1, size, tmp);

    rewind(tmp);

    elf->path = name;
    ELFReadFD(tmp, elf);
    fclose(tmp);
}

void ELFPrintSymTab(FILE *file, Elf *elf) {
    
    if (elf->sym_tab == 0) {
        return;
    }

    char *path = strrchr(elf->path, '/') + 1;
    if (path == 0) {
        path = elf->path;
    }

    fprintf(file, "\n");
    for (size_t i = 0; i < elf->sym_cnt; i++) {

        Elf64_Sym *sym = &elf->sym_tab[i];

        char *sec_name = ELFSpecialSectionName(sym->st_shndx);
        if (sec_name == 0) {
            Elf64_Shdr *shdr = &elf->shdrs[sym->st_shndx];
            sec_name = &elf->sec_name_str_tab[shdr->sh_name];
        }

        fprintf(
            file,
            "%u [%s] [%s] [%s] 0x%lx\n",
            elf->index,
            path,
            &elf->sym_str_tab[sym->st_name],
            sec_name,
            sym->st_value);
    }
}

void ELFPrintStrTab(FILE *file, Elf *elf) {

    fprintf(file, "\n");
    fprintf(file, "File: %u [%s]\n", elf->index, elf->path);
}


void ELFPrintSHdr(FILE *file, Elf *elf, Elf64_Shdr *shdr) {
    
    char *path = strrchr(elf->path, '/') + 1;
    if (path == 0) {
        path = elf->path;
    }

    fprintf(
        file, 
        "%u [%s] [%s] [%s] %lu 0x%lx 0x%lx",
        elf->index,
        path,
        &elf->sec_name_str_tab[shdr->sh_name],
        ELFSectionTypeName(shdr->sh_type),
        shdr->sh_size,
        shdr->sh_addralign,
        shdr->sh_addr);

    for (int i = 0; i < ELF_SHFS_CNT; i++) {
        Elf64_Xword m = shdr->sh_flags & ELF_SHFS[i];
        if (m != 0) {
            fprintf(file, " [%s]", ELFSectionFlagName(m));
        }
    }

    fprintf(file, "\n");
}

static char *RelocTypeName(size_t type) {
    switch (type) {
        case 1: return "R_X86_64_64";
        case 2: return "R_X86_64_PC32";
        case 10: return "R_X86_64_32";
        case 11: return "R_X86_64_32S";
    }
    fprintf(stderr, "Can't handle reloc type: %lu\n", type);
    return "{unknown reloc type}";
}

void ELFPrintRelocs(FILE *file, Elf *elf, Elf64_Shdr *shdr) {

    assert(shdr->sh_type == SHT_RELA);

    char *path = strrchr(elf->path, '/') + 1;
    if (path == 0) {
        path = elf->path;
    }

    for (size_t off = 0; off < shdr->sh_size; off += shdr->sh_entsize) {

        Elf64_Rela *reloc = (Elf64_Rela *) &elf->raw[shdr->sh_offset + off];

        Elf64_Sym *sym = &elf->sym_tab[ELF64_R_SYM(reloc->r_info)];
        char *sym_name = &elf->sym_str_tab[sym->st_name];

        fprintf(
            file, 
            "[%s] [%s] %li %lu [%s] %lu [%s]\n", 
            path,
            &elf->sec_name_str_tab[shdr->sh_name],
            // reloc->r_offset,
            reloc->r_addend,
            ELF64_R_SYM(reloc->r_info),
            sym_name,
            ELF64_R_TYPE(reloc->r_info),
            RelocTypeName(ELF64_R_TYPE(reloc->r_info)));
    }
}

int IsElf(char *path) {

    FILE *file = fopen(path, "rb");
    assert(file);

    char magic[4];
    fread(magic, 1, 4, file);

    fclose(file);

    return strncmp(ELF_MAGIC, magic, 4) == 0;
}
