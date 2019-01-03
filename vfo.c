/*
 * loop when in VFO-mode 
 */
#include <avr/io.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "23nbfm.h"
#include <util/delay.h>

void createSmeterChars();
int rxtx();

extern long int freq, lastFreq;
extern int step, shift;
extern int squelchlevel;
extern int tx;
extern int tone;
extern int selectedMemory;
extern int lastSelectedMemory;
extern int tick;
extern int tx;

char str[16];

int Vfo() 
{
	int updn;

	createSmeterChars();

	// get the last freq and settings for mode VFO
	selectedMemory = MAXMEM;
	readMemory(selectedMemory);

	long int savedFreq = freq;
	lastFreq = 0;

	lcdClear();
	lcdCursor(0,0);
	lcdStr("VFO");

	for(;;) {

		// wait for button released
		while (!(PIND & (1<<PUSH))) ;

		// handle internal status of trx
		rxtx();

		// handle encoder 
		int c = handleRotary();
		if (c!=0) {
			freq += c*step;
			// frequency needs to fit raster
			freq = (freq/step)*step;
			// restart timer for inactivity
			tick = 0;
		}
		
		updn = readUpDn();
		if (updn) {
			// up/down always in 1kHz steps
			freq += updn;
			tick = 0;
		}


		// save new vfo frequency in eeprom after ~2 secs inactivity
		if (tick > 200 && freq != savedFreq) {
			writeMemory(selectedMemory);
			savedFreq = freq;
		}

		int push = getRotaryPush();
		if (push) {
			writeMemory(selectedMemory);
			writeGlobalSettings();
			if (push == LONG)
				return MENU;
			else
				return MEMORY;
		}
    }

	// never get here..
	return VFO;
}


int readUpDn()
{
	register unsigned int b;

	// positive pulses!!
	b = (PIND&0x60)>>5;
	if (b==2)
		return 1;		// Up
	else if (b==1)
		return -1;		// Down

	return 0;
}

int Memory()
{
	createSmeterChars();

	selectedMemory = lastSelectedMemory;
	readMemory(selectedMemory);

	lastFreq = 0;

	lcdClear();
	lcdCursor(0,0);
	lcdChar('M');
	lcdChar(0x30 + selectedMemory);

	for(;;) {

		// wait for button released
		while (!(PIND & (1<<PUSH))) ;

		// handle internal status of trx
		rxtx();

		// handle encoder 
		int c = handleRotary();
		if (c!=0) {
			if (c>0) {
				if (++selectedMemory > MAXMEM-1)
					selectedMemory = 0;
			}
			else {
				if (--selectedMemory < 0) 
					selectedMemory = MAXMEM-1;
			}

			// show memory channelno.
			lcdCursor(1,0);
			lcdChar(0x30 + selectedMemory);

			// and load it
			readMemory(selectedMemory);
			tick=0;
		}

		// save current memorychannel in eeprom after ~2 secs inactivity
		if (tick > 200 && selectedMemory != lastSelectedMemory) {
			lastSelectedMemory = selectedMemory;
			writeGlobalSettings();
		}

		int push = getRotaryPush();
		if (push) {
			lastSelectedMemory = selectedMemory;
			writeMemory(selectedMemory);
			writeGlobalSettings();
			if (push == LONG)
				return MEMORY_MENU;
			else
				return VFO;
		}
	}

	// never get here..
	return VFO;
}

void getSquelch()
{
	lcdCursor(9,1);
	sprintf(str, "  %d", squelchlevel);
	lcdStr(str);
}

int setSquelch()
{
	for (;;) {

		lcdCursor(9,1);
		sprintf(str, "> %d ", squelchlevel);
		lcdStr(str);


		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++squelchlevel>64) squelchlevel=64;;
				}
				else {
					if (--squelchlevel<0) squelchlevel = 0;
				}
				break;
			}

			// squelch
			readRSSI();

			int push = getRotaryPush();
			if (push) {
				writeGlobalSettings();
				return push;
			}
		}
	}
}

void getShift()
{
	lcdCursor(9,1);
	sprintf(str, "  %-2d   ", shift);
	lcdStr(str);
}

int setShift()
{
	int sh = shift;

	for (;;) {

		lcdCursor(9,1);
		sprintf(str, "> %-2d   ", sh);
		lcdStr(str);

		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++sh>60) sh = 60;
				}
				else {
					if (--sh<-60) sh = -60;
				}
				break;
			}

			int push = getRotaryPush();
			if (push) {
				shift = sh;
				writeMemory(selectedMemory);
				return push;
			}
		}
	}
}

void getStep()
{
	sprintf(str, "  %d  ", step);
	lcdCursor(9,1);
	lcdStr(str);
}

int setStep()
{
	for (;;) {

		sprintf(str, "> %d  ", step);
		lcdCursor(9,1);
		lcdStr(str);


		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (step==1) step=25;
					else if (step==25) step=500;
					else step=1;
				}
				else {
					if (step==500) step=25;
					else if(step==25) step=1;
					else step=500;
				}
#ifdef ADF4113
				if (step<25) step=25;
#endif
				break;
			}

			int push = getRotaryPush();
			if (push) {
				writeGlobalSettings();
				return push;
			}
		}
	}
}

