#include "../includes/signals.hpp"
#include "../includes/SetupException.hpp"

volatile sig_atomic_t g_running = 1;


static void handleShutdown(int)
{
	g_running = 0;
}

void setupSignals()
{
	struct sigaction sa; // the struct that describes how to handle a signal

	sa.sa_handler = handleShutdown; // function to run
	sigemptyset(&sa.sa_mask); // mask = no extra signals blocked during the handler
	sa.sa_flags = 0; // no special flags

	if (sigaction(SIGINT, &sa, NULL) == -1)
		throw SetupException("sigaction failed for SIGINT");
	if (sigaction(SIGTERM, &sa, NULL) == -1)
		throw SetupException("sigaction failed for SIGTERM");
	if (sigaction(SIGQUIT, &sa, NULL) == -1)
		throw SetupException("sigaction failed for SIGQUIT");

	sa.sa_handler = SIG_IGN; // don't shut down, just ignore
	if (sigaction(SIGPIPE, &sa, NULL) == -1)
		throw SetupException("sigaction failed for SIGPIPE");
}
