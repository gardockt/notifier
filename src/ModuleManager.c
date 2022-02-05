#include "Globals.h"
#include "Config.h"
#include "Log.h"
#include "Paths.h"
#include "StringOperations.h" /* only for passing its functions to FM, I wish there was a better way */
#include "Stash.h"
#include "Structures/SortedMap.h"
#include "FetchingModules/FetchingModule.h"
#include "Displays/Display.h"
#include "ModuleManager.h"

/* TODO: rename to FMManager */

static void fm_log_definition(const FetchingModule* module, int message_verbosity, const char* format, ...) {
	va_list args;
	va_start(args, format);
	logWriteVararg(module->name, module->verbosity, message_verbosity, format, args);
	va_end(args);
}

static const char* fm_get_config_var(FetchingModule* module, const char* var_name) {
	SortedMap* config = module->config;
	return sortedMapGet(config, var_name);
}

static bool fm_get_config_string_definition(FetchingModule* module, const char* var_name, char** output) {
	const char* value = fm_get_config_var(module, var_name);
	if(value != NULL) {
		*output = strdup(value);
	}
	return value != NULL;
}

static bool fm_get_config_int_definition(FetchingModule* module, const char* var_name, int* output) {
	const char* value = fm_get_config_var(module, var_name);
	if(value == NULL) {
		return false;
	}

	for(int i = 0; value[i] != '\0'; i++) {
		if(!(isdigit(value[i]) || (i == 0 && value[i] == '-'))) {
			return false;
		}
	}

	*output = atoi(value);
	return true;
}

static bool fm_get_config_string_log_definition(FetchingModule* module, const char* var_name, char** output, int message_verbosity) {
	bool success = fm_get_config_string_definition(module, var_name, output);
	if(!success) {
		fm_log_definition(module, message_verbosity, "Invalid %s", var_name);
	}
	return success;
}

static bool fm_get_config_int_log_definition(FetchingModule* module, const char* var_name, int* output, int message_verbosity) {
	bool success = fm_get_config_int_definition(module, var_name, output);
	if(!success) {
		fm_log_definition(module, message_verbosity, "Invalid %s", var_name);
	}
	return success;
}

static Message* fm_new_message_definition() {
	Message* msg = malloc(sizeof *msg);
	memset(msg, 0, sizeof *msg);
	return msg;
}

static void fm_free_message_definition(Message* msg) {
	free(msg);
}

static bool fm_display_message_definition(const FetchingModule* module, const Message* message) {
	Display* display = module->display;
	logWrite("core", coreVerbosity, 3, "Displaying message:\nTitle: %s\nBody: %s\nAction data: %s", message->title, message->body, message->action_data);
	return display->display_message(message);
}

static void* fm_fetching_thread(void* args) {
	FetchingModule* module = args;
	void (*fetch)(FetchingModule*) = (void (*)(FetchingModule*)) dlsym(module->library, "fetch");

	module->log(module, 2, "Fetch started");
	module->busy = true;
	fetch(module);
	module->busy = false;
	module->log(module, 2, "Fetch finished");

	pthread_exit(NULL);
}

static void* fm_thread(void* args) {
	FetchingModule* module = args;

	while(1) {
		if(!module->busy) {
			pthread_create(&module->fetching_thread, NULL, fm_fetching_thread, module);
		}
		sleep(module->interval_secs);
	}
}

static bool fm_create_thread(FetchingModule* module) {
	return pthread_create(&module->thread, NULL, fm_thread, module) == 0;
}

static bool fm_destroy_thread(FetchingModule* module) {
	/* fetching thread quits by itself */
	pthread_join(module->fetching_thread, NULL);

	return pthread_cancel(module->thread) == 0 &&
	       pthread_join(module->thread, NULL) == 0;
}

