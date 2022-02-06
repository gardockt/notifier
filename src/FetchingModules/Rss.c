#include <stdlib.h>
#include <stdbool.h>

#include <curl/curl.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "../Globals.h" /* LIST_ENTRY_SEPARATORS */
#include "Utilities/Network.h"
#include "FetchingModule.h"

#define FORMATTED_DATE_TEMPLATE "YYYY-MM-DD HH:MM:SS"

typedef struct {
	char* url;
	char* last_read;
} Source;

typedef struct {
	Source* sources;
	int source_count;
	CURL* curl;

	char* title_template;
	char* body_template;
	char* icon_path;
} Config;

typedef struct {
	char* title;
	char* source_name;
	char* url;
} NotificationData;

typedef struct {
	int year, month, day, hour, minute, second;
} Date;

typedef enum {
	UNKNOWN,
	RSS,
	ATOM
} RssStandard;

typedef struct {
	RssStandard standard;
	const char* title;
	const char* items;
	const char* date_node;
	const char* title_node;
	const char* url_node;
	const char* default_date;
} RssStandardInfo;

char* base16_table = "0123456789abcdef";

/* base 16 encoding's output is 2x longer than input */

static char* base16_encode(const char* input) {
	if(input == NULL) {
		return NULL;
	}

	char* output = malloc(2 * strlen(input) + 1);
	int input_ptr = 0;
	do {
		output[input_ptr * 2 + 0] = base16_table[input[input_ptr] >> 4];
		output[input_ptr * 2 + 1] = base16_table[input[input_ptr] & ((1 << 4) - 1)];
	} while(input[++input_ptr] != '\0');
	output[input_ptr * 2] = '\0';
	return output;
}

static void fill_notification_data(NotificationData* notification_data, const xmlNode* item_node, const RssStandardInfo* standard_info) {
	xmlNode* info_node = item_node->children;

	while(info_node != NULL) {
		const char* info_node_name = info_node->name;

		if(!strcmp(info_node_name, standard_info->title_node)) {
			notification_data->title = xmlNodeGetContent(info_node);
		} else if(!strcmp(info_node_name, standard_info->url_node)) {
			notification_data->url = xmlNodeGetContent(info_node);
		}

		info_node = info_node->next;
	}
}

static char* get_date_from_node(const xmlNode* item_node, const RssStandardInfo* standard_info) {
	xmlNode* info_node = item_node->children;

	while(info_node != NULL) {
		const char* info_node_name = info_node->name;

		if(!strcmp(info_node_name, standard_info->date_node)) {
			return xmlNodeGetContent(info_node);
		}

		info_node = info_node->next;
	}
}

static char* format_date(const char* unformatted_date, RssStandardInfo* standard_info) {
	int desired_length = strlen(FORMATTED_DATE_TEMPLATE);
	char* date_string = malloc(desired_length + 1);

	if(standard_info->standard == RSS) { /* date format: Tue, 08 Jun 2021 06:32:03 +0000 */
		Date date_struct;
		char* unformatted_month = malloc(16);
		const char* month_short_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

		sscanf(unformatted_date, "%*s %d %s %d %d:%d:%d %*s", &date_struct.day, unformatted_month, &date_struct.year, &date_struct.hour, &date_struct.minute, &date_struct.second);

		for(int i = 0; i < sizeof month_short_names / sizeof *month_short_names; i++) {
			if(!strcmp(unformatted_month, month_short_names[i])) {
				date_struct.month = i + 1;
				break;
			}
		}

		free(unformatted_month);
		sprintf(date_string, "%04d-%02d-%02d %02d:%02d:%02d", date_struct.year, date_struct.month, date_struct.day, date_struct.hour, date_struct.minute, date_struct.second);
	} else if(standard_info->standard == ATOM) {/* date format: 1970-01-01T00:00:00+00:00 */
		strncpy(date_string, unformatted_date, desired_length);
		date_string[10] = ' '; /* replace T with space */
		date_string[desired_length] = '\0';
	}

	return date_string;
}

