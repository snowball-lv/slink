#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
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

            char *name_table = 0;

            printf("\n");
            while (1) {

                ARFileHeader header;
                fread(&header, sizeof(header), 1, file);

                if (feof(file)) {
                    break;
                }

                ARPrintFileHeader(&header);

                size_t file_size = strtoul(header.FileSize, 0, 0);
                assert(file_size <= LONG_MAX);

                if (strncmp("/ ", header.FileIdentifier, 2) == 0) {

                    // TODO
                    fseek(file, (long int) file_size, SEEK_CUR);

                } else if (strncmp("// ", header.FileIdentifier, 3) == 0) {

                    assert(name_table == 0);
                    name_table = malloc(file_size);
                    fread(name_table, 1, file_size, file);
                    for (size_t i = 0; i < file_size; i++) {
                        if (name_table[i] == '\n') {
                            name_table[i] = 0;
                        }
                    }

                } else {

                    char buffer[16] = { 0 };
                    if (sscanf(header.FileIdentifier, "/%15[0-9]s", buffer) > 0) {
                        size_t name_off = strtoul(buffer, 0, 10);
                        printf(" [%s]", &name_table[name_off]);
                    }

                    fseek(file, (long int) file_size, SEEK_CUR);
                }

                printf("\n");
            }
            printf("\n");


            fclose(file);
        }
    }



    return 0;
}
