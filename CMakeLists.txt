cmake_minimum_required(VERSION 3.10)

project(notifier VERSION 0.1 LANGUAGES C)

option(ENABLE_DISPLAY_LIBNOTIFY "Compile libnotify display" ON)

option(ENABLE_MODULE_GITHUB "Compile GitHub module" ON)
option(ENABLE_MODULE_ISOD "Compile ISOD module" ON)
option(ENABLE_MODULE_RSS "Compile RSS module" ON)
option(ENABLE_MODULE_TWITCH "Compile Twitch module" ON)

file(GLOB_RECURSE source_files "src/*.h" "src/*.c")
add_executable(notifier ${source_files})

find_package(Threads REQUIRED)
find_package(CURL REQUIRED) # can be eventually set as optional
find_package(LibXml2)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNOTIFY libnotify)
pkg_check_modules(JSON-C json-c)

macro(check_dependency module_name dependency_name)
	string(TOUPPER ${dependency_name} dependency_name_uppercase)
	if(NOT DEFINED ${dependency_name_uppercase}_FOUND OR NOT "${${dependency_name_uppercase}_FOUND}")
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
			foreach(dep ${dependencies})
				string(TOUPPER ${dep} dependency_name_uppercase)
				set(COMPILE_${dependency_name_uppercase} TRUE)
			endforeach()
		endif()
	endif()
endmacro()

check_dependencies(DISPLAY_LIBNOTIFY   "libnotify")
check_dependencies(MODULE_GITHUB       "curl;json-c")
check_dependencies(MODULE_ISOD         "curl;json-c")
check_dependencies(MODULE_RSS          "curl;libxml2")
check_dependencies(MODULE_TWITCH       "curl;json-c")

# for some reason that's where Ubuntu stores iniparser's header files
if(UNIX)
	target_include_directories(notifier PUBLIC "/usr/include/iniparser")
endif()

target_compile_options(notifier PUBLIC -ggdb)
target_link_libraries(notifier PUBLIC -liniparser -lm ${CMAKE_THREAD_LIBS_INIT})

if(COMPILE_CURL)
	target_include_directories(notifier PUBLIC ${CURL_INCLUDE_DIRS})
	target_compile_options(notifier PUBLIC ${CURL_CFLAGS_OTHER})
	target_link_libraries(notifier PUBLIC ${CURL_LIBRARIES})
endif()

if(COMPILE_LIBNOTIFY)
	target_include_directories(notifier PUBLIC ${LIBNOTIFY_INCLUDE_DIRS})
	target_compile_options(notifier PUBLIC ${LIBNOTIFY_CFLAGS_OTHER})
	target_link_libraries(notifier PUBLIC ${LIBNOTIFY_LIBRARIES})
endif()

if(COMPILE_JSON-C)
	target_include_directories(notifier PUBLIC ${JSON-C_INCLUDE_DIRS})
	target_compile_options(notifier PUBLIC ${JSON-C_CFLAGS_OTHER})
	target_link_libraries(notifier PUBLIC ${JSON-C_LIBRARIES})
endif()

if(COMPILE_LIBXML2)
	target_include_directories(notifier PUBLIC ${LIBXML2_INCLUDE_DIRS})
	target_compile_options(notifier PUBLIC ${LIBXML2_CFLAGS_OTHER})
	target_link_libraries(notifier PUBLIC ${LIBXML2_LIBRARIES})
endif()

install(TARGETS notifier RUNTIME DESTINATION bin)
