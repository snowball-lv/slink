#pragma once

#include <stddef.h>
#include <slink/elf/ELF.h>


typedef struct {

    Elf *elf;
    Elf64_Sym *sym;
    char *name;
    unsigned char binding;
    unsigned char type;

    Elf *def_by;
    Elf64_Sym *def;

} SymRef;

typedef struct {
    SymRef *syms;
    size_t sym_cnt;
} SymTab;

void SymTabAdd(SymTab *symtab, Elf *elf, Elf64_Sym *sym);
size_t SymTabSize(SymTab *symtab);
Elf64_Sym *SymTabGetDef(SymTab *symtab, char *name);
void SymTabAssert(SymTab *symtab);
