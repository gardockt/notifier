#include "Dirs.h"

#define DIR_CONFIG_RELATIVE ".config/notifier/notifier.ini"
#define DIR_STASH_RELATIVE  ".cache/notifier/stash.ini"

// do not free
char* getHomeDirectory() {
	return getenv("HOME");
}

// $HOME/.config/notifier/notifier.ini
char* getConfigDirectory() {
	char* home = getHomeDirectory();
	char* directory = malloc(strlen(home) + 1 + strlen(DIR_CONFIG_RELATIVE) + 1);
	sprintf(directory, "%s/%s", home, DIR_CONFIG_RELATIVE);
	return directory;
}

// $HOME/.cache/notifier/stash.ini
char* getStashDirectory() {
	char* home = getHomeDirectory();
	char* directory = malloc(strlen(home) + 1 + strlen(DIR_STASH_RELATIVE) + 1);
	sprintf(directory, "%s/%s", home, DIR_STASH_RELATIVE);
	return directory;
}

bool createStashDirectory() {
	char* directory = getStashDirectory();
	char* checkedDirectory = malloc(strlen(directory));
	int directoryLength = strlen(directory);
	DIR* dirPointer;
	bool pathExists = true;

	memset(checkedDirectory, 0, directoryLength);
	checkedDirectory[0] = '/'; // to avoid checking empty string
	for(int i = 1; i < directoryLength; i++) {
		if(directory[i] == '/') {
			if(pathExists) {
				dirPointer = opendir(checkedDirectory);
				pathExists = (dirPointer != NULL);
				if(pathExists) {
					closedir(dirPointer);
				}
			}
			if(!pathExists) {
				if(mkdir(checkedDirectory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
					free(checkedDirectory);
					free(directory);
					return false;
				}
			}
		}
		checkedDirectory[i] = directory[i];
	}

	free(checkedDirectory);
	free(directory);
	return true;
}
