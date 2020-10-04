/*
*	Project: 23cm-NBFM-Transceiver
*	Developer: Bas, PE1JPD
*
*	Module: main.c
*	Last change: 03.10.20
*
*	Description: main loop
*/


/* 
 * #	changes												date		by
 * ---------------------------------------------------------------------------
 * 1.0  Initial version										01-01-15	pe1jpd
 * 1.5	Software squelch									26-05-15	pe1jpd
 * 2.0	PORTC change										01-07-15	pe1jpd
 * 2.1	PORTC bug solved									19-08-15	pe1jpd
 * 2.2	fref saved/restored from eeprom						02-10-15	pe1jpd
 * 3.1	New update, incl memories and scan					01-06-16	pe1jpd
 * 3.2	For AO-92, fref=5kHz, menu update					12-02-18	pe1jpd
 * 4.0	Support ADF4113 and ADF4153							12-04-18	pe1jpd
 * 4.1	Included sequencer koaxrelais PD7					12-04-18	pe1jpd
 * 4.2  LCD 4x20 ...										13-07-20	wm DG8WM
 * 4.2  Frequency Adjust +-999 KHz							14-07-20	wm
 * 4.2  Frequency Adjust +-200 KHz							16-07-20	wm
 * 4.2	Display Squelch, Shift, Step						16-07-20	wm
 * 4.2	max. Step 1000 KHz									16-07-20	wm
 * 4.2  New Encoder Handling								18-07-20	wm
 * 4.2	Display RSSI value in dBm							19-07-20	wm
 * 4.2	"V"FO Problem cleared								20-07-20	wm
 * 4.2	Display reverse	mode								20-07-20	wm
 * 4.2	Added "Hz" to CTCSS									21-07-20	wm
 * 4.2	2-line LCD improved									28-07-20	wm
 * 4.2	RSSI Display improved								28-07-20	wm
 * 4.2	Update Parameters in Memory Mode					30-07-20	wm
 * 4.2	Write only to eeprom, if value has changed			30-07-20	wm
 * 4.3	Write eeprom, correction (int)shift					12-08-20	wm
 * 4.3	Correction step down								12-08-20	wm
 * 4.3	Display in Memory Mode 'CTCSS', VFO Mode 'Step'		12-08-20	wm
 * 4.4  Change SEQ from PD7 to PB2							17-08-20	wm
 * 4.4	1750 Hz Tone PD7									17-08-20	wm
 * 4.4	Double click PTT for 1750 Hz tone					20-08-20	wm
 * 4.5	Start Trx only in VFO or MEMORY Mode				25-08-20	wm
 * 4.51	Some cosmetics, new Headers...						02-10-20	wm
 * 4.52	New version numbering								03-10-20	wm
 */


/*
 *
 *		'#define LCD_20x4' delete this for 2-line LCD in 23nbfm.h						wm
 *		'#define PB2_SEQ' for use PB2 and not PD7 for SEQ, PD7 is used for 1750 Hz		wm
 *		'#define TONE_1750' and '#define PB2_SEQ' 1750 Hz on PD7 						wm
 *
 */

#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>												// wm
#include <avr/pgmspace.h>											// wm
#include "23nbfm.h"
#include <util/delay.h>

typedef unsigned char u08;

// variables
int squelchlevel;
int mode;
int selectedMemory;

int lastSelectedMemory;
int step, shift;

#ifdef ADF4153														// wm
	int frqadj;
#endif

long int freq, lastFreq;

int tx = FALSE;
int rv = FALSE;
int enc = 0;
char str[20];														// wm str[16];
int tick;

int tone;															// CTCSS-Tone
long toneCount;														// var for Timer1

#ifdef DECODER														// wm
	volatile int8_t enc_delta;										// Drehgeberbewegung zwischen zwei Auslesungen im Hauptprogramm
#endif

#ifdef DECODER														// wm
	// Dekodertabelle für wackeligen Rastpunkt
	// Quelle: https://www.mikrocontroller.net/articles/Drehgeber
	
	// viertel Auflösung											// wm
	const int8_t table[16] PROGMEM = {0,0,-1,0,0,0,0,1,0,0,0,0,0,0,0,0};

	// halbe Auflösung
	// const int8_t table[16] PROGMEM = {0,0,-1,0,0,0,0,1,1,0,0,0,0,-1,0,0};
		
	// Dekodertabelle für normale Drehgeber
	// volle Auflösung
	// const int8_t table[16] PROGMEM = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};

	ISR( TIMER0_COMPA_vect )										// 1ms fuer manuelle Eingabe
	{
		static int8_t last=0;										// alten Wert speichern

		last = (last << 2)  & 0x0F;
		if (PHASE_A) last |=2;
		if (PHASE_B) last |=1;
		enc_delta += pgm_read_byte(&table[last]);
	}

	void encode_init( void )										// Timer 0 initialisieren
	{
		TIFR0 |= (1<<OCF0A);										// Clear Interrupt Request
		TIMSK0 |= 1<<OCIE0A;										// Enable Output Compare A Interrupt
		
//		OCR0A = (uint8_t)(XTAL / 8.0 * 1e-3 - 0.5);					// 1ms ==> XTAL/8 * 1e-3 - 0.5 = 124						<<<<<
		OCR0A = 125;												// 1ms Compare Time
		TCNT0 = 0;
		TCCR0A |= 1 << WGM01;										// CTC Mode 2
		TCCR0B |= 1 << CS01;										// Prescale XTAL / 8, Timer0 Start
	}
