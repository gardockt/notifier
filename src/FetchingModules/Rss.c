#ifdef ENABLE_MODULE_RSS

#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "Extras/FetchingModuleUtilities.h"
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

char* rssGetDateFromNode(xmlNode* itemNode) {
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

	char* sourcesRaw = getFromMap(configToParse, "sources",  strlen("sources"));
	if(sourcesRaw == NULL) {
		moduleLog(fetchingModule, 0, "Invalid sources");
		return false;
	}

	char** sources;
	config->sourceCount = split(sourcesRaw, FETCHING_MODULE_LIST_ENTRY_SEPARATORS, &sources);
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

bool rssEnable(FetchingModule* fetchingModule, Map* configToParse) {
	if(!rssParseConfig(fetchingModule, configToParse)) {
		return false;
	}

	RssConfig* config = fetchingModule->config;
	bool retVal = (config->curl = curl_easy_init()) != NULL;

	if(retVal) {
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, networkCallback);
	}

	return retVal;
}

char* rssReplaceVariables(char* text, void* notificationDataPtr) {
	RssNotificationData* notificationData = notificationDataPtr;
	char* temp = replace(text, "<title>", notificationData->title);
	char* ret = replace(temp, "<source-name>", notificationData->sourceName);
	free(temp);
	temp = ret;
	ret = replace(temp, "<url>", notificationData->url);
	free(temp);
	return ret;
}

void rssDisplayNotification(FetchingModule* fetchingModule, RssNotificationData* notificationData) {
	Message message = {0};
	moduleFillBasicMessage(fetchingModule, &message, rssReplaceVariables, notificationData, URL, notificationData->url);
	fetchingModule->display->displayMessage(&message);
	moduleDestroyBasicMessage(&message);
}

void rssParseResponse(FetchingModule* fetchingModule, char* response, int responseSize, int sourceIndex) {
	RssConfig* config = fetchingModule->config;
	xmlXPathContext* xPathContext;
	xmlXPathObject* xPathObject;
	xmlDoc* doc = xmlReadMemory(response, responseSize, config->sources[sourceIndex].url, NULL, 0);

	if(doc != NULL) {
		xPathContext = xmlXPathNewContext(doc);
		xPathObject = xmlXPathEvalExpression("/rss/channel/title", xPathContext);

		if(xPathObject->nodesetval->nodeTab != NULL) {
			char* title = xmlNodeGetContent(xPathObject->nodesetval->nodeTab[0]);
			xmlXPathFreeObject(xPathObject);

			xPathObject = xmlXPathEvalExpression("/rss/channel/item", xPathContext);
			xmlNodeSet* nodes = xPathObject->nodesetval;
			int unreadMessagesPointer = 0;
			char* newLastRead = NULL;
			for(; unreadMessagesPointer < nodes->nodeNr; unreadMessagesPointer++) {
				char* unformattedDate = rssGetDateFromNode(nodes->nodeTab[unreadMessagesPointer]);
				char* formattedDate = rssFormatDate(unformattedDate);
				bool isUnread = rssCompareDates(config->sources[sourceIndex].lastRead, formattedDate) < 0;

				free(unformattedDate);
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
				char* sectionName = rssGenerateLastReadKeyName(config->sources[sourceIndex].url);
				stashSetString("rss", sectionName, newLastRead);
				free(sectionName);
				free(newLastRead);
			}

			for(int j = unreadMessagesPointer - 1; j >= 0; j--) {
				RssNotificationData notificationData = {0};
				rssFillNotificationData(&notificationData, nodes->nodeTab[j]);
				notificationData.sourceName = title;
				rssDisplayNotification(fetchingModule, &notificationData);
			}

			free(title);
			xmlXPathFreeContext(xPathContext);
		} else {
			moduleLog(fetchingModule, 0, "Could not get title of feed %s", config->sources[sourceIndex].url);
		}

		xmlXPathFreeObject(xPathObject);
		xmlFreeDoc(doc);
	} else {
		moduleLog(fetchingModule, 0, "Error parsing feed %s", config->sources[sourceIndex].url);
	}
}

void rssFetch(FetchingModule* fetchingModule) {
	RssConfig* config = fetchingModule->config;

	// TODO: use curl_multi instead
	for(int i = 0; i < config->sourceCount; i++) {
		NetworkResponse response = {NULL, 0};
		curl_easy_setopt(config->curl, CURLOPT_URL, config->sources[i].url);
		curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
		CURLcode code = curl_easy_perform(config->curl);

		if(code == CURLE_OK) {
			moduleLog(fetchingModule, 3, "Received response:\n%s", response.data);
			rssParseResponse(fetchingModule, response.data, response.size, i); 
		} else {
			moduleLog(fetchingModule, 0, "Request failed with code %d", code);
		}
		free(response.data);
	}
}

void rssDisable(FetchingModule* fetchingModule) {
	RssConfig* config = fetchingModule->config;

	curl_easy_cleanup(config->curl);

	for(int i = 0; i < config->sourceCount; i++) {
		free(config->sources[i].url);
		free(config->sources[i].lastRead);
	}

	free(config->sources);
	free(config);
}

bool rssTemplate(FetchingModule* fetchingModule) {
	memset(fetchingModule, 0, sizeof *fetchingModule);

	fetchingModule->enable = rssEnable;
	fetchingModule->fetch = rssFetch;
	fetchingModule->disable = rssDisable;

	return true;
}

#endif // ifdef ENABLE_MODULE_RSS
