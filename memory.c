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

#include "avrprog.h"

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
	int i, j, blkno, same;
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
		if (!same) {
			blkno = i >> 7;
			printf("Page %d is different.\n", blkno);
			if (blkno < HIGHEST_BLOCK)
				reprogram_block(blkno);
		}
	}
}

/*
 * Load the local image memory from the device.
 */
void
device_load(int fd)
{
	int i;
	char cmdbuffer[8];

	printf("Load flash image into local memory.\n");
	for (i = 0; i < 128; i++) {
		sprintf(cmdbuffer, "D%02X", i);
		serial_send(fd, cmdbuffer);
		prompt_wait(fd, mem_callback);
		putchar('.');
		fflush(stdout);
	}
	putchar('\n');
	hexdump(device_image, FLASH_SIZE);
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
