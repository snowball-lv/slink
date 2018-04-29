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
#include <slink/Amd64.h>


// 1. Link undefined symbols to their definitions
// 2. Order sections of all input objects
// 3. Lay out symbols
// 4. Apply relocations
// 5. Output executable elf


#define PAGE_SIZE   4096

static int sec_order_compar(const void *p1, const void *p2);
static void AssignSectionAddresses(Section **secs, size_t base);
static Segment **GroupIntoSegments(Section **secs);

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

    // assign section addresses
    AssignSectionAddresses(secs, 0x400000);

    // log section order and new addresses
    for (size_t i = 0; i < ZTArraySize((void **) secs); i++) {
        Section *sec = secs[i];
        Log("sec-order", "[%s] 0x%lx [%s]\n", sec->name, sec->addr, sec->src);
    }

    // lay out symbols
    for (size_t i = 0; i < ZTAS(secs); i++) {
        Section *sec = secs[i];
        if (sec->type == SHT_SYMTAB) {
            size_t sym_cnt = ZTArraySize((void **) sec->symtab);
            for (size_t k = 0; k < sym_cnt; k++) {

                Symbol *sym = sec->symtab[k];

                if (sym->is_shndx_special) {

                    switch (sym->shndx) {
                        case SHN_ABS:
                        case SHN_UNDEF:
                            continue;
                        default:
                            ERROR(
                                "Can't handle shndx: [%s]\n",
                                ELFSpecialSectionName(sym->shndx));
                    }

                } else {

                    Section *sec = sym->sec;
                    sym->value += sec->addr;
                    
                }

                Log(
                    "sym-addr", 
                    "[%s] 0x%lx [%s]\n", 
                    sym->name, sym->value, sym->sec->name);
            }
        }
    }

    // process relocations
    for (size_t i = 0; i < ZTAS(secs); i++) {
        Section *sec = secs[i];
        if (sec->type == SHT_RELA) {
            Amd64ApplyRelocations(&symtab, sec);
        }
    }

    // group sections into segments
    Segment **segs = GroupIntoSegments(secs);
    Log("general", "segments %i\n", ZTAS(segs));
    for (size_t i = 0; i < ZTAS(segs); i++) {
        Segment *seg = segs[i];
    }

    return 0;
}

static Segment **GroupIntoSegments(Section **secs) {

    size_t seg_cnt = 0;
    Segment **segs = calloc(seg_cnt + 1, sizeof(Segment *));
    uint64_t lats_flags = 0;

    for (size_t i = 0; i < ZTAS(secs); i++) {

        Section *sec = secs[i];

        Log(
            "segs",
            sec->falloc ? "Including [%s] [%s]\n" : "Skipping [%s] [%s]\n", 
            sec->name,
            ELFSectionTypeName(sec->type));

        if (!sec->falloc) {
            continue;
        }

        if (lats_flags != sec->flags) {

            assert((sec->addr % PAGE_SIZE) == 0);

            seg_cnt++;
            segs = realloc(segs, (seg_cnt + 1) * sizeof(Segment *));

            Segment *seg = calloc(1, sizeof(Segment));
            seg->secs = calloc(1, sizeof(Section *));

            // TODO

            segs[seg_cnt - 1] = seg;
            segs[seg_cnt] = 0;

            lats_flags = sec->flags;
        }

        Segment *seg = segs[seg_cnt - 1];
        size_t sec_cnt = ZTAS(seg->secs);
        sec_cnt++;
        seg->secs = realloc(seg->secs, (sec_cnt + 1) * sizeof(Section * ));
        seg->secs[sec_cnt - 1] = sec;
        seg->secs[sec_cnt] = 0;
    }

    for (size_t i = 0; i < seg_cnt; i++) {
        Segment *seg = segs[i];
        size_t sec_cnt = ZTAS(seg->secs);
        for (size_t k = 0; k < sec_cnt; k++) {
            Section *sec = seg->secs[k];
            Log("segs", "%lu:%lu [%s] [%s]\n", i, k, sec->name, sec->src);
        }
    }

    return segs;
}

static size_t Align(size_t addr, size_t alignment) {
    assert(alignment > 1);
    addr += alignment - 1;
    addr = (addr / alignment) * alignment;
    return addr;
}

static size_t Align4k(size_t addr) {
    return Align(addr, PAGE_SIZE);
}

static void AssignSectionAddresses(Section **secs, size_t base) {

    size_t sec_cnt = ZTAS(secs);
    size_t addr = base;

    uint64_t last_flags = 0;

    for (size_t i = 0 ; i < sec_cnt; i++) {

        Section *sec = secs[i];

        // start of new segment
        if (sec->flags != last_flags) {
            addr = Align4k(addr);
            last_flags = sec->flags;
        }

        if (sec->addr != 0) {
            fprintf(stderr, "[%s] [%s]\n", sec->name, sec->src);
            ERROR("Can't handle non 0 base sections.\n");
        }

        if (sec->addralign > 1) {
            addr = Align(addr, sec->addralign);
        }

        if (sec->addralign != 0) {
            assert((addr % sec->addralign) == 0);
        }

        sec->addr = addr;
        addr += sec->size;
    }
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