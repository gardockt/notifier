#include "Libnotify.h"

// TODO: close notification on action

int initStack = 0;

void libnotifyOnNotificationClose(NotifyNotification* notification, gpointer mainLoop) {
	g_object_unref(G_OBJECT(notification));
	//pthread_exit(0);
}

bool libnotifyInit() {
	if(initStack++) {
		return true;
	}

	notify_init("notifier");
	return initStack > 0;
}

void libnotifyUninit() {
	if(--initStack == 0) {
		notify_uninit();
	}
}

void libnotifyStartAction(NotifyNotification* notification, char* action, gpointer messagePointer) {
	Message* message = messagePointer;
	switch(message->actionType) {
		case URL:
			char* command = malloc(strlen("xdg-open ") + strlen(message->actionData) + strlen(" &") + 1);
			sprintf(command, "xdg-open %s &", message->actionData);
			system(command);
			free(command);
	}
}

void* libnotifyDisplayMessageThread(void* messagePointer) {
	Message* message = messagePointer;

	NotifyNotification* notification = notify_notification_new(message->title, message->text, NULL);

	GMainLoop* mainLoop = g_main_loop_new(NULL, false);
	g_signal_connect(notification, "closed", G_CALLBACK(libnotifyOnNotificationClose), NULL);

	if(message->actionData != NULL) {
		notify_notification_add_action(notification, "open", "Open", NOTIFY_ACTION_CALLBACK(libnotifyStartAction), message, NULL);
	}

	notify_notification_show(notification, NULL);

	if(message->actionData != NULL) {
		g_main_loop_run(mainLoop);
	}
}

bool libnotifyDisplayMessage(Message* message) {
	pthread_t thread;
	bool ret = pthread_create(&thread, NULL, libnotifyDisplayMessageThread, message); // TODO: end thread gracefully
	return ret;
}

bool libnotifyStructure(Display* display) {
	display->init = libnotifyInit;
	display->displayMessage = libnotifyDisplayMessage;
	display->uninit = libnotifyUninit;

	return true;
}
