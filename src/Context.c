#include <slink/Context.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


void CTXLoadInputFiles(Context *ctx) {
    for (size_t i = 0; i < ctx->ifiles_cnt; i++) {

        char *path = ctx->ifiles[i];
        printf("Loading [%s]\n", path);

        LoadedFile *lfile = calloc(1, sizeof(LoadedFile));

        lfile->path = path;
        lfile->elf = 0;

        if (IsElf(path)) {

            Elf *elf = calloc(1, sizeof(Elf));
            ELFRead(path, elf);
            lfile->elf = elf;

        } else {
            fprintf(stderr, "Unknown file type\n");
            fprintf(stderr, "[%s]\n", path);
            exit(1);
        }

        ctx->lfiles_cnt++;
        ctx->lfiles = realloc(ctx->lfiles, ctx->lfiles_cnt * sizeof(LoadedFile *));
        ctx->lfiles[ctx->lfiles_cnt - 1] = lfile;
    }
}

static void AddUndef(Context *ctx, char *name, unsigned char binding) {

    assert((binding == STB_GLOBAL) || (binding == STB_WEAK));

    for (size_t i = 0; i < ctx->undefs_cnt; i++) {
        Global *undef = ctx->undefs[i];
        if (strcmp(undef->name, name) == 0) {

            // already added to undefs
            // use the strictest binding
            // (global overrides weak)

            if (binding == STB_GLOBAL) {
                if (undef->binding == STB_WEAK) {
                    printf("Overriding WEAK ref to [%s] with GLOBAL\n", name);
                    undef->binding = STB_GLOBAL;
                }
            }

            return;
        }
    }

    Global *undef = calloc(1, sizeof(Global));

    undef->name = name;
    undef->defined = 0;
    undef->binding = binding;
    undef->def_by = 0;
    undef->def = 0;

    ctx->undefs_cnt++;
    ctx->undefs = realloc(ctx->undefs, ctx->undefs_cnt * sizeof(Global *));
    ctx->undefs[ctx->undefs_cnt - 1] = undef;

    printf("Requested [%s]\n", name);
}

static void CollectELFUndefs(Context *ctx, Elf *elf) {
    // skip null symbol
    for (size_t i = 1; i < elf->sym_cnt; i++) {
                    
        Elf64_Sym *sym = &elf->sym_tab[i];

        // TODO: handle SHN_XINDEX
        assert(sym->st_shndx != SHN_XINDEX);

        if (sym->st_shndx == SHN_UNDEF) {
            char *name = &elf->sym_str_tab[sym->st_name];
            AddUndef(ctx, name, ELF64_ST_BIND(sym->st_info));
        }
    }
}

void CTXCollectUndefs(Context *ctx) {
    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {
        LoadedFile *lfile = ctx->lfiles[i];
        CollectELFUndefs(ctx, lfile->elf);
    }
}

static Global *FindUndef(Context *ctx, char *name) {
    for (size_t i = 0; i < ctx->undefs_cnt; i++) {
        Global *undef = ctx->undefs[i];
        if (strcmp(undef->name, name) == 0) {
            return undef;
        }
    }
    return 0;
}

