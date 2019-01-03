# 23cm
23cm NBFM transceiver

These files are the sources for my design of the 23cm NBFM transceiver. 
For more information or a complete kit please visit www.pe1jpd.nl.
The development environment is AVR Studio 4 from Atmel.
The processor is an ATMEGA328 with default, factory fuse settings.
The file spectrum.c is linked but commented out in the menu because 1MHz default clock frequency is just too slow to be useful.

Please check the first lines in 23nbfm.h for configuration options. Please note that BOARD1 is only included for some initial prototype boards, normally you should use BOARD2.

Update january 2019:

Via a #define setting in the h-file you can now choose the type of PLL: ADF4113 or ADF4153.
The ADF4113 is no longer manufactured, and the ADF4153 is a modern fractional N PLL with which frequency steps of 1 kHz are possible. The default is however still set on 25 kHz. With the unchanged loop filter the new PLL locks ok, but low frequencies of the modulated audio are less pronounced. This effects also in the CTCSS, which is now much weaker. Some experiments with parts in the loop filter are useful. 

73 de Bas, PE1JPD
