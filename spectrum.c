/*
 * Spectrum scanner/analyzer
 */

#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

extern long int freq;

// internal function prototypes
void createGraphCharacters();
int avgRSSI();

// define S-meter chars
unsigned char spectrumChars[8][8] = {
	{0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B11111},
	{0B00000,0B00000,0B00000,0B00000,0B00000,0B00000,0B11111,0B11111},
	{0B00000,0B00000,0B00000,0B00000,0B00000,0B11111,0B11111,0B11111},
	{0B00000,0B00000,0B00000,0B00000,0B11111,0B11111,0B11111,0B11111},
	{0B00000,0B00000,0B00000,0B11111,0B11111,0B11111,0B11111,0B11111},
	{0B00000,0B00000,0B11111,0B11111,0B11111,0B11111,0B11111,0B11111},
	{0B00000,0B11111,0B00000,0B11111,0B11111,0B11111,0B11111,0B11111},
	{0B11111,0B11111,0B11111,0B11111,0B11111,0B11111,0B11111,0B11111}
};

/* Maps values (the index) to characters (the value) */
unsigned char graphCharacterMap[9] =  { ' ', 0, 1, 2, 3, 4, 5, 6, 7 };
long int scanWidth = 5000000;


int Spectrum() {
	int x;

	lcdClear();
	createGraphCharacters();
  
	long int startFreq = 1292000; //freq - scanWidth/2;
	long int endFreq   = 1300000; //freq + scanWidth/2;
	long int step = (endFreq - startFreq)/16;
  
	// mute the rx during spectrum display
	sbi(PORTC, MUTE);

	for (;;) {
		for (x=0; x<16; x++) {

    	  	int rssi = avgRSSI(startFreq+x*step, startFreq+x*step+step);

			if (getRotaryPush())
				return VFO;

			// map rssi 48 bars horizontal to 16 vertical
			unsigned char y = rssi/3;

			lcdCursor(x,0);
			if (y>8) 
				lcdChar((char)graphCharacterMap[min(8,y-8)]);
			else 
				lcdChar(' ');

			lcdCursor(x,1);
			lcdChar((char)graphCharacterMap[min(8,y)]);
		}	
	}
}

int avgRSSI(long int start, long int end)
{
	int rssi=0;
	long int f;

	for (f=start; f<end; f+=F_RASTER) {

		setFrequency(f - IF);
		if (getRotaryPush()) return TRUE;

		_delay_ms(10);
		rssi = max(rssi, readRSSI());
	}

	if (rssi < 0) rssi = 0;
	return rssi;
}


void createGraphCharacters()
{
	int i, j;

	// define custom chars for spectrum
	lcdCmd(0x40);
	for (i=0; i<8; i++) {
		for (j=0; j<8; j++) {
			lcdData(spectrumChars[i][j]);
		}
	}
}
