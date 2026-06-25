#include "../includes/signals.hpp"
#include "../includes/SetupException.hpp"

volatile sig_atomic_t g_running = 1;

// Sets g_running to 0 to stop the server loop.
static void handleShutdown(int)
{
	g_running = 0;
}

// Registers SIGINT, SIGTERM, SIGQUIT to stop the server; ignores SIGPIPE.
void setupSignals()
{
	struct sigaction sa;

	sa.sa_handler = handleShutdown;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1)
		throw SetupException("sigaction failed for SIGINT");
	if (sigaction(SIGTERM, &sa, NULL) == -1)
		throw SetupException("sigaction failed for SIGTERM");
	if (sigaction(SIGQUIT, &sa, NULL) == -1)
		throw SetupException("sigaction failed for SIGQUIT");

	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) == -1)
		throw SetupException("sigaction failed for SIGPIPE");
}
