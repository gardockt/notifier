#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <iniparser.h>

#include "Structures/SortedMap.h"

bool configLoadInt(SortedMap* config, char* key, int* output);
bool configLoadString(SortedMap* config, char* key, char** output);

void configLoadCore();
bool configLoad();

#endif // ifndef CONFIG_H
