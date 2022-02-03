#include "Log.h"

void logWriteVararg(const char* sectionName, int desiredVerbosity, int verbosity, const char* format, va_list args) {
	if(desiredVerbosity < verbosity) {
		return;
	}

	char* customFormat = malloc(strlen(sectionName) + strlen("[] \n") + strlen(format) + 1);
	sprintf(customFormat, "[%s] %s\n", sectionName, format);
	vfprintf(stderr, customFormat, args);
	free(customFormat);
}

void logWrite(const char* sectionName, int desiredVerbosity, int verbosity, const char* format, ...) {
	va_list args;
	va_start(args, format);
	logWriteVararg(sectionName, desiredVerbosity, verbosity, format, args);
	va_end(args);
}
