#include "Twitch.h"
#include "../Structures/Map.h"
#include "../StringOperations.h"

#define JSON_STRING(obj, str) json_object_get_string(json_object_object_get((obj),(str)))
#define MIN(x,y) ((x)<(y)?(x):(y))

CURL* curl = NULL;
struct curl_slist* list = NULL; // TODO: global variable might cause issues with multiple instances

// TODO: REFACTOR!!
// idea for improving notification condition I might check later: collect active streams and topics into array (or sorted list), get all keys from array and update each key's value depending on active streams' array

typedef struct {
	char* data;
	int size;
} Memory;

typedef struct {
	char* streamerName;
	char* title;
} TwitchNotificationData;

size_t twitchCurlCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	Memory* memory = (Memory*)userdata;
	int sizeBefore = memory->size;
	int sizeAdded = size * nmemb;

	memory->size = sizeBefore + sizeAdded;
	memory->data = realloc(memory->data, memory->size);
	strncpy(&memory->data[sizeBefore], ptr, sizeAdded);

	return sizeAdded;
}

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

bool twitchParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	// TODO: move interval parsing to enableModule?

	TwitchConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	char* title = getFromMap(configToParse, "title", strlen("title"));
	char* body = getFromMap(configToParse, "body", strlen("body"));
	char* streams = getFromMap(configToParse, "streams", strlen("streams"));
	int streamCount;
	char* clientId = getFromMap(configToParse, "id", strlen("id"));
	char* clientSecret = getFromMap(configToParse, "secret", strlen("secret")); // TODO: currently not a secret but OAuth, fix
	char* interval = getFromMap(configToParse, "interval", strlen("interval"));

	if(title == NULL || body == NULL || clientId == NULL || clientSecret == NULL || interval == NULL || streams == NULL) {
		destroyMap(configToParse);
		return false;
	}

	config->title = title;
	config->body = body;
	config->streamCount = split(streams, ",", &config->streams);
	config->clientId = clientId;
	config->clientSecret = clientSecret;
	fetchingModule->intervalSecs = atoi(interval);
	config->streamTitles = malloc(sizeof *config->streamTitles);
	
	initMap(config->streamTitles);
	for(int i = 0; i < config->streamCount; i++) {
		putIntoMap(config->streamTitles, config->streams[i], strlen(config->streams[i]), NULL);
	}

	int keyCount = getMapSize(configToParse);
	char** keys = malloc(keyCount * sizeof *keys);
	getMapKeys(configToParse, keys);

	for(int i = 0; i < keyCount; i++) {
		char* valueToFree;
		removeFromMap(configToParse, keys[i], strlen(keys[i]), NULL, &valueToFree); // key is already in keys[i]
		if(strcmp(keys[i], "title") != 0 &&
		   strcmp(keys[i], "body") != 0 &&
		   strcmp(keys[i], "id") != 0 &&
		   strcmp(keys[i], "secret") != 0) {
			free(valueToFree);
		}
		free(keys[i]);
	}
	
	free(keys);
	destroyMap(configToParse);

	return true;
}

bool twitchEnable(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;
	bool retVal = (curl = curl_easy_init()) != NULL;

	char* headerClientId = malloc(strlen("Client-ID: ") + strlen(config->clientId) + 1);
	char* headerClientToken = malloc(strlen("Authorization: Bearer ") + strlen(config->clientSecret) + 1);

	sprintf(headerClientId, "Client-ID: %s", config->clientId);
	sprintf(headerClientToken, "Authorization: Bearer %s", config->clientSecret);

	if(retVal) {
		list = curl_slist_append(list, headerClientId);
		list = curl_slist_append(list, headerClientToken);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, twitchCurlCallback);

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
	return ret;
}

void twitchDisplayNotification(FetchingModule* fetchingModule, TwitchNotificationData* notificationData) {
	TwitchConfig* config = fetchingModule->config;
	Message message;

	message.title = twitchReplaceVariables(config->title, notificationData);
	message.text = twitchReplaceVariables(config->body, notificationData);
	fetchingModule->display->displayMessage(&message);
	free(message.title);
	free(message.text);
}

void twitchFetch(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;
	TwitchNotificationData notificationData;
	Message message;
	Memory response = {NULL, 0};
	char* url;

	char** streamsFromMap  = malloc(config->streamCount * sizeof *streamsFromMap);
	char** streamerName    = malloc(config->streamCount * sizeof *streamerName);
	char** streamsNewTopic = malloc(config->streamCount * sizeof *streamsNewTopic);
	getMapKeys(config->streamTitles, streamsFromMap);
	streamsNewTopic = memset(streamsNewTopic, 0, config->streamCount * sizeof *streamsNewTopic);

	// getting response
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
	for(int i = 0; i < config->streamCount; i += 100) {
		url = twitchGenerateUrl(config->streams, i, MIN(config->streamCount - 1, i + 99));
		curl_easy_setopt(curl, CURLOPT_URL, url);

		CURLcode code = curl_easy_perform(curl);

		// parsing response
		json_object* root = json_tokener_parse(response.data);
		json_object* data = json_object_object_get(root, "data");
		int activeStreamers = json_object_array_length(data);

		for(int i = 0; i < activeStreamers; i++) {
			json_object* stream = json_object_array_get_idx(data, i);
			char* activeStreamerName = JSON_STRING(stream, "user_name");

			for(int j = 0; j < config->streamCount; j++) {
				if(!strcasecmp(activeStreamerName, streamsFromMap[j])) {
					streamerName[j]    = activeStreamerName;
					streamsNewTopic[j] = JSON_STRING(stream, "title");
					break;
				}
			}
		}

		for(int i = 0; i < config->streamCount; i++) {
			if(getFromMap(config->streamTitles, streamsFromMap[i], strlen(streamsFromMap[i])) == NULL && streamsNewTopic[i] != NULL) {
				notificationData.streamerName = streamerName[i];
				notificationData.title        = streamsNewTopic[i];
				twitchDisplayNotification(fetchingModule, &notificationData);
			}
			putIntoMap(config->streamTitles, streamsFromMap[i], strlen(streamsFromMap[i]), streamsNewTopic[i]); // value get uninitialized, but we are only checking whether it is NULL
		}

		json_object_put(root);
		free(response.data);
	}

	free(streamsNewTopic);
	free(streamerName);
	free(streamsFromMap);
}

bool twitchDisable(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;

	curl_slist_free_all(list);
	curl_easy_cleanup(curl);

	bool retVal = fetchingModuleDestroyThread(fetchingModule);

	if(retVal) {
		destroyMap(config->streamTitles);
		free(config->streamTitles);
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
