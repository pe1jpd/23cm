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
 * 3.1	new update, incl memories and scan	01-06-16	pe1jpd
 * 3.2	for AO-92, fref=5kHz, menu update	12-02-18	pe1jpd
 * 4.0	support ADF4113 and ADF4153			12-04-18	pe1jpd
 * 4.1	included sequencer oaxrelais PD7	12-04-18	pe1jpd
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
int step, shift;
long int freq, lastFreq;

int tx = FALSE;
int rv = FALSE;
int enc = 0;
char str[16];
int tick;

int tone;
long toneCount;

// Interrupt service routine INT1
ISR(INT1_vect)
{
	if (PIND & (1<<ROT))
 		enc++;
	else 
		enc--;

	_delay_ms(5);
}

// Interrupt service routine Timer1
ISR(TIMER1_OVF_vect) 
{ 
	TCNT1 = 65535-toneCount;		// restart timer
	tick++;
	if (tx && tone>=600)
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
	TCCR1B = 0x01;					// div/1 clock, 1/F_CPU clock
	TIMSK1 |= (1 << TOIE1); 		// Timer1 overflow interrupt 

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
		if (enc>0) count=1;
		else if (enc<0)count=-1;
		enc = 0;
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
		//keep smeter clear
		s = 0;
		// switch from tx to rx??
		if (c & (1<<PTT) ) {
			// sequencer tx off
			switch_tx_off();
			lastFreq = 0;
			tx = FALSE;
		}
	}
	else {
		s = readRSSI();
		displaySmeter(s);

		// switch from rx to tx?
		if (!(c & (1<<PTT) )) {
			// clear smeter
			s = 0;
			displaySmeter(s);
			// sequencer tx on
			switch_tx_on();
			// force update pll
			lastFreq = 0;
			tx = TRUE;
		}
	}

	// calc value for ISR
	toneCount = 5*F_CPU/tone;

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
		lcdCursor(15,0);
		if (tx)
			lcdChar('T');
		else
			lcdChar('R');
	}

	return s;
}

void switch_tx_on()
{
	// mute receiver
	sbi(PORTC, MUTE);
	// switch coax relais to tx
	sbi(PORTD, SEQ);
	//wait a bit
	_delay_ms(200);
	// switch on tx
	sbi(PORTC, TXON);
}

void switch_tx_off()
{
	// switch tx off
	cbi(PORTC, TXON);
	// wait a bit
	_delay_ms(200);
	// switch coax relay to rx
	cbi(PORTD,SEQ);
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

	// PD0-PD6 input with pullup, PD7 output, low
	DDRD = 0x80;
	PORTD = 0x7f;

	lcdInit();
	adcInit();

	readGlobalSettings();
	toneCount = 5*F_CPU/tone;

	initInterrupts();
    initPLL();

	sprintf(str, "PE1JPD 23cm v%s", version);
	lcdCursor(0,0);
	lcdStr(str);
	_delay_ms(500);

	for (;;) {
		switch(mode) {
			case VFO:
				mode = Vfo();
				writeGlobalSettings();
				break;
			case MEMORY:
				mode = Memory();
				writeGlobalSettings();
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
	}
}

#ifdef ADF4113
void initPLL()
{
    long int reg;

    cbi(PORTC, DATA);
    cbi(PORTC, CLK);
    cbi(PORTC, LE);

	// set function latch
	reg = 0x438086;
    setPLL(reg);
    
    // init R-counter
	reg = (2UL<<16) + ((F_REF/25)<<2);
    setPLL(reg);

	unsigned long f = 1170000;
	setFrequency(f);

	reg = 0x438082;
	setPLL(reg);
}

void setFrequency(unsigned long f)
{
	long int reg, N, A, B;
    
	N = f/25;				// counter N in pll
	B = N/16;				// split in A and B
	A = N%16;
    
	reg = ((B & 0x1fff)<<8) + ((A & 0x3f)<<2) + 1;
    setPLL(reg);				// set pll
}
#endif 

#ifdef ADF4153
void initPLL()
{
    unsigned long reg, cntr, R, N, M, F, P1, MUX;

    cbi(PORTC, DATA);
    cbi(PORTC, CLK);
    cbi(PORTC, LE);

	// zero noise&spur registers
	reg = 0x000+3;
	setPLL(reg);

	// set lowest noise
	reg = 0x3c4+3;
	setPLL(reg);

	// reset control reg
	cntr = 0x001046;	//(0x10)+(1<<6)+(1<<2)+2;
	setPLL(cntr);

	// load MUX, R,M
	P1 = 0;
	R = 5;			// Fpfd = 2.6MHz
	M = 2600;
	MUX = 1;		// lock detect
	reg = (MUX<<20)+(P1<<18)+(R<<14)+(M<<2)+1; //0x4168a1
	setPLL(reg);

	// load N,F for F=1170MHz
	N = 450;
	F = 0;
	reg = (N<<14)+(F<<2); //0x708000
	setPLL(reg);

	// enable counter
	cntr = 0x001042;
	setPLL(cntr);
}

void setFrequency(unsigned long f)
{
	long int reg, N, F;

	N = f/2600;
	F = f-2600*N;

    reg = (N<<14)+(F<<2);
    setPLL(reg);
}
#endif

void setPLL(unsigned long r)
{
    int i;
    
    for (i=0; i<24; i++) {
        if (r & 0x800000)
            sbi(PORTC, DATA);
        else
            cbi(PORTC, DATA);
		_delay_us(10);
        sbi(PORTC, CLK);
        _delay_us(10);
        cbi(PORTC, CLK);
        r <<= 1;
    }
	_delay_us(10);
    sbi(PORTC, LE);
    _delay_us(10);
    cbi(PORTC, LE);
}


