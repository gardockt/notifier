#ifdef ENABLE_MODULE_TWITCH

#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "Extras/FetchingModuleUtilities.h"
#include "Twitch.h"

#define JSON_STRING(obj, str) json_object_get_string(json_object_object_get((obj),(str)))
#define JSON_INT(obj, str) json_object_get_int(json_object_object_get((obj),(str)))
#define MIN(x,y) ((x)<(y)?(x):(y))
#define IS_VALID_URL_CHARACTER(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
#define IS_VALID_STREAMS_CHARACTER(c) (IS_VALID_URL_CHARACTER(c) || strchr(FETCHING_MODULE_LIST_ENTRY_SEPARATORS, c) != NULL)

typedef struct {
	const char* streamerName;
	const char* title;
	const char* category;
} TwitchNotificationData;

char* twitchGenerateUrl(char** streams, int start, int stop) {
	const char* base = "https://api.twitch.tv/helix/streams";
	char* output;
	int totalLength;
	int outputPointer;

	totalLength = strlen(base);
	for(int i = start; i <= stop; i++) {
		totalLength += strlen("?user_login=") + strlen(streams[i]);
	}
	output = malloc(totalLength + 1);
	strcpy(output, base);
	outputPointer = strlen(base);

	for(int i = start; i <= stop; i++) {
		outputPointer += sprintf(&output[outputPointer], "%cuser_login=%s", (i == start ? '?' : '&'), streams[i]);
	}

	return output;
}

void twitchSetHeader(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;

	if(config->list != NULL) {
		curl_slist_free_all(config->list);
		config->list = NULL;
	}

	char* headerClientId = malloc(strlen("Client-ID: ") + strlen(config->id) + 1);
	char* headerClientToken = malloc(strlen("Authorization: Bearer ") + strlen(config->token) + 1);

	sprintf(headerClientId, "Client-ID: %s", config->id);
	sprintf(headerClientToken, "Authorization: Bearer %s", config->token);

	config->list = curl_slist_append(config->list, headerClientId);
	config->list = curl_slist_append(config->list, headerClientToken);

	curl_easy_setopt(config->curl, CURLOPT_HTTPHEADER, config->list);

	free(headerClientId);
	free(headerClientToken);
}

bool twitchRefreshToken(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;
	NetworkResponse response = {NULL, 0};
	bool refreshSuccessful = false;

	moduleLog(fetchingModule, 1, "Refreshing token...");

	char* url = malloc(strlen("https://id.twitch.tv/oauth2/token?client_id=&client_secret=&grant_type=client_credentials") + strlen(config->id) + strlen(config->secret) + 1);
	sprintf(url, "https://id.twitch.tv/oauth2/token?client_id=%s&client_secret=%s&grant_type=client_credentials", config->id, config->secret);
	curl_easy_setopt(config->curl, CURLOPT_URL, url);
	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	curl_easy_setopt(config->curl, CURLOPT_POST, true);
	curl_easy_setopt(config->curl, CURLOPT_POSTFIELDS, "\n");
	free(url);

	CURLcode code = curl_easy_perform(config->curl);
	curl_easy_setopt(config->curl, CURLOPT_POST, false);
	if(code == CURLE_OK) {
		moduleLog(fetchingModule, 3, "Received response:\n%s", response.data);
		json_object* root = json_tokener_parse(response.data);
		json_object* newTokenObject = json_object_object_get(root, "access_token");
		if(json_object_get_type(newTokenObject) == json_type_string) {
			const char* newToken = json_object_get_string(newTokenObject);
			moduleLog(fetchingModule, 2, "New token: %s", newToken);

			if(config->token == NULL) {
				config->token = malloc(strlen(newToken) + 1);
			}
			strcpy(config->token, newToken);
			refreshSuccessful = true;
		} else {
			moduleLog(fetchingModule, 0, "Token refreshing failed - invalid response");
		}
		json_object_put(root);
	} else {
		moduleLog(fetchingModule, 0, "Token refreshing failed with CURL code %d", code);
	}

	if(refreshSuccessful) {
		twitchSetHeader(fetchingModule);

		char* tokenKeyName = malloc(strlen("token_") + strlen(config->id) + 1);
		sprintf(tokenKeyName, "token_%s", config->id);
		stashSetString("twitch", tokenKeyName, config->token);
		free(tokenKeyName);
	}

	return refreshSuccessful;
}

