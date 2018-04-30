#include <slink/Amd64.h>

#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <slink/Log.h>
#include <slink/Common.h>
#include <slink/Error.h>

void Amd64ApplyRelocations(SymTab *symtab, Section *sec) {

    for (size_t i = 1; i < ZTAS(sec->relas); i++) {
        if (sec->relas[i - 1]->offset == sec->relas[i]->offset) {
            ERROR("Consecutive relocations are not supported\n");
        }
    }

    for (size_t i = 0; i < ZTAS(sec->relas); i++) {

        RelocationA *rela = sec->relas[i];
        Symbol *sym = sec->symtab[rela->sym];
        Section *target = sec->target;

        if (sym->shndx == SHN_UNDEF) {
            sym = SymTabGetDef(symtab, sym->name);
            if (sym == 0) {
                ERROR("[%s] is undefined\n", sym->name);
            }
        }

        Log(
            "rela", 
            "%s [%s] [%s] [%s]\n",
            ELFRelTypeName(rela->type),
            sym->name,
            sym->is_shndx_special ? 
                ELFSpecialSectionName(sym->shndx) :
                sym->sec->name,
            target->name);

        /*
            S - Represents the value of the symbol whose index resides in 
            the relocation entry.

            A - Represents the addend used to compute the value of the
            relocatable field.

            P - Represents the place (section offset or address) of the 
            storage unit being relocated (computed using r_offset).
        */

        switch (rela->type) {

            // S + A - P
            // word32
            case R_X86_64_PC32: {
                
                assert(sym->value <= INT32_MAX);
                int32_t value = (int32_t) sym->value;

                assert(rela->addend >= INT32_MIN);
                assert(rela->addend <= INT32_MAX);
                value += (int32_t) rela->addend;

                assert(rela->offset <= INT32_MAX);
                value -= (int32_t) (target->addr + rela->offset);

                int32_t *ptr = (int32_t *) &target->data[rela->offset];
                *ptr = value;

                break;
            }

            // S + A
            // word32
            case R_X86_64_32: {

                assert(sym->value <= INT32_MAX);
                int32_t value = (int32_t) sym->value;

                assert(rela->addend >= INT32_MIN);
                assert(rela->addend <= INT32_MAX);
                value += (int32_t) rela->addend;

                int32_t *ptr = (int32_t *) &target->data[rela->offset];
                *ptr = value;

                break;
            }

            // S + A
            // word64
            case R_X86_64_64: {
                
                assert(sym->value <= INT64_MAX);
                int64_t value = (int64_t) sym->value;

                assert(rela->addend >= INT64_MIN);
                assert(rela->addend <= INT64_MAX);
                value += (int64_t) rela->addend;

                int64_t *ptr = (int64_t *) &target->data[rela->offset];
                *ptr = value;

                break;
            }

            // The R_X86_64_32 and R_X86_64_32S relocations truncate the computed
            // value to 32-bits. The linker must verify that the generated value 
            // for the R_X86_64_32 (R_X86_64_32S) relocation zero-extends 
            // (sign-extends) to the original 64-bit value.

            // S + A
            // word32
            case R_X86_64_32S: {

                assert(sym->value <= INT32_MAX);
                int32_t value = (int32_t) sym->value;

                assert(rela->addend >= INT32_MIN);
                assert(rela->addend <= INT32_MAX);
                value += (int32_t) rela->addend;

                int32_t *ptr = (int32_t *) &target->data[rela->offset];
                *ptr = value;

                break;
            }

            default:
                ERROR(
                    "Can't handle relocs of type %u [%s]\n",
                    rela->type, ELFRelTypeName(rela->type));
        }

        // end of switch
    }
}
