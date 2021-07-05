#include "DisplayManager.h"

// displays
#include "Displays/Dunst.h"

bool initDisplayManager(DisplayManager* displayManager) {
	Display* display = malloc(sizeof *display);

	if(!(display != NULL &&
	   initMap(&displayManager->displays))) {
		return false;
	}

	// adding displays to map
	// TODO: make names case-insensitive?
	if(!(dunstStructure(display) && putIntoMap(&displayManager->displays, "dunst", strlen("dunst"), display))) {
		return false;
	}

	return true;
}

void destroyDisplayManager(DisplayManager* displayManager) {
	Map displays = displayManager->displays;

	for(int i = 0; i < displays.size; i++) {
		free(displays.elements[i].value);
	}

	destroyMap(&displays);
}

Display* getDisplay(DisplayManager* displayManager, char* displayName) {
	return getFromMap(&displayManager->displays, displayName, strlen(displayName));
}
