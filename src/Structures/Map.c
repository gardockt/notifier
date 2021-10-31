#include "Map.h"

#define MIN(x,y) ((x)<(y)?(x):(y))

bool initMap(Map* map, int (*compareFunction)(const void*, const void*)) {
	map->elements = malloc(MAP_DEFAULT_SIZE * sizeof *map->elements);
	map->compareFunction = compareFunction;
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
	MapElement* elementsNew = realloc(map->elements, map->availableSize * 2 * sizeof *map->elements);
	if(elementsNew != NULL) {
		map->elements = elementsNew;
		return true;
	}
	return false;
}

bool putIntoMap(Map* map, void* key, void* value) {
	// possible indices of inserted element
	int insertionIndexMin = 0;
	int insertionIndexMax = map->size;

	int checkedIndex;
	MapElement* checkedElement;
	int cmpResult;

	while(insertionIndexMin != insertionIndexMax) {
		checkedIndex = (insertionIndexMin + insertionIndexMax) / 2;
		checkedElement = &map->elements[checkedIndex];
		cmpResult = map->compareFunction(key, checkedElement->key);

		if(cmpResult == 0) {
			checkedElement->value = value;
			return true;
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
	memmove(&map->elements[insertionIndex + 1], &map->elements[insertionIndex], (map->size++ - insertionIndex) * sizeof *checkedElement);

	MapElement* element = &map->elements[insertionIndex];
	element->key = key;
	element->value = value;
	return true;
}

int getKeyIndex(Map* map, const void* key) {
	int min = 0;
	int max = map->size - 1;
	int checkedIndex;
	MapElement* checkedElement;
	int cmpResult;

	while(min <= max) {
		checkedIndex = (min + max) / 2;
		checkedElement = &map->elements[checkedIndex];
		cmpResult = map->compareFunction(key, checkedElement->key);

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

void* getFromMap(Map* map, void* key) {
	int index = getKeyIndex(map, key);
	return index >= 0 ? map->elements[index].value : NULL;
}

bool removeFromMap(Map* map, void* key, void** keyAddress, void** valueAddress) {
	int index = getKeyIndex(map, key);
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

bool existsInMap(Map* map, const void* key) {
	int index = getKeyIndex(map, key);
	return index >= 0;
}

int getMapSize(Map* map) {
	return map->size;
}

void getMapKeys(Map* map, void** keyArray) {
	int size = getMapSize(map);
	for(int i = 0; i < size; i++) {
		keyArray[i] = map->elements[i].key;
	}
}

int mapCompareFunctionStrcmp(const void* a, const void* b) {
	return strcmp((const char*)a, (const char*)b);
}
