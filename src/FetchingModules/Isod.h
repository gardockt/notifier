#ifndef ISOD_H
#define ISOD_H

#include <stdlib.h>
#include <stdbool.h>

#define REQUIRED_CURL
#include <curl/curl.h>
#include <json-c/json.h>

#include "FetchingModule.h"

typedef struct {
	char* username;
	char* token;
	CURL* curl;
	char* lastRead;
	char* maxMessages;
} IsodConfig;

bool isodTemplate(FetchingModule*);

#endif // ifndef ISOD_H
