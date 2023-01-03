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

#include "avrprog.h"

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
				fprintf(stderr, "avrprog: bootstrap_mode: incorrect version: %d\n", ch);
				exit(1);
			}
			printf("Bootstrap code version %d.\n", ch);
			break;
		}
	}
	if (i == OUTER_TIMEOUT) {
		fprintf(stderr, "avrprog: bootstrap_mode: could not initialize device.\n");
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
		fprintf(stderr, "avrprog: prompt_wait: cannot see a prompt.\n");
		exit(1);
	}
	return(rcode);
}
