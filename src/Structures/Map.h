#ifndef MAP_H
#define MAP_H

#include <stdlib.h>
#include <string.h> // memcmp
#include <stdbool.h>

#define MAP_DEFAULT_SIZE	32

typedef struct {
	void* key;
	int keySize;
	void* value;
} MapElement;

typedef struct {
	MapElement* elements;
	int size;
	int availableSize;
} Map;

bool initMap(Map* map);
void destroyMap(Map* map);

bool doubleMapSize(Map* map);

bool putIntoMap(Map* map, void* key, int keySize, void* value);
void* getFromMap(Map* map, void* key, int keySize); 
bool removeFromMap(Map* map, void* key, int keySize);

#endif // ifndef MAP_H
