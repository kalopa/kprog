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

unsigned char	file_image[FLASH_SIZE];
unsigned char	device_image[FLASH_SIZE];

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
 * Make sure the HEX file image fits inside the device flash. This can
 * only be done once we know the device layout, which is after we've
 * talked to the bootstrap code.
 */
void
image_check()
{
	int i;

	for (i = flash_size; i < FLASH_SIZE; i++) {
		if (file_image[i] != 0xff) {
			fprintf(stderr, "kprog: HEX file has data at %04x, beyond the end of the %dK device flash.\n",
						i, flash_size / 1024);
			exit(1);
		}
	}
}

/*
 * Compare two images...
 */
void
image_compare()
{
	int i, j, same, nblocks = 0, nskipped = 0;
	unsigned char *ap, *bp;

	/*
	 * Check each of the images, a block at a time.
	 */
	printf("Checking block differences.\n");
	for (i = 0; i < flash_size; i += BLOCK_SIZE) {
		same = 1;
		ap = &file_image[i];
		bp = &device_image[i];
		for (j = 0; j < BLOCK_SIZE; j++) {
			if (*ap++ != *bp++) {
				same = 0;
				break;
			}
		}
		if (same)
			continue;
		if (boot_block(i / BLOCK_SIZE)) {
			nskipped++;
			continue;
		}
		reprogram_block(i / BLOCK_SIZE);
		nblocks++;
	}
	if (nskipped > 0)
		printf("Skipped %d block(s) which differ in the bootstrap area (cannot be reprogrammed).\n", nskipped);
	if (nblocks == 0)
		printf("Device is already up to date.\n");
	else
		printf("Reprogrammed %d block(s).\n", nblocks);
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
	for (i = 0; i < (flash_size / PAGE_SIZE); i++) {
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
		if (*linep == '\0')
			break;
		if (addr >= FLASH_SIZE) {
			fprintf(stderr, "kprog: device dump address %04x out of range.\n", addr);
			exit(1);
		}
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
