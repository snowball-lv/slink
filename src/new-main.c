#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include <slink/Context.h>
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
} Global_Old;

static Global_Old *undefs = 0;
static size_t undef_cnt = 0;
static int undef_updated = 0;

static void AddUndef(char *name, unsigned char binding) {

    for (size_t i = 0; i < undef_cnt; i++) {
        Global_Old *undef = &undefs[i];
        if (strcmp(undef->name, name) == 0) {
            // printf("Already added [%s]\n", name);
            return;
        }
    }

    // printf("Added undef [%s]\n", name);

    undef_cnt++;
    undefs = realloc(undefs, undef_cnt * sizeof(Global_Old));

    undefs[undef_cnt - 1].name = name;
    undefs[undef_cnt - 1].defined = 0;
    undefs[undef_cnt - 1].binding = binding;

    undef_updated = 1;

    printf("Requested [%s]\n", name);
}

static void DefineUndef(char *name) {

    for (size_t i = 0; i < undef_cnt; i++) {
        Global_Old *undef = &undefs[i];
        if (strcmp(undef->name, name) == 0) {
            if (!undef->defined) {

                printf("Defined [%s]\n", name);
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
        Global_Old *undef = &undefs[i];
        if (!undef->defined && undef->binding != STB_WEAK) {
            printf(
                "Still undefined [%s] [%s]\n",
                undef->name,
                ELFSymBindingName(undef->binding));
        }
    }
}

static void CollectELFUndefs(Elf *elf) {
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
            Global_Old *undef = &undefs[i];
            if (!undef->defined && (undef->binding == STB_GLOBAL) && ARDefinesSymbol(archive, undef->name)) {

                // printf("AR Defines [%s]\n", undef->name);
                ARLoadModuleWithSymbol(archive, undef->name);
                undef->defined = 1;

                undef_updated = 1;
            }
        }
    }
}

static size_t CountELFs(InputFile *ifiles, size_t cnt) {
    size_t total = 0;
    for (size_t i = 0; i < cnt; i++) {
        InputFile *ifile = &ifiles[i];
        if (ifile->elf) {
            total++;
        } else {
            total += ifile->archive->loaded_cnt;
        }
    }
    return total;
}

static Elf **ToElfList(InputFile *ifiles, size_t cnt) {

    size_t total = CountELFs(ifiles, cnt);
    Elf **elfs = malloc(total * sizeof(Elf *));

    size_t counter = 0;

    for (size_t i = 0; i < cnt; i++) {

        InputFile *ifile = &ifiles[i];

        if (ifile->elf) {
            elfs[counter++] = ifile->elf;
        } else {
            Archive *archive = ifile->archive;
            for (size_t k = 0; k < archive->loaded_cnt; k++) {
               elfs[counter++] = &archive->loaded[k];
            }
        }
    }

    assert(counter == total);

    return elfs;
}

typedef struct {
    Elf *elf;
    Elf64_Shdr *shdr;
} SecRef;

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

int main(int argc, char **argv) {
    printf("--- S LINK ---\n");

    // define linking context
    Context ctx = { 0 };

    // save list of input files
    ctx.ifiles_cnt = (size_t) argc - 1;
    ctx.ifiles = &argv[1];

    // print input files
    for (size_t i = 0; i < ctx.ifiles_cnt; i++) {
        printf("Input File [%s]\n", ctx.ifiles[i]);
    }

    CTXLoadInputFiles(&ctx);
    CTXCollectUndefs(&ctx);
    CTXResolveUndefs(&ctx);

    return 0;
}

int main_old2(int argc, char **argv) {
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

    size_t elf_cnt = CountELFs(ifiles, ifile_cnt);
    printf("ELF count %lu\n", elf_cnt);

    Elf **elfs = ToElfList(ifiles, ifile_cnt);

    for (size_t i = 0; i < elf_cnt; i++) {
        printf("--- [%s]\n", elfs[i]->path);
    }
    
    size_t sec_cnt = 0;    
    for (size_t i = 0; i < elf_cnt; i++) {
        sec_cnt += elfs[i]->shnum;
    }

    printf("Sec count %lu\n", sec_cnt);

    SecRef *sec_list = malloc(sec_cnt * sizeof(SecRef));

    FILE *log_secs = fopen("log-sec.txt", "wb");
    FILE *log_secs_ordered = fopen("log-sec-ordered.txt", "wb");

    size_t sec_counter = 0;
    for (size_t i = 0; i < elf_cnt; i++) {

        Elf *elf = elfs[i];

        // printf("File: [%s]\n", elf->path);

        for (size_t k = 0; k < elf->shnum; k++) {

            Elf64_Shdr *shdr = &elf->shdrs[k];
            ELFPrintSHdr(log_secs, elf, shdr);

            sec_list[sec_counter].elf = elf;
            sec_list[sec_counter].shdr = shdr;
            sec_counter++;
        }

        // ELFPrintSymTab(log_sym, elf);
    }

    OrderSecList(sec_list, sec_cnt);

    for (size_t i = 0; i < sec_cnt; i++) {
        SecRef *ref = &sec_list[i];
        ELFPrintSHdr(log_secs_ordered, ref->elf, ref->shdr);
    }

    fclose(log_secs);
    fclose(log_secs_ordered);

    return 0;
}
