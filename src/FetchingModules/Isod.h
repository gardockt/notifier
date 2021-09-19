#ifndef ISOD_H
#define ISOD_H

#ifdef ENABLE_MODULE_ISOD

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

#endif // ifdef ENABLE_MODULE_ISOD

#endif // ifndef ISOD_H
