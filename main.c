/*
 * Copyright (c) 2021-26, Kalopa Robotics Limited.  All rights reserved.
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

int		verbose = 0;
unsigned char	bootstr[MAX_BOOTSTR];
int		bootlen;

void		usage();
int		unescape(unsigned char *, char *, int);

/*
 * It all kicks off, right here...
 */
int
main(int argc, char *argv[])
{
	int i;
	char *device;

	device = "/dev/ttyS0:9600";
	bootlen = unescape(bootstr, "\\005\\\\", MAX_BOOTSTR);
	while ((i = getopt(argc, argv, "b:d:v")) != EOF) {
		switch (i) {
		case 'b':
			bootlen = unescape(bootstr, optarg, MAX_BOOTSTR);
			break;

		case 'd':
			device = optarg;
			break;

		case 'v':
			verbose = 1;
			break;

		default:
			usage();
			break;
		}
	}
	printf("kprog - Kalopa Robotics AVR Programmer. v0.3\n");
	printf("Device: %s\n\n", device);
	if ((argc - optind) != 1)
		usage();
	/*
	 * Initialize both memory buffers, then load the HEX file. Do this
	 * before we open the device so that a bad file doesn't leave the
	 * remote system sitting in bootstrap mode.
	 */
	memory_init();
	intel_load(argv[optind]);
	/*
	 * A device name starting with a slash is a local serial port
	 * (optionally with a baud rate, e.g. /dev/ttyUSB0:9600). Anything
	 * else is a TCP host:port pair.
	 */
	if (*device == '/')
		serial_open(device);
	else
		tcp_open(device);
	/*
	 * Sync the remote device so we're at a command prompt in the
	 * bootstrap code.
	 */
	bootstrap_mode();
	/*
	 * Make sure the image will fit in the device.
	 */
	image_check();
	/*
	 * Load the device flash image.
	 */
	device_load();
	/*
	 * Compare images, and reprogram whatever has changed.
	 */
	image_compare();
	exit(0);
}

/*
 * Convert a string with C-style escapes (\\, \n, \r, \t, \e, \xHH and
 * \ooo) into the raw bytes we send to the device to get into bootstrap
 * mode. This makes it possible to specify a binary packet on the command
 * line, for devices which don't speak plain ASCII. Returns the length, as
 * the result may well contain NUL bytes.
 */
int
unescape(unsigned char *dst, char *src, int maxlen)
{
	int ch, i, val;
	unsigned char *start = dst, *end = dst + maxlen;

	while ((ch = *src++) != '\0' && dst < end) {
		if (ch != '\\') {
			*dst++ = ch;
			continue;
		}
		switch (ch = *src++) {
		case 'n':	ch = '\n'; break;
		case 'r':	ch = '\r'; break;
		case 't':	ch = '\t'; break;
		case 'e':	ch = 033; break;
		case '\\':	break;
		case 'x':
			for (val = i = 0; i < 2 && isxdigit((unsigned char )*src); i++) {
				ch = *src++;
				val = (val << 4) + (isdigit(ch) ? ch - '0' : (tolower(ch) - 'a' + 10));
			}
			if (i == 0) {
				fprintf(stderr, "kprog: bad \\x escape in bootstrap string.\n");
				exit(2);
			}
			ch = val;
			break;
		case '\0':
			fprintf(stderr, "kprog: trailing backslash in bootstrap string.\n");
			exit(2);
		default:
			if (ch < '0' || ch > '7') {
				fprintf(stderr, "kprog: bad escape '\\%c' in bootstrap string.\n", ch);
				exit(2);
			}
			val = ch - '0';
			for (i = 1; i < 3 && *src >= '0' && *src <= '7'; i++)
				val = (val << 3) + (*src++ - '0');
			ch = val;
			break;
		}
		*dst++ = ch;
	}
	if (ch != '\0') {
		fprintf(stderr, "kprog: bootstrap string too long (max %d bytes).\n", maxlen);
		exit(2);
	}
	if (dst == start) {
		fprintf(stderr, "kprog: empty bootstrap string.\n");
		exit(2);
	}
	return(dst - start);
}

/*
 * Print a usage message and exit.
 */
void
usage()
{
	fprintf(stderr, "Usage: kprog [-v][-d DEVICE][-b BOOTSTR] program.hex\n");
	fprintf(stderr, "\tDEVICE is /dev/ttyXX[:baud] or host:port (default: /dev/ttyS0:9600)\n");
	fprintf(stderr, "\tBOOTSTR is the sequence sent to enter bootstrap mode (default: \\005\\\\)\n");
	exit(2);
}
