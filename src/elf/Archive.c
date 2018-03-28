#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <slink/elf/Archive.h>


int IsArchive(char *path) {

    FILE *file = fopen(path, "rb");
    assert(file);

    char magic[sizeof(AR_MAGIC)];
    fread(magic, 1, sizeof(AR_MAGIC) - 1, file);

    fclose(file);

    return strncmp(AR_MAGIC, magic, sizeof(AR_MAGIC) - 1) == 0;
}
