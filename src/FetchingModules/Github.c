#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "Github.h"

#define JSON_STRING(obj, str) json_object_get_string(json_object_object_get((obj),(str)))

typedef struct {
	char* title;
	char* repoName;
	char* repoFullName;
	json_object* notificationObject;
} GithubNotificationData;

int githubCompareDates(char* a, char* b) {
	return strcmp(a, b);
}

char* githubGenerateNotificationUrl(json_object* notification) {
	// TODO: only issue/PR support, implement other notifications

	json_object* subject = json_object_object_get(notification, "subject");
	if(json_object_get_type(subject) != json_type_object) {
		return NULL;
	}
	json_object* baseUrlObject = json_object_object_get(subject, "url");
	if(json_object_get_type(baseUrlObject) != json_type_string) {
		return NULL;
	}
	json_object* commentUrlObject = json_object_object_get(subject, "latest_comment_url");
	if(json_object_get_type(commentUrlObject) != json_type_string) {
		return NULL;
	}

	char* baseUrl = json_object_get_string(baseUrlObject);
	char* commentUrl = json_object_get_string(commentUrlObject);

	char* baseUrlBody = baseUrl + strlen("https://api.github.com/repos/");
	char* commentId = commentUrl + strlen("https://api.github.com/repos/");

	int commentIdPointer;
	do {
		commentIdPointer = 0;
		for(; commentId[commentIdPointer] != '\0'; commentIdPointer++) {
			if(!isdigit(commentId[commentIdPointer])) {
				commentId++;
				break;
			}
		}
	} while(commentId[commentIdPointer] != '\0');

	char* url = malloc(strlen("https://github.com/") + strlen(baseUrlBody) + strlen("#issuecomment-") + strlen(commentId) + 1);
	sprintf(url, "https://github.com/%s#issuecomment-%s", baseUrlBody, commentId);
	return url;
}

bool githubParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
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
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, networkCallback);
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
	message.url = githubGenerateNotificationUrl(notificationData->notificationObject);
	fetchingModule->display->displayMessage(&message);
	free(message.title);
	free(message.text);
	free(message.url);
}

void githubFetch(FetchingModule* fetchingModule) {
	GithubConfig* config = fetchingModule->config;
	GithubNotificationData notificationData;
	Message message;
	NetworkResponse response = {NULL, 0};

	// getting response
	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	CURLcode code = curl_easy_perform(config->curl);

	// parsing response
	if(code == CURLE_OK) {
		json_object* root = json_tokener_parse(response.data);
		if(json_object_get_type(root) == json_type_array) {
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
					notificationData.notificationObject = notification;
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
			fprintf(stderr, "[GitHub] Invalid response:\n%s\n", response.data);
		}
	} else {
		fprintf(stderr, "[GitHub] Request failed with code %d\n", code);
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
