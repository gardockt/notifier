#include <stdlib.h>
#include <stdbool.h>

#include <curl/curl.h>
#include <json-c/json.h>

#include "Utilities/Json.h"
#include "Utilities/Network.h"
#include "FetchingModule.h"

#define SORT_DATE(date) {date[6], date[7], date[8], date[9], date[3], date[4], date[0], date[1], date[11], date[12], date[14], date[15], '\0'}

typedef struct {
	char* username;
	char* token;
	CURL* curl;
	char* last_read;
	char* max_messages;

	char* title_template;
	char* body_template;
	char* icon_path;
} Config;

typedef struct {
	const char* title;
} NotificationData;

static int compare_dates(const char* a, const char* b) {
	/* sort dates to YYYYMMDDHHmm so we can compare them with strcmp */
	char a_sorted[] = SORT_DATE(a);
	char b_sorted[] = SORT_DATE(b);
	return strcmp(a_sorted, b_sorted);
}

static char* generate_url(const FetchingModule* module) {
	Config* config = fm_get_data(module);
	char* url = malloc(strlen("https://isod.ee.pw.edu.pl/isod-portal/wapi?q=mynewsheaders&username=") + strlen(config->username) + strlen("&apikey=") + strlen(config->token) + strlen("&from=0&to=") + strlen(config->max_messages) + 1);
	sprintf(url, "https://isod.ee.pw.edu.pl/isod-portal/wapi?q=mynewsheaders&username=%s&apikey=%s&from=0&to=%s", config->username, config->token, config->max_messages);
	return url;
}

static char* generate_last_read_key_name(const FetchingModule* module) {
	Config* config = fm_get_data(module);
	char* section_name = malloc(strlen("last_read_") + strlen(config->username) + 1);
	sprintf(section_name, "last_read_%s", config->username);
	return section_name;
}

static bool parse_config(FetchingModule* module) {
	Config* config = malloc(sizeof *config);
	memset(config, 0, sizeof *config);

	if(!fm_get_config_string_log(module, "username", &config->username, 0) ||
	   !fm_get_config_string_log(module, "token", &config->token, 0) ||
	   !fm_get_config_string_log(module, "max_messages", &config->max_messages, 0) ||
	   !fm_get_config_string_log(module, "title", &config->title_template, 0) ||
	   !fm_get_config_string_log(module, "body", &config->body_template, 0)) {
		return false;
	}

	fm_get_config_string(module, "icon", &config->icon_path);

	char* section_name = generate_last_read_key_name(module);
	config->last_read = strdup(fm_get_stash_string(module, "isod", section_name, "01.01.1970 00:00"));
	free(section_name);
	
	fm_set_data(module, config);
	return true;
}

void configure(FMConfig* config) {
	fm_config_set_name(config, "isod");
}

bool enable(FetchingModule* module) {
	if(!parse_config(module)) {
		return false;
	}

	Config* config = fm_get_data(module);
	bool success = (config->curl = curl_easy_init()) != NULL;
	char* url = generate_url(module);

	if(success) {
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, network_callback);
		curl_easy_setopt(config->curl, CURLOPT_URL, url);
	}

	free(url);
	return success;
}

static char* replace_variables(const FetchingModule* module, const char* text, const NotificationData* notification_data) {
	return fm_replace(module, text, 1, "<title>", notification_data->title);
}

static void display_notification(const FetchingModule* module, NotificationData* notification_data) {
	Config* config = fm_get_data(module);
	Message* message = fm_new_message();
	message->title = replace_variables(module, config->title_template, notification_data);
	message->body = replace_variables(module, config->body_template, notification_data);
	message->icon_path = config->icon_path;
	fm_display_message(module, message);
	free(message->title);
	free(message->body);
	fm_free_message(message);
}

static void parse_response(FetchingModule* module, const char* response) {
	Config* config = fm_get_data(module);
	json_object* root = json_tokener_parse(response);
	json_object* items;

	if(!json_read_array(root, "items", &items)) {
		fm_log(module, 0, "Invalid response");
		return; /* TODO: memory leak? */
	}

	int notification_count = json_object_array_length(items);
	char* new_last_read = NULL;

	for(int i = notification_count - 1; i >= 0; i--) {
		json_object* notification = json_object_array_get_idx(items, i);
		if(json_object_get_type(notification) != json_type_object) {
			fm_log(module, 0, "Invalid notification object");
			continue;
		}

		const char* modified_date;
		char* modified_date_with_fixed_format;

		if(!json_read_string(notification, "modifiedDate", &modified_date)) {
			fm_log(module, 0, "Invalid modification date in notification %d", i);
			continue;
		}

		if(modified_date[1] == '.') { /* day is one digit long */
			modified_date_with_fixed_format = malloc(strlen("01.01.1970 00:00") + 1);
			sprintf(modified_date_with_fixed_format, "0%s", modified_date);
		} else {
			modified_date_with_fixed_format = strdup(modified_date);
		}

		if(compare_dates(modified_date_with_fixed_format, config->last_read) > 0) {
			if(new_last_read == NULL || compare_dates(modified_date_with_fixed_format, new_last_read) > 0) {
				free(new_last_read);
				new_last_read = strdup(modified_date_with_fixed_format);
			}

			NotificationData notification_data = {0};
			if(json_read_string(notification, "subject", &notification_data.title)) {
				display_notification(module, &notification_data);
			} else {
				fm_log(module, 0, "Invalid title in notification %d", i);
			}
		}

		free(modified_date_with_fixed_format);
	}
	json_object_put(root);
	if(new_last_read != NULL) {
		config->last_read = new_last_read;
		char* section_name = generate_last_read_key_name(module);
		fm_set_stash_string(module, "isod", section_name, config->last_read);
		free(section_name);
	}
}

void fetch(FetchingModule* module) {
	Config* config = fm_get_data(module);
	NetworkResponse response = {NULL, 0};

	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	CURLcode code = curl_easy_perform(config->curl);

	if(code == CURLE_OK) {
		fm_log(module, 3, "Received response:\n%s", response.data);
		parse_response(module, response.data);
		free(response.data);
	} else {
		fm_log(module, 0, "Request failed with code %d", code);
	}
}

void disable(FetchingModule* module) {
	Config* config = fm_get_data(module);

	curl_easy_cleanup(config->curl);

	free(config->max_messages);
	free(config->last_read);
	free(config->username);
	free(config->token);
	free(config->title_template);
	free(config->body_template);
	free(config->icon_path);
	free(config);
}
