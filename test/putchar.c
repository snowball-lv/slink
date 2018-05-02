#include "system.h"


int putchar (int character) {
    char str[] = { (char) character, 0 };
    print(str);
    return character;
}
