#include "Display.h"

void defaultMessageFreeFunction(Message* message) {
	free(message->title);
	free(message->text);
	free(message->actionData);
	free(message);
}
