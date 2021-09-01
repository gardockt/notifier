#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "../GlobalManagers.h"
#include "Twitch.h"

#define JSON_STRING(obj, str) json_object_get_string(json_object_object_get((obj),(str)))
#define MIN(x,y) ((x)<(y)?(x):(y))

// TODO: REFACTOR!!
// idea for improving notification condition I might check later: collect active streams and topics into array (or sorted list), get all keys from array and update each key's value depending on active streams' array

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
		output[outputPointer++] = (i == start ? '?' : '&');
		strcpy(&output[outputPointer], "user_login=");
		outputPointer += strlen("user_login=");
		strcpy(&output[outputPointer], streams[i]);
		outputPointer += strlen(streams[i]);
	}

	output[totalLength] = '\0';
	return output;
}

bool twitchRefreshToken(FetchingModule* fetchingModule) {
	// TODO: implement
	fprintf(stderr, "[Twitch] Token refreshing is not implemented yet.\n");

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

	char* title         = getFromMap(configToParse, "title",    strlen("title"));
	char* body          = getFromMap(configToParse, "body",     strlen("body"));
	char* streams       = getFromMap(configToParse, "streams",  strlen("streams"));
	char* clientId      = getFromMap(configToParse, "id",       strlen("id"));
	char* clientSecret  = getFromMap(configToParse, "secret",   strlen("secret"));
	char* interval      = getFromMap(configToParse, "interval", strlen("interval"));
	char* display       = getFromMap(configToParse, "display",  strlen("display"));
	int streamCount;

	if(title == NULL || body == NULL || clientId == NULL || clientSecret == NULL || interval == NULL || streams == NULL || display == NULL) {
		destroyMap(configToParse);
		return false;
	}

	config->title         = strdup(title);
	config->body          = strdup(body);
	config->streamCount   = split(streams, ",", &config->streams);
	config->clientId      = strdup(clientId);
	config->clientSecret  = strdup(clientSecret);
	config->streamTitles  = malloc(sizeof *config->streamTitles);
	fetchingModule->intervalSecs  = atoi(interval);
	fetchingModule->display       = getDisplay(&displayManager, display);

	if(fetchingModule->display == NULL) {
		destroyMap(configToParse);
		return false;
	}

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

bool twitchEnable(FetchingModule* fetchingModule) {
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
		printf("Twitch enabled\n");
	}

	free(headerClientId);
	free(headerClientToken);
	return retVal;
}

char* twitchReplaceVariables(char* text, TwitchNotificationData* notificationData) {
	char* temp;
	char* ret;

	temp = replace(text, "<streamer-name>", notificationData->streamerName);
	ret = replace(temp, "<stream-title>", notificationData->title);
	free(temp);
	temp = ret;
	ret = replace(temp, "<category>", notificationData->category);
	free(temp);
	return ret;
}

void twitchDisplayNotification(FetchingModule* fetchingModule, TwitchNotificationData* notificationData) {
	TwitchConfig* config = fetchingModule->config;
	Message* message = malloc(sizeof *message);

	char* url = malloc(strlen("https://twitch.tv/") + strlen(notificationData->streamerName) + 1);
	sprintf(url, "https://twitch.tv/%s", notificationData->streamerName);

	message->title = twitchReplaceVariables(config->title, notificationData);
	message->text = twitchReplaceVariables(config->body, notificationData);
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

	char** streamsFromMap     = malloc(config->streamCount * sizeof *streamsFromMap);
	const char** streamerName       = malloc(config->streamCount * sizeof *streamerName);
	const char** streamsNewTopic    = malloc(config->streamCount * sizeof *streamsNewTopic);
	const char** streamsNewCategory = malloc(config->streamCount * sizeof *streamsNewCategory);
	getMapKeys(config->streamTitles, (void**)streamsFromMap);
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
				int activeStreamers = json_object_array_length(data);

				for(int i = 0; i < activeStreamers; i++) {
					json_object* stream = json_object_array_get_idx(data, i);
					const char* activeStreamerName = JSON_STRING(stream, "user_name");

					for(int j = 0; j < config->streamCount; j++) {
						if(!strcasecmp(activeStreamerName, streamsFromMap[j])) {
							streamerName[j]    = activeStreamerName;
							streamsNewTopic[j] = JSON_STRING(stream, "title");
							streamsNewCategory[j] = JSON_STRING(stream, "game_name");
							break;
						}
					}
				}

				for(int i = 0; i < config->streamCount; i++) {
					if(getFromMap(config->streamTitles, streamsFromMap[i], strlen(streamsFromMap[i])) == NULL && streamsNewTopic[i] != NULL) {
						notificationData.streamerName = streamerName[i];
						notificationData.title        = streamsNewTopic[i];
						notificationData.category     = streamsNewCategory[i];
						twitchDisplayNotification(fetchingModule, &notificationData);
					}
					putIntoMap(config->streamTitles, streamsFromMap[i], strlen(streamsFromMap[i]), streamsNewTopic[i]); // value get uninitialized, but we are only checking whether it is NULL
				}

				json_object_put(root);
			} else {
				fprintf(stderr, "[Twitch] Invalid response:\n%s\n", response.data); // TODO: read and print error, example error response: {"error":"Bad Request","status":400,"message":"Malformed query params."}
			}
		} else {
			fprintf(stderr, "[Twitch] Request failed with code %d\n", code);
		}
		free(response.data);
	}

	free(streamsNewCategory);
	free(streamsNewTopic);
	free(streamerName);
	free(streamsFromMap);
}

bool twitchDisable(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;

	curl_slist_free_all(config->list);
	curl_easy_cleanup(config->curl);

	bool retVal = fetchingModuleDestroyThread(fetchingModule);

	if(retVal) {
		destroyMap(config->streamTitles);
		free(config->streamTitles);
		free(config->token);
		free(config->title);
		free(config->body);
		for(int i = 0; i < config->streamCount; i++) {
			free(config->streams[i]);
		}
		free(config->streams);
		free(config->clientId);
		free(config->clientSecret);
		free(config);
		printf("Twitch disabled\n");
	}

	return retVal;
}

bool twitchTemplate(FetchingModule* fetchingModule) {
	fetchingModule->intervalSecs = 1;
	fetchingModule->config = NULL;
	fetchingModule->busy = false;
	fetchingModule->parseConfig = twitchParseConfig;
	fetchingModule->enable = twitchEnable;
	fetchingModule->fetch = twitchFetch;
	fetchingModule->display = NULL;
	fetchingModule->disable = twitchDisable;

	return true;
}