static int DefineUndef(Context *ctx, char *name, Elf *elf, Elf64_Sym *sym, int in_lib) {

    unsigned char binding = ELF64_ST_BIND(sym->st_info);
    assert((binding == STB_GLOBAL) || (binding == STB_WEAK));

    Global *undef = FindUndef(ctx, name);
    if (undef == 0) {
        return 0;
    }

    // skip if undef is WEAK and sym comes from library
    if (undef->binding == STB_WEAK && in_lib) {
        return 0;
    }

    if (undef->defined) {

        // already defined by current sym
        if (undef->def == sym) {
            return 0;
        }

        unsigned char current_binding = ELF64_ST_BIND(undef->def->st_info);
        unsigned char new_binding = ELF64_ST_BIND(sym->st_info);

        if (current_binding == STB_GLOBAL) {
            if (new_binding == STB_GLOBAL) {

                fprintf(stderr, "Multiple definitions of [%s]\n", name);
                fprintf(stderr, "By [%s] and [%s]\n", elf->path, undef->def_by->path);
                exit(1);

            }
        } else if (current_binding == STB_WEAK) {
            if (new_binding == STB_GLOBAL) {

                printf(
                    "Overriding WEAK [%s] from [%s], with GLOBAL from [%s]\n",
                    name, undef->def_by->path, elf->path);
                
                undef->def_by = elf;
                undef->def = sym;
                undef->defined = 1;

                return 1;

            } else if (undef->def->st_shndx != SHN_COMMON) {
                if (sym->st_shndx == SHN_COMMON) {

                    printf(
                        "Overriding WEAK [%s] from [%s], with GLOBAL from [%s]\n",
                        name, undef->def_by->path, elf->path);
                    
                    undef->def_by = elf;
                    undef->def = sym;
                    undef->defined = 1;

                    return 1;
                }
            }
        }

        printf(
            "Ignoring [%s] from [%s], in favor of [%s]\n",
            name, elf->path, undef->def_by->path);

    } else {

        printf("Defined [%s]\n", name);

        undef->def_by = elf;
        undef->def = sym;
        undef->defined = 1;

        return 1;
    }

    return 0;
}

static int ResolveELFUndefs(Context *ctx, Elf *elf, int in_lib) {

    int updated = 0;

    // skip null symbol
    for (size_t i = 1; i < elf->sym_cnt; i++) {

        Elf64_Sym *sym = &elf->sym_tab[i];

        // TODO: handle SHN_XINDEX
        assert(sym->st_shndx != SHN_XINDEX);

        unsigned char binding = ELF64_ST_BIND(sym->st_info);

        switch (binding) {
            // ignore local symbols
            case STB_LOCAL:
                continue;
        }

        // handle only WEAK or GLOBAL symbols
        assert((binding == STB_GLOBAL) || binding == STB_WEAK);

        if (sym->st_shndx != SHN_UNDEF) {
            char *name = &elf->sym_str_tab[sym->st_name];
            updated |= DefineUndef(ctx, name, elf, sym, in_lib);
            // printf("Define [%s]\n", name);
        }
    }

    return updated;
}

int CTXResolveUndefs(Context *ctx) {

    int updated = 0;

    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {

        LoadedFile *lfile = ctx->lfiles[i];

        if (lfile->elf) {

            Elf *elf = lfile->elf;
            updated |= ResolveELFUndefs(ctx, elf, 0);

        }
    }

    return updated;
}

void CTXPrintUndefs(Context *ctx) {
    for (size_t i = 0; i < ctx->undefs_cnt; i++) {
        Global *undef = ctx->undefs[i];
        if (!undef->defined) {

            if (undef->binding == STB_WEAK) {
                continue;
            }

            printf(
                "Still undefined [%s] [%s]\n",
                undef->name,
                ELFSymBindingName(undef->binding));
        }
    }
}

static size_t CountSections(Context *ctx) {

    size_t count = 0;

    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {
        LoadedFile *lfile = ctx->lfiles[i];
        if (lfile->elf) {
            Elf *elf = lfile->elf;
            count += elf->shnum;
        }
    }

    return count;
}

