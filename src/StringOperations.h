#ifndef STRINGOPERATIONS_H
#define STRINGOPERATIONS_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

int split(char* text, char* separators, char*** output);
char* replace(char* text, char* before, char* after);
char* toLowerCase(char* text);

#endif // ifndef STRINGOPERATIONS_H
