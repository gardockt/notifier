#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void log_write_vararg(const char* section_name, int desired_verbosity, int verbosity, const char* format, va_list args);
void log_write(const char* section_name, int desired_verbosity, int verbosity, const char* format, ...);

#endif /* ifndef LOG_H */
