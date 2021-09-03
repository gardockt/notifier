#include "Display.h"

void defaultMessageFreeFunction(Message* message) {
	free(message->title);
	free(message->body);
	free(message->actionData);
	free(message);
}
