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

static size_t Align(size_t addr, size_t alignment) {
    assert(alignment > 1);
    addr += alignment - 1;
    addr = (addr / alignment) * alignment;
    return addr;
}

static size_t Align4k(size_t addr) {
    return Align(addr, PAGE_SIZE);
}

static void AssignSecAddresses(SecRef *list, size_t length, size_t base) {

    size_t addr = base;

    uintmax_t last_flags = 0;

    for (size_t i = 0 ; i < length; i++) {

        SecRef *ref = &list[i];
        Elf64_Shdr *shdr = ref->shdr;

        // start of new segment
        if (shdr->sh_flags != last_flags) {
            addr = Align4k(addr);
            last_flags = shdr->sh_flags;
        }

        assert((shdr->sh_addr == 0) && "Can't handle non 0 base sections.");

        if (shdr->sh_addralign > 1) {
            addr = Align(addr, shdr->sh_addralign);
        }

        assert((addr % shdr->sh_addralign) == 0);

        shdr->sh_addr = addr;
        addr += shdr->sh_size;
    }
}

void CTXCollectSections(Context *ctx) {


    size_t sec_count = 0;

    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {
        LoadedFile *lfile = ctx->lfiles[i];
        Elf *elf = lfile->elf;
        for (size_t k = 0; k < elf->shnum; k++) {
            Elf64_Shdr *shdr = &elf->shdrs[k];
            if (!(shdr->sh_flags & SHF_ALLOC)) {
                continue;
            }
            sec_count++;
        }
    }

    SecRef *sec_refs = calloc(sec_count, sizeof(SecRef));

    size_t counter = 0;

    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {

        LoadedFile *lfile = ctx->lfiles[i];

        Elf *elf = lfile->elf;

        for (size_t k = 0; k < elf->shnum; k++) {

            Elf64_Shdr *shdr = &elf->shdrs[k];

            if (!(shdr->sh_flags & SHF_ALLOC)) {
                continue;
            }

            sec_refs[counter].elf = elf;
            sec_refs[counter].shdr = shdr;
            counter++;
        }
    }

    assert(counter == sec_count);

    OrderSecList(sec_refs, sec_count);
    AssignSecAddresses(sec_refs, sec_count, 0x400000);

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


static void ProcessSingleRelA(Context *ctx, Elf *elf, Elf64_Shdr *shdr, Elf64_Rela *rela) {

    Elf64_Shdr *sym_tab_sh = &elf->shdrs[shdr->sh_link];
    assert(sym_tab_sh->sh_type == SHT_SYMTAB);

    Elf64_Sym *sym_tab = (Elf64_Sym *) &elf->raw[sym_tab_sh->sh_offset];

    Elf64_Shdr *sym_str_tab_sh = &elf->shdrs[sym_tab_sh->sh_link];
    assert(sym_str_tab_sh->sh_type == SHT_STRTAB);

    char *sym_str_tab = &elf->raw[sym_str_tab_sh->sh_offset];

    unsigned type = ELF64_R_TYPE(rela->r_info);

    printf("Processing reloc %u [%s]\n", type, ELFRelTypeName(type));

    /*
        S - Represents the value of the symbol whose index resides in 
        the relocation entry.

        A - Represents the addend used to compute the value of the
        relocatable field.

        P - Represents the place (section offset or address) of the 
        storage unit being relocated (computed using r_offset).
    */

    size_t sym_index = ELF64_R_SYM(rela->r_info);
    Elf64_Sym *sym = &sym_tab[sym_index];
    char *name = &sym_str_tab[sym->st_name];

    if (sym->st_shndx == SHN_UNDEF) {

        Global *undef = FindUndef(ctx, name);
        assert(undef != 0);
        assert(undef->def != 0);

        sym = undef->def;
    }

    Elf64_Shdr *target_sh = &elf->shdrs[shdr->sh_info];
    char *target_data = &elf->raw[target_sh->sh_offset];

    switch (type) {

        // S + A - P
        // word32
        case R_X86_64_PC32: {
            
            assert(sym->st_value <= INT32_MAX);
            int32_t value = (int32_t) sym->st_value;

            assert(rela->r_addend >= INT32_MIN);
            assert(rela->r_addend <= INT32_MAX);
            value += (int32_t) rela->r_addend;

            assert(rela->r_offset <= INT32_MAX);
            value -= (int32_t) (rela->r_offset + shdr->sh_addr);

            int32_t *ptr = (int32_t *) &target_data[rela->r_offset];
            *ptr = value;

            break;
        }

        // S + A
        // word32
        case R_X86_64_32: {

            assert(sym->st_value <= INT32_MAX);
            int32_t value = (int32_t) sym->st_value;

            assert(rela->r_addend >= INT32_MIN);
            assert(rela->r_addend <= INT32_MAX);
            value += (int32_t) rela->r_addend;

            int32_t *ptr = (int32_t *) &target_data[rela->r_offset];
            *ptr = value;

            break;
        }

        default:
            fprintf(
                stderr, 
                "Can't handle relocs of type %u [%s]\n",
                type, ELFRelTypeName(type));
            exit(1);
    }
}

static void ProcessRelA(Context *ctx, Elf *elf, Elf64_Shdr *shdr) {

    assert(shdr->sh_type == SHT_RELA);

    char *name = &elf->sec_name_str_tab[shdr->sh_name];
    printf("Processing relocs in [%s] [%s]\n", name, elf->path);

    assert(shdr->sh_entsize == sizeof(Elf64_Rela));

    for (size_t off = 0; off < shdr->sh_size; off += shdr->sh_entsize) {
        Elf64_Rela *rela = (Elf64_Rela *) &elf->raw[shdr->sh_offset + off];
        ProcessSingleRelA(ctx, elf, shdr, rela);
    }
}

static void ProcessELFRelocations(Context *ctx, Elf *elf) {
    for (size_t i = 0; i < elf->shnum; i++) {
        Elf64_Shdr *shdr = &elf->shdrs[i];
        if (shdr->sh_type == SHT_RELA) {
            ProcessRelA(ctx, elf, shdr);
        }
    }
}

void CTXProcessRelocations(Context *ctx) {
    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {
        LoadedFile *lfile = ctx->lfiles[i];
        Elf *elf = lfile->elf;
        ProcessELFRelocations(ctx, elf);
    }
}

static void LayOutELFSymbols(Context *ctx, Elf *elf) {
    // skip null symbol
    for (size_t i = 0; i < elf->sym_cnt; i++) {
        
        Elf64_Sym *sym = &elf->sym_tab[i];

        if (ELFIsSectionSpecial(sym->st_shndx)) {

            switch (sym->st_shndx) {
                case SHN_ABS: break;
                case SHN_UNDEF: break;
                default:
                    fprintf(
                        stderr,
                        "Can't handle shndx: [%s]\n",
                        ELFSpecialSectionName(sym->st_shndx));
                    exit(1);
            }

        } else {

            Elf64_Shdr *shdr = &elf->shdrs[sym->st_shndx];
            sym->st_value += shdr->sh_addr;

        }
    }
}

void CTXLayOutSymbols(Context *ctx) {
    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {
        LoadedFile *lfile = ctx->lfiles[i];
        Elf *elf = lfile->elf;
        LayOutELFSymbols(ctx, elf);
    }
}

void CTXPrintSymbols(Context *ctx) {
    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {
        LoadedFile *lfile = ctx->lfiles[i];
        Elf *elf = lfile->elf;
        ELFPrintSymTab(stdout, elf);
    }
}

typedef struct {
    size_t size;
    char *data;
} Buffer;

static Buffer ReadFile(char *name) {
    FILE *in = fopen(name, "rb");
    assert(in);
    Buffer buffer;
    fseek(in, 0, SEEK_END);
    long len = ftell(in);
    assert(len > 0);
    buffer.size = (size_t) len;
    rewind(in);
    buffer.data = malloc(buffer.size);
    fread(buffer.data, 1, buffer.size, in);
    fclose(in);
    return buffer;
}

static Elf64_Sym *FindSymbol(Context *ctx, char *name) {
    for (size_t i = 0; i < ctx->lfiles_cnt; i++) {
        LoadedFile *lfile = ctx->lfiles[i];
        Elf *elf = lfile->elf;
        for (size_t k = 0; k < elf->sym_cnt; k++) {
            Elf64_Sym *sym = &elf->sym_tab[k];
            if (ELF64_ST_BIND(sym->st_info) == STB_GLOBAL) {
                char *sym_name = &elf->sym_str_tab[sym->st_name];
                if (strcmp(sym_name, name) == 0) {
                    return sym;
                }
            }
        }
    }
    return 0;
}

void CTXCreateExecutable(Context *ctx, char *name) {

    FILE *out = fopen(name, "wb");
    assert(out);

    Elf64_Sym *entry = FindSymbol(ctx, "_start");
    assert(entry != 0);

    ELFIdent ident = {
        .FileIdent      = { 0x7f, 'E', 'L', 'F' },
        .FileClass      = ELFCLASS64,
        .DataEncoding   = ELFDATA2LSB,
        .FileVersion    = EV_CURRENT,
        .OSABIIdent     = 0,
        .ABIVersion     = 0
    };

    Elf64_Ehdr ehdr = {

        .e_type         = ET_EXEC,
        .e_machine      = EM_X86_64,
        .e_version      = EV_CURRENT,

        .e_entry        = entry->st_value,

        .e_phoff        = sizeof(Elf64_Ehdr),
        .e_phnum        = (short unsigned) ctx->seg_count,
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

    size_t data_off = 0;
    data_off += sizeof(Elf64_Ehdr);
    data_off += ctx->seg_count * sizeof(Elf64_Phdr);

    data_off = Align4k(data_off);

    for (size_t i = 0; i < ctx->seg_count; i++) {

        SegRef *seg_ref = &ctx->seg_refs[i];
        Elf64_Phdr *phdr = seg_ref->phdr;

        phdr->p_offset = data_off;
        data_off += phdr->p_filesz;
        data_off = Align4k(data_off);

        printf("%lu + %lu (%lu)\n", phdr->p_offset, phdr->p_filesz, phdr->p_memsz);

        fwrite(phdr, sizeof(Elf64_Phdr), 1, out);
    }

    for (size_t i = 0; i < ctx->seg_count; i++) {

        SegRef *seg_ref = &ctx->seg_refs[i];
        Elf64_Phdr *phdr = seg_ref->phdr;

        fseek(out, (long) phdr->p_offset, SEEK_SET);

        for (size_t k = 0; k < seg_ref->sec_count; k++) {

            SecRef *sec_ref = &seg_ref->sec_refs[k];
            Elf64_Shdr *shdr = sec_ref->shdr;

            if (shdr->sh_type == SHT_PROGBITS) {
                char *data = &sec_ref->elf->raw[shdr->sh_offset];
                fwrite(data, 1, shdr->sh_size, out);
            }
        }
    }

    fputc(0, out);
    fclose(out);
}

void CTXGroupIntoSegments(Context *ctx) {

    uintmax_t last_flags = 0;

    for (size_t i = 0; i < ctx->sec_count; i++) {
        
        SecRef *ref = &ctx->sec_refs[i];
        Elf64_Shdr *shdr = ref->shdr;

        switch (shdr->sh_type) {

            case SHT_PROGBITS:
            case SHT_NOBITS:
                break;

            default:
                fprintf(
                    stderr,
                    "Can't output section [%s] to executable\n",
                    ELFSectionTypeName(shdr->sh_type));
                exit(1);
        }

        if (last_flags != shdr->sh_flags) {

            ctx->seg_count++;
            ctx->seg_refs = realloc(ctx->seg_refs, ctx->seg_count * sizeof(SegRef));

            SegRef *seg_ref = &ctx->seg_refs[ctx->seg_count - 1];

            Elf64_Phdr *phdr = calloc(1, sizeof(Elf64_Phdr));
                
            phdr->p_type     = PT_LOAD;
            phdr->p_flags    = PF_R;
            phdr->p_offset   = 0;
            phdr->p_vaddr    = shdr->sh_addr;
            phdr->p_paddr    = 0;
            phdr->p_filesz   = 0;
            phdr->p_memsz    = 0;
            phdr->p_align    = PAGE_SIZE;

            if (shdr->sh_flags & SHF_WRITE) {
                phdr->p_flags |= PF_W;
            }

            if (shdr->sh_flags & SHF_EXECINSTR) {
                phdr->p_flags |= PF_X;
            }

            seg_ref->phdr = phdr;
            seg_ref->sec_refs = 0;
            seg_ref->sec_count = 0;

            last_flags = shdr->sh_flags;
        }

        SegRef *seg_ref = &ctx->seg_refs[ctx->seg_count - 1];

        Elf64_Phdr *phdr = seg_ref->phdr;
        phdr->p_memsz += shdr->sh_size;
        if (shdr->sh_type == SHT_PROGBITS) {
            phdr->p_filesz += shdr->sh_size;
        }

        seg_ref->sec_count++;
        seg_ref->sec_refs = realloc(seg_ref->sec_refs, seg_ref->sec_count * sizeof(SecRef));
        seg_ref->sec_refs[seg_ref->sec_count - 1] = *ref;
    }

    printf("Segments %lu\n", ctx->seg_count);
}
