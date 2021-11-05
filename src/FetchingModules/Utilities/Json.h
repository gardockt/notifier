#ifndef JSON_H
#define JSON_H

#include <stdbool.h>

#include <json-c/json.h>

bool jsonReadInt(json_object* object, const char* variableName, int* output);
bool jsonReadString(json_object* object, const char* variableName, const char** output);
bool jsonReadObject(json_object* object, const char* variableName, json_object** output);
bool jsonReadArray(json_object* object, const char* variableName, json_object** output);

#endif // ifndef JSON_H
