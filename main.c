/*
 * 23cm JPD transceiver
 * 
 * #	change								date		by
 * ------------------------------------------------------------
 * 1.0	initial version						01-01-15	pe1jpd
 * 1.5	software squelch					26-05-15	pe1jpd
 * 2.0	PORTC change						01-07-15	pe1jpd
 * 2.1	PORTC bug solved					19-08-15	pe1jpd
 * 2.2	fref saved/restored from eeprom		02-10-15	pe1jpd
 * 3.0	new update, incl memories and scan	01-06-16	pe1jpd
 */

#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

typedef unsigned char u08;

// variables
int squelchlevel;
int mode;
int selectedMemory;

int lastSelectedMemory;
int shift;
long int freq, lastFreq;

int tx = FALSE;
int rv = FALSE;
int enc = 0;
char str[16];
int tick;

int tone;
long int toneCount;

// Interrupt service routine INT1
ISR(INT1_vect)
{
	if (PIND & (1<<ROT))
 		enc++;
	else 
		enc--;
	_delay_ms(10);
}

// Interrupt service routine Timer1
ISR(TIMER1_OVF_vect) 
{ 
	tick++;							// increment tick for freq save timeout
	TCNT1 = 65536-toneCount;		// restart timer

	if (tx && tone>669)
		tbi(PORTB, Beep);			// toggle Beep port
} 

void adcInit(void) 
{ 
	ADCSRA = (1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);  // prescaler /128
#ifdef BOARD2
	DIDR0 = (1<<ADC5D); 			// disable digital input
	ADMUX = (1<<REFS0) + 5;			// ADC channel 5 
#endif
#ifdef BOARD1
	DIDR0 = (1<<ADC0D); 			// disable digital input
	ADMUX = (1<<REFS0) + 1;			// ADC channel 1 
#endif
} 

void initInterrupts(void)
{
	EIMSK |= _BV(INT1);				// enable INT1
	EICRA |= _BV(ISC11);			// int1 on falling edge

	// Setup Timer 1
	TCCR1A = 0x00;					// Normal Mode 
	TCCR1B |= (1<<CS10);			// div/1 clock, 1/F_CPU clock

	// setup PWM on OC1B
//	TCCR1A |= (1<<OCR1B1)|(1<<OCR1B0);

	// Enable interrupts as needed 
	TIMSK1 |= _BV(TOIE1);  			// Timer 1 overflow interrupt 

	// enable interrupts
	sei();
}

void hex2bcd(long int f)
{
	long int x = 10000000;
	int i;
	char d;

	for (i=0; i<8; i++) {
		d = 0;
		for (;;) {
			f -= x;
			if (f < 0) break;
			d++;
		}
		str[i] = d + 0x30;
		f += x;
		x = x/10;
	}
}

void displayFrequency(long int f)
{
	int i;
	char c;

	hex2bcd(f);

	lcdCursor(4,0);

	for (i=1; i<8; i++) {
		c = str[i];
		if (i==5) {
			lcdChar('.');
		}
		lcdChar(c);
	}
}

int getRotaryPush()
{
	int t, c = PIND;

	// rotary button pushed?
	if (!(c & (1<<PUSH))) {
		for (t=0; t<5; t++) {
			_delay_ms(100);
			// button released?
			if ((PIND & (1<<PUSH)))
				break;
		}
		if (t>3)
			return LONG;
		else
			return SHORT;
	}

	return FALSE;
}

int handleRotary()
{
	int count=0;

	if (enc != 0) {
		if (enc>0) {
			enc--; count++;
		}
		if (enc<0) {
			enc++; count--;
		}
	}
	return count;
}

int rxtx()
{
	int s;

	// read ptt on PORTD
	int c = PIND;

	// listen reverse?
	if (rv) {
		if (c & (1<<REVERSE)) {
			lastFreq = 0;
			rv = FALSE;
		}
	}
	else {
		if (!(c & (1<<REVERSE))) {
			lastFreq = 0;
			rv = TRUE;
		}
	}

	if (tx) {
		s = 0;
		// switch from tx to rx??
		if (c & (1<<PTT) ) {
			tx = FALSE;
			cbi(PORTC, TXON);
			lcdCursor(15,0);
			lcdChar('R');
			lastFreq = 0;
		}
	}
	else {
		s = readRSSI();
		// switch from rx to tx?
		if (!(c & (1<<PTT) )) {
			s = 0;
			tx = TRUE;
			sbi(PORTC, TXON);
			lcdCursor(15,0);
			lcdChar('T');
			lastFreq = 0;
		}
	}

	// freq change or update needed?
	if (freq != lastFreq) {
		long int f = freq;
		if (tx) {
			f += (long int) shift*1000;
			setFrequency(f);
		}
		else {
			if (rv) f += (long int)shift*1000;
			setFrequency(f - IF);
		}
		displayFrequency(f);
		lastFreq = freq;
	}

	displaySmeter(s);
	return s;
}

int main()
{
	// PORTB output for LCD
	DDRB = 0xff;
	PORTB = 0xff;

#ifdef BOARD2
	// PORTC PC0-4 output, PC5 input
	DDRC = 0x1f;
	PORTC = 0x00;
	sbi(PORTC, MUTE);
#endif
#ifdef BOARD1
	// PORTC PC0,2-5 output, PC1 input
	DDRC = 0x3d;
	PORTC = 0x00;
	sbi(PORTC, MUTE);
#endif

	// PORTD is input with pullup
	DDRD = 0x00;
	PORTD = 0xff;

	lcdInit();
	adcInit();

	readGlobalSettings();

	initInterrupts();
    initPLL(freq - IF);

	sprintf(str, "PE1JPD 23cm v%s", version);
	lcdCursor(0,0);
	lcdStr(str);
	_delay_ms(500);

	for (;;) {
		switch(mode) {
			case VFO:
				mode = Vfo();
				break;
			case MEMORY:
				mode = Memory();
				break;
			case SPECTRUM:
				mode = Spectrum();
				break;
			case MENU:
				mode = Menu(mode);
				break;
			case MEMORY_MENU:
				mode = MemoryMenu(mode);
				break;
			default:
				mode = VFO;
				break;
		}

    	// When switching to VFO or MEMORY mode,
		// store squelchlevel, mode and selectedMemory
		switch(mode) {
			case VFO:      
			case MEMORY:
			writeGlobalSettings();
		}
	}
}

void initPLL(long int f)
{
    long int reg;

    cbi(PORTC, DATA);
    cbi(PORTC, CLK);
    cbi(PORTC, LE);

	// set function latch
	reg = 0x438086;
    setPLL(reg);
    
    // init R-counter
	reg = (2UL<<16) + ((F_REF/F_RASTER)<<2);
    setPLL(reg);

	setFrequency(f);

	reg = 0x438082;
	setPLL(reg);
}

void setFrequency(long int f)
{
	long int reg, frast, A, B;
    
	frast = f/F_RASTER;
	B = frast/16;
	A = frast%16;
    
	reg = ((B & 0x1fff)<<8) + ((A & 0x3f)<<2) + 1;
    
    setPLL(reg);
}

void setPLL(long int r)
{
    int i;
    
    for (i=0; i<24; i++) {
        if (r & 0x800000)
            sbi(PORTC, DATA);
        else
            cbi(PORTC, DATA);
		_delay_us(1);
        sbi(PORTC, CLK);
        _delay_us(1);
        cbi(PORTC, CLK);
        r <<= 1;
    }
	_delay_us(1);
    sbi(PORTC, LE);
    _delay_us(1);
    cbi(PORTC, LE);
}


