#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../Message.h"

typedef struct {
	bool (*init)();
	bool (*display_message)(const Message* message);
	void (*uninit)();
} Display;

#endif /* ifndef DISPLAY_H */
