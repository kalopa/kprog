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
/*
 * FLASH_SIZE is the largest device we support (the size of the local
 * image buffers). The actual device flash size, and the location of the
 * bootstrap code within it, are learned from the bootstrap banner when
 * we connect to the device.
 */
#define FLASH_SIZE	32768
#define MAX_LINELEN	512

#define BLOCK_SIZE	128
#define BLOCK_COUNT	(FLASH_SIZE / BLOCK_SIZE)
#define PAGE_SIZE	256
#define PAGE_COUNT	(FLASH_SIZE / PAGE_SIZE)

/*
 * Legacy (BOOTv2) layout: an ATmega328P with the bootstrap code in the
 * top 512 bytes of the 32K flash.
 */
#define V2_FLASH_SIZE	32768
#define V2_BOOT_START	0xfc
#define V2_BOOT_COUNT	4

#define MAX_BOOTSTR	64
#define READ_TIMEOUT	4000		/* milliseconds */
#define SETTLE_TIMEOUT	500		/* milliseconds */

extern	int		verbose;
extern	int		serial_fd;
extern	int		flash_size;
extern	int		boot_start;
extern	int		boot_count;
extern	unsigned char	bootstr[];
extern	int		bootlen;
extern	unsigned char	file_image[];
extern	unsigned char	device_image[];

/*
 * Prototypes...
 */
void		bootstrap_mode();
int		prompt_wait(void (*)(char *));
int		boot_block(int);
void		intel_load(char *);
int		get_hex_bytes(char *, int);
void		serial_open(char *);
void		serial_send(char *);
void		serial_write_buf(unsigned char *, int);
int		serial_read();
int		serial_read_to(int);
void		serial_write(int);
void		tcp_open(char *);
void		memory_init();
void		device_load();
void		reprogram_block(int);
void		image_compare();
void		image_check();
void		hexdump(char *, int);
