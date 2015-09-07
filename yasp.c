/* See LICENSE for licence details. */
#include "yasp.h"
#include "error.h"
#include "spfm.h"
#include "util.h"
#include "vgm.h"
#include "s98.h"
#include "play.h"

void usage()
{
	printf(
		"usage: yasp FILE\n"
		"\tavailable format: S98(S98V1/S98V3), VGM(YM2608+ADPCM/YM2151)\n"
	);
}

void sig_handler(int signo)
{
	extern volatile sig_atomic_t catch_sigint;

	if (signo == SIGINT)
		catch_sigint = true;
}

int set_signal(int signo, void (*sig_handler)(int signo))
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = sig_handler;
	sigact.sa_flags   = SA_RESTART;
	return esigaction(signo, &sigact, NULL);
}

int main(int argc, char *argv[])
{
	int serial_fd = -1;
	struct termios old_termio;

	/* check args */
	if (argc <= 1) {
		usage();
		goto err;
	};

	/* initalize */
	if ((serial_fd = serial_init(&old_termio)) < 0) {
		logging(FATAL, "serial_init() failed\n");
		goto err;
	}

	if (spfm_reset(serial_fd) == false) {
		logging(FATAL, "spfm_reset() failed\n");
		goto err;
	}

	if (set_signal(SIGINT, sig_handler) < 0) {
		logging(FATAL, "set_signal() failed\n");
		goto err;
	}

	/* play file */
	if (play_file(serial_fd, argv[1]) == false) {
		logging(WARN, "play_file() failed\n");
		goto err;
	}

	/* end process */
	spfm_reset(serial_fd);
	serial_die(serial_fd, &old_termio);
	return EXIT_SUCCESS;

err:
	if (serial_fd != -1) {
		spfm_reset(serial_fd);
		serial_die(serial_fd, &old_termio);
	}
	return EXIT_FAILURE;
}
