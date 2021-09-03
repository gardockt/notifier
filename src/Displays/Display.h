#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
	URL
} NotificationActionType;

typedef struct {
	char* title;
	char* body;
	char* actionData;
	NotificationActionType actionType;
} Message;

typedef struct {
	bool (*init)();
	bool (*displayMessage)(Message* message, void (*freeFunction)(Message*));
	void (*uninit)();
} Display;

void defaultMessageFreeFunction(Message* message);

#endif // ifndef DISPLAY_H
