#ifdef ENABLE_MODULE_RSS

#include "../Structures/Map.h"
#include "../StringOperations.h"
#include "../Stash.h"
#include "../Network.h"
#include "../Globals.h"
#include "Extras/FetchingModuleUtilities.h"
#include "Rss.h"

#define RSS_FORMATTED_DATE_TEMPLATE "YYYY-MM-DD HH:MM:SS"

typedef struct {
	char* title;
	char* sourceName;
	char* url;
} RssNotificationData;

typedef struct {
	int year, month, day, hour, minute, second;
} RssDate;

typedef enum {
	UNKNOWN,
	RSS,
	ATOM
} RssStandard;

typedef struct {
	RssStandard standard;
	const char* title;
	const char* items;
	const char* dateNode;
	const char* titleNode;
	const char* urlNode;
	const char* defaultDate;
} RssStandardInfo;

char* rssBase16Table = "0123456789abcdef";

// base 16 encoding's output is 2x longer than input

char* rssBase16Encode(char* input) {
	if(input == NULL) {
		return NULL;
	}

	char* output = malloc(2 * strlen(input) + 1);
	int inputPointer = 0;
	do {
		output[inputPointer * 2 + 0] = rssBase16Table[input[inputPointer] >> 4];
		output[inputPointer * 2 + 1] = rssBase16Table[input[inputPointer] & ((1 << 4) - 1)];
	} while(input[++inputPointer] != '\0');
	output[inputPointer * 2] = '\0';
	return output;
}

void rssFillNotificationData(RssNotificationData* notificationData, xmlNode* itemNode, RssStandardInfo* standardData) {
	xmlNode* infoNode = itemNode->children;

	while(infoNode != NULL) {
		const char* infoNodeName = infoNode->name;

		if(!strcmp(infoNodeName, standardData->titleNode)) {
			notificationData->title = xmlNodeGetContent(infoNode);
		} else if(!strcmp(infoNodeName, standardData->urlNode)) {
			notificationData->url = xmlNodeGetContent(infoNode);
		}

		infoNode = infoNode->next;
	}
}

char* rssGetDateFromNode(xmlNode* itemNode, RssStandardInfo* standardData) {
	xmlNode* infoNode = itemNode->children;

	while(infoNode != NULL) {
		const char* infoNodeName = infoNode->name;

		if(!strcmp(infoNodeName, standardData->dateNode)) {
			return xmlNodeGetContent(infoNode);
		}

		infoNode = infoNode->next;
	}
}

char* rssFormatDate(const char* unformattedDate, RssStandardInfo* standardInfo) {
	int desiredLength = strlen(RSS_FORMATTED_DATE_TEMPLATE);
	char* dateString = malloc(desiredLength + 1);

	if(standardInfo->standard == RSS) { // date format: Tue, 08 Jun 2021 06:32:03 +0000
		RssDate dateStruct;
		char* unformattedMonth = malloc(16);
		const char* monthShortNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

		sscanf(unformattedDate, "%*s %d %s %d %d:%d:%d %*s", &dateStruct.day, unformattedMonth, &dateStruct.year, &dateStruct.hour, &dateStruct.minute, &dateStruct.second);

		for(int i = 0; i < sizeof monthShortNames / sizeof *monthShortNames; i++) {
			if(!strcmp(unformattedMonth, monthShortNames[i])) {
				dateStruct.month = i + 1;
				break;
			}
		}

		free(unformattedMonth);
		sprintf(dateString, "%04d-%02d-%02d %02d:%02d:%02d", dateStruct.year, dateStruct.month, dateStruct.day, dateStruct.hour, dateStruct.minute, dateStruct.second);
	} else if(standardInfo->standard == ATOM) {// date format: 1970-01-01T00:00:00+00:00
		strncpy(dateString, unformattedDate, desiredLength);
		dateString[10] = ' '; // replace T with space
		dateString[desiredLength] = '\0';
	}

	return dateString;
}

int rssCompareDates(char* a, char* b) {
	return strcmp(a, b);
}

char* rssGenerateLastReadKeyName(char* url) {
	char* encodedUrl = rssBase16Encode(url);
	char* sectionName = malloc(strlen("last_read_") + strlen(encodedUrl) + 1);
	sprintf(sectionName, "last_read_%s", encodedUrl);
	free(encodedUrl);
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
	config->sourceCount = split(sourcesRaw, LIST_ENTRY_SEPARATORS, &sources);
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
	return replace(text, 3,
		"<title>", notificationData->title,
		"<source-name>", notificationData->sourceName,
		"<url>", notificationData->url);
}

void rssDisplayNotification(FetchingModule* fetchingModule, RssNotificationData* notificationData) {
	Message message = {0};
	moduleFillBasicMessage(fetchingModule, &message, rssReplaceVariables, notificationData, URL, notificationData->url);
	fetchingModule->display->displayMessage(&message);
	moduleDestroyBasicMessage(&message);
}

