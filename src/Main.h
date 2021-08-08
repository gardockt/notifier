#ifndef MAIN_H
#define MAIN_H

#include <signal.h>

#include <iniparser.h>

#ifdef REQUIRED_CURL
#include <curl/curl.h>
#endif

#define CONFIG_FILE                 "notifier.ini"
#define CONFIG_GLOBAL_SECTION_NAME  "_global"

#endif // ifndef MAIN_H
