#include "SortedMap.h"

#define MIN(x,y) ((x)<(y)?(x):(y))

bool sorted_map_init(SortedMap* map, int (*compare_function)(const void*, const void*)) {
	map->elements = malloc(SORTED_MAP_DEFAULT_SIZE * sizeof *map->elements);
	map->compare_function = compare_function;
	map->size = 0;
	map->available_size = map->elements != NULL ? SORTED_MAP_DEFAULT_SIZE : 0;
	return map->elements != 0;
}

void sorted_map_destroy(SortedMap* map) {
	free(map->elements);
	map->size = 0;
	map->available_size = 0;
}

static bool sorted_map_double_size(SortedMap* map) {
	SortedMapElement* elements_new = realloc(map->elements, map->available_size * 2 * sizeof *map->elements);
	if(elements_new != NULL) {
		map->elements = elements_new;
		return true;
	}
	return false;
}

bool sorted_map_put(SortedMap* map, void* key, void* value) {
	/* possible indices of inserted element */
	int insertion_index_min = 0;
	int insertion_index_max = map->size;

	int checked_index;
	SortedMapElement* checked_element;
	int cmp_result;

	while(insertion_index_min != insertion_index_max) {
		checked_index = (insertion_index_min + insertion_index_max) / 2;
		checked_element = &map->elements[checked_index];
		cmp_result = map->compare_function(key, checked_element->key);

		if(cmp_result == 0) {
			checked_element->value = value;
			return true;
		} else if(cmp_result > 0) {
			insertion_index_min = checked_index + 1;
		} else { /* cmp_result < 0 */
			insertion_index_max = checked_index;
		}
	}

	if(map->size >= map->available_size && !sorted_map_double_size(map)) {
		return false;
	}

	int insertion_index = insertion_index_min;
	memmove(&map->elements[insertion_index + 1], &map->elements[insertion_index], (map->size++ - insertion_index) * sizeof *checked_element);

	SortedMapElement* element = &map->elements[insertion_index];
	element->key = key;
	element->value = value;
	return true;
}

static int sorted_map_get_key_index(SortedMap* map, const void* key) {
	int min = 0;
	int max = map->size - 1;
	int checked_index;
	SortedMapElement* checked_element;
	int cmp_result;

	while(min <= max) {
		checked_index = (min + max) / 2;
		checked_element = &map->elements[checked_index];
		cmp_result = map->compare_function(key, checked_element->key);

		if(cmp_result == 0) {
			return checked_index;
		} else if(cmp_result > 0) {
			min = checked_index + 1;
		} else { /* cmp_result < 0 */
			max = checked_index - 1;
		}
	}
	
	return -1;
}

void* sorted_map_get(SortedMap* map, const void* key) {
	int index = sorted_map_get_key_index(map, key);
	return index >= 0 ? map->elements[index].value : NULL;
}

bool sorted_map_remove(SortedMap* map, void* key, void** key_address, void** value_address) {
	int index = sorted_map_get_key_index(map, key);
	if(index < 0) {
		return false;
	}

	SortedMapElement* element_to_remove = &map->elements[index];
	if(key_address != NULL) {
		*key_address = element_to_remove->key;
	}
	if(value_address != NULL) {
		*value_address = element_to_remove->value;
	}

	memmove(element_to_remove, element_to_remove + 1, (map->size-- - index - 1) * sizeof *element_to_remove);
	return true;
}

bool sorted_map_exists(SortedMap* map, const void* key) {
	int index = sorted_map_get_key_index(map, key);
	return index >= 0;
}

int sorted_map_size(SortedMap* map) {
	return map->size;
}

void sorted_map_keys(SortedMap* map, void** key_array) {
	int size = sorted_map_size(map);
	for(int i = 0; i < size; i++) {
		key_array[i] = map->elements[i].key;
	}
}
