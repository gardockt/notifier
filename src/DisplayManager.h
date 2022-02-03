#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "Structures/SortedMap.h"
#include "Displays/Display.h"

typedef struct {
	SortedMap displays;
} DisplayManager;

bool display_manager_init(DisplayManager* manager);
void display_manager_destroy(DisplayManager* manager);

Display* display_manager_get_display(DisplayManager* manager, const char* name);

#endif /* ifndef DISPLAY_MANAGER_H */
