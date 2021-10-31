#include "StringOperations.h"
#include "Globals.h"
#include "Config.h"
#include "Log.h"
#include "Structures/SortedMap.h"
#include "FetchingModules/FetchingModule.h"
#include "FetchingModules/Extras/FetchingModuleUtilities.h"
#include "Displays/Display.h"
#include "ModuleManager.h"

// fetching modules
#include "FetchingModules/Github.h"
#include "FetchingModules/Isod.h"
#include "FetchingModules/Rss.h"
#include "FetchingModules/Twitch.h"

bool addModule(ModuleManager* moduleManager, void (*templateFunc)(FetchingModule*), char* name) {
	FetchingModule* template = malloc(sizeof *template);
	if(template == NULL) {
		return false;
	}
	memset(template, 0, sizeof *template);
	templateFunc(template);
	return sortedMapPut(&moduleManager->availableModules, name, template);
}

bool initModuleManager(ModuleManager* moduleManager) {
	if(!(sortedMapInit(&moduleManager->availableModules, sortedMapCompareFunctionStrcasecmp) &&
	     sortedMapInit(&moduleManager->activeModules, sortedMapCompareFunctionStrcmp))) {
		return false;
	}

	if(
#ifdef ENABLE_MODULE_GITHUB
	   !addModule(moduleManager, githubTemplate,    "github") ||
#endif
#ifdef ENABLE_MODULE_ISOD
	   !addModule(moduleManager, isodTemplate,      "isod") ||
#endif
#ifdef ENABLE_MODULE_RSS
	   !addModule(moduleManager, rssTemplate,       "rss") ||
#endif
#ifdef ENABLE_MODULE_TWITCH
	   !addModule(moduleManager, twitchTemplate,    "twitch") ||
#endif
	   false) {
		return false;
	}

	return true;
}

void destroyModuleManager(ModuleManager* moduleManager) {
	SortedMap mapToFree;
	int mapToFreeSize;
	char** keysToFree;

	mapToFree = moduleManager->availableModules;
	mapToFreeSize = sortedMapSize(&mapToFree);
	keysToFree = malloc(mapToFreeSize * sizeof *keysToFree);
	sortedMapKeys(&mapToFree, (void**)keysToFree);
	for(int i = 0; i < mapToFreeSize; i++) {
		free(sortedMapGet(&mapToFree, keysToFree[i]));
	}
	free(keysToFree);

	mapToFree = moduleManager->activeModules;
	mapToFreeSize = sortedMapSize(&mapToFree);
	keysToFree = malloc(mapToFreeSize * sizeof *keysToFree);
	sortedMapKeys(&mapToFree, (void**)keysToFree);
	for(int i = 0; i < mapToFreeSize; i++) {
		disableModule(moduleManager, keysToFree[i]);
	}
	free(keysToFree);

	sortedMapDestroy(&moduleManager->availableModules);
	sortedMapDestroy(&moduleManager->activeModules);
}

bool moduleLoadBasicSettings(FetchingModule* fetchingModule, SortedMap* config) {
	if(!configLoadString(config, "_name", &fetchingModule->name) ||
	   !moduleLoadIntFromConfigWithErrorMessage(fetchingModule, config, "interval", &fetchingModule->intervalSecs) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, config, "title", &fetchingModule->notificationTitle) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, config, "body", &fetchingModule->notificationBody)) {
		return false;
	}

	configLoadString(config, "icon", &fetchingModule->iconPath);
	configLoadInt(config, "verbosity", &fetchingModule->verbosity);

	char* displayName = sortedMapGet(config, "display");
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

bool enableModule(ModuleManager* moduleManager, char* moduleType, char* moduleCustomName, SortedMap* config) {
	if(moduleManager == NULL || moduleType == NULL || moduleCustomName == NULL || config == NULL) {
		return false;
	}

	char* moduleTypeLowerCase = toLowerCase(moduleType);

	FetchingModule* moduleTemplate = sortedMapGet(&moduleManager->availableModules, moduleTypeLowerCase);
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
		sortedMapPut(&moduleManager->activeModules, moduleCustomName, module);
	}
	return enableModuleSuccess;
}

bool disableModule(ModuleManager* moduleManager, char* moduleCustomName) {
	FetchingModule* module = sortedMapGet(&moduleManager->activeModules, moduleCustomName);
	char* moduleCustomNameCopy = strdup(moduleCustomName);
	int moduleVerbosity = module->verbosity;

	if(module == NULL) {
		return false;
	}

	char* keyToFree;

	fetchingModuleDestroyThread(module);
	module->disable(module);
	moduleFreeBasicSettings(module);
	sortedMapRemove(&moduleManager->activeModules, moduleCustomName, (void**)&keyToFree, NULL); // value is already stored in "module"
	free(keyToFree);
	free(module);

	logWrite(moduleCustomNameCopy, moduleVerbosity, 1, "Module disabled");
	free(moduleCustomNameCopy);
	return true;
}
