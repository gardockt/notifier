#ifndef FETCHING_MODULE_UTILITIES_H
#define FETCHING_MODULE_UTILITIES_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../../Structures/SortedMap.h"
#include "../FetchingModule.h"

typedef struct {
	char* title;
	char* body;
} ModuleFillBasicMessageOptArgs;

bool moduleLoadIntFromConfigWithErrorMessage(FetchingModule* fetchingModule, SortedMap* config, char* key, int* output);
bool moduleLoadStringFromConfigWithErrorMessage(FetchingModule* fetchingModule, SortedMap* config, char* key, char** output);

bool moduleLoadBasicSettings(FetchingModule* fetchingModule, SortedMap* config);
void moduleFreeBasicSettings(FetchingModule* fetchingModule);

void moduleLog(FetchingModule* fetchingModule, int verbosity, char* format, ...);

#endif // ifndef FETCHING_MODULE_UTILITIES_H
