#include "GlobalManagers.h"
#include "ModuleManager.h"
#include "DisplayManager.h"
#include "Displays/Display.h"
#include "FetchingModules/FetchingModule.h"
#include "Stash.h"
#include "Dirs.h"
#include "Main.h"

ModuleManager moduleManager;
DisplayManager displayManager;

void configFillEmptyFields(Map* targetConfig, Map* sourceConfig) {
	if(targetConfig == NULL || sourceConfig == NULL) {
		return;
	}

	int targetKeyCount = getMapSize(targetConfig);
	int sourceKeyCount = getMapSize(sourceConfig);
	char** targetKeys = malloc(targetKeyCount * sizeof *targetKeys);
	char** sourceKeys = malloc(sourceKeyCount * sizeof *sourceKeys);

	getMapKeys(targetConfig, (void**)targetKeys);
	getMapKeys(sourceConfig, (void**)sourceKeys);

	for(int i = 0; i < sourceKeyCount; i++) {
		for(int j = 0; j <= targetKeyCount; j++) {
			if(j < targetKeyCount) {
				if(!strcmp(sourceKeys[i], targetKeys[j])) {
					break;
				}
			} else {
				char* value = getFromMap(sourceConfig, sourceKeys[i], strlen(sourceKeys[i]));
				putIntoMap(targetConfig, strdup(sourceKeys[i]), strlen(sourceKeys[i]), strdup(value));
			}
		}
	}

	free(targetKeys);
	free(sourceKeys);
}

Map* configLoadSection(dictionary* config, char* sectionName) {
	int keyCount = iniparser_getsecnkeys(config, sectionName);
	if(keyCount == 0) {
		return NULL;
	}

	const char** keys = malloc(keyCount * sizeof *keys);
	iniparser_getseckeys(config, sectionName, keys);

	Map* ret = malloc(sizeof *ret);
	initMap(ret);

	putIntoMap(ret, strdup(CONFIG_NAME_FIELD_NAME), strlen(CONFIG_NAME_FIELD_NAME), strdup(sectionName));

	for(int i = 0; i < keyCount; i++) {
		if(keys[i][strlen(sectionName) + 1] == '_') {
			fprintf(stderr, "Option %s contains illegal prefix, ignoring\n", keys[i]);
			continue;
		}

		char* keyTrimmed = strdup(keys[i] + strlen(sectionName) + 1);
		const char* value = iniparser_getstring(config, keys[i], NULL);
		putIntoMap(ret, keyTrimmed, strlen(keyTrimmed), strdup(value));
	}

	free(keys);
	return ret;
}

void configDestroySection(Map* section) {
	int keyCount = getMapSize(section);
	char** keys = malloc(keyCount * sizeof *keys);
	getMapKeys(section, (void**)keys);

	for(int i = 0; i < keyCount; i++) {
		char* valueToFree;
		removeFromMap(section, keys[i], strlen(keys[i]), NULL, (void**)&valueToFree);
		free(valueToFree);
		free(keys[i]);
	}

	free(keys);
	destroyMap(section);
}

