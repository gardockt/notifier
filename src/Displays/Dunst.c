#include "Dunst.h"

int initStack = 0;

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

bool dunstDisplayMessage(Message* message) {
	NotifyNotification* notification = notify_notification_new(message->title, message->text, NULL);
	notify_notification_show(notification, NULL);
	g_object_unref(G_OBJECT(notification));
}

bool dunstStructure(Display* display) {
	display->init = dunstInit;
	display->displayMessage = dunstDisplayMessage;
	display->uninit = dunstUninit;

	return true;
}
