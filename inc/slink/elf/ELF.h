#pragma once

#include <stdint.h>


typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;

#define EI_NIDENT 16

typedef union {
    struct {
        char    FileIdent[4];
        uint8_t FileClass;
        uint8_t DataEncoding;
        uint8_t FileVersion;
        uint8_t OSABIIdent;
        uint8_t ABIVersion;
    };
    char __total_size[EI_NIDENT];
} ELFIdent;

typedef struct {
    unsigned char   e_ident[EI_NIDENT];
    Elf64_Half      e_type;
    Elf64_Half      e_machine;
    Elf64_Word      e_version;
    Elf64_Addr      e_entry;
    Elf64_Off       e_phoff;
    Elf64_Off       e_shoff;
    Elf64_Word      e_flags;
    Elf64_Half      e_ehsize;
    Elf64_Half      e_phentsize;
    Elf64_Half      e_phnum;
    Elf64_Half      e_shentsize;
    Elf64_Half      e_shnum;
    Elf64_Half      e_shstrndx;
} Elf64_Ehdr;

char *ELFFileClassName(uint8_t FileClass);
char *ELFDataEncodingName(uint8_t DataEncoding);
char *ELFFileVersionName(uint8_t FileVersion);
char *ELFOSABIIdentName(uint8_t OSABIIdent);
