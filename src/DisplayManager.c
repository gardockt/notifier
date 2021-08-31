#include "StringOperations.h"
#include "DisplayManager.h"

// displays
#include "Displays/Libnotify.h"

#define ADDDISPLAY(structure,name) (structure(display) && putIntoMap(&displayManager->displays, name, strlen(name), display))

bool initDisplayManager(DisplayManager* displayManager) {
	Display* display = malloc(sizeof *display);

	if(!(display != NULL &&
	   initMap(&displayManager->displays))) {
		return false;
	}

	// adding displays to map; enter names lower-case
	if(!ADDDISPLAY(libnotifyStructure, "libnotify")) {
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
	char* displayNameLowerCase = toLowerCase(displayName);
	Display* display = getFromMap(&displayManager->displays, displayNameLowerCase, strlen(displayNameLowerCase));
	free(displayNameLowerCase);
	return display;
}
