#pragma once

#include <stddef.h>


extern int write(
    unsigned int fd,
    const char *buf,
    size_t count);

size_t strlen(char *str);
int print(char *str);

int puts (const char *str);
int printf (const char *format, ... );

void i2str(int i, char *buffer);

int putchar (int character);
#define printfln(format, ...) printf(format, __VA_ARGS__); printf("\n");
