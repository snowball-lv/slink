#include "mod.h"
#include <slink-c.h>


char my_common[777];
int my_common_int;

void mod_foo() {
    my_common_int = 888;
    print("called mod_foo()\n");
}
