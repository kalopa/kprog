/*
 * Copyright (c) 2021-23, Kalopa Robotics Limited.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
