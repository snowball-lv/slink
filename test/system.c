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

int putchar (int character) {
    char str[] = { (char) character, 0 };
    print(str);
    return character;
}

int printf (const char *format, ... ) {

    va_list vl;
    va_start(vl, format);

    const char *ptr = format;
    while (*ptr) {

        if (*ptr == '%') {
            char s = *(ptr + 1);
            if (s == 'i') {

                int i = va_arg(vl, int);
                char buffer[64];
                i2str(i, buffer);
                print(buffer);

                ptr += 2;
                continue;
            }
        }

        putchar(*ptr);
        ptr++;
    }

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
