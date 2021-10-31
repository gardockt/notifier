#ifndef MAP_H
#define MAP_H

#include <stdlib.h>
#include <string.h> // memcmp
#include <stdbool.h>

#define MAP_DEFAULT_SIZE	32

typedef struct {
	void* key;
	void* value;
} MapElement;

typedef struct {
	MapElement* elements;
	int (*compareFunction)(const void*, const void*);
	int size;
	int availableSize;
} Map;

bool initMap(Map* map, int (*compareFunction)(const void*, const void*));
void destroyMap(Map* map);

bool putIntoMap(Map* map, void* key, void* value);
void* getFromMap(Map* map, void* key);
bool removeFromMap(Map* map, void* key, void** keyAddress, void** valueAddress);
bool existsInMap(Map* map, const void* key);
int getMapSize(Map* map);
void getMapKeys(Map* map, void** keyArray);

int mapCompareFunctionStrcmp(const void* a, const void* b);

#endif // ifndef MAP_H