#else
	// Interrupt service routine INT1
	ISR(INT1_vect)
	{
		if (PIND & (1<<ROT))
 			enc++;
		else 
			enc--;

		_delay_ms(5);
	}
#endif


// Interrupt service routine Timer1
ISR(TIMER1_OVF_vect)				// CTCSS-Ton
{ 
	TCNT1 = 65535-toneCount;		// restart timer 1
	tick++;
	if (tx && tone>=600)
		tbi(PORTB, Beep);			// toggle Beep port	
} 

#ifdef TONE_1750													// wm
	// Interrupt service routine Timer2
	ISR(TIMER2_OVF_vect)											// 1750 Hz
	{
		if (tx && tone==599)
		{
			TCNT2 = 225;											// restart Timer 2, 1743 Hz
			tbi(PORTD, CT);											// toggle CT
		}
	}
#endif

void initInterrupts(void)
{
	#ifdef DECODER													// wm
		encode_init();
	#else
		EIMSK |= _BV(INT1);			// enable INT1
		EICRA |= _BV(ISC11);		// int1 on falling edge
	#endif

	// Setup Timer 1				// CTCSS
	TCCR1A = 0x00;					// Normal Mode 
	TCCR1B = 0x01;					// div/1 clock, F_CPU clock/1
	TIMSK1 |= (1 << TOIE1); 		// Timer1 overflow interrupt 

	#ifdef TONE_1750				// wm
		//Setup Timer 2				// 1750 Hz
		TCCR2A = 0X00;				// Normal Mode
		TCCR2B = 0x00;				// (F_CPU clock/8), stop Timer 2
		TIMSK2 |= (1 << TOIE2);		// Timer2 overflow interrupt 
	#endif

	// enable interrupts
	sei();
}


void adcInit(void)
{
	ADCSRA = (1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);  // prescaler /128
	#ifdef BOARD2
	DIDR0 = (1<<ADC5D); 			// disable digital input cannel 5
	ADMUX = (1<<REFS0) + 5;			// ADC channel 5, ARef = Vcc
	#endif
	#ifdef BOARD1
	DIDR0 = (1<<ADC0D); 			// disable digital input
	ADMUX = (1<<REFS0) + 1;			// ADC channel 1
	#endif
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


#ifdef DECODER														//wm
	int handleRotary( void )         // Encoder auslesen
	{
		int8_t val = 0;

		// atomarer Variablenzugriff
		cli();
		
		if (enc_delta != 0) {
			if (enc_delta > 0) val = 1;
			else if (enc_delta < 0) val = -1; 
		 
			enc_delta = 0;
		}
		
		sei();
		return val;
	}
#else
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
#endif


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


void displayParameter()												// wm
{
	lcdCursor(0,1);
	
	if (selectedMemory==MAXMEM)										// VFO Mode
	{
		sprintf(str, "Sq %2d Sh %3d St %4d", squelchlevel, shift, step);
	}
	else															// Memory Mode
	{
		if (tone<599)
		{
			sprintf(str, "Sq %2d Sh %3d To  off", squelchlevel, shift);
		}
		else if (tone==599)
		{
			sprintf(str, "Sq %2d Sh %3d To 1750", squelchlevel, shift);
		}
		else
		{
			sprintf(str, "Sq %2d Sh %3d To%3d.%1d", squelchlevel, shift, (int)(tone/10), (int)(tone%10));
		}
	}
	lcdStr(str);		
}


int TX_ok()												// wm
{
	// clear smeter
	int s = 0;
	
	#ifdef LCD_20x4										// wm Clear RSSI
		displayRSSI(s);
	#endif
	
	displaySmeter(s);									// Clear S-Meter
	// sequencer tx on
	switch_tx_on();
	// force update pll
	lastFreq = 0;
	tx = TRUE;
	
	return s;
}


int rxtx()
{
	int s;
	int t_1750;														// wm 

	// read ptt on PORTD
	int c = PIND;

	// listen reverse?
	if (rv) {
		if (c & (1<<REVERSE)) {
			lastFreq = 0;
			rv = FALSE;

			
			lcdCursor(3,0);
			lcdData(' ');
		}
	}
	else {
		if (!(c & (1<<REVERSE))) {
			lastFreq = 0;
			rv = TRUE;
			
			lcdCursor(3,0);
			lcdData(3);
		}
	}

	if (tx) {
		//keep smeter clear
		s = 0;
		// switch from tx to rx??									// PTT high switch Tx off
		if (c & (1<<PTT) ) 
		{
			#ifdef TONE_1750
				TCCR2B = 0x00;										// (F_CPU clock/8), stop Timer 2
				cbi(PORTD, CT);										// PB07 low
			#endif
			
			// sequencer tx off
			switch_tx_off();
			lastFreq = 0;
			tx = FALSE;
		}
	}
	else {
		s = readRSSI();
		
		#ifdef LCD_20x4												// wm
			displayRSSI(s);
		#endif
		
		displaySmeter(s);

		// switch from rx to tx?
		if (!(c & (1<<PTT)))								 			// PTT low switch Tx on 
		{
			#ifdef TONE_1750											// wm
				if (tone == 599)										// 1750 Hz is selected
				{
					tick = 0;											// reset counter, 1 tick  ~ 8 ms
					t_1750 = 0;
					
					do
					{
						_delay_ms(1);									// must be here
						if (PIND & (1<<PTT))							// PTT high
						{
							while ((tick < 20) && (PIND & (1<<PTT)));	// wait for PTT low or time out
						
							if (!(PIND & (1<<PTT)))
							{
								// switch on 1750 Hz
								TCCR2B = (1 << CS21);					// start Timer 2, F_CPU/8
								t_1750 = 1;								// PTT low --> 1750 Hz Tone
							}
							else
								t_1750 = -1;							// time out PTT high --> break
						}
						
						if (t_1750 != 0)
							break;
					}
					while ((tick < 20) && !(tx));
				
					if (t_1750 > -1)
						s = TX_ok();									// time out PTT low --> without 1750 Hz Tone
				}
				else			
					s = TX_ok();										// 1750 Hz not selected PTT low
			#else	
				s = TX_ok();
			#endif
		}
	}

	// calc value for ISR
	toneCount = 5*F_CPU/tone;										// CTCSS 

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
		
		#ifdef LCD_20x4												// wm
			displayParameter();
			lcdCursor(18,0);
			
			if (tx)
				lcdStr("Tx");
			else
				lcdStr("Rx");
		#else
			lcdCursor(15,0);
			if (tx)
				lcdChar('T');
			else
				lcdChar('R');
		#endif
	}

	return s;
}


