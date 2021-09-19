#ifdef ENABLE_MODULE_ISOD

#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "Extras/FetchingModuleUtilities.h"
#include "Isod.h"

// TODO: replace stash identifiers with module custom name

#define JSON_STRING(obj, str) json_object_get_string(json_object_object_get((obj),(str)))
#define SORTDATE(date) {date[6], date[7], date[8], date[9], date[3], date[4], date[0], date[1], date[11], date[12], date[14], date[15], '\0'}

typedef struct {
	const char* title;
} IsodNotificationData;

int isodCompareDates(char* a, char* b) {
	// sort dates to YYYYMMDDHHmm so we can compare them with strcmp
	char aSorted[] = SORTDATE(a);
	char bSorted[] = SORTDATE(b);
	return strcmp(aSorted, bSorted);
}

char* isodGenerateUrl(FetchingModule* fetchingModule) {
	IsodConfig* config = fetchingModule->config;
	char* url = malloc(strlen("https://isod.ee.pw.edu.pl/isod-portal/wapi?q=mynewsheaders&username=") + strlen(config->username) + strlen("&apikey=") + strlen(config->token) + strlen("&from=0&to=") + strlen(config->maxMessages) + 1);
	sprintf(url, "https://isod.ee.pw.edu.pl/isod-portal/wapi?q=mynewsheaders&username=%s&apikey=%s&from=0&to=%s", config->username, config->token, config->maxMessages);
	return url;
}

char* isodGenerateLastReadKeyName(FetchingModule* fetchingModule) {
	IsodConfig* config = fetchingModule->config;
	char* sectionName = malloc(strlen("last_read_") + strlen(config->username) + 1);
	sprintf(sectionName, "last_read_%s", config->username);
	return sectionName;
}

bool isodParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	IsodConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	if(!moduleLoadBasicSettings(fetchingModule, configToParse) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "username", &config->username) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "token", &config->token) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "max_messages", &config->maxMessages)) {
		return false;
	}

	char* sectionName = isodGenerateLastReadKeyName(fetchingModule);
	config->lastRead = strdup(stashGetString("isod", sectionName, "01.01.1970 00:00"));
	free(sectionName);
	
	return true;
}

bool isodEnable(FetchingModule* fetchingModule, Map* configToParse) {
	if(!isodParseConfig(fetchingModule, configToParse)) {
		return false;
	}

	IsodConfig* config = fetchingModule->config;
	bool retVal = (config->curl = curl_easy_init()) != NULL;
	char* url = isodGenerateUrl(fetchingModule);

	if(retVal) {
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, networkCallback);
		curl_easy_setopt(config->curl, CURLOPT_URL, url);

		retVal = fetchingModuleCreateThread(fetchingModule);
		moduleLog(fetchingModule, 1, "Module enabled");
	}

	free(url);
	return retVal;
}

char* isodReplaceVariables(char* text, IsodNotificationData* notificationData) {
	return replace(text, "<title>", notificationData->title);
}

void isodDisplayNotification(FetchingModule* fetchingModule, IsodNotificationData* notificationData) {
	Message* message = malloc(sizeof *message);

	bzero(message, sizeof *message);
	moduleFillBasicMessage(fetchingModule, message, isodReplaceVariables, notificationData);
	fetchingModule->display->displayMessage(message, defaultMessageFreeFunction);
}

void isodFetch(FetchingModule* fetchingModule) {
	IsodConfig* config = fetchingModule->config;
	IsodNotificationData notificationData;
	Message message;
	NetworkResponse response = {NULL, 0};

	// getting response
	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	CURLcode code = curl_easy_perform(config->curl);

	// parsing response
	if(code == CURLE_OK) {
		json_object* root = json_tokener_parse(response.data);
		json_object* items = json_object_object_get(root, "items");
		if(json_object_get_type(items) == json_type_array) {
			int notificationCount = json_object_array_length(items);
			char* newLastRead = NULL;

			for(int i = notificationCount - 1; i >= 0; i--) {
				json_object* notification = json_object_array_get_idx(items, i);
				const char* modifiedDate = JSON_STRING(notification, "modifiedDate");
				char* modifiedDateWithFixedFormat;
				if(modifiedDate[1] == '.') { // day is one digit long
					modifiedDateWithFixedFormat = malloc(strlen("01.01.1970 00:00") + 1);
					sprintf(modifiedDateWithFixedFormat, "0%s", modifiedDate);
				} else { // for consistent freeing
					modifiedDateWithFixedFormat = strdup(modifiedDate);
				}

				if(isodCompareDates(modifiedDateWithFixedFormat, config->lastRead) > 0) {
					if(newLastRead == NULL || isodCompareDates(modifiedDateWithFixedFormat, newLastRead) > 0) {
						free(newLastRead);
						newLastRead = strdup(modifiedDateWithFixedFormat);
					}

					notificationData.title = JSON_STRING(notification, "subject");
					isodDisplayNotification(fetchingModule, &notificationData);
				}

				free(modifiedDateWithFixedFormat);
			}
			json_object_put(root);
			if(newLastRead != NULL) {
				config->lastRead = newLastRead;
				char* sectionName = isodGenerateLastReadKeyName(fetchingModule);
				stashSetString("isod", sectionName, config->lastRead);
				free(sectionName);
			}
		} else {
			moduleLog(fetchingModule, 0, "Invalid response:\n%s", response.data);
		}
	} else {
		moduleLog(fetchingModule, 0, "Request failed with code %d", code);
	}
	free(response.data);
}

bool isodDisable(FetchingModule* fetchingModule) {
	IsodConfig* config = fetchingModule->config;

	curl_easy_cleanup(config->curl);

	bool retVal = fetchingModuleDestroyThread(fetchingModule);

	if(retVal) {
		moduleLog(fetchingModule, 1, "Module disabled");
		moduleFreeBasicSettings(fetchingModule);
		free(config->maxMessages);
		free(config->lastRead);
		free(config->username);
		free(config->token);
		free(config);
	}

	return retVal;
}

bool isodTemplate(FetchingModule* fetchingModule) {
	bzero(fetchingModule, sizeof *fetchingModule);

	fetchingModule->enable = isodEnable;
	fetchingModule->fetch = isodFetch;
	fetchingModule->disable = isodDisable;

	return true;
}

#endif // ifdef ENABLE_MODULE_ISOD
