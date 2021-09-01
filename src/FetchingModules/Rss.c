#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "../GlobalManagers.h"
#include "Rss.h"

typedef struct {
	char* title;
	char* sourceName;
	char* url;
} RssNotificationData;

typedef struct {
	int year, month, day, hour, minute, second;
} RssDate;

void rssFillNotificationData(RssNotificationData* notificationData, xmlNode* itemNode) {
	xmlNode* infoNode = itemNode->children;

	while(infoNode != NULL) {
		const char* infoNodeName = infoNode->name;

		if(!strcmp(infoNodeName, "title")) {
			notificationData->title = xmlNodeGetContent(infoNode);
		} else if(!strcmp(infoNodeName, "link")) {
			notificationData->url = xmlNodeGetContent(infoNode);
		}

		infoNode = infoNode->next;
	}
}

const char* rssGetDateFromNode(xmlNode* itemNode) {
	xmlNode* infoNode = itemNode->children;

	while(infoNode != NULL) {
		const char* infoNodeName = infoNode->name;

		if(!strcmp(infoNodeName, "pubDate")) {
			return xmlNodeGetContent(infoNode);
		}

		infoNode = infoNode->next;
	}
}

char* rssFormatDate(const char* pubDate) {
	RssDate dateStruct;
	char* dateString;
	char* unformattedMonth = malloc(16);
	const char* monthShortNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	sscanf(pubDate, "%*s %d %s %d %d:%d:%d %*s", &dateStruct.day, unformattedMonth, &dateStruct.year, &dateStruct.hour, &dateStruct.minute, &dateStruct.second);

	for(int i = 0; i < sizeof monthShortNames / sizeof *monthShortNames; i++) {
		if(!strcmp(unformattedMonth, monthShortNames[i])) {
			dateStruct.month = i + 1;
			break;
		}
	}

	free(unformattedMonth);
	dateString = malloc(strlen("RRRR-MM-DD HH:MM:SS") + 1);
	sprintf(dateString, "%04d-%02d-%02d %02d:%02d:%02d", dateStruct.year, dateStruct.month, dateStruct.day, dateStruct.hour, dateStruct.minute, dateStruct.second);

	return dateString;
}

int rssCompareDates(char* a, char* b) {
	return strcmp(a, b);
}

char* rssGenerateLastReadKeyName(char* url) {
	char* sectionName = malloc(strlen("last_read_") + strlen(url) + 1);
	sprintf(sectionName, "last_read_%s", url);
	return sectionName;
}

bool rssParseConfig(FetchingModule* fetchingModule, Map* configToParse) {
	RssConfig* config = malloc(sizeof *config);
	fetchingModule->config = config;

	char* title       = getFromMap(configToParse, "title",    strlen("title"));
	char* body        = getFromMap(configToParse, "body",     strlen("body"));
	char* sourcesRaw  = getFromMap(configToParse, "sources",  strlen("sources"));
	char* interval    = getFromMap(configToParse, "interval", strlen("interval"));
	char* display     = getFromMap(configToParse, "display",  strlen("display"));

	if(title == NULL || body == NULL || sourcesRaw == NULL || interval == NULL || display == NULL) {
		destroyMap(configToParse);
		return false;
	}

	fetchingModule->display = getDisplay(&displayManager, display);
	if(fetchingModule->display == NULL) {
		destroyMap(configToParse);
		return false;
	}

	char** sources;

	config->title        = strdup(title);
	config->body         = strdup(body);
	config->sourceCount  = split(sourcesRaw, "; ", &sources);
	fetchingModule->intervalSecs  = atoi(interval);

	config->sources = malloc(config->sourceCount * sizeof *config->sources);
	for(int i = 0; i < config->sourceCount; i++) {
		config->sources[i].url = sources[i];

		char* sectionName = rssGenerateLastReadKeyName(sources[i]);
		config->sources[i].lastRead = strdup(stashGetString("rss", sectionName, "1970-01-01 00:00:00"));
		free(sectionName);
	}

	free(sources);
	return true;
}

bool rssEnable(FetchingModule* fetchingModule) {
	RssConfig* config = fetchingModule->config;
	bool retVal = (config->curl = curl_easy_init()) != NULL;

	if(retVal) {
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, networkCallback);

		retVal = fetchingModuleCreateThread(fetchingModule);
		printf("Rss enabled\n");
	}

	return retVal;
}

