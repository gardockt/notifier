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

int coreVerbosity = 0;

static dictionary* config = NULL;

bool configLoadInt(SortedMap* config, char* key, int* output) {
	char* rawValueFromMap = sortedMapGet(config, key);
	if(rawValueFromMap == NULL) {
		return false;
	}
	for(int i = 0; rawValueFromMap[i] != '\0'; i++) {
		if(!(isdigit(rawValueFromMap[i]) || (i == 0 && rawValueFromMap[i] == '-'))) {
			return false;
		}
	}
	*output = atoi(rawValueFromMap);
	return true;
}

bool configLoadString(SortedMap* config, char* key, char** output) {
	char* rawValueFromMap = sortedMapGet(config, key);
	if(rawValueFromMap == NULL) {
		return false;
	}
	*output = strdup(rawValueFromMap);
	return true;
}

bool configOpen() {
	char* configDirectory = get_config_file_path();
	config = iniparser_load(configDirectory);
	free(configDirectory);
	return config != NULL;
}

void configClose() {
	iniparser_freedict(config);
	config = NULL;
}

void configFillEmptyFields(SortedMap* targetConfig, SortedMap* sourceConfig) {
	if(targetConfig == NULL || sourceConfig == NULL) {
		return;
	}

	int targetKeyCount = sortedMapSize(targetConfig);
	int sourceKeyCount = sortedMapSize(sourceConfig);
	char** targetKeys = malloc(targetKeyCount * sizeof *targetKeys);
	char** sourceKeys = malloc(sourceKeyCount * sizeof *sourceKeys);

	sortedMapKeys(targetConfig, (void**)targetKeys);
	sortedMapKeys(sourceConfig, (void**)sourceKeys);

	for(int i = 0; i < sourceKeyCount; i++) {
		for(int j = 0; j <= targetKeyCount; j++) {
			if(j < targetKeyCount) {
				if(!strcmp(sourceKeys[i], targetKeys[j])) {
					break;
				}
			} else {
				char* value = sortedMapGet(sourceConfig, sourceKeys[i]);
				sortedMapPut(targetConfig, strdup(sourceKeys[i]), strdup(value));
			}
		}
	}

	free(targetKeys);
	free(sourceKeys);
}

SortedMap* configLoadSection(dictionary* config, char* sectionName) {
	int keyCount = iniparser_getsecnkeys(config, sectionName);
	if(keyCount == 0) {
		return NULL;
	}

	const char** keys = malloc(keyCount * sizeof *keys);
	iniparser_getseckeys(config, sectionName, keys);

	SortedMap* ret = malloc(sizeof *ret);
	sortedMapInit(ret, sortedMapCompareFunctionStrcmp);

	sortedMapPut(ret, strdup(CONFIG_NAME_FIELD_NAME), strdup(sectionName));

	for(int i = 0; i < keyCount; i++) {
		if(keys[i][strlen(sectionName) + 1] == '_') {
			continue;
		}

		char* keyTrimmed = strdup(keys[i] + strlen(sectionName) + 1);
		const char* value = iniparser_getstring(config, keys[i], NULL);
		sortedMapPut(ret, keyTrimmed, strdup(value));
	}

	free(keys);
	return ret;
}

void configDestroySection(SortedMap* section) {
	int keyCount = sortedMapSize(section);
	char** keys = malloc(keyCount * sizeof *keys);
	sortedMapKeys(section, (void**)keys);

	for(int i = 0; i < keyCount; i++) {
		char* valueToFree;
		sortedMapRemove(section, keys[i], NULL, (void**)&valueToFree);
		free(valueToFree);
		free(keys[i]);
	}

	free(keys);
	sortedMapDestroy(section);
}

void configLoadCore() {
	if(!configOpen()) {
		return;
	}

	SortedMap* coreSection = configLoadSection(config, CONFIG_CORE_SECTION_NAME);
	SortedMap* globalSection = configLoadSection(config, CONFIG_GLOBAL_SECTION_NAME);
	if(coreSection != NULL) {
		configFillEmptyFields(coreSection, globalSection);
	} else {
		coreSection = globalSection;
	}
	if(coreSection == NULL) {
		return;
	}

	configLoadInt(coreSection, "verbosity", &coreVerbosity);

	// do not close the config, configLoad will close it later
	if(globalSection != coreSection && globalSection != NULL) {
		configDestroySection(globalSection);
		free(globalSection);
	}
	configDestroySection(coreSection);
	free(coreSection);
}

