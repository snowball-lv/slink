#include "system.h"


int print(char *str) {
    return sys_write(1, str, strlen(str));
}
