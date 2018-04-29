#pragma once

#include <stdio.h>
#include <stdlib.h>


#define ERROR(...)  {                                           \
    fprintf(stderr, __VA_ARGS__);                               \
    fprintf(stderr, "At %i of [%s]\n", __LINE__, __FILE__);     \
    exit(1);                                                    \
}
