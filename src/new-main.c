#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>


// 1. Link undefined symbols to their definitions
// 2. Order sections of all input objects
// 3. Lay out symbols
// 4. Apply relocations
// 5. Output executable elf


typedef struct {
    char *path;
    Elf *elf;
    Archive *archive;
} InputFile;

typedef struct {
    char *name;
    int defined;
    unsigned char binding;
} Global;

static Global *undefs = 0;
static size_t undef_cnt = 0;
static int undef_updated = 0;

static void AddUndef(char *name, unsigned char binding) {

    for (size_t i = 0; i < undef_cnt; i++) {
        Global *undef = &undefs[i];
        if (strcmp(undef->name, name) == 0) {
            // printf("Already added [%s]\n", name);
            return;
        }
    }

    // printf("Added undef [%s]\n", name);

    undef_cnt++;
    undefs = realloc(undefs, undef_cnt * sizeof(Global));

    undefs[undef_cnt - 1].name = name;
    undefs[undef_cnt - 1].defined = 0;
    undefs[undef_cnt - 1].binding = binding;

    undef_updated = 1;

    // printf("Requested [%s]\n", name);
    printf("    [%s]\n", name);
}

static void DefineUndef(char *name) {

    for (size_t i = 0; i < undef_cnt; i++) {
        Global *undef = &undefs[i];
        if (strcmp(undef->name, name) == 0) {
            if (!undef->defined) {

                // printf("Defined [%s]\n", name);
                printf("    [%s]\n", name);
                undef->defined = 1;

                undef_updated = 1;

            } else {
                
                // fprintf(
                //     stderr,
                //     "Multiple definitions of [%s]\n",
                //     name);
                // exit(1);
            }
        }
    }
}

static void PrintUndefs() {

    for (size_t i = 0; i < undef_cnt; i++) {
        Global *undef = &undefs[i];
        if (!undef->defined) {
            printf(
                "Still undefined [%s] [%s]\n",
                undef->name,
                ELFSymBindingName(undef->binding));
        }
    }
}

static void CollectELFUndefs(Elf *elf) {
    printf("Requests by elf [%s]\n", elf->path);
    // skip null symbol
    for (size_t i = 1; i < elf->sym_cnt; i++) {

        Elf64_Sym *sym = &elf->sym_tab[i];

        // TODO: handle SHN_XINDEX
        assert(sym->st_shndx != SHN_XINDEX);

        if (sym->st_shndx == SHN_UNDEF) {
            char *name = &elf->sym_str_tab[sym->st_name];
            AddUndef(name, ELF64_ST_BIND(sym->st_info));
        }
    }
}

static void CollectFileUndefs(InputFile *ifile) {
    printf("Requests by [%s]\n", ifile->path);
    if (ifile->elf) {

        Elf *elf = ifile->elf;

        // skip null symbol
        for (size_t i = 1; i < elf->sym_cnt; i++) {

            Elf64_Sym *sym = &elf->sym_tab[i];

            // TODO: handle SHN_XINDEX
            assert(sym->st_shndx != SHN_XINDEX);

            if (sym->st_shndx == SHN_UNDEF) {
                char *name = &elf->sym_str_tab[sym->st_name];
                AddUndef(name, ELF64_ST_BIND(sym->st_info));
            }
        }

    } else {

        Archive *archive = ifile->archive;

        for (size_t i = 0; i < archive->loaded_cnt; i++) {
            Elf *mod = &archive->loaded[i];
            CollectELFUndefs(mod);
        }

    }
}

static void ResolveFileUndefs(InputFile *ifile) {
    printf("Definitions by [%s]\n", ifile->path);
    if (ifile->elf) {

        Elf *elf = ifile->elf;

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
                DefineUndef(name);
            }
        }

    } else {

        Archive *archive = ifile->archive;

        for (size_t i = 0; i < undef_cnt; i++) {
            Global *undef = &undefs[i];
            if (!undef->defined && ARDefinesSymbol(archive, undef->name)) {

                // printf("AR Defines [%s]\n", undef->name);
                ARLoadModuleWithSymbol(archive, undef->name);
                undef->defined = 1;

                undef_updated = 1;
            }
        }
    }
}

int main(int argc, char **argv) {
    printf("--- S LINK ---\n");

    char **inputs = &argv[1];
    size_t input_cnt = (size_t) argc - 1;

    InputFile *ifiles = 0;
    size_t  ifile_cnt = 0;

    for (size_t  i = 0; i < input_cnt; i++) {

        char *path = inputs[i];

        if (IsElf(path)) {

            Elf *elf = malloc(sizeof(Elf));
            ELFRead(path, elf);
        
            ifile_cnt++;
            ifiles = realloc(ifiles, ifile_cnt * sizeof(InputFile));

            ifiles[ifile_cnt - 1].path = path;
            ifiles[ifile_cnt - 1].elf = elf;
            ifiles[ifile_cnt - 1].archive = 0;

        } else if (IsArchive(path)) {

            Archive *archive = malloc(sizeof(Archive));
            ARReadArchive(path, archive);
        
            ifile_cnt++;
            ifiles = realloc(ifiles, ifile_cnt * sizeof(InputFile));

            ifiles[ifile_cnt - 1].path = path;
            ifiles[ifile_cnt - 1].elf = 0;
            ifiles[ifile_cnt - 1].archive = archive;
        }
    }

    // print the input files
    for (size_t i = 0; i < ifile_cnt; i++) {

        InputFile *ifile = &ifiles[i];

        printf("File [");
        if (ifile->elf) {
            printf("ELF");
        } else if (ifile->archive) {
            printf("Archive");
        } else {
            printf("unknown");
        }
        printf("] [%s]\n", ifile->path);
    }


    int iter_cnt = 1;
    undef_updated = 1;

    while (undef_updated) {

        undef_updated = 0;

        printf("Iter %i\n", iter_cnt);

        // collect and resolve undefs
        for (size_t i = 0; i < ifile_cnt; i++) {

            InputFile *ifile = &ifiles[i];
            CollectFileUndefs(ifile);
            ResolveFileUndefs(ifile);
        }

        if (!undef_updated) {
            printf("Undef resolution complete\n");
        }

        iter_cnt++;
    }

    PrintUndefs();

    return 0;
}
