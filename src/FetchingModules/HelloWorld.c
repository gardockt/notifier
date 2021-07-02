#include "HelloWorld.h"

void helloWorldParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	HelloWorldConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	// TODO: fix memleak on unused keys
	char* text = getFromMap(configToParse, "text", strlen("text"));
	char* interval = getFromMap(configToParse, "interval", strlen("interval"));

	config->text = text;
	fetchingModule->intervalSecs = atoi(interval);
}

bool helloWorldEnable(FetchingModule* fetchingModule) {
	bool retVal = fetchingModuleCreateThread(fetchingModule);

	if(retVal) {
		printf("HelloWorld enabled\n");
	}
	return retVal;
}

void helloWorldFetch(FetchingModule* fetchingModule) {
	HelloWorldConfig* config = fetchingModule->config;
	printf("%s\n", config->text);
}

bool helloWorldDisable(FetchingModule* fetchingModule) {
	bool retVal = fetchingModuleDestroyThread(fetchingModule);

	if(retVal) {
		printf("HelloWorld disabled\n");
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
