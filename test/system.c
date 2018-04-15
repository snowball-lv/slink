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

static int putchar (int character) {
    char str[] = { (char) character, 0 };
    print(str);
    return character;
}

int printf (const char *format, ... ) {

    va_list vl;
    va_start(vl, format);

    print(format);

    va_end(vl);
    return 0;
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

void i2str(int i, char *buffer) {
    
    char *ptr = buffer;

    if (i < 0) {
        *ptr = '-';
        ptr++;
    }

    int v = abs(i);

    while (1) {
        int r = v % 10;
        *ptr = '0' + r;
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
