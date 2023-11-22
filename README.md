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
jumping to address 0xe000.

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
| M | Dump the memory buffer |
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

As of now, this code is very hard-wired to the ATMega328p, and occupies
around 460 bytes of code.
It needs to be made more portable...

The baud rate is a tricky one.
This code does not want to assume that the serial port has been configured.
It first checks that the serial device is enabled - if so, it assumes
everything has been configured correctly.
Otherwise, it'll configure the stack and the serial port.
It will set a slow baud rate (9600) to minimize errors over the serial
line.
