#include <slink/Context.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


void CTXLoadInputFiles(Context *ctx) {
    for (size_t i = 0; i < ctx->ifiles_cnt; i++) {

        char *path = ctx->ifiles[i];
        printf("Loading [%s]\n", path);

        ctx->lfiles_cnt++;
        ctx->lfiles = realloc(ctx->lfiles, ctx->lfiles_cnt * sizeof(LoadedFile));
        LoadedFile *lfile = &ctx->lfiles[ctx->lfiles_cnt - 1];

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
    }
}

static void AddUndef(Context *ctx, char *name, unsigned char binding) {

    for (size_t i = 0; i < ctx->undefs_cnt; i++) {
        Global *undef = &ctx->undefs[i];
        if (strcmp(undef->name, name) == 0) {
            // already added to undefs
            return;
        }
    }

    ctx->undefs_cnt++;
    ctx->undefs = realloc(ctx->undefs, ctx->undefs_cnt * sizeof(Global));

    ctx->undefs[ctx->undefs_cnt - 1].name = name;
    ctx->undefs[ctx->undefs_cnt - 1].defined = 0;
    ctx->undefs[ctx->undefs_cnt - 1].binding = binding;

    // undef_updated = 1;

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

        LoadedFile *lfile = &ctx->lfiles[i];

        if (lfile->elf) {
            CollectELFUndefs(ctx, lfile->elf);
        } else {
            Archive *archive = lfile->archive;
            for (size_t k = 0; k < archive->loaded_cnt; k++) {
                Elf *mod = &archive->loaded[k];
                CollectELFUndefs(ctx, mod);
            }
        }
    }
}

static int DefineUndef(Context *ctx, char *name) {

    for (size_t i = 0; i < ctx->undefs_cnt; i++) {
        Global *undef = &ctx->undefs[i];
        if (!undef->defined) {
            if (strcmp(undef->name, name) == 0) {

                printf("Defined [%s]\n", name);
                undef->defined = 1;

                return 1;
            }
        }
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

        unsigned char visibility = ELF64_ST_VISIBILITY(sym->st_other);

        // skip hidden symbols
        if (visibility == STV_HIDDEN) {
            continue;
        }

        // TODO: handle non STV_DEFAULT visibilities
        assert(visibility == STV_DEFAULT);

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
            updated |= DefineUndef(ctx, name);
            // printf("Define [%s]\n", name);
        }
    }

    return updated;
}

int CTXResolveUndefs(Context *ctx) {

    int updated = 0;

    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {

        LoadedFile *lfile = &ctx->lfiles[i];

        if (lfile->elf) {

            Elf *elf = lfile->elf;
            updated |= ResolveELFUndefs(ctx, elf, 0);

        } else {

            Archive *archive = lfile->archive;
 
            // load modules that define undef symbols
            for (size_t k = 0; k < ctx->undefs_cnt; k++) {
                Global *undef = &ctx->undefs[k];
                if (!undef->defined && (undef->binding == STB_GLOBAL)) {
                    if (ARDefinesSymbol(archive, undef->name)) {
                        ARLoadModuleWithSymbol(archive, undef->name);
                    }
                }
            }

            // resolve undefs with loaded modules
            for (size_t k = 0; k < archive->loaded_cnt; k++) {
                Elf *mod = &archive->loaded[k];
                updated |= ResolveELFUndefs(ctx, mod, 1);
            }
        }
    }

    return updated;
}