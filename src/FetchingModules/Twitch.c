#ifdef ENABLE_MODULE_TWITCH

#include "../Structures/SortedMap.h"
#include "../Structures/BinaryTree.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "../Globals.h"
#include "../Log.h"
#include "Utilities/FetchingModuleUtilities.h"
#include "Utilities/Json.h"
#include "Twitch.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define IS_VALID_URL_CHARACTER(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
#define IS_VALID_STREAMS_CHARACTER(c) (IS_VALID_URL_CHARACTER(c) || strchr(LIST_ENTRY_SEPARATORS, c) != NULL)

static BinaryTree* twitchOAuthStorage = NULL;
static pthread_mutex_t twitchOAuthGlobalMutex;

int twitchOAuthCompare(const void* a, const void* b) {
	return strcmp(((const TwitchOAuth*)a)->id, ((const TwitchOAuth*)b)->id);
}

void twitchOAuthInit() {
	binaryTreeInit(twitchOAuthStorage, twitchOAuthCompare);
	pthread_mutex_init(&twitchOAuthGlobalMutex, NULL);
}

void twitchOAuthDestroy() {
	binaryTreeDestroy(twitchOAuthStorage);
	pthread_mutex_destroy(&twitchOAuthGlobalMutex);
}

bool twitchOAuthRefreshToken(TwitchOAuth* oauth) {
	bool refreshSuccessful = false;
	int refreshCounterBeforeLock = oauth->refreshCounter;

	pthread_mutex_lock(&oauth->mutex);
	logWrite("core", coreVerbosity, 3, "Refresh counter before lock: %d, current: %d", refreshCounterBeforeLock, oauth->refreshCounter);
	if(refreshCounterBeforeLock != oauth->refreshCounter) {
		refreshSuccessful = oauth->lastRefreshResult;
		goto oauthRefreshTokenUnlockMutex;
	}

	CURL* curl = curl_easy_init();
	NetworkResponse response = {NULL, 0};

	logWrite("core", coreVerbosity, 1, "Refreshing token for ID %s...", oauth->id);
	if(curl == NULL) {
		logWrite("core", coreVerbosity, 0, "Token refreshing failed - failed to initialize CURL object");
		goto oauthRefreshTokenSetLastRefreshResult;
	}

	curl_easy_setopt(curl, CURLOPT_URL, oauth->refreshUrl);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
	curl_easy_setopt(curl, CURLOPT_POST, true);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "\n");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, networkCallback);

	CURLcode code = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if(code != CURLE_OK) {
		logWrite("core", coreVerbosity, 0, "Token refreshing failed with CURL code %d", code);
		goto oauthRefreshTokenSetLastRefreshResult;
	}

	logWrite("core", coreVerbosity, 3, "Received response:\n%s", response.data);
	json_object* root = json_tokener_parse(response.data);
	json_object* newTokenObject = json_object_object_get(root, "access_token");
	if(json_object_get_type(newTokenObject) != json_type_string) {
		logWrite("core", coreVerbosity, 0, "Token refreshing failed - invalid response");
		goto oauthRefreshTokenPutRoot;
	}

	const char* newToken = json_object_get_string(newTokenObject);
	logWrite("core", coreVerbosity, 2, "New token: %s", newToken);

	if(oauth->token == NULL) {
		oauth->token = malloc(strlen(newToken) + 1);
	}
	strcpy(oauth->token, newToken);
	refreshSuccessful = true;

oauthRefreshTokenPutRoot:
	json_object_put(root);
oauthRefreshTokenSetLastRefreshResult:
	oauth->lastRefreshResult = refreshSuccessful;
	oauth->refreshCounter++;
oauthRefreshTokenUnlockMutex:
	pthread_mutex_unlock(&oauth->mutex);
	return refreshSuccessful;
}