static bool fm_manager_add_library(ModuleManager* manager, const char* directory_path, const char* lib_file_name) {
	bool success = false;
	char* full_path = malloc(strlen(directory_path) + 1 + strlen(lib_file_name) + 1);
	if(full_path == NULL) {
		logWrite("core", coreVerbosity, 0, "Out of memory");
		goto fm_manager_add_library_finish;
	}

	sprintf(full_path, "%s/%s", directory_path, lib_file_name);

	void* handle = dlopen(full_path, RTLD_LAZY);
	if(handle == NULL) {
		logWrite("core", coreVerbosity, 0, "Could not open library \"%s\": %s", lib_file_name, dlerror());
		goto fm_manager_add_library_finish;
	}

	void (*configure)(FMConfig*) = (void (*)(FMConfig*)) dlsym(handle, "configure");
	if(configure == NULL) {
		logWrite("core", coreVerbosity, 0, "Configure function not found in library \"%s\"", lib_file_name);
		goto fm_manager_add_library_finish;
	}

	FMConfig config = {0};
	configure(&config);

	success = sortedMapPut(&manager->available_modules, config.name, handle);

fm_manager_add_library_finish:
	free(full_path);
	return success;
}

bool fm_manager_init(ModuleManager* manager) {
	bool success = false;

	if(!(sortedMapInit(&manager->available_modules, sortedMapCompareFunctionStrcasecmp) &&
	     sortedMapInit(&manager->active_modules, sortedMapCompareFunctionStrcmp))) {
		return false;
	}

	/* add modules from directory */
	char* fm_path = get_fm_path();
	struct dirent* de;
	DIR* d = opendir(fm_path);

	if(d == NULL)
		goto fm_manager_init_finish;

	while((de = readdir(d)) != NULL) {
		/* TODO: find a better way of determining whether a file can be read? */
		/* TODO: also check whether file is executable */
		if(de->d_type == DT_REG || de->d_type == DT_LNK) {
			logWrite("core", 2, coreVerbosity, "Loading module \"%s\"", de->d_name);
			if(!fm_manager_add_library(manager, fm_path, de->d_name)) {
				logWrite("core", coreVerbosity, 0, "Could not load module \"%s\"", de->d_name);
			}
		}
	}

	success = true;

fm_manager_init_finish:
	closedir(d);
	free(fm_path);
	return success;
}

void fm_manager_destroy(ModuleManager* manager) {
	SortedMap map_to_free;
	int map_to_free_size;
	char** keys_to_free;

	/* TODO: create a function for getting key/value pairs instead of doing this? */

	map_to_free = manager->active_modules;
	map_to_free_size = sortedMapSize(&map_to_free);
	keys_to_free = malloc(map_to_free_size * sizeof *keys_to_free);
	sortedMapKeys(&map_to_free, (void**)keys_to_free);
	for(int i = 0; i < map_to_free_size; i++) {
		fm_disable(manager, keys_to_free[i]);
	}
	free(keys_to_free);

	map_to_free = manager->available_modules;
	map_to_free_size = sortedMapSize(&map_to_free);
	keys_to_free = malloc(map_to_free_size * sizeof *keys_to_free);
	sortedMapKeys(&map_to_free, (void**)keys_to_free);
	for(int i = 0; i < map_to_free_size; i++) {
		void* handle = sortedMapGet(&map_to_free, keys_to_free[i]);
		dlclose(handle);
	}
	for(int i = 0; i < map_to_free_size; i++) {
		free(keys_to_free[i]);
	}
	free(keys_to_free);

	sortedMapDestroy(&manager->available_modules);
	sortedMapDestroy(&manager->active_modules);
}

