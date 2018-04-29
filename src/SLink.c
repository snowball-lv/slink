#include <slink/SLink.h>

#include <stdlib.h>
#include <slink/Common.h>


static uint8_t IsSectionSpecial(uint16_t shndx) {
    return shndx == SHN_UNDEF || SHN_LORESERVE <= shndx;
}

Section **ExtractSections(Elf *elf) {

    Section **secs = calloc(elf->shnum + 1, sizeof(Section *));

    for (size_t i = 0; i < elf->shnum; i++) {

        Elf64_Shdr *shdr = &elf->shdrs[i];
        Section *sec = calloc(1, sizeof(Section));

        sec->src = elf->path;

        char *name = &elf->sec_name_str_tab[shdr->sh_name];
        sec->name = name;
        
        sec->type = shdr->sh_type;
        
        sec->flags = shdr->sh_flags;
        sec->falloc = (shdr->sh_flags & SHF_ALLOC) != 0;
        sec->fwrite = (shdr->sh_flags & SHF_WRITE) != 0;
        sec->fexecinstr = (shdr->sh_flags & SHF_EXECINSTR) != 0;

        sec->addr = shdr->sh_addr;
        sec->addralign = shdr->sh_addralign;
        sec->size = shdr->sh_size;

        sec->data = (uint8_t *) &elf->raw[shdr->sh_offset];

        switch (shdr->sh_type) {

            case SHT_SYMTAB: {

                Elf64_Shdr *strtab_shdr = &elf->shdrs[shdr->sh_link];
                char *strtab = &elf->raw[strtab_shdr->sh_offset];

                sec->symtab = calloc(elf->sym_cnt + 1, sizeof(Symbol *));
                for (size_t k = 0; k < elf->sym_cnt; k++) {

                    Symbol *sym = calloc(1, sizeof(Symbol));
                    Elf64_Sym *esym = &elf->sym_tab[k];

                    sym->name = &strtab[esym->st_name];
                    sym->binding = ELF64_ST_BIND(esym->st_info);
                    sym->type = ELF64_ST_TYPE(esym->st_info);

                    sym->is_shndx_special = IsSectionSpecial(esym->st_shndx);
                    sym->shndx = esym->st_shndx;
                    sym->value = esym->st_value;

                    // patch after the loop
                    // requires all Section objects to be created
                    sym->sec = 0;

                    sec->symtab[k] = sym;
                }

                break;
            }

            case SHT_RELA: {

                size_t count = shdr->sh_size / shdr->sh_entsize;
                sec->relas = calloc(count + 1, sizeof(RelocationA *));
                for (size_t k = 0; k < count; k++) {

                    size_t off = shdr->sh_offset + k * shdr->sh_entsize;
                    Elf64_Rela *erela = (Elf64_Rela *) &elf->raw[off];
                    RelocationA *rela = calloc(1, sizeof(RelocationA));

                    rela->offset = erela->r_offset;
                    rela->addend = erela->r_addend;
                    rela->type = ELF64_R_TYPE(erela->r_info);
                    rela->sym = (uint32_t) ELF64_R_SYM(erela->r_info);

                    sec->relas[k] = rela;
                }

                break;
            }
        }

        secs[i] = sec;
    }

    for (size_t i = 0; i < ZTAS(secs); i++) {

        Section *sec = secs[i];

        if (sec->type == SHT_SYMTAB) {
            for (size_t k = 0; k < ZTAS(sec->symtab); k++) {
                Symbol *sym = sec->symtab[k];
                if (!sym->is_shndx_special) {
                    sym->sec = secs[sym->shndx];
                }
            }
        } else if (sec->type == SHT_RELA) {
            Elf64_Shdr *shdr = &elf->shdrs[i];
            sec->symtab = secs[shdr->sh_link]->symtab;
            sec->target = secs[shdr->sh_info];
        }
    }

    return secs;
}
