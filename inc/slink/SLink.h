#pragma once

#include <stdint.h>
#include <slink/elf/ELF.h>

typedef struct Section Section;

typedef struct Symbol Symbol;
struct Symbol {

    char *name;
    uint8_t binding;
    uint8_t type;
    uint16_t shndx;
    uint8_t is_shndx_special;

    uint64_t value;

    Symbol *def;
    Section *sec;
};

struct Section {

    char *src;

    char *name;
    uint32_t type;

    uint64_t flags;
    uint64_t falloc : 1;
    uint64_t fwrite : 1;
    uint64_t fexecinstr : 1;

    uint64_t addr;
    uint64_t addralign;
    uint64_t size;

    Symbol **symtab;

};

Section **ExtractSections(Elf *elf);
