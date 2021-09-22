#ifndef FETCHINGMODULE_H
#define FETCHINGMODULE_H

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> // sleep

#include "../Structures/Map.h"
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

	bool (*enable)(struct FetMod*, Map*);
	void (*fetch)(struct FetMod*);
	void (*disable)(struct FetMod*);
} FetchingModule;

bool fetchingModuleCreateThread(FetchingModule* fetchingModule);
bool fetchingModuleDestroyThread(FetchingModule* fetchingModule);

#endif // ifndef FETCHINGMODULE_H
