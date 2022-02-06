#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <iniparser.h>

#include "Structures/SortedMap.h"

bool config_load_int(SortedMap* config, char* key, int* output);
bool config_load_string(SortedMap* config, char* key, char** output);

void config_load_core();
bool config_load();

#endif /* ifndef CONFIG_H */
