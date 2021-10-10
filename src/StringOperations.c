#include "StringOperations.h"
#include "Structures/BinaryTree.h"

typedef struct {
	char* before;
	char* after;
	int lengthDiff;
	int nextOccurrence;
} Replacement;

int split(char* text, char* separators, char*** output) {
	int textLength = strlen(text);
	int textSections = 0;
	bool lastCharacterWasSeparator = true;
	bool isSeparator;

	// count text sections and alloc memory
	for(int i = 0; i < textLength; i++) {
		isSeparator = (strchr(separators, text[i]) != NULL);
		if(lastCharacterWasSeparator && !isSeparator) {
			lastCharacterWasSeparator = false;
			textSections++;
		} else if(!lastCharacterWasSeparator && isSeparator) {
			lastCharacterWasSeparator = true;
		}
	}
	*output = malloc(textSections * sizeof **output);

	// export each section
	int textStart = 0;

	for(int i = 0; i < textSections; i++) {
		for(int j = textStart; j <= textLength; j++) {
			if(j < textLength) {
				isSeparator = (strchr(separators, text[j]) != NULL);
				if(isSeparator) {
					(*output)[i] = malloc(j - textStart + 1);
					strncpy((*output)[i], &text[textStart], j - textStart);
					(*output)[i][j - textStart] = '\0';
					textStart = j + 1;
					break;
				}
			} else {
				(*output)[i] = strdup(&text[textStart]);
			}
		}
		while(textStart < textLength && strchr(separators, text[textStart]) != NULL) {
			textStart++;
		}
	}

	return textSections;
}

int find(char* text, const char* substring) {
	int textLength = strlen(text);
	int substringLength = strlen(substring);

	for(int i = 0; i <= textLength - substringLength; i++) {
		if(!strncmp(&text[i], substring, substringLength)) {
			return i;
		}
	}
	return -1;
}

int compareReplacements(void* aPtr, void* bPtr) {
	Replacement* a = aPtr;
	Replacement* b = bPtr;
	return a->nextOccurrence - b->nextOccurrence;
}

char* replace(char* input, int pairCount, ...) {
	if(input == NULL) {
		return NULL;
	}

	va_list args;
	BinaryTree replacements;
	binaryTreeInit(&replacements, compareReplacements);

	va_start(args, pairCount);
	while(pairCount--) {
		char* before = va_arg(args, char*);
		char* after = va_arg(args, char*);
		int nextOccurrence = find(input, before);
		if(nextOccurrence != -1) {
			Replacement* replacement = malloc(sizeof *replacement);
			replacement->before = before;
			replacement->after = after;
			replacement->lengthDiff = strlen(after) - strlen(before);
			replacement->nextOccurrence = nextOccurrence;
			binaryTreePut(&replacements, replacement);
		}
	}
	va_end(args);

	int inputLength = strlen(input);
	int outputLength = inputLength;
	int inputPointer = 0;
	int pointerDiff = 0;
	char* output = malloc(outputLength);
	bool reallocAfterAllReplacements = false;
	Replacement* nextReplacement;

	while((nextReplacement = binaryTreePopLowest(&replacements)) != NULL) {
		outputLength += nextReplacement->lengthDiff;
		if(nextReplacement->lengthDiff > 0) {
			output = realloc(output, outputLength + 1);
		} else if(nextReplacement->lengthDiff < 0) {
			reallocAfterAllReplacements = true;
		}

		if(nextReplacement->nextOccurrence >= inputPointer) { // ignore replacement if it contains part of "before" text of previous replacement
			strncpy(&output[inputPointer + pointerDiff], &input[inputPointer], nextReplacement->nextOccurrence - inputPointer);
			strcpy(&output[nextReplacement->nextOccurrence + pointerDiff], nextReplacement->after);
			inputPointer = nextReplacement->nextOccurrence + strlen(nextReplacement->before);
			pointerDiff += nextReplacement->lengthDiff;
		}

		nextReplacement->nextOccurrence = find(&input[inputPointer], nextReplacement->before);
		if(nextReplacement->nextOccurrence != -1) {
			nextReplacement->nextOccurrence += inputPointer;
			binaryTreePut(&replacements, nextReplacement);
		} else {
			free(nextReplacement);
		}
	}

	strcpy(&output[inputPointer + pointerDiff], &input[inputPointer]);
	binaryTreeDestroy(&replacements);
	if(reallocAfterAllReplacements) {
		output = realloc(output, outputLength + 1);
	}
	return output;
}

char* toLowerCase(char* text) {
	int textLength = strlen(text);
	char* output = malloc(textLength + 1);

	for(int i = 0; i < textLength; i++) {
		output[i] = (isupper(text[i]) ? tolower(text[i]) : text[i]);
	}
	output[textLength] = '\0';

	return output;
}
