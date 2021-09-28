#include "Display.h"

#define STRDUP_IF_NOT_NULL(str) ((str) != NULL ? strdup(str) : NULL)

Message* messageClone(Message* message) {
	Message* ret = malloc(sizeof *ret);
	ret->title       = STRDUP_IF_NOT_NULL(message->title);
	ret->body        = STRDUP_IF_NOT_NULL(message->body);
	ret->actionData  = STRDUP_IF_NOT_NULL(message->actionData);
	ret->iconPath    = STRDUP_IF_NOT_NULL(message->iconPath);
	ret->actionType  = message->actionType;
	return ret;
}

void messageFreeAllChildren(Message* message) {
	free(message->title);
	free(message->body);
	free(message->actionData);
	free(message->iconPath);
}
