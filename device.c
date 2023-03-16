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

#include "kprog.h"

#define VERSION			2

#define OUTER_TIMEOUT		300
#define INNER_TIMEOUT		8

char		input[MAX_LINELEN];
int		offset;

/*
 * Send the right incantation to get the device into bootstrap
 * mode. Usually this is a ^E\ two character sequence, but it's not
 * straightforward. We might already be at the bootstrap prompt, in
 * which case the ^E is ignored and the backslash causes a reset. This
 * code is non-trivial because it is our only opportunity to synchronize
 * the two sides of the communications channel. It's not pretty.
 */
void
bootstrap_mode()
{
	int i, j, ch;
	char *bootmsg = "BOOTv";

	printf("Trying to get to Bootstrap mode...\n");
	/*
	 * Start by dumping any noise still left on the serial line.
	 */
	while (serial_read() >= 0)
		;
	/*
	 * Wait to get some sort of boot message...
	 */
	for (i = 0; i < OUTER_TIMEOUT; i++) {
		serial_send("\005\\");
		for (j = 0; j < INNER_TIMEOUT; j++) {
			if ((ch = serial_read()) == bootmsg[0])
				break;
		}
		if (j == INNER_TIMEOUT)
			continue;
		j = 1;
		while ((ch = serial_read()) != -1 && bootmsg[j] != '\0') {
			if (bootmsg[j++] != ch)
				break;
		}
		if (bootmsg[j] == '\0') {
			ch -= '0';
			if (ch != VERSION) {
				fprintf(stderr, "kprog: bootstrap_mode: incorrect version: %d\n", ch);
				exit(1);
			}
			printf("Bootstrap code version %d.\n", ch);
			break;
		}
	}
	if (i == OUTER_TIMEOUT) {
		fprintf(stderr, "kprog: bootstrap_mode: could not initialize device.\n");
		exit(1);
	}
	prompt_wait(NULL);
}

/*
 *
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
 * Reprogram a block of code.
 */
void
reprogram_block(int blkno)
{
	int i, j;
	char cmdbuffer[64], *cp, *memp;

	if (blkno >= HIGHEST_BLOCK)
		return;
	printf("Re-programming block %d...\n", blkno);
	/*
	 * Right - is there any chance this is an empty block?
	 */
	memp = &file_image[blkno * BLOCK_SIZE];
	for (i = 0; i < BLOCK_SIZE; i++)
		if (*memp++ != 0xff)
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
	verbose = 2;
	for (i = 0; i < (BLOCK_SIZE/16); i++) {
		cp = cmdbuffer;
		*cp++ = i + '0';
		for (j = 0; j < 16; j++) {
			sprintf(cp, "%02X.", *memp++);
			cp += 3;
		}
		serial_send(cmdbuffer);
		prompt_wait(NULL);
	}
	/*
	 * Now send the program command. To do this, we first erase the
	 * block and then program it.
	 */
	printf("P%02x.", blkno);
	sprintf(cmdbuffer, "E%02X", blkno);
	serial_send(cmdbuffer);
	prompt_wait(NULL);
	sprintf(cmdbuffer, "P%02X", blkno);
	serial_send(cmdbuffer);
	prompt_wait(NULL);
}
