#ifndef STASH_H
#define STASH_H

#include <iniparser.h>
#include <dictionary.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

bool stashInit();
void stashDestroy();

bool stashSetString(char* section, char* key, char* value);
bool stashSetInt(char* section, char* key, int value);

char* stashGetString(char* section, char* key, char* defaultValue);
int stashGetInt(char* section, char* key, int defaultValue);

#endif // ifndef STASH_H
