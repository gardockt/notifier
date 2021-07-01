#include "Map.h"

// TODO: zamienić na memcpy

#define MIN(x,y) ((x)<(y)?(x):(y))

int initMap(Map* map) {
	map->elements = malloc(MAP_DEFAULT_SIZE * sizeof *map->elements);
	map->size = 0;
	map->availableSize = map->elements != NULL ? MAP_DEFAULT_SIZE : 0;
	return map->elements != 0;
}

void destroyMap(Map* map) {
	free(map->elements);
	map->size = 0;
	map->availableSize = 0;
}

int doubleMapSize(Map* map) {
	MapElement* elementsNew = reallocarray(map->elements, map->availableSize * 2, sizeof *map->elements);
	if(elementsNew != NULL) {
		map->elements = elementsNew;
		return 1;
	}
	return 0;
}

int putIntoMap(Map* map, void* key, int keySize, void* value) {
	if(map->size >= map->availableSize && !doubleMapSize(map)) {
		return 0;
	}

	// TODO: zakładamy kolejność alfabetyczną dodawania - NAPRAWIĆ! (można założyć optymalizację dla takiej kolejności)
	// TODO: memcpy?
	MapElement* element = &map->elements[map->size++];
	element->key = key;
	element->keySize = keySize;
	element->value = value;
	return 1;
}

void* getFromMap(Map* map, void* key, int keySize) {
	int min = 0;
	int max = map->size - 1;
	int checkedIndex;
	MapElement* checkedElement;
	int cmpSize;
	int cmpResult;

	while(min <= max) {
		checkedIndex = (min + max) / 2;
		checkedElement = &map->elements[checkedIndex];
		cmpSize = MIN(keySize, checkedElement->keySize);
		if(!(cmpResult = memcmp(key, checkedElement->key, cmpSize))) {
			cmpResult = keySize - checkedElement->keySize;
		}

		if(cmpResult == 0) {
			return checkedElement->value;
		} else if(cmpResult > 0) {
			min = checkedIndex + 1;
		} else { // cmpResult < 0
			max = checkedIndex - 1;
		}
	}
	
	return NULL;
}

bool removeFromMap(Map* map, void* key, int keySize) {
	// TODO: zrobic
}
