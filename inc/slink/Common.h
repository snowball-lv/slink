#pragma once

#include <stddef.h>


char *StringCopy(char *str);
size_t ZTArraySize(void **arr);

#define ZTAS(array) ZTArraySize((void **) array)
