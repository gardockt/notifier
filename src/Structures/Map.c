#include "Map.h"

#define MIN(x,y) ((x)<(y)?(x):(y))

bool initMap(Map* map) {
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

bool doubleMapSize(Map* map) {
	MapElement* elementsNew = reallocarray(map->elements, map->availableSize * 2, sizeof *map->elements);
	if(elementsNew != NULL) {
		map->elements = elementsNew;
		return true;
	}
	return false;
}

bool putIntoMap(Map* map, void* key, int keySize, void* value) {
	// possible indices of inserted element
	int insertionIndexMin = 0;
	int insertionIndexMax = map->size;

	int checkedIndex;
	MapElement* checkedElement;
	int cmpSize;
	int cmpResult;

	while(insertionIndexMin != insertionIndexMax) {
		checkedIndex = (insertionIndexMin + insertionIndexMax) / 2;
		checkedElement = &map->elements[checkedIndex];
		cmpSize = MIN(keySize, checkedElement->keySize);
		if(!(cmpResult = memcmp(key, checkedElement->key, cmpSize))) {
			cmpResult = keySize - checkedElement->keySize;
		}

		if(cmpResult == 0) {
			return false;
		} else if(cmpResult > 0) {
			insertionIndexMin = checkedIndex + 1;
		} else { // cmpResult < 0
			insertionIndexMax = checkedIndex;
		}
	}

	if(map->size >= map->availableSize && !doubleMapSize(map)) {
		return false;
	}

	int insertionIndex = insertionIndexMin;
	memmove(checkedElement + 1, checkedElement, (map->size++ - insertionIndex) * sizeof *checkedElement);

	MapElement* element = &map->elements[insertionIndex];
	element->key = key;
	element->keySize = keySize;
	element->value = value;
	return true;
}

int getKeyIndex(Map* map, void* key, int keySize) {
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
			return checkedIndex;
		} else if(cmpResult > 0) {
			min = checkedIndex + 1;
		} else { // cmpResult < 0
			max = checkedIndex - 1;
		}
	}
	
	return -1;
}

void* getFromMap(Map* map, void* key, int keySize) {
	int index = getKeyIndex(map, key, keySize);
	return index >= 0 ? map->elements[index].value : NULL;
}

bool removeFromMap(Map* map, void* key, int keySize, void** keyAddress, void** valueAddress) {
	int index = getKeyIndex(map, key, keySize);
	if(index < 0) {
		return false;
	}

	MapElement* elementToRemove = &map->elements[index];
	if(keyAddress != NULL) {
		*keyAddress = elementToRemove->key;
	}
	if(valueAddress != NULL) {
		*valueAddress = elementToRemove->value;
	}

	memmove(elementToRemove, elementToRemove + 1, (map->size-- - index - 1) * sizeof *elementToRemove);
	return true;
}

bool existsInMap(Map* map, void* key, int keySize) {
	int index = getKeyIndex(map, key, keySize);
	return index >= 0;
}