void getCTCSS()
{
	lcdCursor(9,1);
	if (tone>=600)
		sprintf(str, "  %d.%d", (int)(tone/10), (int)(tone%10));
	else
		sprintf(str, "  off  ");
	lcdStr(str);
}

int setCTCSS()
{
	for (;;) {

		lcdCursor(9,1);
		if (tone<600)
			sprintf(str, "> off  ");
		else
			sprintf(str, "> %d.%d ", (int)(tone/10), (int)(tone%10));
		lcdStr(str);

		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++tone>1500) tone=1500;
				}
				else {
					if (--tone<599) tone=599;
				}
				break;
			}
			int push = getRotaryPush();
			if (push) {
				writeMemory(selectedMemory);
				return push;
			}
		}
	}
}

void getMemory()
{
	selectedMemory = lastSelectedMemory;
	lcdCursor(9,1);
	sprintf(str, "  M%d", selectedMemory);
	lcdStr(str);
}

int setMemory(int set)
{
	int push;

	for (;;) {

		lcdCursor(9,1);
		sprintf(str, "> M%d", selectedMemory);
		lcdStr(str);

		for (;;) {
			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++selectedMemory > MAXMEM-1)
						selectedMemory = 0;
				}
				else {
					if (--selectedMemory < 0) 
						selectedMemory = MAXMEM-1;
				}
				break;
			}
			push = getRotaryPush();
			if (push) {
				lastSelectedMemory = selectedMemory;
				writeMemory(selectedMemory);
				return push;
			}
		}
	}
}

void scanMemory()
{
	lcdClear();

	for (;;) {
		for (selectedMemory=0; selectedMemory<MAXMEM; selectedMemory++) {
			lcdCursor(0,0);
			lcdStr("M   Scanning");
			lcdCursor(1,0);
			lcdChar('0' + selectedMemory);

			readMemory(selectedMemory);
			setFrequency(freq - IF);

			// not too fast
			_delay_ms(200);

			if (tx || getRotaryPush())
				return;

			int s = readRSSI();
			if (s > squelchlevel) {

				// now receiving so show the current frequency
				displayFrequency(freq);

				for (;;) {
					s = rxtx(); 
					if (s > squelchlevel)
						tick = 0;
					if (tx || getRotaryPush())
						return;
					if (tick > 200)
						break;
				}
			}
		}
	}
}


struct MenuStruct {
    char *name;
	void (* get)();
	int (* set)();
};

#define MAXMENU 4
struct MenuStruct mainMenu[] = {
    { "Squelch ", &getSquelch, &setSquelch},
    { "Step    ", &getStep, &setStep},
    { "Shift   ", &getShift, &setShift},
    { "CTCSS   ", &getCTCSS, &setCTCSS},
	{ "Store   ", &getMemory, &setMemory},
//	{ "Spectrum scan ", 0, &Spectrum},
};

#define MAXMEMMENU 3
struct MenuStruct memoryMenu[] = {
    { "Squelch ", &getSquelch, &setSquelch},
    { "Shift   ", &getShift, &setShift},
    { "CTCSS   ", &getCTCSS, &setCTCSS},
    { "Memory scan   ", NULL, &scanMemory},
};

int Menu()
{
	struct MenuStruct *menu = mainMenu;
	int i=0;

	for (;;) {

		lcdCursor(0,1);
		lcdStr("                ");
		lcdCursor(0,1);
		lcdStr(menu[i].name);

		if (menu[i].get) menu[i].get();
	
		// wait for button released
		while (!(PIND & (1<<PUSH))) ;

		for (;;) {
			// don't forget to squelch..
			readRSSI();

			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++i>MAXMENU) i=0;
				}
				else {
					if (--i<0) i=MAXMENU;
				}
				break;
			}

			int push = getRotaryPush();
			if (push) {
				if (push == SHORT) {
					push = menu[i].set();
				}
				if (push == LONG) {
					// when memory stored, activate that memory
					if (i==4)
						return MEMORY;
					else
						return VFO;
				}
				break;
			}
		}
	}

	return VFO;
}

int MemoryMenu()
{
	struct MenuStruct *menu = memoryMenu;

	int i=0;

	for (;;) {	
		lcdCursor(0,1);
		lcdStr("                ");
		lcdCursor(0,1);
		lcdStr(menu[i].name);
		if (menu[i].get) menu[i].get();
	
		// wait for button released
		while (!(PIND & (1<<PUSH))) ;

		for (;;) {
			// don't forget to squelch
			readRSSI();

			// handle encoder 
			int c = handleRotary();
			if (c!=0) {
				if (c>0) {
					if (++i>MAXMEMMENU) i=0;
				}
				else {
					if (--i<0) i=MAXMEMMENU;
				}
				break;
			}

			int push = getRotaryPush();
			if (push) {
				if (push == SHORT) {
					push = menu[i].set();
				}
				if (push == LONG)
					return MEMORY;
				break;
			}
		}
	}

	return MEMORY;
}

