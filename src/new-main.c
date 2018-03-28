#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

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

        printf("] [%s]\n", path);

        if (IsArchive(path)) {

            FILE *file = fopen(path, "rb");
            assert(file);

            char magic[sizeof(AR_MAGIC)];
            fread(magic, 1, sizeof(AR_MAGIC) - 1, file);

            assert(strncmp(AR_MAGIC, magic, sizeof(AR_MAGIC) - 1) == 0);

            printf("\n");
            while (1) {

                ARFileHeader header;
                fread(&header, sizeof(header), 1, file);

                if (feof(file)) {
                    break;
                }

                ARPrintFileHeader(&header);

                size_t file_size = strtoul(header.FileSize, 0, 0);
                // printf("%lu\n", file_size);
                fseek(file, file_size, SEEK_CUR);
            }
            printf("\n");


            fclose(file);
        }
    }



    return 0;
}
