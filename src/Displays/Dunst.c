#include "Dunst.h"

// TODO: rename to libnotify
// TODO: close notification on action

int initStack = 0;

void dunstOnNotificationClose(NotifyNotification* notification, gpointer mainLoop) {
	g_object_unref(G_OBJECT(notification));
	//pthread_exit(0);
}

bool dunstInit() {
	if(initStack++) {
		return true;
	}

	notify_init("notifier");
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

void* dunstDisplayMessageThread(void* messagePointer) {
	Message* message = messagePointer;

	NotifyNotification* notification = notify_notification_new(message->title, message->text, NULL);

	GMainLoop* mainLoop = g_main_loop_new(NULL, false);
	g_signal_connect(notification, "closed", G_CALLBACK(dunstOnNotificationClose), NULL);

	if(message->url != NULL) {
		notify_notification_add_action(notification, "open", "Open", NOTIFY_ACTION_CALLBACK(dunstOpenUrl), message->url, NULL);
	}

	notify_notification_show(notification, NULL);

	if(message->url != NULL) {
		g_main_loop_run(mainLoop);
	}
}

bool dunstDisplayMessage(Message* message) {
	pthread_t thread;
	bool ret = pthread_create(&thread, NULL, dunstDisplayMessageThread, message); // TODO: end thread gracefully
	return ret;
}

bool dunstStructure(Display* display) {
	display->init = dunstInit;
	display->displayMessage = dunstDisplayMessage;
	display->uninit = dunstUninit;

	return true;
}
