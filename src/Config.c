#include "Structures/Map.h"
#include "Dirs.h"
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

bool configLoadInt(Map* config, char* key, int* output) {
	char* rawValueFromMap = getFromMap(config, key);
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

bool configLoadString(Map* config, char* key, char** output) {
	char* rawValueFromMap = getFromMap(config, key);
	if(rawValueFromMap == NULL) {
		return false;
	}
	*output = strdup(rawValueFromMap);
	return true;
}

bool configOpen() {
	char* configDirectory = getConfigDirectory();
	config = iniparser_load(configDirectory);
	free(configDirectory);
	return config != NULL;
}

void configClose() {
	iniparser_freedict(config);
	config = NULL;
}

void configFillEmptyFields(Map* targetConfig, Map* sourceConfig) {
	if(targetConfig == NULL || sourceConfig == NULL) {
		return;
	}

	int targetKeyCount = getMapSize(targetConfig);
	int sourceKeyCount = getMapSize(sourceConfig);
	char** targetKeys = malloc(targetKeyCount * sizeof *targetKeys);
	char** sourceKeys = malloc(sourceKeyCount * sizeof *sourceKeys);

	getMapKeys(targetConfig, (void**)targetKeys);
	getMapKeys(sourceConfig, (void**)sourceKeys);

	for(int i = 0; i < sourceKeyCount; i++) {
		for(int j = 0; j <= targetKeyCount; j++) {
			if(j < targetKeyCount) {
				if(!strcmp(sourceKeys[i], targetKeys[j])) {
					break;
				}
			} else {
				char* value = getFromMap(sourceConfig, sourceKeys[i]);
				putIntoMap(targetConfig, strdup(sourceKeys[i]), strdup(value));
			}
		}
	}

	free(targetKeys);
	free(sourceKeys);
}

Map* configLoadSection(dictionary* config, char* sectionName) {
	int keyCount = iniparser_getsecnkeys(config, sectionName);
	if(keyCount == 0) {
		return NULL;
	}

	const char** keys = malloc(keyCount * sizeof *keys);
	iniparser_getseckeys(config, sectionName, keys);

	Map* ret = malloc(sizeof *ret);
	initMap(ret, mapCompareFunctionStrcmp);

	putIntoMap(ret, strdup(CONFIG_NAME_FIELD_NAME), strdup(sectionName));

	for(int i = 0; i < keyCount; i++) {
		if(keys[i][strlen(sectionName) + 1] == '_') {
			continue;
		}

		char* keyTrimmed = strdup(keys[i] + strlen(sectionName) + 1);
		const char* value = iniparser_getstring(config, keys[i], NULL);
		putIntoMap(ret, keyTrimmed, strdup(value));
	}

	free(keys);
	return ret;
}

void configDestroySection(Map* section) {
	int keyCount = getMapSize(section);
	char** keys = malloc(keyCount * sizeof *keys);
	getMapKeys(section, (void**)keys);

	for(int i = 0; i < keyCount; i++) {
		char* valueToFree;
		removeFromMap(section, keys[i], NULL, (void**)&valueToFree);
		free(valueToFree);
		free(keys[i]);
	}

	free(keys);
	destroyMap(section);
}

void configLoadCore() {
	if(!configOpen()) {
		return;
	}

	Map* coreSection = configLoadSection(config, CONFIG_CORE_SECTION_NAME);
	Map* globalSection = configLoadSection(config, CONFIG_GLOBAL_SECTION_NAME);
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
	if(globalSection != coreSection) {
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

	Map* specialSections = malloc(sizeof *specialSections); // map of section maps
	initMap(specialSections, mapCompareFunctionStrcmp);

	Map* globalSection = NULL;
	char* globalSectionConfigSectionName;
	for(int i = 0; i < configSectionCount; i++) {
		char* sectionName = configSectionNames[i];

		if(sectionName[0] == '_') { // special section
			Map* specialSection = configLoadSection(config, sectionName);
			putIntoMap(specialSections, sectionName, specialSection);
			if(!strcmp(sectionName, "_global")) {
				globalSection = specialSection;
			}
		} else { // module section
			Map* configMap = configLoadSection(config, sectionName);
			char* moduleType = getFromMap(configMap, CONFIG_TYPE_FIELD_NAME);

			// add global settings for undefined values
			// priority: includes > globalSection > global
			Map* configToMerge;

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
				configToMerge = getFromMap(specialSections, includeSectionName);
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
			configToMerge = getFromMap(specialSections, globalSectionConfigSectionName);
			free(globalSectionConfigSectionName);
			configFillEmptyFields(configMap, configToMerge);

			// global config
			configFillEmptyFields(configMap, globalSection);

			char* enabled = getFromMap(configMap, "enabled");
			if(enabled != NULL && !strcmp(enabled, "false")) {
				free(sectionName);
			} else if(!enableModule(&moduleManager, moduleType, sectionName, configMap)) {
				logWrite("core", coreVerbosity, 0, "Error while enabling module %s", sectionName);
				free(sectionName);
			}

			configDestroySection(configMap);
			free(configMap);
		}
	}

	int specialSectionCount = getMapSize(specialSections);
	char** specialSectionNames = malloc(specialSectionCount * sizeof *specialSectionNames);
	getMapKeys(specialSections, (void**)specialSectionNames);
	for(int i = 0; i < specialSectionCount; i++) {
		Map* sectionToFree;
		removeFromMap(specialSections, specialSectionNames[i], NULL, (void**)&sectionToFree);
		configDestroySection(sectionToFree);
		free(sectionToFree);
		free(specialSectionNames[i]);
	}

	free(specialSectionNames);
	destroyMap(specialSections);
	free(specialSections);
	free(configSectionNames);
	configClose();
	return true;
}