TwitchOAuth* twitchOAuthAdd(TwitchOAuth* oauth) {
	pthread_mutex_lock(&twitchOAuthGlobalMutex);

	if(twitchOAuthStorage == NULL) {
		twitchOAuthStorage = malloc(sizeof *twitchOAuthStorage);
		twitchOAuthInit();
	}

	TwitchOAuth* oauthFromMap = binaryTreeGet(twitchOAuthStorage, oauth);
	if(oauthFromMap != NULL) {
		oauthFromMap->useCount++;
		goto oauthAddUnlockMutex;
	}

	oauthFromMap = malloc(sizeof *oauthFromMap);
	oauthFromMap->id              = strdup(oauth->id);
	oauthFromMap->secret          = strdup(oauth->secret);
	oauthFromMap->token           = strdup(oauth->token);
	oauthFromMap->useCount        = 1;
	oauthFromMap->refreshUrl      = strdup(oauth->refreshUrl);
	oauthFromMap->refreshCounter  = 0;
	pthread_mutex_init(&oauthFromMap->mutex, NULL);

	binaryTreePut(twitchOAuthStorage, oauthFromMap);

oauthAddUnlockMutex:
	pthread_mutex_unlock(&twitchOAuthGlobalMutex);
	return oauthFromMap;
}

void twitchOAuthRemove(TwitchOAuth* oauth) {
	TwitchOAuth* oauthFromMap = binaryTreeGet(twitchOAuthStorage, oauth);

	// this should never happen
	if(oauthFromMap == NULL) {
		return;
	}

	oauthFromMap->useCount--;
	if(oauthFromMap->useCount == 0) {
		binaryTreeRemove(twitchOAuthStorage, oauthFromMap);
		pthread_mutex_destroy(&oauthFromMap->mutex);
		free(oauthFromMap->id);
		free(oauthFromMap->secret);
		free(oauthFromMap->token);
		free(oauthFromMap->refreshUrl);
		free(oauthFromMap);
	}

	if(twitchOAuthStorage->root == NULL) {
		twitchOAuthDestroy();
		free(twitchOAuthStorage);
		twitchOAuthStorage = NULL;
	}
}

typedef struct {
	char* streamerName;
	char* title;
	char* category;
	char* url;
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

char* twitchGenerateTokenRefreshUrl(TwitchOAuth* oauth) {
	char* url = malloc(strlen("https://id.twitch.tv/oauth2/token?client_id=&client_secret=&grant_type=client_credentials") + strlen(oauth->id) + strlen(oauth->secret) + 1);
	sprintf(url, "https://id.twitch.tv/oauth2/token?client_id=%s&client_secret=%s&grant_type=client_credentials", oauth->id, oauth->secret);
	return url;
}

void twitchSetHeader(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;

	if(config->list != NULL) {
		curl_slist_free_all(config->list);
		config->list = NULL;
	}

	char* headerClientId = malloc(strlen("Client-ID: ") + strlen(config->oauth->id) + 1);
	char* headerClientToken = malloc(strlen("Authorization: Bearer ") + strlen(config->oauth->token) + 1);

	sprintf(headerClientId, "Client-ID: %s", config->oauth->id);
	sprintf(headerClientToken, "Authorization: Bearer %s", config->oauth->token);

	config->list = curl_slist_append(config->list, headerClientId);
	config->list = curl_slist_append(config->list, headerClientToken);

	curl_easy_setopt(config->curl, CURLOPT_HTTPHEADER, config->list);

	free(headerClientId);
	free(headerClientToken);
}

bool twitchRefreshTokenForOAuth(TwitchOAuth* oauth) {
	bool success = twitchOAuthRefreshToken(oauth);
	if(success) {
		char* tokenKeyName = malloc(strlen("token_") + strlen(oauth->id) + 1);
		sprintf(tokenKeyName, "token_%s", oauth->id);
		stashSetString("twitch", tokenKeyName, oauth->token);
		free(tokenKeyName);
	}
	return success;
}

bool twitchParseConfig(FetchingModule* fetchingModule, SortedMap* configToParse) {
	TwitchOAuth oauth = {0};
	TwitchConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	if(!moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "id", &oauth.id) ||
	   !moduleLoadStringFromConfigWithErrorMessage(fetchingModule, configToParse, "secret", &oauth.secret)) {
		return false;
	}

	oauth.refreshUrl = twitchGenerateTokenRefreshUrl(&oauth);

	char* tokenKeyName = malloc(strlen("token_") + strlen(oauth.id) + 1);
	sprintf(tokenKeyName, "token_%s", oauth.id);
	const char* token = stashGetString("twitch", tokenKeyName, NULL);
	if(token == NULL) {
		if(!twitchRefreshTokenForOAuth(&oauth)) {
			return false;
		}
	} else {
		oauth.token = strdup(token);
	}
	free(tokenKeyName);

	char* streams = sortedMapGet(configToParse, "streams");
	if(streams == NULL) {
		moduleLog(fetchingModule, 0, "Invalid streams");
		return false;
	}

	for(int i = 0; oauth.id[i] != '\0'; i++) {
		if(!IS_VALID_URL_CHARACTER(oauth.id[i])) {
			moduleLog(fetchingModule, 0, "ID contains illegal character \"%c\"", oauth.id[i]);
			return false;
		}
	}
	for(int i = 0; oauth.secret[i] != '\0'; i++) {
		if(!IS_VALID_URL_CHARACTER(oauth.secret[i])) {
			moduleLog(fetchingModule, 0, "Secret contains illegal character \"%c\"", oauth.secret[i]);
			return false;
		}
	}
	for(int i = 0; streams[i] != '\0'; i++) {
		if(!IS_VALID_STREAMS_CHARACTER(streams[i])) {
			moduleLog(fetchingModule, 0, "Streams contain illegal character \"%c\"", streams[i]);
			return false;
		}
	}

	config->streamCount   = split(streams, LIST_ENTRY_SEPARATORS, &config->streams);
	config->streamTitles  = malloc(sizeof *config->streamTitles);

	config->list = NULL;
	config->curl = curl_easy_init();

	if(config->curl == NULL) {
		moduleLog(fetchingModule, 0, "Error initializing CURL object");
		return false;
	}
	curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, networkCallback);

	config->oauth = twitchOAuthAdd(&oauth);
	free(oauth.id);
	free(oauth.secret);
	free(oauth.token);
	free(oauth.refreshUrl);

	sortedMapInit(config->streamTitles, sortedMapCompareFunctionStrcmp);
	for(int i = 0; i < config->streamCount; i++) {
		sortedMapPut(config->streamTitles, config->streams[i], NULL);
	}

	return true;
}

