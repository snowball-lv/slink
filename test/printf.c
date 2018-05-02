#include "system.h"
#include <stdarg.h>


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
