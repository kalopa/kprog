/*
 * Copyright (C) 2021, Kalopa Robotics Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "kprog.h"

int		intel_parse_line(char *);

/*
 * Read an Intel HEX file into memory
 */
void
intel_load(char *hexfile)
{
	char input[MAX_LINELEN+2], *cp;
	FILE *fp;

	printf("Reading HEX file \"%s\" to local image.\n", hexfile);
	if ((fp = fopen(hexfile, "r")) == NULL) {
		perror(hexfile);
		exit(1);
	}
	while (fgets(input, MAX_LINELEN, fp) != NULL) {
		if ((cp = strpbrk(input, "\r\n")) != NULL)
			*cp = '\0';
		if (intel_parse_line(input) == 01)
			break;
	}
	fclose(fp);
}

/*
 * Parse a single line of an Intel HEX file, returning the last address
 * seen.
 */
int
intel_parse_line(char *linep)
{
	int i, len, addr, type, csum, data;

	if (*linep++ != ':') {
		fprintf(stderr, "kprog: bad Intel HEX format in file - missing header.\n");
		exit(1);
	}
	csum = len = get_hex_bytes(linep, 2);
	linep += 2;
	addr = get_hex_bytes(linep, 4);
	linep += 4;
	csum += (addr >> 8) & 0xff;
	csum += (addr & 0xff);
	type = get_hex_bytes(linep, 2);
	linep += 2;
	csum += type;
	for (i = 0; i < len; i++) {
		data = get_hex_bytes(linep, 2);
		csum += data;
		file_image[addr++] = data;
		linep += 2;
	}
	csum += get_hex_bytes(linep, 2);
	csum &= 0xff;
	if ((csum & 0xff) != 0x00) {
		fprintf(stderr, "kprog: invalid checksum in Intel HEX file.\n");
		exit(1);
	}
	return(type);
}

/*
 * Read 'len' HEX characters from the given line.
 */
int
get_hex_bytes(char *cp, int len)
{
	int ch, value = 0;

	while (len-- > 0) {
		ch = *cp++;
		if (!isxdigit(ch)) {
			fprintf(stderr, "kprog: Bad hexadecimal data in file.\n");
			exit(1);
		}
		if (ch >= '0' && ch <= '9') {
			ch -= '0';
		} else if (ch >= 'A' && ch <= 'F') {
			ch = ch - 'A' + 10;
		} else
			ch = ch - 'a' + 10;
		value <<= 4;
		value += ch;
	}
	return(value);
}
