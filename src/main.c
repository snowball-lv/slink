#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include <slink/Context.h>
#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>
#include <slink/Log.h>
#include <slink/SLink.h>
#include <slink/Error.h>
#include <slink/Common.h>


// 1. Link undefined symbols to their definitions
// 2. Order sections of all input objects
// 3. Lay out symbols
// 4. Apply relocations
// 5. Output executable elf


static int sec_order_compar(const void *p1, const void *p2);

int main(int argc, char **argv) {

    LogClear();
    Log("general", "--- S LINK ---\n");

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        Log("arg", "[%s]\n", arg);
    }

    // every section of every input object
    Section **secs = calloc(1, sizeof(Section *));

    // extract sections from input objects
    for (int i = 1; i < argc; i++) {

        char *file = argv[i];
        Log("input", "File [%s]\n", file);

        Elf *elf = calloc(1, sizeof(Elf));
        ELFRead(file, elf);

        Section **esecs = ExtractSections(elf);
        size_t esec_count = ZTArraySize((void **) esecs);

        Log("input", "[%s] secs %lu (%lu)\n", file, esec_count, elf->shnum);

        size_t sec_count = ZTArraySize((void **) secs);
        size_t total = sec_count + esec_count;

        secs = realloc(secs, (total + 1) * sizeof(Section *));

        for (size_t k = sec_count; k < total; k++) {
            secs[k] = esecs[k - sec_count];
        }
        secs[total - 1] = 0;
    }

    Log("general", "%lu files loaded\n", argc - 1);
    Log("general", "%lu sections loaded\n", ZTArraySize((void **) secs));

    // log names for all laoded sections
    for (size_t i = 0; i < ZTArraySize((void **) secs); i++) {
        Section *sec = secs[i];
        Log("sec", "[%s] [%s]\n", sec->name, sec->src);
    }

    // make sure we recognize every section
    for (size_t i = 0; i < ZTArraySize((void **) secs); i++) {
        Section *sec = secs[i];
        if (strcmp(sec->name, "") == 0) { continue; }
        if (strcmp(sec->name, ".text") == 0) { continue; }
        if (strcmp(sec->name, ".data") == 0) { continue; }
        if (strcmp(sec->name, ".rodata") == 0) { continue; }
        if (strcmp(sec->name, ".bss") == 0) { continue; }
        if (strcmp(sec->name, ".shstrtab") == 0) { continue; }
        if (strcmp(sec->name, ".symtab") == 0) { continue; }
        if (strcmp(sec->name, ".strtab") == 0) { continue; }
        if (strcmp(sec->name, ".comment") == 0) { continue; }
        if (strcmp(sec->name, ".eh_frame") == 0) { continue; }
        if (strcmp(sec->name, ".rela.text") == 0) { continue; }
        if (strcmp(sec->name, ".rela.eh_frame") == 0) { continue; }
        if (strcmp(sec->name, ".note.GNU-stack") == 0) { continue; }
        ERROR("Don't know how to handle section [%s]\n", sec->name);
    }

    // link symbols from all symbol table sections
    SymTab symtab = { 0 };
    while (1) {

        Log("symtab", "\n");
        Log("symtab", "New Pass\n");

        size_t last_size = SymTabSize(&symtab);

        size_t sec_count = ZTArraySize((void **) secs);
        for (size_t i = 0; i < sec_count; i++) {
            Section *sec = secs[i];
            if (sec->type == SHT_SYMTAB) {

                size_t sym_cnt = ZTArraySize((void **) sec->symtab);
                for (size_t k = 0; k < sym_cnt; k++) {
                    Symbol *sym = sec->symtab[k];
                    SymTabAdd(&symtab, sym);
                }
            }
        }

        if (last_size == SymTabSize(&symtab)) {
            break;
        }
    }
    Log("symtab", "Done linknig symbols\n");
    SymTabAssert(&symtab);

    // order sections for output
    // subtract 1 from size to account for 0 terminator
    qsort(secs, ZTAS(secs) - 1, sizeof(Section *), sec_order_compar);

    // log section order
    for (size_t i = 0; i < ZTArraySize((void **) secs); i++) {
        Section *sec = secs[i];
        Log("sec-order", "[%s] [%s]\n", sec->name, sec->src);
    }

    return 0;
}

static int sec_order_compar(const void *p1, const void *p2) {

    Section *seca = *(Section **) p1;
    Section *secb = *(Section **) p2;

    #define FLAG_TEST(a, b) {       \
        if ((a) && !(b)) {          \
            return -1;              \
        } else if ((b) && !(a)) {   \
            return 1;               \
        }                           \
    }

    FLAG_TEST(seca->falloc, secb->falloc);
    FLAG_TEST(seca->fexecinstr, secb->fexecinstr);
    
    FLAG_TEST(secb->fwrite, seca->fwrite);

    FLAG_TEST(seca->type & SHT_PROGBITS, secb->type & SHT_PROGBITS);
    FLAG_TEST(seca->type & SHT_NOBITS, secb->type & SHT_NOBITS);

    #undef FLAG_TEST

    return strcmp(seca->name, secb->name);
}

int main_old(int argc, char **argv) {

    LogClear();
    Log("general", "--- S LINK ---\n");

    // define linking context
    Context ctx = { 0 };

    // save list of input files
    ctx.ifiles_cnt = (size_t) argc - 1;
    ctx.ifiles = &argv[1];

    // print input files
    for (size_t i = 0; i < ctx.ifiles_cnt; i++) {
        Log("input", "%s\n", ctx.ifiles[i]);
    }
    
    CTXLoadInputFiles(&ctx);

    CTXLinkSymbols(&ctx);

    CTXCollectSections(&ctx);
    CTXPrintSections(&ctx);

    Log("general", "%lu modules loaded\n", CTXCountModules(&ctx));

    CTXPrintSymbols(&ctx);

    Log("general", "Laying out symbols\n");

    CTXLayOutSymbols(&ctx);
    CTXPrintSymbols(&ctx);

    CTXProcessRelocations(&ctx);

    CTXGroupIntoSegments(&ctx);
    CTXCreateExecutable(&ctx, "hello_world");

    return 0;
}