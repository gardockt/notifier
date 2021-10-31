#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "Structures/SortedMap.h"
#include "Displays/Display.h"

typedef struct {
	SortedMap displays;
} DisplayManager;

bool initDisplayManager(DisplayManager* displayManager);
void destroyDisplayManager(DisplayManager* displayManager);

Display* getDisplay(DisplayManager* displayManager, char* displayName);

#endif // ifndef DISPLAYMANAGER_H
