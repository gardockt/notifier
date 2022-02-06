#ifndef SORTED_MAP_H
#define SORTED_MAP_H

#include <stdlib.h>
#include <string.h> /* memcmp */
#include <stdbool.h>

#define SORTED_MAP_DEFAULT_SIZE	32

typedef struct {
	void* key;
	void* value;
} SortedMapElement;

typedef struct {
	SortedMapElement* elements;
	int (*compare_function)(const void*, const void*);
	int size;
	int available_size;
} SortedMap;

bool sorted_map_init(SortedMap* map, int (*compare_function)(const void*, const void*));
void sorted_map_destroy(SortedMap* map);

bool sorted_map_put(SortedMap* map, void* key, void* value);
void* sorted_map_get(SortedMap* map, const void* key);
bool sorted_map_remove(SortedMap* map, void* key, void** key_address, void** value_address);
bool sorted_map_exists(SortedMap* map, const void* key);
int sorted_map_size(SortedMap* map);
void sorted_map_keys(SortedMap* map, void** key_array);

#endif /* ifndef SORTED_MAP_H */
