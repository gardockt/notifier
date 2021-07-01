#include "Main.h"
#include "ModuleManager.h"
#include "FetchingModules/HelloWorld.h"

ModuleManager moduleManager;

void onExit(int signal) {
	printf("Niszczenie moduleManager...\n");
	destroyModuleManager(&moduleManager);
}

int main() {
	HelloWorldConfig* config = malloc(sizeof *config);

	struct sigaction signalMgmt;
	signalMgmt.sa_handler = onExit;
	sigemptyset(&signalMgmt.sa_mask);
	signalMgmt.sa_flags = 0;
	if(sigaction(SIGINT, &signalMgmt, NULL) == -1) {
		// blad
	}

	config->text = "gitara siema";

	initModuleManager(&moduleManager);
	enableModule(&moduleManager, "HelloWorld", "Hello World", config);
	sleep(1000);
	return 0;
}
