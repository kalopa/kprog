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
#define FLASH_SIZE	32768
#define MAX_LINELEN	512

#define BLOCK_SIZE	128
#define BLOCK_COUNT	256
#define PAGE_SIZE	256
#define PAGE_COUNT	128

#define HIGHEST_BLOCK	0xfc

extern	int	verbose;
extern	int	serial_fd;
extern	char	file_image[];
extern	char	device_image[];

/*
 * Prototypes...
 */
void		bootstrap_mode();
int		prompt_wait(void (*)(char *));
void		intel_load(char *);
int		get_hex_bytes(char *, int);
void serial_open(char *, int);
void		serial_send(char *);
int		serial_read();
void		serial_write(int);
void		memory_init();
void		device_load();
void		reprogram_block(int);
void		image_compare();
void		hexdump(char *, int);
