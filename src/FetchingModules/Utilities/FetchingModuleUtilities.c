#include "../../Displays/Display.h"
#include "../../Config.h"
#include "../../Log.h"
#include "FetchingModuleUtilities.h"

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

void moduleFillBasicMessage(FetchingModule* fetchingModule, Message* message, char* (*textEditingFunction)(char*, void*), void* textEditingFunctionArg, ModuleFillBasicMessageOptArgs* optArgs) {
	message->title = textEditingFunction(optArgs != NULL && optArgs->title != NULL ?
			optArgs->title : fetchingModule->notification_title, textEditingFunctionArg);
	message->body = textEditingFunction(optArgs != NULL && optArgs->body != NULL ?
			optArgs->body : fetchingModule->notification_body, textEditingFunctionArg);
	message->iconPath = fetchingModule->icon_path;
}

void moduleDestroyBasicMessage(Message* message) {
	free(message->title);
	free(message->body);
}

void moduleLog(FetchingModule* fetchingModule, int verbosity, char* format, ...) {
	va_list args;
	va_start(args, format);
	logWriteVararg(fetchingModule->name, fetchingModule->verbosity, verbosity, format, args);
	va_end(args);
}
