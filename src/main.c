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

    ctx.needs_sym_pass = 1;
    while (ctx.needs_sym_pass) {
        
        ctx.needs_sym_pass = 0;

        printf("\n");
        printf("----------------------------\n");
        printf("          NEW PASS\n");
        printf("----------------------------\n");
        printf("\n");

        CTXCollectUndefs(&ctx);
        CTXResolveUndefs(&ctx);
    }

    printf("\n");
    printf("----------------------------\n");
    printf("            DONE\n");
    printf("----------------------------\n");
    printf("\n");

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