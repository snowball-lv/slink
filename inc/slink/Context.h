#pragma once

#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>


#define PAGE_SIZE   4096

typedef struct {
    char *path;
    Elf *elf;
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
    Elf64_Phdr *phdr;
    SecRef *sec_refs;
    size_t sec_count;
} SegRef;

typedef struct {

    char **ifiles;
    size_t ifiles_cnt;

    LoadedFile **lfiles;
    size_t lfiles_cnt;

    Global **undefs;
    size_t undefs_cnt;

    SecRef *sec_refs;
    size_t sec_count;

    int needs_sym_pass;

    SegRef *seg_refs;
    size_t seg_count;

} Context;

void CTXLoadInputFiles(Context *ctx);

void CTXCollectUndefs(Context *ctx);
void CTXResolveUndefs(Context *ctx);

void CTXPrintUndefs(Context *ctx);

void CTXCollectSections(Context *ctx);
void CTXPrintSections(Context *ctx);

size_t CTXCountModules(Context *ctx);

Global **CTXGetUndefs(Context *ctx);

void CTXProcessRelocations(Context *ctx);

void CTXLayOutSymbols(Context *ctx);
void CTXPrintSymbols(Context *ctx);

void CTXCreateExecutable(Context *ctx, char *name);

void CTXGroupIntoSegments(Context *ctx);
