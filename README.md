# kprog - An AVR Programmer using the serial port

The
[libavr](https://github.com/kalopa/libavr)
library for AVR chips includes a bootstrap code segment and a simple
routine for programming the chip via the serial port.
The intent of that routine was to write an extremely simple and
efficient boot loader in the smallest memory footprint available.
It currently weighs in at 460 bytes.
This code is used to take an AVR HEX file, as produced by avr-gcc,
and program the board using the serial port.
You still need to use some sort of ICE programmer to start with.
Both to get the bootstrap code into position and to set the fuses.
