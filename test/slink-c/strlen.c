#include <stddef.h>


size_t strlen(char *str) {
    size_t len = 0;
    for (; str[len]; len++);
    return len;
}
