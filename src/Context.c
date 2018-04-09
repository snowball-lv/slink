#include <slink/Context.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


static void TestSupport(Elf *elf) {

    assert(elf->sym_cnt > 0);

    // test symbols
    // skip null sym
    for (size_t i = 1; i < elf->sym_cnt; i++) {

        Elf64_Sym *sym = &elf->sym_tab[i];
        unsigned char binding = ELF64_ST_BIND(sym->st_info);
        char *name = &elf->sym_str_tab[sym->st_name];

        // test binding support
        switch (binding) {

            case STB_LOCAL:
            case STB_GLOBAL:
                break;

            default:
                fprintf(stderr, "For symbol [%s]\n", name);
                fprintf(stderr, "No support for binding [%s]\n", ELFSymBindingName(binding));
                exit(1);
        }

        // test special section support
        if (ELFIsSectionSpecial(sym->st_shndx)) {
            switch (sym->st_shndx) {

                case SHN_ABS:
                case SHN_UNDEF:
                    break;

                default:
                    fprintf(stderr, "For symbol [%s]\n", name);
                    fprintf(stderr, "No support for section [%s]\n", ELFSpecialSectionName(sym->st_shndx));
                    exit(1);
            }
        }
    }


    assert(elf->shnum > 0);

    // test sections
    // skip null section
    for (size_t i = 1; i < elf->shnum; i++) {

        Elf64_Shdr *shdr = &elf->shdrs[i];
        char *name = &elf->sec_name_str_tab[shdr->sh_name];

        // test section type support
        switch (shdr->sh_type) {

            case SHT_PROGBITS:
            case SHT_RELA:
            case SHT_NOBITS:
            case SHT_STRTAB:
            case SHT_SYMTAB:
                break;

            default:
                fprintf(stderr, "For section [%s]\n", name);
                fprintf(stderr, "No support for type [%s]\n", ELFSectionTypeName(shdr->sh_type));
                exit(1);
        }

        // test section attribute support
        for (size_t k = 0; k < ELF_SHFS_CNT; k++) {
            Elf64_Xword flag = ELF_SHFS[k];
            if (flag & shdr->sh_flags) {
                switch (flag) {

                    case SHF_ALLOC:
                    case SHF_EXECINSTR:
                    case SHF_INFO_LINK:
                    case SHF_WRITE:
                    case SHF_MERGE:
                    case SHF_STRINGS:
                        break;

                    default:
                        fprintf(stderr, "For section [%s]\n", name);
                        fprintf(stderr, "No support for flag [%s]\n", ELFSectionFlagName(flag));
                        exit(1);
                }
            }
        }
    }
}

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
            TestSupport(elf);
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

static void AddUndef(Context *ctx, Elf *elf, Elf64_Sym *sym) {

    assert(sym->st_shndx != SHN_XINDEX);
    assert(sym->st_shndx == SHN_UNDEF);

    unsigned char binding = ELF64_ST_BIND(sym->st_info);

    // handle only globals
    assert(binding == STB_GLOBAL);
    
    char *name = &elf->sym_str_tab[sym->st_name];

    for (size_t i = 0; i < ctx->undefs_cnt; i++) {
        Global *undef = ctx->undefs[i];
        if (strcmp(undef->name, name) == 0) {
            // already added to undefs
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

void CTXCollectUndefs(Context *ctx) {
    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {

        LoadedFile *lfile = ctx->lfiles[i];
        Elf *elf = lfile->elf;

        // skip null symbol
        for (size_t i = 1; i < elf->sym_cnt; i++) {
            Elf64_Sym *sym = &elf->sym_tab[i];
            if (sym->st_shndx == SHN_UNDEF) {
                AddUndef(ctx, elf, sym);
            }
        }
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

static void DefineUndef(Context *ctx, Elf *elf, Elf64_Sym *sym) {

    assert(sym->st_shndx != SHN_UNDEF);

    unsigned char binding = ELF64_ST_BIND(sym->st_info);

    // handle only globals
    assert(binding == STB_GLOBAL);
    
    char *name = &elf->sym_str_tab[sym->st_name];

    Global *undef = FindUndef(ctx, name);
    if (undef == 0) {
        return;
    }

    if (undef->defined) {

        // already defined by current sym
        if (undef->def == sym) {
            return;
        }

        fprintf(stderr, "Multiple definitions of [%s]\n", name);
        fprintf(stderr, "By [%s] and [%s]\n", elf->path, undef->def_by->path);
        exit(1);

    } else {

        printf("Defined [%s]\n", name);

        undef->def_by = elf;
        undef->def = sym;
        undef->defined = 1;

        ctx->needs_sym_pass = 1;
        return;
    }
}

void CTXResolveUndefs(Context *ctx) {
    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {

        LoadedFile *lfile = ctx->lfiles[i];
        Elf *elf = lfile->elf;    

        // skip null symbol
        for (size_t i = 1; i < elf->sym_cnt; i++) {
            Elf64_Sym *sym = &elf->sym_tab[i];
            unsigned char binding = ELF64_ST_BIND(sym->st_info);
            if (sym->st_shndx != SHN_UNDEF && binding == STB_GLOBAL) {
                DefineUndef(ctx, elf, sym);
            }
        }
    }
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
