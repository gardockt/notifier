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
	return ret;
}

void rssDisplayNotification(FetchingModule* fetchingModule, RssNotificationData* notificationData) {
	Message message;

	memset(&message, 0, sizeof message);
	moduleFillBasicMessage(fetchingModule, &message, rssReplaceVariables, notificationData);
	message.actionData = strdup(notificationData->url);
	message.actionType = URL;
	fetchingModule->display->displayMessage(&message);

	moduleDestroyBasicMessage(&message);
	free(message.actionData);
}

void rssFetch(FetchingModule* fetchingModule) {
	RssConfig* config = fetchingModule->config;
	RssNotificationData notificationData;
	NetworkResponse response = {NULL, 0};

	// TODO: use curl_multi instead

	// getting response
	for(int i = 0; i < config->sourceCount; i++) {
		response.data = NULL;
		curl_easy_setopt(config->curl, CURLOPT_URL, config->sources[i].url);
		curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
		CURLcode code = curl_easy_perform(config->curl);

		// parsing response
		if(code == CURLE_OK) {
			xmlDoc* doc;
			xmlXPathContext* xPathContext;
			xmlXPathObject* xPathObject;

			doc = xmlReadMemory(response.data, response.size, config->sources[i].url, NULL, 0);
			if(doc != NULL) {
				xPathContext = xmlXPathNewContext(doc);

				xPathObject = xmlXPathEvalExpression("/rss/channel/title", xPathContext);
				char* title = xmlNodeGetContent(xPathObject->nodesetval->nodeTab[0]);
				xmlXPathFreeObject(xPathObject);

				xPathObject = xmlXPathEvalExpression("/rss/channel/item", xPathContext);
				xmlNodeSet* nodes = xPathObject->nodesetval;
				int unreadMessagesPointer = 0;
				char* newLastRead = NULL;
				for(; unreadMessagesPointer < nodes->nodeNr; unreadMessagesPointer++) {
					char* unformattedDate = rssGetDateFromNode(nodes->nodeTab[unreadMessagesPointer]);
					char* formattedDate = rssFormatDate(unformattedDate);
					bool isUnread = rssCompareDates(config->sources[i].lastRead, formattedDate) < 0;

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
					char* sectionName = rssGenerateLastReadKeyName(config->sources[i].url);
					stashSetString("rss", sectionName, newLastRead);
					free(sectionName);
					free(newLastRead);
				}

				for(int j = unreadMessagesPointer - 1; j >= 0; j--) {
					RssNotificationData notificationData;
					memset(&notificationData, 0, sizeof notificationData);
					rssFillNotificationData(&notificationData, nodes->nodeTab[j]);
					notificationData.sourceName = title;
					rssDisplayNotification(fetchingModule, &notificationData);
				}

				free(title);
				xmlXPathFreeObject(xPathObject);
				xmlXPathFreeContext(xPathContext);
				xmlFreeDoc(doc);
			} else {
				moduleLog(fetchingModule, 0, "Error parsing feed %s", config->sources[i].url);
			}
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
