#include "Globals.h"
#include "ModuleManager.h"
#include "DisplayManager.h"
#include "Stash.h"
#include "Config.h"
#include "Log.h"
#include "Main.h"

ModuleManager moduleManager;
DisplayManager displayManager;

bool initFunctionality() {
	configLoadCore();

	if(!initModuleManager(&moduleManager)) {
		logWrite("core", coreVerbosity, 0, "Error initializing module manager");
		return false;
	}

	if(!initDisplayManager(&displayManager)) {
		logWrite("core", coreVerbosity, 0, "Error initializing display manager");
		return false;
	}

	if(!stashInit()) {
		logWrite("core", coreVerbosity, 0, "Error loading stash file");
		return false;
	}

#ifdef REQUIRED_CURL
	if(!curl_global_init(CURL_GLOBAL_DEFAULT)) {
		logWrite("core", coreVerbosity, 0, "Error initializing CURL library");
		return false;
	}
#endif

#ifdef REQUIRED_LIBXML
	LIBXML_TEST_VERSION;
#endif

	if(!configLoad()) {
		logWrite("core", coreVerbosity, 0, "Error loading config file");
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
