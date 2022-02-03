cmake_minimum_required(VERSION 3.10)

project(notifier LANGUAGES C)

option(ENABLE_DISPLAY_LIBNOTIFY "Compile libnotify display" ON)

option(ENABLE_MODULE_GITHUB "Compile GitHub module" ON)
option(ENABLE_MODULE_ISOD "Compile ISOD module" ON)
option(ENABLE_MODULE_RSS "Compile RSS module" ON)
option(ENABLE_MODULE_TWITCH "Compile Twitch module" ON)

set(notifier_sources
	src/Config.c
	src/DisplayManager.c
	src/Log.c
	src/Main.c
	src/Message.c
	src/ModuleManager.c
	src/Network.c
	src/Paths.c
	src/Stash.c
	src/StringOperations.c
	src/Displays/Libnotify.c
	src/FetchingModules/Utilities/FetchingModuleUtilities.c
	src/Structures/BinaryTree.c
	src/Structures/SortedMap.c
)

add_executable(notifier ${notifier_sources})

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
target_link_libraries(notifier PUBLIC -liniparser -lm -ldl ${CMAKE_THREAD_LIBS_INIT})

function(compile_if_needed dependency_name)
	string(TOUPPER ${dependency_name} dependency_name_uppercase)
	if(COMPILE_${dependency_name_uppercase})
		target_include_directories(notifier PUBLIC ${${dependency_name_uppercase}_INCLUDE_DIRS})
		target_compile_options(notifier PUBLIC ${${dependency_name_uppercase}_CFLAGS_OTHER})
		target_link_libraries(notifier PUBLIC ${${dependency_name_uppercase}_LIBRARIES})
	endif()
endfunction()

compile_if_needed(curl)
compile_if_needed(libnotify)
compile_if_needed(json-c)
compile_if_needed(libxml2)

install(TARGETS notifier RUNTIME DESTINATION bin)
