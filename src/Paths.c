#include "Paths.h"

#define DIR_CONFIG_RELATIVE ".config/notifier"
#define DIR_STASH_RELATIVE  ".cache/notifier/stash.ini"
#define PATH_CONFIG_RELATIVE ".config/notifier/notifier.ini"

static const char* get_home_path() {
	return getenv("HOME");
}

/* $HOME/.config/notifier */
static char* get_config_path() {
	const char* home = get_home_path();
	char* directory = malloc(strlen(home) + 1 + strlen(DIR_CONFIG_RELATIVE) + 1);
	sprintf(directory, "%s/%s", home, DIR_CONFIG_RELATIVE);
	return directory;
}

/* $HOME/.config/notifier/notifier.ini */
char* get_config_file_path() {
	const char* home = get_home_path();
	char* directory = malloc(strlen(home) + 1 + strlen(PATH_CONFIG_RELATIVE) + 1);
	sprintf(directory, "%s/%s", home, PATH_CONFIG_RELATIVE);
	return directory;
}

/* $HOME/.cache/notifier/stash.ini */
char* get_stash_path() {
	const char* home = get_home_path();
	char* directory = malloc(strlen(home) + 1 + strlen(DIR_STASH_RELATIVE) + 1);
	sprintf(directory, "%s/%s", home, DIR_STASH_RELATIVE);
	return directory;
}

bool create_stash_dir() {
	char* path = get_stash_path();
	char* checked_path = malloc(strlen(path) + 1);
	int path_length = strlen(path);
	DIR* dir;
	bool path_exists = true;

	memset(checked_path, 0, path_length);
	checked_path[0] = '/'; /* to avoid checking empty string */
	for(int i = 1; i < path_length; i++) {
		if(path[i] == '/') {
			if(path_exists) {
				dir = opendir(checked_path);
				path_exists = (dir != NULL);
				if(path_exists) {
					closedir(dir);
				}
			}
			if(!path_exists) {
				if(mkdir(checked_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
					free(checked_path);
					free(path);
					return false;
				}
			}
		}
		checked_path[i] = path[i];
	}

	free(checked_path);
	free(path);
	return true;
}

/* $HOME/.config/notifier/fetchingmodules */
char* get_fm_path() {
	char* config_path = get_config_path();
	char* ret = malloc(strlen(config_path) + strlen("/fetchingmodules") + 1);
	sprintf(ret, "%s/fetchingmodules", config_path);
	free(config_path);
	return ret;
}

/* $HOME/.config/notifier/displays */
char* get_display_path() {
	char* config_path = get_config_path();
	char* ret = malloc(strlen(config_path) + strlen("/displays") + 1);
	sprintf(ret, "%s/displays", config_path);
	free(config_path);
	return ret;
}
