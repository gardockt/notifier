#include "Message.h"

#define STRDUP_IF_NOT_NULL(str) ((str) != NULL ? strdup(str) : NULL)

/* may be changed later */
Message* message_new() {
	return malloc(sizeof (Message));
}

void message_free(Message* message) {
	free(message);
}

Message* message_clone(const Message* message) {
	Message* ret = malloc(sizeof *ret);
	ret->title       = STRDUP_IF_NOT_NULL(message->title);
	ret->body        = STRDUP_IF_NOT_NULL(message->body);
	ret->action_data = STRDUP_IF_NOT_NULL(message->action_data);
	ret->icon_path   = STRDUP_IF_NOT_NULL(message->icon_path);
	ret->action_type = message->action_type;
	return ret;
}

void message_free_all_children(Message* message) {
	free(message->title);
	free(message->body);
	free(message->action_data);
	free(message->icon_path);
}
