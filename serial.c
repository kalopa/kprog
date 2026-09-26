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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#include "kprog.h"

/*
 * Table to convert baud rate to termios setting.
 */
struct  speed   {
	int	value;
	int	code;
} speeds[] = {
	{50, B50},
	{75, B75},
	{110, B110},
	{134, B134},
	{150, B150},
	{200, B200},
	{300, B300},
	{600, B600},
	{1200, B1200},
	{1800, B1800},
	{2400, B2400},
	{4800, B4800},
	{9600, B9600},
	{19200, B19200},
	{38400, B38400},
	{57600, B57600},
	{115200, B115200},
	{230400, B230400},
	{0, 0}
};

/*
 * File descriptor for the device - either a tty or a TCP socket. The
 * read/write functions below work with either.
 */
int		serial_fd = -1;

/*
 * Open the serial port to talk to the AVR device.
 */
void
serial_open(char *device)
{
	int i, speed = 9600;
	char *cp;
	struct termios tios;

	if ((cp = strchr(device, ':')) != NULL) {
		*cp++ = '\0';
		speed = atoi(cp);
	}
	if ((serial_fd = open(device, O_RDWR|O_NOCTTY)) < 0) {
		fprintf(stderr, "?kprog - cannot open serial device: ");
		perror(device);
		exit(1);
	}
	/*
	* Get and set the tty parameters.
	*/
	if (tcgetattr(serial_fd, &tios) < 0) {
		perror("serial_master: tcgetattr failed");
		exit(1);
	}
	for (i = 0; speeds[i].value > 0; i++)
		if (speeds[i].value == speed)
			break;
	if (speeds[i].value == 0) {
		fprintf(stderr, "?kprog - invalid serial baud rate: %d\n", speed);
		exit(1);
	}
	cfsetispeed(&tios, speeds[i].code);
	cfsetospeed(&tios, speeds[i].code);
	tios.c_cflag &= ~(CSIZE|PARENB|CSTOPB);
	tios.c_cflag |= (CLOCAL|CREAD|CS8);
	tios.c_lflag = tios.c_iflag = tios.c_oflag = 0;
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = READ_TIMEOUT / 100;
	if (tcsetattr(serial_fd, TCSANOW, &tios) < 0) {
		perror("?kprog - tcsetattr failed");
		exit(1);
	}
	printf("Serial device %s at %d baud.\n", device, speed);
}

/*
 * Send a string of characters to the device. Written as a single block
 * so that a command travels in one TCP segment rather than dribbling
 * out a byte at a time.
 */
void
serial_send(char *strp)
{
	serial_write_buf((unsigned char *)strp, strlen(strp));
}

/*
 * Send an arbitrary block of bytes (which may include NULs) to the device.
 */
void
serial_write_buf(unsigned char *bufp, int len)
{
	int n;

	while (len > 0) {
		if ((n = write(serial_fd, bufp, len)) < 0) {
			if (errno == EINTR)
				continue;
			perror("kprog: serial_send");
			exit(1);
		}
		bufp += n;
		len -= n;
	}
}

/*
 * Read a single byte from the device. Returns -1 if nothing arrives
 * within READ_TIMEOUT milliseconds. A closed TCP connection is fatal.
 */
int
serial_read()
{
	return(serial_read_to(READ_TIMEOUT));
}

/*
 * As above, but with an explicit timeout (in milliseconds).
 */
int
serial_read_to(int timeout)
{
	int n;
	unsigned char buffer[2];
	struct pollfd pfd;

	pfd.fd = serial_fd;
	pfd.events = POLLIN;
	while ((n = poll(&pfd, 1, timeout)) < 0) {
		if (errno == EINTR)
			continue;
		perror("kprog: serial_read (poll)");
		exit(1);
	}
	if (n == 0)
		return(-1);
	if ((n = read(serial_fd, buffer, 1)) < 0) {
		perror("kprog: serial_read");
		exit(1);
	}
	if (n == 0) {
		fprintf(stderr, "kprog: serial_read: connection closed by remote end.\n");
		exit(1);
	}
	if (verbose)
		fputc(buffer[0], stdout);
	return(buffer[0]);
}

/*
 * Write a single character to the device.
 */
void
serial_write(int ch)
{
	char buffer[2];

	buffer[0] = ch;
	buffer[1] = '\0';
	serial_send(buffer);
}
