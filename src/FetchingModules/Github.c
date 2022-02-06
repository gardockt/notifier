#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include <curl/curl.h>
#include <json-c/json.h>

#include "Utilities/Json.h"
#include "Utilities/Network.h"
#include "FetchingModule.h"

#define GITHUB_API_URL "https://api.github.com/notifications"
#define GITHUB_LAST_UPDATED_DESIRED_LENGTH strlen("1970-01-01T00:00:00Z")

typedef struct {
	char* token;
	CURL* curl;
	struct curl_slist* list;
	char* last_read;

	char* title_template;
	char* body_template;
	char* icon_path;
} Config;

typedef struct {
	const char* title;
	const char* repo_name;
	const char* repo_full_name;
	char* url;
} NotificationData;

static int compare_dates(const char* a, const char* b) {
	return strcmp(a, b);
}

static char* generate_notification_url(FetchingModule* module, json_object* notification) {
	Config* config = fm_get_data(module);
	NetworkResponse response = {NULL, 0};
	char* ret = NULL;

	json_object* subject;
	const char* comment_api_url;
	const char* comment_url;

	if(!json_read_object(notification, "subject", &subject) ||
	   !json_read_string(subject, "latest_comment_url", &comment_api_url)) {
		return NULL;
	}

	curl_easy_setopt(config->curl, CURLOPT_URL, comment_api_url);
	curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, (void*)&response);
	CURLcode code = curl_easy_perform(config->curl);
	if(code != CURLE_OK) {
		return NULL;
	}

	json_object* comment_root = json_tokener_parse(response.data);
	if(json_object_get_type(comment_root) != json_type_object) {
		goto generate_notification_url_finish;
	}

	if(!json_read_string(comment_root, "html_url", &comment_url)) {
		goto generate_notification_url_finish;
	}

	ret = strdup(comment_url);
generate_notification_url_finish:
	json_object_put(comment_root);
	free(response.data);
	return ret;
}

static bool parse_config(FetchingModule* module) {
	Config* config = malloc(sizeof *config);
	memset(config, 0, sizeof *config);
	bool success = false;

	fm_get_config_string(module, "icon", &config->icon_path);

	if(!fm_get_config_string_log(module, "token", &config->token, 0) ||
	   !fm_get_config_string_log(module, "title", &config->title_template, 0) ||
	   !fm_get_config_string_log(module, "body", &config->body_template, 0)) {
		goto parse_config_finish;
	}

	config->list = NULL;
	config->last_read = strdup("1970-01-01T00:00:00Z");

	fm_set_data(module, config);
	success = true;
	
parse_config_finish:
	if(!success) {
		free(config);
	}
	return success;
}

void configure(FMConfig* config) {
	fm_config_set_name(config, "github");
}

bool enable(FetchingModule* module) {
	if(!parse_config(module)) {
		fm_log(module, 0, "Failed to parse config");
		return false;
	}

	Config* config = fm_get_data(module);
	bool success = (config->curl = curl_easy_init()) != NULL;

	char* header_user_agent = malloc(strlen("User-Agent: curl") + 1);
	char* header_token = malloc(strlen("Authorization: token ") + strlen(config->token) + 1);

	sprintf(header_user_agent, "User-Agent: curl");
	sprintf(header_token, "Authorization: token %s", config->token);

	if(success) {
		config->list = curl_slist_append(config->list, header_user_agent);
		config->list = curl_slist_append(config->list, header_token);

		curl_easy_setopt(config->curl, CURLOPT_HTTPHEADER, config->list);
		curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, network_callback);
	}

	free(header_user_agent);
	free(header_token);
	return success;
}

static char* replace_variables(FetchingModule* module, const char* text, NotificationData* notification_data) {
	return fm_replace(module, text, 4,
		"<title>", notification_data->title,
		"<repo-name>", notification_data->repo_name,
		"<repo-full-name>", notification_data->repo_full_name,
		"<url>", notification_data->url);
}

static void display_notification(FetchingModule* module, NotificationData* notification_data) {
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

static void parse_response(FetchingModule* module, const char* response) {
	Config* config = fm_get_data(module);
	NotificationData notification_data;
	json_object* root = json_tokener_parse(response);

	if(json_object_get_type(root) != json_type_array) {
		fm_log(module, 0, "Invalid response");
		return;
	}

	int unread_notifications = json_object_array_length(root);
	char* new_last_read = NULL;

	for(int i = 0; i < unread_notifications; i++) {
		json_object* notification = json_object_array_get_idx(root, i);
		const char* last_updated;
		if(!json_read_string(notification, "updated_at", &last_updated) || strlen(last_updated) != GITHUB_LAST_UPDATED_DESIRED_LENGTH) {
			fm_log(module, 0, "Invalid last update time in notification %d", i);
			continue;
		}

		if(compare_dates(last_updated, config->last_read) > 0) {
			json_object* subject;
			if(!json_read_object(notification, "subject", &subject)) {
				fm_log(module, 0, "Invalid subject object in notification %d", i);
				continue;
			}

			json_object* repository;
			if(!json_read_object(notification, "repository", &repository)) {
				fm_log(module, 0, "Invalid repository object in notification %d", i);
				continue;
			}

			if(new_last_read == NULL || compare_dates(last_updated, new_last_read) > 0) {
				free(new_last_read);
				new_last_read = strdup(last_updated);
			}

			const char* title;
			const char* repo_name;
			const char* repo_full_name;

			if(!json_read_string(subject, "title", &title)) {
				fm_log(module, 0, "Invalid title in notification %d", i);
				continue;
			}
			if(!json_read_string(repository, "name", &repo_name)) {
				fm_log(module, 0, "Invalid repository name in notification %d", i);
				continue;
			}
			if(!json_read_string(repository, "full_name", &repo_full_name)) {
				fm_log(module, 0, "Invalid full repository name in notification %d", i);
				continue;
			}

			notification_data.title           = title;
			notification_data.repo_name       = repo_name;
			notification_data.repo_full_name  = repo_full_name;
			notification_data.url             = generate_notification_url(module, notification);
			display_notification(module, &notification_data);
			free(notification_data.url);
		}
	}

	json_object_put(root);
	if(new_last_read != NULL) {
		free(config->last_read);
		config->last_read = new_last_read;
	}
}

void fetch(FetchingModule* module) {
	Config* config = fm_get_data(module);
	NetworkResponse response = {NULL, 0};

	curl_easy_setopt(config->curl, CURLOPT_URL, GITHUB_API_URL); /* as GenerateNotificationUrl can change it */
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

	curl_slist_free_all(config->list);
	curl_easy_cleanup(config->curl);

	free(config->last_read);
	free(config->token);
	free(config->title_template);
	free(config->body_template);
	free(config->icon_path);
	free(config);
}
