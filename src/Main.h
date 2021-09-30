#ifndef MAIN_H
#define MAIN_H

#include <signal.h>

#include <iniparser.h>

#ifdef REQUIRED_CURL
#include <curl/curl.h>
#endif

#ifdef REQUIRED_LIBXML
#include <libxml/parser.h>
#endif

#define CONFIG_GLOBAL_SECTION_NAME  "_global"
#define CONFIG_NAME_FIELD_NAME      "_name"
#define CONFIG_TYPE_FIELD_NAME      "module"
#define CONFIG_NAME_SEPARATOR       "."

#endif // ifndef MAIN_H
