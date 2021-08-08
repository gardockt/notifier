#ifndef TWITCH_H
#define TWITCH_H

#include <stdlib.h>
#include <stdbool.h>

#define REQUIRED_CURL
#include <curl/curl.h>
#include <json-c/json.h>

#include "FetchingModule.h"

typedef struct {
	char* title;
	char* body;
	char** streams;
	int streamCount;
	char* clientId;
	char* clientSecret;
} TwitchConfig;

bool twitchTemplate(FetchingModule*);

#endif // ifndef TWITCH_H
