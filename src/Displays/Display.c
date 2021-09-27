#include "Display.h"

Message* messageClone(Message* message) {
	Message* ret = malloc(sizeof *ret);
	ret->title       = strdup(message->title);
	ret->body        = strdup(message->body);
	ret->actionData  = strdup(message->actionData);
	ret->iconPath    = strdup(message->iconPath);
	ret->actionType  = message->actionType;
	return ret;
}

void messageFreeAllChildren(Message* message) {
	free(message->title);
	free(message->body);
	free(message->actionData);
	free(message->iconPath);
}
