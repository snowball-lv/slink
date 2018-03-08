#include <slink/elf/ELF.h>

int dummy;

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
            if (64 <= OSABIIdent || OSABIIdent <= 255) {
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
