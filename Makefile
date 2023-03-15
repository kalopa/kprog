#
# Copyright (C) 2021, Kalopa Robotics Limited. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
CFLAGS=	-Wall -O

SRCS=	main.c device.c intel.c serial.c memory.c
OBJS=	$(SRCS:.c=.o)
BIN=	kprog

all:	$(BIN)

install: $(BIN)

clean:
	rm -f $(OBJS) $(BIN)

$(BIN):	$(OBJS)
	$(CC) -o $(BIN) $(OBJS)

$(OBJS): kprog.h