static int compare_dates(const char* a, const char* b) {
	return strcmp(a, b);
}

static char* generate_last_read_key_name(const char* url) {
	char* encoded_url = base16_encode(url);
	char* section_name = malloc(strlen("last_read_") + strlen(encoded_url) + 1);
	sprintf(section_name, "last_read_%s", encoded_url);
	free(encoded_url);
	return section_name;
}

static bool parse_config(FetchingModule* module) {
	Config* config = malloc(sizeof *config);
	memset(config, 0, sizeof *config);

	char* sources_raw;
	if(!fm_get_config_string_log(module, "sources", &sources_raw, 0) ||
	   !fm_get_config_string_log(module, "title", &config->title_template, 0) ||
	   !fm_get_config_string_log(module, "body", &config->body_template, 0)) {
		return false;
	}

	fm_get_config_string(module, "icon", &config->icon_path);

	char** sources;
	config->source_count = fm_split(module, sources_raw, LIST_ENTRY_SEPARATORS, &sources);
	config->sources = malloc(config->source_count * sizeof *config->sources);
	for(int i = 0; i < config->source_count; i++) {
		config->sources[i].url = sources[i];

		char* section_name = generate_last_read_key_name(sources[i]);
		config->sources[i].last_read = strdup(fm_get_stash_string(module, "rss", section_name, "1970-01-01 00:00:00"));
		free(section_name);
	}

	free(sources);
	fm_set_data(module, config);
	return true;
}

void configure(FMConfig* config) {
	fm_config_set_name(config, "rss");
}

bool enable(FetchingModule* module) {
	if(!parse_config(module)) {
		return false;
	}

	Config* config = fm_get_data(module);
	bool success = (config->curl = curl_easy_init()) != NULL;

	if(success) {
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, network_callback);
	}

	return success;
}

static char* replace_variables(const FetchingModule* module, const char* text, const NotificationData* notification_data) {
	return fm_replace(module, text, 3,
		"<title>", notification_data->title,
		"<source-name>", notification_data->source_name,
		"<url>", notification_data->url);
}

static void display_notification(const FetchingModule* module, const NotificationData* notification_data) {
	Config* config = fm_get_data(module);
	Message message = {
		.title       = replace_variables(module, config->title_template, notification_data),
		.body        = replace_variables(module, config->body_template,  notification_data),
		.icon_path   = config->icon_path,
		.action_data = notification_data->url,
		.action_type = URL,
	};
	fm_display_message(module, &message);
	free(message.title);
	free(message.body);
}

static void detect_rss_standard(xmlXPathContext* xml_context, RssStandardInfo* output) {
	xmlXPathObject* xml_object;
	bool found = false;

	/* RSS */
	xml_object = xmlXPathEvalExpression("/rss", xml_context);
	if(xml_object->nodesetval->nodeNr > 0) {
		output->standard = RSS;
		output->title = "/rss/channel/title";
		output->items = "/rss/channel/item";
		output->date_node = "pubDate";
		output->title_node = "title";
		output->url_node = "link";
		found = true;
	}
	xmlXPathFreeObject(xml_object);
	if(found) {
		return;
	}

	/* Atom */
	xmlXPathRegisterNs(xml_context, "atom", "http://www.w3.org/2005/Atom");
	xml_object = xmlXPathEvalExpression("/atom:feed", xml_context);
	if(xml_object->nodesetval->nodeNr > 0) {
		output->standard = ATOM;
		output->title = "/atom:feed/atom:title";
		output->items = "/atom:feed/atom:entry";
		output->date_node = "updated";
		output->title_node = "title";
		output->url_node = "link";
		found = true;
	}
	xmlXPathFreeObject(xml_object);
	if(found) {
		return;
	}

	output->standard = UNKNOWN;
	return;
}

