#include "StringOperations.h"
#include "ModuleManager.h"
#include "Structures/Map.h"
#include "FetchingModules/FetchingModule.h"
#include "Displays/Display.h"

// fetching modules
#include "FetchingModules/Github.h"
#include "FetchingModules/Isod.h"
#include "FetchingModules/Twitch.h"

#define ADDMODULE(templateFunc,name) ((template=malloc(sizeof *template)) != NULL && templateFunc(template) && putIntoMap(&moduleManager->availableModules, name, strlen(name), template))

bool initModuleManager(ModuleManager* moduleManager) {
	FetchingModule* template;

	if(!(initMap(&moduleManager->availableModules) &&
	     initMap(&moduleManager->activeModules))) {
		return false;
	}

	// adding modules to template map; enter names lower-case
	if(!ADDMODULE(githubTemplate, "github") ||
	   !ADDMODULE(isodTemplate, "isod") ||
	   !ADDMODULE(twitchTemplate, "twitch")) {
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
	getMapKeys(&mapToFree, (void**)keysToFree);
	for(int i = 0; i < mapToFreeSize; i++) {
		free(getFromMap(&mapToFree, keysToFree[i], strlen(keysToFree[i])));
	}
	free(keysToFree);

	mapToFree = moduleManager->activeModules;
	mapToFreeSize = getMapSize(&mapToFree);
	keysToFree = malloc(mapToFreeSize * sizeof *keysToFree);
	getMapKeys(&mapToFree, (void**)keysToFree);
	for(int i = 0; i < mapToFreeSize; i++) {
		disableModule(moduleManager, keysToFree[i]);
	}
	free(keysToFree);

	destroyMap(&moduleManager->availableModules);
	destroyMap(&moduleManager->activeModules);
}

bool enableModule(ModuleManager* moduleManager, char* moduleType, char* moduleCustomName, Map* config) {
	if(moduleManager == NULL || moduleType == NULL || moduleCustomName == NULL || config == NULL) {
		return false;
	}

	char* moduleTypeLowerCase = toLowerCase(moduleType);

	FetchingModule* moduleTemplate = getFromMap(&moduleManager->availableModules, moduleTypeLowerCase, strlen(moduleTypeLowerCase));
	FetchingModule* module;

	free(moduleTypeLowerCase);

	if(moduleTemplate == NULL) {
		return false;
	}

	module = malloc(sizeof *module);
	if(module == NULL) {
		return false;
	}

	memcpy(module, moduleTemplate, sizeof *module);
	bool parseConfigSuccess = module->parseConfig(module, config);

	int keyCount = getMapSize(config);
	char** keys = malloc(keyCount * sizeof *keys);
	getMapKeys(config, (void**)keys);

	for(int i = 0; i < keyCount; i++) {
		char* valueToFree;
		removeFromMap(config, keys[i], strlen(keys[i]), NULL, (void**)&valueToFree); // key is already in keys[i]
		free(valueToFree);
		free(keys[i]);
	}

	free(keys);
	destroyMap(config);

	if(parseConfigSuccess) {
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
	removeFromMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName), (void**)&keyToFree, NULL); // value is already stored in "module"
	free(keyToFree);
	free(module);

	return true;
}
