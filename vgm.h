/* See LICENSE for licence details. */
/*
	VGM file format:
		ref http://www.smspower.org/uploads/Music/vgmspec170.txt?sid=c0e0e6727a64fd3bd04bd1d4f3ca8702

			 00  01  02  03   04  05  06  07   08  09  0A  0B   0C  0D  0E  0F
		0x00 ["Vgm " ident   ][EoF offset     ][Version        ][SN76489 clock  ]
		0x10 [YM2413 clock   ][GD3 offset     ][Total # samples][Loop offset    ]
		0x20 [Loop # samples ][Rate           ][SN FB ][SNW][SF][YM2612 clock   ]
		0x30 [YM2151 clock   ][VGM data offset][Sega PCM clock ][SPCM Interface ]
		0x40 [RF5C68 clock   ][YM2203 clock   ][YM2608 clock   ][YM2610/B clock ]
		0x50 [YM3812 clock   ][YM3526 clock   ][Y8950 clock    ][YMF262 clock   ]
		0x60 [YMF278B clock  ][YMF271 clock   ][YMZ280B clock  ][RF5C164 clock  ]
		0x70 [PWM clock      ][AY8910 clock   ][AYT][AY Flags  ][VM] *** [LB][LM]
		0x80 [GB DMG clock   ][NES APU clock  ][MultiPCM clock ][uPD7759 clock  ]
		0x90 [OKIM6258 clock ][OF][KF][CF] *** [OKIM6295 clock ][K051649 clock  ]
		0xA0 [K054539 clock  ][HuC6280 clock  ][C140 clock     ][K053260 clock  ]
		0xB0 [Pokey clock    ][QSound clock   ] *** *** *** *** [Extra Hdr ofs  ]

		- Unused space (marked with *) is reserved for future expansion, and must be
		  zero.
		- All integer values are *unsigned* and written in "Intel" byte order (Little
		  Endian), so for example 0x12345678 is written as 0x78 0x56 0x34 0x12.
		- All pointer offsets are written as relative to the current position in the
		  file, so for example the GD3 offset at 0x14 in the header is the file
		  position of the GD3 tag minus 0x14.
		- All header sizes are valid for all versions from 1.50 on, as long as header
		  has at least 64 bytes. If the VGM data starts at an offset that is lower
		  than 0x100, all overlapping header bytes have to be handled as they were
		  zero.
		- VGMs run with a rate of 44100 samples per second. All sample values use this
		  unit.
*/

enum vgm_misc_t {
	VGM_HEADER_SIZE         = 0x100,
	VGM_SAMPLE_RATE         = 44100,
	VGM_DUAL_CHIP_SUPPORT   = 0x40000000, /* if (VGM_DUAL_CHIP_SUPPORT & header->chip_clock) is true, dual chip enable */
	VGM_DATA_OFFSET_FROM    = 0x34, /* VGM data begins from "VGM_DATA_OFFSET_FROM" + VGM_data_offset */
	VGM_DEFAULT_DATA_OFFSET = 0x40, /* prior to ver1.50, VGM data begins from VGM_DEFAULT_DATA_OFFSET */
	VGM_DEFAULT_WAIT1 = 735,
	VGM_DEFAULT_WAIT2 = 882,
};

struct vgm_header_t {
	char magic[4];

	/* VGM ver1.00 */
	uint32_t EOF_offset, version, SN76489_clock,
		YM2413_clock, GD3_offset, total_samples, loop_offset,
		loop_samples;

	/* VGM ver1.01 addtions */
	uint32_t sample_rate;

	/* VGM ver1.10 addtions */
	uint16_t SN76489_feedback;
	uint8_t SN76489_shift_register_width;

	/* VGM ver1.51 addtions */
	uint8_t SN76489_flags;

	/* VGM ver1.10 addtions */
	uint32_t YM2612_clock, YM2151_clock;

	/* VGM ver1.50 addtions */
	uint32_t VGM_data_offset;

	/* VGM ver1.51 addtions */
	uint32_t SEGA_PCM_clock, SEGA_PCM_interface_register,
		RF5C68_clock, YM2203_clock, YM2608_clock, YM2610B_clock,
		YM3812_clock, YM3526_clock, YM8950_clock, YMF262_clock,
		YM278B_clock, YMF271_clock, YMZ280B_clock, RF5C164_clock,
		PWM_clock, AY8910_clock;
	uint8_t AY8910_chip_type, AY8910_flags, YM2203_AY8910_flags, YM2608_AY8910_flags;

	/* VGM ver1.60 addtions */
	uint8_t volume_modifier, reserved0, loop_base;

	/* VGM ver1.51 addtions */
	uint8_t loop_modifier;

	/* VGM ver1.61 addtions */
	uint32_t GameBoy_DMG_clock, NES_APU_clock, Multi_PCM_clock, uPD7759_clock,
		OKIM6258_clock;
	uint8_t OKIM6258_flags, K054539_flags, C140_chip_type, reserved1;

