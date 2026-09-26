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

#include "kprog.h"

#define OUTER_TIMEOUT		300
#define INNER_TIMEOUT		32
#define DRAIN_LIMIT		2000

/*
 * Device flash layout, as reported by the bootstrap code (or assumed,
 * for the older BOOTv2 protocol). :boot_start and :boot_count describe
 * the 128-byte blocks occupied by the bootstrap code itself, which we
 * must never try to reprogram.
 */
int		flash_size = V2_FLASH_SIZE;
int		boot_start = V2_BOOT_START;
int		boot_count = V2_BOOT_COUNT;

char		input[MAX_LINELEN];
int		offset;

/*
 * Send the right incantation to get the device into bootstrap mode.
 * Usually this is a ^E\ two character sequence, but it's not
 * straightforward. We might already be at the bootstrap prompt, in
 * which case the ^E is ignored and the backslash causes a reset. This
 * code is non-trivial because it is our only opportunity to synchronize
 * the two sides of the communications channel. It's not pretty.
 *
 * The bootstrap string can be overridden (-b) for devices which speak a
 * binary protocol, and where ^E\ would be a bad idea. In that case, if
 * we see a bootstrap prompt (or an error) instead of the sign-on banner,
 * we're already in the bootstrap code so send a backslash to restart it
 * and get the banner.
 *
 * Once we have the banner, we let things settle and then re-sync on a
 * fresh prompt, so that any stragglers (a second banner, or complaints
 * about the bootstrap string) don't get mistaken for the reply to our
 * first real command.
 *
 * The banner is "BOOTv2" for the original ATmega328P-only code. Newer
 * bootstraps send "BOOTv3 KK SS NN" where KK is the flash size in KB,
 * SS is the first 128-byte block occupied by the bootstrap code and NN
 * is the number of blocks it occupies (all in hex).
 */
void
bootstrap_mode()
{
	int i, j, ch, version, kbytes, sent_bs;
	char *bootmsg = "BOOTv";

	printf("Trying to get to Bootstrap mode...\n");
	/*
	 * Start by dumping any noise still left on the serial line. Send the
	 * bootstrap string first, so a chatty application shuts up.
	 */
	serial_write_buf(bootstr, bootlen);
	for (i = 0; i < DRAIN_LIMIT && serial_read_to(SETTLE_TIMEOUT) >= 0; i++)
		;
	/*
	 * Wait to get some sort of boot message...
	 */
	version = -1;
	for (i = 0; i < OUTER_TIMEOUT && version < 0; i++) {
		serial_write_buf(bootstr, bootlen);
		for (j = sent_bs = 0; j < INNER_TIMEOUT; j++) {
			if ((ch = serial_read()) == bootmsg[0])
				break;
			if ((ch == '@' || ch == '-') && !sent_bs) {
				/*
				 * Already in the bootstrap code - restart it.
				 */
				serial_send("\\");
				sent_bs = 1;
			}
		}
		if (j == INNER_TIMEOUT)
			continue;
		for (j = 1; bootmsg[j] != '\0'; j++) {
			if ((ch = serial_read()) != bootmsg[j])
				break;
		}
		if (bootmsg[j] != '\0')
			continue;
		if ((ch = serial_read()) >= '0' && ch <= '9')
			version = ch - '0';
	}
	if (version < 0) {
		fprintf(stderr, "kprog: bootstrap_mode: could not initialize device.\n");
		exit(1);
	}
	printf("Bootstrap code version %d.\n", version);
	switch (version) {
	case 2:
		flash_size = V2_FLASH_SIZE;
		boot_start = V2_BOOT_START;
		boot_count = V2_BOOT_COUNT;
		break;

	case 3:
		/*
		 * Read the rest of the banner line and decode the layout.
		 */
		for (offset = 0; offset < (MAX_LINELEN-2); offset++) {
			if ((ch = serial_read()) < 0 || ch == '\n' || ch == '\r' || ch == '@')
				break;
			input[offset] = ch;
		}
		input[offset] = '\0';
		if (sscanf(input, " %x %x %x", &kbytes, &boot_start, &boot_count) != 3) {
			fprintf(stderr, "kprog: bootstrap_mode: cannot parse banner \"%s\".\n", input);
			exit(1);
		}
		flash_size = kbytes * 1024;
		break;

	default:
		fprintf(stderr, "kprog: bootstrap_mode: unsupported bootstrap version: %d\n", version);
		exit(1);
	}
	if (flash_size <= 0 || flash_size > FLASH_SIZE || (flash_size % PAGE_SIZE) != 0 ||
	    boot_count <= 0 || boot_start < 0 ||
	    (boot_start + boot_count) > (flash_size / BLOCK_SIZE)) {
		fprintf(stderr, "kprog: bootstrap_mode: bad flash layout (%d bytes, boot @%02X+%d).\n",
					flash_size, boot_start, boot_count);
		exit(1);
	}
	printf("Flash: %dK, bootstrap code in blocks %02X to %02X.\n", flash_size / 1024,
					boot_start, boot_start + boot_count - 1);
	prompt_wait(NULL);
	/*
	 * Let the dust settle, then get a clean prompt.
	 */
	while (serial_read_to(SETTLE_TIMEOUT) >= 0)
		;
	serial_send("\r");
	prompt_wait(NULL);
}

