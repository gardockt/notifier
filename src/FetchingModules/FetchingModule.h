#ifndef FETCHING_MODULE_H
#define FETCHING_MODULE_H

#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> /* sleep */
#include <string.h> /* strdup */

#include "../Message.h"

typedef struct FetMod {
	char* name;
	int interval_secs;
	void* custom_data;
	void* config;
	pthread_t thread;
	pthread_t fetching_thread;
	bool busy;
	void* display;
	int verbosity;
	void* library;

	bool (*enable)(struct FetMod* module);
	void (*fetch)(struct FetMod* module);
	void (*disable)(struct FetMod* module);

	const char* (*get_config_var)(struct FetMod* module, const char* var_name);

	Message* (*new_message)();
	bool (*display_message)(struct FetMod* module, const Message* message);
	void (*free_message)(Message* message);
} FetchingModule;

typedef struct {
	char* name;
} FMConfig;

#define fm_config_set_name(config, fm_name) ((config)->name = strdup(fm_name))

#define fm_get_config_var(module, var_name) ((module)->get_config_var(module, var_name))

#define fm_set_data(module, data) ((module)->custom_data = data)
#define fm_get_data(module)       ((module)->custom_data)

#define fm_new_message()                ((module)->new_message())
#define fm_display_message(module, msg) ((module)->display_message(module, msg))
#define fm_free_message(msg)            ((module)->free_message(msg))

#endif /* ifndef FETCHING_MODULE_H */
