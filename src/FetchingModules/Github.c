#include "Github.h"
#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"

#define JSON_STRING(obj, str) json_object_get_string(json_object_object_get((obj),(str)))

typedef struct {
	char* data;
	int size;
} Memory;

typedef struct {
	char* title;
	char* repoName;
	char* repoFullName;
} GithubNotificationData;

int githubCompareDates(char* a, char* b) {
	return strcmp(a, b);
}

size_t githubCurlCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	Memory* memory = (Memory*)userdata;
	int sizeBefore = memory->size;
	int sizeAdded = size * nmemb;

	memory->size = sizeBefore + sizeAdded;
	memory->data = realloc(memory->data, memory->size);
	strncpy(&memory->data[sizeBefore], ptr, sizeAdded);

	return sizeAdded;
}

bool githubParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	// TODO: move interval parsing to enableModule?

	GithubConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	char* title = getFromMap(configToParse, "title", strlen("title"));
	char* body = getFromMap(configToParse, "body", strlen("body"));
	char* token = getFromMap(configToParse, "token", strlen("token"));
	char* interval = getFromMap(configToParse, "interval", strlen("interval"));

	if(title == NULL || body == NULL || token == NULL || interval == NULL) {
		destroyMap(configToParse);
		return false;
	}

	config->title = title;
	config->body = body;
	config->token = token;
	fetchingModule->intervalSecs = atoi(interval);

	config->list = NULL;

	char* sectionName = malloc(strlen("last_read_") + strlen(config->token) + 1);
	sprintf(sectionName, "last_read_%s", config->token);
	config->lastRead = stashGetString("github", sectionName, "1970-01-01T00:00:00Z");
	config->lastRead = strdup(config->lastRead);
	free(sectionName);
	
	int keyCount = getMapSize(configToParse);
	char** keys = malloc(keyCount * sizeof *keys);
	getMapKeys(configToParse, keys);

	for(int i = 0; i < keyCount; i++) {
		char* valueToFree;
		removeFromMap(configToParse, keys[i], strlen(keys[i]), NULL, &valueToFree); // key is already in keys[i]
		if(strcmp(keys[i], "title") != 0 &&
		   strcmp(keys[i], "body") != 0 &&
		   strcmp(keys[i], "token") != 0) {
			free(valueToFree);
		}
		free(keys[i]);
	}
	
	free(keys);
	destroyMap(configToParse);

	return true;
}

bool githubEnable(FetchingModule* fetchingModule) {
	GithubConfig* config = fetchingModule->config;
	bool retVal = (config->curl = curl_easy_init()) != NULL;

	char* headerUserAgent = malloc(strlen("User-Agent: curl") + 1);
	char* headerToken = malloc(strlen("Authorization: token ") + strlen(config->token) + 1);

	sprintf(headerUserAgent, "User-Agent: curl");
	sprintf(headerToken, "Authorization: token %s", config->token);

	if(retVal) {
		config->list = curl_slist_append(config->list, headerUserAgent);
		config->list = curl_slist_append(config->list, headerToken);

		curl_easy_setopt(config->curl, CURLOPT_HTTPHEADER, config->list);
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, githubCurlCallback);
		curl_easy_setopt(config->curl, CURLOPT_URL, "https://api.github.com/notifications");

		retVal = fetchingModuleCreateThread(fetchingModule);
		printf("Github enabled\n");
	}

	free(headerUserAgent);
	free(headerToken);
	return retVal;
}

char* githubReplaceVariables(char* text, GithubNotificationData* notificationData) {
	char* temp;
	char* ret;

	temp = replace(text, "<title>", notificationData->title);
	ret = replace(temp, "<repo-name>", notificationData->repoName);
	free(temp);
	temp = ret;
	ret = replace(temp, "<repo-full-name>", notificationData->repoFullName);
	free(temp);
	return ret;
}

void githubDisplayNotification(FetchingModule* fetchingModule, GithubNotificationData* notificationData) {
	GithubConfig* config = fetchingModule->config;
	Message message;

	message.title = githubReplaceVariables(config->title, notificationData);
	message.text = githubReplaceVariables(config->body, notificationData);
	fetchingModule->display->displayMessage(&message);
	free(message.title);
	free(message.text);
}

void githubFetch(FetchingModule* fetchingModule) {
	GithubConfig* config = fetchingModule->config;
	GithubNotificationData notificationData;
	Message message;
	Memory response = {NULL, 0};

	// getting response
	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	CURLcode code = curl_easy_perform(config->curl);

	// parsing response
	if(code == CURLE_OK) {
		json_object* root = json_tokener_parse(response.data);
		int unreadNotifications = json_object_array_length(root);
		char* newLastRead = NULL;

		for(int i = 0; i < unreadNotifications; i++) {
			json_object* notification = json_object_array_get_idx(root, i);
			char* lastUpdated = JSON_STRING(notification, "updated_at");

			if(githubCompareDates(lastUpdated, config->lastRead) > 0) {
				if(newLastRead != NULL) {
					free(newLastRead);
				}
				newLastRead = strdup(lastUpdated);

				json_object* subject    = json_object_object_get(notification, "subject");
				json_object* repository = json_object_object_get(notification, "repository");
				notificationData.title        = JSON_STRING(subject,    "title");
				notificationData.repoName     = JSON_STRING(repository, "name");
				notificationData.repoFullName = JSON_STRING(repository, "full_name");
				githubDisplayNotification(fetchingModule, &notificationData);
			}

			json_object_put(root);
		}
		if(newLastRead != NULL) {
			config->lastRead = newLastRead;
			char* sectionName = malloc(strlen("last_read_") + strlen(config->token) + 1);
			sprintf(sectionName, "last_read_%s", config->token);
			stashSetString("github", sectionName, config->lastRead);
			free(sectionName);
		}
	} else {
		fprintf(stderr, "[GitHub] Request failed with code %d: %s\n", code, response.data);
	}
	free(response.data);
}

bool githubDisable(FetchingModule* fetchingModule) {
	GithubConfig* config = fetchingModule->config;

	curl_slist_free_all(config->list);
	curl_easy_cleanup(config->curl);

	bool retVal = fetchingModuleDestroyThread(fetchingModule);

	if(retVal) {
		free(config->lastRead);
		free(config->title);
		free(config->body);
		free(config->token);
		free(config);
		printf("Github disabled\n");
	}

	return retVal;
}

bool githubTemplate(FetchingModule* fetchingModule) {
	fetchingModule->intervalSecs = 1;
	fetchingModule->config = NULL;
	fetchingModule->busy = false;
	fetchingModule->parseConfig = githubParseConfig;
	fetchingModule->enable = githubEnable;
	fetchingModule->fetch = githubFetch;
	fetchingModule->display = NULL;
	fetchingModule->disable = githubDisable;

	return true;
}
