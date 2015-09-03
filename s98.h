/* See LICENSE for licence details. */
/*
	S98V1 file format:

	[HEADER FORMAT]

	0000 3BYTE  MAGIC 'S98'
	0003 1BYTE  FORMAT VERSION '1''
	0004 4BYTE(LE)  TIMER INFO  sync numerator. If value is 0, default time is 10.
	0008 4BYTE(LE)  TIMER INFO2 reserved (always 0)
	000C 4BYTE(LE)  COMPRESSING not used?
	0010 4BYTE(LE)  FILE OFFSET TO TAG If value is 0, no title exist. TAG string ends by '0x00'
	0014 4BYTE(LE)  FILE OFFSET TO DUMP DATA
	0018 4BYTE(LE)  FILE OFFSET TO LOOP POINT DUMP DATA

	no device info?

	S98V2 file format:
		<no information>

	S98V3 file format:

	[HEADER FORMAT]

	0000 3BYTE  MAGIC 'S98'
	0003 1BYTE  FORMAT VERSION '3''
	0004 4BYTE(LE)  TIMER INFO  sync numerator. If value is 0, default time is 10.
	0008 4BYTE(LE)  TIMER INFO2 sync denominator. If value is 0, default time is 1000.
	000C 4BYTE(LE)  COMPRESSING The value is 0 always.
	0010 4BYTE(LE)  FILE OFFSET TO TAG If value is 0, no title exist. TAG string include multiple line (separated with LF '0x0A') and ends by '0x00'
	0014 4BYTE(LE)  FILE OFFSET TO DUMP DATA
	0018 4BYTE(LE)  FILE OFFSET TO LOOP POINT DUMP DATA
	001C 4BYTE(LE)  DEVICE COUNT    If value is 0, default type is OPNA, clock is 7987200Hz
	0020 4BYTE(LE)  DEVICE INFO * count

	[DEVICE INFO]

	0000 4BYTE(LE)  DEVICE TYPE
	0004 4BYTE(LE)  CLOCK(Hz)
	0008 4BYTE(LE)  PAN
	000C-000F RESERVE
*/

enum s98_misc_t {
	BITS_PER_BYTE    = 8,
	S98_TAGSIZE      = 128,
	S98_MAX_DEVICE   = 64,
	/* S98_DEFAULT_NUMERATOR / S98_DEFAULT_DENOMINATOR == 1 step (sec) */
	S98_DEFAULT_NUMERATOR   = 10,
	S98_DEFAULT_DENOMINATOR = 1000,
};

enum device_type_t {
	S98_NONE      = 0,
	S98_YM2149    = 1,  /* PSG  */
	S98_YM2203    = 2,  /* OPN  */
	S98_YM2612    = 3,  /* OPN2 */
	S98_YM2608    = 4,  /* OPNA */
	S98_YM2151    = 5,  /* OPM  */
	S98_YM2413    = 6,  /* OPLL */
	S98_YM3526    = 7,  /* OPL  */
	S98_YM3812    = 8,  /* OPL2 */
	S98_YMF262    = 9,  /* OPL3 */
	S98_AY_3_8910 = 15, /* PSG  */
	S98_SN76489   = 16, /* DCSG */
};

struct s98_device_t {
	uint32_t type;
	uint32_t clock;
	uint32_t pan;
};

struct s98_header_t {
	uint8_t version; /* 1, 2 or 3 */
	uint32_t numerator, denominator;
	uint32_t compressing;
	uint32_t offset_tag;
	uint32_t offset_dump;
	uint32_t offset_loop;
	uint32_t device_count;

	struct s98_device_t device[S98_MAX_DEVICE];
};

int read_4byte_le(FILE *fp, uint32_t *value)
{
	int count = 0;
	uint8_t byte;

	*value = 0;

	while (count < 4) {
		if (fread(&byte, 1, 1, fp) != 1)
			return -1;

		*value |= (byte << (BITS_PER_BYTE * count));
		count++;
	}
	return count;
}

bool read_tag_string(FILE *fp, char tag[S98_TAGSIZE], int size)
{
	char *cp;

	cp = tag;

	while (1) {
		*cp = getc(fp);

		if (cp - tag > size) {
			logging(ERROR, "tag buffer overflow\n");
			return false;
		}

		if (*cp == 0x0A || *cp == 0x00) {
			if (*cp == 0x0A)
				*cp = '\0';

			if (cp - tag > 0)
				return true;
			else /* string length == 0, no more tag */
				return false;
		}

		cp++;
	}
}

