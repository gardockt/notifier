#include "Log.h"

void log_write_vararg(const char* section_name, int desired_verbosity, int verbosity, const char* format, va_list args) {
	if(desired_verbosity < verbosity) {
		return;
	}

	char* custom_format = malloc(strlen(section_name) + strlen("[] \n") + strlen(format) + 1);
	sprintf(custom_format, "[%s] %s\n", section_name, format);
	vfprintf(stderr, custom_format, args);
	free(custom_format);
}

/* TODO: get desired verbosity from globals? */
void log_write(const char* section_name, int desired_verbosity, int verbosity, const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_write_vararg(section_name, desired_verbosity, verbosity, format, args);
	va_end(args);
}
