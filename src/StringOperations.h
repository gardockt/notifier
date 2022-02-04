#ifndef STRING_OPERATIONS_H
#define STRING_OPERATIONS_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

int split(const char* text, const char* separators, char*** output);
char* replace(const char* input, int pairCount, ...);

#endif /* ifndef STRING_OPERATIONS_H */
