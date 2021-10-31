#include "DisplayManager.h"

// displays
#include "Displays/Libnotify.h"

#define ADDDISPLAY(structure,name) (structure(display) && sortedMapPut(&displayManager->displays, name, display))

bool initDisplayManager(DisplayManager* displayManager) {
	Display* display = malloc(sizeof *display);

	if(!(display != NULL &&
	   sortedMapInit(&displayManager->displays, sortedMapCompareFunctionStrcasecmp))) {
		return false;
	}

	if(
#ifdef ENABLE_DISPLAY_LIBNOTIFY
	   !ADDDISPLAY(libnotifyStructure, "libnotify") ||
#endif
		false) {
		return false;
	}

	return true;
}

void destroyDisplayManager(DisplayManager* displayManager) {
	SortedMap displays = displayManager->displays;

	for(int i = 0; i < displays.size; i++) {
		free(displays.elements[i].value);
	}

	sortedMapDestroy(&displays);
}

Display* getDisplay(DisplayManager* displayManager, char* displayName) {
	return sortedMapGet(&displayManager->displays, displayName);
}
