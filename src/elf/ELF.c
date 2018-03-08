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
        
        case 64:
        case 255: 
            return "Architecture-specific";
    }
    return "{invalid os/abi ident}";
}
