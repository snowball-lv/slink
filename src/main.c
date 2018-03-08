#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <slink/elf/ELF.h>


int main(int argc, char **argv) {
    printf("--- S LINK ---\n");
    
    printf("\n");
    int in_cnt = argc - 1;
    printf("Inputs: %d\n", in_cnt);
    
    for (int i = 1; i < argc; i++) {
        
        char *name = argv[i];
        printf("\n");
        printf("File: [%s]\n", name);
        
    }
    
    return 0;
}
