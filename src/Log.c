#include "Log.h"

void logWrite(char* sectionName, int desiredVerbosity, int verbosity, char* format, ...) {
	if(desiredVerbosity < verbosity) {
		return;
	}

	char* customFormat = malloc(strlen(sectionName) + strlen("[] \n") + strlen(format) + 1);
	sprintf(customFormat, "[%s] %s\n", sectionName, format);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, customFormat, args);
	va_end(args);

	free(customFormat);
}

