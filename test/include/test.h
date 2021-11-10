#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdbool.h>

extern void (*before_function)();

extern int tests_total;
extern int tests_passed;

#define do_test(function) {\
	if(before_function != NULL) before_function();\
	printf("%s", #function);\
	bool result = function();\
	printf(" -> %s\n", result ? "success" : "FAILURE!!!");\
	if(result) tests_passed++;\
	tests_total++;\
}

void set_before_function(void (*new_before_function)());
void print_test_summary();

#endif // ifndef TEST_H
