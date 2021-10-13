#include "Structures/Map.h"
#include "Dirs.h"
#include "Globals.h"
#include "Config.h"

#define CONFIG_GLOBAL_SECTION_NAME  "_global"
#define CONFIG_NAME_FIELD_NAME      "_name"
#define CONFIG_TYPE_FIELD_NAME      "module"
#define CONFIG_NAME_SEPARATOR       "."

bool configLoadInt(Map* config, char* key, int* output) {
	char* rawValueFromMap = getFromMap(config, key, strlen(key));
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
	char* rawValueFromMap = getFromMap(config, key, strlen(key));
	if(rawValueFromMap == NULL) {
		return false;
	}
	*output = strdup(rawValueFromMap);
	return true;
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
				char* value = getFromMap(sourceConfig, sourceKeys[i], strlen(sourceKeys[i]));
				putIntoMap(targetConfig, strdup(sourceKeys[i]), strlen(sourceKeys[i]), strdup(value));
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
	initMap(ret);

	putIntoMap(ret, strdup(CONFIG_NAME_FIELD_NAME), strlen(CONFIG_NAME_FIELD_NAME), strdup(sectionName));

	for(int i = 0; i < keyCount; i++) {
		if(keys[i][strlen(sectionName) + 1] == '_') {
			fprintf(stderr, "Option %s contains illegal prefix, ignoring\n", keys[i]);
			continue;
		}

		char* keyTrimmed = strdup(keys[i] + strlen(sectionName) + 1);
		const char* value = iniparser_getstring(config, keys[i], NULL);
		putIntoMap(ret, keyTrimmed, strlen(keyTrimmed), strdup(value));
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
		removeFromMap(section, keys[i], strlen(keys[i]), NULL, (void**)&valueToFree);
		free(valueToFree);
		free(keys[i]);
	}

	free(keys);
	destroyMap(section);
}

int configCompareSectionNames(const void* a, const void* b) {
	// sort so that special sections will be first
	const char** aString = (const char**)a;
	const char** bString = (const char**)b;
	return (**aString != '_') - (**bString != '_');
}

bool configLoad() {
	char* configDirectory = getConfigDirectory();
	dictionary* config = iniparser_load(configDirectory);
	free(configDirectory);

	if(config == NULL) {
		return false;
	}

	int configSectionCount = iniparser_getnsec(config);
	char** configSectionNames = malloc(configSectionCount * sizeof *configSectionNames);
	for(int i = 0; i < configSectionCount; i++) {
		configSectionNames[i] = strdup(iniparser_getsecname(config, i));
	}
	qsort(configSectionNames, configSectionCount, sizeof *configSectionNames, configCompareSectionNames);

	Map* specialSections = malloc(sizeof *specialSections); // map of section maps
	initMap(specialSections);

	for(int i = 0; i < configSectionCount; i++) {
		char* sectionName = configSectionNames[i];

		if(sectionName[0] == '_') { // special section
			Map* specialSection = configLoadSection(config, sectionName);
			putIntoMap(specialSections, sectionName, strlen(sectionName), specialSection);
		} else { // module section
			Map* configMap = configLoadSection(config, sectionName);
			char* moduleType = getFromMap(configMap, CONFIG_TYPE_FIELD_NAME, strlen(CONFIG_TYPE_FIELD_NAME));

			// add global settings for undefined values
			// priority: globalSection > global
			Map* configToMerge;

			// global section config
			char* globalSectionConfigSectionName = malloc(strlen(CONFIG_GLOBAL_SECTION_NAME) + strlen(CONFIG_NAME_SEPARATOR) + strlen(moduleType) + 1);
			sprintf(globalSectionConfigSectionName, "%s%s%s", CONFIG_GLOBAL_SECTION_NAME, CONFIG_NAME_SEPARATOR, moduleType);
			configToMerge = getFromMap(specialSections, globalSectionConfigSectionName, strlen(globalSectionConfigSectionName));
			free(globalSectionConfigSectionName);
			configFillEmptyFields(configMap, configToMerge);

			// global config
			configToMerge = getFromMap(specialSections, CONFIG_GLOBAL_SECTION_NAME, strlen(CONFIG_GLOBAL_SECTION_NAME));
			configFillEmptyFields(configMap, configToMerge);

			char* enabled = getFromMap(configMap, "enabled", strlen("enabled"));
			if(enabled != NULL && !strcmp(enabled, "false")) {
				free(sectionName);
			} else if(!enableModule(&moduleManager, moduleType, sectionName, configMap)) {
				fprintf(stderr, "Error while enabling module %s\n", sectionName);
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
		removeFromMap(specialSections, specialSectionNames[i], strlen(specialSectionNames[i]), NULL, (void**)&sectionToFree);
		configDestroySection(sectionToFree);
		free(sectionToFree);
		free(specialSectionNames[i]);
	}

	free(specialSectionNames);
	destroyMap(specialSections);
	free(specialSections);
	free(configSectionNames);
	iniparser_freedict(config);
	return true;
}
