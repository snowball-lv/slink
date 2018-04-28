#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <slink/SymTab.h>
#include <slink/Error.h>
#include <slink/Log.h>


static Symbol *FindSym(SymTab *symtab, char *name) {
    for (size_t i = 0; i < symtab->sym_cnt; i++) {
        Symbol *sym = symtab->syms[i];
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
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
        if (sym->def == 0) {
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
            case SHN_XINDEX:
            case SHN_COMMON:
                ERROR(
                    "Unsupported sym section [%s]\n",
                    ELFSpecialSectionName(sym->shndx));
            default:
                break;
        }
    }

    switch (sym->type) {
        case STT_NOTYPE:
        case STT_FUNC:
            break;
        default:
            ERROR(
                "Unsupported sym type [%s]\n",
                ELFSymTypeName(sym->type));
    }

    Symbol *last = FindSym(symtab, sym->name);
    if (last != 0) {

        // reconcile symbols

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

        assert(!sym->is_shndx_special);

        if (last->def == 0) {

            Log("symtab", "Satisfied [%s]\n", sym->name);
            last->def = sym;
        
        } else {
            
            if (last->def == sym) {
                // already defined by this very same symbol
            } else {
                ERROR("Multiple definitions of [%s]\n", sym->name);
            }
        }

    } else {

        // create new entry

        if (sym->shndx != SHN_UNDEF) {
            // not undefined, ignore
            return;
        }

        Log("symtab", "Requesting [%s]\n", sym->name);
        
        symtab->sym_cnt++;
        symtab->syms = realloc(
            symtab->syms, 
            symtab->sym_cnt * (sizeof(Symbol *)));

        symtab->syms[symtab->sym_cnt - 1] = sym;
    }
}