cmake_minimum_required(VERSION 3.10)

project(notifier VERSION 0.1)

option(ENABLE_DISPLAY_LIBNOTIFY "Compile libnotify display" ON)

option(ENABLE_MODULE_GITHUB "Compile GitHub module" ON)
option(ENABLE_MODULE_ISOD "Compile ISOD module" ON)
option(ENABLE_MODULE_RSS "Compile RSS module" ON)
option(ENABLE_MODULE_TWITCH "Compile Twitch module" ON)

file(GLOB_RECURSE source_files "src/*.h" "src/*.c")
add_executable(notifier ${source_files})

find_package(CURL REQUIRED) # can be eventually set as optional
find_package(LibXml2)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNOTIFY libnotify)
pkg_check_modules(JSON-C json-c)

macro(check_dependency module_name dependency_name)
	string(TOUPPER ${dependency_name} dependency_name_uppercase)
	if(${${dependency_name_uppercase}_FOUND})
	else() # unfortunately NOT does not work as intended
		message(WARNING "${dependency_name} not found, disabling ${module_name}")
		set(ENABLE_${module_name} OFF)
		break()
	endif()
endmacro()

macro(check_dependencies module_name dependencies)
	if(${ENABLE_${module_name}})
		foreach(dep ${dependencies})
			check_dependency(${module_name} ${dep})
		endforeach()
		if(${ENABLE_${module_name}})
			add_definitions("-DENABLE_${module_name}")
		endif()
	endif()
endmacro()

check_dependencies(DISPLAY_LIBNOTIFY   "libnotify")
check_dependencies(MODULE_GITHUB       "curl;json-c")
check_dependencies(MODULE_ISOD         "curl;json-c")
check_dependencies(MODULE_RSS          "curl;libxml2")
check_dependencies(MODULE_TWITCH       "curl;json-c")

target_include_directories(notifier PUBLIC ${LIBNOTIFY_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${JSON-C_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS}})
target_compile_options(notifier PUBLIC -ggdb ${LIBNOTIFY_CFLAGS_OTHER} ${CURL_CFLAGS_OTHER} ${JSON-C_CFLAGS_OTHER} ${LIBXML2_CFLAGS_OTHER})
target_link_libraries(notifier -lpthread -liniparser -lm ${LIBNOTIFY_LIBRARIES} ${CURL_LIBRARIES} ${JSON-C_LIBRARIES} ${LIBXML2_LIBRARIES})

install(TARGETS notifier)
