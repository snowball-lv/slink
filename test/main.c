#include <stddef.h>
#include "system.h"
#include "mod.h"


int main() {
    
    print("Hello world! From pure C!\n");

    char *str = "Test string\n";
    print(str);
    
    mod_foo();

    // char buffer[64];

    return 0;
}