	uint32_t OKIM6295_clock, K051649_clock,
		K054539_clock, HuC6280_clock, C140_clock, K053260_clock,
		POKEY_clock, QSound_clock, reserved2;

	/* VGM ver1.70 addtions */
	uint32_t extra_header_offset, reserved3[16];
};

bool vgm_parse_header(FILE *fp, struct vgm_header_t *header)
{
	size_t size;
	long data_offset;

	/* XXX: very rough parsing, It fails on Big Endian CPUs */
	if ((size = efread(header, 1, VGM_HEADER_SIZE, fp)) != VGM_HEADER_SIZE) {
		logging(ERROR, "file size (%d bytes) is smaller than VGM header size (%d bytes)\n", size, VGM_HEADER_SIZE);
		return false;
	}

	logging(DEBUG, "magic: '%c' '%c' '%c' '%c'\n"
		"\tversion:0x%.4X EOF offest:0x%.8X GD3_offset:0x%.8X\n"
		"\ttotal samples:%u loop offset:0x%.8X loop samples:%u\n"
		"\tsample rate:%u VGM_data_offset:0x%.5X\n",
		header->magic[0], header->magic[1], header->magic[2], header->magic[3],
		header->version, header->EOF_offset, header->GD3_offset,
		header->total_samples, header->loop_offset, header->loop_samples,
		header->sample_rate, header->VGM_data_offset);

	if (header->version < 0x151) /* YM2608 chip is supported from version 1.51 or later */
		header->YM2608_clock = 0;

	logging(DEBUG, "YM2151 clock:%u YM2608 clock:%u\n", header->YM2151_clock, header->YM2608_clock);

	if (header->YM2151_clock == 0 && header->YM2608_clock == 0) {
		logging(ERROR, "only support YM2151 and YM2608 chip\n");
		return false;
	}

	if (header->YM2151_clock & VGM_DUAL_CHIP_SUPPORT
		|| header->YM2608_clock & VGM_DUAL_CHIP_SUPPORT) {
		logging(ERROR, "dual chip not supported\n");
		return false;
	}

	if (header->version < 0x150 || header->VGM_data_offset == 0)
		data_offset = VGM_DEFAULT_DATA_OFFSET;
	else
		data_offset = VGM_DATA_OFFSET_FROM + header->VGM_data_offset;

	logging(DEBUG, "VGM data from: 0x%.4X\n", data_offset);
	if (efseek(fp, data_offset, SEEK_SET) < 0)
		return false;

	return true;
}

void vgm_wait(int nsync)
{
	usleep((double) 1.0 / VGM_SAMPLE_RATE * nsync * 1000000);
}

/*
	YM2608 RAM-WRITE:
		ref: YM2608 (OPNA) Application Manual

		addr    data
		0x10    0x13 // flag brdy/eos enable
		0x10    0x80 // flag reset
		0x00    0x60 // memory write mode
		0x01    0x** // set memory type: 0x00 for X1bit 0x02 for X8bit

		0x02    0x** // start addr (low)
		0x03    0x** // start addr (high)
		0x04    0x** // stop addr (low)
		0x05    0x** // stop addr (high)
		0x0C    0x** // limit addr (low)
		0x0D    0x** // limit addr (high)

		0x08    0x** // ADPCM data
		0x10    0x1B // flag brdy reset
		0x10    0x13 // flag brdy/eos enable

		0x00    0x00 // end process
		0x10    0x80

	form Rebirth Sample Song 05

		// start process?
		slot:0x01 port:0x01 addr:0x00 data:0x01
		slot:0x01 port:0x01 addr:0x10 data:0x17
		slot:0x01 port:0x01 addr:0x10 data:0x80
		slot:0x01 port:0x01 addr:0x00 data:0x60
		slot:0x01 port:0x01 addr:0x01 data:0x02

		// set range
		slot:0x01 port:0x01 addr:0x0C data:0xFF
		slot:0x01 port:0x01 addr:0x0D data:0xFF
		slot:0x01 port:0x01 addr:0x02 data:0xFF
		slot:0x01 port:0x01 addr:0x03 data:0x1F
		slot:0x01 port:0x01 addr:0x04 data:0xFF
		slot:0x01 port:0x01 addr:0x05 data:0xFF

		// write data to ram
		slot:0x01 port:0x01 addr:0x08 data:0x00
		slot:0x01 port:0x01 addr:0x10 data:0x80

		// end process?
		slot:0x01 port:0x01 addr:0x10 data:0x00
		slot:0x01 port:0x01 addr:0x10 data:0x80
		slot:0x01 port:0x01 addr:0x00 data:0x01
*/

