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
#include <fcntl.h>
#include <termios.h>
#include <syslog.h>
#include <errno.h>

#include "avrprog.h"

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
 * Open the serial port to talk to the AVR device.
 */
int
serial_open(char *device, int baud)
{
	int i, fd;
	struct termios tios;

	/*
	 * Try and open the device.
	 */
	if ((fd = open(device, O_RDWR|O_NOCTTY)) < 0) {
		perror(device);
		exit(1);
	}
	/*
	 * Get the current tty parameters.
	 */
	if (tcgetattr(fd, &tios) < 0) {
		perror("avrprog: serial_open: tcgetattr");
		exit(1);
	}
	for (i = 0; speeds[i].value > 0; i++)
		if (speeds[i].value == baud)
			break;
	if (speeds[i].value == 0) {
		fprintf(stderr, "avrprog: invalid serial baud rate: %d\n", baud);
		exit(1);
	}
	cfsetispeed(&tios, speeds[i].code);
	cfsetospeed(&tios, speeds[i].code);
	tios.c_cflag &= ~(CSIZE|PARENB|CSTOPB);
	tios.c_cflag |= (CLOCAL|CREAD|CS8);
	tios.c_lflag = tios.c_iflag = tios.c_oflag = 0;
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 40;
	if (tcsetattr(fd, TCSANOW, &tios) < 0) {
		perror("avrprog: serial_open: tcsetattr");
		exit(1);
	}
	return(fd);
}

/*
 * Send a string of characters to the serial port.
 */
void
serial_send(int fd, char *strp)
{
	int ch;

	while ((ch = *strp++) != '\0')
		serial_write(fd, ch);
}

/*
 * Read a single byte from the serial port.
 */
int
serial_read(int fd)
{
	int n;
	char buffer[2];

	if ((n = read(fd, buffer, 1)) < 0) {
		perror("avrprog: serial_read");
		exit(1);
	}
	if (n == 0)
		return(-1);
	return(buffer[0]);
}

/*
 * Write a single character to the serial port.
 */
void
serial_write(int fd, int ch)
{
	char buffer[2];

	buffer[0] = ch;
	if (write(fd, buffer, 1) < 0) {
		perror("avrprog: serial_write");
		exit(1);
	}
}
