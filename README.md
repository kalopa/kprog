# avrprog

Programmer code for AVR chip using the libavr bootstrap module.
This module is designed to work with the *libavr/bootstrap.S* code
to program an AVR chip, in-situ.
The **libavr** bootstrap code is designed to run in a measly 512
words of code space at the top of AVR memory, and provide the
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