bool s98_header(FILE *fp, struct s98_header_t *header)
{
	uint8_t buf[BUFSIZE];
	char tag[S98_TAGSIZE];

	/* read common header */
	if (fread(buf, 1, 4, fp) != 4
		|| strncmp((char *) buf, "S98", 3)   != 0) {
		logging(ERROR, "magic number miss match: %c %c %c %c\n",
			buf[0], buf[1], buf[2], buf[3]);
		return false;
	}
	logging(DEBUG, "magic number: '%c '%c' '%c' '%c'\n",
		buf[0], buf[1], buf[2], buf[3]);

	header->version = buf[3] - '0';

	if (read_4byte_le(fp, &header->numerator)       != 4
		|| read_4byte_le(fp, &header->denominator)  != 4
		|| read_4byte_le(fp, &header->compressing)  != 4
		|| read_4byte_le(fp, &header->offset_tag)   != 4
		|| read_4byte_le(fp, &header->offset_dump)  != 4
		|| read_4byte_le(fp, &header->offset_loop)  != 4
		|| read_4byte_le(fp, &header->device_count) != 4)
		return false;

	logging(DEBUG, "S98 header:\n"
		"\tversion     : %u\n"
		"\tnumerator   : %u\n"
		"\tdenominator : %u\n"
		"\tcompressing : %u\n"
		"\toffset_tag  : 0x%.8X\n"
		"\toffset_dump : 0x%.8X\n"
		"\toffset_loop : 0x%.8X\n"
		"\tdevice_count: %u\n",
		header->version, header->numerator, header->denominator,
		header->compressing, header->offset_tag, header->offset_dump,
		header->offset_loop, header->device_count);

	/* read device info */
	if (header->version == 3 && header->device_count > 0) {
		for (unsigned int i = 0; i < header->device_count; i++) {
			if (read_4byte_le(fp, &header->device[i].type)     != 4
				|| read_4byte_le(fp, &header->device[i].clock) != 4
				|| read_4byte_le(fp, &header->device[i].pan)   != 4)
				continue;
		}
	} else { /* header->version == 1 or header->device_count == 0 */
		/* default device info:
		 *
	 	 * device type: OPNA
		 * clock      : 7987200Hz
		 * pan        : all mute disable
		 */
		 header->device_count    = 1;
		 header->device[0].type  = S98_YM2608;
		 header->device[0].clock = 7987200;
		 header->device[0].pan   = 0x00;
	}

	for (unsigned int i = 0; i < header->device_count; i++) {
		logging(DEBUG, "device[%d] info:\n"
			"\ttype : %u\n"
			"\tclock: %u\n"
			"\tpan  : 0x%.8X\n",
			i,
			header->device[i].type,
			header->device[i].clock,
			header->device[i].pan);
	}

	/* read tag */
	if (header->offset_tag != 0) {
		logging(DEBUG, "tag:\n");

		fseek(fp, header->offset_tag, SEEK_SET);

		while (read_tag_string(fp, tag, S98_TAGSIZE))
			logging(DEBUG, "%s\n", tag);
	}

	/* seek to data dump offset */
	fseek(fp, header->offset_dump, SEEK_SET);

	return true;
}

long read_7bit_variable_le(FILE *fp)
{
	char byte;
	int count = 0;
	long nsync = 0L;

	do {
		if (fread(&byte, 1, 1, fp) != 1)
			return -1;

		nsync |= (byte & 0x7F) << (7 * count);
		count++;
	} while (byte & 0x80);

	return nsync;
}

void sync_wait(double step, long nsync)
{
	logging(DEBUG, "step:%lf nsync:%ld\n", step, nsync);
	usleep(step * nsync * 1000000);
}

/*
	[DUMP DATA FORMAT]

	00 aa dd  DEVICE1(normal)
	01 aa dd  DEVICE1(extend)
	02 aa dd  DEVICE2(normal)
	03 aa dd  DEVICE2(extend)
	...
	FF        1SYNC
	FE vv     nSYNC
	FD        END/LOOP

	S98V1: 1 SYNC = TIMER INFO / 1000 (sec)
	S98V3: 1 SYNC = TIMER INFO / TIMER INFO2 (sec)

	DEVICE1, DEVICE2, ...: according to the order of DEVICE INFO

	normal/extend
		PSG/OPN/OPM/OPLL/OPL/OPL2   only normal
		OPNA/OPN2/OPL3              normal/extend
		DCSG                        only normal
*/

bool s98_parse(int fd, FILE *fp)
{
	uint8_t buf[BUFSIZE], op;
	long nsync = 0L;
	double step;
	struct s98_header_t header;
	extern volatile sig_atomic_t catch_sigint;

	if (s98_header(fp, &header) == false) {
		logging(ERROR, "s98_header() failed\n");
		return false;
	}

	if (header.numerator != 0 && header.denominator != 0)
		step = (double) header.numerator / header.denominator;
	else if (header.numerator != 0)
		step = (double) header.numerator / S98_DEFAULT_DENOMINATOR;
	else
		step = (double) S98_DEFAULT_NUMERATOR / S98_DEFAULT_DENOMINATOR;

	logging(DEBUG, "1 step: %lf (sec)\n", step);

	while (catch_sigint == false) {
		if (fread(buf, 1, 1, fp) != 1) {
			logging(ERROR, "couldn't read op of s98 dump\n");
			return false;
		}

		op = buf[0];
		logging(DEBUG, "op: 0x%.2X\n", op);

		switch (op) {
		case 0x00: /* device 1 (normal) */
		case 0x01: /* device 1 (extended) */
			if (fread(buf, 1, 2, fp) == 2) {
				spfm_send(fd, op, buf[0], buf[1]);
				//logging(DEBUG, "0x%.2X 0x%.2X 0x%.2X\n", op, buf[0], buf[1]);
			}
			break;
		/* ignore device 2, device 3, ... and more */
		case 0xFD: /* END/LOOP */
			logging(DEBUG, "end of s98 data\n");
			//fseek(fp, header.offset_dump, SEEK_SET);
			//break;
			return true;
		case 0xFE: /* n sync */
			nsync = read_7bit_variable_le(fp);
			sync_wait(step, nsync);
			break;
		case 0xFF: /* 1 sync */
			sync_wait(step, 1);
			break;
		}
	}

	if (catch_sigint)
		logging(DEBUG, "caught SIGINT\n");

	return true;
}
