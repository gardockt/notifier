#include "ModuleManager.h"
#include "Structures/Map.h"
#include "FetchingModules/FetchingModule.h"

// fetching modules
#include "FetchingModules/HelloWorld.h"

bool initModuleManager(ModuleManager* moduleManager) {
	FetchingModule* template = malloc(sizeof *template);

	if(!(template != NULL &&
	     initMap(&moduleManager->availableModules) &&
	     initMap(&moduleManager->activeModules))) {
		return false;
	}

	// adding modules to template map
	if(!(helloWorldTemplate(template) && putIntoMap(&moduleManager->availableModules, "HelloWorld", strlen("HelloWorld"), template))) {
		return false;
	}

	return true;
}

void destroyModuleManager(ModuleManager* moduleManager) {
	Map mapToFree;

	mapToFree = moduleManager->availableModules;
	for(int i = 0; i < mapToFree.size; i++) {
		free(mapToFree.elements[i].value);
	}

	mapToFree = moduleManager->activeModules;
	for(int i = 0; i < mapToFree.size; i++) {
		char* moduleName = (char*)(mapToFree.elements[i].key);
		disableModule(moduleManager, moduleName);
	}

	destroyMap(&moduleManager->availableModules);
	destroyMap(&moduleManager->activeModules);
}

bool enableModule(ModuleManager* moduleManager, char* moduleType, char* moduleCustomName, Map* config) {
	if(moduleManager == NULL || moduleType == NULL || moduleCustomName == NULL || config == NULL) {
		return false;
	}

	FetchingModule* moduleTemplate = getFromMap(&moduleManager->availableModules, moduleType, strlen(moduleType));
	FetchingModule* module;
	if(moduleTemplate == NULL) {
		return false;
	}

	module = malloc(sizeof *module);
	if(module == NULL) {
		return false;
	}

	memcpy(module, moduleTemplate, sizeof *module);
	module->parseConfig(module, config);
	module->enable(module);
	putIntoMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName), module);

	return true;
}

bool disableModule(ModuleManager* moduleManager, char* moduleCustomName) {
	FetchingModule* module = getFromMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName));

	if(module == NULL) {
		return false;
	}

	module->disable(module);
	removeFromMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName), NULL, NULL);
	free(module);

	return true;
}
