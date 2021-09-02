#ifndef FETCHING_MODULE_UTILITIES_H
#define FETCHING_MODULE_UTILITIES_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../../Structures/Map.h"
#include "../FetchingModule.h"

bool moduleLoadIntFromConfig(FetchingModule* fetchingModule, Map* config, char* key, int* output);
bool moduleLoadStringFromConfig(FetchingModule* fetchingModule, Map* config, char* key, char** output);

bool moduleLoadBasicSettings(FetchingModule* fetchingModule, Map* config);
void moduleFreeBasicSettings(FetchingModule* fetchingModule);

#endif // ifndef FETCHING_MODULE_UTILITIES_H
