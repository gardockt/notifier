#include <pthread.h> /* OAuth mutex */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <curl/curl.h>
#include <json-c/json.h>

#include "Utilities/Json.h"
#include "Utilities/Network.h"
#include "FetchingModule.h"

#include "../Globals.h"
#include "../Structures/BinaryTree.h"
#include "../Structures/SortedMap.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define IS_VALID_URL_CHARACTER(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
#define IS_VALID_STREAMS_CHARACTER(c) (IS_VALID_URL_CHARACTER(c) || strchr(LIST_ENTRY_SEPARATORS, c) != NULL)

static BinaryTree* oauth_storage = NULL;
static pthread_mutex_t oauth_global_mutex;

typedef struct {
	char* id;
	char* secret;
	char* token;
	int use_count;
	char* refresh_url;
	pthread_mutex_t mutex;
	int refresh_counter;
	bool last_refresh_result;
} OAuthEntry;

typedef struct {
	char** streams;
	int stream_count;
	SortedMap* stream_titles;
	OAuthEntry* oauth_entry;
	CURL* curl;
	struct curl_slist* list;

	char* title_template;
	char* body_template;
	char* icon_path;
} Config;

typedef struct {
	char* streamer_name;
	char* title;
	char* category;
	char* url;
} NotificationData;

static int oauth_entry_compare(const void* a, const void* b) {
	return strcmp(((const OAuthEntry*)a)->id, ((const OAuthEntry*)b)->id);
}

static void oauth_storage_init() {
	binaryTreeInit(oauth_storage, oauth_entry_compare);
	pthread_mutex_init(&oauth_global_mutex, NULL);
}

static void oauth_storage_destroy() {
	binaryTreeDestroy(oauth_storage);
	pthread_mutex_destroy(&oauth_global_mutex);
}

static bool oauth_entry_refresh_token(const FetchingModule* module, OAuthEntry* entry) {
	bool success = false;
	int refresh_counter_before_lock = entry->refresh_counter;

	pthread_mutex_lock(&entry->mutex);
	fm_log(module, 3, "Refresh counter before lock: %d, current: %d", refresh_counter_before_lock, entry->refresh_counter);
	if(refresh_counter_before_lock != entry->refresh_counter) {
		success = entry->last_refresh_result;
		goto oauth_entry_refresh_token_unlock_mutex;
	}

	CURL* curl = curl_easy_init();
	NetworkResponse response = {NULL, 0};

	fm_log(module, 1, "Refreshing token for ID %s...", entry->id);
	if(curl == NULL) {
		fm_log(module, 0, "Token refreshing failed - failed to initialize CURL object");
		goto oauth_entry_refresh_token_set_last_refresh_result;
	}

	curl_easy_setopt(curl, CURLOPT_URL, entry->refresh_url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
	curl_easy_setopt(curl, CURLOPT_POST, true);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "\n");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, network_callback);

	CURLcode code = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if(code != CURLE_OK) {
		fm_log(module, 0, "Token refreshing failed with CURL code %d", code);
		goto oauth_entry_refresh_token_set_last_refresh_result;
	}

	fm_log(module, 3, "Received refresh response:\n%s", response.data);
	json_object* root = json_tokener_parse(response.data);
	json_object* new_token_object = json_object_object_get(root, "access_token");
	if(json_object_get_type(new_token_object) != json_type_string) {
		fm_log(module, 0, "Token refreshing failed - invalid response");
		goto oauth_entry_refresh_token_put_root;
	}

	const char* new_token = json_object_get_string(new_token_object);
	fm_log(module, 2, "New token: %s", new_token);

	if(entry->token == NULL) {
		entry->token = malloc(strlen(new_token) + 1);
	}
	strcpy(entry->token, new_token);
	success = true;

oauth_entry_refresh_token_put_root:
	json_object_put(root);
oauth_entry_refresh_token_set_last_refresh_result:
	entry->last_refresh_result = success;
	entry->refresh_counter++;
oauth_entry_refresh_token_unlock_mutex:
	pthread_mutex_unlock(&entry->mutex);
	return success;
}

