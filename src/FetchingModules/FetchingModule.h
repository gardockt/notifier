#ifndef FETCHINGMODULE_H
#define FETCHINGMODULE_H

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> // sleep

#include "../Structures/SortedMap.h"
#include "../Displays/Display.h"

typedef struct FetMod {
	char* name;
	int intervalSecs;
	void* config;
	pthread_t thread;
	pthread_t fetchingThread;
	bool busy;
	Display* display;
	char* notificationTitle;
	char* notificationBody;
	char* iconPath;
	int verbosity;

	bool (*enable)(struct FetMod*, SortedMap*);
	void (*fetch)(struct FetMod*);
	void (*disable)(struct FetMod*);
} FetchingModule;

typedef enum {
	FM_DEFAULTS            = 0,
	FM_DISABLE_CHECK_TITLE = 1 << 0,
	FM_DISABLE_CHECK_BODY  = 1 << 1
} FetchingModuleInitFlags;

bool fetchingModuleCreateThread(FetchingModule* fetchingModule);
bool fetchingModuleDestroyThread(FetchingModule* fetchingModule);

bool fetchingModuleInit(FetchingModule* fetchingModule, SortedMap* config, FetchingModuleInitFlags initFlags);

#endif // ifndef FETCHINGMODULE_H
