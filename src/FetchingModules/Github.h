#ifndef GITHUB_H
#define GITHUB_H

#ifndef DISABLE_MODULE_GITHUB

#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define REQUIRED_CURL
#include <curl/curl.h>
#include <json-c/json.h>

#include "FetchingModule.h"

typedef struct {
	char* token;
	CURL* curl;
	struct curl_slist* list;
	char* lastRead;
} GithubConfig;

bool githubTemplate(FetchingModule*);

#endif // ifndef DISABLE_MODULE_GITHUB

#endif // ifndef GITHUB_H
