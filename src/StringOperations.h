#ifndef STRINGOPERATIONS_H
#define STRINGOPERATIONS_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

int split(char* text, char* separators, char*** output);
char* replace(char* input, int pairCount, ...);
char* toLowerCase(char* text);

#endif // ifndef STRINGOPERATIONS_H
