#ifndef JSON_H
#define JSON_H

#include <stdbool.h>

#include <json-c/json.h>

bool json_read_int(json_object* object, const char* var_name, int* output);
bool json_read_string(json_object* object, const char* var_name, const char** output);
bool json_read_object(json_object* object, const char* var_name, json_object** output);
bool json_read_array(json_object* object, const char* var_name, json_object** output);

#endif /* ifndef JSON_H */
