#include "Globals.h"
#include "Config.h"
#include "Log.h"
#include "Paths.h"
#include "Structures/SortedMap.h"
#include "FetchingModules/FetchingModule.h"
#include "FetchingModules/Utilities/FetchingModuleUtilities.h"
#include "Displays/Display.h"
#include "ModuleManager.h"

/* TODO: rename to FMManager */

static const char* fm_get_config_var_definition(FetchingModule* module, const char* var_name) {
	SortedMap* config = module->config;
	return sortedMapGet(config, var_name);
}

static void* fm_fetching_thread(void* args) {
	FetchingModule* module = args;
	void (*fetch)(FetchingModule*) = (void (*)(FetchingModule*)) dlsym(module->library, "fetch");

	moduleLog(module, 2, "Fetch started");
	module->busy = true;
	fetch(module);
	module->busy = false;
	moduleLog(module, 2, "Fetch finished");

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

static bool fm_init(FetchingModule* module, SortedMap* config, FMInitFlags init_flags) {
	if(!(init_flags & FM_DISABLE_CHECK_TITLE)) {
		if(!moduleLoadStringFromConfigWithErrorMessage(module, config, "title", &module->notification_title)) {
			return false;
		}
	}
	if(!(init_flags & FM_DISABLE_CHECK_BODY)) {
		if(!moduleLoadStringFromConfigWithErrorMessage(module, config, "body", &module->notification_body)) {
			return false;
		}
	}

	return true;
}

static bool fm_manager_add_library(ModuleManager* manager, const char* directory_path, const char* lib_file_name) {
	bool success = false;
	char* full_path = malloc(strlen(directory_path) + 1 + strlen(lib_file_name) + 1);
	if(full_path == NULL) {
		goto fm_manager_add_library_finish;
	}

	sprintf(full_path, "%s/%s", directory_path, lib_file_name);

	void* handle = dlopen(full_path, RTLD_LAZY);
	if(handle == NULL) {
		goto fm_manager_add_library_finish;
	}

	void (*configure)(FMConfig*) = (void (*)(FMConfig*)) dlsym(handle, "configure");
	if(configure == NULL) {
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
			logWrite("core", 2, coreVerbosity, "Loading module %s", de->d_name);
			if(!fm_manager_add_library(manager, fm_path, de->d_name)) {
				logWrite("core", 0, coreVerbosity, "Could not load module %s", de->d_name);
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
		dlclose(sortedMapGet(&map_to_free, keys_to_free[i]));
		free(keys_to_free[i]);
	}
	free(keys_to_free);

	sortedMapDestroy(&manager->available_modules);
	sortedMapDestroy(&manager->active_modules);
}

static bool fm_load_basic_settings(FetchingModule* module, SortedMap* config, void* library) {
	if(!configLoadString(config, "_name", &module->name) ||
	   !moduleLoadIntFromConfigWithErrorMessage(module, config, "interval", &module->interval_secs)) {
		return false;
	}

	configLoadString(config, "icon", &module->icon_path);
	configLoadInt(config, "verbosity", &module->verbosity);

	/* TODO: restore
	char* display_name = sortedMapGet(config, "display");
	if(display_name == NULL) {
		moduleLog(module, 0, "Invalid display");
		return false;
	}
	module->display = getDisplay(&displayManager, display_name);
	if(module->display == NULL) {
		moduleLog(module, 0, "Display does not exist");
		return false;
	}
	if(!module->display->init()) {
		moduleLog(module, 0, "Failed to init display");
		return false;
	}
	*/

	module->library = library;

	module->get_config_var = fm_get_config_var_definition;
	module->config = config;

	return true;
}

static void fm_free_basic_settings(FetchingModule* module) {
	/* TODO: restore */
	/* module->display->uninit(); */
	free(module->name);
	/*
	free(module->notification_title);
	free(module->notification_body);
	free(module->icon_path);
	*/
}

bool fm_enable(ModuleManager* manager, char* type, char* custom_name, SortedMap* config) {
	if(manager == NULL || type == NULL || custom_name == NULL || config == NULL)
		return false;

	void* library = sortedMapGet(&manager->available_modules, type);
	bool (*enable)(FetchingModule*);
	FetchingModule* module;

	if(library == NULL)
		return false;

	enable = (bool (*)(FetchingModule*)) dlsym(library, "enable");
	if(dlerror() != NULL)
		return false;

	module = malloc(sizeof *module);
	if(module == NULL)
		return false;

	bool success = fm_load_basic_settings(module, config, library) &&
		           enable(module) &&
				   fm_create_thread(module);

	if(success) {
		moduleLog(module, 1, "Module enabled");
		sortedMapPut(&manager->active_modules, custom_name, module);
	}
	return success;
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
