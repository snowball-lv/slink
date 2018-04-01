#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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
    printf("FileIdentifier: [%.16s]\n", header->FileIdentifier);
    // printf("Timestamp: [%.12s]\n", header->Timestamp);
    // printf("OwnerID: [%.6s]\n", header->OwnerID);
    // printf("GroupID: [%.6s]\n", header->GroupID);
    // printf("FileMode: [%.8s]\n", header->FileMode);
    // printf("FileSize: [%.10s]\n", header->FileSize);
    // printf("Ending: 0x%x 0x%x\n", header->Ending[0], header->Ending[1]);
}

static int FileHeaderCount(Archive *archive) {

    int count = 0;

    assert(strncmp(archive->data, AR_MAGIC, strlen(AR_MAGIC)) == 0);

    size_t off = 0;
    off += strlen(AR_MAGIC);

    while (off < archive->data_length) {

        ARFileHeader *header = (ARFileHeader *) &archive->data[off];
        
        assert(strncmp(AR_ENDING, header->Ending, 2) == 0);

        off += sizeof(ARFileHeader);

        size_t file_size = strtoul(header->FileSize, 0, 10);
        off += file_size;

        if (off % 2 != 0) {
            off++;
        }

        count++;
    }

    return count;
}

static ARFileHeader *FindFile(Archive *archive, char *name) {

    assert(strlen(name) <= 16);

    char name_buf[16];
    memset(name_buf, ' ', 16);
    memcpy(name_buf, name, strlen(name));

    size_t off = 0;
    off += strlen(AR_MAGIC);

    while (off < archive->data_length) {

        ARFileHeader *header = (ARFileHeader *) &archive->data[off];
        
        assert(strncmp(AR_ENDING, header->Ending, 2) == 0);

        if (memcmp(name_buf, header->FileIdentifier, 16) == 0) {
            return header;
        }

        off += sizeof(ARFileHeader);

        size_t file_size = strtoul(header->FileSize, 0, 10);
        off += file_size;

        if (off % 2 != 0) {
            off++;
        }
    }

    return 0;
}

void ARReadArchive(char *path, Archive *archive) {

    if (!IsArchive(path)) {
        return;
    }

    FILE *file = fopen(path, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    assert(length >= 0);

    archive->data_length = (size_t) length;

    rewind(file);

    archive->data = malloc(archive->data_length);
    fread(archive->data, 1, archive->data_length, file);

    fclose(file);

    assert(strncmp(archive->data, AR_MAGIC, strlen(AR_MAGIC)) == 0);

    ARFileHeader *sym_tab_fh = FindFile(archive, "/");
    assert(sym_tab_fh != 0);

    archive->sym_tab = ((char *) sym_tab_fh) + sizeof(ARFileHeader);
}

static uint32_t ReadU32BE(char *ptr) {
    char buff[4] = {
        ptr[3], ptr[2], ptr[1], ptr[0]
    };
    return *(uint32_t *) buff;
}

int ARDefinesSymbol(Archive *archive, char *name) {

    char *ptr = archive->sym_tab;
    uint32_t sym_cnt = ReadU32BE(ptr);
    
    ptr += 4;
    ptr += 4 * sym_cnt;

    for (size_t i = 0; i < sym_cnt; i++) {
        if (strcmp(name, ptr) == 0) {
            return 1;
        }
        ptr += strlen(ptr) + 1;
    }

    return 0;
}