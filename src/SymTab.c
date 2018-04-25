#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <slink/SymTab.h>
#include <slink/Error.h>


static SymRef *FindSym(SymTab *symtab, char *name) {
    for (size_t i = 0; i < symtab->sym_cnt; i++) {
        SymRef *ref = &symtab->syms[i];
        if (strcmp(ref->name, name) == 0) {
            return ref;
        }
    }
    return 0;
}

Elf64_Sym *SymTabGetDef(SymTab *symtab, char *name) {
    for (size_t i = 0; i < symtab->sym_cnt; i++) {
        SymRef *ref = &symtab->syms[i];
        if (strcmp(ref->name, name) == 0) {
            if (ref->def != 0) {
                return ref->def;
            } else {
                ERROR("[%s] is undefined\n", name);
            }
        }
    }
    ERROR("[%s] not in symbol table\n", name);
}

size_t SymTabSize(SymTab *symtab) {
    return symtab->sym_cnt;
}


void SymTabAssert(SymTab *symtab) {
    FILE *symtablog = fopen("symtab.log.txt", "a");
    fprintf(symtablog, "\nChecking for undefs\n");
    size_t undefs = 0;
    for (size_t i = 0; i < symtab->sym_cnt; i++) {
        SymRef *ref = &symtab->syms[i];
        if (ref->def == 0) {
            undefs++;
            fprintf(symtablog, "[%s] undefined\n", ref->name);
        }
    }
    if (undefs > 0) {
        ERROR("Some symbols still undefined\n");
    }
    fclose(symtablog);
}


void SymTabAdd(SymTab *symtab, Elf *elf, Elf64_Sym *sym) {

    unsigned char binding = ELF64_ST_BIND(sym->st_info);
    switch (binding) {
        case STB_LOCAL:
            return;
        case STB_GLOBAL:
            break;
        default:
            ERROR(
                "Unsupported sym binding [%s]\n",
                ELFSymBindingName(binding));
    }

    switch (sym->st_shndx) {
        case SHN_XINDEX:
        case SHN_COMMON:
            ERROR(
                "Unsupported sym section [%s]\n",
                ELFSpecialSectionName(sym->st_shndx));
        default:
            break;
    }

    unsigned char type = ELF64_ST_TYPE(sym->st_info);
    switch (type) {
        case STT_NOTYPE:
        case STT_FUNC:
            break;
        default:
            ERROR(
                "Unsupported sym type [%s]\n",
                ELFSymTypeName(type));
    }

    char *name = &elf->sym_str_tab[sym->st_name];

    SymRef *ref = FindSym(symtab, name);
    if (ref != 0) {

        // reconcile symbols

        if (ref->elf == elf && ref->sym == sym) {
            return;
        }

        // check binding types
        if (ref->binding != binding) {
            ERROR(
                "Incompatible bindings [%s] [%s]\n",
                ELFSymBindingName(ref->binding), ELFSymBindingName(binding));
        }

        // check sym types
        if (ref->type != type) {
            if (ref->type == STT_NOTYPE || type == STT_NOTYPE) {
                // compatible
            } else {
                ERROR(
                    "[%s] != [%s]\n",
                    ELFSymTypeName(ref->type), ELFSymTypeName(type));
            }
        }

        // a compatible undefine, no more work to do
        if (sym->st_shndx == SHN_UNDEF) {
            return;
        }

        assert(!ELFIsSectionSpecial(sym->st_shndx));

        if (ref->def_by == 0) {

            FILE *symtablog = fopen("symtab.log.txt", "a");
            fprintf(symtablog, "Satisfied [%s] with [%s]\n", name, elf->path);
            fclose(symtablog);

            ref->def_by = elf;
            ref->def = sym;
        
        } else {
            
            if (ref->def_by == elf && ref->def == sym) {
                // already defined by this very same symbol
            } else {
                fprintf(stderr, "[%s] defined in [%s]\n", name, elf->path);
                fprintf(stderr, "Last was in [%s]\n", ref->def_by->path);
                ERROR("Multiple definitions\n");
            }
        }

    } else {

        // create new entry

        switch (sym->st_shndx) {
            case SHN_UNDEF:
                break;
            default:
                // not undefined, ignore
                return;
        }

        FILE *symtablog = fopen("symtab.log.txt", "a");
        fprintf(symtablog, "Requesting [%s]\n", name);
        fclose(symtablog);
        
        symtab->sym_cnt++;
        symtab->syms = realloc(
            symtab->syms, 
            symtab->sym_cnt * (sizeof(SymRef)));

        SymRef *ref = &symtab->syms[symtab->sym_cnt - 1];

        ref->elf = elf;
        ref->sym = sym;
        ref->name = name;
        ref->binding = binding;
        ref->type = type;

        ref->def_by = 0;
        ref->def = 0;
    }
}