static void parse_response(const FetchingModule* module, const char* response, int response_size, int source_index) {
	Config* config = fm_get_data(module);
	xmlXPathContext* xpath_context;
	xmlXPathObject* xpath_object;
	xmlDoc* doc = xmlReadMemory(response, response_size, config->sources[source_index].url, NULL, 0);
	RssStandardInfo standard_info = {0};

	if(doc == NULL) {
		fm_log(module, 0, "Error parsing feed %s", config->sources[source_index].url);
		return;
	}

	xpath_context = xmlXPathNewContext(doc);
	detect_rss_standard(xpath_context, &standard_info);
	if(standard_info.standard == UNKNOWN) {
		fm_log(module, 0, "Unrecognized format of feed %s", config->sources[source_index].url);
		goto xml_parse_response_free_doc;
	}

	xpath_object = xmlXPathEvalExpression(standard_info.title, xpath_context);
	if(xpath_object->nodesetval->nodeTab == NULL) {
		fm_log(module, 0, "Could not get title of feed %s", config->sources[source_index].url);
		goto xml_parse_response_free_object;
	}

	char* title = xmlNodeGetContent(xpath_object->nodesetval->nodeTab[0]);
	xmlXPathFreeObject(xpath_object);

	xpath_object = xmlXPathEvalExpression(standard_info.items, xpath_context);
	xmlNodeSet* nodes = xpath_object->nodesetval;
	int unread_messages_ptr = 0;
	char* new_last_read = NULL;
	for(; unread_messages_ptr < nodes->nodeNr; unread_messages_ptr++) {
		char* unformatted_date = get_date_from_node(nodes->nodeTab[unread_messages_ptr], &standard_info);
		if(unformatted_date == NULL) {
			fm_log(module, 0, "Could not get date for feed %s, message %d", config->sources[source_index].url, unread_messages_ptr);
			break;
		}
		char* formatted_date = format_date(unformatted_date, &standard_info);
		bool is_unread = compare_dates(config->sources[source_index].last_read, formatted_date) < 0;

		free(unformatted_date);

		if(is_unread) {
			NotificationData notification_data = {0};
			fill_notification_data(&notification_data, nodes->nodeTab[unread_messages_ptr], &standard_info);
			notification_data.source_name = title;
			display_notification(module, &notification_data);

			if(new_last_read == NULL || compare_dates(new_last_read, formatted_date) < 0) {
				new_last_read = formatted_date;
			} else {
				free(formatted_date);
			}
		} else {
			free(formatted_date);
		}
	}

	if(new_last_read != NULL) { /* unread messages exist */
		char* section_name = generate_last_read_key_name(config->sources[source_index].url);
		fm_set_stash_string(module, "rss", section_name, new_last_read);
		free(section_name);

		free(config->sources[source_index].last_read);
		config->sources[source_index].last_read = new_last_read;
	}

	free(title);
	xmlXPathFreeContext(xpath_context);

xml_parse_response_free_object:
	xmlXPathFreeObject(xpath_object);

xml_parse_response_free_doc:
	xmlFreeDoc(doc);
}

void fetch(const FetchingModule* module) {
	Config* config = fm_get_data(module);

	/* TODO: use curl_multi instead */
	for(int i = 0; i < config->source_count; i++) {
		NetworkResponse response = {NULL, 0};
		curl_easy_setopt(config->curl, CURLOPT_URL, config->sources[i].url);
		curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
		CURLcode code = curl_easy_perform(config->curl);

		if(code == CURLE_OK) {
			fm_log(module, 3, "Received response:\n%s", response.data);
			parse_response(module, response.data, response.size, i); 
		} else {
			fm_log(module, 0, "Request failed with code %d", code);
		}
		free(response.data);
	}
}

void disable(const FetchingModule* module) {
	Config* config = fm_get_data(module);

	curl_easy_cleanup(config->curl);

	for(int i = 0; i < config->source_count; i++) {
		free(config->sources[i].url);
		free(config->sources[i].last_read);
	}

	free(config->sources);
	free(config->title_template);
	free(config->body_template);
	free(config->icon_path);
	free(config);
}
