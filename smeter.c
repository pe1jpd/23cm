/*
*	Project: 23cm-NBFM-Transceiver
*	Developer: Bas, PE1JPD
*
*	Module: smeter.c
*	Last change: 02.10.20
*
*	Description: S-meter and RSSI
*/


#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

extern int tx;
extern int squelchlevel;
extern int mode;


// define S-meter chars
unsigned char smeterChars[4][8] = {										// wm, [3][8]
	{0b00000,0b00000,0b10000,0b10000,0b10000,0b10000,0b00000,0b00000},
	{0b00000,0b00000,0b10100,0b10100,0b10100,0b10100,0b00000,0b00000},
	{0b00000,0b00000,0b10101,0b10101,0b10101,0b10101,0b00000,0b00000},
	{0b00000,0b00000,0b00100,0b00100,0b00100,0b01110,0b00100,0b00000}	// wm
};


void createSmeterChars()
{
	// define custom chars for s-meter
	int i, j;

	lcdCmd(0x40);
	for (i=0; i<4; i++) {											// wm, i<3
		for (j=0; j<8; j++) {
			lcdData(smeterChars[i][j]);
		}
	}
}


int readRSSI()
{
	register int rssi;
	int rssi_; 

	ADCSRA |= (1<<ADSC)|(1<<ADEN); 

	while ((ADCSRA & (1<<ADSC))!=0);								// wait for result of conversion

	// calculate Smeter
	rssi = (1024-ADC);												// -44, RSSIoff moves RSSI
	// if (rssi<0) rssi=0;											// wm moved
	
	// mute when tx or squelched
	rssi_ = rssi-RSSIoff;											// wm
	if (rssi_<0) rssi_ = 0;											// wm
	if (tx || rssi_<squelchlevel || mode==SPECTRUM) {				// wm
		sbi(PORTC, MUTE);
	}
	else {
		cbi(PORTC, MUTE);

//		analog S output on Timer1, not implemented yet!				// wm not used anymore
//		OCR1B = rssi;
	}

	return rssi;
}


void displaySmeter(int rssi) 
{
	short n = 16;
	int s = rssi-RSSIoff;											// wm
	
	if (s<0) s=0;													// wm

	#ifdef LCD_20x4													// wm
		// goto fourth line on lcd
		lcdCursor(0,3);
		lcdStr("S-M:");
	#else
		// goto second line on lcd
		lcdCursor(0,1);
	#endif

	// chars in the full bar are 3 vertical lines
	while(s>=3 && n>0) {
		lcdData(2);
		s -= 3;
		n--;
	}
	
	// last char 0, 1 or 2 lines
	switch (s) {
		case 2: 
			lcdData(1);
			break;
		case 1:
			lcdData(0);
			break;
		default:
			lcdData(' ');
			break;
	}

	// clear any chars to the right (when tx, clear all)
	n--;															// wm, Line overflow to 0,0!
	while (n-->0) lcdData(' ');
}


void displayRSSI(int rssi) 											// wm
{
	int s = rssi-44;												// wm, RSSIoff, "rssi-44" in dBm calibrated for original value
	char str[20];
	
	if (s<0) s=0;													// wm
	
	lcdCursor(0, 2);
	
	if (s==0) {
		lcdStr("RSSI:               ");
	}
	else
	{
		s = 9835 - (173 * s);										// all values*100 --> Integer
		
		sprintf(str, "RSSI: -%d.%d dBm", (int)(s/100), (int)(s%100));
		lcdCursor(0,2);
		lcdStr(str);
	}
}