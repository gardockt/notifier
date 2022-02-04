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

	bool (*get_config_string)(struct FetMod* module, const char* var_name, char** output);
	bool (*get_config_int)(struct FetMod* module, const char* var_name, int* output);
	bool (*get_config_string_log)(struct FetMod* module, const char* var_name, char** output, int message_verbosity);
	bool (*get_config_int_log)(struct FetMod* module, const char* var_name, int* output, int message_verbosity);

	Message* (*new_message)();
	bool (*display_message)(const struct FetMod* module, const Message* message);
	void (*free_message)(Message* message);

	void (*log)(struct FetMod* module, int message_verbosity, const char* format, ...);

	bool (*set_stash_string)(const char* section, const char* key, const char* value);
	bool (*set_stash_int)(const char* section, const char* key, int value);
	const char* (*get_stash_string)(const char* section, const char* key, const char* default_value);
	int (*get_stash_int)(const char* section, const char* key, int default_value);

	int (*split)(const char* text, const char* separators, char*** output);
	char* (*replace)(const char* input, int pair_count, ...);
} FetchingModule;

typedef struct {
	char* name;
} FMConfig;

#define fm_config_set_name(config, fm_name) ((config)->name = strdup(fm_name))

#define fm_get_config_string(module, var_name, output)                ((module)->get_config_string(module, var_name, output))
#define fm_get_config_int(module, var_name, output)                   ((module)->get_config_int(module, var_name, output))
#define fm_get_config_string_log(module, var_name, output, verbosity) ((module)->get_config_string_log(module, var_name, output, verbosity))
#define fm_get_config_int_log(module, var_name, output, verbosity)    ((module)->get_config_int_log(module, var_name, output, verbosity))

#define fm_set_data(module, data) ((module)->custom_data = data)
#define fm_get_data(module)       ((module)->custom_data)

#define fm_new_message()                ((module)->new_message())
#define fm_display_message(module, msg) ((module)->display_message(module, msg))
#define fm_free_message(msg)            ((module)->free_message(msg))

#define fm_log(module, msg_verbosity, format, ...) ((module)->log(module, msg_verbosity, format __VA_OPT__(,) __VA_ARGS__))

#define fm_set_stash_string(module, section, key, value)         ((module)->set_stash_string(section, key, value))
#define fm_set_stash_int(module, section, key, value)            ((module)->set_stash_int(section, key, value))
#define fm_get_stash_string(module, section, key, default_value) ((module)->get_stash_string(section, key, default_value))
#define fm_get_stash_int(module, section, key, default_value)    ((module)->get_stash_int(section, key, default_value))

#define fm_split(module, text, separators, output) ((module)->split(text, separators, output))
#define fm_replace(module, input, pair_count, ...) ((module)->replace(input, pair_count, __VA_ARGS__))

#endif /* ifndef FETCHING_MODULE_H */
