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
        lfile->archive = 0;

        if (IsElf(path)) {

            Elf *elf = calloc(1, sizeof(Elf));
            ELFRead(path, elf);
            lfile->elf = elf;

        } else if (IsArchive(path)) {

            Archive *archive = calloc(1, sizeof(Archive));
            ARReadArchive(path, archive);
            lfile->archive = archive;

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

        if (lfile->elf) {
            CollectELFUndefs(ctx, lfile->elf);
        } else {
            Archive *archive = lfile->archive;
            for (size_t k = 0; k < archive->loaded_cnt; k++) {
                Elf *mod = archive->loaded[k];
                CollectELFUndefs(ctx, mod);
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

static int DefineUndef(Context *ctx, char *name, Elf *elf, Elf64_Sym *sym) {

    unsigned char binding = ELF64_ST_BIND(sym->st_info);
    assert((binding == STB_GLOBAL) || (binding == STB_WEAK));

    Global *undef = FindUndef(ctx, name);
    if (undef == 0) {
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

static int ResolveELFUndefs(Context *ctx, Elf *elf) {

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
            updated |= DefineUndef(ctx, name, elf, sym);
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
            updated |= ResolveELFUndefs(ctx, elf);

        } else {

            Archive *archive = lfile->archive;
 
            // load modules that define undef symbols
            for (size_t k = 0; k < ctx->undefs_cnt; k++) {

                Global *undef = ctx->undefs[k];

                if (!undef->defined && (undef->binding == STB_GLOBAL)) {
                    if (ARDefinesSymbol(archive, undef->name)) {
                        ARLoadModuleWithSymbol(archive, undef->name);
                    }
                }
            }

            // resolve undefs with loaded modules
            for (size_t k = 0; k < archive->loaded_cnt; k++) {
                Elf *mod = archive->loaded[k];
                updated |= ResolveELFUndefs(ctx, mod);
            }
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
