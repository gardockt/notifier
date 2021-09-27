#ifndef FETCHING_MODULE_UTILITIES_H
#define FETCHING_MODULE_UTILITIES_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "../../Structures/Map.h"
#include "../FetchingModule.h"

#define FETCHING_MODULE_LIST_ENTRY_SEPARATORS " \t\r\n"

bool moduleLoadIntFromConfig(FetchingModule* fetchingModule, Map* config, char* key, int* output);
bool moduleLoadStringFromConfig(FetchingModule* fetchingModule, Map* config, char* key, char** output);

bool moduleLoadIntFromConfigWithErrorMessage(FetchingModule* fetchingModule, Map* config, char* key, int* output);
bool moduleLoadStringFromConfigWithErrorMessage(FetchingModule* fetchingModule, Map* config, char* key, char** output);

bool moduleLoadBasicSettings(FetchingModule* fetchingModule, Map* config);
void moduleFreeBasicSettings(FetchingModule* fetchingModule);

void moduleFillBasicMessage(FetchingModule* fetchingModule, Message* message, char* (*textEditingFunction)(char*, void*), void* textEditingFunctionArg);
void moduleDestroyBasicMessage(Message* message);

void moduleLogCustom(char* sectionName, int desiredVerbosity, int verbosity, char* format, ...);
void moduleLog(FetchingModule* fetchingModule, int verbosity, char* format, ...);

#endif // ifndef FETCHING_MODULE_UTILITIES_H
