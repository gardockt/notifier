#include "StringOperations.h"
#include "Structures/BinaryTree.h"

typedef struct {
	char* before;
	char* after;
	int length_diff;
	int next_occurrence;
} Replacement;

/* TODO: do we really need this? */
int split(const char* text, const char* separators, char*** output) {
	int text_length = strlen(text);
	int text_sections = 0;
	bool last_character_was_separator = true;
	bool is_separator;

	/* count text sections and alloc memory */
	for(int i = 0; i < text_length; i++) {
		is_separator = (strchr(separators, text[i]) != NULL);
		if(last_character_was_separator && !is_separator) {
			last_character_was_separator = false;
			text_sections++;
		} else if(!last_character_was_separator && is_separator) {
			last_character_was_separator = true;
		}
	}
	*output = malloc(text_sections * sizeof **output);

	/* export each section */
	int text_start = 0;

	for(int i = 0; i < text_sections; i++) {
		for(int j = text_start; j <= text_length; j++) {
			if(j < text_length) {
				is_separator = (strchr(separators, text[j]) != NULL);
				if(is_separator) {
					(*output)[i] = malloc(j - text_start + 1);
					strncpy((*output)[i], &text[text_start], j - text_start);
					(*output)[i][j - text_start] = '\0';
					text_start = j + 1;
					break;
				}
			} else {
				(*output)[i] = strdup(&text[text_start]);
			}
		}
		while(text_start < text_length && strchr(separators, text[text_start]) != NULL) {
			text_start++;
		}
	}

	return text_sections;
}

static int find(const char* text, const char* substring) {
	int text_length = strlen(text);
	int substring_length = strlen(substring);

	for(int i = 0; i <= text_length - substring_length; i++) {
		if(!strncmp(&text[i], substring, substring_length)) {
			return i;
		}
	}
	return -1;
}

static int compare_replacements(const void* a_ptr, const void* b_ptr) {
	const Replacement* a = a_ptr;
	const Replacement* b = b_ptr;
	return a->next_occurrence - b->next_occurrence;
}

char* replace(const char* input, int pair_count, ...) {
	if(input == NULL) {
		return NULL;
	}

	va_list args;
	BinaryTree replacements;
	binaryTreeInit(&replacements, compare_replacements);

	va_start(args, pair_count);
	while(pair_count--) {
		char* before = va_arg(args, char*);
		char* after = va_arg(args, char*);
		int next_occurrence = find(input, before);
		if(next_occurrence != -1) {
			Replacement* replacement = malloc(sizeof *replacement);
			replacement->before = before;
			replacement->after = after;
			replacement->length_diff = strlen(after) - strlen(before);
			replacement->next_occurrence = next_occurrence;
			binaryTreePut(&replacements, replacement);
		}
	}
	va_end(args);

	int input_length = strlen(input);
	int output_length = input_length;
	int input_pointer = 0;
	int pointer_diff = 0;
	char* output = malloc(output_length);
	bool realloc_after_all_replacements = false;
	Replacement* next_replacement;

	while((next_replacement = binaryTreePopLowest(&replacements)) != NULL) {
		output_length += next_replacement->length_diff;
		if(next_replacement->length_diff > 0) {
			output = realloc(output, output_length + 1);
		} else if(next_replacement->length_diff < 0) {
			realloc_after_all_replacements = true;
		}

		if(next_replacement->next_occurrence >= input_pointer) { /* ignore replacement if it contains part of "before" text of previous replacement */
			strncpy(&output[input_pointer + pointer_diff], &input[input_pointer], next_replacement->next_occurrence - input_pointer);
			strcpy(&output[next_replacement->next_occurrence + pointer_diff], next_replacement->after);
			input_pointer = next_replacement->next_occurrence + strlen(next_replacement->before);
			pointer_diff += next_replacement->length_diff;
		}

		next_replacement->next_occurrence = find(&input[input_pointer], next_replacement->before);
		if(next_replacement->next_occurrence != -1) {
			next_replacement->next_occurrence += input_pointer;
			binaryTreePut(&replacements, next_replacement);
		} else {
			free(next_replacement);
		}
	}

	strcpy(&output[input_pointer + pointer_diff], &input[input_pointer]);
	binaryTreeDestroy(&replacements);
	if(realloc_after_all_replacements) {
		output = realloc(output, output_length + 1);
	}
	return output;
}
