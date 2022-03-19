#include "Globals.h"
#include "Log.h"
#include "Paths.h"
#include "DisplayManager.h"

static bool display_manager_add_library(DisplayManager* manager, const char* dir_path, const char* lib_file_name) {
	bool success = false;
	char* full_path = malloc(strlen(dir_path) + 1 + strlen(lib_file_name) + 1);
	if(full_path == NULL) {
		log_write("core", core_verbosity, 0, "Out of memory");
		goto display_manager_add_library_finish;
	}

	sprintf(full_path, "%s/%s", dir_path, lib_file_name);

	void* handle = dlopen(full_path, RTLD_LAZY);
	if(handle == NULL) {
		log_write("core", core_verbosity, 0, "Could not open library \"%s\": %s", lib_file_name, dlerror());
		goto display_manager_add_library_finish;
	}

	void (*f_configure)(DisplayLibConfig*) = (void (*)(DisplayLibConfig*)) dlsym(handle, "configure");
	bool (*f_enable)() = (bool (*)()) dlsym(handle, "enable");
	bool (*f_display)(const Message*) = (bool (*)(const Message*)) dlsym(handle, "display");
	void (*f_disable)() = (void (*)()) dlsym(handle, "disable");

	if(f_configure == NULL) {
		log_write("core", core_verbosity, 0, "Configure function not found in library \"%s\"", lib_file_name);
		goto display_manager_add_library_finish;
	}
	if(f_enable == NULL) {
		log_write("core", core_verbosity, 0, "Enable function not found in library \"%s\"", lib_file_name);
		goto display_manager_add_library_finish;
	}
	if(f_display == NULL) {
		log_write("core", core_verbosity, 0, "Display function not found in library \"%s\"", lib_file_name);
		goto display_manager_add_library_finish;
	}
	if(f_disable == NULL) {
		log_write("core", core_verbosity, 0, "Disable function not found in library \"%s\"", lib_file_name);
		goto display_manager_add_library_finish;
	}

	DisplayLibConfig config = {0};
	f_configure(&config);

	if(config.name == NULL) {
		log_write("core", core_verbosity, 0, "No name set for library \"%s\"", lib_file_name);
		goto display_manager_add_library_finish;
	}

	Display* display = malloc(sizeof *display);
	if(display == NULL) {
		log_write("core", core_verbosity, 0, "Out of memory\n");
		goto display_manager_add_library_finish;
	}
	display->enable = f_enable;
	display->display = f_display;
	display->disable = f_disable;
	display->handle = handle;

	success = sorted_map_put(&manager->displays, config.name, display);
display_manager_add_library_finish:
	free(full_path);
	return success;
}

bool display_manager_init(DisplayManager* manager) {
	if(!sorted_map_init(&manager->displays, (int (*)(const void*, const void*))strcasecmp)) {
		log_write("core", core_verbosity, 1, "Error initializing display map");
		return false;
	}

	bool success = false;
	char* display_path = get_display_path();
	struct dirent* de;
	DIR* d = opendir(display_path);

	if(d == NULL) {
		log_write("core", core_verbosity, 1, "Could not open display directory");
		goto display_manager_init_finish;
	}

	while((de = readdir(d)) != NULL) {
		/* TODO: find a better way of determining whether a file can be read? */
		/* TODO: also check whether file is executable */
		if(de->d_type == DT_REG || de->d_type == DT_LNK) {
			log_write("core", 2, core_verbosity, "Loading display \"%s\"", de->d_name);
			if(!display_manager_add_library(manager, display_path, de->d_name)) {
				log_write("core", core_verbosity, 0, "Could not load display \"%s\"", de->d_name);
			}
		}
	}

	success = true;

display_manager_init_finish:
	closedir(d);
	free(display_path);
	return success;
}

void display_manager_destroy(DisplayManager* manager) {
	int map_to_free_size;
	char** keys_to_free;

	/* TODO: create a function for getting key/value pairs instead of doing this? */

	map_to_free_size = sorted_map_size(&manager->displays);
	keys_to_free = malloc(map_to_free_size * sizeof *keys_to_free);
	sorted_map_keys(&manager->displays, (void**)keys_to_free);
	for(int i = 0; i < map_to_free_size; i++) {
		Display* display = sorted_map_get(&manager->displays, keys_to_free[i]);
		dlclose(display->handle);
	}
	for(int i = 0; i < map_to_free_size; i++) {
		free(keys_to_free[i]);
	}
	free(keys_to_free);

	sorted_map_destroy(&manager->displays);
}

Display* display_manager_get_display(DisplayManager* manager, const char* name) {
	return sorted_map_get(&manager->displays, name);
}
