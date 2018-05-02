#pragma once

#include <stddef.h>


extern int sys_write(unsigned int fd, const char *buf, size_t count);
extern int sys_exit(int error_code) __attribute__ ((noreturn));
