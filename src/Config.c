#include "Structures/SortedMap.h"
#include "Paths.h"
#include "Globals.h"
#include "StringOperations.h"
#include "Log.h"
#include "Config.h"

#define CONFIG_GLOBAL_SECTION_NAME   "_global"
#define CONFIG_INCLUDE_SECTION_NAME  "_include"
#define CONFIG_CORE_SECTION_NAME     "_core"
#define CONFIG_NAME_FIELD_NAME       "_name"
#define CONFIG_TYPE_FIELD_NAME       "module"
#define CONFIG_NAME_SEPARATOR        "."

int core_verbosity = 0;

static dictionary* config = NULL;

bool config_load_int(SortedMap* config, char* key, int* output) {
	char* raw_value_from_map = sorted_map_get(config, key);
	if(raw_value_from_map == NULL) {
		return false;
	}
	for(int i = 0; raw_value_from_map[i] != '\0'; i++) {
		if(!(isdigit(raw_value_from_map[i]) || (i == 0 && raw_value_from_map[i] == '-'))) {
			return false;
		}
	}
	*output = atoi(raw_value_from_map);
	return true;
}

bool config_load_string(SortedMap* config, char* key, char** output) {
	char* raw_value_from_map = sorted_map_get(config, key);
	if(raw_value_from_map == NULL) {
		return false;
	}
	*output = strdup(raw_value_from_map);
	return true;
}

static bool config_open() {
	char* config_path = get_config_file_path();
	config = iniparser_load(config_path);
	free(config_path);
	return config != NULL;
}

static void config_close() {
	iniparser_freedict(config);
	config = NULL;
}

static void config_fill_empty_fields(SortedMap* target_config, SortedMap* source_config) {
	if(target_config == NULL || source_config == NULL) {
		return;
	}

	int target_key_count = sorted_map_size(target_config);
	int source_key_count = sorted_map_size(source_config);
	char** target_keys = malloc(target_key_count * sizeof *target_keys);
	char** source_keys = malloc(source_key_count * sizeof *source_keys);

	sorted_map_keys(target_config, (void**)target_keys);
	sorted_map_keys(source_config, (void**)source_keys);

	for(int i = 0; i < source_key_count; i++) {
		for(int j = 0; j <= target_key_count; j++) {
			if(j < target_key_count) {
				if(!strcmp(source_keys[i], target_keys[j])) {
					break;
				}
			} else {
				char* value = sorted_map_get(source_config, source_keys[i]);
				sorted_map_put(target_config, strdup(source_keys[i]), strdup(value));
			}
		}
	}

	free(target_keys);
	free(source_keys);
}

static SortedMap* config_load_section(dictionary* config, char* section_name) {
	int key_count = iniparser_getsecnkeys(config, section_name);
	if(key_count == 0) {
		return NULL;
	}

	const char** keys = malloc(key_count * sizeof *keys);
	iniparser_getseckeys(config, section_name, keys);

	SortedMap* ret = malloc(sizeof *ret);
	sorted_map_init(ret, (int (*)(const void*, const void*))strcmp);

	sorted_map_put(ret, strdup(CONFIG_NAME_FIELD_NAME), strdup(section_name));

	for(int i = 0; i < key_count; i++) {
		if(keys[i][strlen(section_name) + 1] == '_') {
			continue;
		}

		char* key_trimmed = strdup(keys[i] + strlen(section_name) + 1);
		const char* value = iniparser_getstring(config, keys[i], NULL);
		sorted_map_put(ret, key_trimmed, strdup(value));
	}

	free(keys);
	return ret;
}

static void config_destroy_section(SortedMap* section) {
	int key_count = sorted_map_size(section);
	char** keys = malloc(key_count * sizeof *keys);
	sorted_map_keys(section, (void**)keys);

	for(int i = 0; i < key_count; i++) {
		char* value_to_free;
		sorted_map_remove(section, keys[i], NULL, (void**)&value_to_free);
		free(value_to_free);
		free(keys[i]);
	}

	free(keys);
	sorted_map_destroy(section);
}

void config_load_core() {
	if(!config_open()) {
		return;
	}

	SortedMap* core_section = config_load_section(config, CONFIG_CORE_SECTION_NAME);
	SortedMap* global_section = config_load_section(config, CONFIG_GLOBAL_SECTION_NAME);
	if(core_section != NULL) {
		config_fill_empty_fields(core_section, global_section);
	} else {
		core_section = global_section;
	}
	if(core_section == NULL) {
		return;
	}

	config_load_int(core_section, "verbosity", &core_verbosity);

	/* do not close the config, config_load will close it later */
	if(global_section != core_section && global_section != NULL) {
		config_destroy_section(global_section);
		free(global_section);
	}
	config_destroy_section(core_section);
	free(core_section);
}

