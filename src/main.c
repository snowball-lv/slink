#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>
#include <slink/Log.h>
#include <slink/SLink.h>
#include <slink/Error.h>
#include <slink/Common.h>
#include <slink/Amd64.h>


// 1. Link undefined symbols to their definitions
// 2. Order sections of all input objects
// 3. Group sections into segments
// 4. Assign section addresses
// 5. Assign symbol addresses
// 6. Apply relocations
// 7. Output executable elf


#define PAGE_SIZE   4096

static int sec_order_compar(const void *p1, const void *p2);
static Segment **GroupIntoSegments(Section **secs);
static void CreateExecutable(char *file, Symbol *entry, Segment **segs);
static void AssignAddresses(Segment **segs, size_t base);

static size_t Align(size_t addr, size_t alignment) {
    assert(alignment >= 1);
    addr += alignment - 1;
    addr = (addr / alignment) * alignment;
    return addr;
}

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

        if (IsElf(file)) {

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
            secs[total] = 0;

        } else {

            assert(IsArchive(file));
            Log("input", "Archive [%s]\n", file);

            // for archives, insert a dummy section that points to the archive.
            // use this section when resolving symbols to check if the currently 
            // undefined symbols in the symtab are definined within the archive.
            // if they are, extract the objects from the archive, and add their
            // sections to the master section list before this dummy section.
            // otherwise, ignore this section.

            Archive *archive = calloc(1, sizeof(Archive));
            ARReadArchive(file, archive);

            Section *dummy = calloc(1, sizeof(Section));

            size_t name_size = strlen(file) + strlen("*SLink:");
            
            dummy->src = malloc(name_size + 1);
            sprintf(dummy->src, "*SLink:%s", file);

            dummy->name = "slink.ar.symtab";
            dummy->type = SHT_NULL;

            dummy->is_archive_symtab = 1;
            dummy->archive = archive;
            
            size_t sec_count = ZTAS(secs);
            size_t total = sec_count + 1;

            secs = realloc(secs, (total + 1) * sizeof(Section *));

            secs[total - 1] = dummy;
            secs[total] = 0;
        }
    }

    Log("general", "%lu files loaded\n", argc - 1);
    Log("general", "%lu sections loaded\n", ZTArraySize((void **) secs));

    // log names for all loaded sections
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
        if (strcmp(sec->name, "slink.ar.symtab") == 0) { continue; }
        ERROR("Don't know how to handle section [%s]\n", sec->name);
    }

    // link symbols from all symbol table sections
    SymTab symtab = { 0 };
    while (1) {

        Log("symtab", "\n");
        Log("symtab", "New Pass\n");

        size_t last_size = SymTabSize(&symtab);

        for (size_t i = 0; i < ZTAS(secs); i++) {

            Section *sec = secs[i];

            if (sec->type == SHT_SYMTAB) {

                size_t sym_cnt = ZTArraySize((void **) sec->symtab);
                for (size_t k = 0; k < sym_cnt; k++) {
                    Symbol *sym = sec->symtab[k];
                    SymTabAdd(&symtab, sym);
                }

            } else if (sec->is_archive_symtab) {

                int loaded_mod = 0;

                size_t size = SymTabSize(&symtab);
                for (size_t k = 0; k < size; k++) {
                    
                    Symbol *sym = SymTabGetSymIdx(&symtab, k);
                    Symbol *def = SymTabGetDefIdx(&symtab, k);

                    if (def == 0 && ARDefinesSymbol(sec->archive, sym->name)) {

                        Log("symtab", "[%s] found in archive [%s]\n", sym->name, sec->src);
                        
                        // load object from archive and add it to section list
                        ARLoadModuleWithSymbol(sec->archive, sym->name);
                        Elf *elf = ARElfOfSym(sec->archive, sym->name);

                        Log("symtab", "Loaded [%s] from archive [%s]\n", elf->path, sec->src);

                        // extract sections
                        Section **esecs = ExtractSections(elf);
                        size_t esec_count = ZTAS(esecs);

                        Log("symtab", "%lu sections extracted from [%s]\n", esec_count, elf->path);

                        // resize section array
                        size_t sec_count = ZTAS(secs);
                        size_t total = sec_count + esec_count;
                        secs = realloc(secs, (total + 1) * sizeof(Section *));

                        // shift remaining sections to end of list
                        for (size_t n = i; n < sec_count; n++) {
                            secs[n + esec_count] = secs[n];
                        }

                        // insert new sections right before dummy 
                        // section (current)
                        for (size_t n = 0; n < esec_count; n++) {
                            secs[i + n] = esecs[n];
                        }

                        secs[total] = 0;

                        loaded_mod = 1;
                        break;
                    }
                }

                if (loaded_mod) {
                    i--;
                    continue;
                }
            }
        }

        if (last_size == SymTabSize(&symtab)) {
            break;
        }
    }
    Log("symtab", "Done linknig symbols\n");
    SymTabAssert(&symtab);

    // create section for common symbols
    Section *common = calloc(1, sizeof(Section));
    common->src = "*SLink";
    common->name = "slink.common";
    common->type = SHT_NOBITS;
    common->flags = SHF_ALLOC | SHF_WRITE;
    common->falloc = 1;
    common->fwrite = 1;

    // lay out common symbols
    for (size_t i = 0 ; i < SymTabSize(&symtab); i++) {
        Symbol *sym = SymTabGetDefIdx(&symtab, i);
        if (sym->shndx == SHN_COMMON) {
            if (sym->value > common->addralign) {
                common->addralign = sym->value;
            }
            sym->sec = common;
        }
    }
    for (size_t i = 0 ; i < SymTabSize(&symtab); i++) {
        Symbol *sym = SymTabGetDefIdx(&symtab, i);
        if (sym->shndx == SHN_COMMON) {
            common->size = Align(common->size, sym->value);
            sym->value = common->size;
            common->size += sym->size;
        }
    }

    Log("common", "sum of common symbols %lu\n", common->size);
    {
        // append common section to list
        size_t sec_cnt = ZTAS(secs);
        sec_cnt++;
        secs = realloc(secs, (sec_cnt + 1) * sizeof(Section *));
        secs[sec_cnt - 1] = common;
        secs[sec_cnt] = 0;
    }

    // order sections for output
    qsort(secs, ZTAS(secs), sizeof(Section *), sec_order_compar);

    // log section order
    for (size_t i = 0; i < ZTArraySize((void **) secs); i++) {
        Section *sec = secs[i];
        Log("sec-order", "[%s] [%s]\n", sec->name, sec->src);
    }

    // group sections into segments
    Segment **segs = GroupIntoSegments(secs);
    Log("general", "segments %i\n", ZTAS(segs));

    // assign section addresses
    AssignAddresses(segs, 0x400000);

    // log section addresses
    for (size_t i = 0; i < ZTArraySize((void **) secs); i++) {
        Section *sec = secs[i];
        if (sec->falloc) {
            Log("sec-addr", "[%s] 0x%lx\n", sec->name, sec->addr);
        }
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
                        case SHN_COMMON:
                            if (sym->sec != 0) {
                                sym->value += sym->sec->addr;
                                break;
                            } else {
                                continue;
                            }
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

    Log("general", "symbols laid out\n");

    // process relocations
    for (size_t i = 0; i < ZTAS(secs); i++) {
        Section *sec = secs[i];
        if (sec->type == SHT_RELA) {
            Amd64ApplyRelocations(&symtab, sec);
        }
    }

    Log("general", "relocations applied\n");

    // create executable
    Symbol *entry = SymTabGetDef(&symtab, "_start");
    assert(entry != 0);
    CreateExecutable("hello_world", entry, segs);

    return 0;
}

