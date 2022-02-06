cmake_minimum_required(VERSION 3.10)

project(notifier LANGUAGES C)

option(ENABLE_DISPLAY_LIBNOTIFY "Compile libnotify display" ON)

option(ENABLE_FM_GITHUB "Compile GitHub fetching module" ON)
option(ENABLE_FM_ISOD "Compile ISOD fetching module" ON)
option(ENABLE_FM_RSS "Compile RSS fetching module" ON)
option(ENABLE_FM_TWITCH "Compile Twitch fetching module" ON)

find_package(Threads REQUIRED)
find_package(CURL)
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
	endif()
endmacro()



### FETCHING MODULES ###

set(fm_github_sources
	src/FetchingModules/Github.c
	src/FetchingModules/Utilities/Json.c
	src/FetchingModules/Utilities/Network.c
)

set(fm_isod_sources
	src/FetchingModules/Isod.c
	src/FetchingModules/Utilities/Json.c
	src/FetchingModules/Utilities/Network.c
)

set(fm_rss_sources
	src/FetchingModules/Rss.c
	src/FetchingModules/Utilities/Network.c
)

set(fm_twitch_sources
	src/FetchingModules/Twitch.c
	src/FetchingModules/Utilities/Json.c
	src/FetchingModules/Utilities/Network.c
	src/Structures/BinaryTree.c
	src/Structures/SortedMap.c
)

macro(configure_fm name dependencies)
	string(TOUPPER ${name} name_upper)
	set(target_name fm_${name})
	check_dependencies(FM_${name_upper} ${dependencies})
	if(ENABLE_FM_${name_upper})
		add_library(${target_name} SHARED ${${target_name}_sources})
		set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/fetchingmodules LIBRARY_OUTPUT_NAME ${name} PREFIX "")
		foreach(dep ${dependencies})
			string(TOUPPER ${dep} dep_upper)
			target_include_directories(${target_name} PUBLIC ${${dep_upper}_INCLUDE_DIRS})
			target_compile_options(${target_name} PUBLIC ${${dep_upper}_CFLAGS_OTHER})
			target_link_libraries(${target_name} PUBLIC ${${dep_upper}_LIBRARIES})
		endforeach()
	endif()
endmacro()

configure_fm(github  "curl;json-c")
configure_fm(isod    "curl;json-c")
configure_fm(rss     "curl;libxml2")
configure_fm(twitch  "curl;json-c")



### MAIN EXECUTABLE ###

set(notifier_sources
	src/Config.c
	src/DisplayManager.c
	src/Log.c
	src/Main.c
	src/Message.c
	src/ModuleManager.c
	src/Paths.c
	src/Stash.c
	src/StringOperations.c
	src/Displays/Libnotify.c
	src/Structures/BinaryTree.c
	src/Structures/SortedMap.c
)

add_executable(notifier ${notifier_sources})

macro(check_main_dependencies module_name dependencies)
	check_dependencies(${module_name} ${dependencies})
	if(${ENABLE_${module_name}})
		add_definitions("-DENABLE_${module_name}")
		foreach(dep ${dependencies})
			string(TOUPPER ${dep} dependency_name_uppercase)
			set(COMPILE_${dependency_name_uppercase} TRUE)
		endforeach()
	endif()
endmacro()

check_main_dependencies(DISPLAY_LIBNOTIFY   "libnotify")

# for some reason that's where Ubuntu stores iniparser's header files
if(UNIX)
	target_include_directories(notifier PUBLIC "/usr/include/iniparser")
endif()

target_compile_options(notifier PUBLIC -ggdb)
target_link_libraries(notifier PUBLIC -liniparser -lm -ldl ${CMAKE_THREAD_LIBS_INIT})

function(compile_if_needed dependency_name)
	string(TOUPPER ${dependency_name} dependency_name_uppercase)
	if(COMPILE_${dependency_name_uppercase})
		target_include_directories(notifier PUBLIC ${${dependency_name_uppercase}_INCLUDE_DIRS})
		target_compile_options(notifier PUBLIC ${${dependency_name_uppercase}_CFLAGS_OTHER})
		target_link_libraries(notifier PUBLIC ${${dependency_name_uppercase}_LIBRARIES})
	endif()
endfunction()

compile_if_needed(libnotify)

install(TARGETS notifier RUNTIME DESTINATION bin)
