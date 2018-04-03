#include <slink/Common.h>
#include <stdlib.h>
#include <string.h>


char *StringCopy(char *str) {
    char *cpy = malloc(strlen(str) + 1);
    strcpy(cpy, str);
    return cpy;
}
