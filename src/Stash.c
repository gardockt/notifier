#include "Paths.h"
#include "Log.h"
#include "Globals.h"
#include "Stash.h"

dictionary* stash = NULL;

bool stashInit() {
	char* stashDirectory = get_stash_path();
	stash = iniparser_load(stashDirectory);
	if(stash == NULL) {
		logWrite("core", coreVerbosity, 2, "Stash file was not found, creating new stash");
		stash = dictionary_new(0);
	}
	free(stashDirectory);
	return stash != NULL;
}

void stashDestroy() {
	iniparser_freedict(stash);
}

bool stashSave() {
	if(stash == NULL) {
		return false;
	}

	char* stashDirectory = get_stash_path();
	FILE* filePointer = fopen(stashDirectory, "w");
	if(filePointer == NULL) {
		if(create_stash_dir()) {
			filePointer = fopen(stashDirectory, "w");
		} else {
			free(stashDirectory);
			return false;
		}
	}
	iniparser_dump_ini(stash, filePointer);

	free(stashDirectory);
	fclose(filePointer);
	return true;
}

// returned value has to be freed
char* stashCreateSectionName(char* section, char* key) {
	char* sectionName = malloc(strlen(section) + 1 + strlen(key) + 1);
	if(sectionName != NULL) {
		sprintf(sectionName, "%s:%s", section, key);
	}
	return sectionName;
}

bool stashSetString(char* section, char* key, char* value) {
	char* sectionName = stashCreateSectionName(section, key);
	bool result = (dictionary_set(stash, section, NULL) == 0 && dictionary_set(stash, sectionName, value) == 0);
	if(result) {
		stashSave();
	}

	free(sectionName);
	return result;
}

bool stashSetInt(char* section, char* key, int value) {
	char* sectionName = stashCreateSectionName(section, key);
	char* stringValue = malloc(ilogb(value) + 1 + 1);
	bool result = false;

	if(sectionName != NULL && stringValue != NULL) {
		sprintf(stringValue, "%d", value);
		result = (dictionary_set(stash, section, NULL) == 0 && dictionary_set(stash, sectionName, stringValue) == 0);
		if(result) {
			stashSave();
		}
	}

	free(sectionName);
	free(stringValue);
	return result;
}

const char* stashGetString(char* section, char* key, char* defaultValue) {
	char* sectionName = stashCreateSectionName(section, key);
	const char* result = iniparser_getstring(stash, sectionName, defaultValue);
	free(sectionName);
	return result;
}

int stashGetInt(char* section, char* key, int defaultValue) {
	char* sectionName = stashCreateSectionName(section, key);
	int result = iniparser_getint(stash, sectionName, defaultValue);
	free(sectionName);
	return result;
}
