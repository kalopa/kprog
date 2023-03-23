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

#include "kprog.h"

int		verbose = 0;
int		mem_size;

void		usage();

/*
 * It all kicks off, right here...
 */
int
main(int argc, char *argv[])
{
	int i;
	char *device;;

	device = "/dev/ttyS0:9600";
	mem_size = 32768;
	while ((i = getopt(argc, argv, "d:v")) != EOF) {
		switch (i) {
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
	printf("kprog - Kalopa Robotics AVR Programmer. v0.2\n");
	printf("Device: %s\n", device);
	printf("Memory: %d Bytes\n\n", mem_size);
	if ((argc - optind) != 1)
		usage();
	if (*device == '/')
		serial_open(device);
	else
		tcp_open(device);
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
	fprintf(stderr, "Usage: kprog [-v][-d DEVICE] program.hex\n");
	exit(2);
}