static bool fm_load_basic_settings(FetchingModule* module, SortedMap* config, void* library) {
	module->config = config;

	if(!fm_get_config_string_definition(module, "_name", &module->name) ||
	   !fm_get_config_int_log_definition(module, "interval", &module->interval_secs, 0)) {
		return false;
	}

	fm_get_config_int_definition(module, "verbosity", &module->verbosity);

	char* display_name = sortedMapGet(config, "display");
	if(display_name == NULL) {
		module->log(module, 0, "Invalid display");
		return false;
	}

	Display* display = display_manager_get_display(&displayManager, display_name);
	if(display == NULL) {
		module->log(module, 0, "Display does not exist");
		return false;
	}
	if(!display->init()) {
		module->log(module, 0, "Failed to init display");
		return false;
	}

	module->display = display;
	module->library = library;

	module->get_config_string = fm_get_config_string_definition;
	module->get_config_int = fm_get_config_int_definition;
	module->get_config_string_log = fm_get_config_string_log_definition;
	module->get_config_int_log = fm_get_config_int_log_definition;

	module->new_message = fm_new_message_definition;
	module->display_message = fm_display_message_definition;
	module->free_message = fm_free_message_definition;
	module->log = fm_log_definition;

	module->get_stash_string = stash_get_string;
	module->get_stash_int = stash_get_int;
	module->set_stash_string = stash_set_string;
	module->set_stash_int = stash_set_int;

	module->split = split;
	module->replace = replace;

	/* TODO: config can be reached only on enable? verify and do something about it  */

	return true;
}

static void fm_free_basic_settings(FetchingModule* module) {
	Display* display = module->display;
	display->uninit();
	free(module->name);
}

bool fm_enable(ModuleManager* manager, char* type, char* custom_name, SortedMap* config) {
	if(manager == NULL) {
		logWrite("core", coreVerbosity, 0, "Manager is NULL");
		return false;
	}
	if(type == NULL) {
		logWrite("core", coreVerbosity, 0, "Module type is NULL");
		return false;
	}
	if(custom_name == NULL) {
		logWrite("core", coreVerbosity, 0, "Module custom name is NULL");
		return false;
	}
	if(config == NULL) {
		logWrite("core", coreVerbosity, 0, "Config for module %s is NULL", custom_name);
		return false;
	}

	void* library = sortedMapGet(&manager->available_modules, type);
	bool (*enable)(FetchingModule*);
	FetchingModule* module;

	if(library == NULL) {
		logWrite("core", coreVerbosity, 0, "Fetching module \"%s\" not found", type);
		return false;
	}

	enable = (bool (*)(FetchingModule*)) dlsym(library, "enable");
	if(dlerror() != NULL) {
		logWrite("core", coreVerbosity, 0, "Enable function not found in fetching module \"%s\"", type);
		return false;
	}

	module = malloc(sizeof *module);
	if(module == NULL) {
		logWrite("core", coreVerbosity, 0, "Out of memory");
		return false;
	}

	if(!fm_load_basic_settings(module, config, library)) {
		logWrite("core", coreVerbosity, 0, "Could not initialize fetching module \"%s\"", custom_name);
		return false;
	}
	if(!enable(module)) {
		logWrite("core", coreVerbosity, 0, "Failed to enable fetching module \"%s\"", custom_name);
		return false;
	}
	if(!fm_create_thread(module)) {
		logWrite("core", coreVerbosity, 0, "Failed to create thread for fetching module \"%s\"", custom_name);
		return false;
	}

	fm_log_definition(module, 1, "Module enabled");
	sortedMapPut(&manager->active_modules, custom_name, module);
	return true;
}

bool fm_disable(ModuleManager* manager, char* custom_name) {
	FetchingModule* module = sortedMapGet(&manager->active_modules, custom_name);
	void (*disable)(FetchingModule*) = (void (*)(FetchingModule*)) dlsym(module->library, "disable");
	char* custom_name_copy = strdup(custom_name);
	int verbosity = module->verbosity;

	if(module == NULL) {
		return false;
	}

	char* key_to_free;

	fm_destroy_thread(module);
	disable(module);
	fm_free_basic_settings(module);
	sortedMapRemove(&manager->active_modules, custom_name, (void**)&key_to_free, NULL); /* value is already stored in "module" */
	free(key_to_free);
	free(module);

	logWrite(custom_name_copy, verbosity, 1, "Module disabled");
	free(custom_name_copy);
	return true;
}
