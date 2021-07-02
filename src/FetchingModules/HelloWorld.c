#include "HelloWorld.h"

void helloWorldParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	HelloWorldConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	char* text = getFromMap(configToParse, "text", strlen("text"));
	char* interval = getFromMap(configToParse, "interval", strlen("interval"));

	config->text = text;
	fetchingModule->intervalSecs = atoi(interval);

	int keyCount = getMapSize(configToParse);
	char** keys = malloc(keyCount * sizeof *keys);
	getMapKeys(configToParse, keys);

	for(int i = 0; i < keyCount; i++) {
		char* valueToFree;
		removeFromMap(configToParse, keys[i], strlen(keys[i]), NULL, &valueToFree); // key is already in keys[i]
		if(strcmp(keys[i], "text") != 0) {
			free(valueToFree);
		}
		free(keys[i]);
	}
	
	free(keys);
	destroyMap(configToParse);
}

bool helloWorldEnable(FetchingModule* fetchingModule) {
	bool retVal = fetchingModuleCreateThread(fetchingModule);

	if(retVal) {
		printf("HelloWorld enabled\n");
	}
	return retVal;
}

void helloWorldFetch(FetchingModule* fetchingModule) {
	HelloWorldConfig* config = (HelloWorldConfig*)(fetchingModule->config);
	printf("%s\n", config->text);
}

bool helloWorldDisable(FetchingModule* fetchingModule) {
	HelloWorldConfig* config = fetchingModule->config;

	bool retVal = fetchingModuleDestroyThread(fetchingModule);

	if(retVal) {
		printf("HelloWorld disabled\n");
		free(config->text);
		free(config);
	}

	return retVal;
}

bool helloWorldTemplate(FetchingModule* fetchingModule) {
	fetchingModule->intervalSecs = 1;
	fetchingModule->config = NULL;
	fetchingModule->busy = false;
	fetchingModule->parseConfig = helloWorldParseConfig;
	fetchingModule->enable = helloWorldEnable;
	fetchingModule->fetch = helloWorldFetch;
	fetchingModule->disable = helloWorldDisable;

	return true;
}
