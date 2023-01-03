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

#include "avrprog.h"

int		verbose = 0;

void		usage();

/*
 * It all kicks off, right here...
 */
int
main(int argc, char *argv[])
{
	int i, speed;
	char *device;;

	optind = opterr = 0;
	speed = 9600;
	device = "/dev/ttyS0";
	while ((i = getopt(argc, argv, "s:l:v")) != EOF) {
		switch (i) {
		case 's':
			if ((speed = atoi(optarg)) < 50)
				usage();
			break;

		case 'l':
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
	if ((argc - optind) != 1)
		usage();
	serial_open(device, speed);
	/*
	 * Initialize both memory buffers, then load the HEX file.
	 */
	memory_init();
	intel_load(argv[optind]);
	/*
	 * Sync the remote device so we're at a command prompt in the
	 * bootstrap code.
	 */
	bootstrap_mode();
	/*
	 * Load the device flash image.
	 */
	device_load();
	/*
	 * Compare images.
	 */
	image_compare();
}

/*
 * Print a usage message and exit.
 */
void
usage()
{
	fprintf(stderr, "Usage: avrprog [-v][-s SPEED][-l LINE] program.hex\n");
	exit(2);
}
