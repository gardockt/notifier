#include "ModuleManager.h"
#include "Structures/Map.h"
#include "FetchingModules/FetchingModule.h"
#include "Displays/Display.h"

// fetching modules
#include "FetchingModules/Github.h"
#include "FetchingModules/HelloWorld.h"
#include "FetchingModules/Twitch.h"

#define ADDMODULE(templateFunc,name) ((template=malloc(sizeof *template)) != NULL && templateFunc(template) && putIntoMap(&moduleManager->availableModules, name, strlen(name), template))

bool initModuleManager(ModuleManager* moduleManager) {
	FetchingModule* template;

	if(!(initMap(&moduleManager->availableModules) &&
	     initMap(&moduleManager->activeModules))) {
		return false;
	}

	// adding modules to template map
	if(!ADDMODULE(helloWorldTemplate, "HelloWorld") ||
	   !ADDMODULE(githubTemplate, "Github") ||
	   !ADDMODULE(twitchTemplate, "Twitch")) {
		return false;
	}

	return true;
}

void destroyModuleManager(ModuleManager* moduleManager) {
	Map mapToFree;
	int mapToFreeSize;
	char** keysToFree;

	mapToFree = moduleManager->availableModules;
	mapToFreeSize = getMapSize(&mapToFree);
	keysToFree = malloc(mapToFreeSize * sizeof *keysToFree);
	getMapKeys(&mapToFree, keysToFree);
	for(int i = 0; i < mapToFreeSize; i++) {
		free(getFromMap(&mapToFree, keysToFree[i], strlen(keysToFree[i])));
	}
	free(keysToFree);

	mapToFree = moduleManager->activeModules;
	mapToFreeSize = getMapSize(&mapToFree);
	keysToFree = malloc(mapToFreeSize * sizeof *keysToFree);
	getMapKeys(&mapToFree, keysToFree);
	for(int i = 0; i < mapToFreeSize; i++) {
		disableModule(moduleManager, keysToFree[i]);
	}
	free(keysToFree);

	destroyMap(&moduleManager->availableModules);
	destroyMap(&moduleManager->activeModules);
}

// TODO: passing display separately feels bad, fix with global display manager?
bool enableModule(ModuleManager* moduleManager, char* moduleType, char* moduleCustomName, Map* config, Display* display) {
	if(moduleManager == NULL || moduleType == NULL || moduleCustomName == NULL || config == NULL || display == NULL) {
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
	if(module->parseConfig(module, config)) {
		module->display = display;
		module->display->init();
		module->enable(module);
		putIntoMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName), module);
		return true;
	} else {
		return false;
	}
}

bool disableModule(ModuleManager* moduleManager, char* moduleCustomName) {
	FetchingModule* module = getFromMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName));

	if(module == NULL) {
		return false;
	}

	char* keyToFree;

	module->disable(module);
	module->display->uninit();
	removeFromMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName), &keyToFree, NULL); // value is already stored in "module"
	free(keyToFree);
	free(module);

	return true;
}
