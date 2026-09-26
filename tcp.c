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
 *
 * ABSTRACT
 * Connect to an AVR device which sits behind a serial concentrator
 * (or a ser2net/socat style bridge) using a raw TCP connection. The
 * device is specified as "host:port" (an IPv6 literal can be given
 * as "[addr]:port"). Once connected, the socket descriptor is used
 * by the same serial_read/serial_write routines as the tty code, so
 * the rest of kprog neither knows nor cares which transport is in use.
 *
 * Note that the concentrator port is expected to be in "raw" mode,
 * without any telnet option negotiation (RFC2217 is not supported).
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "kprog.h"

#define DEFAULT_PORT	"5000"

/*
 * Open a TCP connection to the remote host/port.
 */
void
tcp_open(char *device)
{
	int fd, err, one = 1;
	char *host, *port, *cp;
	struct addrinfo hints, *res, *rp;

	host = strdup(device);
	port = DEFAULT_PORT;
	if (*host == '[') {
		/*
		 * IPv6 literal, [addr]:port
		 */
		host++;
		if ((cp = strchr(host, ']')) == NULL) {
			fprintf(stderr, "?kprog - bad IPv6 address: %s\n", device);
			exit(1);
		}
		*cp++ = '\0';
		if (*cp == ':')
			port = cp + 1;
	} else if ((cp = strrchr(host, ':')) != NULL) {
		*cp++ = '\0';
		port = cp;
	}
	if (*port == '\0' || atoi(port) <= 0 || atoi(port) > 65535) {
		fprintf(stderr, "?kprog - invalid TCP port: %s\n", port);
		exit(1);
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((err = getaddrinfo(host, port, &hints, &res)) != 0) {
		fprintf(stderr, "?kprog - cannot resolve %s: %s\n", host, gai_strerror(err));
		exit(1);
	}
	fd = -1;
	for (rp = res; rp != NULL; rp = rp->ai_next) {
		if ((fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0)
			continue;
		if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;
		close(fd);
		fd = -1;
	}
	freeaddrinfo(res);
	if (fd < 0) {
		fprintf(stderr, "?kprog - cannot connect to %s port %s: ", host, port);
		perror("");
		exit(1);
	}
	/*
	 * The bootstrap protocol is a strict command/response affair, so
	 * make sure each command goes out on the wire immediately.
	 */
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0)
		perror("kprog: warning - TCP_NODELAY");
	printf("Connected to %s, TCP port %s.\n", host, port);
	serial_fd = fd;
}
