#include "test.h"

void (*before_function)() = NULL;
int tests_total = 0;
int tests_passed = 0;

void set_before_function(void (*new_before_function)()) {
	before_function = new_before_function;
}

void print_test_summary() {
	printf("Tests passed: %d/%d\n", tests_passed, tests_total);
	if(tests_passed != tests_total) {
		printf("Errors occurred!\n");
	}
}
