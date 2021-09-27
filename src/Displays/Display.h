#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	URL
} NotificationActionType;

typedef struct {
	char* title;
	char* body;
	char* actionData;
	NotificationActionType actionType;
	char* iconPath;
} Message;

typedef struct {
	bool (*init)();
	bool (*displayMessage)(Message* message);
	void (*uninit)();
} Display;

Message* messageClone(Message* message);
void messageFreeAllChildren(Message* message);

#endif // ifndef DISPLAY_H
