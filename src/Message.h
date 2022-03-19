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

#endif /* ifndef MESSAGE_H */
