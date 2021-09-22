#include "FetchingModuleUtilities.h"

bool moduleLoadIntFromConfig(FetchingModule* fetchingModule, Map* config, char* key, int* output) {
	char* rawValueFromMap = getFromMap(config, key, strlen(key));
	if(rawValueFromMap == NULL) {
		return false;
	}
	*output = atoi(rawValueFromMap);
	return true;
}

bool moduleLoadStringFromConfig(FetchingModule* fetchingModule, Map* config, char* key, char** output) {
	char* rawValueFromMap = getFromMap(config, key, strlen(key));
	if(rawValueFromMap == NULL) {
		return false;
	}
	*output = strdup(rawValueFromMap);
	return true;
}

bool moduleLoadIntFromConfigWithErrorMessage(FetchingModule* fetchingModule, Map* config, char* key, int* output) {
	bool success = moduleLoadIntFromConfig(fetchingModule, config, key, output);
	if(!success) {
		moduleLog(fetchingModule, 0, "Invalid %s", key);
	}
	return success;
}

bool moduleLoadStringFromConfigWithErrorMessage(FetchingModule* fetchingModule, Map* config, char* key, char** output) {
	bool success = moduleLoadStringFromConfig(fetchingModule, config, key, output);
	if(!success) {
		moduleLog(fetchingModule, 0, "Invalid %s", key);
	}
	return success;
}

void moduleFillBasicMessage(FetchingModule* fetchingModule, Message* message, char* (*textEditingFunction)(char*, void*), void* textEditingFunctionArg) {
	message->title = textEditingFunction(fetchingModule->notificationTitle, textEditingFunctionArg);
	message->body = textEditingFunction(fetchingModule->notificationBody, textEditingFunctionArg);
	message->iconPath = fetchingModule->iconPath;
}

void moduleLogCustom(char* sectionName, int desiredVerbosity, int verbosity, char* format, ...) {
	if(desiredVerbosity < verbosity) {
		return;
	}

	char* customFormat = malloc(strlen(sectionName) + strlen("[] \n") + strlen(format) + 1);
	sprintf(customFormat, "[%s] %s\n", sectionName, format);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, customFormat, args);
	va_end(args);

	free(customFormat);
}

void moduleLog(FetchingModule* fetchingModule, int verbosity, char* format, ...) {
	va_list args;
	va_start(args, format);
	moduleLogCustom(fetchingModule->name, fetchingModule->verbosity, verbosity, format, args);
	va_end(args);
}
