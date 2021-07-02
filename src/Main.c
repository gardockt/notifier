#include "Main.h"
#include "ModuleManager.h"
#include "FetchingModules/HelloWorld.h"

ModuleManager moduleManager;

dictionary* config;

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
	//HelloWorldConfig* config = malloc(sizeof *config);

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

	//config->text = "gitara siema";

	int configSectionCount = iniparser_getnsec(config);
	printf("Ilość sekcji: %d\n\n", configSectionCount);
	for(int i = 0; i < configSectionCount; i++) {
		char* sectionName = iniparser_getsecname(config, i);
		char** keys;
		int keyCount;
		Map* configMap;
		char* value;
		char* moduleType;

		printf("Nazwa sekcji %d: %s\n", i, sectionName);
		if(!strncmp(sectionName, CONFIG_GLOBAL_SECTION_NAME, strlen(CONFIG_GLOBAL_SECTION_NAME))) {
			fprintf(stderr, "Global config is not supported yet\n");
		} else {
			// nieglobalny config
			keyCount = iniparser_getsecnkeys(config, sectionName);
			printf("Ilość kluczy: %d\n", keyCount);
			
			configMap = malloc(sizeof *configMap);
			initMap(configMap);
			keys = malloc(keyCount * sizeof *keys);
			iniparser_getseckeys(config, sectionName, keys);

			for(int i = 0; i < keyCount; i++) {
				printf("Klucz %d: %s\n", i, keys[i]);
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
					printf("Dodawany moduł: %s\n", value);
					moduleType = value;
				} else {
					printf("Dodawanie do mapy pary %s -> %s\n", keyTrimmed, value);
					putIntoMap(configMap, keyTrimmed, strlen(keyTrimmed), value);
				}
			}

			// odpalanie modułu
			printf("Odpalanie modułu %s (%s)...\n", sectionName, moduleType);
			enableModule(&moduleManager, moduleType, sectionName, configMap);
		}
		printf("\n");
	}

	sleep(1000);

	return 0;
}
