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
    Elf *def_by;
    Elf64_Sym *def;
} Global;

typedef struct {
    Elf *elf;
    Elf64_Shdr *shdr;
} SecRef;

typedef struct {

    char **ifiles;
    size_t ifiles_cnt;

    LoadedFile **lfiles;
    size_t lfiles_cnt;

    Global **undefs;
    size_t undefs_cnt;

    SecRef *sec_refs;
    size_t sec_count;

} Context;

void CTXLoadInputFiles(Context *ctx);

void CTXCollectUndefs(Context *ctx);
int CTXResolveUndefs(Context *ctx);

void CTXPrintUndefs(Context *ctx);

void CTXCollectSections(Context *ctx);
void CTXPrintSections(Context *ctx);

size_t CTXCountModules(Context *ctx);
