/* See LICENSE for licence details. */
enum fd_state_t {
	FD_IS_BUSY     = -1,
	FD_IS_READABLE = 0,
	FD_IS_WRITABLE = 1,
};

enum check_type_t {
	CHECK_READ_FD  = 0,
	CHECK_WRITE_FD = 1,
};

/* spfm functions */
int serial_init(struct termios *old_termio)
{
	int fd = -1;
	bool get_old_termio = false;
	struct termios cur_termio;

	if ((fd = eopen(serial_dev, O_RDWR | O_NOCTTY | O_NDELAY)) < 0
		|| etcgetattr(fd, old_termio) < 0)
		goto err;

	get_old_termio = true;
	cur_termio = *old_termio;

	/*
	 * SPFM light serial:
	 *
	 * baud        : 1500000
	 * data size   : 8bit
	 * parity      : none
	 * flow control: disable
	 */

	cur_termio.c_iflag = 0;
	cur_termio.c_oflag = 0;
	cur_termio.c_lflag = 0;
	cur_termio.c_cflag = CS8;

	cur_termio.c_cc[VMIN]  = 1;
	cur_termio.c_cc[VTIME] = 0;

	if (ecfsetispeed(&cur_termio, B1500000) < 0
		|| etcsetattr(fd, TCSAFLUSH, &cur_termio) < 0)
		goto err;

	return fd;

err:
	if (get_old_termio)
		etcsetattr(fd, TCSAFLUSH, old_termio);

	if (fd != -1)
		eclose(fd);

	return -1;
}

void serial_die(int fd, struct termios *old_termio)
{
	etcsetattr(fd, TCSAFLUSH, old_termio);
	eclose(fd);
}

/*
 * SPFM light protocol:
 * <client>                <light>
 *
 * check interface:
 *         --> 0xFF    -->
 *         <-- 'L' 'T' <--
 * reset:
 *         --> 0xFE    -->
 *         <-- 'O' 'K' <--
 * nop:
 *         --> 0x80    -->
 *         <-- (none)  <--
 *
 * send register data:
 *
 * 1st byte: module number (0x00 or 0x01)
 * 2nd byte: command byte  (0x0n, n: set A0-A3 bit)
 * 3rd byte: register address
 * 4th byte: register data
 *
 * command byte:
 *
 * bit 0: A0 bit
 * bit 1: A1 bit
 * bit 2: A2 bit
 * bit 3: A3 bit
 * bit 4: CS1 bit
 * bit 5: CS2 bit
 * bit 6: CS3 bit
 * bit 7: (on: check/reset/nop, off: other commands)
 *
 * send data:
 *
 * 1st byte: module number (0x00 or 0x01)
 * 2nd byte: command byte  (0x8n, n: set A0-A3 bit)
 * 3rd byte: data
 *
 * SN76489 send data:
 *
 * 1st byte: module number (0x00 or 0x01)
 * 2nd byte: command byte  (0x20)
 * 3rd byte: data
 * 4th byte: 0x00          (dummy)
 * 5th byte: 0x00          (dummy)
 * 6th byte: 0x00          (dummy)
 *
 * read data (not implemented):
 *
 * 1st byte: module number (0x00 or 0x01)
 * 2nd byte: command byte  (0x4n, n: set A0-A3 bit?)
 * 3rd byte: register address
 *
 */

enum fd_state_t check_fds(int fd, enum check_type_t type)
{
	struct timeval tv;
	fd_set rfds, wfds;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	FD_SET(fd, &rfds);
	FD_SET(fd, &wfds);

	tv.tv_sec  = 0;
	tv.tv_usec = SELECT_TIMEOUT;

	eselect(fd + 1, &rfds, &wfds, NULL, &tv);

	if (type == CHECK_READ_FD && FD_ISSET(fd, &rfds))
		return FD_IS_READABLE;
	else if (type == CHECK_WRITE_FD && FD_ISSET(fd, &wfds))
		return FD_IS_WRITABLE;
	else
		return FD_IS_BUSY;
}

void send_data(int fd, uint8_t *buf, int size)
{
	ssize_t wsize;
	
	while (check_fds(fd, CHECK_WRITE_FD) != FD_IS_WRITABLE);

	wsize = ewrite(fd, buf, size);

	/*
	if (LOG_LEVEL == DEBUG) {
		logging(DEBUG, "%ld byte(s) wrote\t", wsize);
		for (int i = 0; i < wsize; i++)
			fprintf(stderr, "0x%.2X ", buf[i]);
		fprintf(stderr, "\n");
	}
	*/
}

void recv_data(int fd, uint8_t *buf, int size)
{
	ssize_t rsize;

	while (check_fds(fd, CHECK_READ_FD) != FD_IS_READABLE);

	rsize = eread(fd, buf, size);

	/*
	if (LOG_LEVEL == DEBUG) {
		logging(DEBUG, "%ld byte(s) read\t", rsize);
		for (int i = 0; i < rsize; i++)
			fprintf(stderr, "0x%.2X ('%c') ", buf[i], buf[i]);
		fprintf(stderr, "\n");
	}
	*/
}

bool spfm_reset(int fd)
{
	uint8_t buf[BUFSIZE];

	send_data(fd, &(uint8_t){0xFF}, 1);
	recv_data(fd, buf, BUFSIZE);

	if (strncmp((char *) buf, "LT", 2) != 0)
		return false;

	send_data(fd, &(uint8_t){0xFE}, 1);
	recv_data(fd, buf, BUFSIZE);

	if (strncmp((char *) buf, "OK", 2) != 0)
		return false;

	return true;
}

void OPNA_register_info(uint8_t port, uint8_t addr)
{
	if (port == 0x00) {
		if (addr <= 0x0F)
			logging(DEBUG, "SSG:\n");
		else if (0x10 <= addr && addr <= 0x1F)
			logging(DEBUG, "RHYTHM:\n");
		else if (0x20 <= addr && addr <= 0x2F)
			logging(DEBUG, "FM COMMON:\n");
		else if (0x30 <= addr && addr <= 0xB6)
			logging(DEBUG, "FM (1-3ch):\n");
		else
			logging(DEBUG, "unknown:\n");
	} else {
		if (addr <= 0x10)
			logging(DEBUG, "ADPCM:\n");
		else if (0x30 <= addr && addr <= 0xB6)
			logging(DEBUG, "FM (4-6ch):\n");
		else
			logging(DEBUG, "unknown:\n");
	}
}

void spfm_send(int fd, uint8_t slot, uint8_t port, uint8_t addr, uint8_t data)
{
	send_data(fd, &slot, 1);

	if (port == 0x00)
		send_data(fd, &(uint8_t){0x00}, 1);
	else /* OPNA extend: A1 bit on */
		send_data(fd, &(uint8_t){0x02}, 1);

	send_data(fd, &addr, 1);

	send_data(fd, &data, 1);

	logging(DEBUG, "slot:0x%.2X port:0x%.2X addr:0x%.2X data:0x%.2X\n",
		slot, port, addr, data);
}