bool twitchParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	TwitchConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	if(!moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "id", &config->id) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "secret", &config->secret)) {
		return false;
	}

	char* streams = getFromMap(configToParse, "streams",  strlen("streams"));
	int streamCount;

	if(streams == NULL) {
		moduleLog(fetchingModule, 0, "Invalid streams");
		return false;
	}

	for(int i = 0; config->id[i] != '\0'; i++) {
		if(!IS_VALID_URL_CHARACTER(config->id[i])) {
			moduleLog(fetchingModule, 0, "ID contains illegal character \"%c\"", config->id[i]);
			return false;
		}
	}
	for(int i = 0; config->secret[i] != '\0'; i++) {
		if(!IS_VALID_URL_CHARACTER(config->secret[i])) {
			moduleLog(fetchingModule, 0, "Secret contains illegal character \"%c\"", config->secret[i]);
			return false;
		}
	}
	for(int i = 0; streams[i] != '\0'; i++) {
		if(!IS_VALID_STREAMS_CHARACTER(streams[i])) {
			moduleLog(fetchingModule, 0, "Streams contain illegal character \"%c\"", streams[i]);
			return false;
		}
	}

	config->streamCount   = split(streams, FETCHING_MODULE_LIST_ENTRY_SEPARATORS, &config->streams);
	config->streamTitles  = malloc(sizeof *config->streamTitles);

	config->list = NULL;
	config->curl = curl_easy_init();

	if(config->curl == NULL) {
		moduleLog(fetchingModule, 0, "Error initializing CURL object");
		return false;
	}
	curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, networkCallback);

	char* tokenKeyName = malloc(strlen("token_") + strlen(config->id) + 1);
	sprintf(tokenKeyName, "token_%s", config->id);
	const char* token = stashGetString("twitch", tokenKeyName, NULL);
	if(token == NULL) {
		config->token = NULL;
		twitchRefreshToken(fetchingModule);
	} else {
		// token has to be freed after regenerating, after duplicating we don't have to worry when to free
		config->token = strdup(token);
	}
	free(tokenKeyName);

	initMap(config->streamTitles);
	for(int i = 0; i < config->streamCount; i++) {
		putIntoMap(config->streamTitles, config->streams[i], strlen(config->streams[i]), NULL);
	}

	return true;
}

bool twitchEnable(FetchingModule* fetchingModule, Map* configToParse) {
	if(!twitchParseConfig(fetchingModule, configToParse)) {
		return false;
	}

	twitchSetHeader(fetchingModule);

	return true;
}

char* twitchReplaceVariables(char* text, void* notificationDataPtr) {
	TwitchNotificationData* notificationData = notificationDataPtr;
	char* temp;
	char* ret;

	temp = replace(text, "<streamer-name>", notificationData->streamerName);
	ret = replace(temp, "<title>", notificationData->title);
	free(temp);
	temp = ret;
	ret = replace(temp, "<category>", notificationData->category);
	free(temp);
	return ret;
}

void twitchDisplayNotification(FetchingModule* fetchingModule, TwitchNotificationData* notificationData) {
	Message* message = malloc(sizeof *message);

	char* url = malloc(strlen("https://twitch.tv/") + strlen(notificationData->streamerName) + 1);
	sprintf(url, "https://twitch.tv/%s", notificationData->streamerName);

	memset(message, 0, sizeof *message);
	moduleFillBasicMessage(fetchingModule, message, twitchReplaceVariables, notificationData);
	message->actionData = url;
	message->actionType = URL;
	fetchingModule->display->displayMessage(message, defaultMessageFreeFunction);
}

