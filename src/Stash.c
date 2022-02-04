#include "Paths.h"
#include "Log.h"
#include "Globals.h"
#include "Stash.h"

dictionary* stash = NULL;

bool stash_init() {
	char* stash_path = get_stash_path();
	stash = iniparser_load(stash_path);
	if(stash == NULL) {
		logWrite("core", coreVerbosity, 2, "Stash file was not found, creating new stash");
		stash = dictionary_new(0);
	}
	free(stash_path);
	return stash != NULL;
}

void stash_destroy() {
	iniparser_freedict(stash);
}

static bool stash_save() {
	if(stash == NULL) {
		return false;
	}

	char* stash_path = get_stash_path();
	FILE* file_ptr = fopen(stash_path, "w");
	if(file_ptr == NULL) {
		if(create_stash_dir()) {
			file_ptr = fopen(stash_path, "w");
		} else {
			free(stash_path);
			return false;
		}
	}
	iniparser_dump_ini(stash, file_ptr);

	free(stash_path);
	fclose(file_ptr);
	return true;
}

/* returned value has to be freed */
static char* stash_create_section_name(const char* section, const char* key) {
	char* section_name = malloc(strlen(section) + 1 + strlen(key) + 1);
	if(section_name != NULL) {
		sprintf(section_name, "%s:%s", section, key);
	}
	return section_name;
}

bool stash_set_string(const char* section, const char* key, const char* value) {
	char* section_name = stash_create_section_name(section, key);
	bool result = (dictionary_set(stash, section, NULL) == 0 && dictionary_set(stash, section_name, value) == 0);
	if(result) {
		stash_save();
	}

	free(section_name);
	return result;
}

bool stash_set_int(const char* section, const char* key, int value) {
	char* section_name = stash_create_section_name(section, key);
	char* string_value = malloc(ilogb(value) + 1 + 1);
	bool result = false;

	if(section_name != NULL && string_value != NULL) {
		sprintf(string_value, "%d", value);
		result = (dictionary_set(stash, section, NULL) == 0 && dictionary_set(stash, section_name, string_value) == 0);
		if(result) {
			stash_save();
		}
	}

	free(section_name);
	free(string_value);
	return result;
}

const char* stash_get_string(const char* section, const char* key, const char* default_value) {
	char* section_name = stash_create_section_name(section, key);
	const char* result = iniparser_getstring(stash, section_name, default_value);
	free(section_name);
	return result;
}

int stash_get_int(const char* section, const char* key, int default_value) {
	char* section_name = stash_create_section_name(section, key);
	int result = iniparser_getint(stash, section_name, default_value);
	free(section_name);
	return result;
}
