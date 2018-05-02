#include "system.h"


static int iabs(int i);
static void strrev(char *str);

void i2str(int i, char *buffer) {
    
    char *ptr = buffer;

    if (i < 0) {
        *ptr = '-';
        ptr++;
    }

    int v = iabs(i);

    while (1) {
        int r = v % 10;
        *ptr = (char) ('0' + r);
        ptr++;
        v = v / 10;
        if (v == 0) {
            break;
        }
    }

    *ptr = 0;

    if (i < 0) {
        strrev(buffer + 1);
    } else {
        strrev(buffer);
    }
}

static int iabs(int i) {
    if (i < 0) {
        i *= -1;
    }
    return i;
}

static void strrev(char *str) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len / 2; i++) {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}
