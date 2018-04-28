#pragma once

#include <stddef.h>
#include <slink/elf/ELF.h>
#include <slink/SLink.h>


typedef struct {
    Symbol **syms;
    size_t sym_cnt;
} SymTab;

void SymTabAdd(SymTab *symtab, Symbol *sym);
size_t SymTabSize(SymTab *symtab);
void SymTabAssert(SymTab *symtab);
