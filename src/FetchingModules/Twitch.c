#ifdef ENABLE_MODULE_TWITCH

#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "Extras/FetchingModuleUtilities.h"
#include "Twitch.h"

#define JSON_STRING(obj, str) json_object_get_string(json_object_object_get((obj),(str)))
#define MIN(x,y) ((x)<(y)?(x):(y))

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

bool twitchRefreshToken(FetchingModule* fetchingModule) {
	// TODO: implement
	moduleLog(fetchingModule, 0, "Token refreshing is not implemented yet.");

	// saving token to stash - uncomment after this function is implemented
	/*
	TwitchConfig* config = fetchingModule->config;
	char* tokenKeyName = malloc(strlen("token_") + strlen(config->clientId) + 1);
	sprintf(tokenKeyName, "token_%s", config->clientId);
	stashSetString("twitch", tokenKeyName, config->token);
	free(tokenKeyName);
	*/

	return false;
}

bool twitchParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	TwitchConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	if(!moduleLoadBasicSettings(fetchingModule, configToParse) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "id", &config->clientId) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "secret", &config->clientSecret)) {
		return false;
	}

	char* streams = getFromMap(configToParse, "streams",  strlen("streams"));
	int streamCount;

	if(streams == NULL) {
		moduleLog(fetchingModule, 0, "Invalid streams");
		return false;
	}

	config->streamCount   = split(streams, FETCHING_MODULE_LIST_ENTRY_SEPARATORS, &config->streams);
	config->streamTitles  = malloc(sizeof *config->streamTitles);

	config->list = NULL;
	
	char* tokenKeyName = malloc(strlen("token_") + strlen(config->clientId) + 1);
	sprintf(tokenKeyName, "token_%s", config->clientId);
	const char* token = stashGetString("twitch", tokenKeyName, NULL);
	if(token == NULL) {
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

	TwitchConfig* config = fetchingModule->config;
	bool retVal = (config->curl = curl_easy_init()) != NULL;

	char* headerClientId = malloc(strlen("Client-ID: ") + strlen(config->clientId) + 1);
	char* headerClientToken = malloc(strlen("Authorization: Bearer ") + strlen(config->token) + 1);

	sprintf(headerClientId, "Client-ID: %s", config->clientId);
	sprintf(headerClientToken, "Authorization: Bearer %s", config->token);

	if(retVal) {
		config->list = curl_slist_append(config->list, headerClientId);
		config->list = curl_slist_append(config->list, headerClientToken);

		curl_easy_setopt(config->curl, CURLOPT_HTTPHEADER, config->list);
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, networkCallback);

		retVal = fetchingModuleCreateThread(fetchingModule);
		moduleLog(fetchingModule, 1, "Module enabled");
	}

	free(headerClientId);
	free(headerClientToken);
	return retVal;
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

	bzero(message, sizeof *message);
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

		// TODO: look for illegal characters while loading config (example: dot in streams)

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

				json_object_put(root);
			} else {
				moduleLog(fetchingModule, 0, "Invalid response:\n%s", response.data); // TODO: read and print error, example error response: {"error":"Bad Request","status":400,"message":"Malformed query params."}
			}
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

bool twitchDisable(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;

	curl_slist_free_all(config->list);
	curl_easy_cleanup(config->curl);

	bool retVal = fetchingModuleDestroyThread(fetchingModule);

	if(retVal) {
		moduleLog(fetchingModule, 1, "Module disabled");

		char* streamTitle;
		for(int i = 0; i < config->streamCount; i++) {
			removeFromMap(config->streamTitles, config->streams[i], strlen(config->streams[i]), NULL, (void**)&streamTitle);
			free(streamTitle);
		}
		destroyMap(config->streamTitles);
		moduleFreeBasicSettings(fetchingModule);
		free(config->streamTitles);
		free(config->token);
		for(int i = 0; i < config->streamCount; i++) {
			free(config->streams[i]);
		}
		free(config->streams);
		free(config->clientId);
		free(config->clientSecret);
		free(config);
	}

	return retVal;
}

bool twitchTemplate(FetchingModule* fetchingModule) {
	bzero(fetchingModule, sizeof *fetchingModule);

	fetchingModule->enable = twitchEnable;
	fetchingModule->fetch = twitchFetch;
	fetchingModule->disable = twitchDisable;

	return true;
}

#endif // ifdef ENABLE_MODULE_TWITCH
