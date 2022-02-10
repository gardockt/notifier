#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdlib.h>
#include <string.h>

typedef enum {
	NONE,
	URL
} NotificationActionType;

typedef struct {
	char* title;
	char* body;
	char* action_data;
	NotificationActionType action_type;
	char* icon_path;
} Message;

Message* message_clone(const Message* message);
void message_free_all_children(Message* message);

#endif /* ifndef MESSAGE_H */
