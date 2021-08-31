#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>

typedef enum {
	URL
} NotificationActionType;

typedef struct {
	char* title;
	char* text;
	char* actionData;
	NotificationActionType actionType;
} Message;

typedef struct {
	bool (*init)();
	bool (*displayMessage)(Message* message);
	void (*uninit)();
} Display;

#endif // ifndef DISPLAY_H