bool twitchEnable(FetchingModule* fetchingModule, SortedMap* configToParse) {
	if(!fetchingModuleInit(fetchingModule, configToParse, FM_DEFAULTS) ||
	   !twitchParseConfig(fetchingModule, configToParse)) {
		return false;
	}

	twitchSetHeader(fetchingModule);
	return true;
}

char* twitchReplaceVariables(char* text, void* notificationDataPtr) {
	TwitchNotificationData* notificationData = notificationDataPtr;
	return replace(text, 4,
		"<streamer-name>", notificationData->streamerName,
		"<title>", notificationData->title,
		"<category>", notificationData->category,
		"<url>", notificationData->url);
}

void twitchDisplayNotification(FetchingModule* fetchingModule, TwitchNotificationData* notificationData) {
	Message message = {0};
	moduleFillBasicMessage(fetchingModule, &message, twitchReplaceVariables, notificationData, NULL);
	message.actionData = notificationData->url;
	message.actionType = URL;
	fetchingModule->display->displayMessage(&message);

	moduleDestroyBasicMessage(&message);
}

int twitchParseResponse(FetchingModule* fetchingModule, char* response, char** checkedStreamNames, TwitchNotificationData* newData) {
	TwitchConfig* config = fetchingModule->config;
	json_object* root = json_tokener_parse(response);
	json_object* data;

	// TODO: memory leak?
	if(!jsonReadArray(root, "data", &data)) {
		int errorStatus;
		const char* errorCode;
		const char* errorMessage;
		if(json_object_get_type(root) == json_type_object &&
		   jsonReadInt(root, "status", &errorStatus) &&
		   jsonReadString(root, "error", &errorCode) &&
		   jsonReadString(root, "message", &errorMessage)) {
			moduleLog(fetchingModule, 0, "Error %d - %s (%s)", errorStatus, errorCode, errorMessage);
			return errorStatus;
		} else {
			moduleLog(fetchingModule, 0, "Invalid response");
			return -1;
		}
	}

	int activeStreamersCount = json_object_array_length(data);

	for(int i = 0; i < activeStreamersCount; i++) {
		json_object* stream = json_object_array_get_idx(data, i);
		const char* activeStreamerName;
		const char* newTitle;
		const char* newCategory;

		if(!jsonReadString(stream, "user_name", &activeStreamerName) ||
		   !jsonReadString(stream, "title", &newTitle) ||
		   !jsonReadString(stream, "game_name", &newCategory)) {
			moduleLog(fetchingModule, 0, "Invalid data for stream %d", i);
			continue;
		}

		for(int j = 0; j < config->streamCount; j++) {
			if(!strcasecmp(activeStreamerName, checkedStreamNames[j])) {
				newData[j].streamerName  = strdup(activeStreamerName);
				newData[j].title         = strdup(newTitle);
				newData[j].category      = strdup(newCategory);
				newData[j].url           = malloc(strlen("https://twitch.tv/") + strlen(activeStreamerName) + 1);
				sprintf(newData[j].url, "https://twitch.tv/%s", activeStreamerName);

				break;
			}
		}
	}
	json_object_put(root);
	return 0;
}

