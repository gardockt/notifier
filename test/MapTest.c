#include "src/Structures/Map.h"

#include <stdio.h>
#include <string.h>

#define BLAD() {printf("Test w linii %d nie przechodzi pomyślnie", __LINE__); return 1;}

// TODO: poprawić strukturę
// TODO: NIETESTOWANE doubleMapSize

int main() {
	Map map;

	if(!initMap(&map)) {
		BLAD();
	}

	if(getFromMap(&map, "key", 3) != NULL) {
		BLAD();
	}

	if(!putIntoMap(&map, "key", 3, "value")) {
		BLAD();
	}

	if(strcmp(getFromMap(&map, "key", 3), "value") != 0) {
		BLAD();
	}

	destroyMap(&map);
	printf("Testy zakończone pomyślnie\n");
	return 0;
}
