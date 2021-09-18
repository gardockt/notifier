#ifndef TWITCH_H
#define TWITCH_H

#ifndef DISABLE_MODULE_TWITCH

#include <stdlib.h>
#include <stdbool.h>

#define REQUIRED_CURL
#include <curl/curl.h>
#include <json-c/json.h>

#include "FetchingModule.h"

typedef struct {
	char** streams;
	int streamCount;
	char* clientId;
	char* clientSecret;
	Map* streamTitles;
	char* token;
	CURL* curl;
	struct curl_slist* list;
} TwitchConfig;

bool twitchTemplate(FetchingModule*);

#endif // ifndef DISABLE_MODULE_TWITCH

#endif // ifndef TWITCH_H
