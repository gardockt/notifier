#ifndef DIRS_H
#define DIRS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>

char* getConfigDirectory();
char* getStashDirectory();
bool createStashDirectory();

#endif // ifndef DIRS_H