void twitchFetch(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;
	char** checkedStreamNames = malloc(config->streamCount * sizeof *checkedStreamNames);
	TwitchNotificationData* newData = malloc(config->streamCount * sizeof *newData);

	sortedMapKeys(config->streamTitles, (void**)checkedStreamNames);
	newData = memset(newData, 0, config->streamCount * sizeof *newData);

	for(int i = 0; i < config->streamCount; i += 100) {
		NetworkResponse response = {NULL, 0};
		char* tokenUsed = strdup(config->oauth->token);
		char* url = twitchGenerateUrl(config->streams, i, MIN(config->streamCount - 1, i + 99));
		curl_easy_setopt(config->curl, CURLOPT_URL, url);
		curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, &response);
		free(url);

		CURLcode code = curl_easy_perform(config->curl);
		if(code != CURLE_OK) {
			moduleLog(fetchingModule, 0, "Request failed with code %d", code);
			goto twitchFetchFreeResponse;
		}

		moduleLog(fetchingModule, 3, "Received response:\n%s", response.data);
		int errorCode = twitchParseResponse(fetchingModule, response.data, checkedStreamNames, newData);
		if(errorCode == 401) {
			moduleLog(fetchingModule, 3, "Used token: %s, current: %s", tokenUsed, config->oauth->token);
			if(strcmp(tokenUsed, config->oauth->token) != 0 || twitchRefreshTokenForOAuth(config->oauth)) {
				twitchSetHeader(fetchingModule);
				i -= 100; // retry
			} else {
				moduleLog(fetchingModule, 0, "Unable to refresh token, skipping fetch");
				i = config->streamCount; // end the loop
			}
		}

twitchFetchFreeResponse:
		free(response.data);
		free(tokenUsed);
	}

	for(int i = 0; i < config->streamCount; i++) {
		if(sortedMapGet(config->streamTitles, checkedStreamNames[i]) == NULL && newData[i].title != NULL) {
			TwitchNotificationData notificationData = {0};
			notificationData.streamerName  = newData[i].streamerName;
			notificationData.title         = newData[i].title;
			notificationData.category      = newData[i].category;
			notificationData.url           = newData[i].url;
			twitchDisplayNotification(fetchingModule, &notificationData);
		}
		char* lastStreamTitle;
		sortedMapRemove(config->streamTitles, checkedStreamNames[i], NULL, (void**)&lastStreamTitle);
		free(lastStreamTitle);
		sortedMapPut(config->streamTitles, checkedStreamNames[i], newData[i].title);

		free(newData[i].streamerName);
		free(newData[i].category);
		free(newData[i].url);
	}

	free(checkedStreamNames);
	free(newData);
}

void twitchDisable(FetchingModule* fetchingModule) {
	TwitchConfig* config = fetchingModule->config;

	curl_slist_free_all(config->list);
	curl_easy_cleanup(config->curl);

	char* streamTitle;
	for(int i = 0; i < config->streamCount; i++) {
		sortedMapRemove(config->streamTitles, config->streams[i], NULL, (void**)&streamTitle);
		free(streamTitle);
	}
	twitchOAuthRemove(config->oauth);
	sortedMapDestroy(config->streamTitles);
	free(config->streamTitles);
	for(int i = 0; i < config->streamCount; i++) {
		free(config->streams[i]);
	}
	free(config->streams);
	free(config);
}

void twitchTemplate(FetchingModule* fetchingModule) {
	fetchingModule->enable = twitchEnable;
	fetchingModule->fetch = twitchFetch;
	fetchingModule->disable = twitchDisable;
}

#endif // ifdef ENABLE_MODULE_TWITCH
