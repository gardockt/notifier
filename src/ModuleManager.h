#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "Structures/Map.h"
#include "Displays/Display.h" // TODO: unncessary after removing Display argument

typedef struct {
	Map availableModules;
	Map activeModules;
} ModuleManager;

bool initModuleManager(ModuleManager* moduleManager);
void destroyModuleManager(ModuleManager* moduleManager);

bool enableModule(ModuleManager* moduleManager, char* moduleType, char* moduleCustomName, Map* config, Display* display);
bool disableModule(ModuleManager* moduleManager, char* moduleCustomName);

#endif // ifndef MODULEMANAGER_H
