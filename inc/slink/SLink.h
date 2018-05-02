#pragma once

#include <stdint.h>
#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>

typedef struct {
    uint64_t    offset;
    uint32_t    type;
    uint32_t    sym;
    int64_t	    addend;
} RelocationA;

typedef struct Section Section;
typedef struct Symbol Symbol;

struct Symbol {

    char *name;
    uint8_t binding;
    uint8_t type;
    uint16_t shndx;
    uint8_t is_shndx_special;

    uint64_t value;
    uint64_t size;

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

    uint8_t *data;

    Symbol **symtab;
    RelocationA **relas;
    Section *target;

    // kludge to help intepret the archive sym table as a section
    int is_archive_symtab;
    Archive *archive;
};

typedef struct {
    Section **secs;
} Segment;

Section **ExtractSections(Elf *elf);
