#ifndef STASH_H
#define STASH_H

#include <iniparser.h>
#include <dictionary.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

bool stash_init();
void stash_destroy();

bool stash_set_string(const char* section, const char* key, const char* value);
bool stash_set_int(const char* section, const char* key, int value);

const char* stash_get_string(const char* section, const char* key, const char* default_value);
int stash_get_int(const char* section, const char* key, int default_value);

#endif /* ifndef STASH_H */
