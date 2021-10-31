#include "../../Displays/Display.h"
#include "../../Config.h"
#include "../../Log.h"
#include "FetchingModuleUtilities.h"

#define STRDUP_IF_NOT_NULL(x) ((x) != NULL ? strdup(x) : NULL)

bool moduleLoadIntFromConfigWithErrorMessage(FetchingModule* fetchingModule, SortedMap* config, char* key, int* output) {
	bool success = configLoadInt(config, key, output);
	if(!success) {
		moduleLog(fetchingModule, 0, "Invalid %s", key);
	}
	return success;
}

bool moduleLoadStringFromConfigWithErrorMessage(FetchingModule* fetchingModule, SortedMap* config, char* key, char** output) {
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

void moduleLog(FetchingModule* fetchingModule, int verbosity, char* format, ...) {
	va_list args;
	va_start(args, format);
	logWriteVararg(fetchingModule->name, fetchingModule->verbosity, verbosity, format, args);
	va_end(args);
}