void twitchFetch(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;
	TwitchNotificationData notificationData;
	Message message;
	NetworkResponse response = {NULL, 0};
	char* url;
	json_object* root;

	char** checkedStreamNames       = malloc(config->streamCount * sizeof *checkedStreamNames);
	char** streamsNewTopic          = malloc(config->streamCount * sizeof *streamsNewTopic);
	const char** streamerName       = malloc(config->streamCount * sizeof *streamerName);
	const char** streamsNewCategory = malloc(config->streamCount * sizeof *streamsNewCategory);
	getMapKeys(config->streamTitles, (void**)checkedStreamNames);
	streamsNewTopic = memset(streamsNewTopic, 0, config->streamCount * sizeof *streamsNewTopic);

	// getting response
	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	for(int i = 0; i < config->streamCount; i += 100) {
		url = twitchGenerateUrl(config->streams, i, MIN(config->streamCount - 1, i + 99));
		curl_easy_setopt(config->curl, CURLOPT_URL, url);
		free(url);

		CURLcode code = curl_easy_perform(config->curl);

		// parsing response
		if(code == CURLE_OK) {
			json_object* root = json_tokener_parse(response.data);
			json_object* data = json_object_object_get(root, "data");
			if(json_object_get_type(data) == json_type_array) {
				int activeStreamersCount = json_object_array_length(data);

				for(int i = 0; i < activeStreamersCount; i++) {
					json_object* stream = json_object_array_get_idx(data, i);
					const char* activeStreamerName = JSON_STRING(stream, "user_name");

					for(int j = 0; j < config->streamCount; j++) {
						if(!strcasecmp(activeStreamerName, checkedStreamNames[j])) {
							streamerName[j]    = activeStreamerName;
							streamsNewTopic[j] = strdup(JSON_STRING(stream, "title"));
							streamsNewCategory[j] = JSON_STRING(stream, "game_name");
							break;
						}
					}
				}

				for(int i = 0; i < config->streamCount; i++) {
					if(getFromMap(config->streamTitles, checkedStreamNames[i], strlen(checkedStreamNames[i])) == NULL && streamsNewTopic[i] != NULL) {
						notificationData.streamerName = streamerName[i];
						notificationData.title        = streamsNewTopic[i];
						notificationData.category     = streamsNewCategory[i];
						twitchDisplayNotification(fetchingModule, &notificationData);
					}
					char* lastStreamTopic;
					removeFromMap(config->streamTitles, checkedStreamNames[i], strlen(checkedStreamNames[i]), NULL, (void**)&lastStreamTopic);
					free(lastStreamTopic);
					putIntoMap(config->streamTitles, checkedStreamNames[i], strlen(checkedStreamNames[i]), streamsNewTopic[i]);
				}
			} else {
				if(json_object_get_type(root) == json_type_object) {
					int errorCode = JSON_INT(root, "status");
					moduleLog(fetchingModule, 0, "Error %d - %s (%s)", errorCode, JSON_STRING(root, "error"), JSON_STRING(root, "message"));
					if(errorCode == 401) { // Unauthorized
						if(twitchRefreshToken(fetchingModule)) {
							twitchFetch(fetchingModule);
						}
					}
				} else {
					moduleLog(fetchingModule, 0, "Invalid response:\n%s", response.data);
				}
			}
			json_object_put(root);
		} else {
			moduleLog(fetchingModule, 0, "Request failed with code %d", code);
		}
		free(response.data);
	}

	free(streamsNewCategory);
	free(streamsNewTopic);
	free(streamerName);
	free(checkedStreamNames);
}

void twitchDisable(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;

	curl_slist_free_all(config->list);
	curl_easy_cleanup(config->curl);

	char* streamTitle;
	for(int i = 0; i < config->streamCount; i++) {
		removeFromMap(config->streamTitles, config->streams[i], strlen(config->streams[i]), NULL, (void**)&streamTitle);
		free(streamTitle);
	}
	destroyMap(config->streamTitles);
	free(config->streamTitles);
	free(config->token);
	for(int i = 0; i < config->streamCount; i++) {
		free(config->streams[i]);
	}
	free(config->streams);
	free(config->id);
	free(config->secret);
	free(config);
}

bool twitchTemplate(FetchingModule* fetchingModule) {
	memset(fetchingModule, 0, sizeof *fetchingModule);

	fetchingModule->enable = twitchEnable;
	fetchingModule->fetch = twitchFetch;
	fetchingModule->disable = twitchDisable;

	return true;
}

#endif // ifdef ENABLE_MODULE_TWITCH
