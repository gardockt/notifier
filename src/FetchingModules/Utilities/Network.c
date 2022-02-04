#include "Network.h"

size_t network_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	NetworkResponse* memory = (NetworkResponse*)userdata;
	int size_before = memory->size;
	int size_added = size * nmemb;

	memory->size = size_before + size_added;
	memory->data = realloc(memory->data, memory->size + 1);
	strncpy(&memory->data[size_before], ptr, size_added);
	memory->data[memory->size] = '\0';

	return size_added;
}
