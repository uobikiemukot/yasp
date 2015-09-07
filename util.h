/* See LICENSE for licence details. */
int read_2byte_le(FILE *fp, uint16_t *value)
{
	int count = 0;
	uint8_t byte;

	*value = 0;

	while (count < 2) {
		if (fread(&byte, 1, 1, fp) != 1) {
			logging(ERROR, "read_2byte_le(): fread() failed\n");
			return -1;
		}

		*value |= byte << (BITS_PER_BYTE * count);
		count++;
	}
	return count;
}

int read_4byte_le(FILE *fp, uint32_t *value)
{
	int count = 0;
	uint8_t byte;

	*value = 0;

	while (count < 4) {
		if (fread(&byte, 1, 1, fp) != 1) {
			logging(ERROR, "read_4byte_le(): fread() failed\n");
			return -1;
		}

		*value |= byte << (BITS_PER_BYTE * count);
		count++;
	}
	return count;
}

uint64_t read_variable_length_7bit_le(FILE *fp)
{
	char byte;
	int count = 0;
	uint64_t nsync = 0L;

	do {
		if (fread(&byte, 1, 1, fp) != 1)
			return -1;

		nsync |= (byte & 0x7F) << (7 * count);
		count++;
	} while (byte & 0x80);

	return nsync;
}

bool read_string(FILE *fp, char *str, int size)
{
	char *cp;

	cp = str;

	while (1) {
		*cp = getc(fp);

		if (cp - str > size) {
			logging(ERROR, "str buffer overflow\n");
			return false;
		}

		if (*cp == 0x0A || *cp == 0x00) {
			if (*cp == 0x0A)
				*cp = '\0';

			if (cp - str > 0)
				return true;
			else /* string length == 0, no more str */
				return false;
		}

		cp++;
	}
}

uint8_t low_byte(uint32_t value)
{
	return (value & 0xFF);
}

uint8_t high_byte(uint32_t value)
{
	return ((value >> 8) & 0xFF);
}
