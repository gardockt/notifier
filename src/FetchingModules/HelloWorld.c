#include "HelloWorld.h"

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
	fetchingModule->enable = helloWorldEnable;
	fetchingModule->fetch = helloWorldFetch;
	fetchingModule->disable = helloWorldDisable;

	return true;
}
