#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>

typedef struct {
	char* title;
	char* text;
} Message;

typedef struct {
	bool (*init)();
	bool (*displayMessage)(Message* message);
	void (*uninit)();
} Display;

#endif // ifndef DISPLAY_H