void switch_tx_on()
{
	// mute receiver
	sbi(PORTC, MUTE);

	// switch coax relais to tx
	#ifdef PB2_SEQ													// wm
		sbi(PORTB, SEQ);
	#else
		sbi(PORTD, SEQ);
	#endif
	
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
	#ifdef PB2_SEQ													// wm
		cbi(PORTB, SEQ);
	#else
		cbi(PORTD, SEQ);
	#endif
}


void Test1()														// wm, normaly not used
{
	tbi(PORTB, SEQ);
	_delay_ms(5);
	tbi(PORTB, SEQ);	
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

	N = (f+frqadj)/2600;											// wm
	F = (f+frqadj)-2600*N;											// wm

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



//=============================================================================
int main()
{
	// PORTB output for LCD
	DDRB = 0xff;
	
	#ifdef PB2_SEQ													// wm
		PORTB = 0xfb;												// PB2 for SEQ output low
	#else
		PORTB = 0xff;
	#endif

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
	// wm, PD7 used for 1750 Hz tone --> '#define PB2_SEQ' and '#define TONE_1750' 
	DDRD = 0x80;
	PORTD = 0x7f;

	lcdInit();
	adcInit();

	readGlobalSettings();
	
	if ((mode != VFO) && (mode != MEMORY))							// wm
	{
		mode = VFO;
	}
	
	toneCount = 5*F_CPU/tone;										// CTCSS, tick

	initInterrupts();
	initPLL();

	#ifdef LCD_20x4													// wm
		sprintf(str, "23cm-NBFM-Trx  v%s", version);	
		lcdCursor(0,0);
		lcdStr(str);

		lcdCursor(0,1);
		lcdStr("      by PE1JPD     ");
		_delay_ms(2000);
		
		lcdCursor(0,1);
		lcdStr("  and improved by ");
		
		lcdCursor(0,2);
		lcdStr("       DG8WM      ");
	#else
		sprintf(str, "23cm-Trx   v%s", version);					// wm
		lcdCursor(0,0);
		lcdStr(str);

		lcdCursor(0,1);												// wm
		lcdStr("   by PE1JPD    ");
		_delay_ms(2000);
		
		lcdCursor(0,1);
		lcdStr("Improv. by DG8WM");
	#endif
	
	_delay_ms(2000);												// wm 1000

	#ifdef LCD_20x4
		lcdCursor(0,2);
		lcdStr("              ");
	#endif

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

