#include "Stash.h"

dictionary* stash = NULL;

bool stashInit() {
	stash = iniparser_load(STASH_FILE);
	if(stash == NULL) {
		fprintf(stderr, "Warning: Stash file was not found, creating new stash\n");
		stash = dictionary_new(0);
	}
	return stash != NULL;
}

void stashDestroy() {
	iniparser_freedict(stash);
}

bool stashSave() {
	if(stash == NULL) {
		return false;
	}

	FILE* filePointer = fopen(STASH_FILE, "w");
	if(filePointer != NULL) {
		iniparser_dump_ini(stash, filePointer);
	}

	fclose(filePointer);
	return filePointer != NULL;
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

char* stashGetString(char* section, char* key, char* defaultValue) {
	char* sectionName = stashCreateSectionName(section, key);
	char* result = iniparser_getstring(stash, sectionName, defaultValue);
	free(sectionName);
	return result;
}

int stashGetInt(char* section, char* key, int defaultValue) {
	char* sectionName = stashCreateSectionName(section, key);
	int result = iniparser_getint(stash, sectionName, defaultValue);
	free(sectionName);
	return result;
}
