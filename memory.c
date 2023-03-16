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

char		file_image[FLASH_SIZE];
char		device_image[FLASH_SIZE];

void		mem_callback(char *);

/*
 * Initialize memory images
 */
void
memory_init()
{
	memset((void *)file_image, 0xff, FLASH_SIZE);
	memset((void *)device_image, 0xff, FLASH_SIZE);
}

/*
 * Compare two images...
 */
void
image_compare()
{
	int i, j, same;
	char *ap, *bp;

	/*
	 * Check each of the images, a page at a time.
	 */
	printf("Checking page differences.\n");
	for (i = 0; i < FLASH_SIZE; i += BLOCK_SIZE) {
		same = 1;
		ap = &file_image[i];
		bp = &device_image[i];
		for (j = 0; j < BLOCK_SIZE; j++) {
			if (*ap++ != *bp++) {
				same = 0;
				break;
			}
		}
		if (!same)
			reprogram_block(i >> 7);
	}
}

/*
 * Load the local image memory from the device.
 */
void
device_load()
{
	int i;
	char cmdbuffer[8];

	printf("Load flash image into local memory.\n");
	for (i = 0; i < PAGE_COUNT; i++) {
		sprintf(cmdbuffer, "D%02X", i);
		serial_send(cmdbuffer);
		prompt_wait(mem_callback);
		putchar('.');
		fflush(stdout);
	}
	putchar('\n');
}

/*
 * Callback from serial code after a memory dump command.
 * P0050 0C 94 34 00 0C 94 34 00 0C 94 34 00 0C 94 34 00
 */
void
mem_callback(char *linep)
{
	int addr;

	if (*linep++ != 'P')
		return;
	addr = get_hex_bytes(linep, 4);
	linep += 4;
	while (*linep != '\0') {
		while (isspace(*linep))
			linep++;
		device_image[addr++] = get_hex_bytes(linep, 2);
		linep += 2;
	}
}

/*
 * Hex dump of image memory.
 */
void
hexdump(char *imagep, int size)
{
	int i, j, k, n, same, didstars = 0;
	char lastline[16];

	for (i = 0; size > 0;) {
		if ((n = size) > 16)
			n = 16;
		same = 0;
		if (i > 0 && size > 16) {
			same = 1;
			for (j = 0; j < 16; j++) {
				if (imagep[j] != lastline[j]) {
					same = 0;
					break;
				}
			}
		}
		memcpy(lastline, imagep, n);
		if (same) {
			if (!didstars)
				printf("      *\n");
			didstars = 1;
		} else {
			didstars = 0;
			printf("%04x ", i);
			for (j = 0; j < n; j++) {
				if (j == 8)
					putchar(' ');
				printf(" %02x", imagep[j]);
			}
			printf("  *");
			for (j = 0; j < n; j++) {
				if ((k = imagep[j] & 0x7f) < 0x20 || k > 0x7e)
					k = '.';
				putchar(k);
			}
			printf("*\n");
		}
		imagep += n;
		i += n;
		size -= n;
	}
}
