#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

extern int tx;
extern int squelchlevel;

// define S-meter chars
unsigned char smeterChars[3][8] = {
	{0b00000,0b00000,0b10000,0b10000,0b10000,0b10000,0b00000,0b00000},
	{0b00000,0b00000,0b10100,0b10100,0b10100,0b10100,0b00000,0b00000},
	{0b00000,0b00000,0b10101,0b10101,0b10101,0b10101,0b00000,0b00000}
};

void createSmeterChars()
{
	// define custom chars for s-meter
	int i, j;

	lcdCmd(0x40);
	for (i=0; i<3; i++) {
		for (j=0; j<8; j++) {
			lcdData(smeterChars[i][j]);
		}
	}
}

int readRSSI()
{
	register int rssi;

	ADCSRA |= (1<<ADSC)|(1<<ADEN); 
	while ((ADCSRA & (1<<ADSC))!=0);

	// calculate Smeter
	rssi = (1024-ADC)-44;
	if (rssi<0) rssi=0;

	// mute when tx or squelched
	if (tx || rssi<squelchlevel)
		sbi(PORTC, MUTE);
	else
		cbi(PORTC, MUTE);

	return rssi;
}

void displaySmeter(int rssi) 
{
	short n = 16;
	int s = rssi;

	// goto second line on lcd
	lcdCursor(0,1);

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
	while (n-->0) lcdData(' ');
}

