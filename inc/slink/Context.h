#pragma once

#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>


typedef struct {
    char *path;
    Elf *elf;
    Archive *archive;
} LoadedFile;

typedef struct {
    char *name;
    int defined;
    unsigned char binding;
} Global;

typedef struct {

    char **ifiles;
    size_t ifiles_cnt;

    LoadedFile **lfiles;
    size_t lfiles_cnt;

    Global **undefs;
    size_t undefs_cnt;

} Context;

void CTXLoadInputFiles(Context *ctx);

void CTXCollectUndefs(Context *ctx);
int CTXResolveUndefs(Context *ctx);

void CTXPrintUndefs(Context *ctx);