static int config_compare_section_names(const void* a, const void* b) {
	/* sort so that special sections will be first */
	const char** a_string = (const char**)a;
	const char** b_string = (const char**)b;
	return (**a_string != '_') - (**b_string != '_');
}

bool config_load() {
	if(config == NULL && !config_open()) {
		return false;
	}

	int config_section_count = iniparser_getnsec(config);
	char** config_section_names = malloc(config_section_count * sizeof *config_section_names);
	for(int i = 0; i < config_section_count; i++) {
		config_section_names[i] = strdup(iniparser_getsecname(config, i));
	}
	qsort(config_section_names, config_section_count, sizeof *config_section_names, config_compare_section_names);

	SortedMap* special_sections = malloc(sizeof *special_sections); /* map of section maps */
	sorted_map_init(special_sections, (int (*)(const void*, const void*))strcmp);

	SortedMap* global_section = NULL;
	char* global_section_config_section_name;
	for(int i = 0; i < config_section_count; i++) {
		char* section_name = config_section_names[i];

		if(section_name[0] == '_') { /* special section */
			SortedMap* special_section = config_load_section(config, section_name);
			sorted_map_put(special_sections, section_name, special_section);
			if(!strcmp(section_name, "_global")) {
				global_section = special_section;
			}
		} else { /* module section */
			SortedMap* config_map = config_load_section(config, section_name);
			char* module_type = sorted_map_get(config_map, CONFIG_TYPE_FIELD_NAME);

			/* add global settings for undefined values    *
			 * priority: includes > globalSection > global */
			SortedMap* config_to_merge;

			/* includes */
			char* includes_raw;
			if(!config_load_string(config_map, "include", &includes_raw)) {
				goto config_load_global_section;
			}
			char** include_names;
			int include_count = split(includes_raw, LIST_ENTRY_SEPARATORS, &include_names);
			for(int include_index = 0; include_index < include_count; include_index++) {
				int include_name_length = strlen(include_names[include_index]);
				char* include_section_name = malloc(strlen(CONFIG_INCLUDE_SECTION_NAME) + strlen(CONFIG_NAME_SEPARATOR) + include_name_length + 1);
				sprintf(include_section_name, "%s%s%s", CONFIG_INCLUDE_SECTION_NAME, CONFIG_NAME_SEPARATOR, include_names[include_index]);
				config_to_merge = sorted_map_get(special_sections, include_section_name);
				if(config_to_merge != NULL) {
					config_fill_empty_fields(config_map, config_to_merge);
				} else {
					log_write("core", core_verbosity, 1, "Include section %s not found for module %s", include_names[include_index], section_name);
				}

				free(include_section_name);
				free(include_names[include_index]);
			}
			free(include_names);
			free(includes_raw);

			/* global section config */
config_load_global_section:
			global_section_config_section_name = malloc(strlen(CONFIG_GLOBAL_SECTION_NAME) + strlen(CONFIG_NAME_SEPARATOR) + strlen(module_type) + 1);
			sprintf(global_section_config_section_name, "%s%s%s", CONFIG_GLOBAL_SECTION_NAME, CONFIG_NAME_SEPARATOR, module_type);
			config_to_merge = sorted_map_get(special_sections, global_section_config_section_name);
			free(global_section_config_section_name);
			config_fill_empty_fields(config_map, config_to_merge);

			/* global config */
			config_fill_empty_fields(config_map, global_section);

			char* enabled = sorted_map_get(config_map, "enabled");
			if(enabled != NULL && !strcmp(enabled, "false")) {
				free(section_name);
			} else if(!fm_enable(&module_manager, module_type, section_name, config_map)) {
				log_write("core", core_verbosity, 0, "Error while enabling module %s", section_name);
				free(section_name);
			}

			config_destroy_section(config_map);
			free(config_map);
		}
	}

	int special_section_count = sorted_map_size(special_sections);
	char** special_section_names = malloc(special_section_count * sizeof *special_section_names);
	sorted_map_keys(special_sections, (void**)special_section_names);
	for(int i = 0; i < special_section_count; i++) {
		SortedMap* section_to_free;
		sorted_map_remove(special_sections, special_section_names[i], NULL, (void**)&section_to_free);
		config_destroy_section(section_to_free);
		free(section_to_free);
		free(special_section_names[i]);
	}

	free(special_section_names);
	sorted_map_destroy(special_sections);
	free(special_sections);
	free(config_section_names);
	config_close();
	return true;
}
