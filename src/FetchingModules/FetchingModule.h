#ifndef FETCHINGMODULE_H
#define FETCHINGMODULE_H

#include <stdbool.h>
#include <pthread.h>

typedef struct FetMod {
	int intervalSecs;
	void* config;
	pthread_t thread;
	bool busy;

	bool (*enable)(struct FetMod*);
	void (*fetch)(struct FetMod*);
	void (*disable)(struct FetMod*);
} FetchingModule;

#endif // ifndef FETCHINGMODULE_H
