# kprog - Kalopa Robotics Programmer

Programmer code for AVR chip using the
[libavr](https://github.com/kalopa/libavr)
bootstrap module.
This module is designed to work with the
[libavr/bootstrap.S](https://github.com/kalopa/libavr/blob/master/bootstrap.S)
code to program an AVR chip, in-situ.
The **libavr** bootstrap code is designed to run in a measly 512
bytes of code space at the top of AVR memory, and provide the
bare minimum of programming functionality.
This code complements that library function and communicates
with it to re-program the chip.
Note that it cannot (obviously) update the bootstrap code,
you'll need an In-Circuit programmer to do that.

The trick here is to get to the Bootstrap code.
The general arrangement, and with a hat-tip to history,
is to send a ^E\ two-character sequence to the running
firmware, which tells it to go into bootstrap mode by
jumping to the `_bootstrap` entry point.

## Usage

    kprog [-v] [-d DEVICE] [-b BOOTSTR] program.hex

The device can be a local serial port, optionally with a baud rate
(`-d /dev/ttyUSB0:9600`, the default is `/dev/ttyS0:9600`), or a
TCP host and port (`-d serial.kalopa.com:5016`) for a board which
sits behind a serial concentrator or a `ser2net`-style bridge.
The TCP port must be in raw mode (no telnet option negotiation).
An IPv6 address can be given as `[addr]:port`.

`-b` overrides the sequence used to kick the running firmware into
bootstrap mode.
It takes C-style escapes (`\\`, `\n`, `\r`, `\t`, `\e`, `\xHH` and
`\ooo`), so a binary packet can be given for devices which don't
speak plain ASCII.
The default is `\005\\` (^E\).
If **kprog** sees a bootstrap prompt rather than the sign-on banner,
it assumes the device is already in bootstrap mode and sends a
backslash to restart it, so it is safe to run against a device in
either state.

`-v` echoes everything received from the device.

**kprog** reads the whole flash from the device, compares it with the
HEX file, and only erases/programs the 128-byte blocks which differ.
Blocks occupied by the bootstrap code itself are never touched (and
are reported if they differ).
Intel HEX records outside program flash (EEPROM, fuses) are ignored.

## Supported devices

The bootstrap code announces itself with a banner, and **kprog** uses
that to work out the flash layout:

| Banner | Meaning |
| ------ | ------- |
| `BOOTv2` | Original code: ATmega328P, 32K flash, bootstrap in blocks FC-FF (top 512 bytes) |
| `BOOTv3 KK SS NN` | KK = flash size in KB, SS = first bootstrap block, NN = number of bootstrap blocks (hex) |

So an ATmega328P built from the current **libavr** reports
`BOOTv3 20 FC 04`, and an ATtiny1626 reports `BOOTv3 10 00 04`.
On the tinyAVR 0/1/2-series parts the bootstrap lives in the BOOT
section at the *bottom* of flash (FUSE.BOOTEND=2, 512 bytes) and the
application is linked to start at 0x0200.
See the comments in
[libavr/bootstrap.S](https://github.com/kalopa/libavr/blob/master/bootstrap.S)
for the details, and `avr.mk` in the same place for the linker flags.

The real end-game here is to get this functionality folded into
[avrdude](https://github.com/avrdudes/avrdude)
but I needed something working immediately, for further testing
and proving that the bootstrap.S code actually works.

## Notes from bootstrap.S

Using the serial port, bootstrap new firmware. For the ATMega328, the
application firmware runs from 0x0000 to 0x3eff and the boot loader
runs from 0x3f00 to 0x3fff (word addressing). This allows for 256
words (512 bytes) of bootstrap instructions, or four pages.

BOOTSZ1=1, BOOTSZ0=1.

The bootstrap subprogram is invoked by calling \_bootstrap from the main
code.
It will reconfigure the serial port and runs without interrupts.
The serial port will now run at 9600 baud (slower for better reliability).

The serial commands are as follows:

| Command | Description |
| :-------: | ----------- |
| 0-7xx | Upload 16 bytes of program data to memory |
| Dnn | Dump page 'nn' of program flash |
| Ebb | Erase a block (bb) of flash memory |
| M | Dump the memory buffer (ATmega only) |
| Pbb | Program a block (bb) of flash memory |
| R | Reset the system (jump to zero) |

The argument to the dump command is the upper byte of the address.
So **D7F** will dump from *7f00* through *7fff*.
The argument to the Erase and Program commands is a block number in the
range 0x00 to 0xFF, so they work on blocks of 128 bytes.
As the upload command only accepts 16 bytes, you need 8 of them to upload
a complete block into memory.

Success is indicated by a '+' and an error by a '-'.
As it is sometimes possible to get lost in a command, there is a
sync character '!' which can be produced by sending a carriage-return
(repeatedly).
So if you don't know if you're in a command which wants input and you
want to abort, keep hitting return until you see the resync.

To bootstrap memory, upload 128 bytes of block data, 16 bytes at a time,
using the 0 through 7 commands.
You can verify what you've uploaded by using the M command.
You can erase a block of flash (other than the bootstrap code) by using
the 'E' command, and then the 'P' command for the block.
Use the 'D' command to dump 256 bytes of flash memory, to verify the
programming correctly.
Bear in mind that the 'nn' argument to the D command is not the same as
the 'bb' argument to the E and P commands.

To upload sixteen bytes to the end of the memory buffer (112->127)
use a command such as:

    790.91.92.93.94.95.96.97.98.99.9A.9B.9C.9D.9E.9F.

The command is 7, followed by 16 hex values, each separated/terminated
by a full stop.
You can verify this has happened, via the 'M' command.

## WARNING/LIMITATIONS:

The bootstrap code has to fit in 512 bytes, which it does with a
handful of bytes to spare on both the ATmega and the tinyAVR.
The `M` command didn't make the cut on the tinyAVR.

The baud rate is a tricky one.
This code does not want to assume that the serial port has been configured.
It first checks that the serial device is enabled - if so, it assumes
everything has been configured correctly (so a board which jumps into
the bootstrap from running firmware keeps its existing baud rate).
Otherwise, it'll configure the stack and the serial port.
It will set a slow baud rate (9600) to minimize errors over the serial
line.
On the ATmega this assumes a 16MHz clock; on the tinyAVR it assumes
the reset-default clock (the 16/20MHz oscillator divided by six, as
per FUSE.OSCCFG) and the default USART0 pins.
