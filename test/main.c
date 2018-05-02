#include <stddef.h>
#include <slink-c.h>
#include "mod.h"


char my_common[1337];
int my_common_int;

const int MY_CONST = 1337;

int main() {
    
    print("Hello world! From pure C!\n");

    char *str = "Test string\n";
    print(str);
    
    printf("pre: my common int %i\n", my_common_int);
    mod_foo();
    printf("post: my common int %i\n", my_common_int);

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

    printf("const int %i\n", MY_CONST);

    return 0;
}
