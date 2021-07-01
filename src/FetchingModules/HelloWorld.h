#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "FetchingModule.h"

typedef struct {
	char* text;
} HelloWorldConfig;

bool helloWorldTemplate(FetchingModule*);

#endif
