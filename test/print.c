#include "slink-sys.h"
#include "slink-c.h"


int print(char *str) {
    return sys_write(1, str, strlen(str));
}
