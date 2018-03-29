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

void ARPrintFileHeader(ARFileHeader *header) {
    printf("FileIdentifier: [%.16s]", header->FileIdentifier);
    // printf("Timestamp: [%.12s]\n", header->Timestamp);
    // printf("OwnerID: [%.6s]\n", header->OwnerID);
    // printf("GroupID: [%.6s]\n", header->GroupID);
    // printf("FileMode: [%.8s]\n", header->FileMode);
    // printf("FileSize: [%.10s]\n", header->FileSize);
    // printf("Ending: 0x%x 0x%x\n", header->Ending[0], header->Ending[1]);
}
