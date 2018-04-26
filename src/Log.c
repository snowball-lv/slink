#include <slink/Log.h>

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define LOG_FILE    "log.txt"

void LogClear() {
    FILE *log = fopen(LOG_FILE, "w");
    assert(log);
    fclose(log);
}

void Log(char *tag, char *format, ...) {

    FILE *log = fopen(LOG_FILE, "a");
    assert(log);

    va_list args;
    va_start(args, format);

    fprintf(log, "[%s] ", tag);
    vfprintf(log, format, args);

    va_end(args);

    fclose(log);
}
