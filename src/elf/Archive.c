#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <slink/elf/Archive.h>
#include <slink/Common.h>


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

static uint32_t ReadU32BE(char *ptr) {
    char buff[4] = {
        ptr[3], ptr[2], ptr[1], ptr[0]
    };
    return *(uint32_t *) buff;
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

    ARFileHeader *str_tab_fh = FindFile(archive, "//");
    assert(str_tab_fh != 0);
    archive->str_tab = ((char *) str_tab_fh) + sizeof(ARFileHeader);

    // replace LF with 0 in str_tab
    // makes it easier to use as strings
    size_t str_tab_size = strtoul(str_tab_fh->FileSize, 0, 10);
    for (size_t i = 0; i < str_tab_size; i++) {
        if (archive->str_tab[i] == '\n') {
            archive->str_tab[i] = 0;
        }
    }

    archive->loaded = 0;
    archive->loaded_cnt = 0;

    // printf("\n");
    // printf("[%s]\n", path);
    // printf("------------------------------\n");
    // printf("\n");

    // // log symbols
    // char *ptr = archive->sym_tab;
    // uint32_t sym_cnt = ReadU32BE(ptr);
    // ptr += 4;
    // ptr += sym_cnt * 4;
    // for (size_t i = 0; i < sym_cnt; i++) {
    //     printf("[%s]\n", ptr);
    //     ptr += strlen(ptr) + 1;
    // }

    // printf("\n");
    // printf("------------------------------\n");

}

static ARFileHeader *GetSymFH(Archive *archive, char *name) {
     
    char *ptr = archive->sym_tab;
    uint32_t sym_cnt = ReadU32BE(ptr);
    
    ptr += 4;
    char *off_ptr = ptr;
    char *str_ptr = ptr + 4 * sym_cnt;

    for (size_t i = 0; i < sym_cnt; i++) {
        if (strcmp(name, str_ptr) == 0) {
            
            uint32_t off = ReadU32BE(off_ptr);
            ARFileHeader *fh = (ARFileHeader *) &archive->data[off];
            assert(strncmp(AR_ENDING, fh->Ending, 2) == 0);
            return fh;
        }
        str_ptr += strlen(str_ptr) + 1;
        off_ptr += 4;
    }

    return 0;
    
}

int ARDefinesSymbol(Archive *archive, char *name) {
    return GetSymFH(archive, name) != 0;
}

void ARLoadModuleWithSymbol(Archive *archive, char *name) {

    ARFileHeader *fh = GetSymFH(archive, name);
    assert(fh != 0);

    char name_buf[32];
    ARGetFileName(archive, fh, name_buf);

    for (size_t i = 0; i < archive->loaded_cnt; i++) {
        Elf *mod = &archive->loaded[i];
        if (strcmp(name_buf, mod->path) == 0) {
            // printf("Already loaded [%s]\n", name_buf);
            return;
        }
    }

    printf("Loading [%s]\n", name_buf);

    archive->loaded_cnt++;
    archive->loaded = realloc(archive->loaded, archive->loaded_cnt * sizeof(Elf));

    Elf *elf = &archive->loaded[archive->loaded_cnt - 1];
    size_t mem_size = strtoul(fh->FileSize, 0, 10);
    char *mem_ptr = ((char *) fh) + sizeof(ARFileHeader);

    ELFReadFromMem(StringCopy(name_buf), mem_ptr, mem_size, elf);

    // if (strcmp(name_buf, "stdio.o/") == 0) {

    //     for (size_t i = 0; i < elf->sym_cnt; i++) {
    //         Elf64_Sym *sym = &elf->sym_tab[i];
    //         char *name = &elf->sym_str_tab[sym->st_name];
    //         printf("[%s]\n", name);
    //     }

    //     exit(1);
    // }
}

void ARGetFileName(Archive *ar, ARFileHeader *fh, char *buffer) {

    memcpy(buffer, fh->FileIdentifier, 16);
    buffer[17] = 0;

    for (int i = 15; i >= 0; i--) {
        if (buffer[i] == ' ') {
            buffer[i] = 0;
        } else {
            break;
        }
    }

    size_t name_off = 0;
    if (sscanf(buffer, "/%lu", &name_off) > 0) {
        char *lname = &ar->str_tab[name_off];
        sprintf(buffer, "%s-%s", buffer, lname);
    }
}

Elf *ARElfOfSym(Archive *ar, char *name) {

    ARFileHeader *fh = GetSymFH(ar, name);
    assert(fh != 0);

    char name_buf[128];
    ARGetFileName(ar, fh, name_buf);

    for (size_t i = 0; i < ar->loaded_cnt; i++) {
        Elf *mod = &ar->loaded[i];
        if (strcmp(name_buf, mod->path) == 0) {
            return mod;
        }
    }

    fprintf(
        stderr, 
        "Archive doesn not define symbol [%s]\n",
        name);
    exit(1);
}