int configCompareSectionNames(const void* a, const void* b) {
	// sort so that special sections will be first
	const char** aString = (const char**)a;
	const char** bString = (const char**)b;
	return (**aString != '_') - (**bString != '_');
}

bool configLoad() {
	if(config == NULL && !configOpen()) {
		return false;
	}

	int configSectionCount = iniparser_getnsec(config);
	char** configSectionNames = malloc(configSectionCount * sizeof *configSectionNames);
	for(int i = 0; i < configSectionCount; i++) {
		configSectionNames[i] = strdup(iniparser_getsecname(config, i));
	}
	qsort(configSectionNames, configSectionCount, sizeof *configSectionNames, configCompareSectionNames);

	SortedMap* specialSections = malloc(sizeof *specialSections); // map of section maps
	sortedMapInit(specialSections, sortedMapCompareFunctionStrcmp);

	SortedMap* globalSection = NULL;
	char* globalSectionConfigSectionName;
	for(int i = 0; i < configSectionCount; i++) {
		char* sectionName = configSectionNames[i];

		if(sectionName[0] == '_') { // special section
			SortedMap* specialSection = configLoadSection(config, sectionName);
			sortedMapPut(specialSections, sectionName, specialSection);
			if(!strcmp(sectionName, "_global")) {
				globalSection = specialSection;
			}
		} else { // module section
			SortedMap* configMap = configLoadSection(config, sectionName);
			char* moduleType = sortedMapGet(configMap, CONFIG_TYPE_FIELD_NAME);

			// add global settings for undefined values
			// priority: includes > globalSection > global
			SortedMap* configToMerge;

			// includes
			char* includesRaw;
			if(!configLoadString(configMap, "include", &includesRaw)) {
				goto loadConfigGlobalSection;
			}
			char** includeNames;
			int includeCount = split(includesRaw, LIST_ENTRY_SEPARATORS, &includeNames);
			for(int includeIndex = 0; includeIndex < includeCount; includeIndex++) {
				int includeNameLength = strlen(includeNames[includeIndex]);
				char* includeSectionName = malloc(strlen(CONFIG_INCLUDE_SECTION_NAME) + strlen(CONFIG_NAME_SEPARATOR) + includeNameLength + 1);
				sprintf(includeSectionName, "%s%s%s", CONFIG_INCLUDE_SECTION_NAME, CONFIG_NAME_SEPARATOR, includeNames[includeIndex]);
				configToMerge = sortedMapGet(specialSections, includeSectionName);
				if(configToMerge != NULL) {
					configFillEmptyFields(configMap, configToMerge);
				} else {
					logWrite("core", coreVerbosity, 1, "Include section %s not found for module %s", includeNames[includeIndex], sectionName);
				}

				free(includeSectionName);
				free(includeNames[includeIndex]);
			}
			free(includeNames);
			free(includesRaw);

			// global section config
loadConfigGlobalSection:
			globalSectionConfigSectionName = malloc(strlen(CONFIG_GLOBAL_SECTION_NAME) + strlen(CONFIG_NAME_SEPARATOR) + strlen(moduleType) + 1);
			sprintf(globalSectionConfigSectionName, "%s%s%s", CONFIG_GLOBAL_SECTION_NAME, CONFIG_NAME_SEPARATOR, moduleType);
			configToMerge = sortedMapGet(specialSections, globalSectionConfigSectionName);
			free(globalSectionConfigSectionName);
			configFillEmptyFields(configMap, configToMerge);

			// global config
			configFillEmptyFields(configMap, globalSection);

			char* enabled = sortedMapGet(configMap, "enabled");
			if(enabled != NULL && !strcmp(enabled, "false")) {
				free(sectionName);
			} else if(!fm_enable(&moduleManager, moduleType, sectionName, configMap)) {
				logWrite("core", coreVerbosity, 0, "Error while enabling module %s", sectionName);
				free(sectionName);
			}

			configDestroySection(configMap);
			free(configMap);
		}
	}

	int specialSectionCount = sortedMapSize(specialSections);
	char** specialSectionNames = malloc(specialSectionCount * sizeof *specialSectionNames);
	sortedMapKeys(specialSections, (void**)specialSectionNames);
	for(int i = 0; i < specialSectionCount; i++) {
		SortedMap* sectionToFree;
		sortedMapRemove(specialSections, specialSectionNames[i], NULL, (void**)&sectionToFree);
		configDestroySection(sectionToFree);
		free(sectionToFree);
		free(specialSectionNames[i]);
	}

	free(specialSectionNames);
	sortedMapDestroy(specialSections);
	free(specialSections);
	free(configSectionNames);
	configClose();
	return true;
}
