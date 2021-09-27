#include "FetchingModule.h"
#include "Extras/FetchingModuleUtilities.h"

void* fetchingModuleFetchingThread(void* args) {
	FetchingModule* fetchingModule = args;

	moduleLog(fetchingModule, 2, "Fetch started");
	fetchingModule->busy = true;
	fetchingModule->fetch(fetchingModule);
	fetchingModule->busy = false;
	moduleLog(fetchingModule, 2, "Fetch finished");

	pthread_exit(NULL);
}

void* fetchingModuleThread(void* args) {
	FetchingModule* fetchingModule = args;

	while(1) {
		if(!fetchingModule->busy) {
			pthread_create(&fetchingModule->fetchingThread, NULL, fetchingModuleFetchingThread, fetchingModule);
		}
		sleep(fetchingModule->intervalSecs);
	}
}

bool fetchingModuleCreateThread(FetchingModule* fetchingModule) {
	return pthread_create(&fetchingModule->thread, NULL, fetchingModuleThread, fetchingModule) == 0;
}

bool fetchingModuleDestroyThread(FetchingModule* fetchingModule) {
	// fetching thread quits by itself
	pthread_join(fetchingModule->fetchingThread, NULL);

	return pthread_cancel(fetchingModule->thread) == 0 &&
	       pthread_join(fetchingModule->thread, NULL) == 0;
}
