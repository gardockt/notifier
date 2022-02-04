#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef struct {
	char* data;
	int size;
} NetworkResponse;

size_t network_callback(char* ptr, size_t size, size_t nmemb, void* userdata);

#endif /* ifndef NETWORK_H */