static OAuthEntry* oauth_storage_add_entry(OAuthEntry* entry) {
	pthread_mutex_lock(&oauth_global_mutex);

	if(oauth_storage == NULL) {
		oauth_storage = malloc(sizeof *oauth_storage);
		oauth_storage_init();
	}

	OAuthEntry* oauth_entry_from_map = binaryTreeGet(oauth_storage, entry);
	if(oauth_entry_from_map != NULL) {
		oauth_entry_from_map->use_count++;
		goto oauth_storage_add_entry_unlock_mutex;
	}

	oauth_entry_from_map = malloc(sizeof *oauth_entry_from_map);
	oauth_entry_from_map->id              = strdup(entry->id);
	oauth_entry_from_map->secret          = strdup(entry->secret);
	oauth_entry_from_map->token           = strdup(entry->token);
	oauth_entry_from_map->use_count       = 1;
	oauth_entry_from_map->refresh_url     = strdup(entry->refresh_url);
	oauth_entry_from_map->refresh_counter = 0;
	pthread_mutex_init(&oauth_entry_from_map->mutex, NULL);

	binaryTreePut(oauth_storage, oauth_entry_from_map);

oauth_storage_add_entry_unlock_mutex:
	pthread_mutex_unlock(&oauth_global_mutex);
	return oauth_entry_from_map;
}

static void oauth_storage_remove_entry(const OAuthEntry* entry) {
	OAuthEntry* oauth_entry_from_map = binaryTreeGet(oauth_storage, entry);

	/* this should never happen */
	if(oauth_entry_from_map == NULL) {
		return;
	}

	oauth_entry_from_map->use_count--;
	if(oauth_entry_from_map->use_count == 0) {
		binaryTreeRemove(oauth_storage, oauth_entry_from_map);
		pthread_mutex_destroy(&oauth_entry_from_map->mutex);
		free(oauth_entry_from_map->id);
		free(oauth_entry_from_map->secret);
		free(oauth_entry_from_map->token);
		free(oauth_entry_from_map->refresh_url);
		free(oauth_entry_from_map);
	}

	if(oauth_storage->root == NULL) {
		oauth_storage_destroy();
		free(oauth_storage);
		oauth_storage = NULL;
	}
}

static char* generate_url(char** streams, int start, int stop) {
	const char* base = "https://api.twitch.tv/helix/streams";
	char* output;
	int total_length;
	int output_ptr;

	total_length = strlen(base);
	for(int i = start; i <= stop; i++) {
		total_length += strlen("?user_login=") + strlen(streams[i]);
	}
	output = malloc(total_length + 1);
	strcpy(output, base);
	output_ptr = strlen(base);

	for(int i = start; i <= stop; i++) {
		output_ptr += sprintf(&output[output_ptr], "%cuser_login=%s", (i == start ? '?' : '&'), streams[i]);
	}

	return output;
}

static char* generate_token_refresh_url(const OAuthEntry* entry) {
	char* url = malloc(strlen("https://id.twitch.tv/oauth2/token?client_id=&client_secret=&grant_type=client_credentials") + strlen(entry->id) + strlen(entry->secret) + 1);
	sprintf(url, "https://id.twitch.tv/oauth2/token?client_id=%s&client_secret=%s&grant_type=client_credentials", entry->id, entry->secret);
	return url;
}

static void set_header(const FetchingModule* module) {
	Config* config = fm_get_data(module);

	if(config->list != NULL) {
		curl_slist_free_all(config->list);
		config->list = NULL;
	}

	char* header_client_id = malloc(strlen("Client-ID: ") + strlen(config->oauth_entry->id) + 1);
	char* header_client_token = malloc(strlen("Authorization: Bearer ") + strlen(config->oauth_entry->token) + 1);

	sprintf(header_client_id, "Client-ID: %s", config->oauth_entry->id);
	sprintf(header_client_token, "Authorization: Bearer %s", config->oauth_entry->token);

	config->list = curl_slist_append(config->list, header_client_id);
	config->list = curl_slist_append(config->list, header_client_token);

	curl_easy_setopt(config->curl, CURLOPT_HTTPHEADER, config->list);

	free(header_client_id);
	free(header_client_token);
}

static bool refresh_token(const FetchingModule* module, OAuthEntry* entry) {
	bool success = oauth_entry_refresh_token(module, entry);
	if(success) {
		char* token_key_name = malloc(strlen("token_") + strlen(entry->id) + 1);
		sprintf(token_key_name, "token_%s", entry->id);
		fm_set_stash_string(module, "twitch", token_key_name, entry->token);
		free(token_key_name);
	}
	return success;
}

