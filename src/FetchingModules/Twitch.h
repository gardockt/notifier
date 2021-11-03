#ifndef TWITCH_H
#define TWITCH_H

#ifdef ENABLE_MODULE_TWITCH

#include <stdlib.h>
#include <stdbool.h>

#define REQUIRED_CURL
#include <curl/curl.h>
#include <json-c/json.h>

#include "FetchingModule.h"

typedef struct {
	char* id;
	char* secret;
	char* token;
	int useCount;
	char* refreshUrl;
} TwitchOAuth;

typedef struct {
	char** streams;
	int streamCount;
	SortedMap* streamTitles;
	TwitchOAuth* oauth;
	CURL* curl;
	struct curl_slist* list;
} TwitchConfig;

void twitchTemplate(FetchingModule*);

#endif // ifdef ENABLE_MODULE_TWITCH

#endif // ifndef TWITCH_H
