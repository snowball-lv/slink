#include <stdio.h>

#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>


int main(int argc, char **argv) {
    printf("--- S LINK ---\n");

    char **inputs = &argv[1];
    int input_cnt = argc - 1;

    for (int i = 0; i < input_cnt; i++) {
        char *path = inputs[i];

        printf("File [");

        if (IsElf(path)) {
            printf("ELF");
        } else if (IsArchive(path)) {
            printf("Archive");
        } else {
            printf("unknown");
        }

        printf("] ");

        printf("[%s]\n", path);
    }



    return 0;
}
