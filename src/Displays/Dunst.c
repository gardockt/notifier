#include "Dunst.h"

int initStack = 0;
GMainLoop* mainLoop = NULL;

bool dunstInit() {
	if(initStack++) {
		return true;
	}

	notify_init("notifier");
	mainLoop = g_main_loop_new(NULL, false);

	return initStack > 0;
}

void dunstUninit() {
	if(--initStack == 0) {
		notify_uninit();
	}
}

void dunstOpenUrl(NotifyNotification* notification, char* action, gpointer url) {
	char* command = malloc(strlen("xdg-open ") + strlen(url) + strlen(" &") + 1);
	sprintf(command, "xdg-open %s &", url);
	system(command);
	free(command);
}

bool dunstDisplayMessage(Message* message) {
	NotifyNotification* notification = notify_notification_new(message->title, message->text, NULL);

	if(message->url != NULL) {
		notify_notification_add_action(notification, "open", "Open", NOTIFY_ACTION_CALLBACK(dunstOpenUrl), message->url, NULL);
	}

	notify_notification_show(notification, NULL);

	if(message->url != NULL) {
		g_main_loop_run(mainLoop);
	}

	g_object_unref(G_OBJECT(notification));
}

bool dunstStructure(Display* display) {
	display->init = dunstInit;
	display->displayMessage = dunstDisplayMessage;
	display->uninit = dunstUninit;

	return true;
}