static bool parse_config(FetchingModule* module) {
	Config* config = malloc(sizeof *config);
	memset(config, 0, sizeof *config);
	fm_set_data(module, config);

	OAuthEntry oauth = {0};
	char* streams;

	if(!fm_get_config_string_log(module, "id", &oauth.id, 0) ||
	   !fm_get_config_string_log(module, "secret", &oauth.secret, 0) ||
	   !fm_get_config_string_log(module, "streams", &streams, 0) ||
	   !fm_get_config_string_log(module, "title", &config->title_template, 0) ||
	   !fm_get_config_string_log(module, "body", &config->body_template, 0)) {
		return false;
	}

	fm_get_config_string(module, "icon", &config->icon_path);

	oauth.refresh_url = generate_token_refresh_url(&oauth);

	char* token_key_name = malloc(strlen("token_") + strlen(oauth.id) + 1);
	sprintf(token_key_name, "token_%s", oauth.id);
	const char* token = fm_get_stash_string(module, "twitch", token_key_name, NULL);
	if(token == NULL) {
		if(!refresh_token(module, &oauth)) {
			return false;
		}
	} else {
		oauth.token = strdup(token);
	}
	free(token_key_name);

	for(int i = 0; oauth.id[i] != '\0'; i++) {
		if(!IS_VALID_URL_CHARACTER(oauth.id[i])) {
			fm_log(module, 0, "ID contains illegal character \"%c\"", oauth.id[i]);
			return false;
		}
	}
	for(int i = 0; oauth.secret[i] != '\0'; i++) {
		if(!IS_VALID_URL_CHARACTER(oauth.secret[i])) {
			fm_log(module, 0, "Secret contains illegal character \"%c\"", oauth.secret[i]);
			return false;
		}
	}
	for(int i = 0; streams[i] != '\0'; i++) {
		if(!IS_VALID_STREAMS_CHARACTER(streams[i])) {
			fm_log(module, 0, "Streams contain illegal character \"%c\"", streams[i]);
			return false;
		}
	}

	config->stream_count  = fm_split(module, streams, LIST_ENTRY_SEPARATORS, &config->streams);
	config->stream_titles = malloc(sizeof *config->stream_titles);

	config->list = NULL;
	config->curl = curl_easy_init();

	if(config->curl == NULL) {
		fm_log(module, 0, "Error initializing CURL object");
		return false;
	}
	curl_easy_setopt(config->curl, CURLOPT_WRITEFUNCTION, network_callback);

	config->oauth_entry = oauth_storage_add_entry(&oauth);
	free(oauth.id);
	free(oauth.secret);
	free(oauth.token);
	free(oauth.refresh_url);

	sortedMapInit(config->stream_titles, (int (*)(const void*, const void*))strcmp);
	for(int i = 0; i < config->stream_count; i++) {
		sortedMapPut(config->stream_titles, config->streams[i], NULL);
	}

	return true;
}

void configure(FMConfig* config) {
	fm_config_set_name(config, "twitch");
}

bool enable(FetchingModule* module) {
	if(!parse_config(module)) {
		return false;
	}

	set_header(module);
	return true;
}

static char* replace_variables(const FetchingModule* module, const char* text, const NotificationData* notification_data) {
	return fm_replace(module, text, 4,
		"<streamer-name>", notification_data->streamer_name,
		"<title>", notification_data->title,
		"<category>", notification_data->category,
		"<url>", notification_data->url);
}

static void display_notification(const FetchingModule* module, const NotificationData* notification_data) {
	Config* config = fm_get_data(module);
	Message* message = fm_new_message();
	message->title = replace_variables(module, config->title_template, notification_data);
	message->body = replace_variables(module, config->body_template, notification_data);
	message->icon_path = config->icon_path;
	message->action_data = notification_data->url;
	message->action_type = URL;
	fm_display_message(module, message);
	free(message->title);
	free(message->body);
	fm_free_message(message);
}

