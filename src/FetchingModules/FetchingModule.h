#ifndef FETCHING_MODULE_H
#define FETCHING_MODULE_H

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> /* sleep */
#include <string.h> /* strdup */

/* TODO: avoid having to call functions directly from struct */

typedef struct FetMod {
	char* name;
	int interval_secs;
	void* config;
	pthread_t thread;
	pthread_t fetching_thread;
	bool busy;
	void* display;
	char* notification_title;
	char* notification_body;
	char* icon_path;
	int verbosity;
	void* library;

	bool (*enable)(struct FetMod* module);
	void (*fetch)(struct FetMod* module);
	void (*disable)(struct FetMod* module);

	const char* (*get_config_var)(struct FetMod* module, const char* var_name);
} FetchingModule;

typedef struct {
	char* name;
} FMConfig;

typedef enum {
	FM_DEFAULTS            = 0,
	FM_DISABLE_CHECK_TITLE = 1 << 0,
	FM_DISABLE_CHECK_BODY  = 1 << 1
} FMInitFlags;

#define fm_config_set_name(config, fm_name) ((config)->name = strdup(fm_name))

#endif /* ifndef FETCHING_MODULE_H */
