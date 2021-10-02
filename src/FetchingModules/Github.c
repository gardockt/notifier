#ifdef ENABLE_MODULE_GITHUB

#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Network.h"
#include "Extras/FetchingModuleUtilities.h"
#include "Github.h"

#define JSON_STRING(obj, str) json_object_get_string(json_object_object_get((obj),(str)))

typedef struct {
	const char* title;
	const char* repoName;
	const char* repoFullName;
	char* url;
} GithubNotificationData;

int githubCompareDates(const char* a, const char* b) {
	return strcmp(a, b);
}

char* githubGenerateNotificationUrl(FetchingModule* fetchingModule, json_object* notification) {
	GithubConfig* config = fetchingModule->config;
	NetworkResponse response = {NULL, 0};

	json_object* subject = json_object_object_get(notification, "subject");
	if(json_object_get_type(subject) != json_type_object) {
		return NULL;
	}
	json_object* commentApiUrlObject = json_object_object_get(subject, "latest_comment_url"); // can be null on PR commits
	if(json_object_get_type(commentApiUrlObject) != json_type_string) {
		return NULL;
	}

	const char* commentApiUrl = json_object_get_string(commentApiUrlObject);
	curl_easy_setopt(config->curl, CURLOPT_URL, commentApiUrl);
	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	CURLcode code = curl_easy_perform(config->curl);

	json_object* commentRoot = json_tokener_parse(response.data);
	if(json_object_get_type(commentRoot) != json_type_object) {
		return NULL;
	}
	json_object* commentUrl = json_object_object_get(commentRoot, "html_url");
	if(json_object_get_type(commentUrl) != json_type_string) {
		return NULL;
	}

	char* ret = strdup(json_object_get_string(commentUrl));
	json_object_put(commentUrl);
	return ret;
}

bool githubParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	GithubConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	if(!moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "token", &config->token)) {
		return false;
	}

	config->list = NULL;
	config->lastRead = strdup("1970-01-01T00:00:00Z");
	
	return true;
}

bool githubEnable(FetchingModule* fetchingModule, Map* configToParse) {
	if(!githubParseConfig(fetchingModule, configToParse)) {
		return false;
	}

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
	}

	free(headerUserAgent);
	free(headerToken);
	return retVal;
}

char* githubReplaceVariables(char* text, void* notificationDataPtr) {
	GithubNotificationData* notificationData = notificationDataPtr;
	char* temp;
	char* ret;

	temp = replace(text, "<title>", notificationData->title);
	ret = replace(temp, "<repo-name>", notificationData->repoName);
	free(temp);
	temp = ret;
	ret = replace(temp, "<repo-full-name>", notificationData->repoFullName);
	free(temp);
	temp = ret;
	ret = replace(temp, "<url>", notificationData->url);
	free(temp);
	return ret;
}

void githubDisplayNotification(FetchingModule* fetchingModule, GithubNotificationData* notificationData) {
	Message message = {0};
	moduleFillBasicMessage(fetchingModule, &message, githubReplaceVariables, notificationData, URL, notificationData->url);
	fetchingModule->display->displayMessage(&message);
	moduleDestroyBasicMessage(&message);
}

void githubParseResponse(FetchingModule* fetchingModule, char* response) {
	GithubConfig* config = fetchingModule->config;
	GithubNotificationData notificationData;
	json_object* root = json_tokener_parse(response);

	if(json_object_get_type(root) == json_type_array) {
		int unreadNotifications = json_object_array_length(root);
		char* newLastRead = NULL;

		for(int i = 0; i < unreadNotifications; i++) {
			json_object* notification = json_object_array_get_idx(root, i);
			const char* lastUpdated = JSON_STRING(notification, "updated_at");

			if(lastUpdated == NULL) {
				moduleLog(fetchingModule, 0, "Invalid last update time in notification %d", i);
				continue;
			}

			if(githubCompareDates(lastUpdated, config->lastRead) > 0) {
				if(newLastRead == NULL || githubCompareDates(lastUpdated, newLastRead) > 0) {
					free(newLastRead);
					newLastRead = strdup(lastUpdated);
				}

				json_object* subject  = json_object_object_get(notification, "subject");
				if(json_object_get_type(subject) != json_type_object) {
					moduleLog(fetchingModule, 0, "Invalid subject object in notification %d", i);
					continue;
				}

				json_object* repository  = json_object_object_get(notification, "repository");
				if(json_object_get_type(repository) != json_type_object) {
					moduleLog(fetchingModule, 0, "Invalid repository object in notification %d", i);
					continue;
				}

				notificationData.title               = JSON_STRING(subject, "title");
				notificationData.repoName            = JSON_STRING(repository, "name");
				notificationData.repoFullName        = JSON_STRING(repository, "full_name");
				notificationData.url                 = githubGenerateNotificationUrl(fetchingModule, notification);
				githubDisplayNotification(fetchingModule, &notificationData);
				free(notificationData.url);
			}
		}

		json_object_put(root);
		if(newLastRead != NULL) {
			config->lastRead = newLastRead;
		}
	} else {
		moduleLog(fetchingModule, 0, "Invalid response");
	}
}

void githubFetch(FetchingModule* fetchingModule) {
	GithubConfig* config = fetchingModule->config;
	NetworkResponse response = {NULL, 0};

	curl_easy_setopt(config->curl, CURLOPT_URL, "https://api.github.com/notifications"); // as GenerateNotificationUrl can change it
	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	CURLcode code = curl_easy_perform(config->curl);

	if(code == CURLE_OK) {
		moduleLog(fetchingModule, 3, "Received response:\n%s", response.data);
		githubParseResponse(fetchingModule, response.data);
		free(response.data);
	} else {
		moduleLog(fetchingModule, 0, "Request failed with code %d", code);
	}
}

void githubDisable(FetchingModule* fetchingModule) {
	GithubConfig* config = fetchingModule->config;

	curl_slist_free_all(config->list);
	curl_easy_cleanup(config->curl);

	free(config->lastRead);
	free(config->token);
	free(config);
}

bool githubTemplate(FetchingModule* fetchingModule) {
	memset(fetchingModule, 0, sizeof *fetchingModule);

	fetchingModule->enable = githubEnable;
	fetchingModule->fetch = githubFetch;
	fetchingModule->disable = githubDisable;

	return true;
}

#endif // ifdef ENABLE_MODULE_GITHUB
