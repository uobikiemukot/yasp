/* See LICENSE for licence details. */
enum play_misc_t {
	MAGIC_NUMBER_SIZE = 3,
};

enum filetype_t {
	FILETYPE_S98 = 0,
	FILETYPE_VGM,
	FILETYPE_UNKNOWN,
};

const char s98_header[] = {'S', '9', '8'};
const char vgm_header[] = {'V', 'g', 'm'};

const char *filetype2str[] = {
	[FILETYPE_S98]     = "S98",
	[FILETYPE_VGM]     = "VGM",
	[FILETYPE_UNKNOWN] = "unknown",
};

/* VGM/S98 common loader functions */
enum filetype_t check_filetype(FILE *fp)
{
	uint8_t header[MAGIC_NUMBER_SIZE];
	size_t size;

	if ((size = efread(header, 1, MAGIC_NUMBER_SIZE, fp)) != MAGIC_NUMBER_SIZE)
		return FILETYPE_UNKNOWN;

	efseek(fp, 0L, SEEK_SET);

	if (memcmp(header, s98_header, MAGIC_NUMBER_SIZE) == 0)
		return FILETYPE_S98;
	else if (memcmp(header, vgm_header, MAGIC_NUMBER_SIZE) == 0)
		return FILETYPE_VGM;
	else
		return FILETYPE_UNKNOWN;
}

bool play_file(int serial_fd, const char *path)
{
	FILE *input_fp;
	enum filetype_t type;

	if ((input_fp = efopen(path, "r")) == NULL)
		return false;
	
	type = check_filetype(input_fp);
	logging(DEBUG, "filetype:%s\n", filetype2str[type]);

	switch (type) {
	case FILETYPE_S98:
		s98_play(serial_fd, input_fp);
		break;
	case FILETYPE_VGM:
		vgm_play(serial_fd, input_fp);
		break;
	default:
		logging(ERROR, "unknown filetype\n");
		efclose(input_fp);
		return false;
	}

	efclose(input_fp);
	return true;
}
