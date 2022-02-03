#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void logWriteVararg(const char* sectionName, int desiredVerbosity, int verbosity, const char* format, va_list args);
void logWrite(const char* sectionName, int desiredVerbosity, int verbosity, const char* format, ...);

#endif // ifndef LOG_H
