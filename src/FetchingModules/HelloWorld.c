#include "HelloWorld.h"

void* helloWorldFetchingThread(void* args) {
	FetchingModule* fetchingModule = args;

	fetchingModule->busy = true;
	fetchingModule->fetch(fetchingModule);
	fetchingModule->busy = false;

	pthread_exit(NULL);
}

void* helloWorldThread(void* args) {
	FetchingModule* fetchingModule = args;
	pthread_t fetchingThread;

	while(1) {
		if(!fetchingModule->busy) {
			pthread_create(&fetchingThread, NULL, helloWorldFetchingThread, fetchingModule);
			pthread_detach(fetchingThread);
		}
		sleep(fetchingModule->intervalSecs);
	}
}

bool helloWorldEnable(FetchingModule* fetchingModule) {
	pthread_create(&fetchingModule->thread, NULL, helloWorldThread, fetchingModule);

	printf("HelloWorld enabled\n");
	return true;
}

void helloWorldFetch(FetchingModule* fetchingModule) {
	HelloWorldConfig* config = fetchingModule->config;
	printf("%s\n", config->text);
}

void helloWorldDisable(FetchingModule* fetchingModule) {
	pthread_cancel(fetchingModule->thread);

	printf("HelloWorld disabled\n");
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
