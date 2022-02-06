#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdarg.h>

#include "Structures/SortedMap.h"

typedef struct {
	SortedMap available_modules;
	SortedMap active_modules;
} ModuleManager;

bool fm_manager_init(ModuleManager* manager);
void fm_manager_destroy(ModuleManager* manager);

bool fm_enable(ModuleManager* manager, char* type, char* custom_name, SortedMap* config);
bool fm_disable(ModuleManager* manager, char* custom_name);

#endif /* ifndef MODULE_MANAGER_H */
