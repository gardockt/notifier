#include "../../Displays/Display.h"
#include "../../Config.h"
#include "FetchingModuleUtilities.h"

#define STRDUP_IF_NOT_NULL(x) ((x) != NULL ? strdup(x) : NULL)

bool moduleLoadIntFromConfigWithErrorMessage(FetchingModule* fetchingModule, Map* config, char* key, int* output) {
	bool success = configLoadInt(config, key, output);
	if(!success) {
		moduleLog(fetchingModule, 0, "Invalid %s", key);
	}
	return success;
}

bool moduleLoadStringFromConfigWithErrorMessage(FetchingModule* fetchingModule, Map* config, char* key, char** output) {
	bool success = configLoadString(config, key, output);
	if(!success) {
		moduleLog(fetchingModule, 0, "Invalid %s", key);
	}
	return success;
}

void moduleFillBasicMessage(FetchingModule* fetchingModule, Message* message, char* (*textEditingFunction)(char*, void*), void* textEditingFunctionArg, NotificationActionType defaultActionType, char* defaultActionData) {
	message->title = textEditingFunction(fetchingModule->notificationTitle, textEditingFunctionArg);
	message->body = textEditingFunction(fetchingModule->notificationBody, textEditingFunctionArg);
	message->actionType = defaultActionType;
	message->actionData = STRDUP_IF_NOT_NULL(defaultActionData);
	message->iconPath = fetchingModule->iconPath;
}

void moduleDestroyBasicMessage(Message* message) {
	free(message->title);
	free(message->body);
	free(message->actionData);
}

// TODO: make it a wrapper for logWrite
void moduleLog(FetchingModule* fetchingModule, int verbosity, char* format, ...) {
	if(fetchingModule->verbosity < verbosity) {
		return;
	}

	char* customFormat = malloc(strlen(fetchingModule->name) + strlen("[] \n") + strlen(format) + 1);
	sprintf(customFormat, "[%s] %s\n", fetchingModule->name, format);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, customFormat, args);
	va_end(args);

	free(customFormat);
}
