#ifndef FETCHINGMODULE_H
#define FETCHINGMODULE_H

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> // sleep

#include "../Structures/Map.h"
#include "../Displays/Display.h"

typedef struct FetMod {
	int intervalSecs;
	void* config;
	pthread_t thread;
	bool busy;
	Display* display;
	char* notificationTitle;
	char* notificationBody;

	bool (*parseConfig)(struct FetMod*, Map*);
	bool (*enable)(struct FetMod*);
	void (*fetch)(struct FetMod*);
	bool (*disable)(struct FetMod*);
} FetchingModule;

bool fetchingModuleCreateThread(FetchingModule* fetchingModule);
bool fetchingModuleDestroyThread(FetchingModule* fetchingModule);

#endif // ifndef FETCHINGMODULE_H
