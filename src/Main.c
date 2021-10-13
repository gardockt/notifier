#include "Globals.h"
#include "ModuleManager.h"
#include "DisplayManager.h"
#include "Stash.h"
#include "Config.h"
#include "Main.h"

ModuleManager moduleManager;
DisplayManager displayManager;

bool initFunctionality() {
	if(!initModuleManager(&moduleManager)) {
		fprintf(stderr, "Error initializing module manager!\n");
		return false;
	}

	if(!initDisplayManager(&displayManager)) {
		fprintf(stderr, "Error initializing display manager!\n");
		return false;
	}

	if(!stashInit()) {
		fprintf(stderr, "Error loading stash file!\n");
		return false;
	}

#ifdef REQUIRED_CURL
	if(!curl_global_init(CURL_GLOBAL_DEFAULT)) {
		fprintf(stderr, "Error initializing CURL library!\n");
		return false;
	}
#endif

#ifdef REQUIRED_LIBXML
	LIBXML_TEST_VERSION;
#endif

	if(!configLoad()) {
		fprintf(stderr, "Error loading config file!\n");
		return false;
	}

	return true;
}

void destroyFunctionality(int signal) {
	destroyModuleManager(&moduleManager);
	destroyDisplayManager(&displayManager);
	stashDestroy();

#ifdef REQUIRED_CURL
	curl_global_cleanup();
#endif

#ifdef REQUIRED_LIBXML
	xmlCleanupParser();
#endif
}

int main() {
	struct sigaction signalMgmt;
	signalMgmt.sa_handler = destroyFunctionality;
	sigemptyset(&signalMgmt.sa_mask);
	signalMgmt.sa_flags = 0;
	sigaction(SIGINT, &signalMgmt, NULL);

	if(!initFunctionality()) {
		return 1;
	}

	pause();

	return 0;
}
