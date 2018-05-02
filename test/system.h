#pragma once

#include <stddef.h>


extern int sys_write(unsigned int fd, const char *buf, size_t count);
extern int sys_exit(int error_code) __attribute__ ((noreturn));

void exit (int status) __attribute__ ((noreturn));

size_t strlen(char *str);
int print(char *str);

int puts(const char *str);
int printf(const char *format, ... );

void i2str(int i, char *buffer);

int putchar (int character);
#define printfln(format, ...) printf(format, __VA_ARGS__); printf("\n");
