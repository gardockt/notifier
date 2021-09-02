#include "StringOperations.h"

bool isCharacterSeparator(char character, char* separators) {
	int separatorCount = strlen(separators);
	for(int i = 0; i < separatorCount; i++) {
		if(character == separators[i]) {
			return true;
		}
	}
	return false;
}

int split(char* text, char* separators, char*** output) {
	int textLength = strlen(text);
	int textSections = 0;
	bool lastCharacterWasSeparator = true;
	bool isSeparator;

	// count text sections and alloc memory
	for(int i = 0; i < textLength; i++) {
		isSeparator = isCharacterSeparator(text[i], separators);
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
				isSeparator = isCharacterSeparator(text[j], separators);
				if(isSeparator) {
					(*output)[i] = malloc(j - textStart + 1);
					strncpy((*output)[i], &text[textStart], j - textStart);
					(*output)[i][j - textStart] = '\0';
					textStart = j + 1;
					break;
				}
			} else {
				(*output)[i] = malloc(textLength - textStart + 1);
				strcpy((*output)[i], &text[textStart]);
				(*output)[i][j - textStart] = '\0';
			}
		}
		while(textStart < textLength && isCharacterSeparator(text[textStart], separators)) {
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

char* replace(char* text, const char* before, const char* after) {
	int outputLength = strlen(text);
	int beforeLength = strlen(before);
	int afterLength = strlen(after);
	int diffLength = afterLength - beforeLength;

	char* output = malloc(outputLength + 1);
	int textPointer = 0;
	int pointerDiff = 0;
	int nextOccurrence;

	while((nextOccurrence = find(&text[textPointer], before)) != -1) {
		outputLength += diffLength;
		if(diffLength > 0) {
			output = realloc(output, outputLength + 1);
		}
		strncpy(&output[textPointer + pointerDiff], &text[textPointer], nextOccurrence - textPointer);
		strcpy(&output[nextOccurrence + pointerDiff], after);
		textPointer = nextOccurrence + beforeLength;
		pointerDiff += diffLength;
	}
	strcpy(&output[textPointer + pointerDiff], &text[textPointer]);
	if(diffLength < 0) {
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
