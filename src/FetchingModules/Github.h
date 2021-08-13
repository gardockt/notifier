#ifndef GITHUB_H
#define GITHUB_H

#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define REQUIRED_CURL
#include <curl/curl.h>
#include <json-c/json.h>

#include "FetchingModule.h"

typedef struct {
	char* title;
	char* body;
	char* token;
	CURL* curl;
	struct curl_slist* list;
	char* lastRead;
} GithubConfig;

bool githubTemplate(FetchingModule*);

#endif // ifndef GITHUB_H