static void AssignAddresses(Segment **segs, size_t base) {

    size_t addr = base;
    size_t seg_cnt = ZTAS(segs);

    addr += sizeof(Elf64_Ehdr);
    addr += seg_cnt * sizeof(Elf64_Phdr);

    for (size_t i = 0; i < seg_cnt; i++) {

        Segment *seg = segs[i];
        addr += PAGE_SIZE;

        size_t sec_cnt = ZTAS(seg->secs);
        for (size_t k = 0; k < sec_cnt; k++) {

            Section *sec = seg->secs[k];

            if (sec->addr != 0) {
                fprintf(stderr, "[%s] [%s]\n", sec->name, sec->src);
                ERROR("Can't handle non 0 base sections.\n");
            }

            size_t alignment = sec->addralign;
            if (alignment == 0) {
                alignment = 1;
            }

            addr = Align(addr, alignment);
            sec->addr = addr;
            addr += sec->size;
        }
    }
}

static size_t Floor4k(size_t addr) {
    return (addr / PAGE_SIZE) * PAGE_SIZE;
}

static void CreateExecutable(char *file, Symbol *entry, Segment **segs) {

    FILE *out = fopen(file, "w");
    assert(out);

    ELFIdent ident = {
        .FileIdent      = { 0x7f, 'E', 'L', 'F' },
        .FileClass      = ELFCLASS64,
        .DataEncoding   = ELFDATA2LSB,
        .FileVersion    = EV_CURRENT,
        .OSABIIdent     = 0,
        .ABIVersion     = 0
    };

    size_t seg_cnt = ZTAS(segs);

    Elf64_Ehdr ehdr = {

        .e_type         = ET_EXEC,
        .e_machine      = EM_X86_64,
        .e_version      = EV_CURRENT,

        .e_entry        = entry->value,

        .e_phoff        = sizeof(Elf64_Ehdr),
        .e_phnum        = (short unsigned) seg_cnt,
        .e_phentsize    = sizeof(Elf64_Phdr),

        .e_ehsize       = sizeof(Elf64_Ehdr),

        .e_flags        = 0,
        .e_shoff        = 0,
        .e_shnum        = 0,
        .e_shentsize    = 0,
        .e_shstrndx     = SHN_UNDEF,
    };

    memcpy(ehdr.e_ident, &ident, EI_NIDENT);
    fwrite(&ehdr, sizeof(Elf64_Ehdr), 1, out);

    Elf64_Phdr *phdrs = calloc(seg_cnt, sizeof(Elf64_Phdr));
    for (size_t i = 0; i < seg_cnt; i++) {

        Elf64_Phdr *phdr = &phdrs[i];
        phdr->p_type    = PT_LOAD;
        phdr->p_flags   = PF_R;
        phdr->p_offset  = 0;
        phdr->p_vaddr   = 0;
        phdr->p_paddr   = 0;
        phdr->p_filesz  = 0;
        phdr->p_memsz   = 0;
        phdr->p_align    = PAGE_SIZE;

        Segment *seg = segs[i];
        size_t sec_cnt = ZTAS(seg->secs);
        for (size_t k = 0; k < sec_cnt; k++) {

            Section *sec = seg->secs[k];

            if (sec->fwrite) {
                phdr->p_flags |= PF_W;
            }

            if (sec->fexecinstr) {
                phdr->p_flags |= PF_X;
            }

            if (k == 0) {
                phdr->p_vaddr = Floor4k(sec->addr);
                phdr->p_memsz += sec->addr - phdr->p_vaddr;
                phdr->p_filesz += sec->addr - phdr->p_vaddr;
            }

            size_t alignment = sec->addralign;
            if (alignment == 0) {
                alignment = 1;
            }

            phdr->p_memsz = Align(phdr->p_memsz, alignment);
            phdr->p_memsz += sec->size;

            if (sec->type == SHT_PROGBITS) {
                phdr->p_filesz = Align(phdr->p_filesz, alignment);
                phdr->p_filesz += sec->size;
            }
        }
    }

    size_t data_off = 0;
    data_off += sizeof(Elf64_Ehdr);
    data_off += seg_cnt * sizeof(Elf64_Phdr);
    
    for (size_t i = 0; i < seg_cnt; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        phdr->p_offset = Floor4k(data_off);
        data_off += phdr->p_filesz;
    }
    
    for (size_t i = 0; i < seg_cnt; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        fwrite(phdr, sizeof(Elf64_Phdr), 1, out);
    }
    
    for (size_t i = 0; i < seg_cnt; i++) {
        
        Segment *seg = segs[i];
        Elf64_Phdr *phdr = &phdrs[i];

        size_t sec_cnt = ZTAS(seg->secs);
        for (size_t k = 0; k < sec_cnt; k++) {

            Section *sec = seg->secs[k];

            size_t off = phdr->p_offset + (sec->addr - phdr->p_vaddr);

            assert(off <= LONG_MAX);
            fseek(out, (long) off, SEEK_SET);
            if (sec->type == SHT_PROGBITS) {
                fwrite(sec->data, 1, sec->size, out);
            }
        }
    }

    fclose(out);
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

            seg_cnt++;
            segs = realloc(segs, (seg_cnt + 1) * sizeof(Segment *));

            Segment *seg = calloc(1, sizeof(Segment));
            seg->secs = calloc(1, sizeof(Section *));

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
