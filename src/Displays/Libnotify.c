#ifdef ENABLE_DISPLAY_LIBNOTIFY

#include "Libnotify.h"

int init_stack = 0;

static void libnotify_on_notification_close(NotifyNotification* notification, gpointer msg_ptr) {
	message_free_all_children(msg_ptr);
	free(msg_ptr);
	g_object_unref(G_OBJECT(notification));
}

static bool libnotify_init() {
	if(init_stack++) {
		return true;
	}

	notify_init("notifier");
	return init_stack > 0;
}

static void libnotify_uninit() {
	if(--init_stack == 0) {
		notify_uninit();
	}
}

static void libnotify_start_action(NotifyNotification* notification, const char* action, const gpointer msg_ptr) {
	const Message* message = msg_ptr;
	char* command;
	switch(message->action_type) {
		case URL:
			command = malloc(strlen("xdg-open ") + strlen(message->action_data) + strlen(" &") + 1);
			sprintf(command, "xdg-open %s &", message->action_data);
			system(command);
			free(command);
	}

	notify_notification_close(notification, NULL);
}

static void* libnotify_display_message_thread(void* msg_ptr) {
	Message* message = msg_ptr;

	NotifyNotification* notification = notify_notification_new(message->title, message->body, NULL);

	GMainLoop* main_loop = g_main_loop_new(NULL, false);
	g_signal_connect(notification, "closed", G_CALLBACK(libnotify_on_notification_close), message);

	if(message->icon_path != NULL) {
		GError *error = NULL;
		GdkPixbuf *icon = gdk_pixbuf_new_from_file(message->icon_path, &error);
		if(error == NULL) {
			notify_notification_set_image_from_pixbuf(notification, icon);
		}
	}

	if(message->action_data != NULL) {
		notify_notification_add_action(notification, "open", "Open", NOTIFY_ACTION_CALLBACK(libnotify_start_action), message, NULL);
	}

	notify_notification_show(notification, NULL);

	if(message->action_data != NULL) {
		g_main_loop_run(main_loop);
	}
}

static bool libnotify_display_message(const Message* message) {
	Message* cloned_message = message_clone(message);

	pthread_t thread;
	bool ret = pthread_create(&thread, NULL, libnotify_display_message_thread, cloned_message);
	if(ret) {
		pthread_detach(thread);
	}
	return ret;
}

bool libnotify_structure(Display* display) {
	display->init = libnotify_init;
	display->display_message = libnotify_display_message;
	display->uninit = libnotify_uninit;

	return true;
}

#endif // ifdef ENABLE_DISPLAY_LIBNOTIFY
