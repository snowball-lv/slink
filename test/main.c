#include <stddef.h>
#include "system.h"
#include "mod.h"


int main() {
    
    print("Hello world! From pure C!\n");

    char *str = "Test string\n";
    print(str);
    
    mod_foo();

    char buffer[64];
    
    i2str(-1337, buffer);
    print(buffer);
    print("\n");
    
    i2str(0, buffer);
    print(buffer);
    print("\n");
    
    i2str(1337, buffer);
    print(buffer);
    print("\n");

    printf("printf int %i\n", 1337);
    printfln("printf int %i", -1337);


    return 0;
}
