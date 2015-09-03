/* See LICENSE for licence details. */
#include "yasp.h"
#include "error.h"
#include "spfm.h"
#include "s98.h"

void sig_handler(int signo)
{
	extern volatile sig_atomic_t catch_sigint;

	if (signo == SIGINT)
		catch_sigint = true;
}

void set_signal(int signo, void (*sig_handler)(int signo))
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = sig_handler;
	sigact.sa_flags   = SA_RESTART;
	esigaction(signo, &sigact, NULL);
}

int main(int argc, char *argv[])
{
	int fd;
	FILE *fp;
	struct termios old_termio;

	if ((fd = serial_init(&old_termio)) < 0) {
		logging(FATAL, "serial_init() failed\n");
		return EXIT_FAILURE;
	}

	if (spfm_reset(fd) == false) {
		logging(FATAL, "spfm_reset() failed\n");
		goto err;
	}

	set_signal(SIGINT, sig_handler);

	fp = (argc > 1) ? efopen(argv[1], "r"): stdin;

	if (s98_parse(fd, fp) == false)
		logging(WARN, "s98_parse() failed\n");

	fclose(fp);

	spfm_reset(fd);

	serial_die(fd, &old_termio);

	return EXIT_SUCCESS;

err:
	spfm_reset(fd);
	serial_die(fd, &old_termio);
	return EXIT_FAILURE;
}
