#pragma once

#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>
#include <slink/SymTab.h>


#define PAGE_SIZE   4096

typedef struct {
    char *path;
    Elf *elf;
} LoadedFile;

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

    SecRef *sec_refs;
    size_t sec_count;

    SegRef *seg_refs;
    size_t seg_count;

    SymTab symtab;

} Context;

void CTXLoadInputFiles(Context *ctx);

void CTXPrintUndefs(Context *ctx);

void CTXCollectSections(Context *ctx);
void CTXPrintSections(Context *ctx);

size_t CTXCountModules(Context *ctx);

void CTXProcessRelocations(Context *ctx);

void CTXLayOutSymbols(Context *ctx);
void CTXPrintSymbols(Context *ctx);

void CTXCreateExecutable(Context *ctx, char *name);

void CTXGroupIntoSegments(Context *ctx);

void CTXLinkSymbols(Context *ctx);
