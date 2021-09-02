#include "../../GlobalManagers.h"
#include "../../Displays/Display.h"
#include "FetchingModuleUtilities.h"

bool moduleLoadIntFromConfig(FetchingModule* fetchingModule, Map* config, char* key, int* output) {
	char* rawValueFromMap = getFromMap(config, key, strlen(key));
	if(rawValueFromMap == NULL) {
		return false;
	}
	*output = atoi(rawValueFromMap);
	return true;
}

bool moduleLoadStringFromConfig(FetchingModule* fetchingModule, Map* config, char* key, char** output) {
	char* rawValueFromMap = getFromMap(config, key, strlen(key));
	if(rawValueFromMap == NULL) {
		return false;
	}
	*output = strdup(rawValueFromMap);
	return true;
}

// loads interval and display
bool moduleLoadBasicSettings(FetchingModule* fetchingModule, Map* config) {
	if(!moduleLoadIntFromConfig(fetchingModule, config, "interval", &fetchingModule->intervalSecs)) {
		return false;
	}

	char* displayName = getFromMap(config, "display", strlen("display"));
	if(displayName == NULL) {
		return false;
	}
	fetchingModule->display = getDisplay(&displayManager, displayName);
	if(fetchingModule->display == NULL) {
		return false;
	}

	return true;
}
