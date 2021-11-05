#ifdef ENABLE_MODULE_ISOD

#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "Utilities/FetchingModuleUtilities.h"
#include "Utilities/Json.h"
#include "Isod.h"

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

	if(!moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "username", &config->username) ||
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
	}

	free(url);
	return retVal;
}

char* isodReplaceVariables(char* text, void* notificationDataPtr) {
	IsodNotificationData* notificationData = notificationDataPtr;
	return replace(text, 1, "<title>", notificationData->title);
}

void isodDisplayNotification(FetchingModule* fetchingModule, IsodNotificationData* notificationData) {
	Message message = {0};
	moduleFillBasicMessage(fetchingModule, &message, isodReplaceVariables, notificationData, NONE, NULL);
	fetchingModule->display->displayMessage(&message);
	moduleDestroyBasicMessage(&message);
}

void isodParseResponse(FetchingModule* fetchingModule, char* response) {
	IsodConfig* config = fetchingModule->config;
	json_object* root = json_tokener_parse(response);
	json_object* items;

	if(!jsonReadArray(root, "items", &items)) {
		moduleLog(fetchingModule, 0, "Invalid response");
		return; // TODO: memory leak?
	}

	int notificationCount = json_object_array_length(items);
	char* newLastRead = NULL;

	for(int i = notificationCount - 1; i >= 0; i--) {
		json_object* notification = json_object_array_get_idx(items, i);
		if(json_object_get_type(notification) != json_type_object) {
			moduleLog(fetchingModule, 0, "Invalid notification object");
			continue;
		}

		const char* modifiedDate;
		char* modifiedDateWithFixedFormat;

		if(!jsonReadString(notification, "modifiedDate", &modifiedDate)) {
			moduleLog(fetchingModule, 0, "Invalid modification date in notification %d", i);
			continue;
		}

		if(modifiedDate[1] == '.') { // day is one digit long
			modifiedDateWithFixedFormat = malloc(strlen("01.01.1970 00:00") + 1);
			sprintf(modifiedDateWithFixedFormat, "0%s", modifiedDate);
		} else {
			modifiedDateWithFixedFormat = strdup(modifiedDate);
		}

		if(isodCompareDates(modifiedDateWithFixedFormat, config->lastRead) > 0) {
			if(newLastRead == NULL || isodCompareDates(modifiedDateWithFixedFormat, newLastRead) > 0) {
				free(newLastRead);
				newLastRead = strdup(modifiedDateWithFixedFormat);
			}

			IsodNotificationData notificationData = {0};
			if(jsonReadString(notification, "subject", &notificationData.title)) {
				isodDisplayNotification(fetchingModule, &notificationData);
			} else {
				moduleLog(fetchingModule, 0, "Invalid title in notification %d", i);
			}
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
}

void isodFetch(FetchingModule* fetchingModule) {
	IsodConfig* config = fetchingModule->config;
	NetworkResponse response = {NULL, 0};

	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	CURLcode code = curl_easy_perform(config->curl);

	if(code == CURLE_OK) {
		moduleLog(fetchingModule, 3, "Received response:\n%s", response.data);
		isodParseResponse(fetchingModule, response.data);
		free(response.data);
	} else {
		moduleLog(fetchingModule, 0, "Request failed with code %d", code);
	}
}

void isodDisable(FetchingModule* fetchingModule) {
	IsodConfig* config = fetchingModule->config;

	curl_easy_cleanup(config->curl);

	free(config->maxMessages);
	free(config->lastRead);
	free(config->username);
	free(config->token);
	free(config);
}

void isodTemplate(FetchingModule* fetchingModule) {
	fetchingModule->enable = isodEnable;
	fetchingModule->fetch = isodFetch;
	fetchingModule->disable = isodDisable;
}

#endif // ifdef ENABLE_MODULE_ISOD