void rssDetectStandard(xmlXPathContext* xmlContext, RssStandardInfo* output) {
	xmlXPathObject* xmlObject;
	bool found = false;

	// RSS
	xmlObject = xmlXPathEvalExpression("/rss", xmlContext);
	if(xmlObject->nodesetval->nodeNr > 0) {
		output->standard = RSS;
		output->title = "/rss/channel/title";
		output->items = "/rss/channel/item";
		output->dateNode = "pubDate";
		output->titleNode = "title";
		output->urlNode = "link";
		found = true;
	}
	xmlXPathFreeObject(xmlObject);
	if(found) {
		return;
	}

	// Atom
	xmlXPathRegisterNs(xmlContext, "atom", "http://www.w3.org/2005/Atom");
	xmlObject = xmlXPathEvalExpression("/atom:feed", xmlContext);
	if(xmlObject->nodesetval->nodeNr > 0) {
		output->standard = ATOM;
		output->title = "/atom:feed/atom:title";
		output->items = "/atom:feed/atom:entry";
		output->dateNode = "updated";
		output->titleNode = "title";
		output->urlNode = "link";
		found = true;
	}
	xmlXPathFreeObject(xmlObject);
	if(found) {
		return;
	}

	output->standard = UNKNOWN;
	return;
}

void rssParseResponse(FetchingModule* fetchingModule, char* response, int responseSize, int sourceIndex) {
	RssConfig* config = fetchingModule->config;
	xmlXPathContext* xPathContext;
	xmlXPathObject* xPathObject;
	xmlDoc* doc = xmlReadMemory(response, responseSize, config->sources[sourceIndex].url, NULL, 0);
	RssStandardInfo standardData = {0};

	if(doc == NULL) {
		moduleLog(fetchingModule, 0, "Error parsing feed %s", config->sources[sourceIndex].url);
		return;
	}

	xPathContext = xmlXPathNewContext(doc);
	rssDetectStandard(xPathContext, &standardData);
	if(standardData.standard == UNKNOWN) {
		moduleLog(fetchingModule, 0, "Unrecognized format of feed %s", config->sources[sourceIndex].url);
		goto xmlParseResponseFreeDoc;
	}

	xPathObject = xmlXPathEvalExpression(standardData.title, xPathContext);
	if(xPathObject->nodesetval->nodeTab == NULL) {
		moduleLog(fetchingModule, 0, "Could not get title of feed %s", config->sources[sourceIndex].url);
		goto xmlParseResponseFreeObject;
	}

	char* title = xmlNodeGetContent(xPathObject->nodesetval->nodeTab[0]);
	xmlXPathFreeObject(xPathObject);

	xPathObject = xmlXPathEvalExpression(standardData.items, xPathContext);
	xmlNodeSet* nodes = xPathObject->nodesetval;
	int unreadMessagesPointer = 0;
	char* newLastRead = NULL;
	for(; unreadMessagesPointer < nodes->nodeNr; unreadMessagesPointer++) {
		char* unformattedDate = rssGetDateFromNode(nodes->nodeTab[unreadMessagesPointer], &standardData);
		if(unformattedDate == NULL) {
			moduleLog(fetchingModule, 0, "Could not get date for feed %s, message %d", config->sources[sourceIndex].url, unreadMessagesPointer);
			break;
		}
		char* formattedDate = rssFormatDate(unformattedDate, &standardData);
		bool isUnread = rssCompareDates(config->sources[sourceIndex].lastRead, formattedDate) < 0;

		free(unformattedDate);

		if(isUnread) {
			RssNotificationData notificationData = {0};
			rssFillNotificationData(&notificationData, nodes->nodeTab[unreadMessagesPointer], &standardData);
			notificationData.sourceName = title;
			rssDisplayNotification(fetchingModule, &notificationData);

			if(newLastRead == NULL || rssCompareDates(newLastRead, formattedDate) < 0) {
				newLastRead = formattedDate;
			} else {
				free(formattedDate);
			}
		} else {
			free(formattedDate);
		}
	}

	if(newLastRead != NULL) { // unread messages exist
		char* sectionName = rssGenerateLastReadKeyName(config->sources[sourceIndex].url);
		stashSetString("rss", sectionName, newLastRead);
		free(sectionName);

		free(config->sources[sourceIndex].lastRead);
		config->sources[sourceIndex].lastRead = newLastRead;
	}

	free(title);
	xmlXPathFreeContext(xPathContext);

xmlParseResponseFreeObject:
	xmlXPathFreeObject(xPathObject);

xmlParseResponseFreeDoc:
	xmlFreeDoc(doc);
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

void rssTemplate(FetchingModule* fetchingModule) {
	fetchingModule->enable = rssEnable;
	fetchingModule->fetch = rssFetch;
	fetchingModule->disable = rssDisable;
}

#endif // ifdef ENABLE_MODULE_RSS
