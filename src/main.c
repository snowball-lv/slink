#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include <slink/Context.h>
#include <slink/elf/ELF.h>
#include <slink/elf/Archive.h>


// 1. Link undefined symbols to their definitions
// 2. Order sections of all input objects
// 3. Lay out symbols
// 4. Apply relocations
// 5. Output executable elf

int main(int argc, char **argv) {

    printf("--- S LINK ---\n");

    FILE *symtablog = fopen("symtab.log.txt", "w");
    fprintf(symtablog, "--- SYM TAB LOG ---\n");
    fclose(symtablog);

    // define linking context
    Context ctx = { 0 };

    // save list of input files
    ctx.ifiles_cnt = (size_t) argc - 1;
    ctx.ifiles = &argv[1];

    // print input files
    for (size_t i = 0; i < ctx.ifiles_cnt; i++) {
        printf("Input File [%s]\n", ctx.ifiles[i]);
    }
    
    CTXLoadInputFiles(&ctx);

    CTXLinkSymbols(&ctx);

    CTXPrintUndefs(&ctx);

    CTXCollectSections(&ctx);
    CTXPrintSections(&ctx);

    printf("%lu modules loaded\n", CTXCountModules(&ctx));

    CTXPrintSymbols(&ctx);

    printf("\n");
    printf("Laying out symbols\n");

    CTXLayOutSymbols(&ctx);
    CTXPrintSymbols(&ctx);

    CTXProcessRelocations(&ctx);

    CTXGroupIntoSegments(&ctx);
    CTXCreateExecutable(&ctx, "hello_world");

    return 0;
}