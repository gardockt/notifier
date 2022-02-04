#include "Json.h"

bool json_read_int(json_object* object, const char* var_name, int* output) {
	json_object* output_object = json_object_object_get(object, var_name);
	if(json_object_get_type(output_object) != json_type_int) {
		return false;
	}
	*output = json_object_get_int(output_object);
	return true;
}

bool json_read_string(json_object* object, const char* var_name, const char** output) {
	json_object* output_object = json_object_object_get(object, var_name);
	if(json_object_get_type(output_object) != json_type_string) {
		return false;
	}
	*output = json_object_get_string(output_object);
	return true;
}

bool json_read_object(json_object* object, const char* var_name, json_object** output) {
	json_object* output_object = json_object_object_get(object, var_name);
	if(json_object_get_type(output_object) != json_type_object) {
		return false;
	}
	*output = output_object;
	return true;
}

bool json_read_array(json_object* object, const char* var_name, json_object** output) {
	json_object* output_object = json_object_object_get(object, var_name);
	if(json_object_get_type(output_object) != json_type_array) {
		return false;
	}
	*output = output_object;
	return true;
}
