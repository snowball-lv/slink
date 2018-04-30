#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <slink/SymTab.h>
#include <slink/Error.h>
#include <slink/Log.h>


static int SymIndex(SymTab *symtab, char *name) {
    assert(symtab->sym_cnt <= INT_MAX);
    for (int i = 0; i < (int) symtab->sym_cnt; i++) {
        Symbol *sym = symtab->syms[i];
        if (strcmp(sym->name, name) == 0) {
            return i;
        }
    }
    return -1;
}

Symbol *SymTabGetDef(SymTab *symtab, char *name) {
    int index = SymIndex(symtab, name);
    if (index != -1) {
        return symtab->defs[index];
    }
    return 0;
}

size_t SymTabSize(SymTab *symtab) {
    return symtab->sym_cnt;
}

void SymTabAssert(SymTab *symtab) {
    size_t undefs = 0;
    for (size_t i = 0; i < symtab->sym_cnt; i++) {
        Symbol *sym = symtab->syms[i];
        Symbol *def = symtab->defs[i];
        if (def == 0) {
            Log("undefined", "[%s]\n", sym->name);
            undefs++;
        }
    }
    if (undefs > 0) {
        ERROR("Not all symbols defined\n");
    } else {
        Log("symtab", "All undefs satisfied\n");
    }
}

Symbol *SymTabGetDefIdx(SymTab *symtab, size_t index) {
    return symtab->defs[index];
}

void SymTabAdd(SymTab *symtab, Symbol *sym) {

    switch (sym->binding) {
        case STB_LOCAL:
            return;
        case STB_GLOBAL:
            break;
        default:
            ERROR(
                "Unsupported sym binding [%s]\n",
                ELFSymBindingName(sym->binding));
    }

    if (sym->is_shndx_special) {
        switch (sym->shndx) {
            case SHN_UNDEF:
            case SHN_COMMON:
                break;
            default:
                ERROR(
                    "Unsupported sym section [%s]\n",
                    ELFSpecialSectionName(sym->shndx));
        }
    }

    switch (sym->type) {
        case STT_NOTYPE:
        case STT_FUNC:
        case STT_OBJECT:
            break;
        default:
            ERROR(
                "Unsupported sym type [%s]\n",
                ELFSymTypeName(sym->type));
    }

    int index = SymIndex(symtab, sym->name);
    if (index != -1) {

        // reconcile symbols
        Symbol *last = symtab->syms[index];

        if (last == sym) {
            return;
        }

        // check binding types
        if (last->binding != sym->binding) {
            ERROR(
                "Incompatible bindings [%s] [%s]\n",
                ELFSymBindingName(last->binding), 
                ELFSymBindingName(sym->binding));
        }        

        // check sym types
        if (last->type != sym->type) {
            if (last->type == STT_NOTYPE || sym->type == STT_NOTYPE) {
                // compatible
            } else {
                ERROR(
                    "[%s] != [%s]\n",
                    ELFSymTypeName(last->type),
                    ELFSymTypeName(sym->type));
            }
        }

        // a compatible undefine, no more work to do
        if (sym->shndx == SHN_UNDEF) {
            return;
        }

        if (symtab->defs[index] == 0) {

            Log("symtab", "Satisfied [%s]\n", sym->name);
            symtab->defs[index] = sym;
        
        } else {
            
            if (symtab->defs[index] == sym) {
                // already defined by this very same symbol
            } else {
                Symbol *old = symtab->defs[index];
                if (old->shndx == SHN_COMMON && sym->shndx == SHN_COMMON) {
                    if (sym->size > old->size) {
                        symtab->defs[index] = sym;
                        Log(
                            "symtab",
                            "common [%s] %lu replaced with %lu\n",
                            old->name, old->size, sym->size);
                    } else {
                        Log(
                            "symtab",
                            "common [%s] %lu ignores %lu\n",
                            old->name, old->size, sym->size);
                    }
                } else {
                    ERROR("Multiple definitions of [%s]\n", sym->name);
                }
            }
        }

    } else {

        // create new entry

        assert(sym->binding == STB_GLOBAL);
        
        symtab->sym_cnt++;
        
        symtab->syms = realloc(
            symtab->syms, 
            symtab->sym_cnt * (sizeof(Symbol *)));
        
        symtab->defs = realloc(
            symtab->defs, 
            symtab->sym_cnt * (sizeof(Symbol *)));

        symtab->syms[symtab->sym_cnt - 1] = sym;

        if (sym->shndx != SHN_UNDEF) {
            Log("symtab", "Defined [%s]\n", sym->name);
            symtab->defs[symtab->sym_cnt - 1] = sym;
        } else {
            Log("symtab", "Requesting [%s]\n", sym->name);
            symtab->defs[symtab->sym_cnt - 1] = 0;
        }
    }
}
