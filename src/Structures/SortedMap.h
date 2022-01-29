#ifndef SORTEDMAP_H
#define SORTEDMAP_H

#include <stdlib.h>
#include <string.h> // memcmp
#include <stdbool.h>

#define SORTED_MAP_DEFAULT_SIZE	32

typedef struct {
	void* key;
	void* value;
} SortedMapElement;

typedef struct {
	SortedMapElement* elements;
	int (*compareFunction)(const void*, const void*);
	int size;
	int availableSize;
} SortedMap;

bool sortedMapInit(SortedMap* map, int (*compareFunction)(const void*, const void*));
void sortedMapDestroy(SortedMap* map);

bool sortedMapPut(SortedMap* map, void* key, void* value);
void* sortedMapGet(SortedMap* map, const void* key);
bool sortedMapRemove(SortedMap* map, void* key, void** keyAddress, void** valueAddress);
bool sortedMapExists(SortedMap* map, const void* key);
int sortedMapSize(SortedMap* map);
void sortedMapKeys(SortedMap* map, void** keyArray);

int sortedMapCompareFunctionStrcmp(const void* a, const void* b);
int sortedMapCompareFunctionStrcasecmp(const void* a, const void* b);

#endif // ifndef SORTEDMAP_H
