#include "Main.h"
#include "ModuleManager.h"
#include "FetchingModules/HelloWorld.h"

ModuleManager moduleManager;

dictionary* config;

// TODO: fix memleaks

void onExit(int signal) {
	printf("Niszczenie moduleManager...\n");
	destroyModuleManager(&moduleManager);
}

bool loadConfig() {
	config = iniparser_load(CONFIG_FILE);
	return config != NULL;
}

void unloadConfig() {
	iniparser_freedict(config);
}

int main() {
	struct sigaction signalMgmt;
	signalMgmt.sa_handler = onExit;
	sigemptyset(&signalMgmt.sa_mask);
	signalMgmt.sa_flags = 0;
	if(sigaction(SIGINT, &signalMgmt, NULL) == -1) {
		// blad
	}

	initModuleManager(&moduleManager);

	if(!loadConfig()) {
		fprintf(stderr, "Error loading config file!\n");
		return 1;
	}

	int configSectionCount = iniparser_getnsec(config);
	int keyCount;

	int globalConfigKeyCount;
	char** globalConfigKeys;

	// global settings loading
	globalConfigKeyCount = iniparser_getsecnkeys(config, CONFIG_GLOBAL_SECTION_NAME);
	if(globalConfigKeyCount > 0) {
		globalConfigKeys = malloc(globalConfigKeyCount * sizeof *globalConfigKeys);
		iniparser_getseckeys(config, CONFIG_GLOBAL_SECTION_NAME, globalConfigKeys);
	}

	for(int i = 0; i < configSectionCount; i++) {
		char* sectionName;
		char** keys;
		Map* configMap;
		char* value;
		char* moduleType = NULL;

		char* sectionNameTemp = iniparser_getsecname(config, i);
		sectionName = malloc(strlen(sectionNameTemp) + 1);
		strcpy(sectionName, sectionNameTemp);

		// TODO: add global section config

		// non-global settings loading
		if(sectionName[0] == '_') {
			continue;
		}

		keyCount = iniparser_getsecnkeys(config, sectionName);

		configMap = malloc(sizeof *configMap);
		initMap(configMap);
		keys = malloc(keyCount * sizeof *keys);
		iniparser_getseckeys(config, sectionName, keys);

		for(int i = 0; i < keyCount; i++) {
			if(keys[i][strlen(sectionName) + 1] == '_') {
				fprintf(stderr, "Option %s contains illegal prefix, ignoring\n", keys[i]);
				continue;
			}

			char* keyTrimmed = malloc(strlen(keys[i]) - strlen(sectionName));
			strcpy(keyTrimmed, keys[i] + strlen(sectionName) + 1);

			char* valueTemp = iniparser_getstring(config, keys[i], NULL);
			value = malloc(strlen(valueTemp) + 1);
			strcpy(value, valueTemp);

			if(!strcmp(keyTrimmed, "module")) {
				moduleType = value;
				free(keyTrimmed);
			} else {
				putIntoMap(configMap, keyTrimmed, strlen(keyTrimmed), value);
			}
		}

		// add global settings for undefined values
		for(int i = 0; i < globalConfigKeyCount; i++) {
			if(!existsInMap(configMap, globalConfigKeys[i], strlen(globalConfigKeys[i]))) {
				char* keyTrimmed = malloc(strlen(globalConfigKeys[i]) - strlen(CONFIG_GLOBAL_SECTION_NAME));
				char* valueTemp = iniparser_getstring(config, globalConfigKeys[i], NULL);
				char* value = malloc(strlen(valueTemp) + 1);

				strcpy(keyTrimmed, globalConfigKeys[i] + strlen(CONFIG_GLOBAL_SECTION_NAME) + 1);
				strcpy(value, valueTemp);
				putIntoMap(configMap, keyTrimmed, strlen(keyTrimmed), value);
			}
		}

		if(!enableModule(&moduleManager, moduleType, sectionName, configMap)) {
			fprintf(stderr, "Error while enabling module %s\n", sectionName);
		}

		free(moduleType);
		free(keys);
	}

	unloadConfig();
	free(globalConfigKeys);

	sleep(1000);

	return 0;
}