bool loadConfig() {
	dictionary* config;
	char* configDirectory = getConfigDirectory();
	config = iniparser_load(configDirectory);
	free(configDirectory);

	if(config == NULL) {
		return false;
	}

	int configSectionCount = iniparser_getnsec(config);
	int keyCount;

	Map* specialSections = malloc(sizeof *specialSections); // map of section maps
	initMap(specialSections);

	// special sections loading (they HAVE to be loaded first)
	for(int i = 0; i < configSectionCount; i++) {
		char* sectionName;

		sectionName = strdup(iniparser_getsecname(config, i));
		if(sectionName[0] != '_') {
			free(sectionName);
			continue;
		}

		Map* specialSection = configLoadSection(config, sectionName);
		putIntoMap(specialSections, sectionName, strlen(sectionName), specialSection);
	}

	// module sections loading
	for(int i = 0; i < configSectionCount; i++) {
		char* sectionName;
		Map* configMap;
		char* moduleType;

		sectionName = strdup(iniparser_getsecname(config, i));
		if(sectionName[0] == '_') {
			free(sectionName);
			continue;
		}

		configMap = configLoadSection(config, sectionName);
		moduleType = getFromMap(configMap, CONFIG_TYPE_FIELD_NAME, strlen(CONFIG_TYPE_FIELD_NAME));

		// add global settings for undefined values
		// priority: globalSection > global
		Map* configToMerge;

		// global section config
		char* globalSectionConfigSectionName = malloc(strlen(CONFIG_GLOBAL_SECTION_NAME) + strlen(CONFIG_NAME_SEPARATOR) + strlen(moduleType) + 1);
		sprintf(globalSectionConfigSectionName, "%s%s%s", CONFIG_GLOBAL_SECTION_NAME, CONFIG_NAME_SEPARATOR, moduleType);
		configToMerge = getFromMap(specialSections, globalSectionConfigSectionName, strlen(globalSectionConfigSectionName));
		free(globalSectionConfigSectionName);
		configFillEmptyFields(configMap, configToMerge);

		// global config
		configToMerge = getFromMap(specialSections, CONFIG_GLOBAL_SECTION_NAME, strlen(CONFIG_GLOBAL_SECTION_NAME));
		configFillEmptyFields(configMap, configToMerge);

		if(!enableModule(&moduleManager, moduleType, sectionName, configMap)) {
			fprintf(stderr, "Error while enabling module %s\n", sectionName);
		}

		configDestroySection(configMap);
		free(configMap);
	}

	int specialSectionCount = getMapSize(specialSections);
	char** specialSectionNames = malloc(specialSectionCount * sizeof *specialSectionNames);
	getMapKeys(specialSections, (void**)specialSectionNames);
	for(int i = 0; i < specialSectionCount; i++) {
		Map* sectionToFree;
		removeFromMap(specialSections, specialSectionNames[i], strlen(specialSectionNames[i]), NULL, (void**)&sectionToFree);
		configDestroySection(sectionToFree);
		free(sectionToFree);
		free(specialSectionNames[i]);
	}
	free(specialSectionNames);
	destroyMap(specialSections);
	free(specialSections);

	iniparser_freedict(config);
	return true;
}

bool initFunctionality() {
	if(!initModuleManager(&moduleManager)) {
		fprintf(stderr, "Error initializing module manager!\n");
		return false;
	}

	if(!initDisplayManager(&displayManager)) {
		fprintf(stderr, "Error initializing display manager!\n");
		return false;
	}

	if(!stashInit()) {
		fprintf(stderr, "Error loading stash file!\n");
		return false;
	}

#ifdef REQUIRED_CURL
	if(!curl_global_init(CURL_GLOBAL_DEFAULT)) {
		fprintf(stderr, "Error initializing CURL library!\n");
		return false;
	}
#endif

#ifdef REQUIRED_LIBXML
	LIBXML_TEST_VERSION;
#endif

	if(!loadConfig()) {
		fprintf(stderr, "Error loading config file!\n");
		return false;
	}

	return true;
}

void destroyFunctionality(int signal) {
	destroyModuleManager(&moduleManager);
	destroyDisplayManager(&displayManager);
	stashDestroy();

#ifdef REQUIRED_CURL
	curl_global_cleanup();
#endif

#ifdef REQUIRED_LIBXML
	xmlCleanupParser();
#endif
}

int main() {
	struct sigaction signalMgmt;
	signalMgmt.sa_handler = destroyFunctionality;
	sigemptyset(&signalMgmt.sa_mask);
	signalMgmt.sa_flags = 0;
	sigaction(SIGINT, &signalMgmt, NULL);

	if(!initFunctionality()) {
		return 1;
	}

	pause();

	return 0;
}