static int parse_response(const FetchingModule* module, const char* response, char** checked_stream_names, NotificationData* new_data) {
	Config* config = fm_get_data(module);
	json_object* root = json_tokener_parse(response);
	json_object* data;

	/* TODO: memory leak? */
	if(!json_read_array(root, "data", &data)) {
		int error_status;
		const char* error_code;
		const char* error_msg;
		if(json_object_get_type(root) == json_type_object &&
		   json_read_int(root, "status", &error_status) &&
		   json_read_string(root, "error", &error_code) &&
		   json_read_string(root, "message", &error_msg)) {
			fm_log(module, 0, "Error %d - %s (%s)", error_status, error_code, error_msg);
			return error_status;
		} else {
			fm_log(module, 0, "Invalid response");
			return -1;
		}
	}

	int active_streamers_count = json_object_array_length(data);

	for(int i = 0; i < active_streamers_count; i++) {
		json_object* stream = json_object_array_get_idx(data, i);
		const char* active_streamer_name;
		const char* new_title;
		const char* new_category;

		if(!json_read_string(stream, "user_name", &active_streamer_name) ||
		   !json_read_string(stream, "title", &new_title) ||
		   !json_read_string(stream, "game_name", &new_category)) {
			fm_log(module, 0, "Invalid data for stream %d", i);
			continue;
		}

		for(int j = 0; j < config->stream_count; j++) {
			if(!strcasecmp(active_streamer_name, checked_stream_names[j])) {
				new_data[j].streamer_name = strdup(active_streamer_name);
				new_data[j].title         = strdup(new_title);
				new_data[j].category      = strdup(new_category);
				new_data[j].url           = malloc(strlen("https://twitch.tv/") + strlen(active_streamer_name) + 1);
				sprintf(new_data[j].url, "https://twitch.tv/%s", active_streamer_name);

				break;
			}
		}
	}
	json_object_put(root);
	return 0;
}

void fetch(const FetchingModule* module) {
	Config* config = fm_get_data(module);
	char** checked_stream_names = malloc(config->stream_count * sizeof *checked_stream_names);
	NotificationData* new_data = malloc(config->stream_count * sizeof *new_data);

	sortedMapKeys(config->stream_titles, (void**)checked_stream_names);
	new_data = memset(new_data, 0, config->stream_count * sizeof *new_data);

	for(int i = 0; i < config->stream_count; i += 100) {
		NetworkResponse response = {NULL, 0};
		char* token_used = strdup(config->oauth_entry->token);
		char* url = generate_url(config->streams, i, MIN(config->stream_count - 1, i + 99));
		curl_easy_setopt(config->curl, CURLOPT_URL, url);
		curl_easy_setopt(config->curl, CURLOPT_WRITEDATA, &response);
		free(url);

		CURLcode code = curl_easy_perform(config->curl);
		if(code != CURLE_OK) {
			fm_log(module, 0, "Request failed with code %d", code);
			goto fetch_free_response;
		}

		fm_log(module, 3, "Received response:\n%s", response.data);
		int error_code = parse_response(module, response.data, checked_stream_names, new_data);
		if(error_code == 401) {
			fm_log(module, 3, "Used token: %s, current: %s", token_used, config->oauth_entry->token);
			if(strcmp(token_used, config->oauth_entry->token) != 0 || refresh_token(module, config->oauth_entry)) {
				set_header(module);
				i -= 100; /* retry */
			} else {
				fm_log(module, 0, "Unable to refresh token, skipping fetch");
				i = config->stream_count; /* end the loop */
			}
		}

fetch_free_response:
		free(response.data);
		free(token_used);
	}

	for(int i = 0; i < config->stream_count; i++) {
		if(sortedMapGet(config->stream_titles, checked_stream_names[i]) == NULL && new_data[i].title != NULL) {
			NotificationData notification_data = {0};
			notification_data.streamer_name = new_data[i].streamer_name;
			notification_data.title         = new_data[i].title;
			notification_data.category      = new_data[i].category;
			notification_data.url           = new_data[i].url;
			display_notification(module, &notification_data);
		}
		char* last_stream_title;
		sortedMapRemove(config->stream_titles, checked_stream_names[i], NULL, (void**)&last_stream_title);
		free(last_stream_title);
		sortedMapPut(config->stream_titles, checked_stream_names[i], new_data[i].title);

		free(new_data[i].streamer_name);
		free(new_data[i].category);
		free(new_data[i].url);
	}

	free(checked_stream_names);
	free(new_data);
}

void disable(FetchingModule* module) {
	Config* config = fm_get_data(module);

	curl_slist_free_all(config->list);
	curl_easy_cleanup(config->curl);

	char* stream_title;
	for(int i = 0; i < config->stream_count; i++) {
		sortedMapRemove(config->stream_titles, config->streams[i], NULL, (void**)&stream_title);
		free(stream_title);
	}
	oauth_storage_remove_entry(config->oauth_entry);
	sortedMapDestroy(config->stream_titles);
	free(config->stream_titles);
	for(int i = 0; i < config->stream_count; i++) {
		free(config->streams[i]);
	}
	free(config->streams);
	free(config->title_template);
	free(config->body_template);
	free(config->icon_path);
	free(config);
}
