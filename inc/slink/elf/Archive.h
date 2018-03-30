#pragma once


#define AR_MAGIC    "!<arch>\n"
#define AR_ENDING   "`\n"

int IsArchive(char *path);

typedef struct {
    char FileIdentifier[16];
    char Timestamp[12];
    char OwnerID[6];
    char GroupID[6];
    char FileMode[8];
    char FileSize[10];
    char Ending[2];
} ARFileHeader;

typedef struct {

    char *data;
    size_t data_length;

    ARFileHeader **headers;
    int header_count;
    
} Archive;

void ARReadArchive(char *path, Archive *archive);

void ARPrintFileHeader(ARFileHeader *header);
