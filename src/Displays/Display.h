#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../Message.h"

typedef struct {
	bool (*enable)();
	bool (*display)(const Message* message);
	void (*disable)();
	void* handle;
} Display;

typedef struct {
	char* name;
} DisplayLibConfig;

#define display_config_set_name(config, display_name) ((config)->name = strdup(display_name))

#endif /* ifndef DISPLAY_H */
