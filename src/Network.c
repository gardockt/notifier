#include "Network.h"

size_t networkCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	NetworkResponse* memory = (NetworkResponse*)userdata;
	int sizeBefore = memory->size;
	int sizeAdded = size * nmemb;

	memory->size = sizeBefore + sizeAdded;
	memory->data = realloc(memory->data, memory->size + 1);
	strncpy(&memory->data[sizeBefore], ptr, sizeAdded);
	memory->data[memory->size] = '\0';

	return sizeAdded;
}
