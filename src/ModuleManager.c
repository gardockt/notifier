#include "StringOperations.h"
#include "ModuleManager.h"
#include "GlobalManagers.h"
#include "Structures/Map.h"
#include "FetchingModules/FetchingModule.h"
#include "FetchingModules/Extras/FetchingModuleUtilities.h"
#include "Displays/Display.h"

// fetching modules
#include "FetchingModules/Github.h"
#include "FetchingModules/Isod.h"
#include "FetchingModules/Rss.h"
#include "FetchingModules/Twitch.h"

#define ADDMODULE(templateFunc,name) ((template = malloc(sizeof *template)) != NULL && memset(template, 0, sizeof *template) && templateFunc(template) && putIntoMap(&moduleManager->availableModules, name, strlen(name), template))

bool initModuleManager(ModuleManager* moduleManager) {
	FetchingModule* template;

	if(!(initMap(&moduleManager->availableModules) &&
	     initMap(&moduleManager->activeModules))) {
		return false;
	}

	// adding modules to template map; enter names lower-case
	if(
#ifdef ENABLE_MODULE_GITHUB
	   !ADDMODULE(githubTemplate,    "github") ||
#endif
#ifdef ENABLE_MODULE_ISOD
	   !ADDMODULE(isodTemplate,      "isod") ||
#endif
#ifdef ENABLE_MODULE_RSS
	   !ADDMODULE(rssTemplate,       "rss") ||
#endif
#ifdef ENABLE_MODULE_TWITCH
	   !ADDMODULE(twitchTemplate,    "twitch") ||
#endif
	   false) {
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

bool moduleLoadBasicSettings(FetchingModule* fetchingModule, Map* config) {
	if(!moduleLoadStringFromConfig(fetchingModule, config, "_name", &fetchingModule->name) ||
	   !moduleLoadIntFromConfigWithErrorMessage(fetchingModule, config, "interval", &fetchingModule->intervalSecs) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, config, "title", &fetchingModule->notificationTitle) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, config, "body", &fetchingModule->notificationBody)) {
		return false;
	}

	moduleLoadStringFromConfig(fetchingModule, config, "icon", &fetchingModule->iconPath);
	moduleLoadIntFromConfig(fetchingModule, config, "verbosity", &fetchingModule->verbosity);

	char* displayName = getFromMap(config, "display", strlen("display"));
	if(displayName == NULL) {
		moduleLog(fetchingModule, 0, "Invalid display");
		return false;
	}
	fetchingModule->display = getDisplay(&displayManager, displayName);
	if(fetchingModule->display == NULL) {
		moduleLog(fetchingModule, 0, "Display does not exist");
		return false;
	}
	if(!fetchingModule->display->init()) {
		moduleLog(fetchingModule, 0, "Failed to init display");
		return false;
	}

	return true;
}

void moduleFreeBasicSettings(FetchingModule* fetchingModule) {
	fetchingModule->display->uninit();
	free(fetchingModule->name);
	free(fetchingModule->notificationTitle);
	free(fetchingModule->notificationBody);
	free(fetchingModule->iconPath);
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
	bool enableModuleSuccess = moduleLoadBasicSettings(module, config) && module->enable(module, config) && fetchingModuleCreateThread(module);

	if(enableModuleSuccess) {
		moduleLog(module, 1, "Module enabled");
		putIntoMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName), module);
	}
	return enableModuleSuccess;
}

bool disableModule(ModuleManager* moduleManager, char* moduleCustomName) {
	FetchingModule* module = getFromMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName));
	char* moduleCustomNameCopy = strdup(moduleCustomName);
	int moduleVerbosity = module->verbosity;

	if(module == NULL) {
		return false;
	}

	char* keyToFree;

	fetchingModuleDestroyThread(module);
	module->disable(module);
	moduleFreeBasicSettings(module);
	removeFromMap(&moduleManager->activeModules, moduleCustomName, strlen(moduleCustomName), (void**)&keyToFree, NULL); // value is already stored in "module"
	free(keyToFree);
	free(module);

	moduleLogCustom(moduleCustomNameCopy, moduleVerbosity, 1, "Module disabled");
	free(moduleCustomNameCopy);
	return true;
}