/*
 * Wait for a prompt ('@') from the bootstrap code. Any intervening lines
 * are handed to the callback function, if there is one. A '-' means the
 * last command failed.
 */
int
prompt_wait(void (*func)(char *))
{
	int ch, rcode = 0;

	offset = 0;
	while ((ch = serial_read()) != -1) {
		if (ch == '+') {
			rcode = 1;
			continue;
		}
		if (ch == '-') {
			printf("FAIL!\n");
			exit(1);
		}
		if (ch == '@')
			break;
		if (func == NULL)
			continue;
		if (ch == '\n') {
			input[offset] = '\0';
			if (offset > 0)
				func(input);
			offset = 0;
			continue;
		}
		if (offset < (MAX_LINELEN-2) && ch != '\r')
			input[offset++] = ch;
	}
	if (ch == -1) {
		fprintf(stderr, "kprog: prompt_wait: cannot see a prompt.\n");
		exit(1);
	}
	return(rcode);
}

/*
 * Is this block part of the bootstrap code (or beyond the end of flash)?
 */
int
boot_block(int blkno)
{
	if (blkno >= (flash_size / BLOCK_SIZE))
		return(1);
	return(blkno >= boot_start && blkno < (boot_start + boot_count));
}

/*
 * Reprogram a block of code.
 */
void
reprogram_block(int blkno)
{
	int i, j;
	char cmdbuffer[64], *cp;
	unsigned char *memp;

	if (boot_block(blkno))
		return;
	printf("Re-programming block %02X...\n", blkno);
	/*
	 * Right - is there any chance this is an empty block?
	 */
	memp = &file_image[blkno * BLOCK_SIZE];
	for (i = 0; i < BLOCK_SIZE; i++)
		if ((int )*memp++ != 0xff)
			break;
	if (i == BLOCK_SIZE) {
		/*
		 * Easy! A page full of 0xff, just use the erase command.
		 */
		sprintf(cmdbuffer, "E%02X", blkno);
		serial_send(cmdbuffer);
		prompt_wait(NULL);
		return;
	}
	/*
	 * Start by filling the remote memory buffer with a block of data.
	 */
	memp = &file_image[blkno * BLOCK_SIZE];
	for (i = 0; i < (BLOCK_SIZE/16); i++) {
		cp = cmdbuffer;
		*cp++ = i + '0';
		for (j = 0; j < 16; j++) {
			sprintf(cp, "%02X.", (int )*memp++);
			cp += 3;
		}
		serial_send(cmdbuffer);
		prompt_wait(NULL);
	}
	/*
	 * Now send the program command. To do this, we first erase the
	 * block and then program it.
	 */
	sprintf(cmdbuffer, "E%02X", blkno);
	serial_send(cmdbuffer);
	prompt_wait(NULL);
	sprintf(cmdbuffer, "P%02X", blkno);
	serial_send(cmdbuffer);
	prompt_wait(NULL);
}
