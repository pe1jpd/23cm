# 23cm
23cm NBFM transceiver

Update 17 july 2019
Uploaded v4.1, with the following changes:
- added support for the fractional N pll ADF4153, selectable in the .h file
- added support for a sequencer on PD7 for swiching an external coaxrelais in case of higer power with a PA
- some minor changes over time

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


Hi all,

from 13 july 2020 I have started to update the version V4.1 from Bas. The first thing what I have done was to implement an LCD with 4 x 20 characters and update the software to use it.

73 de Werner, DG8WM
