#include "Json.h"

bool jsonReadInt(json_object* object, const char* variableName, int* output) {
	json_object* outputObject = json_object_object_get(object, variableName);
	if(json_object_get_type(outputObject) != json_type_int) {
		return false;
	}
	*output = json_object_get_int(outputObject);
	return true;
}

bool jsonReadString(json_object* object, const char* variableName, const char** output) {
	json_object* outputObject = json_object_object_get(object, variableName);
	if(json_object_get_type(outputObject) != json_type_string) {
		return false;
	}
	*output = json_object_get_string(outputObject);
	return true;
}

bool jsonReadObject(json_object* object, const char* variableName, json_object** output) {
	json_object* outputObject = json_object_object_get(object, variableName);
	if(json_object_get_type(outputObject) != json_type_object) {
		return false;
	}
	*output = outputObject;
	return true;
}

bool jsonReadArray(json_object* object, const char* variableName, json_object** output) {
	json_object* outputObject = json_object_object_get(object, variableName);
	if(json_object_get_type(outputObject) != json_type_array) {
		return false;
	}
	*output = outputObject;
	return true;
}
