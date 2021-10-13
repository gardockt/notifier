#ifdef ENABLE_DISPLAY_LIBNOTIFY

#include "Libnotify.h"

int initStack = 0;

void libnotifyOnNotificationClose(NotifyNotification* notification, gpointer messagePointer) {
	messageFreeAllChildren(messagePointer);
	free(messagePointer);
	g_object_unref(G_OBJECT(notification));
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
	char* command;
	switch(message->actionType) {
		case URL:
			command = malloc(strlen("xdg-open ") + strlen(message->actionData) + strlen(" &") + 1);
			sprintf(command, "xdg-open %s &", message->actionData);
			system(command);
			free(command);
	}

	notify_notification_close(notification, NULL);
}

void* libnotifyDisplayMessageThread(void* messagePointer) {
	Message* message = messagePointer;

	NotifyNotification* notification = notify_notification_new(message->title, message->body, NULL);

	GMainLoop* mainLoop = g_main_loop_new(NULL, false);
	g_signal_connect(notification, "closed", G_CALLBACK(libnotifyOnNotificationClose), message);

	if(message->iconPath != NULL) {
		GError *error = NULL;
		GdkPixbuf *icon = gdk_pixbuf_new_from_file(message->iconPath, &error);
		if(error == NULL) {
			notify_notification_set_image_from_pixbuf(notification, icon);
		}
	}

	if(message->actionData != NULL) {
		notify_notification_add_action(notification, "open", "Open", NOTIFY_ACTION_CALLBACK(libnotifyStartAction), message, NULL);
	}

	notify_notification_show(notification, NULL);

	if(message->actionData != NULL) {
		g_main_loop_run(mainLoop);
	}
}

bool libnotifyDisplayMessage(Message* message) {
	Message* clonedMessage = messageClone(message);

	pthread_t thread;
	bool ret = pthread_create(&thread, NULL, libnotifyDisplayMessageThread, clonedMessage);
	if(ret) {
		pthread_detach(thread);
	}
	return ret;
}

bool libnotifyStructure(Display* display) {
	display->init = libnotifyInit;
	display->displayMessage = libnotifyDisplayMessage;
	display->uninit = libnotifyUninit;

	return true;
}

#endif // ifdef ENABLE_DISPLAY_LIBNOTIFY
