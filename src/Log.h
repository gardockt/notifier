#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void logWrite(char* sectionName, int desiredVerbosity, int verbosity, char* format, ...);

#endif // ifndef LOG_H
