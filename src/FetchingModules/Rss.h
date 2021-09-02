#ifndef RSS_H
#define RSS_H

#include <stdlib.h>
#include <stdbool.h>

#define REQUIRED_CURL
#define REQUIRED_LIBXML
#include <curl/curl.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "FetchingModule.h"

typedef struct {
	char* url;
	char* lastRead;
} RssSource;

typedef struct {
	RssSource* sources;
	int sourceCount;
	CURL* curl;
} RssConfig;

bool rssTemplate(FetchingModule*);

#endif // ifndef RSS_H
