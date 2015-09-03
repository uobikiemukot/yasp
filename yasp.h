/* See LICENSE for licence details. */
#define _XOPEN_SOURCE 600
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
//#include <sys/mman.h>
#include <termios.h>
#include <unistd.h>

enum misc_t {
	BUFSIZE        = 16,
	SELECT_TIMEOUT = 15000, /* usec */
	OPNA_SLOT_NUM  = 0x01, /* slot0: 0x00, slot1: 0x01 */
};

static const char *serial_dev      = "/dev/ttyUSB0";
volatile sig_atomic_t catch_sigint = false;
