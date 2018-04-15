#include "system.h"
#include <stdarg.h>

size_t strlen(char *str) {
    size_t len = 0;
    for (; str[len]; len++);
    return len;
}

int print(char *str) {
    return write(1, str, strlen(str));
}
