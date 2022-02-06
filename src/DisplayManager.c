#include "DisplayManager.h"

// displays
#include "Displays/Libnotify.h"

#define ADD_DISPLAY(structure,name) (structure(display) && sortedMapPut(&manager->displays, name, display))

bool display_manager_init(DisplayManager* manager) {
	Display* display = malloc(sizeof *display);

	if(!(display != NULL &&
	   sortedMapInit(&manager->displays, (int (*)(const void*, const void*))strcasecmp))) {
		return false;
	}

	if(
#ifdef ENABLE_DISPLAY_LIBNOTIFY
	   !ADD_DISPLAY(libnotify_structure, "libnotify") ||
#endif
		false) {
		return false;
	}

	return true;
}

void display_manager_destroy(DisplayManager* manager) {
	SortedMap displays = manager->displays;

	for(int i = 0; i < displays.size; i++) {
		free(displays.elements[i].value);
	}

	sortedMapDestroy(&displays);
}

Display* display_manager_get_display(DisplayManager* manager, const char* name) {
	return sortedMapGet(&manager->displays, name);
}
