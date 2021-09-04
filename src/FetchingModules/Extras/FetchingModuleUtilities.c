#include "../../GlobalManagers.h"
#include "../../Displays/Display.h"
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

// loads fetchingModule variables
bool moduleLoadBasicSettings(FetchingModule* fetchingModule, Map* config) {
	if(!moduleLoadIntFromConfig(fetchingModule, config, "interval", &fetchingModule->intervalSecs) ||
	   !moduleLoadStringFromConfig(fetchingModule, config, "title", &fetchingModule->notificationTitle) ||
	   !moduleLoadStringFromConfig(fetchingModule, config, "body", &fetchingModule->notificationBody) ||
	   !moduleLoadStringFromConfig(fetchingModule, config, "_name", &fetchingModule->name)) {
		return false;
	}

	moduleLoadStringFromConfig(fetchingModule, config, "icon", &fetchingModule->iconPath);

	char* displayName = getFromMap(config, "display", strlen("display"));
	if(displayName == NULL) {
		return false;
	}
	fetchingModule->display = getDisplay(&displayManager, displayName);
	if(fetchingModule->display == NULL) {
		return false;
	}

	return true;
}

void moduleFreeBasicSettings(FetchingModule* fetchingModule) {
	free(fetchingModule->notificationTitle);
	free(fetchingModule->notificationBody);
	free(fetchingModule->name);
}

void moduleFillBasicMessage(FetchingModule* fetchingModule, Message* message, char* (*textEditingFunction)(char*, void*), void* textEditingFunctionArg) {
	message->title = textEditingFunction(fetchingModule->notificationTitle, textEditingFunctionArg);
	message->body = textEditingFunction(fetchingModule->notificationBody, textEditingFunctionArg);
	message->iconPath = fetchingModule->iconPath;
}

void moduleLog(FetchingModule* fetchingModule, int verbosity, char* format, ...) {
	// TODO: implement verbosity
	char* customFormat = malloc(strlen(fetchingModule->name) + strlen("[] \n") + strlen(format) + 1);
	sprintf(customFormat, "[%s] %s\n", fetchingModule->name, format);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, customFormat, args);
	va_end(args);

	free(customFormat);
}
