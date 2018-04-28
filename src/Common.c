#include <slink/Common.h>
#include <stdlib.h>
#include <string.h>


char *StringCopy(char *str) {
    char *cpy = malloc(strlen(str) + 1);
    strcpy(cpy, str);
    return cpy;
}

size_t ZTArraySize(void **arr) {
    
    if (arr == 0) {
        return 0;
    }

    size_t size = 0;
    while (arr[size]) {
        size++;
    }
    return size;
}
