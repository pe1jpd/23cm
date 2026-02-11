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

from 13 july 2020 I have started to update the version V4.1 from Bas.
The development environment is AtmelStudio 7.0. For more information please visit www.mdtweb.de.

The first thing what I have done was to implement an LCD with 4 x 20 characters and updated software to use it. Furtheron I have implementet following changes to the software:

 * 4.2  LCD 4x20 ...										                  13-07-20	wm DG8WM
 * 4.2  Frequency Adjust +-999 KHz			    				      14-07-20	wm
 * 4.2  Frequency Adjust +-200 KHz	    						      16-07-20	wm
 * 4.2	Display Squelch, Shift, Step						          16-07-20	wm
 * 4.2	max. Step 1000 KHz									              16-07-20	wm
 * 4.2  New Encoder Handling								              18-07-20	wm
 * 4.2	Display RSSI value in dBm				    			        19-07-20	wm
 * 4.2	"V"FO Problem cleared				    				          20-07-20	wm
 * 4.2	Display reverse	mode		    						          20-07-20	wm
 * 4.2	Added "Hz" to CTCSS	    								          21-07-20	wm
 * 4.2	2-line LCD improved									              28-07-20	wm
 * 4.2	RSSI Display improved								              28-07-20	wm
 * 4.2	Update Parameters in Memory Mode					        30-07-20	wm
 * 4.2	Write only to eeprom, if value has changed			  30-07-20	wm
 * 4.3	Write eeprom, correction (int)shift					      12-08-20	wm
 * 4.3	Correction step down								              12-08-20	wm
 * 4.3	Display in Memory Mode 'CTCSS', VFO Mode 'Step'		12-08-20	wm
 * 4.4  Change SEQ from PD7 to PB2							          17-08-20	wm
 * 4.4	1750 Hz Tone PD7									                17-08-20	wm
 * 4.4	Double click PTT for 1750 Hz tone					        20-08-20	wm
 * 4.5 Start Trx only in VFO or MEMORY Mode    25-08-20 wm
 * 4.51	Some cosmetics, new Headers...						02-10-20	wm
 * 4.52	New version numbering								03-10-20	wm 
 
 '#define LCD_20x4' delete this for 2-line LCD in 23nbfm.h
 '#define PB2_SEQ' for use PB2 and not PD7 for SEQ, PD7 is used for 1750 Hz
 '#define TONE_1750' and '#define PB2_SEQ' for use 1750 Hz on PD7


73 de Werner, DG8WM
