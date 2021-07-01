#include "FetchingModule.h"

void* fetchingModuleFetchingThread(void* args) {
	FetchingModule* fetchingModule = args;

	fetchingModule->busy = true;
	fetchingModule->fetch(fetchingModule);
	fetchingModule->busy = false;

	pthread_exit(NULL);
}

void* fetchingModuleThread(void* args) {
	FetchingModule* fetchingModule = args;
	pthread_t fetchingThread;

	while(1) {
		if(!fetchingModule->busy) {
			pthread_create(&fetchingThread, NULL, fetchingModuleFetchingThread, fetchingModule);
			pthread_detach(fetchingThread);
		}
		sleep(fetchingModule->intervalSecs);
	}
}

bool fetchingModuleCreateThread(FetchingModule* fetchingModule) {
	return pthread_create(&fetchingModule->thread, NULL, fetchingModuleThread, fetchingModule) == 0;
}

bool fetchingModuleDestroyThread(FetchingModule* fetchingModule) {
	return pthread_cancel(fetchingModule->thread) == 0;
}
