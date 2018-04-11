#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>


typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

#define EI_NIDENT   16

#define SHN_UNDEF   0
#define SHN_XINDEX  0xffff
#define SHN_ABS     0xfff1
#define SHN_COMMON  0xfff2

#define SHT_NOBITS  8
#define SHT_SYMTAB  2

#define ELF_MAGIC   "\x7f" "ELF"
#define ELFCLASS64  2
#define ELFDATA2LSB 1
#define EM_X86_64   62
#define ET_REL      1
#define EV_CURRENT  1

#define ELF64_ST_BIND(i)        ((i)>>4)
#define ELF64_ST_TYPE(i)        ((i)&0xf)
#define ELF64_ST_VISIBILITY(o)  ((o)&0x3)

#define ELF64_R_SYM(i)    ((i)>>32)
#define ELF64_R_TYPE(i)   ((i)&0xffffffffL)

typedef union {
    struct {
        char    FileIdent[4];
        uint8_t FileClass;
        uint8_t DataEncoding;
        uint8_t FileVersion;
        uint8_t OSABIIdent;
        uint8_t ABIVersion;
    };
    char __total_size[EI_NIDENT];
} ELFIdent;

typedef struct {
    unsigned char   e_ident[EI_NIDENT];
    Elf64_Half      e_type;
    Elf64_Half      e_machine;
    Elf64_Word      e_version;
    Elf64_Addr      e_entry;
    Elf64_Off       e_phoff;
    Elf64_Off       e_shoff;
    Elf64_Word      e_flags;
    Elf64_Half      e_ehsize;
    Elf64_Half      e_phentsize;
    Elf64_Half      e_phnum;
    Elf64_Half      e_shentsize;
    Elf64_Half      e_shnum;
    Elf64_Half      e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    Elf64_Word	sh_name;
    Elf64_Word	sh_type;
    Elf64_Xword	sh_flags;
    Elf64_Addr	sh_addr;
    Elf64_Off	sh_offset;
    Elf64_Xword	sh_size;
    Elf64_Word	sh_link;
    Elf64_Word	sh_info;
    Elf64_Xword	sh_addralign;
    Elf64_Xword	sh_entsize;
} Elf64_Shdr;

typedef struct {
	Elf64_Word     st_name;
	unsigned char  st_info;
	unsigned char  st_other;
	Elf64_Half     st_shndx;
	Elf64_Addr     st_value;
	Elf64_Xword    st_size;
} Elf64_Sym;

typedef struct {
    Elf64_Word	p_type;
    Elf64_Word	p_flags;
    Elf64_Off	p_offset;
    Elf64_Addr	p_vaddr;
    Elf64_Addr	p_paddr;
    Elf64_Xword	p_filesz;
    Elf64_Xword	p_memsz;
    Elf64_Xword	p_align;
} Elf64_Phdr;

typedef struct {
    Elf64_Addr	    r_offset;
    Elf64_Xword	    r_info;
    Elf64_Sxword	r_addend;
} Elf64_Rela;

typedef struct {

    char *path;
    int index;
    char *raw;

    Elf64_Ehdr *ehdr;

    Elf64_Xword	shnum;
    Elf64_Shdr *shdrs;
    Elf64_Word shstrndx;
    char *sec_name_str_tab;

    Elf64_Phdr *phdrs;

    Elf64_Shdr *sym_tab_sh;
    Elf64_Sym *sym_tab;
    size_t sym_cnt;
    char *sym_str_tab;

} Elf;

int IsElf(char *path);
void ELFRead(char *path, Elf *elf);
void ELFReadFromMem(char *name, char *mem, size_t size, Elf *elf);

char *ELFFileClassName(uint8_t FileClass);
char *ELFDataEncodingName(uint8_t DataEncoding);
char *ELFFileVersionName(uint8_t FileVersion);
char *ELFOSABIIdentName(uint8_t OSABIIdent);

char *ELFTypeName(Elf64_Half e_type);
char *ELFMachineName(Elf64_Half e_machine);
char *ELFVersionName(Elf64_Word e_version);

extern Elf64_Xword ELF_SHFS[];
extern size_t ELF_SHFS_CNT;

char *ELFSectionTypeName(Elf64_Word sh_type);
char *ELFSectionFlagName(Elf64_Xword sh_flag);
char *ELFSpecialSectionName(Elf64_Word index);
int ELFIsSectionSpecial(Elf64_Half st_shndx);

char *ELFSymBindingName(unsigned char st_bind);
char *ELFSymTypeName(unsigned char st_type);
char *ELFSymVisibilityName(unsigned char st_other);

void ELFPrintIdent(ELFIdent *ident);
void ELFPrintPHdr(Elf64_Phdr *phdr);

void ELFPrintEHdr(FILE *file, Elf *elf, Elf64_Ehdr *ehdr);
void ELFPrintSHdr(FILE *file, Elf *elf, Elf64_Shdr *shdr);
void ELFPrintSymTab(FILE *file, Elf *elf);
void ELFPrintStrTab(FILE *file, Elf *elf);
void ELFPrintRelocs(FILE *file, Elf *elf, Elf64_Shdr *shdr);

char *ELFSegmentTypeName(Elf64_Word p_type);

extern Elf64_Word ELF_SPFS[];
extern int ELF_SPFS_CNT;

char *ELFSegmentPermissionFlagName(Elf64_Word p_flags);

int ELFIsShNdxSpecial(Elf64_Half index);

char *ELFRelTypeName(unsigned type);

#define SHT_NULL            0
#define SHT_PROGBITS        1
#define SHT_SYMTAB          2
#define SHT_STRTAB          3
#define SHT_RELA            4
#define SHT_HASH            5
#define SHT_DYNAMIC         6
#define SHT_NOTE            7
#define SHT_NOBITS          8
#define SHT_REL             9
#define SHT_SHLIB           10
#define SHT_DYNSYM          11
#define SHT_INIT_ARRAY      14
#define SHT_FINI_ARRAY      15
#define SHT_PREINIT_ARRAY   16
#define SHT_GROUP           17
#define SHT_SYMTAB_SHNDX    18
#define SHT_LOOS            0x60000000
#define SHT_HIOS            0x6fffffff
#define SHT_LOPROC          0x70000000
#define SHT_HIPROC          0x7fffffff
#define SHT_LOUSER          0x80000000
#define SHT_HIUSER          0xffffffff

#define SHF_WRITE               0x1
#define SHF_ALLOC               0x2
#define SHF_EXECINSTR           0x4
#define SHF_MERGE               0x10
#define SHF_STRINGS             0x20
#define SHF_INFO_LINK           0x40
#define SHF_LINK_ORDER          0x80
#define SHF_OS_NONCONFORMING    0x100
#define SHF_GROUP               0x200
#define SHF_TLS                 0x400
#define SHF_MASKOS              0x0ff00000
#define SHF_MASKPROC            0xf0000000

#define PT_INTERP   3

#define R_X86_64_64     1
#define R_X86_64_PC32   2
#define R_X86_64_32     10
#define R_X86_64_32S    11

#define STB_LOCAL       0

#define STV_DEFAULT     0
#define STV_HIDDEN      2

#define STB_GLOBAL      1
#define STB_WEAK        2

#define ET_EXEC     2
#define PT_LOAD     1
#define PF_X        0x1
#define PF_R        0x4