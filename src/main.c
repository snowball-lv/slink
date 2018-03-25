#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <slink/elf/ELF.h>


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

    return strcmp(na, nb);
    // return 0;
}

static void OrderSecList(SecRef *list, size_t length) {
    qsort(list, length, sizeof(SecRef), sec_order_compar);
}

int main(int argc, char **argv) {
    printf("--- S LINK ---\n");
    
    FILE *log_sec = fopen("log-sec.txt", "wb");
    FILE *log_sym = fopen("log-sym.txt", "wb");
    FILE *log_sec_order = fopen("log-sec-order.txt", "wb");

    assert(log_sec);
    assert(log_sym);
    assert(log_sec_order);

    printf("Inputs: %i\n", argc - 1);

    Elf *elfs = malloc(sizeof(Elf) * (unsigned) (argc - 1));

    for (int i = 1; i < argc; i++) {
        char *path = argv[i];
        Elf *elf = &elfs[i - 1];
        ELFRead(path, elf);
        elf->index = i - 1;
    }

    size_t sec_cnt = 0;    
    for (int i = 1; i < argc; i++) {
        Elf *elf = &elfs[i - 1];
        sec_cnt += elf->shnum;
    }

    SecRef *sec_list = malloc(sec_cnt * sizeof(SecRef));

    printf("\n");
    int sec_counter = 0;
    for (int i = 1; i < argc; i++) {

        Elf *elf = &elfs[i - 1];

        printf("File: %i [%s]\n", elf->index, elf->path);

        fprintf(log_sec, "\n");
        for (size_t k = 0; k < elf->shnum; k++) {

            Elf64_Shdr *shdr = &elf->shdrs[k];
            ELFPrintSHdr(log_sec, elf, shdr);

            sec_list[sec_counter].elf = elf;
            sec_list[sec_counter].shdr = shdr;
            sec_counter++;
        }

        ELFPrintSymTab(log_sym, elf);
    }

    OrderSecList(sec_list, sec_cnt);

    for (size_t i = 0; i < sec_cnt; i++) {
        SecRef *ref = &sec_list[i];
        ELFPrintSHdr(log_sec_order, ref->elf, ref->shdr);
    }

    fclose(log_sec);
    fclose(log_sym);
    fclose(log_sec_order);

    return 0;
}
