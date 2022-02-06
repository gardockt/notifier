#include "Globals.h"
#include "ModuleManager.h"
#include "DisplayManager.h"
#include "Stash.h"
#include "Config.h"
#include "Log.h"
#include "Paths.h"
#include "Main.h"

ModuleManager module_manager;
DisplayManager display_manager;

static int silent_error_callback(const char* format, ...) {
	return 0;
}

static bool init_functionality() {
	config_load_core();

	/* silence iniparser errors on core verbosity <= 0 */
	iniparser_set_error_callback(core_verbosity <= 0 ? silent_error_callback : NULL);

	if(!fm_manager_init(&module_manager)) {
		log_write("core", core_verbosity, 0, "Error initializing module manager");
		return false;
	}

	if(!display_manager_init(&display_manager)) {
		log_write("core", core_verbosity, 0, "Error initializing display manager");
		return false;
	}

	if(!stash_init()) {
		log_write("core", core_verbosity, 0, "Error loading stash file");
		return false;
	}

#ifdef REQUIRED_CURL
	if(!curl_global_init(CURL_GLOBAL_DEFAULT)) {
		log_write("core", core_verbosity, 0, "Error initializing CURL library");
		return false;
	}
#endif

#ifdef REQUIRED_LIBXML
	LIBXML_TEST_VERSION;
#endif

	if(!config_load()) {
		log_write("core", core_verbosity, 0, "Error loading config file");
		return false;
	}

	return true;
}

static void destroy_functionality(int signal) {
	fm_manager_destroy(&module_manager);
	display_manager_destroy(&display_manager);
	stash_destroy();

#ifdef REQUIRED_CURL
	curl_global_cleanup();
#endif

#ifdef REQUIRED_LIBXML
	xmlCleanupParser();
#endif
}

int main() {
	struct sigaction signal_mgmt;
	signal_mgmt.sa_handler = destroy_functionality;
	sigemptyset(&signal_mgmt.sa_mask);
	signal_mgmt.sa_flags = 0;
	sigaction(SIGINT, &signal_mgmt, NULL);

	if(!init_functionality()) {
		return 1;
	}

	pause();

	return 0;
}