bool opna_adpcm_write(int serial_fd, FILE *input_fp, uint8_t type, uint32_t size)
{
	int count, adpcm_size = size - 8; /* size - sizeof(rom_size) (4byte) -  sizeof(start_addr) (4byte) */
	uint32_t rom_size, start_addr, stop_addr;
	uint8_t adpcm[adpcm_size];

	logging(DEBUG, "data block size:%lu\n", size);

	if (efread(&rom_size, 1, 4, input_fp) != 4
		|| efread(&start_addr, 1, 4, input_fp) != 4)
		return false;
	logging(DEBUG, "rom size:%lu start addr:0x%.8X adpcm size:%d\n", rom_size, start_addr, adpcm_size);

	if (efread(adpcm, 1, adpcm_size, input_fp) != (size_t) adpcm_size)
		return false;

	stop_addr = start_addr + adpcm_size;

	if (type != 0x81) {
		logging(WARN, "only support YM2608 DELTA-T ROM data (0x%.2X != 0x81)\n", type);
		return false;
	}

	/* debug: write ADPCM data to stdout */
	//fwrite(adpcm, 1, adpcm_size, stdout);

	/* sequence from YM2608 application manual */
	//memcpy(adpcm.src + start_addr, adp, adpcm_size);
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x10, 0x13);
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x10, 0x80);
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x00, 0x60);
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x01, 0x02);

	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x02, low_byte(start_addr));
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x03, high_byte(start_addr));

	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x04, low_byte(stop_addr));
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x05, high_byte(stop_addr));

	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x0C, low_byte(stop_addr));
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x0D, high_byte(stop_addr));

	count = 0;
	while (count < adpcm_size) {
		spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x08, adpcm[count]);
		spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x10, 0x1B);
		spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x10, 0x13);
		count++;
	}
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x00, 0x00);
	spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, 0x10, 0x80);

	return true;
}

bool vgm_play(int serial_fd, FILE *input_fp)
{
	uint8_t op, buf[BUFSIZE];
	uint16_t u16_tmp;
	uint32_t u32_tmp;
	int vgm_wait1, vgm_wait2;
	struct vgm_header_t header;

	if (vgm_parse_header(input_fp, &header) == false) {
		logging(ERROR, "vgm_parse_header() failed\n");
		return false;
	}

	if (header.YM2151_clock == 0 && header.YM2608_clock == 0) {
		logging(ERROR, "only support YM2151 and YM2608\n");
		return false;
	}

	vgm_wait1 = VGM_DEFAULT_WAIT1;
	vgm_wait2 = VGM_DEFAULT_WAIT2;

	//adpcm_rom_reset(serial_fd);

	while (catch_sigint == false) {
		if (efread(&op, 1, 1, input_fp) != 1)
			return false;

		logging(DEBUG, "op: 0x%.2X\n", op);

		switch (op) {
		case 0x54: /* YM2151 */
			if (efread(buf, 1, 2, input_fp) != 2)
				return false;
			spfm_send(serial_fd, OPM_SLOT_NUM, 0x00, buf[0], buf[1]);
			break;
		case 0x56: /* YM2608 normal */
		case 0x57: /* YM2608 extended */
			if (efread(buf, 1, 2, input_fp) != 2)
				return false;
			if (op == 0x56)
				spfm_send(serial_fd, OPNA_SLOT_NUM, 0x00, buf[0], buf[1]);
			else if (op == 0x57)
				spfm_send(serial_fd, OPNA_SLOT_NUM, 0x01, buf[0], buf[1]);
			break;
		case 0x61: /* vgm n wait */
			if (efread(&u16_tmp, 1, 2, input_fp) != 2)
				return false;
			vgm_wait(u16_tmp);
			break;
		case 0x62: /* vgm fixed wait1 */
			vgm_wait(vgm_wait1);
			break;
		case 0x63: /* vgm fixed wait2 */
			vgm_wait(vgm_wait2);
			break;
		case 0x64: /* vgm reset wait1/wait2 */
			if (efread(buf, 1, 1, input_fp) != 1
				|| efread(&u16_tmp, 1, 2, input_fp) != 2)
				return false;
			if (buf[0] == 0x62)
				vgm_wait1 = u16_tmp;
			else if (buf[0] == 0x63)
				vgm_wait2 = u16_tmp;
			break;
		case 0x66: /* end of vgm data */
			logging(DEBUG, "end of vgm data\n");
			return true;
		case 0x67: /* data block */
			if (efread(buf, 1, 2, input_fp) != 2
				|| efread(&u32_tmp, 1, 4, input_fp) != 4)
				return false;
			if (buf[0] != 0x66) {
				logging(ERROR, "invalid sequence: 0x67 0x%.2X (expected 0x67 0x66)\n", buf[0]);
				return false;
			}
			opna_adpcm_write(serial_fd, input_fp, buf[1], u32_tmp);
			break;
		default: /* vgm 1-16 wait */
			if (0x70 <= op && op <= 0x7F)
				vgm_wait((op & 0x0F) + 1);
			else
				logging(DEBUG, "unknown VGM command:0x%.2X\n", op);
			break;
		}
	}

	return true;
}