char* rssReplaceVariables(char* text, RssNotificationData* notificationData) {
	char* temp = replace(text, "<title>", notificationData->title);
	char* ret = replace(temp, "<source-name>", notificationData->sourceName);
	free(temp);
	return ret;
}

void rssDisplayNotification(FetchingModule* fetchingModule, RssNotificationData* notificationData) {
	RssConfig* config = fetchingModule->config;
	Message* message = malloc(sizeof *message);

	message->title = rssReplaceVariables(config->title, notificationData);
	message->text = rssReplaceVariables(config->body, notificationData);
	message->actionData = strdup(notificationData->url);
	message->actionType = URL;
	fetchingModule->display->displayMessage(message, defaultMessageFreeFunction);
}

void rssFetch(FetchingModule* fetchingModule) {
	RssConfig* config = fetchingModule->config;
	RssNotificationData notificationData;
	Message message;
	NetworkResponse response = {NULL, 0};

	// TODO: use curl_multi instead

	// getting response
	for(int i = 0; i < config->sourceCount; i++) {
		curl_easy_setopt(config->curl, CURLOPT_URL, config->sources[i].url);
		curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
		CURLcode code = curl_easy_perform(config->curl);

		// parsing response
		if(code == CURLE_OK) {
			xmlDoc* doc;
			xmlXPathContext* xPathContext;
			xmlXPathObject* xPathObject;

			doc = xmlReadMemory(response.data, response.size, config->sources[i].url, NULL, 0);
			xPathContext = xmlXPathNewContext(doc);

			xPathObject = xmlXPathEvalExpression("/rss/channel/title", xPathContext);
			char* title = strdup(xmlNodeGetContent(xPathObject->nodesetval->nodeTab[0]));
			xmlXPathFreeObject(xPathObject);

			xPathObject = xmlXPathEvalExpression("/rss/channel/item", xPathContext);
			RssNotificationData* newMessages = malloc(xPathObject->nodesetval->nodeNr * sizeof *newMessages);

			xmlNodeSet* nodes = xPathObject->nodesetval;
			int unreadMessagesPointer = 0;
			char* newLastRead = NULL;
			for(; unreadMessagesPointer < nodes->nodeNr; unreadMessagesPointer++) {
				char* formattedDate = rssFormatDate(rssGetDateFromNode(nodes->nodeTab[unreadMessagesPointer]));
				bool isUnread = rssCompareDates(config->sources[i].lastRead, formattedDate) < 0;

				if(isUnread && unreadMessagesPointer == 0) { // we are assuming chronological order
					newLastRead = formattedDate;
				} else {
					free(formattedDate);
				}
				if(!isUnread) {
					break;
				}
			}

			unreadMessagesPointer--;
			if(newLastRead != NULL) { // unread messages exist
				char* sectionName = rssGenerateLastReadKeyName(config->sources[i].url);
				stashSetString("rss", sectionName, newLastRead);
				free(sectionName);
				free(newLastRead);
			}

			for(int j = unreadMessagesPointer - 1; j >= 0; j--) {
				bzero(&newMessages[j], sizeof newMessages[j]);
				rssFillNotificationData(&newMessages[j], nodes->nodeTab[j]);
				newMessages[j].sourceName = title;
				rssDisplayNotification(fetchingModule, &newMessages[j]);
			}

			free(title);
			xmlXPathFreeObject(xPathObject);
			xmlXPathFreeContext(xPathContext);
			xmlFreeDoc(doc);

		} else {
			fprintf(stderr, "[RSS] Request failed with code %d\n", code);
		}
		free(response.data);
	}
}

bool rssDisable(FetchingModule* fetchingModule) {
	RssConfig* config = fetchingModule->config;

	curl_easy_cleanup(config->curl);

	bool retVal = fetchingModuleDestroyThread(fetchingModule);

	for(int i = 0; i < config->sourceCount; i++) {
		free(config->sources[i].url);
	}

	if(retVal) {
		free(config->title);
		free(config->body);
		free(config->sources);
		free(config);
		printf("Rss disabled\n");
	}

	return retVal;
}

bool rssTemplate(FetchingModule* fetchingModule) {
	fetchingModule->intervalSecs = 1;
	fetchingModule->config = NULL;
	fetchingModule->busy = false;
	fetchingModule->parseConfig = rssParseConfig;
	fetchingModule->enable = rssEnable;
	fetchingModule->fetch = rssFetch;
	fetchingModule->display = NULL;
	fetchingModule->disable = rssDisable;

	return true;
}