static int sec_order_compar(const void *p1, const void *p2) {

    const Elf *elfa = ((SecRef *) p1)->elf;
    const Elf *elfb = ((SecRef *) p2)->elf;

    const Elf64_Shdr *a = ((SecRef *) p1)->shdr;
    const Elf64_Shdr *b = ((SecRef *) p2)->shdr;

    #define FLAG_TEST(flag, a, b) {                 \
        if ((a & flag) && !(b & flag)) {            \
            return -1;                              \
        } else if ((b & flag) && !(a & flag)) {     \
            return 1;                               \
        }                                           \
    }

    FLAG_TEST(SHF_ALLOC, a->sh_flags, b->sh_flags);
    FLAG_TEST(SHF_WRITE, b->sh_flags, a->sh_flags);
    FLAG_TEST(SHF_EXECINSTR, a->sh_flags, b->sh_flags);

    #undef FLAG_TEST

    if (a->sh_type != b->sh_type) {
        if (a->sh_type == SHT_PROGBITS) {
            return -1;
        } else if (b->sh_type == SHT_PROGBITS) {
            return 1;
        } else if (a->sh_type == SHT_NOBITS) {
            return -1;
        } else if (b->sh_type == SHT_NOBITS) {
            return 1;
        }
    }

    char *na = &elfa->sec_name_str_tab[a->sh_name];
    char *nb = &elfb->sec_name_str_tab[b->sh_name];

    if (strcmp(na, nb) != 0) {
        return strcmp(na, nb);
    }

    return elfa->index - elfb->index;
}

static void OrderSecList(SecRef *list, size_t length) {
    qsort(list, length, sizeof(SecRef), sec_order_compar);
}

static void AssignSecAddresses(SecRef *list, size_t length, size_t base) {

    size_t addr = base;

    for (size_t i = 0 ; i < length; i++) {

        SecRef *ref = &list[i];
        Elf64_Shdr *shdr = ref->shdr;

        assert((shdr->sh_addr == 0) && "Can't handle non 0 base sections.");

        if (shdr->sh_addralign > 1) {
            if ((addr % shdr->sh_addralign) != 0) {
                addr += shdr->sh_addralign;
                addr = (addr / shdr->sh_addralign) * shdr->sh_addralign;
            }
        }

        shdr->sh_addr = addr;
        addr += shdr->sh_size;
    }
}

void CTXCollectSections(Context *ctx) {

    size_t sec_count = CountSections(ctx);
    SecRef *sec_refs = calloc(sec_count, sizeof(SecRef));

    size_t counter = 0;

    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {

        LoadedFile *lfile = ctx->lfiles[i];

        if (lfile->elf) {

            Elf *elf = lfile->elf;

            for (size_t k = 0; k < elf->shnum; k++) {

                Elf64_Shdr *shdr = &elf->shdrs[k];

                sec_refs[counter].elf = elf;
                sec_refs[counter].shdr = shdr;
                counter++;
            }

        }
    }

    assert(counter == sec_count);

    OrderSecList(sec_refs, sec_count);
    AssignSecAddresses(sec_refs, sec_count, 0);

    ctx->sec_count = sec_count;
    ctx->sec_refs = sec_refs;
}

void CTXPrintSections(Context *ctx) {
    for (size_t i = 0; i < ctx->sec_count; i++) {
        SecRef *sr = &ctx->sec_refs[i];
        char *sec_name = &sr->elf->sec_name_str_tab[sr->shdr->sh_name];
        printf("0x%.8lx [%s] [%s]\n", sr->shdr->sh_addr, sec_name, sr->elf->path);
    }
}

size_t CTXCountModules(Context *ctx) {

    size_t count = 0;

    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {
        LoadedFile *lfile = ctx->lfiles[i];
        if (lfile->elf) {
            count++;
        }
    }

    return count;
}

Global **CTXGetUndefs(Context *ctx) {

    size_t count = 0;
    Global **undefs = 0;

    for (size_t i = 0; i < ctx->undefs_cnt; i++) {
        Global *undef = ctx->undefs[i];
        if (!undef->defined) {

            // skip WEAK symbols
            if (undef->binding == STB_WEAK) {
                continue;
            }

            count++;
            undefs = realloc(undefs, count * sizeof(Global *));
            undefs[count - 1] = undef;
        }
    }

    // add terminating zero
    count++;
    undefs = realloc(undefs, count * sizeof(Global *));
    undefs[count - 1] = 0;

    return undefs;
}
