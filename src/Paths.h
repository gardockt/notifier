#ifndef PATHS_H
#define PATHS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>

char* get_config_file_path();
char* get_stash_path();
bool create_stash_dir();
char* get_fm_path();
char* get_display_path();

#endif /* ifndef PATHS_H */
